/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id: idv.cpp 18910 2006-11-13 01:56:34Z shsuh $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idv.h>
#include <idp.h>
#include <idu.h>
#include <idvProfile.h>

iduMutex idvProfile::mFileMutex;  // open/flush/close/flag change
iduMutex idvProfile::mHeaderMutex; // buf copy + header offset move
iduMutex idvProfile::mTailMutex;  // write + tail offset move

UInt     idvProfile::mProfFlag;
UInt     idvProfile::mBufSize;
UInt     idvProfile::mBufFlushSize;
UInt     idvProfile::mBufSkipFlag;
UInt     idvProfile::mFileSize;
SChar    idvProfile::mFileName[MAXPATHLEN];
SChar   *idvProfile::mQueryProfLogDir; /* BUG-36806 */

UChar   *idvProfile::mBufArray;
UInt     idvProfile::mHeaderOffset;
UInt     idvProfile::mHeaderOffsetForWriting; /* BUG-40123 */
UInt     idvProfile::mTailOffset;
UInt     idvProfile::mFileNumber;
idBool   idvProfile::mIsWriting;   /* I/O doing now? */
ULong    idvProfile::mCurrFileSize;
PDL_HANDLE    idvProfile::mFP;

/* ------------------------------------------------
 *  idvProfile
 * ----------------------------------------------*/

IDE_RC idvProfile::initialize()
{
    IDE_ASSERT(mFileMutex.initialize((SChar *)"QUERY_PROFILE_FILE_MUTEX",
                                     IDU_MUTEX_KIND_NATIVE,
                                     IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);

    IDE_ASSERT(mHeaderMutex.initialize((SChar *)"QUERY_PROFILE_HEADER_MUTEX",
                                       IDU_MUTEX_KIND_NATIVE,
                                       IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);

    IDE_ASSERT(mTailMutex.initialize((SChar *)"QUERY_PROFILE_TAIL_MUTEX",
                                     IDU_MUTEX_KIND_NATIVE,
                                     IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);

    mProfFlag        = 0;
    mBufSize         = 0;
    mBufFlushSize    = 0;
    mBufSkipFlag     = 0;
    mFileSize        = 0;
    mQueryProfLogDir = NULL;

    mBufArray     = NULL;
    mHeaderOffset = 0;
    mTailOffset   = 0;
    mFP           = PDL_INVALID_HANDLE;
    mFileNumber   = 0;
    idlOS::memset(mFileName, 0, ID_SIZEOF(mFileName));

    // property pre setting
    IDE_TEST(setBufSize(iduProperty::getQueryProfBufSize()) != IDE_SUCCESS);
    IDE_TEST(setBufFlushSize(iduProperty::getQueryProfBufFlushSize()) != IDE_SUCCESS);
    IDE_TEST(setBufFullSkipFlag(iduProperty::getQueryProfBufFullSkip()) != IDE_SUCCESS);
    IDE_TEST(setFileSize(iduProperty::getQueryProfFileSize()) != IDE_SUCCESS);
    /* BUG-36806 */
    IDE_TEST(setLogDir(iduProperty::getQueryProfLogDir()) != IDE_SUCCESS);

    // activate if > 0
    IDE_TEST(setProfFlag(iduProperty::getQueryProfFlag()) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC idvProfile::destroy()
{
    IDE_TEST(setProfFlag(0) != IDE_SUCCESS);

    IDE_ASSERT(mFileMutex.destroy() == IDE_SUCCESS);
    IDE_ASSERT(mHeaderMutex.destroy() == IDE_SUCCESS);
    IDE_ASSERT(mTailMutex.destroy() == IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// hold file mutex state!!
IDE_RC idvProfile::openFile()
{
    UInt  sClock;

    idlOS::umask(0);
    sClock = idlOS::time(NULL); // second

    /* BUG-36806 */
    idlOS::snprintf(mFileName, ID_SIZEOF(mFileName),
                    "%s%salti-%"ID_UINT32_FMT"-%"ID_UINT32_FMT".prof",
                    mQueryProfLogDir,
                    IDL_FILE_SEPARATORS,
                    sClock,
                    mFileNumber++);

    mFP = idlOS::open(mFileName, O_CREAT | O_WRONLY | O_APPEND, 0600);

    IDE_TEST_RAISE(mFP == PDL_INVALID_HANDLE, fopen_error);

    mCurrFileSize = 0;

    return IDE_SUCCESS;
    IDE_EXCEPTION(fopen_error);
    {
        // BUG-21760 profiling file open의 실패는 ABORT로 충분하다.
        IDE_SET(ideSetErrorCode(idERR_ABORT_FILE_OPEN, mFileName));
        mFP = PDL_INVALID_HANDLE;
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// hold file mutex state!!
IDE_RC idvProfile::closeFile()
{
    IDE_TEST_RAISE(idlOS::close(mFP) != 0, fclose_error);

    mFP = PDL_INVALID_HANDLE;

    return IDE_SUCCESS;
    IDE_EXCEPTION(fclose_error);
    {
        // BUG-21760 profiling file close의 실패는 ABORT로 충분하다.
        IDE_SET(ideSetErrorCode(idERR_ABORT_FILE_CLOSE));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Action from property
 *
 *  FileMutex와 Header/TailMutex를 함께 잡아야 한다.
 *  성능상의 잇점을 얻기 위해
 *  빈도가 높은 버퍼에 기록하는 놈은 HeaderMutex만을 잡고,
 *  빈도가 높은 화일에 기록하는 놈은
 *  TailMutex만을 잡고 수행하고,
 *
 *  상대적으로 빈도가 낮은 프로퍼티 변경하는 놈은
 *  File과 Tail Mutex, Header를 모두 잡고 수행한다.
 *
 * ----------------------------------------------*/


IDE_RC idvProfile::setProfFlag(UInt aFlag)
{
    UInt    sState       = 0;
    idBool  sNeedUnlock  = ID_FALSE;

    /* 
     * BUG-43563   [mm] profile에서 flag를 잘못입력해도 에러가 발생하지 않음. 
     */
    IDE_TEST_RAISE( aFlag > IDV_PROF_TYPE_MAX_FLAG, wrong_flag );

    lockAllMutexForProperty();
    sNeedUnlock = ID_TRUE;

    if (mProfFlag == 0) // deactive state
    {
        if (aFlag > 0) // active!!
        {
            IDE_TEST(openBuffer() != IDE_SUCCESS);
            
            sState = 1;
            IDE_TEST(openFile() != IDE_SUCCESS);

            sState = 0;
            mProfFlag = aFlag;
        }
        else
        {
            // nothing
        }
    }
    else // acitve state
    {
        IDE_TEST_RAISE(aFlag > 0, state_error);

        sState = 2;
        mHeaderOffsetForWriting = mHeaderOffset;

        // BUG-42741 turn off profiling whether 
        // write succeeds or not.
        (void)writeBufferToDiskInternal();

        // BUG-21760 file 삭제가 실패하더라도 profiling은
        // 끌 수 있어야 한다.
        (void) closeFile();
        
        (void) closeBuffer();
        
        sState = 0;
        mProfFlag   = aFlag; // should be zero!
        mFileNumber = 0;     // reset file number
    }

    sNeedUnlock = ID_FALSE;
    unlockAllMutexForProperty();

    return IDE_SUCCESS;

    IDE_EXCEPTION(state_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Invalid_Profile_State));
    }
    IDE_EXCEPTION(wrong_flag);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_WRONG_Profile_Flag,
                                IDV_PROF_TYPE_MAX_FLAG));
    }
    IDE_EXCEPTION_END;
    
    switch(sState)
    {
        case 2:
            (void) closeFile();
        case 1:
            (void) closeBuffer();
        default:
            break;
    };

    if ( sNeedUnlock == ID_TRUE )
    {
        unlockAllMutexForProperty();
    }
    
    return IDE_FAILURE;
}

IDE_RC idvProfile::setBufSize(UInt aSize)
{
    lockAllMutexForProperty();

    IDE_TEST_RAISE(mProfFlag > 0, state_error);

    mBufSize = aSize;

    unlockAllMutexForProperty();

    return IDE_SUCCESS;

    IDE_EXCEPTION(state_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Invalid_Profile_State));
    }
    IDE_EXCEPTION_END;
    unlockAllMutexForProperty();
    return IDE_FAILURE;
}


IDE_RC idvProfile::setBufFlushSize(UInt aSize)
{
    lockAllMutexForProperty();

    IDE_TEST_RAISE(mProfFlag > 0, state_error);

    mBufFlushSize = aSize;

    unlockAllMutexForProperty();

    return IDE_SUCCESS;

    IDE_EXCEPTION(state_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Invalid_Profile_State));
    }
    IDE_EXCEPTION_END;
    unlockAllMutexForProperty();
    return IDE_FAILURE;
}

IDE_RC idvProfile::setBufFullSkipFlag(UInt aFlag)
{
    lockAllMutexForProperty();

    IDE_TEST_RAISE(mProfFlag > 0, state_error);

    mBufSkipFlag = aFlag;

    unlockAllMutexForProperty();
    return IDE_SUCCESS;

    IDE_EXCEPTION(state_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Invalid_Profile_State));
    }
    IDE_EXCEPTION_END;
    unlockAllMutexForProperty();
    return IDE_FAILURE;
}

IDE_RC idvProfile::setFileSize(UInt aSize)
{
    lockAllMutexForProperty();

    IDE_TEST_RAISE(mProfFlag > 0, state_error);

    mFileSize = aSize;

    unlockAllMutexForProperty();

    return IDE_SUCCESS;

    IDE_EXCEPTION(state_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Invalid_Profile_State));
    }
    IDE_EXCEPTION_END;
    unlockAllMutexForProperty();
    return IDE_FAILURE;
}

/* BUG-36806 */
IDE_RC idvProfile::setLogDir( SChar *aQueryProfLogDir )
{
    lockAllMutexForProperty();

    IDE_TEST_RAISE(mProfFlag > 0, state_error);

    mQueryProfLogDir = aQueryProfLogDir;

    unlockAllMutexForProperty();

    return IDE_SUCCESS;

    IDE_EXCEPTION(state_error);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Invalid_Profile_State));
    }
    IDE_EXCEPTION_END;
    unlockAllMutexForProperty();
    return IDE_FAILURE;
}

/* ------------------------------------------------
 *  Buffer Control
 * ----------------------------------------------*/

IDE_RC idvProfile::openBuffer()
{
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_ID_PROFILE_MANAGER,
                                mBufSize,
                                (void **)&mBufArray) != IDE_SUCCESS);

    mHeaderOffset = 0;
    mTailOffset   = 0;
    mIsWriting    = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

IDE_RC idvProfile::closeBuffer()
{
    // BUG-42741 Do not try to free mBufArray when it is NULL
    if(mBufArray != NULL)
    {
        IDE_TEST(iduMemMgr::free( (void *)mBufArray) != IDE_SUCCESS);
        mBufArray = NULL;
    }
    else
    {
        /* Do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    mBufArray = NULL;

    return IDE_FAILURE;

}

/* ------------------------------------------------
 *   CORE COPY FUNCTION!!!!!
 * ----------------------------------------------*/

void idvProfile::copyToBufferInternalOneUnit(void *aSrc, UInt aLength)
{
    if ( (mHeaderOffset + aLength) > mBufSize)
    {
        UInt sPieceSize1;
        UInt sPieceSize2;

        sPieceSize1 = mBufSize - mHeaderOffset;
        sPieceSize2 = aLength -  sPieceSize1;

        idlOS::memcpy(mBufArray + mHeaderOffset, aSrc, sPieceSize1);
        idlOS::memcpy(mBufArray, (UChar *)aSrc + sPieceSize1, sPieceSize2);
    }
    else // just copy ok
    {
        idlOS::memcpy(mBufArray + mHeaderOffset, aSrc, aLength);
    }

    increaseHeader(aLength);

}


void idvProfile::copyToBufferInternal(idvProfDescInfo *aInfo)
{
    for (;aInfo->mData != NULL; aInfo++)
    {
        copyToBufferInternalOneUnit(aInfo->mData, aInfo->mLength);
    }
}

UInt   idvProfile::getBufFree()
{
    /* ------------------------------------------------
     *  1) normal order
     *
     *       T                 H-->
     *       |                 |
     *   +---+*****************+--------------+
     *   |                N                   |
     *
     *   remain(*)= N - (H - T)
     *
     *
     *  2) reverse order
     *
     *       H-->              T
     *       |                 |
     *   +***+------------------**************+
     *   |                N                   |
     *
     *   remain(*)= T - H
     *
     * ----------------------------------------------*/

    if (mHeaderOffset >= mTailOffset) // normal order
    {
        return mBufSize - (mHeaderOffset - mTailOffset);
    }
    else // reverse order
    {
        return (mTailOffset - mHeaderOffset);
    }
}

UInt   idvProfile::getBufUsed()
{
    return mBufSize - getBufFree();
}

idBool idvProfile::isEnoughSpace(UInt aToCopyDataLen)
{
    if (getBufFree() > aToCopyDataLen)
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

idBool idvProfile::isWriteNeed()
{
    if (getBufUsed() >= mBufFlushSize)
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

void   idvProfile::increaseHeader(UInt aCopiedSize)
{
    mHeaderOffset += aCopiedSize;

    if (mHeaderOffset >  mBufSize)
    {
        mHeaderOffset -= mBufSize;
    }
}

void   idvProfile::increaseTail  (UInt aWrittenSize)
{
    mTailOffset += aWrittenSize;

    if (mTailOffset >  mBufSize)
    {
        mTailOffset -= mBufSize;
    }
}

/* ------------------------------------------------
 *
 *  1. normal order 일 경우 그냥 쓰기
 *  2. reverse order일 경우에는
 *     Tail 부터 끝까지.
 *     처음부터 Header까지.
 * ----------------------------------------------*/

IDE_RC idvProfile::writeBufferToDisk()
{
    UInt sState = 0;

    IDE_ASSERT(lockTailMtx() == IDE_SUCCESS);
    sState = 1;

    if (mProfFlag > 0)
    {
        IDE_TEST(writeBufferToDiskInternal() != IDE_SUCCESS);

        if ( (mFileSize > 0) && (mCurrFileSize > mFileSize) )
        {
            IDE_ASSERT(lockFileMtx() == IDE_SUCCESS);
            sState = 2;
            IDE_TEST(closeFile() != IDE_SUCCESS);

            IDE_TEST(openFile() != IDE_SUCCESS);

            IDE_ASSERT(unlockFileMtx() == IDE_SUCCESS);
            sState = 1;
        }
    }

    IDE_ASSERT(unlockTailMtx() == IDE_SUCCESS);
    sState = 0;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 2:
            IDE_ASSERT(unlockFileMtx() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(unlockTailMtx() == IDE_SUCCESS);
        case 0:
            break;
        default:
            IDE_ASSERT(0 == 1);
    }

    return IDE_FAILURE;
}

IDE_RC idvProfile::writeBufferToDiskInternal()
{
    /* ------------------------------------------------
     * 반드시 임시변수로 복사한 이후에 사용해야 한다.
     * 왜냐하면, header offset의 경우에는 mutex를 잡지
     * 않기 때문에 여러번 접근시 그 값이 계속 변하기 때문에
     * 문제가 생길수 있다. 복사한 이후 그 값을 이용하여
     * 기록해야 한다.
     * ----------------------------------------------*/
    UInt sHeader, sTail, sUsed;

    /* BUG-40123 A file split of a query profiling is not correct. */
    sHeader = mHeaderOffsetForWriting;
    sTail   = mTailOffset;
    sUsed   = sHeader - sTail;

    if (sHeader == sTail)
    {
        // no data to write
        return IDE_SUCCESS;
    }

    /* BUG-39152 If an open trace log file has been accidentally removed by user, 
     * Audit and Profile do not write the monitored data. */
    if ( idlOS::access( mFileName, R_OK | W_OK | F_OK) < 0 )
    {
        IDE_TEST(closeFile() != IDE_SUCCESS);
        IDE_TEST(openFile() != IDE_SUCCESS);
    }
    else
    {
        /* do nothing */
    }
    
    if (sHeader > sTail) // normal order
    {
        if (idlOS::write(mFP, mBufArray + sTail, sUsed) != (ssize_t)sUsed)
        {
            ideLog::log(IDE_SERVER_0, "[ERROR] Query Profile "
                        "write(size=%u) errno=%u\n",
                        sUsed, errno);
        }

        /* 
         * BUG-40123 A file split of a query profiling is not correct.
         *
         * idlOS::write의 리턴값 말고 sUsed를 사용한다. 최종적으로 나온 prof 파일의
         * 크기가 QUERY_PROF_FILE_SIZE와 많이 차이가 난다면  write()가 실패한 경우라고
         * 볼 수 있다.
         */
        mCurrFileSize += sUsed;
    }
    else // reverse order
    {
        UInt sUsedPiece;

        sUsedPiece = mBufSize - sTail;

        if (idlOS::write(mFP, mBufArray + sTail, sUsedPiece) != (ssize_t)sUsedPiece)
        {
            ideLog::log(IDE_SERVER_0, "[ERROR] Query Profile "
                        "fwrite(size=%u) errno=%u\n",
                        sUsed, errno);
        }

        mCurrFileSize += sUsedPiece;

        sUsedPiece = sHeader;

        if (idlOS::write(mFP, mBufArray, sUsedPiece) != (ssize_t)sUsedPiece)
        {
            ideLog::log(IDE_SERVER_0, "[ERROR] Query Profile "
                        "fwrite(size=%u) errno=%u\n",
                        sUsedPiece, errno);
        }

        mCurrFileSize += sUsedPiece;
    }

    increaseTail(sUsed);
    IDL_MEM_BARRIER;
    mIsWriting = ID_FALSE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC idvProfile::copyToBuffer(idvProfDescInfo *aInfo)
{
    UInt sTotalSize;

    sTotalSize = ((idvProfHeader *)(aInfo->mData))->mSize;

  retry: ;

    IDE_ASSERT(lockHeaderMtx() == IDE_SUCCESS);

    if (mProfFlag == 0)
    {
        IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);
        return IDE_SUCCESS;
    }

    if (isEnoughSpace(sTotalSize) == ID_TRUE)
    {
        copyToBufferInternal(aInfo);
        if (isWriteNeed() == ID_TRUE)
        {
            if (mIsWriting == ID_TRUE)
            {
                IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);
            }
            else
            {
                // I have to write to file, because nobody try to write.
                mIsWriting = ID_TRUE;
                mHeaderOffsetForWriting = mHeaderOffset;
                IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);

                writeBufferToDisk();
            }
        }
        else
        {
            // copied & no write need
            IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);
        }
    }
    else
    {
        // 공간이 부족함.
        if (mBufSkipFlag == ID_TRUE)
        {
            // just skip.
            IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);
        }
        else
        {
            if (mIsWriting == ID_TRUE)
            {
                IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);

                while(mIsWriting == ID_TRUE)
                {
                    PDL_Time_Value sSleep;

                    sSleep.initialize(0, 1000);

                    idlOS::sleep(sSleep);
                }
                goto retry;
            }
            else
            {
                mIsWriting = ID_TRUE;
                mHeaderOffsetForWriting = mHeaderOffset;
                IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);

                writeBufferToDisk();
            }
        }
    }

    return IDE_SUCCESS;
}



/* ------------------------------------------------
 * Write Function
 * ----------------------------------------------*/
IDE_RC idvProfile::writeSessSyssInternal(idvProfType mType, idvSession *aSess)
{
    idvProfDescInfo  sDescInfo[3];
    idvProfHeader    sHeader;

    // data filled
    sHeader.mSize = ID_SIZEOF(idvProfHeader) + ID_SIZEOF(idvSession);
    sHeader.mType = mType;
    sHeader.mTime = idvManager::getSecond();

    // descriptor filled
    sDescInfo[0].mData   = &sHeader;
    sDescInfo[0].mLength = ID_SIZEOF(idvProfHeader);

    sDescInfo[1].mData   = aSess;
    sDescInfo[1].mLength = ID_SIZEOF(idvSession);

    sDescInfo[2].mData   = NULL;

    copyToBuffer(&sDescInfo[0]);

    return IDE_SUCCESS;
}


IDE_RC idvProfile::writeMemInfoInternal()
{
    idvProfDescInfo  sDescInfo[3];
    idvProfHeader    sHeader;

    // data filled
    sHeader.mSize = ID_SIZEOF(idvProfHeader) + ID_SIZEOF(iduMemMgr::mClientInfo);
    sHeader.mType = IDV_PROF_TYPE_MEMS;
    sHeader.mTime = idvManager::getSecond();

    // descriptor filled
    sDescInfo[0].mData   = &sHeader;
    sDescInfo[0].mLength = ID_SIZEOF(idvProfHeader);

    sDescInfo[1].mData   = iduMemMgr::mClientInfo;
    sDescInfo[1].mLength = ID_SIZEOF(iduMemMgr::mClientInfo);

    sDescInfo[2].mData   = NULL;

    copyToBuffer(&sDescInfo[0]);

    return IDE_SUCCESS;
}


IDE_RC idvProfile::writeStatementInternal(idvSQL         *aStmt,
                                         idvProfStmtInfo *aInfo,
                                         SChar           *aQueryString,
                                         UInt             aQueryLen)
{
    idvProfDescInfo  sDescInfo[6];
    idvProfHeader    sHeader;

    // data filled
    sHeader.mType = IDV_PROF_TYPE_STMT;
    sHeader.mTime = idvManager::getSecond();
    sHeader.mSize = ID_SIZEOF(idvProfHeader) +
                    ID_SIZEOF(idvProfStmtInfo) +
                    ID_SIZEOF(idvSQL) +
                    ID_SIZEOF(UInt) + aQueryLen; /* length info + real length */

    // descriptor filled
    sDescInfo[0].mData   = &sHeader;
    sDescInfo[0].mLength = ID_SIZEOF(idvProfHeader);

    sDescInfo[1].mData   = aInfo;
    sDescInfo[1].mLength = ID_SIZEOF(idvProfStmtInfo);

    sDescInfo[2].mData   = aStmt;
    sDescInfo[2].mLength = ID_SIZEOF(idvSQL);

    sDescInfo[3].mData   = &aQueryLen;
    sDescInfo[3].mLength = ID_SIZEOF(UInt);

    sDescInfo[4].mData   = aQueryString;
    sDescInfo[4].mLength = aQueryLen;

    sDescInfo[5].mData   = NULL;

    copyToBuffer(&sDescInfo[0]);

    return IDE_SUCCESS;
}

IDE_RC idvProfile::writeBindInternalA5(void            *aData,
                                     UInt             aSID,
                                     UInt             aSSID,
                                     UInt             aTxID,
                                     idvWriteCallbackA5 aCallback)
{
    idvProfDescInfo  sDescInfo[256];
    idvProfHeader    sHeader;
    idvProfStmtInfo  sStmtInfo;

    // data filled
    sHeader.mType = IDV_PROF_TYPE_BIND_A5;
    sHeader.mTime = idvManager::getSecond();
    sHeader.mSize = 0; // Not Determined : set in callback

    sStmtInfo.mSID  =  aSID;
    sStmtInfo.mSSID =  aSSID;
    sStmtInfo.mTxID =  aTxID;

    // descriptor filled
    sDescInfo[0].mData   = &sHeader;
    sDescInfo[0].mLength = ID_SIZEOF(idvProfHeader);

    sDescInfo[1].mData   = &sStmtInfo;
    sDescInfo[1].mLength = ID_SIZEOF(idvProfStmtInfo);

    sHeader.mSize = aCallback(aData,
                              &sDescInfo[0],
                              2,
                              256,
                              ID_SIZEOF(idvProfHeader) +
                              ID_SIZEOF(idvProfStmtInfo));

    copyToBuffer(&sDescInfo[0]);

    return IDE_SUCCESS;
}

// proj_2160 cm_type removal
// this function is used for A7 or higher.
// 2 new arguments added: aType, aOffset
IDE_RC idvProfile::writeBindInternal(idvProfBind     *aData,
                                     UInt             aSID,
                                     UInt             aSSID,
                                     UInt             aTxID,
                                     idvWriteCallback aCallback)
{
    idvProfDescInfo  sDescInfo[256];
    idvProfHeader    sHeader;
    idvProfStmtInfo  sStmtInfo;

    // data filled
    sHeader.mType = IDV_PROF_TYPE_BIND; // new constant for A7
    sHeader.mTime = idvManager::getSecond();
    sHeader.mSize = 0; // Not Determined : set in callback

    sStmtInfo.mSID  =  aSID;
    sStmtInfo.mSSID =  aSSID;
    sStmtInfo.mTxID =  aTxID;

    // descriptor filled
    sDescInfo[0].mData   = &sHeader;
    sDescInfo[0].mLength = ID_SIZEOF(idvProfHeader);

    sDescInfo[1].mData   = &sStmtInfo;
    sDescInfo[1].mLength = ID_SIZEOF(idvProfStmtInfo);

    sHeader.mSize = aCallback(aData, &sDescInfo[0], 2,
                              ID_SIZEOF(idvProfHeader) +
                              ID_SIZEOF(idvProfStmtInfo));

    copyToBuffer(&sDescInfo[0]);

    return IDE_SUCCESS;
}

IDE_RC idvProfile::writePlanInternal(UInt          aSID,
                                     UInt          aSSID,
                                     UInt          aTxID,
                                     iduVarString *aPlanString)
{
    SChar              sErrorMsg[] = "Error In Get Plan Tree";
    idvProfDescInfo    sDescInfo[256];
    idvProfHeader      sHeader;
    idvProfStmtInfo    sStmtInfo;
    iduListNode       *sIterator;
    iduVarStringPiece *sPiece;
    UInt               sPlanLen;
    UInt               sIndex;

    // data filled
    sHeader.mType = IDV_PROF_TYPE_PLAN;
    sHeader.mTime = idvManager::getSecond();
    sHeader.mSize = ID_SIZEOF(idvProfHeader) +
                    ID_SIZEOF(idvProfStmtInfo) +
                    ID_SIZEOF(UInt);

    sStmtInfo.mSID  =  aSID;
    sStmtInfo.mSSID =  aSSID;
    sStmtInfo.mTxID =  aTxID;

    // descriptor filled
    sDescInfo[0].mData   = &sHeader;
    sDescInfo[0].mLength = ID_SIZEOF(idvProfHeader);

    sDescInfo[1].mData   = &sStmtInfo;
    sDescInfo[1].mLength = ID_SIZEOF(idvProfStmtInfo);

    sDescInfo[2].mData   = &sPlanLen;
    sDescInfo[2].mLength = ID_SIZEOF(UInt);

    sPlanLen = 0;
    sIndex   = 3;

    if (aPlanString != NULL)
    {
        IDU_LIST_ITERATE(&aPlanString->mPieceList, sIterator)
        {
            sPiece = (iduVarStringPiece *)sIterator->mObj;

            sDescInfo[sIndex].mData   = sPiece->mData;
            sDescInfo[sIndex].mLength = sPiece->mLength;
            sPlanLen                 += sPiece->mLength;

            sIndex++;

            if (sIndex == 255)
            {
                break;
            }
        }
    }
    else
    {
        sDescInfo[sIndex].mData   = (void *)sErrorMsg;
        sDescInfo[sIndex].mLength = idlOS::strlen(sErrorMsg);
        sPlanLen                 += idlOS::strlen(sErrorMsg);

        sIndex++;
    }

    sDescInfo[sIndex].mData = NULL;

    sHeader.mSize += sPlanLen;

    copyToBuffer(&sDescInfo[0]);

    return IDE_SUCCESS;
}

IDE_RC idvProfile::flushAllBufferToFile()
{
    /* BUG-40123 A file split of a query profiling is not correct. */
    IDE_ASSERT(lockHeaderMtx() == IDE_SUCCESS);
    mHeaderOffsetForWriting = mHeaderOffset;
    IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);

    IDE_TEST(writeBufferToDisk() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}



/* ------------------------------------------------
 *  Whole Mutex Operation for Property
 * ----------------------------------------------*/

void   idvProfile::lockAllMutexForProperty()
{
    IDE_ASSERT(lockTailMtx() == IDE_SUCCESS);
    IDE_ASSERT(lockHeaderMtx() == IDE_SUCCESS);
    IDE_ASSERT(lockFileMtx() == IDE_SUCCESS);
}

void   idvProfile::unlockAllMutexForProperty()
{
    IDE_ASSERT(unlockFileMtx() == IDE_SUCCESS);
    IDE_ASSERT(unlockHeaderMtx() == IDE_SUCCESS);
    IDE_ASSERT(unlockTailMtx() == IDE_SUCCESS);
}


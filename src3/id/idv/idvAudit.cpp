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
 * $Id$
 **********************************************************************/
#include <idl.h>
#include <idv.h>
#include <idvAudit.h>

#ifdef ALTI_CFG_OS_LINUX
#include <syslog.h>
#endif

iduMutex idvAudit::mFileMutex;
iduMutex idvAudit::mHeaderMutex;
iduMutex idvAudit::mTailMutex;

UInt   idvAudit::mBufferSize;
UInt   idvAudit::mBufferFlushSize;
UInt   idvAudit::mBufferSkipFlag;
UInt   idvAudit::mFileSize;
SChar *idvAudit::mAuditLogDir;

UChar      *idvAudit::mBufferArray;
UInt        idvAudit::mHeaderOffset;
UInt        idvAudit::mHeaderOffsetForWriting; /* BUG-40123 */
UInt        idvAudit::mTailOffset;
PDL_HANDLE  idvAudit::mFP;
SChar       idvAudit::mFileName[MAXPATHLEN];
UInt        idvAudit::mFileNumber;
idBool      idvAudit::mIsWriting; 
ULong       idvAudit::mCurrFileSize;

UInt        idvAudit::mOutputMethod;


IDE_RC idvAudit::initialize()
{
    UInt    sOutputProperty;
    /* BUG-39760 Enable AltiAudit to write log into syslog */
    mOutputMethod = IDV_AUDIT_OUTPUT_NORMAL;

#ifdef ALTI_CFG_OS_LINUX
    sOutputProperty = iduProperty::getAuditOutputMethod();
    if( (IDV_AUDIT_OUTPUT_SYSLOG <= sOutputProperty)  &&
        (sOutputProperty <=IDV_AUDIT_OUTPUT_SYSLOG_LOCAL7) )
    {
        mOutputMethod = sOutputProperty;

    }
    else
    {
        /* do nothing */
    }

#endif

    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        /* Initialize mutexes */
        IDE_ASSERT(mFileMutex.initialize((SChar *)"QUERY_AUDIT_FILE_MUTEX",
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);

        IDE_ASSERT(mHeaderMutex.initialize((SChar *)"QUERY_AUDIT_HEADER_MUTEX",
                                           IDU_MUTEX_KIND_NATIVE,
                                           IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);

        IDE_ASSERT(mTailMutex.initialize((SChar *)"QUERY_AUDIT_TAIL_MUTEX",
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL) == IDE_SUCCESS);
        
        /* initialize member variables */
        mBufferSize      = 0;
        mBufferFlushSize = 0;
        mBufferSkipFlag  = 0;
        mFileSize        = 0;
        mAuditLogDir     = NULL;

        mBufferArray     = NULL;
        mHeaderOffset    = 0;
        mFP              = PDL_INVALID_HANDLE;
        mFileNumber      = 0;
        idlOS::memset(mFileName, 0, ID_SIZEOF(mFileName));

        /* properties */
        IDE_TEST( setBufferSize( iduProperty::getAuditBufferSize() ) 
                  != IDE_SUCCESS);
        IDE_TEST( setBufferFlushSize( iduProperty::getAuditBufferFlushSize() ) 
                  != IDE_SUCCESS );
        IDE_TEST( setBufferFullSkipFlag( iduProperty::getAuditBufferFullSkip() ) 
                  != IDE_SUCCESS );
        IDE_TEST( setFileSize( iduProperty::getAuditFileSize() ) 
                  != IDE_SUCCESS );
        /* BUG-36807 */
        IDE_TEST( setAuditLogDir( iduProperty::getAuditLogDir() ) 
                  != IDE_SUCCESS );

        openBuffer();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idvAudit::destroy()
{
    /* BUG-39760 Enable AltiAudit to write log into syslog */
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        IDE_ASSERT( mFileMutex.destroy()   == IDE_SUCCESS );
        IDE_ASSERT( mHeaderMutex.destroy() == IDE_SUCCESS );
        IDE_ASSERT( mTailMutex.destroy()   == IDE_SUCCESS );

        closeBuffer();
        closeFile();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

/* write functions */
IDE_RC idvAudit::writeAuditEntriesInternal( idvAuditTrail *aAuditTrail,
                                            SChar         *aQuery, 
                                            UInt           aQueryLen )
{
    idvAuditHeader    sHeader;
    idvAuditDescInfo  sDescInfo[6];

    IDE_DASSERT( aQuery[aQueryLen] == '\0' );

    IDE_DASSERT( (mOutputMethod < IDV_AUDIT_OUTPUT_MAX) ); //just for sure

    /* BUG-39760 Enable AltiAudit to write log into syslog */
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL ) 
    {
        /* header */
        sHeader.mTime = idvManager::getSecond();
        /* 
        sHeader.mSize = ID_SIZEOF(idvAuditHeader) +
                        ID_SIZEOF(idvAuditTrail)  +
                        aQueryLen + 1;  // null 포함
        */
        sDescInfo[0].mData   = &sHeader;
        sDescInfo[0].mLength = ID_SIZEOF(idvAuditHeader);
        sHeader.mSize = sDescInfo[0].mLength;

        sDescInfo[1].mData   = aAuditTrail;
        sDescInfo[1].mLength = ID_SIZEOF(idvAuditTrail);
        sHeader.mSize += sDescInfo[1].mLength;

        sDescInfo[2].mData   = aQuery;
        sDescInfo[2].mLength = aQueryLen + 1;  // null 포함
        
        /* Add some padding bytes to avoid SIGBUG 
         * caused by the violation of address alignment */
        sDescInfo[2].mLength = idlOS::align8(sDescInfo[2].mLength);

        sHeader.mSize += sDescInfo[2].mLength;

        sDescInfo[3].mData   = NULL;

        copyToBuffer( &sDescInfo[0] );

    }
#ifdef ALTI_CFG_OS_LINUX
    else if( mOutputMethod < IDV_AUDIT_OUTPUT_MAX ) 
    {
        SInt sPriority = LOG_INFO;

        switch( mOutputMethod )
        {
            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL0:
                sPriority |= LOG_LOCAL0;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL1:
                sPriority |= LOG_LOCAL1;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL2:
                sPriority |= LOG_LOCAL2;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL3:
                sPriority |= LOG_LOCAL3;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL4:
                sPriority |= LOG_LOCAL4;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL5:
                sPriority |= LOG_LOCAL5;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL6:
                sPriority |= LOG_LOCAL6;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG_LOCAL7:
                sPriority |= LOG_LOCAL7;
                break;

            case IDV_AUDIT_OUTPUT_SYSLOG:
            default:
                sPriority |= LOG_USER; 
                break;
        }

        //we don't have to be concerned of time. syslog() prints the time.
        ::syslog( sPriority,
                  "[%s]"
                  "%s,%d,%s,%s,%s,%s,%d,"
                  "%d,%d,%d,%d,%lu,%lu,%lu,%lu,%d,"
                  "%lu,%lu,%lu,%lu,%lu,%lu,%lu,"
                  "\"%s\""
                  "\n",
                  iduProperty::getAuditTagNameInSyslog(),
                  aAuditTrail->mLoginID,
                  aAuditTrail->mSessionID,
                  aAuditTrail->mLoginIP,
                  aAuditTrail->mClientType,
                  aAuditTrail->mClientAppInfo,
                  aAuditTrail->mAction,
                  aAuditTrail->mAutoCommitFlag,
                  aAuditTrail->mStatementID,
                  aAuditTrail->mTransactionID,
                  aAuditTrail->mExecutionFlag,
                  aAuditTrail->mFetchFlag,
                  aAuditTrail->mExecuteSuccessCount,
                  aAuditTrail->mExecuteFailureCount,
                  aAuditTrail->mProcessRow,
                  aAuditTrail->mUseMemory,
                  aAuditTrail->mXaSessionFlag,
                  aAuditTrail->mTotalTime,
                  aAuditTrail->mSoftPrepareTime,
                  aAuditTrail->mParseTime,
                  aAuditTrail->mValidateTime,
                  aAuditTrail->mOptimizeTime,
                  aAuditTrail->mExecuteTime,
                  aAuditTrail->mFetchTime,
                  aQuery );
    }
#endif
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC idvAudit::flushAllBufferToFile()
{
    /* BUG-39760 Enable AltiAudit to write log into syslog */
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        /* BUG-40123 A file split of a query profiling is not correct. */
        IDE_ASSERT(lockHeaderMutex() == IDE_SUCCESS);
        mHeaderOffsetForWriting = mHeaderOffset;
        IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);

        IDE_TEST( writeBufferToDisk() != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* buffer */
IDE_RC idvAudit::copyToBuffer( idvAuditDescInfo *aInfo )
{
    UInt sTotalSize; 

    sTotalSize = ((idvAuditHeader *)(aInfo->mData))->mSize;

retry:
    IDE_ASSERT( lockHeaderMutex() == IDE_SUCCESS );

    if( isEnoughSpace(sTotalSize) == ID_TRUE )
    {
        copyToBufferInternal( aInfo );
        if (isWriteNeed() == ID_TRUE)
        {
            if (mIsWriting == ID_TRUE)
            {
                IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);
            }
            else
            {
                mIsWriting = ID_TRUE;
                mHeaderOffsetForWriting = mHeaderOffset;
                IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);
                
                writeBufferToDisk();
               
            }
        }
        else
        {
            IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);
        }
    }
    else
    {
        if (mBufferSkipFlag == ID_TRUE)
        {
            IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);
        }
        else
        {
            if (mIsWriting == ID_TRUE)
            {
                IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);

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
                IDE_ASSERT(unlockHeaderMutex() == IDE_SUCCESS);

                writeBufferToDisk();
            }
        }
    }

    return IDE_SUCCESS;
}

void idvAudit::copyToBufferInternal( idvAuditDescInfo *aInfo )
{
    for (;aInfo->mData != NULL; aInfo++)
    {
        copyToBufferInternalOneUnit(aInfo->mData, aInfo->mLength);
    }
}


void idvAudit::copyToBufferInternalOneUnit( void *aSrc, UInt aLength )
{
    UInt sAlignedLength = idlOS::align8(aLength);

    if ( (mHeaderOffset + sAlignedLength) > mBufferSize)
    {   
        UInt sPieceSize1;
        UInt sPieceSize2;

        sPieceSize1 = mBufferSize - mHeaderOffset;
        sPieceSize2 = aLength -  sPieceSize1;

        idlOS::memcpy(mBufferArray + mHeaderOffset, aSrc, sPieceSize1);
        idlOS::memcpy(mBufferArray, (UChar *)aSrc + sPieceSize1, sPieceSize2);
    }   
    else 
    {   
        idlOS::memcpy(mBufferArray + mHeaderOffset, aSrc, aLength);
    }   

    increaseHeader(sAlignedLength);

}

void idvAudit::increaseHeader( UInt aCopiedSize )
{
    mHeaderOffset += aCopiedSize;

    if (mHeaderOffset >=  mBufferSize)
    {
        mHeaderOffset -= mBufferSize;
    }
}




IDE_RC idvAudit::openBuffer()
{
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_ID_AUDIT_MANAGER,
                                mBufferSize,
                                (void **)&mBufferArray) != IDE_SUCCESS);

    mHeaderOffset = 0;
    mTailOffset   = 0;
    mIsWriting    = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC idvAudit::closeBuffer()
{
    IDE_TEST(iduMemMgr::free( (void *)mBufferArray) != IDE_SUCCESS);

    mBufferArray = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}



IDE_RC idvAudit::writeBufferToDiskInternal()
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
        if (idlOS::write(mFP, mBufferArray + sTail, sUsed) != (ssize_t)sUsed)
        {
            ideLog::log(IDE_SERVER_0, "[ERROR] Auditing "
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

        sUsedPiece = mBufferSize - sTail;

        if (idlOS::write(mFP, mBufferArray + sTail, sUsedPiece) != (ssize_t)sUsedPiece)
        {
            ideLog::log(IDE_SERVER_0, "[ERROR] Auditing "
                        "fwrite(size=%u) errno=%u\n",
                        sUsed, errno);
        }

        mCurrFileSize += sUsedPiece;

        sUsedPiece = sHeader;

        if (idlOS::write(mFP, mBufferArray, sUsedPiece) != (ssize_t)sUsedPiece)
        {
            ideLog::log(IDE_SERVER_0, "[ERROR] Auditing "
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

void idvAudit::increaseTail( UInt aWrittenSize )
{
    mTailOffset += aWrittenSize;

    if (mTailOffset >  mBufferSize)
    {
        mTailOffset -= mBufferSize;
    }
}

/* ------------------------------------------------
 *
 *  1. normal order 일 경우 그냥 쓰기
 *  2. reverse order일 경우에는
 *     Tail 부터 끝까지.
 *     처음부터 Header까지.
 * ----------------------------------------------*/
IDE_RC idvAudit::writeBufferToDisk()
{
    UInt sState = 0;

    if( mHeaderOffset != mTailOffset ) 
    {
        IDE_ASSERT(lockTailMutex() == IDE_SUCCESS);
        sState = 1;

        if ( mFP == PDL_INVALID_HANDLE )
        {
            openFile();
        }
    
        IDE_TEST(writeBufferToDiskInternal() != IDE_SUCCESS);

        if ( (mFileSize > 0) && (mCurrFileSize > mFileSize) ) 
        {
            IDE_ASSERT(lockFileMutex() == IDE_SUCCESS);
            sState = 2;
            IDE_TEST(closeFile() != IDE_SUCCESS);

            IDE_TEST(openFile() != IDE_SUCCESS);

            IDE_ASSERT(unlockFileMutex() == IDE_SUCCESS);
            sState = 1;
        }

        IDE_ASSERT(unlockTailMutex() == IDE_SUCCESS);
        sState = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 2:
            IDE_ASSERT(unlockFileMutex() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(unlockTailMutex() == IDE_SUCCESS);
        case 0:
            break;
        default:
            IDE_ASSERT(0 == 1);
    }

    return IDE_FAILURE;
}

// hold file mutex state!!
IDE_RC idvAudit::openFile()
{
    UInt  sClock;

    idlOS::umask(0);
    sClock = idlOS::time(NULL); // second

    /* BUG-36807 */
    idlOS::snprintf(mFileName, ID_SIZEOF(mFileName),
                    "%s%salti-%"ID_UINT32_FMT"-%"ID_UINT32_FMT".aud",
                    mAuditLogDir,
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
IDE_RC idvAudit::closeFile()
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

idBool idvAudit::isWriteNeed()
{
    if (getBufUsed() >= mBufferFlushSize)
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

UInt idvAudit::getBufUsed()
{
    return mBufferSize - getBufFree();
}


idBool idvAudit::isEnoughSpace( UInt aToCopyDataLen )
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

UInt idvAudit::getBufFree()
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
        return mBufferSize - (mHeaderOffset - mTailOffset);
    }
    else // reverse order
    {
        return (mTailOffset - mHeaderOffset);
    }
}

/* set properties */

IDE_RC idvAudit::setBufferFlushSize( UInt aSize )
{
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        lockAllMutexForProperty();

        mBufferFlushSize = aSize;

        unlockAllMutexForProperty();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC idvAudit::setBufferSize( UInt aSize )
{
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        lockAllMutexForProperty();

        mBufferSize = aSize;

        unlockAllMutexForProperty();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC idvAudit::setBufferFullSkipFlag( UInt aFlag )
{
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        lockAllMutexForProperty();

        mBufferSkipFlag = aFlag;

        unlockAllMutexForProperty();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

IDE_RC idvAudit::setFileSize( UInt aSize )
{
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        lockAllMutexForProperty();

        mFileSize = aSize;

        unlockAllMutexForProperty();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

/* BUG-36807 */
IDE_RC idvAudit::setAuditLogDir( SChar * aAuditLogDir )
{
    if( mOutputMethod == IDV_AUDIT_OUTPUT_NORMAL)
    {
        lockAllMutexForProperty();

        mAuditLogDir = aAuditLogDir;

        unlockAllMutexForProperty();
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
}

/* mutex */
void idvAudit::lockAllMutexForProperty()
{
    IDE_ASSERT( lockTailMutex()   == IDE_SUCCESS );
    IDE_ASSERT( lockHeaderMutex() == IDE_SUCCESS );
    IDE_ASSERT( lockFileMutex()   == IDE_SUCCESS );
}

void idvAudit::unlockAllMutexForProperty()
{
    IDE_ASSERT( unlockFileMutex()   == IDE_SUCCESS );
    IDE_ASSERT( unlockHeaderMutex() == IDE_SUCCESS );
    IDE_ASSERT( unlockTailMutex()   == IDE_SUCCESS );
}


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
 * $Id: idv.h 18910 2006-11-13 01:56:34Z shsuh $
 **********************************************************************/

#ifndef _O_IDV_PROFILE_H_
#define _O_IDV_PROFILE_H_ 1

#include <idl.h>
#include <idu.h>
#include <idv.h>
#include <idvProfileDef.h>
/* ------------------------------------------------
 *  Profile : PROJ-1598
 * ----------------------------------------------*/

class idvProfile
{
    // action flags
    static UInt     mProfFlag;
    static UInt     mBufSize;
    static UInt     mBufFlushSize;
    static UInt     mBufSkipFlag;
    static UInt     mFileSize;
    static SChar    mFileName[MAXPATHLEN];
    static SChar   *mQueryProfLogDir;
    
    // buf info
    static UChar   *mBufArray;
    static UInt     mHeaderOffset;
    static UInt     mHeaderOffsetForWriting;
    static UInt     mTailOffset;
    static PDL_HANDLE   mFP;
    static UInt     mFileNumber;
    static idBool   mIsWriting;   /* I/O doing now? */
    static ULong    mCurrFileSize;
    
private:
    static iduMutex mFileMutex;  // open/flush/close/flag change
    static iduMutex mHeaderMutex; // buf copy + header offset move
    static iduMutex mTailMutex;  // write + tail offset move
    
public:
    /* ------------------------------------------------
     *  Functions for Manager
     * ----------------------------------------------*/
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC lockFileMtx()   { return mFileMutex.lock( NULL ); }
    static IDE_RC unlockFileMtx() { return mFileMutex.unlock(); }

    static IDE_RC lockHeaderMtx()   { return mHeaderMutex.lock( NULL ); }
    static IDE_RC unlockHeaderMtx() { return mHeaderMutex.unlock(); }

    static IDE_RC lockTailMtx()   { return mTailMutex.lock( NULL ); }
    static IDE_RC unlockTailMtx() { return mTailMutex.unlock(); }

private:
    static void   lockAllMutexForProperty();
    static void   unlockAllMutexForProperty();
    
    // file open/close
    static IDE_RC openFile();
    static IDE_RC closeFile();

    /* ------------------------------------------------
     *  Buffer Control
     * ----------------------------------------------*/
    static IDE_RC openBuffer();
    static IDE_RC closeBuffer();

    static void   copyToBufferInternalOneUnit(void *aSrc, UInt aLength);
    static void   copyToBufferInternal(idvProfDescInfo *aInfo);
    
    static IDE_RC copyToBuffer(idvProfDescInfo *aInfo);

    static UInt   getBufFree();
    static UInt   getBufUsed();
    static idBool isEnoughSpace(UInt aToCopyDataLen);
    static idBool isWriteNeed();

    static void   increaseHeader(UInt aCopiedSize);
    static void   increaseTail  (UInt aWrittenSize);

    
    /* ------------------------------------------------
     *  Write Function 
     * ----------------------------------------------*/

    static IDE_RC writeBufferToDiskInternal();
    
    static IDE_RC writeBufferToDisk();
    
    static IDE_RC writeSessionInternal(idvSession *aSess);
    
    static IDE_RC writeStatementInternal(idvSQL          *aStmt,
                                         idvProfStmtInfo *aInfo,
                                         SChar           *aQueryString,
                                         UInt             aQueryLen);
    
    static IDE_RC writePlanInternal(UInt             aSID,
                                    UInt             aSSID,
                                    UInt             aTxID,
                                    iduVarString    *aPlanString);

    static IDE_RC writeBindInternalA5(void            *aBindData,
                                    UInt             aSID,
                                    UInt             aSSID,
                                    UInt             aTxID,
                                    idvWriteCallbackA5 aCallback);

    static IDE_RC writeBindInternal(idvProfBind     *aData,
                                    UInt             aSID,
                                    UInt             aSSID,
                                    UInt             aTxID,
                                    idvWriteCallback aCallback);

    static IDE_RC writeSessSyssInternal(idvProfType mType, idvSession *aSess);
    
    static IDE_RC writeMemInfoInternal();
    
public:
    
    /* ------------------------------------------------
     *  Action from property
     * ----------------------------------------------*/

    static IDE_RC setProfFlag(UInt aFlag);
    static IDE_RC setBufSize(UInt aFlag);
    static IDE_RC setBufFlushSize(UInt aFlag);
    static IDE_RC setBufFullSkipFlag(UInt aFlag);
    static IDE_RC setFileSize(UInt aFlag);
    static IDE_RC flushAllBufferToFile();

    /* BUG-36806 */
    static IDE_RC setLogDir(SChar *aQueryProfLogDir);

    static inline UInt getProfFlag()
        {
            return mProfFlag;
        }
    // write
    static IDE_RC writeSession(idvSession *aSess)
        {
            if ( (mProfFlag & IDV_PROF_TYPE_SESS_FLAG) == IDV_PROF_TYPE_SESS_FLAG)
            {
                return writeSessSyssInternal(IDV_PROF_TYPE_SESS, aSess);
            }
            return IDE_SUCCESS;
        }
    
    static IDE_RC writeSystem()
        {
            if ( (mProfFlag & IDV_PROF_TYPE_SYSS_FLAG) == IDV_PROF_TYPE_SYSS_FLAG)
            {
                return writeSessSyssInternal(IDV_PROF_TYPE_SYSS, &gSystemInfo);
            }
            return IDE_SUCCESS;
        }
    
    /* ------------------------------------------------
     *  writeStatement의 경우 호출자가 Flag를 검사한다.
     *  복잡한 사전작업으로 인해 예외를 인정.
     * ----------------------------------------------*/
    static IDE_RC writeStatement(idvSQL          *aStmt,
                                 idvProfStmtInfo *aInfo,
                                 SChar           *aQueryString,
                                 UInt             aQueryLen)
        {
            return writeStatementInternal(aStmt, aInfo, aQueryString, aQueryLen);
        }
    // proj_2160 cm_type removal: for A5
    static IDE_RC writeBindA5(void   *aData,
                            UInt    aSID,
                            UInt    aSSID,
                            UInt    aTxID,
                            idvWriteCallbackA5 aCallback)
        {
            if ( (mProfFlag & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
            {
                return writeBindInternalA5(aData, aSID, aSSID, aTxID, aCallback);
            }
            return IDE_SUCCESS;
        }

    // proj_2160 cm_type removal: for A7 or higher
    static IDE_RC writeBind(idvProfBind *aData,
                            UInt    aSID,
                            UInt    aSSID,
                            UInt    aTxID,
                            idvWriteCallback aCallback)
        {
            IDE_RC sRC = IDE_SUCCESS;
            if ( (mProfFlag & IDV_PROF_TYPE_BIND_FLAG) == IDV_PROF_TYPE_BIND_FLAG)
            {
                if (aData->mData != NULL)
                {
                    sRC =  writeBindInternal(aData,
                                             aSID,
                                             aSSID,
                                             aTxID,
                                             aCallback);
                }
                else
                {
                    sRC = IDE_FAILURE;
                }
            }
            return sRC;
        }
 
    /* ------------------------------------------------
     *  Plan 출력시에는 상위에서 판단하여, 호출한다.
     *  Plan Generation 루틴 때문임.
     * ----------------------------------------------*/
    static IDE_RC writePlan(UInt          aSID,
                            UInt          aSSID,
                            UInt          aTxID,
                            iduVarString *aPlanString)
        {
            return writePlanInternal(aSID, aSSID, aTxID, aPlanString);
        }
    
    
    static IDE_RC writeMemInfo()
        {
            if ( (mProfFlag & IDV_PROF_TYPE_MEMS_FLAG) == IDV_PROF_TYPE_MEMS_FLAG)
            {
                return writeMemInfoInternal();
            }
            return IDE_SUCCESS;
        }
    

    /*
    static IDE_RC writeBindInfo(idvSQL *);
    */

};

#endif

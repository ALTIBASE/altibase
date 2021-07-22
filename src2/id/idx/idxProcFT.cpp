/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idxProcFT.cpp 74400 2016-02-04 04:21:29Z donovan.seo $
 **********************************************************************/

#include <idu.h>
#include <idx.h>

class idvSQL;

static iduFixedTableColDesc gAgentProcColDesc[] =
{
    {
        (SChar *)"SID",
        offsetof(idxAgentProc, mSID),
        IDU_FT_SIZEOF(idxAgentProc, mSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"PID",
        offsetof(idxAgentProc, mPID),
        IDU_FT_SIZEOF(idxAgentProc, mPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SOCK_FILE",
        offsetof(idxAgentProc, mSockFile),
        IDU_FT_SIZEOF(idxAgentProc, mSockFile),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"CREATED",
        offsetof(idxAgentProc, mCreated),
        IDU_FT_SIZEOF(idxAgentProc, mCreated),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LAST_RECEIVED",
        offsetof(idxAgentProc, mLastReceived),
        IDU_FT_SIZEOF(idxAgentProc, mLastReceived),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LAST_SENT",
        offsetof(idxAgentProc, mLastSent),
        IDU_FT_SIZEOF(idxAgentProc, mLastSent),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"STATE",
        offsetof(idxAgentProc, mState),
        IDU_FT_SIZEOF(idxAgentProc, mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0, NULL // for internal use
    }
};

// BUG-40819 Remove latch for managing external procedure agent list.
// agent의 상태가 IDX_PROC_ALLOC이 아닌 경우에만 buildRecord를 호출함.
static IDE_RC buildRecordForAgentProc( idvSQL              *,
                                       void                *aHeader,
                                       void                * /* aDumpObj */,
                                       iduFixedTableMemory *aMemory )
{
    idxAgentProc * sAgentInfo= NULL;
    idxAgentProc   sAgentTempInfo;
    UInt           sListCount       = 0;
    UInt           sSockFileNameLen = 0; 
    UInt           i;

    sListCount = iduProperty::getMaxClient()
                 + iduProperty::getJobThreadCount()
                 + iduProperty::getConcExecDegreeMax();;

    sAgentInfo = idxProc::mAgentProcList;

    for ( i = 0; i < sListCount; i++ )
    {
        // 1. agent의 mStatem가 IDX_PROC_ALLOC이 아닌지 검사.
        if ( sAgentInfo->mState != IDX_PROC_ALLOC )
        {
            sAgentTempInfo.mSID           = sAgentInfo->mSID;
            sAgentTempInfo.mPID           = sAgentInfo->mPID;
            sAgentTempInfo.mCreated       = sAgentInfo->mCreated;
            sAgentTempInfo.mLastReceived  = sAgentInfo->mLastReceived;
            sAgentTempInfo.mLastSent      = sAgentInfo->mLastSent;
            sAgentTempInfo.mState         = sAgentInfo->mState;

            // BUG-41482
            sSockFileNameLen = idlOS::strlen( sAgentInfo->mSockFile );
            idlOS::strncpy( sAgentTempInfo.mSockFile,
                            sAgentInfo->mSockFile,
                            sSockFileNameLen );
            sAgentTempInfo.mSockFile[sSockFileNameLen] = '\0';

            // 2. agentInfo를 복사한 뒤에도 상태가 IDX_PROC_ALLOC인지 검사.
            //    2-1. mSID, mSockFile은 변하지 않는 값이다.
            //    2-2. no-latch 구조여서 mPID, mCreated, mLastReceived,
            //         mLastSent는 일시적으로 짝이 맞지 않을 수 있다.
            //    2-3. 메모리를 긁거나 변수를 partial read하는
            //         극단적인 상황은 발생하지 않는다.
            if ( sAgentTempInfo.mState != IDX_PROC_ALLOC )
            {
                IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                      aMemory,
                                                      &(sAgentTempInfo) )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        sAgentInfo++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gAgentProcTableDesc =
{
    (SChar *)"X$EXTPROC_AGENT",
    buildRecordForAgentProc,
    gAgentProcColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

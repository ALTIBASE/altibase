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
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideMsgLog.h>
#include <iduMemList.h>
#include <iduMemMgr.h>
#include <idp.h>
#include <idwPMMgr.h>

/* ------------------------------------------------
 *  Fixed Table Define for Process Monitoring Manager
 * ----------------------------------------------*/
static iduFixedTableColDesc gPMMgrColDesc[] =
{
    {
        (SChar *)"PID",
        offsetof(idwShmProcFTInfo, mPID),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mPID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"State",
        offsetof(idwShmProcFTInfo, mState),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LPID",
        offsetof(idwShmProcFTInfo, mLPID),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mLPID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"THREAD_COUNT",
        offsetof(idwShmProcFTInfo, mThreadCnt),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mThreadCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LOG_BUFFER_SIZE",
        offsetof(idwShmProcFTInfo, mLogBuffSize),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mLogBuffSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"LOG_BUFFER_OFFSET",
        offsetof(idwShmProcFTInfo, mLogOffset),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mLogOffset),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SEM_ID",
        offsetof(idwShmProcFTInfo, mSemID),
        IDU_FT_SIZEOF(idwShmProcFTInfo, mSemID),
        IDU_FT_TYPE_INTEGER,
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


static IDE_RC buildRecord4PMMgr(idvSQL              */* aStatistics */,
                                void                *aHeader,
                                void                */* aDumpObj */,
                                iduFixedTableMemory *aMemory)
{
    UInt               i;
    idwShmProcFTInfo   sShmProcInfo4FT;
    iduShmProcInfo   * sProcInfo;

    for (i = 0; i < IDU_MAX_CLIENT_COUNT; i++)
    {
        sProcInfo = idwPMMgr::getProcInfo(i);

        sShmProcInfo4FT.mPID   = sProcInfo->mPID;
        sShmProcInfo4FT.mState = sProcInfo->mState;
        sShmProcInfo4FT.mLPID  = sProcInfo->mLPID;
        sShmProcInfo4FT.mType  = sProcInfo->mType;
        sShmProcInfo4FT.mThreadCnt   = sProcInfo->mThreadCnt;
        sShmProcInfo4FT.mLogBuffSize = IDU_LOG_BUFFER_SIZE;
        sShmProcInfo4FT.mSemID       = sProcInfo->mSemID;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *) &sShmProcInfo4FT )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gProcMgrTableDesc =
{
    (SChar *)"X$PROCESS",
    buildRecord4PMMgr,
    gPMMgrColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

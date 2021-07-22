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
#include <idwWatchDog.h>

typedef struct idwWatchDogInfo
{
    ULong mDeadProcCnt;
} idwWatchDogInfo;

/* ------------------------------------------------
 *  Fixed Table Define for Process Monitoring Manager
 * ----------------------------------------------*/
static iduFixedTableColDesc gWatchDogDesc[] =
{
    {
        (SChar *)"Dead_Process_Count",
        offsetof(idwWatchDogInfo, mDeadProcCnt),
        IDU_FT_SIZEOF(idwWatchDogInfo, mDeadProcCnt),
        IDU_FT_TYPE_UBIGINT,
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


static IDE_RC buildRecord4WatchDog( idvSQL              */* aStatistics */,
                                    void                *aHeader,
                                    void                */* aDumpObj */,
                                    iduFixedTableMemory *aMemory )
{
    idwWatchDogInfo sWatchDogInfo;

    sWatchDogInfo.mDeadProcCnt = idwWatchDog::getDeadProcCnt();

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *) &sWatchDogInfo )
                  != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gWatchDogTableDesc =
{
    (SChar *)"X$WATCHDOG",
    buildRecord4WatchDog,
    gWatchDogDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

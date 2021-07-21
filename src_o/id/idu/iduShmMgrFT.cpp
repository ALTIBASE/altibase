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
static iduFixedTableColDesc gShmSegColDesc[] =
{
    {
        (SChar *)"Signature",
        offsetof(iduShmHeader, mSigniture),
        IDU_FT_SIZEOF(iduShmHeader, mSigniture),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SegID",
        offsetof(iduShmHeader, mSegID),
        IDU_FT_SIZEOF(iduShmHeader, mSegID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"Size",
        offsetof(iduShmHeader, mSize),
        IDU_FT_SIZEOF(iduShmHeader, mSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"SHM_KEY",
        offsetof(iduShmHeader, mShmKey),
        IDU_FT_SIZEOF(iduShmHeader, mShmKey),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"State",
        offsetof(iduShmHeader, mState),
        IDU_FT_SIZEOF(iduShmHeader, mState),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"Type",
        offsetof(iduShmHeader, mType),
        IDU_FT_SIZEOF(iduShmHeader, mType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0, NULL // for internal use
    },
    {
        (SChar *)"Version",
        offsetof(iduShmHeader, mVersion),
        IDU_FT_SIZEOF(iduShmHeader, mVersion),
        IDU_FT_TYPE_VARCHAR,
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


static IDE_RC buildRecord4ShmSeg( idvSQL              */* aStatistics */,
                                  void                *aHeader,
                                  void                */* aDumpObj */,
                                  iduFixedTableMemory *aMemory )
{
    UInt             i;
    iduShmSSegment * sSysShmSegment;
    iduShmHeader   * sShmSegment;

    sSysShmSegment = iduShmMgr::getSysSeg();

    for( i = 0; i < sSysShmSegment->mSegCount; i++ )
    {
        sShmSegment = iduShmMgr::getSegShmHeader(i);

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *) sShmSegment )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

iduFixedTableDesc gShmSegTableDesc =
{
    (SChar *)"X$SHM_SEGMENT",
    buildRecord4ShmSeg,
    gShmSegColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

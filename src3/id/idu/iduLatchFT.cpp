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
 * $Id:
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduFixedTable.h>
#include <iduLatch.h>
#include <idp.h>

/* ------------------------------------------------
 *  Fixed Table Define for  MemoryMgr
 * ----------------------------------------------*/

SChar* iduLatchCore::mTypes[] = 
{
    (SChar*)"POSIX",
    (SChar*)"POSIX2",
    (SChar*)"Native",
    (SChar*)"Native2"
};

SChar* iduLatchCore::mModes[] =
{
    (SChar*)"Idle",
    (SChar*)"Shared",
    (SChar*)"Exclusive"
};

#if !defined(SMALL_FOOTPRINT) || defined(WRS_VXWORKS)
static iduFixedTableColDesc gLatchColDesc[] =
{
    {
        (SChar *)"LATCH_NAME",
        offsetof(iduLatchCore, mName),
        64,
        IDU_FT_TYPE_VARCHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"LATCH_PTR",
        offsetof(iduLatchCore, mPtrString),
        32,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"LATCH_TYPE",
        offsetof(iduLatchCore, mTypeString),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"LATCH_MODE",
        offsetof(iduLatchCore, mModeString),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"GETREADCOUNT",
        offsetof(iduLatchCore, mGetReadCount),
        IDU_FT_SIZEOF(iduLatchCore, mGetReadCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"GETWRITECOUNT",
        offsetof(iduLatchCore, mGetWriteCount),
        IDU_FT_SIZEOF(iduLatchCore, mGetWriteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"READMISSES",
        offsetof(iduLatchCore, mReadMisses),
        IDU_FT_SIZEOF(iduLatchCore, mReadMisses),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"WRITEMISSES",
        offsetof(iduLatchCore, mWriteMisses),
        IDU_FT_SIZEOF(iduLatchCore, mWriteMisses),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SLEEPCOUNT",
        offsetof(iduLatchCore, mSleepCount),
        IDU_FT_SIZEOF(iduLatchCore, mSleepCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPINCOUNT",
        offsetof(iduLatchCore, mSpinCount),
        IDU_FT_SIZEOF(iduLatchCore, mSpinCount),
        IDU_FT_TYPE_INTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"OWNER",
        offsetof(iduLatchCore, mOwner),
        IDU_FT_SIZEOF(iduLatchCore, mOwner),
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


/*
 * BUG-32243
 * Retrieve only valid mutex entries
 */
static IDE_RC buildRecordForLatch(idvSQL      *,
                                  void        *aHeader,
                                  void        * /* aDumpObj */,
                                  iduFixedTableMemory *aMemory)
{
    iduLatchCore*    sBase;
    iduLatchCore*    sRoot;

    sRoot = iduLatch::getFirstInfo();
    IDE_DASSERT(sRoot != NULL);

    for(sBase  = sRoot->getNextInfo();
        sBase != NULL;
        sBase  = sBase->getNextInfo())
    {
        sBase->mTypeString = iduLatchCore::mTypes[sBase->mType];
        if(sBase->mMode == 0)
        {
            sBase->mModeString = iduLatchCore::mModes[0];
        }
        else
        {
            if(sBase->mMode > 0)
            {
                sBase->mModeString = iduLatchCore::mModes[1];
            }
            else
            {
                sBase->mModeString = iduLatchCore::mModes[2];
            }
        }
        (void)idlOS::snprintf(sBase->mOwner, sizeof(sBase->mOwner),
                              "%X", sBase->mWriteThreadID);
        (void)idlOS::snprintf(sBase->mPtrString, sizeof(sBase->mPtrString),
                              "%llX", (ULong)((iduLatchObj*)sBase));

        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) sBase)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

iduFixedTableDesc gLatchTableDesc =
{
    (SChar *)"X$LATCH",
    buildRecordForLatch,
    gLatchColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
#endif

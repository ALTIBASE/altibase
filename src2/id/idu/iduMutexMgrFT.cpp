/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexMgrFT.cpp 81183 2017-09-25 08:31:43Z yoonhee.kim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduRunTimeInfo.h>
#include <iduFixedTable.h>
#include <iduMutexMgr.h>
#include <idp.h>

class idvSQL;

/* ------------------------------------------------
 *  Fixed Table Define for  MemoryMgr
 * ----------------------------------------------*/

#if !defined(SMALL_FOOTPRINT) || defined(WRS_VXWORKS)
static iduFixedTableColDesc gMutexMgrColDesc[] =
{
    {
        (SChar *)"NAME",
        offsetof(iduMutexEntry, mName),
        IDU_FT_SIZEOF(iduMutexEntry, mName),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MUTEX_PTR",
        offsetof(iduMutexEntry, mStat) + offsetof(iduMutexStat, mMutexPtr),
        16,
        IDU_FT_TYPE_CHAR | IDU_FT_TYPE_POINTER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TRY_COUNT",
        offsetof(iduMutexEntry, mStat) + offsetof(iduMutexStat, mTryCount),
        IDU_FT_SIZEOF(iduMutexStat, mTryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"LOCK_COUNT",
        offsetof(iduMutexEntry, mStat) + offsetof(iduMutexStat, mLockCount),
        IDU_FT_SIZEOF(iduMutexStat, mLockCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MISS_COUNT",
        offsetof(iduMutexEntry, mStat) + offsetof(iduMutexStat, mMissCount),
        IDU_FT_SIZEOF(iduMutexStat, mMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"SPIN_VALUE",
        offsetof(iduMutexEntry, mStat) + offsetof(iduMutexStat, mBusyValue),
        IDU_FT_SIZEOF(iduMutexStat, mBusyValue),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TOTAL_LOCK_TIME_US",
        offsetof(iduMutexEntry, mStat) +
        offsetof(iduMutexStat, mTotalLockTimeUS),
        IDU_FT_SIZEOF(iduMutexStat, mTotalLockTimeUS),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"MAX_LOCK_TIME_US",
        offsetof(iduMutexEntry, mStat) +
        offsetof(iduMutexStat, mMaxLockTimeUS),
        IDU_FT_SIZEOF(iduMutexStat, mMaxLockTimeUS),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"ACCUMULATED_LOCK_COUNT",
        offsetof(iduMutexEntry, mStat) +
        offsetof(iduMutexStat, mAccLockCount),
        IDU_FT_SIZEOF(iduMutexStat, mAccLockCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    /* BUG-43940 V$mutex에서 mutex lock을 획득한 스레드 ID출력 */
    {
        (SChar *)"THREAD_ID",
        offsetof(iduMutexEntry, mStat) + offsetof(iduMutexStat, mThreadID),
        IDU_FT_SIZEOF(iduMutexStat, mThreadID),
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"IDLE",
        offsetof(iduMutexEntry, mIdle), 
        IDU_FT_SIZEOF(iduMutexEntry, mIdle),
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
        0, 0,NULL // for internal use
    }
};


/*
 * BUG-32243
 * Retrieve only valid mutex entries
 */
static IDE_RC buildRecordForMutexMgr(idvSQL      *,
                                     void        *aHeader,
                                     void        * /* aDumpObj */,
                                     iduFixedTableMemory *aMemory)
{
    iduMutexEntry *sBase;
    iduMutexEntry *sRoot;
    UChar         sMutexPtrString[100];
    idBool        sIsLock = ID_FALSE;


    iduMutexMgr::lockInfoHead();
    sIsLock = ID_TRUE;

    sRoot = iduMutexMgr::getRoot();
    sBase = sRoot;

    do
    {
        (void)idlOS::snprintf( (SChar*)sMutexPtrString,
                               ID_SIZEOF(sMutexPtrString),
                               "%"ID_XPOINTER_FMT,
                               (SChar*)(&(sBase->mMutex)));

        sBase->mStat.mMutexPtr = sMutexPtrString;

        sBase->setThreadID();
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) sBase)
                 != IDE_SUCCESS);

        sBase = sBase->getNextInfo();
    } while(sBase != NULL);

    sIsLock = ID_FALSE;
    iduMutexMgr::unlockInfoHead();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sIsLock == ID_TRUE )
    {
        sIsLock = ID_FALSE;
        iduMutexMgr::unlockInfoHead();
    }
    else
    {
        /* Do nothing */
    }
    IDE_POP();

    return IDE_FAILURE;
}

iduFixedTableDesc gMutexMgrTableDesc =
{
    (SChar *)"X$MUTEX",
    buildRecordForMutexMgr,
    gMutexMgrColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};
#endif

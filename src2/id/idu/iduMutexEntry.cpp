/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexEntry.cpp 86279 2019-10-15 01:10:42Z hykim $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduRunTimeInfo.h>
#include <iduMutexMgr.h>
#include <iduMutexEntry.h>
#include <idp.h>

IDE_RC
iduMutexEntry::create(iduMutexKind aKind)
{
    mKind   = aKind;
    mOP     = iduMutexMgr::getMutexOP( aKind );
    IDE_TEST(mOP->mCreate(&mMutex) != IDE_SUCCESS);
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC
iduMutexEntry::initialize(const SChar      *aName,
                          idvWaitIndex      aWEID,
                          UInt              aBusyValue)
{
    mStat.mMutexPtr         = (UChar*)(&mMutex);
    mStat.mTryCount         = 0;
    mStat.mLockCount        = 0;
    mStat.mTotalLockTimeUS  = 0;
    mStat.mMaxLockTimeUS    = 0;
    mStat.mMissCount        = 0;
    mStat.mBusyValue        = (aBusyValue == 0 ? 1 : aBusyValue);
    mStat.mAccLockCount     = 0;
    mStat.mOwner            = (ULong)PDL_INVALID_HANDLE;
    idlOS::memset( &mWeArgs, 0x00, ID_SIZEOF(idvWeArgs) );
    mWeArgs.mWaitEventID = aWEID;

    if ( aWEID == IDV_WAIT_INDEX_NULL )
    {
        mWeArgsPtr = NULL;
    }
    else
    {
        mWeArgsPtr = &mWeArgs;
    }
    
    idlOS::memset(mName, 0, IDU_MUTEX_NAME_LEN);
    idlOS::strncpy(mName, aName, IDU_MUTEX_NAME_LEN - 1);

    IDE_TEST(mOP->mInitialize(&mMutex,
                              aBusyValue) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

/* PROJ-2109 : Remove the bottleneck of alloc/free stmts. */
void
iduMutexEntry::resetStat()
{
    mStat.mTryCount         = 0;
    mStat.mLockCount        = 0;
    mStat.mTotalLockTimeUS  = 0;
    mStat.mMaxLockTimeUS    = 0;
    mStat.mMissCount        = 0;
    mStat.mAccLockCount     = 0;
}

void
iduMutexEntry::setName(SChar *aName)
{
    if (aName != NULL)
    {
        idlOS::memset(mName, 0, IDU_MUTEX_NAME_LEN);
        idlOS::strncpy(mName, aName, IDU_MUTEX_NAME_LEN - 1);
    }
}

/* 
 * BUG-43940 V$mutex에서 mutex lock을 획득한 스레드 ID출력
 * lock을 획득한 mOwner를 16진수로 출력하기 위한 함수
*/
void
iduMutexEntry::setThreadID()
{
    idlOS::memset(mStat.mThreadID, 0, IDU_MUTEX_THREADID_LEN); 
    
    if ( mStat.mOwner != (ULong)PDL_INVALID_HANDLE )
    {
        idlOS::snprintf(mStat.mThreadID, 
                        IDU_MUTEX_THREADID_LEN, 
                        "%"ID_XPOINTER_FMT, 
                        mStat.mOwner);
    }
    else
    {
        /* Do nothing */
    }
}

/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexObj.h 77670 2016-10-18 06:17:00Z yoonhee.kim $
 **********************************************************************/

/* ------------------------------------------------
 *      !!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!
 *
 *  이 헤더화일은 C 소스코드와 호환이 되도록
 *  구현되어야 합니다.
 * ----------------------------------------------*/

#ifndef _O_IDU_MUTEX_OBJ_H_
#define _O_IDU_MUTEX_OBJ_H_ 1

#include <idTypes.h>

/* BUG-31200 - negative values can be printed at v$mutex
 * TRY_COUNT, MISS_COUNT, LOCK_COUNT를 증가시키는 과정에서
 * 오버플로우가 발생하면 모두 0으로 리셋한다.
 * 서버의 경우에는 trc log에 기존 값을 기록하고 리셋한다. */
#define IDU_MUTEX_STAT_INCREASE_COUNT(aMutexStat, aMember)              \
{                                                                       \
    if ( (aMutexStat)->aMember >= (UInt)ID_SLONG_MAX)                   \
    {                                                                   \
        (aMutexStat)->mTryCount  = 0;                                   \
        (aMutexStat)->mMissCount = 0;                                   \
        (aMutexStat)->mLockCount = 0;                                   \
    }                                                                   \
    (aMutexStat)->aMember++;                                            \
}

#define IDU_MUTEX_STAT_INCREASE_COUNT_WITH_TRC_LOG(aMutexStat, aMember) \
{                                                                       \
    if ( (aMutexStat)->aMember>= (UInt)ID_SLONG_MAX)                    \
    {                                                                   \
        ideLog::log( IDE_MISC_0,                                        \
                     "--- reset Mutex Statistics ---\n"                 \
                     " TRY_COUNT:  %"ID_UINT64_FMT"\n"                  \
                     " LOCK_COUNT: %"ID_UINT64_FMT"\n"                  \
                     " MISS_COUNT: %"ID_UINT64_FMT"\n"                  \
                     "------------------------------",                  \
                     (aMutexStat)->mTryCount,                           \
                     (aMutexStat)->mLockCount,                          \
                     (aMutexStat)->mMissCount );                        \
        (aMutexStat)->mTryCount  = 0;                                   \
        (aMutexStat)->mMissCount = 0;                                   \
        (aMutexStat)->mLockCount = 0;                                   \
    }                                                                   \
    (aMutexStat)->aMember++;                                            \
}

#define IDU_MUTEX_STAT_INCREASE_TRY_COUNT_WITH_TRC_LOG(aMutexStat)   \
    IDU_MUTEX_STAT_INCREASE_COUNT_WITH_TRC_LOG(aMutexStat, mTryCount)

#define IDU_MUTEX_STAT_INCREASE_MISS_COUNT_WITH_TRC_LOG(aMutexStat)   \
    IDU_MUTEX_STAT_INCREASE_COUNT_WITH_TRC_LOG(aMutexStat, mMissCount)

#define IDU_MUTEX_STAT_INCREASE_LOCK_COUNT_WITH_TRC_LOG(aMutexStat)   \
    IDU_MUTEX_STAT_INCREASE_COUNT_WITH_TRC_LOG(aMutexStat, mLockCount)

#define IDU_MUTEX_STAT_INCREASE_TRY_COUNT(aMutexStat)   \
    IDU_MUTEX_STAT_INCREASE_COUNT(aMutexStat, mTryCount)

#define IDU_MUTEX_STAT_INCREASE_MISS_COUNT(aMutexStat)   \
    IDU_MUTEX_STAT_INCREASE_COUNT(aMutexStat, mMissCount)

#define IDU_MUTEX_STAT_INCREASE_LOCK_COUNT(aMutexStat)   \
    IDU_MUTEX_STAT_INCREASE_COUNT(aMutexStat, mLockCount)

#define IDU_MUTEX_THREADID_LEN (64)

typedef enum 
{
    IDU_MUTEX_KIND_POSIX = 0,
    
    IDU_MUTEX_KIND_NATIVE,
    IDU_MUTEX_KIND_NATIVE2,
    /*
     * BUG-28856 logging 병목제거
     */
    IDU_MUTEX_KIND_NATIVE_FOR_LOGGING,
    
    IDU_MUTEX_KIND_MAX
}iduMutexKind;

typedef struct iduMutexStat
{
    UChar*  mMutexPtr;
    ULong   mTryCount;  /* How Many Time Tried on BusyWait Loop. */
    ULong   mLockCount; /* Plus when Lock Success  */
    ULong   mMissCount; /* Plus When First Lock Miss */
    UInt    mBusyValue; /* Spin Lock Count */
    ULong   mTotalLockTimeUS; /* Total time of Interval of lock() ~ unlock()
                               In micro seconds */
    ULong   mMaxLockTimeUS;   /* Max time of Interval of lock() ~ unlock()
                               In micro seconds */    
    UInt    mAccLockCount;  /* +1 on lock, -1 on unlock */
    ULong   mOwner;         /* Who is holding this lock */
    SChar   mThreadID[IDU_MUTEX_THREADID_LEN];      /* convert mOwner to Hex */
} iduMutexStat;

typedef struct iduMutexOP
{
    IDE_RC (*mInitializeStatic)();
    IDE_RC (*mDestroyStatic)();
    IDE_RC (*mCreate)(void *aRsc);
    IDE_RC (*mInitialize)(void *aRsc, UInt aBusyValue);
    IDE_RC (*mFinalize)(void *aRsc);
    IDE_RC (*mDestroy)(void *aRsc);
    void   (*mLock)(void *aRsc, iduMutexStat *aStat, void * aStatSQL, void * aWeArgs );
    void   (*mTryLock)(void *aRsc, idBool *aRet, iduMutexStat *aStat );
    void   (*mUnlock)(void *aRsc, iduMutexStat *aStat);    
} iduMutexOP;

#endif	// _O_MUTEX_OBJ_H_


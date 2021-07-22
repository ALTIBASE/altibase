/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutexEntryPOSIX.cpp 68602 2015-01-23 00:13:11Z sbjang $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduMemPool.h>
#include <iduMutexEntry.h>
#include <iduProperty.h>
#include <idp.h>
#include <idv.h>

static inline void iduPosixProfileLockedTime(iduMutexResPOSIX * aMutexRes )
{
    if ( IDV_TIME_AVAILABLE() )
    {
        IDV_TIME_GET( & aMutexRes->mLastLockedTime );
        aMutexRes->mIsLockTimeValid = ID_TRUE;
    }
}

static inline void iduPosixProfileUnlockedTime(iduMutexResPOSIX * aMutexRes,
                                               iduMutexStat *aStat)
{
    idvTime       sUnlockTime ;
    ULong         sLockedUS;
    
    if ( IDV_TIME_AVAILABLE() )
    {
        if ( aMutexRes->mIsLockTimeValid == ID_TRUE )
        {
            IDV_TIME_GET( & sUnlockTime );
            
            sLockedUS = IDV_TIME_DIFF_MICRO( &aMutexRes->mLastLockedTime,
                                             &sUnlockTime );
            
            aStat->mTotalLockTimeUS += sLockedUS;
            
            if ( sLockedUS > aStat->mMaxLockTimeUS )
            {
                aStat->mMaxLockTimeUS = sLockedUS ;
            }
        }
    }
}


static IDE_RC iduPosixInitializeStatic()
{
    iduMutexEntry sDummy;

    IDE_ASSERT( ID_SIZEOF(iduMutexResPOSIX) <= ID_SIZEOF(sDummy.mMutex));

    return IDE_SUCCESS;
}

static IDE_RC iduPosixDestroyStatic()
{
    return IDE_SUCCESS;
}

static IDE_RC iduPosixCreate(void* aRsc)
{
    iduMutexResPOSIX* sMutexRes = ( iduMutexResPOSIX* ) aRsc;

    IDE_TEST(idlOS::thread_mutex_init( & sMutexRes->mMutex,
                                       USYNC_THREAD,
                                       0 ) != 0);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    IDE_SET_AND_DIE(ideSetErrorCode (idERR_FATAL_ThrMutexInit));
    return IDE_FAILURE;
}

static IDE_RC iduPosixInitialize(void *aRsc, UInt /*aBusyValue*/)
{
    iduMutexResPOSIX* sMutexRes = ( iduMutexResPOSIX* ) aRsc;

    sMutexRes->mIsLockTimeValid = ID_FALSE;

    return IDE_SUCCESS;
}

static IDE_RC iduPosixFinalize(void * /*aRsc*/)
{
    return IDE_SUCCESS;
}

static IDE_RC iduPosixDestroy(void *aRsc)
{
    iduMutexResPOSIX * sMutexRes;
    sMutexRes = ( iduMutexResPOSIX* )aRsc;

    IDE_TEST_RAISE(idlOS::thread_mutex_destroy( & sMutexRes->mMutex ) != 0,
                   m_mutexdestroy_error);

    return IDE_SUCCESS;
    IDE_EXCEPTION(m_mutexdestroy_error);
    {
        IDE_SET_AND_DIE(ideSetErrorCode (idERR_FATAL_ThrMutexDestroy));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static void iduPosixTryLock(void *aRsc, idBool *aRet, iduMutexStat *aStat)
{
    iduMutexResPOSIX * sMutexRes;
    sMutexRes = ( iduMutexResPOSIX* )aRsc;
    
    IDU_MUTEX_STAT_INCREASE_TRY_COUNT_WITH_TRC_LOG( aStat );
    if (idlOS::thread_mutex_trylock( & sMutexRes->mMutex ) == 0)
    {
        
        if ( iduProperty::getCheckMutexDurationTimeEnable() == 1 )
        {
            iduPosixProfileLockedTime( sMutexRes );
        }

        IDU_MUTEX_STAT_INCREASE_LOCK_COUNT_WITH_TRC_LOG( aStat );
        *aRet = ID_TRUE;
    }
    else
    {
        IDE_DASSERT(errno == EBUSY);
        IDU_MUTEX_STAT_INCREASE_MISS_COUNT_WITH_TRC_LOG( aStat );
        *aRet = ID_FALSE;
    }
}

static void iduPosixLock(void         * aRsc,
                         iduMutexStat * aStat,
                         void         * aStatSQL,
                         void         * aWeArgs )
{
    iduMutexResPOSIX * sMutexRes;
    sMutexRes = ( iduMutexResPOSIX* )aRsc;

    IDU_MUTEX_STAT_INCREASE_TRY_COUNT_WITH_TRC_LOG( aStat );
    if (idlOS::thread_mutex_trylock( & sMutexRes->mMutex ) != 0)
    {
        IDE_DASSERT(errno == EBUSY);
        
        IDU_MUTEX_STAT_INCREASE_MISS_COUNT_WITH_TRC_LOG( aStat );
        
        IDV_BEGIN_WAIT_EVENT( aStatSQL, aWeArgs );
        
        IDE_ASSERT(idlOS::thread_mutex_lock( & sMutexRes->mMutex ) == 0);
        
        IDV_END_WAIT_EVENT( aStatSQL, aWeArgs );
    }
    
    IDU_MUTEX_STAT_INCREASE_LOCK_COUNT_WITH_TRC_LOG( aStat );
    
    if ( iduProperty::getCheckMutexDurationTimeEnable() ==1 )
    {
        iduPosixProfileLockedTime( sMutexRes );
    }
}

static void iduPosixUnlock(void *aRsc, iduMutexStat *aStat)
{
    iduMutexResPOSIX * sMutexRes;
    
    sMutexRes = ( iduMutexResPOSIX* )aRsc;

    if ( iduProperty::getCheckMutexDurationTimeEnable() ==1 )
    {
        iduPosixProfileUnlockedTime( sMutexRes, aStat );
    }
    
    sMutexRes->mIsLockTimeValid = ID_FALSE;
    
    IDE_ASSERT(idlOS::thread_mutex_unlock( & sMutexRes->mMutex ) == 0);
}

//fix PROJ-1749
iduMutexOP gPosixMutexServerOps =
{
    iduPosixInitializeStatic,
    iduPosixDestroyStatic,
    iduPosixCreate,
    iduPosixInitialize,
    iduPosixFinalize,
    iduPosixDestroy,
    iduPosixLock,
    iduPosixTryLock,
    iduPosixUnlock
};




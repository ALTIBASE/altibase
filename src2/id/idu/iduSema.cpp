/***********************************************************************
 * Copyright 1999-2000, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduSema.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <iduSema.h>
#include <idErrorCode.h>

IDE_RC iduSema::initialize(iduSema_t *aSema, UInt aCount)
{
#define IDE_FN "iduSema::initialize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE(idlOS::mutex_init(&aSema->lock_, USYNC_PROCESS) != 0,
                   mutex_init_error);
    IDE_TEST_RAISE(idlOS::cond_init(&aSema->count_nonzero_, (short)USYNC_PROCESS) != 0,
                   cond_init_error);
    
    aSema->count_   = aCount;
    aSema->waiters_ = 0;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(mutex_init_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexInit));
    }
    
    IDE_EXCEPTION(cond_init_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondInit));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
            
}

IDE_RC iduSema::destroy(iduSema_t *aSema)
{
#define IDE_FN "iduSema::destroy()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE(idlOS::mutex_destroy(&aSema->lock_) != 0,
                   mutex_destroy_error);
    
    IDE_TEST_RAISE(idlOS::cond_init(&aSema->count_nonzero_) != 0,
                   cond_destroy_error);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(mutex_destroy_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexDestroy));
    }
    
    IDE_EXCEPTION(cond_destroy_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondDestroy));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduSema::post (iduSema_t *aSema)
{
#define IDE_FN "iduSema::post (iduSema_t *aSema)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    SInt sFlag = 0;
    
    IDE_TEST_RAISE(idlOS::mutex_lock (&(aSema->lock_)) != 0,
                   mutex_lock_error);
    sFlag = 1;
    
    // Always allow a waiter to continue if there is one.
    if (aSema->waiters_ > 0)
    {
        IDE_TEST_RAISE(idlOS::cond_signal (&(aSema->count_nonzero_))
                       != 0, cond_signal_error);
    }
    
    aSema->count_++;
    sFlag = 0;
    IDE_TEST_RAISE(idlOS::mutex_unlock (&(aSema->lock_)) != 0,
                   mutex_unlock_error);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    IDE_EXCEPTION(cond_signal_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrCondSignal));
    }
    IDE_EXCEPTION_END;
    {
        if (sFlag == 1)
        {
            IDE_PUSH();
            {
                if (idlOS::mutex_unlock (&(aSema->lock_)) != 0)
                {
                    IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
                }
            }
            IDE_POP();
        }
    }
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduSema::post (iduSema_t *aSema, size_t aReleaseCount)
{
#define IDE_FN "iduSema::post (iduSema_t *aSema, size_t aReleaseCount)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    size_t i;
    
    for (i = 0; i < aReleaseCount; i++)
    {
        IDE_TEST(iduSema::post (aSema) != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduSema::trywait (iduSema_t *aSema, idBool *aSuccess)
{
#define IDE_FN "iduSema::trywait (iduSema_t *aSema)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE(idlOS::mutex_lock (&(aSema->lock_)) != 0,
                   mutex_lock_error);
    if (aSema->count_ > 0)
    {
        --aSema->count_;
        *aSuccess = ID_TRUE;
    }
    else
    {
        *aSuccess = ID_FALSE;
    }
    
    IDE_TEST_RAISE(idlOS::mutex_unlock (&aSema->lock_) != 0,
                   mutex_unlock_error);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduSema::wait (iduSema_t *aSema)
{
#define IDE_FN "iduSema::wait (iduSema_t *aSema)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_TEST_RAISE(idlOS::mutex_lock (&aSema->lock_) != 0, mutex_lock_error);
    aSema->waiters_++;
        
    while (aSema->count_ == 0)
    {
        if (idlOS::cond_wait (&aSema->count_nonzero_, &aSema->lock_) != 0)
        {
            
            ideLog::log(IDE_SERVER_0, "cond_wait() wakeup (errno=%d)\n", errno);
            break;
        }
    }
    
    --aSema->waiters_;
    --aSema->count_;
    
    IDE_TEST_RAISE(idlOS::mutex_unlock (&aSema->lock_), mutex_unlock_error);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduSema::wait (iduSema_t *aSema, PDL_Time_Value *aTime)
{
#define IDE_FN "iduSema::wait (iduSema_t *aSema, PDL_Time_Value *aTime)"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));
    
    IDE_TEST_RAISE(idlOS::mutex_lock (&aSema->lock_) != 0, mutex_lock_error);
    aSema->waiters_++;
        
    // Wait until the semaphore count is > 0 or until we time out.
    while (aSema->count_ == 0)
    {
        if (idlOS::cond_timedwait (&aSema->count_nonzero_, &aSema->lock_, aTime) != 0)
        {
            
            ideLog::log(IDE_SERVER_0, "cond_timedwait() wakeup (errno=%d)\n", errno);
            break;
        }
    }
    
    --aSema->waiters_;
    --aSema->count_;

    IDE_TEST_RAISE(idlOS::mutex_unlock (&aSema->lock_), mutex_unlock_error);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(mutex_lock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(mutex_unlock_error);
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_ThrMutexUnlock));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}


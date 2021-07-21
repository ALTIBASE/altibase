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
 
/*******************************************************************************
 * $Id: acpThrMutex.h 9116 2009-12-10 02:03:59Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_THR_MUTEX_H_)
#define _O_ACP_THR_MUTEX_H_

/**
 * @file
 * @ingroup CoreThr
 */

#include <acpError.h>
#include <acpAtomic.h>
#include <acpThr.h>

ACP_EXTERN_C_BEGIN


/*
 * flag for mutex attribute
 */
/**
 * default mutex (fastest mutex for each platform)
 *
 * @see acpThrMutexCreate()
 */
#define ACP_THR_MUTEX_DEFAULT    0
/**
 * error check mutex
 *
 * @see acpThrMutexCreate()
 */
#define ACP_THR_MUTEX_ERRORCHECK 1
/**
 * recursive mutex
 *
 * @see acpThrMutexCreate()
 */
#define ACP_THR_MUTEX_RECURSIVE  2


#if defined(ACP_CFG_DOXYGEN)
/**
 * thread mutex initializer
 */
#define ACP_THR_MUTEX_INITIALIZER
#elif defined(ALINT)
#define ACP_THR_MUTEX_INITIALIZER { 0 }
#else
#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_THR_MUTEX_INITIALIZER { 0, NULL, { NULL } }
#else
#define ACP_THR_MUTEX_INITIALIZER { PTHREAD_MUTEX_INITIALIZER }
#endif
#endif

/**
 * thread mutex object
 */
typedef struct acp_thr_mutex_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    volatile acp_sint32_t   mInitFlag;
    struct acpThrMutexFunc *mOp;
    union
    {
        HANDLE              mHandle;
        CRITICAL_SECTION    mCSHandle;
    } mMech;
#elif defined(ALINT)
    acp_sint32_t    mMutex;
#else
    pthread_mutex_t mMutex;
#endif
} acp_thr_mutex_t;

#if defined(ALTI_CFG_OS_WINDOWS)
typedef struct acpThrMutexFunc
{
    acp_rc_t (*mLockFunc)(acp_thr_mutex_t *aMutex);
    acp_rc_t (*mTryLockFunc)(acp_thr_mutex_t *aMutex);
    acp_rc_t (*mUnlockFunc)(acp_thr_mutex_t *aMutex);
    acp_rc_t (*mDestroyFunc)(acp_thr_mutex_t *aMutex);
} acpThrMutexFunc;
#endif

ACP_EXPORT acp_rc_t acpThrMutexCreate(acp_thr_mutex_t *aMutex,
                                      acp_sint32_t     aFlag);

#if defined(ALTI_CFG_OS_WINDOWS)

ACP_INLINE acp_rc_t acpThrMutexCreateIfNot(acp_thr_mutex_t *aMutex)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;

    if (aMutex->mInitFlag != 2)
    {
        if (acpAtomicCas32(&aMutex->mInitFlag, 1, 0) == 0)
        {
            sRC = acpThrMutexCreate(aMutex, ACP_THR_MUTEX_DEFAULT);
        }
        else
        {
            while (aMutex->mInitFlag != 2)
            {
                ACP_MEM_BARRIER();
                acpThrYield();
            }
        }
    }
    else
    {
        /* do nothing */
    }

    return sRC;
}

#endif

/**
 * destroys a thread mutex
 *
 * @param aMutex pointer to the thread mutex
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrMutexDestroy(acp_thr_mutex_t *aMutex)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    return (aMutex->mOp->mDestroyFunc)(aMutex);
#else
    return pthread_mutex_destroy(&aMutex->mMutex);
#endif
}

/**
 * locks a thread mutex
 *
 * @param aMutex pointer to the thread mutex
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrMutexLock(acp_thr_mutex_t *aMutex)
{
#if (__STATIC_ANALYSIS_DOING__)
    /*
     * Removing Codesonar warning: Double-Lock
     */
    ACP_UNUSED(aMutex);
    return ACP_RC_SUCCESS;
#elif defined(ALTI_CFG_OS_WINDOWS)

    acp_rc_t sRC;

    sRC = acpThrMutexCreateIfNot(aMutex);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        return (aMutex->mOp->mLockFunc)(aMutex);
    }
    else
    {
        return sRC;
    }
#else
    return pthread_mutex_lock(&aMutex->mMutex);
#endif
}

/**
 * trys lock a thread mutex
 *
 * @param aMutex pointer to the thread mutex
 * @return result code
 *
 * returns #ACP_RC_EBUSY if the mutex is already locked by other thread
 */
ACP_INLINE acp_rc_t acpThrMutexTryLock(acp_thr_mutex_t *aMutex)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_rc_t sRC;

    sRC = acpThrMutexCreateIfNot(aMutex);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        return (aMutex->mOp->mTryLockFunc)(aMutex);
    }
    else
    {
        return sRC;
    }
#else
    return pthread_mutex_trylock(&aMutex->mMutex);
#endif
}

/**
 * unlocks a thread mutex
 *
 * @param aMutex pointer to the thread mutex
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrMutexUnlock(acp_thr_mutex_t *aMutex)
{
#if (__STATIC_ANALYSIS_DOING__)
    /*
     * Removing Codesonar warning: Double-Unlock
     */
    ACP_UNUSED(aMutex);
    return ACP_RC_SUCCESS;
#elif defined(ALTI_CFG_OS_WINDOWS)
    return (aMutex->mOp->mUnlockFunc)(aMutex);
#else
    return pthread_mutex_unlock(&aMutex->mMutex);
#endif
}

ACP_EXTERN_C_END


#endif

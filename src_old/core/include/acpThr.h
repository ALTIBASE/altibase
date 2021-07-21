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
 * $Id: acpThr.h 8921 2009-12-01 05:44:21Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_THR_H_)
#define _O_ACP_THR_H_

/**
 * @file
 * @ingroup CoreThr
 */

#include <acpAtomic.h>
#include <acpError.h>
#include <acpOS.h>

#if defined(ALTI_CFG_OS_TRU64)
#undef pthread_getsequence_np
#undef pthread_getselfseq_np
#endif


ACP_EXTERN_C_BEGIN


/**
 * thread attribute object
 */
typedef struct acp_thr_attr_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    acp_sint32_t   mDetach;
    acp_size_t     mStackSize;
#else
    pthread_attr_t mAttr;
#endif
} acp_thr_attr_t;

/**
 * thread function
 */
typedef acp_sint32_t acp_thr_func_t(void *);

/**
 * thread object
 */
typedef struct acp_thr_t
{
    /* BUGBUG: mArg, mFunc are not used anywhere.
     * They can be removed. */
    void           *mArg;
    acp_thr_func_t *mFunc;
#if defined(ALTI_CFG_OS_WINDOWS)
    HANDLE          mHandle;
    acp_uint64_t    mThrId;
#else
    pthread_t       mHandle;
#endif
} acp_thr_t;


/**
 * thread once function
 */
typedef void acp_thr_once_func_t(void);

/*
 * Flags to represent the status of acpThrOnce
 */
typedef volatile enum
{
    ACP_THR_ONCE_INIT = 0, /* before execution */
    ACP_THR_ONCE_WAIT,     /* under  execution */
    ACP_THR_ONCE_DONE      /* after  execution */
} acp_thr_once_t;

#define ACP_THR_ONCE_INITIALIZER ACP_THR_ONCE_INIT

/*
 * thread attribute functions
 */
/**
 * creates thread attribute object
 *
 * @param aAttr pointer to the thread attribute object
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrAttrCreate(acp_thr_attr_t *aAttr)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aAttr->mDetach    = ACP_FALSE;
    aAttr->mStackSize = 0;

    return ACP_RC_SUCCESS;
#else
    return pthread_attr_init(&aAttr->mAttr);
#endif
}

/**
 * destroys thread attribute object
 *
 * @param aAttr pointer to the thread attribute object
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrAttrDestroy(acp_thr_attr_t *aAttr)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aAttr->mDetach    = ACP_FALSE;
    aAttr->mStackSize = 0;

    return ACP_RC_SUCCESS;
#else
    return pthread_attr_destroy(&aAttr->mAttr);
#endif
}

/**
 * sets thread bound mode to the thread attribute object
 *
 * @param aAttr pointer to the thread attribute object
 * @param aFlag #ACP_TRUE for bound mode, #ACP_FALSE for unbound mode
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrAttrSetBound(acp_thr_attr_t *aAttr,
                                       acp_bool_t      aFlag)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    if (aFlag == ACP_TRUE)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_ENOTSUP;
    }
#else
    return pthread_attr_setscope(&aAttr->mAttr,
                                 (aFlag == ACP_TRUE) ?
                                 PTHREAD_SCOPE_SYSTEM : PTHREAD_SCOPE_PROCESS);
#endif
}

/**
 * sets thread detach state to the thread attribute object
 *
 * @param aAttr pointer to the thread attribute object
 * @param aFlag #ACP_TRUE for detached state, #ACP_FALSE for undetached state
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrAttrSetDetach(acp_thr_attr_t *aAttr,
                                        acp_bool_t      aFlag)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aAttr->mDetach = aFlag;

    return ACP_RC_SUCCESS;
#else
    return pthread_attr_setdetachstate(&aAttr->mAttr,
                                       (aFlag == ACP_TRUE) ?
                                       PTHREAD_CREATE_DETACHED :
                                       PTHREAD_CREATE_JOINABLE);
#endif
}

/**
 * sets thread stack size to the thread attribute object
 *
 * @param aAttr pointer to the thread attribute object
 * @param aStackSize thread stack size
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrAttrSetStackSize(acp_thr_attr_t *aAttr,
                                           acp_size_t      aStackSize)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    aAttr->mStackSize = aStackSize;

    return ACP_RC_SUCCESS;
#else
    return pthread_attr_setstacksize(&aAttr->mAttr, aStackSize);
#endif
}


/*
 * thread functions
 */
ACP_EXPORT acp_rc_t acpThrCreate(acp_thr_t      *aThr,
                                 acp_thr_attr_t *aAttr,
                                 acp_thr_func_t *aFunc,
                                 void           *aArg);

/**
 * detaches a thread
 *
 * @param aThr pointer to the thread object
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrDetach(acp_thr_t *aThr)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    DWORD sRet;

    sRet = CloseHandle(aThr->mHandle);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
#else
    return pthread_detach(aThr->mHandle);
#endif
}

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_EXPORT acp_rc_t acpThrJoin(acp_thr_t *aThr, acp_sint32_t *aExitCode);

#else

/**
 * joins a thread
 *
 * @param aThr pointer to the thread object
 * @param aExitCode pointer to a variable to store thread exit code
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrJoin(acp_thr_t *aThr, acp_sint32_t *aExitCode)
{
    acp_sint32_t  sRet;
    void         *sExitCode = NULL;

    sRet = pthread_join(aThr->mHandle, &sExitCode);

    if (aExitCode != NULL)
    {
        *aExitCode = (acp_sint32_t)(acp_ulong_t)sExitCode;
    }
    else
    {
        /* do nothing */
    }
    return sRet;
}

#endif

/**
 * terminates current thread
 *
 * @param aExitCode thread exit code to return
 */
ACP_INLINE void acpThrExit(acp_sint32_t aExitCode)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    ExitThread((DWORD)aExitCode);
#else
    pthread_exit((void *)(acp_ulong_t)aExitCode);
#endif
}

/**
 * yields current thread
 */
ACP_INLINE void acpThrYield(void)
{
#if defined(ALTI_CFG_OS_LINUX)
    (void)pthread_yield();
#elif defined(ALTI_CFG_OS_TRU64)
    (void)pthread_yield_np();
#elif defined(ALTI_CFG_OS_WINDOWS)
    /*
     * Reliquishes the remainder of it's time slice
     * to any other thread of equal priority that is
     * ready to run.
     */
    Sleep(0);
#else
    (void)sched_yield();
#endif
}

/**
 * gets thread id of a thread
 * This id is not system-wide unique, but only unique in the same process.
 *
 * @param aThr pointer to the thread object
 * @param aID pointer to a variable to store thread id
 * @return result code
 */
ACP_INLINE acp_rc_t acpThrGetID(acp_thr_t *aThr, acp_uint64_t *aID)
{
    /*
     * PORTING
     */
#if defined(ALTI_CFG_OS_AIX)
    acp_sint32_t sID;
    acp_sint32_t sRet;

    sRet = pthread_getunique_np(&aThr->mHandle, &sID);

    *aID = (acp_uint64_t)sID;

    return sRet;
#elif defined(ALTI_CFG_OS_WINDOWS)
    /*
     * BUG-30136
     * Simple is best :)
     */
    *aID = aThr->mThrId;
    return ACP_RC_SUCCESS;
#elif defined(ALTI_CFG_OS_TRU64)
    *aID = pthread_getsequence_np(aThr->mHandle);

    return ACP_RC_SUCCESS;
#else
    *aID = (acp_uint64_t)aThr->mHandle;

    return ACP_RC_SUCCESS;
#endif
}

/**
 * gets thread id of current thread
 * This id is not system-wide unique, but only unique in the same process.
 *
 * @return thread id
 */
ACP_INLINE acp_uint64_t acpThrGetSelfID(void)
{
    /*
     * PORTING
     */
#if defined(ALTI_CFG_OS_AIX)
    pthread_t    sThr;
    acp_sint32_t sID;

    sThr = pthread_self();

    (void)pthread_getunique_np(&sThr, &sID);

    return (acp_uint64_t)sID;
#elif defined(ALTI_CFG_OS_TRU64)
    return pthread_getselfseq_np();
#elif defined(ALTI_CFG_OS_WINDOWS)
    return GetCurrentThreadId();
#else
    return (acp_uint64_t)pthread_self();
#endif
}

/**
 * gets parallel index of current thread
 *
 * @return parallel index
 *
 * parallel index is suitable for hashing to increase parallelism
 */
ACP_INLINE acp_uint32_t acpThrGetParallelIndex(void)
{
    /*
     * PORTING
     */
#if defined(ALTI_CFG_OS_LINUX)
    /*
     * NPTL 2.4
     */
    return (acp_uint32_t)(acpThrGetSelfID() >> 12);
#elif defined(ALTI_CFG_OS_WINDOWS)
    return (acp_uint32_t)(acpThrGetSelfID() >> 4);
#elif defined(ALTI_CFG_OS_FREEBSD)
    return (acp_uint32_t)(acpThrGetSelfID() >> 8);
#elif defined(ALTI_CFG_OS_DARWIN)
    return (acp_uint32_t)(acpThrGetSelfID() >> 12);
#else
    return (acp_uint32_t)acpThrGetSelfID();
#endif
}

ACP_EXPORT void acpThrOnce(acp_thr_once_t      *aOnceControl,
                           acp_thr_once_func_t *aFunc);

ACP_EXTERN_C_END


#endif

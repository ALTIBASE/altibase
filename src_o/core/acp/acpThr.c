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
 * $Id: acpThr.c 8921 2009-12-01 05:44:21Z gurugio $
 ******************************************************************************/

#include <acpSpinWait.h>
#include <acpThr.h>
#include <acpTest.h>

/**
 * runs a function only once
 *
 * @param aOnceControl pointer to the thread once control
 * @param aFunc pointer to the function to run
 *
 * before the first call,
 * @a aOnceControl must be initialized as ACP_THR_ONCE_INITIALIZER.
 */
ACP_EXPORT void acpThrOnce(acp_thr_once_t      *aOnceControl,
                           acp_thr_once_func_t *aFunc)
{

    /* avoid duplication */
    ACP_TEST(*aOnceControl == ACP_THR_ONCE_DONE);


    if (acpAtomicCas32(aOnceControl,
                       ACP_THR_ONCE_WAIT,
                       ACP_THR_ONCE_INIT) == ACP_THR_ONCE_INIT)
    {
        (*aFunc)();

        *aOnceControl = ACP_THR_ONCE_DONE;
    }
    else
    {
        /*
         * wait until the other threads are finished
         */
        ACP_SPIN_WAIT(*aOnceControl == ACP_THR_ONCE_DONE, 0);
    }


    return; /* explicitly exit */

    ACP_EXCEPTION_END;
    return; /* explicitly exit */
}

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_EXPORT acp_rc_t acpThrCreate(acp_thr_t      *aThr,
                                 acp_thr_attr_t *aAttr,
                                 acp_thr_func_t *aFunc,
                                 void           *aArg)
{
    DWORD  sThrId;
    HANDLE sHandle;

    sHandle = CreateThread(NULL,
                           ((aAttr != NULL) && (aAttr->mStackSize > 0)) ?
                           aAttr->mStackSize : 0,
                           (LPTHREAD_START_ROUTINE)aFunc,
                           aArg,
                           0,
                           &sThrId);

    if (sHandle == NULL)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    if ((aAttr != NULL) && (aAttr->mDetach == ACP_TRUE))
    {
        CloseHandle(sHandle);
    }
    else
    {
        /*
         * BUG-30136
         * Storing newly created thread id into acp_thr_t
         */
        aThr->mArg      = aArg;
        aThr->mFunc     = aFunc;
        aThr->mHandle   = sHandle;
        aThr->mThrId    = sThrId;
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpThrJoin(acp_thr_t *aThr, acp_sint32_t *aExitCode)
{
    acp_rc_t sRC;
    BOOL     sRet;
    DWORD    sExitCode;

    if (aThr->mHandle == NULL)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* do nothing */
    }

    sRet = WaitForSingleObject(aThr->mHandle, INFINITE);

    if (sRet == WAIT_OBJECT_0)
    {
        sRet = GetExitCodeThread(aThr->mHandle, &sExitCode);

        if (sRet != 0)
        {
            if (aExitCode != NULL)
            {
                *aExitCode = sExitCode;
            }
            else
            {
                /* do nothing */
            }

            sRC = ACP_RC_SUCCESS;
        }
        else
        {
            sRC = ACP_RC_GET_OS_ERROR();
        }
    }
    else
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    CloseHandle(aThr->mHandle);

    return sRC;
}

#else

/**
 * creates a thread
 *
 * @param aThr pointer to the thread object
 * @param aAttr pointer to the thread attribute object
 * @param aFunc pointer to the thread function
 * @param aArg user data for thread function
 * @return result code
 */
ACP_EXPORT acp_rc_t acpThrCreate(acp_thr_t      *aThr,
                                 acp_thr_attr_t *aAttr,
                                 acp_thr_func_t *aFunc,
                                 void           *aArg)
{
    acp_sint32_t sRet;

    sRet = pthread_create(&aThr->mHandle,
                          (aAttr != NULL) ? &aAttr->mAttr : NULL,
                          (void *(*)(void *))aFunc,
                          aArg);

    if (sRet == 0)
    {
        aThr->mFunc  = aFunc;
        aThr->mArg   = aArg;

        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_GET_OS_ERROR();
    }
}

#endif

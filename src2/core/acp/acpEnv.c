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
 * $Id: acpEnv.c 9797 2010-01-28 05:39:03Z gurugio $
 ******************************************************************************/

#include <acpEnv.h>
#include <acpMem.h>
#include <acpPrintf.h>
#include <acpSpinLock.h>

#define ACP_ENV_MAX_LENGTH 1024

static acp_spin_lock_t gEnvGetLock = ACP_SPIN_LOCK_INITIALIZER(-1);
static acp_spin_lock_t gEnvSetLock = ACP_SPIN_LOCK_INITIALIZER(-1);


/**
 * get an environment variable
 *
 * @param aName the variable name to get
 * @param aValue the pointer to the memory buffer
 * to get the value of the environment variable
 * @return result code
 *
 * it returns #ACP_RC_ENOENT if there is no such environment variable
 */
ACP_EXPORT acp_rc_t acpEnvGet(const acp_char_t *aName, acp_char_t **aValue)
{
    acp_char_t*     sValue = NULL;
    acp_rc_t        sRC;

    /* Ensure thread-safety
     * Read-Lock */
    acpSpinLockLock(&gEnvGetLock);

    if (NULL == aName)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sValue = getenv(aName);

        if (sValue != NULL)
        {
            *aValue =  sValue;

            sRC = ACP_RC_SUCCESS;
        }
        else
        {
            sRC = ACP_RC_ENOENT;
        }
    }

    acpSpinLockUnlock(&gEnvGetLock);

    return sRC;
}


/**
 * set an environment variable
 *
 * @param aName the name of environment variable to set
 * @param aValue the value of environment variable to set
 * @a aValue becomes part of the environment. Since it does not copy
 * the string, changing it will change the environment.
 * NEVER pass an automatic variable as the argument.
 * When a program is terminated, it should free environment variables
 * that be set by itself.
 * @param aOverwrite whether the value of environment variable
 * should be replaced or not, if the environment variable already exists
 * @return result code
 */
ACP_EXPORT acp_rc_t acpEnvSet(const acp_char_t  *aName,
                              const acp_char_t  *aValue,
                              acp_bool_t   aOverwrite)
{
    acp_sint32_t      sRet;
    acp_rc_t          sRC;

    /* Ensure thread-safety 
     * Read and Write Lock */
    acpSpinLockLock(&gEnvGetLock);
    acpSpinLockLock(&gEnvSetLock);

#if defined(ALTI_CFG_OS_AIX)   ||                \
    defined(ALTI_CFG_OS_LINUX) ||                \
    defined(ALTI_CFG_OS_TRU64)
    sRet = setenv(aName, aValue, (acp_sint32_t)aOverwrite);

    if (sRet == 0)
    {
        sRC = ACP_RC_SUCCESS;
    }
    else
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
#else
    /*
     * BUGBUG
     *
     * 1. Check thread safety of putenv on each platform
     * 2. Some platform may cause memory leak when overwrite
     */
    if ((aOverwrite == ACP_TRUE) || (getenv(aName) == NULL))
    {
        acp_char_t *sBuffer = NULL;
        acp_size_t  sLen;

        sLen = acpCStrLen(aName, ACP_ENV_MAX_LENGTH) +
               acpCStrLen(aValue, ACP_ENV_MAX_LENGTH) + 2;
        sRC  = acpMemAlloc((void **)&sBuffer, sLen);

        if (ACP_RC_IS_SUCCESS(sRC))
        {
            sRC = acpSnprintf(sBuffer, sLen, "%s=%s", aName, aValue);

            if (ACP_RC_IS_SUCCESS(sRC))
            {
                sRet = putenv(sBuffer);

                if (sRet == 0)
                {
                    sRC = ACP_RC_SUCCESS;
                }
                else
                {
                    sRC = ACP_RC_GET_OS_ERROR();
                }
            }
            else
            {
                /* Do nothing
                 * We already know error code */
            }
# if defined(ALTI_CFG_OS_WINDOWS)
            acpMemFree(sBuffer);
# endif
        }
        else
        {
            /* Do nothing
             * We already know error code */
        }
    }
    else
    {
        sRC = ACP_RC_SUCCESS;
    }
#endif

    acpSpinLockUnlock(&gEnvSetLock);
    acpSpinLockUnlock(&gEnvGetLock);

    return sRC;
}


/**
 * List all environment variable
 */
ACP_EXPORT acp_rc_t acpEnvList(acp_char_t**     aEnv,
                               const acp_size_t aEnvSize,
                               acp_size_t*      aNoEnv)
{
    acp_size_t      sNoEnv;

#if defined(ALTI_CFG_OS_WINDOWS)
    acp_char_t*     sEnv = NULL;
    acp_uint32_t    sIndex = 0;

    acpSpinLockLock(&gEnvGetLock);
    sEnv = GetEnvironmentStrings();

    sNoEnv = 0;
    while(ACP_CSTR_TERM != sEnv[sIndex])
    {
        while(ACP_CSTR_TERM != sEnv[sIndex])
            sIndex++;

        if((NULL != aEnv) && (aEnvSize > sNoEnv))
        {
            aEnv[sNoEnv] = sEnv;
        }
        else
        {
            /* Do nothing */
        }

        sEnv += (sIndex + 1);
        sIndex = 0;
        sNoEnv++;
    }

    if(NULL != aNoEnv)
    {
        *aNoEnv = sNoEnv;
    }
    else
    {
        /* Do nothing */
    }

    acpSpinLockUnlock(&gEnvGetLock);

#else

#  if defined(ALTI_CFG_OS_SOLARIS) || defined(ALTI_CFG_OS_FREEBSD)
    extern acp_char_t** environ;
#  endif

    
    /* Ensure thread-safety
     * Read-Lock */
    acpSpinLockLock(&gEnvGetLock);

    sNoEnv = 0;
    while(NULL != environ[sNoEnv])
    {
        if((NULL != aEnv) && (aEnvSize > sNoEnv))
        {
            aEnv[sNoEnv] = environ[sNoEnv];
        }
        else
        {
            /* Do nothing */
        }

        sNoEnv++;
    }

    if(NULL != aNoEnv)
    {
        *aNoEnv = sNoEnv;
    }
    else
    {
        /* Do nothing */
    }

    acpSpinLockUnlock(&gEnvGetLock);
#endif
    ACP_TEST(aEnvSize < sNoEnv);
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_ETRUNC;
}

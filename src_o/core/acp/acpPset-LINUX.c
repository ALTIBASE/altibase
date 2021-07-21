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
 * $Id: acpPset-LINUX.c 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#include <acp.h>
#include <acpSys.h>
#include <acpPset.h>

/* for preventing warning message from native code*/
#if defined(ALINT)
#undef CPU_SET
#define ACP_LINUX_NATIVE_CPU_SET(a,b) ;
#undef CPU_ZERO
#define ACP_LINUX_NATIVE_CPU_ZERO(a) ;
#else
#define ACP_LINUX_NATIVE_CPU_SET(a,b)  CPU_SET((a), (b))
#define ACP_LINUX_NATIVE_CPU_ZERO(a) CPU_ZERO((a))
#endif

/* ------------------------------------------------
 *  Get CPU Toplogy Mask
 * ----------------------------------------------*/

static acp_rc_t acpPsetGetConfInit(acp_pset_t *aPset)
{
    acp_uint32_t sCpuCount;
    acp_rc_t     sRC;
    acp_uint32_t i;
    cpu_set_t    sLinuxOrgSet;
    acp_uint32_t sActualCpuCount = 0;

    sRC = acpSysGetCPUCount(&sCpuCount);

    if ( sRC != ACP_RC_SUCCESS)
    {
        return sRC;
    }
    else
    {
        /* no error in getting CPU count */
    }

    /* ------------------------------------------------
     *  Store the old affinity information to restore
     * ----------------------------------------------*/
    if (sched_getaffinity(0, sizeof(sLinuxOrgSet), &sLinuxOrgSet) == 0)
    {
        /* ok : save the current cpu set info */
    }
    else
    {
        /* error on set affinity */
        return ACP_RC_GET_OS_ERROR();
    }

    ACP_PSET_ZERO(aPset);

    /* ------------------------------------------------
     *  Get Actual CPU count by loop checking
     *  In linux, there is no api for determine the number of CPUs.
     *  so, we are binding each cpu to this process in order to know
     *  whether this number of cpu exist actually or not.
     * ----------------------------------------------*/
    for (i = 0; i < ACP_PSET_MAX_CPU; i++)
    {
        acp_pset_t sSet;
        cpu_set_t  sLinuxSet;

        ACP_PSET_ZERO(&sSet);

        ACP_LINUX_NATIVE_CPU_ZERO(&sLinuxSet);
        ACP_LINUX_NATIVE_CPU_SET(i, &sLinuxSet);

        if (sched_setaffinity(0, sizeof(sLinuxSet), &sLinuxSet) == 0)
        {
            /* success of bind : that means, exist this cpu */
            sActualCpuCount++;
            ACP_PSET_SET(aPset, i);

            if (sActualCpuCount == sCpuCount)
            {
                /* got all cpu number */
                break;
            }
            else
            {
                /* continue to get more cpu info */
            }

        }
        else
        {
            /* error on binding : this means there is no cpu number %i */
        }
    }

    if (sActualCpuCount != sCpuCount)
    {
        /* not mismatch between actual bind info and  acpSysGetCPUCount() */
        return ACP_RC_ENOENT;
    }
    else
    {
        /* information corrent */

        /* restore the cpu bind mask */
        if (sched_setaffinity(0, sizeof(sLinuxOrgSet), &sLinuxOrgSet) == 0)
        {
            /* success of restoring */
        }
        else
        {
            /* error on restoring affinity */
            return ACP_RC_GET_OS_ERROR();
        }
    }

    /* save pset to cache */
    gAcpPsetSysCache = (*aPset);

    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 * Various internal
 * ----------------------------------------------*/

static void acpPsetConvertPset2Native(const acp_pset_t *aSrc,
                                      cpu_set_t  *aDest)
{
    acp_uint32_t i;

    ACP_LINUX_NATIVE_CPU_ZERO(aDest);

    for (i = 0; (i < ACP_PSET_MAX_CPU) && (i < CPU_SETSIZE); i++)
    {
        if (ACP_PSET_ISSET(aSrc, i) != 0)
        {
            ACP_LINUX_NATIVE_CPU_SET(i, aDest);
        }
        else
        {
            /* no bit here, so we dont' have to add the bit to  cpu_set */
        }
    }
}

/**
 * bind cpusets to a current process.
 *
 * @param aPset CPUSET to be bound
 * @return result code
 */

ACP_EXPORT acp_rc_t acpPsetBindProcess(const acp_pset_t *aPset)
{
    /* convertion from acp_pset_t to linux cpu_set */
    cpu_set_t    sLinuxSet;

    acpPsetConvertPset2Native(aPset, &sLinuxSet);

    if (sched_setaffinity(0, sizeof(sLinuxSet), &sLinuxSet) != 0)
    {
        /* error on restoring affinity */
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* success of setaffinity*/
    }

    return ACP_RC_SUCCESS;
}

/**
 * bind cpusets to a current thread.
 *
 * @param aPset CPUSET to be bound
 * @return result code
 */

ACP_EXPORT acp_rc_t acpPsetBindThread(const acp_pset_t *aPset)
{
    /* convertion from acp_pset_t to linux cpu_set */
    cpu_set_t    sLinuxSet;

    acpPsetConvertPset2Native(aPset, &sLinuxSet);

    if (pthread_setaffinity_np(pthread_self(),
                               sizeof(sLinuxSet),
                               &sLinuxSet) != 0)
    {
        /* error on restoring affinity */
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* success of setaffinity*/
    }
    return ACP_RC_SUCCESS;
}


/**
 * unbind current process
 *
 * @return result code
 */
ACP_EXPORT acp_rc_t acpPsetUnbindProcess(void)
{
    return acpPsetBindProcess(&gAcpPsetSysCache);
}


/**
 * unbind current thread
 *
 * @return result code
 */
ACP_EXPORT acp_rc_t acpPsetUnbindThread(void)
{
    return acpPsetBindThread(&gAcpPsetSysCache);
}




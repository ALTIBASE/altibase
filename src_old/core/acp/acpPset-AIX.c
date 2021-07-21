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
 * $Id: acpPset-AIX.c 5245 2009-04-14 02:02:23Z jykim $
 ******************************************************************************/

#include <acp.h>
#include <acpSys.h>
#include <acpPset.h>
/* ------------------------------------------------
 *  Get CPU Toplogy Mask
 * ----------------------------------------------*/

static acp_rc_t acpPsetGetConfInit(acp_pset_t *aPset)
{
    acp_uint32_t    i;
    acp_uint32_t    sCpuCount;
    acp_rc_t        sRC;
    acp_uint32_t    sActualCpuCount = 0;

    sRC = acpSysGetCPUCount(&sCpuCount);

    if ( sRC != ACP_RC_SUCCESS)
    {
        return sRC;
    }
    else
    {
        /* no error in getting CPU count */
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

        ACP_PSET_ZERO(&sSet);

        if (bindprocessor(BINDTHREAD, thread_self(), i) == 0)
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
    }

    /* BUG-44084 The affinity should be unbound after getting configuration */
    sRC = acpPsetUnbindProcess();
    if ( sRC != ACP_RC_SUCCESS )
    {
        return sRC;
    }
    else
    {
        /* fall through */
    }

    /* save pset to cache */
    gAcpPsetSysCache = (*aPset);
    return ACP_RC_SUCCESS;
}

/* ------------------------------------------------
 *  Convert from CPU SET to Number
 *  ==> Will not support of multiple CPU sets
 * ----------------------------------------------*/

static acp_sint32_t acpPsetGetCPUNumber(const acp_pset_t *aSrc)
{
    acp_uint32_t i;
    acp_sint32_t sNumber = -1;

    for (i = 0; i < ACP_PSET_MAX_CPU; i++)
    {
        if (ACP_PSET_ISSET(aSrc, i) != 0)
        {
            if (sNumber == -1)
            {
                sNumber = (acp_sint32_t)i;
            }
            else
            {
                return -1; /* never support of multiple sets */
            }
        }
        else
        {
            /* no bit here, so we dont' have to add the bit to  cpu_set */
        }
    }
    return sNumber;
}

/* ------------------------------------------------
 * Bind Process
 * ----------------------------------------------*/

ACP_EXPORT acp_rc_t acpPsetBindProcess(const acp_pset_t *aPset)
{
    acp_sint32_t sRC;
    acp_sint32_t sNumber;

    sNumber = acpPsetGetCPUNumber(aPset);

    if (sNumber == -1)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        sRC = bindprocessor(BINDPROCESS, getpid(), (cpu_t)sNumber);

        if (sRC != 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
}

/* ------------------------------------------------
 * Bind  Thread
 * ----------------------------------------------*/

ACP_EXPORT acp_rc_t acpPsetBindThread(const acp_pset_t *aPset)
{
    acp_sint32_t  sRC;
    acp_sint32_t  sNumber;

    sNumber = acpPsetGetCPUNumber(aPset);

    if (sNumber == -1)
    {
        return ACP_RC_EINVAL;
    }
    else
    {
        /* not support by DEC.. */

        sRC = bindprocessor(BINDTHREAD, thread_self(), (cpu_t)sNumber);

        if (sRC != 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
}

/* ------------------------------------------------
 * Unbind process
 * ----------------------------------------------*/
ACP_EXPORT acp_rc_t acpPsetUnbindProcess(void)
{
    acp_sint32_t  sRC;

    sRC = bindprocessor(BINDPROCESS, getpid(), PROCESSOR_CLASS_ANY);

    if (sRC != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}


/* ------------------------------------------------
 * Unbind thread
 * ----------------------------------------------*/
ACP_EXPORT acp_rc_t acpPsetUnbindThread(void)
{
    acp_sint32_t  sRC;

    sRC = bindprocessor(BINDTHREAD, thread_self(), PROCESSOR_CLASS_ANY);

    if (sRC != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
}




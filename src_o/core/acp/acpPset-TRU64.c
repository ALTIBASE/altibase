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
 * $Id: acpPset-TRU64.c 5245 2009-04-14 02:02:23Z jykim $
 ******************************************************************************/

#include <acp.h>
#include <acpSys.h>
#include <acpPset.h>
/* ------------------------------------------------
 *  Get CPU Toplogy Mask
 * ----------------------------------------------*/

static acp_rc_t acpPsetGetConfInit(acp_pset_t *aPset)
{
    acp_uint32_t    sCpuCount;
    acp_rc_t        sRC;
    acp_uint32_t    sActualCpuCount = 0;
    struct cpu_info sInfo;
    acp_sint32_t    sStart;

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

    if (getsysinfo(GSI_CPU_INFO,
                   (caddr_t)&sInfo,
                   sizeof(sInfo),
                   &sStart,
                   NULL,
                   NULL) <= 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        acp_uint32_t i;
        ulong_t      sBit = 1;

        for (i = 0; i < sizeof(sInfo.cpus_present) * 8; i++)
        {
            if ( (sInfo.cpus_present & sBit) != 0)
            {
                ACP_PSET_SET(aPset, i);
                sActualCpuCount++;
            }
            else
            {
                /* no cpu info */
            }
            sBit <<= 1;
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

    /* save pset to cache */
    gAcpPsetSysCache = (*aPset);
    return ACP_RC_SUCCESS;
}


static void acpPsetConvertPset2Native(acp_pset_t     *aSrc,
                                      unsigned long  *aDest)
{
    acp_uint32_t i;
    unsigned long    sBit = 1;

    (*aDest) = 0;

    for (i = 0; (i < ACP_PSET_MAX_CPU) && (sizeof(unsigned long) * 8); i++)
    {
        if (ACP_PSET_ISSET(aSrc, i) != 0)
        {
            (*aDest) |= sBit;
        }
        else
        {
            /* no bit here, so we dont' have to add the bit to  cpu_set */
        }
        sBit <<= 1;
    }
}

/* ------------------------------------------------
 * Bind Process
 * ----------------------------------------------*/

ACP_EXPORT acp_rc_t acpPsetBindProcess(acp_pset_t *aPset)
{
    acp_sint32_t sRC;
    unsigned long sMask;

    acpPsetConvertPset2Native(aPset, &sMask);

    sRC = bind_to_cpu(getpid(), sMask, 0);

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
 * Bind  Thread
 * ----------------------------------------------*/

ACP_EXPORT acp_rc_t acpPsetBindThread(acp_pset_t *aPset)
{
    acp_uint32_t  i;
    acp_sint32_t  sRC;

    radset_t sRadset;

    sRC = radsetcreate(&sRadset);

    if (sRC != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = rademptyset(sRadset);
        if (sRC != 0)
        {
            (void)radsetdestroy(&sRadset);
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            for (i = 0; (i < ACP_PSET_MAX_CPU) && (sizeof(unsigned long) * 8); i++)
            {
                if (ACP_PSET_ISSET(aPset, i) != 0)
                {
                    sRC = radaddset(sRadset, i);

                    if (sRC != 0)
                    {
                        (void)radsetdestroy(&sRadset);
                        return ACP_RC_GET_OS_ERROR();
                    }
                    else
                    {
                        /* success of adding */
                    }
                }
                else
                {
                    /* none bits here */
                }
            } /*for */

            sRC = pthread_rad_bind(pthread_self(), sRadset, RAD_INSIST);
            if (sRC != 0)
            {
                (void)radsetdestroy(&sRadset);
                return ACP_RC_GET_OS_ERROR();
            }
            else
            {
                (void)radsetdestroy(&sRadset);
                return ACP_RC_SUCCESS;
            }
        }
    }
}

/* ------------------------------------------------
 * Unbind process
 * ----------------------------------------------*/
ACP_EXPORT acp_rc_t acpPsetUnbindProcess(void)
{
    return acpPsetBindProcess(&gAcpPsetSysCache);
}


/* ------------------------------------------------
 * Unbind thread
 * ----------------------------------------------*/
ACP_EXPORT acp_rc_t acpPsetUnbindThread(void)
{
    return acpPsetBindThread(&gAcpPsetSysCache);
}




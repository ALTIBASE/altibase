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
 * $Id: acpPset-WINDOWS.c 5245 2009-04-14 02:02:23Z jykim $
 ******************************************************************************/

#include <acp.h>
#include <acpSys.h>
#include <acpPset.h>

/* ------------------------------------------------
 * Various internal
 * ----------------------------------------------*/

static void acpPsetConvertPset2Native(acp_pset_t *aSrc,
                                      DWORD_PTR  *aDest)
{
    acp_uint32_t i;
    DWORD_PTR    sBit = 1;

    (*aDest) = 0;

    for (i = 0; (i < ACP_PSET_MAX_CPU) && (sizeof(DWORD_PTR) * 8); i++)
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

static void acpPsetConvertNative2Pset(DWORD_PTR  *aDest,
                                      acp_pset_t *aSrc)
{
    acp_uint32_t i;

    ACP_PSET_ZERO(aSrc);

    for (i = 0; i < sizeof(DWORD_PTR) * 8; i++)
    {
        DWORD_PTR sBit;

        sBit = ((DWORD_PTR)1) << i;

        if ( ((*aDest) & sBit) != 0)
        {
            ACP_PSET_SET(aSrc, i);
        }
        else
        {
            /* no bit here, so we dont' have to add the bit to  cpu_set */
        }
    }
}

/* ------------------------------------------------
 *  Get CPU Toplogy Mask
 * ----------------------------------------------*/

static acp_rc_t acpPsetGetConfInit(acp_pset_t *aPset)
{
    acp_uint32_t sCpuCount;
    acp_rc_t     sRC;
    acp_uint32_t sActualCpuCount = 0;
    DWORD_PTR    sProcMask;
    DWORD_PTR    sSysMask;

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
    if (GetProcessAffinityMask(GetCurrentProcess(),
                               &sProcMask,
                               &sSysMask) != 0)
    {
        /* convert mask to acp_pset_t */
        acpPsetConvertNative2Pset(&sSysMask, aPset);
    }
    else
    {
        return ACP_RC_GET_OS_ERROR();
    }


    sActualCpuCount = (acp_uint32_t)ACP_PSET_COUNT(aPset);

    if (sActualCpuCount != sCpuCount)
    {
        /* not mismatch between actual bind info and  acpSysGetCPUCount() */
        return ACP_RC_ENOENT;
    }
    else
    {
        /* save pset to cache */
        gAcpPsetSysCache = *(aPset);

        return ACP_RC_SUCCESS;
    }
}

/* ------------------------------------------------
 * Bind Process
 * ----------------------------------------------*/

ACP_EXPORT acp_rc_t acpPsetBindProcess(acp_pset_t *aPset)
{
    /* convertion from acp_pset_t to linux cpu_set */
    DWORD_PTR    sWindowsSet;

    acpPsetConvertPset2Native(aPset, &sWindowsSet);

    if (SetProcessAffinityMask(GetCurrentProcess(), sWindowsSet) == 0)
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

/* ------------------------------------------------
 * Bind  Thread
 * ----------------------------------------------*/

ACP_EXPORT acp_rc_t acpPsetBindThread(acp_pset_t *aPset)
{
    /* convertion from acp_pset_t to linux cpu_set */
    DWORD_PTR    sWindowsSet;

    acpPsetConvertPset2Native(aPset, &sWindowsSet);

    if (SetThreadAffinityMask(GetCurrentThread(), sWindowsSet) == 0)
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




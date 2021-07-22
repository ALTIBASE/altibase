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
 * $Id: acpInit.c 1448 2007-12-13 20:15:21Z shsuh $
 ******************************************************************************/

#include <acpInit.h>
#include <acpOS.h>
#include <acpSignal.h>
#include <acpSock.h>
#include <acpSpinWait.h>
#include <acpSys.h>
#include <acpThr.h>


static acp_thr_once_t gAcpInitControl = ACP_THR_ONCE_INITIALIZER;
static acp_rc_t       gAcpInitRC      = ACP_RC_EINVAL;


static void acpInitOnce(void)
{
    acp_uint32_t sCPUCount;
    acp_rc_t     sRC;

#if defined(ALTI_CFG_OS_WINDOWS)
    sRC = acpOSGetVersionInfo();

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        gAcpInitRC = sRC;

        return;
    }
    else
    {
        /* do nothing */
    }

    sRC = acpSockInitialize();

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        gAcpInitRC = sRC;

        return;
    }
    else
    {
        /* do nothing */
    }
#else
    acpSignalGetDefaultBlockSet();
#endif

    sRC = acpSysGetCPUCount(&sCPUCount);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        gAcpInitRC = sRC;

        return;
    }
    else
    {
        if (sCPUCount > 1)
        {
            acpSpinWaitSetDefaultSpinCount(
                ACP_SPIN_WAIT_DEFAULT_SPIN_COUNT);
        }
        else
        {
            acpSpinWaitSetDefaultSpinCount(0);
        }
    }

    gAcpInitRC = ACP_RC_SUCCESS;
}

/**
 * initializes Altibase Core Porting layer
 * @return result code
 */
ACP_EXPORT acp_rc_t acpInitialize(void)
{
    (void)acpThrOnce(&gAcpInitControl, acpInitOnce);

    return gAcpInitRC;
}

/**
 * finalizes Altibase Core Porting layer
 * @return result code
 */
ACP_EXPORT acp_rc_t acpFinalize(void)
{
    return ACP_RC_SUCCESS;
}

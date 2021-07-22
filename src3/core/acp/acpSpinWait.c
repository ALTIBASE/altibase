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
 * $Id: acpSpinWait.c 3355 2008-10-21 07:56:38Z djin $
 ******************************************************************************/

#include <acpSpinWait.h>


static acp_sint32_t gAcpSpinWaitDefaultSpinCount =
    ACP_SPIN_WAIT_DEFAULT_SPIN_COUNT;

/*
 * Get Spin Counter
 */
ACP_EXPORT acp_sint32_t acpSpinWaitGetDefaultSpinCount(void)
{
    return gAcpSpinWaitDefaultSpinCount;
}

/*
 * Set Spin Counter
 */
ACP_EXPORT void acpSpinWaitSetDefaultSpinCount(acp_sint32_t aSpinCount)
{
    gAcpSpinWaitDefaultSpinCount = aSpinCount;
}

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
 * $Id: libtestAcpDl.c 11316 2010-06-22 23:56:42Z djin $
 ******************************************************************************/

#include <acpMem.h>
#include <acpError.h>

/*
 * Not Implemented
 */
ACP_EXPORT acp_rc_t libtestAcpDlFunc1(void)
{
    return ACP_RC_ENOIMPL;
}

/*
 * Internal test function
 */
ACP_EXPORT acp_rc_t libtestAcpDlFunc2(acp_bool_t aFlag)
{
    if (aFlag == ACP_TRUE)
    {
        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_ENOTSUP;
    }
}

/*
 * Internal test function
 */
ACP_EXPORT acp_rc_t libtestAcpDlAlloc(void** aPtr)
{
    return acpMemAlloc(aPtr, 1024);
}

/*
 * Internal test function
 */
ACP_EXPORT void libtestAcpDlFree(void* aPtr)
{
    acpMemFree(aPtr);
}


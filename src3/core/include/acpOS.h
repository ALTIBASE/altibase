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
 * $Id: acpOS.h 3401 2008-10-28 04:26:42Z sjkim $
 ******************************************************************************/

#if !defined(_O_ACP_OS_H_)
#define _O_ACP_OS_H_

#include <acpError.h>


#if defined(ALTI_CFG_OS_WINDOWS)


/*
 * to check dwPlatformId in OSVERSIONINFO
 *
 * for VER_PLATFORM_WIN32_NT      : acpOSGetVersion() >= ACP_OS_WIN_NT
 * for VER_PLATFORM_WIN32_WINDOWS : acpOSGetVersion() < ACP_OS_WIN_NT
 */

typedef enum acp_os_ver_t
{
    ACP_OS_WIN_UNK      =  0,
    ACP_OS_WIN_UNSUP    =  1,
    ACP_OS_WIN_95       = 10,
    ACP_OS_WIN_95_B     = 11,
    ACP_OS_WIN_95_OSR2  = 12,
    ACP_OS_WIN_98       = 14,
    ACP_OS_WIN_98_SE    = 16,
    ACP_OS_WIN_ME       = 18,

    ACP_OS_WIN_UNICODE  = 20, /* Prior versions support only narrow chars */

    ACP_OS_WIN_CE_3     = 23, /* CE is an odd beast, not supporting       */
                              /* some pre-NT features, such as the        */
                              /* narrow charset APIs (fooA fns), while    */
                              /* not supporting some NT-family features.  */

    ACP_OS_WIN_NT       = 30,
    ACP_OS_WIN_NT_3_5   = 35,
    ACP_OS_WIN_NT_3_51  = 36,

    ACP_OS_WIN_NT_4     = 40,
    ACP_OS_WIN_NT_4_SP2 = 42,
    ACP_OS_WIN_NT_4_SP3 = 43,
    ACP_OS_WIN_NT_4_SP4 = 44,
    ACP_OS_WIN_NT_4_SP5 = 45,
    ACP_OS_WIN_NT_4_SP6 = 46,

    ACP_OS_WIN_2000     = 50,
    ACP_OS_WIN_2000_SP1 = 51,
    ACP_OS_WIN_2000_SP2 = 52,
    ACP_OS_WIN_XP       = 60,
    ACP_OS_WIN_XP_SP1   = 61,
    ACP_OS_WIN_XP_SP2   = 62,
    ACP_OS_WIN_2003     = 70,
    ACP_OS_WIN_VISTA    = 80
} acp_os_ver_t;


ACP_EXPORT acp_rc_t acpOSGetVersionInfo(void);

ACP_EXPORT acp_os_ver_t acpOSGetVersion(void);


#endif


#endif

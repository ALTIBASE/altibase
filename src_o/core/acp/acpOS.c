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
 * $Id: acpOS.c 3468 2008-11-04 00:53:17Z jykim $
 ******************************************************************************/

#include <acpChar.h>
#include <acpOS.h>
#include <acpProc.h>
#include <acpStr.h>


#if defined(ALTI_CFG_OS_WINDOWS)


static acp_os_ver_t gAcpOsVersion = ACP_OS_WIN_UNK;


ACP_EXPORT acp_rc_t acpOSGetVersionInfo(void)
{
    OSVERSIONINFO sVersionInfo;
    BOOL          sRet;

    sVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    sRet = GetVersionEx(&sVersionInfo);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    if (sVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        acp_uint32_t  sServicePack = 0;
        acp_char_t   *sPtr         = sVersionInfo.szCSDVersion;;

        if (sPtr != 0)
        {
            while ((*sPtr != 0) && (acpCharIsDigit(*sPtr) != ACP_TRUE))
            {
                sPtr++;
            }

            if (*sPtr != 0)
            {
                acp_str_t    sStr = ACP_STR_CONST(sPtr);
                acp_sint32_t sSign;
                acp_uint64_t sNum;
                acp_rc_t     sRC;

                sRC = acpStrToInteger(&sStr, &sSign, &sNum, NULL, 0, 0);

                if (ACP_RC_IS_SUCCESS(sRC))
                {
                    sServicePack = (acp_sint32_t)sNum * sSign;
                }
                else
                {
                    sServicePack = 0;
                }
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }

        if (sVersionInfo.dwMajorVersion < 3)
        {
            gAcpOsVersion = ACP_OS_WIN_UNSUP;
        }
        else if (sVersionInfo.dwMajorVersion == 3)
        {
            if (sVersionInfo.dwMajorVersion < 50)
            {
                gAcpOsVersion = ACP_OS_WIN_UNSUP;
            }
            else if (sVersionInfo.dwMajorVersion == 50)
            {
                gAcpOsVersion = ACP_OS_WIN_NT_3_5;
            }
            else
            {
                gAcpOsVersion = ACP_OS_WIN_NT_3_51;
            }
        }
        else if (sVersionInfo.dwMajorVersion == 4)
        {
            if (sServicePack < 2)
            {
                gAcpOsVersion = ACP_OS_WIN_NT_4;
            }
            else if (sServicePack <= 2)
            {
                gAcpOsVersion = ACP_OS_WIN_NT_4_SP2;
            }
            else if (sServicePack <= 3)
            {
                gAcpOsVersion = ACP_OS_WIN_NT_4_SP3;
            }
            else if (sServicePack <= 4)
            {
                gAcpOsVersion = ACP_OS_WIN_NT_4_SP4;
            }
            else if (sServicePack <= 5)
            {
                gAcpOsVersion = ACP_OS_WIN_NT_4_SP5;
            }
            else
            {
                gAcpOsVersion = ACP_OS_WIN_NT_4_SP6;
            }
        }
        else if (sVersionInfo.dwMajorVersion == 5)
        {
            if (sVersionInfo.dwMinorVersion == 0)
            {
                if (sServicePack == 0)
                {
                    gAcpOsVersion = ACP_OS_WIN_2000;
                }
                else if (sServicePack == 1)
                {
                    gAcpOsVersion = ACP_OS_WIN_2000_SP1;
                }
                else
                {
                    gAcpOsVersion = ACP_OS_WIN_2000_SP2;
                }
            }
            else if (sVersionInfo.dwMinorVersion == 2)
            {
                gAcpOsVersion = ACP_OS_WIN_2003;
            }
            else
            {
                if (sServicePack < 1)
                {
                    gAcpOsVersion = ACP_OS_WIN_XP;
                }
                else if (sServicePack == 1)
                {
                    gAcpOsVersion = ACP_OS_WIN_XP_SP1;
                }
                else
                {
                    gAcpOsVersion = ACP_OS_WIN_XP_SP2;
                }
            }
        }
        else if (sVersionInfo.dwMajorVersion == 6)
        {
            gAcpOsVersion = ACP_OS_WIN_VISTA;
        }
        else
        {
            gAcpOsVersion = ACP_OS_WIN_XP;
        }
    }
    else if (sVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        acp_char_t *sPtr = sVersionInfo.szCSDVersion;

        if (sPtr != 0)
        {
            while ((*sPtr != 0) && (acpCharIsUpper(*sPtr) != ACP_TRUE))
            {
                sPtr++;
            }
        }
        else
        {
            sPtr = "";
        }

        if (sVersionInfo.dwMinorVersion < 10)
        {
            if (*sPtr < 'C')
            {
                gAcpOsVersion = ACP_OS_WIN_95;
            }
            else
            {
                gAcpOsVersion = ACP_OS_WIN_95_OSR2;
            }
        }
        else if (sVersionInfo.dwMinorVersion < 90)
        {
            if (*sPtr < 'A')
            {
                gAcpOsVersion = ACP_OS_WIN_98;
            }
            else
            {
                gAcpOsVersion = ACP_OS_WIN_98_SE;
            }
        }
        else
        {
            gAcpOsVersion = ACP_OS_WIN_ME;
        }
    }
#if defined(ALTI_CFG_OS_WINCE)
    else if (sVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_CE)
    {
        if (sVersionInfo.dwMajorVersion < 3)
        {
            gAcpOsVersion = ACP_OS_WIN_UNSUP;
        }
        else
        {
            gAcpOsVersion = ACP_OS_WIN_CE_3;
        }
    }
#endif
    else
    {
        gAcpOsVersion = ACP_OS_WIN_UNSUP;
    }

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_os_ver_t acpOSGetVersion(void)
{
    return gAcpOsVersion;
}


#endif

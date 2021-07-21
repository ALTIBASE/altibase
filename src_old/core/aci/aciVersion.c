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
 
/*****************************************************************************
 * $Id: aciVersion.c 11106 2010-05-20 11:06:29Z gurugio $
 ****************************************************************************/

#include <acp.h>
#include <aciVersion.h>
#include <aciTypes.h>


/* ------------------------------------------------------------------------------
 *
 *           VERSION을 아래의 매크로에서 수정하기 바람.
 *
 *  -> example)  버젼이 9.2.123일 경우
 *
 *               #define IDU_ALTIBASE_MAJOR_VERSION    9
 *               #define IDU_ALTIBASE_MINOR_VERSION    2
 *               #define IDU_ALTIBASE_PATCH_LEVEL    123
 *
 * -----------------------------------------------------------------------------*/

#define MKHEX2(a)       (unsigned int)(((a / 10) * 16) + (a % 10))
#define MKHEX4(a)       (unsigned int)((((a % 10000) / 1000) * 16 * 16 * 16) + (( (a % 1000) / 100) * 16 * 16) + (( (a % 100) / 10) * 16) + (a % 10))

/* ------------------------------------------------
 *  For Altibase - 4
 * ----------------------------------------------*/

/* fix BUG-20140 */
const acp_char_t *aciVersionString = ACI_ALTIBASE_VERSION_STRING;

#if defined(ALTI_CFG_OS_SOLARIS) && (ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR == 5)
#define __MajorVersion__ (MKHEX2(ACI_ALTIBASE_MAJOR_VERSION) << 24)
#define __MinorVersion__ (MKHEX2(ACI_ALTIBASE_MINOR_VERSION) << 16)
#define __PatchLevel__   (MKHEX4(ACI_ALTIBASE_PATCH_LEVEL))
#else
#define __MajorVersion__ (MKHEX2(ACI_ALTIBASE_MAJOR_VERSION) << 24)
#define __MinorVersion__ (MKHEX2(ACI_ALTIBASE_MINOR_VERSION) << 16)
#define __PatchLevel__   (MKHEX4(ACI_ALTIBASE_PATCH_LEVEL))
#endif

const acp_uint32_t   aciVersionID = __MajorVersion__ + __MinorVersion__ + __PatchLevel__;

const acp_char_t *aciCopyRightString = "(c) Copyright 2001 ALTIBase Corporation.  All rights reserved.";

const acp_uint32_t   aciCompileBit      =
#if defined(ALTI_CFG_BITTYPE_64)
                   64;
#else
                   32;
#endif


/* ------------------------------------------------
 *  Production TimeStamp Creation
 * ----------------------------------------------*/

static acp_char_t  aciProductionString[128];
static acp_char_t  aciSystemInfoString[ACI_SYSTEM_INFO_LENGTH];
static acp_char_t  aciPackageInfoString[ACI_SYSTEM_INFO_LENGTH];

ACP_EXPORT
acp_sint32_t aciMkProductionTimeString()
{
    acpSnprintf(aciProductionString, ACI_SIZEOF(aciProductionString), "%s %s",
                __DATE__, __TIME__);
    return 0;
}

ACP_EXPORT
acp_sint32_t aciMkSystemInfoString()
{
    acp_sint32_t sAvailableLen;
    acp_char_t sCompilerVer[16];
        
#if defined (ALTI_CFG_OS_WINDOWS)
    if (_MSC_VER == 1000)
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer), "4.0", 3);
    }
    else if (_MSC_VER == 1100)
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer), "5.0", 3);
    }
    else if (_MSC_VER == 1200)
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer), "6.0", 3);
    }
    else if (_MSC_VER == 1300)
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer), "7.0", 3);
    }
    else if (_MSC_VER == 1400)
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer), "8.0", 3);
    }
    else
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer), "-unknown_ver", 7);
    }
#else
# if defined(__GNUC__)
    if (acpCStrCmp(ACP_CFG_COMPILER_STR, "gcc", 3) == 0)
    {
        acpSnprintf(sCompilerVer, sizeof(sCompilerVer), "%d.%d.%d",
                    __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer),
                   "-unknown_ver", sizeof("-unknown_ver"));
    }
    else
    {
        acpCStrCpy(sCompilerVer, sizeof(sCompilerVer),
                   "-unknown_ver", sizeof("-unknown_ver"));
    }
# else
    acpCStrCpy(sCompilerVer, sizeof(sCompilerVer),
            "-unknown_ver", sizeof("-unknown_ver"));
# endif
#endif

    /*
     * NOTICE: PROJ-1000
     * String format is changed at PROJ-1000
     */
    acpSnprintf(aciPackageInfoString, ACI_SIZEOF(aciPackageInfoString),
                "%s_"
                "%d.%d"
                "-%dbit"
                "-%s"
                "-%s"
                "-%s%s",
                ALTI_CFG_OS,
                ALTI_CFG_OS_MAJOR, ALTI_CFG_OS_MINOR,
                aciCompileBit,
                aciVersionString,
                ACP_CFG_BUILDMODEFULL,
                ACP_CFG_COMPILER_STR, sCompilerVer);

    acpCStrCpy(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH,
               aciPackageInfoString, acpCStrLen(aciPackageInfoString, ACI_SYSTEM_INFO_LENGTH));

    /*     아래 코드는 너무 이상하게 보일 수도 있겠지만 */
    /* klocworks 에러를 없애기 위해 부득이하게 이런식으로 작성했다. */
    sAvailableLen = sizeof(aciSystemInfoString) - acpCStrLen(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH) - 1;
    if (sAvailableLen > 0)
    {
        acpCStrCat(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH,
                   " (", sAvailableLen);
        sAvailableLen = sizeof(aciSystemInfoString) - acpCStrLen(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH) - 1;
        if (sAvailableLen > 0)
        {
            acpCStrCat(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH,
                       ALTI_CFG_OS, sAvailableLen);
            sAvailableLen = sizeof(aciSystemInfoString) - acpCStrLen(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH) - 1;
            if (sAvailableLen > 0)
            {
                acpCStrCat(aciSystemInfoString, ACI_SYSTEM_INFO_LENGTH,
                           ")", sAvailableLen);
            }
        }
    }

    return 0;
}


/* ------------------------------------------------
 *  Version Information API
 * ----------------------------------------------*/

ACP_EXPORT
const acp_char_t *aciGetVersionString()
{
    return aciVersionString;
}

ACP_EXPORT
const acp_char_t *aciGetCopyRightString()
{
    return aciCopyRightString;
}

/* "(c) Copyright 2001 ALTIBase Corporation.  All rights reserved." */
ACP_EXPORT
const acp_char_t *aciGetProductionTimeString()
{
    return aciProductionString;
}

/* "SPARC_SOLARIS2.7-compat5-32bit(sparc-sun-solaris2.7)" */
ACP_EXPORT
const acp_char_t *aciGetSystemInfoString()
{
    return aciSystemInfoString;
}

ACP_EXPORT
const acp_char_t *aciGetPackageInfoString()
{
    return aciPackageInfoString;
}


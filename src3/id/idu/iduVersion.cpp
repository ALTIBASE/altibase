/*****************************************************************************
 * Copyright 1999-2000, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduVersion.cpp 68602 2015-01-23 00:13:11Z sbjang $
 ****************************************************************************/

#include <idl.h>
#include <iduVersion.h>

/* ------------------------------------------------------------------------------
 *
 *           VERSION을 아래의 매크로에서 수정하기 바람.
 *
 *  -> example)  버젼이 9.2.123일 경우
 *
 *               #define IDU_ALTIBASE_MAJOR_VERSION    9
 *               #define IDU_ALTIBASE_MINOR_VERSION    2
 *               #define IDU_ALTIBASE_PATCHSET_LEVEL    123
 *
 * -----------------------------------------------------------------------------*/

#define MKHEX2(a)       (unsigned int)(((a / 10) * 16) + (a % 10))
#define MKHEX4(a)       (unsigned int)((((a % 10000) / 1000) * 16 * 16 * 16) + (( (a % 1000) / 100) * 16 * 16) + (( (a % 100) / 10) * 16) + (a % 10))

/* ------------------------------------------------
 *  For Altibase - 4
 * ----------------------------------------------*/

#define IDU_VERSION_STRING(a,b,c)  #a"."#b"."#c

#define IDU_SHM_MAJOR_VERSION   6
// PROJ-1557 32K memmory varchar
// PROJ-1362 LOB
#define IDU_SHM_MINOR_VERSION   1
#define IDU_SHM_PATCH_LEVEL     0

// fix BUG-20140
const SChar *iduVersionString = IDU_ALTIBASE_VERSION_STRING;
const SChar *iduShmVersionString = IDU_VERSION_STRING( 1, 0, 0 );

#if defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
UInt __MajorVersion__ = MKHEX2(IDU_ALTIBASE_MAJOR_VERSION) << 24;
UInt __MinorVersion__ = MKHEX2(IDU_ALTIBASE_MINOR_VERSION) << 16;
UInt __PatchLevel__   = MKHEX4(IDU_ALTIBASE_PATCHSET_LEVEL);

UInt __SHM_MajorVersion__ = MKHEX2(IDU_SHM_MAJOR_VERSION) << 24;
UInt __SHM_MinorVersion__ = MKHEX2(IDU_SHM_MINOR_VERSION) << 16;
UInt __SHM_PatchLevel__   = MKHEX4(IDU_SHM_PATCH_LEVEL);
#else
const UInt __MajorVersion__ = MKHEX2(IDU_ALTIBASE_MAJOR_VERSION) << 24;
const UInt __MinorVersion__ = MKHEX2(IDU_ALTIBASE_MINOR_VERSION) << 16;
const UInt __PatchLevel__   = MKHEX4(IDU_ALTIBASE_PATCHSET_LEVEL);

const UInt __SHM_MajorVersion__ = MKHEX2(IDU_SHM_MAJOR_VERSION) << 24;
const UInt __SHM_MinorVersion__ = MKHEX2(IDU_SHM_MINOR_VERSION) << 16;
const UInt __SHM_PatchLevel__   = MKHEX4(IDU_SHM_PATCH_LEVEL);
#endif

const UInt   iduVersionID = __MajorVersion__ + __MinorVersion__ + __PatchLevel__;
const UInt   iduShmVersionID = __SHM_MajorVersion__ + __SHM_MinorVersion__ + __SHM_PatchLevel__;

const SChar *iduCopyRightString = "(c) Copyright 2001 ALTIBase Corporation.  All rights reserved.";

const UInt   iduCompileBit      =
#ifdef COMPILE_64BIT
                   64;
#else
                   32;
#endif

const idBool iduBigEndian =
#if defined(ENDIAN_IS_BIG_ENDIAN)
    ID_TRUE;
#else
    ID_FALSE;
#endif

/* ------------------------------------------------
 *  Production TimeStamp Creation
 * ----------------------------------------------*/

static SChar  iduProductionString[128];
static SChar  iduSystemInfoString[IDU_SYSTEM_INFO_LENGTH];
static SChar  iduPackageInfoString[IDU_SYSTEM_INFO_LENGTH];

static SInt iduMkProductionTimeString()
{
    idlOS::snprintf(iduProductionString, ID_SIZEOF(iduProductionString), "%s %s",
                    __DATE__, __TIME__);
    return 0;
}

static SInt iduMkSystemInfoString()
{
    SInt sAvailableLen;

    idlOS::snprintf(iduPackageInfoString, ID_SIZEOF(iduPackageInfoString),
                    "%s_"
#if defined(OS_LINUX_KERNEL)
                    "%s_%s"
#else
                    "%d.%d"
#endif
                    "-%dbit"
#if defined(SPARC_SOLARIS)
                    "-compat%s"
#endif
                    "-%s"
                    "-%s"
                    "-%s%s",
                    OS_TARGET,
#if defined(OS_LINUX_KERNEL)
                    OS_LINUX_PACKAGE,
                    OS_LINUX_VERSION,
#else
                    OS_MAJORVER,
                    OS_MINORVER,
#endif
                    iduCompileBit,
#if defined(SPARC_SOLARIS)
                    OS_COMPAT_MODE,
#endif
                    iduVersionString,
                    BUILD_MODE,
                    COMPILER_NAME,
                    GCC_VERSION);
    idlOS::strcpy(iduSystemInfoString, iduPackageInfoString);

    // 아래 코드는 너무 이상하게 보일 수도 있겠지만
    // klocworks 에러를 없애기 위해 부득이하게 이런식으로 작성했다.
    sAvailableLen = sizeof(iduSystemInfoString) - idlOS::strlen(iduSystemInfoString) - 1;
    if (sAvailableLen > 0)
    {
        idlOS::strncat(iduSystemInfoString, " (", sAvailableLen);
        sAvailableLen = sizeof(iduSystemInfoString) - idlOS::strlen(iduSystemInfoString) - 1;
        if (sAvailableLen > 0)
        {
            idlOS::strncat(iduSystemInfoString, OS_SYSTEM_TYPE, sAvailableLen);
            sAvailableLen = sizeof(iduSystemInfoString) - idlOS::strlen(iduSystemInfoString) - 1;
            if (sAvailableLen > 0)
            {
                idlOS::strncat(iduSystemInfoString, ")", sAvailableLen);
            }
        }
    }

    return 0;
}

static SInt   idu_static_runner1 = iduMkProductionTimeString();
static SInt   idu_static_runner2 = iduMkSystemInfoString();

/* ------------------------------------------------
 *  Version Information API
 * ----------------------------------------------*/

const SChar *iduGetVersionString()
{
    return iduVersionString;
}

const SChar *iduGetCopyRightString()
{
    return iduCopyRightString;
}

//"(c) Copyright 2001 ALTIBase Corporation.  All rights reserved."
const SChar *iduGetProductionTimeString()
{
    return iduProductionString;
}

//"SPARC_SOLARIS2.7-compat5-32bit(sparc-sun-solaris2.7)"
const SChar *iduGetSystemInfoString()
{
    return iduSystemInfoString;
}

const SChar *iduGetPackageInfoString()
{
    return iduPackageInfoString;
}


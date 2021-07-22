/*****************************************************************************
 * Copyright 1999-2000, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduVersion.h 68602 2015-01-23 00:13:11Z sbjang $
 ****************************************************************************/

#ifndef _O_IDU_VERSION_H_
#define _O_IDU_VERSION_H_ 1

#include <iduVersionDef.h>

#if defined(SPARC_SOLARIS) && (OS_MAJORVER == 2) && (OS_MINORVER == 5)
extern UInt __MajorVersion__;
extern UInt __MinorVersion__;
extern UInt __PatchLevel__;
#else
extern const UInt __MajorVersion__;
extern const UInt __MinorVersion__;
extern const UInt __PatchLevel__;
#endif

extern const UInt    iduVersionID;
extern const UInt    iduShmVersionID;
extern const SChar * iduVersionString;
extern const SChar * iduShmVersionString;
extern const SChar * iduCopyRightString;

extern const UInt   iduCompileBit;
extern const idBool iduBigEndian;

const SChar *iduGetProductionTimeString();
const SChar *iduGetVersionString();
const SChar *iduGetCopyRightString();
const SChar *iduGetSystemInfoString();
const SChar *iduGetPackageInfoString();

#endif

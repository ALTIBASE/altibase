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
 * $Id: aciVersion.h 11319 2010-06-23 02:39:42Z djin $
 ****************************************************************************/

#ifndef _O_ACI_VERSION_H_
#define _O_ACI_VERSION_H_ 1

#include <acp.h>

ACP_EXTERN_C_BEGIN

/*  fix BUG-20140  */
#define ACI_ALTIBASE_MAJOR_VERSION   5
#define ACI_ALTIBASE_MINOR_VERSION   3
#define ACI_ALTIBASE_DEV_VERSION     4
#define ACI_ALTIBASE_PATCH_LEVEL     0

#if defined(COMPILE_FOR_NO_SMP)
#define ACI_ALTIBASE_VERSION_STRING  "5.3.4.0-nosmp"
#else
#define ACI_ALTIBASE_VERSION_STRING  "5.3.4.0"
#endif

#define ACI_SYSTEM_INFO_LENGTH      512

#define ACI_VERSION_CHECK_MASK      (0xFFFF0000)
#define ACI_MAJOR_VERSION_MASK      (0xFF000000)
#define ACI_MINOR_VERSION_MASK      (0x00FF0000)
#define ACI_PATCH_VERSION_MASK      (0x0000FFFF)


#if defined(ALTI_CFG_OS_SOLARIS) && (ALTI_CFG_OS_MAJOR == 2) && (ALTI_CFG_OS_MINOR == 5)
extern acp_uint32_t __MajorVersion__;
extern acp_uint32_t __MinorVersion__;
extern acp_uint32_t __PatchLevel__;
#else
extern const acp_uint32_t __MajorVersion__;
extern const acp_uint32_t __MinorVersion__;
extern const acp_uint32_t __PatchLevel__;
#endif

extern const acp_uint32_t aciVersionID;
extern const acp_char_t *aciVersionString;
extern const acp_char_t *aciCopyRightString;

extern const acp_uint32_t   aciCompileBit;

ACP_EXPORT acp_sint32_t aciMkProductionTimeString(void);
ACP_EXPORT acp_sint32_t aciMkSystemInfoString(void);

ACP_EXPORT const acp_char_t *aciGetProductionTimeString();
ACP_EXPORT const acp_char_t *aciGetVersionString();
ACP_EXPORT const acp_char_t *aciGetCopyRightString();
ACP_EXPORT const acp_char_t *aciGetSystemInfoString();
ACP_EXPORT const acp_char_t *aciGetPackageInfoString();

ACP_EXTERN_C_END

#endif

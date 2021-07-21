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
 * $Id: iduVersionDef.h $
 ****************************************************************************/

#include <idConfig.h>

#ifndef _O_IDU_VERSIONDEF_H_
#define _O_IDU_VERSIONDEF_H_ 1

/* BUG-41502 */

#if defined(ALTIBASE_PRODUCT_HDB)
/* HDB Version Info */
# define IDU_ALTIBASE_MAJOR_VERSION   7
# define IDU_ALTIBASE_MINOR_VERSION   3
# define IDU_ALTIBASE_DEV_VERSION     0
# define IDU_ALTIBASE_PATCHSET_LEVEL  0
# define IDU_ALTIBASE_PATCH_LEVEL     0

/* BUG-48106 */
#if defined(IBM_AIX) 
   #if (OS_MAJORVER >= 5) && defined(ALTIBASE_HEAPMIN)
      #if defined(COMPILE_FOR_NO_SMP)
      #define IDU_ALTIBASE_VERSION_STRING  "7.3.0.0.0-nosmp"
      #else
      #define IDU_ALTIBASE_VERSION_STRING "7.3.0.0.0"
      #endif
   #else
      #if defined(COMPILE_FOR_NO_SMP)
      #define IDU_ALTIBASE_VERSION_STRING  "7.3.0.0.0-nosmp-noheapmin"
      #else
      #define IDU_ALTIBASE_VERSION_STRING "7.3.0.0.0-noheapmin"
      #endif
   #endif
# else
   #if defined(COMPILE_FOR_NO_SMP)
   #define IDU_ALTIBASE_VERSION_STRING  "7.3.0.0.0-nosmp"
   #else
   #define IDU_ALTIBASE_VERSION_STRING "7.3.0.0.0"
   #endif
#endif

#else
/* XDB Version Info */
# define IDU_ALTIBASE_MAJOR_VERSION   6
# define IDU_ALTIBASE_MINOR_VERSION   7
# define IDU_ALTIBASE_DEV_VERSION     1
# define IDU_ALTIBASE_PATCHSET_LEVEL  0
# define IDU_ALTIBASE_PATCH_LEVEL     0

# if defined(COMPILE_FOR_NO_SMP)
#  define IDU_ALTIBASE_VERSION_STRING  "6.7.1.0.0-nosmp"
# else
#  define IDU_ALTIBASE_VERSION_STRING  "6.7.1.0.0"
# endif

#endif

#define IDU_SYSTEM_INFO_LENGTH      512

#define IDU_VERSION_CHECK_MASK      (0xFFFF0000)
#define IDU_MAJOR_VERSION_MASK      (0xFF000000)
#define IDU_MINOR_VERSION_MASK      (0x00FF0000)
#define IDU_PATCH_VERSION_MASK      (0x0000FFFF)

#define IDU_CHECK_VERSION_MASK      (0xFFFF0000)


#endif /* _O_IDU_VERSIONDEF_H_ */

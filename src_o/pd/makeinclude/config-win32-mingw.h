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
 
// -*- C++ -*-
// config-win32-mingw.h,v 4.4 2001/12/15 22:24:27 schmidt Exp

//
// The following configuration file is designed to work for win32
// platforms using gcc/g++ with mingw32 (http://www.mingw.org).
//

#ifndef PDL_CONFIG_WIN32_MINGW_H
#define PDL_CONFIG_WIN32_MINGW_H

#ifndef PDL_CONFIG_WIN32_H
#error Use config-win32.h in config.h instead of this header
#endif /* PDL_CONFIG_WIN32_H */

#define PDL_CC_NAME PDL_LIB_TEXT ("g++")
#define PDL_CC_PREPROCESSOR "cpp"
#define PDL_CC_PREPROCESOR_ARGS ""

#define PDL_HAS_BROKEN_SAP_ANY

// Why all this is not in config-g++-common.h?
#define PDL_CC_MAJOR_VERSION __GNUC__
#define PDL_CC_MINOR_VERSION __GNUC_MINOR__
#define PDL_CC_BETA_VERSION (0)

#if !defined(__MINGW32__)
#  error You do not seem to be using mingw32
#endif

#include "config-g++-common.h"

#include /**/ <_mingw.h>

#define PDL_LACKS_MODE_MASKS
#define PDL_HAS_USER_MODE_MASKS

#if (__MINGW32_MAJOR_VERSION == 0) && (__MINGW32_MINOR_VERSION < 5)
#error You need a newer version (>= 0.5) of mingw32/w32api
#endif

#define PDL_LACKS_STRRECVFD
#define PDL_HAS_STRERROR

// We trust the user: He must have used -mpentiumpro or -mpentium
// if that is what he wants.
#if defined(pentiumpro) || defined(pentium)
# define PDL_HAS_PENTIUM
#endif

#if !defined (PDL_HAS_WINNT4)
# if (defined (WINNT) && WINNT == 1) \
     || (defined (__WINNT__) && __WINNT__ == 1)
#  define PDL_HAS_WINNT4 1
# else
#  define PDL_HAS_WINNT4 0
# endif
#endif

#define PDL_ENDTHREADEX(STATUS)  ::_endthreadex ((DWORD) (STATUS))

#endif /* PDL_CONFIG_WIN32_MINGW_H */

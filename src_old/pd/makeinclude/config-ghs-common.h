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
 
/* -*- C++ -*- */
// config-ghs-common.h,v 4.5 1999/08/26 17:25:05 levine Exp

// This configuration file is designed to be included by another,
// specific configuration file.  It provides config information common
// to all Green Hills platforms.

#ifndef PDL_GHS_COMMON_H
#define PDL_GHS_COMMON_H

#if !defined (PDL_CONFIG_INCLUDE_GHS_COMMON)
# error config-ghs-common.h: PDL configuration error!  Do not #include this file directly!
#endif

#if defined (ghs)

# if defined (sun)
    // Need nonstatic Object_Manager on Solaris to prevent seg fault
    // on startup.
#   define PDL_HAS_NONSTATIC_OBJECT_MANAGER
# endif /* sun */

# if defined (__STANDARD_CXX)
    // Green Hills 1.8.9, but not 1.8.8.
#   define PDL_HAS_STANDARD_CPP_LIBRARY 1
#   define PDL_LACKS_AUTO_PTR
#   define PDL_LACKS_CHAR_RIGHT_SHIFTS
#   define PDL_LACKS_UNBUFFERED_STREAMBUF
#   define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
# else
#   define PDL_HAS_TEMPLATE_INSTANTIATION_PRAGMA
# endif /* __STANDARD_CXX */

# define PDL_HAS_WCHAR_TYPEDEFS_CHAR
# define PDL_LACKS_LINEBUFFERED_STREAMBUF
# define PDL_LACKS_LONGLONG_T
# define PDL_LACKS_SIGNED_CHAR

#else  /* ! ghs */
# error config-ghs-common.h can only be used with Green Hills compilers!
#endif /* ! ghs */

#endif /* PDL_GHS_COMMON_H */

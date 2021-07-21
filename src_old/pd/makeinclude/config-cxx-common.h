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
 
// config-cxx-common.h,v 4.7 1999/12/29 21:44:40 levine Exp

#ifndef PDL_CXX_COMMON_H
#define PDL_CXX_COMMON_H

#if !defined (PDL_CONFIG_INCLUDE_CXX_COMMON)
# error config-cxx-common.h: PDL configuration error!  Do not #include this file directly!
#endif

#if defined (__DECCXX)
# define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR
# define PDL_LACKS_LINEBUFFERED_STREAMBUF
# define PDL_LACKS_SIGNED_CHAR
# if defined (linux)
#   define PDL_HAS_STANDARD_CPP_LIBRARY 1
#   define PDL_HAS_CPLUSPLUS_HEADERS
# else  /* ! linux */
#   define PDL_HAS_STRING_CLASS
#   if (__DECCXX_VER >= 60090010)
#     define PDL_HAS_STDCPP_STL_INCLUDES
#   endif /* __DECCXX_VER < 60090010 */
# endif /* ! linux */
# define DEC_CXX
# if (__DECCXX_VER >= 60090010)
    // DEC CXX 6.0 supports exceptions, etc., by default.  Exceptions
    // are enabled by platform_osf1_4.0.GNU/wrapper_macros.GNU.
#   if defined (PDL_HAS_EXCEPTIONS)
/* #     define PDL_NEW_THROWS_EXCEPTIONS */
#   endif /* PDL_HAS_EXCEPTIONS */
#   define PDL_HAS_ANSI_CASTS
#   if !defined (__RTTI)
#     define PDL_LACKS_RTTI
#   endif
#   define PDL_HAS_TEMPLATE_SPECIALIZATION
#   define PDL_HAS_TEMPLATE_TYPEDEFS
#   define PDL_HAS_TYPENAME_KEYWORD
#   define PDL_HAS_USING_KEYWORD

#   define PDL_ENDLESS_LOOP \
      unsigned int pdl_endless_loop____ = 0; if (pdl_endless_loop____) break;

#   if defined (__USE_STD_IOSTREAM)
#     define PDL_LACKS_CHAR_RIGHT_SHIFTS
#     define PDL_LACKS_IOSTREAM_FX
#     define PDL_LACKS_UNBUFFERED_STREAMBUF
#   else  /* ! __USE_STD_IOSTREAM */
#     define PDL_USES_OLD_IOSTREAMS
#   endif /* ! __USE_STD_IOSTREAM */

//    9: nested comment not allowed.  (/usr/include/pdsc.h!) (nestcomment)
//  177: variable was declared but never referenced (declbutnotref)
//  193: zero used for undefined preprocessing identifier (undpreid)
//  236: controlling expression is constant (boolexprconst)
//  401: base_class_with_nonvirtual_dtor (basclsnondto)
// 1016: expected type is incompatible with declared type of int (incint)
// 1136: conversion to smaller size integer could lose data (intconlosbit)

#   pragma message disable basclsnondto
#   pragma message disable boolexprconst
#   pragma message disable undpreid

#   if (__DECCXX_VER >= 60190029)
      // 6.1-029 and later support msg 1136.  Disable it because it
      // causes warnings from PDL and/or TAO.
#     pragma message disable intconlosbit
#   endif /* __DECCXX_VER >= 60190029 */

#   if defined (DIGITAL_UNIX)  &&  DIGITAL_UNIX >= 0x40F
      // variable "PTHREAD_THIS_CATCH_NP" was declared but never referenced
#     pragma message disable declbutnotref
#   endif /* DIGITAL_UNIX >= 4.0f */

# else  /* __DECCXX_VER < 60090010 */
#   define PDL_LACKS_PRAGMA_ONCE
# endif /* __DECCXX_VER < 60090010 */
#else  /* ! __DECCXX */
# error config-cxx-common.h can only be used with Compaq CXX!
#endif /* ! __DECCXX */

#endif /* PDL_CXX_COMMON_H */

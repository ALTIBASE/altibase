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
//=============================================================================
/**
 *  @file   config-win32-msvc-7.h
 *
 *  config-win32-msvc-7.h,v 4.3 2002/04/15 18:30:40 ossama Exp
 *
 *  @brief  Microsoft Visual C++ 7.0 configuration file.
 *
 *  This file is the PDL configuration file for Microsoft Visual C++ version 7.
 *
 *  @note Do not include this file directly, include config-win32.h instead.
 *
 *  @author Darrell Brunsch <brunsch@cs.wustl.edu>
 */
//=============================================================================

#ifndef PDL_CONFIG_WIN32_MSVC_7_H
#define PDL_CONFIG_WIN32_MSVC_7_H

#ifndef PDL_CONFIG_WIN32_H
#error Use config-win32.h in config.h instead of this header
#endif /* PDL_CONFIG_WIN32_H */

// Visual C++ 7.0 (.NET) deprecated the old iostreams
#if !defined (PDL_HAS_STANDARD_CPP_LIBRARY)
#define PDL_HAS_STANDARD_CPP_LIBRARY 1
#endif

#if !defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
#endif

// Win64 SDK compiler claims std::auto_ptr<>::reset not available.
#if defined (_WIN64) || defined (WIN64)
#define PDL_AUTO_PTR_LACKS_RESET
#endif

#if !defined (PDL_HAS_BROKEN_NESTED_TEMPLATES)
#define PDL_HAS_BROKEN_NESTED_TEMPLATES
#endif

// By default, we disable the C++ casting because
// it requires the RTTI support to be turned on which
// is not something we usually do.
#if !defined (PDL_HAS_ANSI_CASTS)
#define PDL_HAS_ANSI_CASTS 0
#endif

#define PDL_HAS_EXPLICIT_KEYWORD
#define PDL_HAS_MUTABLE_KEYWORD
#define PDL_HAS_TYPENAME_KEYWORD
#define PDL_HAS_USING_KEYWORD

#define PDL_HAS_ITOA

#define PDL_HAS_BROKEN_IMPLICIT_CONST_CAST
#define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR

#define PDL_ITOA_EQUIVALENT ::_itoa
#define PDL_STRCASECMP_EQUIVALENT ::_stricmp
#define PDL_STRNCASECMP_EQUIVALENT ::_strnicmp
#define PDL_WCSDUP_EQUIVALENT ::_wcsdup

#if !defined (PDL_HAS_WINCE)
#define PDL_HAS_EXCEPTIONS
#endif /* PDL_HAS_WINCE */

#if defined (PDL_HAS_ANSI_CASTS) && (PDL_HAS_ANSI_CASTS == 0)
#undef  PDL_HAS_ANSI_CASTS
#endif /* PDL_HAS_ANSI_CASTS && PDL_HAS_ANSI_CASTS == 0 */

#define PDL_HAS_STRERROR
#define PDL_HAS_STRPTIME
#define PDL_LACKS_NATIVE_STRPTIME

#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
#define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES
#define PDL_LACKS_MODE_MASKS
#define PDL_LACKS_STRRECVFD
#define PDL_HAS_CPLUSPLUS_HEADERS

#define PDL_TEMPLATES_REQUIRE_SOURCE
#define PDL_HAS_TEMPLATE_SPECIALIZATION

#define PDL_INT64_FORMAT_SPECIFIER PDL_LIB_TEXT ("%I64d")
#define PDL_UINT64_FORMAT_SPECIFIER PDL_LIB_TEXT ("%I64u")

// Platform provides PDL_TLI function prototypes.
// For Win32, this is not really true, but saves a lot of hassle!
#define PDL_HAS_TLI_PROTOTYPES

// Platform support linebuffered streaming is broken
#define PDL_LACKS_LINEBUFFERED_STREAMBUF

#if defined (PDL_HAS_STANDARD_CPP_LIBRARY) && (PDL_HAS_STANDARD_CPP_LIBRARY != 0)

// Platform has its Standard C++ library in the namespace std
# if !defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
# define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB       1
# endif /* PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB */

// iostream.h does not work with the standard cpp library (yet).
# if !defined (PDL_USES_OLD_IOSTREAMS)
# define PDL_LACKS_PDL_IOSTREAM
# endif /* ! PDL_USES_OLD_IOSTREAMS */

#else

// iostream header lacks ipfx (), isfx (), etc., declarations
#  define PDL_LACKS_IOSTREAM_FX

#endif

// There are too many instances of this warning to fix it right now.
// Maybe in the future.
// 'this' : used in base member initializer list
#pragma warning(disable:4355)
// 'class1' : inherits 'class2::member' via dominance
#pragma warning(disable:4250)
// C++ Exception Specification ignored
#pragma warning(disable:4290)

#define PDL_ENDTHREADEX(STATUS) ::_endthreadex ((DWORD) STATUS)

// With the MSVC7 compiler there is a new 'feature' when a base-class is a
// specialization of a class template. The class template must be explicit
// instantiated and exported.
#define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_EXPORT

// At least for PDL_UNIMPLEMENTED_FUNC in class templates, this is needed to
// explicitly instantiate a template that has PDL_UNIMPLEMENTED_FUNC.
# define PDL_NEEDS_FUNC_DEFINITIONS

#endif /* PDL_CONFIG_WIN32_MSVC_7_H */

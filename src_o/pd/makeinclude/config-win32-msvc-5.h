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
 *  @file   config-win32-msvc-5.h
 *
 *  config-win32-msvc-5.h,v 4.4 2001/07/09 16:15:38 parsons Exp
 *
 *  @brief  Microsoft Visual C++ 5.0 configuration file. 
 *
 *  This file is the PDL configuration file for Microsoft Visual C++ version 5. 
 *
 *  @note   Do not include this file directly, include config-win32.h instead.
 *
 *  @author Darrell Brunsch <brunsch@cs.wustl.edu>
 */
//=============================================================================

#ifndef PDL_CONFIG_WIN32_MSVC_5_H
#define PDL_CONFIG_WIN32_MSVC_5_H

#ifndef PDL_CONFIG_WIN32_H
#error Use config-win32.h in config.h instead of this header
#endif /* PDL_CONFIG_WIN32_H */

# if defined (PDL_HAS_PACE)
#  ifndef PACE_HAS_ALL_POSIX_FUNCS
#   define PACE_HAS_ALL_POSIX_FUNCS 1
#  endif /* PACE_HAS_ALL_POSIX_FUNCS */
# endif /* PDL_HAS_PACE */

# if !defined (PDL_HAS_STANDARD_CPP_LIBRARY)
#   define PDL_HAS_STANDARD_CPP_LIBRARY    0
# endif

// The STL that comes with PDL uses the std namespace. Note however, it is not
// part of the standard C++ library
# if !defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#  define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
# endif /* PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB */

# if !defined (PDL_HAS_BROKEN_NESTED_TEMPLATES)
#  define PDL_HAS_BROKEN_NESTED_TEMPLATES
# endif /* PDL_HAS_BROKEN_NESTED_TEMPLATES */

// By default, we disable the C++ casting because
// it requires the RTTI support to be turned on which
// is not something we usually do.
# if !defined (PDL_HAS_ANSI_CASTS)
#  define PDL_HAS_ANSI_CASTS 0
# endif

# define PDL_HAS_EXPLICIT_KEYWORD
# define PDL_HAS_MUTABLE_KEYWORD

#define PDL_HAS_ITOA

#define PDL_ITOA_EQUIVALENT ::_itoa
#define PDL_STRCASECMP_EQUIVALENT ::_stricmp
#define PDL_STRNCASECMP_EQUIVALENT ::_strnicmp
#define PDL_WCSDUP_EQUIVALENT ::_wcsdup

// VC5 doesn't support operator placement delete
# if (_MSC_VER < 1200)  
#  define PDL_LACKS_PLACEMENT_OPERATOR_DELETE
# endif /* _MSC_VER < 1200 */

# if !defined (PDL_HAS_WINCE)
#  define PDL_HAS_EXCEPTIONS
# endif /* PDL_HAS_WINCE */
# define PDL_HAS_BROKEN_NAMESPACES
# define PDL_HAS_BROKEN_IMPLICIT_CONST_CAST
# define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR

# if defined (PDL_HAS_ANSI_CASTS) && (PDL_HAS_ANSI_CASTS == 0)
#  undef  PDL_HAS_ANSI_CASTS
# endif /* PDL_HAS_ANSI_CASTS && PDL_HAS_ANSI_CASTS == 0 */

# if !defined (PDL_HAS_WINCE)
#   define PDL_HAS_SIG_ATOMIC_T
# endif /* PDL_HAS_WINCE */

// Only >= MSVC 6.0 definitions
# if (_MSC_VER >= 1200)
#  define PDL_HAS_TYPENAME_KEYWORD
#  define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
# endif /* _MSC_VER >= 1200 */

// Compiler doesn't support static data member templates.
# define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES

# define PDL_LACKS_MODE_MASKS
# define PDL_LACKS_STRRECVFD

# if !defined (PDL_HAS_WINCE)
# define PDL_HAS_LLSEEK
# endif /* PDL_HAS_WINCE */

// Compiler/platform has correctly prototyped header files.
# define PDL_HAS_CPLUSPLUS_HEADERS

// Platform supports POSIX timers via timestruc_t.
//define PDL_HAS_POSIX_TIME
# define PDL_HAS_STRPTIME
# define PDL_LACKS_NATIVE_STRPTIME

// Compiler/platform supports strerror ().
# define PDL_HAS_STRERROR

# define PDL_TEMPLATES_REQUIRE_SOURCE

// Platform provides PDL_TLI function prototypes.
// For Win32, this is not really true, but saves a lot of hassle!
# define PDL_HAS_TLI_PROTOTYPES

// Platform support linebuffered streaming is broken
# define PDL_LACKS_LINEBUFFERED_STREAMBUF

// Template specialization is supported.
# define PDL_HAS_TEMPLATE_SPECIALIZATION

// ----------------- "derived" defines and includes -----------

# if defined (PDL_HAS_STANDARD_CPP_LIBRARY) && (PDL_HAS_STANDARD_CPP_LIBRARY != 0)

// Platform has its Standard C++ library in the namespace std
#   if !defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#    define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB       1
#   endif /* PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB */

// Microsoft's auto_ptr in standard cpp library doesn't have reset ().
#  define PDL_AUTO_PTR_LACKS_RESET

// iostream.h does not work with the standard cpp library (yet).
#  if !defined (PDL_USES_OLD_IOSTREAMS)
#   define PDL_LACKS_PDL_IOSTREAM
#  endif /* ! PDL_USES_OLD_IOSTREAMS */
# else
// iostream header lacks ipfx (), isfx (), etc., declarations
#  define PDL_LACKS_IOSTREAM_FX
# endif

// While digging the MSVC 4.0 include files, I found how to disable
// MSVC warnings: --Amos Shapira

// "C4355: 'this' : used in base member initializer list"
# pragma warning(disable:4355) /* disable C4514 warning */
//      #pragma warning(default:4355)   // use this to reenable, if desired

# pragma warning(disable:4201)  /* winnt.h uses nameless structs */

# pragma warning(disable:4231)
// Disable warning of using Microsoft Extension.

// MSVC allows throw keyword but complains about it.
# pragma warning( disable : 4290 )

// Inheritance by dominance is A-OK by us
# pragma warning (disable:4250)

// Compiler/Platform supports the "using" keyword.
#define PDL_HAS_USING_KEYWORD

# if !defined(PDL_HAS_WINCE)
#  if defined(PDL_HAS_DLL) && (PDL_HAS_DLL != 0)
#   if !defined(_DLL)
// *** DO NOT *** DO NOT *** defeat this error message
// by defining _DLL yourself.  RTFM and see who set _DLL.
#    error You must link against (Debug) Multithreaded DLL run-time libraries.
#   endif /* !_DLL */
#  endif  /* PDL_HAS_DLL && PDL_HAS_DLL != 0 */
# endif /* !PDL_HAS_WINCE */

# pragma warning(default: 4201)  /* winnt.h uses nameless structs */

// At least for Win32 - MSVC compiler (ver. 5)
# define PDL_INT64_FORMAT_SPECIFIER PDL_LIB_TEXT ("%I64d")
# define PDL_UINT64_FORMAT_SPECIFIER PDL_LIB_TEXT ("%I64u")

# if !defined (PDL_LD_DECORATOR_STR)
#  if defined (_DEBUG)
#   if PDL_HAS_MFC == 1
#    define PDL_LD_DECORATOR_STR PDL_LIB_TEXT ("mfcd")
#   else
#    define PDL_LD_DECORATOR_STR PDL_LIB_TEXT ("d")
#   endif //PDL_HAS_MFC
#  else // _NDEBUG
#   if PDL_HAS_MFC == 1
#    define PDL_LD_DECORATOR_STR PDL_LIB_TEXT ("mfc")
#   endif //PDL_HAS_MFC
#  endif // _DEBUG
# endif

# define PDL_ENDTHREADEX(STATUS) ::_endthreadex ((DWORD) STATUS)

#endif /* PDL_CONFIG_WIN32_MSVC_5_H */

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
// config-win32-ghs.h,v 4.3 2001/12/08 19:01:22 schmidt Exp

// The following configuration file contains defines for Green Hills compilers.

#ifndef PDL_CONFIG_WIN32_GHS_H
#define PDL_CONFIG_WIN32_GHS_H

#ifndef PDL_CONFIG_WIN32_H
#error Use config-win32.h in config.h instead of this header
#endif /* PDL_CONFIG_WIN32_H */

#ifndef WIN32
#  define WIN32
#endif /* WIN32 */

#undef _M_IX86
// This turns on PDL_HAS_PENTIUM
#define _M_IX86 500
// GHS does not provide DLL support
#define PDL_HAS_DLL 0
#define TAO_HAS_DLL 0
#undef _DLL
#define PDL_OS_HAS_DLL 0

//Green Hills Native x86 does not support structural exceptions
# undef PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS
# undef PDL_HAS_WCHAR
# define PDL_CONFIG_INCLUDE_GHS_COMMON
# include "config-ghs-common.h"

// Changed PDL_TEXT to PDL_LIB_TEXT in the following line
# define PDL_CC_NAME PDL_LIB_TEXT ("Green Hills C++")
# define PDL_CC_MAJOR_VERSION (1)
# define PDL_CC_MINOR_VERSION (8)
# define PDL_CC_BETA_VERSION (9)
# define PDL_CC_PREPROCESSOR "GCX.EXE"
# define PDL_CC_PREPROCESSOR_ARGS "-E"

// GHS uses Microsoft's standard cpp library, which has auto_ptr.
# undef PDL_LACKS_AUTO_PTR
// Microsoft's standard cpp library auto_ptr doesn't have reset ().
# define PDL_AUTO_PTR_LACKS_RESET

#define PDL_ENDTHREADEX(STATUS) ::_endthreadex ((DWORD) STATUS)

// This section below was extracted from config-win32-msvc
#define PDL_HAS_ITOA
#define PDL_ITOA_EQUIVALENT ::_itoa
#define PDL_STRCASECMP_EQUIVALENT ::_stricmp
#define PDL_STRNCASECMP_EQUIVALENT ::_strnicmp
#define PDL_WCSDUP_EQUIVALENT ::_wcsdup
//  This section above was extracted from config-win32-msvc

# define PDL_EXPORT_NESTED_CLASSES 1
# define PDL_HAS_ANSI_CASTS 1
# define PDL_HAS_CPLUSPLUS_HEADERS 1
//# define PDL_HAS_EXCEPTIONS 1
# define PDL_HAS_EXPLICIT_KEYWORD 1
# define PDL_HAS_GNU_CSTRING_H 1
# define PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION 1
# define PDL_HAS_MUTABLE_KEYWORD 1
# define PDL_HAS_NONCONST_SELECT_TIMEVAL 1
# define PDL_HAS_SIG_ATOMIC_T 1
# define PDL_HAS_STANDARD_CPP_LIBRARY 1
//# if (__BORLANDC__ <= 0x540)
//#  define PDL_HAS_STD_TEMPLATE_METHOD_SPECIALIZATION 1
//# endif
# define PDL_HAS_STD_TEMPLATE_SPECIALIZATION 1
# define PDL_HAS_STDCPP_STL_INCLUDES 1
# define PDL_HAS_STRERROR 1
# define PDL_HAS_STRING_CLASS 1
# define PDL_HAS_STRPTIME 1
# define PDL_HAS_TEMPLATE_SPECIALIZATION 1
# define PDL_HAS_TEMPLATE_TYPEDEFS 1
# define PDL_HAS_TYPENAME_KEYWORD 1
//# define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
# define PDL_HAS_USER_MODE_MASKS 1
# define PDL_HAS_USING_KEYWORD 1
# define PDL_LACKS_PDL_IOSTREAM 1
//# define PDL_LACKS_LINEBUFFERED_STREAMBUF 1
# define PDL_LACKS_MODE_MASKS 1
# define PDL_LACKS_NATIVE_STRPTIME 1
//# define PDL_LACKS_PLACEMENT_OPERATOR_DELETE 1
//# define PDL_LACKS_PRAGMA_ONCE 1
# define PDL_LACKS_STRRECVFD 1
//# define PDL_NEW_THROWS_EXCEPTIONS 1
# define PDL_SIZEOF_LONG_DOUBLE 10
# define PDL_TEMPLATES_REQUIRE_SOURCE 1
// Changed PDL_TEXT to PDL_LIB_TEXT in the following two lines
# define PDL_UINT64_FORMAT_SPECIFIER PDL_LIB_TEXT ("%I64u")
# define PDL_INT64_FORMAT_SPECIFIER PDL_LIB_TEXT ("%I64d")
# define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
// Set the following to zero to placate SString.h PDL_WString CTOR
# undef PDL_WSTRING_HAS_USHORT_SUPPORT

// Green Hills Native x86 does not support __int64 keyword
# define PDL_LACKS_LONGLONG_T

/* need to ensure these are included before <iomanip> */
# include <time.h>
# include <stdlib.h>

# if !defined (PDL_LD_DECORATOR_STR) && defined (_DEBUG)
#  define PDL_LD_DECORATOR_STR PDL_LIB_TEXT ("d")
# endif

#endif /* PDL_CONFIG_WIN32_GHS_H */

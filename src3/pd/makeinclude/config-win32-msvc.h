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
// config-win32-msvc.h,v 4.1 2000/01/15 19:04:06 schmidt Exp

// The following configuration file contains the defines
// common to all Win32/MSVC/MFC combinations.

#ifndef PDL_WIN32_MSVC_H
#define PDL_WIN32_MSVC_H

// Win64 SDK compiler claims std::auto_ptr<>::reset not available.
#if defined (_WIN64) || defined (WIN64)
#define PDL_AUTO_PTR_LACKS_RESET
#endif

#if defined (_MSC_VER)

# include "config-win32-common.h"

# define PDL_CC_NAME "Visual C++"
# define PDL_CC_PREPROCESSOR "CL.EXE"
# define PDL_CC_PREPROCESSOR_ARGS "-nologo -E"

/* BUG-23087 */
# if (_MSC_VER >= 1500)
#  define PDL_CC_MAJOR_VERSION 9
/* bug-18654 */
# elif (_MSC_VER >= 1400)
#  define PDL_CC_MAJOR_VERSION 8
# elif (_MSC_VER >= 1300)
#  define PDL_CC_MAJOR_VERSION 7
# elif (_MSC_VER >= 1200)
#  define PDL_CC_MAJOR_VERSION 6
# elif (_MSC_VER >= 1100)
#  define PDL_CC_MAJOR_VERSION 5
# elif (_MSC_VER >= 1000)
#  define PDL_CC_MAJOR_VERSION 4
# endif /* _MSC_VER  >= 1200 */

# define PDL_CC_MINOR_VERSION (_MSC_VER % 100)
# define PDL_CC_BETA_VERSION (0)

// Define this if you want to use the standard C++ library
//#define PDL_HAS_STANDARD_CPP_LIBRARY 1

// MSVC enforces the One Definition Rule
# define PDL_HAS_ONE_DEFINITION_RULE

# if defined (_MSC_VER) && (_MSC_VER >= 1020)
#  if !defined (PDL_HAS_STANDARD_CPP_LIBRARY)
#   define PDL_HAS_STANDARD_CPP_LIBRARY    0
#  endif
# else
#  if defined (PDL_HAS_STANDARD_CPP_LIBRARY)
#   undef PDL_HAS_STANDARD_CPP_LIBRARY
#  endif
#  define PDL_HAS_STANDARD_CPP_LIBRARY 0
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

// MSVC already defined __TEXT
# define PDL_HAS_TEXT_MACRO_CONFLICT

# define PDL_HAS_EXPLICIT_KEYWORD
# define PDL_HAS_MUTABLE_KEYWORD

// VC5 doesn't support operator placement delete
# if defined (_MSC_VER) && (_MSC_VER < 1200)  
#  define PDL_LACKS_PLACEMENT_OPERATOR_DELETE
# endif /* _MSC_VER && _MSC_VER < 1200 */

# define PDL_HAS_WCHAR_TYPEDEFS_USHORT
# if !defined (PDL_HAS_WINCE)
#  define PDL_HAS_EXCEPTIONS
# endif /* PDL_HAS_WINCE */
# define PDL_HAS_BROKEN_NAMESPACES
# define PDL_HAS_BROKEN_IMPLICIT_CONST_CAST
# define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR

# if defined (PDL_HAS_ANSI_CASTS) && (PDL_HAS_ANSI_CASTS == 0)
#  undef  PDL_HAS_ANSI_CASTS
# endif /* PDL_HAS_ANSI_CASTS && PDL_HAS_ANSI_CASTS == 0 */

# if _MSC_VER < 1000
#  define PDL_LACKS_PRAGMA_ONCE
# endif /* _MSC_VER < 1000 */

// Only >= MSVC 5.0 definitions
# if (_MSC_VER >= 1100)
#  if !defined (PDL_HAS_WINCE)
#   define PDL_HAS_SIG_ATOMIC_T
#  endif /* PDL_HAS_WINCE */
# endif /* _MSC_VER >= 1100 */

// Only >= MSVC 6.0 definitions
# if (_MSC_VER >= 1200)
#  define PDL_HAS_TYPENAME_KEYWORD
#  define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
# endif /* _MSC_VER >= 1200 */

// Compiler doesn't support static data member templates.
# define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES

# define PDL_LACKS_MODE_MASKS
# define PDL_LACKS_STRRECVFD

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
//# define PDL_HAS_TLI_PROTOTYPES
# define PDL_HAS_GNU_CSTRING_H

// Platform support linebuffered streaming is broken
# define PDL_LACKS_LINEBUFFERED_STREAMBUF

// Template specialization is supported.
# define PDL_HAS_TEMPLATE_SPECIALIZATION

// ----------------- "derived" defines and includes -----------

# if defined (PDL_HAS_STANDARD_CPP_LIBRARY) && (PDL_HAS_STANDARD_CPP_LIBRARY != 0)

#  if (_MSC_VER > 1020)
// Platform has its Standard C++ library in the namespace std
#   if !defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#    define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB       1
#   endif /* PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB */
#  else /* (_MSC_VER > 1020) */
#   if defined (PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB)
#    undef PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB
#   endif /* PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB */
#   define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 0
#  endif /* (_MSC_VER > 1020) */

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

// MSVC 4.0 or greater
# if (_MSC_VER >= 1000)
// Compiler/Platform supports the "using" keyword.
#  define PDL_HAS_USING_KEYWORD
# endif

// MSVC 2.2 or lower
# if (_MSC_VER < 1000)
// This needs to be here since MSVC++ 2.0 seems to have forgotten to
// include it. Does anybody know which MSC_VER MSVC 2.0 has ?
inline void *operator new (unsigned int, void *p) { return p; }
# endif

# if !defined(PDL_HAS_WINCE)
#  if defined(PDL_HAS_DLL) && (PDL_HAS_DLL != 0)
#   if !defined(_DLL)
// *** DO NOT *** DO NOT *** defeat this error message
// by defining _DLL yourself.  RTFM and see who set _DLL.
#    error You must link against (Debug) Multithreaded DLL run-time libraries.
#   endif /* !_DLL */
#  endif  /* PDL_HAS_DLL && PDL_HAS_DLL != 0 */
# endif /* !PDL_HAS_WINCE */

# ifdef _DEBUG
#  if !defined (PDL_HAS_WINCE)
#   include /**/ <crtdbg.h>
#  endif /* PDL_HAS_WINCE */
# endif

# pragma warning(default: 4201)  /* winnt.h uses nameless structs */

// At least for Win32 - MSVC compiler (ver. 5)
# define PDL_UINT64_FORMAT_SPECIFIER "%I64u"

#if (_MSC_VER >= 1200)
// hjohn add for vc-win32 porting
#define PDL_LACKS_SEEKDIR
//#define RLIMIT_CPU      0               /* CPU time in ms */
#define RLIMIT_FSIZE    1               /* Maximum filesize */
//#define RLIMIT_DATA     2               /* max data size */
//#define RLIMIT_STACK    3               /* max stack size */
//#define RLIMIT_CORE     4               /* max core file size */
#endif
#endif /* _MSC_VER */

#endif /* PDL_WIN32_COMMON_H */

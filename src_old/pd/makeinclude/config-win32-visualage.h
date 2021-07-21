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
// config-win32-visualage.h,v 4.2 2000/01/15 07:52:26 schmidt Exp

//Created for IBMCPP
// The following configuration file contains the defines
// common to all VisualAge compilers.

#ifndef PDL_WIN32_VISUALAGECPP_H
#define PDL_WIN32_VISUALAGECPP_H

#if defined (__IBMCPP__) && (__IBMCPP__ >= 400)

#define PDL_CC_NAME "IBM VisualAge C++"
#define PDL_CC_MAJOR_VERSION (__IBMCPP__ / 0x100)
#define PDL_CC_MINOR_VERSION (__IBMCPP__ % 0x100)
#define PDL_CC_BETA_VERSION (0)
#define PDL_CC_PREPROCESSOR ""
#define PDL_CC_PREPROCESSOR_ARGS ""

//These need to be defined for VisualAgeC++
#define ERRMAX 256 /* Needed for following define */
#define PDL_LACKS_SYS_NERR /* Needed for sys_nerr in Log_Msg.cpp */
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES /* For signal handling */
#define PDL_HAS_TYPENAME_KEYWORD
#define PDL_LACKS_MKTEMP
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define NSIG 23 /* Maximum no. of signals + 1 */
#define PDL_HAS_ANSI_CASTS 1
#define PDL_HAS_BROKEN_NESTED_TEMPLATES 1
#define PDL_HAS_CPLUSPLUS_HEADERS 1
#define PDL_HAS_EXCEPTIONS 1
#define PDL_HAS_EXPLICIT_KEYWORD 1
#define PDL_HAS_GNU_CSTRING_H 1
#define PDL_HAS_MUTABLE_KEYWORD 1
#define PDL_HAS_NONCONST_SELECT_TIMEVAL 1
#define PDL_HAS_ONE_DEFINITION_RULE 1
#define PDL_HAS_SIG_ATOMIC_T 1
#define PDL_HAS_STANDARD_CPP_LIBRARY 1
#define PDL_HAS_STDCPP_STL_INCLUDES 1
#define PDL_HAS_STRERROR 1
#define PDL_HAS_STRING_CLASS 1
#define PDL_HAS_STRPTIME 1
#define PDL_HAS_TEMPLATE_SPECIALIZATION 1
#define PDL_HAS_TEMPLATE_TYPEDEFS 1
#define PDL_HAS_TEXT_MACRO_CONFLICT 1
#define PDL_HAS_TYPENAME_KEYWORD 1
#define PDL_HAS_USING_KEYWORD 1
#define PDL_LACKS_PDL_IOSTREAM 1
#define PDL_LACKS_LINEBUFFERED_STREAMBUF 1
#define PDL_LACKS_MODE_MASKS 1
#define PDL_LACKS_NATIVE_STRPTIME 1
#define PDL_LACKS_PRAGMA_ONCE 1
#define PDL_LACKS_STRRECVFD 1
/* #define PDL_NEW_THROWS_EXCEPTIONS 1 */
#define PDL_SIZEOF_LONG_DOUBLE 10
#define PDL_TEMPLATES_REQUIRE_SOURCE 1
#define PDL_UINT64_FORMAT_SPECIFIER "%I64u"
#define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1

#undef WIFEXITED
#undef WEXITSTATUS

#define _O_RDWR   O_RDWR
#define _O_WRONLY O_WRONLY
#define _O_RDONLY O_RDONLY
#define _O_APPEND O_APPEND
#define _O_BINARY O_BINARY
#define _O_TEXT O_TEXT

#define _endthreadex _endthread
#define _beginthreadex _beginthread

//Error codes that are in MS Visual C++
#define EFAULT 99 /* Error code (should be in errno.h) */
#define ENODEV		19
#define EPIPE		32
#define ENAMETOOLONG	38

#if defined (PDL_HAS_UNICODE)
# undef PDL_HAS_UNICODE
#endif /* PDL_HAS_UNICODE */

#endif /* defined(__IBMCPP__) */

#endif /* PDL_WIN32_VISUALAGECPP_H */

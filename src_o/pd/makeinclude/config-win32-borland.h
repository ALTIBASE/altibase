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
// config-win32-borland.h,v 4.20 2000/02/01 02:40:59 schmidt Exp

// The following configuration file contains defines for Borland compilers.

#ifndef PDL_WIN32_BORLAND_H
#define PDL_WIN32_BORLAND_H

#if defined (__BORLANDC__)

# include "config-win32-common.h"

# define PDL_CC_NAME "Borland C++ Builder"
# define PDL_CC_MAJOR_VERSION (__BORLANDC__ / 0x100)
# define PDL_CC_MINOR_VERSION (__BORLANDC__ % 0x100)
# define PDL_CC_BETA_VERSION (0)
# define PDL_CC_PREPROCESSOR "CPP32.EXE"
# define PDL_CC_PREPROCESSOR_ARGS "-P- -ocon"

# define PDL_EXPORT_NESTED_CLASSES 1
# define PDL_HAS_ANSI_CASTS 1
# define PDL_HAS_CPLUSPLUS_HEADERS 1
# define PDL_HAS_EXCEPTIONS 1
# define PDL_HAS_EXPLICIT_KEYWORD 1
# define PDL_HAS_GNU_CSTRING_H 1
# define PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION 1
# define PDL_HAS_MUTABLE_KEYWORD 1
# define PDL_HAS_NONCONST_SELECT_TIMEVAL 1
# define PDL_HAS_ONE_DEFINITION_RULE 1
# define PDL_HAS_SIG_ATOMIC_T 1
# define PDL_HAS_STANDARD_CPP_LIBRARY 1
# if (__BORLANDC__ <= 0x540)
#  define PDL_HAS_STD_TEMPLATE_METHOD_SPECIALIZATION 1
# endif
# define PDL_HAS_STD_TEMPLATE_SPECIALIZATION 1
# define PDL_HAS_STDCPP_STL_INCLUDES 1
# define PDL_HAS_STRERROR 1
# define PDL_HAS_STRING_CLASS 1
# define PDL_HAS_STRPTIME 1
# define PDL_HAS_TEMPLATE_SPECIALIZATION 1
# define PDL_HAS_TEMPLATE_TYPEDEFS 1
# define PDL_HAS_TEXT_MACRO_CONFLICT 1
# define PDL_HAS_TYPENAME_KEYWORD 1
# define PDL_HAS_USER_MODE_MASKS 1
# define PDL_HAS_USING_KEYWORD 1
# define PDL_LACKS_PDL_IOSTREAM 1
# define PDL_LACKS_LINEBUFFERED_STREAMBUF 1
# define PDL_LACKS_MODE_MASKS 1
# define PDL_LACKS_NATIVE_STRPTIME 1
# define PDL_LACKS_PLACEMENT_OPERATOR_DELETE 1
# define PDL_LACKS_PRAGMA_ONCE 1
# define PDL_LACKS_STRRECVFD 1
/* # define PDL_NEW_THROWS_EXCEPTIONS 1 */
# define PDL_SIZEOF_LONG_DOUBLE 10
# define PDL_TEMPLATES_REQUIRE_SOURCE 1
# define PDL_UINT64_FORMAT_SPECIFIER "%Lu"
# define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1

/* need to ensure these are included before <iomanip> */
# include <time.h>
# include <stdlib.h>

#endif /* defined(__BORLANDC__) */

#endif /* PDL_WIN32_BORLAND_H */

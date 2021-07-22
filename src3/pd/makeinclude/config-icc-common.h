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
 
// config-icc-common.h,v 4.2 2002/05/10 23:31:13 kitty Exp

#ifndef PDL_LINUX_ICC_COMMON_H
#define PDL_LINUX_ICC_COMMON_H

# define PDL_HAS_ANSI_CASTS
# define PDL_HAS_CPLUSPLUS_HEADERS
# define PDL_HAS_EXPLICIT_KEYWORD
# define PDL_HAS_MUTABLE_KEYWORD
# define PDL_HAS_STDCPP_STL_INCLUDES
# define PDL_HAS_TEMPLATE_TYPEDEFS
# define PDL_HAS_TYPENAME_KEYWORD
# define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
# define PDL_HAS_STANDARD_CPP_LIBRARY 1
# define PDL_HAS_TEMPLATE_SPECIALIZATION
# define PDL_HAS_USING_KEYWORD
# define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR
# define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
# define PDL_HAS_STRING_CLASS

// __EXCEPTIONS is defined with -fexceptions, the egcs default.  It
// is not defined with -fno-exceptions, the PDL default for g++.
// PDL_HAS_EXCEPTIONS is defined in
// include/makeinclude/wrapper_macros.GNU, so this really isn't
// necessary.  Just in case . . .
# if defined (__EXCEPTIONS) && !defined (PDL_HAS_EXCEPTIONS)
#   define PDL_HAS_EXCEPTIONS
# endif /* __EXCEPTIONS && ! PDL_HAS_EXCEPTIONS */

# if defined (PDL_HAS_EXCEPTIONS)
#   define PDL_NEW_THROWS_EXCEPTIONS
# endif /* PDL_HAS_EXCEPTIONS */

#if (defined (i386) || defined (__i386__)) && !defined (PDL_SIZEOF_LONG_DOUBLE)
# define PDL_SIZEOF_LONG_DOUBLE 12
#endif /* i386 */

#if !defined (__MINGW32__) && (defined (i386) || defined (__i386__))
  // If running an Intel, assume that it's a Pentium so that
  // PDL_OS::gethrtime () can use the RDTSC instruction.  If running a
  // 486 or lower, be sure to comment this out.  (If not running an
  // Intel CPU, this #define will not be seen because of the i386
  // protection, so it can be ignored.)
# define PDL_HAS_PENTIUM
#endif /* i386 */

#if !defined (PDL_LACKS_PRAGMA_ONCE)
  // We define it with a -D with make depend.
# define PDL_LACKS_PRAGMA_ONCE
#endif /* ! PDL_LACKS_PRAGMA_ONCE */

#define PDL_TEMPLATES_REQUIRE_SOURCE

# if defined (PDL_HAS_EXCEPTIONS)
#   define PDL_NEW_THROWS_EXCEPTIONS
# endif /* PDL_HAS_EXCEPTIONS */

#endif /* PDL_LINUX_ICC_COMMON_H */

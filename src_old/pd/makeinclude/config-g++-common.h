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
// config-g++-common.h,v 4.36 2000/01/20 02:18:35 irfan Exp

// This configuration file is designed to be included by another,
// specific configuration file.  It provides config information common
// to all g++ platforms, including egcs.

#ifndef PDL_GNUG_COMMON_H
#define PDL_GNUG_COMMON_H

#if __GNUC__ > 2  ||  ( __GNUC__ == 2 && __GNUC_MINOR__ >= 8)  || \
    (defined (PDL_VXWORKS) && PDL_VXWORKS >= 0x540)
  // egcs or g++ >= 2.8.0

# define PDL_HAS_ANSI_CASTS
# define PDL_HAS_CPLUSPLUS_HEADERS
# define PDL_HAS_EXPLICIT_KEYWORD
# define PDL_HAS_MUTABLE_KEYWORD
# define PDL_HAS_STDCPP_STL_INCLUDES
# define PDL_HAS_TEMPLATE_TYPEDEFS
# define PDL_HAS_TYPENAME_KEYWORD
# define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
# define PDL_HAS_STANDARD_CPP_LIBRARY 1
# define PDL_USES_OLD_IOSTREAMS
// For some reason EGCS doesn't define this in its stdlib.
# define PDL_LACKS_AUTO_PTR

# if __GNUC__ == 2 && __GNUC_MINOR__ >= 91
#   define PDL_HAS_USING_KEYWORD
    // This is only needed with egcs 1.1 (egcs-2.91.57).  It can't be
    // used with older versions.
#   define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR
# else /* This is for gcc 2.8.x */
# define PDL_LACKS_PLACEMENT_OPERATOR_DELETE
# endif /* __GNUC__ >= 2.91 */

# if __GNUC__ == 2  &&  __GNUC_MINOR__ != 9  &&  __GNUC_MINOR__ != 91
#   // g++ 2.9 and egcs 2.91 apparently have a bug with this . . .
#   define PDL_HAS_TEMPLATE_SPECIALIZATION
# endif /* __GNUC__ != 2.9  &&  __GNUC__ != 2.91*/

  // Some versions of egcs, e.g., egcs-2.90.27 980315 (egcs-1.0.2 release),
  // have bugs with static data members in template classes.
# define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES

  // __EXCEPTIONS is defined with -fexceptions, the egcs default.  It
  // is not defined with -fno-exceptions, the PDL default for g++.
  // PDL_HAS_EXCEPTIONS is defined in
  // makeinclude/wrapper_macros.GNU, so this really isn't
  // necessary.  Just in case . . .
# if defined (__EXCEPTIONS) && !defined (PDL_HAS_EXCEPTIONS)
#   define PDL_HAS_EXCEPTIONS
# endif /* __EXCEPTIONS && ! PDL_HAS_EXCEPTIONS */

# if defined (PDL_HAS_EXCEPTIONS)
/* #   define PDL_NEW_THROWS_EXCEPTIONS */
# endif /* PDL_HAS_EXCEPTIONS */

#else  /* ! egcs */
  // Plain old g++.
# define PDL_LACKS_PLACEMENT_OPERATOR_DELETE
# define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES
# define PDL_HAS_GNUG_PRE_2_8
# define PDL_HAS_TEMPLATE_SPECIALIZATION
# define PDL_LACKS_MIN_MAX_TEMPLATES
#endif /* ! egcs */

#if (defined (i386) || defined (__i386__)) && !defined (PDL_SIZEOF_LONG_DOUBLE)
# define PDL_SIZEOF_LONG_DOUBLE 12
#endif /* i386 */

#if defined (i386) || defined (__i386__)
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

#if defined (PDL_HAS_GNU_REPO)
  // -frepo causes unresolved symbols of basic_string left- and
  // right-shift operators with PDL_HAS_STRING_CLASS.
# if defined (PDL_HAS_STRING_CLASS)
#   undef PDL_HAS_STRING_CLASS
# endif /* PDL_HAS_STRING_CLASS */
#else  /* ! PDL_HAS_GNU_REPO */
# define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
#endif /* ! PDL_HAS_GNU_REPO */
#define PDL_HAS_GNUC_BROKEN_TEMPLATE_INLINE_FUNCTIONS
#define PDL_TEMPLATES_REQUIRE_SOURCE

#endif /* PDL_GNUG_COMMON_H */

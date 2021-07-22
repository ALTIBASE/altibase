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
// config-sunos5.7.h,v 4.14 1999/12/06 14:17:52 schmidt Exp

// The following configuration file is designed to work for SunOS 5.7
// (Solaris 7) platforms using the SunC++ 4.x, 5.x, or g++ compilers.

#ifndef PDL_CONFIG_H

// PDL_CONFIG_H is defined by one of the following #included headers.

// #include the SunOS 5.6 config file, then add SunOS 5.7 updates below.

#include "config-sunos5.7.h"

#if defined (__GNUG__)
# if __GNUC__ <= 2  &&  __GNUC_MINOR__ < 8
    // Assume that later g++ were built on SunOS 5.7, so they don't
    // have these problems . . .

    // Disable the following, because g++ 2.7.2.3 can't handle it.
    // Maybe later g++ versions can?
#   undef PDL_HAS_XPG4_MULTIBYTE_CHAR

    // The Solaris86 g++ 2.7.2.3 sys/types.h doesn't have these . . .
    typedef long          t_scalar_t;  /* historical versions */
    typedef unsigned long t_uscalar_t;
    typedef void          *timeout_id_t;
# endif /* __GNUC__ <= 2  &&  __GNUC_MINOR__ < 8 */

#elif defined (ghs)
  // SunOS 5.7's /usr/include/sys/procfs_isa.h needs uint64_t,
  // but /usr/include/sys/int_types.h doesn't #define it because
  // _NO_LONGLONG is #
# undef PDL_HAS_PROC_FS
# undef PDL_HAS_PRUSAGE_T

#elif defined (__KCC)
typedef unsigned long long uint64_t;
#endif /* __GNUG__ || ghs || __KCC */

// Solaris 5.7 supports SCHED_FIFO and SCHED_RR, as well as SCHED_OTHER.
#undef PDL_HAS_ONLY_SCHED_OTHER

// Solaris 5.7 gets this right...
#undef PDL_HAS_BROKEN_T_ERROR

// Solaris 2.7 can support Real-Time Signals and POSIX4 AIO operations
// are supported.

#if !defined (PDL_HAS_AIO_CALLS)
#define PDL_HAS_AIO_CALLS
#endif /* !PDL_HAS_AIO_CALLS */

#if defined (PDL_POSIX_AIOCB_PROACTOR)
#undef PDL_POSIX_AIOCB_PROACTOR
#endif /* PDL_POSIX_AIOCB_PROACTOR */

// This is anyway default.
#define PDL_POSIX_SIG_PROACTOR

#ifdef PDL_HAS_LIMITED_SELECT
#undef PDL_HAS_LIMITED_SELECT
#endif /* PDL_HAS_LIMITED_SELECT */

#if defined (__sparcv9)
# if defined (__GNUG__)
#  if __GNUC__ >= 3  &&  __GNUC_MINOR__ >= 2
#   undef PDL_LACKS_SYS_NERR
#  else
#   define ERRMAX 256 /* Needed for following define */
#   define PDL_LACKS_SYS_NERR
#   define _LP64
#   define PDL_SIZEOF_LONG 8 /* Needed to circumvent compiler bug #4294969 */
#  endif/* __GNUC__ */
# else
#  define ERRMAX 256 /* Needed for following define */
#  define PDL_LACKS_SYS_NERR
#  define _LP64
#  define PDL_SIZEOF_LONG 8 /* Needed to circumvent compiler bug #4294969 */
# endif
#endif /* __sparcv9 */

#if !defined (PDL_HAS_IP6)
#define PDL_HAS_IP6 // Platform supports IPv6
#endif

#endif /* PDL_CONFIG_H */

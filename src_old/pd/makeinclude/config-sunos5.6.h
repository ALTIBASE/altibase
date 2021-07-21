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
// config-sunos5.6.h,v 4.20 1999/08/26 15:45:27 coryan Exp

// The following configuration file is designed to work for SunOS 5.6
// platforms using the SunC++ 4.x or g++ compilers.

#ifndef PDL_CONFIG_H

// PDL_CONFIG_H is defined by one of the following #included headers.

// #include the SunOS 5.5 config file, then add SunOS 5.6 updates below.

#include "config-sunos5.5.h"
 
#if defined(__GNUC__) && __GNUC__ >= 2 && __GNUC_MINOR__ >= 95
// gcc-2.95 fixes this problem for us!
#undef PDL_HAS_STL_QUEUE_CONFLICT
#endif /* __GNUC__ */

#if (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 199506L) || \
    defined (__EXTENSIONS__)
# define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
# define PDL_HAS_SIGWAIT
// Hack 'cuz -DPOSIX_SOURCE=199506L and -DEXTENSIONS hides this.
# include <sys/types.h>
  extern "C" int madvise(caddr_t, size_t, int);
#endif /* _POSIX_C_SOURCE >= 199506L  ||  __EXTENSIONS__ */

// SunOS 5.6 has AIO calls.
#define PDL_HAS_AIO_CALLS

// Sunos 5.6's aio_* with RT signals is broken.
#define PDL_POSIX_AIOCB_PROACTOR

// SunOS 5.6 has a buggy select
#define PDL_HAS_LIMITED_SELECT

#if !defined(_LP64)
#define FD_SETSIZE      65536  /* PR-1685 by jdlee */
#endif /* !defined(_LP64) */

#endif /* PDL_CONFIG_H */

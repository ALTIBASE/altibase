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
// config-sunos4-g++.h,v 4.26 2000/01/08 02:39:46 schmidt Exp

// for SunOS4 platforms using the GNU g++ compiler

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if ! defined (__PDL_INLINE__)
#define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
// this must appear before its #include.
#define PDL_HAS_STRING_CLASS

#include "config-g++-common.h"
// This config file has not been tested with PDL_HAS_TEMPLATE_SPECIALIZATION.
// Maybe it will work?
#undef PDL_HAS_TEMPLATE_SPECIALIZATION

// Maximum compensation (10 ms) for early return from timed ::select ().
#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID

#define PDL_HAS_CHARPTR_SPRINTF
#define PDL_HAS_UNION_WAIT

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE

// Compiler/platform supports strerror ().
// #define PDL_HAS_STRERROR
#define PDL_HAS_SYS_ERRLIST

// Header files lack t_errno for PDL_TLI.
// #define PDL_LACKS_T_ERRNO

// Compiler/platform uses old malloc()/free() prototypes (ugh).
#define PDL_HAS_OLD_MALLOC

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

// Compiler/platform supports poll().
#define PDL_HAS_POLL

// Compiler/platform defines a union semun for SysV shared memory.
#define PDL_HAS_SEMUN

// Compiler/platform provides the sockio.h file.
#define PDL_HAS_SOCKIO_H

// Compiler/platform supports struct strbuf
#define PDL_HAS_STRBUF_T

// Platform supports STREAMS.
#define PDL_HAS_STREAMS

// Compiler/platform supports SVR4 dynamic linking semantics.
#define PDL_HAS_SVR4_DYNAMIC_LINKING

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H
// Platform supports PDL_TLI tiuser header.
// #define PDL_HAS_TIUSER_H

// Platform has PDL_TLI.
// #define PDL_HAS_TLI

#define PDL_LACKS_U_LONGLONG_T

#define PDL_LACKS_DIFFTIME
#define PDL_HAS_DIRENT

// 10 millisecond fudge factor to account for Solaris timers...
#if !defined (PDL_TIMER_SKEW)
#define PDL_TIMER_SKEW 1000 * 10
#endif /* PDL_TIMER_SKEW */

#define PDL_HAS_SUNOS4_SIGNAL_T
#define PDL_HAS_CPLUSPLUS_HEADERS
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 6))
#define PDL_HAS_SYSENT_H
#endif
#define PDL_HAS_ALLOCA
// Compiler/platform has <alloca.h>
#define PDL_HAS_ALLOCA_H
#define PDL_HAS_SVR4_GETTIMEOFDAY
// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

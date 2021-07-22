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
// config-sunos4-sun4.x-orbix.h,v 4.16 1999/08/30 21:26:47 schmidt Exp

// The following configuration file is designed to work
// for SunOS4 platforms using the SunC++ 4.x compiler.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID

// Maximum compensation (10 ms) for early return from timed ::select ().
#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_HAS_CHARPTR_SPRINTF
#define PDL_HAS_UNION_WAIT

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// PDL sparcworks 4.01 signal handling under SunOS
#define PDL_HAS_SPARCWORKS_401_SIGNALS

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE

// Platform contains Orbix CORBA implementation.
#define PDL_HAS_ORBIX 1

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

// Compiler has brain-damaged SPARCwork signal prototype...
#define PDL_HAS_SPARCWORKS_401_SIGNALS

// Compiler/platform supports struct strbuf
#define PDL_HAS_STRBUF_T

// Platform supports STREAMS.
#define PDL_HAS_STREAMS

// SunOS 4 style prototype.
#define PDL_HAS_SUNOS4_GETTIMEOFDAY

// Compiler/platform supports SVR4 dynamic linking semantics.
#define PDL_HAS_SVR4_DYNAMIC_LINKING

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

// Platform supports PDL_TLI tiuser header.
// #define PDL_HAS_TIUSER_H

// Platform has PDL_TLI.
// #define PDL_HAS_TLI

#define PDL_LACKS_LINEBUFFERED_STREAMBUF
#define PDL_LACKS_SIGNED_CHAR

#define PDL_NEEDS_DEV_IO_CONVERSION

#define PDL_LACKS_U_LONGLONG_T

#define PDL_LACKS_DIFFTIME

// 10 millisecond fudge factor to account for Solaris timers...
#if !defined (PDL_TIMER_SKEW)
#define PDL_TIMER_SKEW 1000 * 10
#endif /* PDL_TIMER_SKEW */

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

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
// config-irix5.3-sgic++.h,v 4.14 1998/10/20 15:35:38 levine Exp

// The following configuration file is designed to work
// for the SGI Indigo2EX running Irix 5.3 platform using
// the SGI C++ Compiler.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define IRIX5

#include <sys/bsd_types.h>
#define _BSD_TYPES

#define PDL_SIZEOF_LONG_DOUBLE 8

#define PDL_LACKS_SYSTIME_H
// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// Platform supports getpagesize() call.
#define PDL_HAS_GETPAGESIZE

#define PDL_LACKS_SYSTIME_H
#define PDL_HAS_SIGWAIT

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Platform requires void * for mmap().
#define PDL_HAS_VOIDPTR_MMAP

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// Compiler/platform supports alloca()
#define PDL_HAS_ALLOCA

// Compiler/platform has <alloca.h>
#define PDL_HAS_ALLOCA_H

// IRIX5 needs to define bzero() in this odd file <bstring.h>
#define PDL_HAS_BSTRING

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// No multi-threading so use poll() call
// - for easier debugging, if nothing else
// #define PDL_USE_POLL

// Platform supports the /proc file system.
// #define PDL_HAS_PROC_FS

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Platform supports SVR4 extended signals.
#define PDL_HAS_SIGINFO_T
// #define PDL_HAS_UCONTEXT_T
#define PDL_LACKS_UCONTEXT_H

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Platform supports STREAMS.
#define PDL_HAS_STREAMS

// Platform supports STREAM pipes (note that this is disabled by
// default, see the manual page on pipe(2) to find out how to enable
// it).
// #define PDL_HAS_STREAM_PIPES

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// Compiler/platform supports struct strbuf.
#define PDL_HAS_STRBUF_T

// Compiler/platform supports SVR4 dynamic linking semantics.
#define PDL_HAS_SVR4_DYNAMIC_LINKING

// Compiler/platform supports SVR4 signal typedef.
#define PDL_HAS_IRIX_53_SIGNALS

// Compiler/platform supports sys_siglist array.
// #define PDL_HAS_SYS_SIGLIST

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

// Compiler/platform defines a union semun for SysV shared memory.
#define PDL_HAS_SEMUN

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

#define PDL_NEEDS_DEV_IO_CONVERSION

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

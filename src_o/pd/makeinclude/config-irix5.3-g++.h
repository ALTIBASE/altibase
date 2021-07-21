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
// config-irix5.3-g++.h,v 4.20 1999/08/30 02:52:49 levine Exp

// The following configuration file is designed to work for the SGI
// Indigo2EX running Irix 5.3 platform using the GNU C++ Compiler

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
// this must appear before its #include.
#define PDL_HAS_STRING_CLASS

#include "config-g++-common.h"

#define PDL_SIZEOF_LONG_DOUBLE 8

#define PDL_LACKS_SYSTIME_H
// Platform supports getpagesize() call.
#define PDL_HAS_GETPAGESIZE
#define IRIX5
#define PDL_HAS_SIGWAIT
#define PDL_HAS_DIRENT

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

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

// Denotes that GNU has cstring.h as standard
// which redefines memchr()
#define PDL_HAS_GNU_CSTRING_H

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// Platform supports the /proc file system.
#define PDL_HAS_PROC_FS

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Platform supports SVR4 extended signals.
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T

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

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

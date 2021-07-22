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
// config-irix5.2.h,v 4.9 1998/10/20 15:35:38 levine Exp

// The following configuration file is designed to work for the SGI
// Indigo2EX running Irix 5.2 platform using the gcc v2.6.x compiler
// and libg++ v2.6.x.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// Platform supports STREAM pipes (note that this is disabled by
// default, see the manual page on pipe(2) to find out how to enable
// it).
//#define PDL_HAS_STREAM_PIPES

// Platform supports getpagesize() call.
#define PDL_HAS_GETPAGESIZE
// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_HAS_SIGWAIT

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Platform requires void * for mmap().
#define PDL_HAS_VOIDPTR_MMAP

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

#define PDL_SIZEOF_LONG_DOUBLE 8

#define IRIX5
#define PDL_HAS_ALLOCA
// Compiler/platform has <alloca.h>
#define PDL_HAS_ALLOCA_H
#define PDL_HAS_BSTRING
#define PDL_HAS_GETRUSAGE
#define PDL_HAS_POSIX_NONBLOCK
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_POLL
#define PDL_HAS_PROC_FS
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T
#define PDL_HAS_STREAMS
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STRERROR
#define PDL_HAS_STRBUF_T
#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_SVR4_SIGNAL_T
#define PDL_HAS_SYS_SIGLIST
#define PDL_HAS_SYS_FILIO_H
#define PDL_HAS_SEMUN
#define PDL_NEEDS_DEV_IO_CONVERSION

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

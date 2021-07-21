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
//
// config-irix6.x-common.h,v 4.13 1999/11/16 13:59:29 schmidt Exp
//
// This file contains the common configuration options for both
// SGI/MIPSPro C++ and g++ under IRIX 6.X
//
#ifndef PDL_CONFIG_IRIX6X_COMMON_H

#if !(defined(PDL_CONFIG_H) || defined(PDL_CONFIG_IRIX6X_NTHR_H))
#error "This file may only be included via config.h or config-irix6.x-nothreads.h"
#endif

#if (defined(PDL_CONFIG_H) && defined(PDL_CONFIG_IRIX6X_NTHR_H))
#error "May only be included via config.h *OR* config-irix6.x-nothreads.h, not both!"
#endif

// The Irix 6.x float.h doesn't allow us to distinguish between a
// double and a long double.  So, we have to hard-code this.  Thanks
// to Bob Laferriere <laferrie@gsao.med.ge.com> for figuring it out.
#if defined (_MIPS_SIM)             /* 6.X System */
# include <sgidefs.h>
# if defined (_MIPS_SIM_NABI32) && (_MIPS_SIM == _MIPS_SIM_NABI32)
#   define PDL_SIZEOF_LONG_DOUBLE 16
# elif defined (_MIPS_SIM_ABI32) && (_MIPS_SIM == _MIPS_SIM_ABI32)
#   define PDL_SIZEOF_LONG_DOUBLE 8
# elif defined (_MIPS_SIM_ABI64) && (_MIPS_SIM == _MIPS_SIM_ABI64)
#   define PDL_SIZEOF_LONG_DOUBLE 16
# elif !defined (PDL_SIZEOF_LONG_DOUBLE)
#   define PDL_SIZEOF_LONG_DOUBLE 8
# endif
#else
# define PDL_SIZEOF_LONG_DOUBLE 8   /* 5.3 System */
#endif

// petern, Next part of it:

// Platform supports getpagesize() call.
#define PDL_HAS_GETPAGESIZE

// Platform has no implementation of pthread_condattr_setpshared(),
// even though it supports pthreads! (like Irix 6.2)
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_MUTEXATTR_PSHARED

// Platform/compiler has the sigwait(2) prototype
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIGTIMEDWAIT
#define PDL_HAS_SIGSUSPEND

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

// Irix needs to define bzero() in this odd file <bstring.h>
#define PDL_HAS_BSTRING

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// No multi-threading so use poll() call
// - for easier debugging, if nothing else
// #define PDL_USE_POLL

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

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

// Compiler/platform defines a union semun for SysV shared memory.
#define PDL_HAS_SEMUN

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

#define PDL_HAS_STRPTIME

//**************************************************************
// Not so sure how next lines should look like

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME
#define PDL_LACKS_SYSTIME_H

//**************************************************************

// IRIX 6.4 and below do not support reentrant netdb functions
// (getprotobyname_r, getprotobynumber_r, gethostbyaddr_r,
// gethostbyname_r, getservbyname_r).
#if PDL_IRIX_VERS <= 64 && !defined (PDL_HAS_NETDB_REENTRANT_FUNCTIONS)
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#endif /* PDL_HAS_NETDB_REENTRANT_FUNCTIONS */

#define PDL_HAS_DIRENT
// Unless the thread enabled version is used the readdir_r interface
// does not get defined in IRIX 6.2
#define PDL_LACKS_READDIR_R
#define PDL_LACKS_RWLOCK_T

#define PDL_HAS_GPERF

#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_BROKEN_DGRAM_SENDV

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_LACKS_PLACEMENT_OPERATOR_DELETE

#endif /* PDL_CONFIG_IRIX6X_COMMON_H */

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
// config-freebsd-pthread.h,v 4.43 2000/01/08 02:39:45 schmidt Exp

// The following configuration file is designed to work for FreeBSD
// platforms with GNU C++ and the POSIX (pthread) threads package.

// Notice that the threaded version of PDL is only supported for -current.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#include <osreldate.h>
// Make sure we source in the OS version.

#if ! defined (__PDL_INLINE__)
#define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if (__FreeBSD_version < 220000)
#error Threads are not supported.
#endif /* __FreeBSD_version < 220000 */

#define PDL_SIZEOF_LONG_DOUBLE 12

#if defined (__GNUG__)
# include "config-g++-common.h"
#endif /* __GNUG__ */

#if defined (PDL_HAS_PENTIUM)
# undef PDL_HAS_PENTIUM
#endif /* PDL_HAS_PENTIUM */

// Platform specific directives
// gcc defines __FreeBSD__ automatically for us.
#if !defined (_THREAD_SAFE)
#define _THREAD_SAFE
#endif /* _THREAD_SAFE */

#define PDL_HAS_GPERF

#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS
#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_READDIR_R
#define PDL_HAS_SIG_MACROS
// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_CHARPTR_DL
//#define PDL_USES_ASM_SYMBOL_IN_DLSYM
#define PDL_LACKS_SIGSET
#define PDL_NEEDS_SCHED_H

// Use of <malloc.h> is deprecated.
#define PDL_LACKS_MALLOC_H

// sched.h still not fully support on FreeBSD ?
// this is taken from /usr/src/lib/libc_r/uthread/pthread-private.h
enum schedparam_policy {
        SCHED_RR,
        SCHED_IO,
        SCHED_FIFO,
        SCHED_OTHER
};

// Platform supports POSIX timers via struct timespec.
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_UALARM

// Platform defines struct timespec but not timespec_t
#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_SYSTIME_H

#define PDL_LACKS_STRRECVFD

#define PDL_HAS_SIN_LEN

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

#if (__FreeBSD_version >= 300000)
#define PDL_HAS_SIGINFO_T
#endif /* __FreeBSD_version >= 300000 */

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define PDL_LACKS_SIGINFO_H
#define PDL_LACKS_UCONTEXT_H
#define PDL_LACKS_SI_ADDR

// Compiler/platform supports SVR4 signal typedef
#define PDL_HAS_SVR4_SIGNAL_T

// Compiler/platform supports alloca().
#define PDL_HAS_ALLOCA

// Compiler/platform supports SVR4 dynamic linking semantics..
#define PDL_HAS_SVR4_DYNAMIC_LINKING

// Compiler/platform correctly calls init()/fini() for shared libraries.
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Explicit dynamic linking permits "lazy" symbol resolution
#define PDL_HAS_RTLD_LAZY_V

// platform supports POSIX O_NONBLOCK semantics
#define PDL_HAS_POSIX_NONBLOCK

// platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Compiler/platform has <alloca.h>
//#define PDL_HAS_ALLOCA_H

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Compiler/platform supports sys_siglist array.
// *** This refers to (_sys_siglist) instead of (sys_siglist)
// #define PDL_HAS_SYS_SIGLIST

// Compiler/platform defines a union semun for SysV shared memory.
#define PDL_HAS_SEMUN

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// Compiler/platform provides the sockio.h file.
#define PDL_HAS_SOCKIO_H

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

// Compiler/platform supports SVR4 gettimeofday() prototype
#define PDL_HAS_SUNOS4_GETTIMEOFDAY
// #define PDL_HAS_TIMEZONE_GETTIMEOFDAY

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_HAS_MSG
#define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#define PDL_HAS_NONCONST_MSGSND

// Thread specific settings
// Yes, we do have threads.
#define PDL_HAS_THREADS
// And they're even POSIX pthreads
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif /* ! PDL_MT_SAFE */
#define PDL_HAS_PTHREADS
#define PDL_LACKS_SETSCHED
#define PDL_LACKS_PTHREAD_CANCEL
#define PDL_LACKS_THREAD_PROCESS_SCOPING
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_DIRENT

#define PDL_HAS_SIGWAIT

#define PDL_HAS_TERM_IOCTLS
#define PDL_USES_OLD_TERMIOS_STRUCT
#define PDL_USES_HIGH_BAUD_RATES
#define TCGETS TIOCGETA
#define TCSETS TIOCSETA

#endif /* PDL_CONFIG_H */

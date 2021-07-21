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
// config-macosx.h,v 4.9 2002/02/10 22:55:19 schmidt Exp

// This configuration file is designed to work with the MacOS X operating system.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if ! defined (__PDL_INLINE__)
#define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if defined (__GNUG__)
# include "config-g++-common.h"
#endif /* __GNUG__ */

#undef PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION

#define PDL_SIZE_T_FORMAT_SPECIFIER PDL_LIB_TEXT ("%lu")

#if defined (PDL_HAS_PENTIUM)
# undef PDL_HAS_PENTIUM
#endif /* PDL_HAS_PENTIUM */

// Platform specific directives

#define __MACOSX__
#define PDL_HAS_MACOSX_DYLIB

#if !defined (_THREAD_SAFE)
#define _THREAD_SAFE
#endif /* _THREAD_SAFE */

#define PDL_HAS_GPERF
//#define PDL_HAS_POSIX_SEM

//#define PDL_HAS_SVR4_TLI

#define PDL_HAS_MEMCHR

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Compiler/platform supports SVR4 signal typedef
#define PDL_HAS_SVR4_SIGNAL_T

//Platform/compiler has the sigwait(2) prototype
#define PDL_HAS_SIGWAIT

//Platform supports sigsuspend()
#define PDL_HAS_SIGSUSPEND

//Platform/compiler has macros for sig{empty,fill,add,del}set (e.g., SCO and FreeBSD)
#define PDL_HAS_SIG_MACROS

//#define PDL_LACKS_GETPGID // mac supports getpgid
#define PDL_LACKS_RWLOCK_T // mac supports pthread_rwlock_tryrdlock but do not use it

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_HAS_NONCONST_SELECT_TIMEVAL

// mac supports sigprocmaks
//#define PDL_LACKS_SIGSET

#define PDL_NEEDS_SCHED_H

// Use of <malloc.h> is deprecated.
// malloc.h is in /usr/include/malloc.
#define PDL_LACKS_MALLOC_H

#define PDL_HAS_ALT_CUSERID

// Platform supports POSIX timers via struct timespec.
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_UALARM

// Platform defines struct timespec but not timespec_t
#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_STRRECVFD

#define PDL_HAS_SIN_LEN

#define PDL_HAS_ANSI_CASTS

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform supports alloca().
#define PDL_HAS_ALLOCA

// Compiler/platform has <alloca.h>
#define PDL_HAS_ALLOCA_H


// Compiler/platform correctly calls init()/fini() for shared libraries.
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Explicit dynamic linking permits "lazy" symbol resolution
//#define PDL_HAS_RTLD_LAZY_V

// platform supports POSIX O_NONBLOCK semantics
#define PDL_HAS_POSIX_NONBLOCK

// platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE
#define PDL_HAS_GETRUSAGE_PROTO

// Compiler/platform defines a union semun for SysV shared memory.
//#define PDL_LACKS_SEMBUF_T

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// Compiler/platform provides the sockio.h file.
#define PDL_HAS_SOCKIO_H

// Defines the page size of the system.
#define PDL_HAS_GETPAGESIZE

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

// Compiler/platform supports SVR4 gettimeofday() prototype
#define PDL_HAS_SUNOS4_GETTIMEOFDAY
#define PDL_HAS_TIMEZONE_GETTIMEOFDAY

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

//#define PDL_LACKS_SYSV_MSG_H
#define PDL_HAS_MSG
#define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#define PDL_HAS_NONCONST_MSGSND

// And they're even POSIX pthreads
// Yes, we do have threads.
#define PDL_HAS_THREADS
// And they're even POSIX pthreads (MIT implementation)
#define PDL_HAS_PTHREADS
// ... and the final standard even!
#define PDL_HAS_PTHREADS_STD

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif /* ! PDL_MT_SAFE */

#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PTHREAD_MIN_PRIORITY            0       // JCEJ 12/22/96        #3
#if !defined(PDL_LACKS_PTHREAD_SIGMASK)
#  define PTHREAD_MAX_PRIORITY          99      // CJC  02/11/97
#else
#  define PTHREAD_MAX_PRIORITY          32      // JCEJ 12/22/96        #3
#endif

#define PDL_LACKS_THREAD_STACK_ADDR             // JCEJ 12/17/96
#define PDL_LACKS_THREAD_STACK_SIZE             // JCEJ 12/17/96
#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS    // JCEJ 1/7-8/96




#define PDL_HAS_DIRENT

//#define PDL_LACKS_SETSCHED // mac supports pthread_getschedparam
//#define PDL_HAS_RECURSIVE_MUTEXES

#define PDL_HAS_TERM_IOCTLS
#define TCGETS TIOCGETA
#define TCSETS TIOCSETA



//by gurugio
#define PDL_HAS_SEMUN
//#define __GLIBC__
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T
#define _XOPEN_SOURCE
#define PDL_HAS_SOCKLEN_T

#define PDL_HAS_SVR4_DYNAMIC_LINKING
// Compiler/platform supports sys_siglist array.
// *** This refers to (_sys_siglist) instead of (sys_siglist)
#define PDL_HAS_SYS_SIGLIST


#if defined(__GLIBC__)
// Platform supports reentrant functions (i.e., all the POSIX *_r
// functions).
#define PDL_HAS_REENTRANT_FUNCTIONS
// getprotobyname_r have a different signature!
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
// uses ctime_r & asctime_r with only two parameters vs. three
#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
#endif

#include /**/ <pthread.h>

// enable IPv6 features such as getaddrinfo
#define PDL_HAS_IP6

# define PDL_UINT64_FORMAT_SPECIFIER "%Lu"

#endif /* PDL_CONFIG_H */

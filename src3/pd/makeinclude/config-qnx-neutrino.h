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
 
// config-qnx-neutrino.h,v 4.13 1999/10/12 19:28:23 levine Exp

// The following configuration file is designed to work for Neutrino
// 2.0 (Beta) with GNU C++ and the POSIX (pthread) threads package.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define _POSIX_C_SOURCE 199506
#define _QNX_SOURCE

// These constants are in i386-nto/include/limits.h, but egcs
// picks up its own limits.h instead:
#define _POSIX_NAME_MAX     14      /*  Max bytes in a filename             */
#define _POSIX_PATH_MAX     256     /*  Num. bytes in pathname (excl. NULL) */

// gcc can do inline
#if __GNUC__ > 2 || ( __GNUC__ == 2 && __GNUC_MINOR__ >= 8)
# if !defined (__PDL_INLINE__)
#   define __PDL_INLINE__
# endif /* ! __PDL_INLINE__ */
#endif

#if defined(__OPTIMIZE__)
# if defined(__X86__)
    // string.h can't be used by PDL with __OPTIMIZE__.
#   undef __OPTIMIZE__
#   include <string.h>
#   define __OPTIMIZE__
# endif /* __X86__ */
#endif /* __OPTIMIZE__ */

#include "config-g++-common.h"

// The following defines the Neutrino compiler.
// gcc should know to call g++ as necessary
#ifdef __GNUC__
# define PDL_CC_NAME "gcc"
#else
# define PDL_CC_NAME "NTO compiler ??"
#endif

// /usr/nto/include/float.h defines
//  FLT_MAX_EXP 127
//  DBL_MAX_EXP 1023
//  pdl expects 128 & 1024 respectively
//  to set the following macros in Basic_Types.h
//  These macros are:
// #define PDL_SIZEOF_DOUBLE   8
// #define PDL_SIZEOF_FLOAT    4

#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
#define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#define PDL_HAS_ALLOCA
#define PDL_HAS_ALLOCA_H
#define PDL_HAS_AUTOMATIC_INIT_FINI
#define PDL_HAS_CLOCK_GETTIME
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define PDL_HAS_DIRENT
#define PDL_HAS_GETPAGESIZE
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_NONSTATIC_OBJECT_MANAGER
/* #define PDL_HAS_INLINED_OSCALLS */ /* by jdlee */
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_MSG
#define PDL_HAS_MT_SAFE_MKTIME
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_POSIX_SEM
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD
#define PDL_HAS_PTHREAD_SIGMASK
#define PDL_HAS_P_READ_WRITE
#define PDL_HAS_REENTRANT_FUNCTIONS
#define PDL_HAS_SELECT_H
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_SIGISMEMBER_BUG
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_SIG_MACROS
#define PDL_HAS_SIN_LEN
#define PDL_HAS_SIZET_SOCKET_LEN
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STRERROR
#define PDL_HAS_SVR4_GETTIMEOFDAY
#define PDL_HAS_TERM_IOCTLS
#define PDL_HAS_THREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_THR_C_DEST
#define PDL_HAS_THR_C_FUNC
#define PDL_HAS_TIMEZONE_GETTIMEOFDAY
#define PDL_HAS_UALARM
#define PDL_HAS_UCONTEXT_T
#define PDL_HAS_VOIDPTR_MMAP
#define PDL_HAS_VOIDPTR_SOCKOPT
#define PDL_LACKS_CMSG_DATA_MEMBER
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_CONST_TIMESPEC_PTR
/* #define PDL_LACKS_FORK */ /* by jdlee */
#define PDL_LACKS_LINEBUFFERED_STREAMBUF
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MSGBUF_T
#define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_NAMED_POSIX_SEM
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
/* #define PDL_LACKS_RLIMIT */ /* by jdlee */
/* #define PDL_LACKS_RLIMIT_PROTOTYPE */ /* by jdlee */
#define PDL_LACKS_RPC_H
#define PDL_LACKS_RTTI
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SBRK
#define PDL_LACKS_SEEKDIR
#define PDL_LACKS_SOCKET_BUFSIZ
#define PDL_LACKS_SOCKETPAIR
#define PDL_LACKS_SOME_POSIX_PROTOTYPES
#define PDL_LACKS_STREAM_MODULES
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_SYSCALL
#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_SYSV_MSG_H
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_TCP_NODELAY
#define PDL_LACKS_TELLDIR
#define PDL_LACKS_TIMESPEC_T
#define PDL_LACKS_TRUNCATE
#define PDL_LACKS_T_ERRNO
#define PDL_LACKS_UALARM_PROTOTYPE
#define PDL_LACKS_UCONTEXT_H
#define PDL_LACKS_UNIX_DOMAIN_SOCKETS
#define PDL_LACKS_U_LONGLONG_T
#define PDL_MT_SAFE 1
#define PDL_NEEDS_FUNC_DEFINITIONS
#define PDL_NEEDS_HUGE_THREAD_STACKSIZE 64000
#define PDL_TEMPLATES_REQUIRE_SOURCE
#define PDL_THR_PRI_FIFO_DEF 10
#define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
#define PDL_HAS_SIGTIMEDWAIT
#define PDL_HAS_SIGSUSPEND

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#if !defined(_LP64)
#define FD_SETSIZE      65536  /* PR-1685 by jdlee */
#endif /* !defined(_LP64) */

#endif /* PDL_CONFIG_H */

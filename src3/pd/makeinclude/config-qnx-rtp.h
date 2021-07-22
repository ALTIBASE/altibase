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
 
// config-qnx-rtp.h,v 1.7 2001/04/16 23:20:37 coryan Exp

// The following configuration file is designed to work for QNX RTP
// GNU C++ and the POSIX (pthread) threads package. You can get QNX
// RTP at http://get.qnx.com

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
# define PDL_CC_NAME "QNX-RTP compiler ??"
#endif

// /usr/nto/include/float.h defines
//  FLT_MAX_EXP 127
//  DBL_MAX_EXP 1023
//  pdl expects 128 & 1024 respectively
//  to set the following macros in pdl/Basic_Types.h
//  These macros are:
#define PDL_SIZEOF_DOUBLE   8
#define PDL_SIZEOF_FLOAT    4

/////////////////////////////////////////////////////////////////
//    Definition of the features that are available.
//
//                PDL_HAS Section
/////////////////////////////////////////////////////////////////

#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
#define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#define PDL_HAS_ALLOCA
#define PDL_HAS_ALLOCA_H
#define PDL_HAS_AUTOMATIC_INIT_FINI
#define PDL_HAS_CLOCK_GETTIME
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define PDL_HAS_DIRENT
#define PDL_HAS_GETPAGESIZE
// Enable gperf, this is a hosted configuration.
#define PDL_HAS_GPERF
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
//#define PDL_HAS_NONSTATIC_OBJECT_MANAGER
#define PDL_HAS_INLINED_OSCALLS
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_MSG
#define PDL_HAS_MT_SAFE_MKTIME
#define PDL_HAS_MUTEX_TIMEOUTS
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
#define PDL_HAS_STRINGS
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

/////////////////////////////////////////////////////////////////
//    Definition of the features that are not available.
//
//                PDL_LACKS Section
/////////////////////////////////////////////////////////////////
#define PDL_LACKS_CMSG_DATA_MEMBER
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_CONST_TIMESPEC_PTR
#define PDL_LACKS_LINEBUFFERED_STREAMBUF
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MSGBUF_T
#define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_NAMED_POSIX_SEM
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
#define PDL_LACKS_RPC_H
#define PDL_LACKS_RTTI
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SBRK
#define PDL_LACKS_SEEKDIR
#define PDL_LACKS_SOCKET_BUFSIZ
#define PDL_LACKS_SOCKETPAIR     // Even if the QNX RTP docs says that socket pair are
                                 // available, there is actually no implementation of
				 // soket-pairs.
#define PDL_LACKS_SOME_POSIX_PROTOTYPES
#define PDL_LACKS_STREAM_MODULES
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_SYSCALL
#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_SYSV_MSG_H
#define PDL_LACKS_SYSV_SHMEM
//#define PDL_LACKS_TCP_NODELAY  // Based on the  QNX RTP documentation, this option seems to
                                 // to be supported.
#define PDL_LACKS_TELLDIR
#define PDL_LACKS_TIMESPEC_T
#define PDL_LACKS_TRUNCATE
#define PDL_LACKS_T_ERRNO
#define PDL_LACKS_UALARM_PROTOTYPE
#define PDL_LACKS_UCONTEXT_H
#define PDL_LACKS_UNIX_DOMAIN_SOCKETS
#define PDL_LACKS_U_LONGLONG_T

#define PDL_LACKS_RLIMIT         // QNX rlimit syscalls don't work properly with PDL.

#define PDL_MT_SAFE 1
#define PDL_NEEDS_FUNC_DEFINITIONS
#define PDL_NEEDS_HUGE_THREAD_STACKSIZE 64000
#define PDL_TEMPLATES_REQUIRE_SOURCE
#define PDL_THR_PRI_FIFO_DEF 10
#define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
#define PDL_HAS_SIGTIMEDWAIT
#define PDL_HAS_SIGSUSPEND

#define PDL_HAS_BROKEN_PREALLOCATED_OBJECTS_AFTER_FORK 1

#define PDL_SIZEOF_WCHAR 4

// Not really, but the prototype returns wchar_t instead of wchar_t *
#define PDL_LACKS_WCSSTR

// No prototypes
#define PDL_LACKS_ITOW
#define PDL_LACKS_WCSICMP
#define PDL_LACKS_WCSNICMP
#define PDL_LACKS_WCSDUP

// And these have prototypes but no implementation
#define PDL_LACKS_WCSLEN
#define PDL_LACKS_WCSNCMP
#define PDL_LACKS_WCSCPY
#define PDL_LACKS_WCSNCPY
#define PDL_LACKS_TOWLOWER
#define PDL_LACKS_WCSCMP
#define PDL_LACKS_WCSCAT
#define PDL_LACKS_WCSNCAT
#define PDL_LACKS_WCSSPN
#define PDL_LACKS_WCSCHR
#define PDL_LACKS_WCSPBRK
#define PDL_LACKS_WCSRCHR

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

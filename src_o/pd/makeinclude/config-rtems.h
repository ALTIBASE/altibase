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
 
/* -*- C -*- */
// config-rtems.h,v 4.2 2001/08/30 01:02:38 kitty Exp

/* The following configuration file is designed to work for RTEMS
   platforms using GNU C.
*/

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// begin of linux-common.h

/* #define PDL_HAS_BYTESEX_H */

#if ! defined (__PDL_INLINE__)
#define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// Needed to make some prototypes visible.
// #if ! defined (_GNU_SOURCE)
// #define _GNU_SOURCE
// #endif /* ! _GNU_SOURCE */

// First the machine specific part
//   There are no known port specific issues with the RTEMS port of PDL.
//   XXX Pentium and PowerPC have high res timer support in PDL.

// Then the compiler specific parts
#if defined (__GNUG__)
  // config-g-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
  // this must appear before its #include.
# define PDL_HAS_STRING_CLASS
# include "config-g-common.h"
#elif defined (__KCC)
# define PDL_HAS_STRING_CLASS
# include "config-kcc-common.h"
#elif defined (__DECCXX)
# define PDL_CONFIG_INCLUDE_CXX_COMMON
# include "config-cxx-common.h"
#else  /* ! __GNUG__ && ! __KCC && !__DECCXX */
# error unsupported compiler in config-linux-common.h
#endif /* ! __GNUG__ && ! __KCC */

// Completely common part :-)

#define PDL_HAS_NONSTATIC_OBJECT_MANAGER
# if !defined (PDL_MAIN)
#   define PDL_MAIN pdl_main
# endif /* ! PDL_MAIN */

// Yes, we do have threads.
#define PDL_HAS_THREADS
// And they're even POSIX pthreads (MIT implementation)
#define PDL_HAS_PTHREADS
// ... and the final standard even!
#define PDL_HAS_PTHREADS_STD
#define PDL_HAS_THREAD_SPECIFIC_STORAGE

// XXX thread defines go here
#define PDL_MT_SAFE 1
#define PDL_PAGE_SIZE 4096
#define PDL_HAS_ALT_CUSERID
#define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#define PDL_HAS_CLOCK_GETTIME
/* #define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES */
#define PDL_HAS_DIRENT
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
/* #define PDL_HAS_LLSEEK */
#define PDL_HAS_MEMCHR
#define PDL_HAS_MSG
#define PDL_HAS_MT_SAFE_MKTIME
#define PDL_HAS_POSIX_SEM
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_PROCESS_ENUM
#define PDL_HAS_REENTRANT_FUNCTIONS
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_SIGSUSPEND
#define PDL_HAS_SSIZE_T
/* #define PDL_HAS_STANDARD_CPP_LIBRARY */
#define PDL_HAS_STRERROR
#define PDL_HAS_SUNOS4_GETTIMEOFDAY
#define PDL_HAS_SYS_ERRLIST
#define PDL_HAS_SYS_FILIO_H
#define PDL_HAS_TIMEZONE_GETTIMEOFDAY
#define PDL_LACKS_DIFFTIME
#define PDL_LACKS_EXEC
#define PDL_LACKS_FILELOCKS
#define PDL_LACKS_FORK
#define PDL_LACKS_GETOPT_PROTO
#define PDL_LACKS_GETPGID
#define PDL_LACKS_TIMESPEC_T
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MKFIFO
#define PDL_LACKS_MMAP
#define PDL_LACKS_MPROTECT
#define PDL_LACKS_MSYNC
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
/* #define PDL_LACKS_POSIX_PROTOTYPES */
  // ... for System V functions like shared memory and message queues
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
#define PDL_LACKS_READDIR_R
#define PDL_LACKS_READLINK
#define PDL_HAS_BROKEN_READV
#define PDL_LACKS_READV
#define PDL_LACKS_RLIMIT
#define PDL_LACKS_RLIMIT_PROTOTYPE
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SBRK
#define PDL_LACKS_SEMBUF_T
#define PDL_LACKS_SETREUID
#define PDL_LACKS_SETREUID_PROTOTYPE
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREGID_PROTOTYPE
#define PDL_LACKS_SOME_POSIX_PROTOTYPES
  // ... for System V functions like shared memory and message queues
#define PDL_LACKS_NATIVE_STRPTIME
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_SI_ADDR
#define PDL_LACKS_SOCKETPAIR
#define PDL_LACKS_SYSV_MSG_H
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_SYSCALL
#define PDL_LACKS_SYS_NERR
#define PDL_LACKS_UALARM_PROTOTYPE
#define PDL_LACKS_UCONTEXT_H
#define PDL_HAS_BROKEN_WRITEV
#define PDL_LACKS_WRITEV
#define PDL_NEEDS_HUGE_THREAD_STACKSIZE 65536
#define PDL_NEEDS_SCHED_H
#define PDL_HAS_POSIX_NONBLOCK

/*  What should these be set to?.

PDL_TLI_TCP_DEVICE
PDL_HAS_RECVFROM_TIMEDWAIT
PDL_HAS_SEND_TIMEDWAIT
PDL_HAS_SENDTO_TIMEDWAIT
PDL_HAS_IP_MULTICAST
PDL_HAS_NONCONST_SELECT_TIMEVAL
PDL_HAS_WCHAR_TYPEDEFS_CHAR
PDL_HAS_WCHAR_TYPEDEFS_USHORT
PDL_HAS_SIGNAL_SAFE_OS_CALLS
PDL_HAS_SIZET_SOCKET_LEN
PDL_HAS_SOCKADDR_MSG_NAME
PDL_HAS_SOCKLEN_T
PDL_HAS_STRBUF_T
PDL_HAS_SYS_SIGLIST
PDL_HAS_TERM_IOCTLS
PDL_HAS_THREAD_SAFE_ACCEPT
PDL_LACKS_COND_TIMEDWAIT_RESET
PDL_LACKS_MSG_ACCRIGHTS
PDL_LACKS_NETDB_REENTRANT_FUNCTIONS

Why don't we have alloca.h?
 */

#include /**/ <pthread.h>

extern "C"
{
  int getopt (int, char *const *, const char *);
}

#endif /* PDL_CONFIG_H */

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
// config-psos-diab-ppc.h,v 4.7 2000/01/02 06:51:20 cdgill Exp

// The following configuration file is designed to work for:
//      a) pSOSystem V2.5.0 for PowerPC (pssppc.250)
//      b) DiabData C++ compiler - dplus 4.2b
//        *note pSOSystem V2.5.0 is contained in the PRISM+2.0.0 package


#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// #if ! defined (__PDL_INLINE__)
// # define __PDL_INLINE__
// #endif /* ! __PDL_INLINE__ */
#if defined (__PDL_INLINE__)
# undef __PDL_INLINE__
#endif /* __PDL_INLINE__ */

#define PDL_LACKS_ISATTY

#define PDL_LACKS_DIFFTIME

#define PDL_LACKS_FCNTL

#define PDL_LACKS_FILELOCKS

#define PDL_LACKS_FSYNC

#define PDL_LACKS_INLINE_FUNCTIONS

#define PDL_LACKS_TEMPLATE_AS_TEMPLATE_PARAMETER

#if defined (__GNUG__)
# include "config-g++-common.h"
#endif /* __GNUG__ */

#define PDL_PSOS_LACKS_PREPC

#define PDL_PSOS_HAS_TIME

#define PDL_HAS_STRDUP_EMULATION

#define PDL_HAS_IP_MULTICAST

#define PDL_HAS_CPLUSPLUS_HEADERS

#define PDL_HAS_BROKEN_EXPLICIT_TYPECAST_OPERATOR_INVOCATION

#define PDL_HAS_BROKEN_CONDITIONAL_STRING_CASTS

#define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR

# define PDL_HAS_BROKEN_NOOP_DTORS

# define PDL_HAS_DIRENT
# define PDL_LACKS_READDIR_R
# define PDL_LACKS_TELLDIR
# define PDL_LACKS_SEEKDIR
# define PDL_LACKS_REWINDDIR

#define PDL_LACKS_TRUNCATE

// Template instantiation definitions
// #define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_FILE
#define PDL_TEMPLATES_REQUIRE_SOURCE

// IO Streams stuff
// #define PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION
//#define PDL_LACKS_IOSTREAM_TOTALLY
//#define PDL_LACKS_PDL_IOSTREAM
//#define PDL_LACKS_IOSTREAM_FX
//#define PDL_DEFAULT_LOG_STREAM 0
//#define ostream FILE

#define PDL_LACKS_HRTIME_T

// #define PDL_LACKS_EVENT_T

#define PDL_HAS_VERBOSE_NOTSUP

#define PDL_LACKS_MEMORY_H

// #define PDL_LACKS_MALLOC_H

#define PDL_LACKS_MMAP

#define PDL_LACKS_UNIX_DOMAIN_SOCKETS

#define PDL_HAS_NONSTATIC_OBJECT_MANAGER

#define PDL_HAS_NONCONST_SELECT_TIMEVAL

#define PDL_LACKS_SEMBUF_T

#define PDL_LACKS_EXEC

#define PDL_LACKS_FORK

#define PDL_LACKS_WRITEV

#define PDL_LACKS_READV

// rename the main entry point
#define PDL_MAIN extern "C" void root

// All this was commented out for the single threaded port

#define PDL_HAS_THREADS

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

// #define PDL_HAS_TSS_EMULATION
// #define PDL_DEFAULT_THREAD_KEYS 256
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_PSOS_HAS_TSS
#define PDL_DEFAULT_THREAD_KEYS KC_NTSD

#define PDL_PSOS_HAS_MUTEX
#define PDL_PSOS_HAS_PRIO_MUTEX
#define PDL_PSOS_HAS_PRIO_INHERIT_MUTEX
#define PDL_HAS_RECURSIVE_MUTEXES

// #define PDL_LACKS_COND_T
#define PDL_PSOS_HAS_COND_T

#define PDL_LACKS_PTHREAD_THR_SIGSETMASK

/* #define PDL_HAS_BROKEN_SENDMSG */

/* #define PDL_HAS_BROKEN_WRITEV  */

#define PDL_HAS_BROKEN_CONVERSIONS

#define PDL_HAS_CHARPTR_SOCKOPT

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_HAS_MSG

#define PDL_HAS_POSIX_NONBLOCK

#define PDL_HAS_SIGINFO_T
#define PDL_LACKS_SIGINFO_H
#define PDL_SIGINFO_IS_SIGINFO_T
#define PDL_LACKS_SIGSET_DEFINITIONS
#define PDL_LACKS_SIGSET
#define PDL_LACKS_SIGACTION

#define PDL_HAS_SIGWAIT

//#define PDL_HAS_SIG_ATOMIC_T

#define PDL_HAS_STRERROR

#define PDL_LACKS_ACCESS

#define PDL_LACKS_GETHOSTENT

#define PDL_LACKS_GETSERVBYNAME

#define PDL_LACKS_KEY_T

#define PDL_LACKS_LINEBUFFERED_STREAMBUF

#define PDL_LACKS_LONGLONG_T

#define PDL_LACKS_LSTAT

#define PDL_LACKS_MADVISE

#define PDL_LACKS_MKTEMP

#define PDL_LACKS_MPROTECT

#define PDL_LACKS_MSYNC

#define PDL_LACKS_PARAM_H

#define PDL_LACKS_PWD_FUNCTIONS

#define PDL_LACKS_READLINK

#define PDL_LACKS_RLIMIT

#define PDL_LACKS_RWLOCK_T

#define PDL_LACKS_SBRK

#define PDL_LACKS_SIGNED_CHAR

#define PDL_LACKS_SI_ADDR

#define PDL_LACKS_SOCKETPAIR

#define PDL_LACKS_STRCASECMP

#define PDL_LACKS_STRRECVFD

#define PDL_LACKS_SYSCALL

#define PDL_LACKS_SYSV_MSG_H

#define PDL_LACKS_SYSV_SHMEM

#define PDL_LACKS_SYS_NERR

#define PDL_LACKS_SYS_TYPES_H

#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_UCONTEXT_H

#define PDL_LACKS_UNIX_SIGNALS

#defined PDL_THREADS_LACK_SHARED_SOCKETS

// #define PDL_MALLOC_ALIGN 8
// #define PDL_LACKS_SYSTIME_H

#define PDL_PAGE_SIZE 4096

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_PSOS_CANT_USE_SYS_TYPES

// #define PDL_PSOS_LACKS_PHILE

#define PDL_PSOS_HAS_C_LIBRARY

#define PDL_PSOS_SNARFS_HEADER_INFO

#define PDL_PSOS_LACKS_ARGC_ARGV

#if !defined (PDL_PSOS)
#define PDL_PSOS
#endif /* PDL_PSOS */

#if !defined (PDL_PSOS_TBD)
#define PDL_PSOS_TBD
#endif /* PDL_PSOS_TBD */

#define PDL_LACKS_MKFIFO

#define PDL_PSOS_DIAB

#define PDL_PSOS_DIAB_PPC

/* Defining PNANOREDEF will avoid the redefinition of close to pna_close in
 * pna.h  In OS.i we make sure that pna_close is called in place of close
 * when a socket is closed.
 */
#define _PNANOREDEF_

/* needed for portability, due to non-standard ip_mreq
   structure definition for pSOS Diab PPC */
#define IMR_MULTIADDR imr_mcastaddr

#define PDL_THREADS_DONT_INHERIT_LOG_MSG  /* Stream sharing problem for tasks */

#define PDL_THREADS_MUST_EXPLICITLY_SHARE_SOCKETS /* Can't just share sockets */

#define PDL_PSOS_DEFAULT_STACK_SIZE 30720  /* Default task stack size to 30K */

#define PDL_DEFAULT_THREAD_PRIORITY 239

#define PDL_THREAD_MANAGER_LACKS_STATICS

#endif /* PDL_CONFIG_H */

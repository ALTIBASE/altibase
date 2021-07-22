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
// config-psos-tm.h,v 4.16 1999/08/22 00:15:54 schmidt Exp

// The following configuration file is designed to work for pSOSystem V2.2.1,
// using the Diab Data D-C++ 4.1a compiler

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// #if ! defined (__PDL_INLINE__)
// # define __PDL_INLINE__
// #endif /* ! __PDL_INLINE__ */
#if defined (__PDL_INLINE__)
# undef __PDL_INLINE__
#endif /* __PDL_INLINE__ */

#if defined (__GNUG__)
# include "config-g++-common.h"
#endif /* __GNUG__ */

#define PDL_HAS_STRDUP_EMULATION

#define PDL_HAS_IP_MULTICAST

#define PDL_HAS_CPLUSPLUS_HEADERS

#define PDL_HAS_WORKING_EXPLICIT_TEMPLATE_DESTRUCTOR

// Template instantiation definitions
// #define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION_FILE
#define PDL_TEMPLATES_REQUIRE_SOURCE

#define PDL_DONT_INCLUDE_PDL_SIGNAL_H
#  include <signal.h>        //JINLU
#undef PDL_DONT_INCLUDE_PDL_SIGNAL_H
#include <pna.h>             //JINLU
#define NSIG _NSIG           //_NSIG = 32 defined in signal.h JINLU
#define TCP_NODELAY  1       // this will be defined later by pSOS/TM

/* #define PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION */

#define PDL_LACKS_HRTIME_T

// #define PDL_LACKS_EVENT_T

#define PDL_HAS_VERBOSE_NOTSUP

#define PDL_LACKS_MEMORY_H

// #define PDL_LACKS_MALLOC_H

#define PDL_LACKS_MMAP

#define PDL_LACKS_UNIX_DOMAIN_SOCKETS

#define PDL_HAS_NONSTATIC_OBJECT_MANAGER

#define PDL_LACKS_SEMBUF_T

#define PDL_LACKS_EXEC

#define PDL_LACKS_FORK

#define PDL_LACKS_WRITEV

#define PDL_LACKS_READV

// rename the main entry point
// #define PDL_MAIN extern "C" void root

// All this was commented out for the single threaded port

#define PDL_HAS_THREADS

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#define PDL_HAS_TSS_EMULATION

#define PDL_DEFAULT_THREAD_KEYS 256

#define PDL_LACKS_COND_T

// #define PDL_HAS_TSS_EMULATION

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

#define PDL_HAS_SIGWAIT

#define PDL_HAS_SIG_ATOMIC_T

#define PDL_HAS_STRERROR

#define PDL_LACKS_ACCESS

#define PDL_LACKS_GETHOSTENT

#define PDL_LACKS_GETSERVBYNAME

#define PDL_LACKS_IOSTREAM_FX

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

#define PDL_LACKS_SYS_NERR    // psos/tm does not have sys_nerr

#define PDL_LACKS_SYS_TYPES_H

#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_UCONTEXT_H

#define PDL_LACKS_UNIX_SIGNALS

// #define PDL_LACKS_SYSTIME_H

#define PDL_PAGE_SIZE 4096

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_PSOS_TM
#define PDL_PSOS_PROVIDES_ERROR_SYMBOLS_TM
#define USER_INCLUDE_SYS_TIME_TM
#define PDL_LACKS_FILELOCKS
#define PDL_LACKS_SIGSET
#define PDL_LACKS_SIGACTION
#define PDL_LACKS_FCNTL
#define PDL_PSOS_LACKS_PHILE
#define PDL_PSOS_LACKS_PREPC
#define PDL_PSOS_CANT_USE_SYS_TYPES
#define PDL_PSOS_HAS_TIME

//#define PDL_PSOS_SNARFS_HEADER_INFO

#if !defined (PDL_PSOS)
#define PDL_PSOS
#endif /* PDL_PSOS */

#if !defined (PDL_PSOS_TBD)
#define PDL_PSOS_TBD
#endif /* PDL_PSOS_TBD */
#define PDL_LACKS_MKFIFO
#define PDL_MALLOC_ALIGN 8
#endif /* PDL_CONFIG_H */

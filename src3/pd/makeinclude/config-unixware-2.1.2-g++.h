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
// config-unixware-2.1.2-g++.h,v 4.18 1999/08/30 02:52:59 levine Exp

// The following configuration file is designed to work
// for Unixware platforms running UnixWare 2.1.2 and gcc version 2.7.2.2

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// See README for what the PDL_HAS... and PDL_LACKS... macros mean

#if ! defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
// this must appear before its #include.
#define PDL_HAS_STRING_CLASS

#if defined (__GNUG__)
# include "config-g++-common.h"
#endif /* __GNUG__ */

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_LACKS_SYSTIME_H
// ualarm is only in BSD compatibility lib, but no header is provided
// #define PDL_HAS_UALARM
#define PDL_HAS_SIZET_SOCKET_LEN
#define PDL_HAS_AUTOMATIC_INIT_FINI
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_GNU_CSTRING_H
#define PDL_HAS_MSG
#define PDL_HAS_SVR4_GETTIMEOFDAY
#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
#define PDL_HAS_NONCONST_GETBY
#define PDL_HAS_POLL
#define PDL_HAS_POSIX_NONBLOCK
#define PDL_HAS_POSIX_TIME
#define PDL_LACKS_TIMESPEC_T
#define PDL_HAS_REENTRANT_FUNCTIONS
#define PDL_HAS_REGEX
#define PDL_HAS_LAZY_V
#define PDL_HAS_SELECT_H
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_SOCKIO_H
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STHREADS
#define PDL_HAS_THR_KEYDELETE
#define PDL_HAS_STRBUF_T
#define PDL_HAS_STREAMS
#define PDL_HAS_STREAM_PIPES
#define PDL_HAS_STRERROR
#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_SYSCALL_H
#define PDL_HAS_SYSINFO
#define PDL_HAS_SYSV_IPC
#define PDL_HAS_SYS_FILIO_H
#define PDL_HAS_TERM_IOCTLS
#define PDL_HAS_THREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_THREAD_T
#define PDL_HAS_TIMOD_H
#define PDL_HAS_TIUSER_H
//#define PDL_HAS_TLI
//#define PDL_HAS_TLI_PROTOTYPES
#define PDL_HAS_UNIXWARE_SVR4_SIGNAL_T
#define PDL_HAS_VOIDPTR_SOCKOPT
#define PDL_HAS_THR_MINSTACK

#define PDL_LACKS_MADVISE
#define PDL_LACKS_STRCASECMP
#define PDL_LACKS_PRI_T
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_PWD_REENTRANT_FUNCTIONS

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#define PDL_PAGE_SIZE 4096
#define PDL_REDEFINES_XTI_FUNCTIONS

#if !defined (UNIXWARE)
# define UNIXWARE
# define UNIXWARE_2_1
#endif /* ! UNIXWARE */

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_HAS_IDTYPE_T

#endif /* PDL_CONFIG_H */

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
// config-osf1-4.0.h,v 4.87 1999/12/18 04:28:21 levine Exp

// NOTE:  if you are using Digital UNIX V4.0f or later, you must
// use config-tru64.h instead of directly using this config file.

// The following configuration file is designed to work for the
// Digital UNIX V4.0a through V4.0d with either the GNU g++, DEC
// cxx 5.4 and later, Rational RCC (2.4.1) compilers, or KAI 3.3
// compilers.  It is configured to use the IEEE Std 1003.1c-1995,
// POSIX System Application Program Interface, or DCE threads (with
// cxx only); it automatically selects the proper thread interface
// depending on whether the cxx -pthread or -threads option was
// specified.  By 4.0a, the version is meant that is called "V4.0 464"
// by uname -a.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if !defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// Compile using multi-thread libraries.
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif /* ! PDL_MT_SAFE */

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* ! PDL_NTRACE */

// Include unistd.h to define _POSIX_C_SOURCE.
#include <unistd.h>

// Configuration-specific #defines:
// 1) g++ or cxx
// 2) pthreads or DCE threads
#if defined (__GNUG__)
  // g++ with pthreads

  // config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
  // this must appear before its #include.
# define PDL_HAS_STRING_CLASS

# include "config-g++-common.h"

# define PDL_HAS_GNU_CSTRING_H
# define PDL_HAS_REENTRANT_FUNCTIONS
#elif defined (__DECCXX)

# define PDL_CONFIG_INCLUDE_CXX_COMMON
# include "config-cxx-common.h"

#elif defined (__rational__)
# define PDL_HAS_REENTRANT_FUNCTIONS
# define PDL_HAS_STRING_CLASS
# define PDL_LACKS_LINEBUFFERED_STREAMBUF
# define PDL_LACKS_SIGNED_CHAR

    // Exceptions are enabled by platform_osf1_4.0_rcc.GNU.
# define PDL_HAS_ANSI_CASTS
# define PDL_HAS_STDCPP_STL_INCLUDES
# define PDL_HAS_TEMPLATE_SPECIALIZATION
# define PDL_HAS_TYPENAME_KEYWORD
# define PDL_HAS_USING_KEYWORD
#elif defined (__KCC)
# define PDL_HAS_STRING_CLASS
# include "config-kcc-common.h"
#else
# error unsupported compiler on Digital Unix
#endif /* ! __GNUG__ && ! __DECCXX && ! __rational__ && !_KCC */

#if defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 199506L)
  // cxx with POSIX 1003.1c-1995 threads (pthreads) . . .
# define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R
# define PDL_HAS_BROKEN_IF_HEADER
# define PDL_HAS_BROKEN_R_ROUTINES
# define PDL_HAS_PTHREADS
# define PDL_HAS_PTHREADS_STD
# define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS
# define PDL_LACKS_T_ERRNO
# define PDL_POLL_IS_BROKEN
# if !defined (DIGITAL_UNIX)
#   define DIGITAL_UNIX 0x400
# endif /* ! DIGITAL_UNIX */
  // DJT removed this due to some minor issues related to the
  // definitions of timestruc_t and tid_t in procfs.h not sure what
  // functionality is lost?  Platform supports <sys/procfs.h>
  //#define PDL_HAS_PROC_FS
#else /* _POSIX_C_SOURCE < 199506L */
  // cxx with DCE threads . . .
  // This PDL configuration is only supported with cxx; it has not been
  // test with g++.
# define PDL_HAS_BROKEN_MSG_H
# define PDL_HAS_BROKEN_POSIX_TIME
# define PDL_HAS_PTHREADS
# define PDL_HAS_PTHREADS_DRAFT4
# define PDL_HAS_GETPAGESIZE
# define PDL_HAS_PROC_FS
# define PDL_HAS_SETKIND_NP
# define PDL_HAS_THREAD_SELF
# define PDL_LACKS_CONST_TIMESPEC_PTR
# define PDL_LACKS_THREAD_PROCESS_SCOPING
# define PDL_LACKS_PTHREAD_THR_SIGSETMASK
# define PDL_LACKS_PTHREAD_THR_SIGSETMASK
# define PDL_LACKS_READDIR_R
# define PDL_LACKS_SETSCHED
# define PDL_LACKS_SIGNED_CHAR
# define PDL_LACKS_SYSV_MSQ_PROTOS
#endif /* _POSIX_C_SOURCE < 199506L */

// Maximum compensation (10 ms) for early return from timed ::select ().
#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

#define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)
// NOTE: PDL_HAS_64BIT_LONGS is deprecated.  Instead, use PDL_SIZEOF_LONG == 8.
#define PDL_HAS_64BIT_LONGS
#define PDL_HAS_AUTOMATIC_INIT_FINI
#define PDL_HAS_BROKEN_SETRLIMIT
#define PDL_HAS_BROKEN_T_ERROR
#define PDL_HAS_BROKEN_WRITEV
#define PDL_HAS_CLOCK_GETTIME
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_DIRENT
#define PDL_HAS_GETRUSAGE
#define PDL_HAS_GPERF
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_LLSEEK
#define PDL_HAS_LONG_MAP_FAILED
#define PDL_HAS_MSG
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_OSF1_GETTIMEOFDAY
#define PDL_HAS_OSF_TIMOD_H
#define PDL_HAS_POLL
#define PDL_HAS_POSIX_NONBLOCK
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_PRIOCNTL
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STRBUF_T
#define PDL_HAS_STREAMS
#define PDL_HAS_STRERROR
#define PDL_HAS_STRPTIME
#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_SVR4_SIGNAL_T
#define PDL_HAS_SYSCALL_H
#define PDL_HAS_SYSV_IPC
#define PDL_HAS_THREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_TIUSER_H
//#define PDL_HAS_TLI
//#define PDL_HAS_TLI_PROTOTYPES
#define PDL_HAS_UALARM
#define PDL_HAS_UCONTEXT_T
#define PDL_LACKS_PRI_T
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_THREAD_STACK_ADDR
#define PDL_PAGE_SIZE 8192
#define PDL_HAS_SIGTIMEDWAIT
#define PDL_HAS_SIGSUSPEND

// DJT 6/10/96 All these broken macro's can now be removed with the
// approporiate ordering of the include files. The Platinum release
// now temporarily supports both forms.  Platform's implementation of
// sendmsg() has a non-const msgheader parameter.
#define PDL_HAS_BROKEN_SENDMSG

// As 1MB thread-stack size seems to become standard (at least Solaris and
// NT have it), we should raise the minimum stack size to this level for
// avoiding unpleasant surprises when porting PDL software to Digital UNIX.
// Do not define this smaller than 64KB, because PDL_Log_Msg::log needs that!
// TK, 05 Feb 97
#define PDL_NEEDS_HUGE_THREAD_STACKSIZE (1024 * 1024)
#define PDL_HAS_IDTYPE_T
#endif /* PDL_CONFIG_H */

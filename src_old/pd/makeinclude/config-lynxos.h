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
 
// config-lynxos.h,v 4.48 2000/01/08 02:39:46 schmidt Exp

// The following configuration file is designed to work for LynxOS,
// version 2.5.0 and later, using the GNU g++ compiler.

// Note on why PDL_HAS_POSIX_SEM is not #defined:
// PDL_HAS_POSIX_SEM would cause native LynxOS mutexes and condition
// variables to be used.  But, they don't appear to be intended to be
// used between processes.  Without PDL_HAS_POSIX_SEM, PDL uses
// semaphores for all synchronization.  Those can be used between
// processes

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if ! defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if defined (__GNUG__)
# if __GNUC_MINOR__ == 7

#   include "config-g++-common.h"

    // The g++ that's distributed with LynxOS 3.0.0 needs this.
    // It won't hurt with 2.5.0.
#   undef PDL_HAS_TEMPLATE_SPECIALIZATION
# elif __LYNXOS_SDK_VERSION <= 199603L
    // config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
    // this must appear before its #include.

    // If PDL_HAS_STRING_CLASS is used with LynxOS 3.0.0, some
    // executables, such as IOStream_Test, require linking with
    // libg++.
#   define PDL_HAS_STRING_CLASS

#   include "config-g++-common.h"
# endif /* __GNUC_MINOR__ == 7 */
#endif /* __GNUG__ */

#if defined (__x86__)
  // PowerPC libraries don't seem to have alloca (), so only use with x86.
# define PDL_HAS_ALLOCA
# define PDL_HAS_ALLOCA_H
# define PDL_HAS_PENTIUM
#elif defined (__powerpc__)
  // It looks like the default stack size is 15000.
  // PDL's Recursive_Mutex_Test needs more.
# define PDL_NEEDS_HUGE_THREAD_STACKSIZE 32000
  // This doesn't work on LynxOS 3.0.0, because it resets the TimeBaseRegister.
  // # define PDL_HAS_POWERPC_TIMER
#endif /* __x86__ || __powerpc__ */

#define PDL_DEFAULT_BASE_ADDR ((char *) 0)
#define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#define PDL_HAS_AUTOMATIC_INIT_FINI
#define PDL_HAS_BROKEN_READV
#define PDL_HAS_BROKEN_SETRLIMIT
#define PDL_HAS_BROKEN_WRITEV
#define PDL_HAS_CLOCK_GETTIME
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_DIRENT
#define PDL_HAS_GETRUSAGE
#define PDL_HAS_GNU_CSTRING_H
#define PDL_HAS_GPERF
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_LYNXOS_SIGNALS
#define PDL_HAS_MSG
#define PDL_HAS_NONCONST_GETBY
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_POLL
#define PDL_HAS_POSIX_NONBLOCK
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS
#define PDL_HAS_SEMUN
#define PDL_HAS_SHM_OPEN
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_SOCKIO_H
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STDARG_THR_DEST
#define PDL_HAS_STRBUF_T
#define PDL_HAS_STREAMS
#define PDL_HAS_STRERROR
#define PDL_HAS_SYSV_IPC
#define PDL_HAS_SYS_SIGLIST
#define PDL_HAS_TIMEZONE_GETTIMEOFDAY
#define PDL_LACKS_CONST_TIMESPEC_PTR
#define PDL_LACKS_GETHOSTENT
#define PDL_LACKS_GETOPT_PROTO
#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MKTEMP
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SIGINFO_H
#define PDL_LACKS_SI_ADDR
#define PDL_LACKS_SOME_POSIX_PROTOTYPES
#define PDL_LACKS_STRCASECMP
#define PDL_LACKS_TIMESPEC_T
#define PDL_LACKS_UCONTEXT_H
#define PDL_MALLOC_ALIGN 8
#define PDL_HAS_TYPENAME_KEYWORD
// Don't use MAP_FIXED, at least for now.
#define PDL_MAP_FIXED 0
// LynxOS, through 3.0.0, does not support MAP_PRIVATE, so map it to
// MAP_SHARED.
#define PDL_MAP_PRIVATE PDL_MAP_SHARED
#define PDL_PAGE_SIZE 4096
#define PDL_POLL_IS_BROKEN

// Compile using multi-thread libraries.
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
# define _REENTRANT
#endif

#if PDL_MT_SAFE == 1
  // Platform supports threads.
# define PDL_HAS_PTHREADS
# define PDL_HAS_PTHREADS_DRAFT4
# define PDL_HAS_THREADS
# define PDL_HAS_THREAD_SPECIFIC_STORAGE
  // Without TSS emulation, you'll only have 3 native TSS keys, on
  // LynxOS 3.0.0/ppc.
# define PDL_HAS_TSS_EMULATION
# define PDL_LACKS_NULL_PTHREAD_STATUS
# define PDL_LACKS_SETDETACH
# define PDL_LACKS_THREAD_PROCESS_SCOPING
# define PDL_LACKS_THREAD_STACK_ADDR
  // This gets around Lynx broken macro calls resulting in "::0"
# define _POSIX_THREADS_CALLS
#endif /* PDL_MT_SAFE */

#define PDL_HAS_AIO_CALLS
#define PDL_POSIX_AIOCB_PROACTOR
// AIOCB Proactor works on Lynx. But it is not
// multi-threaded.
// Lynx OS 3.0.0 lacks POSIX call <pthread_sigmask>. So,we cannot use
// SIG Proactor also, with multiple threads. So, let us use the AIOCB
// Proactor. Once <pthreadd_sigmask> is available on Lynx, we can turn
// on SIG Proactor for this platform.
// #define PDL_POSIX_SIG_PROACTOR

// Maximum compensation (10 ms) for early return from timed ::select ().
#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

// By default, don't include RCS Id strings in object code.
#if !defined (PDL_USE_RCSID)
# define PDL_USE_RCSID 0
#endif /* ! PDL_USE_RCSID */

// System include files are not in sys/, this gets rid of warning.
#define __NO_INCLUDE_WARN__

extern "C"
{
  int getopt (int, char *const *, const char *);
  int putenv (const char *);
}

#endif /* PDL_CONFIG_H */

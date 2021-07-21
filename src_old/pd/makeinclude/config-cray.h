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
// config-cray.h,v 4.6 2000/01/08 02:39:45 schmidt Exp

#ifndef PDL_CONFIG_CRAY_H
#define PDL_CONFIG_CRAY_H

/*
    The following predefined macros are used within PDL ifdefs.
    These are defined when using the Cray compilers.  _CRAYMPP
    is defined, for example, if you are running on a Cray T3E
    massively parallel machine.  Moreover, in the case of the T3E,
    _CRAYT3E will be defined.  This is used to determine the
    PDL_SIZEOF defines for primitive types.

    _UNICOS is defined as either the major version of UNICOS being run,
    e.g. 9 or 10 on the vector machines (e.g. C90, T90, J90, YMP, ...)
    or the major+minor+level UNICOS/mk version, e.g. 2.0.3 => 203,
    being run on an MPP machine.

    Summary:

    _CRAYMPP  (defined only if running on MPP machine, e.g. T3E, UNICOS/mk)
    _CRAYT3E  (defined specifically if compiling on a Cray T3E)
    _UNICOS   (defined if running UNICOS or UNICOS/mk)

    Tested on UNICOS 10.0.0.5, UNICOS/mk 2.0.4.57
    Compiles on UNICOS 9.0.2.8, but some tests deadlock

    Contributed by Doug Anderson <dla@home.com>
*/

#if defined (_UNICOS) && !defined (MAXPATHLEN)
#define MAXPATHLEN 1023
#endif /* _UNICOS */

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_DEFAULT_CLOSE_ALL_HANDLES 0

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

#define PDL_HAS_CPLUSPLUS_HEADERS

// using cray's autoinstantiation gives C++ prelinker: error: instantiation loop
#define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION

#define PDL_HAS_TEMPLATE_SPECIALIZATION

#define PDL_HAS_ANSI_CASTS

#define PDL_HAS_USING_KEYWORD

#define PDL_HAS_SSIZE_T

#define PDL_HAS_SYSV_IPC

#define PDL_MT_SAFE 1

#define PDL_HAS_THREADS

#define PDL_HAS_PTHREADS

// UNICOS 10 and UNICOS/mk implement a idall subset of POSIX Threads,
// but the prototypes follow the POSIX.1c-1995 definitions.  Earlier
// UNICOS versions sport Draft 7 threads.

#if _UNICOS > 9
# define PDL_HAS_PTHREADS_STD
#else
# define PDL_HAS_PTHREADS_DRAFT7
# define PDL_LACKS_THREAD_STACK_SIZE
# define PDL_LACKS_THREAD_STACK_ADDR
  // UNICOS 9 doesn't have this, nor sched.h
# define SCHED_OTHER 0
# define SCHED_FIFO 1
# define SCHED_RR 2
#endif

#define PDL_HAS_THREAD_SPECIFIC_STORAGE

#define PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP

#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R

#define PDL_HAS_POSIX_TIME

#define PDL_HAS_TIMEZONE_GETTIMEOFDAY

#define PDL_HAS_POSIX_NONBLOCK

#define PDL_HAS_TERM_IOCTLS

#define PDL_HAS_DIRENT

#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#define PDL_HAS_IP_MULTICAST

#define PDL_HAS_SIN_LEN

#define PDL_HAS_NONCONST_SELECT_TIMEVAL

#define PDL_HAS_NONCONST_READLINK

#define PDL_HAS_CHARPTR_SOCKOPT

#define PDL_HAS_NONCONST_GETBY

// has man pages, but links with missing symbols and I can't find lib yet
/* #define PDL_HAS_REGEX */

#define PDL_HAS_SIG_MACROS

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

#if _UNICOS > 9
# define PDL_HAS_SIGWAIT
#endif

#define PDL_HAS_SIG_ATOMIC_T

#define PDL_HAS_SIGISMEMBER_BUG

#define PDL_HAS_MSG

#define PDL_HAS_STRERROR

#define PDL_HAS_GPERF

// Special modifications that apply to UNICOS/mk
#if defined(_CRAYMPP)

# define PDL_HAS_SIGINFO_T
# define PDL_HAS_UCONTEXT_T

#endif

// The Cray T90 supposedly supports SYSV SHMEM, but I was unable to get it
// working.  Of course, all other Cray PVP and MPP systems do NOT support it,
// so it's probably good to just define like this for consistency
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_MMAP
#define PDL_LACKS_CONST_TIMESPEC_PTR
#define PDL_LACKS_SYSCALL
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_MADVISE
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_LINEBUFFERED_STREAMBUF
#define PDL_LACKS_PTHREAD_CLEANUP
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_THREAD_PROCESS_SCOPING

#if !defined(_CRAYMPP)

#define PDL_LACKS_PTHREAD_CANCEL
#define PDL_LACKS_PTHREAD_KILL

#endif

#define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_PRI_T
#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
#define PDL_LACKS_MPROTECT
#define PDL_LACKS_MSYNC
#define PDL_LACKS_READV
#define PDL_LACKS_RLIMIT

// we probably want to fake not having this, since Cray memory mgmt is different
#define PDL_LACKS_SBRK

#define PDL_LACKS_SETSCHED

#define PDL_LACKS_SIGINFO_H

#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_WRITEV

// Cray vector machines are "word" oriented, and modern ones are hard 64-bit.
// "char" is somewhat of a special case.  Most problems arise when code thinks
// it can address 32-bit quantities and the like.  MPP crays are typically
// byte oriented, e.g. T3E uses Alpha processors, so we don't need as much
// special treatment.

#ifndef _CRAYMPP

# define PDL_SIZEOF_CHAR        1
# define PDL_SIZEOF_SHORT       8
# define PDL_SIZEOF_INT         8
# define PDL_SIZEOF_LONG        8
# define PDL_SIZEOF_LONG_LONG   8
# define PDL_SIZEOF_FLOAT       8
# define PDL_SIZEOF_DOUBLE      8
# define PDL_SIZEOF_LONG_DOUBLE 16
# define PDL_SIZEOF_VOID_P      8

#elif defined(_CRAYT3E)

# define PDL_SIZEOF_CHAR        1
# define PDL_SIZEOF_SHORT       4
# define PDL_SIZEOF_INT         8
# define PDL_SIZEOF_LONG        8
# define PDL_SIZEOF_LONG_LONG   8
# define PDL_SIZEOF_FLOAT       4
# define PDL_SIZEOF_DOUBLE      8
# define PDL_SIZEOF_LONG_DOUBLE 8
# define PDL_SIZEOF_VOID_P      8

#endif

// Ones to check out at some point

/* #define PDL_HAS_SYS_SIGLIST */

// C++ Compiler stuff to verify
/* #define PDL_NEW_THROWS_EXCEPTIONS */
/* #define PDL_HAS_TEMPLATE_TYPEDEFS */

// thread issues to check out
/* #define PDL_LACKS_TIMEDWAIT_PROTOTYPES */

// Cray does seem to support it, in -lnsl and has tiuser.h header
/* #define PDL_HAS_TLI */
/* #define PDL_HAS_TIUSER_H */
/* #define PDL_HAS_TLI_PROTOTYPES */
/* #define PDL_LACKS_T_ERRNO */

/* #define PDL_LACKS_NAMED_POSIX_SEM */

/* #define PDL_HAS_SYS_ERRLIST */

#endif /* PDL_CONFIG_CRAY_H */

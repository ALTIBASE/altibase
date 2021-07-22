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
// config-mvs.h,v 4.45 1999/09/03 20:01:00 brunsch Exp

// Config file for MVS with OpenEdition

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// The following #defines are hacks to get around things
// that seem to be missing or different in MVS land
#define MAXPATHLEN 1024         /* sys/param.h not on MVS */
#define NSIG 44                 /* missing from Signal.h */
#define MAXHOSTNAMELEN 256      /* missing form netdb.h */
#define howmany __howmany       /* MVS uses different names than most others */
#define fd_mask __fd_mask
#define MAXNAMLEN  __DIR_NAME_MAX
#define ERRMAX __sys_nerr
#if defined (log)               /* log is a macro in math.h */
# undef log                     /* conflicts with log function in PDL */
#endif /* log */

#define PDL_MVS

// Preprocesor requires an extra argument
#define PDL_CC_PREPROCESSOR_ARGS "-+ -E"

// See the README file in this directory
// for a description of the following PDL_ macros

#if __COMPILER_VER__ >= 0x21020000   /* OS/390 r2 or higher */
# define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
# define PDL_HAS_UCONTEXT_T
#else
# define PDL_LACKS_UCONTEXT_H
#endif /* __COMPILER_VER__ >= 0x21020000 */

#define PDL_HAS_BROKEN_CTIME
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_DIRENT
#define PDL_HAS_EXCEPTIONS
#define PDL_HAS_GETPAGESIZE
#define PDL_HAS_GETRUSAGE
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_LIMITED_RUSAGE_T
#define PDL_HAS_MSG
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_NONSCALAR_THREAD_KEY_T
#define PDL_HAS_POLL
#define PDL_HAS_POSIX_NONBLOCK
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_DRAFT6
#define PDL_HAS_PTHREAD_CONDATTR_SETKIND_NP
#define PDL_HAS_PTHREAD_MUTEXATTR_SETKIND_NP
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_SIG_C_FUNC
#define PDL_HAS_SIN_LEN
#define PDL_HAS_SIZET_SOCKET_LEN
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STRERROR
#define PDL_HAS_STRINGS
#define PDL_HAS_SYSV_IPC
#define PDL_HAS_TEMPLATE_SPECIALIZATION
#define PDL_HAS_THREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_THR_C_DEST
#define PDL_HAS_THR_C_FUNC
#define PDL_HAS_TIMEZONE_GETTIMEOFDAY
#define PDL_HAS_UALARM
#define PDL_HAS_UTIME
#define PDL_HAS_VOIDPTR_MMAP
#define PDL_HAS_VOIDPTR_SOCKOPT
#define PDL_HAS_XPG4_MULTIBYTE_CHAR

#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_IOSTREAM_FX
#define PDL_LACKS_LINEBUFFERED_STREAMBUF
#define PDL_LACKS_LONGLONG_T
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MALLOC_H
#define PDL_LACKS_MSGBUF_T
#define PDL_LACKS_PARAM_H
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
#define PDL_LACKS_READDIR_R
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SETSCHED
#define PDL_LACKS_SIGINFO_H
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_SYS_NERR
#define PDL_LACKS_TCP_H
#define PDL_LACKS_THREAD_PROCESS_SCOPING
#define PDL_LACKS_THREAD_STACK_ADDR
#define PDL_LACKS_TIMESPEC_T

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#define PDL_NEEDS_DEV_IO_CONVERSION

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_SIZEOF_FLOAT 4
#define PDL_SIZEOF_DOUBLE 8
#define PDL_SIZEOF_LONG_DOUBLE 16

#define PDL_TEMPLATES_REQUIRE_SOURCE

#endif /* PDL_CONFIG_H */

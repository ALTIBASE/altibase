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
// config-linux-common.h,v 4.68 2000/01/19 05:06:32 othman Exp

// This configuration file is designed to be included by another,
// specific configuration file.  It provides config information common
// to all Linux platforms.  It automatically determines the CPU
// architecture, compiler (g++ or egcs), and libc (libc5 or glibc),
// and configures based on those.

#ifndef PDL_LINUX_COMMON_H
#define PDL_LINUX_COMMON_H

//#define PDL_HAS_BYTESEX_H

#if ! defined (__PDL_INLINE__)
#define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// Needed to make some prototypes visible.
// #if ! defined (_GNU_SOURCE)
// #define _GNU_SOURCE
// #endif /* ! _GNU_SOURCE */

// Needed to differentiate between libc 5 and libc 6 (aka glibc).
// It's there on all libc 5 systems I checked.
//#include <features.h>


// First the machine specific part

#if defined (__alpha)
  // This is necessary on Alphas with glib 2.0.7-13.
# define PDL_POLL_IS_BROKEN
#elif defined (__powerpc__)
# if !defined (PDL_DEFAULT_BASE_ADDR)
#   define PDL_DEFAULT_BASE_ADDR ((char *) 0x40000000)
# endif /* ! PDL_DEFAULT_BASE_ADDR */
#endif /* ! __alpha  &&  ! __powerpc__ */

// Then glibc/libc5 specific parts

#if defined(__GLIBC__)
# define PDL_HAS_BROKEN_SETRLIMIT
# define PDL_HAS_RUSAGE_WHO_ENUM enum __rusage_who
# define PDL_HAS_RLIMIT_RESOURCE_ENUM enum __rlimit_resource
# define PDL_HAS_SOCKLEN_T
# define PDL_HAS_4_4BSD_SENDMSG_RECVMSG

  // To avoid the strangeness with Linux's ::select (), which modifies
  // its timeout argument, use ::poll () instead.
# define PDL_HAS_POLL

// Don't define _XOPEN_SOURCE and _XOPEN_SOURCE_EXTENDED in PDL to make
// getpgid() prototype visible.  PDL shouldn't depend on feature test
// macros to make prototypes visible.
# define PDL_LACKS_GETPGID_PROTOTYPE

// NOTE:  the following defines are necessary with glibc 2.0 (0.961212-5)
//        on Alpha.  I assume that they're necessary on Intel as well,
//        but that may depend on the version of glibc that is used.
//# define PDL_HAS_DLFCN_H_BROKEN_EXTERN_C
# define PDL_HAS_VOIDPTR_SOCKOPT
# define PDL_LACKS_SYSTIME_H

// Don't define _POSIX_SOURCE in PDL to make strtok() prototype
// visible.  PDL shouldn't depend on feature test macros to make
// prototypes visible.
# define PDL_LACKS_STRTOK_R_PROTOTYPE
// NOTE:  end of glibc 2.0 (0.961212-5)-specific configuration.

# if __GLIBC__ > 1 && __GLIBC_MINOR__ >= 1
    // These were suggested by Robert Hanzlik <robi@codalan.cz> to get
    // PDL to compile on Linux using glibc 2.1 and libg++/gcc 2.8.
#   undef PDL_HAS_BYTESEX_H
#   define PDL_HAS_SIGINFO_T
#   define PDL_LACKS_SIGINFO_H
#   define PDL_HAS_UCONTEXT_T

    // Pre-glibc (RedHat 5.2) doesn't have sigtimedwait.
#   define PDL_HAS_SIGTIMEDWAIT
# endif /* __GLIBC__ 2.1+ */
#else  /* ! __GLIBC__ */
    // Fixes a problem with some non-glibc versions of Linux...
#   define PDL_LACKS_MADVISE
#   define PDL_LACKS_MSG_ACCRIGHTS
#endif /* ! __GLIBC__ */

// Don't define _LARGEFILE64_SOURCE in PDL to make llseek() or
// lseek64() prototype visible.  PDL shouldn't depend on feature test
// macros to make prototypes visible.
#if __GLIBC__ > 1
#  if __GLIBC_MINOR__ == 0
#    define PDL_HAS_LLSEEK
#    define PDL_LACKS_LLSEEK_PROTOTYPE
#  else  /* __GLIBC_MINOR__ > 0 */
#    define PDL_HAS_LSEEK64
#    define PDL_LACKS_LSEEK64_PROTOTYPE
#  endif
#endif /* __GLIBC__ > 1 */

#if __GLIBC__ > 1 && __GLIBC_MINOR__ >= 1
# define PDL_HAS_P_READ_WRITE
# define PDL_LACKS_PREAD_PROTOTYPE
#endif /* __GLIBC__ > 1 && __GLIBC_MINOR__ >= 0 */


// Then the compiler specific parts

//#if defined (__GNUG__)
  // config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
  // this must appear before its #include.
//# define PDL_HAS_STRING_CLASS
# include "config-g++-common.h"
//#elif defined (__KCC)
//# define PDL_HAS_STRING_CLASS
//# include "config-kcc-common.h"
//#elif defined (__DECCXX)
//# define PDL_CONFIG_INCLUDE_CXX_COMMON
//# include "config-cxx-common.h"
//#else  /* ! __GNUG__ && ! __KCC && !__DECCXX */
//# error unsupported compiler in config-linux-common.h
//#endif /* ! __GNUG__ && ! __KCC */

// Completely common part :-)

// Platform/compiler has the sigwait(2) prototype
# define PDL_HAS_SIGWAIT

//# define PDL_HAS_SIGSUSPEND

#if !defined (PDL_DEFAULT_BASE_ADDR)
# define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)
#endif /* ! PDL_DEFAULT_BASE_ADDR */

// Compiler/platform supports alloca().
//#define PDL_HAS_ALLOCA

// Compiler/platform has <alloca.h>
//#define PDL_HAS_ALLOCA_H

// Compiler/platform has the getrusage() system call.
//#define PDL_HAS_GETRUSAGE
//#define PDL_HAS_GETRUSAGE_PROTO

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// ONLY define this if you have config'd multicast into a 2.0.34 or
// prior kernel.  It is enabled by default in 2.0.35 kernels.
#if !defined (PDL_HAS_IP_MULTICAST)
# define PDL_HAS_IP_MULTICAST
#endif /* ! PDL_HAS_IP_MULTICAST */

#define PDL_HAS_BIG_FD_SET

// Linux defines struct msghdr in /usr/include/socket.h
#define PDL_HAS_MSG

// Linux "improved" the interface to select() so that it modifies
// the struct timeval to reflect the amount of time not slept
// (see NOTES in Linux's select(2) man page).
#define PDL_HAS_NONCONST_SELECT_TIMEVAL

//#define PDL_HAS_TERM_IOCTLS

#define PDL_DEFAULT_MAX_SOCKET_BUFSIZ 65535

#if !defined (PDL_DEFAULT_SELECT_REACTOR_SIZE)
#define PDL_DEFAULT_SELECT_REACTOR_SIZE 256
#endif /* PDL_DEFAULT_SELECT_REACTOR_SIZE */

#define PDL_HAS_GETPAGESIZE 1

// Platform lacks POSIX prototypes for certain System V functions
// like shared memory and message queues.
#if defined(__USE_GNU)  /* BUBUG gcc 2.96 by jdlee */
#define PDL_LACKS_SOME_POSIX_PROTOTYPES
#endif /* __USE_GNU */

// Platform defines struct timespec but not timespec_t
#define PDL_LACKS_TIMESPEC_T

#define PDL_LACKS_STRRECVFD

//#define PDL_LACKS_MSYNC
#define PDL_HAS_PROCFS

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Compiler/platform contains the <sys/syscall.h> file.
//#define PDL_HAS_SYSCALL_H

#define PDL_HAS_SUNOS4_GETTIMEOFDAY

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

#define PDL_HAS_STRPTIME
// Don't define _XOPEN_SOURCE in PDL to make strptime() prototype
// visible.  PDL shouldn't depend on feature test macros to make
// prototypes visible.
#define PDL_LACKS_STRPTIME_PROTOTYPE

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Compiler/platform defines a union semun for SysV shared memory.
#define PDL_HAS_SEMUN

#define PDL_HAS_POSIX_TIME

#define PDL_HAS_GPERF

#define PDL_HAS_DIRENT

# define PDL_UINT64_FORMAT_SPECIFIER "%Lu"

#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

void _printf( char * format, ... );

#define SMALL_FOOTPRINT

#endif /* PDL_LINUX_COMMON_H */

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
// config-cygwin32-common.h,v 4.9 2000/01/08 02:39:45 schmidt Exp

// This configuration file is designed to be included by another,
// specific configuration file.  It provides config information common
// to all CygWin platforms.  It automatically determines the CPU
// architecture, compiler (g++ or egcs), and libc (libc5 or glibc),
// and configures based on those.

#ifndef PDL_CYGWIN32_COMMON_H
#define PDL_CYGWIN32_COMMON_H

// BUGBUG_CYGWIN : version 2.340.25
// thread_yield and sigwait has problem
// but OK
#if !defined (PDL_MT_SAFE)
#define PDL_MT_SAFE 1
#endif

#define CYGWIN32

#define PDL_LACKS_UNIX_DOMAIN_SOCKETS
#define PDL_LACKS_SYSV_MSG_H
#define PDL_HAS_SIG_MACROS
#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_TELLDIR
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_SEMBUF_T
#define PDL_LACKS_NAMED_POSIX_SEM
#define PDL_LACKS_SENDMSG
#define PDL_LACKS_RECVMSG
#define PDL_LACKS_READDIR_R
#define PDL_LACKS_RLIMIT
#define PDL_LACKS_SOCKETPAIR
#define PDL_LACKS_SEEKDIR
#define PDL_LACKS_TEMPNAM
#define PDL_LACKS_MKTEMP

#if ! defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// Needed to differentiate between libc 5 and libc 6 (aka glibc).
// It's there on all libc 5 systems I checked.
#include <features.h>


// First the machine specific part
#define PDL_HAS_CYGWIN32_SOCKET_H
#define PDL_LACKS_TCP_H

// Then glibc/libc5 specific parts

#if defined(__GLIBC__)
# define PDL_HAS_BROKEN_SETRLIMIT
# define PDL_HAS_RUSAGE_WHO_ENUM enum __rusage_who
# define PDL_HAS_RLIMIT_RESOURCE_ENUM enum __rlimit_resource
# define PDL_HAS_SOCKLEN_T

  // To avoid the strangeness with Linux's ::select (), which modifies
  // its timeout argument, use ::poll () instead.
# define PDL_HAS_POLL

  // NOTE:  the following defines are necessary with glibc 2.0 (0.961212-5)
  //        on Alpha.  I assume that they're necessary on Intel as well,
  //        but that may depend on the version of glibc that is used.
# define PDL_HAS_DLFCN_H_BROKEN_EXTERN_C
# define PDL_HAS_VOIDPTR_SOCKOPT
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
# define PDL_LACKS_GETPGID
  // The strtok_r declaration is protected in string.h.
  extern "C" char *strtok_r __P ((char *__s, __const char *__delim,
                                  char **__save_ptr));
  // NOTE:  end of glibc 2.0 (0.961212-5)-specific configuration.

# if __GLIBC__ > 1 && __GLIBC_MINOR__ >= 1
#   undef PDL_HAS_BYTESEX_H
#   define PDL_HAS_SIGINFO_T
#   define PDL_LACKS_SIGINFO_H
#   define PDL_HAS_UCONTEXT_T
# endif /* __GLIBC__ 2.1+ */
  // Changes above were suggested by Robert Hanzlik <robi@codalan.cz>
  // to get PDL to compile on Linux using glibc 2.1 and libg++/gcc 2.8

#endif /* __GLIBC__ */


// Then the compiler specific parts

// config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
// this must appear before its #include.
#define PDL_HAS_STRING_CLASS

#if defined (__GNUG__)
# include "config-g++-common.h"
#elif defined (__KCC)
# include "config-kcc-common.h"
#else  /* ! __GNUG__ && ! __KCC */
# error unsupported compiler in config-linux-common.h
#endif /* ! __GNUG__ && ! __KCC */


// Completely common part :-)

// Platform/compiler has the sigwait(2) prototype
#define PDL_HAS_SIGWAIT

# define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)

// Compiler/platform supports alloca().
#define PDL_HAS_ALLOCA

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE
#define PDL_HAS_GETRUSAGE_PROTO

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// ONLY define this if you have config'd multicast into a 2.x kernel.
// If you do anything else, we've never tested it!
#if !defined(PDL_HAS_IP_MULTICAST)
# define PDL_HAS_IP_MULTICAST
#endif /* #if ! defined(PDL_HAS_IP_MULTICAST) */

#define PDL_HAS_BIG_FD_SET

// Linux defines struct msghdr in /usr/include/socket.h
#define PDL_HAS_MSG

// Linux "improved" the interface to select() so that it modifies
// the struct timeval to reflect the amount of time not slept
// (see NOTES in Linux's select(2) man page).
#define PDL_HAS_NONCONST_SELECT_TIMEVAL

#define PDL_HAS_TERM_IOCTLS

#define PDL_DEFAULT_MAX_SOCKET_BUFSIZ 65535

#define PDL_DEFAULT_SELECT_REACTOR_SIZE 256

#define PDL_HAS_GETPAGESIZE 1

// Platform lacks POSIX prototypes for certain System V functions
// like shared memory and message queues.
#define PDL_LACKS_SOME_POSIX_PROTOTYPES


#define PDL_LACKS_STRRECVFD

#define PDL_LACKS_MSYNC
#define PDL_LACKS_MADVISE

#define PDL_HAS_SUNOS4_GETTIMEOFDAY

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

#define PDL_HAS_GPERF

#define PDL_HAS_DIRENT

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_LACKS_MKFIFO


#if PDL_MT_SAFE
// Yes, we do have threads.
#  define PDL_HAS_THREADS
// And they're even POSIX pthreads (LinuxThreads implementation)
#  define PDL_HAS_PTHREADS

// Compiler/platform has thread-specific storage
#   define PDL_HAS_THREAD_SPECIFIC_STORAGE

#  if !defined (PDL_HAS_PTHREADS_UNIX98_EXT)
#    define PDL_LACKS_RWLOCK_T
#  endif  /* !PDL_HAS_PTHREADS_UNIX98_EXT */

// ... and the final standard even!
#  define PDL_HAS_PTHREADS_STD
// Cygwin (see pthread.h): Not supported or implemented.
#  define PDL_LACKS_THREAD_SETSTACK
#  define PDL_LACKS_THREAD_STACK_ADDR
#  define PDL_LACKS_SETSCHED
#  define PDL_LACKS_SETDETACH
#  define PDL_LACKS_PTHREAD_CANCEL
#endif  /* PDL_MT_SAFE */


#endif /* PDL_LINUX_COMMON_H */

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
// config-hpux-10.x.h,v 4.51 1999/12/13 02:14:29 nanbor Exp

// The following configuration file is designed to work for HP
// platforms running HP/UX 10.x.  It includes all of the PDL information
// needed for HP-UX 10.x itself.  The compiler-specific information is in
// config-hpux-10.x-<compiler>.h - they include this file.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if defined (__GNUG__)

#include "config-hpux-10.x-g++.h"

#else // native compiler

#include "config-hpux-10.x-hpc++.h"

#endif










// Compiling for HPUX.
#if !defined (HPUX)
#define HPUX
#endif /* HPUX */
#define HPUX_10

#ifndef _HPUX_SOURCE
#define _HPUX_SOURCE
#endif

// Some things are different for 10.10 vs. 10.20 vs. 10.30
// If the version number wasn't set up by the compiler command line,
// set up as if it was 10.20.
#if !defined (HPUX_VERS)
#define HPUX_VERS 1020
#endif

#if (HPUX_VERS < 1020)                  // 10.10
#  define PDL_HAS_BROKEN_MMAP_H
#  define PDL_LACKS_T_ERRNO
#  define PDL_LACKS_TIMESPEC_T
#elif (HPUX_VERS < 1030)                // 10.20

   // Platform supports reentrant functions (all the POSIX *_r functions).
#  define PDL_HAS_REENTRANT_FUNCTIONS
   // But this one is not like other platforms
#  define PDL_CTIME_R_RETURNS_INT
   // And _REENTRANT must be set, even if not using threads.
#  if !defined (_REENTRANT)
#    define _REENTRANT
#  endif /* _REENTRANT */

#else                                   // 10.30
// Don't know yet... probably will be 10.20 but with some different thread
// settings.
#endif /* HPUX_VERS tests */

#include /**/ <sys/stdsyms.h>
#include /**/ <sched.h>         /*  pthread.h doesn't include this */

extern int h_errno;     /* This isn't declared in a header file on HP-UX */


////////////////////////////////////////////////////////////////////////////
//
// General OS information - see README for more details on what they mean
//
///////////////////////////////////////////////////////////////////////////

// HP/UX needs to have these addresses in a special range.
#define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H
// But doesn't have a prototype for syscall()
#define PDL_LACKS_SYSCALL

// Platform supports POSIX 1.b clock_gettime ()
#define PDL_HAS_CLOCK_GETTIME

// Prototypes for both signal() and struct sigaction are consistent.
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Compiler/platform has Dirent iterator functions.
#define PDL_HAS_DIRENT

// Platform supports getpagesize() call
#define PDL_HAS_GETPAGESIZE
// But we define this just to be safe
#define PDL_PAGE_SIZE 4096

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// select's timeval arg is non-const
#define PDL_HAS_NONCONST_SELECT_TIMEVAL

// Compiler/platform supports poll().
#define PDL_HAS_POLL

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Platform supports the POSIX struct timespec type
#define PDL_HAS_POSIX_TIME

// Compiler/platform defines the sig_atomic_t typedef
#define PDL_HAS_SIG_ATOMIC_T

// Platform supports SVR4 extended signals
#define PDL_HAS_SIGINFO_T

// Platform doesn't detect a signal out of range unless it's way out of range.
#define PDL_HAS_SIGISMEMBER_BUG

// Platform supports ucontext_t (which is used in the extended signal API).
#define PDL_HAS_UCONTEXT_T

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// SunOS 4 style prototype for gettimeofday
#define PDL_HAS_SUNOS4_GETTIMEOFDAY

// HP/UX has an undefined syscall for GETRUSAGE...
#define PDL_HAS_SYSCALL_GETRUSAGE
// Note, this only works if the flag is set above!
#define PDL_HAS_GETRUSAGE

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

#define PDL_HAS_UALARM

// Platform has XPG4 wide character support
#define PDL_HAS_XPG4_MULTIBYTE_CHAR

// Platform lacks readers/writer locks.
#define PDL_LACKS_RWLOCK_T

// Shared library path/search components
#define PDL_DLL_SUFFIX      ".sl"
#define PDL_LD_SEARCH_PATH  "SHLIB_PATH"

//////////////////////////////////////////////////////////////////////////
//
// STREAMS information
//
//////////////////////////////////////////////////////////////////////////

// Platform supports STREAMS
#define PDL_HAS_STREAMS
// Compiler/platform supports struct strbuf.
#define PDL_HAS_STRBUF_T
// But the putmsg signature doesn't have it as const...
#define PDL_LACKS_CONST_STRBUF_PTR

// Platform supports STREAM pipes
// This is possible, but not by default - need to rebuild the kernel to
// get them enabled - see pipe(2) and "STREAMS/UX for the HP 9000"
// #define PDL_HAS_STREAM_PIPES

/////////////////////////////////////////////////////////////////////////
//
// TLI information
//
////////////////////////////////////////////////////////////////////////

// Platform supports PDL_TLI, including SVR4 facilities.
//#define PDL_HAS_TLI

// t_error's arg is char *, not const char *
#define PDL_HAS_BROKEN_T_ERROR
// PDL_HAS_SVR4_TLI should work on HP-UX, but doesn't yet.  Riverace
// problem ID P27.
//#define PDL_HAS_SVR4_TLI
// Platform supports PDL_TLI tiuser header.
#define PDL_HAS_TIUSER_H
// But it has _terrno() outside the extern "C" stuff.
#define PDL_HAS_TIUSER_H_BROKEN_EXTERN_C
// Platform provides PDL_TLI function prototypes.
//#define PDL_HAS_TLI_PROTOTYPES
// Platform uses a TCP TLI device other than /dev/tcp.  Uses XTI only.
#define PDL_TLI_TCP_DEVICE  "/dev/inet_cots"

/////////////////////////////////////////////////////////////////////////
//
// Threads information.
// Threads definitions are controlled by the threads setting in the
// makeinclude/platform_hpux_aCC.GNU file - see that for details.
// If you build with threads support, the DCE Core subset must be installed
// from the core OS CD.
//
////////////////////////////////////////////////////////////////////////

#ifdef PDL_HAS_THREADS
#  if !defined (PDL_MT_SAFE)
        #define PDL_MT_SAFE 1
#  endif

#  define PDL_HAS_PTHREADS
#  define PDL_HAS_PTHREADS_DRAFT4
// POSIX real-time semaphore definitions are in the header files, and it
// will compile and link with this in place, but will not run.  HP says
// the functions are not implemented.
//#  define PDL_HAS_POSIX_SEM

#  define PDL_HAS_THREAD_SPECIFIC_STORAGE

// They forgot a const in the prototype of pthread_cond_timedwait
#  define PDL_LACKS_CONST_TIMESPEC_PTR

// Platform lacks pthread_thr_sigsetmask
#  define PDL_LACKS_PTHREAD_THR_SIGSETMASK

// Platform has no implementation of pthread_condattr_setpshared()
#  define PDL_LACKS_CONDATTR_PSHARED

// Platform lacks pthread_attr_setdetachstate()
#  define PDL_LACKS_SETDETACH

// Platform lacks pthread_attr_setscope
#  define PDL_LACKS_THREAD_PROCESS_SCOPING

// Platform lacks pthread_attr_setstackaddr
#  define PDL_LACKS_THREAD_STACK_ADDR
#  define PDL_LACKS_THREAD_SETSTACK

// If this is not turned on, the CMA wrappers will redefine a bunch of
// system calls with wrappers - one being select() and it only defines
// select with int arguments (not fd_set).  So, as long as _CMA_NOWRAPPERS_
// is set, the regular fd_set arg types are used for select().
// Without threads being compiled in, the fd_set/int thing is not an issue.
#  define _CMA_NOWRAPPERS_

#else
// If threading is disabled, then timespec_t does not get defined.
#  ifndef PDL_LACKS_TIMESPEC_T
#    define PDL_LACKS_TIMESPEC_T
#  endif
#endif /* PDL_HAS_THREADS */

// Manually tweaking malloc paddings.
#define PDL_MALLOC_PADDING 16
#define PDL_MALLOC_ALIGN 8
#define PDL_CONTROL_BLOCK_ALIGN_LONGS 0

// Turns off the tracing feature.
// To build with tracing enabled, make sure PDL_NTRACE is not defined
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif

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
// config-sco-5.0.0-mit-pthread.h,v 4.32 1999/05/07 02:53:01 othman Exp

// The following configuration file is designed to work for SCO UNIX
// version 5.0 with MIT pthreads.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if !defined (__PDL_INLINE__)
#define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if defined (__GNUG__)
# include "config-g++-common.h"
  // This config file has not been tested with PDL_HAS_TEMPLATE_SPECIALIZATION.
  // Maybe it will work?
# undef PDL_HAS_TEMPLATE_SPECIALIZATION
#endif /* __GNUG__ */

// Compiling for SCO.
#if !defined (SCO)
#define SCO
#define _SVID3
#endif /* SCO */

#define PDL_DEFAULT_CLOSE_ALL_HANDLES 0
#define PDL_HAS_SIG_MACROS
// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#if defined (SCO) && !defined (MAXPATHLEN)
#define MAXPATHLEN 1023
#endif /* SCO */

#define PDL_LACKS_PWD_FUNCTIONS
#define PDL_HAS_BIG_FD_SET

//#define PDL_LACKS_SYSCALL
//#define PDL_LACKS_STRRECVFD
//#define PDL_NEEDS_FTRUNCATE
#define PDL_LACKS_RLIMIT
#define PDL_LACKS_MADVISE

#define PDL_HAS_REENTRANT_FUNCTIONS

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC
#define PDL_HAS_NONCONST_MSGSND
// #define      PDL_LACKS_POSIX_PROTOTYPES
#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Platform supports Term Ioctls
#define PDL_HAS_TERM_IOCTLS

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
//#define PDL_HAS_SYSCALL_H

// Fixes a problem with HP/UX not wrapping the mmap(2) header files
// with extern "C".
//#define PDL_HAS_BROKEN_MMAP_H

// Prototypes for both signal() and struct sigaction are consistent.
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Header files lack t_errno for PDL_TLI.
//#define PDL_LACKS_T_ERRNO

// Compiler/platform supports poll().
// #define PDL_HAS_POLL

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Compiler/platform defines the sig_atomic_t typedef
#define PDL_HAS_SIG_ATOMIC_T

// Compiler supports the ssize_t typedef.
//#define PDL_HAS_SSIZE_T

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// ???
// #define PDL_HAS_SUNOS4_GETTIMEOFDAY
#define PDL_HAS_TIMEZONE_GETTIMEOFDAY

// HP/UX has an undefined syscall for GETRUSAGE...
//#define PDL_HAS_SYSCALL_GETRUSAGE

// Note, this only works if the flag is set above!
//#define PDL_HAS_GETRUSAGE

// Platform uses int for select() rather than fd_set.
#define PDL_HAS_SELECT_H

// Platform has prototypes for PDL_TLI.
//#define PDL_HAS_TLI
//#define       PDL_HAS_SVR4_TLI
#define PDL_HAS_T_OPMGMT
//#define PDL_HAS_TLI_PROTOTYPES
#define PDL_HAS_TIMOD_H
#define PDL_HAS_TIUSER_H
#define PDL_LACKS_T_ERRNO

// Platform has the XLI version of PDL_TLI.
// #define PDL_HAS_XLI

#define PDL_HAS_GNU_CSTRING_H

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T

#define PDL_LACKS_SYSTIME_H
#define PDL_HAS_INLINED_OSCALLS

#define PDL_HAS_STRBUF_T
#define PDL_HAS_STREAMS
//#define       PDL_HAS_STREAM_PIPES
#define PDL_HAS_IP_MULTICAST

// Threads
#define PDL_HAS_THREADS
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD
#define PDL_LACKS_PTHREAD_CANCEL
#define PDL_HAS_SIGWAIT
//#define       PDL_HAS_PTHREAD_YIELD_VOID_PTR
//#define       PDL_HAS_PTHREAD_ATTR_INIT
//#define       PDL_HAS_PTHREAD_ATTR_DESTROY
#define PDL_LACKS_THREAD_PROCESS_SCOPING
//#define PDL_LACKS_THREAD_STACK_ADDR
// If PDL doesn't compile due to the lack of these methods, please
// send email to schmidt@cs.wustl.edu reporting this.
// #define PDL_LACKS_CONDATTR_PSHARED
// #define PDL_LACKS_MUTEXATTR_PSHARED
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SETSCHED
#define PDL_HAS_POSIX_TIME

#include <pthread.h>
#include <sys/regset.h>

#define PDL_LACKS_TIMEDWAIT_PROTOTYPES
#define PDL_HAS_RECV_TIMEDWAIT
#define PDL_HAS_RECVFROM_TIMEDWAIT
#define PDL_HAS_RECVMSG_TIMEDWAIT
#define PDL_HAS_SEND_TIMEDWAIT
#define PDL_HAS_SENDTO_TIMEDWAIT
#define PDL_HAS_SENDMSG_TIMEDWAIT
#define PDL_HAS_READ_TIMEDWAIT
#define PDL_HAS_READV_TIMEDWAIT
#define PDL_HAS_WRITE_TIMEDWAIT
#define PDL_HAS_WRITEV_TIMEDWAIT
#define PDL_HAS_DIRENT

#endif /* PDL_CONFIG_H */

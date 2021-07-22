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
// config-osf1-3.2.h,v 4.22 1998/12/10 17:43:11 levine Exp

// The following configuration file is designed to work for OSF1 3.2
// platforms with the DEC 5.1 C++ compiler.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define PDL_LACKS_SETSCHED
#define PDL_LACKS_RWLOCK_T
// DF: this platform uses Digital's CXX compiler
#define DEC_CXX

// DF: DEC's CXX supports explicit template specialization.
#define PDL_HAS_TEMPLATE_SPECIALIZATION

// DF: 3.2 has getpgid but no prototype defined anywhere.  So we cheat
// and declare it here.
extern "C" pid_t getpgid (pid_t);

// DF: PDL_HAS_STRING_CLASS seems the right thing to do...
#define PDL_HAS_STRING_CLASS

// DF: Seems apropriate since this is a new compiler...
#if !defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#define PDL_HAS_BROKEN_MSG_H
#define PDL_LACKS_SYSV_MSQ_PROTOS

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// Platform supports <sys/procfs.h>
#define PDL_HAS_PROC_FS

#define PDL_HAS_UALARM

// If PDL doesn't compile due to the lack of these methods, please
// send email to schmidt@cs.wustl.edu reporting this.
// #define PDL_LACKS_CONDATTR_PSHARED
// #define PDL_LACKS_MUTEXATTR_PSHARED

// Platform lacks support for stack address information
#define PDL_LACKS_THREAD_STACK_ADDR

// Platform lacks thread process scoping
#define PDL_LACKS_THREAD_PROCESS_SCOPING

// Platform has non-POSIX setkind and other functions.
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK
#define PDL_HAS_SETKIND_NP

// Platform supports POSIX 1.b clock_gettime ()
#define PDL_HAS_CLOCK_GETTIME

// Platform defines MAP_FAILED as a long constant.
#define PDL_HAS_LONG_MAP_FAILED

// Platform's implementation of sendmsg() has a non-const msgheader parameter.
#define PDL_HAS_BROKEN_SENDMSG

// Platform's implementation of writev() has a non-const iovec parameter.
#define PDL_HAS_BROKEN_WRITEV

// Platform's implementation of setlrmit() has a non-const rlimit parameter.
#define PDL_HAS_BROKEN_SETRLIMIT

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// Compiler/platform correctly calls init()/fini().
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Prototypes for both signal() and struct sigaction are consistent.
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform has thread-specific storage
#define PDL_HAS_THREAD_SPECIFIC_STORAGE

// Platform supports C++ headers
#define PDL_HAS_CPLUSPLUS_HEADERS

// Compiler/platform has the getrusage() system call.
#define PDL_HAS_GETRUSAGE

// Platform supports the OSF PDL_TLI timod STREAMS module.
#define PDL_HAS_OSF_TIMOD_H

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME

// Platform defines struct timespec in <sys/timers.h>
#define PDL_HAS_BROKEN_POSIX_TIME

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

#define PDL_LACKS_PTHREAD_THR_SIGSETMASK

// PDL supports POSIX Pthreads. OSF/1 3.2 has draft 4
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_DRAFT4
#define PDL_HAS_THREAD_SELF

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Added 6/13/95, 1 line
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T

// Compiler/platform has ssize_t.
#define PDL_HAS_SSIZE_T

// Compiler/platform supports struct strbuf.
#define PDL_HAS_STRBUF_T

// Platform supports STREAMS.
#define PDL_HAS_STREAMS

// Platform has 64bit longs and 32bit ints...
// NOTE: PDL_HAS_64BIT_LONGS is deprecated.  Instead, use PDL_SIZEOF_LONG == 8.
#define PDL_HAS_64BIT_LONGS

// Platform supports STREAM pipes.
// #define PDL_HAS_STREAM_PIPES

// Compiler/platform supports SVR4 dynamic linking semantics.
#define PDL_HAS_SVR4_DYNAMIC_LINKING

// Platform support OSF1 gettimeofday
#define PDL_HAS_OSF1_GETTIMEOFDAY

// Compiler/platform supports SVR4 signal typedef.
#define PDL_HAS_SVR4_SIGNAL_T

// Compiler/platform has strerror().
#define PDL_HAS_STRERROR

// PDL supports threads.
#define PDL_HAS_THREADS

// Platform supports PDL_TLI tiuser header.
#define PDL_HAS_TIUSER_H

// Platform supports PDL_TLI timod STREAMS module.
// #define PDL_HAS_TIMOD_H

// Platform provides PDL_TLI function prototypes.
//#define PDL_HAS_TLI_PROTOTYPES

// Platform supports PDL_TLI.
//#define PDL_HAS_TLI

// Compile using multi-thread libraries.
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#define PDL_NEEDS_DEV_IO_CONVERSION

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

// Defines the page size of the system.
#define PDL_PAGE_SIZE 8192
#define PDL_HAS_GETPAGESIZE

#endif /* PDL_CONFIG_H */

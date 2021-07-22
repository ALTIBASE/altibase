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
// config-dgux-4.11-epc.h,v 4.15 1998/10/20 15:35:38 levine Exp

// The following configuration file is designed to work for DG/UX
// 4.11 platforms using the EPC compiler.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define PDL_DGUX

// Platform requires (struct sockaddr *) for msg_name field of struct
// msghdr.
#define PDL_HAS_SOCKADDR_MSG_NAME

// Platform lacks strcasecmp().
#define PDL_LACKS_STRCASECMP

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Platform supports system configuration information.
#define PDL_HAS_SYSINFO

// Platform supports the POSIX regular expression library.
#define PDL_HAS_REGEX

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
// #define PDL_HAS_SYSCALL_H

// Platform supports reentrant functions (i.e., all the POSIX *_r functions).
#define PDL_HAS_REENTRANT_FUNCTIONS

// Platform has terminal ioctl flags like TCGETS and TCSETS.
#define PDL_HAS_TERM_IOCTLS

// Compiler/platform correctly calls init()/fini() for shared libraries.
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Compiler/platform supports SunOS high resolution timers.
// #define PDL_HAS_HI_RES_TIMER

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Compiler/platform supports alloca()
// #define PDL_HAS_ALLOCA

// Compiler/platform has <alloca.h>
// #define PDL_HAS_ALLOCA_H

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME

// Platform supports the /proc file system.
#define PDL_HAS_PROC_FS

// Platform supports the prusage_t struct.
// #define PDL_HAS_PRUSAGE_T
#define PDL_HAS_GETRUSAGE

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Platform supports SVR4 extended signals.
#define PDL_HAS_SIGINFO_T

// Compiler/platform provides the sockio.h file.
#define PDL_HAS_SOCKIO_H

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Platform supports STREAMS.
#define PDL_HAS_STREAMS

// Platform supports STREAM pipes.
#define PDL_HAS_STREAM_PIPES

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// Compiler/platform supports struct strbuf.
#define PDL_HAS_STRBUF_T

// Compiler/platform supports SVR4 dynamic linking semantics.
// #define PDL_HAS_SVR4_DYNAMIC_LINKING

// Compiler/platform supports SVR4 gettimeofday() prototype.
// #define PDL_HAS_SVR4_GETTIMEOFDAY

// DG/UX uses the same gettimeofday() prototype as OSF/1.
#define PDL_HAS_OSF1_GETTIMEOFDAY

// Compiler/platform supports SVR4 signal typedef.
#define PDL_HAS_SVR4_SIGNAL_T

// Compiler/platform supports SVR4 PDL_TLI (in particular, T_GETNAME stuff)...
#define PDL_HAS_SVR4_TLI

// Platform provides <sys/filio.h> header.
// #define PDL_HAS_SYS_FILIO_H

// Compiler/platform supports sys_siglist array.
#define PDL_HAS_SYS_SIGLIST

/* Turn off the following four defines if you want to disable threading. */
// Compile using multi-thread libraries.
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 0
#endif

// Platform supports threads.
// #define PDL_HAS_THREADS

// Platform supports POSIX pthreads *and* Solaris threads!
// #define PDL_HAS_STHREADS
// #define PDL_HAS_PTHREADS
#define PDL_HAS_SIGWAIT
// If PDL doesn't compile due to the lack of these methods, please
// send email to schmidt@cs.wustl.edu reporting this.
// #define PDL_LACKS_CONDATTR_PSHARED
// #define PDL_LACKS_MUTEXATTR_PSHARED

// Compiler/platform has thread-specific storage
//
#define PDL_HAS_THREAD_SPECIFIC_STORAGE

// Reactor detects deadlock
// #define PDL_REACTOR_HAS_DEADLOCK_DETECTION

// Platform supports PDL_TLI timod STREAMS module.
#define PDL_HAS_TIMOD_H

// Platform supports PDL_TLI tiuser header.
#define PDL_HAS_TIUSER_H

// Platform provides PDL_TLI function prototypes.
//#define PDL_HAS_TLI_PROTOTYPES

// Platform supports PDL_TLI.
//#define PDL_HAS_TLI

// Use the poll() event demultiplexor rather than select().
//#define PDL_USE_POLL

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

// #define _USING_POSIX4A_DRAFT6
#define _POSIX_SOURCE
#define _DGUX_SOURCE
// #define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
#define PDL_HAS_UCONTEXT_T
#define PDL_LACKS_SYSTIME_H
#define PDL_HAS_NONCONST_GETBY
#define PDL_LACKS_MADVISE

#if !defined (IP_ADD_MEMBERSHIP)
# define IP_ADD_MEMBERSHIP 0x13
#endif

#if !defined (IP_DROP_MEMBERSHIP)
# define IP_DROP_MEMBERSHIP 0x14
#endif

// Header files lack t_errno for PDL_TLI.
#define PDL_LACKS_T_ERRNO

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_IDTYPE_T

#endif /* PDL_CONFIG_H */

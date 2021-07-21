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
// config-m88k.h,v 4.14 1998/10/20 15:35:38 levine Exp

// The following configuration file is designed to work for Motorola
// 88k SVR4 platforms using pthreads from Florida State (PDL_HAS_FSU_PTHREADS).

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if ! defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if defined (__GNUG__)
# include "config-g++-common.h"
  // This config file has not been tested with PDL_HAS_TEMPLATE_SPECIALIZATION.
  // Maybe it will work?
# undef PDL_HAS_TEMPLATE_SPECIALIZATION
#endif /* __GNUG__ */

#if !defined (m88k)
#define m88k
#endif

extern "C" void pthread_init();

#define PTHREAD_STACK_MIN 1024

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

#if !defined (IP_ADD_MEMBERSHIP)
#define IP_ADD_MEMBERSHIP 0x13
#endif  /*  m88k */

#if !defined (IP_DROP_MEMBERSHIP)
#define IP_DROP_MEMBERSHIP 0x14
#endif  /*  m88k */

struct sched_param
{
  int sched_priority;
  int prio;
};

// This seems to be necessary for m88k.
struct ip_mreq
{
  struct in_addr imr_multiaddr; // IP multicast address of the group
  struct in_addr imr_interface; // local IP address of the interface
};

#if !defined (PDL_HAS_FSU_PTHREADS)
# define PDL_HAS_FSU_PTHREADS
#endif
#if !defined (PDL_HAS_PTHREADS_DRAFT6)
# define PDL_HAS_PTHREADS_DRAFT6
#endif

// Added for compilation on the m88k
#if defined (m88k)
# define PDL_LACKS_T_ERRNO
# define PDL_LACKS_MADVISE
# define PDL_HAS_GNU_CSTRING_H
#endif  /* m88k */

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// Sun has the wrong prototype for sendmsg.
#define PDL_HAS_BROKEN_SENDMSG

// The SunOS 5.x version of rand_r is inconsistent with the header files...
#define PDL_HAS_BROKEN_RANDR

// Platform supports system configuration information.
#define PDL_HAS_SYSINFO

// Platform supports the POSIX regular expression library.
#define PDL_HAS_REGEX

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

#if !defined (PDL_HAS_FSU_PTHREADS)
// Platform supports reentrant functions (i.e., all the POSIX *_r functions).
#define PDL_HAS_REENTRANT_FUNCTIONS
#endif  /* PDL_HAS_FSU_PTHREADS */

// Platform has terminal ioctl flags like TCGETS and TCSETS.
#define PDL_HAS_TERM_IOCTLS

// Compiler/platform correctly calls init()/fini() for shared libraries.
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

#if !defined (m88k)
// Compiler/platform supports SunOS high resolution timers.
# define PDL_HAS_HI_RES_TIMER
#endif  /* m88k */

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Compiler/platform supports alloca()
#define PDL_HAS_ALLOCA

#if !defined (m88k)
// Compiler/platform has <alloca.h>
#define PDL_HAS_ALLOCA_H
#endif  /* m88k */

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME

// Platform supports the /proc file system.
#define PDL_HAS_PROC_FS

#if !defined (m88k)
// Platform supports the prusage_t struct.
#define PDL_HAS_PRUSAGE_T
#endif  /* m88k */

// Compiler/platform defines the sig_atomic_t typedef.
#define PDL_HAS_SIG_ATOMIC_T

// Platform supports SVR4 extended signals.
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T

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
#define PDL_HAS_SVR4_DYNAMIC_LINKING

// Compiler/platform supports SVR4 gettimeofday() prototype.
#define PDL_HAS_SVR4_GETTIMEOFDAY

// Compiler/platform supports SVR4 signal typedef.
#define PDL_HAS_SVR4_SIGNAL_T

// Compiler/platform supports SVR4 PDL_TLI (in particular, T_GETNAME stuff)...
#define PDL_HAS_SVR4_TLI

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

#if !defined (m88k)
// Compiler/platform supports sys_siglist array.
#define PDL_HAS_SYS_SIGLIST
#endif  /* m88k */

/* Turn off the following five defines if you want to disable threading. */
// Compile using multi-thread libraries.
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#define PDL_HAS_PTHREADS
#define PDL_LACKS_RWLOCK_T

// Platform supports threads.
#define PDL_HAS_THREADS

#if defined (PDL_HAS_FSU_PTHREADS)
#define PDL_LACKS_THREAD_STACK_ADDR
#endif  /*  PDL_HAS_FSU_PTHREADS */

// Compiler/platform has thread-specific storage
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

#endif /* PDL_CONFIG_H */

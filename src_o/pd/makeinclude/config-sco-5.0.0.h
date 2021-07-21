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
// config-sco-5.0.0.h,v 4.13 1999/07/16 17:49:46 schmidt Exp

#ifndef PDL_CONFIG_SCO_5_0_0_H
#define PDL_CONFIG_SCO_5_0_0_H

// Compiling for SCO.
#if !defined (SCO)
#define SCO
#endif /* SCO */

#if defined (SCO) && !defined (MAXPATHLEN)
#define MAXPATHLEN 1023
#endif /* SCO */

#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_SIG_MACROS
#define PDL_LACKS_CONST_TIMESPEC_PTR
#define PDL_LACKS_SYSCALL
#define PDL_LACKS_STRRECVFD
#define PDL_NEEDS_FTRUNCATE
#define PDL_LACKS_MADVISE
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS

#define PDL_DEFAULT_CLOSE_ALL_HANDLES 0

// Compiler doesn't support static data member templates.
//#define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC
#define PDL_HAS_NONCONST_MSGSND
#define PDL_HAS_BIG_FD_SET
// #define      PDL_LACKS_POSIX_PROTOTYPES
#define PDL_HAS_SVR4_DYNAMIC_LINKING
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Platform supports Term Ioctls
#define PDL_HAS_TERM_IOCTLS

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

#define PDL_HAS_TIMEZONE_GETTIMEOFDAY

// HP/UX has an undefined syscall for GETRUSAGE...
//#define PDL_HAS_SYSCALL_GETRUSAGE

// Note, this only works if the flag is set above!
//#define PDL_HAS_GETRUSAGE

// Platform uses int for select() rather than fd_set.
#define PDL_HAS_SELECT_H

// Platform has prototypes for PDL_TLI.
//#define PDL_HAS_TLI_PROTOTYPES
// Platform has the XLI version of PDL_TLI.
// #define PDL_HAS_XLI

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_HAS_SIGINFO_T
#define PDL_HAS_UCONTEXT_T

#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_STRCASECMP

// #define      PDL_HAS_POSIX_TIME
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_DIRENT
#define PDL_LACKS_READDIR_R
#define PDL_HAS_GPERF

#endif /* PDL_CONFIG_SCO_5_0_0_H */

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
// config-hpux-9.x.h,v 4.12 2000/01/08 02:39:46 schmidt Exp

// The following configuration file is designed to work
// for HP platforms running HP/UX 9.x.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// Compiling for HPUX.
#if !defined (HPUX)
#define HPUX
#endif /* HPUX */

#if __cplusplus < 199707L
#define PDL_HAS_BROKEN_HPUX_TEMPLATES
#endif /* __cplusplus < 199707L */

#define PDL_HAS_IP_MULTICAST
#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
#define PDL_HAS_BROKEN_CONVERSIONS
// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// They forgot a const in the prototype of const_timewait...
#define PDL_LACKS_CONST_TIMESPEC_PTR

#define PDL_LACKS_SYSCALL
#define PDL_LACKS_STRRECVFD

// Compiler doesn't support static data member templates.
#define PDL_LACKS_STATIC_DATA_MEMBER_TEMPLATES

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC			

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// Fixes a problem with HP/UX not wrapping the mmap(2) header files
// with extern "C".
#define PDL_HAS_BROKEN_MMAP_H

// Prototypes for both signal() and struct sigaction are consistent.
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Header files lack t_errno for PDL_TLI.
#define PDL_LACKS_T_ERRNO

// Compiler/platform supports poll().
#define PDL_HAS_POLL

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Compiler/platform defines the sig_atomic_t typedef
#define PDL_HAS_SIG_ATOMIC_T

// Compiler supports the ssize_t typedef.
#define PDL_HAS_SSIZE_T

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// ???
#define PDL_HAS_SUNOS4_GETTIMEOFDAY

// HP/UX has an undefined syscall for GETRUSAGE...
#define PDL_HAS_SYSCALL_GETRUSAGE

// Note, this only works if the flag is set above!
#define PDL_HAS_GETRUSAGE

// Platform uses int for select() rather than fd_set.
#define PDL_SELECT_USES_INT

// Platform has prototypes for PDL_TLI.
//#define PDL_HAS_TLI_PROTOTYPES
// Platform has the XLI version of PDL_TLI.
// #define PDL_HAS_XLI

#define PDL_LACKS_PDL_IOSTREAM

#define PDL_NEEDS_DEV_IO_CONVERSION

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

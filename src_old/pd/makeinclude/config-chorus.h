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
// config-chorus.h,v 4.54 2000/01/08 02:39:45 schmidt Exp

// The following configuration file is designed to work for Chorus
// platforms using one of these compilers:
//   * GNU g++
//   * GreenHills
// It uses the Chorus POSIX threads interface.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#define CHORUS 3.1b
#if defined (linux)
  // This shouldn't be necessary.
# undef linux
#endif /* linux */

// Compiler-specific configuration.

#if defined (__GNUG__)
# include "config-g++-common.h"
# undef PDL_HAS_ANSI_CASTS
# define PDL_LACKS_CHAR_STAR_RIGHT_SHIFTS
#elif defined (ghs)
# define PDL_CONFIG_INCLUDE_GHS_COMMON
# include "config-ghs-common.h"

# define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
# define PDL_HAS_TANDEM_SIGNALS
# define PDL_HAS_TEMPLATE_SPECIALIZATION
# define PDL_LACKS_PDL_IOSTREAM  /* MVME lacks signed and unsigned char */
# define PDL_LACKS_FLOATING_POINT
#else  /* ! __GNUG__ && ! ghs */
# error unsupported compiler for PDL on Chorus
#endif /* ! __GNUG__ && ! ghs */

// OS-specific configuration

#define PDL_CHORUS_DEFAULT_MIN_STACK_SIZE 0x2000
// Chorus cannot grow shared memory, so this is the default size for a
// local name space
#define PDL_CHORUS_LOCAL_NAME_SPACE_T_SIZE 128000
// Used in OS.i to map an actor id into a KnCap.
#define PDL_CHORUS_MAX_ACTORS 64

#define PDL_HAS_BROKEN_READV
#define PDL_HAS_CLOCK_GETTIME
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_DIRENT
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_LONG_MAP_FAILED
#define PDL_HAS_MSG
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_POSIX_SEM
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SSIZE_T
#define PDL_HAS_STRDUP_EMULATION
#define PDL_HAS_STRERROR
#define PDL_HAS_TSS_EMULATION
#define PDL_LACKS_ACCESS
#define PDL_LACKS_CONDATTR_PSHARED
#define PDL_LACKS_FORK
#define PDL_LACKS_FSYNC
#define PDL_LACKS_GETHOSTENT
#define PDL_LACKS_GETPGID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
#define PDL_LACKS_GETSERVBYNAME
#define PDL_LACKS_KEY_T
#define PDL_LACKS_LONGLONG_T
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MALLOC_H
#define PDL_LACKS_MEMORY_H
#define PDL_LACKS_MKFIFO
#define PDL_LACKS_MPROTECT
#define PDL_LACKS_MSYNC
#define PDL_LACKS_NAMED_POSIX_SEM
#define PDL_LACKS_PARAM_H
#define PDL_LACKS_READDIR_R
#define PDL_LACKS_READLINK
#define PDL_LACKS_READV
#define PDL_LACKS_RLIMIT
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SBRK
#define PDL_LACKS_SEMBUF_T
#define PDL_LACKS_SIGSET
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_SYSV_MSG_H
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_TRUNCATE
#define PDL_LACKS_UNIX_SIGNALS
#define PDL_LACKS_UTSNAME_T
#define PDL_LACKS_WRITEV
#define PDL_PAGE_SIZE 4096

// Yes, we do have threads.
#define PDL_HAS_THREADS
#define PDL_HAS_THREAD_SPECIFIC_STORAGE
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif
// And they're even POSIX pthreads
#define PDL_HAS_PTHREADS
#define PDL_HAS_PTHREADS_STD
#define PDL_HAS_PTHREAD_PROCESS_ENUM
#define PDL_LACKS_PTHREAD_CANCEL
#define PDL_LACKS_PTHREAD_CLEANUP
#define PDL_LACKS_PTHREAD_SIGMASK
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK

#if !defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

// By default, don't include RCS Id strings in object code.
#if !defined (PDL_USE_RCSID)
# define PDL_USE_RCSID 0
#endif /* #if !defined (PDL_USE_RCSID) */

// Needed to wait for "processes" to exit.
#include <am/await.h>

#endif /* PDL_CONFIG_H */

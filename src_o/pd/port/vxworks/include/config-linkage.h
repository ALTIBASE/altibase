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
// config-vxworks5.x.h,v 4.41 1999/11/18 04:48:16 levine Exp

// The following configuration file is designed to work for VxWorks
// 5.2/5.3 platforms using one of these compilers:
// 1) The GNU/Cygnus g++ compiler that is shipped with Tornado 1.0.1.
// 2) The Green Hills 1.8.8 (not 1.8.7!!!!) and 1.8.9 compilers.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if ! defined (VXWORKS)
# define VXWORKS
#endif /* ! VXWORKS */

#if ! defined (__PDL_INLINE__)
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// Compiler-specific configuration.
#if defined (__GNUG__)
# include "config-g++-common.h"
# undef PDL_HAS_TEMPLATE_SPECIALIZATION

# define PDL_LACKS_IOSTREAM_FX
# if !defined (PDL_MAIN)
#   define PDL_MAIN pdl_main
# endif /* ! PDL_MAIN */

  // Even though the documentation suggests that g++/VxWorks 5.3.1
  // (Tornado 1.0.1) supports long long, Wind River tech support says
  // that it doesn't.  It causes undefined symbols for math functions.
# define PDL_LACKS_LONGLONG_T

  // On g++/VxWorks, iostream.h defines a static instance (yes, instance)
  // of the Iostream_init class.  That causes all files that #include it
  // to put in the global constructor/destructor hooks.  For files that
  // don't have any static instances of non-class (built-in) types, the
  // hooks refer to the file name, e.g., "foo.cpp".  That file name gets
  // embedded in a variable name by munch.  The output from munch won't
  // compile, though, because of the period!  So, let g++/VxWorks users
  // include iostream.h only where they need it.
# define PDL_HAS_MINIMUM_IOSTREAMH_INCLUSION

# define PDL_LACKS_LINEBUFFERED_STREAMBUF
# define PDL_LACKS_SIGNED_CHAR

#elif defined (ghs)
  // Processor type, if necessary.  Green Hills defines "ppc".
# if defined (ppc)
#   define PDL_HAS_POWERPC_TIMER
# elif defined (i386) || defined (__i386__)
    // If running an Intel, assume that it's a Pentium so that
    // PDL_OS::gethrtime () can use the RDTSC instruction.  If
    // running a 486 or lower, be sure to comment this out.
    // (If not running an Intel CPU, this #define will not be seen
    //  because of the i386 protection, so it can be ignored.)
#   define PDL_HAS_PENTIUM
# endif /* ppc || i386 */

# define PDL_CONFIG_INCLUDE_GHS_COMMON
# include "config-ghs-common.h"

# define PDL_LACKS_UNISTD_H

#elif defined (__DCPLUSPLUS__)
  // Diab 4.2a or later.
# if !defined (PDL_LACKS_PRAGMA_ONCE)
    // We define it with a -D with make depend.
#   define PDL_LACKS_PRAGMA_ONCE
# endif /* ! PDL_LACKS_PRAGMA_ONCE */

  // Diab doesn't support VxWorks' iostream libraries.
# define PDL_LACKS_IOSTREAM_TOTALLY

  // #include <new.h> causes strange compilation errors in
  // the system header files.
# define PDL_LACKS_NEW_H

#else  /* ! __GNUG__ && ! ghs */
# error unsupported compiler on VxWorks
#endif /* ! __GNUG__ && ! ghs */

// OS-specific configuration

#define PDL_DEFAULT_MAX_SOCKET_BUFSIZ 32768
#define PDL_DEFAULT_THREAD_KEYS 16
#define PDL_HAS_BROKEN_ACCEPT_ADDR
#define PDL_HAS_BROKEN_SENDMSG
#define PDL_HAS_BROKEN_WRITEV
#define PDL_HAS_CHARPTR_SOCKOPT
#define PDL_HAS_CLOCK_GETTIME
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES
#define PDL_HAS_CPLUSPLUS_HEADERS
#define PDL_HAS_DIRENT
#define PDL_HAS_DLL 0
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_MSG
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_NONSTATIC_OBJECT_MANAGER
#define PDL_HAS_POSIX_NONBLOCK
#define PDL_HAS_POSIX_TIME
#define PDL_HAS_RECURSIVE_MUTEXES
#define PDL_HAS_SIGINFO_T
#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIG_ATOMIC_T
#define PDL_HAS_STRDUP_EMULATION
#define PDL_HAS_STRERROR
#define PDL_HAS_THREADS
#define PDL_HAS_TSS_EMULATION
#define PDL_LACKS_ACCESS
#define PDL_LACKS_COND_T
#define PDL_LACKS_EXEC
#define PDL_LACKS_FCNTL
#define PDL_LACKS_FILELOCKS
#define PDL_LACKS_FORK
#define PDL_LACKS_FSYNC
#define PDL_LACKS_GETHOSTENT
#define PDL_LACKS_GETSERVBYNAME
#define PDL_LACKS_KEY_T
#define PDL_LACKS_LSTAT
#define PDL_LACKS_MADVISE
#define PDL_LACKS_MALLOC_H
#define PDL_LACKS_MEMORY_H
#define PDL_LACKS_MKFIFO
//#define PDL_LACKS_MKTEMP
#define PDL_LACKS_MMAP
#define PDL_LACKS_MPROTECT
#define PDL_LACKS_MSYNC
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS
#define PDL_LACKS_PARAM_H
#define PDL_LACKS_PWD_FUNCTIONS
#define PDL_LACKS_READDIR_R
#define PDL_LACKS_READLINK
#define PDL_LACKS_RLIMIT
#define PDL_LACKS_RWLOCK_T
#define PDL_LACKS_SBRK
#define PDL_LACKS_SEEKDIR
#define PDL_LACKS_SEMBUF_T
#define PDL_LACKS_SIGINFO_H
#define PDL_LACKS_SI_ADDR
#define PDL_LACKS_SOCKETPAIR
#define PDL_LACKS_STRCASECMP
#define PDL_LACKS_STRRECVFD
#define PDL_LACKS_SYSCALL
#define PDL_LACKS_SYSTIME_H
#define PDL_LACKS_SYSV_MSG_H
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_SYS_NERR
#define PDL_LACKS_TELLDIR
#define PDL_LACKS_TIMESPEC_T
#define PDL_LACKS_TRUNCATE
#define PDL_LACKS_UCONTEXT_H
#define PDL_LACKS_UNIX_SIGNALS
#define PDL_LACKS_UTSNAME_T
#define PDL_PAGE_SIZE 4096
#define PDL_THR_PRI_FIFO_DEF 101
#define PDL_THR_PRI_OTHER_DEF PDL_THR_PRI_FIFO_DEF
#define PDL_HAS_SIGTIMEDWAIT
#define PDL_HAS_SIGSUSPEND
#if !defined (PDL_VXWORKS_SPARE)
# define PDL_VXWORKS_SPARE spare4
#endif /* ! PDL_VXWORKS_SPARE */

#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

#if !defined (PDL_NEEDS_HUGE_THREAD_STACKSIZE)
# define PDL_NEEDS_HUGE_THREAD_STACKSIZE 64000
#endif /* PDL_NEEDS_HUGE_THREAD_STACKSIZE */

#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

// By default, don't include RCS Id strings in object code.
#if !defined (PDL_USE_RCSID)
#define PDL_USE_RCSID 0
#endif /* #if !defined (PDL_USE_RCSID) */

#define USE_RAW_DEVICE 1

#define SMALL_FOOTPRINT 1

#endif /* PDL_CONFIG_H */

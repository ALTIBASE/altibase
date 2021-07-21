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
// config-sunos5.5.h,v 4.77 2000/02/12 21:17:54 nanbor Exp

// This configuration file is designed to work for SunOS 5.5 platforms
// using the following compilers:
//   * Sun C++ 4.2 and later (including 5.x), patched as noted below
//   * g++ 2.7.2 and later, including egcs
//   * Green Hills 1.8.8 and later

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

// Compiler version-specific settings:
#if defined (__SUNPRO_CC)
# if (__SUNPRO_CC < 0x410)
    // The following might not be necessary, but I can't tell: my build
    // with Sun C++ 4.0.1 never completes.
#   define PDL_NEEDS_DEV_IO_CONVERSION
# elif (__SUNPRO_CC >= 0x420)
# define PDL_HAS_ANSI_CASTS
# if (__SUNPRO_CC >= 0x500)
// Sun C++ 5.0 supports the `using' and `typename' keywords.
#   define PDL_HAS_USING_KEYWORD
#   define PDL_HAS_TYPENAME_KEYWORD
    /* Explicit instantiation requires the -instances=explicit
       CCFLAG.  It seems to work for the most part, except for:
       1) Static data members get instantiated multiple times.
       2) In TAO, the TAO_Unbounded_Sequence vtbl can't be found.
       With CC 5.0, those problems may be fixed.  And, this is necessary
       to work around problems with automatic template instantiation. */
#   define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION
#   define PDL_TEMPLATES_REQUIRE_SOURCE
    // If -compat=4 is turned on, the old 4.2 settings for iostreams are used,
    // but the newer, explicit instantiation is used (above)
#   if (__SUNPRO_CC_COMPAT >= 5)
#     define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
// Note that SunC++ 5.0 doesn't yet appear to support
// PDL_HAS_STD_TEMPLATE_METHOD_SPECIALIZATION...
#     define PDL_HAS_STANDARD_CPP_LIBRARY 1
#     define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
#     define PDL_USES_OLD_IOSTREAMS 1
#     define PDL_HAS_THR_C_DEST
#   endif
#  if !defined (PDL_HAS_EXCEPTIONS)
     // See /opt/SUNWspro_5.0/SC5.0/include/CC/stdcomp.h:
#    define _RWSTD_NO_EXCEPTIONS 1
#  endif /* ! PDL_HAS_EXCEPTIONS */
# elif (__SUNPRO_CC == 0x420)
# define PDL_LACKS_PLACEMENT_OPERATOR_DELETE
# endif /* __SUNPRO_CC >= 0x500 */
# endif /* __SUNPRO_CC >= 0x420 */

# define PDL_CAST_CONST const
# define PDL_HAS_HI_RES_TIMER
# define PDL_HAS_SIG_C_FUNC /* Sun CC 5.0 needs this, 4.2 doesn't mind. */
# define PDL_HAS_TEMPLATE_SPECIALIZATION
# define PDL_HAS_XPG4_MULTIBYTE_CHAR
# define PDL_LACKS_LINEBUFFERED_STREAMBUF
# define PDL_LACKS_SIGNED_CHAR

  // PDL_HAS_EXCEPTIONS precludes -noex in
  // makeinclude/platform_macros.GNU.  But beware, we have
  // seen problems with exception handling on multiprocessor
  // UltraSparcs:  threaded executables core dump when threads exit.
  // This problem does not seem to appear on single-processor UltraSparcs.
  // And, it is solved with the application of patch
  //   104631-02 "C++ 4.2: Jumbo Patch for C++ 4.2 on Solaris SPARC"
  // to Sun C++ 4.2.
  // To provide optimum performance, PDL_HAS_EXCEPTIONS is disabled by
  // default.  It can be enabled by adding "exceptions=1" to the "make"
  // invocation.  See makeinclude/platform_sunos5_sunc++.GNU
  // for details.

#  if defined (PDL_HAS_EXCEPTIONS)
     // If exceptions are enabled and we are using Sun/CC then
     // <operator new> throws an exception instead of returning 0.
/* #    define PDL_NEW_THROWS_EXCEPTIONS */
#  endif /* PDL_HAS_EXCEPTIONS */

    /* If you want to disable threading with Sun CC, remove -mt
       from your CFLAGS, e.g., using make threads=0. */

#elif defined (__GNUG__)
  // config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
  // this must appear before its #include.
# define PDL_HAS_STRING_CLASS
# include "config-g++-common.h"
# define PDL_HAS_HI_RES_TIMER
  // Denotes that GNU has cstring.h as standard, to redefine memchr().
# define PDL_HAS_GNU_CSTRING_H
# define PDL_HAS_XPG4_MULTIBYTE_CHAR

# if !defined (PDL_MT_SAFE) || PDL_MT_SAFE != 0
    // PDL_MT_SAFE is #defined below, for all compilers.
#   if !defined (_REENTRANT)
    /* If you want to disable threading, comment out the following
       line.  Or, add -DPDL_MT_SAFE=0 to your CFLAGS, e.g., using
       make threads=0. */
#     define _REENTRANT
#   endif /* _REENTRANT */
# endif /* !PDL_MT_SAFE */

#elif defined (ghs)

# if !defined (PDL_MT_SAFE) || PDL_MT_SAFE != 0
    // PDL_MT_SAFE is #defined below, for all compilers.
#   if !defined (_REENTRANT)
    /* If you want to disable threading, comment out the following
       line.  Or, add -DPDL_MT_SAFE=0 to your CFLAGS, e.g., using
       make threads=0. */
#     define _REENTRANT
#   endif /* _REENTRANT */
# endif /* !PDL_MT_SAFE */

# define PDL_CONFIG_INCLUDE_GHS_COMMON
# include "config-ghs-common.h"

  // To avoid warning about inconsistent declaration between Sun's
  // stdlib.h and Green Hills' ctype.h.
# include <stdlib.h>

  // IOStream_Test never halts with Green Hills 1.8.9.
# define PDL_LACKS_PDL_IOSTREAM

#elif defined (__KCC) /* KAI compiler */

# define PDL_HAS_ANSI_CASTS
# include "config-kcc-common.h"

#else  /* ! __SUNPRO_CC && ! __GNUG__  && ! ghs */
# error unsupported compiler in config-sunos5.5.h
#endif /* ! __SUNPRO_CC && ! __GNUG__  && ! ghs */

#if !defined (__PDL_INLINE__)
// NOTE:  if you have link problems with undefined inline template
// functions with Sun C++, be sure that the #define of __PDL_INLINE__
// below is not commented out.
# define __PDL_INLINE__
#endif /* ! __PDL_INLINE__ */

// Platform supports the POSIX regular expression library.
// NOTE:  please comment out the PDL_HAS_REGEX #define if you
// have link problems with g++ or egcs on SunOS 5.5.
#define PDL_HAS_REGEX

// Maximum compensation (10 ms) for early return from timed ::select ().
#if !defined (PDL_TIMER_SKEW)
# define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// select()'s timeval arg is not declared as const and may be modified
#define PDL_HAS_NONCONST_SELECT_TIMEVAL

// Platform supports pread() and pwrite()
#define PDL_HAS_P_READ_WRITE
#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS
#define PDL_HAS_UALARM
#define PDL_LACKS_UALARM_PROTOTYPE

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Platform supports system configuration information.
#define PDL_HAS_SYSINFO

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H

// Platform has terminal ioctl flags like TCGETS and TCSETS.
#define PDL_HAS_TERM_IOCTLS

// Compiler/platform correctly calls init()/fini() for shared libraries.
#define PDL_HAS_AUTOMATIC_INIT_FINI

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Compiler/platform supports alloca()
#define PDL_HAS_ALLOCA

// Compiler/platform has <alloca.h>
#define PDL_HAS_ALLOCA_H

// Platform contains <poll.h>.
#define PDL_HAS_POLL

// Platform supports POSIX timers via timestruc_t.
#define PDL_HAS_POSIX_TIME

// PDL_HAS_CLOCK_GETTIME requires linking with -lposix4.
#define PDL_HAS_CLOCK_GETTIME

// Platform supports the /proc file system.
#define PDL_HAS_PROC_FS

// Platform supports the prusage_t struct.
#define PDL_HAS_PRUSAGE_T

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

// Compiler/platform supports SVR4 PDL_TLI (in particular, T_GETNAME stuff)...
#define PDL_HAS_SVR4_TLI

// Platform provides <sys/filio.h> header.
#define PDL_HAS_SYS_FILIO_H

// Compiler/platform supports sys_siglist array.
#define PDL_HAS_SYS_SIGLIST

#if defined (_REENTRANT) || \
 (defined (_POSIX_C_SOURCE) && (_POSIX_C_SOURCE - 0 >= 199506L)) || \
 defined (_POSIX_PTHREAD_SEMANTICS)
  // Compile using multi-thread libraries.
# define PDL_HAS_THREADS

# if !defined (PDL_MT_SAFE)
#   define PDL_MT_SAFE 1
# endif /* PDL_MT_SAFE */

  // Platform supports POSIX pthreads *and* Solaris threads, by
  // default!  If you only want to use POSIX pthreads, add
  // -D_POSIX_PTHREAD_SEMANTICS to your CFLAGS.  Or, #define it right
  // here.  See the Intro (3) man page for information on
  // -D_POSIX_PTHREAD_SEMANTICS.
# if defined (_POSIX_PTHREAD_SEMANTICS)
#   define PDL_LACKS_RWLOCK_T
# else
#   define PDL_HAS_STHREADS
# endif /* ! _POSIX_PTHREAD_SEMANTICS */

# define PDL_HAS_PTHREADS
# define PDL_HAS_PTHREADS_STD
  // . . . but only supports SCHED_OTHER scheduling policy
# define PDL_HAS_ONLY_SCHED_OTHER
# define PDL_HAS_SIGWAIT
# define PDL_HAS_SIGTIMEDWAIT
# define PDL_HAS_SIGSUSPEND

  // Compiler/platform has thread-specific storage
# define PDL_HAS_THREAD_SPECIFIC_STORAGE

  // Platform supports reentrant functions (i.e., all the POSIX *_r functions).
# define PDL_HAS_REENTRANT_FUNCTIONS

# define PDL_NEEDS_LWP_PRIO_SET
# define PDL_HAS_THR_YIELD
# define PDL_LACKS_PTHREAD_YIELD

// 2015/01/06 hcs
// Lacks support for pthread_attr_setstack()
# define PDL_LACKS_THREAD_SETSTACK
#endif /* _REENTRANT || _POSIX_C_SOURCE >= 199506L || \
          _POSIX_PTHREAD_SEMANTICS */

# define PDL_HAS_PRIOCNTL

// Platform supports PDL_TLI timod STREAMS module.
#define PDL_HAS_TIMOD_H

// Platform supports PDL_TLI tiuser header.
#define PDL_HAS_TIUSER_H

// Platform provides PDL_TLI function prototypes.
//#define PDL_HAS_TLI_PROTOTYPES

// Platform has broken t_error() prototype.
#define PDL_HAS_BROKEN_T_ERROR

// Platform supports PDL_TLI.
//#define PDL_HAS_TLI

#define PDL_HAS_STRPTIME

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#define PDL_HAS_GETPAGESIZE 1

#define PDL_HAS_STL_MAP_CONFLICT

// Sieg - gcc 2.95.1 declares queue in stream.h.  Might want to change
// the == to >= to allow for future versions
#if !( __GNUG__ && (__GNUC__ == 2) && (__GNUC_MINOR__ == 95) )
#define PDL_HAS_STL_QUEUE_CONFLICT
#endif /* !( __GNUG__ && (__GNUC__ == 2) && (__GNUC_MINOR__ == 95) ) */
#define PDL_HAS_IDTYPE_T

#define PDL_HAS_GPERF
#define PDL_HAS_DIRENT
#define PDL_HAS_MEMCHR

#if defined (__SUNPRO_CC)
# define PDL_CC_NAME "SunPro C++"
# define PDL_CC_MAJOR_VERSION (__SUNPRO_CC >> 8)
# define PDL_CC_MINOR_VERSION (__SUNPRO_CC & 0x00ff)
# define PDL_CC_BETA_VERSION  (0)
#elif defined (__GNUG__)
# define PDL_CC_MAJOR_VERSION __GNUC__
# define PDL_CC_MINOR_VERSION __GNUC_MINOR__
# define PDL_CC_BETA_VERSION  (0)
# if __GNUC_MINOR__ >= 90
#   define PDL_CC_NAME "egcs"
# else
#   define PDL_CC_NAME "g++"
# endif /* __GNUC_MINOR__ */
#endif /* __GNUG__ */

#if defined (i386)
# define PDL_HAS_X86_STAT_MACROS
#endif /* i386 */

#define PDL_MALLOC_ALIGN 8
#define PDL_LACKS_SETREUID_PROTOTYPE
#define PDL_LACKS_SETREGID_PROTOTYPE

#if defined (_LARGEFILE_SOURCE) || (_FILE_OFFSET_BITS==64)
#undef PDL_HAS_PROC_FS
#undef PDL_HAS_PRUSAGE_T
#endif /* (_LARGEFILE_SOURCE) || (_FILE_OFFSET_BITS==64) */

#if !defined(_LP64)
#undef FD_SETSIZE
#define FD_SETSIZE      65536  /* PR-1685 by jdlee */
#endif /* !defined(_LP64) */

/* For CPU information and affinity */
# include <sys/pset.h>
# include <kstat.h>

#endif /* PDL_CONFIG_H */

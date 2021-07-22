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
// config-aix-4.x.h,v 1.36 2002/04/06 01:10:41 shuston Exp

// The following configuration file is designed to work for OS
// platforms running AIX 4.x using the IBM C++ compiler (xlC),
// Visual Age C++ or g++/egcs.

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if defined (__xlC__) || defined (__IBMCPP__)
   // AIX xlC, IBM C/C++, and Visual Age C++ compilers
   //********************************************************************
   //

   // Compiler supports the ssize_t typedef.
#  define PDL_HAS_SSIZE_T

// When using the preprocessor, Ignore info msg; invalid #pragma
#  if defined (__IBMCPP__) && (__IBMCPP__ < 400)  // IBM C++ 3.6
#    define PDL_CC_PREPROCESSOR_ARGS "-E -qflag=w:w"
#  endif /* (__IBMCPP__) && (__IBMCPP__ < 400) */

   // Keep an eye on this as the compiler and standards converge...
#  define PDL_LACKS_LINEBUFFERED_STREAMBUF
#  define PDL_LACKS_PRAGMA_ONCE

   // C Set++ 3.1, IBM C/C++ 3.6, and Visual Age C++ 5 batch (__xlC__)
#  if defined (__xlC__)
#    if (__xlC__ < 0x0500)
#      define PDL_LACKS_PLACEMENT_OPERATOR_DELETE
#    endif /* __xlC__ < 0x0500 */
#    define PDL_TEMPLATES_REQUIRE_PRAGMA
     // If compiling without thread support, turn off PDL's thread capability.
#    if !defined (_THREAD_SAFE)
#      define PDL_HAS_THREADS 0
#    endif /* _THREAD_SAFE */
#  endif

   // These are for Visual Age C++ only
#  if defined (__IBMCPP__) && (__IBMCPP__ >= 400)
#    define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
#    define PDL_HAS_TYPENAME_KEYWORD
#    undef WIFEXITED
#    undef WEXITSTATUS
#    if (__IBMCPP__ >= 500)  /* Visual Age C++ 5 */
#      if !defined (PDL_HAS_USING_KEYWORD)
#        define PDL_HAS_USING_KEYWORD            1
#      endif /* PDL_HAS_USING_KEYWORD */
#      define PDL_HAS_STANDARD_CPP_LIBRARY 1
#      define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1
#    endif /* __IBMCPP__ >= 500 */
#  endif /* __IBMCPP__ */

#elif defined (__GNUG__)
  // config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
  // this must appear before its #include.
# define PDL_HAS_STRING_CLASS

# include "config-g++-common.h"
  // Denotes that GNU has cstring.h as standard, to redefine memchr().
# define PDL_HAS_GNU_CSTRING_H
# define PDL_HAS_SSIZE_T

# if !defined (PDL_MT_SAFE) || PDL_MT_SAFE != 0
    // PDL_MT_SAFE is #defined below, for all compilers.
#   if !defined (_REENTRANT)
#     define _REENTRANT
#   endif /* _REENTRANT */
# endif /* !PDL_MT_SAFE */

#else  /* ! __xlC__ && ! __GNUG__ */
# error unsupported compiler in config-aix-4.x.h
#endif /* ! __xlC__ && ! __GNUG__ */


// Compiling for AIX.
#ifndef AIX
#  define AIX
#endif /* AIX */

// AIX shared libs look strangely like archive libs until you look inside
// them.
#if defined (PDL_DLL_SUFFIX)
#  undef PDL_DLL_SUFFIX
#endif
#define PDL_DLL_SUFFIX ".so"

// Use BSD 4.4 socket definitions for pre-AIX 4.2.  The _BSD setting also
// controls the data type used for waitpid(), wait(), and wait3().
#if (PDL_AIX_VERS < 402)
#  define _BSD 44
#  define PDL_HAS_UNION_WAIT
#endif /* PDL_AIX_VERS < 402 */

// This environment requires this thing, pre-AIX 4.3
#if (PDL_AIX_VERS < 403)
#  define _BSD_INCLUDES
#endif /* PDL_AIX_VERS < 403 */

#define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)

#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R

// PDL_HAS_AIX_BROKEN_SOCKET_HEADER may be required if your OS patches are
// not up to date.  On up-to-date 4.2+ systems, it should not be required.
// 4.2+ has 4.4 BSD sendmsg/recvmsg
#if (PDL_AIX_VERS < 402)
#  define PDL_HAS_AIX_BROKEN_SOCKET_HEADER
#else
#  if (PDL_AIX_VERS == 402)
#    define PDL_HAS_SIZET_SOCKET_LEN
#  else
#    define PDL_HAS_SOCKLEN_T   
#  endif
#  define PDL_HAS_4_4BSD_SENDMSG_RECVMSG
#endif /* PDL_AIX_VERS < 402 */

#if (PDL_AIX_VERS >= 403)
// AIX has AIO, but the functions don't match those of other AIO-enabled
// platforms. If this is to work, it'll require some significant work,
// maybe moving the OS-abstraction stuff to an OS_AIO or some such thing.
//#  define PDL_HAS_AIO_CALLS
#endif /* PDL_AIX_VERS >= 403 */

#define PDL_HAS_AIX_HI_RES_TIMER
#define PDL_HAS_ALLOCA

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS
#if (PDL_AIX_VERS < 403)
#  define PDL_HAS_CHARPTR_DL
#endif /* PDL_AIX_VERS < 403 */

// Prototypes for both signal() and struct sigaction are consistent.
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// OS has readdir and friends.
#define PDL_HAS_DIRENT

// OS supports the getrusage() system call
#define PDL_HAS_GETRUSAGE

#define PDL_HAS_GPERF

#define PDL_HAS_H_ERRNO

#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT
#define PDL_HAS_NONCONST_SELECT_TIMEVAL
#define PDL_HAS_IP_MULTICAST
#define PDL_HAS_MSG
#if (PDL_AIX_VERS < 402)
#  define PDL_LACKS_MSG_ACCRIGHTS
#  define PDL_LACKS_RLIMIT
#endif

// Compiler/platform supports poll().
#define PDL_HAS_POLL

// Platform has terminal ioctl flags like TCGETS and TCSETS.
#define PDL_HAS_TERM_IOCTLS

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

#define PDL_HAS_POSIX_TIME
// ... but needs to include another header for it on 4.2+
#if (PDL_AIX_VERS >= 402)
#  define PDL_HAS_BROKEN_POSIX_TIME
#endif /* PDL_AIX_VERS > 402 */
// ... and needs another typedef
#define PDL_LACKS_TIMESPEC_T
#define PDL_HAS_SELECT_H

#define PDL_HAS_REENTRANT_FUNCTIONS

// Compiler/platform defines the sig_atomic_t typedef
#define PDL_HAS_SIG_ATOMIC_T
#if (PDL_AIX_VERS >= 402)
#  define PDL_HAS_SIGINFO_T
#  define PDL_LACKS_SIGINFO_H
#endif /* PDL_AIX_VERS >= 402 */
#if (PDL_AIX_VERS >= 403)
// it may exist in earlier revs, but I'm not sure and I don't have a
// system to validate
#  define PDL_HAS_P_READ_WRITE
#endif /* PDL_AIX_VERS >= 403 */

#define PDL_HAS_SIGWAIT
#define PDL_HAS_SIN_LEN
#define PDL_HAS_STRBUF_T

// Compiler supports stropts.h
#define PDL_HAS_STREAMS
// #define PDL_HAS_STREAM_PIPES

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// AIX bzero()
#define PDL_HAS_STRINGS

#define PDL_HAS_STRUCT_NETDB_DATA

// Dynamic linking is in good shape on newer OS/patch levels.  If you have
// trouble with the dynamic linking parts of PDL, and can't patch your OS
// up to latest levels, comment this out.
#define PDL_HAS_SVR4_DYNAMIC_LINKING
// This is tightly related to dynamic linking...
#define PDL_HAS_AUTOMATIC_INIT_FINI

#define PDL_HAS_SVR4_GETTIMEOFDAY

#define PDL_HAS_SYSV_IPC
#define PDL_HAS_TIMOD_H
#define PDL_HAS_TIUSER_H
//#define PDL_HAS_TLI //- by jdlee
#define PDL_HAS_BROKEN_T_ERROR
//#define PDL_HAS_TLI_PROTOTYPES //- by jdlee
#define PDL_TLI_TCP_DEVICE  "/dev/xti/tcp"

#define PDL_HAS_UALARM

#if (PDL_AIX_VERS >= 402)
#  define PDL_HAS_UCONTEXT_T
#endif /* PDL_AIX_VERS >= 402 */

#define PDL_HAS_UTIME

// Platform has XPG4 wide character type and functions
#define PDL_HAS_XPG4_MULTIBYTE_CHAR

#define PDL_LACKS_TCP_H

// AIX uses LIBPATH to search for libraries
#define PDL_LD_SEARCH_PATH "LIBPATH"

#define PDL_NEEDS_DEV_IO_CONVERSION

// Defines the page size of the system.
#define PDL_PAGE_SIZE 4096

//**************************************************************
//
// Threads related definitions.
//
// The threads on AIX are generally POSIX P1003.1c (PDL_HAS_PTHREADS).
// However, there is also a kernel thread ID (tid_t) that is used in
// PDL_Log_Msg (printing the thread ID).  The tid_t is not the same as
// pthread_t, and can't derive one from the other - thread_self() gets
// the tid_t (kernel thread ID) if called from a thread.
// Thanks very much to Chris Lahey for straightening this out.

// Unless threads are specifically turned off, build with them enabled.
#if !defined (PDL_HAS_THREADS)
#  define PDL_HAS_THREADS 1
#endif

#if (PDL_HAS_THREADS != 0)
#  if !defined (PDL_MT_SAFE)
#    define PDL_MT_SAFE 1
#  endif

#  define PDL_HAS_PTHREADS
// 4.3 and up has 1003.1c standard; 4.2 has draft 7
#  if (PDL_AIX_VERS >= 403)
#    define PDL_HAS_PTHREADS_STD
#    define PDL_HAS_PTHREADS_UNIX98_EXT
#  else
#    define PDL_HAS_PTHREADS_DRAFT7
#    define PDL_LACKS_RWLOCK_T
#    define PDL_LACKS_THREAD_STACK_ADDR
// If PDL doesn't compile due to the lack of these methods, please
// send email to pdl-users@cs.wustl.edu reporting this.
// #define PDL_LACKS_CONDATTR_PSHARED
// #define PDL_LACKS_MUTEXATTR_PSHARED
#    define PDL_LACKS_SETSCHED
#  endif /* PDL_AIX_VERS >= 403 */

#  define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS
#  define PDL_HAS_SIGTHREADMASK
#  define PDL_HAS_THREAD_SPECIFIC_STORAGE

#  define PDL_LACKS_THREAD_PROCESS_SCOPING
#else
#  undef PDL_HAS_THREADS
#endif /* PDL_HAS_THREADS != 0 */

#define PDL_MALLOC_ALIGN 8

// By default, tracing code is not compiled.  To compile it in, cause
// PDL_NTRACE to not be defined, and rebuild PDL.
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

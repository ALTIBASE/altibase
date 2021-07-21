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
// config-hpux-11.00.h,v 1.2 2000/02/18 23:34:10 shuston Exp

// The following configuration file is designed to work for HP
// platforms running HP-UX 11.00 using aC++, CC, or gcc (2.95 and up).

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

#if defined (__GNUG__)

// config-g++-common.h undef's PDL_HAS_STRING_CLASS with -frepo, so
// this must appear before its #include.
#  define PDL_HAS_STRING_CLASS

#  include "config-g++-common.h"

// gcc 2.95.2 supplies the ssize_t typedef.
#  define PDL_HAS_SSIZE_T

// To porting TASK-1793 HP-11.11 GCC 4.0.2 porting 
#if defined (HP_GCC_402) 
#  define PDL_HAS_SOCKLEN_T 1 
#endif /* HP_GCC_402 */ 

#  include "config-hpux11.h"

// KCC Specific Section
#elif defined(__KCC)

#  include "config-kcc-common.h"

#else

// The following configuration section is designed to work for HP
// platforms running HP/UX 11.x with either of the HP C++ compilers.
// There isn't a predefined macro for all cases of the compilers that
// can be used to tell them apart from other compilers (e.g. __KCC, etc.)
// only to tell C++ from aC++, using the value of __cplusplus.
//
// NOTE - HP advises people on 11.x to use aC++ since the older C++ doesn't
// support 64-bit or kernel threads.  So, though this file has the C++ info
// in it, it's copied from the 10.x file and hasn't been verified.

// There are 2 compiler-specific sections, plus a 3rd for common to both.
// First is the HP C++ section...
#  if __cplusplus < 199707L

#    define PDL_HAS_BROKEN_HPUX_TEMPLATES

// Compiler can't handle calls like foo->operator T *()
#    define PDL_HAS_BROKEN_CONVERSIONS

// Necessary with some compilers to pass PDL_TTY_IO as parameter to
// DEV_Connector.
#    define PDL_NEEDS_DEV_IO_CONVERSION

// Compiler's template mechanism must see source code (i.e., .C files).
#    define PDL_TEMPLATES_REQUIRE_SOURCE

// Compiler's template mechanism requires the use of explicit C++
// specializations for all used templates.
#    define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION

// The HP/UX compiler doesn't support volatile!!!!
#    define volatile

#else  // aC++ definitions
 
// Parts of TAO (at least) use __HP_aCC to detect this compiler, but the
// macro is not set until A.03.13. If it's not set, set it - it won't be an
// HP-advertised value, but we don't check the value/version - just whether
// it's set or not.
#    if !defined (__HP_aCC)
#      define __HP_aCC
#    endif /* __HP_aCC */

// Precompiler needs extra flags to ignore "invalid #pragma directive"
#    define PDL_CC_PREPROCESSOR_ARGS "-E +W 67"

// Compiler supports ANSI casts
#    define PDL_HAS_ANSI_CASTS

// Compiler can't handle calls like foo->operator T *()
#    define PDL_HAS_BROKEN_CONVERSIONS

// Compiler supports C++ exception handling
#    define PDL_HAS_EXCEPTIONS 1

// Compiler enforces the "One Definition Rule"
#    define PDL_HAS_ONE_DEFINITION_RULE

#    define PDL_HAS_TYPENAME_KEYWORD

// Compiler implements templates that support typedefs inside of classes
// used as formal arguments to a template class.
#    define PDL_HAS_TEMPLATE_TYPEDEFS

// This is legit for A.03.05 - not sure A.03.04, but it should be.
#    define PDL_HAS_USING_KEYWORD

// Platform lacks streambuf "linebuffered ()".
#    define PDL_LACKS_LINEBUFFERED_STREAMBUF 1

// Lack of (and broken) support for placement operator delete is a known
// bug by HP, at least as of aC++ A.03.10. It may be fixed later, and if so
// this would change to be a #if against an appropriate value of __HP_aCC
#    define PDL_LACKS_PLACEMENT_OPERATOR_DELETE

// Compiler's 'new' throws exceptions on failure.
/* #    define PDL_NEW_THROWS_EXCEPTIONS */

// Compiler's template mechanism must see source code (i.e., .C files).
#    define PDL_TEMPLATES_REQUIRE_SOURCE

// Compiler supports template specialization.
#    define PDL_HAS_TEMPLATE_SPECIALIZATION
// ... and uses the template<> syntax
#    define PDL_HAS_STD_TEMPLATE_SPECIALIZATION
#    define PDL_HAS_STD_TEMPLATE_METHOD_SPECIALIZATION

// Preprocessor needs some help with data types
#    if defined (__LP64__)
#      define PDL_SIZEOF_LONG 8
#    else
#      define PDL_SIZEOF_LONG 4
#    endif

#  endif /* __cplusplus < 199707L */

// Compiler supports the ssize_t typedef.
#  define PDL_HAS_SSIZE_T

// Compiler doesn't handle 'signed char' correctly (used in IOStream.h)
#  define PDL_LACKS_SIGNED_CHAR

#endif /* __GNUG__, __KCC, HP */

//*********************************************************************
//
// From here down is the compiler-INdependent OS settings.
//
//*********************************************************************

// Compiling for HPUX.
#if !defined (HPUX)
#define HPUX
#endif /* HPUX */
#define HPUX_11

#ifndef _HPUX_SOURCE
#define _HPUX_SOURCE
#endif

#include /**/ <sys/stdsyms.h>

////////////////////////////////////////////////////////////////////////////
//
// General OS information - see README for more details on what they mean
//
///////////////////////////////////////////////////////////////////////////

// HP/UX needs to have these addresses in a special range.
// If this is on a 64-bit model, the default is to use 64-bit addressing.
// It can also be set so that the mapped region is shareable with 32-bit
// programs.  To enable the 32/64 sharing, comment out the first definition
// of PDL_DEFAULT_BASE_ADDR and uncomment the two lines after it.
// Note - there's a compiler bug on aC++ A.03.04 in 64-bit mode which prevents
// these from working as-is.  So, there's some hackery in Naming_Context.cpp
// and Memory_Pool.cpp which works around it.  It requires the
// PDL_DEFAULT_BASE_ADDRL definition below - make sure it has the same
// value as what you use for PDL_DEFAULT_BASE_ADDR.  This is allegedly fixed
// in A.03.10 on the June Applications CD.
#if defined (__LP64__)
#  define PDL_DEFAULT_BASE_ADDR ((char *) 0x0000001100000000)
//#  define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)
//#  define PDL_OS_EXTRA_MMAP_FLAGS MAP_ADDR32

#  define PDL_DEFAULT_BASE_ADDRL (0x0000001100000000)
//#  define PDL_DEFAULT_BASE_ADDRL (0x80000000)
#else
#  define PDL_DEFAULT_BASE_ADDR ((char *) 0x80000000)
#endif  /* __LP64__ */

// Platform can do async I/O (aio_*)
#define PDL_HAS_AIO_CALLS
// ... but seems to require this in order to keep from hanging.  Needs some
// investigation, maybe with HP.  John Mulhern determined this value
// empirically.  YMMV.  If it does vary, set it up in your own config.h which
// then includes the PDL-supplied config.
#if !defined (PDL_INFINITE)
#  define PDL_INFINITE 10000000
#endif

// Compiler/platform contains the <sys/syscall.h> file.
#define PDL_HAS_SYSCALL_H
// But doesn't have a prototype for syscall()
#define PDL_LACKS_SYSCALL

// Platform supports POSIX 1.b clock_gettime ()
#define PDL_HAS_CLOCK_GETTIME

// Prototypes for both signal() and struct sigaction are consistent.
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES

// Compiler/platform has correctly prototyped header files.
#define PDL_HAS_CPLUSPLUS_HEADERS

// Compiler/platform has Dirent iterator functions.
#define PDL_HAS_DIRENT

// Platform supports getpagesize() call
#define PDL_HAS_GETPAGESIZE
// But we define this just to be safe
#define PDL_PAGE_SIZE 4096

// Can run gperf on this platform (needed for TAO)
#  define PDL_HAS_GPERF

// Optimize PDL_Handle_Set for select().
#  define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// Platform supports IP multicast
#define PDL_HAS_IP_MULTICAST

// Platform supports recvmsg and sendmsg.
#define PDL_HAS_MSG

// Platform's select() has non-const timeval argument
#define PDL_HAS_NONCONST_SELECT_TIMEVAL

// Compiler/platform supports poll().
#define PDL_HAS_POLL

// Platform supports POSIX O_NONBLOCK semantics.
#define PDL_HAS_POSIX_NONBLOCK

// Platform supports the POSIX struct timespec type
#define PDL_HAS_POSIX_TIME

// Platform supports reentrant functions (all the POSIX *_r functions).
#define PDL_HAS_REENTRANT_FUNCTIONS

// HP-UX 11 has reentrant netdb functions.  The catch is that the old
// functions (gethostbyname, etc.) are thread-safe and the _r versions are
// not used and will be removed at some point.  So, define things so
// the _r versions are not used.  This will slow things down a bit due to
// the extra mutex lock in the PDL_NETDBCALL_RETURN macro, and will be fixed
// in the future (problem ID P64).
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS

// Compiler/platform defines the sig_atomic_t typedef
#define PDL_HAS_SIG_ATOMIC_T

// Platform supports SVR4 extended signals
#define PDL_HAS_SIGINFO_T

// Platform doesn't detect a signal out of range unless it's way out of range.
#define PDL_HAS_SIGISMEMBER_BUG

#define PDL_HAS_UALARM

// Platform supports ucontext_t (which is used in the extended signal API).
#define PDL_HAS_UCONTEXT_T

// Compiler/platform supports strerror ().
#define PDL_HAS_STRERROR

// SunOS 4 style prototype for gettimeofday
#define PDL_HAS_SUNOS4_GETTIMEOFDAY

// Platform supports SVR4 dynamic linking semantics, in 64-bit mode only.
// When used, this requires -ldl on the PDL library link line.
//#ifdef __LP64__
#define PDL_HAS_SVR4_DYNAMIC_LINKING
//#endif

// HP/UX has an undefined syscall for GETRUSAGE...
#define PDL_HAS_SYSCALL_GETRUSAGE
// Note, this only works if the flag is set above!
#define PDL_HAS_GETRUSAGE

// Platform has the sigwait function in a header file
#define PDL_HAS_SIGWAIT

// Platform supports System V IPC (most versions of UNIX, but not Win32)
#define PDL_HAS_SYSV_IPC

// accept() is thread-safe
#define PDL_HAS_THREAD_SAFE_ACCEPT

// Platform has XPG4 wide character support
#define PDL_HAS_XPG4_MULTIBYTE_CHAR

// Platform lacks a typedef for timespec_t, but has struct timespec
#define PDL_LACKS_TIMESPEC_T

// Platform supports IPv6
#define PDL_HAS_IP6

// Platform needs a timer skew value.  It *may* vary by processor, but this
// one works.  You can override it in your config.h file if needed.
// It's in units of microseconds.  This value is 10 msec.
#if !defined (PDL_TIMER_SKEW)
#  define PDL_TIMER_SKEW 10 * 1000
#endif /* PDL_TIMER_SKEW */

// Shared library name/path components
#define PDL_DLL_SUFFIX  ".sl"
#if defined (__LP64__)
#  define PDL_LD_SEARCH_PATH "LD_LIBRARY_PATH"
#else
#  define PDL_LD_SEARCH_PATH "SHLIB_PATH"
#endif  /* __LP64__ */

//////////////////////////////////////////////////////////////////////////
//
// STREAMS information
//
//////////////////////////////////////////////////////////////////////////

// Platform supports STREAMS
#define PDL_HAS_STREAMS
// Compiler/platform supports struct strbuf.
#define PDL_HAS_STRBUF_T
// But the putmsg signature doesn't have it as const...
// Well, it really does, but it depends on preprocessor defines.
#define PDL_LACKS_CONST_STRBUF_PTR

// Platform supports STREAM pipes
// This is possible, but not by default - need to rebuild the kernel to
// get them enabled - see pipe(2) and "STREAMS/UX for the HP 9000"
// #define PDL_HAS_STREAM_PIPES

/////////////////////////////////////////////////////////////////////////
//
// TLI/XTI information
//
////////////////////////////////////////////////////////////////////////

// Platform supports XTI (includes TLI), including SVR4 facilities.
//#define PDL_HAS_TLI
// PDL_HAS_SVR4_TLI should work on HP-UX, but doesn't yet.  Riverace
// problem ID P27.
//#define PDL_HAS_SVR4_TLI
// Platform uses <xti.h>, not tiuser.h
#define PDL_HAS_XTI
// But it has _terrno() outside the extern "C" stuff.
#define PDL_HAS_TIUSER_H_BROKEN_EXTERN_C
// Platform provides PDL_TLI function prototypes.
//#define PDL_HAS_TLI_PROTOTYPES
// HP-UX 11.00 (at least at initial releases) has some busted macro defs
#define PDL_HAS_BROKEN_XTI_MACROS
// HP-UX 11 conforms to the XPG4 spec, which PDL calls broken for the
// errmsg not being const...
#define PDL_HAS_BROKEN_T_ERROR

/////////////////////////////////////////////////////////////////////////
//
// Threads information.
//
// Use of threads is controlled by the 'threads' argument to make.  See
// makeinclude/platform_hpux_aCC.GNU for details. If it's not set,
// the default is to enable it, since kernel threads are always available
// on HP-UX 11, as opposed to 10.x where it was optional software.
//
////////////////////////////////////////////////////////////////////////

#if !defined (PDL_HAS_THREADS)
#  define PDL_HAS_THREADS
#endif /* PDL_HAS_THREADS */

#if defined (PDL_HAS_THREADS)

#  if !defined (PDL_MT_SAFE)
#    define PDL_MT_SAFE 1
#  endif

#  define PDL_HAS_PTHREADS
#  define PDL_HAS_PTHREADS_STD
#  define PDL_HAS_PTHREADS_UNIX98_EXT

#  define PDL_HAS_THREAD_SPECIFIC_STORAGE

// 2015/01/06 hcs
// Lacks support for pthread_attr_setstack()
#  define PDL_LACKS_THREAD_SETSTACK
#endif /* PDL_HAS_THREADS */

#define PDL_HAS_POSIX_SEM

#define PDL_LACKS_PREADV

// Turns off the tracing feature.
// To build with tracing enabled, make sure PDL_NTRACE is not defined
#if !defined (PDL_NTRACE)
#define PDL_NTRACE 1
#endif /* PDL_NTRACE */

#endif /* PDL_CONFIG_H */

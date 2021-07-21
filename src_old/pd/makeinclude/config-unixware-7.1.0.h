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
// config-unixware-7.1.0.h,v 4.1 2000/06/06 18:04:24 oci Exp

#ifndef PDL_CONFIG_H
#define PDL_CONFIG_H

/* PDL configuration header file */

/* Include the commong gnu config file */
#include "config-g++-common.h"

/* For unixware 7.1 && g++ 2.91.57, see if this fixes my problem */
#ifndef UNIXWARE_7_1
#define UNIXWARE_7_1
#endif

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA 1

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if lex declares yytext as a char * by default, not a char[].  */
#define YYTEXT_POINTER 1

/* Define _REENTRANT if reentrant functions should be used. */
#ifndef _REENTRANT
# define _REENTRANT 1
#endif

#define PDL_HAS_NEW_NO_H 1
#define PDL_HAS_STDEXCEPT_NO_H 1
#define PDL_HAS_IOMANIP_NO_H 1

/* Platform provides <sys/ioctl.h> header */
#define PDL_HAS_SYS_IOCTL_H 1

#define PDL_THREAD_MIN_PRIORITY 0
#if defined (PDL_THREAD_MIN_PRIORITY)
# define PTHREAD_MIN_PRIORITY PDL_THREAD_MIN_PRIORITY
#endif  /* #if defined (PDL_THREAD_MIN_PRIORITY) */

#define PDL_THREAD_MAX_PRIORITY 99
#if defined (PDL_THREAD_MAX_PRIORITY)
# define PTHREAD_MAX_PRIORITY PDL_THREAD_MAX_PRIORITY
#endif  /* #if defined (PDL_THREAD_MAX_PRIORITY) */

/* Specify sizes of given built-in types.  If a size isn't defined here,
   then Basic_Types.h will attempt to deduce the size. */
/* #undef PDL_SIZEOF_CHAR */
#define PDL_SIZEOF_SHORT 2
#define PDL_SIZEOF_INT 4
#define PDL_SIZEOF_LONG 4
#define PDL_SIZEOF_LONG_LONG 8
#define PDL_SIZEOF_VOID_P 4
#define PDL_SIZEOF_FLOAT 4
#define PDL_SIZEOF_DOUBLE 8
#define PDL_SIZEOF_LONG_DOUBLE 12

/* typedef for PDL_UINT64 */
/*
   We only make the typedef if PDL_UINT64_TYPEDEF is defined.  Otherwise,
   let Basic_Types.h do the work for us.
*/
#define PDL_UINT64_TYPEDEF unsigned long long
#if defined(PDL_UINT64_TYPEDEF)
   typedef PDL_UINT64_TYPEDEF PDL_UINT64;
#endif /* PDL_UINT64_TYPEDEF && !PDL_DISABLE_AUTOCONF_UINT64 */

/* Enable PDL inlining */
#define __PDL_INLINE__ 1

/* OS has priocntl (2) */
#define PDL_HAS_PRIOCNTL 1

/* Platform has pread() and pwrite() support */
#define PDL_HAS_P_READ_WRITE 1

/* Compiler/platform supports alloca() */
#define PDL_HAS_ALLOCA 1

/* Compiler/platform correctly calls init()/fini() for shared libraries */
#define PDL_HAS_AUTOMATIC_INIT_FINI 1

/* Platform doesn't cast MAP_FAILED to a (void *). */
/* #undef PDL_HAS_BROKEN_MAP_FAILED */
/* Staller: oh yes, let's do this! */
#define PDL_HAS_BROKEN_MAP_FAILED

/* Prototypes for both signal() and struct sigaction are consistent. */
#define PDL_HAS_CONSISTENT_SIGNAL_PROTOTYPES 1

/* Platform supports operations on directories via struct dirent,
   readdir_r, etc. */
#define PDL_HAS_DIRENT 1

/* Compiler supports C++ exception handling */
// MM-Graz if ! defined inserted, to prevent warnings, because it is already
//  defined in config-g++common.h
# if !defined (PDL_HAS_EXCEPTIONS)
#define PDL_HAS_EXCEPTIONS 1
# endif

/* Platform supports getpagesize() call (otherwise, PDL_PAGE_SIZE must be
   defined, except on Win32) */
#define PDL_HAS_GETPAGESIZE 1

/* Platform supports the getrusage() system call. */
#define PDL_HAS_GETRUSAGE 1

/* Platform has a getrusage () prototype in sys/resource.h that differs from
   the one in OS.i. */
#define PDL_HAS_GETRUSAGE_PROTO 1

/* Denotes that GNU has cstring.h as standard which redefines memchr() */
#define PDL_HAS_GNU_CSTRING_H

/* The GPERF utility is compiled for this platform */
#define PDL_HAS_GPERF 1

/* Optimize PDL_Handle_Set::count_bits for select() operations (common case) */
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT 1

/* Compiler/platform supports idtype_t. */
#define PDL_HAS_IDTYPE_T 1

/* Platform supports IP multicast */
#define PDL_HAS_IP_MULTICAST 1

/* Platform supports thr_keydelete (e.g,. UNIXWARE) */
#define PDL_HAS_THR_KEYDELETE 1

/* Platform calls thr_minstack() rather than thr_min_stack() (e.g., Tandem). */
#define PDL_HAS_THR_MINSTACK 1

/* Platform supports recvmsg and sendmsg */
#define PDL_HAS_MSG 1

/* Platform's select() uses non-const timeval* (only found on Linux right
   now) */
#define PDL_HAS_NONCONST_SELECT_TIMEVAL 1

/* Uses ctime_r & asctime_r with only two parameters vs. three. */
#define PDL_HAS_2_PARAM_ASCTIME_R_AND_CTIME_R 1

/* Platform is an Intel Pentium microprocessor. */
/* There is a problem with the gethrtime() because of (apparently) a problem
   with the inline assembly instruction.  Hopefully there is a way to resolve
   that with an improvement to the assembler
*/
#ifdef PDL_HAS_PENTIUM
#undef PDL_HAS_PENTIUM
#endif /* PDL_HAS_PENTIUM */


/* Platform contains <poll.h> */
#define PDL_HAS_POLL 1

/* Platform supports POSIX O_NONBLOCK semantics */
#define PDL_HAS_POSIX_NONBLOCK 1

/* Platform supports the POSIX struct timespec type */
#define PDL_HAS_POSIX_TIME 1

/* Platform supports the /proc file system and defines tid_t
   in <sys/procfs.h> */
#define PDL_HAS_PROC_FS 1

/* Platform supports POSIX Threads */
#define PDL_HAS_PTHREADS 1

/* Platform supports POSIX.1c-1995 threads */
#define PDL_HAS_PTHREADS_STD 1

/* pthread.h declares an enum with PTHREAD_PROCESS_PRIVATE and
   PTHREAD_PROCESS_SHARED values */
#define PDL_HAS_PTHREAD_PROCESS_ENUM 1

/* Platform has pthread_sigmask() defined. */
#define PDL_HAS_PTHREAD_SIGMASK 1

/* Platform will recurse infinitely on thread exits from TSS cleanup routines
   (e.g., AIX) */
#define PDL_HAS_RECURSIVE_THR_EXIT_SEMANTICS 1

/* Platform supports reentrant functions (i.e., all the POSIX *_r
   functions). */
#define PDL_HAS_REENTRANT_FUNCTIONS 1

/* Platform has support for multi-byte character support compliant with the
   XPG4 Worldwide Portability Interface wide-character classification. */
#define PDL_HAS_XPG4_MULTIBYTE_CHAR 1

/* Platform does not support reentrant netdb functions (getprotobyname_r,
   getprotobynumber_r, gethostbyaddr_r, gethostbyname_r, getservbyname_r). */
#define PDL_LACKS_NETDB_REENTRANT_FUNCTIONS 1

/* Platform supports the POSIX regular expression library */
#define PDL_HAS_REGEX 1

/* Platform has special header for select(). */
#define PDL_HAS_SELECT_H 1

/* Platform has a function to set t_errno (e.g., Tandem). */
#define PDL_HAS_SET_T_ERRNO 1

/* Platform supports SVR4 extended signals */
#define PDL_HAS_SIGINFO_T 1

/* Platform/compiler has the sigwait(2) prototype */
#define PDL_HAS_SIGWAIT 1

/* Compiler/platform defines the sig_atomic_t typedef */
#define PDL_HAS_SIG_ATOMIC_T 1

/* Platform supports new BSD inet_addr len field. */
#define PDL_HAS_SIN_LEN 1

/* OS/compiler uses size_t * rather than int * for socket lengths */
#define PDL_HAS_SIZET_SOCKET_LEN 1

/* Compiler/platform provides the sys/sockio.h file */
#define PDL_HAS_SOCKIO_H 1

/* Compiler supports the ssize_t typedef */
#define PDL_HAS_SSIZE_T 1

/* Platform supports UNIX International Threads */
#define PDL_HAS_STHREADS 1

/* Platform has thr_yield() */
#define PDL_HAS_THR_YIELD 1

/* Compiler/platform supports struct strbuf */
#define PDL_HAS_STRBUF_T 1

/* Platform supports STREAMS */
#define PDL_HAS_STREAMS 1

/* Platform supports STREAM pipes */
#define PDL_HAS_STREAM_PIPES 1

/* Compiler/platform supports strerror () */
#define PDL_HAS_STRERROR 1

/* Platform/Compiler supports a String class (e.g., GNU or Win32). */
#define PDL_HAS_STRING_CLASS 1

/* Platform has <strings.h> (which contains bzero() prototype) */
#define PDL_HAS_STRINGS 1

/* Platform has void * as second parameter to gettimeofday and a has a
   prototype */
#define PDL_HAS_SUNOS4_GETTIMEOFDAY 1

/* Compiler/platform supports SVR4 dynamic linking semantics */
#define PDL_HAS_SVR4_DYNAMIC_LINKING 1

/* Compiler/platform supports SVR4 TLI (in particular, T_GETNAME stuff)... */
#define PDL_HAS_SVR4_TLI 1

/* Compiler/platform contains the <sys/syscall.h> file. */
#define PDL_HAS_SYSCALL_H 1

/* Platform supports system configuration information */
#define PDL_HAS_SYSINFO 1

/* Platform supports System V IPC (most versions of UNIX, but not Win32) */
#define PDL_HAS_SYSV_IPC 1

/* Platform provides <sys/filio.h> header */
#define PDL_HAS_SYS_FILIO_H 1

/* Platform provides <sys/xti.h> header */
#define PDL_HAS_SYS_XTI_H 1

/* Platform has terminal ioctl flags like TCGETS and TCSETS. */
#define PDL_HAS_TERM_IOCTLS 1

/* Platform supports threads */
#define PDL_HAS_THREADS 1

/* Compiler/platform has thread-specific storage */
#define PDL_HAS_THREAD_SPECIFIC_STORAGE 1

/* Platform supports TLI timod STREAMS module */
#define PDL_HAS_TIMOD_H 1

/* Platform supports TLI tiuser header */
#define PDL_HAS_TIUSER_H 1

/* Platform supports TLI.  Also see PDL_TLI_TCP_DEVICE. */
#define PDL_HAS_TLI 1

/* Platform provides TLI function prototypes */
#define PDL_HAS_TLI_PROTOTYPES 1

/* Platform supports ualarm() */
#define PDL_HAS_UALARM 1

/* Platform supports ucontext_t (which is used in the extended signal API). */
#define PDL_HAS_UCONTEXT_T 1

/* Platform has <utime.h> header file */
#define PDL_HAS_UTIME 1

/* Platform requires void * for mmap(). */
#define PDL_HAS_VOIDPTR_MMAP 1

/* Platform has XTI (X/Open-standardized superset of TLI).  Implies
   PDL_HAS_TLI but uses a different header file. */
#define PDL_HAS_XTI 1

/* Platform can not build IOStream{,_T}.cpp.  This does not necessarily
   mean that the platform does not support iostreams. */
#define PDL_LACKS_PDL_IOSTREAM 1

/* Platform does not have u_longlong_t typedef */
#define PDL_LACKS_U_LONGLONG_T 1

/* Platform lacks madvise() (e.g., Linux) */
#define PDL_LACKS_MADVISE 1

/* Platform lacks POSIX prototypes for certain System V functions like shared
   memory and message queues. */
#define PDL_LACKS_SOME_POSIX_PROTOTYPES 1

/* Platform lacks pri_t (e.g., Tandem NonStop UNIX). */
#define PDL_LACKS_PRI_T 1

/* Platform lacks pthread_thr_sigsetmask (e.g., MVS, HP/UX, and OSF/1 3.2) */
#define PDL_LACKS_PTHREAD_THR_SIGSETMASK 1

/* Platfrom lack pthread_yield() support. */
#define PDL_LACKS_PTHREAD_YIELD 1

/* Platform lacks readers/writer locks. */
#define PDL_LACKS_RWLOCK_T 1

/* MIT pthreads platform lacks the timedwait prototypes */
#define PDL_LACKS_TIMEDWAIT_PROTOTYPES 1

/* Platform does not define timepec_t as a typedef for struct timespec. */
#define PDL_LACKS_TIMESPEC_T 1

/* Compile using multi-thread libraries */
#define PDL_MT_SAFE 1

/* Platform needs regexpr.h for regular expression support */
#define PDL_NEEDS_REGEXPR_H 1

/* Platform needs to #include <sched.h> to get thread scheduling defs. */
#define PDL_NEEDS_SCHED_H 1

/* <time.h> doesn't automatically #include <sys/time.h> */
#define PDL_LACKS_SYSTIME_H 1

/* Turns off the tracing feature. */
#define PDL_NTRACE 1

/*********************************************************************/
/* Compiler's template mechanim must see source code (i.e., .cpp files).  This
   is used for GNU G++. */
/* Staller -> make 0 */
// #undef PDL_TEMPLATES_REQUIRE_SOURCE

/* Compiler's template instantiation mechanism supports the use of explicit
   C++ specializations for all used templates. This is also used for GNU G++
   if you don't use the "repo" patches. */
/* Staller -> make 0 */
// #define PDL_HAS_EXPLICIT_TEMPLATE_INSTANTIATION 1
/*********************************************************************/

/* The OS/platform supports the poll() event demultiplexor */
#define PDL_USE_POLL 1

/* Platform has its standard c++ library in the namespace std. */
#define PDL_USES_STD_NAMESPACE_FOR_STDCPP_LIB 1

/* The number of bytes in a double.  */
#define SIZEOF_DOUBLE 8

/* The number of bytes in a float.  */
#define SIZEOF_FLOAT 4

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long.  */
#define SIZEOF_LONG 4

/* The number of bytes in a long double.  */
#define SIZEOF_LONG_DOUBLE 12

/* The number of bytes in a long long.  */
#define SIZEOF_LONG_LONG 8

/* The number of bytes in a short.  */
#define SIZEOF_SHORT 2

/* The number of bytes in a signed char.  */
#define SIZEOF_SIGNED_CHAR 1

/* The number of bytes in a void *.  */
#define SIZEOF_VOID_P 4

/* Define if you have the execv function.  */
#define HAVE_EXECV 1

/* Define if you have the execve function.  */
#define HAVE_EXECVE 1

/* Define if you have the execvp function.  */
#define HAVE_EXECVP 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <fstream> header file.  */
#define HAVE_FSTREAM 1

/* Define if you have the <iomanip> header file.  */
#define HAVE_IOMANIP 1

/* Define if you have the <iostream> header file.  */
#define HAVE_IOSTREAM 1

/* Define if you have the <pwd.h> header file.  */
#define HAVE_PWD_H 1

/* Name of package */
#define PACKAGE "pdl"

/* Added by Staller */
#define ENUM_BOOLEAN // See file /usr/local/lib/gcc-lib/i486-pc-sysv5/egcs-2.91.60/include/sys/types.h
#define howmany(x, y)   (((x)+((y)-1))/(y))
#define PDL_HAS_BROKEN_T_ERROR // make a nasty warning disappear in OS.i
#define __USLC__ 1
#define  __IOCTL_VERSIONED__ // By Carlo!

#endif /* PDL_CONFIG_H */

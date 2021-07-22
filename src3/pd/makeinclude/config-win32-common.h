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
// config-win32-common.h,v 4.121 2000/02/12 21:17:54 nanbor Exp

// The following configuration file is designed to work for Windows 95,
// Windows NT 3.51 and Windows NT 4.0 platforms using the Microsoft Visual C++
// compilers 2.0, 4.0, 4.1, 4.2, 5.0 and 6.0

#ifndef PDL_WIN32_COMMON_H
#define PDL_WIN32_COMMON_H

// Complain if WIN32 is not already defined.
#if !defined (WIN32) && !defined (PDL_HAS_WINCE)
#define WIN32
//# error Please define WIN32 in your project settings.
#endif

#define PDL_WIN32
#if defined (_WIN64) || defined (WIN64)
#  define PDL_WIN64
#endif /* _WIN64 || WIN64 */

// Define this if you're running NT 4.x
//  Setting applies to  : building PDL
//  Runtime restrictions: System must be Windows NT => 4.0
//  Additonal notes: Defining _WIN32_WINNT as 0x0400 implies PDL_HAS_WINSOCK2
//  unless you set PDL_HAS_WINSOCK2 to 0 in the config.h file.
#if !defined (PDL_HAS_WINNT4)
# define PDL_HAS_WINNT4 1      // assuming Win NT 4.0 or greater
#endif

#if (defined (PDL_HAS_WINNT4) && PDL_HAS_WINNT4 != 0)
# if !defined (_WIN32_WINNT)
#  define _WIN32_WINNT 0x0400
# endif
#endif

// Define PDL_HAS_MFC to 1, if you want PDL to use CWinThread. This should
// be defined, if your application uses MFC.
//  Setting applies to  : building PDL
//  Runtime restrictions: MFC DLLs must be installed
//  Additonal notes             : If both PDL_HAS_MFC and PDL_MT_SAFE are
//                        defined, the MFC DLL (not the static lib)
//                        will be used from PDL.
#if !defined (PDL_HAS_MFC)
# define PDL_HAS_MFC 0
#endif

// Define PDL_HAS_STRICT to 1 in your config.h file if you want to use
// STRICT type checking.  It is disabled by default because it will
// break existing application code.
//  Setting applies to  : building PDL, linking with PDL
//  Runtime restrictions: -
//  Additonal notes             : PDL_HAS_MFC implies PDL_HAS_STRICT
#if !defined (PDL_HAS_STRICT)
# define PDL_HAS_STRICT 0
#endif

// Turn off the following define if you want to disable threading.
// Compile using multi-thread libraries.
//  Setting applies to  : building PDL, linking with PDL
//  Runtime restrictions: multithreaded runtime DLL must be installed
#if !defined (PDL_MT_SAFE)
# define PDL_MT_SAFE 1
#endif

// Build as as a DLL.  Define PDL_HAS_DLL to 0 if you want to build a static
// lib.
//  Setting applies to  : building PDL, linking with PDL
//  Runtime restrictions: PDL DLL must be installed :-)
#if !defined (PDL_HAS_DLL)
/*BUGBUG_NT*//*# define PDL_HAS_DLL 1*/
# define PDL_HAS_DLL 0
/*BUGBUG_NT*/
#endif

// Build PDL services as DLLs.  If you write a library and want it to
// use PDL_Svc_Export, this will cause those macros to build dlls.  If
// you want your PDL service to be a static library, comment out this
// line.  As far as I know, the only reason to have a library be an
// PDL "service" is to leverage the PDL_Svc_Export macros.  It's just
// as easy to define your own export macros.
#if !defined (PDL_HAS_SVC_DLL)
# define PDL_HAS_SVC_DLL 1
#endif

// Define PDL_HAS_WINSOCK2 to 0 in your config.h file if you do *not*
// want to compile with WinSock 2.0.
//  Setting applies to  : building PDL
//  Runtime restrictions: winsock2 must be installed.
//      #define PDL_HAS_WINSOCK2 0

// Define PDL_HAS_ORBIX to 1 in your config.h file if you want to integrate
// PDL and Orbix in Win32.
//  Setting applies to  : building PDL, linking with PDL
//  Runtime restrictions: system must have Orbix DLLs
#if !defined (PDL_HAS_ORBIX)
# define PDL_HAS_ORBIX 0
#endif

#if !defined (PDL_HAS_MT_ORBIX)
# define PDL_HAS_MT_ORBIX 0
#endif

// By default, you will get the proper namespace usage for Orbix.  If
// you don't like this, comment out the #define line or #undef
// PDL_ORBIX_HAS_NAMESPACES in your config.h file after including this
// file.
#if !defined (PDL_ORBIX_HAS_NAMESPACES)
# define PDL_ORBIX_HAS_NAMESPACES
#endif /* PDL_ORBIX_HAS_NAMESPACES */

// By default, we use non-static object manager on Win32.  That is,
// the object manager is allocated in main's stack memory.  If this
// does not suit your need, i.e., if your programs depend on the use
// of static object manager, you neet to disable the behavior by adding
//
//   #undef PDL_HAS_NONSTATIC_OBJECT_MANAGER
//
// in the config.h after including config-win32.h
//
// MFC users: the main function is defined within a MFC library and
// therefore, PDL won't be able to meddle with main function and
// instantiate the non-static object manager for you.  To solve the
// problem, you'll need to instantiate the PDL_Object_Manager by
// either:
//
// 1. Using static object manager (as described above), however, using
// the non-static object manager is prefered, therefore,
// 2. Instantiate the non-static object manager yourself by either 1)
//    call PDL::init () at the beginning and PDL::fini () at the end,
//    _or_ 2) instantiate the PDL_Object_Manager in your CWinApp
//    derived class.
//
// Optionally, you can #define
// PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER in your
// config.h and always take care of the business by yourself.
// PDL_DOESNT_INSTANTIATE_NONSTATIC_OBJECT_MANAGER has no effect when
// using static object managers.
#if !defined (PDL_HAS_NONSTATIC_OBJECT_MANAGER)
# define PDL_HAS_NONSTATIC_OBJECT_MANAGER
#elif (PDL_HAS_NONSTATIC_OBJECT_MANAGER == 0)
# undef PDL_HAS_NONSTATIC_OBJECT_MANAGER
#endif /* PDL_HAS_NONSTATIC_OBJECT_MANAGER */

#define PDL_HAS_GPERF

// ---------------- platform features or lack of them -------------

// Windows doesn't like 65536 ;-) If 65536 is specified, it is
// listenly ignored by the OS, i.e., setsockopt does not fail, and you
// get stuck with the default size of 8k.
#define PDL_DEFAULT_MAX_SOCKET_BUFSIZ 65535

//
// It seems like Win32 does not have a limit on the number of buffers
// that can be transferred by the scatter/gather type of I/O
// functions, e.g., WSASend and WSARecv.  We are arbitrarily setting
// this to be 1k for now.  The typically use case is to create an I/O
// vector array of size IOV_MAX on the stack and then filled in.  Note
// that we probably don't want too big a value for IOV_MAX since it
// may mostly go to waste or the size of the activation record may
// become excessively large.
//
#if !defined (IOV_MAX)
# define IOV_MAX 1024
#endif /* IOV_MAX */

#if !defined (PDL_HAS_WINCE)
// Platform supports pread() and pwrite()
# define PDL_HAS_P_READ_WRITE
# define PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS
#endif /* ! PDL_HAS_WINCE */

#if !defined (__MINGW32__)
# define PDL_HAS_WIN32_STRUCTURAL_EXCEPTIONS
#endif /* __MINGW32__ */

#define PDL_DEFAULT_THREAD_PRIORITY 0

#define PDL_HAS_RECURSIVE_MUTEXES
#define PDL_HAS_MSG
#define PDL_HAS_SOCKADDR_MSG_NAME
#define PDL_LACKS_GETPGID
#define PDL_LACKS_GETPPID
#define PDL_LACKS_SETPGID
#define PDL_LACKS_SETREGID
#define PDL_LACKS_SETREUID
#define PDL_HAS_THREAD_SAFE_ACCEPT
#define PDL_LACKS_GETHOSTENT
#define PDL_LACKS_SIGACTION
#define PDL_LACKS_SIGSET
#define PDL_LACKS_FORK
#define PDL_LACKS_UNIX_SIGNALS
#define PDL_LACKS_SBRK
#define PDL_LACKS_UTSNAME_T
#define PDL_LACKS_SEMBUF_T
#define PDL_LACKS_MSGBUF_T
#define PDL_LACKS_SYSV_SHMEM
#define PDL_LACKS_UNISTD_H
#define PDL_LACKS_RLIMIT

#define PDL_SIZEOF_LONG_LONG 8
typedef unsigned __int64 PDL_UINT64;

// Optimize PDL_Handle_Set for select().
#define PDL_HAS_HANDLE_SET_OPTIMIZED_FOR_SELECT

// Win32 has UNICODE support
#if !defined (PDL_HAS_UNICODE)
#define PDL_HAS_UNICODE
#endif /* !PDL_HAS_UNICODE */

// Win32 has wchar_t support
#define PDL_HAS_WCHAR

// Compiler/platform correctly calls init()/fini() for shared
// libraries. - applied for DLLs ?
//define PDL_HAS_AUTOMATIC_INIT_FINI

// Platform supports POSIX O_NONBLOCK semantics.
//define PDL_HAS_POSIX_NONBLOCK

// Platform contains <poll.h>.
//define PDL_HAS_POLL

// Platform supports the /proc file system.
//define PDL_HAS_PROC_FS

#if (defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0))
// Platform supports the rusage struct.
#define PDL_HAS_GETRUSAGE
#endif /* (defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)) */

// Andreas Ueltschi tells me this is a good thing...
#define PDL_HAS_SVR5_GETTIMEOFDAY

// Compiler/platform supports SVR4 signal typedef.
//define PDL_HAS_SVR4_SIGNAL_T

// Platform provides <sys/filio.h> header.
//define PDL_HAS_SYS_FILIO_H

// Compiler/platform supports sys_siglist array.
//define PDL_HAS_SYS_SIGLIST

// Platform supports PDL_TLI timod STREAMS module.
//define PDL_HAS_TIMOD_H

// Platform supports PDL_TLI tiuser header.
//define PDL_HAS_TIUSER_H

// Platform provides PDL_TLI function prototypes.
// For Win32, this is not really true, but saves a lot of hassle!
//#define PDL_HAS_TLI_PROTOTYPES

// Platform supports PDL_TLI.
//define PDL_HAS_TLI

// I'm pretty sure NT lacks these
#define PDL_LACKS_UNIX_DOMAIN_SOCKETS

// Windows NT needs readv() and writev()
#define PDL_LACKS_WRITEV
#define PDL_LACKS_READV

#define PDL_LACKS_COND_T
#define PDL_LACKS_RWLOCK_T

#define PDL_LACKS_KEY_T

// Platform support for non-blocking connects is broken
#define PDL_HAS_BROKEN_NON_BLOCKING_CONNECTS

// No system support for replacing any previous mappings.
#define PDL_LACKS_AUTO_MMAP_REPLACEMENT

// Turns off the tracing feature.
#if !defined (PDL_NTRACE)
# define PDL_NTRACE 1
#endif /* PDL_NTRACE */
// #define PDL_NLOGGING

// MFC itself defines STRICT.
#if defined( PDL_HAS_MFC ) && (PDL_HAS_MFC != 0)
# if !defined(PDL_HAS_STRICT)
#  define PDL_HAS_STRICT 1
# endif
# if (PDL_HAS_STRICT != 1)
#  undef PDL_HAS_STRICT
#  define PDL_HAS_STRICT 1
# endif
#endif

// If you want to use highres timers, ensure that
// Build.Settings.C++.CodeGeneration.Processor is
// set to Pentium !
#if !defined(PDL_HAS_PENTIUM) && (_M_IX86 > 400)
# define PDL_HAS_PENTIUM
#endif

#if defined(PDL_MT_SAFE) && (PDL_MT_SAFE != 0)
// Platform supports threads.
# define PDL_HAS_THREADS

// Platform supports Windows32 threads.
# define PDL_HAS_WTHREADS

// Compiler/platform has thread-specific storage
# define PDL_HAS_THREAD_SPECIFIC_STORAGE

// Win32 doesn't have fcntl
#define PDL_LACKS_FCNTL

// must have _MT defined to include multithreading
// features from win32 headers
# if !defined(_MT) 
// *** DO NOT *** DO NOT *** defeat this error message
// by defining _MT yourself.  RTFM and see who set _MT.
#  error You must link PDL against multi-threaded libraries.
# endif /* _MT */
#endif /* PDL_MT_SAFE && PDL_MT_SAFE != 0 */

// We are using STL's min and max (in algobase.h).  Therefore the
// macros in window.h are extra
#if !defined (NOMINMAX)
# define NOMINMAX
#endif /* NOMINMAX */

#if defined (PDL_HAS_MOSTLY_UNICODE_APIS) && !defined (UNICODE)
# define UNICODE
#endif /* PDL_HAS_MOSTLY_UNICODE_APIS && !UNICODE */

#if defined (_UNICODE)
# if !defined (UNICODE)
#  define UNICODE         /* UNICODE is used by Windows headers */
# endif /* UNICODE */
#endif /* _UNICODE */

#if defined (UNICODE)
# if !defined (_UNICODE)
#  define _UNICODE        /* _UNICODE is used by C-runtime/MFC headers */
# endif /* _UNICODE */
#endif /* UNICODE */

#if !defined(_DEBUG)
// If we are making a release, and the user has not specified
// inline directives, we will default to inline
# if ! defined (__PDL_INLINE__)
#  define __PDL_INLINE__ 1
# endif /* __PDL_INLINE__ */
#endif

// If __PDL_INLINE__ is defined to be 0, we will undefine it
#if defined (__PDL_INLINE__) && (__PDL_INLINE__ == 0)
# undef __PDL_INLINE__
#endif /* __PDL_INLINE__ */

// PDL_USES_STATIC_MFC always implies PDL_HAS_MFC
#if defined (PDL_USES_STATIC_MFC)
# if defined (PDL_HAS_MFC)
#   undef PDL_HAS_MFC
# endif
# define PDL_HAS_MFC 1
#endif /* PDL_USES_STATIC_MFC */

// We are build PDL and want to use MFC (multithreaded)
#if defined(PDL_HAS_MFC) && (PDL_HAS_MFC != 0) && defined (_MT)
# if (PDL_HAS_DLL != 0) && defined(PDL_BUILD_DLL) && !defined (_WINDLL)
// force multithreaded MFC DLL
#  define _WINDLL
# endif /* _AFXDLL */
# if !defined (_AFXDLL) && !defined (PDL_USES_STATIC_MFC)
#  define _AFXDLL
# endif /* _AFXDLL */
#endif

// <windows.h> and MFC's <afxwin.h> are mutually
// incompatible. <windows.h> is brain-dead about MFC; it doesn't check
// to see whether MFC stuff is anticipated or already in progress
// before doing its thing. PDL needs (practically always) <winsock.h>,
// and winsock in turn needs support either from windows.h or from
// afxwin.h. One or the other, not both.
//
// The MSVC++ V4.0 environment defines preprocessor macros that
// indicate the programmer has chosen something from the
// Build-->Settings-->General-->MFC combo-box. <afxwin.h> defines a
// macro itself to protect against double inclusion. We'll take
// advantage of all this to select the proper support for winsock. -
// trl 26-July-1996

// This is necessary since MFC users apparently can't #include
// <windows.h> directly.
#if defined (PDL_HAS_MFC) && (PDL_HAS_MFC != 0)
# include /**/ <afxwin.h>   /* He is doing MFC */
// Windows.h will be included via afxwin.h->afx.h->afx_ver_.h->afxv_w32.h
// #define      _INC_WINDOWS  // Prevent winsock.h from including windows.h
#elif defined (PDL_HAS_WINCE)
# include /**/ <windows.h>
#endif

#if !defined (_INC_WINDOWS)     /* Already include windows.h ? */
// Must define strict before including windows.h !
# if defined (PDL_HAS_STRICT) && (PDL_HAS_STRICT != 0) && !defined (STRICT)
#  define STRICT 1
# endif /* PDL_HAS_STRICT */

# if !defined (WIN32_LEAN_AND_MEAN)
#  define WIN32_LEAN_AND_MEAN
# endif /* WIN32_LEAN_AND_MEAN */

# if defined (_UNICODE)
#  if !defined (UNICODE)
#   define UNICODE         /* UNICODE is used by Windows headers */
#  endif /* UNICODE */
# endif /* _UNICODE */

# if defined (UNICODE)
#  if !defined (_UNICODE)
#   define _UNICODE        /* _UNICODE is used by C-runtime/MFC headers */
#  endif /* _UNICODE */
# endif /* UNICODE */

#endif /* !defined (_INC_WINDOWS) */

// Always use WS2 when available
#if (defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0))
# if !defined(PDL_HAS_WINSOCK2)
#  define PDL_HAS_WINSOCK2 1
# endif /* !defined(PDL_HAS_WINSOCK2) */
#endif /* (defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)) */

#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
# if !defined (_WINSOCK2API_)
// will also include windows.h, if not present
#  include /**/ <winsock2.h>
# endif /* _WINSOCK2API */

# if defined (PDL_HAS_FORE_ATM_WS2)
#  include /**/ <ws2atm.h>
# endif /*PDL_HAS_FORE_ATM_WS2 */

# if !defined _MSWSOCK_ &&  !defined (PDL_HAS_WINCE)
#  include /**/ <mswsock.h>
# endif /* _MSWSOCK_ */

# if defined (_MSC_VER)
#  pragma comment(lib, "ws2_32.lib")
#  pragma comment(lib, "mswsock.lib")
# endif /* _MSC_VER */

# define PDL_WSOCK_VERSION 2, 0
#else
# if !defined (_WINSOCKAPI_)
   // will also include windows.h, if not present
#  include /**/ <winsock.h>
# endif /* _WINSOCKAPI */

// PharLap ETS has its own winsock lib, so don't grab the one
// supplied with the OS.
# if defined (_MSC_VER) && !defined (UNDER_CE) && !defined (PDL_HAS_PHARLAP)
#  pragma comment(lib, "wsock32.lib")
# endif /* _MSC_VER */

// We can't use recvmsg and sendmsg unless WinSock 2 is available
# define PDL_LACKS_RECVMSG
# define PDL_LACKS_SENDMSG

// Version 1.1 of WinSock
# define PDL_WSOCK_VERSION 1, 1
#endif /* PDL_HAS_WINSOCK2 */

// Platform supports IP multicast on Winsock 2
#if defined (PDL_HAS_WINSOCK2) && (PDL_HAS_WINSOCK2 != 0)
# define PDL_HAS_IP_MULTICAST
#endif /* PDL_HAS_WINSOCK2 */

#if (defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)) && !defined (PDL_USES_WINCE_SEMA_SIMULATION)
# define PDL_HAS_INTERLOCKED_EXCHANGEADD
# define PDL_HAS_WIN32_TRYLOCK
# define PDL_HAS_SIGNAL_OBJECT_AND_WAIT

// If CancelIO is undefined get the updated sp2-sdk from MS
# define PDL_HAS_CANCEL_IO
#endif /* (defined (PDL_HAS_WINNT4) && (PDL_HAS_WINNT4 != 0)) && !defined (PDL_USES_WINCE_SEMA_SIMULATION) */

#if !defined (PDL_SEH_DEFAULT_EXCEPTION_HANDLING_ACTION)
# define PDL_SEH_DEFAULT_EXCEPTION_HANDLING_ACTION EXCEPTION_CONTINUE_SEARCH
#endif /* PDL_SEH_DEFAULT_EXCEPTION_HANDLING_ACTION */

// Try to make a good guess whether we are compiling with the newer version
// of WinSock 2 that has GQOS support.
#if !defined (PDL_HAS_WINSOCK2_GQOS)
# if defined (WINSOCK_VERSION)
#  define PDL_HAS_WINSOCK2_GQOS 1
# endif /* WINSOCK_VERSION */
#endif /* PDL_HAS_WINSOCK2_GQOS */

#if defined (PDL_WIN64)
// Data must be aligned on 8-byte boundaries, at a minimum.
#  define PDL_MALLOC_ALIGN 8
// Void pointers are 8 bytes
#  define PDL_SIZEOF_VOID_P 8
#endif /* PDL_WIN64 */

// Platform supports IPv6
#define PDL_HAS_IP6

#endif /* PDL_WIN32_COMMON_H */

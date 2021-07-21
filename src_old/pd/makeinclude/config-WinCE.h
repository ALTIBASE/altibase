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
 
// config-WinCE.h,v 1.36 2000/01/17 20:29:00 nanbor Exp

#ifndef PDL_CONFIG_WINCE_H
#define PDL_CONFIG_WINCE_H

#if !defined (PDL_HAS_WINCE)
#define PDL_HAS_WINCE 1
#endif

// We need these libraries to build:
#pragma comment(lib,"corelibc.lib")
#pragma comment(linker, "/nodefaultlib:oldnames.lib")

// Unicode is the OS standard string type for WinCE.
#ifdef PDL_HAS_UNICODE
# undef PDL_HAS_UNICODE
#endif  /* PDL_HAS_UNICODE */
#define PDL_HAS_UNICODE 1
#define PDL_HAS_MOSTLY_UNICODE_APIS

// Only DLL version is supported on CE.
#if defined (PDL_HAS_DLL)
# undef PDL_HAS_DLL
#endif /* PDL_HAS_DLL */
//#define PDL_HAS_DLL 1

// Need to define LD search path explicitly on CE because
// CE doesn't have environment variables and we can't get
// the information using getenv.
#define PDL_DEFAULT_LD_SEARCH_PATH ".\\;\\windows"

// CE is not NT.
#if defined (PDL_HAS_WINNT4)
# undef PDL_HAS_WINNT4
#endif /* PDL_HAS_WINNT4 */
#define PDL_HAS_WINNT4 0

#define PDL_LACKS_PDL_TOKEN
#define PDL_LACKS_PDL_OTHER

// You must use MFC with PDL on CE.
#if defined (PDL_HAS_MFC) && (PDL_HAS_MFC != 0)
# undef PDL_HAS_MFC
#endif /* PDL_HAS_MFC */
#define PDL_HAS_MFC 0

#if !defined (PDL_USES_WCHAR)
# define PDL_USES_WCHAR
#endif  // PDL_USES_WCHAR

#define PDL_USES_WINCE_SEMA_SIMULATION

#define PDL_LACKS_IOSTREAM_TOTALLY

#if defined (PDL_HAS_STRICT)
# undef PDL_HAS_STRICT
#endif /* PDL_HAS_STRICT */
#define PDL_HAS_STRICT 1

//#define PDL_HAS_NONSTATIC_OBJECT_MANAGER 1

// We need to rename program entry name "main" with pdl_ce_main here
// so that we can call it from CE's bridge class.
#define PDL_MAIN pdl_ce_main

// SH3 cross-compiler can't handle inline functions correctly (along with other bugs.)
#if defined (SH3) && defined (DEBUG)
#define PDL_LACKS_INLINE_FUNCTIONS
#endif /* SH3 && _DEBUG */

#ifndef PDL_DEFAULT_SERVER_HOST
# define PDL_DEFAULT_SERVER_HOST L"localhost"
#endif /* PDL_DEFAULT_SERVER_HOST */

// @@ Need to remap every function that uses any of these flags to
//    Win32 API.  These are for ANSI styled function and are not
//    available on WinCE.

#define _O_RDONLY       0x0000  /* open for reading only */
#define _O_WRONLY       0x0001  /* open for writing only */
#define _O_RDWR         0x0002  /* open for reading and writing */
#define _O_APPEND       0x0008  /* writes done at eof */

#define _O_CREAT        0x0100  /* create and open file */
#define _O_TRUNC        0x0200  /* open and truncate */
#define _O_EXCL         0x0400  /* open only if file doesn't already exist */

/* O_TEXT files have <cr><lf> sequences translated to <lf> on read()'s,
** and <lf> sequences translated to <cr><lf> on write()'s
*/

#define _O_TEXT         0x4000  /* file mode is text (translated) */
#define _O_BINARY       0x8000  /* file mode is binary (untranslated) */

/* macro to translate the C 2.0 name used to force binary mode for files */

//#define _O_RAW  _O_BINARY

/* Open handle inherit bit */

//#define _O_NOINHERIT    0x0080  /* child process doesn't inherit file */

/* Temporary file bit - file is deleted when last handle is closed */

#define _O_TEMPORARY    0x0040  /* temporary file bit */

/* temporary access hint */

//#define _O_SHORT_LIVED  0x1000  /* temporary storage file, try not to flush */

/* sequential/random access hints */

//#define _O_SEQUENTIAL   0x0020  /* file access is primarily sequential */
//#define _O_RANDOM       0x0010  /* file access is primarily random */


// Non-ANSI names
#define O_RDONLY        _O_RDONLY
#define O_WRONLY        _O_WRONLY
#define O_RDWR          _O_RDWR
#define O_APPEND        _O_APPEND
#define O_CREAT         _O_CREAT
#define O_TRUNC         _O_TRUNC
#define O_EXCL          _O_EXCL
#define O_TEXT          _O_TEXT
#define O_BINARY        _O_BINARY
//#define O_RAW           _O_BINARY
#define O_TEMPORARY     _O_TEMPORARY
//#define O_NOINHERIT     _O_NOINHERIT
//#define O_SEQUENTIAL    _O_SEQUENTIAL
//#define O_RANDOM        _O_RANDOM


// @@ NSIG value.  This is definitely not correct.
#define NSIG 23


// @@ For some reason, WinCE forgot to define this.
//    Need to find out what it is. (Used in MapViewOfFile ().)
#define FILE_MAP_COPY 0


#define PDL_LACKS_STRCASECMP    // WinCE doesn't support _stricmp
#define PDL_LACKS_GETSERVBYNAME
//#define PDL_LACKS_ACCESS
#define PDL_LACKS_FILELOCKS
#define PDL_LACKS_EXEC
//#define PDL_LACKS_MKTEMP
#define PDL_LACKS_STRRCHR
#define PDL_LACKS_BSEARCH
#define PDL_LACKS_SOCKET_BUFSIZ
#define PDL_LACKS_ISATTY

// @@ Followings are used to keep existing programs happy.

#if !defined (BUFSIZ)
#  define BUFSIZ   1024
#endif /* BUFSIZ */

#if defined (UNDER_CE) && (UNDER_CE < 211)
#define EOF  -1
#endif /* UNDER_CE && UNDER_CE < 211 */

typedef void (*__sighandler_t)(int); // keep Signal compilation happy
typedef long off_t;

//#if defined (UNDER_CE) && (UNDER_CE > 200)
#define EMFILE WSAEMFILE
#define EINTR  WSAEINTR
#define EACCES ERROR_ACCESS_DENIED
#define ENOSPC ERROR_HANDLE_DISK_FULL
#define EEXIST ERROR_FILE_EXISTS
#define EPIPE  ERROR_BROKEN_PIPE
#define EFAULT WSAEFAULT
#define ENOENT WSAEINVAL
#define EINVAL WSAEINVAL
#define ERANGE WSAEINVAL
#define EAGAIN WSAEWOULDBLOCK
#define ENOMEM ERROR_OUTOFMEMORY
#define ENODEV ERROR_BAD_DEVICE
#define EFBIG  (0xDF)
#define PDL_LACKS_MALLOC_H      // We do have malloc.h, but don't use it.
//#endif /* UNDER_CE && UNDER_CE > 201 */

#if defined (UNDER_CE) && (UNDER_CE < 211)
//#define  FILE  void             // Try to map FILE* to HANDLE
#define SEEK_SET FILE_BEGIN
#define SEEK_CUR FILE_CURRENT
#define SEEK_END FILE_END
//#define stderr 0
//#define stdin 0
//#define stdout 0
#endif /* UNDER_CE && UNDER_CE < 211 */

//#if defined (UNDER_CE) && (UNDER_CE >= 211)
#define PDL_HAS_WINCE_BROKEN_ERRNO
#define _MAX_FNAME 255
//#endif /* UNDER_CE && UNDER_CE >= 211 */

#define PDL_HAS_STRDUP_EMULATION

// @@ This needs to be defined and initialized as a static. (Singleton?)
#define PDL_DEFAULT_LOG_STREAM 0
#define PDL_HAS_WINSOCK2 1
// isprint macro is missing.

// fix TASK-3168
#include <new.h>

#define O_SYNC 010000           /* /usr/include/bits/fcntl.h */ 
#define EBADF 9                 /* /usr/include/asm-generic/errno-base.h */

void _printf( char * format, ... );

#define SMALL_FOOTPRINT

#endif /* PDL_CONFIG_WINCE_H */


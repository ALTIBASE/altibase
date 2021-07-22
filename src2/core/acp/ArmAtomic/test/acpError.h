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
 
/*******************************************************************************
 * $Id: acpError.h 10870 2010-04-21 01:39:39Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_ERROR_H_)
#define _O_ACP_ERROR_H_

/**
 * @file
 * @ingroup CoreError
 */

#include <acpTypes.h>

/* ALINT:S9-SKIP */

ACP_EXTERN_C_BEGIN


/**
 * result code type
 */
typedef acp_sint32_t acp_rc_t;


/*
 * range of error code
 */
#define ACP_RR_STD      0
#define ACP_RR_WIN  20000
#define ACP_RR_ACP  40000
#define ACP_RR_APP 100000

/**
 * base of application defined result code
 */
#define ACP_RC_BASE_APP_DEFINED ACP_RR_APP

/*
 * access error code
 */
#if defined(__STATIC_ANALYSIS_DOING__)

# define ACP_RC_FROM_SYS_ERROR(e) (e)
# define ACP_RC_TO_SYS_ERROR(e)   (e)

# define ACP_RC_GET_OS_ERROR()    (-1) /* define as a constant */
# define ACP_RC_SET_OS_ERROR(e)   (errno = (e))

# define ACP_RC_GET_NET_ERROR()   (-1) /* define as a constant */
# define ACP_RC_SET_NET_ERROR(e)  (errno = (e))

#else

# if defined(ALTI_CFG_OS_WINDOWS)

#  define ACP_RC_CONVERT(e)        ((e) + ACP_RR_WIN)
#  define ACP_RC_REVERT(e)         ((e) - ACP_RR_WIN)
#  define ACP_RC_FROM_SYS_ERROR(e)              \
    ((e) == 0 ?                                 \
        ACP_RC_SUCCESS :                        \
        ACP_RC_CONVERT(e)                       \
    )
#  define ACP_RC_TO_SYS_ERROR(e)                \
    ((e) == 0 ?                                 \
        ACP_RC_SUCCESS :                        \
        ACP_RC_REVERT(e)                        \
        )

#  define ACP_RC_GET_OS_ERROR()    (ACP_RC_FROM_SYS_ERROR(GetLastError()))
#  define ACP_RC_SET_OS_ERROR(e)   (SetLastError(ACP_RC_TO_SYS_ERROR(e)))

#  define ACP_RC_GET_NET_ERROR()   (ACP_RC_FROM_SYS_ERROR(WSAGetLastError()))
#  define ACP_RC_SET_NET_ERROR(e)  (WSASetLastError(ACP_RC_TO_SYS_ERROR(e)))

# else

#  define ACP_RC_CONVERT(e)        (e)
#  define ACP_RC_REVERT(e)         (e)
#  define ACP_RC_FROM_SYS_ERROR(e) (e)
#  define ACP_RC_TO_SYS_ERROR(e)   (e)

#  define ACP_RC_GET_OS_ERROR()    (errno)
#  define ACP_RC_SET_OS_ERROR(e)   (errno = (e))

#  define ACP_RC_GET_NET_ERROR()   (errno)
#  define ACP_RC_SET_NET_ERROR(e)  (errno = (e))

# endif
#endif


/*
 * acp error code definition
 */
/**
 * success
 *
 * should use #ACP_RC_IS_SUCCESS() or #ACP_RC_NOT_SUCCESS() to test
 */
#define ACP_RC_SUCCESS           0
/**
 * invalid argument
 *
 * should use #ACP_RC_IS_EINVAL() or #ACP_RC_NOT_EINVAL() to test
 */
#define ACP_RC_EINVAL            (ACP_RR_ACP + 1)
/**
 * operation interrupted
 *
 * should use #ACP_RC_IS_EINTR() or #ACP_RC_NOT_EINTR() to test
 */
#define ACP_RC_EINTR             (ACP_RR_ACP + 2)
/**
 * no such process
 *
 * should use #ACP_RC_IS_ESRCH() or #ACP_RC_NOT_ESRCH() to test
 */
#define ACP_RC_ESRCH             (ACP_RR_ACP + 3)
/**
 * name is too long
 *
 * should use #ACP_RC_IS_ENAMETOOLONG() or #ACP_RC_NOT_ENAMETOOLONG() to test
 */
#define ACP_RC_ENAMETOOLONG      (ACP_RR_ACP + 4)
/**
 * already exists
 *
 * should use #ACP_RC_IS_EEXIST() or #ACP_RC_NOT_EEXIST() to test
 */
#define ACP_RC_EEXIST            (ACP_RR_ACP + 5)
/**
 * not exist
 *
 * should use #ACP_RC_IS_ENOENT() or #ACP_RC_NOT_ENOENT() to test
 */
#define ACP_RC_ENOENT            (ACP_RR_ACP + 6)
/**
 * not empty
 *
 * should use #ACP_RC_IS_ENOTEMPTY() or #ACP_RC_NOT_ENOTEMPTY() to test
 */
#define ACP_RC_ENOTEMPTY         (ACP_RR_ACP + 7)
/**
 * range overflow
 *
 * should use #ACP_RC_IS_ERANGE() or #ACP_RC_NOT_ERANGE() to test
 */
#define ACP_RC_ERANGE            (ACP_RR_ACP + 8)
/**
 * resource busy
 *
 * should use #ACP_RC_IS_EBUSY() or #ACP_RC_NOT_EBUSY() to test
 */
#define ACP_RC_EBUSY             (ACP_RR_ACP + 9)
/**
 * deadlock condition
 *
 * should use #ACP_RC_IS_EDEADLK() or #ACP_RC_NOT_EDEADLK() to test
 */
#define ACP_RC_EDEADLK           (ACP_RR_ACP + 10)
/**
 * permission denied
 *
 * should use #ACP_RC_IS_EPERM() or #ACP_RC_NOT_EPERM() to test
 */
#define ACP_RC_EPERM             (ACP_RR_ACP + 11)
/**
 * access denied
 *
 * should use #ACP_RC_IS_EACCES() or #ACP_RC_NOT_EACCES() to test
 */
#define ACP_RC_EACCES            (ACP_RR_ACP + 12)
/**
 * resource temporarily unavailable
 *
 * should use #ACP_RC_IS_EAGAIN() or #ACP_RC_NOT_EAGAIN() to test
 */
#define ACP_RC_EAGAIN            (ACP_RR_ACP + 13)
/**
 * operation now in progress
 *
 * should use #ACP_RC_IS_EINPROGRESS() or #ACP_RC_NOT_EINPROGRESS() to test
 */
#define ACP_RC_EINPROGRESS       (ACP_RR_ACP + 14)
/**
 * operation timed out
 *
 * should use #ACP_RC_IS_ETIMEDOUT() or #ACP_RC_NOT_ETIMEDOUT() to test
 */
#define ACP_RC_ETIMEDOUT         (ACP_RR_ACP + 15)
/**
 * bad file descriptor
 *
 * should use #ACP_RC_IS_EBADF() or #ACP_RC_NOT_EBADF() to test
 */
#define ACP_RC_EBADF             (ACP_RR_ACP + 16)
/**
 * not a directory
 *
 * should use #ACP_RC_IS_ENOTDIR() or #ACP_RC_NOT_ENOTDIR() to test
 */
#define ACP_RC_ENOTDIR           (ACP_RR_ACP + 17)
/**
 * not a socket
 *
 * should use #ACP_RC_IS_ENOTSOCK() or #ACP_RC_NOT_ENOTSOCK() to test
 */
#define ACP_RC_ENOTSOCK          (ACP_RR_ACP + 18)
/**
 * not enough space
 *
 * should use #ACP_RC_IS_ENOSPC() or #ACP_RC_NOT_ENOSPC() to test
 */
#define ACP_RC_ENOSPC            (ACP_RR_ACP + 19)
/**
 * not enough memory
 *
 * should use #ACP_RC_IS_ENOMEM() or #ACP_RC_NOT_ENOMEM() to test
 */
#define ACP_RC_ENOMEM            (ACP_RR_ACP + 20)
/**
 * too many open files
 *
 * should use #ACP_RC_IS_EMFILE() or #ACP_RC_NOT_EMFILE() to test
 */
#define ACP_RC_EMFILE            (ACP_RR_ACP + 21)
/**
 * file table overflow
 *
 * should use #ACP_RC_IS_ENFILE() or #ACP_RC_NOT_ENFILE() to test
 */
#define ACP_RC_ENFILE            (ACP_RR_ACP + 22)
/**
 * illegal seek
 *
 * should use #ACP_RC_IS_ESPIPE() or #ACP_RC_NOT_ESPIPE() to test
 */
#define ACP_RC_ESPIPE            (ACP_RR_ACP + 23)
/**
 * broken pipe
 *
 * should use #ACP_RC_IS_EPIPE() or #ACP_RC_NOT_EPIPE() to test
 */
#define ACP_RC_EPIPE             (ACP_RR_ACP + 24)
/**
 * connection refused
 *
 * should use #ACP_RC_IS_ECONNREFUSED() or #ACP_RC_NOT_ECONNREFUSED() to test
 */
#define ACP_RC_ECONNREFUSED      (ACP_RR_ACP + 25)
/**
 * connection aborted
 *
 * should use #ACP_RC_IS_ECONNABORTED() or #ACP_RC_NOT_ECONNABORTED() to test
 */
#define ACP_RC_ECONNABORTED      (ACP_RR_ACP + 26)
/**
 * connection reset by peer
 *
 * should use #ACP_RC_IS_ECONNRESET() or #ACP_RC_NOT_ECONNRESET() to test
 */
#define ACP_RC_ECONNRESET        (ACP_RR_ACP + 27)
/**
 * host is unreachable
 *
 * should use #ACP_RC_IS_EHOSTUNREACH() or #ACP_RC_NOT_EHOSTUNREACH() to test
 */
#define ACP_RC_EHOSTUNREACH      (ACP_RR_ACP + 28)
/**
 * network is unreachable
 *
 * should use #ACP_RC_IS_ENETUNREACH() or #ACP_RC_NOT_ENETUNREACH() to test
 */
#define ACP_RC_ENETUNREACH       (ACP_RR_ACP + 29)
/**
 * cross-device link
 *
 * should use #ACP_RC_IS_EXDEV() or #ACP_RC_NOT_EXDEV() to test
 */
#define ACP_RC_EXDEV             (ACP_RR_ACP + 30)
/**
 * operation not supported
 *
 * should use #ACP_RC_IS_ENOTSUP() or #ACP_RC_NOT_ENOTSUP() to test
 */
#define ACP_RC_ENOTSUP           (ACP_RR_ACP + 31)
/**
 * operation not implemented
 *
 * should use #ACP_RC_IS_ENOIMPL() or #ACP_RC_NOT_ENOIMPL() to test
 */
#define ACP_RC_ENOIMPL           (ACP_RR_ACP + 32)
/**
 * end of file
 *
 * should use #ACP_RC_IS_EOF() or #ACP_RC_NOT_EOF() to test
 */
#define ACP_RC_EOF               (ACP_RR_ACP + 33)
/**
 * dynamic linker error
 *
 * should use #ACP_RC_IS_EDLERR() or #ACP_RC_NOT_EDLERR() to test
 */
#define ACP_RC_EDLERR            (ACP_RR_ACP + 34)
/**
 * result truncated
 *
 * should use #ACP_RC_IS_ETRUNC() or #ACP_RC_NOT_ETRUNC() to test
 */
#define ACP_RC_ETRUNC            (ACP_RR_ACP + 35)
/**
 * bad address
 *
 * should use #ACP_RC_IS_EFAULT() or #ACP_RC_NOT_EFAULT() to test
 */
#define ACP_RC_EFAULT            (ACP_RR_ACP + 36)
/**
 * bad message file
 *
 * should use #ACP_RC_IS_EBADMDL() or #ACP_RC_NOT_EBADMDL() to test
 */
#define ACP_RC_EBADMDL           (ACP_RR_ACP + 37)
/**
 * operation canceled
 *
 * should use #ACP_RC_IS_ECANCELED() or #ACP_RC_NOT_ECANCELED() to test
 */
#define ACP_RC_ECANCELED         (ACP_RR_ACP + 38)
/**
 * no child process to wait for
 *
 * should use #ACP_RC_IS_ECHILD() or #ACP_RC_NOT_ECHILD() to test
 */
#define ACP_RC_ECHILD            (ACP_RR_ACP + 39)
/**
 * socket - protocol type or specified protocol not supported within this domain
 *
 * should use #ACP_RC_IS_EPROTONOSUPPORT() or
 * #ACP_RC_NOT_EPROTONOSUPPORT() to test
 */
#define ACP_RC_EPROTONOSUPPORT   (ACP_RR_ACP + 40)
/**
 * socket - socket is not connected
 *
 * should use #ACP_RC_IS_ENOTCONN() or #ACP_RC_NOT_ENOTCONN() to test
 */
#define ACP_RC_ENOTCONN          (ACP_RR_ACP + 41)
/**
 * socket - socket is already connected
 *
 * should use #ACP_RC_IS_EISCONN() or #ACP_RC_NOT_EISCONN() to test
 */
#define ACP_RC_EISCONN           (ACP_RR_ACP + 42)
/**
 * socket - specified option is not supported
 *
 * should use #ACP_RC_IS_ENOPROTOOPT() or #ACP_RC_NOT_ENOPROTOOPT() to test
 */
#define ACP_RC_ENOPROTOOPT       (ACP_RR_ACP + 43)
/**
 * socket - local address is already in use
 *
 * should use #ACP_RC_IS_EADDRINUSE() or #ACP_RC_NOT_EADDRINUSE() to test
 */
#define ACP_RC_EADDRINUSE        (ACP_RR_ACP + 44)
/**
 * socket - operation already in progress
 *
 * should use #ACP_RC_IS_EALREADY() or #ACP_RC_NOT_EALREADY() to test
 */
#define ACP_RC_EALREADY          (ACP_RR_ACP + 45)
/**
 * socket - too many symbolic links
 *
 * should use #ACP_RC_IS_ELOOP() or #ACP_RC_NOT_ELOOP() to test
 */
#define ACP_RC_ELOOP             (ACP_RR_ACP + 46)
/**
 * socket - cannot assign requested address
 *
 * should use #ACP_RC_IS_EADDRNOTAVAIL() or #ACP_RC_NOT_EADDRNOTAVAIL() to test
 */
#define ACP_RC_EADDRNOTAVAIL     (ACP_RR_ACP + 47)
/**
 * socket - operation not supported on transport endpoint
 *
 * should use #ACP_RC_IS_EOPNOTSUPP() or #ACP_RC_NOT_EOPNOTSUPP() to test
 */
#define ACP_RC_EOPNOTSUPP        (ACP_RR_ACP + 48)
/**
 * socket - destination address required
 *
 * should use #ACP_RC_IS_EDESTADDRREQ() or #ACP_RC_NOT_EDESTADDRREQ() to test
 */
#define ACP_RC_EDESTADDRREQ      (ACP_RR_ACP + 49)
/**
 * socket - address family not supported
 *
 * should use #ACP_RC_IS_ENOTSUP() or #ACP_RC_NOT_ENOTSUP() to test
 */
#define ACP_RC_EAFNOSUPPORT      (ACP_RR_ACP + 50)
/**
 * socket - protocol error
 *
 * should use #ACP_RC_IS_EPROTONOSUPPORT()
 * or #ACP_RC_NOT_EPROTONOSUPPORT() to test
 */
#define ACP_RC_EPROTO            (ACP_RR_ACP + 51)
/**
 * socket - operation would block
 *
 * should use #ACP_RC_IS_EAGAIN() or #ACP_RC_NOT_EAGAIN() to test
 * Note! Never use switch-case phrase to distinguish
 * #ACP_RC_EAGAIN from #ACP_RC_EWOULDBLOCK
 */
#define ACP_RC_EWOULDBLOCK       (ACP_RC_EAGAIN)
/**
 * no more lock available
 *
 * should use #ACP_RC_IS_ENOLCK() or #ACP_RC_NOT_ENOLCK() to test
 */
#define ACP_RC_ENOLCK            (ACP_RR_ACP + 52)
/**
 * The socket is shutdowned
 *
 * should use #ACP_RC_IS_ESHUTDOWN() or #ACP_RC_NOT_ESHUTDOWN() to test
 */
#define ACP_RC_ESHUTDOWN         (ACP_RR_ACP + 53)

/**
 * Invalid value for `ai_flags' field
 *
 * should use #ACP_RC_IS_EAI_BADFLAGS() or #ACP_RC_NOT_EAI_BADFLAGS() to test
 */
# define ACP_RC_EAI_BADFLAGS            (ACP_RR_ACP + 54)
/**
 * NAME or SERVICE is unknown.
 *
 * should use #ACP_RC_IS_EAI_NONAME() or #ACP_RC_NOT_EAI_NONAME() to test
 */
# define ACP_RC_EAI_NONAME              (ACP_RR_ACP + 55)
/**
 * Temporary failure in name resolution.
 *
 * should use #ACP_RC_IS_EAI_AGAIN() or #ACP_RC_NOT_EAI_AGAIN() to test
 */
# define ACP_RC_EAI_AGAIN               (ACP_RR_ACP + 56)
/**
 * Non-recoverable failure in name res.
 *
 * should use #ACP_RC_IS_EAI_FAIL() or #ACP_RC_NOT_EAI_FAIL() to test
 */
# define ACP_RC_EAI_FAIL                (ACP_RR_ACP + 57)
/**
 * `ai_family' not supported.
 *
 * should use #ACP_RC_IS_EAI_FAMILY() or #ACP_RC_NOT_EAI_FAMILY() to test
 */
# define ACP_RC_EAI_FAMILY              (ACP_RR_ACP + 58)
/**
 * `ai_socktype' not supported.
 *
 * should use #ACP_RC_IS_EAI_SOCKTYPE() or #ACP_RC_NOT_EAI_SOCKTYPE() to test
 */
# define ACP_RC_EAI_SOCKTYPE            (ACP_RR_ACP + 59)
/**
 * SERVICE not supported for `ai_socktype'.
 *
 * should use #ACP_RC_IS_EAI_SERVICE() or #ACP_RC_NOT_EAI_SERVICE() to test
 */
# define ACP_RC_EAI_SERVICE             (ACP_RR_ACP + 60)
/**
 * Memory allocation failure.
 *
 * should use #ACP_RC_IS_EAI_MEMORY() or #ACP_RC_NOT_EAI_MEMORY() to test
 */
# define ACP_RC_EAI_MEMORY              (ACP_RR_ACP + 61)
/**
 * System error returned in `errno'.
 *
 * should use #ACP_RC_IS_EAI_SYSTEM() or #ACP_RC_NOT_EAI_SYSTEM() to test
 */
# define ACP_RC_EAI_SYSTEM              (ACP_RR_ACP + 62)
/**
 * Argument buffer overflow.
 *
 * should use #ACP_RC_IS_EAI_OVERFLOW() or #ACP_RC_NOT_EAI_OVERFLOW() to test
 */
# define ACP_RC_EAI_OVERFLOW            (ACP_RR_ACP + 63)
/**
 * The specified network host does not have any network
 * addresses in the requested address family.
 *
 * should use #ACP_RC_IS_EAI_ADDRFAMILY() or
 * #ACP_RC_NOT_EAI_ADDRFAMILY() to test
 */
#define ACP_RC_EAI_ADDRFAMILY           (ACP_RR_ACP + 64)
/**
 * The specified network host exists, but does not have any
 * network addresses defined.
 *
 * should use #ACP_RC_IS_EAI_NODATA() or #ACP_RC_NOT_EAI_NODATA()
 */
#define ACP_RC_EAI_NODATA               (ACP_RR_ACP + 65)

/**
 * end of error code definition
 */
#define ACP_RC_MAX                      (ACP_RR_ACP + 66)

/*
 * re-define error code if the system already have it
 */
#if defined(EINVAL)
#undef  ACP_RC_EINVAL
#define ACP_RC_EINVAL EINVAL
#endif
#if defined(EINTR)
#undef  ACP_RC_EINTR
#define ACP_RC_EINTR EINTR
#endif
#if defined(ESRCH)
#undef  ACP_RC_ESRCH
#define ACP_RC_ESRCH ESRCH
#endif
#if defined(ENAMETOOLONG)
#undef  ACP_RC_ENAMETOOLONG
#define ACP_RC_ENAMETOOLONG ENAMETOOLONG
#endif
#if defined(EEXIST)
#undef  ACP_RC_EEXIST
#define ACP_RC_EEXIST EEXIST
#endif
#if defined(ENOENT)
#undef  ACP_RC_ENOENT
#define ACP_RC_ENOENT ENOENT
#endif
#if defined(ENOTEMPTY)
#undef  ACP_RC_ENOTEMPTY
#define ACP_RC_ENOTEMPTY ENOTEMPTY
#endif
#if defined(ERANGE)
#undef  ACP_RC_ERANGE
#define ACP_RC_ERANGE ERANGE
#endif
#if defined(EBUSY)
#undef  ACP_RC_EBUSY
#define ACP_RC_EBUSY EBUSY
#endif
#if defined(EDEADLK)
#undef  ACP_RC_EDEADLK
#define ACP_RC_EDEADLK EDEADLK
#endif
#if defined(EPERM)
#undef  ACP_RC_EPERM
#define ACP_RC_EPERM EPERM
#endif
#if defined(EACCES)
#undef  ACP_RC_EACCES
#define ACP_RC_EACCES EACCES
#endif
#if defined(EAGAIN)
#undef  ACP_RC_EAGAIN
#define ACP_RC_EAGAIN EAGAIN
#elif defined(EWOULDBLOCK)
#undef  ACP_RC_EAGAIN
#define ACP_RC_EAGAIN EWOULDBLOCK
#endif
#if defined(EINPROGRESS)
#undef  ACP_RC_EINPROGRESS
#define ACP_RC_EINPROGRESS EINPROGRESS
#endif
#if defined(ETIMEDOUT)
#undef  ACP_RC_ETIMEDOUT
#define ACP_RC_ETIMEDOUT ETIMEDOUT
#endif
#if defined(EBADF)
#undef  ACP_RC_EBADF
#define ACP_RC_EBADF EBADF
#endif
#if defined(ENOTDIR)
#undef  ACP_RC_ENOTDIR
#define ACP_RC_ENOTDIR ENOTDIR
#endif
#if defined(ENOTSOCK)
#undef  ACP_RC_ENOTSOCK
#define ACP_RC_ENOTSOCK ENOTSOCK
#endif
#if defined(ENOSPC)
#undef  ACP_RC_ENOSPC
#define ACP_RC_ENOSPC ENOSPC
#endif
#if defined(ENOMEM)
#undef  ACP_RC_ENOMEM
#define ACP_RC_ENOMEM ENOMEM
#endif
#if defined(EMFILE)
#undef  ACP_RC_EMFILE
#define ACP_RC_EMFILE EMFILE
#endif
#if defined(ENFILE)
#undef  ACP_RC_ENFILE
#define ACP_RC_ENFILE ENFILE
#endif
#if defined(ESPIPE)
#undef  ACP_RC_ESPIPE
#define ACP_RC_ESPIPE ESPIPE
#endif
#if defined(EPIPE)
#undef  ACP_RC_EPIPE
#define ACP_RC_EPIPE EPIPE
#endif
#if defined(ECONNREFUSED)
#undef  ACP_RC_ECONNREFUSED
#define ACP_RC_ECONNREFUSED ECONNREFUSED
#endif
#if defined(ECONNABORTED)
#undef  ACP_RC_ECONNABORTED
#define ACP_RC_ECONNABORTED ECONNABORTED
#endif
#if defined(ECONNRESET)
#undef  ACP_RC_ECONNRESET
#define ACP_RC_ECONNRESET ECONNRESET
#endif
#if defined(EHOSTUNREACH)
#undef  ACP_RC_EHOSTUNREACH
#define ACP_RC_EHOSTUNREACH EHOSTUNREACH
#endif
#if defined(ENETUNREACH)
#undef  ACP_RC_ENETUNREACH
#define ACP_RC_ENETUNREACH ENETUNREACH
#endif
#if defined(EXDEV)
#undef  ACP_RC_EXDEV
#define ACP_RC_EXDEV EXDEV
#endif
#if defined(ENOTSUP)
#undef  ACP_RC_ENOTSUP
#define ACP_RC_ENOTSUP ENOTSUP
#endif
#if defined(EFAULT)
#undef  ACP_RC_EFAULT
#define ACP_RC_EFAULT EFAULT
#endif
#if defined(ECANCELED)
#undef  ACP_RC_ECANCELED
#define ACP_RC_ECANCELED ECANCELED
#endif
#if defined(ECHILD)
#undef  ACP_RC_ECHILD
#define ACP_RC_ECHILD ECHILD
#endif
#if defined(ENOLCK)
#undef  ACP_RC_ENOLCK
#define ACP_RC_ENOLCK ENOLCK
#endif


/*
 * re-define socket errors
 */
#if defined(ALTI_CFG_OS_WINDOWS)

#if defined(WSAEPROTONOSUPPORT)
#undef  ACP_RC_EPROTONOSUPPORT
#define ACP_RC_EPROTONOSUPPORT      ACP_RC_CONVERT(WSAEPROTONOSUPPORT)
#endif

#if defined(WSAENOTCONN)
#undef  ACP_RC_ENOTCONN
#define ACP_RC_ENOTCONN             ACP_RC_CONVERT(WSAENOTCONN)
#endif

#if defined(WSAENOPROTOOPT)
#undef  ACP_RC_ENOPTOTOOPT
#define ACP_RC_ENOPTOTOOPT          ACP_RC_CONVERT(WSAENOPROTOOPT)
#endif

#if defined(WSAEADDRINUSE)
#undef  ACP_RC_EADDRINUSE
#define ACP_RC_EADDRINUSE           ACP_RC_CONVERT(WSAEADDRINUSE)
#endif

#if defined(WSAEALREADY)
#undef  ACP_RC_EALREADY
#define ACP_RC_EALREADY             ACP_RC_CONVERT(WSAEALREADY)
#endif

#if defined(WSAELOOP)
#undef  ACP_RC_ELOOP
#define ACP_RC_ELOOP                ACP_RC_CONVERT(WSAELOOP)
#endif

#if defined(WSAEADDRNOTAVAIL)
#undef  ACP_RC_EADDRNOTAVAIL
#define ACP_RC_EADDRNOTAVAIL        ACP_RC_CONVERT(WSAEADDRNOTAVAIL)
#endif

#if defined(WSAEOPNOTSUPP)
#undef  ACP_RC_EOPNOTSUPP
#define ACP_RC_EOPNOTSUPP           ACP_RC_CONVERT(WSAEOPNOTSUPP)
#endif

#if defined(WSAEDESTADDRREQ)
#undef  ACP_RC_EDESTADDRREQ
#define ACP_RC_EDESTADDRREQ         ACP_RC_CONVERT(WSAEDESTADDRREQ)
#endif

#if defined(WSAENOTSOCK)
#undef  ACP_RC_ENOTSOCK
#define ACP_RC_ENOTSOCK             ACP_RC_CONVERT(WSAENOTSOCK)
#endif

#if defined(WSAEAFNOSUPPORT)
#undef  ACP_RC_EAFNOSUPPORT
#define ACP_RC_EAFNOSUPPORT         ACP_RC_CONVERT(WSAEAFNOSUPPORT)
#endif

#if defined(WSAEPROTO)
#undef  ACP_RC_EPROTO
#define ACP_RC_EPROTO               ACP_RC_CONVERT(WSAEPROTO)
#endif

#if defined(WSAEWOULDBLOCK)
#undef  ACP_RC_EWOULDBLOCK
#define ACP_RC_EWOULDBLOCK          ACP_RC_CONVERT(WSAEWOULDBLOCK)
#endif

#if defined(WSAECONNREFUSED)
#undef  ACP_RC_ECONNREFUSED
#define ACP_RC_ECONNREFUSED         ACP_RC_CONVERT(WSAECONNREFUSED)
#endif

#if defined(WSAECONNABORTED)
#undef  ACP_RC_ECONNABORTED
#define ACP_RC_ECONNABORTED         ACP_RC_CONVERT(WSAECONNABORTED)
#endif

#if defined(WSAECONNRESET)
#undef  ACP_RC_ECONNRESET
#define ACP_RC_ECONNRESET           ACP_RC_CONVERT(WSAECONNRESET)
#endif

#if defined(WSAEHOSTUNREACH)
#undef  ACP_RC_EHOSTUNREACH
#define ACP_RC_EHOSTUNREACH         ACP_RC_CONVERT(WSAEHOSTUNREACH)
#endif

#if defined(WSAENETUNREACH)
#undef  ACP_RC_ENETUNREACH
#define ACP_RC_ENETUNREACH          ACP_RC_CONVERT(WSAENETUNREACH)
#endif

#if defined(WSAESHUTDOWN)
#undef  ACP_RC_ESHUTDOWN
#define ACP_RC_ESHUTDOWN            ACP_RC_CONVERT(WSAESHUTDOWN)
#endif

/* IPv6 error common for all platform */
#if defined(EAI_BADFLAGS)
#undef  ACP_RC_EAI_BADFLAGS
# define ACP_RC_EAI_BADFLAGS ACP_RC_CONVERT(WSAEINVAL)
#endif
#if defined(EAI_NONAME)
#undef ACP_RC_EAI_NONAME
# define ACP_RC_EAI_NONAME ACP_RC_CONVERT(WSAHOST_NOT_FOUND)
#endif
#if defined(EAI_AGAIN)
#undef ACP_RC_EAI_AGAIN
# define ACP_RC_EAI_AGAIN ACP_RC_CONVERT(WSATRY_AGAIN)
#endif
#if defined(EAI_FAIL)
#undef ACP_RC_EAI_FAIL
# define ACP_RC_EAI_FAIL ACP_RC_CONVERT(WSANO_RECOVERY)
#endif
#if defined(EAI_FAMILY)
#undef ACP_RC_EAI_FAMILY
# define ACP_RC_EAI_FAMILY ACP_RC_CONVERT(WSAEAFNOSUPPORT)
#endif
#if defined(EAI_SOCKTYPE)
#undef ACP_RC_EAI_SOCKTYPE
# define ACP_RC_EAI_SOCKTYPE ACP_RC_CONVERT(WSAESOCKTNOSUPPORT)
#endif
#if defined(EAI_SERVICE)
#undef ACP_RC_EAI_SERVICE
# define ACP_RC_EAI_SERVICE ACP_RC_CONVERT(WSATYPE_NOT_FOUND)
#endif
#if defined(EAI_MEMORY)
#undef ACP_RC_EAI_MEMORY
# define ACP_RC_EAI_MEMORY ACP_RC_CONVERT(WSA_NOT_ENOUGH_MEMORY)
#endif
#if defined(EAI_NODATA)
#undef ACP_RC_EAI_NODATA
# define ACP_RC_EAI_NODATA ACP_RC_CONVERT(WSANO_DATA)
#endif

#else /* Unix series */

#if defined(EPROTONOSUPPORT)
#undef  ACP_RC_EPROTONOSUPPORT
#define ACP_RC_EPROTONOSUPPORT      EPROTONOSUPPORT
#endif

#if defined(ENOTCONN)
#undef  ACP_RC_ENOTCONN
#define ACP_RC_ENOTCONN             ENOTCONN
#endif

#if defined(ENOPROTOOPT)
#undef  ACP_RC_ENOPTOTOOPT
#define ACP_RC_ENOPTOTOOPT          ENOPROTOOPT
#endif

#if defined(EADDRINUSE)
#undef  ACP_RC_EADDRINUSE
#define ACP_RC_EADDRINUSE           EADDRINUSE
#endif

#if defined(EALREADY)
#undef  ACP_RC_EALREADY
#define ACP_RC_EALREADY             EALREADY
#endif

#if defined(ELOOP)
#undef  ACP_RC_ELOOP
#define ACP_RC_ELOOP                ELOOP
#endif

#if defined(EADDRNOTAVAIL)
#undef  ACP_RC_EADDRNOTAVAIL
#define ACP_RC_EADDRNOTAVAIL        EADDRNOTAVAIL
#endif

#if defined(EOPNOTSUPP)
#undef  ACP_RC_EOPNOTSUPP
#define ACP_RC_EOPNOTSUPP           EOPNOTSUPP
#endif

#if defined(EDESTADDRREQ)
#undef  ACP_RC_EDESTADDRREQ
#define ACP_RC_EDESTADDRREQ         EDESTADDRREQ
#endif

#if defined(EAFNOSUPPORT)
#undef  ACP_RC_EAFNOSUPPORT
#define ACP_RC_EAFNOSUPPORT         EAFNOSUPPORT
#endif

#if defined(EPROTO)
#undef  ACP_RC_EPROTO
#define ACP_RC_EPROTO               EPROTO
#endif

#if defined(EWOULDBLOCK)
#undef  ACP_RC_EWOULDBLOCK
#define ACP_RC_EWOULDBLOCK          EWOULDBLOCK
#endif

#if defined(ESHUTDOWN)
#undef  ACP_RC_ESHUTDOWN
#define ACP_RC_ESHUTDOWN            ACP_RC_CONVERT(ESHUTDOWN)
#endif

/* IPv6 error common for all platform */
#if defined(EAI_BADFLAGS)
#undef  ACP_RC_EAI_BADFLAGS
# define ACP_RC_EAI_BADFLAGS (EAI_BADFLAGS)
#endif
#if defined(EAI_NONAME)
#undef ACP_RC_EAI_NONAME
# define ACP_RC_EAI_NONAME (EAI_NONAME)
#endif
#if defined(EAI_AGAIN)
#undef ACP_RC_EAI_AGAIN
# define ACP_RC_EAI_AGAIN (EAI_AGAIN)
#endif
#if defined(EAI_FAIL)
#undef ACP_RC_EAI_FAIL
# define ACP_RC_EAI_FAIL (EAI_FAIL)
#endif
#if defined(EAI_FAMILY)
#undef ACP_RC_EAI_FAMILY
# define ACP_RC_EAI_FAMILY (EAI_FAMILY)
#endif
#if defined(EAI_SOCKTYPE)
#undef ACP_RC_EAI_SOCKTYPE
# define ACP_RC_EAI_SOCKTYPE (EAI_SOCKTYPE)
#endif
#if defined(EAI_SERVICE)
#undef ACP_RC_EAI_SERVICE
# define ACP_RC_EAI_SERVICE (EAI_SERVICE)
#endif
#if defined(EAI_MEMORY)
#undef ACP_RC_EAI_MEMORY
# define ACP_RC_EAI_MEMORY (EAI_MEMORY)
#endif
#if defined(EAI_NODATA)
#undef ACP_RC_EAI_NODATA
# define ACP_RC_EAI_NODATA (EAI_NODATA)
#endif

/* IPv6 error common only for Unix platform */
#if defined(EAI_SYSTEM)
#undef ACP_RC_EAI_SYSTEM
# define ACP_RC_EAI_SYSTEM EAI_SYSTEM
#endif
#if defined(EAI_OVERFLOW)
#undef ACP_RC_EAI_OVERFLOW
# define ACP_RC_EAI_OVERFLOW EAI_OVERFLOW
#endif
#if defined(EAI_ADDRFAMILY)
#undef ACP_RC_EAI_ADDRFAMILY
# define ACP_RC_EAI_ADDRFAMILY EAI_ADDRFAMILY
#endif

#endif /* end of Unix series */

/*
 * test IS error code
 */
#if defined(ACP_CFG_DOXYGEN)

/**
 * test whether the result code is #ACP_RC_SUCCESS
 */
#define ACP_RC_IS_SUCCESS(e)
/**
 * test whether the result code is #ACP_RC_EINVAL or equivalent
 */
#define ACP_RC_IS_EINVAL(e)
/**
 * test whether the result code is #ACP_RC_EINTR or equivalent
 */
#define ACP_RC_IS_EINTR(e)
/**
 * test whether the result code is #ACP_RC_ESRCH or equivalent
 */
#define ACP_RC_IS_ESRCH(e)
/**
 * test whether the result code is #ACP_RC_ENAMETOOLONG or equivalent
 */
#define ACP_RC_IS_ENAMETOOLONG(e)
/**
 * test whether the result code is #ACP_RC_EEXIST or equivalent
 */
#define ACP_RC_IS_EEXIST(e)
/**
 * test whether the result code is #ACP_RC_ENOENT or equivalent
 */
#define ACP_RC_IS_ENOENT(e)
/**
 * test whether the result code is #ACP_RC_ENOTEMPTY or equivalent
 */
#define ACP_RC_IS_ENOTEMPTY(e)
/**
 * test whether the result code is #ACP_RC_ERANGE or equivalent
 */
#define ACP_RC_IS_ERANGE(e)
/**
 * test whether the result code is #ACP_RC_EBUSY or equivalent
 */
#define ACP_RC_IS_EBUSY(e)
/**
 * test whether the result code is #ACP_RC_EDEADLK or equivalent
 */
#define ACP_RC_IS_EDEADLK(e)
/**
 * test whether the result code is #ACP_RC_EPERM or equivalent
 */
#define ACP_RC_IS_EPERM(e)
/**
 * test whether the result code is #ACP_RC_EACCES or equivalent
 */
#define ACP_RC_IS_EACCES(e)
/**
 * test whether the result code is #ACP_RC_EAGAIN or equivalent
 */
#define ACP_RC_IS_EAGAIN(e)
/**
 * test whether the result code is #ACP_RC_EINPROGRESS or equivalent
 */
#define ACP_RC_IS_EINPROGRESS(e)
/**
 * test whether the result code is #ACP_RC_ETIMEDOUT or equivalent
 */
#define ACP_RC_IS_ETIMEDOUT(e)
/**
 * test whether the result code is #ACP_RC_EBADF or equivalent
 */
#define ACP_RC_IS_EBADF(e)
/**
 * test whether the result code is #ACP_RC_ENOTDIR or equivalent
 */
#define ACP_RC_IS_ENOTDIR(e)
/**
 * test whether the result code is #ACP_RC_ENOTSOCK or equivalent
 */
#define ACP_RC_IS_ENOTSOCK(e)
/**
 * test whether the result code is #ACP_RC_ENOSPC or equivalent
 */
#define ACP_RC_IS_ENOSPC(e)
/**
 * test whether the result code is #ACP_RC_ENOMEM or equivalent
 */
#define ACP_RC_IS_ENOMEM(e)
/**
 * test whether the result code is #ACP_RC_EMFILE or equivalent
 */
#define ACP_RC_IS_EMFILE(e)
/**
 * test whether the result code is #ACP_RC_ENFILE or equivalent
 */
#define ACP_RC_IS_ENFILE(e)
/**
 * test whether the result code is #ACP_RC_ESPIPE or equivalent
 */
#define ACP_RC_IS_ESPIPE(e)
/**
 * test whether the result code is #ACP_RC_EPIPE or equivalent
 */
#define ACP_RC_IS_EPIPE(e)
/**
 * test whether the result code is #ACP_RC_ECONNREFUSED or equivalent
 */
#define ACP_RC_IS_ECONNREFUSED(e)
/**
 * test whether the result code is #ACP_RC_ECONNABORTED or equivalent
 */
#define ACP_RC_IS_ECONNABORTED(e)
/**
 * test whether the result code is #ACP_RC_ECONNRESET or equivalent
 */
#define ACP_RC_IS_ECONNRESET(e)
/**
 * test whether the result code is #ACP_RC_EHOSTUNREACH or equivalent
 */
#define ACP_RC_IS_EHOSTUNREACH(e)
/**
 * test whether the result code is #ACP_RC_ENETUNREACH or equivalent
 */
#define ACP_RC_IS_ENETUNREACH(e)
/**
 * test whether the result code is #ACP_RC_EXDEV or equivalent
 */
#define ACP_RC_IS_EXDEV(e)
/**
 * test whether the result code is #ACP_RC_ENOTSUP or equivalent
 */
#define ACP_RC_IS_ENOTSUP(e)
/**
 * test whether the result code is #ACP_RC_ENOIMPL or equivalent
 */
#define ACP_RC_IS_ENOIMPL(e)
/**
 * test whether the result code is #ACP_RC_EOF or equivalent
 */
#define ACP_RC_IS_EOF(e)
/**
 * test whether the result code is #ACP_RC_EDLERR or equivalent
 */
#define ACP_RC_IS_EDLERR(e)
/**
 * test whether the result code is #ACP_RC_ETRUNC or equivalent
 */
#define ACP_RC_IS_ETRUNC(e)
/**
 * test whether the result code is #ACP_RC_EFAULT or equivalent
 */
#define ACP_RC_IS_EFAULT(e)
/**
 * test whether the result code is #ACP_RC_EBADMDL or equivalent
 */
#define ACP_RC_IS_EBADMDL(e)
/**
 * test whether the result code is #ACP_RC_ECANCELED or equivalent
 */
#define ACP_RC_IS_ECANCELED(e)
/**
 * test whether the result code is #ACP_RC_ECHILD or equivalent
 */
#define ACP_RC_IS_ECHILD(e)
/**
 * test whether the result code is #ACP_RC_EPROTONOSUPPORT or equivalent
 */
#define ACP_RC_IS_EPROTONOSUPPORT(e)
/**
 * test whether the result code is #ACP_RC_ENOTCONN
 */
#define ACP_RC_IS_ENOTCONN(e)
/**
 * test whether the result code is #ACP_RC_EISCONN
 */
#define ACP_RC_IS_EISCONN(e)
/**
 * test whether the result code is #ACP_RC_ENOPROTOOPT
 */
#define ACP_RC_IS_ENOPROTOOPT(e)
/**
 * test whether the result code is #ACP_RC_EADDRINUSE
 */
#define ACP_RC_IS_EADDRINUSE(e)
/**
 * test whether the result code is #ACP_RC_EALREADY
 */
#define ACP_RC_IS_EALREADY(e)
/**
 * test whether the result code is #ACP_RC_ELOOP
 */
#define ACP_RC_IS_ELOOP(e)
/**
 * test whether the result code is #ACP_RC_EADDRNOTAVAIL
 */
#define ACP_RC_IS_EADDRNOTAVAIL(e)
/**
 * test whether the result code is #ACP_RC_EOPNOTSUPP
 */
#define ACP_RC_IS_EOPNOTSUPP(e)
/**
 * test whether the result code is #ACP_RC_EDESTADDRREQ
 */
#define ACP_RC_IS_EDESTADDRREQ(e)
/**
 * test whether the result code is #ACP_RC_ENOLCK
 */
#define ACP_RC_IS_ENOLCK(e)
/**
 * test whether the result code is #ACP_RC_ESHUTDOWN
 */
#define ACP_RC_IS_ESHUTDOWN(e)
/**
 * test whether the result code represents that
 * an initialization have to come first
 */
#define ACP_RC_IS_NOTINITIALIZED(e)
/**
 * test whether the result code represents that
 * the network subsystem is not working
 */
#define ACP_RC_IS_NETUNUSABLE(e)

/* for IPv6 error */

/**
 * test whether the result code is #ACP_RC_EAI_BADFLAGS
 */
#define ACP_RC_IS_EAI_BADFLAGS(e)
/**
 * test whether the result code is #ACP_RC_EAI_NONAME
 */
#define ACP_RC_IS_EAI_NONAME(e)
/**
 * test whether the result code is #ACP_RC_EAI_AGAIN
 */
#define ACP_RC_IS_EAI_AGAIN(e)
/**
 * test whether the result code is #ACP_RC_EAI_FAIL
 */
#define ACP_RC_IS_EAI_FAIL(e)
/**
 * test whether the result code is #ACP_RC_EAI_FAMILY
 */
#define ACP_RC_IS_EAI_FAMILY(e)
/**
 * test whether the result code is #ACP_RC_EAI_SOCKTYPE
 */
#define ACP_RC_IS_EAI_SOCKTYPE(e)
/**
 * test whether the result code is #ACP_RC_EAI_SERVICE
 */
#define ACP_RC_IS_EAI_SERVICE(e)
/**
 * test whether the result code is #ACP_RC_EAI_MEMORY
 */
#define ACP_RC_IS_EAI_MEMORY(e)
/**
 * test whether the result code is #ACP_RC_EAI_SYSTEM
 */
#define ACP_RC_IS_EAI_SYSTEM(e)
/**
 * test whether the result code is #ACP_RC_EAI_OVERFLOW
 */
#define ACP_RC_IS_EAI_OVERFLOW(e)
/**
 * test whether the result code is #ACP_RC_EAI_ADDRFAMILY
 */
#define ACP_RC_IS_EAI_ADDRFAMILY(e)
/**
 * test whether the result code is #ACP_RC_EAI_NODATA
 */
#define ACP_RC_IS_EAI_NODATA(e)


#elif defined(ALTI_CFG_OS_WINDOWS)

#define ACP_RC_IS_SUCCESS(e)  ((e) == ACP_RC_SUCCESS)
#define ACP_RC_IS_EINVAL(e)                                     \
    (((e) == ACP_RC_EINVAL)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_ACCESS))           \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_DATA))             \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_FUNCTION))         \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_HANDLE))           \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_PARAMETER))        \
     || ((e) == ACP_RC_CONVERT(ERROR_NEGATIVE_SEEK))            \
     || ((e) == ACP_RC_CONVERT(ERROR_PATH_NOT_FOUND))           \
     || ((e) == ACP_RC_CONVERT(WSAEINVAL)))
#define ACP_RC_IS_EINTR(e)                                      \
    (((e) == ACP_RC_EINTR)                                      \
     || ((e) == ACP_RC_CONVERT(WSAEINTR)))
#define ACP_RC_IS_ESRCH(e)        ((e) == ACP_RC_ESRCH)
#define ACP_RC_IS_ENAMETOOLONG(e)                               \
    (((e) == ACP_RC_ENAMETOOLONG)                               \
     || ((e) == ACP_RC_CONVERT(ERROR_FILENAME_EXCED_RANGE))     \
     || ((e) == ACP_RC_CONVERT(WSAENAMETOOLONG)))
#define ACP_RC_IS_EEXIST(e)                                     \
    (((e) == ACP_RC_EEXIST)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_FILE_EXISTS))              \
     || ((e) == ACP_RC_CONVERT(ERROR_ALREADY_EXISTS)))
#define ACP_RC_IS_ENOENT(e)                                     \
    (((e) == ACP_RC_ENOENT)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_FILE_NOT_FOUND))           \
     || ((e) == ACP_RC_CONVERT(ERROR_PATH_NOT_FOUND))           \
     || ((e) == ACP_RC_CONVERT(ERROR_OPEN_FAILED))              \
     || ((e) == ACP_RC_CONVERT(ERROR_NO_MORE_FILES)))
#define ACP_RC_IS_ENOTEMPTY(e)                                  \
    (((e) == ACP_RC_ENOTEMPTY)                                  \
     || ((e) == ACP_RC_CONVERT(ERROR_DIR_NOT_EMPTY)))
#define ACP_RC_IS_ERANGE(e)       ((e) == ACP_RC_ERANGE)
#define ACP_RC_IS_EBUSY(e)        ((e) == ACP_RC_EBUSY)
#define ACP_RC_IS_EDEADLK(e)                                    \
    (((e) == ACP_RC_EDEADLK)                                    \
     || ((e) == ACP_RC_CONVERT(ERROR_POSSIBLE_DEADLOCK)))
#define ACP_RC_IS_EPERM(e)                                      \
    (((e) == ACP_RC_EPERM)                                      \
     ||  ((e) == ACP_RC_CONVERT(ERROR_NOT_OWNER)))
#define ACP_RC_IS_EACCES(e)                                     \
    (((e) == ACP_RC_EACCES)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_ACCESS_DENIED))            \
     || ((e) == ACP_RC_CONVERT(ERROR_CANNOT_MAKE))              \
     || ((e) == ACP_RC_CONVERT(ERROR_CURRENT_DIRECTORY))        \
     || ((e) == ACP_RC_CONVERT(ERROR_DRIVE_LOCKED))             \
     || ((e) == ACP_RC_CONVERT(ERROR_FAIL_I24))                 \
     || ((e) == ACP_RC_CONVERT(ERROR_LOCK_VIOLATION))           \
     || ((e) == ACP_RC_CONVERT(ERROR_LOCK_FAILED))              \
     || ((e) == ACP_RC_CONVERT(ERROR_NOT_LOCKED))               \
     || ((e) == ACP_RC_CONVERT(ERROR_NETWORK_ACCESS_DENIED))    \
     || ((e) == ACP_RC_CONVERT(ERROR_SHARING_VIOLATION))        \
     || ((e) == ACP_RC_CONVERT(WSAEACCES)))
#define ACP_RC_IS_EAGAIN(e)                                     \
    (((e) == ACP_RC_EAGAIN)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_NO_DATA))                  \
     || ((e) == ACP_RC_CONVERT(ERROR_NO_PROC_SLOTS))            \
     || ((e) == ACP_RC_CONVERT(ERROR_NESTING_NOT_ALLOWED))      \
     || ((e) == ACP_RC_CONVERT(ERROR_MAX_THRDS_REACHED))        \
     || ((e) == ACP_RC_CONVERT(ERROR_LOCK_VIOLATION))           \
     || ((e) == ACP_RC_EWOULDBLOCK))
#define ACP_RC_IS_EINPROGRESS(e)                                \
    (((e) == ACP_RC_EINPROGRESS)                                \
     || ((e) == ACP_RC_EWOULDBLOCK))
#define ACP_RC_IS_ETIMEDOUT(e)                                  \
    (((e) == ACP_RC_ETIMEDOUT)                                  \
     || ((e) == ACP_RC_CONVERT(WSAETIMEDOUT))                   \
     || ((e) == ACP_RC_CONVERT(WAIT_TIMEOUT)))
#define ACP_RC_IS_EBADF(e)                                      \
    (((e) == ACP_RC_EBADF)                                      \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_HANDLE))           \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_TARGET_HANDLE))    \
     || ((e) == ACP_RC_EBADF))
#define ACP_RC_IS_ENOTDIR(e)                                    \
    (((e) == ACP_RC_ENOTDIR)                                    \
     || ((e) == ACP_RC_CONVERT(ERROR_DIRECTORY))                \
     || ((e) == ACP_RC_CONVERT(ERROR_PATH_NOT_FOUND))           \
     || ((e) == ACP_RC_CONVERT(ERROR_BAD_NETPATH))              \
     || ((e) == ACP_RC_CONVERT(ERROR_BAD_NET_NAME))             \
     || ((e) == ACP_RC_CONVERT(ERROR_BAD_PATHNAME))             \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_DRIVE)))
#define ACP_RC_IS_ENOTSOCK(e)                                   \
    (((e) == ACP_RC_ENOTSOCK)                                   \
     || ((e) == ACP_RC_CONVERT(WSAENOTSOCK)))
#define ACP_RC_IS_ENOSPC(e)                                     \
    (((e) == ACP_RC_ENOSPC)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_DISK_FULL))                \
     || ((e) == ACP_RC_CONVERT(WSAEDQUOT)))
#define ACP_RC_IS_ENOMEM(e)                                     \
    (((e) == ACP_RC_ENOMEM)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_ARENA_TRASHED))            \
     || ((e) == ACP_RC_CONVERT(ERROR_NOT_ENOUGH_MEMORY))        \
     || ((e) == ACP_RC_CONVERT(ERROR_INVALID_BLOCK))            \
     || ((e) == ACP_RC_CONVERT(ERROR_NOT_ENOUGH_QUOTA))         \
     || ((e) == ACP_RC_CONVERT(ERROR_OUTOFMEMORY))              \
     || ((e) == ACP_RC_CONVERT(WSAENOBUFS)))
#define ACP_RC_IS_EMFILE(e)                                     \
    (((e) == ACP_RC_EMFILE)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_TOO_MANY_OPEN_FILES))      \
     || ((e) == ACP_RC_CONVERT(WSAEMFILE)))
#define ACP_RC_IS_ENFILE(e)       ((e) == ACP_RC_ENFILE)
#define ACP_RC_IS_ESPIPE(e)                                     \
    (((e) == ACP_RC_ESPIPE)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_SEEK_ON_DEVICE))           \
     || ((e) == ACP_RC_CONVERT(ERROR_NEGATIVE_SEEK)))
#define ACP_RC_IS_EPIPE(e)                                      \
    (((e) == ACP_RC_EPIPE)                                      \
     || ((e) == ACP_RC_CONVERT(ERROR_BROKEN_PIPE)))
#define ACP_RC_IS_ECONNREFUSED(e) ((e) == ACP_RC_ECONNREFUSED)
#define ACP_RC_IS_ECONNABORTED(e)                               \
    ((e) == ACP_RC_ECONNABORTED || (e) == ACP_RC_EPROTO)
#define ACP_RC_IS_ECONNRESET(e)                                 \
    (((e) == ACP_RC_ECONNRESET)                                 \
     || ((e) == ACP_RC_CONVERT(ERROR_NETNAME_DELETED))          \
     || ((e) == ACP_RC_CONVERT(WSAENETRESET)))
#define ACP_RC_IS_EHOSTUNREACH(e) ((e) == ACP_RC_EHOSTUNREACH)
#define ACP_RC_IS_ENETUNREACH(e)  ((e) == ACP_RC_ENETUNREACH)
#define ACP_RC_IS_EXDEV(e)                                      \
    (((e) == ACP_RC_EXDEV)                                      \
     || ((e) == ACP_RC_CONVERT(ERROR_NOT_SAME_DEVICE)))
#define ACP_RC_IS_ENOTSUP(e)                                    \
    (((e) == ACP_RC_ENOTSUP)                                    \
     || ((e) == ACP_RC_EAFNOSUPPORT)                            \
     || ((e) == ACP_RC_CONVERT(WSAESOCKTNOSUPPORT)))
#define ACP_RC_IS_ENOIMPL(e)      ((e) == ACP_RC_ENOIMPL)
#define ACP_RC_IS_EOF(e)          ((e) == ACP_RC_EOF)
#define ACP_RC_IS_EDLERR(e)       ((e) == ACP_RC_EDLERR)
#define ACP_RC_IS_ETRUNC(e)                                     \
    (((e) == ACP_RC_ETRUNC)                                     \
     || ((e) == WSAEMSGSIZE))
#define ACP_RC_IS_EFAULT(e)                                     \
    (((e) == ACP_RC_EFAULT)                                     \
     || ((e) == ACP_RC_CONVERT(WSAEFAULT)))
#define ACP_RC_IS_EBADMDL(e)      ((e) == ACP_RC_EBADMDL)
#define ACP_RC_IS_ECANCELED(e)    ((e) == ACP_RC_ECANCELED)
#define ACP_RC_IS_ECHILD(e)                                     \
    (((e) == ACP_RC_ECHILD)                                     \
     || ((e) == ACP_RC_CONVERT(ERROR_WAIT_NO_CHILDREN)))

/* Socket error checkers */
#define ACP_RC_IS_EPROTONOSUPPORT(e)    ((e) == ACP_RC_EPROTONOSUPPORT)
#define ACP_RC_IS_ENOTCONN(e)           ((e) == ACP_RC_ENOTCONN)
#define ACP_RC_IS_EISCONN(e)            ((e) == ACP_RC_EISCONN)
#define ACP_RC_IS_ENOPROTOOPT(e)        ((e) == ACP_RC_ENOPROTOOPT)
#define ACP_RC_IS_EADDRINUSE(e)         ((e) == ACP_RC_EADDRINUSE)
#define ACP_RC_IS_EALREADY(e)           ((e) == ACP_RC_EALREADY)
#define ACP_RC_IS_ELOOP(e)              ((e) == ACP_RC_ELOOP)
#define ACP_RC_IS_EADDRNOTAVAIL(e)      ((e) == ACP_RC_EADDRNOTAVAIL)
#define ACP_RC_IS_EOPNOTSUPP(e)         ((e) == ACP_RC_EOPNOTSUPP)
#define ACP_RC_IS_EDESTADDRREQ(e)       ((e) == ACP_RC_EDESTADDRREQ)
#define ACP_RC_IS_ENOLCK(e)             ((e) == ACP_RC_ENOCLK)
#define ACP_RC_IS_ESHUTDOWN(e)          ((e) == ACP_RC_ESHUTDOWN)
#define ACP_RC_IS_NOTINITIALIZED(e)     ((e) == WSANOTINITIALISED)
#define ACP_RC_IS_NETUNUSABLE(e)        ((e) == WSAENETDOWN)

/* for IPv6 error */
#define ACP_RC_IS_EAI_BADFLAGS(e)       ((e) == ACP_RC_EAI_BADFLAGS)
#define ACP_RC_IS_EAI_NONAME(e)         ((e) == ACP_RC_EAI_NONAME)
#define ACP_RC_IS_EAI_AGAIN(e)          ((e) == ACP_RC_EAI_AGAIN)
#define ACP_RC_IS_EAI_FAIL(e)           ((e) == ACP_RC_EAI_FAIL)
#define ACP_RC_IS_EAI_FAMILY(e)         ((e) == ACP_RC_EAI_FAMILY)
#define ACP_RC_IS_EAI_SOCKTYPE(e)       ((e) == ACP_RC_EAI_SOCKTYPE)
#define ACP_RC_IS_EAI_SERVICE(e)        ((e) == ACP_RC_EAI_SERVICE)
#define ACP_RC_IS_EAI_MEMORY(e)         ((e) == ACP_RC_EAI_MEMORY)
#define ACP_RC_IS_EAI_NODATA(e)         ((e) == ACP_RC_EAI_NODATA)

#else   /* Not in Windows */

#define ACP_RC_IS_SUCCESS(e)      ((e) == ACP_RC_SUCCESS)
#define ACP_RC_IS_EINVAL(e)       ((e) == ACP_RC_EINVAL)
#define ACP_RC_IS_EINTR(e)        ((e) == ACP_RC_EINTR)
#define ACP_RC_IS_ESRCH(e)        ((e) == ACP_RC_ESRCH)
#define ACP_RC_IS_ENAMETOOLONG(e) ((e) == ACP_RC_ENAMETOOLONG)
#define ACP_RC_IS_EEXIST(e)       ((e) == ACP_RC_EEXIST)
#if defined(EMVSCATLG)
#define ACP_RC_IS_ENOENT(e)       ((e) == ACP_RC_ENOENT || (e) == EMVSCATLG)
#else
#define ACP_RC_IS_ENOENT(e)       ((e) == ACP_RC_ENOENT)
#endif
#define ACP_RC_IS_ENOTEMPTY(e)                                  \
    ((e) == ACP_RC_ENOTEMPTY || (e) == ACP_RC_EEXIST)
#define ACP_RC_IS_ERANGE(e)       ((e) == ACP_RC_ERANGE)
#define ACP_RC_IS_EBUSY(e)        ((e) == ACP_RC_EBUSY)
#define ACP_RC_IS_EDEADLK(e)      ((e) == ACP_RC_EDEADLK)
#define ACP_RC_IS_EPERM(e)        ((e) == ACP_RC_EPERM)
#define ACP_RC_IS_EACCES(e)       ((e) == ACP_RC_EACCES || (e) == EBADF)
#if !defined(EWOULDBLOCK) || !defined(EAGAIN)
#define ACP_RC_IS_EAGAIN(e)       ((e) == ACP_RC_EAGAIN)
#elif (EWOULDBLOCK == EAGAIN)
#define ACP_RC_IS_EAGAIN(e)       ((e) == ACP_RC_EAGAIN)
#else
#define ACP_RC_IS_EAGAIN(e)                                     \
    ((e) == ACP_RC_EAGAIN || (e) == ACP_RC_EWOULDBLOCK)
#endif
#define ACP_RC_IS_EINPROGRESS(e)  ((e) == ACP_RC_EINPROGRESS)
#define ACP_RC_IS_ETIMEDOUT(e)    ((e) == ACP_RC_ETIMEDOUT)
#define ACP_RC_IS_EBADF(e)        ((e) == ACP_RC_EBADF)
#define ACP_RC_IS_ENOTDIR(e)      ((e) == ACP_RC_ENOTDIR)
#define ACP_RC_IS_ENOTSOCK(e)     ((e) == ACP_RC_ENOTSOCK)
#if defined(EDQUOT)
#define ACP_RC_IS_ENOSPC(e)                                     \
    ((e) == ACP_RC_ENOSPC || (e) == EDQUOT)
#else
#define ACP_RC_IS_ENOSPC(e)       ((e) == ACP_RC_ENOSPC)
#endif
#if defined(ENOBUFS)
#define ACP_RC_IS_ENOMEM(e)       ((e) == ACP_RC_ENOMEM || (e) == ENOBUFS)
#else
#define ACP_RC_IS_ENOMEM(e)       ((e) == ACP_RC_ENOMEM)
#endif
#define ACP_RC_IS_EMFILE(e)       ((e) == ACP_RC_EMFILE)
#define ACP_RC_IS_ENFILE(e)       ((e) == ACP_RC_ENFILE)
#define ACP_RC_IS_ESPIPE(e)       ((e) == ACP_RC_ESPIPE)
#define ACP_RC_IS_EPIPE(e)        ((e) == ACP_RC_EPIPE)
#define ACP_RC_IS_ECONNREFUSED(e) ((e) == ACP_RC_ECONNREFUSED)
#define ACP_RC_IS_ECONNABORTED(e)                               \
    ((e) == ACP_RC_ECONNABORTED || (e) == ACP_RC_EPROTO)
#define ACP_RC_IS_ECONNRESET(e)   ((e) == ACP_RC_ECONNRESET)
#define ACP_RC_IS_EHOSTUNREACH(e) ((e) == ACP_RC_EHOSTUNREACH)
#define ACP_RC_IS_ENETUNREACH(e)  ((e) == ACP_RC_ENETUNREACH)
#define ACP_RC_IS_EXDEV(e)        ((e) == ACP_RC_EXDEV)
#if defined(EAFNOSUPPORT)
#define ACP_RC_IS_ENOTSUP(e)      ((e) == ACP_RC_ENOTSUP || (e) == EAFNOSUPPORT)
#else
#define ACP_RC_IS_ENOTSUP(e)      ((e) == ACP_RC_ENOTSUP)
#endif
#define ACP_RC_IS_ENOIMPL(e)      ((e) == ACP_RC_ENOIMPL)
#define ACP_RC_IS_EOF(e)          ((e) == ACP_RC_EOF)
#define ACP_RC_IS_EDLERR(e)       ((e) == ACP_RC_EDLERR)
#define ACP_RC_IS_ETRUNC(e)       ((e) == ACP_RC_ETRUNC)
#define ACP_RC_IS_EFAULT(e)       ((e) == ACP_RC_EFAULT)
#define ACP_RC_IS_EBADMDL(e)      ((e) == ACP_RC_EBADMDL)
#define ACP_RC_IS_ECANCELED(e)    ((e) == ACP_RC_ECANCELED)
#define ACP_RC_IS_ECHILD(e)       ((e) == ACP_RC_ECHILD)

/* Socket error checkers */
#define ACP_RC_IS_EPROTONOSUPPORT(e)    ((e) == ACP_RC_EPROTONOSUPPORT)
#define ACP_RC_IS_ENOTCONN(e)           ((e) == ACP_RC_ENOTCONN)
#define ACP_RC_IS_EISCONN(e)            ((e) == ACP_RC_EISCONN)
#define ACP_RC_IS_ENOPROTOOPT(e)        ((e) == ACP_RC_ENOPROTOOPT)
#define ACP_RC_IS_EADDRINUSE(e)         ((e) == ACP_RC_EADDRINUSE)
#define ACP_RC_IS_EALREADY(e)           ((e) == ACP_RC_EALREADY)
#define ACP_RC_IS_ELOOP(e)              ((e) == ACP_RC_ELOOP)
#define ACP_RC_IS_EADDRNOTAVAIL(e)      ((e) == ACP_RC_EADDRNOTAVAIL)
#define ACP_RC_IS_EOPNOTSUPP(e)         ((e) == ACP_RC_EOPNOTSUPP)
#define ACP_RC_IS_EDESTADDRREQ(e)       ((e) == ACP_RC_EDESTADDRREQ)
#define ACP_RC_IS_ENOLCK(e)             ((e) == ACP_RC_ENOCLK)
#define ACP_RC_IS_ESHUTDOWN(e)          ((e) == ACP_RC_ESHUTDOWN)
#define ACP_RC_IS_NOTINITIALIZED(e)     (ACP_FALSE)
#define ACP_RC_IS_NETUNUSABLE(e)        (ACP_FALSE)

/* for IPv6 error */
#define ACP_RC_IS_EAI_BADFLAGS(e)       ((e) == ACP_RC_EAI_BADFLAGS)
#define ACP_RC_IS_EAI_NONAME(e)         ((e) == ACP_RC_EAI_NONAME)
#define ACP_RC_IS_EAI_AGAIN(e)          ((e) == ACP_RC_EAI_AGAIN)
#define ACP_RC_IS_EAI_FAIL(e)           ((e) == ACP_RC_EAI_FAIL)
#define ACP_RC_IS_EAI_FAMILY(e)         ((e) == ACP_RC_EAI_FAMILY)
#define ACP_RC_IS_EAI_SOCKTYPE(e)       ((e) == ACP_RC_EAI_SOCKTYPE)
#define ACP_RC_IS_EAI_SERVICE(e)        ((e) == ACP_RC_EAI_SERVICE)
#define ACP_RC_IS_EAI_MEMORY(e)         ((e) == ACP_RC_EAI_MEMORY)
#define ACP_RC_IS_EAI_SYSTEM(e)         ((e) == ACP_RC_EAI_SYSTEM)
#define ACP_RC_IS_EAI_OVERFLOW(e)        ((e) == ACP_RC_EAI_OVERFLOW)
#define ACP_RC_IS_EAI_ADDRFAMILY(e)     ((e) == ACP_RC_EAI_ADDRFAMILY)
#define ACP_RC_IS_EAI_NODATA(e)         ((e) == ACP_RC_EAI_NODATA)

#endif

/*
 * test NOT
 */
/**
 * test whether the result code is not #ACP_RC_SUCCESS
 */
#define ACP_RC_NOT_SUCCESS(e) (!ACP_RC_IS_SUCCESS(e))
/**
 * test whether the result code is not #ACP_RC_EINVAL or equivalent
 */
#define ACP_RC_NOT_EINVAL(e) (!ACP_RC_IS_EINVAL(e))
/**
 * test whether the result code is not #ACP_RC_EINTR or equivalent
 */
#define ACP_RC_NOT_EINTR(e) (!ACP_RC_IS_EINTR(e))
/**
 * test whether the result code is not #ACP_RC_ESRCH or equivalent
 */
#define ACP_RC_NOT_ESRCH(e) (!ACP_RC_IS_ESRCH(e))
/**
 * test whether the result code is not #ACP_RC_ENAMETOOLONG or equivalent
 */
#define ACP_RC_NOT_ENAMETOOLONG(e) (!ACP_RC_IS_ENAMETOOLONG(e))
/**
 * test whether the result code is not #ACP_RC_EEXIST or equivalent
 */
#define ACP_RC_NOT_EEXIST(e) (!ACP_RC_IS_EEXIST(e))
/**
 * test whether the result code is not #ACP_RC_ENOENT or equivalent
 */
#define ACP_RC_NOT_ENOENT(e) (!ACP_RC_IS_ENOENT(e))
/**
 * test whether the result code is not #ACP_RC_ENOTEMPTY or equivalent
 */
#define ACP_RC_NOT_ENOTEMPTY(e) (!ACP_RC_IS_ENOTEMPTY(e))
/**
 * test whether the result code is not #ACP_RC_ERANGE or equivalent
 */
#define ACP_RC_NOT_ERANGE(e) (!ACP_RC_IS_ERANGE(e))
/**
 * test whether the result code is not #ACP_RC_EBUSY or equivalent
 */
#define ACP_RC_NOT_EBUSY(e) (!ACP_RC_IS_EBUSY(e))
/**
 * test whether the result code is not #ACP_RC_EDEADLK or equivalent
 */
#define ACP_RC_NOT_EDEADLK(e) (!ACP_RC_IS_EDEADLK(e))
/**
 * test whether the result code is not #ACP_RC_EPERM or equivalent
 */
#define ACP_RC_NOT_EPERM(e) (!ACP_RC_IS_EPERM(e))
/**
 * test whether the result code is not #ACP_RC_EACCES or equivalent
 */
#define ACP_RC_NOT_EACCES(e) (!ACP_RC_IS_EACCES(e))
/**
 * test whether the result code is not #ACP_RC_EAGAIN or equivalent
 */
#define ACP_RC_NOT_EAGAIN(e) (!ACP_RC_IS_EAGAIN(e))
/**
 * test whether the result code is not #ACP_RC_EINPROGRESS or equivalent
 */
#define ACP_RC_NOT_EINPROGRESS(e) (!ACP_RC_IS_EINPROGRESS(e))
/**
 * test whether the result code is not #ACP_RC_ETIMEDOUT or equivalent
 */
#define ACP_RC_NOT_ETIMEDOUT(e) (!ACP_RC_IS_ETIMEDOUT(e))
/**
 * test whether the result code is not #ACP_RC_EBADF or equivalent
 */
#define ACP_RC_NOT_EBADF(e) (!ACP_RC_IS_EBADF(e))
/**
 * test whether the result code is not #ACP_RC_ENOTDIR or equivalent
 */
#define ACP_RC_NOT_ENOTDIR(e) (!ACP_RC_IS_ENOTDIR(e))
/**
 * test whether the result code is not #ACP_RC_ENOTSOCK or equivalent
 */
#define ACP_RC_NOT_ENOTSOCK(e) (!ACP_RC_IS_ENOTSOCK(e))
/**
 * test whether the result code is not #ACP_RC_ENOSPC or equivalent
 */
#define ACP_RC_NOT_ENOSPC(e) (!ACP_RC_IS_ENOSPC(e))
/**
 * test whether the result code is not #ACP_RC_ENOMEM or equivalent
 */
#define ACP_RC_NOT_ENOMEM(e) (!ACP_RC_IS_ENOMEM(e))
/**
 * test whether the result code is not #ACP_RC_EMFILE or equivalent
 */
#define ACP_RC_NOT_EMFILE(e) (!ACP_RC_IS_EMFILE(e))
/**
 * test whether the result code is not #ACP_RC_ENFILE or equivalent
 */
#define ACP_RC_NOT_ENFILE(e) (!ACP_RC_IS_ENFILE(e))
/**
 * test whether the result code is not #ACP_RC_ESPIPE or equivalent
 */
#define ACP_RC_NOT_ESPIPE(e) (!ACP_RC_IS_ESPIPE(e))
/**
 * test whether the result code is not #ACP_RC_EPIPE or equivalent
 */
#define ACP_RC_NOT_EPIPE(e) (!ACP_RC_IS_EPIPE(e))
/**
 * test whether the result code is not #ACP_RC_ECONNREFUSED or equivalent
 */
#define ACP_RC_NOT_ECONNREFUSED(e) (!ACP_RC_IS_ECONNREFUSED(e))
/**
 * test whether the result code is not #ACP_RC_ECONNABORTED or equivalent
 */
#define ACP_RC_NOT_ECONNABORTED(e) (!ACP_RC_IS_ECONNABORTED(e))
/**
 * test whether the result code is not #ACP_RC_ECONNRESET or equivalent
 */
#define ACP_RC_NOT_ECONNRESET(e) (!ACP_RC_IS_ECONNRESET(e))
/**
 * test whether the result code is not #ACP_RC_EHOSTUNREACH or equivalent
 */
#define ACP_RC_NOT_EHOSTUNREACH(e) (!ACP_RC_IS_EHOSTUNREACH(e))
/**
 * test whether the result code is not #ACP_RC_ENETUNREACH or equivalent
 */
#define ACP_RC_NOT_ENETUNREACH(e) (!ACP_RC_IS_ENETUNREACH(e))
/**
 * test whether the result code is not #ACP_RC_EXDEV or equivalent
 */
#define ACP_RC_NOT_EXDEV(e) (!ACP_RC_IS_EXDEV(e))
/**
 * test whether the result code is not #ACP_RC_ENOTSUP or equivalent
 */
#define ACP_RC_NOT_ENOTSUP(e) (!ACP_RC_IS_ENOTSUP(e))
/**
 * test whether the result code is not #ACP_RC_ENOIMPL or equivalent
 */
#define ACP_RC_NOT_ENOIMPL(e) (!ACP_RC_IS_ENOIMPL(e))
/**
 * test whether the result code is not #ACP_RC_EOF or equivalent
 */
#define ACP_RC_NOT_EOF(e) (!ACP_RC_IS_EOF(e))
/**
 * test whether the result code is not #ACP_RC_EDLERR or equivalent
 */
#define ACP_RC_NOT_EDLERR(e) (!ACP_RC_IS_EDLERR(e))
/**
 * test whether the result code is not #ACP_RC_ETRUNC or equivalent
 */
#define ACP_RC_NOT_ETRUNC(e) (!ACP_RC_IS_ETRUNC(e))
/**
 * test whether the result code is not #ACP_RC_EFAULT or equivalent
 */
#define ACP_RC_NOT_EFAULT(e) (!ACP_RC_IS_EFAULT(e))
/**
 * test whether the result code is not #ACP_RC_EBADMDL or equivalent
 */
#define ACP_RC_NOT_EBADMDL(e) (!ACP_RC_IS_EBADMDL(e))
/**
 * test whether the result code is not #ACP_RC_ECANCELED or equivalent
 */
#define ACP_RC_NOT_ECANCELED(e) (!ACP_RC_IS_ECANCELED(e))
/**
 * test whether the result code is not #ACP_RC_ECHILD or equivalent
 */
#define ACP_RC_NOT_ECHILD(e) (!ACP_RC_IS_ECHILD(e))
/**
 * test whether the result code is not #ACP_RC_EPROTONOSUPPORT or equivalent
 */
#define ACP_RC_NOT_EPROTONOSUPPORT(e) (!ACP_RC_IS_EPROTONOSUPPORT(e))
/**
 * test whether the result code is not #ACP_RC_ENOTCONN
 */
#define ACP_RC_NOT_ENOTCONN(e) (!ACP_RC_IS_ENOTCONN(e))
/**
 * test whether the result code is not #ACP_RC_EISCONN
 */
#define ACP_RC_NOT_EISCONN(e) (!ACP_RC_IS_EISCONN(e))
/**
 * test whether the result code is not #ACP_RC_ENOPROTOOPT
 */
#define ACP_RC_NOT_ENOPROTOOPT(e) (!ACP_RC_IS_ENOPROTOOPT(e))
/**
 * test whether the result code is not #ACP_RC_EADDRINUSE
 */
#define ACP_RC_NOT_EADDRINUSE(e) (!ACP_RC_IS_EADDRINUSE(e))
/**
 * test whether the result code is not #ACP_RC_EALREADY
 */
#define ACP_RC_NOT_EALREADY(e) (!ACP_RC_IS_EALREADY(e))
/**
 * test whether the result code is not #ACP_RC_ELOOP
 */
#define ACP_RC_NOT_ELOOP(e) (!ACP_RC_IS_ELOOP(e))
/**
 * test whether the result code is not #ACP_RC_EADDRNOTAVAIL
 */
#define ACP_RC_NOT_EADDRNOTAVAIL(e) (!ACP_RC_IS_EADDRNOTAVAIL(e))
/**
 * test whether the result code is not #ACP_RC_EOPNOTSUPP
 */
#define ACP_RC_NOT_EOPNOTSUPP(e) (!ACP_RC_IS_EOPNOTSUPP(e))
/**
 * test whether the result code is not #ACP_RC_EDESTADDRREQ
 */
#define ACP_RC_NOT_EDESTADDRREQ(e) (!ACP_RC_IS_EDESTADDRREQ(e))
/**
 * test whether the result code is not #ACP_RC_ENOLCK
 */
#define ACP_RC_NOT_ENOLCK(e) (!ACP_RC_IS_ENOLCK(e))
/**
 * test whether the result code is not #ACP_RC_ESHUTDOWN
 */
#define ACP_RC_NOT_ESHUTDOWN(e) (!ACP_RC_IS_ESHUTDOWN(e))
/**
 * test whether the result code does not represent that
 * an initialization have to come first
 */
#define ACP_RC_NOT_NOTINITIALIZED(e) (!ACP_RC_IS_NOTINITIALIZED(e))
/**
 * test whether the result code does not represent that
 * the network subsystem is not working
 */
#define ACP_RC_NOT_NETUNUSABLE(e)

/* for IPv6 error */
/**
 * test whether the result code does not represent that
 * Invalid value for `ai_flags' field.
 */
# define ACP_RC_NOT_EAI_BADFLAGS(e) (!ACP_RC_IS_EAI_BADFLAGS(e))
/**
 * test whether the result code does not represent that
 * NAME or SERVICE is unknown.
 */
# define ACP_RC_NOT_EAI_NONAME(e) (!ACP_RC_IS_EAI_NONAME(e))
/**
 * test whether the result code does not represent that
 * Temporary failure in name resolution.
 */
# define ACP_RC_NOT_EAI_AGAIN(e) (!ACP_RC_IS_EAI_AGAIN(e))
/**
 * test whether the result code does not represent that
 * Non-recoverable failure in name res.
 */
# define ACP_RC_NOT_EAI_FAIL(e) (!ACP_RC_IS_EAI_FAIL(e))
/**
 * test whether the result code does not represent that
 * `ai_family' not supported.
 */
# define ACP_RC_NOT_EAI_FAMILY(e) (!ACP_RC_IS_EAI_FAMILY(e))
/**
 * test whether the result code does not represent that
 * `ai_socktype' not supported.
 */
# define ACP_RC_NOT_EAI_SOCKTYPE(e) (!ACP_RC_IS_EAI_SOCKTYPE(e))
/**
 * test whether the result code does not represent that
 * SERVICE not supported for `ai_socktype'.
 */
# define ACP_RC_NOT_EAI_SERVICE(e) (!ACP_RC_IS_EAI_SERVICE(e))
/**
 * test whether the result code does not represent that
 * Memory allocation failure.
 */
# define ACP_RC_NOT_EAI_MEMORY(e) (!ACP_RC_IS_EAI_MEMORY(e))
/**
 * test whether the result code does not represent that
 * The specified network host exists, but does not have any
 * network addresses defined.
 */
#define ACP_RC_NOT_EAI_NODATA(e)         (!ACP_RC_IS_EAI_NODATA(e))

#if !defined(ALTI_CFG_OS_WINDOWS)
/**
 * test whether the result code does not represent that
 * System error returned in `errno'.
 */
# define ACP_RC_NOT_EAI_SYSTEM(e) (!ACP_RC_IS_EAI_SYSTEM(e))
/**
 * test whether the result code does not represent that
 * Argument buffer overflow.
 */
# define ACP_RC_NOT_EAI_OVERFLOW(e) (!ACP_RC_IS_EAI_OVERFLOW(e))
/**
 * test whether the result code does not represent that
 * The specified network host does not have any network
 * addresses in the requested address family
 */
# define ACP_RC_NOT_EAI_ADDRFAMILY(e)     (!ACP_RC_IS_EAI_ADDRFAMILY(e))
#endif


/*
 * copy description of error code
 */
ACP_EXPORT void acpErrorString(acp_rc_t    aRC,
                               acp_char_t *aBuffer,
                               acp_size_t  aSize);

ACP_EXTERN_C_END

#endif

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
 * $Id: acpSock.h 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_SOCK_H_)
#define _O_ACP_SOCK_H_

/**
 * @file
 * @ingroup CoreNet
 */

#include <acpTime.h>


ACP_EXTERN_C_BEGIN

#if defined(ACP_CFG_DOXYGEN)
/**
 * ACP_SOC_INADDR_INITIALIZER - struct in_addr - Initializer
 */
#  define ACP_SOCK_INADDR_INITIALIZER
/**
 * ACP_SOC_INADDR_SET - set aValue to aAddr in_addr structure
 */
#  define ACP_SOCK_INADDR_SET(aAddr, aValue)
/**
 * ACP_SOC_INADDR_INITIALIZER - get 32bit value in aAddr structure
 */
#  define ACP_SOCK_INADDR_GET(aAddr)

#elif defined(ALTI_CFG_OS_SOLARIS)
#  define ACP_SOCK_INADDR_INITIALIZER {{{0, 0, 0, 0}}}
#  define ACP_SOCK_INADDR_SET(aAddr, aValue) (aAddr._S_un._S_addr = aValue)
#  define ACP_SOCK_INADDR_GET(aAddr) ((acp_uint32_t)(aAddr._S_un._S_addr))

#elif defined(ALTI_CFG_OS_WINDOWS)
#  define ACP_SOCK_INADDR_INITIALIZER {{0, 0, 0, 0}}
#  define ACP_SOCK_INADDR_SET(aAddr, aValue) (aAddr.S_un.S_addr = aValue)
#  define ACP_SOCK_INADDR_GET(aAddr) ((acp_uint32_t)(aAddr.S_un.S_addr))

#else
#  define ACP_SOCK_INADDR_INITIALIZER {0}
#  define ACP_SOCK_INADDR_SET(aAddr, aValue) (aAddr.s_addr = aValue)
#  define ACP_SOCK_INADDR_GET(aAddr) ((acp_uint32_t)(aAddr.s_addr))

#endif

/**
 * IPv4 address family
 * @see acpSockOpen()
 */
#define ACP_AF_INET       AF_INET
#if defined(AF_INET6) || defined(ACP_CFG_DOXYGEN)
/**
 * IPv6 address family; currently not supported
 * @see acpSockOpen()
 */
#define ACP_AF_INET6      AF_INET6
#else
#define ACP_AF_INET6      26
#endif
/**
 * local Unix-Domain address family
 * @see acpSockOpen()
 */
#define ACP_AF_UNIX       AF_UNIX

/**
* Not specified address family
* @see acpSockOpen()
*/
#define ACP_AF_UNSPEC     AF_UNSPEC

/**
 * stream socket type
 * @see acpSockOpen()
 */
#define ACP_SOCK_STREAM   SOCK_STREAM
/**
 * datagram socket type
 * @see acpSockOpen()
 */
#define ACP_SOCK_DGRAM    SOCK_DGRAM



#if defined(ALTI_CFG_OS_WINDOWS)
#define ACP_SHUT_RD       SD_RECEIVE
#define ACP_SHUT_WR       SD_SEND
#define ACP_SHUT_RDWR     SD_BOTH
#else
/**
 * shutdown read part of a full-duplex connection
 * @see acpSockShutdown()
 */
#define ACP_SHUT_RD       SHUT_RD
/**
 * shutdown write part of a full-duplex connection
 * @see acpSockShutdown()
 */
#define ACP_SHUT_WR       SHUT_WR
/**
 * shutdown all part of a full-duplex connection
 * @see acpSockShutdown()
 */
#define ACP_SHUT_RDWR     SHUT_RDWR
#endif

/*
 * BUGBUG: should define socket option constants
 */


#if defined(MSG_DONTWAIT) || defined(ACP_CFG_DOXYGEN)
/**
 * requests non-blocking operation
 * @see acpSockRecv() acpSockRecvFrom() acpSockSend() acpSockSendTo()
 */
#define ACP_MSG_DONTWAIT  MSG_DONTWAIT
#elif defined(MSG_NONBLOCK)
#define ACP_MSG_DONTWAIT  MSG_NONBLOCK
#else
#define ACP_MSG_DONTWAIT  0x4000
#endif
/**
 * requests operation for out-of-band data
 * that whould not be transmitted in the normal data stream
 * @see acpSockRecv() acpSockRecvAll() acpSockRecvFrom()
 * acpSockSend() acpSockSendAll() acpSockSendTo()
 */
#define ACP_MSG_OOB       MSG_OOB
/**
 * causes the receive operation to return data
 * from the beginning of the receive queue
 * without removing that data from the queue.
 * thus, a subsequent receive call will return the same data
 * @see acpSockRecv() acpSockRecvFrom()
 */
#define ACP_MSG_PEEK      MSG_PEEK

/**
 * default flag value of sockets
 * disables non-blocking operation, out-of-band data,
 * and message peek.
 * @see acpSockRecv() acpSockRecvAll() acpSockRecvFrom()
 * acpSockSend() acpSockSendAll() acpSockSendTo()
 */
#define ACP_MSG_DEFAULT     (0)

#if defined(ALTI_CFG_OS_WINDOWS)

#define ACP_SOCK_ERROR          SOCKET_ERROR
#define ACP_SOCK_INVALID_HANDLE INVALID_SOCKET
#define ACP_SOCK_SIZE_TYPE      (acp_sint32_t)

#else

#define ACP_SOCK_ERROR          -1
#define ACP_SOCK_INVALID_HANDLE -1
#define ACP_SOCK_SIZE_TYPE

#endif


#if !defined(MSG_DONTWAIT) && !defined(MSG_NONBLOCK)
#define ACP_SOCK_DO_IO(aSock, aFlag, aEvent, aOpStmt)                       \
    do                                                                      \
    {                                                                       \
        if ((((aFlag) & ACP_MSG_DONTWAIT) != 0) &&                          \
            ((aSock)->mBlockMode == ACP_TRUE))                              \
        {                                                                   \
            acp_rc_t sPollRC;                                               \
                                                                            \
            sPollRC = acpSockPoll((aSock), (aEvent), ACP_TIME_IMMEDIATE);   \
                                                                            \
            if (ACP_RC_IS_SUCCESS(sPollRC))                                 \
            {                                                               \
                (aFlag) &= ~ACP_MSG_DONTWAIT;                               \
                aOpStmt;                                                    \
            }                                                               \
            else if (ACP_RC_IS_ETIMEDOUT(sPollRC))                          \
            {                                                               \
                return ACP_RC_EAGAIN;                                       \
            }                                                               \
            else                                                            \
            {                                                               \
                return sPollRC;                                             \
            }                                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            aOpStmt;                                                        \
        }                                                                   \
    } while (0)
#else
#define ACP_SOCK_DO_IO(aSock, aFlag, aEvent, aOpStmt) aOpStmt
#endif


#if defined(ALTI_CFG_OS_HPUX)
# if !defined(_HPUX_SOURCE)
/*
 * the way to avoid "redefinition erorr of ip_mreq"
 *
 * in case of HP11.23
 *    #ifndef _XOPEN_SOURCE_EXTENDED
 *        ... definition of struct ip_mreq ...
 *    #endif
 *
 *   _XOPEN_SOURCE_EXTENDED macro will hide ip_mreq definition away.
 *
 * in case of HP11.31
 *    #ifndef _HPUX_SOURCE
 *        ... definition of struct ip_mreq ...
 *    #endif
 *
 *    _HPUX_SOURCE macro will define ip_mreq structure.
 */
struct ip_mreq
{
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
};

# endif  /* end of _HPUX_SOURCE */
#endif   /* end of ALTI_CFG_OS_HPUX */


/**
 * socket object
 */
typedef struct acp_sock_t
{
#if defined(ALTI_CFG_OS_WINDOWS)
    SOCKET       mHandle;
#else
    acp_sint32_t mHandle;
#endif
    acp_bool_t   mBlockMode;
} acp_sock_t;

/**
 * socket address structure (struct sockaddr) <br>
 * <table width=100% border=0>
 * <tr><td> Member </td> <td> Description </td></tr>
 * <tr>
 * <td> sa_family    : </td><td> Socket address family. </td>
 * </tr> <tr>
 * <td>                </td><td> #ACP_AF_INET to use IPv4. </td>
 * </tr> <tr>
 * <td>                </td><td> #ACP_AF_INET6 to use IPv6. </td>
 * </tr> <tr>
 * <td>                </td><td> #ACP_AF_UNIX to use local
 *                               Unix-Domain address family.</td>
 * </tr> <tr>
 * <td> sa_data      : </td> <td> Socket data </td>
 * </tr>
 * </table>
 */
typedef struct sockaddr acp_sock_addr_t;

/**
 * socket address structure (struct sockaddr_in) <br>
 * <table width=100% border=0>
 * <tr><td> Member </td> <td> Description </td></tr>
 * <tr>
 * <td> sa_family    : </td><td> Socket address family. </td>
 * </tr><tr>
 * <td>                </td><td> #ACP_AF_INET to use IPv4. </td>
 * </tr><tr>
 * <td>                </td><td> #ACP_AF_INET6 to use IPv6. </td>
 * </tr><tr>
 * <td>                </td><td> #ACP_AF_UNIX to use local
 *                               Unix-Domain address family. </td>
 * </tr><tr>
 * <td> sin_addr      : </td><td>Socket address.
 *                      sin_addr.s_addr can be used.</td>
 * </tr><tr>
 * <td>                 </td><td> To assign an address, thou hash to
 *                  assign a value converted with #ACP_TON_BYTE4(). </td>
 * </tr><tr>
 * <td> sin_port      : </td><td>Socket port. To set a port number,
 *                  thou hast assign a value converted with #ACP_TON_BYTE2()
 *                  </td>
 * </tr>
 * </table>
 */
typedef struct sockaddr_in acp_sock_addr_in_t;

/**
 * socket address structure for Unix-Domain.
 *
 * @see #acp_sock_addr_in_t
 */
typedef struct sockaddr_un acp_sock_addr_un_t;

/**
 * socket address structure for IPv6
 *
 * @see #acp_sock_addr_in_t
 */
#if !defined(AF_INET6) && !defined(ACP_INET6_DEFINED)

#define ACP_INET6_DEFINED

struct addrinfo
{
  acp_sint32_t		ai_flags;	/* Input flags.  */
  acp_sint32_t		ai_family;	/* Protocol family for socket.  */
  acp_sint32_t		ai_socktype;	/* Socket type.  */
  acp_sint32_t		ai_protocol;	/* Protocol for socket.  */
  socklen_t		ai_addrlen;	/* Length of socket address.  */
  struct sockaddr*	ai_addr;	/* Socket address for socket.  */
  char*			ai_canonname;	/* Canonical name for service location.  */
  struct addrinfo*	ai_next;	/* Pointer to next in list.  */
};

struct in6_addr {
    union {
        acp_uint8_t  u6_addr8[16];
        acp_uint16_t u6_addr16[8];
        acp_uint32_t u6_addr32[4];
    } in6_u;
#define s6_addr            in6_u.u6_addr8
#define s6_addr16        in6_u.u6_addr16
#define s6_addr32        in6_u.u6_addr32
};

struct sockaddr_in6 {
    acp_uint16_t      sin6_family;    /* AF_INET6 */
    acp_uint16_t      sin6_port;      /* Transport layer port # */
    acp_uint32_t      sin6_flowinfo;  /* IPv6 flow information */
    struct in6_addr   sin6_addr;      /* IPv6 address */
    acp_uint32_t      sin6_scope_id;  /* scope id (new in RFC2553) */
};

struct sockaddr_storage
{
    acp_uint16_t     ss_family; /* Address family, etc.  */
    acp_uint32_t     ss_align;
    acp_uint8_t      ss_padding[128 - sizeof(acp_uint32_t)*2];
};

#endif

typedef struct sockaddr_in6 acp_sock_addr_in6_t;

/**
 * socket address structure common for IPv6 and IPv4
 */
typedef struct sockaddr_storage acp_sock_addr_storage_t;
  

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)
typedef acp_sint32_t acp_sock_len_t;
#else
/**
 * length type for socket address, name, or option
 */
typedef socklen_t acp_sock_len_t;
#endif


/**
 * IPv6 address
 */
typedef struct in6_addr acp_sock_in6_addr_t;

/**
 * IPv4 multicast request
 */
typedef struct ip_mreq acp_ip_mreq_t;

/**
 * IPv6 multicast request
 */
typedef struct ipv6_mreq acp_ipv6_mreq_t;


acp_rc_t acpSockInitialize(void);
acp_rc_t acpSockFinalize(void);


/*
 * open/close
 */
ACP_EXPORT acp_rc_t acpSockOpen(acp_sock_t   *aSock,
                                acp_sint32_t  aFamily,
                                acp_sint32_t  aType,
                                acp_sint32_t  aProtocol);
ACP_EXPORT acp_rc_t acpSockClose(acp_sock_t *aSock);
ACP_EXPORT acp_rc_t acpSockShutdown(acp_sock_t *aSock, acp_sint32_t aHow);

/*
 * option
 */
ACP_EXPORT acp_rc_t acpSockGetBlockMode(acp_sock_t *aSock,
                                        acp_bool_t *aBlockMode);
ACP_EXPORT acp_rc_t acpSockSetBlockMode(acp_sock_t *aSock,
                                        acp_bool_t  aBlockMode);

ACP_EXPORT acp_rc_t acpSockGetOpt(acp_sock_t     *aSock,
                                  acp_sint32_t    aLevel,
                                  acp_sint32_t    aOptName,
                                  void           *aOptVal,
                                  acp_sock_len_t *aOptLen);
ACP_EXPORT acp_rc_t acpSockSetOpt(acp_sock_t     *aSock,
                                  acp_sint32_t    aLevel,
                                  acp_sint32_t    aOptName,
                                  const void     *aOptVal,
                                  acp_sock_len_t  aOptLen);

/*
 * poll event
 */
ACP_EXPORT acp_rc_t acpSockPoll(acp_sock_t   *aSock,
                                acp_sint32_t  aEvent,
                                acp_time_t    aTimeout);

/*
 * client socket
 */
ACP_EXPORT acp_rc_t acpSockConnect(acp_sock_t      *aSock,
                                   acp_sock_addr_t *aAddr,
                                   acp_sock_len_t   aAddrLen);
ACP_EXPORT acp_rc_t acpSockTimedConnect(acp_sock_t      *aSock,
                                        acp_sock_addr_t *aAddr,
                                        acp_sock_len_t   aAddrLen,
                                        acp_time_t       aTimeout);

/*
 * server socket
 */
ACP_EXPORT acp_rc_t acpSockBind(acp_sock_t            *aSock,
                                const acp_sock_addr_t *aAddr,
                                acp_sock_len_t         aAddrLen,
                                acp_bool_t             aReuseAddr);
ACP_EXPORT acp_rc_t acpSockListen(acp_sock_t   *aSock,
                                  acp_sint32_t  aBackLog);
ACP_EXPORT acp_rc_t acpSockAccept(acp_sock_t      *aAcceptSock,
                                  acp_sock_t      *aListenSock,
                                  acp_sock_addr_t *aAddr,
                                  acp_sock_len_t  *aAddrLen);

/*
 * name
 */
ACP_EXPORT acp_rc_t acpSockGetName(acp_sock_t      *aSock,
                                   acp_sock_addr_t *aName,
                                   acp_sock_len_t  *aNameLen);
ACP_EXPORT acp_rc_t acpSockGetPeerName(acp_sock_t      *aSock,
                                       acp_sock_addr_t *aName,
                                       acp_sock_len_t  *aNameLen);

/*
 * recv/send
 */
ACP_EXPORT acp_rc_t acpSockRecv(acp_sock_t   *aSock,
                                void         *aBuffer,
                                acp_size_t    aSize,
                                acp_size_t   *aRecvSize,
                                acp_sint32_t  aFlag);
ACP_EXPORT acp_rc_t acpSockSend(acp_sock_t   *aSock,
                                const void   *aBuffer,
                                acp_size_t    aSize,
                                acp_size_t   *aSendSize,
                                acp_sint32_t  aFlag);
ACP_EXPORT acp_rc_t acpSockRecvAll(acp_sock_t   *aSock,
                                   void         *aBuffer,
                                   acp_size_t    aSize,
                                   acp_size_t   *aRecvSize,
                                   acp_sint32_t  aFlag,
                                   acp_time_t    aTimeout);
ACP_EXPORT acp_rc_t acpSockSendAll(acp_sock_t   *aSock,
                                   const void   *aBuffer,
                                   acp_size_t    aSize,
                                   acp_size_t   *aSendSize,
                                   acp_sint32_t  aFlag,
                                   acp_time_t    aTimeout);

ACP_EXPORT acp_rc_t acpSockRecvFrom(acp_sock_t      *aSock,
                                    void            *aBuffer,
                                    acp_size_t       aSize,
                                    acp_size_t      *aRecvSize,
                                    acp_sint32_t     aFlag,
                                    acp_sock_addr_t *aAddr,
                                    acp_sock_len_t  *aAddrLen);
ACP_EXPORT acp_rc_t acpSockSendTo(acp_sock_t            *aSock,
                                  const void            *aBuffer,
                                  acp_size_t             aSize,
                                  acp_size_t            *aSendSize,
                                  acp_sint32_t           aFlag,
                                  const acp_sock_addr_t *aAddr,
                                  acp_sock_len_t         aAddrLen);


ACP_EXTERN_C_END


#endif

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
 * $Id: acpInet.h 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_INET_H_)
#define _O_ACP_INET_H_

/**
 * @file
 * @ingroup CoreNet
 */

#include <acpSock.h>

ACP_EXTERN_C_BEGIN

/**
 * host entry
 */
typedef struct hostent acp_inet_hostent_t;

/**
 * address information
 */
typedef struct addrinfo acp_inet_addr_info_t;

#if !defined(AF_INET6)

#define ACP_INET_NI_MAXHOST 1025
#define ACP_INET_NI_MAXSERV 32

#define ACP_INET_NI_NAMEREQD 	0x01
#define ACP_INET_NI_DGRAM 	0x04
#define ACP_INET_NI_NOFQDN 	0x08
#define ACP_INET_NI_NUMERICHOST 0x10
#define ACP_INET_NI_NUMERICSERV 0x20

#define ACP_INET_AI_PASSIVE 	0x01
#define ACP_INET_AI_CANONNAME 	0x02
#define ACP_INET_AI_NUMERICHOST 0x04
#define ACP_INET_AI_NUMERICSERV 0x08
#define ACP_INET_AI_V4MAPPED 	0x10
#define ACP_INET_AI_ALL 	0x20
#define ACP_INET_AI_ADDRCONFIG  0x40

#define ACP_INET_IN6_IS_ADDR_UNSPECIFIED(a)   ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_LOOPBACK(a)      ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_MULTICAST(a)     ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_LINKLOCAL(a)     ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_SITELOCAL(a)     ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_V4MAPPED(a)      ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_V4COMPAT(a)      ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_MC_NODELOCAL(a)  ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_MC_LINKLOCAL(a)  ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_MC_SITELOCAL(a)  ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_MC_ORGLOCAL(a)   ACP_FALSE
#define ACP_INET_IN6_IS_ADDR_MC_GLOBAL(a)     ACP_FALSE

#define ACP_INET_IN6ADDR_ANY_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } } }
#define ACP_INET_IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }

#define ACP_INET_INET_ADDRSTRLEN 16
#define ACP_INET_INET6_ADDRSTRLEN 46

#define ACP_INET_IP_ADDR_MAX_LEN 64
#define ACP_INET_IP_PORT_MAX_LEN 8
#define ACP_INET_IPPROTO_IPV6    41

#define ACP_INET_IPV6_JOIN_GROUP  0
#define ACP_INET_IPV6_LEAVE_GROUP 0
#define ACP_INET_IPV6_MULTICAST_HOPS 0
#define ACP_INET_IPV6_MULTICAST_IF 0
#define ACP_INET_IPV6_MULTICAST_LOOP 0
#define ACP_INET_IPV6_UNICAST_HOPS 0

static const struct in6_addr in6addr_any = ACP_INET_IN6ADDR_ANY_INIT;

#else /* AF_INET6 */

/* max sizes for buffer supplied for
 * acpInetGetNameInfo and acpInetGetAddrInfo */
/**
 * max size of buffer to get host-name
 *
 * @see acpInetAddrInfo(), acpInetNameInfo()
 */
#define ACP_INET_NI_MAXHOST 1025

/**
 * max size of buffer to get service-name
 *
 * @see acpInetAddrInfo(), acpInetNameInfo()
 */
#define ACP_INET_NI_MAXSERV 32

/*
 * flags for getnameinfo
 */
/**
 * an error is returned if the hostname cannot be determined
 *
 * @see acpInetNameInfo()
 */
#define ACP_INET_NI_NAMEREQD NI_NAMEREQD
/**
 * the service is datagram (UDP) based
 *
 * @see acpInetNameInfo()
 */
#define ACP_INET_NI_DGRAM NI_DGRAM
/**
 * return only the hostname part of the fully qualified
 * domain name for local hosts
 *
 * @see acpInetNameInfo()
 */
#define ACP_INET_NI_NOFQDN NI_NOFQDN
/**
 * the numeric form of the hostname  is  returned
 *
 * @see acpInetNameInfo()
 */
#define ACP_INET_NI_NUMERICHOST NI_NUMERICHOST
/**
 * the numeric form  of  the  service  address  is returned
 *
 * @see acpInetNameInfo()
 */
#define ACP_INET_NI_NUMERICSERV NI_NUMERICSERV

/*
 * flags for getaddrinfo
 */
/**
 * Socket address is intended for `bind'.
 *
 * @see acpInetAddrInfo()
 */
#define ACP_INET_AI_PASSIVE AI_PASSIVE
/**
 * Request for canonical name.
 *
 * @see acpInetAddrInfo()
 */
#define ACP_INET_AI_CANONNAME AI_CANONNAME
/**
 * Don't use name resolution.
 *
 * @see acpInetAddrInfo()
 */
#define ACP_INET_AI_NUMERICHOST AI_NUMERICHOST

/**
 * Don't use service resolution.
 *
 * @see acpInetAddrInfo()
 */
#if defined (ALTI_CFG_OS_WINDOWS) && !defined (AI_NUMERICSERV)
/*
 * The AI_NUMERICSERV flag is defined on Windows SDK for Windows Vista 
 * and later. It is not defined for Windows 2003 so make fake definition.
 */
# define ACP_INET_AI_NUMERICSERV 0
#elif defined (ALTI_CFG_OS_SOLARIS) && !defined (AI_NUMERICSERV)
/*
 * also for SUN OS 2.8
 */
# define ACP_INET_AI_NUMERICSERV 0
#else
# define ACP_INET_AI_NUMERICSERV AI_NUMERICSERV
#endif

#if !defined(ALTI_CFG_OS_WINDOWS)
/**
 * IPv4 mapped addresses are acceptable.
 *
 * @see acpInetAddrInfo()
 */
#define ACP_INET_AI_V4MAPPED AI_V4MAPPED
/**
 * Return IPv4 mapped and IPv6 addresses.
 *
 * @see acpInetAddrInfo()
 */
# define ACP_INET_AI_ALL AI_ALL
/**
 * Use configuration of this host to choose returned address type..
 *
 * @see acpInetAddrInfo()
 */
# define ACP_INET_AI_ADDRCONFIG AI_ADDRCONFIG
#endif


/*
 * test macros for IPv6 address
 */

/**
 * test empty IPv6 address
 */
#define ACP_INET_IN6_IS_ADDR_UNSPECIFIED IN6_IS_ADDR_UNSPECIFIED
/**
 * test loop-back IPv6 address
 */
#define ACP_INET_IN6_IS_ADDR_LOOPBACK IN6_IS_ADDR_LOOPBACK
/**
 * test multicasting IPv6 address
 */
/*
 * BUG-29972
 * IN6_IS_ADDR_MULTICAST of HPUX seems to be buggy.
 * This code is borrowed from LINUX.
 */
#if defined(ALTI_CFG_OS_HPUX)
#define ACP_INET_IN6_IS_ADDR_MULTICAST(aAddr) \
    (((const acp_uint8_t *)(aAddr))[0] == 0xff)
#else
#define ACP_INET_IN6_IS_ADDR_MULTICAST IN6_IS_ADDR_MULTICAST
#endif
/**
 * test link local IPv6 address
 */
#define ACP_INET_IN6_IS_ADDR_LINKLOCAL IN6_IS_ADDR_LINKLOCAL
/**
 * test site local IPv6 address
 */
#define ACP_INET_IN6_IS_ADDR_SITELOCAL IN6_IS_ADDR_SITELOCAL
/**
 * test IPv4-mapped IPv6 address
 */
#define ACP_INET_IN6_IS_ADDR_V4MAPPED IN6_IS_ADDR_V4MAPPED
/**
 * test IPv4-compatible IPv6 address
 */
#define ACP_INET_IN6_IS_ADDR_V4COMPAT IN6_IS_ADDR_V4COMPAT
/**
 * test IPv6 multicast address with node local scope
 */
#define ACP_INET_IN6_IS_ADDR_MC_NODELOCAL IN6_IS_ADDR_MC_NODELOCAL
/**
 * test IPv6 multicast address with link local scope
 */
#define ACP_INET_IN6_IS_ADDR_MC_LINKLOCAL IN6_IS_ADDR_MC_LINKLOCAL
/**
 * test IPv6 multicast address with site local scope
 */
#define ACP_INET_IN6_IS_ADDR_MC_SITELOCAL IN6_IS_ADDR_MC_SITELOCAL
/**
 * test IPv6 multicast address with organization local scope
 */
#define ACP_INET_IN6_IS_ADDR_MC_ORGLOCAL IN6_IS_ADDR_MC_ORGLOCAL
/**
 * test IPv6 multicast address with global scope
 */
#define ACP_INET_IN6_IS_ADDR_MC_GLOBAL IN6_IS_ADDR_MC_GLOBAL


/*
 * pre-defined values for IPv6
 */
/**
 * emptry IPv6 address
 */
#define ACP_INET_IN6ADDR_ANY_INIT IN6ADDR_ANY_INIT

/**
 * loop-back IPv6 address
 */
#define ACP_INET_IN6ADDR_LOOPBACK_INIT IN6ADDR_LOOPBACK_INIT
/**
 * maximum length of string that prints IPv6 address
 */
#define ACP_INET_INET6_ADDRSTRLEN INET6_ADDRSTRLEN
/**
 * maximum length of string that prints IPv4 address
 */
#define ACP_INET_INET_ADDRSTRLEN INET_ADDRSTRLEN

/**
 * maximum length of string that prints IPv4 or Ipv6 address
 */
#define ACP_INET_IP_ADDR_MAX_LEN 64

/**
 * maximum length of string that prints IP port
 */
#define ACP_INET_IP_PORT_MAX_LEN 8

/**
 * IPv6 protocol header
 */
#define ACP_INET_IPPROTO_IPV6 IPPROTO_IPV6


/*
 * options for getsockopt and setsockopt
 */
#define ACP_INET_IPV6_JOIN_GROUP IPV6_JOIN_GROUP
#define ACP_INET_IPV6_LEAVE_GROUP IPV6_LEAVE_GROUP
#define ACP_INET_IPV6_MULTICAST_HOPS IPV6_MULTICAST_HOPS
#define ACP_INET_IPV6_MULTICAST_IF IPV6_MULTICAST_IF
#define ACP_INET_IPV6_MULTICAST_LOOP IPV6_MULTICAST_LOOP
#define ACP_INET_IPV6_UNICAST_HOPS IPV6_UNICAST_HOPS

#endif

/*
 * address conversion functions
 *
 * inet_addr()  string -> binary (obsolete)
 * inet_aton()  string -> binary (obsolete)
 * inet_pton()  string -> binary
 * inet_ntoa()  binary -> string (obsolete)
 * inet_ntop()  binary -> string
 *
 * inet_aton(), inet_pton(), inet_ntop() is not supported by HPUX(11.00-),
 * Solaris(2.7-), TRU64(4.0-)
 * but, above platforms support thread safe version of inet_ntoa().
 * in the platforms, which support newer interface,
 * inet_ntoa() may not be thread safe.
 *
 * TRU64(5.1) support inet_pton(), inet_ntop(), but not inet_aton().
 *
 * WSAStringToAddress(), WSAAddressToString() is equivalent to
 * inet_pton(), inet_ntop() repectively in Windows
 */

ACP_EXPORT acp_rc_t acpInetAddrToStr(acp_sint32_t  aFamily,
                                     const void   *aAddr,
                                     acp_char_t   *aStr,
                                     acp_size_t    aBufLen);
ACP_EXPORT acp_rc_t acpInetStrToAddr(acp_sint32_t  aFamily,
                                     acp_char_t   *aStr,
                                     void         *aAddr);


/*
 * address/name lookup functions
 *
 * gethostbyname(), gethostbyaddr() is thread safe in AIX, HPUX, TRU64, Windows.
 * in Linux and Solaris, gethostbyname_r(), gethostbyaddr_r() should be used.
 *
 * getaddrinfo(), getnameinfo() is newer interface for
 * gethostbyname(), gethostbyaddr() respectively.
 *
 * getaddrinfo(), getnameinfo() is supported by AIX, HPUX(11.11+), Linux,
 * Solaris(2.8+), Windows
 */

ACP_EXPORT acp_rc_t acpInetGetHostByAddr(acp_inet_hostent_t **aHostEnt,
                                         acp_sock_addr_in_t *aAddr,
                                         acp_sock_len_t aLen,
                                         acp_sint32_t aType);
ACP_EXPORT acp_rc_t acpInetGetHostByName(acp_inet_hostent_t **aHostEnt,
                                         const acp_char_t *aName);

/*
 * BUG-30166 Changed acpInetGetAddrInfo function.
 * Added aServName and aSockType parameters.
 * Removed aSockType parameter.
 */
ACP_EXPORT acp_rc_t acpInetGetAddrInfo(acp_inet_addr_info_t **aInfo,
                                       acp_char_t   *aNodeName,
                                       acp_char_t   *aServName,
                                       acp_sint32_t aSockType,
                                       acp_sint32_t aFlag,
                                       acp_sint32_t aFamily);

ACP_EXPORT void acpInetFreeAddrInfo(acp_inet_addr_info_t *aInfo);

ACP_EXPORT acp_rc_t acpInetGetNameInfo(const acp_sock_addr_t *aSockAddr,
                                       acp_size_t aSockLen,
                                       acp_char_t *aNameBuf,
                                       acp_size_t aNameBufLen,
                                       acp_sint32_t aFlag);
ACP_EXPORT acp_rc_t acpInetGetServInfo(const acp_sock_addr_t *aSockAddr,
                                       acp_size_t aSockLen,
                                       acp_char_t *aServBuf,
                                       acp_size_t aServBufLen,
                                       acp_sint32_t aFlag);

ACP_EXPORT acp_rc_t acpInetGetStrError(acp_sint32_t  aErrCode,
                                       acp_char_t  **aErrStr);

ACP_EXTERN_C_END


#endif

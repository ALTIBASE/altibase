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
 * $Id: acpSys.h 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_SYS_H_)
#define _O_ACP_SYS_H_

/**
 * @file
 * @ingroup CoreSys
 */

#include <acpError.h>
#include <acpTime.h>
#include <acpStr.h>


ACP_EXTERN_C_BEGIN


#if !defined(ALTI_CFG_OS_WINDOWS)
#define ACP_SYS_NOLIMIT RLIM_INFINITY
#endif

/**
 * length of mac address
 */
#define ACP_SYS_MAC_ADDR_LEN 6

/**
 * mac address structure
 */
typedef struct acp_mac_addr_t
{
    acp_bool_t  mValid;                      /**< whether this entry is valid */
    acp_bool_t  mDummy;                      /**< unused (just for alignment) */
    acp_uint8_t mAddr[ACP_SYS_MAC_ADDR_LEN]; /**< mac address                 */
} acp_mac_addr_t;

/**
 * ip address structure
 */
typedef struct acp_ip_addr_t
{
    acp_bool_t     mValid;  /**< whether this entry is valid */
    acp_uint16_t   mFamily; /**< inet address family         */
    struct in_addr mAddr;   /**< ip address                  */
    struct in_addr mBrdAddr;/**< broadcast address           */
} acp_ip_addr_t;

#if !defined(AF_INET6) && !defined(ACP_INET6_DEFINED)

#define ACP_INET6_DEFINED

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

#endif

/**
 * ipv6 address structure
 */
typedef struct acp_ipv6_addr_t
{
    acp_bool_t     mValid;  /**< whether this entry is valid */
    acp_uint16_t   mFamily; /**< inet address family         */
    struct in6_addr mAddr6; /**< ipv6 address                */
} acp_ipv6_addr_t;


/**
 * cpu times information
 */
typedef struct acp_cpu_times_t
{
    acp_time_t mUserTime;   /**< user time   */
    acp_time_t mSystemTime; /**< system time */
} acp_cpu_times_t;


/*
 * resource limit
 */
ACP_EXPORT acp_rc_t acpSysGetHandleLimit(acp_uint32_t *aHandleLimit);
ACP_EXPORT acp_rc_t acpSysSetHandleLimit(acp_uint32_t aHandleLimit);

/*
 * system information
 */
ACP_EXPORT acp_rc_t acpSysGetCPUCount(acp_uint32_t *aCount);

ACP_EXPORT acp_rc_t acpSysGetPageSize(acp_size_t *aPageSize);

ACP_EXPORT acp_rc_t acpSysGetBlockSize(acp_size_t *aBlockSize);

ACP_EXPORT acp_rc_t acpSysGetHostName(acp_char_t *aBuffer, acp_size_t aSize);

ACP_EXPORT acp_rc_t acpSysGetUserName(acp_char_t *aBuffer, acp_size_t aSize);

ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount);

ACP_EXPORT acp_rc_t acpSysGetIPAddress(acp_ip_addr_t *aIPAddr,
                                       acp_uint32_t   aCount,
                                       acp_uint32_t  *aRealCount);
ACP_EXPORT acp_rc_t acpSysGetIPv6Address(acp_ipv6_addr_t *aIPv6Addr,
                                         acp_uint32_t   aCount,
                                         acp_uint32_t  *aRealCount);

/*
 * process information
 */
ACP_EXPORT acp_rc_t acpSysGetCPUTimes(acp_cpu_times_t *aCpuTimes);

#define ACP_SYS_ID_MAXSIZE 1024

/**
 * acpSysGetHardwareID : Get hardware ID
 * @param aID Pointer to store Hardware ID
 * @param aBufLen length of @a aID buffer
 * @return ACP_RC_SUCCESS when aID successfully got.
 * ACP_RC_ENOIMPL when the OS is not supported
 * other when system error.
 */
ACP_EXPORT acp_rc_t acpSysGetHardwareID(
    acp_char_t* aID,
    acp_size_t  aBufLen );

ACP_EXTERN_C_END

#endif

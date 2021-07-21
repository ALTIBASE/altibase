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
 * $Id: acpSysIPAddr-UNIX.c 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#include <acpMem.h>
#include <acpPrintf.h>
#include <acpSock.h>
#include <acpSys.h>
#include <acpTimePrivate.h>
#include <acpTest.h>
#include <acpCStr.h>
#include <acpSysUtil.h>

#if !defined(ALTI_CFG_OS_WINDOWS)
#if defined(ACP_CFG_DOXYGEN)
/**
 * gets ip address
 *
 * @param aIPAddr array of #acp_ip_addr_t to get ip address
 * @param aCount array size
 * @param aRealCount pointer to a variable to store number of ip addresses
 * @return result code
 *
 * A platform can have many IPs, aRealCount must be checked to get
 * the count of IPs. And if the aRealCount value is bigger than you expected,
 * you have to call the function again to read every IPs.
 */
ACP_EXPORT acp_rc_t acpSysGetIPAddress(acp_ip_addr_t *aIPAddr,
                                       acp_uint32_t   aCount,
                                       acp_uint32_t  *aRealCount)
{
    return ACP_RC_SUCCESS;
}

#elif defined(ALTI_CFG_OS_FREEBSD) || defined(ALTI_CFG_OS_DARWIN)

ACP_EXPORT acp_rc_t acpSysGetIPAddress(acp_ip_addr_t *aIPAddr,
                                       acp_uint32_t   aCount,
                                       acp_uint32_t  *aRealCount)
{
    struct ifaddrs*     sIfs = NULL;
    struct ifaddrs*     sHead = NULL;
    struct sockaddr_in* sIn = NULL;
    struct sockaddr_in* sInBrd = NULL;
    acp_size_t          sCount;
    acp_rc_t            sRC = ACP_RC_SUCCESS;
    acp_bool_t          sFlagCopyAddr = ACP_TRUE;

    ACP_TEST_RAISE(0 != getifaddrs(&sIfs), GETIF_FAILED);
    sHead = sIfs;
    for(sCount = 0; NULL != sIfs;)
    {
        if(sCount >= aCount)
        {
            /* over user-requested count,
             * so do not copy data 
             */
            sFlagCopyAddr = ACP_FALSE;
        }
        else
        {
            /* Fall through */
        }

        /* Getting IP Addresses - No need of pass loopback
        Fall through and get IP address */

        sIn = (struct sockaddr_in*)sIfs->ifa_addr;
        sInBrd = (struct sockaddr_in*)sIfs->ifa_broadaddr;
        if((NULL != sIn) &&
           (NULL != sInBrd) &&
           (AF_INET == sIn->sin_family))
        {
            if (sFlagCopyAddr == ACP_TRUE)
            {
                /* Fill aIPAddr */
                aIPAddr->mFamily = sIn->sin_family;
                acpMemCpy(&(aIPAddr->mAddr), &(sIn->sin_addr),
                          sizeof(struct in_addr));
                acpMemCpy(&(aIPAddr->mBrdAddr), &(sInBrd->sin_addr),
                          sizeof(struct in_addr));

                /* Increase the count of IP address */
                aIPAddr->mValid = ACP_TRUE;
                aIPAddr++;
            }
            else
            {
                /* do not copy */
            }
        
            sCount++;
        }
        else
        {
            /* Do nothing */
        }

        sIfs = sIfs->ifa_next;
    }

    if(NULL != aRealCount)
    {
        *aRealCount = sCount;
    }
    else
    {
        /* Do nothing */
    }

    for (; sCount < aCount; sCount++)
    {
        aIPAddr->mValid = ACP_FALSE;
        aIPAddr++;
    }

    freeifaddrs(sHead);
    return sRC;


    ACP_EXCEPTION(GETIF_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    return sRC;
}

#else

ACP_EXPORT acp_rc_t acpSysGetIPAddress(acp_ip_addr_t *aIPAddr,
                                       acp_uint32_t   aCount,
                                       acp_uint32_t  *aRealCount)
{
    acp_char_t    *sPtr = NULL;
    acp_sint32_t   sSocket;
    acp_sint32_t   sRet;
    struct ifconf  sIfConf;
    struct ifreq  *sIfReq = NULL;
    acp_uint32_t   sCount;
    acp_uint32_t   i;
    acp_rc_t       sRC;

    sSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (sSocket == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRC = acpSysGetIfConf(sSocket, &sIfConf);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)close(sSocket);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sCount = 0;

    for (sPtr = (acp_char_t *)sIfConf.ifc_req;
         sPtr < (acp_char_t *)sIfConf.ifc_req + sIfConf.ifc_len;
         sPtr += sizeof(struct ifreq))
    {
        struct ifreq   sIfReqIP; /* to check AF_INET, AF_INET6 */
        struct ifreq   sIfReqBrd;
        struct in_addr sAddr;
        struct in_addr sBrdAddr;

        sIfReq = (struct ifreq *)sPtr;
            
        acpMemCpy(sIfReqIP.ifr_name, sIfReq->ifr_name, IFNAMSIZ);
        acpMemCpy(sIfReqBrd.ifr_name, sIfReq->ifr_name, IFNAMSIZ);
            
        sAddr = ((struct sockaddr_in *)&sIfReq->ifr_addr)->sin_addr;

#if defined (ALTI_CFG_OS_AIX)
        /* AIX returns lo0 twice, 
         * but only one lo0 has IP and it is real interface.*/
        if (sAddr.s_addr == 0)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }
#elif defined(ALTI_CFG_OS_TRU64)
        /* TRU64 returns the same interface many times,
         * but only one interface has UP flag */
        if (0 == (sIfReq->ifr_flags & IFF_UP))
        {
            continue;
        }
        else
        {
            /* do nothing */
        }
#else
        /* no code for other OSes */
#endif

        /* When ifconf is set, it sets also IP-address in ifreq->ifr_addr field.
         * Some platforms returns the same interfaces repeatly,
         * but only one interface has non-zero value in ifr_addr field.
         */
        if (ioctl(sSocket, SIOCGIFADDR, &sIfReqIP) < 0)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        if (ioctl(sSocket, SIOCGIFBRDADDR, &sIfReqBrd) < 0)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sBrdAddr = ((struct sockaddr_in *)&sIfReqBrd.ifr_addr)->sin_addr;
        sAddr = ((struct sockaddr_in *)&sIfReqIP.ifr_addr)->sin_addr;

        acpSysGetIPAddrCopy(aIPAddr, &sAddr, &sBrdAddr, aCount, &sCount);
    }

    if (aRealCount != NULL)
    {
        *aRealCount = sCount;
    }
    else
    {
        /* do nothing */
    }

    for (i = sCount; i < aCount; i++)
    {
        aIPAddr[i].mValid = ACP_FALSE;
    }

    acpMemFree(sIfConf.ifc_buf);

    sRet = close(sSocket);

    if (sRet == 0)
    {
        sRC = ACP_RC_SUCCESS;
    }
    else
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    return sRC;
}

#endif
#endif


#if !defined(ALTI_CFG_OS_WINDOWS)
#if defined(ACP_CFG_DOXYGEN)
/**
 * gets ipv6 address
 *
 * @param aIPv6Addr array of #acp_ipv6_addr_t to get ip address
 * @param aCount array size
 * @param aRealCount pointer to a variable to store number of ip addresses
 * @return result code
 *
 * A platform can have many IPs, aRealCount must be checked to get
 * the count of IPs. And if the aRealCount value is bigger than you expected,
 * you have to call the function again to read every IPs.
 */
ACP_EXPORT acp_rc_t acpSysGetIPv6Address(acp_ipv6_addr_t *aIPv6Addr,
                                         acp_uint32_t   aCount,
                                         acp_uint32_t  *aRealCount)
{
    return ACP_RC_SUCCESS;
}

#elif defined(ALTI_CFG_OS_FREEBSD) || \
    defined(ALTI_CFG_OS_DARWIN) || defined(ALTI_CFG_OS_LINUX)

ACP_EXPORT acp_rc_t acpSysGetIPv6Address(acp_ipv6_addr_t *aIPv6Addr,
                                         acp_uint32_t   aCount,
                                         acp_uint32_t  *aRealCount)
{
    struct ifaddrs*     sIfs = NULL;
    struct ifaddrs*     sHead = NULL;
    struct sockaddr_in6* sIn6 = NULL;
    acp_uint32_t          sCount;
    acp_rc_t            sRC = ACP_RC_SUCCESS;
    acp_bool_t          sFlagCopyAddr = ACP_TRUE;
    acp_uint16_t sFamily;

    ACP_TEST_RAISE(0 != getifaddrs(&sIfs), GETIF_FAILED);
    sHead = sIfs;
    for(sCount = 0; NULL != sIfs;)
    {
        if(sCount >= aCount)
        {
            /* over user-requested count,
             * so do not copy data 
             */
            sFlagCopyAddr = ACP_FALSE;
        }
        else
        {
            /* Fall through */
        }

        /* Getting IP Addresses - No need of pass loopback
        Fall through and get IP address */

        sIn6 = (struct sockaddr_in6 *)sIfs->ifa_addr;
        sFamily = (acp_uint16_t)(sIfs->ifa_addr->sa_family);
        
        if((NULL != sIn6) &&
           (AF_INET6 == sFamily))
        {
            if (sFlagCopyAddr == ACP_TRUE)
            {
                /* Fill aIPv6Addr */
                aIPv6Addr->mFamily = sFamily;
                acpMemCpy(&(aIPv6Addr->mAddr6),
                          &(sIn6->sin6_addr),
                          sizeof(struct in6_addr));

                /* Increase the count of IP address */
                aIPv6Addr->mValid = ACP_TRUE;
                aIPv6Addr++;
            }
            else
            {
                /* do not copy */
            }

            sCount++;
        }
        else
        {
            /* Do nothing */
        }

        sIfs = sIfs->ifa_next;
    }

    if(NULL != aRealCount)
    {
        *aRealCount = sCount;
    }
    else
    {
        /* Do nothing */
    }

    for (; sCount < aCount; sCount++)
    {
        aIPv6Addr->mValid = ACP_FALSE;
        aIPv6Addr++;
    }

    freeifaddrs(sHead);
    return sRC;


    ACP_EXCEPTION(GETIF_FAILED)
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    ACP_EXCEPTION_END;
    return sRC;
}

#else


ACP_EXPORT acp_rc_t acpSysGetIPv6Address(acp_ipv6_addr_t *aIPv6Addr,
                                         acp_uint32_t   aCount,
                                         acp_uint32_t  *aRealCount)
{
#if defined(AF_INET6)
    acp_char_t    *sPtr = NULL;
    acp_sint32_t   sSocket;
    acp_sint32_t   sRet;
    struct ifconf  sIfConf;
    struct ifreq  *sIfReq = NULL;
    acp_uint32_t   sCount;
    acp_uint32_t   i;
    acp_rc_t       sRC;

    sSocket = socket(AF_INET6, SOCK_DGRAM, 0);

    if (sSocket == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRC = acpSysGetIfConf(sSocket, &sIfConf);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        (void)close(sSocket);
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sCount = 0;

    for (sPtr = (acp_char_t *)sIfConf.ifc_req;
         sPtr < (acp_char_t *)sIfConf.ifc_req + sIfConf.ifc_len;
         sPtr += sizeof(struct ifreq))
    {
        struct ifreq   sIfReqIP; /* to check AF_INET, AF_INET6 */
        struct in6_addr sAddr;

        sIfReq = (struct ifreq *)sPtr;
            
        acpMemCpy(sIfReqIP.ifr_name, sIfReq->ifr_name, IFNAMSIZ);

        sAddr = ((struct sockaddr_in6 *)&sIfReq->ifr_addr)->sin6_addr;

#if defined (ALTI_CFG_OS_AIX)
        /* AIX returns lo0 twice, 
         * but only one lo0 has IP and it is real interface.*/
        if (sAddr.s6_addr == 0)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }
#elif defined(ALTI_CFG_OS_TRU64)
        /* TRU64 returns the same interface many times,
         * but only one interface has UP flag */
        if (0 == (sIfReq->ifr_flags & IFF_UP))
        {
            continue;
        }
        else
        {
            /* do nothing */
        }
#else
        /* no code for other OSes */
#endif

        
        /* reference for getting IP on AIX:
         * http://www.ibm.com/developerworks/aix/library/au-ioctl-socket.html */

        if (((struct sockaddr_in6 *)&sIfReq->ifr_addr)->sin6_family ==
            AF_INET6)
        {
            acpSysGetIPv6AddrCopy(aIPv6Addr, &sAddr, aCount, &sCount);
        }
        else
        {
            /* ignore */
        }
    }

    if (aRealCount != NULL)
    {
        *aRealCount = sCount;
    }
    else
    {
        /* do nothing */
    }

    for (i = sCount; i < aCount; i++)
    {
        aIPv6Addr[i].mValid = ACP_FALSE;
    }

    acpMemFree(sIfConf.ifc_buf);

    sRet = close(sSocket);

    if (sRet == 0)
    {
        sRC = ACP_RC_SUCCESS;
    }
    else
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }

    return sRC;
#else
    return ACP_RC_ENOTSUP;
#endif
}

#endif
#endif

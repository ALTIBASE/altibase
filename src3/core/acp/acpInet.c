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
 * $Id: acpInet.c 10961 2010-04-28 12:03:18Z gurugio $
 ******************************************************************************/

#include <acpInet.h>
#include <acpMem.h>
#include <acpSock.h>
#include <acpPrintf.h>
#include <acpSpinLock.h>



/**
 * converts binary representation of address to string
 * @param aFamily address family; it can be one of #ACP_AF_INET, #ACP_AF_INET6
 * @param aAddr pointer to the binary representation of address
 * @param aStr pointer to the string
 * @param aBufLen length of aStr buffer
 * @return result code
 *
 * this function does not extend the buffer of @a aStr
 * even if it does not have enough buffer.
 * so if @a aStr uses dynamic memory buffer, the buffer should be preallocated.
 *
 * it returns #ACP_RC_ENOSPC if the buffer of @a aStr is not enough to convert
 *
 * it returns #ACP_RC_ENOTSUP
 * if the platform does not support specified address family
 */
ACP_EXPORT acp_rc_t acpInetAddrToStr(acp_sint32_t  aFamily,
                                     const void   *aAddr,
                                     acp_char_t   *aStr,
                                     size_t        aBufLen)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    struct sockaddr_in   sAddr4;
    struct sockaddr_in6  sAddr6;
    struct sockaddr     *sAddr = NULL;
    socklen_t            sAddrLen;
    acp_sint32_t         sRet;

    if (aFamily == ACP_AF_INET)
    {
        acpMemSet(&sAddr4, 0, sizeof(sAddr4));

        sAddr4.sin_family = AF_INET;

        acpMemCpy(&sAddr4.sin_addr, aAddr, sizeof(struct in_addr));

        sAddr    = (struct sockaddr *)&sAddr4;
        sAddrLen = (socklen_t)sizeof(sAddr4);
    }
    else if (aFamily == ACP_AF_INET6)
    {
        acpMemSet(&sAddr6, 0, sizeof(sAddr6));

        sAddr6.sin6_family = AF_INET6;

        acpMemCpy(&sAddr6.sin6_addr, aAddr, sizeof(struct in6_addr));

        sAddr    = (struct sockaddr *)&sAddr6;
        sAddrLen = (socklen_t)sizeof(sAddr6);
    }
    else
    {
        return ACP_RC_ENOTSUP;
    }

    sRet = getnameinfo(sAddr,
                       sAddrLen,
                       aStr,
                       (DWORD)aBufLen,
                       NULL,
                       0,
                       NI_NUMERICHOST);

    if (sRet != 0)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
#elif defined(AF_INET6)
    const acp_char_t *sRet;

    sRet = inet_ntop(aFamily,
                     aAddr,
                     aStr,
                     (socklen_t)aBufLen);

    if (sRet == NULL)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else
    {
        return ACP_RC_SUCCESS;
    }
#else
    if (aFamily == ACP_AF_INET6)
    {
        return ACP_RC_ENOTSUP;
    }
    else
    {
        static acp_spin_lock_t sInetLock = ACP_SPIN_LOCK_INITIALIZER(-1);

        acp_char_t *sStr = NULL;
        acp_rc_t    sRC;

        acpSpinLockLock(&sInetLock);
        sStr = inet_ntoa(*(struct in_addr *)aAddr);
        sRC  = acpSnprintf(aStr,
                           aBufLen,
                           "%s", sStr);
        acpSpinLockUnlock(&sInetLock);

        if (ACP_RC_IS_ETRUNC(sRC))
        {
            return ACP_RC_ENOSPC;
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
#endif
}

/**
 * converts address string to binary representation
 * @param aFamily address family; it can be one of #ACP_AF_INET, #ACP_AF_INET6
 * @param aStr pointer to the string of address
 * @param aAddr pointer to the buffer for binary representation of address
 * @return result code
 *
 * it returns #ACP_RC_EINVAL if the address string is invalid
 *
 * it returns #ACP_RC_ENOTSUP
 * if the platform does not support specified address family
 */
ACP_EXPORT acp_rc_t acpInetStrToAddr(acp_sint32_t  aFamily,
                                     acp_char_t   *aStr,
                                     void         *aAddr)
{
#if defined(ALTI_CFG_OS_WINDOWS)
    struct addrinfo  sHints;
    struct addrinfo *sResult = NULL;
    acp_sint32_t     sRet;

    acpMemSet(&sHints, 0, sizeof(sHints));

    sHints.ai_family = aFamily;
    sHints.ai_flags  = AI_NUMERICHOST;

    sRet = getaddrinfo(aStr, NULL, &sHints, &sResult);

    if (sRet != 0)
    {
        if (sRet == WSAHOST_NOT_FOUND)
        {
            return ACP_RC_EINVAL;
        }
        else
        {
            return ACP_RC_FROM_SYS_ERROR(sRet);
        }
    }
    else
    {
        assert(sResult != NULL);
        assert(sResult->ai_family == aFamily);

        if (aFamily == AF_INET)
        {
            struct sockaddr_in *sAddr = (struct sockaddr_in *)sResult->ai_addr;

            acpMemCpy(aAddr, &sAddr->sin_addr, sizeof(struct in_addr));
        }
        else if (aFamily == AF_INET6)
        {
            struct sockaddr_in6 *sAddr = (struct sockaddr_in6 *)sResult->ai_addr;

            acpMemCpy(aAddr, &sAddr->sin6_addr, sizeof(struct in6_addr));
        }
        else
        {
            assert(0);
        }

        freeaddrinfo(sResult);

        return ACP_RC_SUCCESS;
    }
#elif defined(AF_INET6)
    acp_sint32_t sRet;

    sRet = inet_pton(aFamily, aStr, aAddr);

    if (sRet > 0)
    {
        return ACP_RC_SUCCESS;
    }
    else if (sRet < 0)
    {
        return ACP_RC_GET_NET_ERROR();
    }
    else /* if (sRet == 0) */
    {
        return ACP_RC_EINVAL;
    }
#else
    if (aFamily == ACP_AF_INET6)
    {
        return ACP_RC_ENOTSUP;
    }
    else
    {
#if defined(ALTI_CFG_OS_TRU64)
        in_addr_t sAddr;

        sAddr = inet_addr(aStr);

        if (sAddr == (in_addr_t)-1)
        {
            /*
             * BUGBUG:
             *
             * if aStr is invalid address, inet_addr() returns (in_addr_t)-1.
             * but actually (in_addr_t)-1 is valid address for 255.255.255.255.
             * so, in this case, we should check aStr is really invalid or not.
             */
            return ACP_RC_EINVAL;
        }
        else
        {
            ((struct in_addr *)aAddr)->s_addr = sAddr;

            return ACP_RC_SUCCESS;
        }
#else
        acp_sint32_t sRet;

        sRet = inet_aton(aStr, (struct in_addr *)aAddr);

        if (sRet == 0)
        {
            return ACP_RC_EINVAL;
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
#endif
    }
#endif
}


/**
 * get a structure of type acp_inet_hostent_t for the given address
 * and type
 * @param aHostEnt pointer to a variable which points to the gotten result
 * @param aAddr pointer to the binary representation of address
 * (acp_sock_addr_in_t type)
 * @param aLen the length of address
 * @param aType ACP_AF_INET for IPv4 or ACP_AF_INET6 for IPv6 address
 * @return result code
 *
 * The result data that points to acp_inet_hostent_t structure is a
 * pointer to static data, which can be overwritten by latter calls.
 * Locking the result data and a deep copy are necessary to keep data
 * in multi-thread execution.
 */
ACP_EXPORT acp_rc_t acpInetGetHostByAddr(acp_inet_hostent_t **aHostEnt,
                                         acp_sock_addr_in_t *aAddr,
                                         acp_sock_len_t aLen,
                                         acp_sint32_t aType)
{
    acp_rc_t sRC;
    acp_inet_hostent_t *sHost = NULL;
#if defined(ALTI_CFG_OS_WINDOWS) || defined(ALTI_CFG_OS_SOLARIS) || \
    defined(ALTI_CFG_OS_HPUX) || defined(ALTI_CFG_OS_ITRON) ||      \
    defined(ALTI_CFG_OS_TRU64)
    acp_char_t sCStrAddr[100];
#endif

    ACP_TEST_RAISE(aType == ACP_AF_INET6, INET6_FAILED);

#if defined(ALINT)
    /* The ALINT warns that gethostbyname_r should be used instead. */    
#elif defined(ALTI_CFG_OS_WINDOWS) || defined(ALTI_CFG_OS_SOLARIS) || \
    defined(ALTI_CFG_OS_HPUX) || defined(ALTI_CFG_OS_ITRON) ||      \
    defined(ALTI_CFG_OS_TRU64)
    /* Windows platform use C-string to present IP address,
     * not sockaddr_in structure. So the aAddr is translated to
     * C-string. */
    sRC = acpInetAddrToStr(ACP_AF_INET, aAddr, sCStrAddr, 100);
    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), NET_FAILED);
    sHost = gethostbyaddr((acp_char_t *)aAddr, aLen, aType);
#else
    sHost = gethostbyaddr((acp_sock_addr_in_t *)aAddr, aLen, aType);
#endif    

    ACP_TEST_RAISE(sHost == NULL, NET_FAILED);

    *aHostEnt = sHost;

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(INET6_FAILED)
    {
        sRC = ACP_RC_ENOTSUP;
    }
    
    ACP_EXCEPTION(NET_FAILED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;
    return sRC;
}


/**
 * get a structure of type acp_inet_hostent_t for the given name
 * @param aHostEnt pointer to a variable which points to the gotten result
 * @param aName pointer to the host name
 * @return result code
 *
 * IPv6 is not supported here. 
 * The result data that points to acp_inet_hostent_t structure is a
 * pointer to static data, which can be overwritten by latter calls.
 * Locking the result data and a deep copy are necessary to keep data
 * in multi-thread execution.
 */
ACP_EXPORT acp_rc_t acpInetGetHostByName(acp_inet_hostent_t **aHostEnt,
                                         const acp_char_t *aName)
{
    acp_inet_hostent_t *sHost = NULL;

#if defined(ALINT)
    /* The ALINT warns that gethostbyname_r should be used instead. */
#else
    sHost = gethostbyname(aName);
#endif

    ACP_TEST_RAISE(sHost == NULL, NET_FAILED);

    *aHostEnt = sHost;
    
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(NET_FAILED)
    {
        *aHostEnt = NULL;
    }

    ACP_EXCEPTION_END;
    return ACP_RC_EINVAL;
}


/**
 * Given an internet host-name, acpInetGetAddrInfo returns addrinfo
 * structures, each of which contains an Internet address.
 * acpInetGetAddrInfo is reentrant and allows programs to eliminate
 * IPv4-vs-IPv6 dependencies.
 * The gethostbyname*() and gethostbyaddr*() functions are obsolete.
 * Applications should use #acpInetGetAddrInfo() and
 * #acpInetGetNameInfo() instead.
 *
 * @param aInfo pointer to a variable which points to the list of
 * address info structures
 * @param aNodeName Internet host name
 * @param aServName Service name
 * @param aSockType Socket type
 * @param aFlag flags for address info structure
 * @param aFamily address family; it can be one of #ACP_AF_INET, #ACP_AF_INET6
 *
 * @return result code
 */
ACP_EXPORT acp_rc_t acpInetGetAddrInfo(acp_inet_addr_info_t **aInfo,
                                       acp_char_t   *aNodeName,
                                       acp_char_t   *aServName,
                                       acp_sint32_t aSockType,
                                       acp_sint32_t aFlag,
                                       acp_sint32_t aFamily)
{
    acp_sint32_t sRet;
    acp_rc_t sRC;
    acp_inet_addr_info_t sHint;
    acp_inet_addr_info_t *sRetInfo = NULL;

    acpMemSet(&sHint, 0, sizeof(sHint));
    sHint.ai_flags    = aFlag;
    sHint.ai_family   = aFamily;
    sHint.ai_socktype = aSockType;

    sRet = getaddrinfo(aNodeName,
                       aServName,
                       &sHint,
                       &sRetInfo);

    if (sRet == 0)
    {
        (*aInfo) = sRetInfo;
        sRC = ACP_RC_SUCCESS;
    }
#if defined(ALTI_CFG_OS_WINDOWS)
    else
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
#else
    else if (ACP_RC_IS_EAI_SYSTEM(sRet))
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_CONVERT(sRet);
    }
#endif
    
    return sRC;
}


/**
 * Free address info structures returned by acpInetGetAddrInfo()
 *
 * @param aInfo pointer to the address info
 *
 * @return no result code
 */
ACP_EXPORT void acpInetFreeAddrInfo(acp_inet_addr_info_t *aInfo)
{
    freeaddrinfo(aInfo);
}


/**
 * Convert a socket address to a corresponding host name
 *
 * @param aSockAddr pointer to a socket address
 * @param aSockLen length of the socket address
 * @param aNameBuf a buffer supplied by caller to take host name
 * @param aNameBufLen length of the buffer
 * @param aFlag flags
 *
 * @return result code
 */
ACP_EXPORT acp_rc_t acpInetGetNameInfo(const acp_sock_addr_t *aSockAddr,
                                       acp_size_t aSockLen,
                                       acp_char_t *aNameBuf,
                                       acp_size_t aNameBufLen,
                                       acp_sint32_t aFlag)
{
    acp_sint32_t sRet;
    acp_rc_t sRC;

#if defined(ALTI_CFG_OS_LINUX)
    /* Linux require socklen_t for length */
    sRet = getnameinfo(aSockAddr, (acp_sock_len_t)aSockLen,
                       aNameBuf, (acp_sock_len_t)aNameBufLen,
                       NULL, 0, aFlag);
#else
    sRet = getnameinfo(aSockAddr, aSockLen,
                       aNameBuf, aNameBufLen,
                       NULL, 0, aFlag);
#endif

    if (sRet == 0)
    {
        sRC = ACP_RC_SUCCESS;
    }
#if defined(ALTI_CFG_OS_WINDOWS)
    else
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
#else
    else if (ACP_RC_IS_EAI_SYSTEM(sRet))
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_CONVERT(sRet);
    }
#endif

    return sRC;
}


/**
 * Convert a socket address to a corresponding service name
 *
 * @param aSockAddr pointer to a socket address
 * @param aSockLen length of the socket address
 * @param aServBuf a buffer supplied by caller to take host name
 * @param aServBufLen length of the buffer
 * @param aFlag flags
 *
 * @return result code
 */
ACP_EXPORT acp_rc_t acpInetGetServInfo(const acp_sock_addr_t *aSockAddr,
                                       acp_size_t aSockLen,
                                       acp_char_t *aServBuf,
                                       acp_size_t aServBufLen,
                                       acp_sint32_t aFlag)
{
    acp_sint32_t sRet;
    acp_rc_t sRC;

#if defined(ALTI_CFG_OS_LINUX)
    /* Linux require socklen_t for length */
    sRet = getnameinfo(aSockAddr, (acp_sock_len_t)aSockLen, NULL, 0,
                       aServBuf, (acp_sock_len_t)aServBufLen, aFlag);
#else
    sRet = getnameinfo(aSockAddr, aSockLen, NULL, 0,
                       aServBuf, aServBufLen, aFlag);
#endif

    if (sRet == 0)
    {
        sRC = ACP_RC_SUCCESS;
    }
#if defined(ALTI_CFG_OS_WINDOWS)
    else
    {
        sRC = ACP_RC_GET_NET_ERROR();
    }
#else
    else if (ACP_RC_IS_EAI_SYSTEM(sRet))
    {
        sRC = ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sRC = ACP_RC_CONVERT(sRet);
    }
#endif

    return sRC;
}

/**
 * Return address and name information error description text string
 *
 * @param aErrCode - error code
 * @param aErrStr - pointer to a string describing the error.
 * If the argument is not valid, the function shall return a pointer to
 * a string whose contents indicate an unknown error.
 *
 * @return result code
 * ACP_RC_SUCCESS - on success
 * ACP_RC_ENOTSUP - if IPv6 is not supportd by platform
 * ACP_RC_EINVAL  - if aErrStr is NULL pointer
 */

ACP_EXPORT acp_rc_t acpInetGetStrError(acp_sint32_t  aErrCode,
                                       acp_char_t  **aErrStr)
{
#if defined(AF_INET6) 

    if (aErrStr != NULL)
    {

        *aErrStr = gai_strerror(aErrCode);

        return ACP_RC_SUCCESS;
    }
    else
    {
        return ACP_RC_EINVAL;
    }

#else
    /* gai_strerror is supported only for IPv6 */
    return ACP_RC_ENOTSUP;

#endif
}


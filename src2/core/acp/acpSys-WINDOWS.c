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
 * $Id: acpSys-WINDOWS.c 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#include <acpCStr.h>
#include <acpMem.h>
#include <acpPrintf.h>
#include <acpSock.h>
#include <acpSys.h>
#include <acpTimePrivate.h>

#include <acpTest.h>

#define ACPSYS_MAX_REGPATH  (128)
#define ACPSYS_MAX_REGVALUE (1024)

#define ACPSYS_ADAPTER_ADDR_BUF_LEN (15 * 1024)


ACP_INLINE void acpSysParseIP(acp_char_t* aIP, acp_uint32_t* aAddr)
{
    acp_char_t*     sIP = aIP;
    acp_char_t*     sEnd = NULL;
    acp_sint32_t    sDummy;
    acp_sint32_t    i;

    for(i = 0; i < 4; i++)
    {
        (void)acpCStrToInt32(sIP, ACP_PATH_MAX_LENGTH, &sDummy, aAddr, 10, &sEnd);
        sIP = sEnd + 1;
        aAddr++;
    }
}

ACP_INLINE acp_rc_t acpSysGetIPAddrCopy(acp_ip_addr_t*      aIPAddr,
                                        acp_uint32_t        aCount,
                                        acp_uint32_t*       aRealCount,
                                        PIP_ADAPTER_INFO    aAdapter)
{
    acp_uint32_t    sAddr[4];
    acp_uint32_t    sMskAddr[4];
    PIP_ADDR_STRING sIpAddrList = &(aAdapter->IpAddressList);

    while(NULL != sIpAddrList)
    {
            /*
        sscanf_s(sIpAddrList->IpAddress.String, "%d.%d.%d.%d",
                 &(sAddr[0]), &(sAddr[1]), &(sAddr[2]), &(sAddr[3]));
                 */
        acpSysParseIP(sIpAddrList->IpAddress.String, sAddr);

        acpSysParseIP(sIpAddrList->IpMask.String, sMskAddr);

        if((0 == sAddr[0]) && (0 == sAddr[1]) &&
           (0 == sAddr[2]) && (0 == sAddr[3]))
        {
            /*
             * Network cable may be disconnected
             * Retrieve ip address directly from the registry
             */
            HKEY        sKey;
            DWORD       sRegSize = ACPSYS_MAX_REGVALUE;
            DWORD       sMaskRegSize = ACPSYS_MAX_REGVALUE;
            DWORD       sRet;
            acp_size_t  sRegLen;
            acp_size_t  sMaskRegLen;
            acp_char_t  sRegPath[ACPSYS_MAX_REGPATH] =
                "SYSTEM\\CurrentControlSet\\Services\\"
                "Tcpip\\Parameters\\Interfaces\\%s";
            acp_char_t  sRegValue[ACPSYS_MAX_REGVALUE];
            acp_char_t  sMaskRegValue[ACPSYS_MAX_REGVALUE];
            acp_char_t* sRegPos = NULL;
            acp_char_t* sMaskRegPos = NULL;

            (void)acpCStrCat(sRegPath, ACPSYS_MAX_REGPATH,
                             aAdapter->AdapterName, ACPSYS_MAX_REGPATH);

            sRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                sRegPath,
                                0,
                                KEY_ALL_ACCESS,
                                &sKey);
            ACP_TEST(0 != sRet);

            sRet = RegQueryValueEx(sKey,
                                   "IPAddress",
                                   NULL,
                                   NULL,
                                   (LPBYTE)sRegValue,
                                   &sRegSize);
            ACP_TEST(0 != sRet);
            sRet = RegQueryValueEx(sKey,
                                   "SubnetMask",
                                   NULL,
                                   NULL,
                                   (LPBYTE)sMaskRegValue,
                                   &sMaskRegSize);
            ACP_TEST(0 != sRet);

            sRegPos = sRegValue;
            sMaskRegPos = sMaskRegValue;
            while(sRegSize > 1)
            {
                acpSysParseIP(sRegPos, sAddr);
                /*
                sscanf_s(sRegPos, "%d.%d.%d.%d",
                         &(sAddr[0]), &(sAddr[1]), &(sAddr[2]), &(sAddr[3]));
                         */
                acpSysParseIP(sMaskRegPos, sMskAddr);
                
                if((0 == sAddr[0]) && (0 == sAddr[1]) &&
                   (0 == sAddr[2]) && (0 == sAddr[3]))
                {
                    /* Do not copy this address */
                    continue;
                }
                else
                {
                    /* fall through */
                }

                if(*aRealCount < aCount)
                {
                    aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b1 = sAddr[0];
                    aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b2 = sAddr[1];
                    aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b3 = sAddr[2];
                    aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b4 = sAddr[3];

                    aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b1 = (sAddr[0] & sMskAddr[0]) | ~sMskAddr[0];
                    aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b2 = (sAddr[1] & sMskAddr[1]) | ~sMskAddr[1];
                    aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b3 = (sAddr[2] & sMskAddr[2]) | ~sMskAddr[2];
                    aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b4 = (sAddr[3] & sMskAddr[3]) | ~sMskAddr[3];

                    aIPAddr[*aRealCount].mValid = ACP_TRUE;
                }
                else
                {
                    /* Do not copy */
                }

                *aRealCount   = *aRealCount + 1;
                sRegLen       = acpCStrLen(sRegPos, ACPSYS_MAX_REGPATH) + 1;
                sMaskRegLen   = acpCStrLen(sMaskRegPos, ACPSYS_MAX_REGPATH) + 1;
                sRegPos      += sRegLen;
                sMaskRegPos  += sMaskRegLen;
                sRegSize     -= sRegLen;
                sMaskRegSize -= sMaskRegLen;
            }

            (void)RegCloseKey(sKey);
        }
        else
        {
            if(*aRealCount < aCount)
            {
                aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b1 = sAddr[0];
                aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b2 = sAddr[1];
                aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b3 = sAddr[2];
                aIPAddr[*aRealCount].mAddr.S_un.S_un_b.s_b4 = sAddr[3];

                aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b1 = (sAddr[0] & sMskAddr[0]) | ~sMskAddr[0];
                aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b2 = (sAddr[1] & sMskAddr[1]) | ~sMskAddr[1];
                aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b3 = (sAddr[2] & sMskAddr[2]) | ~sMskAddr[2];
                aIPAddr[*aRealCount].mBrdAddr.S_un.S_un_b.s_b4 = (sAddr[3] & sMskAddr[3]) | ~sMskAddr[3];

                aIPAddr[*aRealCount].mValid = ACP_TRUE;
            }
            else
            {
                /* Do not copy */
            }

            *aRealCount = *aRealCount + 1;
        }

        sIpAddrList = sIpAddrList->Next;
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION_END;
    return ACP_RC_GET_OS_ERROR();
}

ACP_INLINE void acpSysGetMacAddrCopy(acp_mac_addr_t *aMacAddr,
                                     acp_uint32_t    aCount,
                                     acp_uint32_t   *aRealCount,
                                     void           *aAddr)
{
    acp_uint8_t  *sAddr = aAddr;
    acp_uint32_t i;

    for (i = 0; i < ACP_SYS_MAC_ADDR_LEN; i++)
    {
        if (sAddr[i] != 0)
        {
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    if (i < ACP_SYS_MAC_ADDR_LEN)
    {
        for (i = 0; (i < *aRealCount) && (i < aCount); i++)
        {
            if (acpMemCmp(aMacAddr[i].mAddr, aAddr, ACP_SYS_MAC_ADDR_LEN) == 0)
            {
                return;
            }
            else
            {
                /* do nothing */
            }
        }

        if (*aRealCount < aCount)
        {
            acpMemCpy(aMacAddr[*aRealCount].mAddr, aAddr, ACP_SYS_MAC_ADDR_LEN);
            aMacAddr[*aRealCount].mValid = ACP_TRUE;
        }
        else
        {
            /* do nothing */
        }

        *aRealCount = *aRealCount + 1;
    }
    else
    {
        /* do nothing */
    }
}

ACP_EXPORT acp_rc_t acpSysSetHandleLimit(acp_uint32_t aHandleLimit)
{
    ACP_UNUSED(aHandleLimit);

    return ACP_RC_ENOTSUP;
}

ACP_EXPORT acp_rc_t acpSysGetCPUCount(acp_uint32_t *aCount)
{
    acp_slong_t sRet;

    SYSTEM_INFO sSysInfo;

    GetSystemInfo(&sSysInfo);

    sRet = sSysInfo.dwNumberOfProcessors;

    if (sRet == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        *aCount = (sRet > 1) ? (acp_uint32_t)sRet : 1;

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpSysGetPageSize(acp_size_t *aPageSize)
{
    acp_slong_t sRet;

    SYSTEM_INFO sSysInfo;

    GetSystemInfo(&sSysInfo);

    sRet = sSysInfo.dwPageSize;

    if (sRet == -1)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        *aPageSize = (acp_size_t)sRet;

        return ACP_RC_SUCCESS;
    }
}

ACP_EXPORT acp_rc_t acpSysGetBlockSize(acp_size_t *aBlockSize)
{
    acp_rc_t     sRC;
    acp_sint32_t sRet;

    DWORD sSectorsPerCluster;
    DWORD sBytesPerSector;

    sRet = GetDiskFreeSpace(NULL,
                            &sSectorsPerCluster,
                            &sBytesPerSector,
                            NULL,
                            NULL);

    if (sRet != 0)
    {
        *aBlockSize = (acp_size_t)(sSectorsPerCluster * sBytesPerSector);
        sRC         = ACP_RC_SUCCESS;
    }
    else
    {
        *aBlockSize = 0;
        sRC         = ACP_RC_GET_OS_ERROR();
    }

    return sRC;
}

ACP_EXPORT acp_rc_t acpSysGetHostName(acp_char_t *aBuffer, acp_size_t aSize)
{
    acp_sint32_t sRet;

    sRet = gethostname(aBuffer, (acp_sint32_t)aSize);

    return (sRet != 0) ? ACP_RC_GET_NET_ERROR() : ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSysGetUserName(acp_char_t *aBuffer, acp_size_t aSize)
{
    BOOL  sRet;
    DWORD sSize;

    sSize = (DWORD)aSize;
    sRet  = GetUserName(aBuffer, &sSize);

    return (sRet == 0) ? ACP_RC_GET_OS_ERROR() : ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
    PIP_ADAPTER_INFO sAdapterInfo;
    PIP_ADAPTER_INFO sAdapter;
    ULONG            sBufferSize;
    DWORD            sRet;
    DWORD            sNumIfs;
    acp_rc_t         sRC;
    acp_uint32_t     sCount;
    acp_uint32_t     i;

    sAdapterInfo = NULL;
    sBufferSize  = sizeof(IP_ADAPTER_INFO);

    sRet = GetNumberOfInterfaces(&sNumIfs);

    if (sRet != NO_ERROR)
    {
        return ACP_RC_FROM_SYS_ERROR(sRet);
    }
    else
    {
        /* BUG-21473 windows장비에서 acpSysGetMacAddr()함수가 실패합니다. */
        sBufferSize *= sNumIfs;

        sRC = acpMemRealloc((void **)&sAdapterInfo, sBufferSize);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }

        sRet = GetAdaptersInfo(sAdapterInfo, &sBufferSize);
    }

    sCount = 0;

    switch (sRet)
    {
        case ERROR_NO_DATA:
            sRC = ACP_RC_SUCCESS;
            break;

        case ERROR_SUCCESS:
            for (sAdapter = sAdapterInfo;
                 sAdapter != NULL;
                 sAdapter = sAdapter->Next)
            {
                if (sAdapter->AddressLength == ACP_SYS_MAC_ADDR_LEN)
                {
                    acpSysGetMacAddrCopy(aMacAddr,
                                         aCount,
                                         &sCount,
                                         sAdapter->Address);
                }
                else
                {
                    /* do nothing */
                }
            }

            sRC = ACP_RC_SUCCESS;
            break;

        default:
            sRC = ACP_RC_FROM_SYS_ERROR(sRet);
            break;
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
        aMacAddr[i].mValid = ACP_FALSE;
    }

    acpMemFree(sAdapterInfo);

    return sRC;
}

ACP_EXPORT acp_rc_t acpSysGetCPUTimes(acp_cpu_times_t *aCpuTimes)
{
    FILETIME sCreationTime;
    FILETIME sExitTime;
    FILETIME sKernelTime;
    FILETIME sUserTime;
    BOOL     sRet;

    sRet = GetProcessTimes(GetCurrentProcess(),
                           &sCreationTime,
                           &sExitTime,
                           &sKernelTime,
                           &sUserTime);

    if (sRet == 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    aCpuTimes->mSystemTime = acpTimeFromRelFileTime(&sKernelTime);
    aCpuTimes->mUserTime   = acpTimeFromRelFileTime(&sUserTime);

    return ACP_RC_SUCCESS;
}

ACP_EXPORT acp_rc_t acpSysGetIPAddress(acp_ip_addr_t *aIPAddr,
                                       acp_uint32_t   aCount,
                                       acp_uint32_t  *aRealCount)
{
    PIP_ADAPTER_INFO sAdapterInfo;
    PIP_ADAPTER_INFO sAdapter;
    ULONG            sBufferSize;
    DWORD            sRet;
    DWORD            sNumIfs;
    acp_rc_t         sRC;
    acp_uint32_t     sCount;
    acp_uint32_t     i;

    sAdapterInfo = NULL;
    sBufferSize  = sizeof(IP_ADAPTER_INFO);

    sRet = GetNumberOfInterfaces(&sNumIfs);

    if (sRet != NO_ERROR)
    {
        return ACP_RC_FROM_SYS_ERROR(sRet);
    }
    else
    {
        sBufferSize *= sNumIfs;

        sRC = acpMemRealloc((void **)&sAdapterInfo, sBufferSize);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            return sRC;
        }
        else
        {
            /* do nothing */
        }

        sRet = GetAdaptersInfo(sAdapterInfo, &sBufferSize);
    }

    sCount = 0;

    switch (sRet)
    {
        case ERROR_NO_DATA:
            sRC = ACP_RC_SUCCESS;
            break;

        case ERROR_SUCCESS:
            for (sAdapter = sAdapterInfo;
                 sAdapter != NULL;
                 sAdapter = sAdapter->Next)
            {
                sRC = acpSysGetIPAddrCopy(aIPAddr,
                                          aCount,
                                          &sCount,
                                          sAdapter);
                if(ACP_RC_NOT_SUCCESS(sRC))
                {
                    return sRC;
                }
                else
                {
                    /* Do nothing */
                }
            }

            sRC = ACP_RC_SUCCESS;
            break;

        default:
            sRC = ACP_RC_FROM_SYS_ERROR(sRet);
            break;
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

    acpMemFree(sAdapterInfo);

    return sRC;
}

ACP_EXPORT acp_rc_t acpSysGetIPv6Address(acp_ipv6_addr_t *aIPv6Addr,
                                         acp_uint32_t    aCount,
                                         acp_uint32_t    *aRealCount)
{
    PIP_ADAPTER_ADDRESSES sAddresses = NULL;
    PIP_ADAPTER_ADDRESSES sCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS sUnicast = NULL;

    DWORD sBufferSize = ACPSYS_ADAPTER_ADDR_BUF_LEN;
    DWORD sRet = 0;

    PSOCKADDR_IN6 sSockAddr6 = NULL;
   
    acp_rc_t     sRC;
    acp_uint32_t sCount = 0;
    acp_uint32_t i = 0;

    /* 
     * Allocate memory for IP_ADAPTER_ADDRESSES structures 
     * Per MSDN recomendation allocate 15KB buffer at first 
     */  
    sRC = acpMemAlloc((void **)&sAddresses, 
		      ACPSYS_ADAPTER_ADDR_BUF_LEN);

    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_MALLOC);
  
    /* Get addresses associated with the adapters */
    sRet = GetAdaptersAddresses(AF_INET6, 
                                GAA_FLAG_INCLUDE_PREFIX, 
                                NULL, 
                                sAddresses, 
                                &sBufferSize);

    /* 
     * Not enough memory for address structures 
     * GetAdaptersAddresses returned required 
     * size in sBufferSize
     */
    if (sRet == ERROR_BUFFER_OVERFLOW) 
    {
        /* Reallocate buffer to required size */
        sRC = acpMemRealloc((void **)&sAddresses, 
	   	           sBufferSize);
       
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), E_REALLOC);
        
        /* Get addresses again */
        sRet = GetAdaptersAddresses(AF_INET6, 
                                    GAA_FLAG_INCLUDE_PREFIX, 
                                    NULL, 
                                    sAddresses, 
                                    &sBufferSize);
    }
    else
    {
       /* Continue */	   
    }

    ACP_TEST_RAISE(sRet != NO_ERROR, E_GET_ADDRESS);

    sCurrAddresses = sAddresses;
    
    /* Traverse through the list */
    while (sCurrAddresses != NULL) 
    {
        sUnicast = sCurrAddresses->FirstUnicastAddress;
          
        while(sUnicast != NULL)
        {
            sSockAddr6 = (struct sockaddr_in6 *)sUnicast->Address.lpSockaddr;
               
            if (sCount < aCount)
            {
                aIPv6Addr[sCount].mAddr6 = sSockAddr6->sin6_addr;
                aIPv6Addr[sCount].mValid = ACP_TRUE;
            }
            sUnicast = sUnicast->Next;
            sCount++;
        } 
        sCurrAddresses = sCurrAddresses->Next;
    }

    /* Set rest of addreses non valid */
    for (i = sCount; i < aCount; i++)
    {
        aIPv6Addr[i].mValid = ACP_FALSE;
    }
    
    /* Return real number of addresses */
    *aRealCount = sCount; 

    acpMemFree(sAddresses);
    
    return ACP_RC_SUCCESS;
 
    ACP_EXCEPTION(E_MALLOC)
    {
	/* Do nothing */
    }

    ACP_EXCEPTION(E_REALLOC)
    {
        (void)acpMemFree(sAddresses);
    }

    ACP_EXCEPTION(E_GET_ADDRESS)
    {
        (void)acpMemFree(sAddresses);
        sRC = ACP_RC_FROM_SYS_ERROR(sRet);
    }

    ACP_EXCEPTION_END;

    return sRC;
}

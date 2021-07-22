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
 * $Id: acpSysMacAddr-UNIX.c 11173 2010-06-04 02:09:52Z gurugio $
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
 * gets mac address (hardware address for network interface)
 *
 * @param aMacAddr array of #acp_mac_addr_t to get mac address
 * @param aCount array size
 * @param aRealCount pointer to a variable to store number of mac
 * addresses (WARNING! aRealCount can be zero)
 * @return result code
 * 
 * A platform is able to have many MACs or no MAC.
 * Therefore aRealCount must be checked to get
 * the count of MACs. And if the aRealCount value is bigger than you expected,
 * you have to call the function again to read every MACs.
 *
 * If the platform generates a virtual network card with composition
 * of several network cards, physical MAC address is ignored and
 * no MAC address is available. In that case, acpSysGetMacAddress
 * returns ZERO at aRealCount.
 */
ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
}

#elif defined(ALTI_CFG_OS_DARWIN)
ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
    struct ifaddrs     *sIfCur, *sIfHead = NULL;
    struct sockaddr_dl *sSockAddr = NULL;
    caddr_t             sPtr;
    acp_uint32_t        sEntry;
    acp_uint32_t        i;
    acp_bool_t          sAlloc = ACP_FALSE;
    
    ACP_TEST(getifaddrs(&sIfHead) != 0);
    sAlloc = ACP_TRUE;
    
    for (sEntry = 0, sIfCur = sIfHead; sIfCur; sIfCur = sIfCur->ifa_next)
    {
        if(sIfCur->ifa_addr->sa_family == AF_LINK)
        {
            sSockAddr = (struct sockaddr_dl *)sIfCur->ifa_addr;
            sPtr = ((caddr_t)((sSockAddr)->sdl_data + (sSockAddr)->sdl_nlen));
            if((sPtr != NULL) && (sSockAddr->sdl_alen == 6))
            {
                for(i = 0; i < sSockAddr->sdl_alen; i++, sPtr++)
                {
                    aMacAddr[sEntry].mAddr[i] = (acp_uint8_t)(*sPtr);
                }
                aMacAddr[sEntry].mValid = ACP_TRUE;
                if (++sEntry == aCount)
                {
                    break; /* end of buffer */
                }
                else
                {
                    continue;
                }
            }
        }
    }

    *aRealCount = sEntry;
    
    for (i = sEntry; i < aCount; i++)
    {
        aMacAddr[i].mValid = ACP_FALSE;
    }
    
    freeifaddrs(sIfHead);
        
    return ACP_RC_SUCCESS;
    ACP_EXCEPTION_END;
    if (sAlloc == ACP_TRUE)
    {
        freeifaddrs(sIfHead);
    }
    else
    {
        /* don't have to free */
    }
    
    return ACP_RC_GET_OS_ERROR();
}


#elif defined(ALTI_CFG_OS_AIX)

ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
    struct kinfo_ndd *sNddInfo = NULL;
    acp_sint32_t      sSize;
    acp_sint32_t      sRet;
    acp_uint32_t      sRetCount;
    acp_uint32_t      sCount;
    acp_uint32_t      i;
    acp_rc_t          sRC;

    sSize = getkerninfo(KINFO_NDD, 0, 0, 0);

    if (sSize <= 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sRC = acpMemAlloc((void **)&sNddInfo, sSize);

    if (ACP_RC_NOT_SUCCESS(sRC))
    {
        return sRC;
    }
    else
    {
        /* do nothing */
    }

    sRetCount = (acp_uint32_t)(sSize / sizeof(struct kinfo_ndd));

    sRet = getkerninfo(KINFO_NDD, sNddInfo, &sSize, 0);

    if (sRet < 0)
    {
        acpMemFree(sNddInfo);
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        /* do nothing */
    }

    sCount = 0;

    for (i = 0; i < sRetCount; i++)
    {
        if (sNddInfo[i].ndd_addrlen == ACP_SYS_MAC_ADDR_LEN)
        {
            acpSysGetMacAddrCopy(aMacAddr,
                                 aCount,
                                 &sCount,
                                 sNddInfo[i].ndd_addr);
        }
        else
        {
            /* do nothing */
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
        aMacAddr[i].mValid = ACP_FALSE;
    }

    acpMemFree(sNddInfo);

    return ACP_RC_SUCCESS;
}

#elif defined(ALTI_CFG_OS_HPUX)

#define ACP_SYS_GET_MAC_ADDR_PPA_MAX 16

ACP_INLINE acp_sint32_t acpSysGetMacAddrOpenDLPI(const acp_char_t *aDev,
                                                 acp_sint32_t      aPpa)
{
    struct strbuf   sCtrlMsg;
    struct strbuf   sDataMsg;
    dl_attach_req_t sReq;
    dl_error_ack_t  sAck;
    acp_sint32_t    sFlag;
    acp_sint32_t    sHandle;
    acp_sint32_t    sRet;

    sHandle = open(aDev, O_RDWR);

    if (sHandle < 0)
    {
        return -1;
    }
    else
    {
        /* do nothing */
    }

    sReq.dl_primitive = DL_ATTACH_REQ;
    sReq.dl_ppa       = aPpa;

    sCtrlMsg.maxlen   = sizeof(sReq);
    sCtrlMsg.len      = sizeof(sReq);
    sCtrlMsg.buf      = (void *)&sReq;

    sRet = putmsg(sHandle, &sCtrlMsg, NULL, 0);

    if (sRet < 0)
    {
        close(sHandle);
        return -1;
    }
    else
    {
        /* do nothing */
    }

    sCtrlMsg.maxlen = sizeof(sAck);
    sCtrlMsg.len    = 0;
    sCtrlMsg.buf    = (void *)&sAck;

    sDataMsg.maxlen = 0;
    sDataMsg.len    = 0;
    sDataMsg.buf    = NULL;

    sFlag           = 0;

    sRet = getmsg(sHandle, &sCtrlMsg, &sDataMsg, &sFlag);

    if (sRet < 0)
    {
        close(sHandle);
        return -1;
    }
    else
    {
        /* do nothing */
    }

    if (sAck.dl_primitive != DL_OK_ACK)
    {
        close(sHandle);
        return -1;
    }
    else
    {
        /* do nothing */
    }

    return sHandle;
}

ACP_INLINE void acpSysGetMacAddrFromDLPI(acp_sint32_t    aHandle,
                                         acp_mac_addr_t *aMacAddr,
                                         acp_uint32_t    aCount,
                                         acp_uint32_t   *aRealCount)
{
    struct strbuf sCtrlMsg;
    struct strbuf sDataMsg;
    dl_bind_req_t sReq;
    dl_bind_ack_t sAck[2];
    acp_sint32_t  sFlag;
    acp_sint32_t  sRet;

    sReq.dl_primitive    = DL_BIND_REQ;
    sReq.dl_sap          = 22; /* INSAP */
    sReq.dl_max_conind   = 1;
    sReq.dl_service_mode = DL_CLDLS;
    sReq.dl_conn_mgmt    = 0;
    sReq.dl_xidtest_flg  = 0;

    sCtrlMsg.maxlen      = sizeof(sReq);
    sCtrlMsg.len         = sizeof(sReq);
    sCtrlMsg.buf         = (void *)&sReq;

    sRet = putmsg(aHandle, &sCtrlMsg, NULL, 0);

    if (sRet < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    sCtrlMsg.maxlen = sizeof(sAck);
    sCtrlMsg.len    = 0;
    sCtrlMsg.buf    = (void *)sAck;

    sDataMsg.maxlen = 0;
    sDataMsg.len    = 0;
    sDataMsg.buf    = NULL;

    sFlag           = 0;

    sRet = getmsg(aHandle, &sCtrlMsg, &sDataMsg, &sFlag);

    if (sRet < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if (sAck[0].dl_primitive != DL_BIND_ACK)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if (sAck[0].dl_addr_length != (ACP_SYS_MAC_ADDR_LEN + 1))
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    acpSysGetMacAddrCopy(aMacAddr,
                         aCount,
                         aRealCount,
                         (acp_char_t *)sAck + sAck[0].dl_addr_offset);
}

ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
    acp_sint32_t sHandle;
    acp_uint32_t sCount;
    acp_uint32_t i;

    sCount = 0;

    for (i = 0; i < ACP_SYS_GET_MAC_ADDR_PPA_MAX; i++)
    {
        sHandle = acpSysGetMacAddrOpenDLPI("/dev/dlpi", i);

        if (sHandle == -1)
        {
            /* do nothing */
        }
        else
        {
            acpSysGetMacAddrFromDLPI(sHandle, aMacAddr, aCount, &sCount);

            close(sHandle);
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
        aMacAddr[i].mValid = ACP_FALSE;
    }

    return ACP_RC_SUCCESS;
}

#elif defined(ALTI_CFG_OS_FREEBSD)

#define ACP_SYS_LOOPBACK ("lo0")
#define ACP_SYS_LO_LEN   (3)

ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
    struct ifaddrs*     sIfs = NULL;
    struct ifaddrs*     sHead = NULL;
    acp_size_t          sCount;
    acp_rc_t            sRC = ACP_RC_SUCCESS;
    struct sockaddr_dl* sSdl = NULL;
    acp_bool_t          sFlagCopyAddr = ACP_TRUE;

    ACP_TEST_RAISE(0 != getifaddrs(&sIfs), GETIF_FAILED);
    sHead = sIfs;

    sRC = ACP_RC_SUCCESS;
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

        /* Must pass Loopback */
        if(0 == acpCStrCmp(sIfs->ifa_name, ACP_SYS_LOOPBACK, ACP_SYS_LO_LEN))
        {
            sIfs = sIfs->ifa_next;
            continue;
        }
        else
        {
            /* Fall through and get mac address */
        }

        sSdl = (struct sockaddr_dl*)sIfs->ifa_addr;
        if( (NULL != sSdl) &&
            (AF_LINK == sSdl->sdl_family) &&
            (IFT_ETHER == sSdl->sdl_type))
        {
            if (sFlagCopyAddr == ACP_TRUE)
            {
                acpMemCpy(aMacAddr->mAddr, LLADDR(sSdl), ACP_SYS_MAC_ADDR_LEN);
                aMacAddr->mValid = ACP_TRUE;

                /* Increase the count of mac address */
                aMacAddr++;
            }
            else
            {
                /* do not copy */
            }
            /* total amount of MAC addresses, actuall exists in system */
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
        aMacAddr->mValid = ACP_FALSE;
        aMacAddr++;
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

#else /* of FreeBSD Code */

ACP_INLINE void acpSysGetMacAddrFromIfName(acp_sint32_t      aSocket,
                                           const acp_char_t *aIfName,
                                           acp_mac_addr_t   *aMacAddr,
                                           acp_uint32_t      aCount,
                                           acp_uint32_t     *aRealCount)
{
#if defined(ALTI_CFG_OS_SOLARIS)
    struct ifreq        sIfReq;
    struct arpreq       sArpReq;
    struct sockaddr_in *sAddr = NULL;

    acpMemCpy(sIfReq.ifr_name, aIfName, IFNAMSIZ);

    if (ioctl(aSocket, SIOCGIFADDR, &sIfReq) < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    if (sIfReq.ifr_addr.sa_family != AF_INET)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    sAddr             = (struct sockaddr_in *)&sArpReq.arp_pa;
    sAddr->sin_family = AF_INET;
    sAddr->sin_addr   = ((struct sockaddr_in *)&sIfReq.ifr_addr)->sin_addr;

    if (ioctl(aSocket, SIOCGARP, &sArpReq) < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    acpSysGetMacAddrCopy(aMacAddr, aCount, aRealCount, sArpReq.arp_ha.sa_data);
#elif defined(SIOCGIFHWADDR)
    struct ifreq sIfReq;

    acpMemCpy(sIfReq.ifr_name, aIfName, IFNAMSIZ);

    if (ioctl(aSocket, SIOCGIFHWADDR, &sIfReq) < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    acpSysGetMacAddrCopy(aMacAddr,
                         aCount,
                         aRealCount,
                         &sIfReq.ifr_hwaddr.sa_data);

#elif defined(SIOCGENADDR)
    struct ifreq sIfReq;

    acpMemCpy(sIfReq.ifr_name, aIfName, IFNAMSIZ);

    if (ioctl(aSocket, SIOCGENADDR, &sIfReq) < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    acpSysGetMacAddrCopy(aMacAddr, aCount, aRealCount, sIfReq.ifr_enaddr);
#elif defined(SIOCRPHYSADDR)
    struct ifdevea sIfDev;

    acpMemCpy(sIfDev.ifr_name, aIfName, IFNAMSIZ);

    if (ioctl(aSocket, SIOCRPHYSADDR, &sIfDev) < 0)
    {
        return;
    }
    else
    {
        /* do nothing */
    }

    acpSysGetMacAddrCopy(aMacAddr, aCount, aRealCount, sIfDev.default_pa);
#else
    ACP_UNUSED(aSocket);
    ACP_UNUSED(aIfName);
    ACP_UNUSED(aMacAddr);
    ACP_UNUSED(aCount);
    ACP_UNUSED(aRealCount);
#warning no implementation of acpSysGetMacAddrFromIfName
#endif
}


#define ACP_SYS_PROCNETDEV "/proc/net/dev"

/* for Linux */
ACP_EXPORT acp_rc_t acpSysGetMacAddress(acp_mac_addr_t *aMacAddr,
                                        acp_uint32_t    aCount,
                                        acp_uint32_t   *aRealCount)
{
    acp_sint32_t    sSocket;
    struct ifconf   sIfConf;
    struct ifreq   *sIfReq = NULL;
    acp_char_t     *sPtr = NULL;
    acp_rc_t        sRC;
    acp_uint32_t    sCount;
    acp_uint32_t    i;
    acp_std_file_t sProcFile;


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

    for (sPtr = sIfConf.ifc_buf;
         sPtr < sIfConf.ifc_buf + sIfConf.ifc_len;
         sPtr += sizeof(struct ifreq))
    {
        sIfReq = (struct ifreq *)sPtr;

        acpSysGetMacAddrFromIfName(sSocket,
                                   sIfReq->ifr_name,
                                   aMacAddr,
                                   aCount,
                                   &sCount);
    }

    sRC = acpStdOpen(&sProcFile, ACP_SYS_PROCNETDEV,
                     ACP_STD_OPEN_READ_TEXT);

    if (ACP_RC_IS_SUCCESS(sRC))
    {
        acp_char_t sName[256];
        
        for (i = 0; ACP_RC_IS_SUCCESS(sRC) || ACP_RC_IS_ENOENT(sRC); i++)
        {
            sRC = acpSysGetIfNameProc(&sProcFile, sName, 255);

            /*
             * BUG-30255
             * /proc/net/dev file contains title line
             * The title line does not include a ':' or device name
             */
            if(ACP_RC_IS_ENOENT(sRC))
            {
                /* A line may not include interface name */
                continue;
            }
            else
            {
                /* Fall through */
            }

            if (ACP_RC_IS_SUCCESS(sRC))
            {
                acpSysGetMacAddrFromIfName(sSocket,
                                           sName,
                                           aMacAddr,
                                           aCount,
                                           &sCount);
            }
            else
            {
                /* stop reading file */
                break;
            }
        } /* end of for-loop */
    
        (void)acpStdClose(&sProcFile);
    }
    else
    {
        /* There is no proc file */
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

    acpMemFree(sIfConf.ifc_buf);

    sRC = (0 == close(sSocket))? ACP_RC_SUCCESS : ACP_RC_GET_OS_ERROR();

    return sRC;
}

#endif /* of Non-FreeBSD Code */
#endif

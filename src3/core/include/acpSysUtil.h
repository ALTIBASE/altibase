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
 * $Id: acpSysUtil.h 11294 2010-06-18 02:09:00Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACP_SYS_UTIL_H_)
#define _O_ACP_SYS_UTIL_H_

/**
 * @file
 * @ingroup CoreSys
 */

#include <acpError.h>
#include <acpTime.h>
#include <acpStr.h>
#include <acpMem.h>
#include <acpPrintf.h>
#include <acpSock.h>
#include <acpSys.h>
#include <acpTimePrivate.h>
#include <acpTest.h>
#include <acpCStr.h>

/* ACP_EXTERN_C_BEGIN */

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)


#else

ACP_INLINE acp_rc_t acpSysGetResourceLimit(acp_sint32_t  aResource,
                                           acp_ulong_t  *aValue)
{
    struct rlimit sLimit;
    acp_sint32_t  sRet;

    sRet = getrlimit(aResource, &sLimit);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        *aValue = (acp_ulong_t)sLimit.rlim_cur;

        return ACP_RC_SUCCESS;
    }
}

ACP_INLINE acp_rc_t acpSysSetResourceLimit(acp_sint32_t aResource,
                                           acp_ulong_t  aValue)
{
    struct rlimit sLimit;
    acp_sint32_t  sRet;

    sRet = getrlimit(aResource, &sLimit);

    if (sRet != 0)
    {
        return ACP_RC_GET_OS_ERROR();
    }
    else
    {
        sLimit.rlim_cur = (rlim_t)aValue;

        sRet = setrlimit(aResource, &sLimit);

        if (sRet != 0)
        {
            return ACP_RC_GET_OS_ERROR();
        }
        else
        {
            return ACP_RC_SUCCESS;
        }
    }
}

ACP_INLINE void acpSysGetIPAddrCopy(acp_ip_addr_t *aIPAddr,
                                    void          *aAddr,
                                    void          *aBrdAddr,
                                    acp_uint32_t   aCount,
                                    acp_uint32_t  *aRealCount)
{
    struct in_addr *sAddr = (struct in_addr *)aAddr;
    struct in_addr *sBrdAddr = (struct in_addr *)aBrdAddr;
    acp_uint32_t    i;

    /* If the same IP-address is already copied, skip it. */
    for (i = 0; (i < *aRealCount) && (i < aCount); i++)
    {
        if (acpMemCmp(&aIPAddr[i].mAddr,
                      sAddr,
                      sizeof(struct in_addr)) == 0)
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
        acpMemCpy(&aIPAddr[*aRealCount].mBrdAddr,
                  sBrdAddr,
                  sizeof(struct in_addr));
        acpMemCpy(&aIPAddr[*aRealCount].mAddr,
                  sAddr,
                  sizeof(struct in_addr));
        aIPAddr[*aRealCount].mValid = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }

    *aRealCount = *aRealCount + 1;
}

ACP_INLINE void acpSysGetIPv6AddrCopy(acp_ipv6_addr_t *aIPv6Addr,
                                      void          *aAddr,
                                      acp_uint32_t   aCount,
                                      acp_uint32_t  *aRealCount)
{
    struct in6_addr *sAddr = (struct in6_addr *)aAddr;
    acp_uint32_t    i;

    /* If the same IP-address is already copied, skip it. */
    for (i = 0; (i < *aRealCount) && (i < aCount); i++)
    {
        if (acpMemCmp(&aIPv6Addr[i].mAddr6,
                      sAddr,
                      sizeof(struct in6_addr)) == 0)
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
        acpMemCpy(&aIPv6Addr[*aRealCount].mAddr6,
                  sAddr,
                  sizeof(struct in6_addr));
        aIPv6Addr[*aRealCount].mValid = ACP_TRUE;
    }
    else
    {
        /* do nothing */
    }

    *aRealCount = *aRealCount + 1;
}

ACP_INLINE void acpSysGetMacAddrCopy(acp_mac_addr_t *aMacAddr,
                                     acp_uint32_t    aCount,
                                     acp_uint32_t   *aRealCount,
                                     void           *aAddr)
{
    acp_uint8_t  *sAddr = aAddr;
    acp_uint32_t i;

    /* check thrash data */
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

    if (i < ACP_SYS_MAC_ADDR_LEN) /* valid data? */
    {
        for (i = 0; (i < *aRealCount) && (i < aCount); i++)
        {
            /* BUGBUG: if two interface has the same MAC and
             * they are not not storead, aRealCount is increased
             * twice according to aCount.
             *
             * ex) aCount = 1, eth2 is virtual interface of eth1
             * eth0 - 0:1:2:3:4:5 -> stored , aRealCount = 1
             * eth1 - 1:2:3:4:5:6 -> not stored, aRealCount = 2
             * eth2 - 1:2:3:4:5:6 -> not stored, aRealCount = 3
             * ex) aCount = 3, eth2 is virtual interface of eth1
             * eth0 - 0:1:2:3:4:5 -> stored , aRealCount = 1
             * eth1 - 1:2:3:4:5:6 -> stored, aRealCount = 2
             * eth2 - 1:2:3:4:5:6 -> compared to eth1 and not stored,
             *                       aRealCount = 2
             */
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

ACP_INLINE acp_rc_t acpSysGetIfConf(acp_sint32_t   aSocket,
                                    struct ifconf *aIfConf)
{
    acp_sint32_t sLastLen;
    acp_rc_t     sRC;

    sLastLen         = 0;
    aIfConf->ifc_len = 1024;
    aIfConf->ifc_buf = NULL;

    while (1)
    {
        sRC = acpMemRealloc((void **)&aIfConf->ifc_buf, aIfConf->ifc_len);

        if (ACP_RC_NOT_SUCCESS(sRC))
        {
            if (aIfConf->ifc_buf != NULL)
            {
                acpMemFree(aIfConf->ifc_buf);
            }
            else
            {
                /* dont need to free */
            }

            return sRC;
        }
        else
        {
            /* do nothing */
        }

        if (ioctl(aSocket, SIOCGIFCONF, aIfConf) < 0)
        {
            sRC = ACP_RC_GET_OS_ERROR();

            if ((errno != EINVAL) || (sLastLen != 0))
            {
                acpMemFree(aIfConf->ifc_buf);
                return sRC;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            if (sLastLen == aIfConf->ifc_len)
            {
                break;
            }
            else
            {
                /* do nothing */
            }
        }

        sLastLen          = aIfConf->ifc_len;
        aIfConf->ifc_len *= 2;
    }

    return ACP_RC_SUCCESS;
}


ACP_INLINE acp_rc_t acpSysGetIfNameProc(acp_std_file_t *aProcFile,
                                        acp_char_t *aNameBuf,
                                        acp_size_t aNameBufLen)
{
    acp_char_t sFileBuf[256];
    acp_size_t sBufIndex = 0;
    acp_char_t *sIfName = NULL;
    acp_rc_t sRC;

    sFileBuf[0] = ACP_CSTR_TERM;
        
    sRC = acpStdGetCString(aProcFile, sFileBuf, 255);

    ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), END_FILE);
    ACP_TEST_RAISE(sFileBuf[0] == ACP_CSTR_TERM, READ_FAILED);
    
    /* skip spaces */
    while (sFileBuf[sBufIndex] != ACP_CSTR_TERM
           && acpCharIsSpace(sFileBuf[sBufIndex]) == ACP_TRUE)
    {
        sBufIndex++;
    }

    /* read interface name (terminated with ':')*/
    sIfName = &sFileBuf[sBufIndex];
        
    while (sFileBuf[sBufIndex] != ACP_CSTR_TERM
           && sFileBuf[sBufIndex] != ':'
           && acpCharIsSpace(sFileBuf[sBufIndex]) != ACP_TRUE)
    {
        sBufIndex++;
    }

    if (sBufIndex < sizeof(sFileBuf)
        && (acp_size_t)(&(sFileBuf[sBufIndex]) - sIfName) < aNameBufLen
        && sFileBuf[sBufIndex] == ':')
    {
        sFileBuf[sBufIndex] = ACP_CSTR_TERM;

        sRC = acpCStrCpy(aNameBuf, aNameBufLen,
                         sIfName, &(sFileBuf[sBufIndex]) - sIfName);

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), READ_FAILED);
    }
    else
    {
        /*
         * BUG-30255
         * /proc/net/dev file contains title line
         * The title line does not include a ':' or device name
         */
        ACP_RAISE(NO_NAME_FOUND);
    }

    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(END_FILE)
    {
        sRC = ACP_RC_EOF;
    }

    ACP_EXCEPTION(READ_FAILED)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION(NO_NAME_FOUND)
    {
        sRC = ACP_RC_ENOENT;
    }

    ACP_EXCEPTION_END;
    return sRC;

}

#endif

ACP_EXTERN_C_END

#endif

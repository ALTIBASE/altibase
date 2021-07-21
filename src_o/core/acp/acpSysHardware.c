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
 * $Id: acpSysHardware.c 11173 2010-06-04 02:09:52Z gurugio $ 
 ******************************************************************************/

#include <acpSys.h>
#include <acpCStr.h>
#include <acpPrintf.h>
#include <acpTest.h>

ACP_EXPORT acp_rc_t acpSysGetHardwareID(acp_char_t* aID,
                                        acp_size_t  aBufLen)
{
    acp_rc_t sRC = ACP_RC_SUCCESS;
    acp_mac_addr_t *sMacAddr = NULL;

    if(NULL == aID)
    {
        sRC = ACP_RC_EFAULT;
    }
    else
    {
#if defined(ALTI_CFG_OS_HPUX)
# if (ALTI_CFG_OS_MAJOR==11) && (ALTI_CFG_OS_MINOR==0)
        sRC = acpSnprintf(aID, aBufLen, "%08X", gethostid());
# else

        acp_size_t sLength;
        acp_char_t sID[ACP_SYS_ID_MAXSIZE];

        sLength = confstr(_CS_MACHINE_IDENT, sID, ACP_SYS_ID_MAXSIZE);

        if(0 == sLength)
        {
            sRC = ACP_RC_EINVAL;
        }
        else
        {
            sRC = acpCStrCpy(aID, aBufLen, sID, ACP_SYS_ID_MAXSIZE);
        }
# endif

#elif defined(ALTI_CFG_OS_SOLARIS) ||\
    defined(ALTI_CFG_OS_TRU64)     || \
    defined(ALTI_CFG_OS_AIX)
        ACP_UNUSED(sMacAddr);
        sRC = acpSnprintf(aID, aBufLen, "%08X", gethostid());

#elif defined(ALTI_CFG_OS_LINUX)    || \
      defined(ALTI_CFG_OS_WINDOWS)  || \
      defined(ALTI_CFG_OS_DARWIN)   || \
      defined(ALTI_CFG_OS_FREEBSD)
        
        acp_uint32_t sRealCount;
        acp_uint32_t sCount;
        acp_uint8_t sAddr[ACP_SYS_MAC_ADDR_LEN] = {0,0,0,0,0,0};
        acp_uint32_t sSum;
        
        /* check a count of MACs */
        sCount = 1;

        sRC = acpMemAlloc((void **)&sMacAddr, sCount*sizeof(acp_mac_addr_t));
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAC_FAIL);
        
        sRC = acpSysGetMacAddress(sMacAddr, sCount, &sRealCount);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAC_FAIL);

        /* read every MACs if many MACs exist */
        if (sCount < sRealCount)
        {
            sCount = sRealCount;
        
            sRC = acpMemRealloc((void **)&sMacAddr, 
                                sCount*sizeof(acp_mac_addr_t));
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAC_FAIL);

            sRC = acpSysGetMacAddress(sMacAddr, sCount, &sRealCount);
            ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAC_FAIL);
        }
        else
        {
            /* do nothing */
        }

        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAC_FAIL);

        ACP_TEST_RAISE(sCount < sRealCount, MAC_FAIL);

        for (sCount = 0; sCount < sRealCount; sCount++)
        {
            /*
             * sAddr[x] is 1-byte size,
             * so that over-wrapping can occur.
             * However it is unique value
             * and it can be system-specific data.
             */
            sAddr[0] += sMacAddr[sCount].mAddr[0];
            sAddr[1] += sMacAddr[sCount].mAddr[1];
            sAddr[2] += sMacAddr[sCount].mAddr[2];
            sAddr[3] += sMacAddr[sCount].mAddr[3];
            sAddr[4] += sMacAddr[sCount].mAddr[4];
            sAddr[5] += sMacAddr[sCount].mAddr[5];
        }

        sSum = sAddr[0] + sAddr[1] + sAddr[2] + sAddr[3] +
                sAddr[4] + sAddr[5];
        
        sRC = acpSnprintf(aID, aBufLen,
                              "%02X%02X%02X%02X%02X%02X%04X",
                              sAddr[0],
                              sAddr[1],
                              sAddr[2],
                              sAddr[3],
                              sAddr[4],
                              sAddr[5],
                              sSum & 0xFFFF);
        ACP_TEST_RAISE(ACP_RC_NOT_SUCCESS(sRC), MAC_FAIL);
        
        acpMemFree(sMacAddr);
#else
        /* Currently no implementation for other OSes */
        sRC = ACP_RC_ENOIMPL;

#endif
    }

#if defined(ALTI_CFG_OS_LINUX)    || \
      defined(ALTI_CFG_OS_WINDOWS)  || \
      defined(ALTI_CFG_OS_DARWIN)   || \
      defined(ALTI_CFG_OS_FREEBSD)
    ACP_EXCEPTION(MAC_FAIL)
    {
        sRC = ACP_RC_GET_OS_ERROR();
        
        if (sMacAddr != NULL)
        {
            acpMemFree(sMacAddr);
        }
        else
        {
            /* do nothing */
        }
    }
    ACP_EXCEPTION_END;
#endif

    return sRC;
}

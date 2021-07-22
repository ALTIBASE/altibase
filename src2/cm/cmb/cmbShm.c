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

#include <cmAllClient.h>

#if !defined(CM_DISABLE_IPC)

/*
 * IPC Channel Information
 */
acp_thr_mutex_t     gIpcMutex;
acp_bool_t          gIpcMutexInit = ACP_FALSE;
acp_sint32_t        gIpcClientCount;
cmbShmInfo          gIpcShmInfo;
acp_sint8_t        *gIpcShmBuffer;
acp_key_t           gIpcShmKey;
acp_sint32_t        gIpcShmID;
acp_key_t          *gIpcSemChannelKey;
acp_sint32_t       *gIpcSemChannelID;
acp_shm_t           gIpcShm;

acp_uint64_t cmbShmGetFullSize(acp_uint32_t aChannelCount)
{
    return ((acp_uint64_t)ACI_SIZEOF(cmbShmChannelInfo) * (acp_uint64_t)aChannelCount) +
           (acp_uint64_t)CMB_BLOCK_DEFAULT_SIZE *
           (acp_uint64_t)(aChannelCount * (CMB_SHM_DEFAULT_BUFFER_COUNT));
}

cmbShmChannelInfo* cmbShmGetChannelInfo(acp_sint8_t *aBaseBuf,
                                        acp_uint32_t aChannelID)
{
    return (cmbShmChannelInfo*)(aBaseBuf + ACI_SIZEOF(cmbShmChannelInfo) * aChannelID);
}

acp_sint8_t* cmnShmGetBuffer(acp_sint8_t *aBaseBuf,
                             acp_uint32_t aChannelCount,
                             acp_uint32_t aBufIdx)
{
    return (aBaseBuf +                                       /* base buffer */
            ACI_SIZEOF(cmbShmChannelInfo) * aChannelCount +  /* channel info */
            (CMB_BLOCK_DEFAULT_SIZE * aBufIdx));
}

acp_sint8_t* cmbShmGetDefaultServerBuffer(acp_sint8_t *aBaseBuf,
                                          acp_uint32_t aChannelCount,
                                          acp_uint32_t aChannelID)
{
    return (cmnShmGetBuffer(aBaseBuf,
                            aChannelCount,
                            aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT));
}

acp_sint8_t* cmbShmGetDefaultClientBuffer(acp_sint8_t *aBaseBuf,
                                          acp_uint32_t aChannelCount,
                                          acp_uint32_t aChannelID)
{
    return (cmnShmGetBuffer(aBaseBuf,
                            aChannelCount,
                            (aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT) + 1));
}

ACI_RC cmbShmInitializeStatic()
{
    /*
     * Initialize IPC Mutex
     */
    if (gIpcMutexInit == ACP_FALSE)
    {
        ACI_TEST(acpThrMutexCreate(&gIpcMutex, ACP_THR_MUTEX_DEFAULT) != ACP_RC_SUCCESS);
        gIpcMutexInit = ACP_TRUE;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbShmFinalizeStatic()
{
    /*
     * Destroy IPC Mutex
     */
    if (gIpcMutexInit == ACP_TRUE)
    {
        ACI_TEST(acpThrMutexDestroy(&gIpcMutex) != ACP_RC_SUCCESS);
        gIpcMutexInit = ACP_FALSE;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbShmAttach()
{
    if (gIpcShmBuffer == NULL)
    {
        ACI_TEST(acpShmAttach(&gIpcShm, gIpcShmKey) != ACP_RC_SUCCESS);

        gIpcShmBuffer = (acp_sint8_t*)acpShmGetAddress(&gIpcShm);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

ACI_RC cmbShmDetach()
{
    if (gIpcShmBuffer != NULL)
    {
        acpShmDetach(&gIpcShm);
        gIpcShmBuffer = NULL;
    }

    return ACI_SUCCESS;
}

#endif

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

#if !defined(CM_DISABLE_IPCDA)

/*
 * IPC Channel Information
 */
cmbShmIPCDAInfo     gIPCDAShmInfo;
acp_bool_t          gIPCDAMutexInit = ACP_FALSE;
acp_thr_mutex_t     gIPCDAMutex;

acp_uint64_t cmbShmIPCDAGetSize(acp_uint32_t aChannelCount)
{
    return ((acp_uint64_t)ACI_SIZEOF(cmbShmIPCDAChannelInfo) * (acp_uint64_t)aChannelCount) +
            (acp_uint64_t)CMB_BLOCK_DEFAULT_SIZE *
            (acp_uint64_t)(aChannelCount * (CMB_SHM_DEFAULT_BUFFER_COUNT));
}

acp_uint64_t cmbShmIPCDAGetDataBlockSize(acp_uint32_t aChannelCount)
{
    return (acp_uint64_t)cmbBlockGetIPCDASimpleQueryDataBlockSize() *
            (acp_uint64_t)(aChannelCount*(CMB_SHM_DEFAULT_DATABLOCK_COUNT));
}

cmbShmIPCDAChannelInfo* cmbShmIPCDAGetChannelInfo(acp_uint8_t *aBaseBuf, acp_uint32_t aChannelID)
{
    return (cmbShmIPCDAChannelInfo*)(aBaseBuf + ACI_SIZEOF(cmbShmIPCDAChannelInfo) * aChannelID);
}

acp_uint8_t* cmnShmIPCDAGetBuffer(acp_uint8_t *aBaseBuf,
                                  acp_uint32_t aChannelCount,
                                  acp_uint32_t aBufIdx)
{
    acp_uint8_t * sBufPtr = (aBaseBuf +
                            ACI_SIZEOF(cmbShmIPCDAChannelInfo) * 
                            aChannelCount + 
                            (CMB_BLOCK_DEFAULT_SIZE * aBufIdx));
    return sBufPtr;
}

acp_uint8_t* cmbShmIPCDAGetDefaultServerBuffer(acp_uint8_t *aBaseBuf,
                                               acp_uint32_t aChannelCount,
                                               acp_uint32_t aChannelID)
{
    acp_uint8_t * sBufPtr = cmnShmIPCDAGetBuffer(aBaseBuf,
                                                 aChannelCount,
                                                 aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT);
    return sBufPtr;
}

acp_uint8_t* cmbShmIPCDAGetDefaultClientBuffer(acp_uint8_t *aBaseBuf,
                                               acp_uint32_t aChannelCount,
                                               acp_uint32_t aChannelID)
{
    return (cmnShmIPCDAGetBuffer(aBaseBuf,
                                 aChannelCount,
                                 (aChannelID * CMB_SHM_DEFAULT_BUFFER_COUNT) + 1));
}

ACI_RC cmbShmIPCDAInitializeStatic()
{
    /* Initialize IPCDA Mutex */
    if (gIPCDAMutexInit == ACP_FALSE)
    {
        ACI_TEST(ACP_RC_NOT_SUCCESS(acpThrMutexCreate(&gIPCDAMutex, ACP_THR_MUTEX_DEFAULT)));
        gIPCDAMutexInit = ACP_TRUE;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbShmIPCDAFinalizeStatic()
{
    /* Destroy IPCDA Mutex */
    if (gIPCDAMutexInit == ACP_TRUE)
    {
        ACI_TEST(ACP_RC_NOT_SUCCESS(acpThrMutexDestroy(&gIPCDAMutex)));
        gIPCDAMutexInit = ACP_FALSE;
    }

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbShmIPCDAAttach()
{
    if (gIPCDAShmInfo.mShmBuffer == NULL)
    {
        ACI_TEST(ACP_RC_NOT_SUCCESS(acpShmAttach(&gIPCDAShmInfo.mShm, gIPCDAShmInfo.mShmKey)));

        gIPCDAShmInfo.mShmBuffer = (acp_uint8_t*)acpShmGetAddress(&gIPCDAShmInfo.mShm);
    }

    if (gIPCDAShmInfo.mShmBufferForSimpleQuery == NULL)
    {
        ACI_TEST(ACP_RC_NOT_SUCCESS(acpShmAttach(&gIPCDAShmInfo.mShmForSimpleQuery, gIPCDAShmInfo.mShmKeyForSimpleQuery)));

        gIPCDAShmInfo.mShmBufferForSimpleQuery = (acp_uint8_t*)acpShmGetAddress(&gIPCDAShmInfo.mShmForSimpleQuery);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    ACI_SET(aciSetErrorCode(cmERR_FATAL_CMN_SHM_ATTACH));

    if (gIPCDAShmInfo.mShmBuffer != NULL)
    {
        acpShmDetach(&gIPCDAShmInfo.mShm);
        gIPCDAShmInfo.mShmBuffer = NULL;
    }

    return ACI_FAILURE;
}

void cmbShmIPCDADetach()
{
    if (gIPCDAShmInfo.mShmBuffer != NULL)
    {
        acpShmDetach(&gIPCDAShmInfo.mShm);
        gIPCDAShmInfo.mShmBuffer = NULL;
    }

    if (gIPCDAShmInfo.mShmBufferForSimpleQuery != NULL)
    {
        acpShmDetach(&gIPCDAShmInfo.mShmForSimpleQuery);
        gIPCDAShmInfo.mShmBufferForSimpleQuery = NULL;
    }
}

#endif

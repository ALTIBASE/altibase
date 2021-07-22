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


typedef struct cmbPoolIPCDA
{
    cmbPool        mPool;
    acl_mem_pool_t mBlockPool;
} cmbPoolIPCDA;


ACI_RC cmbPoolInitializeIPCDA(cmbPool *aPool)
{
    ACP_UNUSED(aPool);

    return ACI_SUCCESS;
}

ACI_RC cmbPoolFinalizeIPCDA(cmbPool * aPool)
{
    ACP_UNUSED(aPool);

    return ACI_SUCCESS;
}

ACI_RC cmbPoolAllocBlockIPCDA(cmbPool * aPool, cmbBlock ** aBlock)
{
    ACP_UNUSED(aPool);

    *aBlock = NULL;

    return ACI_SUCCESS;
}

ACI_RC cmbPoolFreeBlockIPCDA(cmbPool *aPool, cmbBlock * aBlock)
{
    ACP_UNUSED(aPool);
    ACP_UNUSED(aBlock);

    return ACI_SUCCESS;
}


struct cmbPoolOP gCmbPoolOpIPCDAClient =
{
    "IPCDA",

    cmbPoolInitializeIPCDA,
    cmbPoolFinalizeIPCDA,

    cmbPoolAllocBlockIPCDA,
    cmbPoolFreeBlockIPCDA
};


ACI_RC cmbPoolMapIPCDA(cmbPool *aPool)
{
    /*
     * 함수 포인터 세팅
     */
    aPool->mOp = &gCmbPoolOpIPCDAClient;

    return ACI_SUCCESS;
}

acp_uint32_t cmbPoolSizeIPCDA()
{
    return ACI_SIZEOF(cmbPoolIPCDA);
}


#endif

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


typedef struct cmbPoolIPC
{
    cmbPool        mPool;
    acl_mem_pool_t mBlockPool;
} cmbPoolIPC;


ACI_RC cmbPoolInitializeIPC(cmbPool *aPool)
{
    cmbPoolIPC *sPool      = (cmbPoolIPC *)aPool;

    /*
     * Block Pool 초기화
     *
     * bug-27250 free Buf list can be crushed when client killed
     * block 미리 할당: cmbBlock -> cmbBlock + mBlockSize
     */
    ACI_TEST(aclMemPoolCreate(&sPool->mBlockPool,
                              ACI_SIZEOF(cmbBlock) + aPool->mBlockSize,
                              4, /* ElementCount   */
                              -1 /* ParallelFactor */) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbPoolFinalizeIPC(cmbPool * aPool)
{
    cmbPoolIPC *sPool = (cmbPoolIPC *)aPool;

    /*
     * Block Pool 삭제
     */
    aclMemPoolDestroy(&sPool->mBlockPool);

    return ACI_SUCCESS;
}

ACI_RC cmbPoolAllocBlockIPC(cmbPool * aPool, cmbBlock ** aBlock)
{
    cmbPoolIPC *sPool = (cmbPoolIPC *)aPool;

    /*
     * Block 할당
     */
    ACI_TEST(aclMemPoolAlloc(&sPool->mBlockPool, (void **)aBlock) != ACP_RC_SUCCESS);

    /*
     * Block 초기화
     */
    (*aBlock)->mBlockSize   = aPool->mBlockSize;
    (*aBlock)->mDataSize    = 0;
    (*aBlock)->mCursor      = 0;
    (*aBlock)->mIsEncrypted = ACP_FALSE;

    /*
     * bug-27250 free Buf list can be crushed when client killed
     * tcp 처럼 block을 미리 생성하도록 한다.
     */
    (*aBlock)->mData        = (acp_uint8_t *)((*aBlock) + 1);

    acpListInitObj(&(*aBlock)->mListNode, *aBlock);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbPoolFreeBlockIPC(cmbPool *aPool, cmbBlock * aBlock)
{
    cmbPoolIPC *sPool  = (cmbPoolIPC *)aPool;

    /*
     * List에서 Block 삭제
     */

    acpListDeleteNode(&aBlock->mListNode);

    /*
     * Block 해제
     */
    aclMemPoolFree(&sPool->mBlockPool, (void*)aBlock);

    return ACI_SUCCESS;
}


struct cmbPoolOP gCmbPoolOpIPCClient =
{
    "IPC",

    cmbPoolInitializeIPC,
    cmbPoolFinalizeIPC,

    cmbPoolAllocBlockIPC,
    cmbPoolFreeBlockIPC
};


ACI_RC cmbPoolMapIPC(cmbPool *aPool)
{
    /*
     * 함수 포인터 세팅
     */
    aPool->mOp = &gCmbPoolOpIPCClient;

    return ACI_SUCCESS;
}

acp_uint32_t cmbPoolSizeIPC()
{
    return ACI_SIZEOF(cmbPoolIPC);
}


#endif

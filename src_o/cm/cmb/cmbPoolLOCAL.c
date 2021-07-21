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


typedef struct cmbPoolLOCAL
{
    cmbPool        mPool;
    acl_mem_pool_t mBlockPool;
} cmbPoolLOCAL;


ACI_RC cmbPoolInitializeLOCAL(cmbPool *aPool)
{
    cmbPoolLOCAL *sPool = (cmbPoolLOCAL *)aPool;

    /*
     * Block Pool 초기화
     */
    ACI_TEST(aclMemPoolCreate(&sPool->mBlockPool,
                              ACI_SIZEOF(cmbBlock) + aPool->mBlockSize,
                              4, /* ElementCount   */
                              -1 /* ParallelFactor */) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbPoolFinalizeLOCAL(cmbPool *aPool)
{
    cmbPoolLOCAL *sPool = (cmbPoolLOCAL *)aPool;

    /*
     * Block Pool 삭제
     */
    aclMemPoolDestroy(&sPool->mBlockPool);

    return ACI_SUCCESS;
}

ACI_RC cmbPoolAllocBlockLOCAL(cmbPool *aPool, cmbBlock **aBlock)
{
    cmbPoolLOCAL *sPool = (cmbPoolLOCAL *)aPool;

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
    (*aBlock)->mData        = (acp_uint8_t *)((*aBlock) + 1);

    acpListInitObj(&(*aBlock)->mListNode, *aBlock);

    return ACI_SUCCESS;
    ACI_EXCEPTION_END;
    return ACI_FAILURE;
}

ACI_RC cmbPoolFreeBlockLOCAL(cmbPool *aPool, cmbBlock *aBlock)
{
    cmbPoolLOCAL *sPool  = (cmbPoolLOCAL *)aPool;

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


struct cmbPoolOP gCmbPoolOpLOCALClient =
{
    "LOCAL",

    cmbPoolInitializeLOCAL,
    cmbPoolFinalizeLOCAL,

    cmbPoolAllocBlockLOCAL,
    cmbPoolFreeBlockLOCAL
};


ACI_RC cmbPoolMapLOCAL(cmbPool *aPool)
{
    /*
     * 함수 포인터 세팅
     */
    aPool->mOp = &gCmbPoolOpLOCALClient;

    return ACI_SUCCESS;
}

acp_uint32_t cmbPoolSizeLOCAL()
{
    return ACI_SIZEOF(cmbPoolLOCAL);
}

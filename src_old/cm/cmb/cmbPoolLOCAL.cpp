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

#include <cmAll.h>


typedef struct cmbPoolLOCAL
{
    cmbPool    mPool;

    iduMemPool mBlockPool;
} cmbPoolLOCAL;


IDE_RC cmbPoolInitializeLOCAL(cmbPool *aPool)
{
    cmbPoolLOCAL *sPool      = (cmbPoolLOCAL *)aPool;

    /*
     * Block Pool 초기화
     */
    IDE_TEST(sPool->mBlockPool.initialize(IDU_MEM_CMB,
                                          (SChar *)"CMB_LOCAL_POOL",
                                          ID_SCALABILITY_SYS,
                                          ID_SIZEOF(cmbBlock) + aPool->mBlockSize,
                                          4,
                                          IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                          ID_TRUE,							/* UseMutex */
                                          IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,  /* AlignByte */
                                          ID_FALSE,							/* ForcePooling */
                                          ID_TRUE,							/* GarbageCollection */
                                          ID_TRUE,                          /* HWCacheLine */
                                          IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
              != IDE_SUCCESS);			

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbPoolFinalizeLOCAL(cmbPool *aPool)
{
    cmbPoolLOCAL *sPool = (cmbPoolLOCAL *)aPool;

    /*
     * Block Pool 삭제
     */
    IDE_TEST(sPool->mBlockPool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbPoolAllocBlockLOCAL(cmbPool *aPool, cmbBlock **aBlock)
{
    cmbPoolLOCAL *sPool = (cmbPoolLOCAL *)aPool;

    IDU_FIT_POINT( "cmbPoolLOCAL::cmbPoolAllocBlockLOCAL::alloc::Block" );

    /*
     * Block 할당
     */
    IDE_TEST(sPool->mBlockPool.alloc((void **)aBlock) != IDE_SUCCESS);

    /*
     * Block 초기화
     */
    (*aBlock)->mBlockSize   = aPool->mBlockSize;
    (*aBlock)->mDataSize    = 0;
    (*aBlock)->mCursor      = 0;
    (*aBlock)->mIsEncrypted = ID_FALSE;
    (*aBlock)->mData        = (UChar *)((*aBlock) + 1);

    IDU_LIST_INIT_OBJ(&(*aBlock)->mListNode, *aBlock);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbPoolFreeBlockLOCAL(cmbPool *aPool, cmbBlock *aBlock)
{
    cmbPoolLOCAL *sPool  = (cmbPoolLOCAL *)aPool;

    /*
     * List에서 Block 삭제
     */
    IDU_LIST_REMOVE(&aBlock->mListNode);

    /*
     * Block 해제
     */
    IDE_TEST(sPool->mBlockPool.memfree(aBlock) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


struct cmbPoolOP gCmbPoolOpLOCAL =
{
    (SChar *)"LOCAL",

    cmbPoolInitializeLOCAL,
    cmbPoolFinalizeLOCAL,

    cmbPoolAllocBlockLOCAL,
    cmbPoolFreeBlockLOCAL
};


IDE_RC cmbPoolMapLOCAL(cmbPool *aPool)
{
    /*
     * 함수 포인터 세팅
     */
    aPool->mOp = &gCmbPoolOpLOCAL;

    return IDE_SUCCESS;
}

UInt cmbPoolSizeLOCAL()
{
    return ID_SIZEOF(cmbPoolLOCAL);
}

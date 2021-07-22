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

#if !defined(CM_DISABLE_IPC)


typedef struct cmbPoolIPC
{
    cmbPool    mPool;
    iduMemPool mBlockPool;
} cmbPoolIPC;


IDE_RC cmbPoolInitializeIPC(cmbPool *aPool)
{
    cmbPoolIPC *sPool      = (cmbPoolIPC *)aPool;

    /*
     * Block Pool 초기화
     */
    // bug-27250 free Buf list can be crushed when client killed
    // block 미리 할당: cmbBlock -> cmbBlock + mBlockSize
    IDE_TEST(sPool->mBlockPool.initialize(IDU_MEM_CMB,
                                          (SChar *)"CMB_IPC_POOL",
                                          ID_SCALABILITY_SYS,
                                          ID_SIZEOF(cmbBlock) + aPool->mBlockSize,
                                          4,
                                          IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                          ID_TRUE,							/* UseMutex */
                                          IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,  /* AlignByte */
                                          ID_FALSE,							/* ForcePooling */
                                          ID_TRUE,							/* GarbageCollection */
                                          ID_TRUE,                          /* HWCacheLine */
                                          IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/
                ) != IDE_SUCCESS);			


    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbPoolFinalizeIPC(cmbPool * aPool)
{
    cmbPoolIPC *sPool = (cmbPoolIPC *)aPool;

    /*
     * Block Pool 삭제
     */
    IDE_TEST(sPool->mBlockPool.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbPoolAllocBlockIPC(cmbPool * aPool, cmbBlock ** aBlock)
{
    cmbPoolIPC *sPool = (cmbPoolIPC *)aPool;

    IDU_FIT_POINT( "cmbPoolIPC::cmbPoolAllocBlockIPC::alloc::Block" );

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
    // bug-27250 free Buf list can be crushed when client killed
    // tcp 처럼 block을 미리 생성하도록 한다.
    (*aBlock)->mData        = (UChar *)((*aBlock) + 1);

    IDU_LIST_INIT_OBJ(&(*aBlock)->mListNode, *aBlock);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC cmbPoolFreeBlockIPC(cmbPool *aPool, cmbBlock * aBlock)
{
    cmbPoolIPC *sPool  = (cmbPoolIPC *)aPool;

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


struct cmbPoolOP gCmbPoolOpIPC =
{
    (SChar *)"IPC",

    cmbPoolInitializeIPC,
    cmbPoolFinalizeIPC,

    cmbPoolAllocBlockIPC,
    cmbPoolFreeBlockIPC
};


IDE_RC cmbPoolMapIPC(cmbPool *aPool)
{
    /*
     * 함수 포인터 세팅
     */
    aPool->mOp = &gCmbPoolOpIPC;

    return IDE_SUCCESS;
}

UInt cmbPoolSizeIPC()
{
    return ID_SIZEOF(cmbPoolIPC);
}


#endif

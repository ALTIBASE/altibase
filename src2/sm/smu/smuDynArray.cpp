/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: smuDynArray.cpp 83870 2018-09-03 04:32:39Z kclee $
 **********************************************************************/

#include <idl.h>
#include <smuProperty.h>
#include <smuDynArray.h>

iduMemPool smuDynArray::mNodePool;
UInt       smuDynArray::mChunkSize = 0;

IDE_RC smuDynArray::initializeStatic(UInt aNodeMemSize)
{
    UInt sNodeSize;

    IDE_ASSERT(mChunkSize == 0);  // Initialize Just Once.
    IDE_ASSERT(aNodeMemSize > 0);
    
    mChunkSize = aNodeMemSize;

    sNodeSize = idlOS::align8((UInt)ID_SIZEOF(smuDynArrayNode) + aNodeMemSize);

    IDE_TEST(mNodePool.initialize(IDU_MEM_SM_SMU,
                                  (SChar*)"DYNAMIC_ARRAY",
                                  ID_SCALABILITY_SYS,
                                  sNodeSize,
                                  64,
                                  IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                  ID_TRUE,							/* UseMutex */
                                  IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                  ID_FALSE,							/* ForcePooling */
                                  ID_TRUE,							/* GarbageCollection */
                                  ID_TRUE,                          /* HWCacheLine */
                                  IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/) 
            != IDE_SUCCESS);			
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC smuDynArray::destroyStatic()
{
    IDE_TEST(mNodePool.destroy() != IDE_SUCCESS);

    mChunkSize = 0;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


IDE_RC smuDynArray::allocDynNode(smuDynArrayNode **aNode)
{
    IDE_TEST(mNodePool.alloc((void **)aNode) != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

IDE_RC smuDynArray::freeDynNode(smuDynArrayNode *aNode)
{
    IDE_TEST(mNodePool.memfree((void *)aNode) != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
 * 객체 초기화 수행. List에 대한 초기화 필수!
 */

IDE_RC smuDynArray::initialize(smuDynArrayBase *aBase)
{
    aBase->mTotalSize  = 0;
    
    aBase->mNode.mStoredSize = 0;
    
    aBase->mNode.mCapacity   = SMU_DA_BASE_SIZE;
    
    SMU_LIST_INIT_BASE(&(aBase->mNode.mList));  // make circular list 
    aBase->mNode.mList.mData = &(aBase->mNode);
        
    return IDE_SUCCESS;
    
//     IDE_EXCEPTION_END;
    
//     return IDE_FAILURE;
}

/*
 * 객체 소멸화 수행
 * 1. Sub Node들에 대한 메모리 해제. (debug mode일 경우 ASSERT 체크)
 * 2. 객체 멤버 초기화 
 */

IDE_RC smuDynArray::destroy(smuDynArrayBase *aBase)
{
    smuList *sCurList;    // for interation
    smuList *sBaseList;   // for check fence 
    smuList *sDeleteList; // for store deleting node ptr
        

    sBaseList = &(aBase->mNode.mList);
    
    // 1. Node 탐색 및 리스트 끊기
    // 2. 메모리 해제 

    sCurList = SMU_LIST_GET_NEXT(sBaseList);
    while(sCurList != sBaseList)
    {
        sDeleteList = sCurList;
        sCurList = SMU_LIST_GET_NEXT(sCurList); // for preventing FMR
        
        SMU_LIST_DELETE(sDeleteList);
        IDE_TEST(freeDynNode((smuDynArrayNode *)(sDeleteList->mData))
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


/*
 *  Storing Logic:
 *
 *     A. Src의 크기가 Node 크기를 넘지 않은 경우, Copy 후 리턴.
 *     B. Src의 크기가 Node를 넘은 경우 현재 노드를 채우고,
 *        Next Node 할당 받고, 다시 루틴 시작.
 */


IDE_RC smuDynArray::copyData(smuDynArrayBase *aBase,
                             void *aDest,         /* dest */
                             UInt *aStoredSize,   /* dest */
                             UInt  aDestCapacity, /* dest */
                             void *aSrc,          /* src  */
                             UInt  aSize)         /* src  */
{
    UInt sRemainSize;
    
    IDE_ASSERT(aDestCapacity >= *aStoredSize);
    
    sRemainSize = aDestCapacity - *aStoredSize;

    if (sRemainSize >= aSize) // A case
    {
        idlOS::memcpy((UChar *)aDest + (*aStoredSize), aSrc, aSize);
        *aStoredSize += aSize;
        aBase->mTotalSize += aSize;
    }
    else // B case
    {
        smuDynArrayNode *aTailNode;
        
        // 1. copy remain size
        idlOS::memcpy((UChar *)aDest + (*aStoredSize), aSrc, sRemainSize);
        *aStoredSize      += sRemainSize;
        aBase->mTotalSize += sRemainSize;

        // 2. alloc new node & init
        IDE_TEST(allocDynNode(&aTailNode) != IDE_SUCCESS);
        aTailNode->mStoredSize = 0;
        aTailNode->mCapacity   = mChunkSize;
        aTailNode->mList.mData = aTailNode;
        SMU_LIST_ADD_LAST(&(aBase->mNode.mList), &(aTailNode->mList));
        
        // 3. copy recursively

        IDE_TEST(copyData(aBase,
                          (void *)(aTailNode->mBuffer),
                          &(aTailNode->mStoredSize),
                          aTailNode->mCapacity,
                          (UChar *)aSrc + sRemainSize,
                          aSize - sRemainSize) != IDE_SUCCESS);
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/*
 * Head부터 Tail까지 따라가면서, aDest에 순차적으로 복사.
 */

void smuDynArray::load(smuDynArrayBase *aBase, void *aDest, UInt aDestBuffSize)
{
    smuList         *sList;
    smuList         *sBaseList;
    smuDynArrayNode *sNode;
    UInt             sOffset = 0;

    // 1. Base Node Copy
    sNode = &(aBase->mNode);

    /* BUG-32629 [sm-util] The smuDynArray::load function must check
     * destination buffer size. */
    IDE_ASSERT( aDestBuffSize >= aBase->mTotalSize );

    idlOS::memcpy(aDest, sNode->mBuffer, sNode->mStoredSize);
    
    sOffset += sNode->mStoredSize;

    // 2. Sub Node Copy 
    sBaseList = &(aBase->mNode.mList);
    
    for (sList = SMU_LIST_GET_NEXT(sBaseList);
         sList != sBaseList;
         sList = SMU_LIST_GET_NEXT(sList))
    {

        sNode = (smuDynArrayNode *)sList->mData;

        idlOS::memcpy((UChar *)aDest + sOffset,
                      (UChar *)sNode->mBuffer,
                      sNode->mStoredSize);
        sOffset += sNode->mStoredSize;
    }
}



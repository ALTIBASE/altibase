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

#include <acp.h>
#include <acl.h>
#include <ace.h>
#include <aciErrorMgr.h>
#include <ulu.h>
#include <uluMemory.h>

#if defined(ALTIBASE_MEMORY_CHECK)
static const uluMemoryType gUluMemoryType = ULU_MEMORY_TYPE_DIRECT;
#else
static const uluMemoryType gUluMemoryType = ULU_MEMORY_TYPE_MANAGER;
#endif

extern uluChunkPoolOpSet gUluChunkPoolOp[];
extern uluMemoryOpSet    gUluMemoryOp[];

/*
 *      a memory chunk
 *      +---------------+
 *      |mList          |
 *      |mBuffer -----------+
 *      |mChunkSize     |   |
 *      |mSavepoints[0] |   |
 *      |mSavepoints[1] |   |
 *      |      ...      |   |
 *      |mSavepoints[n] |   |
 *      +---------------+ <-+
 *      |      ...      |
 *      |               |
 *      +---------------+
 */

/*
 * manager alloc 방식일 때 쓰는 chunk
 */

struct uluChunk
{
    acp_list_node_t  mList;

    acp_uint8_t     *mBuffer;
    acp_uint32_t     mChunkSize;        /* mBuffer 가 가리키는 곳 부터 청크의 끝까지의 크기 */
    acp_uint32_t     mOffset;           /* Current offset */
    acp_uint32_t     mFreeMemSize;      /* 이 청크에 남은 free 메모리의 크기 */
    acp_uint32_t     mSavepoints[1];    /* 배열에 들어가는 값은 mBuffer 로부터의 offset */
};

/*
 * direct alloc 방식일 때 쓰는 chunk
 */

struct uluChunk_DIRECT
{
    acp_list_node_t  mList;
    void            *mBuffer;
    acp_uint32_t     mBufferSize;
};


/*
 * ==============================
 *
 * uluChunk
 *
 * ==============================
 */

static void uluInitSavepoints(uluChunk *aChunk, acp_uint32_t aNumOfSavepoints)
{
    acp_uint32_t sIndex;

    /*
     * mSavepoints[] 배열에 전부 0 을 넣는 함수.
     */

    for (sIndex = 0; sIndex < aNumOfSavepoints + 1; sIndex++)
    {
        aChunk->mSavepoints[sIndex] = 0;
    }
}

/*
 * 할당되는 메모리의 양
 *
 *      sizeof(uluChunk) + (Savepoint 갯수)개의 acp_uint32_t + sizeof(uluMemory) + align8(aSizeRequested)
 *
 * uluMemory 의 인스턴스가 들어갈 자리를 여유로 더 할당함으로써 사용자가
 * 청크 풀을 만들 때 최대로 지정한 양만큼의 메모리를 할당할 수 있도록 보장한다.
 */
static uluChunk *uluCreateChunk(acp_uint32_t aSizeRequested, acp_uint32_t aNumOfSavepoints)
{
    uluChunk     *sNewChunk;
    acp_uint32_t  sSizeChunkHeader;
    acp_uint32_t  sSizeUluMemory;

    aSizeRequested   = ACP_ALIGN8(aSizeRequested);
    sSizeChunkHeader = ACP_ALIGN8(ACI_SIZEOF(uluChunk) + (ACI_SIZEOF(acp_uint32_t) * aNumOfSavepoints));
    sSizeUluMemory   = ACP_ALIGN8(ACI_SIZEOF(uluMemory));

    ACI_TEST(acpMemAlloc((void**)&sNewChunk,
                         sSizeChunkHeader + sSizeUluMemory + aSizeRequested)
             != ACP_RC_SUCCESS);

    /*
     * uluChunk 와 mSavepoints 배열을 0 으로 초기화
     */
    acpMemSet(sNewChunk, 0, sSizeChunkHeader);

    acpListInitObj(&sNewChunk->mList, sNewChunk);

    sNewChunk->mBuffer      = (acp_uint8_t *)sNewChunk + sSizeChunkHeader;
    sNewChunk->mChunkSize   = aSizeRequested + sSizeUluMemory;
    sNewChunk->mFreeMemSize = aSizeRequested + sSizeUluMemory;
    sNewChunk->mOffset      = 0;

    return sNewChunk;

    ACI_EXCEPTION_END;

    return NULL;
}

static void uluDestroyChunk(uluChunk *sChunk)
{
    acpMemFree(sChunk);
}

/*
 * ====================================
 *
 * uluChunkPool
 *
 * ====================================
 */

static void uluInsertNewChunkToPool(uluChunk *aChunk, uluChunkPool *aPool)
{
    ACE_DASSERT(aChunk != NULL);
    ACE_DASSERT(aPool != NULL);

    acpListPrependNode(&(aPool->mFreeChunkList), &(aChunk->mList));
    aPool->mFreeChunkCnt++;
}

static void uluDeleteChunkFromPool(uluChunk *aChunk)
{
    ACE_DASSERT(aChunk != NULL);

    acpListDeleteNode((acp_list_t *)aChunk);
    uluDestroyChunk(aChunk);
}

static void uluChunkPoolAddMemory(uluChunkPool *aPool, uluMemory *aMemory)
{
    acpListPrependNode(&aPool->mMemoryList, &aMemory->mList);
    aPool->mMemoryCnt++;
}

static void uluChunkPoolRemoveMemory(uluChunkPool * aPool, uluMemory *aMemory)
{
    acpListDeleteNode((acp_list_t *)aMemory);
    aPool->mMemoryCnt--;
}

/*
 * ====================================
 * uluChunkPool : Get Chunk From Pool
 * ====================================
 */

static uluChunk *uluGetChunkFromPool_MANAGER(uluChunkPool *aPool, acp_uint32_t aChunkSize)
{
    acp_list_node_t *sIterator;
    acp_list_node_t *sIteratorSafe;
    uluChunk        *sNewChunk;

    ACE_DASSERT(aPool != NULL);

    sNewChunk = NULL;

    /*
     * 요청한 청크의 크기가 디폴트보다 크면 무조건 청크 할당한다.
     * 청크 사이즈의 검색비용을 줄이기 위함이다.
     *
     * Note : 디폴트 청크 사이즈를 적절하게 잘 결정해야 필요없는 malloc 을 많이 예방할 수
     *        있다.
     */
    if (aChunkSize <= aPool->mDefaultChunkSize)
    {
        /*
         * uluChunkPool 이 가지고 있는 free chunk 의 갯수를 가지고
         * 청크를 더 할당해야 하는지를 판단해서 필요한 만큼을 더 할당한다.
         */
        if (aPool->mFreeChunkCnt == 0)
        {
            if (aPool->mFreeBigChunkCnt == 0)
            {
                /*
                 * 정말 free chunk 가 없다.
                 */
                sNewChunk = uluCreateChunk(aPool->mDefaultChunkSize, aPool->mNumberOfSP);

                ACI_TEST(sNewChunk == NULL);
            }
            else
            {
                /*
                 * 그나마 Big chunk list 에 free chunk 가 있다.
                 */
                sNewChunk = (uluChunk *)(aPool->mFreeBigChunkList.mNext);
                ACI_TEST(sNewChunk == NULL);

                acpListDeleteNode((acp_list_t *)sNewChunk);
                aPool->mFreeBigChunkCnt--;
            }
        }
        else
        {
            sNewChunk = (uluChunk *)(aPool->mFreeChunkList.mNext);
            ACI_TEST(sNewChunk == NULL);

            acpListDeleteNode((acp_list_t *)sNewChunk);
            aPool->mFreeChunkCnt--;
        }
    }
    else
    {
        /*
         * 디폴트 청크 사이즈 보다 큰 크기의 메모리를 요청.
         */

        /*
         * free big chunk list 검색,
         */
        ACP_LIST_ITERATE_SAFE(&(aPool->mFreeBigChunkList), sIterator, sIteratorSafe)
        {
            /*
             * 요청된 사이즈보다 큰 청크가 있으면 그것을 리턴.
             */
            if (((uluChunk *)sIterator)->mChunkSize >= aChunkSize)
            {
                ACE_ASSERT(aPool->mFreeBigChunkCnt != 0);

                sNewChunk = (uluChunk *)sIterator;

                acpListDeleteNode((acp_list_t *)sNewChunk);
                aPool->mFreeBigChunkCnt--;
                break;
            }
        }

        /*
         * 없으면 해당 사이즈로 청크 생성, 리턴.
         */
        if (sNewChunk == NULL)
        {
            sNewChunk = uluCreateChunk(aChunkSize, aPool->mNumberOfSP);
            ACI_TEST(sNewChunk == NULL);
        }
    }

    return sNewChunk;

    ACI_EXCEPTION_END;

    return NULL;
}

static uluChunk *uluGetChunkFromPool_DIRECT(uluChunkPool *aPool,
                                            acp_uint32_t  aChunkSize)
{
    ACP_UNUSED(aPool);
    ACP_UNUSED(aChunkSize);

    return NULL;
}

/*
 * ====================================
 * uluChunkPool : Return Chunk To Pool
 * ====================================
 */

static void uluReturnChunkToPool_MANAGER(uluChunkPool *aPool, uluChunk *aChunk)
{
    ACE_DASSERT(aPool != NULL);
    ACE_DASSERT(aChunk != NULL);

    /*
     * Note: 처음 생성하는 청크는 문제가 없겠지만,
     *       반환된 청크를 재사용하는 경우 모든 필드가 초기 상태는 아님에 틀림없다.
     *       따라서 청크의 필드를 초기화하는 것이 필요하다.
     *       그렇게 하여 청크 풀에 있는 모든 청크는 가져오는 즉시 이용 가능하도록 한다.
     */
    uluInitSavepoints(aChunk, aPool->mNumberOfSP);
    aChunk->mFreeMemSize = aChunk->mChunkSize;
    aChunk->mOffset      = 0;

    if (aChunk->mChunkSize > aPool->mActualSizeOfDefaultSingleChunk)
    {
        /*
         * 크기가 default chunk size 보다 큰 청크를 돌려줌.
         */
        acpListPrependNode(&(aPool->mFreeBigChunkList), (acp_list_t *)aChunk);
        aPool->mFreeBigChunkCnt++;
    }
    else
    {
        /*
         * 크기가 default chunk size 인 청크를 돌려줌.
         */
        acpListPrependNode(&(aPool->mFreeChunkList), (acp_list_t *)aChunk);
        aPool->mFreeChunkCnt++;
    }
}

static void uluReturnChunkToPool_DIRECT(uluChunkPool *aPool, uluChunk *aChunk)
{
    ACP_UNUSED(aPool);
    ACP_UNUSED(aChunk);

    return;
}

/*
 * ==========================================================
 * uluChunkPool : Initialize Chunk Pool : 최초 보유 청크 생성
 * ==========================================================
 */

static ACI_RC uluChunkPoolCreateInitialChunks_MANAGER(uluChunkPool *aChunkPool)
{
    acp_uint32_t  i;
    uluChunk     *sChunk;

    for (i = 0; i < aChunkPool->mMinChunkCnt; i++)
    {
        sChunk = uluCreateChunk(aChunkPool->mDefaultChunkSize, aChunkPool->mNumberOfSP);

        ACI_TEST(sChunk == NULL);

        uluInsertNewChunkToPool(sChunk, aChunkPool);
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC uluChunkPoolCreateInitialChunks_DIRECT(uluChunkPool *aChunkPool)
{
    ACP_UNUSED(aChunkPool);

    return ACI_SUCCESS;
}

/*
 * ====================================
 * uluChunkPool : Destroy Chunk Pool
 * ====================================
 */

static void uluDestroyChunkPool_MANAGER(uluChunkPool *aPool)
{
    acp_list_node_t *sTempHead;
    acp_list_node_t *sTempHeadNext;

    /*
     * aPool 을 소스로 사용하고 있는 모든 uluMemory 의
     * 인스턴스를 방문해서 파괴한다.
     */

    ACP_LIST_ITERATE_SAFE(&aPool->mMemoryList, sTempHead, sTempHeadNext)
    {
        ((uluMemory *)sTempHead)->mOp->mDestroyMyself((uluMemory *)sTempHead);
    }

    /*
     * Free FreeChunks
     */
    ACP_LIST_ITERATE_SAFE(&aPool->mFreeChunkList, sTempHead, sTempHeadNext)
    {
        uluDeleteChunkFromPool((uluChunk *)sTempHead);
    }

    ACP_LIST_ITERATE_SAFE(&aPool->mFreeBigChunkList, sTempHead, sTempHeadNext)
    {
        uluDeleteChunkFromPool((uluChunk *)sTempHead);
    }

    acpMemSet(aPool, 0, ACI_SIZEOF(uluChunkPool));
    acpMemFree(aPool);

    return;
}

static void uluDestroyChunkPool_DIRECT(uluChunkPool *aPool)
{
    acp_list_node_t *sTempHead;
    acp_list_node_t *sTempHeadNext;

    ACP_LIST_ITERATE_SAFE(&aPool->mMemoryList, sTempHead, sTempHeadNext)
    {
        ((uluMemory *)sTempHead)->mOp->mDestroyMyself((uluMemory *)sTempHead);
    }

    acpMemFree(aPool);

    return;
}

/*
 * ==================================================
 * uluChunkPool : Get Reference Count of a Chunk Pool
 * ==================================================
 */

static acp_uint32_t uluChunkPoolGetReferenceCount(uluChunkPool *aPool)
{
    return aPool->mMemoryCnt;
}

/*
 * ==================================================
 * uluChunkPool : Operation Sets
 * ==================================================
 */

uluChunkPool *uluChunkPoolCreate(acp_uint32_t aDefaultChunkSize, acp_uint32_t aNumOfSavepoints, acp_uint32_t aMinChunkCnt)
{
    acp_bool_t    sNeedToFreeChunkPool = ACP_FALSE;
    uluChunkPool *sChunkPool = NULL;

    acp_uint32_t  sSizeUluMemory;
    acp_uint32_t  sActualSizeOfDefaultSingleChunk;

    /*
     * Note : 세이브 포인트의 갯수가 청크 사이즈의 반이 넘으면 의미가 없다.
     */
    ACI_TEST(aNumOfSavepoints >= (aDefaultChunkSize >> 1));

    ACI_TEST(acpMemAlloc((void**)&sChunkPool, ACI_SIZEOF(uluChunkPool)) != ACP_RC_SUCCESS);

    sNeedToFreeChunkPool = ACP_TRUE;

    aDefaultChunkSize               = ACP_ALIGN8(aDefaultChunkSize);
    sSizeUluMemory                  = ACP_ALIGN8(ACI_SIZEOF(uluMemory));
    sActualSizeOfDefaultSingleChunk = aDefaultChunkSize + sSizeUluMemory;

    acpListInit(&sChunkPool->mMemoryList);
    acpListInit(&sChunkPool->mFreeChunkList);
    acpListInit(&sChunkPool->mFreeBigChunkList);

    sChunkPool->mFreeChunkCnt        = 0;
    sChunkPool->mMinChunkCnt         = aMinChunkCnt;
    sChunkPool->mFreeBigChunkCnt     = 0;
    sChunkPool->mMemoryCnt           = 0;
    sChunkPool->mNumberOfSP          = aNumOfSavepoints;

    sChunkPool->mDefaultChunkSize    = aDefaultChunkSize;

    sChunkPool->mActualSizeOfDefaultSingleChunk
                                     = sActualSizeOfDefaultSingleChunk;

    sChunkPool->mOp                  = &gUluChunkPoolOp[gUluMemoryType];

    ACI_TEST(sChunkPool->mOp->mCreateInitialChunks(sChunkPool) != ACI_SUCCESS);

    return sChunkPool;

    ACI_EXCEPTION_END;

    if (sNeedToFreeChunkPool)
    {
        sChunkPool->mOp->mDestroyMyself(sChunkPool);
        sChunkPool = NULL;
    }

    return sChunkPool;
}


/*
 * ====================================
 *
 * uluMemory
 *
 * ====================================
 */

/*
 * ------------------------------
 * uluMemory : Alloc
 * ------------------------------
 */

static ACI_RC uluMemoryAlloc_MANAGER(uluMemory *aMemory, void **aPtr, acp_uint32_t aSize)
{
    void            *sAddrForUser;

    acp_bool_t       sFound = ACP_FALSE;

    acp_list_node_t *sTempNode;
    uluChunk        *sChunkForNewMem;

    /*
     * align 이 4 라고 가정할 때,
     * 여유 메모리가 3 이고, 요청된 사이즈가 3 일 때에도
     * align 한 값이 여유 메모리를 초과하므로 그냥 에러를 리턴하자.
     */
    aSize = ACP_ALIGN8(aSize);

    if (aMemory->mMaxAllocatableSize >= aSize)
    {
        /*
         * 충분한 메모리를 가진 청크를 찾아서 선택한다.
         */
        ACP_LIST_ITERATE(&(aMemory->mChunkList), sTempNode)
        {
            if (((uluChunk *)sTempNode)->mFreeMemSize >= aSize)
            {
                sFound = ACP_TRUE;
                break;
            }
        }
    }

    if (sFound == ACP_FALSE)
    {
        sChunkForNewMem = aMemory->mChunkPool->mOp->mGetChunk(aMemory->mChunkPool, aSize);
        ACI_TEST_RAISE(sChunkForNewMem == NULL, NOT_ENOUGH_MEMORY);

        acpListPrependNode(&(aMemory->mChunkList), (acp_list_t *)sChunkForNewMem);
    }
    else
    {
        sChunkForNewMem = (uluChunk *)sTempNode;
    }

    /*
     * 청크의 준비가 끝났으므로,
     * - 사용자에게 돌려줄 메모리 주소를 준비한다.
     * - mChunk.mFreeMemSize 를 조정한다.
     * - offset 을 조정한다.
     */

    sAddrForUser                   = sChunkForNewMem->mBuffer + sChunkForNewMem->mOffset;
    sChunkForNewMem->mFreeMemSize -= aSize;
    sChunkForNewMem->mOffset      += aSize;

    aMemory->mAllocCount++;


    /*
     * 메모리 주소 리턴
     */

    *aPtr = sAddrForUser;

    return ACI_SUCCESS;

    ACI_EXCEPTION(NOT_ENOUGH_MEMORY);

    ACI_EXCEPTION_END;

    *aPtr = NULL;

    return ACI_FAILURE;
}

static ACI_RC uluMemoryAlloc_DIRECT(uluMemory *aMemory, void **aBufferPtr, acp_uint32_t aSize)
{
    acp_bool_t       sNeedToFreeMemDesc = ACP_FALSE;
    uluChunk_DIRECT *sMemoryDesc = NULL;

    /*
     * descriptor 할당
     */

    ACI_TEST(acpMemAlloc((void**)&sMemoryDesc, ACI_SIZEOF(uluChunk_DIRECT))
             != ACP_RC_SUCCESS);
    sNeedToFreeMemDesc = ACP_TRUE;

    acpListInitObj(&sMemoryDesc->mList, sMemoryDesc);

    /*
     * 실제 버퍼 할당
     */

    sMemoryDesc->mBufferSize = aSize;

    ACI_TEST(acpMemAlloc((void**)&sMemoryDesc->mBuffer, aSize) != ACP_RC_SUCCESS);

    /*
     * 메모리에 매달기
     */

    acpListPrependNode(&aMemory->mChunkList, (acp_list_node_t *)sMemoryDesc);
    aMemory->mAllocCount++;

    /*
     * 할당한 메모리 돌려주기
     */

    *aBufferPtr = sMemoryDesc->mBuffer;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    if (sNeedToFreeMemDesc == ACP_TRUE)
    {
        acpMemFree(sMemoryDesc);
    }

    *aBufferPtr = NULL;

    return ACI_FAILURE;
}

/*
 * ----------------------------------
 * uluMemory : Destroy uluMemory
 * ----------------------------------
 */

static void uluMemoryDestroy_MANAGER(uluMemory *aMemory)
{
    acp_uint32_t     sFirst;
    acp_list_node_t *sTempHead;
    acp_list_node_t *sTempHeadNext;
    uluChunkPool    *sPool = aMemory->mChunkPool;

    /*
     * 내 자신을 청크 풀의 메모리 리스트에서 삭제한다.
     */

    uluChunkPoolRemoveMemory(sPool, aMemory);

    sFirst = 0;

    /*
     * 청크를 모두 반납한다.
     */

    ACP_LIST_ITERATE_SAFE(&aMemory->mChunkList, sTempHead, sTempHeadNext)
    {
        /*
         * uluMemory 가 있는 initial chunk 를 삭제하는 것을 피하기 위함.
         */

        if (sFirst == 0)
        {
            sFirst = 1;
            continue;
        }

        acpListDeleteNode(sTempHead);
        sPool->mOp->mReturnChunk(sPool, (uluChunk *)sTempHead);
    }

    /*
     * initial chunk 를 삭제
     */

    sPool->mOp->mReturnChunk(sPool, (uluChunk *)(aMemory->mChunkList.mNext));
}

static void uluMemoryFreeElement_DIRECT(uluChunk_DIRECT *aMemoryDesc)
{
    /*
     * buffer free
     */

    acpMemFree(aMemoryDesc->mBuffer);
    aMemoryDesc->mBufferSize = 0;

    /*
     * uluChunk_DIRECT free
     */

    acpListDeleteNode((acp_list_node_t *)aMemoryDesc);
    acpMemFree(aMemoryDesc);
}

static void uluMemoryDestroy_DIRECT(uluMemory *aMemory)
{
    uluChunk_DIRECT *sMemDesc;
    acp_list_node_t *sIterator;
    acp_list_node_t *sIterator2;

    /*
     * 내 자신을 청크 풀의 메모리 리스트에서 삭제한다.
     */

    uluChunkPoolRemoveMemory(aMemory->mChunkPool, aMemory);

    /*
     * 가지고 있는 모든 메모리를 free 한다.
     */

    ACP_LIST_ITERATE_SAFE(&aMemory->mChunkList, sIterator, sIterator2)
    {
        sMemDesc = (uluChunk_DIRECT *)sIterator;

        uluMemoryFreeElement_DIRECT(sMemDesc);

        aMemory->mFreeCount++;
    }

    /*
     * uluMemory 자신을 free
     */

    acpMemFree(aMemory->mSPArray);
    acpMemFree(aMemory);
}

/*
 * ----------------------------------
 * uluMemory : Mark Save Point
 * ----------------------------------
 */

/*
 * Note : mChunkPool->mNumberOfSP 가 3 이면,
 *        mSavepoints배열은 0 부터 3 까지의 배열 인자를 갖는다.
 *
 *        이것은 uluMemory 구조체용의 세이브 포인트 0 번과
 *        3개까지의 SP (1, 2, 3) 가 필요하기 때문이다.
 */

static ACI_RC uluMemoryMarkSP_MANAGER(uluMemory *aMemory)
{
    acp_uint32_t     sNewSPIndex = aMemory->mCurrentSPIndex + 1;
    acp_uint32_t     sTempSPIndex;
    acp_uint32_t     sCurrSPIndex;
    acp_list_node_t *sTempNode;

    ACI_TEST_RAISE(sNewSPIndex > aMemory->mChunkPool->mNumberOfSP ||
                   sNewSPIndex <= aMemory->mCurrentSPIndex ||
                   sNewSPIndex == 0,
                   INVALID_SP_INDEX);

    sCurrSPIndex = aMemory->mCurrentSPIndex;

    ACP_LIST_ITERATE(&aMemory->mChunkList, sTempNode)
    {
        for (sTempSPIndex = sCurrSPIndex + 1; sTempSPIndex < sNewSPIndex; sTempSPIndex++)
        {
            /*
             * Current SP Index 에 저장된 offset 을 New SP Index 바로 앞전까지의 SP Index 들에
             * 복사해 넣는다.
             */
            ((uluChunk *)sTempNode)->mSavepoints[sTempSPIndex] = ((uluChunk *)sTempNode)->mOffset;
        }

        /*
         * 새로운 SP Index 에 청크의 현재 offset 을 세팅한다.
         */
        ((uluChunk *)sTempNode)->mSavepoints[sTempSPIndex] = ((uluChunk *)sTempNode)->mOffset;
    }

    aMemory->mCurrentSPIndex = sNewSPIndex;

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_SP_INDEX);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC uluMemoryMarkSP_DIRECT(uluMemory *aMemory)
{
    acp_uint32_t sNewSPIndex = aMemory->mCurrentSPIndex + 1;

    ACI_TEST_RAISE(sNewSPIndex > aMemory->mChunkPool->mNumberOfSP ||
                   sNewSPIndex <= aMemory->mCurrentSPIndex ||
                   sNewSPIndex == 0,
                   INVALID_SP_INDEX);

    aMemory->mSPArray[aMemory->mCurrentSPIndex + 1] = aMemory->mChunkList.mPrev;

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_SP_INDEX);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ----------------------------------
 * uluMemory : Delete Last Save Point
 * ----------------------------------
 */

static ACI_RC uluMemoryDeleteLastSP(uluMemory *aMemory)
{
    if (aMemory->mCurrentSPIndex > 0)
    {
        aMemory->mCurrentSPIndex--;
    }

    return ACI_SUCCESS;
}

/*
 * ----------------------------------
 * uluMemory : Get Current Save Point
 * ----------------------------------
 */

static acp_uint32_t uluMemoryGetCurrentSP(uluMemory * aMemory)
{
    return aMemory->mCurrentSPIndex;
}
/*
 * -------------------------------------
 * uluMemory : Free to Save Point
 * -------------------------------------
 */

static ACI_RC uluMemoryFreeToSP_MANAGER(uluMemory *aMemory, acp_uint32_t aSPIndex)
{
    acp_list_node_t *sTempNode;
    acp_list_node_t *sTempNodeNext;

    ACE_DASSERT(aMemory != NULL);

    /*
     * Note sp 0 으로 가라는 것은 메모리가 처음 생성된 상태로 가라는 뜻이다.
     * 따라서 체크할 필요 없다.
     *
     * Note aSPIndex 가 mCurrentSPIndex 와 같더라도 거기까지 프리를 해야 한다.
     */

    ACI_TEST_RAISE(aSPIndex > aMemory->mCurrentSPIndex, INVALID_SP_INDEX);

    /*
     * 동작과정.
     * - uluMemory.mCurrentSPIndex 를 aSPIndex 로 세팅한다.
     * - uluMemory 가 가지고 있는 mChunkList 를 쭉 돌아보면서
     *   mSavepoints[aSPIndex] 가 0 인 놈이 있으면 청크 풀로 반납한다.
     */
    aMemory->mCurrentSPIndex = aSPIndex;

    ACP_LIST_ITERATE_SAFE(&(aMemory->mChunkList), sTempNode, sTempNodeNext)
    {
        if (((uluChunk *)sTempNode)->mSavepoints[aSPIndex] == 0)
        {
            acpListDeleteNode(sTempNode);

            aMemory->mChunkPool->mOp->mReturnChunk(aMemory->mChunkPool, (uluChunk *)sTempNode);
        }
        else
        {
            /*
             * Chunk 의 Offset 을 지정된 savepoint index 에 들어 있는 값으로 바꾼다.
             */
            ((uluChunk *)sTempNode)->mOffset = ((uluChunk *)sTempNode)->mSavepoints[aSPIndex];

            /*
             * Chunk 의 mFreeMemSize 를 다시 계산해서 세팅한다.
             */
            ((uluChunk *)sTempNode)->mFreeMemSize =
                ((uluChunk *)sTempNode)->mChunkSize - ((uluChunk *)sTempNode)->mOffset;
        }
    }

    aMemory->mFreeCount++;

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_SP_INDEX);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

static ACI_RC uluMemoryFreeToSP_DIRECT(uluMemory *aMemory, acp_uint32_t aSPIndex)
{
    uluChunk_DIRECT *sMemDesc;
    acp_list_node_t *sIterator1;
    acp_list_node_t *sIterator2;

    acp_list_t       sNewList;
    acp_list_node_t *aNode;

    ACI_TEST_RAISE(aSPIndex > aMemory->mCurrentSPIndex, INVALID_SP_INDEX);

    acpListInit(&sNewList);

    aNode = aMemory->mSPArray[aSPIndex];

    if (aNode != NULL)
    {
        /*
         * SP 가 가리키는 메모리블록은 해제되면 안된다.
         * 그 다음 블록부터 끝까지 해제해야 한다.
         */

        aNode = aNode->mNext;

        if (aNode != &aMemory->mChunkList)
        {
            acpListSplit(&aMemory->mChunkList, aNode, &sNewList);

            ACP_LIST_ITERATE_SAFE(&sNewList, sIterator1, sIterator2)
            {
                sMemDesc = (uluChunk_DIRECT *)sIterator1;

                /*
                 * Note : uluMemoryFreeElement_DIRECT() 에서 LIST_REMOVE 까지 한다
                 */

                uluMemoryFreeElement_DIRECT(sMemDesc);

                aMemory->mFreeCount++;
            }
        }
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION(INVALID_SP_INDEX);

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/*
 * ---------------------------------
 * uluMemory : Create
 * ---------------------------------
 */

uluMemory *uluMemoryCreate_MANAGER(uluChunkPool *aSourcePool)
{
    acp_uint32_t  sSizeUluMemory;
    uluMemory    *sNewMemory = NULL;
    uluChunk     *sInitialChunk;

    /*
     * 최초에 필요한 청크를 가져온다.
     */
    sInitialChunk = aSourcePool->mOp->mGetChunk(aSourcePool, aSourcePool->mDefaultChunkSize);
    ACI_TEST(sInitialChunk == NULL);

    sSizeUluMemory = ACP_ALIGN8(ACI_SIZEOF(uluMemory));

    /*
     * uluMemory 인스턴스를 위한 공간을 할당한 뒤에 청크의 savepoint index 0 번에
     * uluMemory 의 사이즈를 넣어 줌으로써 실수로 인한 initial chunk 의 반납을
     * 방지해야 한다.
     *
     * 세이브 포인트 인덱스는 1 베이스로 한다.
     * 왜냐하면 실수로 uluMemory 구조체가 들어가 있는 청크를 반납하면 안되기
     * 때문이다. uluMemory 구조체가 들어있는 청크는 uluMemory 가 파괴될 때까지
     * 반납하지 않는다. 또한 그 청크만 savepoint index 0 에 값을 가지게 된다.
     *
     * 새로 생성한 청크에 uluMemory 를 할당했으므로
     * uluChunk.mFreeMemSize 를 조정한다.
     */

    sNewMemory = (uluMemory *)(sInitialChunk->mBuffer);

    sInitialChunk->mOffset         = sSizeUluMemory;
    sInitialChunk->mFreeMemSize   -= sSizeUluMemory;
    sInitialChunk->mSavepoints[0]  = sInitialChunk->mOffset;
    sNewMemory->mCurrentSPIndex    = 0;

    acpListInitObj(&sNewMemory->mList, sNewMemory);
    acpListInit(&sNewMemory->mChunkList);

    sNewMemory->mMaxAllocatableSize = sInitialChunk->mFreeMemSize;
    sNewMemory->mAllocCount         = 0;
    sNewMemory->mFreeCount          = 0;
    sNewMemory->mSPArray            = NULL;

    sNewMemory->mChunkPool          = aSourcePool;

    uluChunkPoolAddMemory(aSourcePool, sNewMemory);

    /*
     * 할당받은 청크를 uluMemory 의 청크 리스트에 매단다.
     * 매달기 전에 링크헤드의 초기화를 반드시 해 주어야 한다.
     */

    acpListPrependNode(&sNewMemory->mChunkList, (acp_list_t *)sInitialChunk);

    return sNewMemory;

    ACI_EXCEPTION_END;

    return sNewMemory;
}

uluMemory *uluMemoryCreate_DIRECT(uluChunkPool *aSourcePool)
{
    acp_uint32_t  sSPArraySize;
    acp_bool_t    sNeedToFreeNewMemory = ACP_FALSE;
    uluMemory    *sNewMemory = NULL;

    /*
     * 필요한 메모리 할당
     */

    ACI_TEST(acpMemAlloc((void**)&sNewMemory, ACI_SIZEOF(uluMemory)) != ACP_RC_SUCCESS);
    sNeedToFreeNewMemory = ACP_TRUE;

    sSPArraySize = ACI_SIZEOF(acp_list_node_t) * aSourcePool->mNumberOfSP;
    ACI_TEST(acpMemAlloc((void**)&sNewMemory->mSPArray, sSPArraySize) != ACP_RC_SUCCESS);

    acpMemSet(sNewMemory->mSPArray, 0, sSPArraySize);

    /*
     * 초기화
     */

    acpListInitObj(&sNewMemory->mList, sNewMemory);

    sNewMemory->mMaxAllocatableSize = 0;
    sNewMemory->mAllocCount         = 0;
    sNewMemory->mFreeCount          = 0;
    sNewMemory->mCurrentSPIndex     = 0;
    sNewMemory->mChunkPool          = aSourcePool;

    acpListInit(&sNewMemory->mChunkList);

    uluChunkPoolAddMemory(aSourcePool, sNewMemory);

    return sNewMemory;

    ACI_EXCEPTION_END;

    if (sNeedToFreeNewMemory == ACP_TRUE)
    {
        acpMemFree(sNewMemory);
    }

    //BUG-25201 [CodeSonar] 메모리 해제후 포인터 리턴함.
    return NULL;
}

ACI_RC uluMemoryCreate(uluChunkPool *aSourcePool, uluMemory **aNewMem)
{
    uluMemory *sNewMemory;

    /*
     * create 와 initialize 를 분리하고 싶지만
     * 그러자면 구조의 변경이 불가피하므로 그냥 create 안에서 initialize 까지 한다.
     */

    sNewMemory = gUluMemoryOp[gUluMemoryType].mCreate(aSourcePool);
    ACI_TEST(sNewMemory == NULL);

    sNewMemory->mOp = &gUluMemoryOp[gUluMemoryType];

    *aNewMem = sNewMemory;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    *aNewMem = NULL;

    return ACI_FAILURE;
}

uluMemoryOpSet gUluMemoryOp[ULU_MEMORY_TYPE_MAX] =
{
    {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
    },

    {
        /*
         * ULU_MEMORY_TYPE_MANAGER
         */
        uluMemoryCreate_MANAGER,
        uluMemoryDestroy_MANAGER,

        uluMemoryAlloc_MANAGER,
        uluMemoryFreeToSP_MANAGER,

        uluMemoryMarkSP_MANAGER,

        uluMemoryDeleteLastSP,
        uluMemoryGetCurrentSP
    },

    {
        /*
         * ULU_MEMORY_TYPE_DIRECT
         */
        uluMemoryCreate_DIRECT,
        uluMemoryDestroy_DIRECT,

        uluMemoryAlloc_DIRECT,
        uluMemoryFreeToSP_DIRECT,

        uluMemoryMarkSP_DIRECT,

        uluMemoryDeleteLastSP,
        uluMemoryGetCurrentSP
    }
};

uluChunkPoolOpSet gUluChunkPoolOp[ULU_MEMORY_TYPE_MAX] =
{
    {
        NULL, NULL, NULL, NULL, NULL
    },

    {
        /*
         * ULU_MEMORY_TYPE_MANAGER
         */
        uluChunkPoolCreateInitialChunks_MANAGER,

        uluDestroyChunkPool_MANAGER,
        uluGetChunkFromPool_MANAGER,
        uluReturnChunkToPool_MANAGER,

        uluChunkPoolGetReferenceCount
    },

    {
        /*
         * ULU_MEMORY_TYPE_DIRECT
         */
        uluChunkPoolCreateInitialChunks_DIRECT,

        uluDestroyChunkPool_DIRECT,
        uluGetChunkFromPool_DIRECT,
        uluReturnChunkToPool_DIRECT,

        uluChunkPoolGetReferenceCount
    }
};


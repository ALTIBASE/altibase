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

#ifndef _O_ULU_MEMORY_H_
#define _O_ULU_MEMORY_H_  1

#include <acp.h>
#include <acl.h>
#include <aciTypes.h>

typedef enum uluMemoryType
{
    ULU_MEMORY_TYPE_NOT_SET,
    ULU_MEMORY_TYPE_MANAGER,
    ULU_MEMORY_TYPE_DIRECT,
    ULU_MEMORY_TYPE_MAX
} uluMemoryType;

typedef struct uluChunk         uluChunk;
typedef struct uluChunk_DIRECT  uluChunk_DIRECT;
typedef struct uluChunkPool     uluChunkPool;
typedef struct uluMemory        uluMemory;

/*
 * =============================
 * uluChunkPool
 * =============================
 */

typedef acp_uint32_t  uluChunkPoolGetRefCntFunc           (uluChunkPool *aPool);
typedef uluChunk     *uluChunkPoolGetChunkFunc            (uluChunkPool *aPool, acp_uint32_t aChunkSize);
typedef void          uluChunkPoolReturnChunkFunc         (uluChunkPool *aPool, uluChunk *aChunk);
typedef void          uluChunkPoolDestroyFunc             (uluChunkPool *aPool);
typedef ACI_RC        uluChunkPoolCreateInitialChunksFunc (uluChunkPool *aPool);

typedef struct uluChunkPoolOpSet
{
    uluChunkPoolCreateInitialChunksFunc *mCreateInitialChunks;

    uluChunkPoolDestroyFunc             *mDestroyMyself;

    uluChunkPoolGetChunkFunc            *mGetChunk;
    uluChunkPoolReturnChunkFunc         *mReturnChunk;

    uluChunkPoolGetRefCntFunc           *mGetRefCnt;

} uluChunkPoolOpSet;



struct uluChunkPool
{
    acp_list_t         mFreeChunkList;     /* Free Chunk 들의 리스트 */

    acp_list_t         mFreeBigChunkList;  /* default chunk size 보다 큰 크기의 메모리를 가지는
                                              청크들의 free list */

    acp_list_t         mMemoryList;        /* 이 청크 풀의 인스턴스에서 청크를 할당 받아
                                             사용하고 있는 uluMemory 들의 리스트 */

    acp_uint32_t       mMemoryCnt;         /* 이 청크 풀을 소스로 삼는 uluMemory의 갯수 */
    acp_uint32_t       mFreeChunkCnt;      /* Free Chunk 의 갯수 */
    acp_uint32_t       mFreeBigChunkCnt;   /* Free Big Chunk 의 갯수 */
    acp_uint32_t       mMinChunkCnt;       /* 최소로 유지할 free chunk 의 갯수 */

    acp_uint32_t       mDefaultChunkSize;  /* 하나의 청크의 디폴트 크기  */
    acp_uint32_t       mActualSizeOfDefaultSingleChunk;

    acp_uint32_t       mNumberOfSP;        /* 이 청크 풀에 있는 청크들이 가지는 세이브 포인트의 갯수.
                                              청크의 mSavepoints[] 배열은
                                              0 부터 mNumberOfSP+1 까지의 원소를 가진다.
                                              마지막 것은 current offset. */

    uluChunkPoolOpSet *mOp;
};

/*
 * 각종 사이즈의 정의
 *
 * (청크풀 생성시 세팅,고정)             (메모리 할당/해제시 마다 갱신)
 *  uluChunk.mChunkSize ---------+           uluChunk.mFreeMemSize
 *                               |                            |
 *                         |_____v____________________________|________|
 *                         |                            ______v________|
 *                         |                           |               |
 *  +----------+----+------+-----------+-------------------------------+
 *  | uluChunk : SP |SP    | uluMemory | data1 | data2 |   f r e e     |
 *  |          : [0]|[1..n]|           |       |       |               |
 *  +----------+----+------+-----------+-------------------------------+
 *                                     |_______________________________|
 *                                          |
 *  uluMemory.mMaxAllocatableSize ----------+
 *  (uluMemory 생성시 세팅.고정)
 *
 * =====================================
 * The Location Of uluMemory in a Chunk
 * =====================================
 *
 *             +-mBuffer-------+
 *             |               |
 *             |               v
 * +-----------|---+-----------+-----------+---------------------
 * | uluChunk  |   | Savepoints| uluMemory |Actual
 * | {..mBuffer..} |           | {       } |Available Memory ...
 * |               |[0,1,...,n]|           |
 * +---------------+-|---------+-----------+---------------------
 *                   |                     ^
 *                   |                     |
 *                   +---------------------+
 *          mBuffer + mSavepoints[0]
 *
 * 위와 같은 모양새를 가지는 청크는 uluMemory 를 Create 하면서 최초로 청크 풀로부터
 * 할당받는 청크 뿐이다.
 *
 * 다른 청크는 아래와 같은 모양새를 가지게 된다 :
 *
 *             +---------------+
 *             |               |
 *             |              \|/
 * +-----------|---+-----------+---------------------
 * | uluChunk  |   | Savepoints|Actual
 * | {..mBuffer..} |           |Available Memory ...
 * |               |[0,1,...,n]|
 * +---------------+-----------+---------------------
 */

/*
 * =============================
 * uluMemory
 * =============================
 */

typedef uluMemory    *uluMemoryCreateFunc      (uluChunkPool *aSourcePool);
typedef ACI_RC        uluMemoryMallocFunc      (uluMemory *aMemory, void **aPtr, acp_uint32_t aSize);
typedef ACI_RC        uluMemoryFreeToSPFunc    (uluMemory *aMemory, acp_uint32_t aSavepointIndex);
typedef ACI_RC        uluMemoryMarkSPFunc      (uluMemory *aMemory);
typedef ACI_RC        uluMemoryDeleteLastSPFunc(uluMemory *aMemory);
typedef acp_uint32_t  uluMemoryGetCurrentSPFunc(uluMemory *aMemory);
typedef void          uluMemoryDestroyFunc     (uluMemory *aMemory);

typedef struct uluMemoryOpSet
{
    uluMemoryCreateFunc         *mCreate;
    uluMemoryDestroyFunc        *mDestroyMyself;

    uluMemoryMallocFunc         *mMalloc;
    uluMemoryFreeToSPFunc       *mFreeToSP;

    uluMemoryMarkSPFunc         *mMarkSP;

    uluMemoryDeleteLastSPFunc   *mDeleteLastSP;
    uluMemoryGetCurrentSPFunc   *mGetCurrentSPIndex;

} uluMemoryOpSet;



struct uluMemory
{
    acp_list_node_t   mList;              /* 같은 chunk Pool 의 uluMemory 의 리스트 */
    uluChunkPool     *mChunkPool;         /* uluMemory 가 메모리를 가지고 올 ChunkPool */
    acp_list_t        mChunkList;         /* uluMemory 가 소유하고 있는 Chunk의 리스트 */

    acp_uint32_t      mCurrentSPIndex;    /* 마지막으로 Mark 한 SP 의 인덱스.
                                             처음 메모리가 생성되면 0 이다.
                                             처음 메모리 생성되었을 때 마킹된 0 번 SP 인덱스에는
                                             uluMemory의 사이즈가 들어간다.
                                             0 번 인덱스는 uluMemory 가 들어있는 initial chunk를
                                             실수로 반납하는 것을 막기 위해 reserve 되어 있다. */

    acp_uint32_t      mAllocCount;        /* 통계정보를 위한 필드 : 쓰일곳은 없는데, 그냥 궁금해서 */
    acp_uint32_t      mFreeCount;

    acp_uint32_t      mMaxAllocatableSize;

    acp_list_node_t **mSPArray;           /* direct alloc 일 때만 사용한다 */

    uluMemoryOpSet   *mOp;

};

/*
 * ==============================
 * Exported Function Declarations
 * ==============================
 */

uluChunkPool *uluChunkPoolCreate(acp_uint32_t aDefaultChunkSize, acp_uint32_t aNumOfSavepoints, acp_uint32_t aMinChunkCnt);
ACI_RC        uluMemoryCreate(uluChunkPool *aSourcePool, uluMemory **aNewMem);

#endif /* _O_ULU_MEMORY_H_ */


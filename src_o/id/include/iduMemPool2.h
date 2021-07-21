/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemPool2.h 32048 2009-04-08 06:35:00Z newdaily $
 **********************************************************************/

#ifndef _O_IDU_MEM_POOL2_H_
#define _O_IDU_MEM_POOL2_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduMutex.h>
#include <iduList.h>

#if defined (VC_WINCE)
#include <WinCE_Assert.h>
#endif /*VC_WINCE*/

struct iduMemPool2ChunkHeader;
struct iduMemPool2FreeInfo;
class iduMemPool2;

/* Free Block List 유지시 사용 */
typedef struct iduMemPool2FreeInfo
{
    iduMemPool2FreeInfo    *mNext;
    SChar                  *mBlock;
    iduMemPool2ChunkHeader *mMyChunk;
} iduMemPool2FreeInfo;


/* i번째 Block의 Free Info를 가져온다. */
#define IDU_MEMPOOL2_BLOCK_FREE_INFO( aFreeChunk, i ) \
( (iduMemPool2FreeInfo*)( (SChar*)(aFreeChunk) + (aFreeChunk)->mChunkSize - ID_SIZEOF( iduMemPool2FreeInfo ) * (i + 1) ) )

/* Chunk의 첫번째 Block의 Chunk내에서의 위치 */
#define IDU_MEMPOOL2_FST_BLOCK( aFreeChunk ) \
( (SChar*)idlOS::align( (SChar*)(aFreeChunk) + ID_SIZEOF( iduMemPool2ChunkHeader ), (aFreeChunk)->mAlignSize ) )

/* Chunk의 n번째 Block */
#define IDU_MEMPOOL2_NTH_BLOCK( aFreeChunk, n ) ( IDU_MEMPOOL2_FST_BLOCK( aFreeChunk ) + aFreeChunk->mBlockSize * (n) )

/*
                   Chunk 구조
   *------------------------------------------*
   |         iduMemPool2ChunkHeader           |
   |------------------------------------------|
   |                                          |
   |         Block1, Block2 ... Block n       |
   |                                          |
   |------------------------------------------|
   |        iduMemPool2FreeInfo List          |
   *------------------------------------------*
*/
typedef struct iduMemPool2ChunkHeader
{
    /* 할당된 Chunk List */
    iduList                 mChunkNode;

    /* Chunk Matrix에 대한 Linked List */
    iduList                 mMtxNode;

    /* Chunk를 가지고 있는 iduMemPool2에 대한 Pointer */
    iduMemPool2            *mOwner;

    /* Chunk Size */
    UInt                    mChunkSize;

    /* Chunk에서 한번이라도 할당한 Block의 갯수 */
    UInt                    mNewAllocCnt;

    /* Chunk가 가질 수 있는 Total Block 수 */
    UInt                    mTotalBlockCnt;

    /* Alloc된 횟수 */
    UInt                    mFreeBlockCnt;

    /* Free된 Block이 많을 수록 작아진다. */
    UInt                    mFullness;

    /* 마지막으로 Free된 Block의 FreeInfo을 가리키고 있다. */
    iduMemPool2FreeInfo*    mFstFreeInfo;

    /* Block Size */
    UInt                    mBlockSize;

    /* 각 Block의 Align크기. */
    UInt                    mAlignSize;
} iduMemPool2ChunkHeader;


/*******************************************************************
      iduMemPool2 구조도

      |-This - Child 1 |
      |-Child 2        |
      |-Child 3        |  alloc
      |-Child 4        |  ---->  Client
      |-Child 5        |  free
      |- ...           |  <----
      *-Child n        |

      Child 1은 자기자신이다. 
      Child 1을 제외한 나머지 MemPool들은 새로 생성한다.

      Fullness: Chunk가 얼마나 많은 Block을 할당했는지에 따라
                Lev을 정한것으로 클수록 많은 Block을 할당한 것이다.

      Matrix[n]: 각 Fullness Level에 따라서 Chunk List를 구성했다.

      할당시 Fullness가 높은 Chunk를 먼저 할당하기 위해 Matrix의
      n의 값을 큰 값에서 작은 값으로 가면서 Chunk가 있는지 조사한다.

********************************************************************/

class iduMemPool2
{
private:
    /**
     * The number of groups of blocks we maintain based on what
     * fraction of the superblock is empty. NB: This number must be at
     * least 2, and is 1 greater than the EMPTY_FRACTION.
    */
    enum { BLOCK_FULLNESS_GROUP = 9 };

    /**
     * A thread heap must be at least 1/EMPTY_FRACTION empty before we
     * start returning blocks to the process heap.
     */
    enum { EMPTY_FRACTION = BLOCK_FULLNESS_GROUP - 1 };

    /** Reset value for the least-empty bin.  The last bin
     * (BLOCK_FULLNESS_GROUP-1) is for completely full blocks,
     * so we use the next-to-last bin.
     */
    enum { RESET_LEAST_EMPTY_BIN = BLOCK_FULLNESS_GROUP - 2 };

public:
    iduMemPool2(){};
    ~iduMemPool2(){};

    IDE_RC initialize( iduMemoryClientIndex aIndex,
                       SChar               *aName,
                       UInt                 aChildCnt,
                       UInt                 aBlockSize,
                       UInt                 aBlockCntInChunk,
                       UInt                 aChildCacheCnt,
                       UInt                 aAlignSize );

    IDE_RC destroy();

    IDE_RC memAlloc( void **aHandle, void **aAllocMemPtr );

    IDE_RC memFree( void* aFreeMemPtr );

    IDE_RC init( iduMemoryClientIndex aIndex,
                 SChar               *aName,
                 UInt                 aBlockSize,
                 UInt                 aBlockCntInChunk,
                 UInt                 aChildCacheCnt,
                 UInt                 aAlignSize );

    IDE_RC dest();

    IDE_RC alloc( void **aHandle,
                  void** aAllocMemPtr );

    void  status();
    ULong getMemSize();
    UInt  getBlockCnt4Chunk() { return mTotalBlockCntInChunk; }
    UInt  getApproximateFBCntByFN( UInt aTh ) { return mApproximateFBCntByFN[ aTh ]; }
    UInt  getFullnessCnt() { return BLOCK_FULLNESS_GROUP; }

/* For Testing */
public:
    void dumpMemPool4UnitTest();
    void dumpChunk4UnitTest( iduMemPool2ChunkHeader *aChunkHeader );

private:
    IDE_RC findAvailableChunk( iduMemPool2ChunkHeader  **aChunkPtr );

    IDE_RC checkAndFreeChunk( iduMemPool2ChunkHeader *aChunk );

    inline void initChunk( iduMemPool2ChunkHeader *aChunk );

    inline IDE_RC lock();
    inline IDE_RC unlock();

    iduMemPool2ChunkHeader* getFreeChunk();
    inline void addChunkToList( iduMemPool2ChunkHeader* aChunk );
    inline void removeChunkFromList( iduMemPool2ChunkHeader* aChunk );
    inline void removeChunkFromMP( iduMemPool2ChunkHeader* aChunk );
    inline void moveChunkToMP( iduMemPool2ChunkHeader* aChunk );

    inline UInt computeFullness( UInt aTotal, UInt aAvailable );
    inline iduMemPool2ChunkHeader* removeMaxChunk();

    inline void addBlkFreeInfo2FreeLst( iduMemPool2FreeInfo *aCurFreeInfo );
    inline void updateFullness( iduMemPool2ChunkHeader* aChunk );
    inline ULong getFreeBlockCnt();

private:
    iduMutex               mLock;

    /* MemPool List Count */
    UInt                   mPoolCnt;

    /* 할당받은 Chunk의 갯수. */
    ULong                  mChunkCnt;

    /* Chunk 크기. */
    UInt                   mChunkSize;

    /* Block 크기. */
    UInt                   mBlockSize;

    /*
      Child는 mChildCacheCnt이상의 Free Block Count를
      가지게 되면 가장많은 Free Block을 가진 Chunk를 Parent에게
      반환한다.
    */
    ULong                  mCacheBlockCnt;

    /* 하나의 Chunk가 가지는 총 Block수 */
    UInt                   mTotalBlockCntInChunk;

    /*
      Chunk가 mFullness에 따라서 mCHMatrix에 매달려 있는데 이 경우
      mFullness에 따라서 매달려 있는 Chunk의 갯수
    */
    UInt                   mFreeChkCntOfMatrix[ BLOCK_FULLNESS_GROUP ];

    /* Chunk의 mFullness에 따라서 가질 수 있는 대략의 Free Block 갯수 */
    UInt                   mApproximateFBCntByFN[ BLOCK_FULLNESS_GROUP ];

    /* Chunk Header Matrix
       Chunk의 mFullness에 따라서 mCHMarix[mFullness]에 추가됨 */
    iduList                mCHMatrix[ BLOCK_FULLNESS_GROUP ];

    /* 할당된 Chunk List Header */
    iduList                mChunkList;

    /* Free Chunk을 찾을때 mCHMatrix에서 mLeastBinIndex이
     * 가리키는 Chunk List부터 찾는다. 이 값은 Free Block의
     * 갯수가 적은 Chunk List를 되도록 가리키도록 한다. */
    SInt                   mLeastBinIndex;

    iduMemoryClientIndex   mIndex;

    /* MemPool List */
    iduMemPool2           *mMemMgrList;

    UInt                   mAlignSize;

    /* 할당된 Block 갯수 */
    ULong                  mAllocBlkCnt;
};

inline IDE_RC iduMemPool2::lock()
{
    return mLock.lock( NULL /* idvSQL* */ );
}

inline IDE_RC iduMemPool2::unlock()
{
    return mLock.unlock();
}

/* Chunk를 초기화 합니다. */
inline void iduMemPool2::initChunk( iduMemPool2ChunkHeader *aChunk )
{
    IDU_LIST_INIT_OBJ( &aChunk->mChunkNode, aChunk );
    IDU_LIST_INIT_OBJ( &aChunk->mMtxNode, aChunk );

    aChunk->mOwner         = this;
    aChunk->mChunkSize     = mChunkSize;
    aChunk->mBlockSize     = mBlockSize;
    aChunk->mNewAllocCnt   = 0;
    aChunk->mTotalBlockCnt = mTotalBlockCntInChunk;
    aChunk->mFreeBlockCnt  = mTotalBlockCntInChunk;
    aChunk->mFullness      = 0;

    aChunk->mFstFreeInfo   = NULL;
    aChunk->mAlignSize     = mAlignSize;
}

/* List에 대해 Chunk를 Add한다. */
inline void iduMemPool2::addChunkToList( iduMemPool2ChunkHeader* aChunk )
{
    IDU_LIST_ADD_AFTER( &mChunkList, &aChunk->mChunkNode );
}

/* List에서 Chunk를 제거한다. */
inline void iduMemPool2::removeChunkFromList( iduMemPool2ChunkHeader* aChunk )
{
    IDU_LIST_REMOVE( &aChunk->mChunkNode );
}

/* Matrix에서 Chunk를 제거한다. */
inline void iduMemPool2::removeChunkFromMP( iduMemPool2ChunkHeader* aChunk )
{
    IDU_LIST_REMOVE( &aChunk->mMtxNode );
}

/* Chunk를 자신의 Fullness맞는 Matrix에 추가한다. */
inline void iduMemPool2::moveChunkToMP( iduMemPool2ChunkHeader* aChunk )
{
    iduList *sCHListOfMatrix;
    SInt     sFullness = aChunk->mFullness;

    removeChunkFromMP( aChunk );

    sCHListOfMatrix = mCHMatrix + sFullness;

    IDU_LIST_ADD_AFTER( sCHListOfMatrix, &aChunk->mMtxNode );
}

/* Fullness를 계산한다 */
inline UInt iduMemPool2::computeFullness( UInt aTotal, UInt aAvailable )
{
    return (((BLOCK_FULLNESS_GROUP - 1)
             * (aTotal - aAvailable)) / aTotal);
}

/* Block의 Free Info를 Free Info List에 추가한다. */
inline void iduMemPool2::addBlkFreeInfo2FreeLst( iduMemPool2FreeInfo *aFreeInfo )
{
    iduMemPool2ChunkHeader *sChunk;

    IDE_ASSERT( aFreeInfo->mNext == NULL );

    sChunk = aFreeInfo->mMyChunk;

    aFreeInfo->mNext = sChunk->mFstFreeInfo;
    sChunk->mFstFreeInfo = aFreeInfo;

    sChunk->mFreeBlockCnt++;
}

/* Chunk의 Fullness새로 갱신한다. */
inline void iduMemPool2::updateFullness( iduMemPool2ChunkHeader* aChunk )
{
    SInt sOldFullness;
    SInt sNewFullness;

    sOldFullness = aChunk->mFullness;
    sNewFullness = computeFullness( aChunk->mTotalBlockCnt, aChunk->mFreeBlockCnt );

    if( sOldFullness != sNewFullness )
    {
        mFreeChkCntOfMatrix[ aChunk->mFullness ]--;
        aChunk->mFullness = sNewFullness;
        moveChunkToMP( aChunk );
        mFreeChkCntOfMatrix[ sNewFullness ]++;

        if( ( mLeastBinIndex < sNewFullness ) &&
            ( mLeastBinIndex != RESET_LEAST_EMPTY_BIN ) )
        {
            mLeastBinIndex = sNewFullness;
        }
    }

}

/* 현재 MemPool의 Free Block의 갯수를 구한다. */
inline ULong iduMemPool2::getFreeBlockCnt()
{
    return mTotalBlockCntInChunk * mChunkCnt - mAllocBlkCnt;
}

/* 현재 MemPool이 사용중인 Memory크기를 구한다 .*/
inline ULong iduMemPool2::getMemSize()
{
    return mChunkCnt * mChunkSize;
}

#endif  // _O_MEM_PORTAL_H_

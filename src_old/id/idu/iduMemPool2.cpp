/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMemPool2.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduMemPool2.h>

/*******************************************************************
 * Description: MemPool List초기화
 *
 *  aIndex           - [IN] iduMemPool2를 만든 Client Index
 *  aName            - [IN] iduMemPool2의 이름
 *  aChildCnt        - [IN] MemPool Cnt
 *  aBlockSize       - [IN] Block의 크기
 *  aBlockCntInChunk - [IN] Chunk내의 Block의 갯수
 *  aCacheSize       - [IN] MemPool이 이 크기의 메모리를 유지. 나머지
 *                          메모리는 OS에게 반환한다.
 *  aAlignSize       - [IN] "각 Block의 크기"와 "Block의 시작 주소"
 *                          이 크기로 Align된다.
 ********************************************************************/
IDE_RC iduMemPool2::initialize( iduMemoryClientIndex aIndex,
                                SChar               *aName,
                                UInt                 aChildCnt,
                                UInt                 aBlockSize,
                                UInt                 aBlockCntInChunk,
                                UInt                 aCacheSize,
                                UInt                 aAlignSize )
{
    UInt         i;
    iduMemPool2 *sCurMemPool2;

    IDE_ASSERT( aChildCnt  != 0 );
    IDE_ASSERT( aBlockSize != 0 );
    IDE_ASSERT( aAlignSize != 0 );

    /* 1개 이상의 MemPool갯수를 원하게 되면 현재 MemPool외의 다른
       MemPool을 생성한다. */
    IDE_TEST( init( aIndex,
                    aName,
                    aBlockSize,
                    aBlockCntInChunk,
                    aCacheSize / aChildCnt,
                    aAlignSize ) != IDE_SUCCESS );

    if( aChildCnt > 1 )
    {
        IDE_TEST( iduMemMgr::malloc( aIndex,
                                     ID_SIZEOF( iduMemPool2 ) * ( aChildCnt - 1 ),
                                     (void**)&mMemMgrList )
                  != IDE_SUCCESS );

        for( i = 0; i < aChildCnt - 1; i++ )
        {
            sCurMemPool2 = mMemMgrList + i;

#ifdef __CSURF__
            IDE_ASSERT( sCurMemPool2 != NULL );           
 
            new (sCurMemPool2) iduMemPool2;
#else
            sCurMemPool2 = new (sCurMemPool2) iduMemPool2;
#endif

            IDE_TEST( sCurMemPool2->init( aIndex,
                                          aName,
                                          aBlockSize,
                                          aBlockCntInChunk,
                                          aCacheSize / aChildCnt,
                                          aAlignSize )
                      != IDE_SUCCESS );
        }
    }

    /* init함수에서 이값을 0으로 초기화 하기때문에 반드시
       이후에 이값을 초기화 해야함 */
    mPoolCnt = aChildCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: 할당된 Chunk를 iduMemMgr에게 반환하고 할당된 Resource
 *              를 정리한다.
*******************************************************************/
IDE_RC iduMemPool2::destroy()
{
    UInt           i;
    iduMemPool2   *sCurMemPool2;

    for( i = 0; i < mPoolCnt - 1; i++ )
    {
        sCurMemPool2 = mMemMgrList + i;

        IDE_TEST( sCurMemPool2->dest() != IDE_SUCCESS );
    }

    IDE_TEST( dest() != IDE_SUCCESS );

    if( mPoolCnt > 1 )
    {
        IDE_TEST( iduMemMgr::free( mMemMgrList ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: Parent, Child의 실질적인 초기화 함수이다. 각각의
 *              Member를 초기화 한다.
 *
 *  aIndex           - [IN] iduMemPool2를 만든 Client Index
 *  aName            - [IN] iduMemPool2의 이름
 *  aBlockSize       - [IN] Block의 크기
 *  aBlockCntInChunk - [IN] Chunk내의 Block의 갯수
 *  aCacheSize       - [IN] MemPool이 이 크기의 메모리를 유지. 나머지
 *                     메모리는 OS에게 반환한다.
 *  aAlignSize       - [IN] "각 Block의 크기"와 "Block의 시작 주소"
 *                     이 크기로 Align된다.
 *******************************************************************/
IDE_RC iduMemPool2::init( iduMemoryClientIndex aIndex,
                          SChar               *aName,
                          UInt                 aBlockSize,
                          UInt                 aBlockCntInChunk,
                          UInt                 aCacheSize,
                          UInt                 aAlignSize )
{
    SInt      i;
    SFloat    sFreePercent;

    IDE_TEST( mLock.initialize( aName,
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS );

    /* UMR이 생기는 것을 방지 */
    mPoolCnt       = 0;
    mChunkCnt      = 0;
    mLeastBinIndex = RESET_LEAST_EMPTY_BIN;
    mAllocBlkCnt   = 0;

    mBlockSize  = idlOS::align( aBlockSize, aAlignSize );
    mChunkSize  = idlOS::align8( ID_SIZEOF( iduMemPool2ChunkHeader ) ) + aAlignSize +
        aBlockCntInChunk * ( mBlockSize + ID_SIZEOF( iduMemPool2FreeInfo ) );
    mTotalBlockCntInChunk = aBlockCntInChunk;

    mIndex = aIndex;

    mCacheBlockCnt  = aCacheSize / mBlockSize;
    mAlignSize      = aAlignSize;

    IDU_LIST_INIT( &mChunkList );

    sFreePercent = 1.0 / ( BLOCK_FULLNESS_GROUP + 1 );

    /* Initialize Matrix */
    for( i = 0; i < BLOCK_FULLNESS_GROUP; i++ )
    {
        IDU_LIST_INIT( mCHMatrix + i );

        mFreeChkCntOfMatrix[i] = 0;
        mApproximateFBCntByFN[i]  = (UInt)( mTotalBlockCntInChunk * ( 1 - sFreePercent * ( i + 1 ) ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: 현재 할당된 모든 Chunk를 OS에 반환한다.
 *******************************************************************/
IDE_RC iduMemPool2::dest()
{
    iduList *sCurNode;
    iduList *sNxtNode;

    IDU_LIST_ITERATE_SAFE( &mChunkList, sCurNode, sNxtNode )
    {
       IDE_ASSERT( mChunkCnt != 0 );

       mChunkCnt--;

       IDE_ASSERT( sCurNode->mObj != NULL );

       IDE_TEST( iduMemMgr::free( sCurNode->mObj ) != IDE_SUCCESS );
    }

    IDE_ASSERT( mChunkCnt == 0 );

    IDE_TEST( mLock.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: 여러개의 MemPool중에서 하나를 골라서 할당을 요청한다.
 *              할당시 하나의 Block이 할당된다.
 *
 * aMemHandle    - [OUT] 할당된 Memory의 Handle이 설정된다.
 * aAllocMemPtr  - [OUT] 할당된 Memory의 시작주소가 설정된다.
 *******************************************************************/
IDE_RC iduMemPool2::memAlloc( void **aMemHandle, void **aAllocMemPtr )
{
    UInt         sChildNo;
    iduMemPool2 *sAllocMemPool;

    IDE_ASSERT( mPoolCnt != 0 );

    sChildNo  = idlOS::getParallelIndex() % mPoolCnt;

    if( sChildNo == 0 )
    {
        sAllocMemPool = this;
    }
    else
    {
        sAllocMemPool = mMemMgrList + sChildNo - 1;
    }

    IDE_TEST( sAllocMemPool->alloc( aMemHandle, aAllocMemPtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: 하나의 Block할당을 요청한다.
 *
 * aMemHandle    - [OUT] 할당된 Memory의 Handle이 설정된다.
 * aAllocMemPtr  - [OUT] 할당된 Memory의 시작주소가 설정된다.
 *******************************************************************/
IDE_RC iduMemPool2::alloc( void **aMemHandle,
                           void **aAllocMemPtr )
{
    SInt                    sState = 0;
    SChar                  *sBlockPtr;
    iduMemPool2ChunkHeader *sFreeChunk;
    iduMemPool2FreeInfo    *sCurFreeInfo;

    *aMemHandle      = NULL;

    IDE_ASSERT( aAllocMemPtr != NULL );

    *aAllocMemPtr = NULL;

    IDE_TEST( lock() != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( findAvailableChunk( &sFreeChunk ) != IDE_SUCCESS );

    IDE_ASSERT( sFreeChunk->mOwner == this );

    IDE_ASSERT( sFreeChunk->mFreeBlockCnt != 0 );

    if( sFreeChunk->mNewAllocCnt != sFreeChunk->mTotalBlockCnt )
    {
        /* 아직 한번도 Alloc되지 않은 Free Block 이 존재 */
        sBlockPtr    = IDU_MEMPOOL2_NTH_BLOCK( sFreeChunk, sFreeChunk->mNewAllocCnt );
        sCurFreeInfo = IDU_MEMPOOL2_BLOCK_FREE_INFO( sFreeChunk, sFreeChunk->mNewAllocCnt );

        sFreeChunk->mNewAllocCnt++;

        sCurFreeInfo->mMyChunk = sFreeChunk;
        sCurFreeInfo->mNext    = NULL;
        sCurFreeInfo->mBlock   = sBlockPtr;
    }
    else
    {
        /* Free Chunk List에서 Block을 할당한다. */
        IDE_ASSERT( sFreeChunk->mFstFreeInfo != NULL );

        sCurFreeInfo = sFreeChunk->mFstFreeInfo;

        sFreeChunk->mFstFreeInfo = sCurFreeInfo->mNext;
        sCurFreeInfo->mNext = NULL;

        IDE_ASSERT( sCurFreeInfo->mMyChunk == sFreeChunk );

        sBlockPtr = sCurFreeInfo->mBlock;
    }

    sFreeChunk->mFreeBlockCnt--;

    updateFullness( sFreeChunk );

    *aAllocMemPtr = sBlockPtr;
    *aMemHandle   = sCurFreeInfo;

    mAllocBlkCnt++;

    sState = 0;
    IDE_TEST(unlock() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( unlock() == IDE_SUCCESS );
    }

    *aAllocMemPtr = NULL;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: aMemHandle이 가리키는 Block을 Chunk에 Free한다.
 *
 * aMemHandle - [IN] 할당된 Memory Block에 대한 Handle이다. aMemHandle은
 *                   실제로 iduMemPool2FreeInfo에 대한 Pointer이다.
*******************************************************************/
IDE_RC iduMemPool2::memFree( void *aMemHandle )
{
    iduMemPool2FreeInfo    *sBlockFreeInfo;
    iduMemPool2ChunkHeader *sCurChunk;
    iduMemPool2            *sOwner;
    SInt                    sState = 0;

    IDE_ASSERT( aMemHandle != NULL );

    sBlockFreeInfo = (iduMemPool2FreeInfo*)aMemHandle;

    sCurChunk = sBlockFreeInfo->mMyChunk;
    sOwner    = sCurChunk->mOwner;

    IDE_TEST( sOwner->lock() != IDE_SUCCESS );
    sState = 1;

    sOwner->addBlkFreeInfo2FreeLst( sBlockFreeInfo );

    sOwner->updateFullness( sCurChunk );

    sOwner->mAllocBlkCnt--;

    if( sCurChunk->mTotalBlockCnt == sCurChunk->mFreeBlockCnt )
    {
        IDE_TEST( sOwner->checkAndFreeChunk( sCurChunk )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sOwner->unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sOwner->unlock() == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: Free Block을 가지고 있는 Chunk을 찾아 준다. 이때
 *              여러개의 Free Chunk가 존재한다면 가장 할당이 많이
 *              된 Chunk를 준다. 이렇게 함으로써 Chunk의 모든 Block
 *              이 Free되어 OS에 해당 Chunk가 Free될 확율을 높인다.
 *
 * aChunkPtr - [OUT] 할당된 Chunk에 대한 Pointer
*******************************************************************/
IDE_RC iduMemPool2::findAvailableChunk( iduMemPool2ChunkHeader  **aChunkPtr )
{
    iduMemPool2ChunkHeader *sNewChunkPtr;
    iduMemPool2ChunkHeader *sCurChunkPtr;
    SInt                    sNewFullness;

    sCurChunkPtr = NULL;

    sCurChunkPtr = getFreeChunk();

    if( sCurChunkPtr == NULL )
    {
        sCurChunkPtr = NULL;
        sNewChunkPtr = NULL;

        IDE_TEST( iduMemMgr::malloc( mIndex,
                                     mChunkSize,
                                     (void**)&sNewChunkPtr )
                  != IDE_SUCCESS);

        initChunk( sNewChunkPtr );

        addChunkToList( sNewChunkPtr );

        sCurChunkPtr = sNewChunkPtr;

        sNewFullness = computeFullness( sCurChunkPtr->mTotalBlockCnt,
                                        sCurChunkPtr->mTotalBlockCnt );

        sCurChunkPtr->mFullness = sNewFullness;

        mChunkCnt++;

        moveChunkToMP( sCurChunkPtr );

        mFreeChkCntOfMatrix[ sCurChunkPtr->mFullness ]++;
    }
    else
    {
        IDE_ASSERT( sCurChunkPtr->mOwner == this );
    }

    *aChunkPtr = sCurChunkPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aChunkPtr = NULL;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: 현재 MemPool에 Free Block갯수가 Cache Block Count보다
 *              많으면 완전히 빈( Chunk내의 모든 Block이 Free된) Chunk
 *              를 Free시킨다.
 *
 * aChunk - [IN] Free할 Chunk Pointer
*******************************************************************/
IDE_RC iduMemPool2::checkAndFreeChunk( iduMemPool2ChunkHeader *aChunk )
{
    IDE_ASSERT( aChunk->mTotalBlockCnt == aChunk->mFreeBlockCnt );

    if( getFreeBlockCnt() > mCacheBlockCnt )
    {
        removeChunkFromList( aChunk );
        removeChunkFromMP( aChunk );

        IDE_ASSERT( aChunk->mFullness == 0 );
        IDE_ASSERT( mChunkCnt != 0 );

        mFreeChkCntOfMatrix[ aChunk->mFullness ]--;
        mChunkCnt--;

        IDE_TEST( iduMemMgr::free( aChunk ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************
 * Description: 가용 Block을 가진 Chunk를 찾는다. 이때 가급적
 *              많이 할당한 Chunk를 먼저 찾는다. 현재 Free Block을 가
 *              진 Chunk가 없다면 NULL을 Return한다.
*******************************************************************/
iduMemPool2ChunkHeader* iduMemPool2::getFreeChunk()
{
    SInt                    i;
    iduList                *sCurList;
    iduMemPool2ChunkHeader *sCurChunkPtr = NULL;

    IDE_ASSERT( mLeastBinIndex <= RESET_LEAST_EMPTY_BIN );

    for( i = mLeastBinIndex; i >= 0; i-- )
    {
        sCurList       = mCHMatrix + i;
        mLeastBinIndex = i;

        if( IDU_LIST_IS_EMPTY( sCurList ) == ID_FALSE )
        {
            sCurList = IDU_LIST_GET_NEXT( sCurList );
            sCurChunkPtr = (iduMemPool2ChunkHeader*)( sCurList->mObj );
            break;
        }
    }

    return sCurChunkPtr;
}

/*******************************************************************
 * Description: MemPool이 사용하고 있는 Memory정보를 뿌려준다.
*******************************************************************/
void iduMemPool2::status()
{
    SChar sBuffer[IDE_BUFFER_SIZE];

    idlOS::snprintf( sBuffer, ID_SIZEOF(sBuffer), "  - Memory Pool Size:%"ID_UINT64_FMT"\n",
                     getMemSize() );

    IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);
}

/*******************************************************************
 * Description: MemPool의 내부정보를 stdout으로 뿌려준다.
*******************************************************************/
void iduMemPool2::dumpMemPool4UnitTest()
{
    iduMemPool2ChunkHeader *sChunkHeader;
    UInt                    i;
    iduList                *sCurNode;

    idlOS::printf( "====== MemPool2 Info ======= \n" );
    idlOS::printf( "# Member Variable  \n" );
    idlOS::printf( "Pool  Count: %"ID_UINT32_FMT" \n"
                   "Chunk Count: %"ID_UINT64_FMT" \n"
                   "Chunk Size : %"ID_UINT32_FMT" \n"
                   "Block Size : %"ID_UINT32_FMT" \n"
                   "Cache Block Count: %"ID_UINT64_FMT" \n"
                   "Total Block Count In Chunk: %"ID_UINT32_FMT" \n"
                   "Align Size : %"ID_UINT32_FMT" \n"
                   "Alloc Block Count: %"ID_UINT32_FMT"\n"
                   "Pool  Count: %"ID_UINT64_FMT" \n",
                   mPoolCnt,
                   mChunkCnt,
                   mChunkSize,
                   mBlockSize,
                   mCacheBlockCnt,
                   mTotalBlockCntInChunk,
                   mAlignSize,
                   mAllocBlkCnt );

    idlOS::printf( "\n# Martrix Info \n" );
    idlOS::printf( "BLOCK_FULLNESS_GROUP Count: %"ID_INT32_FMT" \n"
                   "LeastBinIndex: %"ID_INT32_FMT" \n",
                   BLOCK_FULLNESS_GROUP,
                   mLeastBinIndex );

    for( i = 0; i < BLOCK_FULLNESS_GROUP; i++ )
    {
        idlOS::printf( "Free Chunk Count: %"ID_UINT32_FMT", "
                       "Approxi Free Block Count: %"ID_UINT32_FMT"\n",
                       mFreeChkCntOfMatrix[i],
                       mApproximateFBCntByFN[i] );

        IDU_LIST_ITERATE( mCHMatrix + i, sCurNode )
        {
            sChunkHeader = (iduMemPool2ChunkHeader*)( sCurNode->mObj );
            dumpChunk4UnitTest( sChunkHeader );
        }
    }
}

/*******************************************************************
 * Description: Chunk의 정보를 stdout에 뿌려준다.
*******************************************************************/
void iduMemPool2::dumpChunk4UnitTest( iduMemPool2ChunkHeader *aChunkHeader )
{
    UInt                   i;
    iduMemPool2FreeInfo   *sCurFreeInfo;

    idlOS::printf( "## Chunk Header\n" );

    idlOS::printf( "Chunk Size : %"ID_UINT32_FMT"\n"
                   "New Alloc Count: %"ID_UINT32_FMT"\n"
                   "Total Block Count: %"ID_UINT32_FMT"\n"
                   "Free  Block Count: %"ID_UINT32_FMT"\n"
                   "Fullness: %"ID_UINT32_FMT"\n"
                   "Block Size: %"ID_UINT32_FMT"\n"
                   "Align Size: %"ID_UINT32_FMT"\n",
                   aChunkHeader->mChunkSize,
                   aChunkHeader->mNewAllocCnt,
                   aChunkHeader->mTotalBlockCnt,
                   aChunkHeader->mFreeBlockCnt,
                   aChunkHeader->mFullness,
                   aChunkHeader->mBlockSize,
                   aChunkHeader->mAlignSize );

    idlOS::printf( "## Chunk Body\n" );

    sCurFreeInfo = aChunkHeader->mFstFreeInfo;

    for( i = 0 ; i < aChunkHeader->mFreeBlockCnt; i++ )
    {
        IDE_ASSERT( sCurFreeInfo != NULL );

        IDE_ASSERT( sCurFreeInfo->mMyChunk == aChunkHeader );

        sCurFreeInfo = sCurFreeInfo->mNext;
    }
}

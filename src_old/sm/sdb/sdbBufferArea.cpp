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
 * $$Id:$
 **********************************************************************/

/******************************************************************************
 * Description :
 *    sdbBufferArea 객체는 sdbBufferPool에게 BCB를 공급하는 역할을 한다.
 *    모든 frame, BCB들은 sdbBufferArea에서 생성되며 관리된다.
 *    frame과 BCB는 그 양을 시스템 구동중에 늘리거나 줄일 수 있도록
 *    chunk단위로 할당한다. 
 *
 ******************************************************************************/
#include <sdbBufferArea.h>
#include <sdbReq.h>
#include <sdbBufferMgr.h>
#include <smu.h>

/******************************************************************************
 * Description :
 *    BufferArea를 초기화한다. 이 Area가 가지는 chunk당 page의 개수와
 *    초기 chunk의 개수, 그리고 page의 size를 인자로 넘겨야 한다.
 *
 * Implementation :
 *    두개의 mutex를 초기화한다. IDE_FAILURE가 발생할 수 있는 경우는
 *    mutex의 초기화 실패뿐이다.
 *    
 * 총 버퍼 크기구하는 식 = aChunkPageCount * aChunkCount * aPageSize
 * 
 * aChunkPageCount  - [IN] chunk당 page의 개수
 * aChunkCount      - [IN] 이 BufferArea가 초기에 가지는 chunk의 개수
 * aPageSize        - [IN] 페이지 하나의 크기(바이트단위)
 ******************************************************************************/
IDE_RC sdbBufferArea::initialize(UInt aChunkPageCount,
                                 UInt aChunkCount,
                                 UInt aPageSize)
{
    SInt sState = 0;

    SMU_LIST_INIT_BASE(&mUnUsedBCBListBase);
    SMU_LIST_INIT_BASE(&mAllBCBList);

    //BUG-48042: BUFFER AREA Parallel 생성  
    UInt    sParDegree  = smuProperty::getBuffAreaCreatePDegree();
    idBool  sMemPoolMtx = ID_FALSE;    
     
    mChunkPageCount = aChunkPageCount;
    mPageSize       = aPageSize;
    mChunkCount     = 0; // expandArea에서 증가된다.
    mBCBCount       = 0;
    initBCBPtrRange();

    IDE_TEST( mBufferAreaLatch.initialize( (SChar*)"BUFFER_AREA_LATCH" )
              != IDE_SUCCESS );
    sState = 1;

    if ( sParDegree > 1 ) 
    {
        sMemPoolMtx = ID_TRUE;
    }

    IDE_TEST(mBCBMemPool.initialize(IDU_MEM_SM_SDB,
                                    (SChar*)"SDB_BCB_MEMORY_POOL",
                                    sParDegree,         // Multi Pool Cnt 
                                    ID_SIZEOF(sdbBCB),  // Block Size 
                                    mChunkPageCount,    // Block Cnt In Chunk 
                                    ID_UINT_MAX,        // chunk limit 
                                    sMemPoolMtx,        // use mutex
                                    8,                  // align byte
                                    ID_FALSE,			// ForcePooling
                                    ID_TRUE,			// GarbageCollection
                                    ID_TRUE,            /* HWCacheLine */
                                    IDU_MEMPOOL_TYPE_LEGACY   /* mempool type*/) 
             != IDE_SUCCESS);			
    sState = 2;

    IDE_TEST(mFrameMemPool.initialize(IDU_MEM_SM_SDB,
                                      (SChar*)"SDB_FRAME_MEMORY_POOL",
                                      sParDegree,      /* Multi Pool Cnt */
                                      mPageSize,       /* Block Size */
                                      mChunkPageCount, /* Block Cnt In Chunk */
                                      0,               /* Cache Size */
                                      mPageSize)       /* Align Size */
             != IDE_SUCCESS);
    sState = 3;

    IDE_TEST(mListMemPool.initialize(IDU_MEM_SM_SDB,
                                     (SChar*)"SDB_BCB_LIST_MEMORY_POOL",
                                     sParDegree,        // Multi Pool Cnt
                                     ID_SIZEOF(smuList),// Block Size 
                                     mChunkPageCount,   // Block Cnt In Chunk 
                                     ID_UINT_MAX,       // chunk limit 
                                     sMemPoolMtx,       // use mutex
                                     8,                 // align byte
                                     ID_FALSE,			// ForcePooling 
                                     ID_TRUE,			// GarbageCollection
                                     ID_TRUE,			// HWCacheLine
                                     IDU_MEMPOOL_TYPE_LEGACY /* mempool type */) 
             != IDE_SUCCESS);			
    sState = 4;

    // 실제로 BCB array와 frame chunk를 할당한다.
    IDE_TEST(expandArea(NULL, aChunkCount) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 4:
            IDE_ASSERT(mListMemPool.destroy() == IDE_SUCCESS);
        case 3:
            IDE_ASSERT(mFrameMemPool.destroy() == IDE_SUCCESS);
        case 2:
            IDE_ASSERT(mBCBMemPool.destroy() == IDE_SUCCESS);
        case 1:
            IDE_ASSERT(mBufferAreaLatch.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    sdbBufferArea를 해제한다. 내부적으로 할당했던 모든 frame chunk와
 *    BCB array와 node들을 모두 해제하고 mutex도 해제한다.
 *    destroy 호출 후 다시 initialize()를 호출하여 재사용할 수 있다.
 ******************************************************************************/
IDE_RC sdbBufferArea::destroy()
{
    freeAllAllocatedMem();

    IDE_ASSERT(mListMemPool.destroy() == IDE_SUCCESS);
    IDE_ASSERT(mFrameMemPool.destroy() == IDE_SUCCESS);
    IDE_ASSERT(mBCBMemPool.destroy() == IDE_SUCCESS);

    IDE_ASSERT(mBufferAreaLatch.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/******************************************************************************
 * Description :
 *  buffer area자신이 생성한 모든 BCB, list, Frame관련 정보 및 메모리를
 *  해제합니다.
 ******************************************************************************/
void sdbBufferArea::freeAllAllocatedMem()
{
    smuList *sNode;
    smuList *sBase;
    sdbBCB  *sBCB;

    sBase = &mAllBCBList;
    sNode = SMU_LIST_GET_FIRST(sBase);

    while (sNode != sBase)
    {
        SMU_LIST_DELETE(sNode);
        sBCB = (sdbBCB*)sNode->mData;

        //BUG-21053 서버 종료시 버퍼매니저의 뮤텍스를 전혀 해제하지 않습니다.
        IDE_ASSERT( sBCB->destroy() == IDE_SUCCESS );

        mFrameMemPool.memFree(sBCB->mFrameMemHandle);
        mBCBMemPool.memfree(sBCB);
        mListMemPool.memfree(sNode);

        sNode = SMU_LIST_GET_FIRST(sBase);
    }
}

/******************************************************************************
 * Description :
 *    aChunkCount 개수만큼의 새로운 BCB를 buffer Area내의 
 *    이와 함께 BCB array도 할당하고 free BCB 리스트를 구성한다.
 *    동시성 제어가 고려되어 있다.
 *    chunk당 page의 개수와 page size는 initialize할 때 정해진 값을 따른다.
 *
 *    + exception:
 *        - malloc에서 메모리 할당에 실패하면 exception이 발생할 수 있음
 *     
 *  aStatistics - [IN]  통계정보
 *  aChunkCount - [IN]  확장하려는 chunk의 개수
 ******************************************************************************/
IDE_RC sdbBufferArea::expandArea(idvSQL *aStatistics, UInt aChunkCount)
{
    UInt     i, j;
    UChar   *sFrame;
    sdbBCB  *sBCB;
    UInt     sBCBID;
    smuList *sNode;
    void    *sFrameMemHandle;
    UInt     sBCBCnt;
    
    //BUG-48042: BUFFER AREA Parallel 생성 
    smuWorkerThreadMgr   sThreadMgr;
    sdbBuffAreaJobInfo * sJob;   
    UInt                 sParDegree  = smuProperty::getBuffAreaCreatePDegree();  //Thread Cnt. 
    UInt                 sJobCnt     = 0;
    UInt                 sJobCntLast = 0;
    
    IDE_ASSERT(aChunkCount > 0);

    lockBufferAreaX(aStatistics);

    sBCBID  = mChunkPageCount * mChunkCount;
    sBCBCnt = mChunkPageCount * aChunkCount;

    //BUG-48042: Parallel degree 1인 경우
    if ( sParDegree == 1 )
    {
        for (i = 0; i < sBCBCnt; i++)
        {
            /* sdbBufferArea_expandArea_alloc_Frame.tc */
            IDU_FIT_POINT("sdbBufferArea::expandArea::alloc::Frame");
            IDE_TEST(mFrameMemPool.alloc(&sFrameMemHandle, (void**)&sFrame) != IDE_SUCCESS);

            /* sdbBufferArea_expandArea_alloc_BCB.tc */
            IDU_FIT_POINT("sdbBufferArea::expandArea::alloc::BCB");
            IDE_TEST(mBCBMemPool.alloc((void**)&sBCB) != IDE_SUCCESS);

            /* sdbBufferArea_expandArea_alloc_Node.tc */
            IDU_FIT_POINT("sdbBufferArea::expandArea::alloc::Node");
            IDE_TEST(mListMemPool.alloc((void**)&sNode) != IDE_SUCCESS);

            IDE_TEST(sBCB->initialize(sFrameMemHandle, sFrame, sBCBID) != IDE_SUCCESS);

            sNode->mData = sBCB;

            SMU_LIST_ADD_LAST(&mUnUsedBCBListBase, &sBCB->mBCBListItem);
            SMU_LIST_ADD_LAST(&mAllBCBList, sNode);

            mBCBCount++;
            sBCBID++;

            setBCBPtrRange( sBCB );
        }
    }
    else
    {
        //BUG-48042: parallel degree > 1 인 경우 ( 병렬 생성 ) 
        IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SDB,
                                     sParDegree, 
                                     (ULong)ID_SIZEOF( sdbBuffAreaJobInfo ), 
                                     (void **)&sJob )
                  != IDE_SUCCESS );
        
        sJobCnt     = sBCBCnt / sParDegree;
        sJobCntLast = sBCBCnt % sParDegree;

        //병렬 생성 초기화 
        IDE_TEST( smuWorkerThread::initialize( sdbBufferArea::expandAreaParallel,
                                               sParDegree,   /* Thread Count */
                                               sParDegree,   /* Queue Size */
                                               &sThreadMgr )
                  != IDE_SUCCESS ); 
        
        for ( i = 0 ; i < sParDegree ; i++ )
        {
            sJob[i].mStartBCBID = sBCBID + ( i * sJobCnt ); 

            if ( (i+1) != sParDegree )
            {
                sJob[i].mJobCnt = sJobCnt;
            } 
            else
            {
                //마지막 thread는 job을 더 가져감  
                sJob[i].mJobCnt = ( sJobCnt + sJobCntLast );
            }
            
            IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SDB,
                                         sJob[i].mJobCnt, 
                                         (ULong)ID_SIZEOF( sdbBuffAreaPtrSet ), 
                                         (void **)&(sJob[i].mPtrSet) )
                      != IDE_SUCCESS );
           
            IDE_TEST( smuWorkerThread::addJob( &sThreadMgr,
                                               (void *)&sJob[i] )
                      != IDE_SUCCESS );
        }

        IDE_TEST( smuWorkerThread::finalize( &sThreadMgr ) != IDE_SUCCESS );

        //할당한 BCB를 리스트에 달아 놓기 
        for ( i = 0 ; i < sParDegree ; i++ )
        {
            for ( j = 0 ; j < sJob[i].mJobCnt ; j++ )
            { 
                SMU_LIST_ADD_LAST(&mUnUsedBCBListBase, &(sJob[i].mPtrSet[j].mTmpBCB->mBCBListItem));
                SMU_LIST_ADD_LAST(&mAllBCBList, sJob[i].mPtrSet[j].mTmpNode);

                setBCBPtrRange( sJob[i].mPtrSet[j].mTmpBCB );
            }
            mBCBCount += sJob[i].mJobCnt ;

            IDE_TEST( iduMemMgr::free( sJob[i].mPtrSet ) != IDE_SUCCESS );
        }

        IDE_TEST( iduMemMgr::free( sJob ) != IDE_SUCCESS );
    }

    mChunkCount += aChunkCount;

    unlockBufferArea();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdbBufferArea::expandAreaParallel( void   * aJob )
{
    UInt i;
    sdbBuffAreaJobInfo* sBuffJob;
    
    sBuffJob = (sdbBuffAreaJobInfo *)aJob; 

    for ( i = 0 ; i < sBuffJob->mJobCnt ; i++ )
    {
        IDE_TEST( sdbBufferMgr::getArea()->mFrameMemPool.alloc( 
                                                    &(sBuffJob->mPtrSet[i].mTmpFrameMemHandle), 
                                                    (void**)&(sBuffJob->mPtrSet[i].mTmpFrame) ) 
                != IDE_SUCCESS );
        
        IDE_TEST( sdbBufferMgr::getArea()->mBCBMemPool.alloc(
                                                (void**)&(sBuffJob->mPtrSet[i].mTmpBCB)) 
                != IDE_SUCCESS );
        
        IDE_TEST( sdbBufferMgr::getArea()->mListMemPool.alloc(
                                                (void**)&(sBuffJob->mPtrSet[i].mTmpNode)) 
                != IDE_SUCCESS );

        IDE_TEST( sBuffJob->mPtrSet[i].mTmpBCB->initialize( 
                                                    sBuffJob->mPtrSet[i].mTmpFrameMemHandle, 
                                                    sBuffJob->mPtrSet[i].mTmpFrame, 
                                                    (sBuffJob->mStartBCBID + i) ) 
                != IDE_SUCCESS );

        sBuffJob->mPtrSet[i].mTmpNode->mData = sBuffJob->mPtrSet[i].mTmpBCB;
    }

    return;

    IDE_EXCEPTION_END;

    return;
}

/******************************************************************************
 * Description :
 *    주어진 aChunkCount 개수만큼 chunk를 해제한다.
 *    그 chunk에 속한 모든 BCB들은 버퍼에서 제거된다.
 ******************************************************************************/
IDE_RC sdbBufferArea::shrinkArea(idvSQL */*aStatistics*/, UInt /*aChunkCount*/)
{
    // 아직 지원하지 않는다.
    return IDE_FAILURE;
}

/******************************************************************************
 * Description :
 *    aFirst부터 aLast까지 구성된 BCB list를 BufferArea에 추가한다.
 *    aFirst부터 aLast까지 aCount개수가 맞는지는 내부적으로 검사하지 않기
 *    때문에 이 함수의 호출하는 곳에서 올바른 count 정보를 책임져야 한다.
 *
 *  aStatistics - [IN]  내부적으로 mutex를 획득하기 때문에
 *                      통계 정보를 넘겨야 한다.
 *  aCount      - [IN]  추가할 BCB list의 개수
 *  aFirst      - [IN]  추가할 BCB list의 처음. 이것의 mPrev는 NULL이어야 한다.
 *  aLast       - [IN]  추가할 BCB list의 마지막. 이것의 mNext는 NULL이어야 한다.
 ******************************************************************************/
void sdbBufferArea::addBCBs(idvSQL *aStatistics,
                            UInt    aCount,
                            sdbBCB *aFirst,
                            sdbBCB *aLast)
{
    IDE_DASSERT(aCount > 0);
    IDE_DASSERT(aFirst != NULL);
    IDE_DASSERT(aLast != NULL);

    lockBufferAreaX(aStatistics);
    SMU_LIST_ADD_LIST_FIRST(&mUnUsedBCBListBase,
                            &aFirst->mBCBListItem,
                            &aLast->mBCBListItem);
    mBCBCount += aCount;
    unlockBufferArea();
}

/******************************************************************************
 * Description :
 *    BufferArea가 가지고 있는 BCB를 하나 가져온다.
 *    반환되는 BCB는 리스트에서 제거되며 free상태이다.
 *    BufferArea에 BCB가 하나도 없으면 NULL이 반환된다.
 *
 *  aStatistics - [IN]  mutex 획득을 위한 통계정보
 ******************************************************************************/
sdbBCB* sdbBufferArea::removeLast(idvSQL *aStatistics)
{
    smuList *sNode;
    sdbBCB  *sRet = NULL;

    lockBufferAreaX(aStatistics);

    if (SMU_LIST_IS_EMPTY(&mUnUsedBCBListBase))
    {
        sRet = NULL;
    }
    else
    {
        sNode = SMU_LIST_GET_LAST(&mUnUsedBCBListBase);
        SMU_LIST_DELETE(sNode);
        mBCBCount--;

        sRet = (sdbBCB*)sNode->mData;
        SDB_INIT_BCB_LIST(sRet);
    }

    unlockBufferArea();

    return sRet;
}

/******************************************************************************
 * Description :
 *    이 BufferArea가 가지고 있는 모든 BCB list를 반환하고
 *    BufferArea는 0개의 BCB를 가진 상태가된다.
 *
 *  aStatistics - [IN]  mutex 획득을 위한 통계정보
 *  aFirst      - [OUT] 반환될 BCB list의 첫번째 BCB pointer
 *  aLast       - [OUT] 반환될 BCB list의 마지막 BCB pointer
 *  aCount      - [OUT] 반환될 BCB list의 BCB 개수
 ******************************************************************************/
void sdbBufferArea::getAllBCBs(idvSQL  *aStatistics,
                               sdbBCB **aFirst,
                               sdbBCB **aLast,
                               UInt    *aCount)
{
    lockBufferAreaX(aStatistics);

    if (SMU_LIST_IS_EMPTY(&mUnUsedBCBListBase))
    {
        *aFirst = NULL;
        *aLast  = NULL;
        *aCount = 0;
    }
    else
    {
        *aFirst = (sdbBCB*)SMU_LIST_GET_FIRST(&mUnUsedBCBListBase)->mData;
        *aLast  = (sdbBCB*)SMU_LIST_GET_LAST(&mUnUsedBCBListBase)->mData;
        *aCount = mBCBCount;
        SMU_LIST_CUT_BETWEEN(&mUnUsedBCBListBase, &(*aFirst)->mBCBListItem);
        SMU_LIST_CUT_BETWEEN(&(*aLast)->mBCBListItem, &mUnUsedBCBListBase);
        SMU_LIST_INIT_BASE(&mUnUsedBCBListBase);
        mBCBCount = 0;
    }
    
    unlockBufferArea();
}


/******************************************************************************
 * Description :
 * 
 * 본 함수를 통해 BCB를 접근하는 방식은 문제가 있다.
 * 가장 큰 문제는 BCB가 Buffer Pool의 어느곳에든 위치할 수 있다는 것이다.
 * 이러한 접근 방법을 제외했을때, BCB를 접근할 수 있는 방법은 오직 2가지
 * 뿐이었다. hash 또는 list의 end를 통해서...  그리고 이 2방식은 서로
 * 영향을 미치지 않기 때문에 동시성을 제어하는것이 상대적으로 수월하였다.
 *
 * 그런데, 모든 BCB를 접근하기 위해서 이 함수를 만들었다.
 * 그렇기 때문에 동시성을 잘 따져봐서 본 함수를 사용해야 한다. 
 *
 * 주의사항!
 *     list(LRU, Prepare, flush, flusher 개인 list)를 접근하는 트랜잭션은
 *    자신이 BCB를 list에서 제거하기만 하면 다른 트랜잭션이 그 BCB의 내용을
 *    변경하지 않는다고 생각한다.(fix와 touchCnt는 제외.. ) 그렇기 때문에
 *    이 트랜잭션들은 리스트에서 제거된 BCB에 대해서 dirty read를 마음대로 
 *    해버린다.  그렇기 때문에 이들에게 영향을 미치는 행위를 본 함수 수행 중
 *    해서는 안된다. 단지 읽기만 하는것은 문제가 되지 않으나, 쓰는 행동을
 *    할 경우에는 다른 트랜잭션과의 mutex를 잘 따져 가면서 섬세하게 해야한다.
 *
 *    동시성 관련된 자세한 사항은 sdbBufferPool.cpp의 
 *    ** BufferPool의 동시성 제어 ** 부분을 참고
 *    
 * aFunc    - [IN]  버퍼 area의 각 BCB에 적용할 함수
 * aObj     - [IN]  aFunc수행할때 필요한 변수
 ******************************************************************************/
IDE_RC sdbBufferArea::applyFuncToEachBCBs(
    idvSQL                *aStatistics,
    sdbBufferAreaActFunc   aFunc,
    void                  *aObj)
{
    smuList *sNode;
    sdbBCB  *sBCB;

    lockBufferAreaS( aStatistics );
    sNode = SMU_LIST_GET_FIRST(&mAllBCBList);
    while (sNode != &mAllBCBList)
    {
        sBCB = (sdbBCB*)sNode->mData;
        IDE_TEST(aFunc(sBCB, aObj) != IDE_SUCCESS);
        sNode = SMU_LIST_GET_NEXT(sNode);
    }
    unlockBufferArea();
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

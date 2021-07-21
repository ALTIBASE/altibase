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
 * $$Id: sdbBufferMgr.cpp 85333 2019-04-26 02:34:41Z et16 $
 **********************************************************************/

/***********************************************************************
 * PROJ-1568 BUFFER MANAGER RENEWAL
 **********************************************************************/
#include <idl.h>
#include <ide.h>
#include <smDef.h>
#include <sdbDef.h>
#include <sdbBCB.h>
#include <sdbReq.h>
#include <iduLatch.h>
#include <smErrorCode.h>

#include <sdbBufferPool.h>
#include <sdbFlushMgr.h>
#include <sdbBufferArea.h>
#include <sdbMPRMgr.h>

#include <sdbBufferMgr.h>
#include <smuQueueMgr.h>

#include <sdsFile.h>
#include <sdsBufferArea.h>
#include <sdsMeta.h>
#include <sdsBufferMgr.h> 

#define SDB_BCB_LIST_MIN_LENGTH  (32)

UInt          sdbBufferMgr::mPageCount;
iduMutex      sdbBufferMgr::mBufferMgrMutex;
sdbBufferArea sdbBufferMgr::mBufferArea;
sdbBufferPool sdbBufferMgr::mBufferPool;

/******************************************************************************
 * Description :
 *  BufferManager를 초기화 한다.
 ******************************************************************************/
IDE_RC sdbBufferMgr::initialize()
{
    UInt    sHashBucketDensity;
    UInt    sHashLatchDensity;
    UInt    sLRUListCnt;
    UInt    sPrepareListCnt;
    UInt    sFlushListCnt;
    UInt    sCPListCnt;

    ULong   sAreaSize;
    ULong   sAreaChunkSize;
    UInt    sBCBCntPerAreaChunk;
    UInt    sAreaChunkCnt;

    UInt    sTotalBCBCnt;
    UInt    sHashBucketCnt;

    UInt    sState = 0;

    sdbBCB *sFirstBCB = NULL;
    sdbBCB *sLastBCB  = NULL;

    UInt    sPageSize = SD_PAGE_SIZE;
    UInt    sBCBCnt;

    SChar   sMutexName[128];

    sdbBCB::mTouchUSecInterval = (ULong)smuProperty::getTouchTimeInterval() * (ULong)1000000;

    // 하나의 hash bucket에 들어가는 BCB개수
    sHashBucketDensity  = smuProperty::getBufferHashBucketDensity();
    // 하나의 hash chains latch당 BCB개수
    sHashLatchDensity   = smuProperty::getBufferHashChainLatchDensity();
    sLRUListCnt         = smuProperty::getBufferLRUListCnt();
    sPrepareListCnt     = smuProperty::getBufferPrepareListCnt();
    sFlushListCnt       = smuProperty::getBufferFlushListCnt();
    sCPListCnt          = smuProperty::getBufferCheckPointListCnt();

    sAreaSize           = smuProperty::getBufferAreaSize();
    sAreaChunkSize      = smuProperty::getBufferAreaChunkSize();

    if (sAreaSize < sAreaChunkSize)
    {
        sAreaChunkSize = sAreaSize;
    }

    // 사용자가 원한 sAreaSize가 그대로 버퍼매니저의 크기가 되지 않고,
    // 페이지 size와 buffer area chunk크기로 align down된다.
    sTotalBCBCnt    = sAreaSize / sPageSize;
    sHashBucketCnt  = sTotalBCBCnt / sHashBucketDensity;

    sBCBCntPerAreaChunk = sAreaChunkSize / sPageSize;

    // align down
    sAreaChunkCnt   = sTotalBCBCnt / sBCBCntPerAreaChunk;
    sTotalBCBCnt    = sAreaChunkCnt * sBCBCntPerAreaChunk;
    mPageCount      = sAreaChunkCnt * sBCBCntPerAreaChunk;

    // 리스트 개수 보정: 실제 생성되는 BCB갯수에 비해
    // 리스트의 갯수가 지나치게 많을 경우에 각 리스트 갯수를 비례해서 줄임.
    // 리스트 하나당 최소 SDB_BCB_LIST_MIN_LENGTH 만큼 BCB를 가져야 한다.
    if (mPageCount <= SDB_SMALL_BUFFER_SIZE)
    {
        sLRUListCnt     = 1;
        sPrepareListCnt = 1;
        sFlushListCnt   = 1;
    }
    else
    {
        while ((sLRUListCnt + sPrepareListCnt + sFlushListCnt)
               * SDB_BCB_LIST_MIN_LENGTH > mPageCount)
        {
            if (sLRUListCnt > 1)
            {
                sLRUListCnt /= 2;
            }
            if (sPrepareListCnt > 1)
            {
                sPrepareListCnt /= 2;
            }
            if (sFlushListCnt > 1)
            {
                sFlushListCnt /= 2;
            }

            if ((sLRUListCnt     == 1) &&
                (sPrepareListCnt == 1) &&
                (sFlushListCnt   == 1))
            {
                break;
            }
        }
    }

    idlOS::snprintf( sMutexName, 128, "BUFFER_MANAGER_EXPAND_MUTEX");
    IDE_TEST( sdbBufferMgr::mBufferMgrMutex.initialize(
                  sMutexName,
                  IDU_MUTEX_KIND_NATIVE,
                  IDV_WAIT_INDEX_LATCH_FREE_DRDB_BUFFER_MANAGER_EXPAND_MUTEX)
              != IDE_SUCCESS );
    sState =1;

    // mBufferPool을 처음에 생성하면 전혀 BCB가 존재하지 않는다.
    // 그러므로 mBufferArea에서 가져다가 mBufferPool에 넣어준다.
    // 이것은 차후에 Buffer Pool이 multiple buffer pool로
    // 갈 경우를 대비한 것이다.(현재는 buffer pool은 무조건 한개이다.)
    IDE_TEST( sdbBufferMgr::mBufferArea.initialize( sBCBCntPerAreaChunk,
                                                    sAreaChunkCnt,
                                                    sPageSize )
              != IDE_SUCCESS );
    sState =2;

    IDE_TEST( sdbBufferMgr::mBufferPool.initialize( sTotalBCBCnt,
                                                    sHashBucketCnt,
                                                    sHashLatchDensity,
                                                    sLRUListCnt,
                                                    sPrepareListCnt,
                                                    sFlushListCnt,
                                                    sCPListCnt)
              != IDE_SUCCESS );
    sState =3;

    sdbBufferMgr::mBufferArea.getAllBCBs( NULL,
                                          &sFirstBCB,
                                          &sLastBCB,
                                          &sBCBCnt);

    IDE_DASSERT( sBCBCnt == (sAreaChunkCnt * sBCBCntPerAreaChunk));

    sdbBufferMgr::mBufferPool.distributeBCB2PrepareLsts( NULL,
                                                         sFirstBCB,
                                                         sLastBCB,
                                                         sBCBCnt);

    IDE_TEST( sdbMPRMgr::initializeStatic() != IDE_SUCCESS );
    sState = 4;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    switch(sState)
    {
        case 4:
            (void)sdbMPRMgr::destroyStatic();
        case 3:
            sdbBufferMgr::mBufferPool.destroy();
        case 2:
            IDE_ASSERT( sdbBufferMgr::mBufferArea.destroy()  == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdbBufferMgr::mBufferMgrMutex.destroy() == IDE_SUCCESS );
        default:
            break;
    }
    return IDE_FAILURE;
}


/******************************************************************************
 * Description :
 *  BufferManager를 종료한다.
 ******************************************************************************/
IDE_RC sdbBufferMgr::destroy(void)
{
    IDE_ASSERT( sdbMPRMgr::destroyStatic() == IDE_SUCCESS );
    IDE_ASSERT( sdbBufferMgr::mBufferPool.destroy() == IDE_SUCCESS );
    IDE_ASSERT( sdbBufferMgr::mBufferArea.destroy() == IDE_SUCCESS );
    IDE_ASSERT( sdbBufferMgr::mBufferMgrMutex.destroy() == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/******************************************************************************
 * Description :
 *  BufferManager 크기를 변경한다.
 *  현재 버퍼매니저의 크기를 줄이는 기능은 제공하지 않는다.
 *  만약 줄이고 싶다면 서버를 내리고, 줄인 후 다시 서버를 올려야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aAskedSize  - [IN]  사용자가 요청한 크기
 *  aTrans      - [IN]  해당 연산을 수행한 트랜잭션
 *  aNewSize    - [OUT] resize되고 난 이후 실제 buffer manager 크기
 ******************************************************************************/
IDE_RC sdbBufferMgr::resize( idvSQL *aStatistics,
                             ULong   aAskedSize,
                             void   *aTrans ,
                             ULong  *aNewSize)
{
    ULong sAddSize;
    ULong sBufferAreaSize ;

    sBufferAreaSize = mBufferArea.getTotalCount() * SD_PAGE_SIZE;
    if( sBufferAreaSize > aAskedSize )
    {
        IDE_RAISE( shrink_func_is_not_supported );
    }
    else
    {
        sAddSize = aAskedSize - sBufferAreaSize;

        IDE_TEST( expand( aStatistics, sAddSize, aTrans,aNewSize )
                  != IDE_SUCCESS );

        ideLog::log(SM_TRC_LOG_LEVEL_BUFFER, SM_TRC_BUFFER_MGR_RESIZE,
                    aAskedSize, *aNewSize );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( shrink_func_is_not_supported );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_ILLEGAL_REQ_BUFFER_SIZE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/******************************************************************************
 * Description :
 *  BufferManager의 크기를 증가 한다.
 *
 * Implementation :
 *  1. "aAddedSize+현재 크기"를 통해 실제 확장되어질 크기를 구한다.
 *  2. buffer pool의 hash table의 크기를 실제 확장되어질 크기에 맞게
 *      먼저 증가 시킨다. hash table을 증가시키기전에 hash table에
 *      접근할 가능성이 있는 모든 트랜잭션 및 gc의 동작을 막는다.
 *  3. buffer area에 aAddedSize만큼을 요청한다.
 *  4. buffer area에 생긴 BCB들을 Buffer Pool에 넣어준다.
 *
 *  aStatistics - [IN]  통계정보
 *  aAddedSize  - [IN]  현재 BufferManager에 증가되는 크기. 실제론
 *                      sdbBufferArea의 chunk크기에 맞추어 aling down
 *                      되어서 더해진다.
 *  aTrans      - [IN]  해당 연산을 수행한 트랜잭션
 *  aNewSize    - [OUT] expand되고 난 후의 BufferManager의 크기를 리턴
 ******************************************************************************/
IDE_RC sdbBufferMgr::expand( idvSQL *aStatistics,
                             ULong   aAddedSize ,
                             void   *aTrans,
                             ULong  *aNewSize)
{
    UInt    sChunkPageCnt;
    UInt    sAddedChunkCnt;

    UInt    sCurrBCBCnt;
    UInt    sAddedBCBCnt;
    UInt    sTotalBCBCnt;

    sdbBCB *sFirstBCB;
    sdbBCB *sLastBCB;
    UInt    sCount;

    UInt    sHashBucketCnt;
    UInt    sHashBucketCntPerLatch = 0;

    UInt    sState      = 0;

    idBool  sSuccess    = ID_FALSE;

    sCurrBCBCnt     = sdbBufferMgr::mBufferArea.getTotalCount();
    sChunkPageCnt   = sdbBufferMgr::mBufferArea.getChunkPageCount();

    sAddedBCBCnt    = aAddedSize / SD_PAGE_SIZE;

    //align down
    sAddedChunkCnt  = sAddedBCBCnt / sChunkPageCnt;

    if( sAddedChunkCnt == 0 )
    {
        IDE_RAISE( invalid_buffer_expand_size );
    }
    else
    {
        //  1. "aAddedSize+현재 크기"를 통해 실제 확장되어질 크기를 구한다.
        sTotalBCBCnt   = sCurrBCBCnt + sAddedChunkCnt * sChunkPageCnt;
        sHashBucketCnt = sTotalBCBCnt / smuProperty::getBufferHashBucketDensity();
        sHashBucketCntPerLatch = smuProperty::getBufferHashChainLatchDensity();

        // 2. buffer pool의 hash table의 크기를 실제 확장되어질 크기에 맞게
        //    먼저 증가 시킨다. hash table을 증가시키기전에 hash table에
        //    접근할 가능성이 있는 모든 트랜잭션 및 gc의 동작을 막는다.
        blockAllApprochBufHash( aTrans, &sSuccess );

        // 트랜잭션 block이 실패한다면 에러를 리턴하고 더이상
        // resize를 수행하지 않는다.
        IDE_TEST_RAISE ( sSuccess == ID_FALSE, buffer_manager_busy );
        sState = 1;

        IDE_TEST( sdbBufferMgr::mBufferPool.resizeHashTable( sHashBucketCnt,
                                                             sHashBucketCntPerLatch)
                  != IDE_SUCCESS );

        // 3. buffer area에 aAddedSize만큼을 요청한다.
        IDE_TEST( sdbBufferMgr::mBufferArea.expandArea( aStatistics, sAddedChunkCnt)
                  != IDE_SUCCESS );

        // 4. buffer area에 생긴 BCB들을 Buffer Pool에 넣어준다.
        IDE_ASSERT( sdbBufferMgr::mBufferArea.getTotalCount() == sTotalBCBCnt );

        if( aNewSize != NULL )
        {
            *aNewSize = sTotalBCBCnt * SD_PAGE_SIZE;
        }

        sdbBufferMgr::mBufferArea.getAllBCBs( aStatistics,
                                              &sFirstBCB,
                                              &sLastBCB,
                                              &sCount);

        sdbBufferMgr::mBufferPool.expandBufferPool( aStatistics,
                                                    sFirstBCB,
                                                    sLastBCB,
                                                    sCount );

        unblockAllApprochBufHash();
        sState = 0;
        /* 주의!!
         * 일단 bufferPool에 distributBCB를 수행하고 나면,
         * rollback을 할 수가 없다. 왜냐면, bufferPool에 삽입을 하고 나면,
         * 다른 트랜잭션들이 바로 가져가서 써버리기 때문이다. 그렇기 때문에
         * distributeBCB이후에 abort가 나면 서버를 죽여야 한다.
         * 이것은 버퍼매니저 축소기능과 밀접한 관련이 있는 부분으로,
         * 버퍼매니저 축소 기능이 추가되면 해결된다.
         * */
        return IDE_SUCCESS;
    }

    IDE_EXCEPTION( invalid_buffer_expand_size );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_BUFFER_EXPAND_SIZE ));
    }
    IDE_EXCEPTION( buffer_manager_busy);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_BUFFER_MANAGER_BUSY));
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        unblockAllApprochBufHash();
    }

    return IDE_FAILURE;
}
/******************************************************************************
 * Description :
 *  BufferManager에 존재하는 Dirty BCB들을 flush하여 clean상태로 만든다.
 *  이때, 모든 BCB에 대해서 하는 것이 아니고, checkpoint 리스트에 달려있고,
 *  restart recovery LSN이 낮은 것부터 순서대로 flush한다.
 *  주로 주기적인 체크 포인트를 위해 호출된다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFlushAll   - [IN]  모든 페이지를 flush 해야하는지 여부
 ******************************************************************************/
IDE_RC sdbBufferMgr::flushDirtyPagesInCPList( idvSQL  * aStatistics,
                                              idBool    aFlushAll)
{
    IDE_TEST(sdbFlushMgr::flushDirtyPagesInCPList( aStatistics,
                                                   aFlushAll,
                                                   ID_FALSE )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbBufferMgr::flushDirtyPagesInCPListByCheckpoint( idvSQL  * aStatistics,
                                                          idBool    aFlushAll)
{
    IDE_TEST(sdbFlushMgr::flushDirtyPagesInCPList( aStatistics,
                                                   aFlushAll,
                                                   ID_TRUE )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************************
 * Description :
 *  단지 sdbBufferPool의 관련 함수를 호출하는 껍데기 역활만 한다.
 *  자세한 설명은 sdbBufferPool의 관련 함수를 참조
 *
 *  aBCB        - [IN]  해당 BCB
 *  aMtx        - [IN]  해당 aMtx
 ****************************************************************/
IDE_RC sdbBufferMgr::setDirtyPage( void *aBCB,
                                   void *aMtx )
{

    idvSQL *sStatistics = NULL;
    sStatistics = (idvSQL*)smLayerCallback::getStatSQL( aMtx );

    mBufferPool.setDirtyPage( sStatistics, aBCB, aMtx);

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  단지 sdbBufferPool의 관련 함수를 호출하는 껍데기 역활만 한다.
 *  자세한 설명은 sdbBufferPool의 관련 함수를 참조
 *
 *  aBCB        - [IN]  해당 BCB
 *  aLatchMode  - [IN]  래치 모드
 *  aMtx        - [IN]  해당 Mini transaction
 ****************************************************************/
IDE_RC sdbBufferMgr::releasePage( void            *aBCB,
                                  UInt             aLatchMode,
                                  void            *aMtx )
{
    idvSQL  *sStatistics;

    sStatistics= (idvSQL*)smLayerCallback::getStatSQL( aMtx );

    mBufferPool.releasePage( sStatistics, aBCB, aLatchMode );

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  단지 sdbBufferPool의 관련 함수를 호출하는 껍데기 역활만 한다.
 *  자세한 설명은 sdbBufferPool의 관련 함수를 참조
 *
 *  aStatistics - [IN]  통계정보
 *  aPagePtr    - [IN]  해당 페이지 frame pointer
 ****************************************************************/
IDE_RC sdbBufferMgr::releasePage( idvSQL * aStatistics,
                                  UChar  * aPagePtr )
{
    sdbBCB *sBCB;

    sBCB = getBCBFromPagePtr( aPagePtr );

    mBufferPool.releasePage( aStatistics, sBCB );

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  단지 sdbBufferPool의 관련 함수를 호출하는 껍데기 역활만 한다.
 *  자세한 설명은 sdbBufferPool의 관련 함수를 참조
 *
 *  aBCB        - [IN]  해당 BCB
 *  aLatchMode  - [IN]  래치 모드
 *  aMtx        - [IN]  해당 Mini transaction
 ****************************************************************/
IDE_RC sdbBufferMgr::releasePageForRollback( void         *aBCB,
                                             UInt          aLatchMode,
                                             void         *aMtx)
{
    idvSQL  *sStatistics;

    sStatistics = (idvSQL*)smLayerCallback::getStatSQL( aMtx );

    mBufferPool.releasePageForRollback( sStatistics, aBCB, aLatchMode );

    return IDE_SUCCESS;
}

void sdbBufferMgr::setDiskAgerStatResetFlag(idBool  )
{
    //TODO: 1568
    //IDE_ASSERT(0);
}

/****************************************************************
 * Description :
 *  페이지 프레임 포인터를 넘겨 받아 그것에 해당하는 BCB를 리턴한다.
 *
 *  aPage       - [IN]  해당 페이지 frame pointer
 ****************************************************************/
sdbBCB* sdbBufferMgr::getBCBFromPagePtr( UChar * aPage )
{
    sdbBCB *sBCB;
    UChar  *sPagePtr;

    IDE_ASSERT( aPage != NULL );

    sPagePtr = smLayerCallback::getPageStartPtr( aPage );
    sBCB     = (sdbBCB*)(((sdbFrameHdr*)sPagePtr)->mBCBPtr);

    if( mBufferArea.isValidBCBPtrRange( sBCB ) == ID_FALSE )
    {
        /* BUG-32528 disk page header의 BCB Pointer 가 긁혔을 경우에 대한
         * 디버깅 정보 추가.
         * Disk Page Header의 BCB Pointer 가 깨어졌다면
         * 앞 page에서 긁었을 가능성이 높다. */

        smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                        sPagePtr - SD_PAGE_SIZE,
                                        "[ Prev Page ]" );
        smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                        sPagePtr,
                                        "[ Curr Page ]" );
        smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                        sPagePtr + SD_PAGE_SIZE,
                                        "[ Next Page ]" );

        sBCB = (sdbBCB*)(((sdbFrameHdr*)(sPagePtr - SD_PAGE_SIZE))->mBCBPtr);
        sdbBCB::dump( sBCB );

        sBCB = (sdbBCB*)(((sdbFrameHdr*)(sPagePtr + SD_PAGE_SIZE))->mBCBPtr);
        sdbBCB::dump( sBCB );

        IDE_ASSERT(0);
    }

    if( sBCB->mSpaceID != ((sdbFrameHdr*)sPagePtr)->mSpaceID )
    {
        smLayerCallback::traceDiskPage( IDE_DUMP_0,
                                        sPagePtr,
                                        NULL );

        sdbBCB::dump( sBCB );

        IDE_ASSERT(0);
    }

    return sBCB;
}



#if 0 // not used
/****************************************************************
 * Description :
 *  GRID에 해당하는 page frame의 포인터를 리턴한다.
 *  이때, 반드시 버퍼에 존재하는 경우에만 리턴하고, 그렇지 않은경우에는
 *  디스크에서 읽지 않고 NULL을 리턴한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aGRID       - [IN]  GRID
 *  aPagePtr    - [OUT] 요청한 페이지
 ****************************************************************/
IDE_RC sdbBufferMgr::getPagePtrFromGRID( idvSQL     *aStatistics,
                                         scGRID      aGRID,
                                         UChar     **aPagePtr)
{
    sdbBCB    * sFindBCB;
    UChar     * sSlotDirPtr;

    IDE_DASSERT( SC_GRID_IS_NOT_NULL( aGRID ) );
    IDE_DASSERT(aPagePtr != NULL );

    *aPagePtr = NULL;

    mBufferPool.findBCB( aStatistics,
                         SC_MAKE_SPACE(aGRID),
                         SC_MAKE_PID(aGRID),
                         &sFindBCB);

    if ( sFindBCB != NULL )
    {
        if ( SC_GRID_IS_WITH_SLOTNUM(aGRID) )
        {
            sSlotDirPtr = smLayerCallback::getSlotDirStartPtr(
                                                        sFindBCB->mFrame );

            IDE_TEST( smLayerCallback::getPagePtrFromSlotNum(
                                                        sSlotDirPtr,
                                                        SC_MAKE_SLOTNUM(aGRID),
                                                        aPagePtr )
                       != IDE_SUCCESS );
        }
        else
        {
            *aPagePtr = sFindBCB->mFrame + SC_MAKE_OFFSET(aGRID);
        }
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;    
}
#endif

/****************************************************************
 * Description :
 *  현재 버퍼에 존재하는 BCB들의 recoveryLSN중 가장 작은 값을
 *  리턴한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aRet        - [OUT] 요청한 min recoveryLSN
 ****************************************************************/
void sdbBufferMgr::getMinRecoveryLSN(idvSQL *aStatistics,
                                     smLSN  *aRet)
{
    smLSN         sFlusherMinLSN;
    smLSN         sCPListMinLSN;
    sdbCPListSet *sCPL;

    sCPL = mBufferPool.getCPListSet();

    //주의!! 동시성 문제 때문에 아래 2연산의 순서는 반드시 보장해야 한다.
    sCPL->getMinRecoveryLSN(aStatistics, &sCPListMinLSN);
    sdbFlushMgr::getMinRecoveryLSN(aStatistics, &sFlusherMinLSN);

    if ( smLayerCallback::isLSNLT( &sCPListMinLSN,
                                   &sFlusherMinLSN )
         == ID_TRUE )
    {
        SM_GET_LSN(*aRet, sCPListMinLSN);
    }
    else
    {
        SM_GET_LSN(*aRet, sFlusherMinLSN);
    }
}

/******************************************************************************
 * Description :
 *    통계 정보를 시스템에 반영한다.
 ******************************************************************************/
void sdbBufferMgr::applyStatisticsForSystem()
{
    mBufferPool.applyStatisticsForSystem();
}


/*****************************************************************
 * Description:
 *  [BUG-20861] 버퍼 hash resize를 하기위해서 다른 트랜잭션들을 모두 접근하지
 *  못하게 해야 합니다.
 *
 *  hash table에 접근할수 있는 쓰레드들을 모두 막는다.
 *  이 함수 수행후 hash에 접근하는 쓰레드가 (이 함수를 수행한 쓰레드 외에)
 *  아무도 없음을 보장 할 수 있다.
 *
 *  aStatistics - [IN]  통계정보
 *  aTrans      - [IN]  본 함수 호출한 트랜잭션
 *  aSuccess    - [OUT] 성공여부 리턴
 ****************************************************************/
void  sdbBufferMgr::blockAllApprochBufHash( void     * aTrans,
                                            idBool   * aSuccess )
{
    ULong  sWaitTimeMicroSec = smuProperty::getBlockAllTxTimeOut() *1000*1000;

    /* 트랜잭션의 접근을 막는다. */
    smLayerCallback::blockAllTx( aTrans,
                                 sWaitTimeMicroSec,
                                 aSuccess );

    if( *aSuccess == ID_FALSE)
    {
        unblockAllApprochBufHash();
    }

    return;
}

/*****************************************************************
 * Description:
 *  [BUG-20861] 버퍼 hash resize를 하기위해서 다른 트랜잭션들을 모두 접근하지
 *  못하게 해야 합니다.
 *
 *  sdbBufferMgr::blockAllApprochBufHash 에서 막아놓았던
 *  쓰레드들을 해제한다. 이 함수 호출이후에 hash에 쓰레드들이 접근할 수 있다.
 *
 *  aTrans      - [IN]  본 함수 호출한 트랜잭션
 ****************************************************************/
void  sdbBufferMgr::unblockAllApprochBufHash()
{
    smLayerCallback::unblockAllTx();
}

/*****************************************************************
 * Description :
 *  어떤 BCB를 입력으로 받아도 항상 ID_TRUE를 리턴한다.
 ****************************************************************/
idBool sdbBufferMgr::filterAllBCBs( void   */*aBCB*/,
                                    void   */*aObj*/)
{
    return ID_TRUE;
}

/*****************************************************************
 * Description :
 *  aObj에는 spaceID가 들어있다. spaceID가 같은 BCB에 대해서
 *  ID_TRUE를 리턴
 *
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  함수 수행할때 필요한 자료구조. spaceID로 캐스팅해서 사용
 ****************************************************************/
idBool sdbBufferMgr::filterTBSBCBs( void   *aBCB,
                                    void   *aObj )
{
    scSpaceID *sSpaceID = (scSpaceID*)aObj;
    sdbBCB    *sBCB     = (sdbBCB*)aBCB;

    if( sBCB->mSpaceID == *sSpaceID )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/*****************************************************************
 * Description :
 *  aObj에는 특정 pid의 범위가 들어있다.
 *  BCB가 그 범위에 속할때만 ID_TRUE 리턴.
 *
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  함수 수행할때 필요한 자료구조. sdbBCBRange로 캐스팅해서
 *                      사용
 ****************************************************************/
idBool sdbBufferMgr::filterBCBRange( void   *aBCB,
                                     void   *aObj )
{
    sdbBCBRange *sRange = (sdbBCBRange*)aObj;
    sdbBCB      *sBCB   = (sdbBCB*)aBCB;

    if( sRange->mSpaceID == sBCB->mSpaceID )
    {
        if( (sRange->mStartPID <= sBCB->mPageID) &&
            (sBCB->mPageID <= sRange->mEndPID ))
        {
            return ID_TRUE;
        }
    }
    return ID_FALSE;
}

/****************************************************************
 * 주어진 조건에 맞는 단 하나의 BCB를 선택하기 위한 함수,
 *
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  함수 수행할때 필요한 자료구조. 특정 BCB정보를 가진다.
 ****************************************************************/
idBool sdbBufferMgr::filterTheBCB( void   *aBCB,
                                   void   *aObj )
{
    sdbBCB *sCmpBCB = (sdbBCB*)aObj;
    sdbBCB *sBCB = (sdbBCB*)aBCB;    

    if((sBCB->mPageID  == sCmpBCB->mPageID ) &&
       (sBCB->mSpaceID == sCmpBCB->mSpaceID))
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}


/****************************************************************
 * Description :
 *  페이지를 버퍼내에서 제거한다. 만약 BCB가 dirty라면 flush하지않고
 *  내용을 날려 버린다. 테이블 drop또는 테이블 스페이스 drop연산에
 *  해당하는 BCB는 이연산을 통해서 버퍼내에서 삭제하는 것이 좋다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 ****************************************************************/
IDE_RC sdbBufferMgr::removePageInBuffer( idvSQL     *aStatistics,
                                         scSpaceID   aSpaceID,
                                         scPageID    aPageID )
{
    sdbBCB  *sBCB = NULL;
    sdbBCB   sCmpBCB;
    idBool   sIsSuccess;
    idBool   sDummy;

    IDE_TEST( mBufferPool.findBCB( aStatistics,
                                   aSpaceID,
                                   aPageID,
                                   &sBCB )
              != IDE_SUCCESS );
 
    if ( sBCB != NULL )
    {
        sCmpBCB.mSpaceID = aSpaceID;
        sCmpBCB.mPageID  = aPageID;

        // 조건이 바뀔수 있으므로 filter조건을 넘겨주고,
        // 내부에서 Lock잡고 다시 검사
        mBufferPool.discardBCB( aStatistics,
                                sBCB,
                                filterTheBCB,
                                (void*)&sCmpBCB,
                                ID_TRUE,//WAIT MODE true
                                &sIsSuccess,
                                &sDummy );

        IDE_ASSERT( sIsSuccess == ID_TRUE );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************************
 * Description :
 *  페이지를 받아서 그 페이지에 해당하는 BCB에 pageType을 설정한다.
 *
 *  aPagePtr    - [IN]  page frame pointer
 *  aPageType   - [IN]  설정할 page type
 ****************************************************************/
void sdbBufferMgr::setPageTypeToBCB( UChar *aPagePtr, UInt aPageType )
{
    sdbBCB   *sBCB;

    IDE_DASSERT( aPagePtr == idlOS::align( (void*)aPagePtr, SD_PAGE_SIZE ) );

    sBCB = getBCBFromPagePtr( aPagePtr );

    sBCB->mPageType = aPageType;
}

void sdbBufferMgr::setDirtyPageToBCB( idvSQL * aStatistics,
                                      UChar  * aPagePtr )
{
    sdbBCB   *sBCB;

    IDE_DASSERT( aPagePtr == idlOS::align( (void*)aPagePtr, SD_PAGE_SIZE ) );

    sBCB = getBCBFromPagePtr( aPagePtr );
    mBufferPool.setDirtyPage( aStatistics, sBCB, NULL);
}

/************************************************************
 * Description :
 *  Discard:
 *  페이지를 버퍼내에서 제거한다. 만약 BCB가 dirty라면 flush하지않고
 *  내용을 날려 버린다. 테이블 drop또는 테이블 스페이스 drop연산에
 *  해당하는 BCB는 이연산을 통해서 버퍼내에서 삭제하는 것이 좋다.
 *
 *  aBCB    - [IN]  BCB
 *  aObj    - [IN]  반드시 sdbDiscardPageObj가 들어있어야 한다.
 *                  이 데이터 구조에는 aBCB를 discard할지 말지 결정하는
 *                  함수 및 데이터, 그리고 discard를 실패했을때 해당 BCB를
 *                  삽입할 큐를 가지고 있다.
 ************************************************************/
IDE_RC sdbBufferMgr::discardNoWaitModeFunc( sdbBCB    *aBCB,
                                            void      *aObj)
{
    idBool                  sIsSuccess;
    smuQueueMgr            *sWaitingQueue;
    sdbDiscardPageObj      *sObj = (sdbDiscardPageObj*)aObj;
    idBool                  sMakeFree;
    idvSQL                 *sStat = sObj->mStatistics;
    sdbBufferPool          *sPool = sObj->mBufferPool;

    if( sObj->mFilter( aBCB, sObj->mEnv ) == ID_TRUE )
    {
        if( aBCB->isFree() == ID_TRUE)
        {
            return IDE_SUCCESS;
        }

        sPool->discardBCB( sStat,
                           aBCB ,
                           sObj->mFilter,
                           sObj->mEnv,
                           ID_FALSE, // no wait mode
                           &sIsSuccess,
                           &sMakeFree);

        if( sMakeFree == ID_TRUE )
        {
            /* [BUG-20630] alter system flush buffer_pool을 수행하였을때,
             * hot영역은 freeBCB들로 가득차 있습니다.
             * free로 만든 BCB에 대해서는 리스트에서 제거하여
             * sdbPrepareList로 삽입한다.*/
            if( sPool->removeBCBFromList(sStat, aBCB ) == ID_TRUE )
            {
                IDE_ASSERT( sPool->addBCB2PrepareLst( sStat, aBCB )
                            == IDE_SUCCESS );
            }
        }

        sWaitingQueue = &(sObj->mQueue);

        if( sIsSuccess == ID_FALSE)
        {
            /* discard가 NoWait로 동작하기 때문에, 실패할 수있다.
             * 이경우에는 waitingQueue에 삽입하는데, 나중에 이 큐에 삽입된 BCB를
             * 다시 discard하기를 시도하든지.. 아님 딴거하던지.. 어쨋든 삽입한다.
             * 성공하면 waitingQueue에 삽입하지 않는다.
             * */
            IDE_ASSERT( sWaitingQueue->enqueue( ID_FALSE,//mutex잡을지 말지 여부
                                                (void*)&aBCB )
                        == IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
}

/************************************************************
 * Description :
 *  BCB가 담겨져 있는 Queue를 입력으로 받아 그 queue내에 존재하는 모든 BCB에
 *  대해서 discard를 수행한다. 이때, aFilter조건에 맞는것만 discard시키고,
 *  그렇지 않은 BCB는 큐에서 그냥 제거한다.
 *
 *  Wait mode란? BCB가 aFilter조건을 만족하지만, 내부적으로 바로 삭제할 수
 *  없는 경우가 있다. 예를 들면, BCB상태가 inIOB또는 Redirty상태인 경우엔
 *  삭제할 수 없다.
 *  [동시성] 왜냐면, inIOB또는 redirty상태인 경우엔 플러셔가 디스크에
 *  쓰기를 대기하고 있다는 뜻이된다. 그렇기 때문에, 섣불리 discard를 시켜
 *  버리면, 현재 pid를 가진 BCB는 버퍼내에서 삭제되었다가, 다른 트랜잭션
 *  에 의해 버퍼로 다시 올라올 수있고, 그 트랜잭션이 BCB에 내용을 기록하고
 *  종료 한 이후에, flusher가 자신의 IOB의 내용을 기록해 버릴 수 있기 때문에
 *  동시성이 깨질 우려가 있다.
 *  그래서 그냥 제거하지 않고, 상태가 변경되기를 기다린다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  aQueue에 있는 BCB중 aFilter조건에 맞는 BCB만 discard
 *  aFiltAgr    - [IN]  aFilter에 파라미터로 넣어주는 값
 *  aQueue      - [IN]  BCB들이 들어 있는 queue
 ************************************************************/
void sdbBufferMgr::discardWaitModeFromQueue( idvSQL      *aStatistics,
                                             sdbFiltFunc  aFilter,
                                             void        *aFiltAgr,
                                             smuQueueMgr *aQueue )
{
    sdbBCB *sBCB;
    idBool  sEmpty;
    idBool  sIsSuccess;
    idBool  sMakeFree;

    while(1)
    {
        IDE_ASSERT( aQueue->dequeue( ID_FALSE, // mutex를 잡지 않는다.
                                     (void*)&sBCB, &sEmpty)
                    == IDE_SUCCESS );

        if( sEmpty == ID_TRUE )
        {
            break;
        }

        mBufferPool.discardBCB(  aStatistics,
                                 sBCB,
                                 aFilter,
                                 aFiltAgr,
                                 ID_TRUE, // wait mode
                                 &sIsSuccess,
                                 &sMakeFree);

        IDE_ASSERT( sIsSuccess == ID_TRUE );
        
        if( sMakeFree == ID_TRUE )
        {
            /* [BUG-20630] alter system flush buffer_pool을 수행하였을때,
             * hot영역은 freeBCB들로 가득차 있습니다.
             * free로 만든 BCB에 대해서는 리스트에서 제거하여
             * sdbPrepareList로 삽입한다.*/
            if( mBufferPool.removeBCBFromList( aStatistics, sBCB ) == ID_TRUE )
            {
                IDE_ASSERT( mBufferPool.addBCB2PrepareLst( aStatistics, sBCB )
                            == IDE_SUCCESS );
            }
        }
    }

    return;
}

/****************************************************************
 * Description :
 *  BufferManager에 존재하는 모든 BCB에 대해서, Filt조건을 만족하는 BCB들을
 *  모두 discard시킨다.
 * Implementation:
 *  bufferArea에 모든 BCB가 있다. 이 각 BCB에 대해 discard를 순서대로 적용한다.
 *  그렇기 때문에, bufferArea에서 뒷부분에 있는 BCB의 경우엔 discard를
 *  적용하는데 시간이 걸린다. 그렇기 때문에, 처음엔 filt조건을 만족하다가도,
 *  discard를 적용할때는 filt를 만족하지 않을 수 있다.  이경우엔 무시하면 된다.
 *
 *  반대의 경우라면?
 *  bufferArea의 앞부분에 있는 BCB가 처음엔 filt조건을 만족하지 않다가
 *  (혹은 만족하다가) discard함수 적용이 끝난 이후에 filt조건을 만족하게 된다면
 *  어떻게 되는가? 이런 일이 절대 발생하지 않기를 원한다면, 상위
 *  (이 함수를 호출하는 부분)에서 보장을 해주어야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  aFilter조건에 맞는 BCB만 discard
 *  aFiltAgr    - [IN]  aFilter에 파라미터로 넣어주는 값
 ****************************************************************/
IDE_RC sdbBufferMgr::discardPages( idvSQL        *aStatistics,
                                   sdbFiltFunc    aFilter,
                                   void          *aFiltAgr)
{
    sdbDiscardPageObj sObj;

    sObj.mFilter = aFilter;
    sObj.mStatistics = aStatistics;
    sObj.mBufferPool = &mBufferPool;
    sObj.mEnv = aFiltAgr;

    sObj.mQueue.initialize(IDU_MEM_SM_SDB, ID_SIZEOF( sdbBCB*));

    /* sdbBufferArea에 있는 모든 BCB에 대해서 discardNoWaitModeFunc을 적용한다.
     * 이때, noWailtMode로 동작시킨다. 그리고 미처 discard되지 못한 BCB는
     * 큐에 모아두었다가 wait mode로 discard시킨다.
     *
     * 두번에 나누어서 하는 이유? 예를 들어서 설명하면..
     * 버퍼에 1 ~ 100까지 BCB가 있는데, 짝수 번호인 BCB는 모두 dirty상태이면서
     * filt조건을 만족한다.
     * 이때, 1번이 inIOB상태이다.
     * 만약 wait모드로 discard를 시킨다면, 1번에서 상태가 clean으로 변할때가지
     * 대기한다.  이때, dirty인 BCB들은 다른 flusher에 의해서 디스크에 쓰여질 수
     * 있다. 이때 디스크에 쓰여 지면 질 수록 손해다. 왜냐면 discard의 경우엔
     * dirty를 flush없이 제거하기 때문이다.
     * 만약 위의 예에서 nowait모드로 동작하면,
     * 1번 BCB를 skip한 이후(큐에 삽입)에 재빠르게 모든 짝수 BCB의 dirty를
     * 삭제하기 때문에 전혀 disk IO를 유발 시키지 않는다.
     * 그리고 남은 1번 BCB를 wait모드로 처리하게 된다.
     * */
    IDE_ASSERT( mBufferArea.applyFuncToEachBCBs( aStatistics,
                                                 discardNoWaitModeFunc,
                                                 &sObj)
                == IDE_SUCCESS );


    discardWaitModeFromQueue( aStatistics,
                              aFilter,
                              aFiltAgr,
                              &sObj.mQueue );

    sObj.mQueue.destroy();

    return IDE_SUCCESS;
}


/****************************************************************
 * Description :
 *  버퍼매니저의 특정 pid범위에 속하는 모든 BCB를 discard한다.
 *  이때, pid범위에 속하면서 동시에 aSpaceID도 같아야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPID   - [IN]  pid 범위중 시작
 *  aEndPID     - [IN]  pid 범위중 끝
 ****************************************************************/
IDE_RC sdbBufferMgr::discardPagesInRange( idvSQL    *aStatistics,
                                          scSpaceID  aSpaceID,
                                          scPageID   aStartPID,
                                          scPageID   aEndPID)
{
    sdbBCBRange sRange;

    sRange.mSpaceID  = aSpaceID;
    sRange.mStartPID = aStartPID;
    sRange.mEndPID   = aEndPID;

    return sdbBufferMgr::discardPages( aStatistics,
                                       filterBCBRange,
                                       (void*)&sRange );
}

/****************************************************************
 * Description :
 *  버퍼매니저에 있는 모든 BCB를 discard시킨다.
 *
 *  aStatistics - [IN]  통계정보
 ****************************************************************/
IDE_RC sdbBufferMgr::discardAllPages(idvSQL     *aStatistics)
{
    return sdbBufferMgr::discardPages( aStatistics,
                                       filterAllBCBs,
                                       NULL );// 주석추가
}


/****************************************************************
 * Description :
 *  이 함수에서는 pageOut을 하기위한 전처리 작업을 수행한다. 즉,
 *  실제 pageOut을 하지 않고 BCB가 어떤 상태이냐에 따라 해당 Queue에 삽입한다.
 *  즉, 이함수는 PageOutTargetQueue 를 make하는데 사용하는 Function이다.
 *
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  본 함수 수행하는데 필요한 자료.
 *                      sdbPageOutObj로 캐스팅해서 사용한다.
 ****************************************************************/
IDE_RC sdbBufferMgr::makePageOutTargetQueueFunc( sdbBCB *aBCB,
                                                 void   *aObj)
{
    sdbPageOutObj *sObj = (sdbPageOutObj*)aObj;
    sdbBCBState sState;

    if( sObj->mFilter( aBCB , sObj->mEnv) == ID_TRUE )
    {
        sState = aBCB->mState;
        if( (sState == SDB_BCB_DIRTY) ||
            // BUG-21135 flusher가 flush작업을 완료하기 위해서는
            // INIOB상태의 BCB상태가 변경되길 기다려야 합니다.
            (sState == SDB_BCB_INIOB) ||
            (sState == SDB_BCB_REDIRTY))
        {
            IDE_ASSERT(
                sObj->mQueueForFlush.enqueue( ID_FALSE, //mutex를 잡지 않는다.
                                              (void*)&aBCB)
                        == IDE_SUCCESS );
        }

        IDE_ASSERT(
            sObj->mQueueForMakeFree.enqueue( ID_FALSE, // mutex를 잡지 않는다.
                                             (void*)&aBCB)
            == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  PageOut : 버퍼에 존재하는 페이지를 버퍼매니저에서 제거하는 기능. discard와
 *  다른건 pageOut인 경우에 flush를 한 이후에 제거를 한다는 점이다.
 *
 *  이 함수는 버퍼매니저에 존재하는 페이지 중 filt 조건에 해당하는 페이지에 대해서
 *  pageOut 시킨다.
 *
 * Implementation:
 *  큐는 2가지 종류가 있다.
 *  1. flush가 필요한 BCB를 모아놓은 큐:
 *  2. flush가 필요치 않고, makeFree를 하여야 BCB를 모아놓은 큐:
 *
 *  만약 BCB가 dirty이거나 redirty인 경우엔 1번과 2번 모두에 삽입한다.
 *  1번에 삽입하는 이유는 dirty를 없애기 위해서 이고, 2번에도 삽입하는
 *  이유는, 이 BCB도 역시 makeFree 해야 하기 때문이다.
 *
 *  dirty도 redirty도 아닌 BCB인 경우엔 2번 큐에만 삽입한다.
 *
 *  만약 처음엔 dirty가 아니었다가 큐에 삽입하고 난 이후에 dirty가 되면 어쩌지?
 *  이런 일은 발생해서는 안된다.  이것은 상위(이 함수를 이용하는 부분)에서
 *  보장해 주어야 한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  BCB중 aFilter조건에 맞는 BCB만 flush
 *  aFiltArg    - [IN]  aFilter에 파라미터로 넣어주는 값
 ****************************************************************/
IDE_RC sdbBufferMgr::pageOut( idvSQL            *aStatistics,
                              sdbFiltFunc        aFilter,
                              void              *aFiltAgr)
{
    sdbPageOutObj sObj;
    sdbBCB       *sBCB;
    sdsBCB       *sSBCB;
    smuQueueMgr  *sQueue;
    idBool        sEmpty;

    // BUG-26476
    // 모든 flusher들이 stop 상태이면 abort 에러를 반환한다.
    IDE_TEST_RAISE(sdbFlushMgr::getActiveFlusherCount() == 0,
                   ERR_ALL_FLUSHERS_STOPPED);

    sObj.mFilter = aFilter;
    sObj.mEnv    = aFiltAgr;

    sObj.mQueueForFlush.initialize( IDU_MEM_SM_SDB, ID_SIZEOF(sdbBCB *));
    sObj.mQueueForMakeFree.initialize( IDU_MEM_SM_SDB, ID_SIZEOF(sdbBCB *));

    /* buffer Area에 존재하는 모든 BCB를 돌면서 filt조건에 해당하는 BCB는 모두
     * queue에 모은다.*/
    IDE_ASSERT( mBufferArea.applyFuncToEachBCBs( aStatistics,
                                                 makePageOutTargetQueueFunc,
                                                 &sObj)
                == IDE_SUCCESS );

    /*먼저 flush가 필요한 queue에대해 flush를 수행한다.*/
    IDE_TEST( sdbFlushMgr::flushObjectDirtyPages( aStatistics,
                                                  &(sObj.mQueueForFlush),
                                                  aFilter,
                                                  aFiltAgr)
              != IDE_SUCCESS );
    sObj.mQueueForFlush.destroy();

    sQueue = &(sObj.mQueueForMakeFree);
    while(1)
    {
        IDE_ASSERT( sQueue->dequeue( ID_FALSE, // mutex를 잡지 않는다.
                                     (void*)&sBCB, &sEmpty)
                    == IDE_SUCCESS );

        if( sEmpty == ID_TRUE )
        {
            break;
        }
        /* BCB를 free상태로 만든다. free상태인 BCB는 해시를 통해서 접근할 수
         * 없으며, 해당 pid가 무효이다.  그렇기 때문에, 버퍼에서 삭제된것으로
         * 볼 수 있다. */

        /* 연결된 SBCB가 있다면 delink */
        sSBCB = sBCB->mSBCB;
        if( sSBCB != NULL )
        {
            sdsBufferMgr::pageOutBCB( aStatistics,
                                      sSBCB );
            sBCB->mSBCB = NULL;
        }

        if( mBufferPool.makeFreeBCB( aStatistics, sBCB, aFilter, aFiltAgr )
            == ID_TRUE )
        {
            /* [BUG-20630] alter system flush buffer_pool을 수행하였을때,
             * hot영역은 freeBCB들로 가득차 있습니다.
             * makeFreeBCB를 수행하여 free로 만든 BCB에 대해서는 리스트에서 제거하여
             * sdbPrepareList로 삽입한다.*/
            if( mBufferPool.removeBCBFromList( aStatistics, sBCB ) == ID_TRUE )
            {
                IDE_ASSERT( mBufferPool.addBCB2PrepareLst( aStatistics, sBCB )
                            == IDE_SUCCESS );
            }
        }
    }
    sQueue->destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ALL_FLUSHERS_STOPPED);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AllFlushersStopped));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  해당 pid범위에 있는 모든 BCB들에게 pageout을 적용한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPID   - [IN]  pid 범위중 시작
 *  aEndPID     - [IN]  pid 범위중 끝
 ****************************************************************/
IDE_RC sdbBufferMgr::pageOutInRange( idvSQL         *aStatistics,
                                     scSpaceID       aSpaceID,
                                     scPageID        aStartPID,
                                     scPageID        aEndPID)
{
    sdbBCBRange sRange;

    sRange.mSpaceID     = aSpaceID;
    sRange.mStartPID    = aStartPID;
    sRange.mEndPID      = aEndPID;

    return sdbBufferMgr::pageOut( aStatistics,
                                  filterBCBRange,
                                  (void*)&sRange );
}

/****************************************************************
 * Description :
 *  해당 spaceID에 해당하는 모든 BCB들에게 pageout을 적용한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 ****************************************************************/
IDE_RC sdbBufferMgr::pageOutTBS( idvSQL          *aStatistics,
                                 scSpaceID        aSpaceID)
{
    return sdbBufferMgr::pageOut( aStatistics,
                                  filterTBSBCBs,
                                  (void*)&aSpaceID);
}

/****************************************************************
 * Description :
 *  버퍼매니저에 있는 모든 BCB들에 pageOut을 적용한다.
 *
 *  aStatistics - [IN]  통계정보
 ****************************************************************/
IDE_RC sdbBufferMgr::pageOutAll(idvSQL     *aStatistics)
{
    return sdbBufferMgr::pageOut( aStatistics,
                                  filterAllBCBs,
                                  NULL );
}

/****************************************************************
 * Description :
 *  FlushTargetQueue를 Make하는 Function
 * Implementation:
 *  해당 filt조건을 만족하면서 dirty, redirty인 BCB를 큐에 삽입한다.
 *
 *  aBCB        - [IN]  BCB
 *  aObj        - [IN]  본 함수 수행에 필요한 자료를 가지고 있다.
 *                      sdbFlushObj로 캐스팅해서 사용.
 ****************************************************************/
IDE_RC sdbBufferMgr::makeFlushTargetQueueFunc( sdbBCB *aBCB,
                                               void   *aObj)
{
    sdbFlushObj *sObj = (sdbFlushObj*)aObj;
    sdbBCBState sState;

    if( sObj->mFilter( aBCB , sObj->mEnv) == ID_TRUE )
    {
        sState = aBCB->mState;
        if( (sState == SDB_BCB_DIRTY) ||
            // BUG-21135 flusher가 flush작업을 완료하기 위해서는
            // INIOB상태의 BCB상태가 변경되길 기다려야 합니다.
            (sState == SDB_BCB_INIOB) ||
            (sState == SDB_BCB_REDIRTY))
        {
            IDE_ASSERT( sObj->mQueue.enqueue( ID_FALSE, // mutex를 잡지 않는다.
                                              (void*)&aBCB)
                        == IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
}

/****************************************************************
 * Description :
 *  filt에 해당하는 페이지를 flush한다.
 * Implementation:
 *  sdbFlushMgr::flushObjectDirtyPages함수를 이용하기 위해서,
 *  먼저 flush를 해야할 BCB들을 queue에 모아놓는다.
 *  그리고 sdbFlushMgr::flushObjectDirtyPages함수를 이용해 flush.
 *
 *  aStatistics - [IN]  통계정보
 *  aFilter     - [IN]  BCB중 aFilter조건에 맞는 BCB만 flush
 *  aFiltArg    - [IN]  aFilter에 파라미터로 넣어주는 값
 ****************************************************************/
IDE_RC sdbBufferMgr::flushPages( idvSQL            *aStatistics,
                                 sdbFiltFunc        aFilter,
                                 void              *aFiltAgr)
{
    sdbFlushObj sObj;

    // BUG-26476
    // 모든 flusher들이 stop 상태이면 abort 에러를 반환한다.
    IDE_TEST_RAISE(sdbFlushMgr::getActiveFlusherCount() == 0,
                   ERR_ALL_FLUSHERS_STOPPED);

    sObj.mFilter = aFilter;
    sObj.mEnv    = aFiltAgr;

    sObj.mQueue.initialize( IDU_MEM_SM_SDB, ID_SIZEOF(sdbBCB *));

    IDE_ASSERT( mBufferArea.applyFuncToEachBCBs( aStatistics,
                                                 makeFlushTargetQueueFunc,
                                                 &sObj)
                == IDE_SUCCESS );

    IDE_TEST( sdbFlushMgr::flushObjectDirtyPages( aStatistics,
                                                  &(sObj.mQueue),
                                                  aFilter,
                                                  aFiltAgr )
              != IDE_SUCCESS );
    sObj.mQueue.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ALL_FLUSHERS_STOPPED);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_AllFlushersStopped));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************
 * Description :
 *  버퍼매니저에서 해당 pid범위에 해당하는 BCB를 모두 flush한다.
 *
 *  aStatistics - [IN]  통계정보
 *  aSpaceID    - [IN]  table space ID
 *  aStartPID   - [IN]  pid 범위중 시작
 *  aEndPID     - [IN]  pid 범위중 끝
 ****************************************************************/
IDE_RC sdbBufferMgr::flushPagesInRange( idvSQL         *aStatistics,
                                        scSpaceID       aSpaceID,
                                        scPageID        aStartPID,
                                        scPageID        aEndPID)
{
    sdbBCBRange sRange;

    sRange.mSpaceID     = aSpaceID;
    sRange.mStartPID    = aStartPID;
    sRange.mEndPID      = aEndPID;

    return sdbBufferMgr::flushPages( aStatistics,
                                     filterBCBRange,
                                     (void*)&sRange );
}

/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdbBufferMgr::getPageByGRID( idvSQL             *aStatistics,
                                    scGRID              aGRID,
                                    sdbLatchMode        aLatchMode,
                                    sdbWaitMode         aWaitMode,
                                    void               *aMtx,
                                    UChar             **aRetPage,
                                    idBool             *aTrySuccess)
{
    IDE_RC    rc;
    sdRID     sRID;
    sdSID     sSID;

    IDE_DASSERT( SC_GRID_IS_NOT_NULL( aGRID ));

    if ( SC_GRID_IS_WITH_SLOTNUM(aGRID) )
    {
        sSID = SD_MAKE_SID_FROM_GRID( aGRID );

        rc = getPageBySID( aStatistics,
                           SC_MAKE_SPACE(aGRID),
                           sSID,
                           aLatchMode,
                           aWaitMode,
                           aMtx,
                           aRetPage,
                           aTrySuccess);
    }
    else
    {
        sRID = SD_MAKE_RID_FROM_GRID( aGRID );

        rc = getPageByRID( aStatistics,
                           SC_MAKE_SPACE(aGRID),
                           sRID,
                           aLatchMode,
                           aWaitMode,
                           aMtx,
                           aRetPage,
                           aTrySuccess);
    }

    return rc;
}


/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdbBufferMgr::getPageBySID( idvSQL             *aStatistics,
                                   scSpaceID           aSpaceID,
                                   sdSID               aPageSID,
                                   sdbLatchMode        aLatchMode,
                                   sdbWaitMode         aWaitMode,
                                   void               *aMtx,
                                   UChar             **aRetPage,
                                   idBool             *aTrySuccess )
{
    UChar    * sSlotDirPtr;
    idBool     sIsLatched = ID_FALSE;

    IDE_TEST( getPageByPID( aStatistics,
                            aSpaceID,
                            SD_MAKE_PID(aPageSID),
                            aLatchMode,
                            aWaitMode,
                            SDB_SINGLE_PAGE_READ,
                            aMtx,
                            aRetPage,
                            aTrySuccess,
                            NULL )
              != IDE_SUCCESS );
    sIsLatched = ID_TRUE;

    sSlotDirPtr = smLayerCallback::getSlotDirStartPtr( *aRetPage );

    IDU_FIT_POINT( "1.BUG-43091@sdbBufferMgr::getPageBySID::Jump" );
    IDE_TEST( smLayerCallback::getPagePtrFromSlotNum( sSlotDirPtr,
                                                      SD_MAKE_SLOTNUM(aPageSID),
                                                      aRetPage )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( (sIsLatched == ID_TRUE) && ( aMtx == NULL ) )
    {
        sIsLatched = ID_FALSE;
        (void)sdbBufferMgr::releasePage( aStatistics,
                                         (UChar*)*aRetPage );   
    }
    return IDE_FAILURE;
}

/* BUG-43844 : 이미 free된 슬롯(unused 플래그 체크된)에 접근하여 fetch를 
 * 시도 할 경우 스킵하도록 한다. getPageByGRID, getPageBySID 함수에 skipFetch 
 * 인자 추가. 
 */
IDE_RC sdbBufferMgr::getPageByGRID( idvSQL             *aStatistics,
                                    scGRID              aGRID,
                                    sdbLatchMode        aLatchMode,
                                    sdbWaitMode         aWaitMode,
                                    void               *aMtx,
                                    UChar             **aRetPage,
                                    idBool             *aTrySuccess,
                                    idBool             *aSkipFetch )
{
    IDE_RC    rc;
    sdRID     sRID;
    sdSID     sSID;

    IDE_DASSERT( SC_GRID_IS_NOT_NULL( aGRID ) );

    if ( SC_GRID_IS_WITH_SLOTNUM( aGRID ) )
    {
        sSID = SD_MAKE_SID_FROM_GRID( aGRID );

        rc = getPageBySID( aStatistics,
                           SC_MAKE_SPACE(aGRID),
                           sSID,
                           aLatchMode,
                           aWaitMode,
                           aMtx,
                           aRetPage,
                           aTrySuccess,
                           aSkipFetch );
    }
    else
    {
        sRID = SD_MAKE_RID_FROM_GRID( aGRID );

        rc = getPageByRID( aStatistics,
                           SC_MAKE_SPACE(aGRID),
                           sRID,
                           aLatchMode,
                           aWaitMode,
                           aMtx,
                           aRetPage,
                           aTrySuccess );
    }

    return rc;
}

IDE_RC sdbBufferMgr::getPageBySID( idvSQL             *aStatistics,
                                   scSpaceID           aSpaceID,
                                   sdSID               aPageSID,
                                   sdbLatchMode        aLatchMode,
                                   sdbWaitMode         aWaitMode,
                                   void               *aMtx,
                                   UChar             **aRetPage,
                                   idBool             *aTrySuccess,
                                   idBool             *aSkipFetch )
{
    UChar    * sSlotDirPtr;
    idBool     sIsLatched = ID_FALSE;

    IDE_TEST( getPageByPID( aStatistics,
                            aSpaceID,
                            SD_MAKE_PID(aPageSID),
                            aLatchMode,
                            aWaitMode,
                            SDB_SINGLE_PAGE_READ,
                            aMtx,
                            aRetPage,
                            aTrySuccess,
                            NULL )
              != IDE_SUCCESS );
    sIsLatched = ID_TRUE;

    sSlotDirPtr = smLayerCallback::getSlotDirStartPtr( *aRetPage );

    /* BUG-43844: 이미 free된 슬롯(unused 플래그 체크된)에 접근하여 fetch를
     * 시도 할 경우 스킵 */
    if ( SDP_IS_UNUSED_SLOT_ENTRY( SDP_GET_SLOT_ENTRY( sSlotDirPtr, 
                                                       SD_MAKE_SLOTNUM( aPageSID ) ) )
                                   == ID_TRUE )
    {
        *aSkipFetch = ID_TRUE;
        IDE_CONT( SKIP_GET_VAL );
    }
    else
    {
        *aSkipFetch = ID_FALSE;
    }

    IDU_FIT_POINT( "99.BUG-43091@sdbBufferMgr::getPageBySID::Jump" );
    IDE_TEST( smLayerCallback::getPagePtrFromSlotNum( sSlotDirPtr,
                                                      SD_MAKE_SLOTNUM(aPageSID),
                                                      aRetPage )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_GET_VAL );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if ( ( sIsLatched == ID_TRUE ) && ( aMtx == NULL ) )
    {
        sIsLatched = ID_FALSE;
        (void)sdbBufferMgr::releasePage( aStatistics,
                                         (UChar*)*aRetPage );   
    }
    return IDE_FAILURE;
}
/****************************************************************
 * Description :
 *  Pinning을 위해 쓰이는 기능, aEnv내에 해당 page가 존재하는지 검사하여
 *  있다면 해당 BCB를 리턴하고, 그렇지 않다면 NULL을 리턴한다.
 *
 *  aEnv        - [IN]  pinning env
 *  aSpaceID    - [IN]  table space ID
 *  aPageID     - [IN]  page ID
 ****************************************************************/
sdbBCB* sdbBufferMgr::findPinnedPage( sdbPinningEnv *aEnv,
                                      scSpaceID      aSpaceID,
                                      scPageID       aPageID )
{
    UInt i;

    IDE_DASSERT(aEnv != NULL);
    IDE_DASSERT(aSpaceID != SC_NULL_SPACEID);
    IDE_DASSERT(aPageID != SC_NULL_PID);

    for (i = 0; i < aEnv->mPinningCount; i++)
    {
        if ((aEnv->mPinningList[i].mSpaceID == SC_NULL_SPACEID) &&
            (aEnv->mPinningList[i].mPageID == SC_NULL_PID))
        {
            break;
        }
        if ((aSpaceID == aEnv->mPinningList[i].mSpaceID) &&
            (aPageID == aEnv->mPinningList[i].mPageID))
        {
            return aEnv->mPinningBCBList[i];
        }
    }
    return NULL;
}

/****************************************************************
 * Description :
 *  Pinning 기능을 위해 쓰이는 함수.
 *  mAccessHistory에 없다면, 먼저 mAccessHistory에 삽입한다.
 *  기존의 mAccessHistroy에 존재한다면, mPinningList에 삽입한다.
 *  삽입전에, (문제는 되지않지만) 현재 mPinningList에 같은 pid가 없음을
 *  보장하기 위해 findPinnedPage를 먼저 호출한다.
 *
 *  만약 mPinningList가 가득찬 경우엔 기존에 존재하는 BCB를 unPin시키고,
 *  현재 BCB를 pinning시켜야 한다.
 *
 *  aEnv        - [IN]  pinning env
 *  aBCB        - [IN]  BCB
 *  aBCBPinned  - [IN]  mAccessHistory에만 삽입하고, Pinning시키지
 *                      않았다면 ID_FALSE를 리턴, mPinningList에 삽입했다면
 *                      ID_TRUE를 리턴한다.
 *  aBCBToUnpin - [OUT] mPinningList에 삽입할때, 이미 가득찬 mPinningList라면
 *                      기존에 존재하는 BCB를 Unpin시켜야 한다. 이 Unpin시켜야
 *                      하는 BCB를 리턴한다.
 ****************************************************************/
void sdbBufferMgr::addToHistory( sdbPinningEnv     *aEnv,
                                 sdbBCB            *aBCB,
                                 idBool            *aBCBPinned,
                                 sdbBCB           **aBCBToUnpin )
{
    UInt i;

    IDE_DASSERT(aEnv != NULL);
    IDE_DASSERT(aBCB != NULL);

    *aBCBPinned  = ID_FALSE;
    *aBCBToUnpin = NULL;

    for (i = 0; i < aEnv->mAccessHistoryCount; i++)
    {
        if ((aEnv->mAccessHistory[i].mSpaceID == SC_NULL_SPACEID) &&
            (aEnv->mAccessHistory[i].mPageID == SC_NULL_PID))
        {
            break;
        }

        if ((aBCB->mSpaceID == aEnv->mAccessHistory[i].mSpaceID) &&
            (aBCB->mPageID == aEnv->mAccessHistory[i].mPageID))
        {
            if ((aEnv->mPinningList[aEnv->mPinningInsPos].mSpaceID != SC_NULL_SPACEID) &&
                (aEnv->mPinningList[aEnv->mPinningInsPos].mPageID != SC_NULL_PID))
            {
                *aBCBToUnpin = aEnv->mPinningBCBList[aEnv->mPinningInsPos];
            }

            aEnv->mPinningList[aEnv->mPinningInsPos].mSpaceID = 
                                                          aBCB->mSpaceID;
            aEnv->mPinningList[aEnv->mPinningInsPos].mPageID = 
                                                          aBCB->mPageID;
            aEnv->mPinningBCBList[aEnv->mPinningInsPos] = aBCB;
            aEnv->mPinningInsPos++;

            if (aEnv->mPinningInsPos == aEnv->mPinningCount)
            {
                aEnv->mPinningInsPos = 0;
            }

            *aBCBPinned = ID_TRUE;

            break;
        }
    }

    if( *aBCBPinned == ID_FALSE )
    {
        aEnv->mAccessHistory[aEnv->mAccessInsPos].mSpaceID = 
                                                          aBCB->mSpaceID;
        aEnv->mAccessHistory[aEnv->mAccessInsPos].mPageID = 
                                                          aBCB->mPageID;
        aEnv->mAccessInsPos++;

        if( aEnv->mAccessInsPos == aEnv->mAccessHistoryCount )
        {
            aEnv->mAccessInsPos = 0;
        }
    }
}

IDE_RC sdbBufferMgr::createPage( idvSQL          *aStatistics,
                                 scSpaceID        aSpaceID,
                                 scPageID         aPageID,
                                 UInt             aPageType,
                                 void*            aMtx,
                                 UChar**          aPagePtr)
{
    IDE_RC rc;

    IDV_SQL_OPTIME_BEGIN(aStatistics, IDV_OPTM_INDEX_DRDB_CREATE_PAGE);

    rc = mBufferPool.createPage( aStatistics,
                                 aSpaceID,
                                 aPageID,
                                 aPageType,
                                 aMtx,
                                 aPagePtr);

    IDV_SQL_OPTIME_END(aStatistics, IDV_OPTM_INDEX_DRDB_CREATE_PAGE);

    return rc;
}

IDE_RC sdbBufferMgr::getPageByPID( idvSQL               * aStatistics,
                                   scSpaceID              aSpaceID,
                                   scPageID               aPageID,
                                   sdbLatchMode           aLatchMode,
                                   sdbWaitMode            aWaitMode,
                                   sdbPageReadMode        aReadMode,
                                   void                 * aMtx,
                                   UChar               ** aRetPage,
                                   idBool               * aTrySuccess,
                                   idBool               * aIsCorruptPage )
{
    IDE_RC rc;

    IDV_SQL_OPTIME_BEGIN(aStatistics, IDV_OPTM_INDEX_DRDB_GET_PAGE);

    rc = mBufferPool.getPage( aStatistics,
                              aSpaceID,
                              aPageID,
                              aLatchMode,
                              aWaitMode,
                              aReadMode,
                              aMtx,
                              aRetPage,
                              aTrySuccess,
                              aIsCorruptPage );

    IDV_SQL_OPTIME_END(aStatistics, IDV_OPTM_INDEX_DRDB_GET_PAGE);

    return rc;
}

IDE_RC sdbBufferMgr::getPage( idvSQL             *aStatistics,
                              scSpaceID           aSpaceID,
                              scPageID            aPageID,
                              sdbLatchMode        aLatchMode,
                              sdbWaitMode         aWaitMode,
                              sdbPageReadMode     aReadMode,
                              UChar             **aRetPage,
                              idBool             *aTrySuccess )
{
    IDE_RC rc;

    rc = getPageByPID( aStatistics,
                       aSpaceID,
                       aPageID,
                       aLatchMode,
                       aWaitMode,
                       aReadMode,
                       NULL, /* aMtx */
                       aRetPage,
                       aTrySuccess,
                       NULL /*IsCorruptPage*/);

    return rc;
}

IDE_RC sdbBufferMgr::getPageByRID( idvSQL             *aStatistics,
                                   scSpaceID           aSpaceID,
                                   sdRID               aPageRID,
                                   sdbLatchMode        aLatchMode,
                                   sdbWaitMode         aWaitMode,
                                   void               *aMtx,
                                   UChar             **aRetPage,
                                   idBool             *aTrySuccess )
{
    IDE_RC rc;

    rc = getPageByPID( aStatistics,
                       aSpaceID,
                       SD_MAKE_PID( aPageRID),
                       aLatchMode,
                       aWaitMode,
                       SDB_SINGLE_PAGE_READ,
                       aMtx,
                       aRetPage,
                       aTrySuccess,
                       NULL /*IsCorruptPage*/);

    // BUG-27328 CodeSonar::Uninitialized Variable
    if( rc == IDE_SUCCESS )
    {
        *aRetPage += SD_MAKE_OFFSET( aPageRID );
    }

    return rc;
}


/****************************************************************
 * Description :
 *  pinning 기능과 함께 쓰이는 getPage.
 *  sdbPinningEnv를 반드시 인자로 넘겨 주어야 한다.
 *  먼저 findPinnedPage를 통해서 해당 pid가 mPinningList에 존재하는지
 *  확인한다.
 *  있다면 그것을 리턴하고, 없다면, 버퍼 풀로부터 해당 BCB를 가져와서
 *  mAccessHistory등록하고,
 *  만약 기존에 mAccessHistory에 해당 pid에 대한 기록이 있다면
 *  mPinningList에도 등록한다. 이때, mPinningList가 가득찼다면,
 *  기존에 존재하는 BCB를 unpin시키고, 현재의 BCB를 삽입한다.
 *
 *  주의!!
 *  pinning 기능은 아직 테스팅이 되지 않았다.
 *
 * Implementation:
 ****************************************************************/
IDE_RC sdbBufferMgr::getPage( idvSQL               *aStatistics,
                              void                 *aPinningEnv,
                              scSpaceID             aSpaceID,
                              scPageID              aPageID,
                              sdbLatchMode          aLatchMode,
                              sdbWaitMode           aWaitMode,
                              void                 *aMtx,
                              UChar               **aRetPage,
                              idBool               *aTrySuccess )
{
    sdbPinningEnv *sPinningEnv;
    sdbBCB        *sBCB;
    sdbBCB        *sOldBCB;
    idBool         sPinned;

    if ( aPinningEnv == NULL )
    {
        return getPageByPID( aStatistics,
                             aSpaceID,
                             aPageID,
                             aLatchMode,
                             aWaitMode,
                             SDB_SINGLE_PAGE_READ,
                             aMtx,
                             aRetPage,
                             aTrySuccess,
                             NULL /*IsCorruptPage*/ );
    }

    sPinningEnv = (sdbPinningEnv*)aPinningEnv;
    sBCB = findPinnedPage(sPinningEnv, aSpaceID, aPageID);
    if ( sBCB == NULL )
    {
        IDE_TEST( getPageByPID( aStatistics,
                                aSpaceID,
                                aPageID,
                                aLatchMode,
                                aWaitMode,
                                SDB_SINGLE_PAGE_READ,
                                aMtx,
                                aRetPage,
                                aTrySuccess,
                                NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );

        // 얻은 페이지로부터 BCB 포인터를 얻는다.
        // 지저분하지만 어쩔수가 없다.
        // 이렇게라도 얻자.
        sBCB = (sdbBCB*)((sdbFrameHdr*)*aRetPage)->mBCBPtr;
        addToHistory(sPinningEnv, sBCB, &sPinned, &sOldBCB);
        if ( sPinned == ID_TRUE )
        {
            // pin 시키기 위해 fix count를 하나 올린다.
            sBCB->lockBCBMutex(aStatistics);
            sBCB->incFixCnt();
            sBCB->unlockBCBMutex();
            if ( sOldBCB != NULL )
            {
                sOldBCB->lockBCBMutex(aStatistics);
                sOldBCB->decFixCnt();
                sOldBCB->unlockBCBMutex();
            }
            else
            {
                /* nithing to do */
            }
        }
        else
        {
            /* nithing to do */
        }
    }
    else
    {
        mBufferPool.latchPage( aStatistics,
                               sBCB,
                               aLatchMode,
                               aWaitMode,
                               aTrySuccess );

        if(aMtx != NULL)
        {
            IDE_ASSERT( smLayerCallback::pushToMtx( aMtx, sBCB, aLatchMode )
                        == IDE_SUCCESS );
        }

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbBufferMgr::fixPageByPID( idvSQL     * aStatistics,
                                   scSpaceID    aSpaceID,
                                   scPageID     aPageID,
                                   UChar     ** aRetPage,
                                   idBool     * aTrySuccess )
{
    IDE_RC rc;

    IDV_SQL_OPTIME_BEGIN(aStatistics, IDV_OPTM_INDEX_DRDB_FIX_PAGE);

    rc = mBufferPool.fixPage( aStatistics,
                              aSpaceID,
                              aPageID,
                              aRetPage );

    if( aTrySuccess != NULL )
    {
        if( rc == IDE_FAILURE )
        {
            *aTrySuccess = ID_FALSE;
        }
        else
        {
            *aTrySuccess = ID_TRUE;
        }
    }

    IDV_SQL_OPTIME_END(aStatistics, IDV_OPTM_INDEX_DRDB_FIX_PAGE);

    return rc;
}
#if 0
IDE_RC sdbBufferMgr::fixPageByGRID( idvSQL   * aStatistics,
                                    scGRID     aGRID,
                                    UChar   ** aRetPage,
                                    idBool   * aTrySuccess )
{
    scSpaceID   sSpaceID;
    sdRID       sRID;

    IDE_DASSERT( SC_GRID_IS_NOT_NULL( aGRID ));
    IDE_DASSERT( SC_GRID_IS_WITH_SLOTNUM( aGRID ) );

    if ( SC_GRID_IS_NOT_WITH_SLOTNUM(aGRID) )
    {
        sSpaceID = SC_MAKE_SPACE( aGRID );
        sRID     = SD_MAKE_RID_FROM_GRID( aGRID );

        IDE_TEST( fixPageByRID( aStatistics,
                                sSpaceID,
                                sRID,
                                aRetPage,
                                aTrySuccess )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( 1 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif

IDE_RC sdbBufferMgr::fixPageByRID( idvSQL     * aStatistics,
                                   scSpaceID    aSpaceID,
                                   sdRID        aRID,
                                   UChar     ** aRetPage,
                                   idBool     * aTrySuccess )
{
    IDE_TEST( fixPageByPID( aStatistics,
                            aSpaceID,
                            SD_MAKE_PID(aRID),
                            aRetPage,
                            aTrySuccess  )
              != IDE_SUCCESS );

    *aRetPage += SD_MAKE_OFFSET( aRID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdbBufferMgr::fixPageByPID( idvSQL             * aStatistics,
                                   scSpaceID            aSpaceID,
                                   scPageID             aPageID,
                                   UChar             ** aRetPage )
{
    IDE_RC rc;

    IDV_SQL_OPTIME_BEGIN( aStatistics, IDV_OPTM_INDEX_DRDB_FIX_PAGE );

    rc = mBufferPool.fixPage( aStatistics,
                              aSpaceID,
                              aPageID,
                              aRetPage );

    IDV_SQL_OPTIME_END( aStatistics, IDV_OPTM_INDEX_DRDB_FIX_PAGE );

    return rc;
}

#if 0 //not used!
//
///******************************************************************************
// * Description :
// *  Pinning을 구현한 함수.. (자세한것은 proj-1568 설계 메뉴얼 참조)
// *  Pin 기능을 사용하고자 하는 쪽에서 아래를 호출하여 aEnv를 세팅한다음,
// *  다음 접근때 부터는 aEnv를 통해서 접근한다.
// *  주의!!
// *      아직 테스트가 안되어 있는 코드이다.
// *
// *  aEnv    - [OUT] 생성된 sdbPinningEnv
// ******************************************************************************/
//IDE_RC sdbBufferMgr::createPinningEnv(void **aEnv)
//{
//    UInt           sPinningCount = smuProperty::getBufferPinningCount();
//    UInt           i;
//    sdbPinningEnv *sPinningEnv;
//
//    if (sPinningCount > 0)
//    {
//        // 본함수는 호출되지 않으므로 limit point 설정안함.
//        IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SDB,
//                                   ID_SIZEOF(sdbPinningEnv),
//                                   aEnv)
//                 != IDE_SUCCESS);
//
//        sPinningEnv = (sdbPinningEnv*)*aEnv;
//
//        sPinningEnv->mAccessInsPos = 0;
//        sPinningEnv->mAccessHistoryCount = smuProperty::getBufferPinningHistoryCount();
//        sPinningEnv->mPinningInsPos = 0;
//        sPinningEnv->mPinningCount = sPinningCount;
//
//        // 본함수는 호출되지 않으므로 limit point 설정안함.
//        IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SDB,
//                                   ID_SIZEOF(sdbPageID) * sPinningEnv->mAccessHistoryCount,
//                                   (void**)&sPinningEnv->mAccessHistory)
//                 != IDE_SUCCESS);
//
//        // 본함수는 호출되지 않으므로 limit point 설정안함.
//        IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SDB,
//                                   ID_SIZEOF(sdbPageID) * sPinningEnv->mPinningCount,
//                                   (void**)&sPinningEnv->mPinningList)
//                 != IDE_SUCCESS);
//
//        // 본함수는 호출되지 않으므로 limit point 설정안함.
//        IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SDB,
//                                   ID_SIZEOF(sdbBCB*) * sPinningEnv->mPinningCount,
//                                   (void**)&sPinningEnv->mPinningBCBList)
//                 != IDE_SUCCESS);
//
//        for (i = 0; i < sPinningEnv->mAccessHistoryCount; i++)
//        {
//            sPinningEnv->mAccessHistory[i].mSpaceID = SC_NULL_SPACEID;
//            sPinningEnv->mAccessHistory[i].mPageID = SC_NULL_PID;
//        }
//        for (i = 0; i < sPinningEnv->mPinningCount; i++)
//        {
//            sPinningEnv->mPinningList[i].mSpaceID = SC_NULL_SPACEID;
//            sPinningEnv->mPinningList[i].mPageID = SC_NULL_PID;
//            sPinningEnv->mPinningBCBList[i] = NULL;
//        }
//    }
//    else
//    {
//        *aEnv = NULL;
//    }
//
//    return IDE_SUCCESS;
//
//    IDE_EXCEPTION_END;
//
//    return IDE_FAILURE;
//}
//
///******************************************************************************
// * Description :
// *  Pinning을 구현한 함수.. (자세한것은 proj-1568 설계 메뉴얼 참조)
// *  createPinningEnv에서 세팅한 aEnv를 destroy하는 코드.
// *  createPinningEnv한것은 "반드시" destroyPinningEnv호출해서 destroy해야 한다.
// *  그렇지 않으면 심각한 상황 초래(해당 BCB를 다른Tx가 replace 할 수 없게 된다.)
// *  주의!!
// *      아직 테스트가 안되어 있는 코드이다.
// *  aStatistics - [IN]  통계정보
// *  aEnv        - [IN]  createPinningEnv 통해 생성한 정보
// ******************************************************************************/
//IDE_RC sdbBufferMgr::destroyPinningEnv(idvSQL *aStatistics, void *aEnv)
//{
//    sdbBCB        *sBCB;
//    UInt           i;
//    sdbPinningEnv *sPinningEnv;
//
//    if (aEnv != NULL)
//    {
//        sPinningEnv = (sdbPinningEnv*)aEnv;
//        for (i = 0; i < sPinningEnv->mPinningCount; i++)
//        {
//            if ((sPinningEnv->mPinningList[i].mSpaceID == SC_NULL_SPACEID) &&
//                (sPinningEnv->mPinningList[i].mPageID == SC_NULL_PID))
//            {
//                break;
//            }
//            sBCB = sPinningEnv->mPinningBCBList[i];
//            IDE_DASSERT(sBCB != NULL);
//
//            sBCB->lockBCBMutex(aStatistics);
//            sBCB->decFixCnt();
//            sBCB->unlockBCBMutex();
//        }
//
//        IDE_TEST(iduMemMgr::free(sPinningEnv->mAccessHistory) != IDE_SUCCESS);
//        IDE_TEST(iduMemMgr::free(sPinningEnv->mPinningList) != IDE_SUCCESS);
//        IDE_TEST(iduMemMgr::free(sPinningEnv->mPinningBCBList) != IDE_SUCCESS);
//        IDE_TEST(iduMemMgr::free(sPinningEnv) != IDE_SUCCESS);
//    }
//
//    return IDE_SUCCESS;
//
//    IDE_EXCEPTION_END;
//
//    return IDE_FAILURE;
//}
//
///****************************************************************
// * Description :
// *  full scan인 경우에는 LRU가 엉망이 될 가능성이 있다. 그러므로,
// *  이경우에는 LRU보다는 MRU로 호출을 하게 된다.
// *  이 함수를 통해서 얻은 BCB는 다음 트랜잭션이 replace를 요청할때,
// *  victim으로 선정될 확률이 높다.
// *
// *  이 기능은 버퍼매니저의 Multiple Buffer Read(MBR)기능을 통해서 구현
// *  될 예정. 아직 MBR과 연결되어 있지 않다.
// *
// *  aStatistics - [IN]  통계정보
// *  aSpaceID    - [IN]  table space ID
// *  aPageID     - [IN]  page ID
// *  aLatchMode  - [IN]  래치 모드, shared, exclusive, nolatch가 있다.
// *  aWaitMode   - [IN]  래치를 잡기 위해 대기 할지 말지 설정
// *  aMtx        - [IN]  Mini transaction
// *  aRetPage    - [OUT] 요청한 페이지
// *  aTrySuccess - [OUT] Latch mode가 try인경우에 성공여부리턴
// ****************************************************************/
//IDE_RC sdbBufferMgr::getPageWithMRU( idvSQL             *aStatistics,
//                                     scSpaceID           aSpaceID,
//                                     scPageID            aPageID,
//                                     sdbLatchMode        aLatchMode,
//                                     sdbWaitMode         aWaitMode,
//                                     void               *aMtx,
//                                     UChar             **aRetPage,
//                                     idBool             *aTrySuccess)
//{
//    return getPageByPID( aStatistics,
//                         aSpaceID,
//                         aPageID,
//                         aLatchMode,
//                         aWaitMode,
//                         SDB_SINGLE_PAGE_READ,
//                         aMtx,
//                         aRetPage,
//                         aTrySuccess,
//                         NULL /*IsCorruptPage*/);
//}
#endif

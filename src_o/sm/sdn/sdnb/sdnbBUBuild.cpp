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
 * $Id: sdnbBUBuild.cpp
 *
 * Description :
 *
 * Implementation Of Parallel Building Disk Index
 *
 * # Function
 *
 *
 **********************************************************************/
#include <smErrorCode.h>
#include <smu.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <smn.h>
#include <sdb.h>
#include <sdn.h>
#include <sdnReq.h>
#include <sdbMPRMgr.h>

/*
 * PROJ-1671 Bitmap-based Tablespace And Segment Space Management
 *
 * sdnIndexBuildThread는 공간관리 방식이 FELT와 FMS 기반으로 구현되어 있다.
 */
static UInt gMtxDLogType = SM_DLOG_ATTR_DEFAULT;

sdnbBUBuild::sdnbBUBuild() : idtBaseThread()
{


}

sdnbBUBuild::~sdnbBUBuild()
{


}

IDE_RC sdnbBUBuild::getKeyInfoFromLKey( scPageID       aPageID,
                                        SShort         aSlotSeq,
                                        sdpPhyPageHdr *aPage,
                                        sdnbKeyInfo   *aKeyInfo )
{
    sdnbLKey     *sLKey;
    UChar        *sSlotDirPtr;
    SShort        sSlotSeq = aSlotSeq;

    IDE_ASSERT( aPageID != SC_NULL_PID );
    IDE_DASSERT( aKeyInfo != NULL );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPage);

    if( aSlotSeq == -1 )
    {
        sSlotSeq = sdpSlotDirectory::getCount(sSlotDirPtr) - 1;
    }

    IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                        sSlotSeq,
                                                        (UChar**)&sLKey )
               == IDE_SUCCESS );

    SDNB_LKEY_TO_KEYINFO( sLKey, *aKeyInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * KeyMap에서 swap
 * ----------------------------------------------*/
void sdnbBUBuild::swapKeyMap( UInt aPos1,
                              UInt aPos2 )
{
    sdnbLKey *sKeyInfo;

    sKeyInfo       = mKeyMap[aPos1];
    mKeyMap[aPos1] = mKeyMap[aPos2];
    mKeyMap[aPos2] = sKeyInfo;

    return;
}

/* ------------------------------------------------
 * 쓰레드 작업 시작 루틴
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::threadRun( UInt                 aPhase,
                               UInt                 aThreadCnt,
                               sdnbBUBuild *aThreads )
{
    UInt             i;

    // Working Thread 실행
    for( i = 0; i < aThreadCnt; i++ )
    {
        aThreads[i].mPhase = aPhase;

        IDE_TEST( aThreads[i].start( ) != IDE_SUCCESS );
    }

    // Working Thread 종료 대기
    for( i = 0; i < aThreadCnt; i++ )
    {
        IDE_TEST( aThreads[i].join() != IDE_SUCCESS );
    }

    // Working Thread 수행 결과 확인
    for( i = 0; i < aThreadCnt; i++ )
    {
        if( aThreads[i].mIsSuccess == ID_FALSE )
        {
            IDE_TEST_RAISE( aThreads[i].getErrorCode() != 0, THREADS_ERR_RAISE )
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( THREADS_ERR_RAISE );
    {
        ideCopyErrorInfo( ideGetErrorMgr(),
                          aThreads[i].getErrorMgr() );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Disk Index Build Main Routine
 *
 * 0. Working Thread Initialize
 * 1. Parallel Key Extraction & In-Memory Sort
 * 2. Parallel Merge
 * 3. Make Index Tree
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::main( idvSQL          *aStatistics,
                          smcTableHeader  *aTable,
                          smnIndexHeader  *aIndex,
                          UInt             aThreadCnt,
                          ULong            aTotalSortAreaSize,
                          UInt             aTotalMergePageCnt,
                          idBool           aIsNeedValidation,
                          UInt             aBuildFlag,
                          UInt             aStatFlag )
{
    UInt                  i;
    UInt                  sState = 0;
    sdnbBUBuild         * sThreads = NULL;
    smuQueueMgr           sPIDBlkQueue;
    ULong                 sSortAreaSize;
    UInt                  sMergePageCnt;
    UInt                  sSortAreaPageCnt;
    UInt                  sInsertableMaxKeyCnt;
    UInt                  sThrState = 0;
    UInt                  sAvailablePageSize;
    UInt                  sMinKeyValueSize;
    sdnbStatistic         sIndexStat;       // BUG-18201

    IDE_DASSERT( aTable != NULL );
    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aTotalSortAreaSize > 0 );
    IDE_DASSERT( aTotalMergePageCnt > 0 );
    IDE_DASSERT( aThreadCnt > 0 );

    sMergePageCnt = aTotalMergePageCnt / aThreadCnt;

    sSortAreaSize        = aTotalSortAreaSize / aThreadCnt;
    sSortAreaPageCnt     = sSortAreaSize / SD_PAGE_SIZE;

    sAvailablePageSize  = sdpPhyPage::getEmptyPageFreeSize();
    sAvailablePageSize -= idlOS::align8((UInt)ID_SIZEOF(sdnbNodeHdr)) + ID_SIZEOF( sdpSlotDirHdr );

    /* BUG-23525
     * KeyBuf의 크기는 페이지와 완전히 같은 용량으로 잡는다.
     * KeyMap의 개수는 최소 크기의 키가 삽입될때, 즉 최대 개수로 잡는다. */
    sMinKeyValueSize = sdnbBTree::getMinimumKeyValueSize(aIndex);
    sInsertableMaxKeyCnt = sSortAreaPageCnt *
        sdnbBTree::getInsertableMaxKeyCnt( sMinKeyValueSize );

    /* TC/FIT/Limit/sm/sdn/sdnb/sdnbBUBuild_main_calloc.sql */
    IDU_FIT_POINT_RAISE( "sdnbBUBuild::main::calloc", 
                          insufficient_memory );  
    
    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SDN,
                                       1,
                                       (ULong)ID_SIZEOF(sdnbBUBuild) * aThreadCnt,
                                       (void**)&sThreads ) != IDE_SUCCESS,
                    insufficient_memory );  
    sState = 1;

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 0. BUILD INDEX               \n\
              NAME : %s                 \n\
              ID   : %u                 \n\
========================================\n\
    1. Extract & In-Memory Sort         \n\
    2. Merge & Build Tree               \n\
       2-1. Merge (loop : 0 ~ N )       \n\
    3. Build Tree ( 1 )                 \n\
       3-1. Destroy Free Pages          \n\
========================================\n\
          BUILD_THREAD_COUNT     : %u   \n\
          TOTAL_SORT_AREA_SIZE   : %llu \n\
          SORT_AREA_PAGE_COUNT   : %u   \n\
          MERGE_PAGE_COUNT       : %u   \n\
          INSERTABLE_KEY_COUNT   : %u   \n\
          MIN_KEY_VALUE_SIZE     : %u   \n\
========================================\n",
                 aIndex->mName,
                 aIndex->mId,
                 aThreadCnt,
                 aTotalSortAreaSize,
                 sSortAreaPageCnt,
                 sMergePageCnt,
                 sInsertableMaxKeyCnt,
                 sMinKeyValueSize );
    
    for ( i = 0; i < aThreadCnt; i++)
    {
        new (sThreads + i) sdnbBUBuild;
                
        IDE_TEST( sThreads[i].initialize(
                      aThreadCnt,
                      i,
                      aTable,
                      aIndex,
                      sMergePageCnt,
                      sAvailablePageSize,
                      sSortAreaPageCnt,
                      sInsertableMaxKeyCnt,
                      aIsNeedValidation,
                      aBuildFlag,
                      aStatistics )
                  != IDE_SUCCESS );
        sThrState++;
    }
    
// ------------------------------------------------
// Phase 1. Extract & In-Memory Sort
// ------------------------------------------------
    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 1. Extract & In-Memory Sort  \n\
========================================\n" );
    IDE_TEST( threadRun( SMN_EXTRACT_AND_SORT,
                         aThreadCnt,
                         sThreads )
              != IDE_SUCCESS );

// ------------------------------------------------
// Phase 2. Merge Run
// ------------------------------------------------
    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 2. Merge                     \n\
========================================\n" );
    IDE_TEST( threadRun( SMN_MERGE_RUN,
                         aThreadCnt,
                         sThreads )
              != IDE_SUCCESS );

// ------------------------------------------------
// Phase 3. Build Tree
// ------------------------------------------------
    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 3. Build Tree                \n\
========================================\n" );

    // BUG-18201
    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF( sdnbStatistic ) );

    IDE_TEST( sThreads[0].makeTree( sThreads,
                                    aThreadCnt,
                                    aTotalMergePageCnt,
                                    &sIndexStat,
                                    aStatFlag )
              != IDE_SUCCESS );

    // BUG-18201 : Memory/Disk Index 통계치
    SDNB_ADD_STATISTIC( &(((sdnbHeader*)aIndex->mHeader)->mDMLStat),
                        &sIndexStat );

    while( sThrState > 0 )
    {
        IDE_TEST( sThreads[--sThrState].destroy( ) != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free(sThreads) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    } 
    IDE_EXCEPTION_END;

    IDE_PUSH();

    for( i = 0; i < sThrState; i++)
    {
        (void)sThreads[i].destroy( );
    }
    if( sState == 1 )
    {
        (void)iduMemMgr::free(sThreads);
    }
    IDE_POP();

    return IDE_FAILURE;

}

/* ------------------------------------------------
 * Description :
 *
 * Index build 쓰레드 초기화
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::initialize( UInt             aTotalThreadCnt,
                                UInt             aID,
                                smcTableHeader * aTable,
                                smnIndexHeader * aIndex,
                                UInt             aMergePageCnt,
                                UInt             aAvailablePageSize,
                                UInt             aSortAreaPageCnt,
                                UInt             aInsertableKeyCnt,
                                idBool           aIsNeedValidation,
                                UInt             aBuildFlag,
                                idvSQL*          aStatistics )
{
    UInt sState = 0;

    mTotalThreadCnt      = aTotalThreadCnt;
    mID                  = aID;
    mTable               = aTable;
    mIndex               = aIndex;
    mMergePageCount      = aMergePageCnt;
    mKeyBufferSize       = aAvailablePageSize*aSortAreaPageCnt;
    mLeftSizeThreshold   = aAvailablePageSize;
    mInsertableMaxKeyCnt = aInsertableKeyCnt;
    mIsNeedValidation    = aIsNeedValidation;
    mStatistics          = aStatistics;
    mPhase               = 0;
    mFinished            = ID_FALSE;
    mBuildFlag           = aBuildFlag;
    mErrorCode           = 0;
    mIsSuccess           = ID_TRUE;
    mFreePageCnt         = 0;
    mLoggingMode         = ((aBuildFlag & SMI_INDEX_BUILD_LOGGING_MASK) ==
                            SMI_INDEX_BUILD_LOGGING )? SDR_MTX_LOGGING :
                            SDR_MTX_NOLOGGING;
    mIsForceMode         = ((aBuildFlag & SMI_INDEX_BUILD_FORCE_MASK) ==
                            SMI_INDEX_BUILD_FORCE )? ID_TRUE : ID_FALSE;

    /* TC/FIT/Limit/sm/sdn/sdnb/sdnbBUBuild_initialize_calloc1.sql */
    IDU_FIT_POINT_RAISE( "sdnbBUBuild::initialize::calloc1", 
                          insufficient_memory );  

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SDN,
                                 mInsertableMaxKeyCnt, 
                                 ID_SIZEOF(sdnbLKey*),
                                 (void**)&mKeyMap ) != IDE_SUCCESS,
                    insufficient_memory );  
    sState = 1;

    /* TC/FIT/Limit/sm/sdn/sdnb/sdnbBUBuild_initialize_calloc2.sql */
    IDU_FIT_POINT_RAISE( "sdnbBUBuild::initialize::calloc2", 
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SDN, mKeyBufferSize, 1,
                                 (void**)&mKeyBuffer ) != IDE_SUCCESS,
                    insufficient_memory );  
    sState = 2;

    IDE_TEST( mFreePage.initialize( IDU_MEM_SM_SDN,
                                    ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );
    
    IDE_TEST( mPIDBlkQueue.initialize( IDU_MEM_SM_SDN,
                                       ID_SIZEOF(sdnbSortedBlk) )
              != IDE_SUCCESS);

    mTrans = NULL;
    IDE_TEST( smLayerCallback::allocNestedTrans( &mTrans )
              != IDE_SUCCESS );
    sState = 3;


    // fix BUG-27403 -for Stack overflow
    IDE_TEST(mSortStack.initialize(IDU_MEM_SM_SDN, ID_SIZEOF(smnSortStack))
         != IDE_SUCCESS);
    sState = 4;

    IDE_TEST( smLayerCallback::beginTrans( mTrans, 
                                           ( SMI_TRANSACTION_REPL_NONE |
                                             SMI_COMMIT_WRITE_NOWAIT ), 
                                           NULL )
              != IDE_SUCCESS );

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 1. Extract & In-Memory Sort  \n\
----------------------------------------\n\
    -- KEYMAP      = %u                 \n\
    -- BUFFER      = %u                 \n\
========================================\n",
                 (ULong)mInsertableMaxKeyCnt*ID_SIZEOF(sdnbLKey*),
                 mKeyBufferSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    } 
    IDE_EXCEPTION_END;

    switch(sState)
    {
        case 4:
            (void)mSortStack.destroy();
        case 3:
            (void)smLayerCallback::freeTrans( mTrans );
            mTrans = NULL;
        case 2:
            (void)iduMemMgr::free( mKeyBuffer );
            mKeyBuffer = NULL;
        case 1:
            (void)iduMemMgr::free( mKeyMap );
            mKeyMap = NULL;
            break;
    }
            
    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * buffer flush 쓰레드 해제
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::destroy()
{
    UInt sStatus = 0;

    IDE_TEST( mPIDBlkQueue.destroy() != IDE_SUCCESS );
    IDE_TEST( mFreePage.destroy() != IDE_SUCCESS );
    IDE_TEST( mSortStack.destroy() != IDE_SUCCESS );

    if( mTrans != NULL )
    {
        sStatus = 2;
        IDU_FIT_POINT( "1.BUG-48210@sdnbBUBuild::destroy" );
        IDE_TEST( smLayerCallback::commitTrans( mTrans ) != IDE_SUCCESS );
        sStatus = 1;
        IDE_TEST( smLayerCallback::freeTrans( mTrans ) != IDE_SUCCESS );
        sStatus = 0;
        mTrans = NULL;
    }

    if( mKeyBuffer != NULL )
    {
        IDE_TEST( iduMemMgr::free( mKeyBuffer ) != IDE_SUCCESS );
        mKeyBuffer = NULL;
    }
    if( mKeyMap != NULL )
    {
        IDE_TEST( iduMemMgr::free( mKeyMap ) != IDE_SUCCESS );
        mKeyMap = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-48210 commit 실패에 대한 예외 처리 보강 */
    switch( sStatus )
    {
        case 2:
            (void)smLayerCallback::abortTrans( mTrans );
        case 1:
            (void)smLayerCallback::freeTrans(mTrans);
            mTrans = NULL;
            break;
        default:
            break;
    }

    if( mKeyBuffer != NULL )
    {
        (void)iduMemMgr::free( mKeyBuffer );
        mKeyBuffer = NULL;
    }
    if( mKeyMap != NULL )
    {
        IDE_ASSERT( iduMemMgr::free( mKeyMap ) == IDE_SUCCESS );
        mKeyMap = NULL;
    }

    return IDE_FAILURE;
}


/* ------------------------------------------------
 * Description :
 *
 * 쓰레드 메인 실행 루틴
 *
 * ----------------------------------------------*/
void sdnbBUBuild::run()
{
    sdnbStatistic     sIndexStat;

    // BUG-18201
    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF( sdnbStatistic ) );

    switch(mPhase)
    {
        case SMN_EXTRACT_AND_SORT:
            // Phase 1. Parallel Key Extraction & In-Memory Sort
            IDE_TEST( extractNSort( &sIndexStat ) != IDE_SUCCESS );
            break;

        case SMN_MERGE_RUN:
            // Phase 1. Parallel Key Extraction & In-Memory Sort
            IDE_TEST( merge( &sIndexStat ) != IDE_SUCCESS );
            break;

        default:
            IDE_ASSERT(0);
    }

    // BUG-18201 : Memory/Disk Index 통계치
    SDNB_ADD_STATISTIC( &(((sdnbHeader*)mIndex->mHeader)->mDMLStat),
                        &sIndexStat );

    return;

    IDE_EXCEPTION_END;

    mErrorCode = ideGetErrorCode();
    ideCopyErrorInfo( &mErrorMgr, ideGetErrorMgr() );

    return;
}

/* ------------------------------------------------
 * Phase 1. Key Extraction & In-Memory Sort
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::extractNSort( sdnbStatistic * aIndexStat )
{
    UInt                 sState = 0;
    scSpaceID            sTBSID;
    UChar              * sPage;
    UChar              * sSlotDirPtr;
    UChar              * sSlot;
    SInt                 sSlotCount;
    UInt                 i, j, k;
    SInt                 sLastPos = 0;
    sdSID                sRowSID;
    idBool               sIsSuccess;
    sdrMtx               sMtx;
    UInt                 sTotalRunCnt = 0;
    UInt                 sTempRunCnt;
    UInt                 sTotalMergeCnt = 0;
    UInt                 sLeftPos = 0;
    UInt                 sLeftCnt = 0;
    UInt                 sUsedBufferSize = 0;
    UInt                 sKeyValueSize = 0;
    scOffset             sOffset = 0;
    UInt                 sInsertedKeyCnt = 0;
    idBool               sIsNotNullIndex = ID_FALSE;
    sdnbColumn         * sColumn;
    scPageID             sCurPageID;
    sdpPhyPageHdr      * sPageHdr;
    sdnbHeader         * sIndexHeader;
    ULong                sKeyValueBuf[SDNB_MAX_KEY_BUFFER_SIZE/ID_SIZEOF(ULong)];
    idBool               sIsRowDeleted;
    sdbMPRMgr            sMPRMgr;
    UChar                sBuffer4Compaction[ SD_PAGE_SIZE ]; //짜투리를 저장해두는 공간
    UInt                 sKeySize4Compaction;
    UInt                 sCompactionCount = 0;
    idBool               sIsPageLatchReleased = ID_TRUE;
    scPageID             sRowPID;
    scSlotNum            sRowSlotNum;
    sdnbColLenInfoList * sColLenInfoList;
    UInt                 sColumnHeaderLen;
    UInt                 sColumnValueLen;
    sdbMPRFilter4PScan   sFilter4Scan;
    sIndexHeader     = (sdnbHeader*)mIndex->mHeader;
    sColLenInfoList  = &(sIndexHeader->mColLenInfoList);

    if( sIndexHeader->mIsNotNull == ID_TRUE )
    {
        sIsNotNullIndex = ID_TRUE;
    }

    sTBSID = mTable->mSpaceID;

    IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_NOLOGGING,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sMPRMgr.initialize(
                  mStatistics,
                  sTBSID,
                  sdpSegDescMgr::getSegHandle( &mTable->mFixed.mDRDB ),
                  sdbMPRMgr::filter4PScan )
              != IDE_SUCCESS );
    sFilter4Scan.mThreadCnt = mTotalThreadCnt;
    sFilter4Scan.mThreadID  = mID;
    sState = 2;

    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    // table에 alloc된 extent list를 순회하며 작업
    while (1)              // 1
    {
        IDE_TEST( sMPRMgr.getNxtPageID( (void*)&sFilter4Scan,
                                        &sCurPageID )
                  != IDE_SUCCESS );
        if( sCurPageID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPage( mStatistics,
                                         sTBSID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_MULTI_PAGE_READ,
                                         &sPage,
                                         &sIsSuccess )
                  != IDE_SUCCESS );
        IDE_ASSERT( sPage != NULL );

        sIsPageLatchReleased = ID_FALSE;
        
        sPageHdr = sdpPhyPage::getHdr( sPage );

        if( sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA )
        {
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPage);
            sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

            // page에 alloc된 slot 순회
            for( i = 0; i < (UInt)sSlotCount; i++ )       // 4
            {
                if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, i)
                    == ID_TRUE )
                {
                    continue;
                }
              
                IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                                    i,
                                                                    &sSlot )
                           == IDE_SUCCESS );
                IDE_DASSERT(sSlot != NULL);

                if( sdcRow::isHeadRowPiece(sSlot) != ID_TRUE )
                {
                    continue;
                }
                if( sdcRow::isDeleted(sSlot) == ID_TRUE ) 
                {
                    continue;
                }
                
                sRowSID = SD_MAKE_SID( sPageHdr->mPageID, i );

                IDE_TEST( sdnbBTree::makeKeyValueFromRow(
                                                      mStatistics,
                                                      NULL, /* aMtx */
                                                      NULL, /* aSP */
                                                      mTrans,
                                                      mTable,
                                                      mIndex,
                                                      sSlot,
                                                      SDB_MULTI_PAGE_READ,
                                                      sTBSID,
                                                      SMI_FETCH_VERSION_LAST,
                                                      SD_NULL_RID, /* aTssRID */
                                                      NULL,        /* aSCN */
                                                      NULL,        /* aInfiniteSCN */
                                                      (UChar*)sKeyValueBuf,
                                                      &sIsRowDeleted,
                                                      &sIsPageLatchReleased )
                          != IDE_SUCCESS );

                IDE_ASSERT( sIsRowDeleted == ID_FALSE );

                /* BUG-23319
                 * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
                /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
                 * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
                 * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
                 * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
                 * 상황에 따라 적절한 처리를 해야 한다. */
                if( sIsPageLatchReleased == ID_TRUE )
                {
                    /* BUG-25126
                     * [5.3.1 SD] Index Bottom-up Build 시 Page fetch시 서버사망!! */
                    IDE_TEST( sdbBufferMgr::getPage( mStatistics,
                                                     sTBSID,
                                                     sCurPageID,
                                                     SDB_S_LATCH,
                                                     SDB_WAIT_NORMAL,
                                                     SDB_MULTI_PAGE_READ,
                                                     &sPage,
                                                     &sIsSuccess )
                              != IDE_SUCCESS );
                    sIsPageLatchReleased = ID_FALSE;

                    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPage);
                    sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

                    /* page latch가 풀린 사이에 다른 트랜잭션이
                     * 동일 페이지에 접근하여 변경 연산을 수행할 수 있다. */
                    if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, i)
                        == ID_TRUE )
                    {
                        continue;
                    }
                }

                if( sIsNotNullIndex == ID_TRUE )
                {
                    // 주의
                    // NotNull Index이면, Column 중 하나도 Null이면 안된다.
                    // 따라서 모두 Null인지 아닌지만 판단하는 isNull로는
                    // 이 케이스를 확인하여서는 안된다.
                    for( sColumn = &sIndexHeader->mColumns[0],
                             sOffset = 0,
                             k = 0;
                         sColumn != sIndexHeader->mColumnFence; 
                         sColumn++, k++ )
                    {
                        if( sdnbBTree::isNullColumn( 
                                sColumn, 
                                (UChar*)(sKeyValueBuf)+sOffset )
                                == ID_TRUE )
                        {
                            IDE_RAISE( ERR_NOTNULL_VIOLATION );
                        }
                        sOffset += sdnbBTree::getColumnLength( 
                                            sColLenInfoList->mColLenInfo[ k ],
                                            ((UChar*)sKeyValueBuf) + sOffset,
                                            &sColumnHeaderLen,
                                            &sColumnValueLen,
                                            NULL );
                    }
                }

                // fix BUG-23525 mKeyBuf에 더이상 넣을 수 없거나, InsertableKeyCnt에도 도달하면
                sKeyValueSize = sdnbBTree::getKeyValueLength ( sColLenInfoList, (UChar*)sKeyValueBuf );

                if( (sUsedBufferSize + sKeyValueSize + ID_SIZEOF(sdnbLKey) > mKeyBufferSize) ||
                    (sInsertedKeyCnt == mInsertableMaxKeyCnt) )
                {
                    IDE_DASSERT( 0<sLastPos );

                    // sort area의 key 정렬
                    IDE_TEST( quickSort( aIndexStat, 0, sLastPos - 1 )
                              != IDE_SUCCESS );

                    // temp segment 에 내림
                    IDE_TEST( storeSortedRun(0, sLastPos -1, &sLeftPos, &sUsedBufferSize)
                              != IDE_SUCCESS );

                    sLeftCnt = sLastPos - sLeftPos;
                    sUsedBufferSize = 0;


                    if( sLeftCnt > 0 )
                    {
                        //KeyValue들을 맨 앞쪽으로 붙여넣기
                        for( j = 0; j < sLeftCnt; j++ )
                        {
                            swapKeyMap( j, (sLeftPos + j) );

                            /* fix BUG-23525 저장되는 키의 길이가 Varaibale하면
                             * Fragmentation이 일어날 수 밖에 없고, 따라서 Compaction과정은 필수*/
                            sKeySize4Compaction = sdnbBTree::getKeyLength( sColLenInfoList,
                                                                           (UChar*)mKeyMap[j],
                                                                           ID_TRUE );
                            idlOS::memcpy( &sBuffer4Compaction[ sUsedBufferSize ], mKeyMap[j],
                                   sKeySize4Compaction );

                            mKeyMap[j] = (sdnbLKey*)&mKeyBuffer[ sUsedBufferSize ] ;
                            sUsedBufferSize  += sKeySize4Compaction;
                            sCompactionCount ++;
                        }
                        idlOS::memcpy( mKeyBuffer, sBuffer4Compaction, sUsedBufferSize );

                        sInsertedKeyCnt = sLeftCnt;
                    }
                    else
                    {
                        sInsertedKeyCnt = 0;
                    }

                    sLastPos = sInsertedKeyCnt;
                    sTotalRunCnt++;
                }

                IDE_DASSERT( sInsertedKeyCnt < mInsertableMaxKeyCnt );
                IDE_DASSERT( sUsedBufferSize + sKeyValueSize + ID_SIZEOF(sdnbLKey) <= mKeyBufferSize );

                // key extraction
                mKeyMap[sLastPos] = (sdnbLKey*)&mKeyBuffer[ sUsedBufferSize ] ;
                sRowPID           = SD_MAKE_PID(sRowSID);
                sRowSlotNum       = SD_MAKE_SLOTNUM(sRowSID);
                SDNB_WRITE_LKEY( sRowPID, sRowSlotNum, 
                                 sKeyValueBuf, sKeyValueSize, mKeyMap[sLastPos] );

                sUsedBufferSize  += sKeyValueSize + ID_SIZEOF(sdnbLKey);
                sInsertedKeyCnt++;
                sLastPos++;
            }// for each slot                              // -4
        }

        // release s-latch
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( mStatistics,
                                             sPage )
                  != IDE_SUCCESS );
        sPage  = NULL;

        IDE_TEST( iduCheckSessionEvent(mStatistics)
                  != IDE_SUCCESS );
    }                                                     // -2

    sState = 1;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );
    
    // 작업 공간에 남아 있는 것들 처리
    if( sLastPos > 0 )
    {
        // in-memory sort
        IDE_TEST( quickSort( aIndexStat, 0, sLastPos - 1 )
                  != IDE_SUCCESS );
        // temp segment로 내린다.
        IDE_TEST( storeSortedRun(0, sLastPos - 1, NULL, NULL) != IDE_SUCCESS );
        sTotalRunCnt++;
    }

    sTempRunCnt = sTotalRunCnt;
    while (1)
    {
        sTempRunCnt = (sTempRunCnt - 1) / mMergePageCount + 1;
        sTotalMergeCnt += sTempRunCnt;

        if( sTempRunCnt < mMergePageCount )
        {
            break;
        }
    }

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 1. Extract & In-Memory Sort  \n\
              Completed                 \n\
----------------------------------------\n\
    -- TOTAL RUN COUNT    = %u          \n\
    -- EXPECTED MERGE CNT = %u          \n\
    -- COMPACTION COUNT   = %u          \n\
========================================\n",
                 sTotalRunCnt, sTotalMergeCnt, sCompactionCount );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );


    return IDE_SUCCESS;

    /* PRIMARY KEY는 NULL값을 색인할수 없다 */
    IDE_EXCEPTION( ERR_NOTNULL_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_NULL_VIOLATION ) );
    }
    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;

    IDE_PUSH();
    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sPage != NULL );
        IDE_ASSERT( sdbBufferMgr::releasePage( mStatistics,
                                               sPage )
                    == IDE_SUCCESS );
    }

    switch( sState )
    {
        case 2 :
            IDE_ASSERT( sMPRMgr.destroy() == IDE_SUCCESS );
        case 1 :
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}
                                         
/* ------------------------------------------------
 * quick sort with KeyMap
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::quickSort( sdnbStatistic *aIndexStat,
                               UInt           aHead,
                               UInt           aTail )
{
    idBool               sIsSameValue;
    SInt                 sRet;
    SInt                 i;
    SInt                 j;
    UInt                 m;
    sdnbKeyInfo          sKeyInfo;
    sdnbConvertedKeyInfo sPivotKeyInfo;
    smiValue             sSmiValueList[SMI_MAX_IDX_COLUMNS];
    const sdnbColumn    *sColumn;
    const sdnbColumn    *sColumnFence;
    sdnbColLenInfoList  *sColLenInfoList;
    smnSortStack         sCurStack; // fix BUG-27403 -for Stack overflow
    smnSortStack         sNewStack; 
    idBool               sEmpty;

    sPivotKeyInfo.mSmiValueList = (smiValue*)sSmiValueList;
    sColumn                     = ((sdnbHeader*)mIndex->mHeader)->mColumns;
    sColumnFence                = ((sdnbHeader*)mIndex->mHeader)->mColumnFence;
    sColLenInfoList             = &(((sdnbHeader*)mIndex->mHeader)->mColLenInfoList);

    //fix BUG-27403 최초 해야할 일 입력
    sCurStack.mLeftPos   = aHead;
    sCurStack.mRightPos  = aTail;
    IDE_TEST( mSortStack.push(ID_FALSE, &sCurStack) != IDE_SUCCESS );

    sEmpty = ID_FALSE;

    while( 1 )
    {
        IDE_TEST(mSortStack.pop(ID_FALSE, &sCurStack, &sEmpty)
                 != IDE_SUCCESS);

        // Bug-27403
        // QuickSort의 알고리즘상, CallStack은 ItemCount보다 많아질 수 없다.
        // 따라서 이보다 많으면, 무한루프에 빠졌을 가능성이 높다.
        IDE_ASSERT( (aTail - aHead +1 ) > mSortStack.getTotItemCnt() );

        if( sEmpty == ID_TRUE)
        {
            break;
        }

        i = sCurStack.mLeftPos;
        j = sCurStack.mRightPos + 1;
        m = (sCurStack.mLeftPos + sCurStack.mRightPos)/2;

        if( sCurStack.mLeftPos < sCurStack.mRightPos )
        {
            swapKeyMap( m, sCurStack.mLeftPos );
            SDNB_LKEY_TO_CONVERTED_KEYINFO( mKeyMap[ sCurStack.mLeftPos ], 
                sPivotKeyInfo, 
                sColLenInfoList ); 
    
            while( 1 )
            {
                while( (++i) <= sCurStack.mRightPos )
                {
                    SDNB_LKEY_TO_KEYINFO( mKeyMap[ i ], sKeyInfo );
                    sRet = sdnbBTree::compareConvertedKeyAndKey( aIndexStat,
                                                                 sColumn,
                                                                 sColumnFence,
                                                                 &sPivotKeyInfo,
                                                                 &sKeyInfo,
                                                                 SDNB_COMPARE_VALUE   |
                                                                 SDNB_COMPARE_PID     |
                                                                 SDNB_COMPARE_OFFSET,
                                                                 &sIsSameValue );
                                                                
                    if( sIsSameValue == ID_TRUE )
                    {
                        // To fix BUG-17732
                        IDE_TEST_RAISE( isNull( (sdnbHeader*)(mIndex->mHeader), SDNB_LKEY_KEY_PTR( mKeyMap[i] ) )
                                        != ID_TRUE, ERR_UNIQUENESS_VIOLATION );
                    }
                    
                    if( sRet < 0 )
                    {
                        break;
                    }
                }
    
                while( (--j) > sCurStack.mLeftPos )
                {
                    SDNB_LKEY_TO_KEYINFO( mKeyMap[ j ], sKeyInfo );
                    sRet = sdnbBTree::compareConvertedKeyAndKey( aIndexStat,
                                                                 sColumn,
                                                                 sColumnFence,
                                                                 &sPivotKeyInfo,
                                                                 &sKeyInfo,
                                                                 SDNB_COMPARE_VALUE   |
                                                                 SDNB_COMPARE_PID     |
                                                                 SDNB_COMPARE_OFFSET,
                                                                 &sIsSameValue );
    
                    if( sIsSameValue == ID_TRUE )
                    {
                        IDE_TEST_RAISE( isNull( (sdnbHeader*)(mIndex->mHeader), SDNB_LKEY_KEY_PTR( mKeyMap[j] ) )
                                        != ID_TRUE, ERR_UNIQUENESS_VIOLATION );
                    }
    
                    if( sRet > 0 )
                    {
                        break;
                    }
                }

                if( i < j )
                {
                    swapKeyMap( i, j );
                }
                else
                {
                    break;
                }
            }
    
            swapKeyMap( (i-1), sCurStack.mLeftPos );
    
            if( i > (sCurStack.mLeftPos + 2) )
            {
                sNewStack.mLeftPos  = sCurStack.mLeftPos;
                sNewStack.mRightPos = i-2;

                IDE_TEST(mSortStack.push(ID_FALSE, &sNewStack)
                         != IDE_SUCCESS);
            }
            if( i < sCurStack.mRightPos )
            {
                sNewStack.mLeftPos  = i;
                sNewStack.mRightPos = sCurStack.mRightPos;

                IDE_TEST(mSortStack.push(ID_FALSE, &sNewStack)
                         != IDE_SUCCESS);
            }
        }
    }

    IDE_DASSERT( checkSort( aIndexStat, aHead, aTail ) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNIQUENESS_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smnUniqueViolation ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* ------------------------------------------------
 * check sorted block
 * ----------------------------------------------*/
idBool sdnbBUBuild::checkSort( sdnbStatistic *aIndexStat,
                               UInt           aHead,
                               UInt           aTail )
{
    UInt        i;
    idBool      sIsSameValue;
    idBool      sRet = ID_TRUE;
    sdnbKeyInfo sKey1Info;
    sdnbKeyInfo sKey2Info;

    for( i = aHead + 1; i <= aTail; i++ )
    {
        SDNB_LKEY_TO_KEYINFO( mKeyMap[i-1], sKey1Info );
        SDNB_LKEY_TO_KEYINFO( mKeyMap[i]  , sKey2Info );
        if ( sdnbBTree::compareKeyAndKey( aIndexStat,
                                          ((sdnbHeader*)mIndex->mHeader)->mColumns,
                                          ((sdnbHeader*)mIndex->mHeader)->mColumnFence,
                                          &sKey1Info,
                                          &sKey2Info,
                                          ( SDNB_COMPARE_VALUE   |
                                            SDNB_COMPARE_PID     |
                                            SDNB_COMPARE_OFFSET ),
                                          &sIsSameValue ) > 0 )
        {
            sRet = ID_FALSE;
            break;
        }
    }

    return sRet;
}


/* ------------------------------------------------
 * sorted block을 disk에 내림
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::storeSortedRun( UInt    aHead,
                                    UInt    aTail,
                                    UInt  * aLeftPos,
                                    UInt  * aLeftSize )
{
    UInt             i;
    sdrMtx           sMtx;
    SShort           sSlotSeq;
    sdnbSortedBlk    sQueueInfo;
    sdnbKeyInfo      sKeyInfo;
    UInt             sKeyValueSize;
    scPageID         sPrevPID;
    scPageID         sCurrPID;
    scPageID         sNullPID = SD_NULL_PID;
    sdpPhyPageHdr   *sCurrPage;
    sdnbNodeHdr     *sNodeHdr;
    sdnbHeader      *sIndexHeader;
    idBool           sIsSuccess;
    UInt             sState = 0;
    UInt             sAllocatedPageCnt = 0;

    IDE_DASSERT( aTail < mInsertableMaxKeyCnt );

    // sorted run을 저장할 page를 할당
    sAllocatedPageCnt++;
    
    IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                   &sMtx,
                                   mTrans,
                                   mLoggingMode,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    sState = 1;

    // NOLOGGING mini-transaction의 Persistent flag 설정
    // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
    sdrMiniTrans::setNologgingPersistent( &sMtx );

    IDE_TEST( allocPage( &sMtx,
                         &sCurrPID,
                         &sCurrPage,
                         &sNodeHdr,
                         NULL,       // aPrevPageHdr
                         &sNullPID,  // aPrevPID
                         &sNullPID ) // aNextPID
              != IDE_SUCCESS );

    // sorted run의 첫번째 page의 PID를 PIDBlkQueue에 enqueue
    sQueueInfo.mStartPID = sCurrPID;
    IDE_TEST( mPIDBlkQueue.enqueue(ID_FALSE, (void*)&sQueueInfo)
              != IDE_SUCCESS );

    sSlotSeq = -1;
    sIndexHeader = (sdnbHeader*)mIndex->mHeader;

    if ( aLeftPos != NULL )
    {
        /* BUG-23525 남은 키 크기 계산을 위해 aLeftSize도 있어야 함. */
        IDE_ASSERT( aLeftSize != NULL ); 
        *aLeftPos = aTail + 1;
    }
    else
    {
        /* nothing to do */
    }
    
    for ( i = aHead; i <= aTail; i++ )
    {
        sKeyValueSize = sdnbBTree::getKeyValueLength( &(sIndexHeader->mColLenInfoList),
                                                      SDNB_LKEY_KEY_PTR((UChar*)mKeyMap[i]) );

        if ( sdpPhyPage::canAllocSlot( sCurrPage,
                                       SDNB_LKEY_LEN( sKeyValueSize, SDNB_KEY_TB_CTS ),
                                       ID_TRUE,
                                       SDP_1BYTE_ALIGN_SIZE )
             != ID_TRUE )
        {
            // 남은 키가 일정 이상이라면 다시 삽입을, 일정 이하로 작다면 다음으로 미룬다.
            // 키의 크기가 작다면 다음에 다시 한번 sorting한다.
            if ( aLeftPos != NULL )
            {
                if ( mLeftSizeThreshold > *aLeftSize )
                {
                    *aLeftPos = i;
                    break;
                }
            }
            else
            {
                /* nothing to do */
            }
            
            // Page에 충분한 공간이 남아있지 않다면 새로운 Page Alloc
            sPrevPID = sCurrPage->mPageID;
            
            IDE_TEST( allocPage( &sMtx,
                                 &sCurrPID,
                                 &sCurrPage,
                                 &sNodeHdr,
                                 sCurrPage, // Prev Page Pointer
                                 &sPrevPID,
                                 &sNullPID )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            sAllocatedPageCnt++;

            
            IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                           &sMtx,
                                           mTrans,
                                           mLoggingMode,
                                           ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                           gMtxDLogType )
                      != IDE_SUCCESS );
            sState = 1;

            // NOLOGGING mini-transaction의 Persistent flag 설정
            // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
            sdrMiniTrans::setNologgingPersistent( &sMtx );
            
            // 이전 page를 PrevTail에 getpage
            IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  sCurrPID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  &sMtx,
                                                  (UChar**)&sCurrPage,
                                                  &sIsSuccess,
                                                  NULL )
                      != IDE_SUCCESS );

            /* BUG-27527 Disk Index Bottom Up Build시, 로그 없이 Key Insert할 경우
             * 페이지가 Dirty되지 않음.
             * 이후에 이 페이지에 로그를 남기지 않는 Write작업이 일어나, 페이지가
             * Dirty돼지 않는다.  따라서 강제로 Dirt시킨다.*/
            sdbBufferMgr::setDirtyPageToBCB( mStatistics, (UChar*)sCurrPage );
            
            sSlotSeq = -1;
        }
        sSlotSeq++;
        //fix BUG-23525 페이지 할당을 위한 판단 자료
        if ( aLeftSize != NULL )
        {
            IDE_DASSERT( SDNB_LKEY_LEN( sKeyValueSize, SDNB_KEY_TB_CTS ) <= (*aLeftSize) );
            (*aLeftSize) -= SDNB_LKEY_LEN( sKeyValueSize, SDNB_KEY_TB_CTS );
        }
        else
        {
            /* nothing to do */
        }

        SDNB_LKEY_TO_KEYINFO( mKeyMap[i], sKeyInfo );
        sKeyInfo.mKeyState = SDNB_KEY_STABLE;

        IDE_TEST( sdnbBTree::insertLKey( &sMtx,
                                         sIndexHeader,
                                         sCurrPage,
                                         sSlotSeq,
                                         SDN_CTS_INFINITE,
                                         SDNB_KEY_TB_CTS,
                                         &sKeyInfo,
                                         sKeyValueSize,
                                         ID_FALSE, /* no logging */ 
                                         NULL )    /* key offset */
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if ( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnbBUBuild::preparePages                    *
 * ------------------------------------------------------------------*
 * 주어진 개수만큼의 페이지를 할당받을수 있을지 검사한다.            *
 *********************************************************************/
IDE_RC sdnbBUBuild::preparePages( UInt aNeedPageCnt )
{
    sdnbHeader  * sIdxHeader = NULL;
    sdrMtx        sMtx;
    UInt          sState = 0;

    if ( mFreePageCnt < aNeedPageCnt )
    {
        sIdxHeader = (sdnbHeader *)(mIndex->mHeader);
        
        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*Undoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(
                      &(sIdxHeader->mSegmentDesc) )->mPrepareNewPages(
                          mStatistics,
                          &sMtx,
                          sIdxHeader->mIndexTSID,
                          &(sIdxHeader->mSegmentDesc.mSegHandle),
                          aNeedPageCnt - mFreePageCnt )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }
    
    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::allocPage(sdrMtx         * aMtx,
                              scPageID       * aPageID,
                              sdpPhyPageHdr ** aPageHdr,
                              sdnbNodeHdr   ** aNodeHdr,
                              sdpPhyPageHdr  * aPrevPageHdr,
                              scPageID       * aPrevPID,
                              scPageID       * aNextPID )
{
    sdrMtx            sMtx;
    sdnbNodeHdr     * sNodeHdr;
    sdpPhyPageHdr   * sCurrPage = NULL;
    UInt              sState = 0;
    scPageID          sPopPID;
    idBool            sIsEmpty;
    idBool            sIsSuccess;
    UChar             sIsConsistent = SDP_PAGE_INCONSISTENT;
    sdpSegmentDesc  * sSegmentDesc;

    IDE_DASSERT( aPrevPID != NULL);
    IDE_DASSERT( aNextPID != NULL);
    
    IDE_TEST( mFreePage.pop( ID_FALSE,
                             (void*)&sPopPID,
                             &sIsEmpty )
              != IDE_SUCCESS );

    if( sIsEmpty == ID_TRUE ) // 사용후 free 된 page가 없을 경우
    {
        /* Bottom-up index build 시 사용할 page들을 할당한다.
         * Logging mode로 new page를 alloc 받음 : index build를
         * undo할 경우 extent를 free 시키기 위해서   */
        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       SDR_MTX_LOGGING,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 1;

        sSegmentDesc = &(((sdnRuntimeHeader*)mIndex->mHeader)->mSegmentDesc);
        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(sSegmentDesc)->mAllocNewPage(
                                  mStatistics, /* idvSQL* */
                                  &sMtx,
                                  mIndex->mIndexSegDesc.mSpaceID,
                                  sdpSegDescMgr::getSegHandle(sSegmentDesc),
                                  SDP_PAGE_INDEX_BTREE,
                                  (UChar**)&sCurrPage )  
                  != IDE_SUCCESS );

        IDE_TEST( sdpPhyPage::logAndInitLogicalHdr( sCurrPage,
                                                    ID_SIZEOF(sdnbNodeHdr),
                                                    &sMtx,
                                                    (UChar**)&sNodeHdr )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSlotDirectory::logAndInit( sCurrPage,
                                                &sMtx )
                  != IDE_SUCCESS );

        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( &sMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }

        *aPageID = sCurrPage->mPageID;

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              *aPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL ) 
                  != IDE_SUCCESS );
    }
    else
    {
        mFreePageCnt--;
        *aPageID = sPopPID;

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              *aPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        // BUG-17615 로깅양 줄이기
        // 1단계(extractNSort), 2단계(merge)에서 반환된 페이지들은
        // LOGICAL HEADER는 사용된적이 없기 때문에 PHYSICAL HEADER만
        // RESET 한다.
        IDE_TEST( sdpPhyPage::reset((sdpPhyPageHdr*)sCurrPage,
                                    ID_SIZEOF(sdnbNodeHdr),
                                    aMtx) 
                  != IDE_SUCCESS );

        IDE_TEST( sdpSlotDirectory::logAndInit( sCurrPage,
                                                aMtx )
                  != IDE_SUCCESS );

        sNodeHdr = (sdnbNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sCurrPage );
        
        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( aMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }
    }

    if( aPrevPageHdr != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&aPrevPageHdr->mListNode.mNext,
                      (void*)aPageID,
                      ID_SIZEOF(scPageID) )
                  != IDE_SUCCESS );
    }
    
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sCurrPage->mListNode.mPrev,
                  (void*)aPrevPID,
                  ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sCurrPage->mListNode.mNext,
                  (void*)aNextPID,
                  ID_SIZEOF(scPageID) )
              != IDE_SUCCESS );

    *aPageHdr = sCurrPage;
    *aNodeHdr = sNodeHdr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::freePage( scPageID aPageID )
{

    IDE_TEST( mFreePage.push( ID_FALSE,
                              (void*)&aPageID )
              != IDE_SUCCESS );

    mFreePageCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::mergeFreePage( sdnbBUBuild  * aThreads,
                                   UInt                   aThreadCnt )
{
    scPageID  sPopPID;
    idBool    sIsEmpty;
    UInt      i;
    
    for( i = 1; i < aThreadCnt; i++ )
    {
        while(1)
        {
            IDE_TEST( aThreads[i].mFreePage.pop( ID_FALSE,
                                                 (void*)&sPopPID,
                                                 &sIsEmpty )
                      != IDE_SUCCESS );

            if( sIsEmpty == ID_TRUE )
            {
                break;
            }

            aThreads[i].mFreePageCnt--;

            IDE_TEST( aThreads[0].mFreePage.push( ID_FALSE,
                                                  (void*)&sPopPID )
                      != IDE_SUCCESS );

            aThreads[0].mFreePageCnt++;
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::removeFreePage()
{
    scPageID         sPopPID;
    idBool           sIsEmpty;
    UInt             sState = 0;
    sdrMtx           sMtx;
    UChar          * sSegDesc;
    sdpPhyPageHdr  * sPage;
    idBool           sIsSuccess;
    sdpSegmentDesc * sSegmentDesc;

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] 3.1. FREE PAGE(%u)           \n\
========================================\n", mFreePageCnt );
    
    while(1)
    {
        IDE_TEST( mFreePage.pop( ID_FALSE,
                                 (void*)&sPopPID,
                                 &sIsEmpty )
                  != IDE_SUCCESS );

        if( sIsEmpty == ID_TRUE )
        {
            break;
        }
        
        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       SDR_MTX_LOGGING,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 1;
        
        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              sPopPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              &sMtx,
                                              (UChar**)&sPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        IDE_TEST( sdbBufferMgr::getPageByGRID( mStatistics,
                                               mIndex->mIndexSegDesc,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               &sSegDesc,
                                               &sIsSuccess )
                  != IDE_SUCCESS );

        sSegmentDesc = &(((sdnRuntimeHeader*)(mIndex->mHeader))->mSegmentDesc) ;
        IDE_TEST( sdpSegDescMgr::getSegMgmtOp(sSegmentDesc)->mFreePage(
                                          mStatistics,
                                          &sMtx,
                                          mIndex->mIndexSegDesc.mSpaceID,
                                          sdpSegDescMgr::getSegHandle(sSegmentDesc),
                                          (UChar*)sPage )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            (void)sdrMiniTrans::rollback(&sMtx);
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::merge( sdnbStatistic * aIndexStat )
{
    sdrMtx              sMtx;
    idBool              sIsEmpty;
    idBool              sIsSuccess;
    idBool              sIsClosedRun;
    SShort              sOutSlotSeq = -1;
    UInt                i;
    UInt                sLastPos = 0;
    UInt                sMinIdx;
    UInt                sKeyValueSize;
    UInt                sQueueLength;
    UInt                sClosedRun = 0;
    SInt               *sHeapMap;
    UInt                sState = 0;
    UInt                sHeapMapCount = 1;

    // merge에 사용되는 run의 자료구조
    sdnbMergeRunInfo   *sMergeRunInfo;

    scPageID            sCurrOutPID;
    scPageID            sPrevOutPID;
    scPageID            sNullPID = SD_NULL_PID;

    sdnbNodeHdr        *sNodeHdr;
    sdnbSortedBlk       sQueueInfo;
    sdpPhyPageHdr      *sCurrOutRun;
    sdnbHeader         *sIndexHeader;

    sIndexHeader = (sdnbHeader*)mIndex->mHeader;

    /* sdnbBUBuild_merge_calloc_MergeRunInfo.tc */
    IDU_FIT_POINT("sdnbBUBuild::merge::calloc::MergeRunInfo");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SDN, 1, 
                                 (ULong)mMergePageCount * ID_SIZEOF(sdnbMergeRunInfo),
                                 (void**)&sMergeRunInfo )
              != IDE_SUCCESS );
    
    sState = 1;

    while( sHeapMapCount < (mMergePageCount*2) ) // get heap size
    {
        sHeapMapCount = sHeapMapCount * 2;
    }

    /* TC/FIT/Limit/sm/sdn/sdnb/sdnbBUBuild_merge_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdnbBUBuild::merge::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc(IDU_MEM_SM_SDN,
                                (ULong)sHeapMapCount * ID_SIZEOF(SInt),
                                (void**)&sHeapMap ) != IDE_SUCCESS,
                    insufficient_memory );

    sState = 2;

    for( i = 0; i < sHeapMapCount; i++ )
    {
        sHeapMap[i] = -1;
    }

    while( 1 )
    {
        sLastPos = 0;

        sQueueLength = mPIDBlkQueue.getQueueLength( );

        // merge해야 할 run의 수가 merge 가능한 page의 수보다 작으면
        // merge 종료하고 make tree 수행
        if( sQueueLength <= mMergePageCount )
        {
            break;
        }

        // 한번에 merge할 page 수만큼 각 run의 첫번째 page를 fix
        while( 1 )
        {
            IDE_TEST( mPIDBlkQueue.dequeue( ID_FALSE,
                                            (void*)&sQueueInfo,
                                            &sIsEmpty )
                      != IDE_SUCCESS );

            IDE_ASSERT( sIsEmpty != ID_TRUE );

            sMergeRunInfo[sLastPos].mHeadPID = sQueueInfo.mStartPID;

            IDE_TEST( sdbBufferMgr::fixPageByPID( mStatistics, /* idvSQL* */
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  sMergeRunInfo[sLastPos].mHeadPID,
                                                  (UChar**)&sMergeRunInfo[sLastPos].mPageHdr,
                                                  &sIsSuccess )
                      != IDE_SUCCESS );

            sLastPos++;
            
            sQueueLength = mPIDBlkQueue.getQueueLength( );

            // 배열 범위를 넘어가거나,
            // 다음 스텝에서 Merge Page Count를 넘어가는 경우
            // (Queue를 Merge Page Count만큼 채우기 위해)
            if( (sLastPos > mMergePageCount - 1) ||
                ((sQueueLength + 1) <= mMergePageCount) )
            {
                break;
            }
        }

        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       &sMtx,
                                       mTrans,
                                       mLoggingMode,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sState = 3;

        // NOLOGGING mini-transaction의 Persistent flag 설정
        // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
        sdrMiniTrans::setNologgingPersistent( &sMtx );

        IDE_TEST( allocPage( &sMtx,
                             &sCurrOutPID,
                             &sCurrOutRun,
                             &sNodeHdr,
                             NULL,
                             &sNullPID,
                             &sNullPID )
                  != IDE_SUCCESS );        

        sOutSlotSeq = -1;

        sQueueInfo.mStartPID = sCurrOutPID;

        IDE_TEST( mPIDBlkQueue.enqueue(ID_FALSE, (void*)&sQueueInfo)
                  != IDE_SUCCESS );

        // initialize the selection tree to merge
        IDE_TEST( heapInit( aIndexStat,
                            sLastPos,
                            sHeapMapCount,
                            sHeapMap,
                            sMergeRunInfo )
                  != IDE_SUCCESS );
        
        sClosedRun = 0;

        while ( 1 )
        {
            if ( sClosedRun == sLastPos )
            {
                sState = 2;
                IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
                break;
            }

            sMinIdx = (UInt)sHeapMap[1];   // the root of selection tree

            sKeyValueSize = sdnbBTree::getKeyValueLength( 
                                       &(sIndexHeader->mColLenInfoList), 
                                       sMergeRunInfo[sMinIdx].mRunKey.mKeyValue );
                
            if ( sdpPhyPage::canAllocSlot( sCurrOutRun,
                                           SDNB_LKEY_LEN( sKeyValueSize, SDNB_KEY_TB_CTS ),
                                           ID_TRUE,
                                           SDP_1BYTE_ALIGN_SIZE )
                 != ID_TRUE )
            {
                // Page에 충분한 공간이 남아있지 않다면
                // 새로운 Page Alloc
                sPrevOutPID = sCurrOutPID;

                IDE_TEST( allocPage( &sMtx,
                                     &sCurrOutPID,
                                     &sCurrOutRun,
                                     &sNodeHdr,
                                     sCurrOutRun,
                                     &sPrevOutPID,
                                     &sNullPID )
                          != IDE_SUCCESS );

                sState = 2;
                IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            
                IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                               &sMtx,
                                               mTrans,
                                               mLoggingMode,
                                               ID_TRUE,/*Undoable(PRJ2162)*/
                                               gMtxDLogType )
                          != IDE_SUCCESS );
                sState = 3;

                // NOLOGGING mini-transaction의 Persistent flag 설정
                // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
                sdrMiniTrans::setNologgingPersistent( &sMtx );

                IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                                      mIndex->mIndexSegDesc.mSpaceID,
                                                      sCurrOutPID,
                                                      SDB_X_LATCH,
                                                      SDB_WAIT_NORMAL,
                                                      SDB_SINGLE_PAGE_READ,
                                                      &sMtx,
                                                      (UChar**)&sCurrOutRun,
                                                      &sIsSuccess,
                                                      NULL )
                          != IDE_SUCCESS );                                

                /* BUG-27527 Disk Index Bottom Up Build시, 로그 없이 Key Insert할 경우
                 * 페이지가 Dirty되지 않음.
                 * 이후에 이 페이지에 로그를 남기지 않는 Write작업이 일어나, 페이지가
                 * Dirty돼지 않는다.  따라서 강제로 Dirt시킨다.*/
                sdbBufferMgr::setDirtyPageToBCB( mStatistics, (UChar*)sCurrOutRun );
                    
                sOutSlotSeq = -1;
            }
            sOutSlotSeq++;
            
            IDE_TEST( sdnbBTree::insertLKey( &sMtx,
                                             (sdnbHeader*)mIndex->mHeader,
                                             sCurrOutRun,
                                             sOutSlotSeq,
                                             SDN_CTS_INFINITE,
                                             SDNB_KEY_TB_CTS,
                                             &(sMergeRunInfo[sMinIdx].mRunKey),
                                             sKeyValueSize,
                                             ID_FALSE, /* no logging */ 
                                             NULL )    /* key offset */
                      != IDE_SUCCESS );
            
            IDE_TEST( heapPop( aIndexStat,
                               sMinIdx,
                               &sIsClosedRun,
                               sHeapMapCount,
                               sHeapMap,
                               sMergeRunInfo )
                      != IDE_SUCCESS );

            if ( sIsClosedRun == ID_TRUE )
            {
                sClosedRun++;
            }            
            else
            {
                /* nothing to do */
            }

        }
        
        IDE_TEST( iduCheckSessionEvent(mStatistics)
                  != IDE_SUCCESS );
    }

    sState = 1;
    IDE_TEST( iduMemMgr::free( sHeapMap      ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sMergeRunInfo ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    mIsSuccess = ID_FALSE;
    
    for( i = 0; i < sLastPos; i++)
    {
        if( sMergeRunInfo[i].mPageHdr != NULL )
        {
            (void)sdbBufferMgr::unfixPage(mStatistics, /* idvSQL* */
                                          (UChar*)sMergeRunInfo[i].mPageHdr );
        }
    }
    
    switch( sState )
    {
        case 3:
            (void)sdrMiniTrans::rollback( &sMtx );
        case 2:
            (void)iduMemMgr::free( sHeapMap );
        case 1:
            (void)iduMemMgr::free( sMergeRunInfo );
            break;
        default:
            break;
    }
            
    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::makeTree( sdnbBUBuild    * aThreads,
                              UInt             aThreadCnt,
                              UInt             aMergePageCnt,
                              sdnbStatistic  * aIndexStat,
                              UInt             aStatFlag )
{
    sdrMtx              sMtx;
    idBool              sIsEmpty;
    idBool              sIsSuccess;
    idBool              sIsClosedRun;
    idBool              sIsDupValue;

    SShort              sLeafSlotSeq;
    UInt                i;
    UInt                sLastPos = 0;
    UInt                sMinIdx;
    UInt                sClosedRun = 0;
    UInt                sHeapMapCount = 1;
    SInt              * sHeapMap;

    scPageID            sMinNode = SD_NULL_PID;
    scPageID            sMaxNode = SD_NULL_PID;

    sdnbMergeRunInfo  * sMergeRunInfo;    // merge에 사용되는 run
    
    sdnbStack           sStack;
    sdnbSortedBlk       sQueueInfo;
    sdpPhyPageHdr     * sLeafPage;
    UInt                sState = 0;
    scPageID            sPrevPID4Stat     = SC_NULL_PID;
    scPageID            sCurPID4Stat      = SC_NULL_PID;
    SLong               sIndexHeight      = 0;
    SLong               sNumPage          = 0;
    SLong               sClusteringFactor = 0;
    SLong               sNumDist          = 0;
    SLong               sKeyCount         = 0;
    SInt                sHeight = 0;
    UChar             * sCurrPage = NULL;
    UChar               sIsConsistent = SDP_PAGE_CONSISTENT;
    sdpSegInfo          sSegInfo;
    sdpSegmentDesc    * sSegmentDesc;

    // sdnbBTree::initStack(&sStack);
    sStack.mIndexDepth = -1;
    sStack.mCurrentDepth = -1;
    sStack.mIsSameValueInSiblingNodes = ID_FALSE;

    /* TC/FIT/Limit/sm/sdn/sdnb/sdnbBUBuild_makeTree_calloc.sql */
    IDU_FIT_POINT_RAISE( "sdnbBUBuild::makeTree::calloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::calloc( IDU_MEM_SM_SDN, 
                                       1,
                                       (ULong)aMergePageCnt * ID_SIZEOF(sdnbMergeRunInfo),
                                       (void**)&sMergeRunInfo ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 1;

    while( sHeapMapCount < (aMergePageCnt*2) ) // get heap size
    {
        sHeapMapCount = sHeapMapCount * 2;
    }

    /* TC/FIT/Limit/sm/sdn/sdnb/sdnbBUBuild_makeTree_malloc.sql */
    IDU_FIT_POINT_RAISE( "sdnbBUBuild::makeTree::malloc",
                          insufficient_memory );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SDN,
                                       (ULong)sHeapMapCount * ID_SIZEOF(SInt),
                                       (void**)&sHeapMap ) != IDE_SUCCESS,
                    insufficient_memory );
    sState = 2;

    for( i = 0; i < sHeapMapCount; i++ )
    {
        sHeapMap[i] = -1;
    }    

    for( i = 0; i < aThreadCnt; i++ )
    {
        while(1)
        {
            IDE_TEST( aThreads[i].mPIDBlkQueue.dequeue( ID_FALSE,
                                                        (void*)&sQueueInfo,
                                                        &sIsEmpty )
                      != IDE_SUCCESS );
        
            if( sIsEmpty == ID_TRUE )
            {
                break;
            }

            sMergeRunInfo[sLastPos].mHeadPID = sQueueInfo.mStartPID;

            IDE_TEST( sdbBufferMgr::fixPageByPID(
                          mStatistics, /* idvSQL* */
                          mIndex->mIndexSegDesc.mSpaceID,
                          sMergeRunInfo[sLastPos].mHeadPID,
                          (UChar**)&sMergeRunInfo[sLastPos].mPageHdr,
                          &sIsSuccess )
                      != IDE_SUCCESS );

            sLastPos++;
            
            if( sLastPos >= aMergePageCnt )
            {
                break;
            }
        }
    }
    
    IDE_DASSERT( sLastPos <= aMergePageCnt );
   
    IDE_TEST( mergeFreePage( aThreads, aThreadCnt ) != IDE_SUCCESS );
    
    sLeafSlotSeq = 0;

    IDE_TEST( heapInit( aIndexStat,
                        sLastPos,
                        sHeapMapCount,
                        sHeapMap,
                        sMergeRunInfo )
              != IDE_SUCCESS );

    sLeafPage = NULL;
    sClosedRun = 0;

    IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                   &sMtx,
                                   mTrans,
                                   mLoggingMode,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    sState = 3;

    // NOLOGGING mini-transaction의 Persistent flag 설정
    // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
    sdrMiniTrans::setNologgingPersistent( &sMtx );
        
    while( 1 )
    {
        if( sClosedRun == sLastPos ) // merge : finished
        {
            // BUG-17615 로깅량 줄이기
            if ( sLeafPage != NULL )
            {
                IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx, 
                                                     (UChar*)sLeafPage,
                                                     (void*)sLeafPage,
                                                     SD_PAGE_SIZE, 
                                                     SDR_SDP_BINARY )
                          != IDE_SUCCESS );
            }
            break;
        }

        sMinIdx = sHeapMap[1];

        sIsDupValue = ID_FALSE;
        IDE_TEST( write2LeafNode( aIndexStat,
                                  &sMtx,
                                  &sLeafPage,
                                  &sStack,
                                  &sLeafSlotSeq,
                                  &sMergeRunInfo[sMinIdx].mRunKey,
                                  &sIsDupValue )
                  != IDE_SUCCESS );

        /* 통계정보 구축 */
        sPrevPID4Stat = sCurPID4Stat;
        sCurPID4Stat  = ((sdnbKeyInfo*)&sMergeRunInfo[sMinIdx].mRunKey)->mRowPID;
        sKeyCount++;
        if ( sPrevPID4Stat != sCurPID4Stat )
        {
            sClusteringFactor ++;
        }
    
        if ( ( ( ((sdnRuntimeHeader*)mIndex->mHeader)->mIsUnique == ID_TRUE ) &&
               ( ((sdnRuntimeHeader*)mIndex->mHeader)->mIsNotNull == ID_TRUE ) ) ||
             ( sIsDupValue == ID_FALSE ) )
        {
            /* BUG-30074 - disk table의 unique index에서 NULL key 삭제 시/
             *             non-unique index에서 deleted key 추가 시 NumDist의
             *             정확성이 많이 떨어집니다.
             *
             * Null Key의 경우 NumDist를 갱신하지 않도록 한다. */
            if ( sdnbBTree::isNullKey( (sdnbHeader*)mIndex->mHeader,
                                       sMergeRunInfo[sMinIdx].mRunKey.mKeyValue )
                 != ID_TRUE )
            {
                sNumDist++;
            }
        }
        
        if ( sMinNode == SD_NULL_PID )
        {
            sMinNode = sdpPhyPage::getPageID((UChar*)sLeafPage);
        }

        sMaxNode = sdpPhyPage::getPageID((UChar*)sLeafPage);

        /* FIT/ART/sm/Bugs/BUG-39162/BUG-39162.tc */
        IDU_FIT_POINT( "1.BUG-39162@dnbBUBuild::makeTree" );

        IDE_TEST( heapPop( aIndexStat,
                           sMinIdx,
                           &sIsClosedRun,
                           sHeapMapCount,
                           sHeapMap,
                           sMergeRunInfo )
                  != IDE_SUCCESS );

        if( sIsClosedRun == ID_TRUE )
        {
            sClosedRun++;
        }
    }

    /*
     * To fix BUG-23676
     * NOLOGGING에서의 Consistent Flag는 삽입된 키와 커밋되어야 한다.
     */
    for( sHeight = sStack.mIndexDepth; sHeight >= 0; sHeight-- )
    {
        sCurrPage = sdrMiniTrans::getPagePtrFromPageID(
                                   &sMtx,
                                   mIndex->mIndexSegDesc.mSpaceID,
                                   sStack.mStack[sHeight].mNode );

        if( sCurrPage == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  sStack.mStack[sHeight].mNode,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  &sMtx,
                                                  &sCurrPage,
                                                  &sIsSuccess,
                                                  NULL )
                      != IDE_SUCCESS );
        }
        
        if( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( &sMtx,
                                                      (sdpPhyPageHdr*)sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }
    }

    if( sStack.mIndexDepth >= 0 )
    {
        ((sdnbHeader*)mIndex->mHeader)->mRootNode =
                                    sStack.mStack[sStack.mIndexDepth].mNode;
        sIndexHeight = sStack.mIndexDepth + 1;
    }
    else
    {
        IDE_DASSERT( (mBuildFlag & SMI_INDEX_BUILD_UNCOMMITTED_ROW_MASK) ==
                                     SMI_INDEX_BUILD_UNCOMMITTED_ROW_ENABLE );
        ((sdnbHeader*)mIndex->mHeader)->mRootNode = SD_NULL_PID;
        sIndexHeight = 0;
    }

    sSegmentDesc = &((sdnbHeader*)mIndex->mHeader)->mSegmentDesc;
    IDE_TEST( sdpSegDescMgr::getSegMgmtOp(sSegmentDesc)->mGetSegInfo( 
                                       NULL, /* Statistics */
                                       mIndex->mIndexSegDesc.mSpaceID,
                                       sdpSegDescMgr::getSegPID( sSegmentDesc),
                                       NULL, /* aTableHeader */
                                       &sSegInfo )
              != IDE_SUCCESS );
    sNumPage = sSegInfo.mFmtPageCnt;

    ideLog::log( IDE_SM_0, "\
========================================\n\
[IDX_CRE] 4. Set Stat                   \n\
========================================\n" );
    IDE_TEST( updateStatistics( mStatistics,
                                &sMtx,
                                mIndex,
                                sMinNode,
                                sMaxNode,
                                sNumPage,
                                sClusteringFactor,
                                sIndexHeight,
                                sNumDist,
                                sKeyCount,
                                aStatFlag )
              != IDE_SUCCESS );
    
    sState = 2;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( iduMemMgr::free( sHeapMap      ) != IDE_SUCCESS );
    
    sState = 0;
    IDE_TEST( iduMemMgr::free( sMergeRunInfo ) != IDE_SUCCESS );

    IDE_TEST( removeFreePage() != IDE_SUCCESS );    

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;
    
    mIsSuccess = ID_FALSE;

    switch( sState )
    {
        case 3:
            (void)sdrMiniTrans::rollback( &sMtx );
        case 2:
            (void)iduMemMgr::free( sHeapMap );
        case 1:
            // fix BUG-27173 : [codeSonar] Use After Free
            for( i = 0; i < sLastPos; i++)
            {
                if( sMergeRunInfo[i].mPageHdr != NULL )
                {
                    (void)sdbBufferMgr::unfixPage(mStatistics, /* idvSQL* */
                                                  (UChar*)sMergeRunInfo[i].mPageHdr );
                }
            }
            (void)iduMemMgr::free( sMergeRunInfo );
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::write2LeafNode( sdnbStatistic  *aIndexStat,
                                    sdrMtx         *aMtx,
                                    sdpPhyPageHdr **aPage,
                                    sdnbStack      *aStack,
                                    SShort         *aSlotSeq,
                                    sdnbKeyInfo    *aKeyInfo,
                                    idBool         *aIsDupValue )
{
    UInt             sKeyValueSize;
    SShort           sSlotSeq;
    UShort           sHeight = 0;
    scPageID         sPrevPID;
    sdpPhyPageHdr   *sCurrPage;
    sdnbNodeHdr     *sNodeHdr;
    idBool           sIsSuccess;
    scPageID         sCurrPID;
    scPageID         sNullPID = SD_NULL_PID;
    sdnbKeyInfo      sPrevKey;
    UChar            sIsConsistent = SDP_PAGE_CONSISTENT;
    UChar            sTxBoundType  = SDNB_KEY_TB_CTS;

    IDE_DASSERT( aKeyInfo != NULL );
    *aIsDupValue = ID_FALSE;

    /* BUG-44973
     * TBK 테스트 위해 추가된 프로퍼티( __DISABLE_TRANSACTION_BOUND_IN_CTS = 1 )를 설정하면
     * INDEX BOTTOM UP BUILD 에서도  TBK로 만들어 지도록 한다. */
    sTxBoundType = ( ( smuProperty::getDisableTransactionBoundInCTS() == 1 ) ? SDNB_KEY_TB_KEY : SDNB_KEY_TB_CTS );

    if( *aPage == NULL )
    {
        IDE_TEST( allocPage( aMtx,
                             &sCurrPID,
                             &sCurrPage,
                             &sNodeHdr,
                             NULL,
                             &sNullPID,
                             &sNullPID )
                  != IDE_SUCCESS );

        IDE_TEST( sdnIndexCTL::init( aMtx,
                                     &((sdnRuntimeHeader*)mIndex->mHeader)->mSegmentDesc.mSegHandle,
                                     sCurrPage,
                                     0 )
                  != IDE_SUCCESS );

        IDE_TEST( sdnbBTree::initializeNodeHdr( aMtx,
                                                sCurrPage,
                                                SD_NULL_PID,
                                                sHeight,
                                                ID_TRUE )
                  != IDE_SUCCESS );
                                          
        *aPage    = sCurrPage;
        *aSlotSeq = sSlotSeq = 0;
        aStack->mIndexDepth = sHeight;
        aStack->mStack[sHeight].mNode = sCurrPID;
    }
    else
    {
        sCurrPage = *aPage;
        sSlotSeq  = (++*aSlotSeq);

        IDE_DASSERT( sSlotSeq > 0 );
        
        IDE_TEST( getKeyInfoFromLKey( sCurrPage->mPageID,
                                      sSlotSeq -1,
                                      sCurrPage,
                                      &sPrevKey )
                  != IDE_SUCCESS );
        
        (void)sdnbBTree::compareKeyAndKey( aIndexStat,
                                           ((sdnbHeader*)mIndex->mHeader)->mColumns,
                                           ((sdnbHeader*)mIndex->mHeader)->mColumnFence,
                                           aKeyInfo,
                                           &sPrevKey,
                                           SDNB_COMPARE_VALUE   |
                                           SDNB_COMPARE_PID     |
                                           SDNB_COMPARE_OFFSET,
                                           aIsDupValue ) ;
    }

    sKeyValueSize = sdnbBTree::getKeyValueLength( 
                        &(((sdnbHeader*)mIndex->mHeader)->mColLenInfoList), 
                        aKeyInfo->mKeyValue );

    if ( sdpPhyPage::canAllocSlot( sCurrPage,
                                   SDNB_LKEY_LEN( sKeyValueSize, sTxBoundType ),
                                   ID_TRUE,
                                   SDP_1BYTE_ALIGN_SIZE )
         != ID_TRUE )
    {
        /*
         *  page 할당 실패로 인한 예외 발생으로 인하여
         *  log가 부분적으로만 쓰일 수 있다.
         *  따라서 미리 page가 할당 가능한지 판단해야 한다.
         *  최대 할당 페이지 수는 depth + 2 이다.
         */
        IDE_TEST( preparePages( aStack->mIndexDepth + 2 )
                  != IDE_SUCCESS );
        
        // Page에 충분한 공간이 없다면
        // 새로운 Page Alloc
        // 2개 이상의 leaf page가 생성되었으므로 internal page 필요함
        sPrevPID = sCurrPage->mPageID;

        // BUG-17615 로깅량 줄이기
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx, 
                                             (UChar*)sCurrPage,
                                             (void*)sCurrPage,
                                             SD_PAGE_SIZE, 
                                             SDR_SDP_BINARY )
                  != IDE_SUCCESS );
        
        /*
         * To fix BUG-23676
         * NOLOGGING에서의 Consistent Flag는 삽입된 키와 같이 커밋되어야 한다.
         */
        if ( mLoggingMode == SDR_MTX_NOLOGGING )
        {
            IDE_TEST( sdpPhyPage::setPageConsistency( aMtx,
                                                      sCurrPage,
                                                      &sIsConsistent )
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
        

        IDE_TEST( allocPage( aMtx,
                             &sCurrPID,
                             &sCurrPage,
                             &sNodeHdr,
                             sCurrPage,
                             &sPrevPID,
                             &sNullPID )
                  != IDE_SUCCESS );

        IDE_TEST( sdnIndexCTL::init( aMtx,
                                     &((sdnRuntimeHeader*)mIndex->mHeader)->mSegmentDesc.mSegHandle,
                                     sCurrPage,
                                     0 )
                  != IDE_SUCCESS );

        IDE_TEST( sdnbBTree::initializeNodeHdr( aMtx,
                                                sCurrPage,
                                                SD_NULL_PID,
                                                sHeight,
                                                  ID_TRUE )
                  != IDE_SUCCESS );
                                          
        aStack->mStack[sHeight].mNode = sCurrPID;
         
        IDE_TEST( write2ParentNode( aMtx,
                                    aStack,
                                    sHeight,
                                    aKeyInfo,
                                    sPrevPID,
                                    sCurrPID )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::commit(aMtx) != IDE_SUCCESS );


        IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                       aMtx,
                                       mTrans,
                                       mLoggingMode,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );

        // NOLOGGING mini-transaction의 Persistent flag 설정
        // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
        sdrMiniTrans::setNologgingPersistent( aMtx );

        /* BUG-39162 timeout 확인 위치 변경 */
        // To fix BUG-17728
        // 매번 Session Event를 검사하는 것은 성능상 부하이기 때문에,
        // 새로운 LeafNode가 생성될때마다 Session Event를 검사한다.
        IDE_TEST( iduCheckSessionEvent(mStatistics) != IDE_SUCCESS );

        /* FIT/ART/sm/Bugs/BUG-39162/BUG-39162.tc */
        IDU_FIT_POINT( "1.BUG-39162@sdnbBUBuild::write2LeafNode" );

        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics, /* idvSQL* */
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              sCurrPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        *aPage = sCurrPage;
        *aSlotSeq = sSlotSeq = 0;
    }

    IDE_TEST( sdnbBTree::insertLKey( aMtx,
                                     (sdnbHeader*)mIndex->mHeader,
                                     sCurrPage,
                                     sSlotSeq,
                                     SDN_CTS_INFINITE,
                                     sTxBoundType,
                                     aKeyInfo,
                                     sKeyValueSize,
                                     ID_FALSE, /* no logging */ 
                                     NULL )    /* key offset */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::write2ParentNode( sdrMtx         *aMtx,
                                      sdnbStack      *aStack,
                                      UShort          aHeight,
                                      sdnbKeyInfo    *aKeyInfo,
                                      scPageID        aPrevPID,
                                      scPageID        aChildPID )
{
    UInt             sKeyValueSize;
    SShort           sSlotSeq;
    UShort           sHeight = aHeight + 1;
    scPageID         sPrevPID;
    sdpPhyPageHdr   *sCurrPage;
    sdnbNodeHdr     *sNodeHdr;
    idBool           sIsSuccess;
    scPageID         sCurrPID;
    scPageID         sNullPID = SD_NULL_PID;
    UChar            sIsConsistent = SDP_PAGE_CONSISTENT;
    idBool           sLoggingMode =
        (mLoggingMode == SDR_MTX_LOGGING)? ID_TRUE : ID_FALSE;    
    
    IDE_DASSERT( aKeyInfo != NULL );

    /* 새로운 루트 노드가 생성되는 경우라면 */
    if( aStack->mIndexDepth < sHeight ) 
    {
        IDE_TEST( allocPage( aMtx,
                             &sCurrPID,
                             &sCurrPage,
                             &sNodeHdr,
                             NULL,
                             &sNullPID,
                             &sNullPID )
                  != IDE_SUCCESS );

        // set height
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mHeight,
                                             (void*)&sHeight,
                                             ID_SIZEOF(sHeight) )
                  != IDE_SUCCESS );

        aStack->mIndexDepth = sHeight;
        aStack->mStack[sHeight].mNode = sCurrPID;
        aStack->mStack[sHeight].mKeyMapSeq = 0;        

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mLeftmostChild,
                                             (void*)&aPrevPID,
                                             ID_SIZEOF(aPrevPID) )
                  != IDE_SUCCESS );

        sKeyValueSize = sdnbBTree::getKeyValueLength( 
                            &(((sdnbHeader*)mIndex->mHeader)->mColLenInfoList),
                            aKeyInfo->mKeyValue );
        sSlotSeq = 0;            
        IDE_TEST( sdnbBTree::insertIKey( mStatistics,
                                         aMtx,
                                         (sdnbHeader*)mIndex->mHeader,
                                         sCurrPage,
                                         sSlotSeq,
                                         aKeyInfo,
                                         sKeyValueSize,
                                         aChildPID,
                                         sLoggingMode )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( sdbBufferMgr::getPageByPID( mStatistics,
                                              mIndex->mIndexSegDesc.mSpaceID,
                                              aStack->mStack[sHeight].mNode,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sCurrPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        sSlotSeq = aStack->mStack[sHeight].mKeyMapSeq + 1;
        aStack->mStack[sHeight].mKeyMapSeq += 1;

        sKeyValueSize = sdnbBTree::getKeyValueLength( 
                            &(((sdnbHeader*)mIndex->mHeader)->mColLenInfoList), 
                            aKeyInfo->mKeyValue );

        if ( sdpPhyPage::canAllocSlot( sCurrPage,
                                       SDNB_IKEY_LEN(sKeyValueSize),
                                       ID_TRUE,
                                       SDP_1BYTE_ALIGN_SIZE )
             != ID_TRUE )
        {
            /* 할당할 공간이 남아 있지 않다면 */
            sPrevPID = sCurrPage->mPageID;

            /*
             * To fix BUG-23676
             * NOLOGGING에서의 Consistent Flag는 삽입된 키와 같이 커밋되어야 한다.
             */
            if ( mLoggingMode == SDR_MTX_NOLOGGING )
            {
                IDE_TEST( sdpPhyPage::setPageConsistency( aMtx,
                                                          sCurrPage,
                                                          &sIsConsistent )
                          != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do */
            }

            IDE_TEST( allocPage( aMtx,
                                 &sCurrPID,
                                 &sCurrPage,
                                 &sNodeHdr,
                                 sCurrPage,   // Previous Page
                                 &sPrevPID,
                                 &sNullPID )
                      != IDE_SUCCESS );

            // set height
            IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                                 (UChar*)&sNodeHdr->mHeight,
                                                 (void*)&sHeight,
                                                 ID_SIZEOF(sHeight) )
                      != IDE_SUCCESS );

            // leftmost child PID
            IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                                 (UChar*)&sNodeHdr->mLeftmostChild,
                                                 (void*)&aChildPID,
                                                 ID_SIZEOF(aChildPID) )
                      != IDE_SUCCESS );
            
            sSlotSeq = 0;

            // parent page에 반영
            aStack->mStack[sHeight].mNode = sCurrPID;
            aStack->mStack[sHeight].mKeyMapSeq = -1;

            IDE_TEST( write2ParentNode( aMtx,
                                        aStack,
                                        sHeight,
                                        aKeyInfo,
                                        sPrevPID,
                                        sCurrPID )
                      != IDE_SUCCESS );

            IDE_TEST( sdrMiniTrans::commit(aMtx) != IDE_SUCCESS );


            IDE_TEST( sdrMiniTrans::begin( mStatistics, /* idvSQL* */
                                           aMtx,
                                           mTrans,
                                           mLoggingMode,
                                           ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                           gMtxDLogType )
                      != IDE_SUCCESS );

            // NOLOGGING mini-transaction의 Persistent flag 설정
            // PERSISTENT_ON : Persistent (nologging index build일 경우에만)
            sdrMiniTrans::setNologgingPersistent( aMtx );
        }
        else
        {
            IDE_TEST( sdnbBTree::insertIKey( mStatistics,
                                             aMtx,
                                             (sdnbHeader*)mIndex->mHeader,
                                             sCurrPage,
                                             sSlotSeq,
                                             aKeyInfo,
                                             sKeyValueSize,
                                             aChildPID,
                                             sLoggingMode )
                      != IDE_SUCCESS );
        }
    } // if( aStack->mIndexDepth < sHeight ) 
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::heapInit( sdnbStatistic    * aIndexStat,
                              UInt               aRunCount,
                              UInt               aHeapMapCount,
                              SInt             * aHeapMap,
                              sdnbMergeRunInfo * aMergeRunInfo )
{
    SInt            sRet;
    UInt            i;
    UInt            sLChild;
    UInt            sRChild;
    idBool          sIsSameValue;
    
    UInt            sPos = aHeapMapCount / 2;
    UInt            sHeapLevelCnt = aRunCount;
    UChar          *sPagePtr;
    UChar          *sSlotDirPtr;

    for( i = 0; i < aRunCount; i++)
    {
        aMergeRunInfo[i].mSlotSeq = 0;

        IDE_TEST( getKeyInfoFromLKey( aMergeRunInfo[i].mHeadPID,
                                      aMergeRunInfo[i].mSlotSeq,
                                      aMergeRunInfo[i].mPageHdr,
                                      &aMergeRunInfo[i].mRunKey )
                  != IDE_SUCCESS );

        sPagePtr = (UChar*)aMergeRunInfo[i].mPageHdr;
        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
        aMergeRunInfo[i].mSlotCnt = sdpSlotDirectory::getCount(sSlotDirPtr);
    }

    for( i = 0; i < sHeapLevelCnt; i++)
    {
        aHeapMap[i + sPos] = i;
    }

    for(i = i + sPos ; i < aHeapMapCount; i++)
    {
        aHeapMap[i] = -1;
    }

    IDE_DASSERT(i != 0);
    
    while( 1 )
    {
        sPos = sPos / 2;
        sHeapLevelCnt  = (sHeapLevelCnt + 1) / 2;
        
        for( i = 0; i < sHeapLevelCnt; i++ )
        {
            sLChild  = (i + sPos) * 2;
            sRChild = sLChild + 1;
            
            if (aHeapMap[sLChild] == -1 && aHeapMap[sRChild] == -1)
            {
                aHeapMap[i + sPos]= -1;
            }
            else if (aHeapMap[sLChild] == -1)                     
            {
                aHeapMap[i + sPos] = aHeapMap[sRChild];
            }
            else if (aHeapMap[sRChild] == -1)
            {
                aHeapMap[i + sPos] = aHeapMap[sLChild];
            }
            else
            {
                sRet = sdnbBTree::compareKeyAndKey( aIndexStat,
                                                    ((sdnbHeader*)mIndex->mHeader)->mColumns,
                                                    ((sdnbHeader*)mIndex->mHeader)->mColumnFence,
                                                    &aMergeRunInfo[aHeapMap[sLChild]].mRunKey,
                                                    &aMergeRunInfo[aHeapMap[sRChild]].mRunKey,
                                                    SDNB_COMPARE_VALUE   |
                                                    SDNB_COMPARE_PID     |
                                                    SDNB_COMPARE_OFFSET,
                                                    &sIsSameValue ) ;
                
                if( sIsSameValue == ID_TRUE )
                {
                    IDE_TEST_RAISE( isNull( (sdnbHeader*)(mIndex->mHeader),
                                            aMergeRunInfo[aHeapMap[sLChild]].mRunKey.mKeyValue )
                                    != ID_TRUE, ERR_UNIQUENESS_VIOLATION );
                }
            
                if(sRet > 0)
                {
                    aHeapMap[i + sPos] = aHeapMap[sRChild];
                }
                else
                {
                    aHeapMap[i + sPos] = aHeapMap[sLChild];
                }
            }                
        }
        if(sPos == 1)    // the root of selection tree
        {
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNIQUENESS_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smnUniqueViolation ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdnbBUBuild::heapPop( sdnbStatistic    * aIndexStat,
                             UInt               aMinIdx,
                             idBool           * aIsClosedRun,
                             UInt               aHeapMapCount,
                             SInt             * aHeapMap,
                             sdnbMergeRunInfo * aMergeRunInfo )
{
    SInt            sRet;
    UInt            sLChild;
    UInt            sRChild;
    idBool          sIsSameValue;
    UChar          *sPagePtr;
    UChar          *sSlotDirPtr;

    UInt            sPos = aHeapMapCount / 2;

    idBool          sIsSuccess;

    aMergeRunInfo[aMinIdx].mSlotSeq++;
    *aIsClosedRun = ID_FALSE;
    
    if ( aMergeRunInfo[aMinIdx].mSlotSeq ==
         aMergeRunInfo[aMinIdx].mSlotCnt )
    {
        IDE_TEST (freePage( aMergeRunInfo[aMinIdx].mHeadPID )
                  != IDE_SUCCESS);
            
        aMergeRunInfo[aMinIdx].mHeadPID =
            aMergeRunInfo[aMinIdx].mPageHdr->mListNode.mNext;

        IDE_TEST( sdbBufferMgr::unfixPage(
                                      mStatistics, /* idvSQL* */
                                      (UChar*)aMergeRunInfo[aMinIdx].mPageHdr )
                  != IDE_SUCCESS );

        aMergeRunInfo[aMinIdx].mPageHdr = NULL;
            
        if( aMergeRunInfo[aMinIdx].mHeadPID == SD_NULL_PID )
        {
            *aIsClosedRun = ID_TRUE;
            aHeapMap[sPos + aMinIdx] = -1;
        }
        else
        {
            IDE_TEST( sdbBufferMgr::fixPageByPID( mStatistics, /* idvSQL* */
                                                  mIndex->mIndexSegDesc.mSpaceID,
                                                  aMergeRunInfo[aMinIdx].mHeadPID,
                                                  (UChar**)&aMergeRunInfo[aMinIdx].mPageHdr,
                                                  &sIsSuccess )
                      != IDE_SUCCESS );

            aMergeRunInfo[aMinIdx].mSlotSeq = 0;

            sPagePtr      = (UChar*)aMergeRunInfo[aMinIdx].mPageHdr;
            sSlotDirPtr   = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
            aMergeRunInfo[aMinIdx].mSlotCnt = sdpSlotDirectory::getCount(sSlotDirPtr);

            IDE_DASSERT( aMergeRunInfo[aMinIdx].mSlotCnt > 0 );
        }
    }
    else
    {
        IDE_DASSERT( aMergeRunInfo[aMinIdx].mSlotSeq <
                     aMergeRunInfo[aMinIdx].mSlotCnt );
    }

    if ( aMergeRunInfo[aMinIdx].mHeadPID != SD_NULL_PID )
    {
        IDE_DASSERT( aMergeRunInfo[aMinIdx].mPageHdr != NULL );
        
        IDE_TEST( getKeyInfoFromLKey( aMergeRunInfo[aMinIdx].mHeadPID,
                                      aMergeRunInfo[aMinIdx].mSlotSeq,
                                      aMergeRunInfo[aMinIdx].mPageHdr,
                                      &aMergeRunInfo[aMinIdx].mRunKey )
                  != IDE_SUCCESS );
    }

    sPos = (sPos + aMinIdx) / 2;
    
    while( 1 )
    {
        sLChild = sPos * 2;      // Left child position
        sRChild = sLChild + 1;   // Right child Position
        
        if (aHeapMap[sLChild] == -1 && aHeapMap[sRChild] == -1)
        {
            aHeapMap[sPos]= -1;
        }
        else if (aHeapMap[sLChild] == -1)
        {
            aHeapMap[sPos] = aHeapMap[sRChild];
        }
        else if (aHeapMap[sRChild] == -1)
        {
            aHeapMap[sPos] = aHeapMap[sLChild];
        }
        else
        {
            sRet = sdnbBTree::compareKeyAndKey( aIndexStat,
                                                ((sdnbHeader*)mIndex->mHeader)->mColumns,
                                                ((sdnbHeader*)mIndex->mHeader)->mColumnFence,
                                                &aMergeRunInfo[aHeapMap[sLChild]].mRunKey,
                                                &aMergeRunInfo[aHeapMap[sRChild]].mRunKey,
                                                SDNB_COMPARE_VALUE   |
                                                SDNB_COMPARE_PID     |
                                                SDNB_COMPARE_OFFSET,
                                                &sIsSameValue ) ;
            
            if( sIsSameValue == ID_TRUE )
            {
                IDE_TEST_RAISE( isNull( (sdnbHeader*)(mIndex->mHeader),
                                        aMergeRunInfo[aHeapMap[sLChild]].mRunKey.mKeyValue )
                                != ID_TRUE, ERR_UNIQUENESS_VIOLATION );
            }

            if(sRet > 0)
            {
                aHeapMap[sPos] = aHeapMap[sRChild];
            }
            else
            {
                aHeapMap[sPos] = aHeapMap[sLChild];
            }
        }
        if(sPos == 1)    // the root of selection tree
        {
            break;
        }
        sPos = sPos / 2;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNIQUENESS_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smnUniqueViolation ) );
    }
    IDE_EXCEPTION_END;    

    return IDE_FAILURE;
}


/* ------------------------------------------------
 * 모든 컬럼이 NULL을 가지고 있는지 검사
 * ----------------------------------------------*/
idBool sdnbBUBuild::isNull( sdnbHeader * aHeader,
                            UChar      * aKey )
{
    idBool               sIsNull = ID_TRUE;

    if(aHeader->mIsUnique == ID_TRUE )
    {
        if( aHeader->mIsNotNull == ID_TRUE )
        {
            sIsNull = ID_FALSE;
        }
        else
        {
            sIsNull = sdnbBTree::isNullKey( aHeader , aKey );
        }
    }
    
    return sIsNull;
}


/* ------------------------------------------------
 * Index 통계 정보 갱신
 * ----------------------------------------------*/
IDE_RC sdnbBUBuild::updateStatistics( idvSQL         * aStatistics,
                                      sdrMtx         * aMtx,
                                      smnIndexHeader * aIndex,
                                      scPageID         aMinNode,
                                      scPageID         aMaxNode,
                                      SLong            aNumPage,
                                      SLong            aClusteringFactor,
                                      SLong            aIndexHeight,
                                      SLong            aNumDist,
                                      SLong            aKeyCount,
                                      UInt             aStatFlag )
{
    sdnbHeader       *sIndexHeader = (sdnbHeader*)aIndex->mHeader;
    SShort            sSlotSeq;
    idBool            sIsSuccess;
    idBool            sIsFixPage = ID_FALSE;
    sdnbColumn       *sColumn = &sIndexHeader->mColumns[0];
    sdpPhyPageHdr    *sPage = NULL;
    UChar            *sSlotDirPtr;
    UChar            *sSlotOffset;
    smiIndexStat      sStat;

    IDE_TEST_CONT( sIndexHeader->mRootNode == SD_NULL_PID,
                    SKIP_UPDATE_INDEX_STATISTICS );
    
    idlOS::memset( &sStat, 0, ID_SIZEOF( smiIndexStat ) );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics, /* idvSQL* */
                                          aIndex->mIndexSegDesc.mSpaceID,
                                          sIndexHeader->mRootNode,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          (UChar**)&sPage,
                                          &sIsSuccess,
                                          NULL )
              != IDE_SUCCESS );

    // for min value
    if( aMinNode != sIndexHeader->mRootNode )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics, /* idvSQL* */
                                              aIndex->mIndexSegDesc.mSpaceID,
                                              aMinNode,
                                              (UChar**)&sPage,
                                              &sIsSuccess )
                  != IDE_SUCCESS );
        sIsFixPage = ID_TRUE;
    }

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPage);
    sSlotSeq    = 0;
    IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                        sSlotSeq,
                                                        &sSlotOffset )
               == IDE_SUCCESS );

    while( sdnbBTree::isNullColumn( sColumn, 
                                    SDNB_LKEY_KEY_PTR(sSlotOffset) )  
           == ID_TRUE )
    {
        if( (++sSlotSeq) == sdpSlotDirectory::getCount(sSlotDirPtr) )
        {
            aMinNode = sPage->mListNode.mNext;

            if( sIsFixPage == ID_TRUE )
            {
                sIsFixPage = ID_FALSE;
                IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, /* idvSQL* */
                                                   (UChar*)sPage )
                          != IDE_SUCCESS );
            }
            if( aMinNode == SD_NULL_PID )
            {
                break;
            }

            IDE_TEST( sdbBufferMgr::fixPageByPID(
                                              aStatistics, /* idvSQL* */
                                              aIndex->mIndexSegDesc.mSpaceID,
                                              aMinNode,
                                              (UChar**)&sPage,
                                              &sIsSuccess )
                      != IDE_SUCCESS );
            sIsFixPage = ID_TRUE;

            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPage);
            sSlotSeq    = 0;
        }
        IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                            sSlotSeq,
                                                            &sSlotOffset )
                   == IDE_SUCCESS );
    }

    if( aMinNode != SD_NULL_PID )
    {
        IDE_ERROR( sdnbBTree::setMinMaxValue( sIndexHeader,
                                              (UChar*)sPage,
                                              sSlotSeq,
                                              (UChar*)sStat.mMinValue )
                   == IDE_SUCCESS );

        sIndexHeader->mMinNode = aMinNode;

        // for max value
        if( aMinNode != aMaxNode )
        {
            if( sIsFixPage == ID_TRUE )
            {
                sIsFixPage = ID_FALSE;
                IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, /* idvSQL* */
                                                   (UChar*)sPage )
                          != IDE_SUCCESS );
            }

            IDE_TEST( sdbBufferMgr::fixPageByPID(
                                              aStatistics, /* idvSQL* */
                                              aIndex->mIndexSegDesc.mSpaceID,
                                              aMaxNode,
                                              (UChar**)&sPage,
                                              &sIsSuccess )
                      != IDE_SUCCESS );
            sIsFixPage = ID_TRUE;
        }

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPage);
        sSlotSeq    = sdpSlotDirectory::getCount(sSlotDirPtr) - 1;
        IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                            sSlotSeq, 
                                                            &sSlotOffset )
                   == IDE_SUCCESS );

        while( sdnbBTree::isNullColumn( sColumn,
                                        SDNB_LKEY_KEY_PTR(sSlotOffset)) 
               == ID_TRUE )
        {
            if ( (--sSlotSeq) == -1 )
            {
                aMaxNode = sPage->mListNode.mPrev;

                if ( sIsFixPage == ID_TRUE )
                {
                    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, /* idvSQL* */
                                                       (UChar*)sPage )
                              != IDE_SUCCESS );
                    sIsFixPage = ID_FALSE;
                }
                if( aMaxNode == SD_NULL_PID )
                {
                    break;
                }

                IDE_TEST( sdbBufferMgr::fixPageByPID(
                                                  aStatistics, /* idvSQL* */
                                                  aIndex->mIndexSegDesc.mSpaceID,
                                                  aMaxNode,
                                                  (UChar**)&sPage,
                                                  &sIsSuccess )
                          != IDE_SUCCESS );
                sIsFixPage = ID_TRUE;

                sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPage);
                sSlotSeq    = sdpSlotDirectory::getCount(sSlotDirPtr) - 1;
            }
            else 
            {
                /* nothing to do */
            }
            IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                                sSlotSeq, 
                                                                &sSlotOffset )
                       == IDE_SUCCESS );
        }

       IDE_ERROR( sdnbBTree::setMinMaxValue( sIndexHeader,
                                             (UChar*)sPage,
                                             sSlotSeq,
                                             (UChar*)sStat.mMaxValue )
                  == IDE_SUCCESS );

        sIndexHeader->mMaxNode = aMaxNode;
    }

    if ( sIsFixPage == ID_TRUE )
    {
        sIsFixPage = ID_FALSE;
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, /* idvSQL* */
                                           (UChar*)sPage )
                  != IDE_SUCCESS );
    }

    sStat.mNumDist  = aNumDist;
    sStat.mKeyCount = aKeyCount;
    sStat.mNumPage  = aNumPage;
    sStat.mClusteringFactor = aClusteringFactor;
    sStat.mIndexHeight = aIndexHeight;

    IDE_TEST( smLayerCallback::setIndexStatWithoutMinMax( aIndex,
                                                          sdrMiniTrans::getTrans( aMtx ),
                                                          NULL,
                                                          &sStat,
                                                          ID_FALSE,
                                                          aStatFlag )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::setIndexMinValue( aIndex,
                                                 sdrMiniTrans::getTrans(aMtx),
                                                 (UChar*)sStat.mMinValue,
                                                 aStatFlag )
              != IDE_SUCCESS );
    IDE_TEST( smLayerCallback::setIndexMaxValue( aIndex,
                                                 sdrMiniTrans::getTrans(aMtx),
                                                 (UChar*)sStat.mMaxValue,
                                                 aStatFlag )
              != IDE_SUCCESS );


    IDE_EXCEPTION_CONT( SKIP_UPDATE_INDEX_STATISTICS );

    sIndexHeader->mIsConsistent = ID_TRUE;
    smrLogMgr::getLstLSN( &(sIndexHeader->mCompletionLSN) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsFixPage == ID_TRUE )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics, /* idvSQL* */
                                             (UChar*)sPage )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

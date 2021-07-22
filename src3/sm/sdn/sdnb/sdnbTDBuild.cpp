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
 * $Id: sdnbTDBuild.cpp
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
#include <ide.h>
#include <smu.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <smn.h>
#include <sdn.h>
#include <sdnReq.h>
#include <smxTrans.h>
#include <sdbMPRMgr.h>

sdnbTDBuild::sdnbTDBuild() : idtBaseThread()
{


}

sdnbTDBuild::~sdnbTDBuild()
{


}

/* ------------------------------------------------
 * Description :
 *
 * Index build 쓰레드 초기화
 * ----------------------------------------------*/
IDE_RC sdnbTDBuild::initialize( UInt             aTotalThreadCnt,
                                UInt             aID,
                                smcTableHeader * aTable,
                                smnIndexHeader * aIndex,
                                smSCN            aFstDskViewSCN,
                                idBool           aIsNeedValidation,
                                UInt             aBuildFlag,
                                idvSQL*          aStatistics,
                                idBool         * aContinue )
{

    UInt sStatus = 0;

    mTotalThreadCnt = aTotalThreadCnt;
    mID = aID;
    mTable = aTable;
    mIndex = aIndex;
    mIsNeedValidation = aIsNeedValidation;
    mBuildFlag = aBuildFlag;
    mStatistics = aStatistics;

    mFinished = ID_FALSE;
    mContinue = aContinue;
    mErrorCode = 0;

    mTrans = NULL;
    IDE_TEST( smLayerCallback::allocNestedTrans( &mTrans )
              != IDE_SUCCESS );
    sStatus = 1;

    /*
      * TASK-2356 [제품문제분석] altibase wait interface
      * 정확한 통계정보를 구하기 위해서는
      * Thread 개수만큼 idvSQL 이 존재해야 한다.
      */
    IDE_TEST( smLayerCallback::beginTrans( mTrans,
                                           SMI_TRANSACTION_REPL_NONE,
                                           NULL )
              != IDE_SUCCESS );

    smLayerCallback::setFstDskViewSCN( mTrans, &aFstDskViewSCN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sStatus == 1)
    {
        IDE_ASSERT( smLayerCallback::freeTrans( mTrans )
                    == IDE_SUCCESS );
        mTrans = NULL;
    }

    return IDE_FAILURE;
}

/* ------------------------------------------------
 * Description :
 *
 * buffer flush 쓰레드 해제
 * ----------------------------------------------*/
IDE_RC sdnbTDBuild::destroy()
{

    UInt sStatus = 0;

    if( mTrans != NULL )
    {
        sStatus = 2;
        IDU_FIT_POINT( "1.BUG-48210@sdnbTDBuild::destroy" );
        IDE_TEST( smLayerCallback::commitTrans( mTrans ) != IDE_SUCCESS );
        sStatus = 1;
        IDE_TEST( smLayerCallback::freeTrans( mTrans ) != IDE_SUCCESS );
        sStatus = 0;
        mTrans = NULL;
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

    return IDE_FAILURE;

}


/* ------------------------------------------------
 * Description :
 *
 * 쓰레드 메인 실행 루틴
 *
 * - interval 기간동안 wait하다가 buffer flush
 *   수행
 * ----------------------------------------------*/
void sdnbTDBuild::run()
{
    scSpaceID          sTBSID;
    UChar            * sPage;
    UChar            * sSlotDirPtr;
    UChar            * sSlot;
    SInt               sSlotCount;
    SInt               i;
    smSCN              sInfiniteSCN;
    smSCN              sStmtSCN;
    idBool             sIsSuccess;
    idBool             sIsUniqueIndex = ID_FALSE;
    idBool             sIsNotNullIndex = ID_FALSE;
    sdnbColumn       * sColumn;
    scPageID           sCurPageID;
    sdpPhyPageHdr     *sPageHdr;
    SInt               sState = 0;
    ULong              sRowBuf[SD_PAGE_SIZE/ID_SIZEOF(ULong)];
    sdSID              sRowSID;
    idBool             sIsRowDeleted;
    sdbMPRMgr          sMPRMgr;
    scOffset           sOffset;
    idBool             sIsPageLatchReleased = ID_TRUE;
    UInt               sColumnHeaderLen;
    UInt               sColumnValueLen;
    sdbMPRFilter4PScan sFilter4Scan;

    if( ((sdnRuntimeHeader*)mIndex->mHeader)->mIsUnique == ID_TRUE )
    {
        sIsUniqueIndex = ID_TRUE;
    }
    if( ((sdnRuntimeHeader*)mIndex->mHeader)->mIsNotNull == ID_TRUE )
    {
        sIsNotNullIndex = ID_TRUE;
    }

    ideLog::log( IDE_SM_0, "\
========================================\n\
  [IDX_CRE] BUILD INDEX ( Top-Down )    \n\
            NAME : %s                   \n\
            ID   : %u                   \n\
========================================\n\
          BUILD_THREAD_COUNT     : %d   \n\
========================================\n",
                 mIndex->mName,
                 mIndex->mId,
                 mTotalThreadCnt );
    
    SM_SET_SCN_INFINITE( &sInfiniteSCN );
    SM_MAX_SCN( &sStmtSCN );
    SM_SET_SCN_VIEW_BIT( &sStmtSCN );

    sTBSID = mTable->mSpaceID;

    IDE_TEST( sMPRMgr.initialize(
                  mStatistics,
                  sTBSID,
                  sdpSegDescMgr::getSegHandle( &mTable->mFixed.mDRDB ),
                  sdbMPRMgr::filter4PScan )
              != IDE_SUCCESS );
    sFilter4Scan.mThreadCnt = mTotalThreadCnt;
    sFilter4Scan.mThreadID  = mID;
    sState = 1;

    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    while( 1 )
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
        sIsPageLatchReleased = ID_FALSE;

        sPageHdr = sdpPhyPage::getHdr( sPage );

        if( sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA )
        {
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPage);
            sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

            for( i = 0; i < sSlotCount; i++ )
            {
                if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, i)
                    == ID_TRUE )
                {
                    continue;
                }

                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(sSlotDirPtr, 
                                                                  i,
                                                                  &sSlot)
                          != IDE_SUCCESS );
                IDE_DASSERT(sSlot != NULL);

                if( sdcRow::isHeadRowPiece(sSlot) != ID_TRUE )
                {
                    continue;
                }
                if( sdcRow::isDeleted(sSlot) == ID_TRUE )
                {
                    continue;
                }

                sRowSID = SD_MAKE_SID( sPageHdr->mPageID, i);

                sColumn = &((sdnbHeader*)mIndex->mHeader)->mColumns[0];

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
                                                      (UChar*)sRowBuf,
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
                    sOffset = 0;
                    sColumn = &((sdnbHeader*)mIndex->mHeader)->mColumns[0];
                    for( ; sColumn != ((sdnbHeader*)mIndex->mHeader)->mColumnFence; sColumn++ )
                    {
                        if( sdnbBTree::isNullColumn(sColumn,
                                                    ((UChar*)sRowBuf) + sOffset ) == ID_TRUE )
                        {
                            IDE_RAISE( ERR_NOTNULL_VIOLATION );
                        }
                        sOffset += sdnbBTree::getColumnLength(
                                            SDNB_GET_COLLENINFO( &(sColumn->mKeyColumn) ),
                                            ((UChar*)sRowBuf) + sOffset,
                                            &sColumnHeaderLen,
                                            &sColumnValueLen,
                                            NULL );
                    }
                }

                IDE_TEST_RAISE( mIndex->mInsert( mStatistics, /* idvSQL* */
                                                 mTrans,
                                                 mTable,
                                                 mIndex,
                                                 sInfiniteSCN,
                                                 (SChar*)sRowBuf,
                                                 NULL,
                                                 sIsUniqueIndex,
                                                 sStmtSCN,
                                                 &sRowSID,
                                                 NULL,
                                                 ID_ULONG_MAX,/* aInsertWaitTime */
                                                 ID_FALSE )   /* aForbiddenToRetry */
                                != IDE_SUCCESS, ERR_INSERT );
            }
        }

        /* BUG-23388: Top Down Build시 Meta Page가 Scan될 경우 SLatch를 풀지
         *            않습니다. */
        sIsPageLatchReleased = ID_TRUE;
        IDE_TEST( sdbBufferMgr::releasePage( mStatistics,
                                             sPage )
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(mStatistics) != IDE_SUCCESS);

    }

    sState = 0;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );

    return;

    /* PRIMARY KEY는 NULL값을 색인할수 없다 */
    IDE_EXCEPTION( ERR_NOTNULL_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_NULL_VIOLATION ) );
        mErrorCode = ideGetErrorCode();
        ideCopyErrorInfo( &mErrorMgr, ideGetErrorMgr() );
    }
    IDE_EXCEPTION( ERR_INSERT );
    {
        mErrorCode = ideGetErrorCode();
        ideCopyErrorInfo( &mErrorMgr, ideGetErrorMgr() );
    }
    IDE_EXCEPTION_END;

    (*mContinue) = ID_FALSE;

    IDE_PUSH();
    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( mStatistics,
                                               sPage )
                    == IDE_SUCCESS );
    }

    if( sState != 0 )
    {
        IDE_ASSERT( sMPRMgr.destroy() == IDE_SUCCESS );
    }

    IDE_ASSERT( smLayerCallback::abortTrans( mTrans )
                == IDE_SUCCESS );
    IDE_ASSERT( smLayerCallback::freeTrans( mTrans )
                == IDE_SUCCESS );
    mTrans = NULL;
    IDE_POP();

    return;

}

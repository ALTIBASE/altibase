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
 * $Id: sdptbGroup.cpp 27228 2008-07-23 17:36:52Z newdaily $
 *
 * Bitmap based TBS에서 Global Group( Space Header) 과 Local Group을
 * 관리하기 위한 함수들이다.
 **********************************************************************/

#include <smErrorCode.h>
#include <sdptb.h>
#include <sctTableSpaceMgr.h>
#include <sdp.h>
#include <sdpReq.h>

/***********************************************************************
 * Description:
 *  테이블스페이스 노드에 Space Cache를 할당하고 초기화한다.
 ***********************************************************************/
IDE_RC sdptbGroup::allocAndInitSpaceCache( scSpaceID         aSpaceID,
                                           smiExtMgmtType    aExtMgmtType,
                                           smiSegMgmtType    aSegMgmtType,
                                           UInt              aExtPageCount )
{
    sdptbSpaceCache     * sSpaceCache;
    UInt                  sState = 0;

    IDE_ASSERT( aExtMgmtType == SMI_EXTENT_MGMT_BITMAP_TYPE);

    /* sdptbGroup_allocAndInitSpaceCache_malloc_SpaceCache.tc */
    IDU_FIT_POINT("sdptbGroup::allocAndInitSpaceCache::malloc::SpaceCache");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SDP,
                                 ID_SIZEOF(sdptbSpaceCache),
                                 (void**)&sSpaceCache,
                                 IDU_MEM_FORCE )
              != IDE_SUCCESS );
    sState = 1;

    idlOS::memset( sSpaceCache, 0x00 , ID_SIZEOF(sdptbSpaceCache));

    /* Tablespace의 ID */
    sSpaceCache->mCommon.mSpaceID       = aSpaceID;

    /* Extent 관리 방식 설정 */
    sSpaceCache->mCommon.mExtMgmtType   = aExtMgmtType;
    sSpaceCache->mCommon.mSegMgmtType   = aSegMgmtType;

    /* Extent의 페이지 개수 설정 */
    sSpaceCache->mCommon.mPagesPerExt   = aExtPageCount;


    /* TBS 공간확장을 위한 Mutex*/
    IDE_ASSERT( sSpaceCache->mMutexForExtend.initialize(
                                        (SChar*)"FEBT_EXTEND_MUTEX",
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL ) == IDE_SUCCESS );

    // Condition Variable 초기화
    IDE_TEST_RAISE( sSpaceCache->mCondVar.initialize((SChar *)"FEBT_EXTEND_COND") != IDE_SUCCESS,
                    error_cond_init );

    sSpaceCache->mWaitThr4Extend = 0;

    /* BUG-31608 [sm-disk-page] add datafile during DML
     * TBS Add Datafile을 위한 Mutex */
    IDE_ASSERT( sSpaceCache->mMutexForAddDataFile.initialize(
                                        (SChar*)"FEBT_ADD_DATAFILE_MUTEX",
                                        IDU_MUTEX_KIND_POSIX,
                                        IDV_WAIT_INDEX_NULL ) == IDE_SUCCESS );

    sSpaceCache->mArrIsFreeExtDir[ SDP_TSS_FREE_EXTDIR_LIST ] = ID_TRUE;
    sSpaceCache->mArrIsFreeExtDir[ SDP_UDS_FREE_EXTDIR_LIST ] = ID_TRUE;

    /* Tablespace 노드에 Space Cache 설정 */
    sddDiskMgr::setSpaceCache( aSpaceID, sSpaceCache );

    if( sctTableSpaceMgr::isTempTableSpace( aSpaceID ) == ID_TRUE )
    {
        /* Extent Pool 초기화 */
        IDE_TEST( sSpaceCache->mFreeExtPool.initialize(
                      IDU_MEM_SM_TBS_FREE_EXTENT_POOL,
                      ID_SIZEOF( scPageID ) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_init );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondInit) );
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free(sSpaceCache) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  테이블스페이스 노드에서 Space Cache를 메모리를 해제한다.
 ***********************************************************************/
IDE_RC sdptbGroup::destroySpaceCache( sctTableSpaceNode * aSpaceNode )
{
    sdptbSpaceCache  * sSpaceCache;

    /* Tablespace 노드로부터 Space Cache Ptr 반환 */
    sSpaceCache = sddDiskMgr::getSpaceCache( aSpaceNode );
    IDE_ASSERT( sSpaceCache != NULL );

    if( sctTableSpaceMgr::isTempTableSpace( aSpaceNode ) == ID_TRUE )
    {
        IDE_TEST( sSpaceCache->mFreeExtPool.destroy() != IDE_SUCCESS );
    }
    IDE_TEST_RAISE( sSpaceCache->mCondVar.destroy() != IDE_SUCCESS,
                    error_cond_destroy );

    /* TBS Extend Mutex를 해제한다. */
    IDE_ASSERT( sSpaceCache->mMutexForExtend.destroy() == IDE_SUCCESS );
    /* BUG-31608 [sm-disk-page] add datafile duringDML
     * AddDataFile을 하기 위한 Mutex를 해제한다. */
    IDE_ASSERT( sSpaceCache->mMutexForAddDataFile.destroy() == IDE_SUCCESS );

    IDE_ASSERT( iduMemMgr::free( sSpaceCache ) == IDE_SUCCESS );

    /* 메모리 해제했으므로 NULL ptr 설정한다. */
    sddDiskMgr::setSpaceCache( (sddTableSpaceNode*)aSpaceNode, NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_destroy );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondDestroy));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  TBS 확장에 대한  Mutex을 획득 혹은 대기
 ***********************************************************************/
IDE_RC sdptbGroup::prepareExtendFileOrWait( idvSQL            * aStatistics,
                                            sdptbSpaceCache   * aCache,
                                            idBool            * aDoExtend )
{
    IDE_ASSERT( aCache != NULL );
    IDE_ASSERT( aDoExtend != NULL );

    lockForExtend( aStatistics, aCache );

    // Extent 진행 여부를 판단한다.
    if ( isOnExtend( aCache ) == ID_TRUE )
    {
        /* BUG-44834 특정 장비에서 sprious wakeup 현상이 발생하므로 
                     wakeup 후에도 다시 확인 하도록 while문으로 체크한다.*/
        while ( isOnExtend( aCache ) == ID_TRUE )
        {
            // ExrExt Mutex를 획득한 상태에서 대기자를 증가시킨다.
            aCache->mWaitThr4Extend++;

            IDE_TEST_RAISE( aCache->mCondVar.wait(&(aCache->mMutexForExtend))
                            != IDE_SUCCESS, error_cond_wait );

            aCache->mWaitThr4Extend--;
        }
        // 이미 Extend가 완료되었기 때문에 Extent 확장을
        // 연이어 할필요없이 가용공간 탐색을 진행한다.
        *aDoExtend = ID_FALSE;
    }
    else
    {
        // 직접 파일 확장을 수행하기 위해 OnExtend를 On시킨다.
        aCache->mOnExtend = ID_TRUE;
        *aDoExtend = ID_TRUE;
    }

    unlockForExtend( aCache );

    /* BUG-31608 [sm-disk-page] add datafile during DML */
    if ( *aDoExtend == ID_TRUE )
    {
        /* 스스로 확장을 수행하는 Transaction일 경우, ( DoExtend == true )
         * 동시성 처리를 위해 AddDataFile을 막는다. */
         lockForAddDataFile( aStatistics, aCache );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_wait );
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_ThrCondWait));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description:
 *  TBS 확장에 대한 Mutex을 해제와 함께 대기하는 트랜잭션 깨운다.
 *  (sdpst루틴 참고함)
 ***********************************************************************/
IDE_RC sdptbGroup::completeExtendFileAndWakeUp( idvSQL          * aStatistics,
                                                sdptbSpaceCache * aCache )
{
    IDE_ASSERT( aCache != NULL );
    IDE_ASSERT( isOnExtend(aCache) == ID_TRUE );

    /* BUG-31608 [sm-disk-page] add datafile during DML 
     * 확장을 완료하였기 때문에 addDataFile을 허용한다 */
    unlockForAddDataFile( aCache );

    lockForExtend( aStatistics, aCache );

    if ( aCache->mWaitThr4Extend > 0 )
    {
        // 대기 트랜잭션을 모두 깨운다.
        IDE_TEST_RAISE( aCache->mCondVar.broadcast() != IDE_SUCCESS,
                        error_cond_signal );
    }

    // Segment 확장 진행을 완료하였음을 설정한다.
    aCache->mOnExtend = ID_FALSE;

    unlockForExtend( aCache );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_cond_signal );
    {
        IDE_SET( ideSetErrorCode(smERR_FATAL_ThrCondSignal) );
    }
    IDE_EXCEPTION_END;
 
    return IDE_FAILURE;
}


/***********************************************************************
 * Description:
 *  AddDataFile을 하기 위한 Mutex를 획득하고 대기한다.
 *
 * aStatistics    - [IN] 통계정보
 * aCache         - [IN] 테이블스페이스 런타임 Cache*
 ***********************************************************************/
void sdptbGroup::prepareAddDataFile( idvSQL          * aStatistics,
                                     sdptbSpaceCache * aCache )
{
    IDE_DASSERT( aCache != NULL );

    lockForAddDataFile( aStatistics, aCache );
}
/***********************************************************************
 * Description:
 *  AddDataFile 작업을 종료하였으므로, mutex를 해제한다.
 *
 * aCache         - [IN] 테이블스페이스 런타임 Cache*
 ***********************************************************************/
void sdptbGroup::completeAddDataFile( sdptbSpaceCache * aCache )
{
    IDE_DASSERT( aCache != NULL );

    unlockForAddDataFile( aCache );
}

/***********************************************************************
 *
 * Description:  GG(Global Group) 및 LG(Local Group) header들를 생성한다.
 *
 * aStatistics    - [IN] 통계정보
 * aStartInfo     - [IN] Mtx 시작 정보
 * aSpaceID       - [IN] 테이블스페이스 ID
 * aCache         - [IN] 테이블스페이스 런타임 Cache
 * aFileAttr      - [IN] 데이타파일 속성 Array
 * aFileAttrCount - [IN] 데이타파일 속성 개수
 ***********************************************************************/
IDE_RC sdptbGroup::makeMetaHeaders( idvSQL            * aStatistics,
                                    sdrMtxStartInfo   * aStartInfo,
                                    UInt                aSpaceID,
                                    sdptbSpaceCache   * aCache,
                                    smiDataFileAttr  ** aFileAttr,
                                    UInt                aFileAttrCount )
{
    UInt            i;
    UInt            sPageCnt;  //할당을 고려할 페이지 갯수
    UInt            sLGCnt;    //local group 갯수
    sdrMtx          sMtx;
    UInt            sPagesPerExt    = aCache->mCommon.mPagesPerExt;
    UInt            sState  = 0;
    scPageID        sGGHdrPID;
    sdptbGGHdr    * sGGHdrPtr;
    UChar         * sPagePtr;
    UInt            sGGID;
    idBool          sIsExtraLG; // 모든 비트가 채워지지않은 extra LG가 존재하느냐
    sctPendingOp  * sPendingOp;

    IDE_ASSERT( aCache              != NULL );
    IDE_ASSERT( aFileAttr           != NULL );
    IDE_ASSERT( aStartInfo          != NULL );
    IDE_ASSERT( aStartInfo->mTrans  != NULL );
    IDE_ASSERT( aFileAttrCount <= SD_MAX_FID_COUNT);

    for( i = 0 ; i < aFileAttrCount ; i++ )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aStartInfo,
                                       ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        /* BUG-24369 Undo Tablespace Reset시 DataFile의 CurrSize 기준으로
         *           Reset 해야함.
         * Create Tablespace/Alter Add DataFile/Reset시에 makeMetaHeaders
         * 가 호출된다. 처음 생성하는 경우에는 mCurrSize와 mInitSize가 동일하고,
         * Reset 시에는 mCurrSize 와 mInitSize 다르기 때문에 기준으로
         * MetaHeader를 mCurrSize로 수정한다. */
        sPageCnt = aFileAttr[i]->mCurrSize;
       
        sGGID     = aFileAttr[i]->mID;
   
        IDE_ASSERT( sGGID < SD_MAX_FID_COUNT ) ; 

        //파일의 크기는 최소한 하나의 extent를 할당받을 수 있는 정도는 되야한다.
        //파일을 생성하는 중이므로 만약 요구조건이 충족되지 않으면 assert
        IDE_ASSERT( sPageCnt >=
                    (sPagesPerExt+SDPTB_GG_HDR_PAGE_CNT+ SDPTB_LG_HDR_PAGE_CNT) );

        sLGCnt = getNGroups( sPageCnt,
                             aCache ,
                             &sIsExtraLG );

        IDE_ASSERT( ( 0 < sLGCnt ) && ( sLGCnt <= SDPTB_LG_CNT_MAX ) );

        /*********************************************
         * GG header를 만든다.
         *********************************************/
        sGGHdrPID = SDPTB_GLOBAL_GROUP_HEADER_PID( sGGID ); // GGID

        IDE_TEST( sdpPhyPage::create( aStatistics,
                                      aSpaceID,
                                      sGGHdrPID,
                                      NULL,    /* Parent Info */
                                      0,       /* Page Insertable State */
                                      SDP_PAGE_FEBT_GGHDR,
                                      SM_NULL_OID, // TBS
                                      SM_NULL_INDEX_ID,
                                      &sMtx,   /* BM Create Mtx */
                                      &sMtx,   /* Init Page Mtx */
                                      &sPagePtr) 
                  != IDE_SUCCESS );

        IDE_TEST( sdpPhyPage::logAndInitLogicalHdr(
                                          (sdpPhyPageHdr  *)sPagePtr,
                                          ID_SIZEOF(sdptbGGHdr),
                                          &sMtx,
                                          (UChar**)&sGGHdrPtr )
                  != IDE_SUCCESS );

        IDE_ASSERT( sGGHdrPtr != NULL);

        /* GG header를 적절한 값으로 채운다. & LOGGING */
        IDE_TEST( logAndInitGGHdrPage( &sMtx,
                                       aSpaceID,
                                       sGGHdrPtr,
                                       sGGID,
                                       sPagesPerExt,
                                       sLGCnt,
                                       sPageCnt,
                                       sIsExtraLG )
                  != IDE_SUCCESS );

        /*********************************************
         * LG header들을 만든다.
         *********************************************/
        IDE_TEST( makeNewLGHdrs( aStatistics,
                                 &sMtx,
                                 aSpaceID,
                                 sGGHdrPtr,
                                 0,         //aStartLGID,
                                 sLGCnt,    //aLGCntOfGG,
                                 sGGID,     //aGGID
                                 sPagesPerExt,
                                 sPageCnt)  
                  != IDE_SUCCESS );

        /* To Fix BUG-23874 [AT-F5 ART] alter tablespace add datafile 에
         * 대한 복원이 안되는 것 같음.
         *
         * 생성된 파일에 대한 가용도를 SpaceNode에 반영할때는
         * 트랜잭션 Commit Pending으로 처리해야 한다. */
        IDE_TEST( sctTableSpaceMgr::addPendingOperation(
                                                  sMtx.mTrans,
                                                  aSpaceID,
                                                  ID_TRUE, /* Pending 연산 수행 시점 : Commit 시 */
                                                  SCT_POP_UPDATE_SPACECACHE,
                                                  &sPendingOp ) 
                  != IDE_SUCCESS );

        sPendingOp->mPendingOpFunc  = sdptbSpaceDDL::alterAddFileCommitPending;
        sPendingOp->mFileID         = sGGID;
        sPendingOp->mPendingOpParam = (void*)aCache;

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    } // end of for

    aCache->mGGIDHint = 0 ;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );

    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  Ondemand로 파일 크기를 늘릴때 메타헤더를 만든다
 ***********************************************************************/
IDE_RC sdptbGroup::makeMetaHeadersForAutoExtend(
                                              idvSQL           *aStatistics,
                                              sdrMtxStartInfo  *aStartInfo,
                                              UInt              aSpaceID,
                                              sdptbSpaceCache  *aCache,
                                              UInt              aNeededPageCnt )
{
    sdrMtx              sMtx;
    sddDataFileNode   * sFileNode = NULL;
    sddTableSpaceNode * sSpaceNode= NULL;
    UInt                sState=0;
    UInt                sOldLGCnt;
    UInt                sNewLGCnt;
    idBool              sDummy;
    UInt                sPageCntOld;

    IDE_ASSERT( aCache != NULL );
    IDE_ASSERT( aStartInfo!=NULL );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
                != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( aSpaceID,
                                                        (void**)&sSpaceNode )
              != IDE_SUCCESS );
    IDE_ASSERT( sSpaceNode != NULL );

    //확장이 가능한 가장작은 파일을 찾는다. (void return)
    sddDiskMgr::getExtendableSmallestFileNode( sSpaceNode,
                                               &sFileNode );
    /*
     * 만약 sFileNode가 NULL 이라면 파일확장이 불가능한 상태이다.
     */
    IDE_TEST_RAISE( sFileNode == NULL, error_not_enough_space );

    //물리적확장 이전의 페이지갯수를 저장해둔다.
    sPageCntOld = sFileNode->mCurrSize;

    sOldLGCnt = getNGroups( sFileNode->mCurrSize,
                            aCache,
                            &sDummy);

    sNewLGCnt = getNGroups( sFileNode->mCurrSize + aNeededPageCnt,
                            aCache,
                            &sDummy);

    /*
     *( aNeededPageCnt / sPagesPerExt )는 요청한 extent의 갯수이다.
     * 파일이 확장되었을때 LG 헤더들을 만들어주어야한다면 아래조건이 참이될 수
     * 있다.
     */


    if ( sOldLGCnt < sNewLGCnt )
    {
        //새롭게 만들어지는 LG의 갯수만큼 LG헤더를 만들어야한다.
        aNeededPageCnt +=  SDPTB_LG_HDR_PAGE_CNT*( sOldLGCnt - sNewLGCnt ) ;
    }

    /*
     * 이제 page layer에서 필요한 페이지의 갯수는 모두 구해졌다.
     * 그다음에는 sdd에 이 크기의 파일확장을 요청한다.
     *
     * getSmallestFileNode루틴에서 "이미"  extend mode,
     * mNextSize,  mMaxSize등을 고려하였다.그러므로 여기서는 해당파일을
     * 물리적으로 확장만하면 된다.
     */
    IDE_TEST( sddDiskMgr::extendDataFileFEBT(
                                          aStatistics,
                                          sdrMiniTrans::getTrans( &sMtx ),
                                          aSpaceID,
                                          sFileNode )
                != IDE_SUCCESS );

    /*
     * sFileNode가 성공적으로 얻어졌고 모든조건이 완벽할때 이하의 코드가 실행됨.
     * 여기서는 본격적으로 파일 크기에 따라 필요한 LG헤더를 수정하고,새롭게
     * 만든다.
     */
    IDE_TEST( resizeGG( aStatistics,
                        &sMtx,
                        aSpaceID,
                        sFileNode->mID,         //aGGID
                        sFileNode->mCurrSize )  //실제로확장된크기임
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    /*
     * 만약 확장을 했다면 cache의 FID비트를 켜준다.
     */
    if ( sFileNode->mCurrSize > sPageCntOld )
    {
        /* BUG-47666 mFreenessOfGGs에 동시성 제어가 필요합니다. */
        sdptbBit::atomicSetBit32( (UInt*)aCache->mFreenessOfGGs, sFileNode->mID );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_enough_space );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NOT_ENOUGH_SPACE,
                                  sSpaceNode->mHeader.mName ));

        /* BUG-40980 : AUTOEXTEND OFF상태에서 TBS max size에 도달하여 extend 불가능
         *             error 메시지를 altibase_sm.log에도 출력한다. */
        if ( aSpaceID != SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO )
        {
            ideLog::log( IDE_SM_0, 
                         "The tablespace does not have enough free space ( TBS Name :<%s> ).",
                         sSpaceNode->mHeader.mName );
        }
    }

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  이전페이지 갯수와 지금페이지갯수를 갖고서 resize를 수행한다.
 *  이전페이지 갯수는 인자로 받은 GG를 사용하면 쉽게 얻는다.
 * 
 *  HWM에 대한 검사가 끝난상태에서 이 함수가 호출된다.
 *  이 함수는 메타헤더들을 수정하거나 새로 만든다.
 ***********************************************************************/
IDE_RC sdptbGroup::resizeGG( idvSQL             * aStatistics,
                             sdrMtx             * aMtx,
                             scSpaceID            aSpaceID,
                             UInt                 aGGID,
                             UInt                 aNewPageCnt )
{
    UChar             * sPagePtr;
    idBool              sDummy;
    sdptbGGHdr        * sGGHdr;
    scPageID            sPID;

    IDE_ASSERT( aMtx != NULL );

    sPID = SD_CREATE_PID( aGGID, 0 );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          sPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

    sGGHdr = getGGHdr(sPagePtr);

    IDE_TEST( resizeGGCore( aStatistics,
                            aMtx,
                            aSpaceID,
                            sGGHdr,
                            aNewPageCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description:
 *  GG ptr을 받아서 처리한다.
 ***********************************************************************/
IDE_RC sdptbGroup::resizeGGCore( idvSQL         * aStatistics,
                                 sdrMtx         * aMtx,
                                 scSpaceID        aSpaceID,
                                 sdptbGGHdr     * aGGHdr,
                                 UInt             aNewPageCnt )
{
    sdptbSpaceCache   * sCache;
    UInt                sPagesPerExt;
    idBool              sHasExtraBefore = ID_FALSE;
    UInt                sExtCntOfLastLGOld;
    UInt                sExtCntOfLastLGNew;
    UInt                sOldPageCnt; //확장전의 파일크기 단위(page갯수)
    UInt                sLGCntBefore;
    UInt                sLGCntNow;
    UInt                sExtCntBefore;
    UInt                sExtCntNow;
    scPageID            sAllocLGHdrPID;
    scPageID            sDeallocLGHdrPID;
    UInt                sReducedFreeExtCnt;
    UInt                sAddedFreeExtCnt;
    UInt                sFreeInLG=0;
    UInt                sLastLGID;
    UInt                sValidBits;
    UInt                sBitIdx;
    idBool              sPartialEGNow;

    sCache = sddDiskMgr::getSpaceCache( aSpaceID );

    IDE_ASSERT( sCache != NULL );

    sPagesPerExt = sCache->mCommon.mPagesPerExt;

    /*
     *   HWM 보다 작게 파일을 줄일수는 없다.
     *   앞에서 체크했음. 여기서는 assert만.
     */
    IDE_ASSERT( SD_MAKE_FPID( aGGHdr->mHWM ) < aNewPageCnt )

    sOldPageCnt = aGGHdr->mTotalPages;

    sLGCntBefore = getNGroups( sOldPageCnt,
                               sCache,
                               &sHasExtraBefore );

    //당연히 똑같아야만 함.
    IDE_ASSERT( sLGCntBefore == aGGHdr->mLGCnt );

    sLGCntNow = getNGroups( aNewPageCnt,
                            sCache,
                            &sPartialEGNow );

    sExtCntBefore = getExtentCntByPageCnt( sCache,
                                           sOldPageCnt );

    sExtCntNow = getExtentCntByPageCnt( sCache,
                                        aNewPageCnt );

    // just for assert
    sExtCntOfLastLGOld = getExtCntOfLastLG( sOldPageCnt, sPagesPerExt );

    IDE_ASSERT( ( 0 < sExtCntOfLastLGOld ) &&
                ( sExtCntOfLastLGOld  <= sdptbGroup::nBitsPerLG() ) );

    sExtCntOfLastLGNew = getExtCntOfLastLG( aNewPageCnt, sPagesPerExt );

    IDE_ASSERT( ( 0 < sExtCntOfLastLGNew ) &&
                ( sExtCntOfLastLGNew <= sdptbGroup::nBitsPerLG() ) );

    IDE_TEST_CONT( sOldPageCnt == aNewPageCnt, return_anyway );

    /* BUG-29763 DB FileSize 변경시 Size가 작아서 Extent의 변경이 없을 경우
     * 비정상 종료합니다.
     *
     * Extent 개수에 변경이 없는 경우 LGHdr도 변경될 필요가 없다. 
     * GGHdr의 mTotalPages 만 변경해주면 된다. */
    IDE_TEST_CONT( sExtCntBefore == sExtCntNow, skip_resize_lghdr );

    IDE_ASSERT( sOldPageCnt != aNewPageCnt );

    /*  이전페이지갯수가 바뀐파일의 페이지갯수보다 많다면 축소이다.
     *  이때수정해야하는 메타페이지는 GG와 축소된파일의 가장 마지막 LG이다.
     *
     *  이전페이지갯수가 바뀐파일의 페이지갯수보다 작다면 확장이다.
     *  이때수정해야하는 메타페이지는 GG와 이전파일크기에서의 마지막 LG 그리고
     *  확장되면서 새롭게 만들어진 LG들이다.
     *
     * [축소및 확장시 수정을 고려해야하는 GG필드]
     *   - mLGCnt
     *   - mTotalPages
     *   - alloc LG들에대한 LG free info 에서 mLGFreeness.mFreeExts &
     *                                        mLGFreeness.mBits
     *   - dealloc LG들에대한 LG free info 에서 mLGFreeness.mBits
     *      (dealloc LG들의 mFreeExts는 변화가 없기 때문에 호출할 필요없음.)
     *
     * 참고사항:
     *   - 현재 축소중이고 mbits중 mLGCnt갯수만큼만 사용되어지기 때문에
     *     mLGFreeness의 mBits는 수정할필요가 없다고 생각할수있으나..
     *     NO. 수정해야할수도 있다. 만약 마지막 LG에서 HWM앞부분은 전부
     *     사용중이었을때 HWM로 잘려질경우이다.
     *
     *   - dealloc LG들에대한 logAndModifyFreeExtsOfGG는 호출불필요.
     *     어차피 freeExts값에는 영향을 미치지 않으므로
     */
    if ( sOldPageCnt > aNewPageCnt ) // 축소
    {
        IDE_ASSERT( sLGCntBefore >= sLGCntNow );

        /****** GG header log&수정. *************************************/

        /*
         * (기존 ext갯수 - 바뀐 ext갯수)  만큼 GG의 free에서 빼준다.
         * 줄어든 free extent의 갯수를 계산한다.
         * HWM이후는 모두 free이므로. 단순하게 계산이 가능하다.
         */
        sReducedFreeExtCnt = (sExtCntBefore - sExtCntNow) ;

        //GG hdr에서 alloc LG에 대한  Free를 변경시킨다.
        IDE_TEST( logAndModifyFreeExtsOfGG( aMtx,
                                            aGGHdr,
                                            -sReducedFreeExtCnt )
                  != IDE_SUCCESS );

        sLastLGID = sLGCntNow -1;

        //축소가 되어서 기존의 마지막 LG의 모든 extent가 잘려질때에는
        //resizeLGHdr을 호출할 필요가 없다.

        if ( sPartialEGNow == ID_TRUE )
        {
            sAllocLGHdrPID = getAllocLGHdrPID( aGGHdr, sLastLGID );

            sDeallocLGHdrPID = getDeallocLGHdrPID( aGGHdr, sLastLGID);

            IDE_TEST( resizeLGHdr( aStatistics,
                                   aMtx,
                                   aSpaceID,
                                   sExtCntOfLastLGNew, //수정된 Ext Cnt
                                   sAllocLGHdrPID,
                                   sDeallocLGHdrPID,
                                   &sFreeInLG ) != IDE_SUCCESS );


            /*  (HWM바로 다음 페이지로부터의)
             * 축소로 인하여 LG hdr에 free가 없다면
             * GG의 mLGFreeness를 수정해야한다
             */
            if ( sFreeInLG == 0 )
            {

                IDE_TEST( logAndSetLGFNBitsOfGG( aMtx,
                                                 aGGHdr,
                                                 sLastLGID,
                                                 0 ) //해당비트를 0으로 세트한다
                          != IDE_SUCCESS );

            }
            else /* sFreeInLG  != 0 */
            {
                /* nothing to do */
            }
        }
        else
        {
            /*
             * nothing to do
             * partial EG가 없다면, LG도 수정할 필요가 없고 EG의 LGFN도
             * 수정불필요하다.
             */
        }


    }
    else         // if ( sOldPageCnt < aNewPageCnt )    //확장
    {
        //LG갯수가 더 작아질수는 없다. 지금은 확장중이므로.
        IDE_ASSERT( sLGCntBefore <= sLGCntNow );

        //확장인데 LGCnt가 같다는것은 기존에 extra LG가 있었다는 뜻이다.
        if ( sLGCntBefore == sLGCntNow )
        {
          IDE_ASSERT ( sHasExtraBefore == ID_TRUE );// MUST!!!
        }

        if ( sHasExtraBefore == ID_TRUE )
        {
            sLastLGID = aGGHdr->mLGCnt -1;  //기존의 마지막 LGID

            //기존에 extra LG가 존재했다면 지금 확장중이므로 그 LG를 수정한다.
            sAllocLGHdrPID = getAllocLGHdrPID(aGGHdr, sLastLGID);

            sDeallocLGHdrPID =getDeallocLGHdrPID( aGGHdr,sLastLGID);

            if ( sLGCntBefore == sLGCntNow )
            {
                sValidBits =  sExtCntOfLastLGNew;
            }
            else    //if ( sLGCntBefore < sLGCntNow )
            {
                sValidBits = sdptbGroup::nBitsPerLG();
            }

            IDE_TEST( resizeLGHdr( aStatistics,
                                   aMtx,
                                   aSpaceID,
                                   sValidBits,
                                   sAllocLGHdrPID,
                                   sDeallocLGHdrPID,
                                   &sFreeInLG )
                        != IDE_SUCCESS );
            /*
             * 만약 확장에 성공했다면~~~
             * GG에서 LGFreeness비트를 켜주어야 한다.
             */
            if ( sFreeInLG > 0 )
            {

                sBitIdx = SDPTB_GET_LGID_BY_PID( sAllocLGHdrPID,
                                                 sPagesPerExt );

                IDE_TEST( logAndSetLGFNBitsOfGG( aMtx,
                                                 aGGHdr,
                                                 sBitIdx,
                                                 1 ) //해당LG에서 할당받도록~
                            != IDE_SUCCESS );
            }
            else
            {
                // sFreeInLG가 0인 경우는 발생하지 않음.( 확장중이므로)
            }
        }

        //확장이기는 하나 그 변화가 기존의 마지막 LG로 끝난다면 이 루프에
        //들어갈 필요가 없음.
        if ( sLGCntBefore < sLGCntNow )
        {
            IDE_TEST( makeNewLGHdrs( aStatistics,
                                     aMtx,
                                     aSpaceID,
                                     aGGHdr,
                                     aGGHdr->mLGCnt,  //aStartLGID,
                                     sLGCntNow,       //aLGCntOfGG,
                                     aGGHdr->mGGID,
                                     sPagesPerExt,
                                     aNewPageCnt) != IDE_SUCCESS );

        }

        sAddedFreeExtCnt = sExtCntNow - sExtCntBefore;

        //GG hdr에서 alloc LG에 대한  Free를 변경시킨다.
        IDE_TEST( logAndModifyFreeExtsOfGG( aMtx,
                                            aGGHdr,
                                            sAddedFreeExtCnt )
                  != IDE_SUCCESS );

    }

    IDE_EXCEPTION_CONT( skip_resize_lghdr );

    IDE_TEST( logAndSetPagesOfGG( aMtx,
                                  aGGHdr,
                                  aNewPageCnt )
              != IDE_SUCCESS );

    if ( sLGCntBefore != sLGCntNow )
    {
        IDE_TEST( logAndSetLGCntOfGG( aMtx,
                                      aGGHdr,
                                      sLGCntNow )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_EXCEPTION_CONT( return_anyway );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description:
 *  새로운 LG header를 만든다.
 *  aStartLGID 로부터 aEndLGID를  만든다.
 * 
 *  aStartLGID : 생성할 LG ID의 시작
 *  aLGCntOfGG : GG에서의 LG count (즉, GG의 mLGCnt에 들어가는 값임)
 ***********************************************************************/
IDE_RC sdptbGroup::makeNewLGHdrs( idvSQL          * aStatistics,
                                  sdrMtx          * aMtx,
                                  UInt              aSpaceID,
                                  sdptbGGHdr      * aGGHdrPtr,
                                  UInt              aStartLGID,
                                  UInt              aLGCntOfGG,
                                  sdFileID          aFID,
                                  UInt              aPagesPerExt,
                                  UInt              aPageCntOfGG )
{
    UInt                    sLGID;
    UInt                    sValidBits;

    IDE_ASSERT( aMtx != NULL);

    sLGID = aStartLGID;

    for(  ; sLGID < aLGCntOfGG ; sLGID++ )
    {

        if ( sLGID < ( aLGCntOfGG - 1 ) )
        {
            sValidBits = sdptbGroup::nBitsPerLG();
        }
        else //마지막 LG group이라면.
        {
            sValidBits = getExtCntOfLastLG( aPageCntOfGG, aPagesPerExt );
        }

        IDE_TEST( logAndInitLGHdrPage( aStatistics,
                                       aMtx,
                                       aSpaceID,
                                       aGGHdrPtr,
                                       aFID,
                                       sLGID,
                                       SDPTB_ALLOC_LG,
                                       sValidBits,
                                       aPagesPerExt )
                  != IDE_SUCCESS );

        IDE_TEST( logAndInitLGHdrPage( aStatistics,
                                       aMtx,
                                       aSpaceID,
                                       aGGHdrPtr,
                                       aFID,
                                       sLGID,
                                       SDPTB_DEALLOC_LG,
                                       sValidBits,
                                       aPagesPerExt )
                  != IDE_SUCCESS );

        if ( sValidBits > 0 )
        {

            IDE_TEST( logAndSetLGFNBitsOfGG( aMtx,
                                             aGGHdrPtr,
                                             sLGID,  //bit index~
                                             1 ) //해당LG에서 할당받도록1세트
                      != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  LG hdeaer 하나를 수정하는 함수
 *  확장과 축소 모두에 사용되어질수 있다.
 * 
 *  resizeLG를 할때 LG에서 수정할 필드
 * 
 *  for alloc LG
 *     - mValidBits
 *     - mFree
 *     - mBitmap : 0으로 채움
 * 
 *  for dealloc LG
 *     - mValidBits
 *     //- mFree 불필요. free가 늘지 않으므로.
 *     - mBitmap : 1로 채움
 ***********************************************************************/
IDE_RC sdptbGroup::resizeLGHdr( idvSQL     * aStatistics,
                                sdrMtx     * aMtx,
                                scSpaceID    aSpaceID,
                                ULong        aValidBitsNew, //수정된 Ext Cnt
                                scPageID     aAllocLGPID,
                                scPageID     aDeallocLGPID,
                                UInt       * aFreeInLG )
{
    UChar              * sPagePtr;
    idBool               sDummy;
    sdptbLGHdr         * sLGHdr;
    UInt                 sFreeExts;
    SInt                 sDifference;
    sdptbData4InitLGHdr  sData4InitLGHdr;

    IDE_ASSERT( ( 0 < aValidBitsNew ) &&
                ( aValidBitsNew  <= sdptbGroup::nBitsPerLG() ) );

    /*
     * alloc LG header관련 log&수정.
     */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aAllocLGPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

    sLGHdr = getLGHdr( sPagePtr );

    sDifference = aValidBitsNew - sLGHdr->mValidBits;

    //파일크기가 바뀌지 않았는데 호출되었다면 에러.
    IDE_ASSERT( sDifference != 0 );

    /*
     * sFreeExts = 기존 free +  ( 지금 LG의 ext갯수 - 기존 ext갯수 )
     */
    sFreeExts = sLGHdr->mFree + sDifference;

    /*
     * 마지막 LG가 전부할당되어져 있고, HWM가 그 LG 마지막 page를
     * 가르키고 있을때  sFreeExts는  0이 될수도 있다.
     */
    IDE_ASSERT( sFreeExts <= sdptbGroup::nBitsPerLG() );

    IDE_TEST( logAndSetFreeOfLG( aMtx,
                                 sLGHdr,
                                 sFreeExts ) != IDE_SUCCESS );

     /*
      * 만약 확장이라면 비트멥을 변경한다.
      */
    if ( sDifference > 0 ) //확장이라는 뜻
    {
        sData4InitLGHdr.mBitVal   = 0;
        sData4InitLGHdr.mStartIdx = sLGHdr->mValidBits; //기존 LG의 valid bits를 사용
        sData4InitLGHdr.mCount    = sDifference;

        IDE_ASSERT( (sLGHdr->mValidBits + sDifference ) <=
                       sdptbGroup::nBitsPerLG());

        initBitmapOfLG( (UChar*)sLGHdr,
                        0,                  //aBitVal,
                        sLGHdr->mValidBits, //aStartIdx,
                        sDifference);

        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sLGHdr,
                                             &sData4InitLGHdr,
                                             ID_SIZEOF( sData4InitLGHdr ),
                                             SDR_SDPTB_INIT_LGHDR_PAGE )
                  != IDE_SUCCESS );
    }

    //위에서 writeLogRec을 한다음에 ValidBits을 바꿀것.
    IDE_TEST( logAndSetValidBitsOfLG( aMtx,
                                      sLGHdr,
                                      aValidBitsNew )
              != IDE_SUCCESS );

    /*
     * output값은 당연히 alloc LG의 free이어야 한다.
     */

    *aFreeInLG = sLGHdr->mFree;

    /*
     * dealloc LG header log&수정.
     */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aDeallocLGPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sPagePtr,
                                          &sDummy,
                                          NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

    sLGHdr = getLGHdr(sPagePtr);

     /*
      * 만약 확장이라면 비트맵을 변경한다.
      */
    if ( sDifference > 0  ) //확장이라는 뜻
    {
        sData4InitLGHdr.mBitVal   = 1;
        sData4InitLGHdr.mStartIdx = sLGHdr->mValidBits; //기존 LG의 valid bits를 사용
        sData4InitLGHdr.mCount    = sDifference;

        IDE_ASSERT( (sLGHdr->mValidBits + sDifference ) <=
                      sdptbGroup::nBitsPerLG());

        initBitmapOfLG( (UChar*)sLGHdr,
                        1,                  //aBitVal,
                        sLGHdr->mValidBits, //aStartIdx,
                        sDifference);

        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sLGHdr,
                                             &sData4InitLGHdr,
                                             ID_SIZEOF( sData4InitLGHdr ),
                                             SDR_SDPTB_INIT_LGHDR_PAGE )
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( logAndSetValidBitsOfLG( aMtx,
                                      sLGHdr,
                                      aValidBitsNew ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  한개의 GG에 들어가는 LG의 갯수를 산정한다.
 *  파일의 크기를 page단위로 받아서 몇개의 LG를 만들수있는지 계산한다
 * 
 *  aPageCnt   : page단위의 파일크기 (당연히 GG header까지 포함된크기임)
 *  aIsExtraLG : 부분적으로 사용하는 LG가 존재하는냐
 ***********************************************************************/
UInt sdptbGroup::getNGroups( ULong            aPageCnt,
                             sdptbSpaceCache *aCache,
                             idBool          *aIsExtraLG )
{
    UInt sNum;
    ULong sLGSizeInByte;   //단위는 byte
    ULong sSizeInByte;     //단위는 byte
    ULong sRest;

    IDE_ASSERT( aCache != NULL);
    IDE_ASSERT( aIsExtraLG != NULL);

    aPageCnt -=  SDPTB_GG_HDR_PAGE_CNT;  //GG header를 제외하고 계산해야만 한다

    sSizeInByte = aPageCnt * SD_PAGE_SIZE;

    sLGSizeInByte = SDPTB_PAGES_PER_LG(aCache->mCommon.mPagesPerExt)
                      * SD_PAGE_SIZE ;

    sNum = sSizeInByte / sLGSizeInByte;

    sRest = sSizeInByte % sLGSizeInByte;

    //%연산으로 나머지가 있다고 해서 무조건 LG갯수를 증가시키면 안된다.
    //왜냐하면 극단적인 예로 한바이트가 남았을수도 있기 때문이다.
    if ( (sRest/SD_PAGE_SIZE) >=
                (SDPTB_LG_HDR_PAGE_CNT + aCache->mCommon.mPagesPerExt) )
    {
        sNum++;
        *aIsExtraLG = ID_TRUE ;
    }
    else
    {
        *aIsExtraLG = ID_FALSE ;
    }
    return sNum;
}

/***********************************************************************
 * Description:
 *  GG헤더에대한 로그처리를 한다.
 *
 *  이함수는 TBS를 만들때 호출되는 sdptbGroup::makeMetaHeaders에 의해서만 
 *  호출된다. 
 ***********************************************************************/
IDE_RC sdptbGroup::logAndInitGGHdrPage( sdrMtx        * aMtx,
                                        UInt            aSpaceID,
                                        sdptbGGHdr    * aGGHdrPtr,
                                        sdptbGGID       aGGID,
                                        UInt            aPagesPerExt,
                                        UInt            aLGCnt,
                                        UInt            aPageCnt,
                                        idBool          aIsExtraLG )
{
    ULong       sLongVal;
    UInt        sVal;

    IDE_ASSERT( aMtx        != NULL);
    IDE_ASSERT( aGGHdrPtr   != NULL);

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aGGHdrPtr->mGGID,
                                         &aGGID,
                                         ID_SIZEOF( aGGID ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aGGHdrPtr->mPagesPerExt,
                                         &aPagesPerExt,
                                         ID_SIZEOF( aPagesPerExt ) )
              != IDE_SUCCESS );

    sVal =  SD_CREATE_PID( aGGID,0);

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aGGHdrPtr->mHWM,
                                         (scPageID*)&sVal,
                                         ID_SIZEOF( aGGHdrPtr->mHWM ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aGGHdrPtr->mLGCnt,
                                         &aLGCnt,
                                         ID_SIZEOF( aLGCnt ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aGGHdrPtr->mTotalPages,
                                         &aPageCnt,
                                         ID_SIZEOF( aPageCnt ) )
              != IDE_SUCCESS );

    sVal = SDPTB_ALLOC_LG_IDX_0;

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aGGHdrPtr->mAllocLGIdx,
                                         &sVal,
                                         ID_SIZEOF( sVal ) )
              != IDE_SUCCESS );

    if ( aIsExtraLG == ID_TRUE ) //일부 비트만 사용중인  LG가 존재하는가
    {
        //모든비트를 쓰는 LG의 비트수 + 마지막 LG의 비트수
        sLongVal = ( sdptbGroup::nBitsPerLG() * (aLGCnt-1)) +
                   getExtCntOfLastLG( aPageCnt, aPagesPerExt );

    }
    else
    {
        //모든비트를 쓰는 LG의 비트수
        sLongVal = sdptbGroup::nBitsPerLG() * aLGCnt;
    }

    /*
     * GG에서 alloc LG에대한 필드 로깅.
     */
    IDE_TEST( sdrMiniTrans::writeNBytes(
                                   aMtx,
                                   (UChar*)&aGGHdrPtr->mLGFreeness[0].mFreeExts,
                                   &sLongVal,
                                   ID_SIZEOF( sLongVal ) ) != IDE_SUCCESS );

    /*
     * alloc LG 로깅.
     *
     */
    /*
     * LGFreeness에대한 세팅은 여기서 하지않고 makeNewLGHdrs에서 수행한다.
     * 여기서는 일단 0으로만 세트한다.
     */
    sLongVal = 0;
    IDE_TEST( sdrMiniTrans::writeNBytes(
                                    aMtx,
                                    (UChar*)&aGGHdrPtr->mLGFreeness[0].mBits[0],
                                    &sLongVal,
                                    ID_SIZEOF( sLongVal ) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                                aMtx,
                                (UChar*)&aGGHdrPtr->mLGFreeness[0].mBits[1],
                                &sLongVal,
                                ID_SIZEOF( sLongVal ) ) != IDE_SUCCESS );

    /*
     * GG에서 dealloc LG에대한 필드 로깅.
     * dealloc LG는 처음만들때 free extent가 없으므로 모두 0으로 세팅하면 된다.
     */
    IDE_TEST( sdrMiniTrans::writeNBytes(
                                  aMtx,
                                  (UChar*)&aGGHdrPtr->mLGFreeness[1].mFreeExts,
                                  &sLongVal,
                                  ID_SIZEOF( sLongVal ) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                                  aMtx,
                                  (UChar*)&aGGHdrPtr->mLGFreeness[1].mBits[0],
                                  &sLongVal,
                                  ID_SIZEOF( sLongVal ) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                                  aMtx,
                                  (UChar*)&aGGHdrPtr->mLGFreeness[1].mBits[1],
                                  &sLongVal,
                                  ID_SIZEOF( sLongVal ) ) != IDE_SUCCESS );

    /* PROJ-1704 Disk MVCC Renewal
     * Undo TBS의 첫번째 FGH에는 Free Extent Dir List가 존재한다.
     */
    if ( ( sctTableSpaceMgr::isUndoTableSpace( aSpaceID ) ) &&
         ( aGGID == 0 ) )
    {
        IDE_TEST( sdpSglPIDList::initList( 
                                &(aGGHdrPtr->mArrFreeExtDirList[ SDP_TSS_FREE_EXTDIR_LIST ] ),
                                aMtx )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSglPIDList::initList( 
                                &(aGGHdrPtr->mArrFreeExtDirList[ SDP_UDS_FREE_EXTDIR_LIST ] ),
                                aMtx ) 
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  mLGFreeness의 mBits에서 aBitIdx번째 인덱스값을 aVal로 수정하는 함수이다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetLGFNBitsOfGG( sdrMtx           * aMtx,
                                          sdptbGGHdr       * aGGHdr,
                                          ULong              aBitIdx,
                                          UInt               aVal )
{
    ULong        sResult;

    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );
    IDE_ASSERT( aBitIdx < SDPTB_BITS_PER_ULONG*2 );
    IDE_ASSERT( (aVal == 1) || (aVal == 0) );

    if ( aVal == 1 )
    {
        sdptbBit::setBit( aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits, aBitIdx );
    }
    else
    {
        sdptbBit::clearBit( aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits, aBitIdx );
    }


    if ( aBitIdx < SDPTB_BITS_PER_ULONG )
    {
        sResult = aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits[0];

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&(aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits[0]),
                                             &sResult,
                                             ID_SIZEOF( sResult ) )
                  != IDE_SUCCESS );
    }
    else  //if ( aBitIdx >= SDPTB_BITS_PER_ULONG )
    {
        sResult = aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits[1];

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&(aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits[1]),
                                             &sResult,
                                             ID_SIZEOF( sResult ) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  LG헤더에대한 로그처리를 한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndInitLGHdrPage( idvSQL          *  aStatistics,
                                        sdrMtx          *  aMtx,
                                        UInt               aSpaceID,
                                        sdptbGGHdr      *  aGGHdrPtr,
                                        sdFileID           aFID,
                                        UInt               aLGID,
                                        sdptbLGType        aLGType,
                                        UInt               aValidBits,
                                        UInt               aPagesPerExt )

{
    UInt                    sLGHdrPID;
    UChar                  *sPagePtr;
    sdptbLGHdr             *sLGHdrPtr;
    UInt                    sStartPID;
    UInt                    sZero=0;
    sdptbData4InitLGHdr     sData4InitLGHdr;
    UInt                    sWhich;

    IDE_ASSERT( aMtx != NULL);
    IDE_ASSERT( aLGType < SDPTB_LGTYPE_CNT );

    if ( aLGType == SDPTB_ALLOC_LG )
    {
        sWhich = sdptbGroup::getAllocLGIdx(aGGHdrPtr);
    }
    else       // if ( aLGType == SDPTB_DEALLOC_LG )
    {
        sWhich = sdptbGroup::getDeallocLGIdx(aGGHdrPtr);
    }

    sLGHdrPID = SDPTB_LG_HDR_PID_FROM_LGID( aFID,
                                            aLGID,
                                            sWhich,
                                            aPagesPerExt );

    IDE_TEST( sdpPhyPage::create( aStatistics,
                                  aSpaceID,
                                  sLGHdrPID,
                                  NULL,    /* Parent Info */
                                  0,       /* Page Insertable State */
                                  SDP_PAGE_FEBT_LGHDR,
                                  SM_NULL_OID, // TBS
                                  SM_NULL_INDEX_ID,
                                  aMtx,    /* Create Page Mtx */
                                  aMtx,    /* Init Page Mtx */
                                  &sPagePtr) != IDE_SUCCESS );

    IDE_ASSERT( sPagePtr !=NULL );

    IDE_TEST( sdpPhyPage::logAndInitLogicalHdr( (sdpPhyPageHdr  *)sPagePtr,
                                                ID_SIZEOF(sdptbLGHdr),
                                                aMtx,
                                                (UChar**)&sLGHdrPtr )
              != IDE_SUCCESS );

    sStartPID = SD_CREATE_PID( aFID,
                               SDPTB_EXTENT_START_FPID_FROM_LGID( aLGID ,aPagesPerExt) );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sLGHdrPtr->mLGID,
                                         &aLGID,
                                         ID_SIZEOF( aLGID ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sLGHdrPtr->mStartPID,
                                         &sStartPID,
                                         ID_SIZEOF( sStartPID ))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sLGHdrPtr->mValidBits,
                                         &aValidBits,
                                         ID_SIZEOF( aValidBits ) )
              != IDE_SUCCESS );

    if ( aLGType == SDPTB_ALLOC_LG )
    {
        //alloc page LG인 경우 free의 초기값은 당연히 mValidBits와 같다.
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sLGHdrPtr->mFree,
                                             &aValidBits,
                                             ID_SIZEOF( aValidBits ) )
                  != IDE_SUCCESS );

        initBitmapOfLG( (UChar*)sLGHdrPtr,
                        0,         //aBitVal,
                        0,         //aStartIdx,
                        aValidBits);

        /*
         * alloc LG 인경우는 valid한 비트맵을 0으로 만드는 루틴이 필요하다.
         */
        sData4InitLGHdr.mBitVal   = 0;
        sData4InitLGHdr.mStartIdx = 0;
        sData4InitLGHdr.mCount    = aValidBits;
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sLGHdrPtr,
                                             &sData4InitLGHdr,
                                             ID_SIZEOF(sData4InitLGHdr),
                                             SDR_SDPTB_INIT_LGHDR_PAGE )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sLGHdrPtr->mHint,
                                             &sZero,
                                             ID_SIZEOF(sZero) ) != IDE_SUCCESS );
    }
    else          // if ( aLGType == SDPTB_DEALLOC_LG )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sLGHdrPtr->mFree,
                                             &sZero,
                                             ID_SIZEOF( sZero ) )
                  != IDE_SUCCESS );

        initBitmapOfLG( (UChar*)sLGHdrPtr,
                        1,         //aBitVal,
                        0,         //aStartIdx,
                        aValidBits);
        /*
         * dealloc LG 인경우는 valid한 비트맵을 1로 만드는 루틴이 필요하다.
         */
        sData4InitLGHdr.mBitVal   = 1;
        sData4InitLGHdr.mStartIdx = 0;
        sData4InitLGHdr.mCount    = aValidBits;
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sLGHdrPtr,
                                             &sData4InitLGHdr,
                                             ID_SIZEOF(sData4InitLGHdr),
                                             SDR_SDPTB_INIT_LGHDR_PAGE )
          != IDE_SUCCESS );

        /*
         * Dealloc LG 인경우는  hint를 (최대 extent id +1 )로 설정하도록 한다.
         */
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sLGHdrPtr->mHint,
                                             &aValidBits,
                                             ID_SIZEOF(aValidBits) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  TBS의 모든파일을 돌면서 사용가능한 file을 찾아서 space cache를 생성하고
 *  avail 비트를 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::doRefineSpaceCacheCore( sddTableSpaceNode * aSpaceNode )

{
    UInt                sMaxGGID=0;
    idBool              sIsFirstLoop= ID_TRUE;
    UInt                i;
    sddDataFileNode   * sFileNode;
    sdptbSpaceCache   * sCache;

    UChar             * sPagePtr;
    sdptbGGHdr        * sGGHdrPtr;
    idBool              sDummy;
    sctTableSpaceNode * sSpaceNode = (sctTableSpaceNode *)aSpaceNode;
    idBool              sInvalidTBS;

    IDE_ASSERT( aSpaceNode != NULL );
    IDE_ASSERT( aSpaceNode->mExtMgmtType == SMI_EXTENT_MGMT_BITMAP_TYPE );

    sInvalidTBS = sctTableSpaceMgr::hasState( sSpaceNode->mID,
                                              SCT_SS_INVALID_DISK_TBS );

    if ( ( sctTableSpaceMgr::isDiskTableSpace(sSpaceNode->mID) == ID_TRUE ) &&
         ( sInvalidTBS ==  ID_FALSE ) )
    {
        sCache = (sdptbSpaceCache *)aSpaceNode->mSpaceCache;
        IDE_ASSERT( sCache != NULL );
        
        idlOS::memset( sCache->mFreenessOfGGs, 0x00 , ID_SIZEOF(sCache->mFreenessOfGGs) );
        
        //TBS의 모든 파일을 돌면서 사용가능한 File을 찾은다음 비트세트.
        for ( i=0 ; i < aSpaceNode->mNewFileID ; i++ )
        {
            sFileNode = aSpaceNode->mFileNodeArr[i] ;

            if ( sFileNode != NULL )
            {
                if ( SMI_FILE_STATE_IS_NOT_DROPPED( sFileNode->mState ) )
                {
                    /* 만약에 GGid에 free extent가 없다면 세트하지 말것 */
                    IDE_TEST( sdbBufferMgr::fixPageByPID( NULL,
                                                          sSpaceNode->mID,
                                                          SDPTB_GET_GGHDR_PID_BY_FID( sFileNode->mID),
                                                          (UChar**)&sPagePtr,
                                                          &sDummy )
                              != IDE_SUCCESS );

                    sGGHdrPtr = getGGHdr(sPagePtr);
                    
                    sMaxGGID = i ;

                    if ( sGGHdrPtr->mLGFreeness[ sGGHdrPtr->mAllocLGIdx ].mFreeExts == 0 )
                    {
                        IDE_TEST( sdbBufferMgr::unfixPage( NULL,
                                                           sPagePtr )
                                  != IDE_SUCCESS );

                        continue;
                    }
                }
                else
                {
                    // BUG-27329 CodeSonar::Uninitialized Variable (2)
                    continue;
                }

                // PROJ-1704 Disk MVCC Renewal
                // Undo TBS에서 필요한 Free ExtDir List 정보를 확인한다
                if ( ( sctTableSpaceMgr::isUndoTableSpace(sSpaceNode->mID) )&& 
                     ( sFileNode->mID == 0 ) )
                {
                    if ( sdpSglPIDList::getNodeCnt( 
                            &(sGGHdrPtr->mArrFreeExtDirList[ SDP_TSS_FREE_EXTDIR_LIST ])) > 0 )
                    {
                       sCache->mArrIsFreeExtDir[ SDP_TSS_FREE_EXTDIR_LIST ] = ID_TRUE;    
                    }
                    else
                    {
                        /* nothing to do */
                    }

                    if ( sdpSglPIDList::getNodeCnt( 
                            &(sGGHdrPtr->mArrFreeExtDirList[ SDP_UDS_FREE_EXTDIR_LIST ])) > 0 )
                    {
                        sCache->mArrIsFreeExtDir[ SDP_UDS_FREE_EXTDIR_LIST ] = ID_TRUE;    
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }

                IDE_TEST( sdbBufferMgr::unfixPage( NULL,
                                                   sPagePtr )
                          != IDE_SUCCESS );

                /* BUG-47666 server start시점이라 동시성 제어를 할 필요는 없으나
                 *           atomic 함수의 정상 동작 여부를 확인하기 위해 사용됨*/
                IDE_ASSERT( sdptbBit::getBit( sCache->mFreenessOfGGs, i ) == ID_FALSE );

                sdptbBit::atomicSetBit32( (UInt*)sCache->mFreenessOfGGs, i );

                IDE_ASSERT( sdptbBit::getBit( sCache->mFreenessOfGGs, i ) == ID_TRUE );

                //할당을 고려할 첫번째 GG ID를 세트한다.
                if ( sIsFirstLoop == ID_TRUE )
                {
                    sCache->mGGIDHint = i;
                    sIsFirstLoop = ID_FALSE;
                }
                else
                {
                    /* nothing to do */
                }
            }
        }

        //할당가능한 가장큰 GG ID를 세트한다.
        //가장마지막 파일이 DROP된경우도 있을것이므로 이렇게 한다.
        sCache->mMaxGGID = sMaxGGID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/////////////////////////////////////////////////////////////
//     GG       Logging 관련 함수들.(구현)
/////////////////////////////////////////////////////////////

/***********************************************************************
 * Description:
 *  GG의 HWM 필드를 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetHWMOfGG( sdrMtx      * aMtx,
                                     sdptbGGHdr  * aGGHdr,
                                     UInt          aHWM )
{
    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aGGHdr->mHWM),
                                         &aHWM,
                                         ID_SIZEOF(aHWM) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mLGCnt 필드를 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetLGCntOfGG( sdrMtx      * aMtx,
                                       sdptbGGHdr  * aGGHdr,
                                       UInt          aGroups )
{
    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aGGHdr->mLGCnt),
                                         &aGroups,
                                         ID_SIZEOF(aGroups) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mTotalPages 필드를 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetPagesOfGG( sdrMtx     *  aMtx,
                                       sdptbGGHdr  * aGGHdr,
                                       UInt          aPages )
{
    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aGGHdr->mTotalPages),
                                         &aPages,
                                         ID_SIZEOF(aPages) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mAllocLGIdx 필드를 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetLGTypeOfGG( sdrMtx       * aMtx,
                                        sdptbGGHdr   * aGGHdr,
                                        UInt           aLGType )
{
    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aGGHdr->mAllocLGIdx),
                                         &aLGType,
                                         ID_SIZEOF(aLGType) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mFree 필드를 로깅하고 세트한다.
 *  aVal만큼 값을 변화시킨다. aVal 은 양수음수 모두 가능하다.
 *  이함수는 지금 활성화되어있는 alloc LG의 값을 변화시킨다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndModifyFreeExtsOfGG( sdrMtx          * aMtx,
                                             sdptbGGHdr      * aGGHdr,
                                             SInt              aVal )
{
    ULong sResult;

    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    sResult = aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mFreeExts + aVal;

    IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&(aGGHdr->mLGFreeness[ aGGHdr->mAllocLGIdx ].mFreeExts),
                      (void*)&sResult,
                      ID_SIZEOF( sResult ) ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mFree 필드를 로깅하고 세트한다.
 *  aVal만큼 값을 변화시킨다. aVal 은 양수음수 모두 가능하다.
 *
 *  이함수는 LG type을 인자로 받아서 처리하는것을 빼고는
 *  logAndModifyFreeExtsOfGG과 같다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndModifyFreeExtsOfGGByLGType( sdrMtx       * aMtx,
                                                     sdptbGGHdr   * aGGHdr,
                                                     UInt           aLGType,
                                                     SInt           aVal )
{
    ULong sResult;

    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    sResult = aGGHdr->mLGFreeness[aLGType].mFreeExts + aVal;

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                         (UChar*)&(aGGHdr->mLGFreeness[aLGType].mFreeExts),
                         (void*)&sResult,
                         ID_SIZEOF( sResult ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mLGFreeness에서 ULong배열의 0번째요소를 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetLowBitsOfGG( sdrMtx     * aMtx,
                                         sdptbGGHdr * aGGHdr,
                                         ULong        aBits )
{
    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits[0]),
                  &aBits,
                  ID_SIZEOF(aBits) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  GG의 mLGFreeness에서 ULong배열의 1번째요소를 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetHighBitsOfGG( sdrMtx           * aMtx,
                                          sdptbGGHdr       * aGGHdr,
                                          ULong              aBits )
{
    IDE_ASSERT( aGGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                 aMtx,
                 (UChar*)&(aGGHdr->mLGFreeness[aGGHdr->mAllocLGIdx].mBits[1]),
                 &aBits,
                 ID_SIZEOF(aBits) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/////////////////////////////////////////////////////////////
//     LG       Logging 관련 함수들.(구현)
/////////////////////////////////////////////////////////////

/***********************************************************************
 * Description:
 *  LG의 mStartPID(최초 extent의 pid값)을 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetStartPIDOfLG( sdrMtx     * aMtx,
                                          sdptbLGHdr * aLGHdr,
                                          UInt         aStartPID )
{
    IDE_ASSERT( aLGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aLGHdr->mStartPID),
                                         &aStartPID,
                                         ID_SIZEOF(aStartPID) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  LG의 mHint값을 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetHintOfLG( sdrMtx        * aMtx,
                                      sdptbLGHdr    * aLGHdr,
                                      UInt            aHint )
{
    IDE_ASSERT( aLGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aLGHdr->mHint),
                                         &aHint,
                                         ID_SIZEOF(aHint) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  LG의 mValidBits값을 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetValidBitsOfLG( sdrMtx       * aMtx,
                                           sdptbLGHdr   * aLGHdr,
                                           UInt           aValidBits )
{
    IDE_ASSERT( aLGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aLGHdr->mValidBits),
                                         &aValidBits,
                                         ID_SIZEOF(aValidBits) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  LG의 mFree값을 로깅하고 세트한다.
 ***********************************************************************/
IDE_RC sdptbGroup::logAndSetFreeOfLG( sdrMtx            * aMtx,
                                      sdptbLGHdr        * aLGHdr,
                                      UInt                aFree )
{
    IDE_ASSERT( aLGHdr != NULL );
    IDE_ASSERT( aMtx   != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(aLGHdr->mFree),
                                         &aFree,
                                         ID_SIZEOF(aFree) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  [ redo routine ]
 *  GG header에 대한 redo 루틴이다.
 *  현재 구조에서 GG header에대한 초기화는 writeNBytes를 사용해서 구현하였으므로
 *  이 함수는 사실상 불필요하다. 하지만, 만약을 대비하여 만들어 놓았다.
 ***********************************************************************/
/* BUG-46036 codesonar warning 제거
IDE_RC sdptbGroup::initGG( sdrMtx      * aMtx,
                           UChar       * aPagePtr )
{
    aPagePtr = aPagePtr;
    aMtx = aMtx;

    return IDE_SUCCESS;
}
*/

/***********************************************************************
 * Description:
 *  [ redo routine ]
 *  LG header에 대한 redo 루틴이다.
 * 
 *  aPagePtr  : LG header에 대한 포인터가 넘어온다.
 *  aBitVal   : 초기화할 비트값
 *              (0이면 0으로 초기화하고 1이면 모든비트를 1로 초기화한다.)
 *  aStartIdx : mBitmap으로부터의 비트를 세트해야하는 시작지점인덱스
 *  aCount    : 세트할 비트의 갯수
 ***********************************************************************/
void sdptbGroup::initBitmapOfLG( UChar       * aPagePtr,
                                 UChar         aBitVal,
                                 UInt          aStartIdx,
                                 UInt          aCount )
{
    sdptbLGHdr              *    sLGHdrPtr;
    UInt                         sNrBytes;
    UInt                         sRest;
    UChar                        sLastByte;
    UChar                   *    sStartBitmap =NULL;
    UChar                        sByteForMemset;

    //extent가  하나도 없는 LG를 만드는것은 불가
    IDE_ASSERT( aCount != 0 );
    IDE_ASSERT( (aBitVal == 0) || (aBitVal == 1) );
    IDE_ASSERT( aStartIdx < sdptbGroup::nBitsPerLG() );

    //aLGHdrPtr에는 LG header의 포인터가 넘어온다.
    sLGHdrPtr = (sdptbLGHdr *)aPagePtr;

    /*
     * free가 없다 할지라도 비트열 검색이 끝날수있도록 하는 트릭이다.
     * mBitmap에 해당되는 비트 바로다음을 0으로 채움으로서
     * 비트열을 검색할때마다 index번호를 검사하는 비용을 줄인다.
     *
     * 이동작은 매번할필요는 없고 처음 LG헤더가 만드어질때만 하면 된다.
     */
    if ( aStartIdx == 0 )
    {
        idlOS::memset( (UChar*)sLGHdrPtr->mBitmap + getLenBitmapOfLG() + 1,
                       SDPTB_LG_BITMAP_MAGIC ,
                       ID_SIZEOF(UChar) );

    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT( *((UChar*)sLGHdrPtr->mBitmap + getLenBitmapOfLG() + 1 )
                == SDPTB_LG_BITMAP_MAGIC );

    /*
     * aStartIdx가 byte에 정렬되서 넘어온다는 보장은 없다.
     * 일단, 바이트에 정렬될때까지 짜투리비트를 세트한다.
     */
    while( ((aStartIdx % SDPTB_BITS_PER_BYTE) != 0) && (aCount > 0) )
    {
        if ( aBitVal == 1 )
        {
            sdptbBit::setBit( sLGHdrPtr->mBitmap, aStartIdx );
        }
        else
        {
            sdptbBit::clearBit( sLGHdrPtr->mBitmap, aStartIdx );
        }

        aStartIdx++;
        aCount--;
    }

    sNrBytes = aCount / SDPTB_BITS_PER_BYTE;
    sRest = aCount % SDPTB_BITS_PER_BYTE;

    sStartBitmap = (UChar*)sLGHdrPtr->mBitmap + aStartIdx/SDPTB_BITS_PER_BYTE ;

    /*
     * 이제 start index가 byte에 정렬되었다.
     * 만약 바이트단위로 세트할 것이 하나라도 있다면 세트한다.
     */
    if ( sNrBytes > 0 )
    {

        if ( aBitVal == 1 )
        {
            sByteForMemset = 0xFF;
        }
        else
        {
            sByteForMemset = 0x00;
        }

        idlOS::memset( sStartBitmap, sByteForMemset, sNrBytes );
    }
    else
    {
        /* nothing to do */
    }

    /*
     * 마지막으로 짜투리 비트들이 존재한다면 그 비트들을 세트한다.
     */
    if ( sRest != 0 )
    {
        sLastByte = 0xFF;

        if ( aBitVal == 1 )
        {
            sLastByte = ~(sLastByte << sRest);
        }
        else
        {
            sLastByte = sLastByte << sRest;
        }

        idlOS::memset( sStartBitmap + sNrBytes,
                       sLastByte,
                       ID_SIZEOF(UChar) );
    }
    else
    {
        /* nothing to do */
    }

    IDE_ASSERT( *((UChar*)sLGHdrPtr->mBitmap + getLenBitmapOfLG() + 1 )
                == SDPTB_LG_BITMAP_MAGIC );
}

/***********************************************************************
 * Description:
 *  TBS에서 실제로 사용되어지는 할당된(free가 아닌)페이지갯수를 계산한다.
 *
 *  aStatistics     - [IN] 통계정보
 *  aSpaceID        - [IN] Space ID
 *  aAllocPageCount - [OUT] 할당된 페이지 갯수를 리턴
 **********************************************************************/
IDE_RC sdptbGroup::getAllocPageCount( idvSQL            * aStatistics,
                                      sddTableSpaceNode * aSpaceNode,
                                      ULong             * aAllocPageCount )
{
    UChar               * sPagePtr;
    UInt                  sState=0;
    sdptbGGHdr          * sGGHdr;
    UInt                  sGGID;
    sddDataFileNode     * sFileNode;
    UInt                  sGGPID;
    UInt                  sFreePages=0; //TBS에서 사용하지 않는 free page갯수
    UInt                  sTotalPages=0;//TBS의 전체 page갯수 

    IDE_ASSERT( aAllocPageCount != NULL );

    /* BUG-41895 When selecting the v$tablespaces table, 
     * the server doesn`t check the tablespace`s state whether that is discarded or not */
    if (((aSpaceNode->mHeader.mState & SMI_TBS_DROPPED)   != SMI_TBS_DROPPED) &&
        ((aSpaceNode->mHeader.mState & SMI_TBS_DISCARDED) != SMI_TBS_DISCARDED))
    {
        for ( sGGID=0 ; sGGID < aSpaceNode->mNewFileID ; sGGID++ )
        {
            sFileNode = aSpaceNode->mFileNodeArr[ sGGID ] ;

            if ( sFileNode == NULL )
            {
                continue;
            }

            // Page를 가져오는 동안 FileNode가 Drop되면 안된다.
            sddDiskMgr::lockGlobalPageCountCheckMutex( aStatistics );
            sState = 1;
            /* BUG-33919 - [SM] when selecting the X$TABLESPACES and adding datafiles
             *             in tablespace are executed concurrently, server can be aborted
             * 삭제된 datafile 뿐만 아니라 생성중인 datafile 도 사용하면 안된다.
             * 왜나하면, 아직 생성중이기 때문에 GG 헤더가 초기화되지
             * 않았을 수 있기 때문이다. */
            if ( SMI_FILE_STATE_IS_DROPPED( sFileNode->mState ) ||
                 SMI_FILE_STATE_IS_CREATING( sFileNode->mState ) )
            {
                sState = 0;
                sddDiskMgr::unlockGlobalPageCountCheckMutex();
                continue;
            }

            sGGPID = SDPTB_GLOBAL_GROUP_HEADER_PID( sGGID ); 

            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics, 
                                                  aSpaceNode->mHeader.mID,
                                                  sGGPID,
                                                  SDB_S_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  NULL, /* sdrMtx */
                                                  &sPagePtr,
                                                  NULL, /* aTrySuccess */
                                                  NULL  /* IsCorruptPage */ )
                      != IDE_SUCCESS );
            sState = 2;

            sGGHdr = sdptbGroup::getGGHdr(sPagePtr);

            sTotalPages += sGGHdr->mTotalPages;

            /* 
             * 각 FSB에서 Alloc EG와 Free EG의 free extent갯수를 더하면
             * 해당 TBS의 free extent갯수가 얻어진다.
             */
            sFreePages += sGGHdr->mLGFreeness[0].mFreeExts * sGGHdr->mPagesPerExt;
            sFreePages += sGGHdr->mLGFreeness[1].mFreeExts * sGGHdr->mPagesPerExt;

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                      != IDE_SUCCESS );

            sState = 0;
            sddDiskMgr::unlockGlobalPageCountCheckMutex();
        }

        *aAllocPageCount = sTotalPages - sFreePages;
    }
    else
    {
        *aAllocPageCount = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sPagePtr ) == IDE_SUCCESS );
        case 1:
            sddDiskMgr::unlockGlobalPageCountCheckMutex();
            break;
        default:
            break;
    }
    return IDE_FAILURE;
}

/***********************************************************************
 * Description:
 *  Tablespace에서 Cached된 Free Extent갯수를 Return한다.
 *
 * aSpaceID - [IN] Space ID
 *
 **********************************************************************/
ULong sdptbGroup::getCachedFreeExtCount( sddTableSpaceNode* aSpaceNode )
{
    sdptbSpaceCache  *  sSpaceCache;
    ULong               sPageCount = 0;

    if( sctTableSpaceMgr::isTempTableSpace( aSpaceNode ) == ID_TRUE )
    {
        sctTableSpaceMgr::lockSpaceNode( NULL /* idvSQL* */,
                                         aSpaceNode );

        if (((aSpaceNode->mHeader.mState & SMI_TBS_DROPPED)   != SMI_TBS_DROPPED ) &&
            ((aSpaceNode->mHeader.mState & SMI_TBS_DISCARDED) != SMI_TBS_DISCARDED ))
        {
            sSpaceCache = (sdptbSpaceCache*)aSpaceNode->mSpaceCache;

            if( sSpaceCache != NULL )
            {
                sPageCount = sSpaceCache->mFreeExtPool.getTotItemCnt();
            }
        }

        sctTableSpaceMgr::unlockSpaceNode( aSpaceNode );
    }

    return sPageCount;
}



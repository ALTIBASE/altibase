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
 * $Id: sdcTSSegment.cpp 91026 2021-06-21 00:23:00Z emlee $
 *
 * Description :
 *
 * 본 파일은 Transaction Status Slot Segment 구현파일입니다.
 *
 **********************************************************************/

# include <smu.h>
# include <sdr.h>
# include <sdp.h>
# include <sdb.h>
# include <smx.h>

# include <sdcDef.h>
# include <sdcTSSegment.h>
# include <sdcTSSlot.h>
# include <sdcTXSegFreeList.h>
# include <sdcReq.h>

/***********************************************************************
 *
 * Description : TSS Segment 할당
 *
 * Undo Tablespace의 TSS Segment를 생성하고,
 * 0번페이지의 TSS Segment 정보를 저장한다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::create( idvSQL          * aStatistics,
                             sdrMtxStartInfo * aStartInfo,
                             scPageID        * aTSSegPID )
{
    sdrMtx            sMtx;
    sdpSegMgmtOp    * sSegMgmtOP;
    UInt              sState = 0;
    sdpSegmentDesc    sTssSegDesc;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aTSSegPID  != NULL );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sdpSegDescMgr::setDefaultSegAttr(
                                  &(sTssSegDesc.mSegHandle.mSegAttr),
                                  SDP_SEG_TYPE_TSS );
    sdpSegDescMgr::setDefaultSegStoAttr(
                                  &(sTssSegDesc.mSegHandle.mSegStoAttr) );

    IDE_TEST( sdpSegDescMgr::initSegDesc(
                                  &sTssSegDesc,
                                  SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                  SD_NULL_RID,           /* Before create a segment. */
                                  SDP_SEG_TYPE_TSS,
                                  SM_NULL_OID,
                                  SM_NULL_INDEX_ID ) != IDE_SUCCESS );

    IDE_ASSERT( sTssSegDesc.mSegMgmtType ==
                sdpTableSpace::getSegMgmtType( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO ) );

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( &sTssSegDesc );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mCreateSegment( aStatistics,
                                          &sMtx,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          SDP_SEG_TYPE_TSS,
                                          &sTssSegDesc.mSegHandle )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    *aTSSegPID  = sdpSegDescMgr::getSegPID( &sTssSegDesc );

    IDE_TEST( sdpSegDescMgr::destSegDesc( &sTssSegDesc ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    *aTSSegPID = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : TSS 세그먼트 초기화
 *
 * 시스템 구동시 undo tablespace meta page(0번) 페이지를 판독하여
 * tss segment 정보를 sdcTSSegment에 설정한다.
 *
 * aStatistics  - [IN] 통계정보
 * aEntry       - [IN] 자신이 소속된 트랜잭션 세그먼트 엔트리 포인터
 * aTSSegPID    - [IN] TSS 세그먼트 헤더 페이지 ID
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::initialize( idvSQL        * aStatistics,
                                 sdcTXSegEntry * aEntry,
                                 scPageID        aTSSegPID )
{
    sdpSegInfo     sSegInfo;
    sdpSegMgmtOp * sSegMgmtOp;

    IDE_ASSERT( aEntry != NULL );
    IDE_ASSERT( aEntry->mEntryIdx < SDP_MAX_UDSEG_PID_CNT );

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(
                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    /* Undo Tablespace는 시스템에 의해서 자동으로 관리되므로
     * Segment Storage Parameter 들을 모두 무시한다 */
    sdpSegDescMgr::setDefaultSegAttr(
            sdpSegDescMgr::getSegAttr( &mTSSegDesc),
            SDP_SEG_TYPE_TSS );

    sdpSegDescMgr::setDefaultSegStoAttr(
            sdpSegDescMgr::getSegStoAttr( &mTSSegDesc ));

    IDE_TEST( sdpSegDescMgr::initSegDesc( &mTSSegDesc,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          aTSSegPID,
                                          SDP_SEG_TYPE_TSS,
                                          SM_NULL_OID, 
                                          SM_NULL_INDEX_ID )
            != IDE_SUCCESS );

    mSlotSize       = ID_SIZEOF(sdcTSS);
    mAlignSlotSize  = idlOS::align8(mSlotSize);
    mEntryPtr       = aEntry;
    /* BUG-40014 [PROJ-2506] Insure++ Warning
     * - 멤버 변수의 초기화가 필요합니다.
     */
    mCurAllocExtRID = ID_ULONG_MAX;
    mFstPIDOfCurAllocExt = ID_UINT_MAX;
    mCurAllocPID = ID_UINT_MAX;

    IDE_TEST( sSegMgmtOp->mGetSegInfo( 
                           aStatistics,
                           SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                           aTSSegPID,
                           NULL, /* aTableHeader */
                           &sSegInfo ) != IDE_SUCCESS );

    setCurAllocInfo( sSegInfo.mFstExtRID,
                     sSegInfo.mFstPIDOfLstAllocExt,
                     SD_NULL_PID );  // CurAllocPID 초기화

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : TSS 세그먼트 기술자 해제
 *
 * 세그먼트 기술자에는 런타임 메모리 자료구조가 할당되어
 * 있기 때문에 메모리 해제한다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::destroy()
{
    IDE_ASSERT( sdpSegDescMgr::destSegDesc( &mTSSegDesc )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}


/***********************************************************************
 * Description : Transaction Status Slot 할당
 *
 * pending 연산을 수행해야하는 트랜잭션이나 dml을 수행하는 트랜잭션에
 * 대해서만 tsslot을 할당한다.
 *
 * *********************************************************************/
IDE_RC sdcTSSegment::bindTSS( idvSQL          * aStatistics,
                              sdrMtxStartInfo * aStartInfo )
{
    UInt                 sState = 0;
    sdrMtx               sMtx;
    UChar              * sSlotPtr = NULL;
    UShort               sSlotOffset;
    UShort               sSlotSize;
    sdrSavePoint         sSvp;
    sdSID                sSlotSID;
    sdpPhyPageHdr      * sPageHdr;
    UChar              * sNewCurPagePtr;
    UChar              * sSlotDirPtr;
    idBool               sIsPageCreate;
    void               * sTrans;
    smTID                sTransID;
    sdcTSSPageCntlHdr  * sCntlHdr;
    smSCN                sFstDskViewSCN;
    scSlotNum            sNewSlotNum;

    IDE_ASSERT( aStartInfo != NULL );

    sTrans         = aStartInfo->mTrans;
    sTransID       = smxTrans::getTransID( sTrans );
    sFstDskViewSCN = smxTrans::getFstDskViewSCN( sTrans );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS );

    sPageHdr         = NULL;
    sIsPageCreate    = ID_FALSE;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    if ( mCurAllocPID != SD_NULL_PID )
    {
        sdrMiniTrans::setSavePoint( &sMtx, &sSvp );

        IDE_TEST( getCurPID4Update( aStatistics,
                                    &sMtx,
                                    &sNewCurPagePtr ) != IDE_SUCCESS );

        sPageHdr = sdpPhyPage::getHdr( sNewCurPagePtr );

        if ( sdpPhyPage::canAllocSlot( sPageHdr,
                                       mAlignSlotSize,
                                       ID_TRUE, /* aIsNeedSlotEntry */
                                       SDP_8BYTE_ALIGN_SIZE )
             != ID_TRUE ) // can't alloc
        {
            sdrMiniTrans::releaseLatchToSP( &sMtx, &sSvp );

            sIsPageCreate = ID_TRUE;
        }
        else
        {
            sIsPageCreate = ID_FALSE;
        }
    }
    else
    {
        sIsPageCreate = ID_TRUE;
    }

    if ( sIsPageCreate == ID_TRUE )
    {
        IDE_TEST( allocNewPage( aStatistics,
                                &sMtx,
                                sTransID,
                                sFstDskViewSCN,
                                &sNewCurPagePtr ) != IDE_SUCCESS );
    }

    sPageHdr = sdpPhyPage::getHdr( sNewCurPagePtr );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sNewCurPagePtr);
    sNewSlotNum = sdpSlotDirectory::getCount(sSlotDirPtr);

    IDE_TEST( sdpPhyPage::allocSlot( sPageHdr,
                                     sNewSlotNum,
                                     (UShort)mAlignSlotSize,
                                     ID_TRUE, // aNeedSlotEntry
                                     &sSlotSize,
                                     &sSlotPtr,
                                     &sSlotOffset,
                                     SDP_8BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );
    sState = 2;

    sCntlHdr = (sdcTSSPageCntlHdr*)
                sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sPageHdr );

    sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sNewSlotNum );

    IDE_TEST( sdcTSSlot::logAndInit( &sMtx,
                                     (sdcTSS*)sSlotPtr,
                                     sSlotSID,
                                     mCurAllocExtRID,
                                     sTransID,
                                     mEntryPtr->mEntryIdx ) 
              != IDE_SUCCESS );

    // To fix BUG-23687
    if( SM_SCN_IS_LT( &sFstDskViewSCN, &sCntlHdr->mFstDskViewSCN ) )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( &sMtx,
                                             (UChar*)&sCntlHdr->mTransID,
                                             (void*)&sTransID,
                                             ID_SIZEOF( sTransID ) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes( &sMtx,
                                             (UChar*)&sCntlHdr->mFstDskViewSCN,
                                             (void*)&sFstDskViewSCN,
                                             ID_SIZEOF( smSCN ) )
                  != IDE_SUCCESS );
    }

    ((smxTrans*)sTrans)->setTSSAllocPos( mCurAllocExtRID, sSlotSID );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx,
                                    SMR_CT_END, // aContType
                                    NULL,       // aEndLSN
                                    SMR_RT_WITHMEM ) // aRedoType
              != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 2:
            (void) sdpPhyPage::freeSlot( sPageHdr,
                                         sSlotPtr,
                                         mAlignSlotSize,
                                         ID_TRUE, // aIsFreeSlotEntry
                                         SDP_8BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );

        default:
            break;
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_TSS );

    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : TSS Page의 CntlHdr 초기화 및 로깅
 *
 * TSS Page Control Header에 최근 사용한 트랜잭션의 정보를 저장하여
 * TSS Page가 재사용되었는지 판단할 수 있게 한다.
 *
 * aStatistics    - [IN] 통계정보
 * aPagePtr       - [IN] 할당한 페이지 포인터
 * aTransID       - [IN] 페이지를 할당한 트랜잭션 ID
 * aFstDskViewSCN - [IN] 페이지를 할당한 트랜잭션의 Begin SCN
 *
 **********************************************************************/
IDE_RC sdcTSSegment::logAndInitPage( sdrMtx      * aMtx,
                                     UChar       * aPagePtr,
                                     smTID         aTransID,
                                     smSCN         aFstDskViewSCN )
{
    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aPagePtr != NULL );

    initPage( aPagePtr, aTransID, aFstDskViewSCN );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aPagePtr,
                                         NULL, /* aValue */
                                         ID_SIZEOF( aTransID ) +
                                         ID_SIZEOF( smSCN ),
                                         SDR_SDC_INIT_TSS_PAGE )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aTransID,
                                   ID_SIZEOF( aTransID ) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &aFstDskViewSCN,
                                   ID_SIZEOF( smSCN ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : TSS를 할당한 ExtDir의 CntlHdr에 SCN을 설정
 *
 * unbind TSS 과정에서 CommitSCN 혹은 AbortSCN 을 ExtDir 페이지에 기록한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] 테이블스페이스 ID
 * aSegPID     - [IN] TSS의 Segment PID
 * aFstExtRID  - [IN] 트랜잭션이 사용한 첫번째 ExtRID
 * aCSCNorASCN - [IN] 설정할 CommitSCN 혹은 AbortSCN
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::markSCN4ReCycle( idvSQL   * aStatistics,
                                      scSpaceID  aSpaceID,
                                      scPageID   aSegPID,
                                      sdRID      aFstExtRID,
                                      smSCN    * aCSCNorASCN )
{
    sdpSegMgmtOp   * sSegMgmtOp;

    IDE_ASSERT( aSegPID     != SD_NULL_PID );
    IDE_ASSERT( aFstExtRID  != SD_NULL_RID );
    IDE_ASSERT( aCSCNorASCN != NULL );
    IDE_ASSERT( SM_SCN_IS_NOT_INFINITE( *aCSCNorASCN ) );
    IDE_ASSERT( SM_SCN_IS_NOT_INIT( *aCSCNorASCN ) );

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( &mTSSegDesc );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST( sSegMgmtOp->mMarkSCN4ReCycle( aStatistics,
                                            aSpaceID,
                                            aSegPID,
                                            getSegHandle(),
                                            aFstExtRID,
                                            aFstExtRID,
                                            aCSCNorASCN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Unbinding TSS For Commit Trans
 *
 * 디스크 갱신 트랜잭션에 대한 Commit 작업을 수행한다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::unbindTSS4Commit( idvSQL  * aStatistics,
                                       sdSID     aTSSlotSID,
                                       smSCN   * aCommitSCN )
{
    sdrMtx      sMtx;
    sdcTSS    * sTSS;
    idBool      sTrySuccess;
    UInt        sState = 0;
    UInt        sLogSize;
    sdcTSState  sTSState = SDC_TSS_STATE_COMMIT;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DUMMY )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdbBufferMgr::getPageBySID( aStatistics,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          aTSSlotSID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          &sMtx,
                                          (UChar**)&sTSS,
                                          &sTrySuccess ) != IDE_SUCCESS );

    IDE_ASSERT( sdpPhyPage::getPageType( sdpPhyPage::getHdr((UChar*)sTSS) )
                == SDP_PAGE_TSS );

    sLogSize = ID_SIZEOF(smSCN) +
               ID_SIZEOF(sdcTSState);

    sdcTSSlot::unbind( sTSS, aCommitSCN, sTSState );

    IDE_TEST( sdrMiniTrans::writeLogRec(
                                   &sMtx,
                                   (UChar*)sTSS,
                                   NULL,
                                   sLogSize,
                                   SDR_SDC_UNBIND_TSS ) 
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   aCommitSCN,
                                   ID_SIZEOF(smSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   &sTSState,
                                   ID_SIZEOF(sdcTSState) )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Unbinding TSS For Transaction Abort
 *
 * 디스크 갱신 트랜잭션에 대한 Abort 작업을 수행한다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::unbindTSS4Abort( idvSQL  * aStatistics,
                                      sdrMtx  * aMtx,
                                      sdSID     aTSSlotSID,
                                      smSCN   * aInitSCN )
{
    sdcTSS             * sTSSlotPtr;
    idBool               sTrySuccess;
    UInt                 sLogSize;
    sdcTSState           sState = SDC_TSS_STATE_ABORT;

    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aInitSCN != NULL );
    IDE_ASSERT( aTSSlotSID != SD_NULL_SID );

    IDE_TEST( sdbBufferMgr::getPageBySID(
                  aStatistics,
                  SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                  aTSSlotSID,
                  SDB_X_LATCH,
                  SDB_WAIT_NORMAL,
                  aMtx,
                  (UChar**)&sTSSlotPtr,
                  &sTrySuccess ) != IDE_SUCCESS );

    sLogSize = ID_SIZEOF(smSCN) +
               ID_SIZEOF(sdcTSState);

    sdcTSSlot::unbind( sTSSlotPtr,
                       aInitSCN /* aSCN */,
                       SDC_TSS_STATE_ABORT );

    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)sTSSlotPtr,
                  NULL,
                  sLogSize,
                  SDR_SDC_UNBIND_TSS ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   aInitSCN,
                                   ID_SIZEOF(smSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   &sState,
                                   ID_SIZEOF(sdcTSState) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : TSS에 대한 재사용여부와 CommitSCN 반환
 *
 * 커밋된 TSS에 대해서는 특정시점이 지나면 TSS는 재사용이 가능하다. 재사용이 되지 않은경우
 * 다른 트랜잭션이 TSS를 방문하여 CommitSCN을 할당하다고 할때, 정확한 값을 얻을 수 있지만,
 * 재사용된 경우에는 Unbound CommitSCN(0x0000000000000000)을 얻을 수 밖에 없다.
 * Unbound CommitSCN은 RowPiece가 어떠한 Statment도 모두 볼수 있거나
 * 갱신할 수 있도록 해준다.
 * 또한, 아직 커밋되지 않은 트랜잭션의 TSS라면 CommitSCN은 없으므로, TSS을 소유한
 * 트랜잭션 BeginSCN을 반환한다.
 *
 * aStatistics     - [IN]  통계정보
 * aTrans          - [IN]  트랜잭션 정보 
 * aTSSlotRID      - [IN]  CTS 혹은 RowEx에 기록된 TSS의 RID
 * aFstDskViewSCN  - [IN]  CTS 혹은 RowEx에 기록된 트랜잭션의 FstDskViewSCN ( 이거나 CommitSCN 인데 commit 이 기록되었으면 여기 안옴 )
 * aStmtViewSCN    - [IN]  올바른 버전의 row를 읽기 위한 Statement viewscn
 * aTID4Wait       - [OUT] 만약 TSS를 소유한 TX가 Active 상태인 경우 해당 트랜잭션의 TID
 * aCommitSCN      - [OUT] 트랜잭션의 CommitSCN이나 Unbound CommitSCN 혹은 TSS를
 *                         소유한 트랜잭션의 BeginSCN일수 있다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::getCommitSCN( idvSQL      * aStatistics,
                                   void        * aTrans,
                                   sdSID         aTSSlotSID,
                                   smSCN       * aFstDskViewSCN,
                                   smSCN         aStmtViewSCN,
                                   smTID       * aTID4Wait,
                                   smSCN       * aCommitSCN )
{
    UChar               * sPagePtr    = NULL;
    sdpSlotDirHdr       * sSlotDirHdr = NULL;
    sdcTSS              * sTSS;
    sdcTSState            sTSState;
    smTID                 sTransID;
    sdcTSSPageCntlHdr   * sCntlHdr;
    sdpPageType           sPageType;
    smSCN                 sMinOldestFstViewSCN;
    smSCN                 sCommitSCN;
    idBool                sTrySuccess;
    UInt                  sState = 0;

    IDE_ASSERT( aTSSlotSID != SD_NULL_SID );
    IDE_DASSERT( ( SM_SCN_IS_VIEWSCN(*aFstDskViewSCN)   ) ||
                 ( SM_SCN_IS_COMMITSCN(*aFstDskViewSCN) ) ); //BUG-49079 validateDRDBRedo

    sTransID = SM_NULL_TID;
    SM_INIT_SCN( &sCommitSCN );

    /*
     * CTS에 바인딩된 트랜잭션의 BeginSCN과 System MinDskFstSCN을 확인해보고
     * 재사용여부를 확인한다.
     *
     * 주의할 점은 여기서 판단한다고 해서 통과해도 TSSlotRID를 getPage하기전에
     * 페이지가 다른트랜잭션에 의해서 재사용될수 있다는 것을 알아야 한다.
     * 즉, 판단시에 System Agable SCN보다 aTransFstSCN이 커서 통과해도
     * getPage하기전에 System Agable SCN이 증가하여 aTransFstSCN보다 커질수
     * 있다. 그러므로 BufferPool에서 CreatePage와 GetPage가 동일한 페이지에
     * 대해서 충돌할 수 있다. 이러한 현상은 TSS 페이지에서만 발생한다.
     *
     * 여기서 판단하는 것은 정확하게 하기 보다는 한번 더 체크해서 I/O를
     * 줄여보자는 것이다.
     */
    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    // active 트랜잭션들 중 가장 오래된 OldestDskFstViewSCN을 구함
    smxTransMgr::getMinOldestFstViewSCN( &sMinOldestFstViewSCN );

    // OldestDskFstViewSCN보다 CTS의 SCN이 작으면 이미 완료된 트랜잭션이므로
    // TSS를 볼 필요 없음.
    IDE_TEST_CONT( SM_SCN_IS_LT( aFstDskViewSCN, &sMinOldestFstViewSCN )
                    == ID_TRUE, CONT_ALREADY_FINISHED_TRANS );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          SD_MAKE_PID(aTSSlotSID),
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          NULL, /* aMtx */
                                          &sPagePtr,
                                          &sTrySuccess,
                                          NULL )
              != IDE_SUCCESS);
    sState = 1;

    /*
     * Page를 S-Latch로 획득한 상태에서 SysOldestDskFstViewSCN을 체크해본다.
     */

    // BUG-26881 잘못된 CTS stamping으로 acces할 수 없는 row를 접근함
    // active 트랜잭션들 중 가장 오래된 OldestDskFstViewSCN을 구함
    smxTransMgr::getMinOldestFstViewSCN( &sMinOldestFstViewSCN );

    // OldestDskFstViewSCN보다 CTS의 SCN이 작으면 이미 완료된 트랜잭션이므로
    // TSS를 볼 필요 없음.
    IDE_TEST_CONT( SM_SCN_IS_LT( aFstDskViewSCN, &sMinOldestFstViewSCN )
                    == ID_TRUE, CONT_ALREADY_FINISHED_TRANS );

    IDU_FIT_POINT( "sdcTSSegment::getCommitSCN::getPageByPID" );
    sPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)sPagePtr );

    IDE_TEST_CONT( sPageType != SDP_PAGE_TSS,
                    CONT_ALREADY_FINISHED_TRANS );

    /* BUG-32650 TSSlot이 이미 free되었을 수도 있습니다.
     * 만약 Slot Number가 Slot Entry Count보다 크면
     * 이미 완료된 Transaction이다. */
    sSlotDirHdr = (sdpSlotDirHdr*)sdpPhyPage::getSlotDirStartPtr( sPagePtr );

    IDE_TEST_CONT( ( SD_MAKE_SLOTNUM( aTSSlotSID )>= sSlotDirHdr->mSlotEntryCount ),
                    CONT_ALREADY_FINISHED_TRANS );

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                (UChar*)sSlotDirHdr,
                                                SD_MAKE_SLOTNUM( aTSSlotSID ),
                                                (UChar**)&sTSS )
              != IDE_SUCCESS );

    sCntlHdr = (sdcTSSPageCntlHdr*)
                                sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sTSS );

    if( SM_SCN_IS_LT( aFstDskViewSCN, &sCntlHdr->mFstDskViewSCN ) )
    {
        IDE_CONT( CONT_ALREADY_FINISHED_TRANS );
    }

    sdcTSSlot::getTransInfo( sTSS, &sTransID, &sCommitSCN );
    sTSState = sdcTSSlot::getState( sTSS );

    if ( ( sTSState != SDC_TSS_STATE_ABORT ) &&
         ( SM_SCN_IS_INFINITE( sCommitSCN ) ) &&
         ( SM_SCN_IS_NOT_INIT( aStmtViewSCN ) ) &&
         ( aTrans!= NULL ) &&
         ( ((smxTrans *)aTrans)->mIsGCTx == ID_TRUE ) )
    {
        /* BUG-48587 : waitPendingTx에서 대기할수있으므로 TSS Page Latch를 일단푼다. */
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                != IDE_SUCCESS );

        IDE_TEST( smxTrans::waitPendingTx( (smxTrans*)aTrans,
                                           (sCommitSCN | sTransID), /* aRowSCN */
                                           aStmtViewSCN )           /* aViewSCN */
                  != IDE_SUCCESS );

        /* BUG-48587 
           TSS Page Latch를 풀고 wait 하는중에 TSS가 새로운값으로 변경될수있다.
           TSS Page를 다시 잡고 새로운 값을 가져온다. */ 
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                              SD_MAKE_PID(aTSSlotSID),
                                              SDB_S_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              NULL, /* aMtx */
                                              &sPagePtr,
                                              &sTrySuccess,
                                              NULL )
                  != IDE_SUCCESS );
        sState = 1;

        smxTransMgr::getMinOldestFstViewSCN( &sMinOldestFstViewSCN );

        IDE_TEST_CONT( SM_SCN_IS_LT( aFstDskViewSCN, &sMinOldestFstViewSCN )
                       == ID_TRUE, CONT_ALREADY_FINISHED_TRANS );

        sPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)sPagePtr );

        IDE_TEST_CONT( sPageType != SDP_PAGE_TSS,
                       CONT_ALREADY_FINISHED_TRANS );

        sSlotDirHdr = (sdpSlotDirHdr *)sdpPhyPage::getSlotDirStartPtr( sPagePtr );

        IDE_TEST_CONT( ( SD_MAKE_SLOTNUM( aTSSlotSID )>= sSlotDirHdr->mSlotEntryCount ),
                       CONT_ALREADY_FINISHED_TRANS );

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( (UChar*)sSlotDirHdr,
                                                           SD_MAKE_SLOTNUM( aTSSlotSID ),
                                                           (UChar**)&sTSS )
                  != IDE_SUCCESS );

        sCntlHdr = (sdcTSSPageCntlHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sTSS );

        if ( SM_SCN_IS_LT( aFstDskViewSCN, &sCntlHdr->mFstDskViewSCN ) )
        {
            IDE_CONT( CONT_ALREADY_FINISHED_TRANS );
        }

        sdcTSSlot::getTransInfo( sTSS, &sTransID, &sCommitSCN );
        sTSState = sdcTSSlot::getState( sTSS );
    }

    // BUG-31504: During the cached row's rollback, it can be read.
    // 이미 abort된 트랜잭션의 CommitSCN은 확인할 필요가 없다.
    if ( sTSState != SDC_TSS_STATE_ABORT )
    {
        if ( SM_SCN_IS_INFINITE( sCommitSCN ) )
        {
            // [4-2] 무한대임에도 transaction이 commit된 경우가 있다.

            // 왜냐하면 commit 과정은
            // 1. system SCN 증가
            // 2. TSS에 commit SCN set

            // 그런데 2개 이상의 thread가 1,2사이에 끼어 들때
            // 즉 system SCN은 증가한 상태에서 stmt begin하고,
            // 아직 TSS commit이 안박힌 상태에서 record를 따라 TSS를 따라가면
            // 이 trans가 commit이 되었음에도 불구하고 TSS slot은 아직
            // commit 안된 상태가 될 수 있다.
            // 이로 인해 getValidVersion이 잘못된 레코드를 얻어올 수 있다.
            // 이를 방지하기 위해 A3가 하는 것과 마찬가지로
            // trans가 commit되지 않은 상태라 할지라도
            // TID를 가지고 trans의 commit SCN을 실제로 확인해 봐야 한다.
            // 이 값이 무한대가 아니면 commit된것임을 알수 있다.

            smxTrans::getTransCommitSCN( sTransID,
                                         &sCommitSCN, /* IN */
                                         &sCommitSCN  /* OUT */ );
        }
    }
    else
    {
        IDE_ASSERT( SM_SCN_IS_INFINITE(sCommitSCN) );
    }

    IDE_EXCEPTION_CONT( CONT_ALREADY_FINISHED_TRANS );

    if ( sState != 0 )
    {
        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                  != IDE_SUCCESS );
    }

    /* sCommitSCN은 Commit이 되었으면 유한한 값이고,
     * Active 상태라면 무한대값이다. */
    if ( SM_SCN_IS_COMMITSCN( sCommitSCN ) )
    {
        /* TSS 에 CommitSCN 기록할때 TransID 는 지우지 않음 (sdcTSSlot::unbind) */
        sTransID = SM_NULL_TID;
    }

    *aTID4Wait = sTransID;
    SM_SET_SCN( aCommitSCN, &sCommitSCN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics, sPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : X$TSSEG의 레코드를 구성한다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::build4SegmentPerfV( void                * aHeader,
                                         iduFixedTableMemory * aMemory )
{
    sdpSegInfo           sSegInfo;
    sdcTSSegInfo         sTSSegInfo;
    sdpSegMgmtOp       * sSegMgmtOp;

    sTSSegInfo.mSegPID          = mTSSegDesc.mSegHandle.mSegPID;
    sTSSegInfo.mTXSegID         = mEntryPtr->mEntryIdx;

    sTSSegInfo.mCurAllocExtRID  = mCurAllocExtRID;
    sTSSegInfo.mCurAllocPID     = mCurAllocPID;

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo(
                                    NULL,
                                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                    mTSSegDesc.mSegHandle.mSegPID,
                                    NULL, /* aTableHeader */
                                    &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sTSSegInfo.mSpaceID      = SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO;
    sTSSegInfo.mType         = sSegInfo.mType;
    sTSSegInfo.mState        = sSegInfo.mState;
    sTSSegInfo.mPageCntInExt = sSegInfo.mPageCntInExt;
    sTSSegInfo.mTotExtCnt    = sSegInfo.mExtCnt;
    sTSSegInfo.mTotExtDirCnt = sSegInfo.mExtDirCnt;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sTSSegInfo )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_DUMP_OBJECT );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotExistSegment ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : X$DISK_TSS_RECORDS의 레코드를 구성한다.
 *
 ***********************************************************************/
IDE_RC sdcTSSegment::build4RecordPerfV( UInt                  aSegSeq,
                                        void                * aHeader,
                                        iduFixedTableMemory * aMemory )
{
    sdRID                 sCurExtRID;
    UInt                  sState = 0;
    sdpSegMgmtOp        * sSegMgmtOp;
    sdpSegInfo            sSegInfo;
    sdpExtInfo            sExtInfo;
    scPageID              sCurPageID;
    UChar               * sCurPagePtr;
    UChar               * sSlotDirPtr;
    sdcTSS4FT             sTSS4FT;
    idBool                sIsSuccess;
    SInt                  sSlotSeq;
    SInt                  sSlotCnt;
    sdcTSS              * sTSSlotPtr;
    SChar                 sStrCSCN[ SM_SCN_STRING_LENGTH + 1 ];

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    /* 세그먼트가 리셋된 경우 mCurAllocPID가 SD_NULL_PID이다. */
    IDE_TEST_CONT( mCurAllocPID == SD_NULL_PID, CONT_NO_TSS_PAGE ); 

    //------------------------------------------
    // Get Segment Info
    //------------------------------------------
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(
                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_ASSERT( sSegMgmtOp->mGetSegInfo( NULL,
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         mTSSegDesc.mSegHandle.mSegPID,
                                         NULL, /* aTableHeader */
                                         &sSegInfo )
                == IDE_SUCCESS );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       sCurExtRID,
                                       &sExtInfo )
              != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID )
              != IDE_SUCCESS );

    sTSS4FT.mSegSeq = aSegSeq;
    sTSS4FT.mSegPID = mTSSegDesc.mSegHandle.mSegPID;

    while( sCurPageID != SD_NULL_PID )
    {
        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess ) != IDE_SUCCESS );
        sState = 1;

        IDE_ASSERT( sdpPhyPage::getHdr( sCurPagePtr )->mPageType
                    == SDP_PAGE_TSS );

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sCurPagePtr);
        sSlotCnt    = sdpSlotDirectory::getCount(sSlotDirPtr);

        for( sSlotSeq = 0 ; sSlotSeq < sSlotCnt; sSlotSeq++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                        sSlotDirPtr,
                                                        sSlotSeq,
                                                        (UChar**)&sTSSlotPtr )
                      != IDE_SUCCESS );
            sTSS4FT.mPageID = sCurPageID;
            IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr,
                                                  sSlotSeq,
                                                  &sTSS4FT.mOffset )
                      != IDE_SUCCESS );

            sTSS4FT.mNthSlot = sSlotSeq;
            sTSS4FT.mTransID = sTSSlotPtr->mTransID;

            idlOS::memset( sStrCSCN, 0x00, SM_SCN_STRING_LENGTH+1 );
            idlOS::snprintf( (SChar*)sStrCSCN, SM_SCN_STRING_LENGTH,
                             "%"ID_XINT64_FMT, sTSSlotPtr->mCommitSCN );
            sTSS4FT.mCSCN  = sStrCSCN;
            sTSS4FT.mState = sTSSlotPtr->mState;

            IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                  aMemory,
                                                  (void *) &sTSS4FT )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                             sCurPagePtr )
                  != IDE_SUCCESS );

        if ( sCurPageID == mCurAllocPID )
        {
            break;
        }

        IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                                SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                &sSegInfo,
                                                NULL,
                                                &sCurExtRID,
                                                &sExtInfo,
                                                &sCurPageID )
                  != IDE_SUCCESS );
    }

    //------------------------------------------
    // Finalize
    //------------------------------------------

    IDE_EXCEPTION_CONT( CONT_NO_TSS_PAGE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                               sCurPagePtr )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * TASK-4007 [SM]PBT를 위한 기능 추가
 * Description : page를 Dump하여 table의 ctl을 출력한다.
 *
 * BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서
 * local Array의 ptr를 반환하고 있습니다.
 * Local Array대신 OutBuf를 받아 리턴하도록 수정합니다.
 ***********************************************************************/
IDE_RC sdcTSSegment::dump( UChar *aPage ,
                           SChar *aOutBuf ,
                           UInt   aOutSize )
{
    UChar         * sSlotDirPtr;
    sdpSlotEntry  * sSlotEntry;
    sdpSlotDirHdr * sSlotDirHdr;
    SInt            sSlotSeq;
    UShort          sOffset;
    UShort          sSlotCnt;
    sdcTSS        * sTSSlotPtr;

    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( aPage );
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;
    sSlotCnt    = sSlotDirHdr->mSlotEntryCount;

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- TSS Record Begin ----------\n" );

    for( sSlotSeq = 0 ; sSlotSeq < sSlotCnt; sSlotSeq++ )
    {
        sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, sSlotSeq);

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%3"ID_UINT32_FMT"[%04x]..............................\n",
                             sSlotSeq,
                             *sSlotEntry );

        if( SDP_IS_UNUSED_SLOT_ENTRY(sSlotEntry) == ID_TRUE )
        {
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "Unused slot\n");
        }

        sOffset = SDP_GET_OFFSET( sSlotEntry );
        if( SD_PAGE_SIZE < sOffset )
        {
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "Invalid slot offset\n" );
            continue;
        }

        sTSSlotPtr = 
            (sdcTSS*)sdpPhyPage::getPagePtrFromOffset( aPage, sOffset );

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "mTransID            : %"ID_UINT32_FMT"\n"
                             "mState(Act,Com,Abo) : %"ID_UINT32_FMT"\n"
                             "mCommitSCN          : %"ID_UINT64_FMT"\n",
                             sTSSlotPtr->mTransID,
                             sTSSlotPtr->mState,
                             SM_SCN_TO_LONG( sTSSlotPtr->mCommitSCN ) );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                         "----------- TSS Record End   ----------\n" );

    return IDE_SUCCESS;
}


/******************************************************************************
 *
 * Description : 가용한 언두 페이지를 할당한다.
 *
 * 해당 세그먼트에 존재하지 않는다면, 오프라인 엔트리로부터 가용한
 * 익스텐트 Dir.를 얻는다.
 *
 * aStatistics     - [IN]  통계정보
 * aStartInfo      - [IN]  Mtx 시작정보
 * aTransID        - [IN]  트랜잭션 ID
 * aFstDskViewSCN  - [IN]  페이지를 할당한 트랜잭션의 FstDskViewSCN
 * aPhyPagePtr     - [OUT] 할당받은 페이지 포인터
 *
 ******************************************************************************/
IDE_RC sdcTSSegment::allocNewPage( idvSQL  * aStatistics,
                                   sdrMtx  * aMtx,
                                   smTID     aTransID,
                                   smSCN     aFstDskViewSCN,
                                   UChar  ** aNewPagePtr )
{
    sdRID           sNewCurAllocExtRID;
    scPageID        sNewFstPIDOfCurAllocExt;
    scPageID        sNewCurAllocPID;
    UChar         * sNewCurPagePtr;
    smSCN           sSysMinDskViewSCN;
    sdrMtxStartInfo sStartInfo;
    SInt            sLoop;
    idBool          sTrySuccess = ID_FALSE;
    sdpSegHandle  * sSegHandle  = getSegHandle();

    IDE_ASSERT( aNewPagePtr != NULL );

    sdrMiniTrans::makeStartInfo(aMtx, &sStartInfo );

    for ( sLoop=0; sLoop < 2; sLoop++ )
    {
        sNewCurPagePtr = NULL;

        if ( mTSSegDesc.mSegMgmtOp->mAllocNewPage4Append(
                                         aStatistics,
                                         aMtx,
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         sSegHandle,
                                         mCurAllocExtRID,
                                         mFstPIDOfCurAllocExt,
                                         mCurAllocPID,
                                         ID_TRUE,     /* aIsLogging */
                                         SDP_PAGE_TSS,
                                         &sNewCurAllocExtRID,
                                         &sNewFstPIDOfCurAllocExt,
                                         &sNewCurAllocPID,
                                         &sNewCurPagePtr ) == IDE_SUCCESS )
        {
            break;
        }

        /* Not enough space 이외의 예외 발생시 바로 예외 처리 */
        IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_SPACE );

        /* BUG-47647 loop 는 0에서 1까지 진행된다. Steal 하고도 할당 실패하면 예외처리
         * loop 0 : 먼저 Free Page 할당 시도, 실패하면 Steal 시도
         * loop 1 : 다시 Free Page 할당 시도, 실패하면 예외처리 */
        IDE_TEST_RAISE( sLoop != 0, error_not_enough_space );

        IDE_CLEAR();

        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

        IDE_TEST( sdcTXSegMgr::tryStealFreeExtsFromOtherEntry(
                                            aStatistics,
                                            &sStartInfo,
                                            SDP_SEG_TYPE_TSS,  /* aFromSegType */
                                            SDP_SEG_TYPE_TSS,  /* aToSegType   */ 
                                            mEntryPtr,
                                            &sSysMinDskViewSCN,
                                            &sTrySuccess ) 
                  != IDE_SUCCESS );

        /* BUG-42975 같은 SegType에서 Steal하지 못했으면 다른 SegType에서 스틸 */
        if ( sTrySuccess == ID_FALSE )
        {
            IDE_TEST( sdcTXSegMgr::tryStealFreeExtsFromOtherEntry(
                                            aStatistics,
                                            &sStartInfo,
                                            SDP_SEG_TYPE_UNDO,  /* aFromSegType */
                                            SDP_SEG_TYPE_TSS,   /* aToSegType   */
                                            mEntryPtr,
                                            &sSysMinDskViewSCN,
                                            &sTrySuccess ) 
                       != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do */
        }
    }

    IDE_TEST( logAndInitPage( aMtx,
                              sNewCurPagePtr,
                              aTransID,
                              aFstDskViewSCN ) 
              != IDE_SUCCESS );

    /* Mtx가 Abort되면, PageImage만 Rollback되지 RuntimeValud는
     * 복구되지 않습니다.
     * 따라서 Rollback시 이전 값으로 복구하도록 합니다.
     * 어차피 Segment는 Transaction당 하나씩 혼자쓰는 객체이기
     * 때문에 백업본은 하나만 있으면 됩니다.*/
    IDE_TEST( sdrMiniTrans::addPendingJob( aMtx,
                                           ID_FALSE, // isCommitJob
                                           sdcTSSegment::abortCurAllocInfo,
                                           (void*)this )
              != IDE_SUCCESS );

    setCurAllocInfo( sNewCurAllocExtRID,
                     sNewFstPIDOfCurAllocExt,
                     sNewCurAllocPID );

    IDE_ASSERT( sNewCurPagePtr != NULL );

    *aNewPagePtr = sNewCurPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_enough_space );
    {
        /* BUG-40980 : AUTOEXTEND OFF상태에서 TBS max size에 도달하여 extend 불가능
         *             error 메시지를 altibase_sm.log에도 출력한다.
         * BUG-47647 : Undo Segment, TSS Segment는 max size에 도달해도 Steal로 할당 할 수 있다.
         *             free page 할당 실패 하였을 때 출력하는 것으로 수정 */
        ideLog::log( IDE_SM_0,
                     "The tablespace does not have enough free space ( TBS Name :<SYS_TBS_DISK_UNDO> ).\n"
                     "TSS Page allocation failed.( TSS Segment PageID: %"ID_UINT32_FMT" )\n",
                     sSegHandle->mSegPID );
    }
    IDE_EXCEPTION_END;

    *aNewPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 기존의 할당 정보로 복구
 *
 *               MtxRollback을 할 경우, DiskPage는 복구되지만 Runtime
 *               정보는 복구되지 않는다. 따라서 복구하기 위해
 *               값을 저장해두고, MtxRollback시 복구한다.
 *
 * [IN] aIsCommitJob         - 이것이 Commit을 위한 작업이냐, 아니냐
 * [IN] aTSSegment           - Abort하려는 TSSegment
 *
 **********************************************************************/
IDE_RC sdcTSSegment::abortCurAllocInfo( void * aTSSegment )
{
    sdcTSSegment   * sTSSegment;
    
    IDE_ASSERT( aTSSegment != NULL );

    sTSSegment = ( sdcTSSegment * ) aTSSegment;

    sTSSegment->mCurAllocExtRID = 
        sTSSegment->mCurAllocExtRID4MtxRollback;
    sTSSegment->mFstPIDOfCurAllocExt = 
        sTSSegment->mFstPIDOfCurAllocExt4MtxRollback;
    sTSSegment->mCurAllocPID =
        sTSSegment->mCurAllocPID4MtxRollback;

    return IDE_SUCCESS;
}




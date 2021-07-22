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
 * $Id: sdcUndoSegment.cpp 90259 2021-03-19 01:22:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 undo segment에 대한 구현 파일이다.
 *
 **********************************************************************/

#include <sdp.h>
#include <sdr.h>
#include <smu.h>
#include <smx.h>

#include <sdcDef.h>
#include <sdcUndoSegment.h>
#include <sdcUndoRecord.h>
#include <sdcTSSlot.h>
#include <sdcTSSegment.h>
#include <sdcTXSegFreeList.h>
#include <sdcRow.h>
#include <sdcReq.h>

/***********************************************************************
 *
 * Description : Undo 세그먼트 생성
 *
 * 디비 생성과정에서 Undo 세그먼트를 생성한다.
 *
 * aStatistics  - [IN] 통계정보
 * aStartInfo   - [IN] Mini Transaction 시작 정보.
 * aUDSegPID    - [IN] 생성된 Segment Hdr PageID
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::create( idvSQL          * aStatistics,
                               sdrMtxStartInfo * aStartInfo,
                               scPageID        * aUDSegPID )
{
    sdrMtx           sMtx;
    UInt             sState = 0;
    sdpSegMgmtOp    *sSegMgmtOP;
    sdpSegmentDesc   sUndoSegDesc;

    IDE_ASSERT( aStartInfo    != NULL );
    IDE_ASSERT( aUDSegPID     != NULL );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    /* Undo Segment 생성 */
    sdpSegDescMgr::setDefaultSegAttr(
                                    sdpSegDescMgr::getSegAttr( &sUndoSegDesc ),
                                    SDP_SEG_TYPE_UNDO);
    sdpSegDescMgr::setDefaultSegStoAttr(
                                    sdpSegDescMgr::getSegStoAttr( &sUndoSegDesc ));

    // Undo Segment Handle에 Segment Cache를 초기화한다.
    IDE_TEST( sdpSegDescMgr::initSegDesc( &sUndoSegDesc,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          SD_NULL_PID, // Segment 생성전
                                          SDP_SEG_TYPE_UNDO,
                                          SM_NULL_OID,
                                          SM_NULL_INDEX_ID ) 
              != IDE_SUCCESS );

    sSegMgmtOP = sdpSegDescMgr::getSegMgmtOp( &sUndoSegDesc );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOP != NULL );

    IDE_TEST( sSegMgmtOP->mCreateSegment( aStatistics,
                                          &sMtx,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          SDP_SEG_TYPE_UNDO,
                                          &sUndoSegDesc.mSegHandle )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    IDE_TEST( sdpSegDescMgr::destSegDesc( &sUndoSegDesc ) != IDE_SUCCESS );

    *aUDSegPID = sdpSegDescMgr::getSegPID( &sUndoSegDesc );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    *aUDSegPID = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : Undo 세그먼트 초기화
 *
 * aUDSegPID에 해당하는 Undo 세그먼트 기술자를 초기화한다.
 *
 * aStatistics  - [IN] 통계정보
 * aEntry       - [IN] 자신이 소속된 트랜잭션 세그먼트 엔트리 포인터
 * aUDSegPID    - [IN] Undo 세그먼트 헤더 페이지 ID
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::initialize( idvSQL        * aStatistics,
                                   sdcTXSegEntry * aEntry,
                                   scPageID        aUDSegPID )
{
    sdpSegInfo        sSegInfo;
    sdpSegMgmtOp     *sSegMgmtOp;

    IDE_ASSERT( aEntry != NULL );

    if ( aEntry->mEntryIdx >= SDP_MAX_UDSEG_PID_CNT )
    {
        ideLog::log( IDE_SERVER_0,
                     "EntryIdx: %u, "
                     "UndoSegPID: %u",
                     aEntry->mEntryIdx,
                     aUDSegPID );

        IDE_ASSERT( 0 );
    }

    sSegMgmtOp =
        sdpSegDescMgr::getSegMgmtOp( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    /* Undo Tablespace는 시스템에 의해서 자동으로 관리되므로
     * Segment Storage Parameter 들을 모두 무시한다 */
    sdpSegDescMgr::setDefaultSegAttr(
                                    sdpSegDescMgr::getSegAttr( &mUDSegDesc ),
                                    SDP_SEG_TYPE_UNDO );

    sdpSegDescMgr::setDefaultSegStoAttr(
        sdpSegDescMgr::getSegStoAttr( &mUDSegDesc ));

    IDE_TEST( sdpSegDescMgr::initSegDesc( &mUDSegDesc,
                                          SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                          aUDSegPID,
                                          SDP_SEG_TYPE_UNDO,
                                          SM_NULL_OID,
                                          SM_NULL_INDEX_ID)
              != IDE_SUCCESS );

    mCreateUndoPageCnt = 0;
    mFreeUndoPageCnt   = 0;
    mEntryPtr          = aEntry;
    /* BUG-40014 [PROJ-2506] Insure++ Warning
     * - 멤버 변수의 초기화가 필요합니다.
     */
    mCurAllocExtRID = ID_ULONG_MAX;
    mFstPIDOfCurAllocExt = ID_UINT_MAX;
    mCurAllocPID = ID_UINT_MAX;

    IDE_TEST( sSegMgmtOp->mGetSegInfo( aStatistics,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       aUDSegPID,
                                       NULL, /* aTableHeader */
                                       &sSegInfo ) != IDE_SUCCESS );

    setCurAllocInfo( sSegInfo.mFstExtRID,
                     sSegInfo.mFstPIDOfLstAllocExt,
                     SD_NULL_PID );

    SM_SET_SCN_INFINITE( &mFstDskViewSCNofCurTrans );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aUDSegPID에 해당하는 Undo Segment를 정리한다.
 ***********************************************************************/
IDE_RC sdcUndoSegment::destroy()
{
    IDE_ASSERT( sdpSegDescMgr::destSegDesc( &mUDSegDesc )
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Undo Page의 CntlHdr 초기화 및 로깅
 **********************************************************************/
IDE_RC sdcUndoSegment::logAndInitPage( sdrMtx      * aMtx,
                                       UChar       * aPagePtr )
{
    sdpPageType   sPageType;

    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aPagePtr != NULL );

    IDE_ERROR( initPage( aPagePtr ) == IDE_SUCCESS );

    sPageType = sdpPhyPage::getPageType( (sdpPhyPageHdr*)aPagePtr );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                      aPagePtr,
                                      &sPageType,
                                      ID_SIZEOF( sdpPageType ),
                                      SDR_SDC_INIT_UNDO_PAGE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_INSERT_ROW_PIECE,
 *   SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | NULL
 *   -----------------------------------------------------------------------------------
 *   | NULL
 *   -----------------------------------------------------------------------------------
 *   | NULL
 *   -----------------------------------------------------------------------------------
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addInsertRowPieceUndoRec(
    idvSQL                      *aStatistics,
    sdrMtxStartInfo             *aStartInfo,
    smOID                        aTableOID,
    scGRID                       aInsRecGRID,
    const sdcRowPieceInsertInfo *aInsertInfo,
    idBool                       aIsUndoRec4HeadRowPiece )
{
    sdrMtx            sMtx;
    UShort            sUndoRecSize;
    UChar*            sUndoRecHdr;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecType    sUndoRecType;
    sdcUndoRecFlag    sUndoRecFlag;
    UChar*            sWritePtr;
    UInt              sState = 0;
    sdSID             sUndoSID;

    IDE_ASSERT( aStartInfo  != NULL );
    IDE_ASSERT( aTableOID   != SM_NULL_OID );
    IDE_ASSERT( aInsertInfo != NULL );

    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aInsRecGRID) );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    sUndoPageHdr = NULL; // for exception

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DML )
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecSize = SDC_UNDOREC_HDR_SIZE + ID_SIZEOF(scGRID);

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    if( aInsertInfo->mIsInsert4Upt != ID_TRUE )
    {
        sUndoRecType = SDC_UNDO_INSERT_ROW_PIECE;
    }
    else
    {
        sUndoRecType = SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE;
    }

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);
    if( aIsUndoRec4HeadRowPiece == ID_TRUE )
    {
        IDE_DASSERT( aInsertInfo->mIsInsert4Upt != ID_TRUE );

        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE );


        /* BUG-31092 When updating foreign key, The table cursor  is not
         * considering that The undo record of UPDATE_LOBDESC can have 
         * a head-rowpiece flag. 
         * Update 도중 Overflow가 발생했을때, 즉 원본 페이지의 공간이
         * 부족해 다른 Page에 기록할때 INSERT_ROW_PIECE_FOR_UPDATE가
         * 수행됨. 따라서 HeadRowPiece를 기록하는 경우는 불가능. */
        IDE_TEST_RAISE( sUndoRecType == SDC_UNDO_INSERT_ROW_PIECE_FOR_UPDATE,
                        ERR_ASSERT );
    }

    sWritePtr = sdcUndoRecord::writeUndoRecHdr( sUndoRecHdr,
                                                sUndoRecType,
                                                sUndoRecFlag,
                                                aTableOID );

    IDE_TEST_RAISE( SC_GRID_IS_NULL( aInsRecGRID ),
                    ERR_ASSERT );

    ID_WRITE_AND_MOVE_DEST( sWritePtr,
                            &aInsRecGRID,
                            ID_SIZEOF(scGRID) );

    IDE_TEST_RAISE( sWritePtr != (sUndoRecHdr + sUndoRecSize),
                    ERR_ASSERT );

    IDE_TEST( makeRedoLogOfUndoRec( sUndoRecHdr,
                                    sUndoRecSize,
                                    aTableOID,
                                    &sMtx ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ASSERT );
    {
        ideLog::log( IDE_SERVER_0,
                     "aTableOID               = %lu\n"
                     "aInsRecGRID.mSpaceID    = %u\n"
                     "aInsRecGRID.mOffset     = %u\n"
                     "aInsRecGRID.mPageID     = %u\n"
                     "aIsUndoRec4HeadRowPiece = %u\n",
                     aTableOID,
                     aInsRecGRID.mSpaceID,
                     aInsRecGRID.mOffset,
                     aInsRecGRID.mPageID,
                     aIsUndoRec4HeadRowPiece  );

        ideLog::log( IDE_SERVER_0,
                     "aInsertInfo->mStartColSeq     = %u\n"
                     "aInsertInfo->mStartColOffset  = %u\n"
                     "aInsertInfo->mEndColSeq       = %u\n"
                     "aInsertInfo->mEndColOffset    = %u\n"
                     "aInsertInfo->mRowPieceSize    = %u\n"
                     "aInsertInfo->mColCount        = %u\n"
                     "aInsertInfo->mLobDescCnt      = %u\n"
                     "aInsertInfo->mIsInsert4Upt    = %u\n",
                     aInsertInfo->mStartColSeq,
                     aInsertInfo->mStartColOffset,
                     aInsertInfo->mEndColSeq,
                     aInsertInfo->mEndColOffset,
                     aInsertInfo->mRowPieceSize, 
                     aInsertInfo->mColCount,
                     aInsertInfo->mLobDescCnt,
                     aInsertInfo->mIsInsert4Upt );

        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)sUndoRecHdr,
                        sUndoRecSize );
        IDE_ASSERT( 0 );
    }

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                                    sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         1 );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );

        default:
            break;
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_UPDATE_ROW_PIECE
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [flag(1)], [size(2)], [column count(2)],
 *   | [column desc set size(1)], [column desc set(1~128)]
 *   -----------------------------------------------------------------------------------
 *   | [row header], [ {column len(1 or 3),column data} ... ]
 *   -----------------------------------------------------------------------------------
 *   | [column count(2)], [ {column seq(2),column id(4)} ... ]
 *   -----------------------------------------------------------------------------------
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addUpdateRowPieceUndoRec(
                                idvSQL                      *aStatistics,
                                sdrMtxStartInfo             *aStartInfo,
                                smOID                        aTableOID,
                                const UChar                 *aUptRecPtr,
                                scGRID                       aUptRecGRID,
                                const sdcRowPieceUpdateInfo *aUpdateInfo,
                                idBool                       aReplicate,
                                sdSID                       *aUndoSID )
{

    sdrMtx            sMtx;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UShort            sUndoRecSize;
    UShort            sUndoLogSize;
    UChar*            sUndoRecHdr;
    UInt              sLogAttr;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUptRecPtr != NULL );
    IDE_ASSERT( aUndoSID   != NULL );
    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aUptRecGRID) );

    IDV_SQL_OPTIME_BEGIN(aStatistics,
                         IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    sLogAttr = SM_DLOG_ATTR_DML;
    sLogAttr |= (aReplicate == ID_TRUE) ?
                 SM_DLOG_ATTR_REPLICATE : SM_DLOG_ATTR_NORMAL;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   sLogAttr)
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecSize = sdcRow::calcUpdateRowPieceLogSize(
                                            aUpdateInfo,
                                            ID_TRUE,    /* aIsUndoRec */
                                            ID_FALSE ); /* aIsReplicate */

    IDE_ASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot(aStatistics,
                        aStartInfo->mTrans,
                        sUndoRecSize,
                        ID_TRUE,
                        &sMtx,
                        &sUndoRecHdr,
                        &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);
    if( sdcRow::isHeadRowPiece( aUptRecPtr ) == ID_TRUE )
    {
        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);
    }

    if( aUpdateInfo->mIsUptLobByAPI == ID_TRUE )
    {
        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_TRUE);
    }

    sWritePtr = sdcUndoRecord::writeUndoRecHdr( sUndoRecHdr,
                                                SDC_UNDO_UPDATE_ROW_PIECE,
                                                sUndoRecFlag,
                                                aTableOID );

    ID_WRITE_AND_MOVE_DEST(sWritePtr, &aUptRecGRID, ID_SIZEOF(scGRID));

    sWritePtr = sdcRow::writeUpdateRowPieceUndoRecRedoLog( sWritePtr,
                                                           aUptRecPtr,
                                                           aUpdateInfo );

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    sUndoLogSize = sUndoRecSize;

    if( aReplicate == ID_TRUE )
    {
        sUndoLogSize +=
            sdcRow::calcUpdateRowPieceLogSize4RP(aUpdateInfo, ID_TRUE);
    }

    sdrMiniTrans::setRefOffset(&sMtx, aTableOID); // undo 로그 위치 기록

    IDE_TEST( sdrMiniTrans::writeLogRec(&sMtx,
                                        (UChar*)sUndoRecHdr,
                                        NULL,
                                        sUndoLogSize + ID_SIZEOF(UShort),
                                        SDR_SDC_INSERT_UNDO_REC)
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (UChar*)&sUndoRecSize,
                                  ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (void*)sUndoRecHdr,
                                  sUndoRecSize )
              != IDE_SUCCESS );

    if( aReplicate == ID_TRUE )
    {
        IDE_TEST( sdcRow::writeUpdateRowPieceLog4RP(aUpdateInfo,
                                                    ID_TRUE,
                                                    &sMtx)
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx, SMR_CT_CONTINUE )
              != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         1 );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_OVERWRITE_ROW_PIECE
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [flag(1)], [size(2)]
 *   -----------------------------------------------------------------------------------
 *   | [row header], [next pid(4),
 *   |                next slotnum(2)], [ {column len(1 or 3),column data} ... ]
 *   -----------------------------------------------------------------------------------
 *   | [column count(2)], [ {column seq(2),column id(4)} ... ]
 *   -----------------------------------------------------------------------------------
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addOverwriteRowPieceUndoRec(
                            idvSQL                         *aStatistics,
                            sdrMtxStartInfo                *aStartInfo,
                            smOID                           aTableOID,
                            const UChar                    *aUptRecPtr,
                            scGRID                          aUptRecGRID,
                            const sdcRowPieceOverwriteInfo *aOverwriteInfo,
                            idBool                          aReplicate,
                            sdSID                          *aUndoSID )
{

    sdrMtx            sMtx;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UShort            sUndoRecSize;
    UShort            sUndoLogSize;
    UChar*            sUndoRecHdr;
    UInt              sLogAttr;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUptRecPtr != NULL );
    IDE_ASSERT( aUndoSID   != NULL );
    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aUptRecGRID) );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );


    sLogAttr = SM_DLOG_ATTR_DML;
    sLogAttr |= (aReplicate == ID_TRUE) ?
                 SM_DLOG_ATTR_REPLICATE : SM_DLOG_ATTR_NORMAL;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   sLogAttr)
              != IDE_SUCCESS );
    sState = 1;

    IDE_DASSERT( aOverwriteInfo->mOldRowPieceSize ==
                 sdcRow::getRowPieceSize(aUptRecPtr) );

    sUndoRecSize = sdcRow::calcOverwriteRowPieceLogSize( aOverwriteInfo,
                                                         ID_TRUE,      /* aIsUndoRec */
                                                         ID_FALSE );   /* aIsReplicate */

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) 
              != IDE_SUCCESS );
    sState = 2;

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);

    if( sdcRow::isHeadRowPiece( aUptRecPtr ) == ID_TRUE )
    {
        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);
    }

    if( aOverwriteInfo->mIsUptLobByAPI == ID_TRUE )
    {
        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_LOB_UPDATE_TRUE);
    }

    sWritePtr = sdcUndoRecord::writeUndoRecHdr( sUndoRecHdr,
                                                SDC_UNDO_OVERWRITE_ROW_PIECE,
                                                sUndoRecFlag,
                                                aTableOID );

    ID_WRITE_AND_MOVE_DEST(sWritePtr, &aUptRecGRID, ID_SIZEOF(scGRID));

    sWritePtr =
        sdcRow::writeOverwriteRowPieceUndoRecRedoLog( sWritePtr,
                                                      aUptRecPtr,
                                                      aOverwriteInfo );

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    sUndoLogSize = sUndoRecSize;

    if( aReplicate == ID_TRUE )
    {
        sUndoLogSize +=
            sdcRow::calcOverwriteRowPieceLogSize4RP( aOverwriteInfo,
                                                     ID_TRUE );
    }

    sdrMiniTrans::setRefOffset(&sMtx, aTableOID); // undo 로그 위치 기록

    // make undo record's redo log
    IDE_TEST( sdrMiniTrans::writeLogRec(&sMtx,
                                        (UChar*)sUndoRecHdr,
                                        NULL,
                                        sUndoLogSize + ID_SIZEOF(UShort),
                                        SDR_SDC_INSERT_UNDO_REC)
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (UChar*)&sUndoRecSize,
                                  ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (void*)sUndoRecHdr,
                                  sUndoRecSize )
              != IDE_SUCCESS );

    if( aReplicate == ID_TRUE )
    {
        IDE_TEST( sdcRow::writeOverwriteRowPieceLog4RP(aOverwriteInfo,
                                                       ID_TRUE,
                                                       &sMtx)
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx, SMR_CT_CONTINUE )
              != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                                     sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_CHANGE_ROW_PIECE_LINK
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [flag(1)], [size(2)]
 *   -----------------------------------------------------------------------------------
 *   | [row header], [next pid(4), next slotnum(2)]
 *   -----------------------------------------------------------------------------------
 *   | NULL
 *   -----------------------------------------------------------------------------------
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addChangeRowPieceLinkUndoRec(
                            idvSQL                      *aStatistics,
                            sdrMtxStartInfo             *aStartInfo,
                            smOID                        aTableOID,
                            const UChar                 *aUptRecPtr,
                            scGRID                       aUptRecGRID,
                            idBool                       aIsTruncateNextLink,
                            sdSID                       *aUndoSID )
{
    sdrMtx            sMtx;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UShort            sUndoRecSize;
    UChar*            sUndoRecHdr;
    SShort            sChangeSize;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;
    UChar             sLogFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUndoSID   != NULL );
    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aUptRecGRID) );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );


    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DML )
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecSize = SDC_UNDOREC_HDR_SIZE +
        + ID_SIZEOF(scGRID)
        + ID_SIZEOF(sLogFlag)
        + ID_SIZEOF(sChangeSize)
        + SDC_ROWHDR_SIZE
        + ID_SIZEOF(scPageID)
        + ID_SIZEOF(scSlotNum);

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);
    if( sdcRow::isHeadRowPiece( aUptRecPtr ) == ID_TRUE )
    {
        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);
    }

    sWritePtr = sdcUndoRecord::writeUndoRecHdr( sUndoRecHdr,
                                                SDC_UNDO_CHANGE_ROW_PIECE_LINK,
                                                sUndoRecFlag,
                                                aTableOID );

    ID_WRITE_AND_MOVE_DEST(sWritePtr, &aUptRecGRID, ID_SIZEOF(scGRID));

    /*
     * ###   FSC 플래그   ###
     *
     * FSC 플래그 설정방법에 대한 자세한 설명은 sdcRow.h의 주석을 참고하라
     *
     * redo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE
     * undo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
     * clr redo : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
     */
    sLogFlag  = 0;
    sLogFlag |= SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE;
    ID_WRITE_1B_AND_MOVE_DEST( sWritePtr, &sLogFlag );

    // change size
    if( aIsTruncateNextLink == ID_TRUE )
    {
        sChangeSize = SDC_EXTRASIZE_FOR_CHAINING;
    }
    else
    {
        sChangeSize = 0;
    }

    ID_WRITE_AND_MOVE_DEST(sWritePtr, &sChangeSize, ID_SIZEOF(sChangeSize));

    // row header
    ID_WRITE_AND_MOVE_DEST(sWritePtr, aUptRecPtr, SDC_ROWHDR_SIZE);

    // page id
    ID_WRITE_AND_MOVE_DEST( sWritePtr,
                            aUptRecPtr + SDC_ROW_NEXT_PID_OFFSET,
                            ID_SIZEOF(scPageID) );
    // slot num
    ID_WRITE_AND_MOVE_DEST( sWritePtr,
                            aUptRecPtr + SDC_ROW_NEXT_SNUM_OFFSET,
                            ID_SIZEOF(scSlotNum) );

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    IDE_TEST( makeRedoLogOfUndoRec( sUndoRecHdr,
                                    sUndoRecSize,
                                    aTableOID,
                                    &sMtx )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx, SMR_CT_CONTINUE)
              != IDE_SUCCESS );

    IDV_SQL_OPTIME_END(aStatistics,
                       IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_DELETE_FIRST_COLUMN_PIECE
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [flag(1)], [size(2)]
 *   -----------------------------------------------------------------------------------
 *   | [row header], [column len(1 or 3),column data]
 *   -----------------------------------------------------------------------------------
 *   | [column count(2)], [column seq(2),column id(4)]
 *   -----------------------------------------------------------------------------------
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addDeleteFstColumnPieceUndoRec(
    idvSQL                      *aStatistics,
    sdrMtxStartInfo             *aStartInfo,
    smOID                        aTableOID,
    const UChar                 *aUptRecPtr,
    scGRID                       aUptRecGRID,
    const sdcRowPieceUpdateInfo *aUpdateInfo,
    idBool                       aReplicate,
    sdSID                       *aUndoSID )
{

    sdrMtx            sMtx;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UShort            sUndoRecSize;
    UShort            sUndoLogSize;
    UChar*            sUndoRecHdr;
    UInt              sLogAttr;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUptRecPtr != NULL );
    IDE_ASSERT( aUndoSID      != NULL );
    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aUptRecGRID) );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );


    sLogAttr = SM_DLOG_ATTR_DML;
    sLogAttr |= (aReplicate == ID_TRUE) ?
                 SM_DLOG_ATTR_REPLICATE : SM_DLOG_ATTR_NORMAL;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   sLogAttr)
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecSize = sdcRow::calcDeleteFstColumnPieceLogSize(
                           aUpdateInfo,
                           ID_FALSE );  /* aIsReplicate */

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    if ( sdcRow::isHeadRowPiece(aUptRecPtr) == ID_TRUE )
    {
        ideLog::log( IDE_SERVER_0, "TableOID:%lu", aTableOID );

        ideLog::log( IDE_SERVER_0, "UptRecGRID Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&aUptRecGRID,
                        ID_SIZEOF(scGRID) );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr((void*)aUptRecPtr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);
    SDC_SET_UNDOREC_FLAG_OFF( sUndoRecFlag,
                              SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);

    sWritePtr = sdcUndoRecord::writeUndoRecHdr(
                                           sUndoRecHdr,
                                           SDC_UNDO_DELETE_FIRST_COLUMN_PIECE,
                                           sUndoRecFlag,
                                           aTableOID );

    ID_WRITE_AND_MOVE_DEST(sWritePtr, &aUptRecGRID, ID_SIZEOF(scGRID));

    sWritePtr = sdcRow::writeDeleteFstColumnPieceRedoLog( sWritePtr,
                                                          aUptRecPtr,
                                                          aUpdateInfo );

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    if( aReplicate != ID_TRUE )
    {
        sUndoLogSize = sUndoRecSize;
    }
    else
    {
        sUndoLogSize =
            sdcRow::calcDeleteFstColumnPieceLogSize( aUpdateInfo,
                                                     aReplicate );
    }

    sdrMiniTrans::setRefOffset(&sMtx, aTableOID); // undo 로그 위치 기록

    IDE_TEST( sdrMiniTrans::writeLogRec(&sMtx,
                                        (UChar*)sUndoRecHdr,
                                        NULL,
                                        sUndoLogSize + ID_SIZEOF(UShort),
                                        SDR_SDC_INSERT_UNDO_REC)
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (UChar*)&sUndoRecSize,
                                  ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (void*)sUndoRecHdr,
                                  sUndoRecSize )
              != IDE_SUCCESS );

    if( aReplicate == ID_TRUE )
    {
        IDE_TEST( sdcRow::writeDeleteFstColumnPieceLog4RP( aUpdateInfo,
                                                           &sMtx )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx, SMR_CT_CONTINUE )
              != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_DELETE_ROW_PIECE
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [size(2)]
 *   -----------------------------------------------------------------------------------
 *   | [row header], [next pid(4), next slotnum(2)]
 *   [ {column len(1 or 3),column data} ... ]
 *   -----------------------------------------------------------------------------------
 *   | NULL
 *   -----------------------------------------------------------------------------------
 *
 *   TASK-5030
 *   supplemental log를 추가로 남기는 경우, SDC_UNDO_DELETE_ROW_PIECE 에서
 *   rp info가 기록된다.
 *   -----------------------------------------------------------------------------------
 *   | [column count(2)], [ {column seq(2),column id(4)} ... ]
 *   -----------------------------------------------------------------------------------
 *
 *   SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [size(2)]
 *   -----------------------------------------------------------------------------------
 *   | [row header], [next pid(4), next slotnum(2)],
 *   | [ {column len(1 or 3),column data} .. ]
 *   -----------------------------------------------------------------------------------
 *   | [column count(2)], [ {column seq(2),column id(4)} ... ]
 *   -----------------------------------------------------------------------------------
 *
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addDeleteRowPieceUndoRec(
                                    idvSQL                         *aStatistics,
                                    sdrMtxStartInfo                *aStartInfo,
                                    smOID                           aTableOID,
                                    UChar                          *aDelRecPtr,
                                    scGRID                          aDelRecGRID,
                                    idBool                          aIsDelete4Upt,
                                    idBool                          aReplicate,
                                    const sdcRowPieceUpdateInfo    *aUpdateInfo,
                                    sdSID                          *aUndoSID )
{
    sdrMtx            sMtx;
    UShort            sUndoRecSize;
    UShort            sUndoLogSize;
    UShort            sRowPieceSize;
    UChar*            sUndoRecHdr;
    void             *sTableHeader;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UInt              sLogAttr;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecType    sUndoRecType;
    sdcUndoRecFlag    sUndoRecFlag;
    SShort            sChangeSize;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aTableOID  != SM_NULL_OID );
    IDE_ASSERT( aDelRecPtr != NULL );
    IDE_ASSERT( aUndoSID   != NULL );
    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aDelRecGRID) );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    sUndoPageHdr = NULL; // for exception

    sLogAttr = SM_DLOG_ATTR_DML;
    sLogAttr |= (aReplicate == ID_TRUE) ?
                 SM_DLOG_ATTR_REPLICATE : SM_DLOG_ATTR_NORMAL;

    /* Insert Undo Rec에 대해서는 Undo를 하지 않도록 합니다. 왜냐면
     * Insert에 대한 Undo시 Ager가 Insert된 Record를 삭제하도록 Undo
     * Page에 있는 Insert Undo Rec를 Delete Undo Record로 변경합니다.
     * 때문에 Undo시 지우면 안됩니다. 하지만 Insert DML Log를 찍지
     * 못하고 Server가 죽은 경우에는 Undo를 수행하여 Insert Undo
     * Record를 Truncate하여야 하나 하지 않아도 문제없습니다. Ager는
     * 이 Insert Undo Record를 무시합니다. */
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   sLogAttr ) != IDE_SUCCESS );
    sState = 1;

    sRowPieceSize = sdcRow::getRowPieceSize(aDelRecPtr);

    sChangeSize = sRowPieceSize - SDC_ROWHDR_SIZE;
    if ( sChangeSize < 0 )
    {
        ideLog::log( IDE_SERVER_0,
                     "TableOID:%lu, "
                     "ChangeSize:%d, "
                     "RowPieceSize:%d",
                     aTableOID,
                     sChangeSize,
                     sRowPieceSize );

        ideLog::log( IDE_SERVER_0, "DelRecGRID Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&aDelRecGRID,
                        ID_SIZEOF(scGRID) );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr(aDelRecPtr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }

    sUndoRecSize = SDC_UNDOREC_HDR_SIZE +
                    ID_SIZEOF(scGRID)   +
                    ID_SIZEOF(SShort)   +
                    sRowPieceSize;

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    if( aIsDelete4Upt != ID_TRUE )
    {
        sUndoRecType = SDC_UNDO_DELETE_ROW_PIECE;
    }
    else
    {
        sUndoRecType = SDC_UNDO_DELETE_ROW_PIECE_FOR_UPDATE;
    }

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);

    if( sdcRow::isHeadRowPiece(aDelRecPtr) == ID_TRUE )
    {
        if ( aIsDelete4Upt == ID_TRUE )
        {
            ideLog::log( IDE_SERVER_0,
                         "TableOID:%lu",
                         aTableOID );

            ideLog::log( IDE_SERVER_0, "DelRecGRID Dump:" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&aDelRecGRID,
                            ID_SIZEOF(scGRID) );

            ideLog::log( IDE_SERVER_0, "Page Dump:" );
            ideLog::logMem( IDE_SERVER_0,
                            sdpPhyPage::getPageStartPtr(aDelRecPtr),
                            SD_PAGE_SIZE );

            IDE_ASSERT( 0 );
        }

        SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                                 SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);
    }

    sWritePtr = sdcUndoRecord::writeUndoRecHdr( sUndoRecHdr,
                                                sUndoRecType,
                                                sUndoRecFlag,
                                                aTableOID );

    ID_WRITE_AND_MOVE_DEST(sWritePtr,
                           &aDelRecGRID,
                           ID_SIZEOF(scGRID));

    // sChangeSize
    ID_WRITE_AND_MOVE_DEST(sWritePtr,
                           &sChangeSize,
                           ID_SIZEOF(SShort));

    // copy delete before image
    ID_WRITE_AND_MOVE_DEST(sWritePtr,
                           aDelRecPtr,
                           sRowPieceSize );

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize));

    IDE_ASSERT( smLayerCallback::getTableHeaderFromOID( aTableOID,
                                                        (void**)&sTableHeader )
                == IDE_SUCCESS );

    sUndoLogSize = sUndoRecSize;
    /* TASK-5030 */
    if( ( (aReplicate == ID_TRUE) && (aIsDelete4Upt == ID_TRUE) ) ||
        ( smcTable::isSupplementalTable((smcTableHeader *)sTableHeader) == ID_TRUE))
    {
        sUndoLogSize += sdcRow::calcDeleteRowPieceLogSize4RP( aDelRecPtr,
                                                              aUpdateInfo );
    }

    sdrMiniTrans::setRefOffset(&sMtx, aTableOID); // undo 로그 위치 기록

    // make undo record's redo log
    IDE_TEST( sdrMiniTrans::writeLogRec(&sMtx,
                                        (UChar*)sUndoRecHdr,
                                        NULL,
                                        sUndoLogSize + ID_SIZEOF(UShort),
                                        SDR_SDC_INSERT_UNDO_REC)
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( &sMtx,
                                   (UChar*)&sUndoRecSize,
                                   ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(&sMtx,
                                  (void*)sUndoRecHdr,
                                  sUndoRecSize )
              != IDE_SUCCESS );

    /* TASK-5030 */
    if( ( (aReplicate == ID_TRUE) && (aIsDelete4Upt == ID_TRUE) ) ||
        ( smcTable::isSupplementalTable((smcTableHeader *)sTableHeader) == ID_TRUE))
    {
        IDE_ASSERT( smLayerCallback::getTableHeaderFromOID( aTableOID,
                                                            (void**)&sTableHeader )
                    == IDE_SUCCESS );

        IDE_TEST( sdcRow::writeDeleteRowPieceLog4RP( sTableHeader,
                                                     aDelRecPtr,
                                                     aUpdateInfo,
                                                     &sMtx )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                sdpPhyPage::getPageStartPtr(sUndoRecHdr));

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *   SDC_UNDO_LOCK_ROW
 *   -----------------------------------------------------------------------------------
 *   | [sdrLogHdr]
 *   -----------------------------------------------------------------------------------
 *   | [undo record header], [scGRID]
 *   -----------------------------------------------------------------------------------
 *   | [flag(1)]
 *   -----------------------------------------------------------------------------------
 *   | [row header]
 *   -----------------------------------------------------------------------------------
 *   | NULL
 *   -----------------------------------------------------------------------------------
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addLockRowUndoRec(
                                idvSQL                      *aStatistics,
                                sdrMtxStartInfo             *aStartInfo,
                                smOID                        aTableOID,
                                const UChar                 *aRecPtr,
                                scGRID                       aRecGRID,
                                idBool                       aIsExplicitLock,
                                sdSID                       *aUndoSID )
{
    sdrMtx            sMtx;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UShort            sUndoRecSize;
    UChar*            sUndoRecHdr;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;
    UChar             sLogFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUndoSID   != NULL );
    IDE_ASSERT( SC_GRID_IS_NOT_NULL(aRecGRID) );

    IDV_SQL_OPTIME_BEGIN(aStatistics,
                         IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DML )
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecSize = SDC_UNDOREC_HDR_SIZE +
                    + ID_SIZEOF(scGRID)
                    + ID_SIZEOF(sLogFlag)
                    + SDC_ROWHDR_SIZE;

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    if ( sdcRow::isHeadRowPiece(aRecPtr) == ID_FALSE )
    {
        ideLog::log( IDE_SERVER_0, "TableOID:%lu", aTableOID );

        ideLog::log( IDE_SERVER_0, "RecGRID Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&aRecGRID,
                        ID_SIZEOF(scGRID) );

        ideLog::log( IDE_SERVER_0, "Page Dump:" );
        ideLog::logMem( IDE_SERVER_0,
                        sdpPhyPage::getPageStartPtr((void*)aRecPtr),
                        SD_PAGE_SIZE );

        IDE_ASSERT( 0 );
    }


    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);

    sWritePtr = sdcUndoRecord::writeUndoRecHdr( sUndoRecHdr,
                                                SDC_UNDO_LOCK_ROW,
                                                sUndoRecFlag,
                                                aTableOID );

    ID_WRITE_AND_MOVE_DEST(sWritePtr, &aRecGRID, ID_SIZEOF(scGRID));

    sLogFlag = 0;
    if(aIsExplicitLock == ID_TRUE)
    {
        sLogFlag |= SDC_UPDATE_LOG_FLAG_LOCK_TYPE_EXPLICIT;
    }
    else
    {
        sLogFlag |= SDC_UPDATE_LOG_FLAG_LOCK_TYPE_IMPLICIT;
    }

    ID_WRITE_1B_AND_MOVE_DEST(sWritePtr, &sLogFlag);

    ID_WRITE_AND_MOVE_DEST(sWritePtr, aRecPtr, SDC_ROWHDR_SIZE);

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    IDE_TEST( makeRedoLogOfUndoRec( sUndoRecHdr,
                                    sUndoRecSize,
                                    aTableOID,
                                    &sMtx )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx, SMR_CT_CONTINUE)
              != IDE_SUCCESS );

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                                  sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;
}

#if 0
/***********************************************************************
 *
 * Description :
 *
 *   - [IN]
 *   - [OUT]
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::addIndexCTSlotUndoRec(
                                idvSQL                      *aStatistics,
                                sdrMtxStartInfo             *aStartInfo,
                                smOID                        aTableOID,
                                const UChar                 *aRecPtr,
                                UShort                       aRecSize,
                                sdSID                       *aUndoSID )
{
    sdrMtx            sMtx;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    UShort            sUndoRecSize;
    UChar*            sUndoRecHdr;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUndoSID   != NULL );

    IDV_SQL_OPTIME_BEGIN(aStatistics,
                         IDV_OPTM_INDEX_DRDB_CHAIN_INDEX_TSS );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aStartInfo,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    sUndoRecSize = SDC_UNDOREC_HDR_SIZE + aRecSize;

    /* TC/FIT/ART/sm/Bugs/BUG-32976/BUG-32976.tc */
    IDU_FIT_POINT( "1.BUG-32976@sdcUndoSegment::addIndexCTSlotUndoRec" );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);
    SDC_SET_UNDOREC_FLAG_OFF( sUndoRecFlag,
                              SDC_UNDOREC_FLAG_UNDO_FOR_HEAD_ROWPIECE_TRUE);

    sWritePtr = sdcUndoRecord::writeUndoRecHdr(
                       sUndoRecHdr,
                       SDC_UNDO_INDEX_CTS,
                       sUndoRecFlag,
                       aTableOID );

    // copy CTS record and key list
    ID_WRITE_AND_MOVE_DEST(sWritePtr, aRecPtr, aRecSize);

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    IDE_TEST( makeRedoLogOfUndoRec( sUndoRecHdr,
                                    sUndoRecSize,
                                    SM_NULL_OID,
                                    &sMtx )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    IDV_SQL_OPTIME_END(aStatistics,
                       IDV_OPTM_INDEX_DRDB_CHAIN_INDEX_TSS );

    *aUndoSID = sUndoSID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_CHAIN_INDEX_TSS );

    return IDE_FAILURE;
}
#endif

/*
 * PROJ-2047 Strengthening LOB
 */
IDE_RC sdcUndoSegment::addUpdateLobLeafKeyUndoRec(
                                            idvSQL           *aStatistics,
                                            sdrMtxStartInfo*  aStartInfo,
                                            smOID             aTableOID,
                                            UChar*            aKey,
                                            sdSID            *aUndoSID )
{
    sdrMtx            sMtx;
    UShort            sUndoRecSize;
    UChar*            sUndoRecHdr;
    UChar*            sWritePtr;
    sdSID             sUndoSID;
    scGRID            sDummyGRID;
    UInt              sState = 0;
    sdpPhyPageHdr*    sUndoPageHdr;
    sdcUndoRecFlag    sUndoRecFlag;

    IDE_ASSERT( aStartInfo != NULL );
    IDE_ASSERT( aUndoSID   != NULL );
    IDE_ASSERT( aTableOID  != SM_NULL_OID );

    IDV_SQL_OPTIME_BEGIN( aStatistics,
                          IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    SC_MAKE_NULL_GRID(sDummyGRID);

    sUndoPageHdr = NULL; // for exception

    IDE_TEST( sdrMiniTrans::begin(aStatistics,
                                  &sMtx,
                                  aStartInfo,
                                  ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                  SM_DLOG_ATTR_DEFAULT)
              != IDE_SUCCESS );
    sState = 1;

    // calc undo rec size & set undo record header
    sUndoRecSize =
        SDC_UNDOREC_HDR_SIZE
        + ID_SIZEOF(scGRID)     // row's GSID
        + ID_SIZEOF(sdcLobLKey); // lob index leaf key

    IDE_DASSERT( ( sUndoRecSize + ID_SIZEOF(sdpSlotEntry) ) <=
                 sdpPhyPage::getEmptyPageFreeSize() );

    IDE_TEST( allocSlot( aStatistics,
                         aStartInfo->mTrans,
                         sUndoRecSize,
                         ID_TRUE,
                         &sMtx,
                         &sUndoRecHdr,
                         &sUndoSID ) != IDE_SUCCESS );
    sState = 2;

    sUndoRecFlag = 0;
    SDC_SET_UNDOREC_FLAG_ON( sUndoRecFlag,
                             SDC_UNDOREC_FLAG_IS_VALID_TRUE);

    sWritePtr = sdcUndoRecord::writeUndoRecHdr(
                       sUndoRecHdr,
                       SDC_UNDO_UPDATE_LOB_LEAF_KEY,
                       sUndoRecFlag,
                       aTableOID );

    // record's GSID
    ID_WRITE_AND_MOVE_DEST(sWritePtr, &sDummyGRID, ID_SIZEOF(scGRID));

    // lob index leaf key
    ID_WRITE_AND_MOVE_DEST(sWritePtr, aKey, ID_SIZEOF(sdcLobLKey));

    IDE_ASSERT( sWritePtr == (sUndoRecHdr + sUndoRecSize) );

    IDE_TEST( makeRedoLogOfUndoRec( sUndoRecHdr,
                                    sUndoRecSize,
                                    aTableOID,
                                    &sMtx )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    *aUndoSID = sUndoSID;

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch (sState)
    {
        case 2:
            sUndoPageHdr = sdpPhyPage::getHdr(
                sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            (void) sdpPhyPage::freeSlot( sUndoPageHdr,
                                         sUndoRecHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         1 );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );

        default:
            break;
    }

    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_WRITE_UNDO_RECORD );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : Undo Slot 할당
 *
 * Undo Page로부터 Undo Record를 저장할 수 있는 slot을 할당한다.
 * Undo Page를 새로 생성하는 경우에는 첫번째 Slot Entry는 페이지의 Footer의
 * Offset을 저장하는데 사용하고, 두번째 Slot Entry부터 Undo Record들의 오프셋을
 * 저장하는데 사용한다.( Undo Record의 길이 계산을 위해서 )
 *
 * aStatistics       - [IN] 통계정보
 * aTrans            - [IN] 트랜잭션 포인터
 * aUndoRecSize      - [IN] SlotEntry 할당여부
 * aIsAllocSlotEntry - [IN] SlotEntry 할당여부
 * aMtx              - [IN] Mtx 포인터
 * aSlotPtr          - [OUT] Slot 포인터
 * aUndoSID          - [OUT] UndoRow의 SID
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::allocSlot( idvSQL           * aStatistics,
                                  void             * aTrans,
                                  UShort             aUndoRecSize,
                                  idBool             aIsAllocSlotEntry,
                                  sdrMtx           * aMtx,
                                  UChar           ** aSlotPtr,
                                  sdSID            * aUndoSID )
{
    UChar             * sNewCurPagePtr;
    sdpPhyPageHdr     * sPageHdr;
    UChar             * sSlotDirPtr;
    scOffset            sSlotOffset;
    sdRID               sPrvAllocExtRID;
    UChar             * sAllocSlotPtr;
    UShort              sAllowedSize;
    sdrSavePoint        sSvp;
    UInt                sState = 0;
    idBool              sCanAlloc;
    idBool              sIsCreate;
    smSCN               sFstDskViewSCN;
    scSlotNum           sNewSlotNum;
    sdpPageType         sPageType;

    IDE_ASSERT( aUndoRecSize > 0 );
    IDE_ASSERT( aTrans       != NULL );
    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSlotPtr     != NULL );
    IDE_ASSERT( aUndoSID     != NULL );

    sIsCreate = ID_FALSE;
    sCanAlloc = ID_FALSE;

    if ( mCurAllocPID != SD_NULL_PID )
    {
        sdrMiniTrans::setSavePoint( aMtx, &sSvp ); // set mtx savepoint

        IDE_TEST( getCurPID4Update( aStatistics,
                                    aMtx,
                                    &sNewCurPagePtr ) != IDE_SUCCESS );

        sPageHdr  = sdpPhyPage::getHdr( sNewCurPagePtr );
        sPageType = sdpPhyPage::getPageType( sPageHdr );

        if( sPageType != SDP_PAGE_UNDO )
        {
            // BUG-28785 Case-23923의 Server 비정상 종료에 대한 디버깅 코드 추가
            // Dump Page Hex and Page Hdr
            (void)sdpPhyPage::tracePage( IDE_SERVER_0,
                                         (UChar*)sPageHdr,
                                         "CurAllocUndoPage is not Undo Page Type "
                                         "( CurAllocPID : %"ID_UINT32_FMT""
                                         ", PageType : %"ID_UINT32_FMT" )\n",
                                         mCurAllocPID,
                                         sPageType );

            IDE_ASSERT( 0 );
        }

        sCanAlloc = sdpPhyPage::canAllocSlot( sPageHdr,
                                              aUndoRecSize,
                                              aIsAllocSlotEntry,
                                              SDP_1BYTE_ALIGN_SIZE );
        if ( sCanAlloc == ID_TRUE )
        {
            sIsCreate = ID_FALSE;
        }
        else
        {
            IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sSvp )
                      != IDE_SUCCESS );

            sIsCreate = ID_TRUE;
        }
    }
    else
    {
        sIsCreate = ID_TRUE;
    }

    sPrvAllocExtRID = mCurAllocExtRID;

    if ( sIsCreate == ID_TRUE )
    {
        IDE_TEST( allocNewPage( aStatistics,
                                aMtx,
                                &sNewCurPagePtr ) != IDE_SUCCESS );
    }

    sPageHdr    = sdpPhyPage::getHdr( sNewCurPagePtr );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sNewCurPagePtr);
    sNewSlotNum = sdpSlotDirectory::getCount(sSlotDirPtr);

    IDE_TEST( sdpPhyPage::allocSlot( sPageHdr,
                                     sNewSlotNum,
                                     aUndoRecSize,
                                     aIsAllocSlotEntry,
                                     &sAllowedSize,
                                     &sAllocSlotPtr,
                                     &sSlotOffset,
                                     SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_ASSERT( sAllocSlotPtr != NULL );
    sState = 1;

    /* 페이지를 사용하는 트랜잭션의 Begin SCN을 Extent Dir.
     * 페이지에 기록해두어서 계속 사용되다가 자신이 사용한 Extent Dir.을
     * 재사용하는 일이 없도록 한다. */
    sFstDskViewSCN = smxTrans::getFstDskViewSCN( aTrans );

    if ( (!SM_SCN_IS_EQ( &mFstDskViewSCNofCurTrans, &sFstDskViewSCN )) ||
         (SD_MAKE_PID(sPrvAllocExtRID) != SD_MAKE_PID(mCurAllocExtRID)) )
    {
        /* 해당 UDS를 사용한 마지막 트랜잭션의 Begin SCN과 동일하지 않은 경우만
         * Extent Dir. 페이지에 Begin SCN을 기록하도록하여 동일한 트랜잭션당
         * 한번만 설정하도록 한다. */
        IDE_TEST( mUDSegDesc.mSegMgmtOp->mSetSCNAtAlloc(
                    aStatistics,
                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                    mCurAllocExtRID,
                    &sFstDskViewSCN ) != IDE_SUCCESS );

        SM_SET_SCN( &mFstDskViewSCNofCurTrans, &sFstDskViewSCN );
    }

    ((smxTrans*)aTrans)->setUndoCurPos( SD_MAKE_SID( mCurAllocPID,
                                                     sNewSlotNum ),
                                        mCurAllocExtRID );

    sState    = 0;
    *aSlotPtr = sAllocSlotPtr;
    IDE_DASSERT( mCurAllocPID == sPageHdr->mPageID );
    *aUndoSID = SD_MAKE_SID( mCurAllocPID, sNewSlotNum );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0 )
    {
        IDE_PUSH();
        (void) sdpPhyPage::freeSlot( sPageHdr,
                                     sAllocSlotPtr,
                                     aUndoRecSize,
                                     aIsAllocSlotEntry,
                                     SDP_1BYTE_ALIGN_SIZE );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/******************************************************************************
 *
 * Description : 가용한 언두 페이지를 할당한다.
 *
 * 해당 세그먼트에 존재하지 않는다면, 오프라인 엔트리로부터 가용한
 * 익스텐트 Dir.를 얻는다.
 *
 * aStatistics      - [IN]  통계정보
 * aStartInfo       - [IN]  Mtx 시작정보
 * aTransID         - [IN]  트랜잭션 ID
 * aTransBSCN       - [IN]  페이지를 할당한 트랜잭션의 BegingSCN
 * aPhyPagePtr      - [OUT] 할당받은 페이지 포인터
 *
 ******************************************************************************/
IDE_RC sdcUndoSegment::allocNewPage( idvSQL  * aStatistics,
                                     sdrMtx  * aMtx,
                                     UChar  ** aNewPagePtr )
{
    sdRID           sNewCurAllocExtRID;
    scPageID        sNewFstPIDOfCurAllocExt;
    scPageID        sNewCurAllocPID;
    UChar         * sNewCurPagePtr;
    smSCN           sSysMinDskViewSCN;
    sdrMtxStartInfo sStartInfo;
    UInt            sState = 0;
    SInt            sLoop;
    idBool          sTrySuccess = ID_FALSE;
    sdpSegHandle  * sSegHandle  = getSegHandle();

    IDE_ASSERT( aNewPagePtr != NULL );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    IDV_SQL_OPTIME_BEGIN(
            aStatistics,
            IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE );
    sState = 1;

    for ( sLoop=0; sLoop < 2; sLoop++ )
    {
        sNewCurPagePtr = NULL;

        if ( mUDSegDesc.mSegMgmtOp->mAllocNewPage4Append(
                                             aStatistics,
                                             aMtx,
                                             SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                             sSegHandle,
                                             mCurAllocExtRID,
                                             mFstPIDOfCurAllocExt,
                                             mCurAllocPID,
                                             ID_TRUE,     /* aIsLogging */
                                             SDP_PAGE_UNDO,
                                             &sNewCurAllocExtRID,
                                             &sNewFstPIDOfCurAllocExt,
                                             &sNewCurAllocPID,
                                             &sNewCurPagePtr ) 
             == IDE_SUCCESS )
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
                                                SDP_SEG_TYPE_UNDO, /*aFromSegType*/
                                                SDP_SEG_TYPE_UNDO, /*aToSegType  */
                                                mEntryPtr,
                                                &sSysMinDskViewSCN,
                                                &sTrySuccess ) 
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDV_SQL_OPTIME_END( aStatistics,
                        IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE );

    IDE_TEST( logAndInitPage( aMtx,
                              sNewCurPagePtr ) != IDE_SUCCESS );

    /* Mtx가 Abort되면, PageImage만 Rollback되지 RuntimeValud는
     * 복구되지 않습니다.
     * 따라서 Rollback시 이전 값으로 복구하도록 합니다.
     * 어차피 Segment는 Transaction당 하나씩 혼자쓰는 객체이기
     * 때문에 백업본은 하나만 있으면 됩니다.*/
    IDE_TEST( sdrMiniTrans::addPendingJob( aMtx,
                                           ID_FALSE, // isCommitJob
                                           sdcUndoSegment::abortCurAllocInfo,
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
                     "Undo Page allocation failed.( Undo Segment PageID: %"ID_UINT32_FMT" )\n",
                     sSegHandle->mSegPID );
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDV_SQL_OPTIME_END( aStatistics,
                            IDV_OPTM_INDEX_DRDB_DML_ALLOC_UNDO_PAGE );
    }

    return IDE_FAILURE;
}

/******************************************************************************
 *
 * Description : 가용한 언두 페이지를 확보한다.
 *
 * 현재 확보된 Extent로부터 다음 페이지를 할당할 수 있는지 확인하고,
 * 아니면 다음 Extent, 아니면 새로운 Extent를 할당해 둔다.
 * 최종적으로 확보가 불가능하다고 판단되면, SpaceNotEnough 에러가
 * 반환된다.
 *
 * aStatistics  - [IN] 통계정보
 * aMtx         - [IN] Mtx 시작정보
 *
 ******************************************************************************/
IDE_RC sdcUndoSegment::prepareNewPage( idvSQL       * aStatistics,
                                       sdrMtx       * aMtx )
{
    sdpSegMgmtOp   *sSegMgmtOp;
    SInt            sLoop;
    smSCN           sSysMinDskViewSCN;
    sdrMtxStartInfo sStartInfo;
    idBool          sTrySuccess = ID_FALSE;

    IDE_ASSERT( aMtx != NULL );

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    sSegMgmtOp = mUDSegDesc.mSegMgmtOp;

    /* BUG-43538 디스크 인덱스가 undo Page를 할당해 갈때도
     * Segment간 Steal 정책을 따라야 합니다.
     */
    for ( sLoop = 0 ; sLoop < 2 ; sLoop++ )
    {
        if ( sSegMgmtOp->mPrepareNewPage4Append( 
                                       aStatistics,
                                       aMtx,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       getSegHandle(),
                                       mCurAllocExtRID,
                                       mFstPIDOfCurAllocExt,
                                       mCurAllocPID )
             == IDE_SUCCESS )
        {
            break;
        }

        IDE_TEST( sLoop != 0 );

        IDE_TEST( ideGetErrorCode() != smERR_ABORT_NOT_ENOUGH_SPACE );

        IDE_CLEAR();

        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

        IDE_TEST( sdcTXSegMgr::tryStealFreeExtsFromOtherEntry(
                                                aStatistics,
                                                &sStartInfo,
                                                SDP_SEG_TYPE_UNDO, /*aFromSegType*/
                                                SDP_SEG_TYPE_UNDO, /*aToSegType  */
                                                mEntryPtr,
                                                &sSysMinDskViewSCN,
                                                &sTrySuccess ) 
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : SDR_SDC_INSERT_UNDO_REC 로깅
 *
 * undo record를 작성한 후 이 함수가 호출되어 redo 로그를 mtx에 버퍼링한다.
 * - SDR_UNDO_INSERT 타입으로 undo record image를
 * 버퍼링한다.
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::makeRedoLogOfUndoRec( UChar*        aUndoRecPtr,
                                             UShort        aLength,
                                             smOID         aTableOID,
                                             sdrMtx*       aMtx )
{
    IDE_DASSERT( aUndoRecPtr != NULL );
    IDE_DASSERT( aLength     != 0 );
    IDE_DASSERT( aMtx        != NULL );

    sdrMiniTrans::setRefOffset(aMtx, aTableOID); // undo 로그 위치 기록

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         aUndoRecPtr,
                                         NULL,
                                         aLength + ID_SIZEOF(UShort),
                                         SDR_SDC_INSERT_UNDO_REC )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(aMtx,
                                  (UChar*)&aLength,
                                  ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(aMtx,
                                  (void*)aUndoRecPtr,
                                  aLength )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : UndoRec을 할당한 ExtDir의 CntlHdr에 SCN을 설정
 *
 * unbind TSS 과정에서 CommitSCN 혹은 Abort SCN을 해당 트랜잭션이 사용했던
 * ExtDir 페이지에 No-Logging으로 모두 기록한다. 이때 기록된 SCN을 기준으로
 * UDS의 Extent Dir.페이지의 재사용여부를 판단한다.
 *
 * aStatistics - [IN] 통계정보
 * aSpaceID    - [IN] 테이블스페이스 ID
 * aSegPID     - [IN] UDS의 Segment PID
 * aFstExtRID  - [IN] 트랜잭션이 사용한 첫번째 ExtRID
 * aLstExtRID  - [IN] 트랜잭션이 사용한 마지막 ExtRID
 * aCSCNorASCN - [IN] 설정할 CommitSCN 혹은 AbortSCN(GSCN)
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::markSCN4ReCycle( idvSQL   * aStatistics,
                                        scSpaceID  aSpaceID,
                                        scPageID   aSegPID,
                                        sdRID      aFstExtRID,
                                        sdRID      aLstExtRID,
                                        smSCN    * aCSCNorASCN )
{
    sdpSegMgmtOp   * sSegMgmtOp;

    IDE_ASSERT( aSegPID     != SD_NULL_PID );
    IDE_ASSERT( aFstExtRID  != SD_NULL_RID );
    IDE_ASSERT( aLstExtRID  != SD_NULL_RID );
    IDE_ASSERT( aCSCNorASCN != NULL );
    IDE_ASSERT( SM_SCN_IS_NOT_INFINITE( *aCSCNorASCN ) );
    IDE_ASSERT( SM_SCN_IS_NOT_INIT( *aCSCNorASCN ) );

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( &mUDSegDesc );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST( sSegMgmtOp->mMarkSCN4ReCycle( aStatistics,
                                            aSpaceID,
                                            aSegPID,
                                            getSegHandle(),
                                            aFstExtRID,
                                            aLstExtRID,
                                            aCSCNorASCN )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

 
/* BUG-31055 Can not reuse undo pages immediately after it is used to 
 * aborted transaction 
 * 즉시 재활용 할 수 있도록, ED들을 Shrink한다. */
IDE_RC sdcUndoSegment::shrinkExts( idvSQL            * aStatistics,
                                   sdrMtxStartInfo   * aStartInfo,
                                   scSpaceID           aSpaceID,
                                   scPageID            aSegPID,
                                   sdRID               aFstExtRID,
                                   sdRID               aLstExtRID )
{
    sdpSegMgmtOp   * sSegMgmtOp;

    IDE_ASSERT( aSegPID     != SD_NULL_PID );
    IDE_ASSERT( aFstExtRID  != SD_NULL_RID );
    IDE_ASSERT( aLstExtRID  != SD_NULL_RID );

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( &mUDSegDesc );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST( sSegMgmtOp->mShrinkExts( aStatistics,
                                       aSpaceID,
                                       aSegPID,
                                       getSegHandle(),
                                       aStartInfo,
                                       SDP_UDS_FREE_EXTDIR_LIST,
                                       aFstExtRID,
                                       aLstExtRID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* TASK-4007 [SM]PBT를 위한 기능 추가
 * Page에서 Undo Rec를 Dump하여 보여준다.
 *
 * BUG-28379 [SD] sdnbBTree::dumpNodeHdr( UChar *aPage ) 내에서
 * local Array의 ptr를 반환하고 있습니다.
 * Local Array대신 OutBuf를 받아 리턴하도록 수정합니다. */

IDE_RC sdcUndoSegment::dump( UChar *aPage ,
                             SChar *aOutBuf ,
                             UInt   aOutSize )
{
    UChar               * sSlotDirPtr;
    sdpSlotDirHdr       * sSlotDirHdr;
    sdpSlotEntry        * sSlotEntry;
    UShort                sSlotCount;
    UShort                sSlotNum;
    UShort                sOffset;
    UChar               * sUndoRecHdrPtr;
    sdcUndoRecHdrInfo     sUndoRecHdrInfo;
    UShort                sUndoRecSize;
    SChar                 sDumpBuf[ SD_PAGE_SIZE ]={0};

    IDE_DASSERT( aPage != NULL );
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aPage );
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;
    sSlotCount  = sSlotDirHdr->mSlotEntryCount;

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- Undo Page Begin ----------\n" );

    // Undo Record는 1번부터 시작됨
    for( sSlotNum=1; sSlotNum < sSlotCount; sSlotNum++ )
    {
        sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, sSlotNum);

        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%3"ID_UINT32_FMT"[%04"ID_xINT32_FMT"]..............................\n",
                             sSlotNum,
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

        sUndoRecHdrPtr = sdpPhyPage::getPagePtrFromOffset( aPage, sOffset );

        sdcUndoRecord::parseUndoRecHdr( sUndoRecHdrPtr,
                                        &sUndoRecHdrInfo );

        sUndoRecSize = getUndoRecSize( sSlotDirPtr, sSlotNum );
        ideLog::ideMemToHexStr( ((UChar*)sUndoRecHdrPtr)
                                    + SDC_UNDOREC_HDR_MAX_SIZE,
                                sUndoRecSize,
                                IDE_DUMP_FORMAT_NORMAL,
                                sDumpBuf,
                                aOutSize );


        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "mType        : %"ID_UINT32_FMT"\n"
                             "mFlag        : 0x%"ID_XINT64_FMT"\n"
                             "mTableOID    : %"ID_vULONG_FMT"\n"
                             "mWrittenSize : %"ID_UINT32_FMT"\n"
                             "mValue       : %s\n",
                             sUndoRecHdrInfo.mType,
                             sUndoRecHdrInfo.mFlag,
                             (ULong)sUndoRecHdrInfo.mTableOID,
                             sUndoRecSize,
                             sDumpBuf );
    }

    idlVA::appendFormat( aOutBuf,
                         aOutSize,
                        "----------- Undo Page End ----------\n" );

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : Undo Record Size 반환
 *
 * Undo Record 헤더에는 Undo Record 자신의 길이를 저장하지 않는다.
 * 왜냐하면 SlotEntry로 계산이 가능하기 때문이다.
 * 계산식은 자신의 SlotEntry에 저장된 오프셋 - 이전 SlotEntry에 저장된 오프셋
 * 이다.
 *
 * aUndoRecHdr - [IN] Undo Record 헤더 포인터
 *
 ***********************************************************************/
UShort  sdcUndoSegment::getUndoRecSize( UChar    * aSlotDirStartPtr,
                                        scSlotNum  aSlotNum )
{
    IDE_ASSERT( aSlotNum > 0 );
    return sdpSlotDirectory::getDistance( aSlotDirStartPtr,
                                          aSlotNum,
                                          aSlotNum - 1 );
}


/***********************************************************************
 *
 * Description : X$UDSEG
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::build4SegmentPerfV( void                * aHeader,
                                           iduFixedTableMemory * aMemory )
{
    sdpSegMgmtOp       *sSegMgmtOp;
    sdpSegInfo          sSegInfo;
    sdcUDSegInfo        sUDSegInfo;

    sUDSegInfo.mSegPID          = mUDSegDesc.mSegHandle.mSegPID;
    sUDSegInfo.mTXSegID         = mEntryPtr->mEntryIdx;

    sUDSegInfo.mCurAllocExtRID  = mCurAllocExtRID;
    sUDSegInfo.mCurAllocPID     = mCurAllocPID;

    sSegMgmtOp =
        sdpSegDescMgr::getSegMgmtOp( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_TEST_RAISE( sSegMgmtOp->mGetSegInfo(
                                    NULL,
                                    SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                    mUDSegDesc.mSegHandle.mSegPID,
                                    NULL, /* aTableHeader */
                                    &sSegInfo ) != IDE_SUCCESS,
                    ERR_INVALID_DUMP_OBJECT );

    sUDSegInfo.mSpaceID      = SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO;
    sUDSegInfo.mType         = sSegInfo.mType;
    sUDSegInfo.mState        = sSegInfo.mState;
    sUDSegInfo.mPageCntInExt = sSegInfo.mPageCntInExt;
    sUDSegInfo.mTotExtCnt    = sSegInfo.mExtCnt;
    sUDSegInfo.mTotExtDirCnt = sSegInfo.mExtDirCnt;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sUDSegInfo )
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
 * Description : X$DISK_UNDO_RECORDS
 *
 ***********************************************************************/
IDE_RC sdcUndoSegment::build4RecordPerfV( UInt                  aSegSeq,
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
    idBool                sIsSuccess;
    sdpPhyPageHdr       * sPhyPageHdr;
    sdcUndoRecHdrInfo     sUndoRecHdrInfo;
    sdcUndoRec4FT         sUndoRec4FT;
    UShort                sRecordCount;
    UShort                sSlotSeq;
    UChar               * sUndoRecHdrPtr;
    UChar               * sSlotDirPtr;

    //------------------------------------------
    // Parameter Validation
    //------------------------------------------

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    IDE_TEST_CONT( mCurAllocPID == SD_NULL_PID, CONT_NO_UNDO_PAGE );

    //------------------------------------------
    // Get Segment Info
    //------------------------------------------
    sSegMgmtOp =
        sdpSegDescMgr::getSegMgmtOp( SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );

    IDE_ERROR( sSegMgmtOp->mGetSegInfo(  NULL,
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         mUDSegDesc.mSegHandle.mSegPID,
                                         NULL, /* aTableHeader */
                                         &sSegInfo )
                == IDE_SUCCESS );

    sCurExtRID = sSegInfo.mFstExtRID;

    IDE_TEST( sSegMgmtOp->mGetExtInfo( NULL,
                                       SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                       sCurExtRID,
                                       &sExtInfo ) != IDE_SUCCESS );

    sCurPageID = SD_NULL_PID;

    IDE_TEST( sSegMgmtOp->mGetNxtAllocPage( NULL, /* idvSQL */
                                            SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                            &sSegInfo,
                                            NULL,
                                            &sCurExtRID,
                                            &sExtInfo,
                                            &sCurPageID ) != IDE_SUCCESS );

    sUndoRec4FT.mSegSeq = aSegSeq;
    sUndoRec4FT.mSegPID = mUDSegDesc.mSegHandle.mSegPID;

    while( sCurPageID != SD_NULL_PID )
    {
        IDE_TEST( sdbBufferMgr::getPage( NULL, /* idvSQL */
                                         SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_SINGLE_PAGE_READ,
                                         (UChar**)&sCurPagePtr,
                                         &sIsSuccess) != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sCurPagePtr );

        /* Type이 다르면 ExtDir이 재활용된것일수도 있다 */
        IDE_TEST_CONT( sPhyPageHdr->mPageType != SDP_PAGE_UNDO, CONT_NO_UNDO_PAGE );

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sPhyPageHdr);
        sRecordCount = sdpSlotDirectory::getCount((UChar*)sSlotDirPtr);

        for( sSlotSeq = 1; sSlotSeq < sRecordCount; sSlotSeq++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                        (UChar*)sSlotDirPtr,
                                                        sSlotSeq,
                                                        &sUndoRecHdrPtr )
                      != IDE_SUCCESS );

            sUndoRec4FT.mPageID = sCurPageID;
            IDE_TEST( sdpSlotDirectory::getValue( (UChar*)sSlotDirPtr,
                                                  sSlotSeq,
                                                  &sUndoRec4FT.mOffset )
                      != IDE_SUCCESS );

            sUndoRec4FT.mNthSlot = sSlotSeq;

            sdcUndoRecord::parseUndoRecHdr( sUndoRecHdrPtr,
                                            &sUndoRecHdrInfo );

            sUndoRec4FT.mSize = getUndoRecSize( sSlotDirPtr, sSlotSeq );
            sUndoRec4FT.mType = sUndoRecHdrInfo.mType;
            sUndoRec4FT.mFlag = sUndoRecHdrInfo.mFlag;
            sUndoRec4FT.mTableOID = (ULong)sUndoRecHdrInfo.mTableOID;


            IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                  aMemory,
                                                 (void *) &sUndoRec4FT )
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

    IDE_EXCEPTION_CONT( CONT_NO_UNDO_PAGE );

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
 *
 * Description : 기존의 할당 정보로 복구
 *
 *               MtxRollback을 할 경우, DiskPage는 복구되지만 Runtime
 *               정보는 복구되지 않는다. 따라서 복구하기 위해
 *               값을 저장해두고, MtxRollback시 복구한다.
 *
 * [IN] aIsCommitJob         - 이것이 Commit을 위한 작업이냐, 아니냐
 * [IN] aUndoSegment         - Abort하려는 UndoSegment
 *
 **********************************************************************/
IDE_RC sdcUndoSegment::abortCurAllocInfo( void * aUndoSegment )
{
    sdcUndoSegment   * sUndoSegment;
    
    IDE_ASSERT( aUndoSegment != NULL );

    sUndoSegment = ( sdcUndoSegment * ) aUndoSegment;

    sUndoSegment->mCurAllocExtRID = 
        sUndoSegment->mCurAllocExtRID4MtxRollback;
    sUndoSegment->mFstPIDOfCurAllocExt = 
        sUndoSegment->mFstPIDOfCurAllocExt4MtxRollback;
    sUndoSegment->mCurAllocPID =
        sUndoSegment->mCurAllocPID4MtxRollback;

    return IDE_SUCCESS;
}



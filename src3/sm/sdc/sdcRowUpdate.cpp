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
 

#include <smDef.h>
#include <smErrorCode.h>
#include <smcTable.h>
#include <smxTransMgr.h>
#include <sdcReq.h>
#include <sdcRowUpdate.h>
#include <sdcUndoRecord.h>
#include <sdcTSSlot.h>
#include <sdp.h>


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE( SChar       * aLogPtr,
                                                    UInt          /*aSize*/,
                                                    UChar       * aSlotPtr,
                                                    sdrRedoInfo * aRedoInfo,
                                                    sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr    *sPageHdr;
    UChar            *sAllocSlotPtr = NULL;
    sdSID             sAllocSlotSID;
    UShort            sRowPieceSize;
    scSlotNum         sSlotNumBeforeCrash   = 0;
    scSlotNum         sSlotNumAfterCrash    = 0;
    idBool            sCanAlloc;

    IDE_ERROR( aLogPtr  != NULL );
    IDE_ERROR( aSlotPtr != NULL );
    IDE_ERROR( aRedoInfo!= NULL );
    IDE_ERROR_MSG( (aRedoInfo->mLogType == SDR_SDC_INSERT_ROW_PIECE) ||
                   (aRedoInfo->mLogType == SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE) ,
                   "mLogType : %"ID_UINT32_FMT, aRedoInfo->mLogType );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sRowPieceSize = sdcRow::getRowPieceSize((UChar*)aLogPtr);

    sCanAlloc =  sdpPhyPage::canAllocSlot( sPageHdr,
                                           sRowPieceSize,
                                           ID_TRUE,
                                           1 ); /* Use Key Slot */

    IDE_ERROR( sCanAlloc == ID_TRUE );

    IDE_TEST( sdpPageList::allocSlot( (UChar*)sPageHdr,
                                      sRowPieceSize,
                                      (UChar**)&sAllocSlotPtr,
                                      &sAllocSlotSID )
              != IDE_SUCCESS );

    IDE_ERROR_MSG( sAllocSlotPtr != NULL,
                   "sAllocSlotPtr : %"ID_UINT64_FMT, sAllocSlotPtr );

    sSlotNumBeforeCrash = aRedoInfo->mSlotNum;
    sSlotNumAfterCrash  = SD_MAKE_SLOTNUM(sAllocSlotSID);

    /* recovery는 redo all, undo all 이므로,
     * restart redo시에 할당한 slot num는
     * crash 이전에 할당한 slot num와 반드시 같아야 한다.
     * */
    IDE_ERROR_MSG( sSlotNumBeforeCrash == sSlotNumAfterCrash,
                   " sSlotNumBeforeCrash : %"ID_UINT32_FMT"\n"
                   " sSlotNumAfterCrash  : %"ID_UINT32_FMT,
                   sSlotNumBeforeCrash,
                   sSlotNumAfterCrash );

    idlOS::memcpy( sAllocSlotPtr,
                   aLogPtr,
                   sRowPieceSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO( SChar       * aLogPtr,
                                                                   UInt          /*aSize*/,
                                                                   UChar       * aSlotPtr,
                                                                   sdrRedoInfo * aRedoInfo,
                                                                   sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr    *sPageHdr;
    UChar            *sAllocSlotPtr;
    UShort            sRowPieceSize;
    idBool            sCanAlloc;
    scSlotNum         sAllocSlotNum;
    scOffset          sAllocSlotOffset;

    IDE_ERROR( aLogPtr  != NULL );
    IDE_ERROR( aSlotPtr != NULL );
    IDE_ERROR( aRedoInfo!= NULL );
    IDE_ERROR_MSG( aRedoInfo->mLogType ==
                   SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO,
                   "mLogType : %"ID_UINT32_FMT, aRedoInfo->mLogType );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sRowPieceSize = sdcRow::getRowPieceSize((UChar*)aLogPtr);

    sCanAlloc =  sdpPhyPage::canAllocSlot( sPageHdr,
                                           sRowPieceSize,
                                           ID_TRUE,  /* Use Key Slot */
                                           SDP_1BYTE_ALIGN_SIZE );
    IDE_ERROR( sCanAlloc == ID_TRUE );

    /* BUG-23989
     * [TSM] delete rollback에 대한
     * restart redo가 정상적으로 수행되지 않습니다. */

    /* delete undo 연산에 대한 CLR일 경우,
     * 로그에 적혀있는 slot num로 allocSlot() 한후,
     * 할당한 slot에 old image를 write해야 한다.
     *
     * 그런데 sdpPageList::allocSlot() 함수는
     * 내부적으로 sdpSlotDirectory::findUnusedSlotEntry()를 하여
     * minimum slot entry를 구한다.
     *
     * 그러므로 이 함수를 호출하면 안되고
     * sdpPhyPage::allocSlot4SID()를 직접 호출해야 한다. */

    IDE_TEST( sdpPhyPage::allocSlot4SID( sPageHdr,
                                         sRowPieceSize,
                                         (UChar**)&sAllocSlotPtr,
                                         &sAllocSlotNum,
                                         &sAllocSlotOffset )
              != IDE_SUCCESS );

    IDE_ERROR_MSG( aRedoInfo->mSlotNum == sAllocSlotNum,
                   " aRedoInfo->mSlotNum : %"ID_UINT32_FMT"\n"
                   " sAllocSlotNum       : %"ID_UINT32_FMT,
                   aRedoInfo->mSlotNum,
                   sAllocSlotNum );
    IDE_ERROR( sAllocSlotPtr != NULL );

    idlOS::memcpy( sAllocSlotPtr,
                   aLogPtr,
                   sRowPieceSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_INSERT_ROW_PIECE( idvSQL   * aStatistics,
                                                    smTID      aTransID,
                                                    smOID      aOID,
                                                    scGRID     aRecGRID,
                                                    SChar    * aLogPtr,
                                                    smLSN    * aPrevLSN )
{
    sdrMtx          sMtx;
    idBool          sDummy;
    void           *sTableHeader;
    UChar          *sSlotPtr;
    UInt            sState = 0;
    sdpPhyPageHdr  *sPageHdr;
    sdcRowHdrInfo   sRowHdrInfo;
    sdrLogHdr       sLogHdr;

    IDE_DASSERT( aOID        != SM_NULL_OID );
    IDE_DASSERT( aLogPtr     != NULL );
    IDE_DASSERT( aPrevLSN    != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_INSERT_ROW_PIECE TBS 상태 체크 위치 조정 
    if( sctTableSpaceMgr::hasState(SC_MAKE_SPACE(aRecGRID),
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               (UChar**)&sSlotPtr,
                                               &sDummy )
                  != IDE_SUCCESS );

        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

        sPageHdr = sdpPhyPage::getHdr( sSlotPtr );

        if( sdpPhyPage::isConsistentPage( (UChar*)sPageHdr ) == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );

            if ( SDC_IS_HEAD_ROWPIECE( sRowHdrInfo.mRowFlag ) == ID_TRUE )
            {
                ID_READ_VALUE( aLogPtr, &sLogHdr, ID_SIZEOF(sdrLogHdr) );
                IDE_ERROR( sLogHdr.mType !=
                           SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE );

                if (smrRecoveryMgr::isRestart() == ID_FALSE)
                {
                    IDE_TEST( smLayerCallback::undoInsertOfTableInfo(
                                                smxTransMgr::getTransByTID( aTransID ),
                                                aOID )
                               != IDE_SUCCESS );
                }
            }

            IDE_ERROR( smLayerCallback::getTableHeaderFromOID( aOID,
                                                               (void**)&sTableHeader )
                       == IDE_SUCCESS );
            IDE_DASSERT( sTableHeader != NULL );

            IDE_TEST( sdcTableCTL::unbind( &sMtx,
                                           sSlotPtr,
                                           sRowHdrInfo.mCTSlotIdx,
                                           SDP_CTS_IDX_UNLK, /* aCTSlotIdxAftUndo */
                                           0,                /* aFSCreditSize     */
                                           ID_FALSE )        /* aDecDeleteRowCnt  */
                      != IDE_SUCCESS );

            IDE_TEST( sdcRow::free( aStatistics,
                                    &sMtx,
                                    sTableHeader,
                                    aRecGRID,
                                    sSlotPtr )
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );
        }
        
        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                    == IDE_SUCCESS );
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_UPDATE_ROW_PIECE( SChar       * aLogPtr,
                                                    UInt          /*aSize*/,
                                                    UChar       * aSlotPtr,
                                                    sdrRedoInfo * aRedoInfo,
                                                    sdrMtx      * /*aMtx*/)
{
    sdpPhyPageHdr     *sPageHdr;
    UChar             *sSlotPtr;
    scSlotNum          sSlotNum;
    sdSID              sSlotSID;
    UChar              sFlag;
    UChar             *sNewSlotPtr;

    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aSlotPtr != NULL );
    IDE_DASSERT( aRedoInfo!= NULL );
    IDE_DASSERT( aRedoInfo->mLogType == SDR_SDC_UPDATE_ROW_PIECE );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sSlotPtr)
              != IDE_SUCCESS );

    IDE_ERROR( sSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

    ID_READ_1B_VALUE( aLogPtr, &sFlag );

    if( (sFlag & SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_MASK) ==
        SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_INPLACE )
    {
        IDE_ERROR( sdcRow::redo_undo_UPDATE_INPLACE_ROW_PIECE( 
                                                   NULL,     /* aMtx */
                                                   (UChar*)aLogPtr, 
                                                   sSlotPtr,
                                                   SDC_REDO_MAKE_NEWROW )
                   == IDE_SUCCESS );
    }
    else
    {
        sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sSlotNum );

        IDE_ERROR( sdcRow::redo_undo_UPDATE_OUTPLACE_ROW_PIECE(
                                           NULL,     /* aMtx */
                                           (UChar*)aLogPtr,
                                           sSlotPtr,
                                           sSlotSID,
                                           SDC_REDO_MAKE_NEWROW,
                                           NULL,     /* aRowBuf4MVCC */
                                           &sNewSlotPtr,
                                           NULL )    /* aFSCreditSize */
                    == IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_UPDATE_ROW_PIECE( idvSQL  * aStatistics,
                                                    smTID     aTransID,
                                                    smOID     aOID,
                                                    scGRID    aRecGRID,
                                                    SChar   * /*aLogPtr*/,
                                                    smLSN   * aPrevLSN )
{
    sdrMtx               sMtx;
    sdSID                sSlotSID;
    UChar               *sSlotPtr;
    UChar               *sNewSlotPtr;
    sdpPageListEntry    *sEntry;
    UChar               *sUndoRecHdr;
    UChar               *sUndoRecBodyPtr;
    SShort               sChangeSize   = 0;
    SShort               sFSCreditSize = 0;
    sdcUndoRecFlag       sFlag;
    void                *sTableHeader;
    UInt                 sState = 0;
    idBool               sDummy;
    sdpPhyPageHdr       *sPageHdr;
    sdcRowHdrInfo        sRowHdrInfo;

    IDE_DASSERT( aOID        != SM_NULL_OID );
    IDE_DASSERT( aPrevLSN    != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_UPDATE_ROW_PIECE TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(SC_MAKE_SPACE(aRecGRID),
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               &sSlotPtr,
                                               &sDummy ) != IDE_SUCCESS );

        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

        sPageHdr = sdpPhyPage::getHdr( sSlotPtr );

        if( sdpPhyPage::isConsistentPage( (UChar*)sPageHdr ) == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            IDE_ERROR( sRowHdrInfo.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID( 
                                            aStatistics,
                                            SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                            sRowHdrInfo.mUndoSID,
                                            SDB_X_LATCH,
                                            SDB_WAIT_NORMAL,
                                            NULL,
                                            (UChar**)&sUndoRecHdr,
                                            &sDummy ) 
                      != IDE_SUCCESS );
            sState = 2;

            sUndoRecBodyPtr = sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr);

            ID_READ_VALUE( sUndoRecBodyPtr, &sFlag, ID_SIZEOF(sFlag) );

            sdcUndoRecord::parseUpdateChangeSize( sUndoRecHdr,
                                                  &sChangeSize );

            if( (sFlag & SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_MASK) ==
                SDC_UPDATE_LOG_FLAG_UPDATE_METHOD_INPLACE )
            {
                IDE_ERROR_MSG( sChangeSize == 0,
                               "sChangeSize : %"ID_UINT32_FMT, sChangeSize );

                IDE_TEST( sdcRow::redo_undo_UPDATE_INPLACE_ROW_PIECE( 
                                                    &sMtx,
                                                    sUndoRecBodyPtr,
                                                    sSlotPtr,
                                                    SDC_UNDO_MAKE_OLDROW ) 
                          != IDE_SUCCESS );

                sNewSlotPtr = sSlotPtr;
            }
            else
            {
                sSlotSID = SD_MAKE_SID_FROM_GRID( aRecGRID );

                IDE_TEST( sdcRow::redo_undo_UPDATE_OUTPLACE_ROW_PIECE(
                                                    &sMtx,
                                                    sUndoRecBodyPtr,
                                                    sSlotPtr,
                                                    sSlotSID,
                                                    SDC_UNDO_MAKE_OLDROW,
                                                    NULL,          /* aRowBuf4MVCC */
                                                    &sNewSlotPtr,
                                                    &sFSCreditSize ) 
                           != IDE_SUCCESS );

                if ( sFSCreditSize != 0 )
                {
                    IDE_ERROR( smLayerCallback::getTableHeaderFromOID( aOID,
                                                                       (void**)&sTableHeader )
                               == IDE_SUCCESS );
                    IDE_DASSERT( sTableHeader != NULL );

                    sEntry = (sdpPageListEntry*)smcTable::getDiskPageListEntry(sTableHeader);
                    IDE_DASSERT(sEntry != NULL);

                    // reallocSlot을 한이후에,
                    // Segment에 대한 가용도 변경연산을 수행한다.
                    IDE_TEST( sdpPageList::updatePageState( aStatistics,
                                                            SC_MAKE_SPACE(aRecGRID),
                                                            sEntry,
                                                            sdpPhyPage::getPageStartPtr(sSlotPtr),
                                                            &sMtx )
                              != IDE_SUCCESS );
                }
            }

            IDE_TEST( sdcRow::writeUpdateRowPieceCLR( sUndoRecHdr,
                                                      aRecGRID,
                                                      sRowHdrInfo.mUndoSID,
                                                      &sMtx )
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sdcUndoRecord::setInvalid( sUndoRecHdr );

            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sUndoRecHdr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sUndoRecHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_OVERWRITE_ROW_PIECE( SChar       * aLogPtr,
                                                       UInt          /*aSize*/,
                                                       UChar       * aSlotPtr,
                                                       sdrRedoInfo * aRedoInfo,
                                                       sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr     *sPageHdr;
    UChar             *sSlotPtr;
    scSlotNum          sSlotNum;
    sdSID              sSlotSID;
    UChar             *sNewSlotPtr;

    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aSlotPtr != NULL );
    IDE_DASSERT( aRedoInfo!= NULL );
    IDE_DASSERT( aRedoInfo->mLogType == SDR_SDC_OVERWRITE_ROW_PIECE );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sSlotPtr)
              != IDE_SUCCESS);

    IDE_ERROR( sSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

    sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sSlotNum );
    IDE_ERROR( sdcRow::redo_undo_OVERWRITE_ROW_PIECE(
                                              NULL, /* aMtx */
                                              (UChar*)aLogPtr,
                                              sSlotPtr,
                                              sSlotSID,
                                              SDC_REDO_MAKE_NEWROW,
                                              NULL, /* aRowBuf4MVCC */
                                              &sNewSlotPtr,
                                              NULL )/* aFSCreditSize*/
               == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_OVERWRITE_ROW_PIECE( idvSQL * aStatistics,
                                                       smTID    aTransID,
                                                       smOID    aOID,
                                                       scGRID   aRecGRID,
                                                       SChar  * /*aLogPtr*/,
                                                       smLSN  * aPrevLSN )
{
    sdrMtx             sMtx;
    sdSID              sSlotSID;
    UChar            * sSlotPtr;
    UChar            * sNewSlotPtr;
    idBool             sDummy;
    void             * sTableHeader;
    sdpPageListEntry * sEntry;
    UChar            * sUndoRecHdr;
    SShort             sFSCreditSize = 0;
    UInt               sState = 0;
    sdpPhyPageHdr    * sPageHdr;
    sdcRowHdrInfo      sRowHdrInfo;

    IDE_DASSERT( aOID     != SM_NULL_OID );
    IDE_DASSERT( aPrevLSN != NULL );

    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_OVERWRITE_ROW_PIECE TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState( SC_MAKE_SPACE(aRecGRID),
                                    SCT_SS_SKIP_UNDO ) == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               (UChar**)&sSlotPtr,
                                               &sDummy )
                  != IDE_SUCCESS );

        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );
        sPageHdr = sdpPhyPage::getHdr( sSlotPtr );

        if ( sdpPhyPage::isConsistentPage( (UChar*)sPageHdr )
             == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            IDE_ERROR( sRowHdrInfo.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID( 
                                                aStatistics,
                                                SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                sRowHdrInfo.mUndoSID,
                                                SDB_X_LATCH,
                                                SDB_WAIT_NORMAL,
                                                NULL,
                                                (UChar**)&sUndoRecHdr,
                                                &sDummy ) 
                       != IDE_SUCCESS );
            sState = 2;

            sSlotSID = SD_MAKE_SID_FROM_GRID( aRecGRID );

            IDE_TEST( sdcRow::redo_undo_OVERWRITE_ROW_PIECE(
                                    &sMtx,
                                    sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr),
                                    sSlotPtr,
                                    sSlotSID,
                                    SDC_UNDO_MAKE_OLDROW,
                                    NULL, /* aRowBuf4MVCC */
                                    &sNewSlotPtr,
                                    &sFSCreditSize )
                       != IDE_SUCCESS );

            if( sFSCreditSize != 0 )
            {
                IDE_ERROR( smLayerCallback::getTableHeaderFromOID( aOID,
                                                                   (void**)&sTableHeader )
                           == IDE_SUCCESS );
                IDE_DASSERT( sTableHeader != NULL );

                sEntry = (sdpPageListEntry*)smcTable::getDiskPageListEntry(sTableHeader);
                IDE_DASSERT(sEntry != NULL);

                // reallocSlot을 한이후에, Segment에 대한
                // 가용도 변경연산을 수행한다.
                IDE_TEST( sdpPageList::updatePageState(
                                                    aStatistics,
                                                    SC_MAKE_SPACE(aRecGRID),
                                                    sEntry,
                                                    sdpPhyPage::getPageStartPtr(sSlotPtr),
                                                    &sMtx )
                           != IDE_SUCCESS );
            }

            IDE_TEST( sdcRow::writeOverwriteRowPieceCLR(
                                                    sUndoRecHdr,
                                                    aRecGRID,
                                                    SD_MAKE_SLOTNUM(sRowHdrInfo.mUndoSID),
                                                    &sMtx )
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sdcUndoRecord::setInvalid( sUndoRecHdr );

            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sUndoRecHdr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sUndoRecHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_CHANGE_ROW_PIECE_LINK( SChar       * aLogPtr,
                                                         UInt          /*aSize*/,
                                                         UChar       * aSlotPtr,
                                                         sdrRedoInfo * aRedoInfo,
                                                         sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr     *sPageHdr;
    UChar             *sOldSlotPtr;
    UChar             *sNewSlotPtr;
    UChar             *sLogPtr          = (UChar*)aLogPtr;
    scSlotNum          sSlotNum;
    sdSID              sSlotSID;
    UChar              sLogFlag;
    SShort             sChangeSize;
    UShort             sColCount;
    UShort             sColSeqInRowPiece;
    UInt               sLength;
    UChar              sOldSlotImage[SD_PAGE_SIZE];
    UChar             *sOldSlotImagePtr;
    UShort             sOldRowPieceSize;
    UShort             sNewRowPieceSize;
    scPageID           sNxtPageID;
    scSlotNum          sNxtSlotNum;
    idBool             sReserveFreeSpaceCredit;

    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aSlotPtr != NULL );
    IDE_DASSERT( aRedoInfo!= NULL );
    IDE_DASSERT( aRedoInfo->mLogType == SDR_SDC_CHANGE_ROW_PIECE_LINK );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sOldSlotPtr )
              != IDE_SUCCESS );

    IDE_ERROR( sOldSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sOldSlotPtr) != ID_TRUE );

    ID_READ_1B_AND_MOVE_PTR( sLogPtr, &sLogFlag );

    ID_READ_AND_MOVE_PTR( sLogPtr,
                          &sChangeSize,
                          ID_SIZEOF(sChangeSize) );
    IDE_ERROR_MSG( (sChangeSize == 0) ||
                   (sChangeSize ==  SDC_EXTRASIZE_FOR_CHAINING) ||
                   (sChangeSize == (SShort)(-SDC_EXTRASIZE_FOR_CHAINING)),
                   "sChangeSize : %"ID_UINT32_FMT,
                   sChangeSize );

    if( sChangeSize == 0 )
    {
        ID_WRITE_AND_MOVE_BOTH( sOldSlotPtr,
                                sLogPtr,
                                SDC_ROWHDR_SIZE );

        ID_READ_VALUE( sLogPtr, &sNxtPageID, ID_SIZEOF(sNxtPageID) );
        IDE_ERROR( sNxtPageID  != SC_NULL_PID );
        ID_WRITE_AND_MOVE_BOTH( sOldSlotPtr, sLogPtr, ID_SIZEOF(scPageID) );

        ID_WRITE_AND_MOVE_BOTH( sOldSlotPtr, sLogPtr, ID_SIZEOF(scSlotNum) );
    }
    else
    {
        sOldRowPieceSize = sdcRow::getRowPieceSize(sOldSlotPtr);
        SDC_GET_ROWHDR_FIELD(sOldSlotPtr, SDC_ROWHDR_COLCOUNT, &sColCount);

        sOldSlotImagePtr = sOldSlotImage;
        idlOS::memcpy( sOldSlotImagePtr,
                       sOldSlotPtr,
                       sOldRowPieceSize );

        /*
         * ###   FSC 플래그   ###
         *
         * DML 연산중에는 당연히 FSC를 reserve 해야 한다.
         * 그러면 redo나 undo시에는 어떻게 해야 하나?
         *
         * redo는 DML 연산을 다시 수행하는 것이므로,
         * DML 연산할때와 동일하게 FSC를 reserve 해야 한다.
         *
         * 반면 undo시에는 FSC를 reserve하면 안된다.
         * 왜나하면 FSC는 DML 연산을 undo시킬때를 대비해서
         * 공간을 예약해두는 것이므로,
         * undo시에는 이전에 reserve해둔 FSC를
         * 페이지에 되돌려(restore)주어야 하고,
         * undo시에 또 다시 FSC를 reserve하려고 해서는 안된다.
         *
         * clr은 undo에 대한 redo이므로 undo때와 동일하게
         * FSC를 reserve하면 안된다.
         *
         * 이 세가지 경우를 구분하여
         * FSC reserve 처리를 해야 하는데,
         * 나(upinel9)는 로그를 기록할때 FSC reserve 여부를 플래그로 남겨서,
         * redo나 undo시에는 이 플래그만 보고
         * reallocSlot()을 하도록 설계하였다.
         *
         * redo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE
         * undo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
         * clr redo : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
         */
        if( (sLogFlag & SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_MASK) ==
            SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE )
        {
            sReserveFreeSpaceCredit = ID_TRUE;
        }
        else
        {
            sReserveFreeSpaceCredit = ID_FALSE;
        }

        sNewRowPieceSize = sOldRowPieceSize + (sChangeSize);

        sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sSlotNum );

        IDE_TEST( sdcRow::reallocSlot( sSlotSID,
                                       sOldSlotPtr,
                                       sNewRowPieceSize,
                                       sReserveFreeSpaceCredit,
                                       &sNewSlotPtr )
                  != IDE_SUCCESS );
        sOldSlotPtr = NULL;
        IDE_DASSERT( sNewSlotPtr != NULL );

        ID_WRITE_AND_MOVE_BOTH( sNewSlotPtr, sLogPtr, SDC_ROWHDR_SIZE );

        if( sChangeSize == SDC_EXTRASIZE_FOR_CHAINING )
        {
            ID_READ_VALUE( sLogPtr, &sNxtPageID, ID_SIZEOF(sNxtPageID) );
            IDE_ERROR( sNxtPageID  != SC_NULL_PID );
            ID_WRITE_AND_MOVE_BOTH( sNewSlotPtr, sLogPtr, ID_SIZEOF(scPageID) );

            ID_WRITE_AND_MOVE_BOTH( sNewSlotPtr, sLogPtr, ID_SIZEOF(scSlotNum) );
        }
        else
        {
            IDE_ERROR_MSG( sChangeSize == (SShort)(-SDC_EXTRASIZE_FOR_CHAINING),
                           "sChangeSize : %"ID_UINT32_FMT, sChangeSize );

            ID_READ_AND_MOVE_PTR( sLogPtr, &sNxtPageID, ID_SIZEOF(sNxtPageID) );
            IDE_ERROR_MSG( sNxtPageID  == SC_NULL_PID,
                           "sNxtPageID : %"ID_UINT32_FMT, sNxtPageID );

            ID_READ_AND_MOVE_PTR( sLogPtr, &sNxtSlotNum, ID_SIZEOF(sNxtSlotNum) );
            IDE_ERROR_MSG( sNxtSlotNum == SC_NULL_SLOTNUM,
                           "sNxtSlotNum : %"ID_UINT32_FMT, sNxtSlotNum );
        }

        sOldSlotImagePtr = sdcRow::getDataArea(sOldSlotImagePtr);

        for( sColSeqInRowPiece = 0;
             sColSeqInRowPiece < sColCount;
             sColSeqInRowPiece++ )
        {
            sLength = sdcRow::getColPieceLen(sOldSlotImagePtr);

            ID_WRITE_AND_MOVE_BOTH( sNewSlotPtr,
                                    sOldSlotImagePtr,
                                    SDC_GET_COLPIECE_SIZE(sLength) );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_CHANGE_ROW_PIECE_LINK( idvSQL * aStatistics,
                                                         smTID    aTransID,
                                                         smOID    aOID,
                                                         scGRID   aRecGRID,
                                                         SChar  * /*aLogPtr*/,
                                                         smLSN  * aPrevLSN )
{
    sdrMtx              sMtx;
    sdSID               sSlotSID;
    UChar             * sSlotPtr;
    idBool              sDummy;
    void              * sTableHeader;
    sdpPageListEntry  * sEntry;
    UChar             * sUndoRecHdr;
    SShort              sFSCreditSize = 0;
    UChar             * sNewSlotPtr;
    UInt                sState = 0;
    sdpPhyPageHdr     * sPageHdr;
    sdcRowHdrInfo       sRowHdrInfo;

    IDE_DASSERT( aOID        != SM_NULL_OID );
    IDE_DASSERT( aPrevLSN    != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_CHANGE_ROW_PIECE_LINK TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState( SC_MAKE_SPACE(aRecGRID),
                                    SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               (UChar**)&sSlotPtr,
                                               &sDummy )
                  != IDE_SUCCESS );

        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

        sPageHdr = sdpPhyPage::getHdr( sSlotPtr );
        if( sdpPhyPage::isConsistentPage( (UChar*)sPageHdr ) == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            IDE_ERROR( sRowHdrInfo.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID( 
                                                aStatistics,
                                                SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                sRowHdrInfo.mUndoSID,
                                                SDB_X_LATCH,
                                                SDB_WAIT_NORMAL,
                                                NULL,
                                                (UChar**)&sUndoRecHdr,
                                                &sDummy ) 
                      != IDE_SUCCESS );
            sState = 2;

            sSlotSID = SD_MAKE_SID_FROM_GRID( aRecGRID );

            IDE_TEST( sdcRow::undo_CHANGE_ROW_PIECE_LINK(
                                    &sMtx,
                                    sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr),
                                    sSlotPtr,
                                    sSlotSID,
                                    SDC_UNDO_MAKE_OLDROW,
                                    NULL, /* aRowBuf4MVCC */
                                    &sNewSlotPtr,
                                    &sFSCreditSize ) 
                       != IDE_SUCCESS );

            if( sFSCreditSize != 0 )
            {
                IDE_ERROR( smLayerCallback::getTableHeaderFromOID( aOID,
                                                                   (void**)&sTableHeader )
                           == IDE_SUCCESS );
                IDE_DASSERT( sTableHeader != NULL );

                sEntry = (sdpPageListEntry*)smcTable::getDiskPageListEntry(sTableHeader);
                IDE_DASSERT(sEntry != NULL);

                // reallocSlot을 한이후에, Segment에 대한
                // 가용도 변경연산을 수행한다.
                IDE_TEST( sdpPageList::updatePageState(
                                                    aStatistics,
                                                    SC_MAKE_SPACE(aRecGRID),
                                                    sEntry,
                                                    sdpPhyPage::getPageStartPtr(sSlotPtr),
                                                    &sMtx ) 
                          != IDE_SUCCESS );
            }

            IDE_TEST( sdcRow::writeChangeRowPieceLinkCLR( sUndoRecHdr,
                                                          aRecGRID,
                                                          sRowHdrInfo.mUndoSID,
                                                          &sMtx )
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sdcUndoRecord::setInvalid( sUndoRecHdr );

            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sUndoRecHdr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sUndoRecHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE( SChar       * aLogPtr,
                                                             UInt          /*aSize*/,
                                                             UChar       * aSlotPtr,
                                                             sdrRedoInfo * aRedoInfo,
                                                             sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr     *sPageHdr;
    UChar              sOldSlotImage[SD_PAGE_SIZE];
    UChar             *sOldSlotImagePtr;
    UChar             *sOldSlotPtr;
    UChar             *sNewSlotPtr;
    UChar             *sLogPtr          = (UChar*)aLogPtr;
    scSlotNum          sSlotNum;
    sdSID              sSlotSID;
    UShort             sOldRowPieceSize;
    UShort             sNewRowPieceSize;
    UChar              sFlag;
    SShort             sChangeSize;
    UShort             sColCount;
    UShort             sColSeq;
    UChar              sRowFlag;
    UInt               sLength;
    idBool             sReserveFreeSpaceCredit;

    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aSlotPtr != NULL );
    IDE_DASSERT( aRedoInfo!= NULL );
    IDE_DASSERT( aRedoInfo->mLogType == SDR_SDC_DELETE_FIRST_COLUMN_PIECE );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sOldSlotPtr )
               == IDE_SUCCESS);

    IDE_ERROR( sOldSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sOldSlotPtr) != ID_TRUE );

    sOldRowPieceSize = sdcRow::getRowPieceSize(sOldSlotPtr);
    SDC_GET_ROWHDR_FIELD(sOldSlotPtr, SDC_ROWHDR_COLCOUNT, &sColCount);
    SDC_GET_ROWHDR_1B_FIELD(sOldSlotPtr, SDC_ROWHDR_FLAG, sRowFlag);

    sOldSlotImagePtr = sOldSlotImage;
    idlOS::memcpy( sOldSlotImagePtr,
                   sOldSlotPtr,
                   sOldRowPieceSize );

    ID_READ_1B_AND_MOVE_PTR( sLogPtr, &sFlag );

    ID_READ_AND_MOVE_PTR( sLogPtr,
                           &sChangeSize,
                           ID_SIZEOF(sChangeSize) );

    sNewRowPieceSize = sOldRowPieceSize + (sChangeSize);

    /*
     * ###   FSC 플래그   ###
     *
     * DML 연산중에는 당연히 FSC를 reserve 해야 한다.
     * 그러면 redo나 undo시에는 어떻게 해야 하나?
     *
     * redo는 DML 연산을 다시 수행하는 것이므로,
     * DML 연산할때와 동일하게 FSC를 reserve 해야 한다.
     *
     * 반면 undo시에는 FSC를 reserve하면 안된다.
     * 왜나하면 FSC는 DML 연산을 undo시킬때를 대비해서
     * 공간을 예약해두는 것이므로,
     * undo시에는 이전에 reserve해둔 FSC를
     * 페이지에 되돌려(restore)주어야 하고,
     * undo시에 또 다시 FSC를 reserve하려고 해서는 안된다.
     *
     * clr은 undo에 대한 redo이므로 undo때와 동일하게
     * FSC를 reserve하면 안된다.
     *
     * 이 세가지 경우를 구분하여
     * FSC reserve 처리를 해야 하는데,
     * 나(upinel9)는 로그를 기록할때 FSC reserve 여부를 플래그로 남겨서,
     * redo나 undo시에는 이 플래그만 보고
     * reallocSlot()을 하도록 설계하였다.
     *
     * redo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE
     * undo     : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
     * clr redo : SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_FALSE
     */
    if( (sFlag & SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_MASK) ==
        SDC_UPDATE_LOG_FLAG_RESERVE_FREESPACE_CREDIT_TRUE )
    {
        sReserveFreeSpaceCredit = ID_TRUE;
    }
    else
    {
        sReserveFreeSpaceCredit = ID_FALSE;
    }

    sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sSlotNum );

    IDE_TEST( sdcRow::reallocSlot( sSlotSID,
                                   sOldSlotPtr,
                                   sNewRowPieceSize,
                                   sReserveFreeSpaceCredit,
                                   &sNewSlotPtr )
              != IDE_SUCCESS );
    sOldSlotPtr = NULL;
    IDE_DASSERT( sNewSlotPtr != NULL );

    idlOS::memcpy( sNewSlotPtr,
                   sLogPtr,
                   SDC_ROWHDR_SIZE );

    SDC_MOVE_PTR_TRIPLE( sNewSlotPtr,
                         sLogPtr,
                         sOldSlotImagePtr,
                         SDC_ROWHDR_SIZE );

    if( SDC_IS_LAST_ROWPIECE(sRowFlag) != ID_TRUE )
    {
        ID_WRITE_AND_MOVE_BOTH( sNewSlotPtr,
                                sOldSlotImagePtr,
                                SDC_EXTRASIZE_FOR_CHAINING );
    }

    // skip first column piece
    sOldSlotImagePtr = sdcRow::getNxtColPiece(sOldSlotImagePtr);

    for( sColSeq = 1; sColSeq < sColCount; sColSeq++ )
    {
        sLength = sdcRow::getColPieceLen(sOldSlotImagePtr);

        ID_WRITE_AND_MOVE_BOTH( sNewSlotPtr,
                                sOldSlotImagePtr,
                                SDC_GET_COLPIECE_SIZE(sLength) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE( idvSQL * aStatistics,
                                                             smTID    aTransID,
                                                             smOID    aOID,
                                                             scGRID   aRecGRID,
                                                             SChar  * /*aLogPtr*/,
                                                             smLSN  * aPrevLSN )
{
    sdrMtx              sMtx;
    sdSID               sSlotSID;
    UChar             * sSlotPtr;
    UChar             * sNewSlotPtr;
    idBool              sDummy;
    UInt                sState = 0;
    void              * sTableHeader;
    sdpPageListEntry  * sEntry;
    UChar             * sUndoRecHdr;
    sdpPhyPageHdr     * sPageHdr;
    sdcRowHdrInfo       sRowHdrInfo;

    IDE_DASSERT( aOID        != SM_NULL_OID );
    IDE_DASSERT( aPrevLSN    != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(SC_MAKE_SPACE(aRecGRID),
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               (UChar**)&sSlotPtr,
                                               &sDummy )
                  != IDE_SUCCESS );

        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

        sPageHdr = sdpPhyPage::getHdr( sSlotPtr );
        if( sdpPhyPage::isConsistentPage( (UChar*)sPageHdr ) == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            IDE_ERROR( sRowHdrInfo.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID(
                                                aStatistics,
                                                SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                sRowHdrInfo.mUndoSID,
                                                SDB_X_LATCH,
                                                SDB_WAIT_NORMAL,
                                                NULL,
                                                (UChar**)&sUndoRecHdr,
                                                &sDummy )
                      != IDE_SUCCESS );
            sState = 2;

            IDE_ERROR_MSG( SDC_UNDOREC_FLAG_IS_UNDO_FOR_HEAD_ROWPIECE(
                                *SDC_GET_UNDOREC_HDR_FIELD_PTR( sUndoRecHdr,
                                                                SDC_UNDOREC_HDR_FLAG ))
                           != ID_TRUE,
                           "sUndoRecHdrFlag : %"ID_UINT32_FMT,
                           *SDC_GET_UNDOREC_HDR_FIELD_PTR( sUndoRecHdr,
                                                           SDC_UNDOREC_HDR_FLAG ) );

            sSlotSID = SD_MAKE_SID_FROM_GRID( aRecGRID );

            IDE_TEST( sdcRow::undo_DELETE_FIRST_COLUMN_PIECE(
                                        &sMtx,
                                        sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr),
                                        sSlotPtr,
                                        sSlotSID,
                                        SDC_UNDO_MAKE_OLDROW,
                                        NULL, /* aRowBuf4MVCC */
                                        &sNewSlotPtr ) 
                      != IDE_SUCCESS );

            IDE_ERROR( smLayerCallback::getTableHeaderFromOID( aOID,
                                                               (void**)&sTableHeader )
                       == IDE_SUCCESS );
            IDE_DASSERT( sTableHeader != NULL );

            sEntry = (sdpPageListEntry*)smcTable::getDiskPageListEntry(sTableHeader);
            IDE_DASSERT(sEntry != NULL);

            // reallocSlot을 한이후에, Segment에 대한 가용도 변경연산을 수행한다.
            IDE_TEST( sdpPageList::updatePageState(
                                                aStatistics,
                                                SC_MAKE_SPACE(aRecGRID),
                                                sEntry,
                                                sdpPhyPage::getPageStartPtr(sSlotPtr),
                                                &sMtx )
                      != IDE_SUCCESS );

            IDE_TEST( sdcRow::writeDeleteFstColumnPieceCLR(
                                                sUndoRecHdr,
                                                aRecGRID,
                                                sRowHdrInfo.mUndoSID,
                                                &sMtx ) 
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sdcUndoRecord::setInvalid( sUndoRecHdr );

            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sUndoRecHdr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sUndoRecHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_ADD_FIRST_COLUMN_PIECE( SChar       * aLogPtr,
                                                          UInt          /*aSize*/,
                                                          UChar       * aSlotPtr,
                                                          sdrRedoInfo * aRedoInfo,
                                                          sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr     *sPageHdr;
    UChar             *sOldSlotPtr;
    UChar             *sNewSlotPtr;
    UChar             *sLogPtr          = (UChar*)aLogPtr;
    scSlotNum          sSlotNum;
    sdSID              sSlotSID;

    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aSlotPtr != NULL );
    IDE_DASSERT( aRedoInfo!= NULL );
    IDE_DASSERT( aRedoInfo->mLogType == SDR_SDC_ADD_FIRST_COLUMN_PIECE );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sOldSlotPtr )
              == IDE_SUCCESS );

    IDE_ERROR( sOldSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sOldSlotPtr) != ID_TRUE );

    sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sSlotNum );

    IDE_ERROR( sdcRow::undo_DELETE_FIRST_COLUMN_PIECE(
                                                NULL, /* aMtx */
                                                sLogPtr,
                                                sOldSlotPtr,
                                                sSlotSID,
                                                SDC_REDO_MAKE_NEWROW,
                                                NULL, /* aRowBuf4MVCC */
                                                &sNewSlotPtr )
               == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_DELETE_ROW_PIECE( SChar       * aLogPtr,
                                                    UInt         /*aSize*/,
                                                    UChar       * aSlotPtr,
                                                    sdrRedoInfo * aRedoInfo,
                                                    sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr * sPageHdr;
    UChar         * sSlotPtr;
    scSlotNum       sSlotNum;
    sdSID           sSlotSID;
    SChar         * sLogPtr = aLogPtr;
    UChar         * sNewSlotPtr;
    UShort          sNewRowPieceSize;
    SShort          sChangeSize;

    IDE_DASSERT( aLogPtr   != NULL );
    IDE_DASSERT( aSlotPtr  != NULL );
    IDE_DASSERT( aRedoInfo != NULL );
    IDE_DASSERT( (aRedoInfo->mLogType == SDR_SDC_DELETE_ROW_PIECE) ||
                 (aRedoInfo->mLogType == SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE) );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sSlotPtr)
               == IDE_SUCCESS );

    IDE_ERROR( sSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

    idlOS::memcpy( &sChangeSize, sLogPtr, ID_SIZEOF(SShort) );
    sLogPtr += ID_SIZEOF(SShort);
    IDE_ERROR_MSG( sChangeSize <= 0,
                   "sChangeSize : %"ID_UINT32_FMT, sChangeSize );

    sNewRowPieceSize = sdcRow::getRowPieceSize(sSlotPtr) + sChangeSize;
    IDE_ERROR_MSG( sNewRowPieceSize == SDC_ROWHDR_SIZE,
                   "sNewRowPieceSize : %"ID_UINT32_FMT, sNewRowPieceSize );

    sSlotSID = SD_MAKE_SID( sPageHdr->mPageID, sSlotNum );

    IDE_TEST( sdcRow::reallocSlot( sSlotSID,
                                   sSlotPtr,
                                   sNewRowPieceSize,
                                   ID_TRUE, /* aReserveFreeSpaceCredit */
                                   &sNewSlotPtr )
              != IDE_SUCCESS );
    sSlotPtr = NULL;
    IDE_ERROR( sNewSlotPtr != NULL );

    idlOS::memcpy( sNewSlotPtr, sLogPtr, SDC_ROWHDR_SIZE );

    IDE_ERROR( sdcRow::isDeleted(sNewSlotPtr) == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_DELETE_ROW_PIECE( idvSQL * aStatistics,
                                                    smTID    aTransID,
                                                    smOID    aOID,
                                                    scGRID   aRecGRID,
                                                    SChar  * aLogPtr,
                                                    smLSN  * aPrevLSN )
{
    sdrMtx             sMtx;
    UChar             *sUndoRecBodyPtr;
    sdSID              sSlotSID;
    UChar             *sSlotPtr;
    idBool             sDummy;
    UInt               sState = 0;
    UChar             *sUndoRecHdr;
    UShort             sOldRowPieceSize;
    UShort             sNewRowPieceSize;
    SShort             sChangeSize   = 0;
    SShort             sFSCreditSize = 0;
    sdpPhyPageHdr     *sPageHdr;
    sdcRowHdrInfo      sRowHdrInfo;
    UChar             *sNewSlotPtr;
    sdpPageListEntry  *sEntry;
    void              *sTableHeader;
    smOID              sTableOID;
    sdrLogHdr          sLogHdr;

    IDE_DASSERT( aOID        != SM_NULL_OID );
    IDE_DASSERT( aLogPtr     != NULL );
    IDE_DASSERT( aPrevLSN    != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_DELETE_ROW_PIECE TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(SC_MAKE_SPACE(aRecGRID),
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               (UChar**)&sSlotPtr,
                                               &sDummy )
                  != IDE_SUCCESS );
        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) == ID_TRUE );

        sPageHdr = sdpPhyPage::getHdr( sSlotPtr );

        if( sdpPhyPage::isConsistentPage( (UChar*)sPageHdr ) == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            IDE_ERROR( sRowHdrInfo.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID( 
                                                aStatistics,
                                                SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                sRowHdrInfo.mUndoSID,
                                                SDB_X_LATCH,
                                                SDB_WAIT_NORMAL,
                                                NULL,
                                                (UChar**)&sUndoRecHdr,
                                                &sDummy )
                      != IDE_SUCCESS );
            sState = 2;

            if( SDC_UNDOREC_FLAG_IS_UNDO_FOR_HEAD_ROWPIECE(
                    *SDC_GET_UNDOREC_HDR_FIELD_PTR( sUndoRecHdr,
                                                    SDC_UNDOREC_HDR_FLAG ))
                == ID_TRUE )
            {
                ID_READ_VALUE( aLogPtr, &sLogHdr, ID_SIZEOF(sdrLogHdr) );
                IDE_ERROR( sLogHdr.mType !=
                           SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE );

                if (smrRecoveryMgr::isRestart() == ID_FALSE)
                {
                    IDE_TEST( smLayerCallback::undoDeleteOfTableInfo(
                                                smxTransMgr::getTransByTID( aTransID ),
                                                aOID )
                              != IDE_SUCCESS );
                }
            }

            sdcUndoRecord::parseUpdateChangeSize( sUndoRecHdr,
                                                  &sChangeSize );
            IDE_ERROR( sChangeSize >= 0 );

            sUndoRecBodyPtr = sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr);

            sUndoRecBodyPtr += ID_SIZEOF(SShort);

            sOldRowPieceSize = sdcRow::getRowPieceSize( sSlotPtr );
            sNewRowPieceSize = (sOldRowPieceSize + sChangeSize);

            sFSCreditSize = sdcRow::calcFSCreditSize( sNewRowPieceSize,
                                                      sOldRowPieceSize );

            IDE_TEST( sdcTableCTL::unbind(
                                    &sMtx,
                                    sSlotPtr,
                                    *(UChar*)sSlotPtr,        /* aCTSlotIdxBfrUndo */
                                    *(UChar*)sUndoRecBodyPtr, /* aCTSlotIdxAftUndo */
                                    sFSCreditSize,
                                    ID_TRUE )                 /* aDecDeleteRowCnt */
                      != IDE_SUCCESS );

            sSlotSID = SD_MAKE_SID_FROM_GRID( aRecGRID );

            IDE_TEST( sdcRow::reallocSlot( sSlotSID,
                                           sSlotPtr,
                                           SDC_ROWHDR_SIZE + sChangeSize,
                                           ID_FALSE, /* aReserveFreeSpaceCredit */
                                           &sNewSlotPtr )
                      != IDE_SUCCESS );
            sSlotPtr = NULL;
            IDE_ERROR( sNewSlotPtr != NULL );

            idlOS::memcpy( sNewSlotPtr,
                           sUndoRecBodyPtr,
                           (sChangeSize + SDC_ROWHDR_SIZE) );

            IDE_ERROR( sdcRow::isDeleted(sNewSlotPtr) != ID_TRUE );

            /* Delete된 RowPiece 만 남아있던 것을 FreeSlot하도록 Redo로그만 
             * 남긴다. */
            IDE_TEST( sdrMiniTrans::writeLogRec( &sMtx,
                                                 aRecGRID,
                                                 &sOldRowPieceSize,
                                                 ID_SIZEOF(sOldRowPieceSize),
                                                 SDR_SDP_FREE_SLOT_FOR_SID )
                      != IDE_SUCCESS );

            IDE_TEST( sdcRow::writeDeleteRowPieceCLR( sNewSlotPtr,
                                                      aRecGRID,
                                                      &sMtx ) != IDE_SUCCESS );

            sdcUndoRecord::getTableOID( sUndoRecHdr, &sTableOID );

            IDE_ERROR( smLayerCallback::getTableHeaderFromOID( sTableOID,
                                                               (void**)&sTableHeader )
                       == IDE_SUCCESS );

            sEntry = (sdpPageListEntry*) smcTable::getDiskPageListEntry(sTableHeader);
            IDE_DASSERT(sEntry != NULL);

            // reallocSlot을 한이후에,
            // Segment에 대한 가용도 변경연산을 수행한다.
            IDE_TEST( sdpPageList::updatePageState( aStatistics,
                                                    SC_MAKE_SPACE(aRecGRID),
                                                    sEntry,
                                                    sdpPhyPage::getPageStartPtr(sNewSlotPtr),
                                                    &sMtx )
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sdcUndoRecord::setInvalid( sUndoRecHdr );

            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sUndoRecHdr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sUndoRecHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::redo_SDR_SDC_LOCK_ROW( SChar       * aLogPtr,
                                            UInt          /*aSize*/,
                                            UChar       * aSlotPtr,
                                            sdrRedoInfo * aRedoInfo,
                                            sdrMtx      * /*aMtx*/ )
{
    sdpPhyPageHdr     *sPageHdr;
    UChar             *sSlotPtr;
    scSlotNum          sSlotNum;
    UChar              sRowFlag;

    IDE_DASSERT( aLogPtr  != NULL );
    IDE_DASSERT( aSlotPtr != NULL );
    IDE_DASSERT( aRedoInfo!= NULL );
    IDE_DASSERT( aRedoInfo->mLogType == SDR_SDC_LOCK_ROW );

    sPageHdr = sdpPhyPage::getHdr(aSlotPtr);

    sSlotNum = aRedoInfo->mSlotNum;

    IDE_ERROR( sdpSlotDirectory::getPagePtrFromSlotNum(
                            sdpPhyPage::getSlotDirStartPtr((UChar*)sPageHdr), 
                            sSlotNum,
                            &sSlotPtr)
               == IDE_SUCCESS );

    IDE_ERROR( sSlotPtr != NULL );
    IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

    SDC_GET_ROWHDR_1B_FIELD(sSlotPtr, SDC_ROWHDR_FLAG, sRowFlag);
    IDE_ERROR( SDC_IS_HEAD_ROWPIECE(sRowFlag) == ID_TRUE );

    IDE_ERROR( sdcRow::redo_undo_LOCK_ROW( NULL, /* aMtx */
                                           (UChar*)aLogPtr,
                                           sSlotPtr,
                                           SDC_REDO_MAKE_NEWROW )
                == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 *
 * Description :
 *
 *
 **********************************************************************/
IDE_RC sdcRowUpdate::undo_SDR_SDC_LOCK_ROW( idvSQL * aStatistics,
                                            smTID    aTransID,
                                            smOID    aOID,
                                            scGRID   aRecGRID,
                                            SChar  * /*aLogPtr*/,
                                            smLSN  * aPrevLSN )
{
    sdrMtx          sMtx;
    UChar          *sSlotPtr;
    idBool          sDummy;
    UInt            sState = 0;
    sdpPhyPageHdr  *sPageHdr;
    UChar          *sUndoRecHdr;
    sdcRowHdrInfo   sRowHdrInfo;

    IDE_DASSERT( aPrevLSN    != NULL );
    IDE_DASSERT( SC_GRID_IS_NOT_NULL(aRecGRID));

    //BUG-48460: undo_SDR_SDC_LOCK_ROW TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(SC_MAKE_SPACE(aRecGRID), SCT_SS_SKIP_UNDO)
        == ID_FALSE )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByGRID( aStatistics,
                                               aRecGRID,
                                               SDB_X_LATCH,
                                               SDB_WAIT_NORMAL,
                                               &sMtx,
                                               (UChar**)&sSlotPtr,
                                               &sDummy )
                  != IDE_SUCCESS );

        IDE_ERROR( sSlotPtr != NULL );
        IDE_ERROR( sdcRow::isDeleted(sSlotPtr) != ID_TRUE );

        sPageHdr = sdpPhyPage::getHdr(sSlotPtr);

        if( sdpPhyPage::isConsistentPage((UChar*)sPageHdr) == ID_TRUE )
        {
            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            IDE_ERROR( sRowHdrInfo.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID( 
                                            aStatistics,
                                            SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                            sRowHdrInfo.mUndoSID,
                                            SDB_X_LATCH,
                                            SDB_WAIT_NORMAL,
                                            NULL,
                                            (UChar**)&sUndoRecHdr,
                                            &sDummy )
                      != IDE_SUCCESS );
            sState = 2;

            IDE_TEST( sdcRow::redo_undo_LOCK_ROW(
                                    &sMtx,
                                    sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr),
                                    sSlotPtr,
                                    SDC_UNDO_MAKE_OLDROW )
                       != IDE_SUCCESS );

            IDE_TEST( sdcRow::writeLockRowCLR( sUndoRecHdr,
                                               aRecGRID,
                                               sRowHdrInfo.mUndoSID,
                                               &sMtx )
                      != IDE_SUCCESS );

            sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sdcUndoRecord::setInvalid( sUndoRecHdr );

            sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                             sdpPhyPage::getPageStartPtr(sUndoRecHdr) );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 sUndoRecHdr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sUndoRecHdr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback(&sMtx)
                        == IDE_SUCCESS );
            break;

        default:
            break;
    }

    smrRecoveryMgr::prepareRTOIForUndoFailure( smxTransMgr::getTransByTID(aTransID),
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               aOID, /* aTableOID */
                                               0,    /* aIndexID */
                                               aRecGRID.mSpaceID, 
                                               aRecGRID.mPageID );

    return IDE_FAILURE;
}

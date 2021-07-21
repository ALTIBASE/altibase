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
 * $Id: sdcLobUpdate.cpp 56979 2012-12-18 08:54:19Z jiko $
 **********************************************************************/

#include <sdr.h>
#include <sdp.h>
#include <smxTrans.h>
#include <sdcDef.h>
#include <sdcReq.h>
#include <sdcTableCTL.h>
#include <sdcTSSlot.h>
#include <sdcTSSegment.h>
#include <sdcUndoSegment.h>
#include <sdcLob.h>
#include <sdcLobUpdate.h>
#include <smxTransMgr.h>


/***********************************************************************
 * redo type: SDR_SDC_LOB_UPDATE_LOBDESC
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_UPDATE_LOBDESC( SChar       * aLogPtr,
                                                      UInt         /*aSize*/,
                                                      UChar       * aSlotPtr,
                                                      sdrRedoInfo * /*aRedoInfo*/,
                                                      sdrMtx      * /*aMtx*/ )
{
    SChar       * sLogPtr = aLogPtr;
    UShort        sLobColSeqInRowPiece;
    UChar       * sLobDescPtr;
    sdcLobDesc    sLobDesc;

    IDE_ERROR( aLogPtr  != NULL );
    IDE_ERROR( aSlotPtr != NULL );

    idlOS::memcpy( &sLobColSeqInRowPiece,
                   sLogPtr,
                   ID_SIZEOF(UShort) );
    sLogPtr += ID_SIZEOF(UShort);

    idlOS::memcpy( &sLobDesc,
                   sLogPtr,
                   ID_SIZEOF(sLobDesc) );
    sLogPtr += ID_SIZEOF(sLobDesc);

    sdcRow::getLobDescInRowPiece( aSlotPtr,
                                  sLobColSeqInRowPiece,
                                  &sLobDescPtr );

    idlOS::memcpy( sLobDescPtr,
                   &sLobDesc,
                   ID_SIZEOF(sdcLobDesc) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDC_LOB_INSERT_INTERNAL_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_INSERT_INTERNAL_KEY( SChar       * aData,
                                                           UInt          aLength,
                                                           UChar       * aPagePtr,
                                                           sdrRedoInfo * /*aRedoInfo*/,
                                                           sdrMtx      * aMtx )
{
    SChar           * sDataPtr;
    SShort            sKeySeq;
    sdcLobIKey        sIKey;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobIKey      * sKeyArray;
    
    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(SShort) +
                                 ID_SIZEOF(sdcLobIKey) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sKeySeq, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy( &sIKey, sDataPtr, ID_SIZEOF(sdcLobIKey) );
    sDataPtr += ID_SIZEOF(sdcLobIKey);

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);
    sKeyArray = (sdcLobIKey*)sdcLob::getLobDataLayerStartPtr(aPagePtr);

    IDE_ERROR( sdpPhyPage::getPageType(
                     (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr))
               == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mKeyCnt == sKeySeq );

    sKeyArray[sKeySeq] = sIKey;

    sNodeHdr->mKeyCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type: SDR_SDC_LOB_FREE_INTERNAL_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_FREE_INTERNAL_KEY( SChar       * aData,
                                                         UInt          aLength,
                                                         UChar       * aPagePtr,
                                                         sdrRedoInfo * /*aRedoInfo*/,
                                                         sdrMtx      * aMtx )
{
    SChar           * sDataPtr;
    SShort            sKeySeq;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobIKey      * sKeyArray;
    
    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(SShort) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sKeySeq, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);
    sKeyArray = (sdcLobIKey*)sdcLob::getLobDataLayerStartPtr(aPagePtr);

    IDE_ERROR( sdpPhyPage::getPageType(
                     (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr))
               == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
    IDE_ERROR( (sNodeHdr->mKeyCnt-1) == sKeySeq );

    sKeyArray[sKeySeq].mChild = SD_NULL_PID;

    sNodeHdr->mKeyCnt--;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type: SDR_SDC_LOB_FREE_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_FREE_LEAF_KEY( SChar       * aData,
                                                     UInt          aLength,
                                                     UChar       * aPagePtr,
                                                     sdrRedoInfo * /*aRedoInfo*/,
                                                     sdrMtx      * aMtx )
{
    SChar           * sDataPtr;
    SShort            sKeySeq;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sKeyArray;
    UInt              i;
    
    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(SShort) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sKeySeq, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);
    sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr(aPagePtr);

    IDE_ERROR( sdpPhyPage::getPageType(
                     (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr))
               == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
    IDE_ERROR( (sNodeHdr->mKeyCnt-1) == sKeySeq );

    for( i = 0; i < SDC_LOB_MAX_ENTRY_CNT; i++ )
    {
        sKeyArray[sKeySeq].mEntry[i] = SD_NULL_PID;
    }

    sNodeHdr->mKeyCnt--;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDC_LOB_WRITE_PIECE
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_WRITE_PIECE( SChar       * aData,
                                                   UInt          aLength,
                                                   UChar       * aPagePtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx*       /*aMtx*/ )
{
    IDE_TEST( redoLobWritePiece(aData,
                                aLength,
                                aPagePtr)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDC_LOB_WRITE_PIECE_PREV
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_WRITE_PIECE_PREV( SChar       * aData,
                                                        UInt          aLength,
                                                        UChar       * aPagePtr,
                                                        sdrRedoInfo * /*aRedoInfo*/,
                                                        sdrMtx*       /*aMtx*/ )
{
    IDE_TEST( redoLobWritePiece(aData,
                                aLength,
                                aPagePtr)
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDC_LOB_WRITE_PIECE4DML
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_WRITE_PIECE4DML( SChar       * aData,
                                                       UInt          aLength,
                                                       UChar       * aPagePtr,
                                                       sdrRedoInfo * /*aRedoInfo*/,
                                                       sdrMtx*       /*aMtx*/ )
{
    sdcLobNodeHdr   * sNodeHdr;
    UChar           * sLobDataLayerStartPtr;
    UInt              sWriteOffset;
    UInt              sWriteSize;
    SChar           * sWriteValue;

    IDE_ERROR( aData != NULL );
    IDE_ERROR( aPagePtr != NULL );

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);

    sLobDataLayerStartPtr = sdcLob::getLobDataLayerStartPtr(aPagePtr);

    idlOS::memcpy(&sWriteSize, aData, ID_SIZEOF(UInt));
    aData +=  ID_SIZEOF(UInt);

    sWriteValue = aData;

    IDE_ERROR_MSG( aLength == ( ID_SIZEOF(UInt) +           /* write size */
                                sWriteSize +                /* write value */
                                ID_SIZEOF(UInt) +           /* column id */
                                ID_SIZEOF(UInt) ),          /* amount */
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sWriteOffset =
        sdpPhyPage::getOffsetFromPagePtr(aPagePtr) -
        sdpPhyPage::getOffsetFromPagePtr(sLobDataLayerStartPtr);        

    IDE_ERROR( (sWriteOffset + sWriteSize) <= SDC_LOB_PAGE_BODY_SIZE );

    if( sWriteSize > 0 )
    {
        idlOS::memcpy( sLobDataLayerStartPtr + sWriteOffset,
                       sWriteValue,
                       sWriteSize );

        if( (sWriteOffset + sWriteSize) > sNodeHdr->mStoreSize )
        {
            sNodeHdr->mStoreSize = (sWriteOffset + sWriteSize);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdcLobUpdate::redoLobWritePiece( SChar   * aData,
                                        UInt      aLength,
                                        UChar   * aPagePtr )
{
    sdcLobNodeHdr   * sNodeHdr;
    UChar           * sLobDataLayerStartPtr;
    UInt              sOffset;
    UInt              sAmount;
    UInt              sWriteOffset;
    UInt              sWriteSize;
    UInt              sDummySize;
    SChar           * sWriteValue;

    IDE_ERROR( aData != NULL );
    IDE_ERROR( aPagePtr != NULL );

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);

    sLobDataLayerStartPtr = sdcLob::getLobDataLayerStartPtr(aPagePtr);

    idlOS::memcpy(&sWriteSize, aData, ID_SIZEOF(UInt));
    aData +=  ID_SIZEOF(UInt);

    sWriteValue = aData;

    aData += sWriteSize;
    aData += ID_SIZEOF(smLobLocator);

    idlOS::memcpy(&sOffset, aData, ID_SIZEOF(UInt));
    aData +=  ID_SIZEOF(UInt);

    idlOS::memcpy(&sAmount, aData, ID_SIZEOF(UInt));
    aData +=  ID_SIZEOF(UInt);

    IDE_ERROR_MSG( aLength == ( ID_SIZEOF(UInt) +           /* write size */
                                sWriteSize +                /* write value */
                                ID_SIZEOF(smLobLocator) +   /* lob locator */
                                ID_SIZEOF(UInt) +           /* offset */
                                ID_SIZEOF(UInt) ),          /* amount */
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    IDE_TEST( sdcLob::getWritePageOffsetAndSize( sOffset,
                                                 sAmount,
                                                 &sWriteOffset,
                                                 &sDummySize )
              != IDE_SUCCESS );

    IDE_ERROR( (sWriteOffset + sWriteSize) <= SDC_LOB_PAGE_BODY_SIZE );

    if( sWriteSize > 0 )
    {
        idlOS::memcpy( sLobDataLayerStartPtr + sWriteOffset,
                       sWriteValue,
                       sWriteSize );

        if( (sWriteOffset + sWriteSize) > sNodeHdr->mStoreSize )
        {
            sNodeHdr->mStoreSize = (sWriteOffset + sWriteSize);
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST( SChar       * aData,
                                                             UInt          aLength,
                                                             UChar       * aPagePtr,
                                                             sdrRedoInfo * /*aRedoInfo*/,
                                                             sdrMtx      * aMtx )
{
    SChar           * sDataPtr;
    sdSID             sTSSlotSID;
    smSCN             sFstDskViewSCN;
    sdcLobNodeHdr   * sNodeHdr;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(sdSID) +
                                 ID_SIZEOF(smSCN) +
                                 ID_SIZEOF(scGRID) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy(&sTSSlotSID, sDataPtr, ID_SIZEOF(sdSID));
    sDataPtr += ID_SIZEOF(sdSID);
    
    idlOS::memcpy(&sFstDskViewSCN, sDataPtr, ID_SIZEOF(smSCN));
    sDataPtr += ID_SIZEOF(smSCN);

    sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);
    sNodeHdr->mTSSlotSID = sTSSlotSID;
    sNodeHdr->mFstDskViewSCN = sFstDskViewSCN;
    sNodeHdr->mLobPageState = SDC_LOB_AGING_LIST;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST
 ***********************************************************************/
IDE_RC sdcLobUpdate::undo_SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST(
                                                    idvSQL   * aStatistics,
                                                    void     * aTrans,
                                                    sdrMtx   * aMtx,
                                                    scGRID     aGRID,
                                                    SChar    * aLogPtr,
                                                    UInt       aSize )
{
    sdcLobNodeHdr       * sNodeHdr;
    idBool                sIsSuccess;
    sdpPhyPageHdr       * sPage;
    UChar               * sLobSegMetaPage;
    scGRID                sMetaGRID;
    sdcLobMeta          * sLobMeta;
    sdSID                 sTSSlotSID;
    smSCN                 sFstDskViewSCN;
    UInt                  sLobPageState = SDC_LOB_USED;

    IDE_ERROR_MSG( aSize == ( ID_SIZEOF(sdSID) +
                              ID_SIZEOF(smSCN) +
                              ID_SIZEOF(scGRID) ),
                   "aSize : %"ID_UINT32_FMT,
                   aSize );
    
    //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가
    if( sctTableSpaceMgr::hasState(aGRID.mSpaceID,
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {    
        sTSSlotSID = SD_NULL_SID;
        SM_SET_SCN_INFINITE( &sFstDskViewSCN );

        idlOS::memcpy( &sMetaGRID,
                       aLogPtr + ID_SIZEOF(sdSID) + ID_SIZEOF(smSCN),
                       ID_SIZEOF(scGRID) );

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              sMetaGRID.mSpaceID,
                                              sMetaGRID.mPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sLobSegMetaPage,
                                              NULL,
                                              NULL )
                  != IDE_SUCCESS);

        sLobMeta = (sdcLobMeta*)sdpPhyPage::getLogicalHdrStartPtr( sLobSegMetaPage );

        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aGRID.mSpaceID,
                                              aGRID.mPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              (UChar**)&sPage,
                                              &sIsSuccess,
                                              NULL )
                  != IDE_SUCCESS );

        sNodeHdr = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sPage);

        IDE_TEST( sdpDblPIDList::removeNode( aStatistics,
                                             &sLobMeta->mAgingListBase,
                                             &sPage->mListNode,
                                             aMtx )
                  != IDE_SUCCESS );

        IDE_TEST( sdpDblPIDList::setNxtOfNode(&sPage->mListNode,
                                              SD_NULL_PID,
                                              aMtx)
                  != IDE_SUCCESS );

        IDE_TEST( sdpDblPIDList::setPrvOfNode(&sPage->mListNode,
                                              SD_NULL_PID,
                                              aMtx)
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mTSSlotSID,
                                             (void*)&sTSSlotSID,
                                             ID_SIZEOF(sTSSlotSID) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mFstDskViewSCN,
                                             (void*)&sFstDskViewSCN,
                                             ID_SIZEOF(sFstDskViewSCN) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mLobPageState,
                                             (void*)&sLobPageState,
                                             ID_SIZEOF(sLobPageState) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smrRecoveryMgr::prepareRTOIForUndoFailure( aTrans,
                                               SMR_RTOI_TYPE_DISKPAGE,
                                               SM_NULL_OID, /* aTableOID */
                                               0,    /* aIndexId */
                                               aGRID.mSpaceID,
                                               aGRID.mPageID );

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_OP_SDC_LOB_APPEND_LEAFNODE
 ***********************************************************************/
IDE_RC sdcLobUpdate::undo_SDR_SDC_LOB_APPEND_LEAFNODE(
                                                idvSQL    * aStatistics,
                                                sdrMtx    * aMtx,
                                                scSpaceID   aSpaceID,
                                                scPageID    aRootNodePID,
                                                scPageID    aLeafNodePID )
{
    IDE_TEST( sdcLob::appendLeafNodeRollback( aStatistics,
                                              aMtx,
                                              aSpaceID,
                                              aRootNodePID,
                                              aLeafNodePID)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type:  SDR_SDC_LOB_INSERT_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_INSERT_LEAF_KEY( SChar       * aData,
                                                       UInt          aLength,
                                                       UChar       * aPagePtr,
                                                       sdrRedoInfo * /*aRedoInfo*/,
                                                       sdrMtx      * aMtx )
{
    SChar           * sDataPtr;
    SShort            sKeySeq;
    sdcLobLKey        sLKey;
    sdcLobLKey      * sKeyArray;
    sdcLobNodeHdr   * sNodeHdr;
    
    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aPagePtr != NULL );
    IDE_ERROR( aMtx     != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(SShort) +
                                 ID_SIZEOF(sdcLobLKey) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sKeySeq, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy( &sLKey, sDataPtr, ID_SIZEOF(sdcLobLKey) );
    sDataPtr += ID_SIZEOF(sdcLobLKey);

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aPagePtr);
    sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr(aPagePtr);

    IDE_ERROR( sdpPhyPage::getPageType(
                     (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(aPagePtr))
               == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mKeyCnt == sKeySeq );

    idlOS::memcpy( &sKeyArray[sKeySeq], &sLKey, ID_SIZEOF(sdcLobLKey) );
    
    sNodeHdr->mKeyCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_SDC_LOB_INSERT_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::undo_SDR_SDC_LOB_INSERT_LEAF_KEY( idvSQL  * aStatistics,
                                                       smTID     aTransID,
                                                       smOID     aOID,
                                                       scGRID    aRecGRID,
                                                       SChar   * aLogPtr,
                                                       smLSN   * aPrevLSN )
{
    UInt              sState = 0;
    UChar           * sLeafNode;
    SShort            sKeySeq;
    sdrMtx            sMtx;
    sdrLogHdr         sLogHdr;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sLKey;
    sdcLobLKey      * sKeyArray;
    UInt              i;

    //BUG-48460: TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(aRecGRID.mSpaceID,
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        ID_READ_VALUE( aLogPtr,
                       &sLogHdr,
                       ID_SIZEOF(sdrLogHdr) );

        ID_READ_VALUE( aLogPtr + ID_SIZEOF(sdrLogHdr),
                       &sKeySeq,
                       ID_SIZEOF(SShort) );

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE, /*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aRecGRID.mSpaceID,
                                             aRecGRID.mPageID,
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             &sMtx,
                                             &sLeafNode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        if( sdpPhyPage::isConsistentPage(sLeafNode) == ID_TRUE )
        {
            sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sLeafNode);

            sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr(sLeafNode);

            IDE_ERROR( sdpPhyPage::getPageType(
                          (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sLeafNode))
                       == SDP_PAGE_LOB_INDEX );
            IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
            IDE_ERROR( (sNodeHdr->mKeyCnt-1) == sKeySeq );

            sLKey = &sKeyArray[sKeySeq];

            sNodeHdr->mKeyCnt--;

            for( i = 0; i < SDC_LOB_MAX_ENTRY_CNT; i++ )
            {
                sLKey->mEntry[i] = SD_NULL_PID;
            }

            IDE_TEST( sdcLob::writeLobInsertLeafKeyCLR( &sMtx,
                                                        (UChar*)sLKey,
                                                        sKeySeq)
                       != IDE_SUCCESS );

            (void)sdrMiniTrans::setCLR( &sMtx, aPrevLSN );
        }


        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
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
 * redo type:  SDR_SDC_LOB_UPDATE_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_UPDATE_LEAF_KEY( SChar       * aData,
                                                       UInt          aLength,
                                                       UChar       * aSlotPtr,
                                                       sdrRedoInfo * /*aRedoInfo*/,
                                                       sdrMtx      * /*aMtx*/ )
{
    SChar           * sDataPtr;
    SShort            sKeySeq;
    sdcLobLKey        sLKey;
    sdcLobLKey      * sKeyArray;
    sdcLobNodeHdr   * sNodeHdr;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aSlotPtr != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(SShort) +
                                 ID_SIZEOF(sdcLobLKey) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sKeySeq, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy( &sLKey, sDataPtr, ID_SIZEOF(sdcLobLKey) );
    sDataPtr += ID_SIZEOF(sdcLobLKey);

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aSlotPtr);
    sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr(aSlotPtr);

    IDE_ERROR( sdpPhyPage::getPageType(
                     (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sNodeHdr))
               == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
    IDE_ERROR( sNodeHdr->mKeyCnt > sKeySeq );

    idlOS::memcpy( &sKeyArray[sKeySeq], &sLKey, ID_SIZEOF(sdcLobLKey) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type:  SDR_SDC_LOB_UPDATE_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::undo_SDR_SDC_LOB_UPDATE_LEAF_KEY( idvSQL  * aStatistics,
                                                       smTID     aTransID,
                                                       smOID     aOID,
                                                       scGRID    aRecGRID,
                                                       SChar   * aLogPtr,
                                                       smLSN   * aPrevLSN )
{
    UInt              sState = 0;
    UChar           * sLeafNode;
    UChar           * sUndoRecHdr;
    UChar           * sUndoRecBodyPtr;
    SShort            sKeySeq;
    sdrMtx            sMtx;
    sdrLogHdr         sLogHdr;
    sdcLobLKey        sLKey;
    sdcLobLKey        sOldLKey;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sKeyArray;
    idBool            sDummy;

    //BUG-48460: TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(aRecGRID.mSpaceID,
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {
        ID_READ_VALUE( aLogPtr,
                       &sLogHdr,
                       ID_SIZEOF(sdrLogHdr) );

        ID_READ_VALUE( aLogPtr + ID_SIZEOF(sdrLogHdr),
                       &sKeySeq,
                       ID_SIZEOF(SShort) );

        ID_READ_VALUE( aLogPtr + ID_SIZEOF(sdrLogHdr) + ID_SIZEOF(SShort),
                       &sLKey,
                       ID_SIZEOF(sdcLobLKey) );

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE, /*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aRecGRID.mSpaceID,
                                             aRecGRID.mPageID,
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             &sMtx,
                                             &sLeafNode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        if( sdpPhyPage::isConsistentPage(sLeafNode) == ID_TRUE )
        {
            IDE_ERROR( sLKey.mUndoSID != SD_NULL_SID );

            IDE_TEST( sdbBufferMgr::getPageBySID( aStatistics,
                                                  SMI_ID_TABLESPACE_SYSTEM_DISK_UNDO,
                                                  sLKey.mUndoSID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  NULL,
                                                  (UChar**)&sUndoRecHdr,
                                                  &sDummy )
                      != IDE_SUCCESS );
            sState = 2;

            sUndoRecBodyPtr = sdcUndoRecord::getUndoRecBodyStartPtr(sUndoRecHdr);

            idlOS::memcpy( &sOldLKey, sUndoRecBodyPtr, ID_SIZEOF(sdcLobLKey) );

            sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sLeafNode);

            sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr((UChar*)sNodeHdr);

            IDE_ERROR( sdpPhyPage::getPageType(
                          (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sLeafNode))
                       == SDP_PAGE_LOB_INDEX );
            IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
            IDE_ERROR( sNodeHdr->mKeyCnt > sKeySeq );

            idlOS::memcpy( &sKeyArray[sKeySeq],
                           &sOldLKey,
                           ID_SIZEOF(sdcLobLKey) );

            IDE_TEST( sdcLob::writeLobUpdateLeafKeyCLR( &sMtx,
                                                        (UChar*)&sKeyArray[sKeySeq],
                                                        sKeySeq,
                                                        &sOldLKey )
                      != IDE_SUCCESS );

            (void)sdrMiniTrans::setCLR( &sMtx, aPrevLSN );

            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage(aStatistics,
                                                sUndoRecHdr)
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
            IDE_ASSERT( sdbBufferMgr::releasePage(aStatistics,
                                                  sUndoRecHdr)
                        == IDE_SUCCESS );
        case 1:
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
 * redo type: SDR_SDC_LOB_OVERWRITE_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::redo_SDR_SDC_LOB_OVERWRITE_LEAF_KEY( SChar       * aData,
                                                          UInt          aLength,
                                                          UChar       * aSlotPtr,
                                                          sdrRedoInfo * /*aRedoInfo*/,
                                                          sdrMtx      * /*aMtx*/ )
{
    SChar           * sDataPtr;
    SShort            sKeySeq;
    sdcLobLKey        sNewLKey;
    sdcLobLKey        sOldLKey;
    sdcLobLKey      * sKeyArray;
    sdcLobNodeHdr   * sNodeHdr;

    IDE_ERROR( aData    != NULL );
    IDE_ERROR( aSlotPtr != NULL );
    IDE_ERROR_MSG( aLength  == ( ID_SIZEOF(SShort) +
                                 ID_SIZEOF(sdcLobLKey) +
                                 ID_SIZEOF(sdcLobLKey) ),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sKeySeq, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy( &sOldLKey, sDataPtr, ID_SIZEOF(sdcLobLKey) );
    sDataPtr += ID_SIZEOF(sdcLobLKey);

    idlOS::memcpy( &sNewLKey, sDataPtr, ID_SIZEOF(sdcLobLKey) );
    sDataPtr += ID_SIZEOF(sdcLobLKey);

    sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(aSlotPtr);
    sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr(aSlotPtr);

    IDE_ERROR( sdpPhyPage::getPageType(
                     (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sNodeHdr))
               == SDP_PAGE_LOB_INDEX );
    IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
    IDE_ERROR( sNodeHdr->mKeyCnt > sKeySeq );

    idlOS::memcpy( &sKeyArray[sKeySeq], &sNewLKey, ID_SIZEOF(sdcLobLKey) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * undo type: SDR_SDC_LOB_OVERWRITE_LEAF_KEY
 ***********************************************************************/
IDE_RC sdcLobUpdate::undo_SDR_SDC_LOB_OVERWRITE_LEAF_KEY( idvSQL  * aStatistics,
                                                          smTID     aTransID,
                                                          smOID     aOID,
                                                          scGRID    aRecGRID,
                                                          SChar   * aLogPtr,
                                                          smLSN   * aPrevLSN )
{
    UInt              sState = 0;
    UChar           * sLeafNode;
    SShort            sKeySeq;
    sdrMtx            sMtx;
    sdrLogHdr         sLogHdr;
    sdcLobLKey        sOldLKey;
    sdcLobLKey        sNewLKey;
    sdcLobNodeHdr   * sNodeHdr;
    sdcLobLKey      * sKeyArray;

    //BUG-48460: TBS 상태 체크 위치 조정
    if( sctTableSpaceMgr::hasState(aRecGRID.mSpaceID,
                                   SCT_SS_SKIP_UNDO) == ID_FALSE )
    {

        ID_READ_VALUE( aLogPtr,
                       &sLogHdr,
                       ID_SIZEOF(sdrLogHdr) );

        ID_READ_VALUE( aLogPtr +
                       ID_SIZEOF(sdrLogHdr),
                       &sKeySeq,
                       ID_SIZEOF(SShort) );

        ID_READ_VALUE( aLogPtr +
                       ID_SIZEOF(sdrLogHdr) +
                       ID_SIZEOF(SShort),
                       &sOldLKey,
                       ID_SIZEOF(sdcLobLKey) );

        ID_READ_VALUE( aLogPtr +
                       ID_SIZEOF(sdrLogHdr) +
                       ID_SIZEOF(SShort) +
                       ID_SIZEOF(sdcLobLKey),
                       &sNewLKey,
                       ID_SIZEOF(sdcLobLKey) );

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       smxTransMgr::getTransByTID(aTransID),
                                       SDR_MTX_LOGGING,
                                       ID_FALSE, /*MtxUndoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( sdbBufferMgr::getPageByPID(aStatistics,
                                             aRecGRID.mSpaceID,
                                             aRecGRID.mPageID,
                                             SDB_X_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_SINGLE_PAGE_READ,
                                             &sMtx,
                                             &sLeafNode,
                                             NULL,
                                             NULL)
                  != IDE_SUCCESS );

        if( sdpPhyPage::isConsistentPage(sLeafNode) == ID_TRUE )
        {
            sNodeHdr  = (sdcLobNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(sLeafNode);

            sKeyArray = (sdcLobLKey*)sdcLob::getLobDataLayerStartPtr((UChar*)sNodeHdr);

            IDE_ERROR( sdpPhyPage::getPageType(
                          (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr(sLeafNode))
                       == SDP_PAGE_LOB_INDEX );
            IDE_ERROR( sNodeHdr->mKeyCnt > 0 );
            IDE_ERROR( sNodeHdr->mKeyCnt > sKeySeq );

            idlOS::memcpy( &sKeyArray[sKeySeq],
                           &sOldLKey,
                           ID_SIZEOF(sdcLobLKey) );

            IDE_TEST( sdcLob::writeLobOverwriteLeafKeyCLR( &sMtx,
                                                           (UChar*)&sKeyArray[sKeySeq],
                                                           sKeySeq,
                                                           &sNewLKey,
                                                           &sOldLKey )
                      != IDE_SUCCESS );

            (void)sdrMiniTrans::setCLR( &sMtx, aPrevLSN );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    
    switch( sState )
    {
        case 1:
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

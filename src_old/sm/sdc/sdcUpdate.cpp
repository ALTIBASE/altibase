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
 * $Id: sdcUpdate.cpp 89495 2020-12-14 05:19:22Z emlee $
 *
 * Description :
 *
 * 본 파일은 undo tablespace 관련 redo 함수에 대한 구현파일이다.
 *
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
#include <sdcUpdate.h>
#include <smxTransMgr.h>

/***********************************************************************
 * Description : SDR_SDC_INSERT_UNDO_REC
 **********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_INSERT_UNDO_REC( SChar       * aLogPtr,
                                                UInt          aSize,
                                                UChar       * aRecPtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx      * /* aMtx */ )
{
    sdpPhyPageHdr*   sPageHdr;
    UChar           *sSlotDirPtr;
    UShort           sSlotSize = 0;
    scOffset         sSlotOffset;
    UChar*           sUndoRecPtr;
    UInt             sKeyMapCount;
    UShort           sUndoRecSize;
    UChar           *sData;

    IDE_ERROR( aLogPtr != NULL );
    IDE_ERROR( aSize != 0 );
    IDE_ERROR( aRecPtr != NULL );

    sData = (UChar*)aLogPtr;
    idlOS::memcpy( &sUndoRecSize, sData, ID_SIZEOF(UShort) );
    sData += ID_SIZEOF(UShort);
    IDE_ERROR( sUndoRecSize > 0 );

    sPageHdr = sdpPhyPage::getHdr(aRecPtr);

    IDE_ERROR( sdpPhyPage::canAllocSlot( sPageHdr,
                                         sUndoRecSize,
                                         ID_TRUE,
                                         SDP_1BYTE_ALIGN_SIZE )
               == ID_TRUE );

    sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(aRecPtr);
    sKeyMapCount = sdpSlotDirectory::getCount(sSlotDirPtr);

    IDE_TEST( sdpPhyPage::allocSlot( sPageHdr,
                                     sKeyMapCount,
                                     sUndoRecSize,
                                     ID_TRUE,
                                     &sSlotSize,
                                     &sUndoRecPtr,
                                     &sSlotOffset,
                                     SDP_1BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_ERROR_MSG( sUndoRecPtr != NULL,
                   "sUndoRecPtr : %"ID_vULONG_FMT, sUndoRecPtr );
    IDE_ERROR_MSG( sUndoRecPtr == aRecPtr,
                   "sUndoRecPtr : %"ID_vULONG_FMT"\n"
                   "aRecPtr     : %"ID_vULONG_FMT,
                   sUndoRecPtr,
                   aRecPtr );
    IDE_ERROR_MSG( sSlotSize   == sUndoRecSize,
                   "sSlotSize   : %"ID_UINT32_FMT"\n"
                   "sUndoRecSize: %"ID_UINT32_FMT, 
                   sSlotSize,
                   sUndoRecSize );

    idlOS::memcpy( sUndoRecPtr, (UChar*)sData, sUndoRecSize );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : SDR_SDC_BIND_TSS
 **********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_BIND_TSS( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aRecPtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * aMtx )
{

    sdSID          sTSSlotSID;
    sdpPhyPageHdr* sPageHdr;
    UChar*         sSlotPtr;
    UShort         sSlotSize;
    scOffset       sSlotOffset;
    SChar        * sValuePtr;
    smTID          sTransID;

    IDE_ERROR( aLogPtr != NULL );
    IDE_ERROR( aRecPtr != NULL );
    IDE_ERROR( aMtx != NULL );
    IDE_ERROR_MSG( aSize == (ID_SIZEOF(sdSID) +
                             ID_SIZEOF(sdRID) +
                             ID_SIZEOF(smTID) +
                             ID_SIZEOF(UInt)),
                   "aSize : %"ID_UINT32_FMT,
                   aSize );

    sValuePtr = aLogPtr;

    idlOS::memcpy( &sTSSlotSID, sValuePtr, ID_SIZEOF(sdSID) );
    sValuePtr += ID_SIZEOF(sdSID);
    sValuePtr += ID_SIZEOF(sdRID);

    idlOS::memcpy( &sTransID, sValuePtr, ID_SIZEOF(smTID) );
    sValuePtr += ID_SIZEOF(smTID);

    IDE_ERROR_MSG( sTSSlotSID != SD_NULL_SID,
                   "sTSSlotSID : %"ID_UINT32_FMT,
                   sTSSlotSID );

    sPageHdr = sdpPhyPage::getHdr(aRecPtr);
    IDE_TEST( sdpPhyPage::allocSlot( sPageHdr,
                                     SD_MAKE_SLOTNUM(sTSSlotSID),
                                     (UShort)idlOS::align8(ID_SIZEOF(sdcTSS)),
                                     ID_TRUE,
                                     &sSlotSize,
                                     &sSlotPtr,
                                     &sSlotOffset,
                                     SDP_8BYTE_ALIGN_SIZE )
              != IDE_SUCCESS );

    IDE_ERROR_MSG( sSlotPtr == aRecPtr,
                   "sSlotPtr : %"ID_vULONG_FMT"\n"
                   "aRecPtr  : %"ID_vULONG_FMT,
                   sSlotPtr,
                   aRecPtr );

    sdcTSSlot::init( (sdcTSS*)aRecPtr, sTransID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SDR_SDC_UNBIND_TSS
 **********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_UNBIND_TSS( SChar       * aLogPtr,
                                           UInt          aSize,
                                           UChar       * aRecPtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * aMtx )
{
    SChar        * sValuePtr;
    smSCN          sSCN;
    sdcTSState     sState;

    IDE_ERROR( aLogPtr != NULL );
    IDE_ERROR( aRecPtr != NULL );
    IDE_ERROR( aMtx    != NULL );
    IDE_ERROR_MSG( aSize   == ( ID_SIZEOF(smSCN)      +
                                ID_SIZEOF(sdcTSState)),
                   "aSize : %"ID_UINT32_FMT,
                   aSize );

    sValuePtr = aLogPtr;

    idlOS::memcpy( &sSCN, sValuePtr, ID_SIZEOF(smSCN) );
    sValuePtr += ID_SIZEOF(smSCN);

    idlOS::memcpy( &sState, sValuePtr, ID_SIZEOF(sdcTSState) );

    sdcTSSlot::unbind( (sdcTSS*)aRecPtr, &sSCN, sState );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : SDR_SDC_SET_INITSCN_TO_CTS
 **********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_SET_INITSCN_TO_CTS( SChar       * /*aLogPtr*/,
                                                   UInt          aSize,
                                                   UChar       * aRecPtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx      * aMtx )
{
    smSCN          sSCN;

    IDE_ERROR( aRecPtr != NULL );
    IDE_ERROR( aMtx    != NULL );
    IDE_ERROR_MSG( aSize   == 0,
                   "aSize : %"ID_UINT32_FMT,
                   aSize );

    SM_INIT_SCN( &sSCN );

    sdcTSSlot::setCommitSCN( aMtx,
                             (sdcTSS*)aRecPtr,
                             &sSCN,
                             ID_TRUE ); // aDoUpdate

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/* type:  SDR_SDC_INIT_TSS_PAGE */
IDE_RC sdcUpdate::redo_SDR_SDC_INIT_TSS_PAGE( SChar       * aLogPtr,
                                              UInt          aSize,
                                              UChar       * aRecPtr,
                                              sdrRedoInfo * /*aRedoInfo*/,
                                              sdrMtx      * /*aMtx*/ )
{
    SChar       * sDataPtr;
    smSCN         sSCN;
    smTID         sTransID;

    IDE_ERROR_MSG( aSize == (ID_SIZEOF(smTID) + 
                             ID_SIZEOF(smSCN)),
                   "aSize : %"ID_UINT32_FMT,
                   aSize );

    sDataPtr = aLogPtr;

    idlOS::memcpy(&sTransID, sDataPtr, ID_SIZEOF(smTID));
    sDataPtr += ID_SIZEOF(smTID);

    idlOS::memcpy(&sSCN, sDataPtr, ID_SIZEOF(smSCN));

    sdcTSSegment::initPage( aRecPtr, sTransID, sSCN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* type:  SDR_SDC_INIT_UNDO_PAGE */
IDE_RC sdcUpdate::redo_SDR_SDC_INIT_UNDO_PAGE( SChar       * aLogPtr,
                                               UInt          aSize,
                                               UChar       * aRecPtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * /*aMtx*/ )
{
    sdpPageType   sPageType;

    IDE_ERROR_MSG( aSize == ID_SIZEOF(sdpPageType),
                   "aSize : %"ID_UINT32_FMT,
                   aSize );

    idlOS::memcpy(&sPageType, aLogPtr, ID_SIZEOF(sdpPageType));

    IDE_ERROR_MSG( sPageType == SDP_PAGE_UNDO,
                   "sPageType : %"ID_UINT32_FMT,
                   sPageType );

    IDE_ERROR( sdcUndoSegment::initPage( aRecPtr ) == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_BIND_CTS
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_BIND_CTS( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * /*aMtx*/ )
{
    SChar      * sDataPtr;
    smSCN        sFstDskViewSCN;
    sdRID        sTSSRID;
    scSlotNum    sRowSlotNum;
    UChar        sOldCTSlotIdx;
    UChar        sNewCTSlotIdx;
    idBool       sIncDeleteRowCnt;
    SShort       sFSCreditSize;

    sDataPtr = aData;

    idlOS::memcpy(&sOldCTSlotIdx, sDataPtr, ID_SIZEOF(UChar));
    sDataPtr += ID_SIZEOF(UChar);

    idlOS::memcpy(&sNewCTSlotIdx, sDataPtr, ID_SIZEOF(UChar));
    sDataPtr += ID_SIZEOF(UChar);

    idlOS::memcpy(&sRowSlotNum, sDataPtr, ID_SIZEOF(scSlotNum));
    sDataPtr += ID_SIZEOF(scSlotNum);

    if ( aLength == (ID_SIZEOF(UChar)     +
                     ID_SIZEOF(UChar)     +
                     ID_SIZEOF(scSlotNum) +
                     ID_SIZEOF(smSCN)     +
                     ID_SIZEOF(sdRID)     +
                     ID_SIZEOF(SShort)    +
                     ID_SIZEOF(idBool)) )
    {
        idlOS::memcpy(&sFstDskViewSCN, sDataPtr, ID_SIZEOF(smSCN));
        sDataPtr += ID_SIZEOF(smSCN);

        idlOS::memcpy(&sTSSRID, sDataPtr, ID_SIZEOF(sdRID));
        sDataPtr += ID_SIZEOF(sdRID);
    }
    else
    {
        IDE_ERROR_MSG( aLength == (ID_SIZEOF(UChar)     +
                                   ID_SIZEOF(UChar)     +
                                   ID_SIZEOF(scSlotNum) +
                                   ID_SIZEOF(SShort)    +
                                   ID_SIZEOF(idBool)),
                       "aLength : %"ID_UINT32_FMT,
                       aLength );

        SM_INIT_SCN( &sFstDskViewSCN );
        sTSSRID = SD_NULL_RID;
    }

    idlOS::memcpy(&sFSCreditSize, sDataPtr, ID_SIZEOF(SShort));
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy(&sIncDeleteRowCnt, sDataPtr, ID_SIZEOF(idBool));
    sDataPtr += ID_SIZEOF(idBool);

    IDE_TEST( sdcTableCTL::bindCTS4REDO( (sdpPhyPageHdr*)
                                            sdpPhyPage::getHdr(aPagePtr),
                                         sOldCTSlotIdx,
                                         sNewCTSlotIdx,
                                         sRowSlotNum,
                                         &sFstDskViewSCN,
                                         sTSSRID,
                                         sFSCreditSize,
                                         sIncDeleteRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_BIND_ROW
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_BIND_ROW( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * /*aMtx*/ )
{
    SChar      * sDataPtr;
    idBool       sIncDelRowCnt;
    smSCN        sFstDskViewSCN;
    SShort       sFSCreditSize;
    sdRID        sTSSlotRID;

    IDE_ERROR_MSG( aLength == (ID_SIZEOF(sdRID)  +
                               ID_SIZEOF(smSCN)  +
                               ID_SIZEOF(SShort) +
                               ID_SIZEOF(idBool)),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );
    sDataPtr = aData;

    idlOS::memcpy(&sTSSlotRID, sDataPtr, ID_SIZEOF(sdRID) );
    sDataPtr += ID_SIZEOF(sdRID);

    idlOS::memcpy(&sFstDskViewSCN, sDataPtr, ID_SIZEOF(smSCN) );
    sDataPtr += ID_SIZEOF(smSCN);

    idlOS::memcpy(&sFSCreditSize, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy(&sIncDelRowCnt, sDataPtr, ID_SIZEOF(idBool));
    sDataPtr += ID_SIZEOF(idBool);

    IDE_TEST( sdcTableCTL::bindRow4REDO( aPagePtr,
                                         sTSSlotRID,
                                         sFstDskViewSCN,
                                         sFSCreditSize,
                                         sIncDelRowCnt )
              != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_UNBIND_CTS
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_UNBIND_CTS( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * /*aMtx*/ )
{
    SChar     * sDataPtr;
    UChar       sCTSlotIdxBfrUndo;
    UChar       sCTSlotIdxAftUndo;
    SShort      sFSCreditSize = 0;
    idBool      sDecDelRowCnt;

    IDE_ERROR_MSG( aLength == (ID_SIZEOF(UChar)     +
                               ID_SIZEOF(UChar)     +
                               ID_SIZEOF(SShort)    +
                               ID_SIZEOF(idBool)),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy(&sCTSlotIdxBfrUndo, sDataPtr, ID_SIZEOF(UChar));
    sDataPtr += ID_SIZEOF(UChar);

    idlOS::memcpy(&sCTSlotIdxAftUndo, sDataPtr, ID_SIZEOF(UChar));
    sDataPtr += ID_SIZEOF(UChar);

    idlOS::memcpy( &sFSCreditSize, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy( &sDecDelRowCnt, sDataPtr, ID_SIZEOF(idBool) );

    IDE_TEST( sdcTableCTL::unbindCTS4REDO( (sdpPhyPageHdr*)aPagePtr,
                                           sCTSlotIdxBfrUndo,
                                           sCTSlotIdxAftUndo,
                                           sFSCreditSize,
                                           sDecDelRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_UNBIND_ROW
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_UNBIND_ROW( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * /*aMtx*/ )
{
    SChar     * sDataPtr;
    SShort      sFSCreditSize = 0;
    idBool      sDecDelRowCnt;

    IDE_ERROR_MSG( aLength == (ID_SIZEOF(SShort) + ID_SIZEOF(idBool)),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;

    idlOS::memcpy( &sFSCreditSize, sDataPtr, ID_SIZEOF(SShort) );
    sDataPtr += ID_SIZEOF(SShort);

    idlOS::memcpy( &sDecDelRowCnt, sDataPtr, ID_SIZEOF(idBool) );

    IDE_TEST( sdcTableCTL::unbindRow4REDO( aPagePtr,  /* aRowPieceBfrUndo */
                                           sFSCreditSize,
                                           sDecDelRowCnt )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_ROW_TIMESTAMPING
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_ROW_TIMESTAMPING( SChar       * aData,
                                                 UInt          aLength,
                                                 UChar       * aPagePtr,
                                                 sdrRedoInfo * /*aRedoInfo*/,
                                                 sdrMtx      * /*aMtx*/ )
{
    SChar     *  sDataPtr;
    UChar        sCTSlotIdx;
    smSCN        sCommitSCN;

    IDE_ERROR_MSG( aLength == ID_SIZEOF(UChar) + ID_SIZEOF(smSCN),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    sDataPtr = aData;
    idlOS::memcpy(&sCTSlotIdx, sDataPtr, ID_SIZEOF(UChar));
    sDataPtr += ID_SIZEOF(UChar);
    idlOS::memcpy(&sCommitSCN, sDataPtr, ID_SIZEOF(smSCN));

    if ( SDC_HAS_BOUND_CTS(sCTSlotIdx) )
    {
        IDE_TEST( sdcTableCTL::runRowStampingOnCTS( (sdpCTS*)aPagePtr, 
                                                    sCTSlotIdx, 
                                                    &sCommitSCN )
                  != IDE_SUCCESS );
    }        
    else
    {
        IDE_DASSERT( sCTSlotIdx == SDP_CTS_IDX_NULL );

        IDE_TEST( sdcTableCTL::runRowStampingOnRow( aPagePtr, 
                                                    sCTSlotIdx, 
                                                    &sCommitSCN )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_DATA_SELFAGING
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_DATA_SELFAGING( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * /*aMtx*/ )
{
    smSCN  sSCNtoAging;
    UShort sDummy;

    IDE_ERROR_MSG( aLength == ID_SIZEOF(smSCN),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    idlOS::memcpy(&sSCNtoAging, aData, ID_SIZEOF(smSCN));

    IDE_TEST( sdcTableCTL::runSelfAging( (sdpPhyPageHdr*)aPagePtr,
                                          &sSCNtoAging,
                                          &sDummy ) /* aAgedRowCnt */
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * redo type: SDR_SDC_INIT_CTL
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_INIT_CTL( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * /*aMtx*/ )
{
    UChar  sInitTrans;
    UChar  sMaxTrans;

    IDE_ERROR_MSG( aLength == (ID_SIZEOF(UChar) * 2),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    idlOS::memcpy(&sInitTrans, aData, ID_SIZEOF(UChar));

    idlOS::memcpy(&sMaxTrans, aData + ID_SIZEOF(UChar),
                  ID_SIZEOF(UChar));

    sdcTableCTL::init( (sdpPhyPageHdr*)aPagePtr, sInitTrans, sMaxTrans );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * PROJ-1704 redo type:  SDR_SDC_EXTEND_CTL
 ***********************************************************************/
IDE_RC sdcUpdate::redo_SDR_SDC_EXTEND_CTL( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * /*aMtx*/ )
{
    UChar   sSlotCnt;
    UChar   sSlotIdx;
    idBool  sTrySuccess = ID_FALSE;

    IDE_ERROR_MSG( aLength == ID_SIZEOF(UChar),
                   "aLength : %"ID_UINT32_FMT,
                   aLength );

    idlOS::memcpy(&sSlotCnt, aData, ID_SIZEOF(UChar));

    IDE_ERROR( sdcTableCTL::extend( (sdpPhyPageHdr*)aPagePtr,
                         sSlotCnt,
                         &sSlotIdx,
                         &sTrySuccess )
               == IDE_SUCCESS );
    IDE_ERROR( sTrySuccess == ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

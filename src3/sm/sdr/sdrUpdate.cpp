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
 * $Id: sdrUpdate.cpp 90937 2021-06-02 06:03:12Z emlee $
 *
 * Description :
 *
 * 본 파일은 DRDB의 redo/undo function map 대한 구현파일이다.
 *
 **********************************************************************/

#include <smu.h>
#include <sct.h>
#include <sdd.h>
#include <sdrDef.h>
#include <sdrUpdate.h>
#include <sdnUpdate.h>
#include <sdcUpdate.h>
#include <sdcLobUpdate.h>
#include <sdpUpdate.h>
#include <sdrMiniTrans.h>
#include <sdpsfUpdate.h>
#include <sdpscUpdate.h>
#include <sdpstUpdate.h>
#include <sdptbExtent.h>
#include <sdcRowUpdate.h>
#include <sdptbGroup.h>
#include <sdpDPathInfoMgr.h>

sdrDiskRedoFunction            gSdrDiskRedoFunction[SM_MAX_RECFUNCMAP_SIZE];
sdrDiskUndoFunction            gSdrDiskUndoFunction[SM_MAX_RECFUNCMAP_SIZE];
sdrDiskRefNTAUndoFunction      gSdrDiskRefNTAUndoFunction[SM_MAX_RECFUNCMAP_SIZE];

/* ------------------------------------------------
 * Description : DRDB의 redo/undo 함수 Vector 초기화
 * ----------------------------------------------*/
void sdrUpdate::initialize()
{

    idlOS::memset(gSdrDiskUndoFunction, 0x00,
                  ID_SIZEOF(sdrDiskUndoFunction) * SM_MAX_RECFUNCMAP_SIZE);
    idlOS::memset(gSdrDiskRedoFunction, 0x00,
                  ID_SIZEOF(sdrDiskRedoFunction) * SM_MAX_RECFUNCMAP_SIZE);
    idlOS::memset(gSdrDiskRefNTAUndoFunction, 0x00,
                  ID_SIZEOF(sdrDiskRefNTAUndoFunction) * SM_MAX_RECFUNCMAP_SIZE);

    /* ------------------------------------------------
     * map of undo function
     * ----------------------------------------------*/
    gSdrDiskUndoFunction[ SDR_SDC_INSERT_ROW_PIECE ]
        = sdcRowUpdate::undo_SDR_SDC_INSERT_ROW_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE ]
        = sdcRowUpdate::undo_SDR_SDC_INSERT_ROW_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_UPDATE_ROW_PIECE ]
        = sdcRowUpdate::undo_SDR_SDC_UPDATE_ROW_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_OVERWRITE_ROW_PIECE ]
        = sdcRowUpdate::undo_SDR_SDC_OVERWRITE_ROW_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_CHANGE_ROW_PIECE_LINK ]
        = sdcRowUpdate::undo_SDR_SDC_CHANGE_ROW_PIECE_LINK;

    gSdrDiskUndoFunction[ SDR_SDC_DELETE_FIRST_COLUMN_PIECE ]
        = sdcRowUpdate::undo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE ]
        = sdcRowUpdate::undo_SDR_SDC_DELETE_ROW_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_DELETE_ROW_PIECE ]
        = sdcRowUpdate::undo_SDR_SDC_DELETE_ROW_PIECE;

    gSdrDiskUndoFunction[ SDR_SDC_LOCK_ROW ]
        = sdcRowUpdate::undo_SDR_SDC_LOCK_ROW;

    gSdrDiskUndoFunction[ SDR_SDC_LOB_INSERT_LEAF_KEY ]
        = sdcLobUpdate::undo_SDR_SDC_LOB_INSERT_LEAF_KEY;

    gSdrDiskUndoFunction[ SDR_SDC_LOB_UPDATE_LEAF_KEY ]
        = sdcLobUpdate::undo_SDR_SDC_LOB_UPDATE_LEAF_KEY;

    gSdrDiskUndoFunction[ SDR_SDC_LOB_OVERWRITE_LEAF_KEY ]
        = sdcLobUpdate::undo_SDR_SDC_LOB_OVERWRITE_LEAF_KEY;

    /* ------------------------------------------------
     * map of redo function
     * ----------------------------------------------*/

    /* sdp */
    gSdrDiskRedoFunction[SDR_SDP_1BYTE]
        = sdpUpdate::redo_SDR_SDP_NBYTE;
    gSdrDiskRedoFunction[SDR_SDP_2BYTE]
        = sdpUpdate::redo_SDR_SDP_NBYTE;
    gSdrDiskRedoFunction[SDR_SDP_4BYTE]
        = sdpUpdate::redo_SDR_SDP_NBYTE;
    gSdrDiskRedoFunction[SDR_SDP_8BYTE]
        = sdpUpdate::redo_SDR_SDP_NBYTE;
    gSdrDiskRedoFunction[SDR_SDP_BINARY]
        = sdpUpdate::redo_SDR_SDP_BINARY;


    gSdrDiskRedoFunction[SDR_SDP_INIT_PHYSICAL_PAGE]
        = sdpUpdate::redo_SDR_SDP_INIT_PHYSICAL_PAGE;
    gSdrDiskRedoFunction[SDR_SDP_INIT_LOGICAL_HDR]
        = sdpUpdate::redo_SDR_SDP_INIT_LOGICAL_HDR;
    gSdrDiskRedoFunction[SDR_SDP_INIT_SLOT_DIRECTORY]
        = sdpUpdate::redo_SDR_SDP_INIT_SLOT_DIRECTORY;
    gSdrDiskRedoFunction[SDR_SDP_FREE_SLOT]
        = sdpUpdate::redo_SDR_SDP_FREE_SLOT;
    gSdrDiskRedoFunction[SDR_SDP_FREE_SLOT_FOR_SID]
        = sdpUpdate::redo_SDR_SDP_FREE_SLOT_FOR_SID;
    gSdrDiskRedoFunction[SDR_SDP_RESTORE_FREESPACE_CREDIT]
        = sdpUpdate::redo_SDR_SDP_RESTORE_FREESPACE_CREDIT;
    gSdrDiskRedoFunction[SDR_SDP_RESET_PAGE]
        = sdpUpdate::redo_SDR_SDP_RESET_PAGE;

    /* sdpst */
    gSdrDiskRedoFunction[SDR_SDPST_INIT_SEGHDR]
        = sdpstUpdate::redo_SDPST_INIT_SEGHDR;
    gSdrDiskRedoFunction[SDR_SDPST_INIT_LFBMP]
        = sdpstUpdate::redo_SDPST_INIT_LFBMP;
    gSdrDiskRedoFunction[SDR_SDPST_INIT_BMP]
        = sdpstUpdate::redo_SDPST_INIT_BMP;
    gSdrDiskRedoFunction[SDR_SDPST_INIT_EXTDIR]
        = sdpstUpdate::redo_SDPST_INIT_EXTDIR;
    gSdrDiskRedoFunction[SDR_SDPST_ADD_RANGESLOT]
        = sdpstUpdate::redo_SDPST_ADD_RANGESLOT;
    gSdrDiskRedoFunction[SDR_SDPST_ADD_SLOTS]
        = sdpstUpdate::redo_SDPST_ADD_SLOTS;
    gSdrDiskRedoFunction[SDR_SDPST_ADD_EXTDESC]
        = sdpstUpdate::redo_SDPST_ADD_EXTDESC;
    gSdrDiskRedoFunction[SDR_SDPST_ADD_EXT_TO_SEGHDR]
        = sdpstUpdate::redo_SDPST_ADD_EXT_TO_SEGHDR;
    gSdrDiskRedoFunction[SDR_SDPST_UPDATE_WM]
        = sdpstUpdate::redo_SDPST_UPDATE_WM;
    gSdrDiskRedoFunction[SDR_SDPST_UPDATE_MFNL]
        = sdpstUpdate::redo_SDPST_UPDATE_MFNL;
    gSdrDiskRedoFunction[SDR_SDPST_UPDATE_PBS]
        = sdpstUpdate::redo_SDPST_UPDATE_PBS;
    gSdrDiskRedoFunction[SDR_SDPST_UPDATE_LFBMP_4DPATH]
        = sdpstUpdate::redo_SDPST_UPDATE_LFBMP_4DPATH;

    gSdrDiskRedoFunction[SDR_SDPSC_INIT_SEGHDR]
        = sdpscUpdate::redo_SDPSC_INIT_SEGHDR;
    gSdrDiskRedoFunction[SDR_SDPSC_INIT_EXTDIR]
        = sdpscUpdate::redo_SDPSC_INIT_EXTDIR;
    gSdrDiskRedoFunction[SDR_SDPSC_ADD_EXTDESC_TO_EXTDIR]
        = sdpscUpdate::redo_SDPSC_ADD_EXTDESC_TO_EXTDIR;

    /* sdptb */
    gSdrDiskRedoFunction[SDR_SDPTB_INIT_LGHDR_PAGE]
        = sdptbUpdate::redo_SDPTB_INIT_LGHDR_PAGE;
    gSdrDiskRedoFunction[SDR_SDPTB_ALLOC_IN_LG]
        = sdptbUpdate::redo_SDPTB_ALLOC_IN_LG;
    gSdrDiskRedoFunction[SDR_SDPTB_FREE_IN_LG]
        = sdptbUpdate::redo_SDPTB_FREE_IN_LG;

    gSdrDiskRedoFunction[SDR_SDP_WRITE_PAGEIMG]
        = sdpUpdate::redo_SDR_SDP_WRITE_PAGEIMG;

    // PROJ-1665 : Direct-Path Ins 수행된 Page 전체에 대한 redo
    gSdrDiskRedoFunction[SDR_SDP_WRITE_DPATH_INS_PAGE]
        = sdpUpdate::redo_SDR_SDP_WRITE_PAGEIMG;

    // PROJ-1665 : Page에 대한 Consistent 정보 설정
    gSdrDiskRedoFunction[SDR_SDP_PAGE_CONSISTENT]
        = sdpUpdate::redo_SDR_SDP_PAGE_CONSISTENT;

    /* sdc */
    gSdrDiskRedoFunction[SDR_SDC_INSERT_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE]
        = sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO]
        = sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO;
    gSdrDiskRedoFunction[SDR_SDC_UPDATE_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_UPDATE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_OVERWRITE_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_OVERWRITE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_CHANGE_ROW_PIECE_LINK]
        = sdcRowUpdate::redo_SDR_SDC_CHANGE_ROW_PIECE_LINK;
    gSdrDiskRedoFunction[SDR_SDC_DELETE_FIRST_COLUMN_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE]
        = sdcRowUpdate::redo_SDR_SDC_DELETE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_DELETE_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_DELETE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_LOCK_ROW]
        = sdcRowUpdate::redo_SDR_SDC_LOCK_ROW;

    gSdrDiskRedoFunction[SDR_SDC_PK_LOG] = redoNA;

    gSdrDiskRedoFunction[SDR_SDC_INIT_CTL]
        = sdcUpdate::redo_SDR_SDC_INIT_CTL;
    gSdrDiskRedoFunction[SDR_SDC_EXTEND_CTL]
        = sdcUpdate::redo_SDR_SDC_EXTEND_CTL;

    /* sdc */
    gSdrDiskRedoFunction[SDR_SDC_BIND_TSS]
        = sdcUpdate::redo_SDR_SDC_BIND_TSS;
    gSdrDiskRedoFunction[SDR_SDC_UNBIND_TSS]
        = sdcUpdate::redo_SDR_SDC_UNBIND_TSS;
    gSdrDiskRedoFunction[SDR_SDC_SET_INITSCN_TO_TSS]
        = sdcUpdate::redo_SDR_SDC_SET_INITSCN_TO_CTS;
    gSdrDiskRedoFunction[SDR_SDC_INIT_TSS_PAGE]
        = sdcUpdate::redo_SDR_SDC_INIT_TSS_PAGE;
    gSdrDiskRedoFunction[SDR_SDC_INIT_UNDO_PAGE]
        = sdcUpdate::redo_SDR_SDC_INIT_UNDO_PAGE;

    gSdrDiskRedoFunction[SDR_SDC_BIND_CTS]
        = sdcUpdate::redo_SDR_SDC_BIND_CTS;
    gSdrDiskRedoFunction[SDR_SDC_UNBIND_CTS]
        = sdcUpdate::redo_SDR_SDC_UNBIND_CTS;
    gSdrDiskRedoFunction[SDR_SDC_BIND_ROW]
        = sdcUpdate::redo_SDR_SDC_BIND_ROW;
    gSdrDiskRedoFunction[SDR_SDC_UNBIND_ROW]
        = sdcUpdate::redo_SDR_SDC_UNBIND_ROW;
    gSdrDiskRedoFunction[SDR_SDC_ROW_TIMESTAMPING]
        = sdcUpdate::redo_SDR_SDC_ROW_TIMESTAMPING;
    gSdrDiskRedoFunction[SDR_SDC_DATA_SELFAGING]
        = sdcUpdate::redo_SDR_SDC_DATA_SELFAGING;

    gSdrDiskRedoFunction[SDR_SDC_INSERT_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_INSERT_ROW_PIECE_FOR_UPDATE]
        = sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO]
        = sdcRowUpdate::redo_SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO;
    gSdrDiskRedoFunction[SDR_SDC_UPDATE_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_UPDATE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_OVERWRITE_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_OVERWRITE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_CHANGE_ROW_PIECE_LINK]
        = sdcRowUpdate::redo_SDR_SDC_CHANGE_ROW_PIECE_LINK;
    gSdrDiskRedoFunction[SDR_SDC_DELETE_FIRST_COLUMN_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_ADD_FIRST_COLUMN_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_ADD_FIRST_COLUMN_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_DELETE_ROW_PIECE_FOR_UPDATE]
        = sdcRowUpdate::redo_SDR_SDC_DELETE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_DELETE_ROW_PIECE]
        = sdcRowUpdate::redo_SDR_SDC_DELETE_ROW_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_LOCK_ROW]
        = sdcRowUpdate::redo_SDR_SDC_LOCK_ROW;

    gSdrDiskRedoFunction[SDR_SDC_INSERT_UNDO_REC]
        = sdcUpdate::redo_SDR_SDC_INSERT_UNDO_REC;

    /* sdc - lob */
    gSdrDiskRedoFunction[SDR_SDC_LOB_UPDATE_LOBDESC]
        = sdcLobUpdate::redo_SDR_SDC_LOB_UPDATE_LOBDESC;
    gSdrDiskRedoFunction[SDR_SDC_LOB_INSERT_INTERNAL_KEY]
        = sdcLobUpdate::redo_SDR_SDC_LOB_INSERT_INTERNAL_KEY;
    gSdrDiskRedoFunction[SDR_SDC_LOB_INSERT_LEAF_KEY]
        = sdcLobUpdate::redo_SDR_SDC_LOB_INSERT_LEAF_KEY;
    gSdrDiskRedoFunction[SDR_SDC_LOB_UPDATE_LEAF_KEY]
        = sdcLobUpdate::redo_SDR_SDC_LOB_UPDATE_LEAF_KEY;
    gSdrDiskRedoFunction[SDR_SDC_LOB_OVERWRITE_LEAF_KEY]
        = sdcLobUpdate::redo_SDR_SDC_LOB_OVERWRITE_LEAF_KEY;
    gSdrDiskRedoFunction[SDR_SDC_LOB_FREE_INTERNAL_KEY]
        = sdcLobUpdate::redo_SDR_SDC_LOB_FREE_INTERNAL_KEY;
    gSdrDiskRedoFunction[SDR_SDC_LOB_FREE_LEAF_KEY]
        = sdcLobUpdate::redo_SDR_SDC_LOB_FREE_LEAF_KEY;
    gSdrDiskRedoFunction[SDR_SDC_LOB_WRITE_PIECE]
        = sdcLobUpdate::redo_SDR_SDC_LOB_WRITE_PIECE;
    gSdrDiskRedoFunction[SDR_SDC_LOB_WRITE_PIECE4DML]
        = sdcLobUpdate::redo_SDR_SDC_LOB_WRITE_PIECE4DML;
    gSdrDiskRedoFunction[SDR_SDC_LOB_WRITE_PIECE_PREV]
        = sdcLobUpdate::redo_SDR_SDC_LOB_WRITE_PIECE_PREV;
    gSdrDiskRedoFunction[SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST]
        = sdcLobUpdate::redo_SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST;

    /* sdn ..... */
    gSdrDiskRedoFunction[SDR_SDN_INSERT_INDEX_KEY]
        = sdnUpdate::redo_SDR_SDN_INSERT_INDEX_KEY;
    gSdrDiskRedoFunction[SDR_SDN_FREE_INDEX_KEY]
        = sdnUpdate::redo_SDR_SDN_FREE_INDEX_KEY;
    gSdrDiskRedoFunction[SDR_SDN_INSERT_UNIQUE_KEY]
        = sdnUpdate::redo_SDR_SDN_INSERT_UNIQUE_KEY;
    gSdrDiskRedoFunction[SDR_SDN_INSERT_DUP_KEY]
        = sdnUpdate::redo_SDR_SDN_INSERT_DUP_KEY;
    gSdrDiskRedoFunction[SDR_SDN_DELETE_KEY_WITH_NTA]
        = sdnUpdate::redo_SDR_SDN_DELETE_KEY_WITH_NTA;
    gSdrDiskRedoFunction[SDR_SDN_FREE_KEYS]
        = sdnUpdate::redo_SDR_SDN_FREE_KEYS;
    gSdrDiskRedoFunction[SDR_SDN_COMPACT_INDEX_PAGE]
        = sdnUpdate::redo_SDR_SDN_COMPACT_INDEX_PAGE;
    gSdrDiskRedoFunction[SDR_SDN_KEY_STAMPING]
        = sdnUpdate::redo_SDR_SDN_KEY_STAMPING;
    gSdrDiskRedoFunction[SDR_SDN_INIT_CTL]
        = sdnUpdate::redo_SDR_SDN_INIT_CTL;
    gSdrDiskRedoFunction[SDR_SDN_EXTEND_CTL]
        = sdnUpdate::redo_SDR_SDN_EXTEND_CTL;
    gSdrDiskRedoFunction[SDR_SDN_FREE_CTS]
        = sdnUpdate::redo_SDR_SDN_FREE_CTS;

    /* ------------------------------------------------
     * map of ref nta undo function
     * ----------------------------------------------*/

    /* sdn .... */
    gSdrDiskRefNTAUndoFunction[ SDR_SDN_INSERT_UNIQUE_KEY ]
        = sdnUpdate::undo_SDR_SDN_INSERT_UNIQUE_KEY;
    gSdrDiskRefNTAUndoFunction[ SDR_SDN_INSERT_DUP_KEY ]
        = sdnUpdate::undo_SDR_SDN_INSERT_DUP_KEY;
    gSdrDiskRefNTAUndoFunction[ SDR_SDN_DELETE_KEY_WITH_NTA ] 
        = sdnUpdate::undo_SDR_SDN_DELETE_KEY_WITH_NTA;

    /* sdc - lob */
    gSdrDiskRefNTAUndoFunction[ SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST ]
        = sdcLobUpdate::undo_SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST;

    return;
}

void sdrUpdate::appendExternalUndoFunction( UInt                aUndoMapID,
                                            sdrDiskUndoFunction aDiskUndoFunction )
{
    gSdrDiskUndoFunction[ aUndoMapID ] = aDiskUndoFunction;
}

void sdrUpdate::appendExternalRedoFunction( UInt                aRedoMapID,
                                            sdrDiskRedoFunction aDiskRedoFunction )
{
    gSdrDiskRedoFunction[ aRedoMapID ] = aDiskRedoFunction;
}

void sdrUpdate::appendExternalRefNTAUndoFunction( 
                            UInt                           aRefNTAUndoMapID,
                            sdrDiskRefNTAUndoFunction      aDiskRefNTAUndoFunction )
{
    gSdrDiskRefNTAUndoFunction[ aRefNTAUndoMapID ] = aDiskRefNTAUndoFunction;
}

/***********************************************************************
 * Description : DRDB 로그-based undo 함수
 **********************************************************************/
IDE_RC sdrUpdate::doUndoFunction( idvSQL * aStatistics,
                                  smTID    aTransID,
                                  smOID    aOID,
                                  SChar  * aLogPtr,
                                  smLSN  * aPrevLSN )
{
    sdrLogHdr   sLogHdr;

    IDE_DASSERT( aLogPtr != NULL );

    idlOS::memcpy(&sLogHdr, aLogPtr, ID_SIZEOF(sdrLogHdr));

    IDE_ERROR_MSG( gSdrDiskUndoFunction[sLogHdr.mType] != NULL,
                   "sLogHdr.mType : %"ID_UINT32_FMT"\n", sLogHdr.mType );

    return gSdrDiskUndoFunction[sLogHdr.mType]( aStatistics,
                                                aTransID,
                                                aOID,
                                                sLogHdr.mGRID,
                                                aLogPtr,
                                                aPrevLSN );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : DRDB 로그-based redo 함수
 **********************************************************************/
IDE_RC sdrUpdate::doRedoFunction( SChar       * aValue,
                                  UInt          aValueLen,
                                  UChar       * aPageOffset,
                                  sdrRedoInfo * aRedoInfo,
                                  sdrMtx      * aMtx )
{
    IDE_ERROR_MSG( gSdrDiskRedoFunction[aRedoInfo->mLogType] != NULL,
                   "aRedoInfo->mLogType : %"ID_UINT32_FMT"\n", aRedoInfo->mLogType ); 

    return gSdrDiskRedoFunction[aRedoInfo->mLogType]( aValue,
                                                      aValueLen,
                                                      aPageOffset,
                                                      aRedoInfo,
                                                      aMtx );


    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * redo type:  DRDB NTA로그의 logical undo 수행
 ***********************************************************************/
IDE_RC sdrUpdate::doNTAUndoFunction( idvSQL   * aStatistics,
                                     void     * aTrans,
                                     UInt       aOPType,
                                     scSpaceID  aSpaceID,
                                     smLSN    * aPrevLSN,
                                     ULong    * aArrData,
                                     UInt       aDataCount )
{
    UInt       sState = 0;
    sdrMtx     sMtx;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aOPType < SDR_OP_MAX );

    if ((sdrOPType)aOPType != SDR_OP_NULL)
    {
        IDE_TEST( sdrMiniTrans::begin(aStatistics, /* idvSQL* */
                                      &sMtx,
                                      aTrans,
                                      SDR_MTX_LOGGING,
                                      ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;

        switch ((sdrOPType)aOPType)
        {
            case SDR_OP_SDP_CREATE_TABLE_SEGMENT :
            case SDR_OP_SDP_CREATE_INDEX_SEGMENT :
            case SDR_OP_SDP_CREATE_LOB_SEGMENT :

                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpSegment::freeSeg4OperUndo(
                                              aStatistics,
                                              aSpaceID,
                                              aArrData[0],
                                              (sdrOPType)aOPType,
                                              &sMtx) 
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPST_UPDATE_WMINFO_4DPATH:
                //BUG-48460: NTA로그( SDR_OP_SDPST_UPDATE_WMINFO_4DPATH ) 중 
                //           discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpstUpdate::undo_SDPST_UPDATE_WM_4DPATH(
                                                aStatistics,
                                                &sMtx,
                                                aSpaceID,
                                                aArrData[0],  /* Seg PID */
                                                aArrData[1],  /* Prev HWM Ext RID*/
                                                aArrData[2]   /* Prev HWM PID*/ )
                               != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPST_UPDATE_BMP_4DPATH:
                //BUG-48460: NTA로그 ( SDR_OP_SDPST_UPDATE_BMP_4DPATH ) 중 
                //           discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpstUpdate::undo_SDPST_UPDATE_BMP_4DPATH(
                                        aStatistics,
                                        &sMtx,
                                        aSpaceID,
                                        (scPageID)aArrData[0],  /* BMP */
                                        (SShort)aArrData[1],    /* From SlotNo */
                                        (SShort)aArrData[2],    /* To SlotNo */
                                        (sdpstMFNL)aArrData[3], /* Prv MFNL */
                                        (sdpstMFNL)aArrData[4] )/* Prv LstSlot MFNL */
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPSF_MERGE_SEG_4DPATH:
                //BUG-48460: NTA로그 ( SDR_OP_SDPSF_MERGE_SEG_4DPATH ) 중 
                //           discard TBS접근 가능한 로그에 TBS 체크 추가
                //           ( 이 로그타입은 write 할 때 안 씀? ) 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpsfUpdate::undo_SDPSF_MERGE_SEG_4DPATH(
                                            aStatistics,
                                            &sMtx,
                                            aSpaceID,
                                            aArrData[0],   /* ToSeg PID */
                                            aArrData[1],   /* LstAllocExtRID Of ToSeg */
                                            aArrData[2],   /* FstPIDOfLstAllocExt Of ToSeg */
                                            aArrData[3],   /* FmtPageCnt Of ToSeg */
                                            aArrData[4] )  /* HWM Of ToSeg */
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPSF_UPDATE_HWMINFO_4DPATH:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpsfUpdate::undo_UPDATE_HWM_4DPATH(
                                            aStatistics,
                                            &sMtx,
                                            aSpaceID,
                                            aArrData[0],  /* Seg PID */
                                            aArrData[1],  /* HWM */
                                            aArrData[2],  /* Alloc Extent RID */
                                            aArrData[3],  /* First PID Of Alloc Extent */
                                            aArrData[4] ) /* Alloc Page Cnt */
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPSF_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                //( 이 로그타입은 write 할 때 안 씀? )
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpsfUpdate::undo_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH(
                                                        aStatistics,
                                                        &sMtx,
                                                        aSpaceID,
                                                        aArrData[0],  /* Seg PID */
                                                        aArrData[1],  /* First PageID */
                                                        aArrData[2],  /* Last  PageID */
                                                        aArrData[3] ) /* Page Count */
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPTB_ALLOCATE_AN_EXTENT_FROM_TBS:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    //해제할 extent의 갯수는 1개~4개 일수있다.
                    IDE_ERROR( aDataCount <= 4 );

                    IDE_TEST( sdptbExtent::freeExts( aStatistics,
                                                     &sMtx,
                                                     aSpaceID,
                                                     &aArrData[0],
                                                     aDataCount )
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPTB_RESIZE_GG:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_ERROR( aDataCount == 2 );

                    IDE_TEST( sdptbGroup::resizeGG( aStatistics,
                                                    &sMtx,
                                                    aSpaceID,
                                                    aArrData[0],   //GGID
                                                    aArrData[1])   //prv page cnt
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPSF_ALLOC_PAGE:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpsfUpdate::undo_SDPSF_ALLOC_PAGE( aStatistics,
                                                                  &sMtx,
                                                                  aSpaceID,
                                                                  aArrData[0],  /* SegPID */
                                                                  aArrData[1] ) /* Alloc PID */
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_ERROR( (sdpFreeExtDirType)aArrData[0] < SDP_MAX_FREE_EXTDIR_LIST );

                    IDE_TEST( sdptbUpdate::undo_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST(
                                            aStatistics,
                                            &sMtx,
                                            aSpaceID,
                                            (sdpFreeExtDirType)aArrData[0],  /* aFreeListIdx */
                                            (scPageID)aArrData[1] )          /* Alloc ExtDirPID */
                        != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDP_DPATH_ADD_SEGINFO:
                IDE_TEST( sdpUpdate::undo_SDR_OP_SDP_DPATH_ADD_SEGINFO(
                                                aStatistics,
                                                &sMtx,
                                                aSpaceID,
                                                aArrData[0] ) /* SegInfoSet SeqNo */
                            != IDE_SUCCESS );
                break;

            case SDR_OP_SDPST_ALLOC_PAGE:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                }
                else
                {
                    IDE_TEST( sdpstUpdate::undo_SDPST_ALLOC_PAGE(
                                                    aStatistics,
                                                    &sMtx,
                                                    aSpaceID,
                                                    aArrData[0],  /* Seg PID */
                                                    aArrData[1] ) /* Alloc PID */
                              != IDE_SUCCESS );
                }
                break;

            case SDR_OP_SDC_LOB_APPEND_LEAFNODE:
                //BUG-48460: NTA로그 중 discard TBS접근 가능한 로그에 TBS 체크 추가 
                if ( sctTableSpaceMgr::hasState( aSpaceID,
                                                 SCT_SS_SKIP_UNDO )
                     == ID_TRUE )
                {
                    // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
                    ideLog::log(IDE_ERR_0, "undo SDR_OP_SDC_LOB_APPEND_LEAFNODE");
                }
                else
                {
                    IDE_TEST( sdcLobUpdate::undo_SDR_SDC_LOB_APPEND_LEAFNODE(
                                                    aStatistics,
                                                    &sMtx,
                                                    aSpaceID,
                                                    aArrData[0],  /* RootNode PID */
                                                    aArrData[1] ) /* LeafNode PID */
                              != IDE_SUCCESS );
                } 
               break;

            default:
                break;
        }

        sdrMiniTrans::setCLR( &sMtx, aPrevLSN ); // dummy CLR

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

        /* ------------------------------------------------
         * !! CHECK RECOVERY POINT
         * case) nta logical undo를 수행한 이후에 crash 발생
         * 해당 nta logical undo의 prev undo lsn의 로그부터
         * 이어서 undo를 진행한다.
         * ----------------------------------------------*/
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)sdrMiniTrans::rollback(&sMtx);
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * redo type:  DRDB Index/LOB/외부 NTA로그의 logical undo 수행
 ***********************************************************************/
IDE_RC sdrUpdate::doRefNTAUndoFunction( idvSQL   * aStatistics,
                                        void     * aTrans,
                                        UInt       aOPType,
                                        smLSN    * aPrevLSN,
                                        SChar    * aRefData )
{
    UInt       sState = 0;
    sdrMtx     sMtx;
    sdrLogHdr  sLogHdr;

    IDE_ERROR( aTrans != NULL );
    IDE_ERROR( aOPType < SDR_OP_MAX );

    if ((sdrOPType)aOPType != SDR_OP_NULL)
    {
        IDE_TEST( sdrMiniTrans::begin(aStatistics, /* idvSQL* */
                                      &sMtx,
                                      aTrans,
                                      SDR_MTX_LOGGING,
                                      ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                      SM_DLOG_ATTR_DEFAULT)
                  != IDE_SUCCESS );
        sState = 1;

        /* BUG-25279 Btree for spatial과 Disk Btree의 자료구조 및 로깅 분리
         * NTA Undo도 다른 undo나 redo연산처럼 Map으로 수행한다. */
        idlOS::memcpy(&sLogHdr, aRefData, ID_SIZEOF(sdrLogHdr));
        
        if ( sctTableSpaceMgr::hasState( sLogHdr.mGRID.mSpaceID,
                                         SCT_SS_SKIP_UNDO )
             == ID_TRUE )
        {
            // Skip 할 Undo 로그라면 Dummy CLR만 로깅한다.
        }
        else
        {
            IDE_ERROR( gSdrDiskRefNTAUndoFunction[sLogHdr.mType] != 0 );
            IDE_TEST(  gSdrDiskRefNTAUndoFunction[sLogHdr.mType] (
                                            aStatistics,
                                            aTrans,
                                            &sMtx,
                                            sLogHdr.mGRID,
                                            aRefData + ID_SIZEOF(sdrLogHdr),
                                            sLogHdr.mLength )
                       != IDE_SUCCESS );
        }
        sdrMiniTrans::setCLR( &sMtx, aPrevLSN ); // dummy CLR

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

        /* ------------------------------------------------
         * !! CHECK RECOVERY POINT
         * case) nta logical undo를 수행한 이후에 crash 발생
         * 해당 nta logical undo의 prev undo lsn의 로그부터
         * 이어서 undo를 진행한다.
         * ----------------------------------------------*/
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState != 0)
    {
        (void)sdrMiniTrans::rollback(&sMtx);
    }

    return IDE_FAILURE;

}



/***********************************************************************
 * Description : Redo Not Available
 **********************************************************************/
IDE_RC sdrUpdate::redoNA( SChar       * /*aData*/,
                          UInt          /*aLength*/,
                          UChar       * /*aPagePtr*/,
                          sdrRedoInfo * /*aRedoInfo*/,
                          sdrMtx      * /*aMtx*/ )
{
    return IDE_SUCCESS;
}

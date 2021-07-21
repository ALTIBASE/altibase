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
 * $Id: sdpUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SDP_UPDATE_H_
#define _O_SDP_UPDATE_H_ 1

#include <sdp.h>
#include <smr.h>

class sdpUpdate
{
//For Operation
public:

    /* redo type:  SDR_SDP_1BYTE, SDR_SDP_2BYTE, SDR_SDP_4BYTE, SDR_SDP_8BYTE */
    static IDE_RC redo_SDR_SDP_NBYTE( SChar       * aData,
                                      UInt          aLength,
                                      UChar       * aPagePtr,
                                      sdrRedoInfo * /*aRedoInfo*/,
                                      sdrMtx      * aMtx );

    /* redo type:  SDR_SDP_BINARY */
    static IDE_RC redo_SDR_SDP_BINARY( SChar       * aData,
                                       UInt          aLength,
                                       UChar       * aPagePtr,
                                       sdrRedoInfo * /*aRedoInfo*/,
                                       sdrMtx      * aMtx );

    /* redo type:  SDR_SDP_INIT_PHYSICAL_PAGE */
    static IDE_RC redo_SDR_SDP_INIT_PHYSICAL_PAGE( SChar       * aData,
                                                   UInt          aLength,
                                                   UChar       * aPagePtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx      * aMtx );

    /* redo type:  SDR_SDP_INIT_LOGICAL_HDR */
    static IDE_RC redo_SDR_SDP_INIT_LOGICAL_HDR( SChar       * aData,
                                                 UInt          aLength,
                                                 UChar       * aPagePtr,
                                                 sdrRedoInfo * /*aRedoInfo*/,
                                                 sdrMtx      * aMtx );

    /* redo type:  SDR_SDP_INIT_SLOT_DIRECTORY */
    static IDE_RC redo_SDR_SDP_INIT_SLOT_DIRECTORY( SChar       * aData,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx      * aMtx );

    /* redo type: SDR_SDP_FREE_SLOT */
    static IDE_RC redo_SDR_SDP_FREE_SLOT( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * aMtx );

    /* redo type: SDR_SDP_FREE_SLOT_FOR_SID */
    static IDE_RC redo_SDR_SDP_FREE_SLOT_FOR_SID( SChar       * aData,
                                                  UInt          aLength,
                                                  UChar       * aPagePtr,
                                                  sdrRedoInfo * aRedoInfo,
                                                  sdrMtx      * aMtx );

    /* redo type:  SDR_SDP_RESTORE_FREESPACE_CREDIT */
    static IDE_RC
        redo_SDR_SDP_RESTORE_FREESPACE_CREDIT( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * aMtx );

    /* redo type:  SDR_SDP_RESET_PAGE */
    // BUG-17615
    static IDE_RC redo_SDR_SDP_RESET_PAGE( SChar       * aData,
                                           UInt          aLength,
                                           UChar       * aPagePtr,
                                           sdrRedoInfo * /*aRedoInfo*/,
                                           sdrMtx      * aMtx );

    /* PROJ-1665,1867 redo type: SDR_SDP_WRITE_PAGEIMG, SDR_SDP_DPATH_INS_PAGE */
    static IDE_RC redo_SDR_SDP_WRITE_PAGEIMG( SChar       * aData,
                                              UInt          aLength,
                                              UChar       * aPagePtr,
                                              sdrRedoInfo * aRedoInfo,
                                              sdrMtx      * aMtx );

    /* PROJ-1665 redo type:  SDR_SDP_PAGE_CONSISTENT */
    static IDE_RC redo_SDR_SDP_PAGE_CONSISTENT( SChar       * aData,
                                                UInt          aLength,
                                                UChar       * aPagePtr,
                                                sdrRedoInfo * aRedoInfo,
                                                sdrMtx      * aMtx );

    /* undo type: SDR_OP_SDP_DPATH_ADD_SEGINFO */
    static IDE_RC undo_SDR_OP_SDP_DPATH_ADD_SEGINFO(
                                           idvSQL      * aStatistics,
                                           sdrMtx      * aMtx,
                                           scSpaceID     /*aSpaceID*/,
                                           UInt          aSegInfoSeqNo );
};

#endif

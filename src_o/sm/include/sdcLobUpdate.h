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
 * $Id: sdcLobUpdate.h 56720 2012-12-04 10:44:30Z jiko $
 **********************************************************************/

#ifndef _O_SDC_LOB_UPDATE_H_
#define _O_SDC_LOB_UPDATE_H_ 1

#include <smDef.h>
#include <sdcDef.h>

class sdcLobUpdate
{
public:
    
    /*
     * PROJ-2047 Strengthening LOB
     */
    
    static IDE_RC redo_SDR_SDC_LOB_UPDATE_LOBDESC( SChar       * aLogPtr,
                                                   UInt          /*aSize*/,
                                                   UChar       * aSlotPtr,
                                                   sdrRedoInfo * /*aRedoInfo*/,
                                                   sdrMtx      * /*aMtx*/ );
    
    static IDE_RC redo_SDR_SDC_LOB_INSERT_INTERNAL_KEY( SChar       * aData,
                                                        UInt          aLength,
                                                        UChar       * aPagePtr,
                                                        sdrRedoInfo * /*aRedoInfo*/,
                                                        sdrMtx      * aMtx );

    static IDE_RC redo_SDR_SDC_LOB_FREE_INTERNAL_KEY( SChar       * aData,
                                                      UInt          aLength,
                                                      UChar       * aPagePtr,
                                                      sdrRedoInfo * /*aRedoInfo*/,
                                                      sdrMtx      * aMtx );

    static IDE_RC redo_SDR_SDC_LOB_FREE_LEAF_KEY( SChar       * aData,
                                                  UInt          aLength,
                                                  UChar       * aPagePtr,
                                                  sdrRedoInfo * /*aRedoInfo*/,
                                                  sdrMtx      * aMtx );

    static IDE_RC redo_SDR_SDC_LOB_WRITE_PIECE( SChar       * aData,
                                                UInt          /* aLength */,
                                                UChar       * aPagePtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_LOB_WRITE_PIECE_PREV( SChar       * aData,
                                                     UInt          /* aLength */,
                                                     UChar       * aPagePtr,
                                                     sdrRedoInfo * /*aRedoInfo*/,
                                                     sdrMtx      * /*aMtx*/);

    static IDE_RC redo_SDR_SDC_LOB_WRITE_PIECE4DML( SChar       * aData,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx      * /*aMtx*/ );
    
    static IDE_RC redo_SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST( SChar       * aData,
                                                          UInt          aLength,
                                                          UChar       * aPagePtr,
                                                          sdrRedoInfo * /*aRedoInfo*/,
                                                          sdrMtx      * aMtx );

    static IDE_RC undo_SDR_SDC_LOB_ADD_PAGE_TO_AGINGLIST( idvSQL   * aStatistics,
                                                          void     * aTrans,
                                                          sdrMtx   * aMtx,
                                                          scGRID     aGRID,
                                                          SChar    * aLogPtr,
                                                          UInt       aSize );

    static IDE_RC undo_SDR_SDC_LOB_APPEND_LEAFNODE( idvSQL    * aStatistics,
                                                    sdrMtx    * aMtx,
                                                    scSpaceID   aSpaceID,
                                                    scPageID    aRootNodePID,
                                                    scPageID    aLeafNodePID );

    static IDE_RC redo_SDR_SDC_LOB_INSERT_LEAF_KEY( SChar       * aData,
                                                    UInt          aLength,
                                                    UChar       * aPagePtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx      * aMtx );

    static IDE_RC undo_SDR_SDC_LOB_INSERT_LEAF_KEY( idvSQL  * aStatistics,
                                                    smTID     aTransID,
                                                    smOID     aOID,
                                                    scGRID    aRecGRID,
                                                    SChar   * aLogPtr,
                                                    smLSN   * aPrevLSN );

    static IDE_RC redo_SDR_SDC_LOB_UPDATE_LEAF_KEY( SChar       * aData,
                                                    UInt          /*aSize*/,
                                                    UChar       * aSlotPtr,
                                                    sdrRedoInfo * /*aRedoInfo*/,
                                                    sdrMtx      * /*aMtx*/ );

    static IDE_RC undo_SDR_SDC_LOB_UPDATE_LEAF_KEY( idvSQL  * aStatistics,
                                                    smTID     aTransID,
                                                    smOID     aOID,
                                                    scGRID    aRecGRID,
                                                    SChar   * aLogPtr,
                                                    smLSN   * aPrevLSN );

    static IDE_RC redo_SDR_SDC_LOB_OVERWRITE_LEAF_KEY( SChar       * aData,
                                                       UInt          aLength,
                                                       UChar       * aSlotPtr,
                                                       sdrRedoInfo * /*aRedoInfo*/,
                                                       sdrMtx      * /*aMtx*/ );

    static IDE_RC undo_SDR_SDC_LOB_OVERWRITE_LEAF_KEY( idvSQL  * aStatistics,
                                                       smTID     aTransID,
                                                       smOID     aOID,
                                                       scGRID    aRecGRID,
                                                       SChar   * aLogPtr,
                                                       smLSN   * aPrevLSN );

private:
    static IDE_RC redoLobWritePiece( SChar   * aData,
                                     UInt      aLength,
                                     UChar   * aPagePtr );
};


#endif // _O_SDC_LOB_UPDATE_H_

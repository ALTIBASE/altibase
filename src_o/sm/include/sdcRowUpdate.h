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
 * $Id: sdcRowUpdate.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SDC_ROW_UPDATE_H_
#define _O_SDC_ROW_UPDATE_H_ 1

#include <sdcDef.h>
#include <sdcRow.h>

/* row에 대한 undo, redo를 수행한다. */
class sdcRowUpdate
{
public:

    static IDE_RC redo_SDR_SDC_INSERT_ROW_PIECE( SChar       * aLogPtr,
                                                 UInt          aSize,
                                                 UChar       * aSlotPtr,
                                                 sdrRedoInfo * aRedoInfo,
                                                 sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_INSERT_ROW_PIECE_FOR_DELETEUNDO( SChar       * aLogPtr,
                                                                UInt          aSize,
                                                                UChar       * aSlotPtr,
                                                                sdrRedoInfo * aRedoInfo,
                                                                sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_UPDATE_ROW_PIECE( SChar       * aLogPtr,
                                                 UInt          aSize,
                                                 UChar       * aSlotPtr,
                                                 sdrRedoInfo * aRedoInfo,
                                                 sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_OVERWRITE_ROW_PIECE( SChar       * aLogPtr,
                                                    UInt          aSize,
                                                    UChar       * aSlotPtr,
                                                    sdrRedoInfo * aRedoInfo,
                                                    sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_CHANGE_ROW_PIECE_LINK( SChar       * aLogPtr,
                                                      UInt          aSize,
                                                      UChar       * aSlotPtr,
                                                      sdrRedoInfo * aRedoInfo,
                                                      sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE( SChar       * aLogPtr,
                                                          UInt          aSize,
                                                          UChar       * aSlotPtr,
                                                          sdrRedoInfo * aRedoInfo,
                                                          sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_ADD_FIRST_COLUMN_PIECE( SChar       * aLogPtr,
                                                       UInt          aSize,
                                                       UChar       * aSlotPtr,
                                                       sdrRedoInfo * aRedoInfo,
                                                       sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_DELETE_ROW_PIECE( SChar       * aLogPtr,
                                                 UInt          /*aSize*/,
                                                 UChar       * aSlotPtr,
                                                 sdrRedoInfo * aRedoInfo,
                                                 sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_LOCK_ROW( SChar       * aLogPtr,
                                         UInt          aSize,
                                         UChar       * aSlotPtr,
                                         sdrRedoInfo * aRedoInfo,
                                         sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_UPDATE_LOBDESC( SChar       * LogPtr,
                                               UInt          /*aSize*/,
                                               UChar       * aSlotPtr,
                                               sdrRedoInfo * aRedoInfo,
                                               sdrMtx      * /*aMtx*/ );

    static IDE_RC redo_SDR_SDC_UPDATE_LOBDESC_KEY( SChar       * LogPtr,
                                                   UInt          /*aSize*/,
                                                   UChar       * aSlotPtr,
                                                   sdrRedoInfo * aRedoInfo,
                                                   sdrMtx      * /*aMtx*/ );

    static IDE_RC undo_SDR_SDC_INSERT_ROW_PIECE( idvSQL        * aStatistics,
                                                 smTID           aTransID,
                                                 smOID           aOID,
                                                 scGRID          aRecGRID,
                                                 SChar         * aLogPtr,
                                                 smLSN         * aPrevLSN );

    static IDE_RC undo_SDR_SDC_UPDATE_ROW_PIECE( idvSQL        * aStatistics,
                                                 smTID           aTransID,
                                                 smOID           aOID,
                                                 scGRID          aRecGRID,
                                                 SChar         * aLogPtr,
                                                 smLSN         * aPrevLSN );

    static IDE_RC undo_SDR_SDC_OVERWRITE_ROW_PIECE( idvSQL         * aStatistics,
                                                    smTID            aTransID,
                                                    smOID            aOID,
                                                    scGRID           aRecGRID,
                                                    SChar          * aLogPtr,
                                                    smLSN          * aPrevLSN );

    static IDE_RC undo_SDR_SDC_CHANGE_ROW_PIECE_LINK( idvSQL         * aStatistics,
                                                      smTID            aTransID,
                                                      smOID            aOID,
                                                      scGRID           aRecGRID,
                                                      SChar          * aLogPtr,
                                                      smLSN          * aPrevLSN );

    static IDE_RC undo_SDR_SDC_DELETE_FIRST_COLUMN_PIECE( idvSQL         * aStatistics,
                                                          smTID            aTransID,
                                                          smOID            aOID,
                                                          scGRID           aRecGRID,
                                                          SChar          * aLogPtr,
                                                          smLSN          * aPrevLSN );

    static IDE_RC undo_SDR_SDC_DELETE_ROW_PIECE( idvSQL         * aStatistics,
                                                 smTID            aTransID,
                                                 smOID            aOID,
                                                 scGRID           aRecGRID,
                                                 SChar          * aLogPtr,
                                                 smLSN          * aPrevLSN );

    static IDE_RC undo_SDR_SDC_LOCK_ROW( idvSQL         * aStatistics,
                                         smTID            aTransID,
                                         smOID            aOID,
                                         scGRID           aRecGRID,
                                         SChar          * aLogPtr,
                                         smLSN          * aPrevLSN );

    static IDE_RC undo_SDR_SDC_UPDATE_LOBDESC( idvSQL         * aStatistics,
                                               smTID            aTransID,
                                               smOID            aOID,
                                               scGRID           aRecGRID,
                                               SChar          * aLogPtr,
                                               smLSN          * aPrevLSN );
};

#endif /* _O_SDC_ROW_UPDATE_H_ */

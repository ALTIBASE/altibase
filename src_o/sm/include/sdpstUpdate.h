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
 * $Id: sdpstUpdate.h 27220 2008-07-23 14:56:22Z newdaily $
 ***********************************************************************/

#ifndef _O_SDPST_UPDATE_H_
#define _O_SDPST_UPDATE_H_ 1

#include <sdpstDef.h>

class sdpstUpdate
{

public:
    static IDE_RC undo_SDPST_UPDATE_BMP_4DPATH( idvSQL    * aStatistics,
                                                sdrMtx    * aMtx,
                                                scSpaceID   aSpaceID,
                                                scPageID    aBMP,
                                                SShort      aFmSlotNo,
                                                SShort      aTmSlotNo,
                                                sdpstMFNL   aPrvMFNL,
                                                sdpstMFNL   aPrvLstSlotMFNL );

    static IDE_RC undo_SDPST_UPDATE_MFNL_4DPATH( idvSQL    * aStatistics,
                                                 sdrMtx    * aMtx,
                                                 scSpaceID   aSpaceID,
                                                 scPageID    aLfBMP,
                                                 SShort      aFmPBSNo,
                                                 SShort      aToPBSNo,
                                                 sdpstMFNL   aPrvMFNL );

    static IDE_RC undo_SDPST_MERGE_SEG_4DPATH( idvSQL     * aStatistics,
                                               sdrMtx     * aMtx,
                                               scSpaceID    aSpaceID,
                                               scPageID     aToSegPID,
                                               scPageID     aLstRtBMP,
                                               scPageID     aLstItBMP,
                                               scPageID     aLstLfBMP,
                                               scPageID     aLstExtDir,
                                               ULong        aToExtDirCnt,
                                               scPageID     aFmSegPID );


    static IDE_RC undo_SDPST_UPDATE_WM_4DPATH( idvSQL    * aStatistics,
                                               sdrMtx    * aMtx,
                                               scSpaceID   aSpaceID,
                                               scPageID    aSegPID,
                                               sdRID       aWMExtRID,
                                               scPageID    aWMPID );

    static IDE_RC redo_SDPST_INIT_SEGHDR( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_INIT_LFBMP( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_INIT_BMP( SChar       * aData,
                                       UInt          aLength,
                                       UChar       * aPagePtr,
                                       sdrRedoInfo * /*aRedoInfo*/,
                                       sdrMtx*       aMtx );

    static IDE_RC redo_SDPST_INIT_EXTDIR( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_ADD_RANGESLOT( SChar       * aData,
                                            UInt          aLength,
                                            UChar       * aPagePtr,
                                            sdrRedoInfo * /*aRedoInfo*/,
                                            sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_ADD_SLOTS( SChar       * aData,
                                        UInt          aLength,
                                        UChar       * aPagePtr,
                                        sdrRedoInfo * /*aRedoInfo*/,
                                        sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_ADD_EXTDESC( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_ADD_EXT_TO_SEGHDR( SChar       * aData,
                                                UInt          aLength,
                                                UChar       * aPagePtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_UPDATE_WM( SChar       * aData,
                                        UInt          aLength,
                                        UChar       * aPagePtr,
                                        sdrRedoInfo * /*aRedoInfo*/,
                                        sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_UPDATE_MFNL( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_UPDATE_PBS( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_UPDATE_LFBMP_4DPATH( SChar       * aData,
                                                  UInt          aLength,
                                                  UChar       * aPagePtr,
                                                  sdrRedoInfo * /*aRedoInfo*/,
                                                  sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_UPDATE_BMP_4DPATH( SChar       * aData,
                                                UInt          aLength,
                                                UChar       * aPagePtr,
                                                sdrRedoInfo * /*aRedoInfo*/,
                                                sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_SEG_MERGE_4DPATH( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * aMtx );

    static IDE_RC redo_SDPST_SEG_SPLIT_4DPATH( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * /*aRedoInfo*/,
                                               sdrMtx      * aMtx );

    static IDE_RC undo_SDPST_ALLOC_PAGE( idvSQL    * aStatistics,
                                         sdrMtx    * aMtx,
                                         scSpaceID   aSpaceID,
                                         scPageID    aSegPID,
                                         scPageID    aPID );
};

#endif // _O_SDPST_UPDATE_H_

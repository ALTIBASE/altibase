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
 * $Id:
 *
 * Bitmap-based tablespace를 위한 redo 루틴들이다.
 **********************************************************************/

#ifndef _O_SDPTB_UPDATE_H_
#define _O_SDPTB_UPDATE_H_ 1

#include <sdp.h>
#include <sdptb.h>
#include <smr.h>

class sdptbUpdate
{
//For Operation
public:

    /* redo type:  SDR_SDPTB_INIT_LGHDR_PAGE */
    static IDE_RC redo_SDPTB_INIT_LGHDR_PAGE( SChar       * aData,
                                              UInt          aLength,
                                              UChar       * aPagePtr,
                                              sdrRedoInfo * /*aRedoInfo*/,
                                              sdrMtx      * aMtx );

    /* redo type:  SDR_SDPTB_ALLOC_IN_LG */
    static IDE_RC redo_SDPTB_ALLOC_IN_LG( SChar       * aData,
                                          UInt          aLength,
                                          UChar       * aPagePtr,
                                          sdrRedoInfo * /*aRedoInfo*/,
                                          sdrMtx      * aMtx );

    /* redo type:  SDR_SDPTB_FREE_IN_LG */
    static IDE_RC redo_SDPTB_FREE_IN_LG( SChar       * aData,
                                         UInt          aLength,
                                         UChar       * aPagePtr,
                                         sdrRedoInfo * /*aRedoInfo*/,
                                         sdrMtx      * aMtx );


    static IDE_RC undo_SDPTB_ALLOCATE_AN_EXTDIR_FROM_LIST(
                                         idvSQL          * aStatistics,
                                         sdrMtx          * aMtx,
                                         scSpaceID         aSpaceID,
                                         sdpFreeExtDirType aFreeListIdx,
                                         scPageID          aExtDirPID );
};
#endif

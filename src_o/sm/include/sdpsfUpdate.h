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
 * $Id:$
 ***********************************************************************/

#ifndef _O_SDPSF_UPDATE_H_
#define _O_SDPSF_UPDATE_H_ 1

#include <sdpDef.h>
#include <sdpsfDef.h>

class sdpsfUpdate
{
public:
    static IDE_RC undo_SDPSF_MERGE_SEG_4DPATH( idvSQL     * aStatistics,
                                               sdrMtx     * aMtx,
                                               scSpaceID    aSpaceID,
                                               scPageID     aToSegPID,
                                               sdRID        aAllocExtRID,
                                               scPageID     aFstPIDOfAllocExt,
                                               ULong        aFmtPageCntOfToSeg,
                                               scPageID     aHWMOfToSeg );

    static IDE_RC undo_SDPSF_ALLOC_PAGE( idvSQL    * aStatistics,
                                         sdrMtx    * aMtx,
                                         scSpaceID   aSpaceID,
                                         scPageID    aSegPID,
                                         scPageID    aPageID );

    static IDE_RC undo_UPDATE_HWM_4DPATH( idvSQL    * aStatistics,
                                          sdrMtx    * aMtx,
                                          scSpaceID   aSpaceID,
                                          scPageID    aSegPID,
                                          sdRID       aAllocExtRID,
                                          scPageID    aFstPIDOfAllocExt,
                                          scPageID    aHWM,
                                          ULong       aFmtPageCnt );

    static IDE_RC undo_ADD_PIDLIST_PVTFREEPIDLIST_4DPATH( idvSQL    * aStatistics,
                                                          sdrMtx    * aMtx,
                                                          scSpaceID   aSpaceID,
                                                          scPageID    aSegPID,
                                                          scPageID    aFstPID,
                                                          scPageID    aLstPID,
                                                          ULong       aPageCnt );

};

#endif // _O_SDPSF_UPDATE_H_

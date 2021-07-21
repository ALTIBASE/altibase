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
 * $Id: sdpSegment.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SDP_SEGMENT_H_
#define _O_SDP_SEGMENT_H_ 1

#include <sdpDef.h>

class sdpSegment
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC initCommonCache( sdpSegCCache * aCommonCache,
                                   sdpSegType     aSegType,
                                   UInt           aPageCntInExt,
                                   smOID          aTableOID,
                                   UInt           aIndexID );

    /* PROJ-1671 LOB Segment에 대한 Segment Desc을 생성하고, 초기화한다.*/
    static IDE_RC allocLOBSegDesc( smiColumn * aColumn,
                                   smOID       aTableOID );
    
    /* LOB Segment에 대한 Segment Desc을 해제한다. */
    static IDE_RC freeLOBSegDesc( smiColumn * aColumn );

    static IDE_RC createSegment( idvSQL        * aStatistics,
                                 void          * aTrans,
                                 scSpaceID       aTableSpaceID,
                                 sdrMtxLogMode   aLogMode,
                                 sdpSegHandle  * aSegHandle,
                                 scPageID      * aSegPID );

    static IDE_RC dropSegment( idvSQL        * aStatistics,
                               void          * aTrans,
                               scSpaceID       aSpaceID,
                               sdpSegHandle  * aSegHandle );

     /* Table 혹은 Index Segment에 Extent 확장  */
    static IDE_RC allocExts(  idvSQL           * aStatistics,
                              scSpaceID          aSpaceID,
                              void             * aTrans,
                              sdpSegmentDesc   * aSegDesc,
                              ULong              aExtendSize );

    /* Page List Entry에 Table Segment 할당 */
    static IDE_RC allocTableSeg4Entry( idvSQL            * aStatistics,
                                       void              * aTrans,
                                       scSpaceID           aTableSpaceID,
                                       smOID               aTableOID,
                                       sdpPageListEntry  * aPageEntry,
                                       sdrMtxLogMode       aLoggingMode );

    /* Static Index Header에 Index Segment 할당 */
     static IDE_RC allocIndexSeg4Entry( idvSQL            * aStatistics,
                                        void              * aTrans,
                                        scSpaceID           aTableSpaceID,
                                        smOID               aTableOID,
                                        smOID               aIndexOID,
                                        UInt                aIndexID,
                                        UInt                aType,
                                        UInt                aBuildFlag,
                                        sdrMtxLogMode       aLoggingMode,
                                        smiSegAttr        * aSegAttr,
                                        smiSegStorageAttr * aSegStoAttr );

    /* Page List Entry의 Lob Segment 할당 */
    static IDE_RC allocLobSeg4Entry( idvSQL         * aStatistics,
                                     void*            aTrans,
                                     smiColumn      * aLobColumn,
                                     smOID            aLobColOID,
                                     smOID            aTableOID,
                                     sdrMtxLogMode    aLoggingMode );

    static IDE_RC initLobSegDesc( smiColumn * aLobColumn );

    /* Page List Entry의 Table Segment 해제 */
    static IDE_RC freeTableSeg4Entry(  idvSQL           *aStatistics,
                                       scSpaceID         aSpaceID,
                                       void*             aTrans,
                                       smOID             aTableOID,
                                       sdpPageListEntry *aPageEntry,
                                       sdrMtxLogMode     aLoggingMode );

    /* Page List Entry의 Table Segment 해제 */
    static IDE_RC freeTableSeg4Entry( idvSQL           *aStatistics,
                                      scSpaceID         aSpaceID,
                                      smOID             aTableOID,
                                      sdpPageListEntry *aPageEntry,
                                      sdrMtx*           aMtx );

    /* Table Segment 리셋 ( for Temporary ) */
    static IDE_RC resetTableSeg4Entry( idvSQL           *aStatistics,
                                       scSpaceID         aSpaceID,
                                       sdpPageListEntry *aPageEntry );

    /* Static Index Header에서 index segment 해제 */
    static IDE_RC freeIndexSeg4Entry(  idvSQL           *aStatistics,
                                       scSpaceID         aSpaceID,
                                       void*             aTrans,
                                       smOID             aIndexOID,
                                       sdrMtxLogMode     aLoggingMode );

    /* Static Index Header에서 index segment 해제 */
    static IDE_RC freeIndexSeg4Entry( idvSQL           *aStatistics,
                                      scSpaceID         aSpaceID,
                                      smOID             aIndexOID,
                                      sdrMtx*           aMtx );

    /* LOB Segment free */
    static IDE_RC freeLobSeg(idvSQL           *aStatistics,
                             void*             aTrans,
                             smOID             aLobColOID,
                             smiColumn*        aLobCol,
                             sdrMtxLogMode     aLoggingMode );

    /* LOB Segment 해제 */
    static IDE_RC freeLobSeg(idvSQL           *aStatistics,
                             smOID             aLobColOID,
                             smiColumn*        aLobCol,
                             sdrMtx*           aMtx );

    static IDE_RC getSegInfo( idvSQL          *aStatistics,
                              scSpaceID        aSpaceID,
                              scPageID         aSegPID,
                              sdpSegInfo      *aSegInfo );

    static IDE_RC freeSeg4OperUndo( idvSQL      *aStatistics,
                                    scSpaceID    aSpaceID,
                                    scPageID     aSegPID,
                                    sdrOPType    aOPType,
                                    sdrMtx      *aMtx );

};

#endif /* _O_SDP_SEGMENT_H_ */

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
 *
 * $Id: sdpstSegDDL.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Create/Drop/Alter/Reset 연산관련
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_SEG_DDL_H_
# define _O_SDPST_SEG_DDL_H_ 1

# include <sdpDef.h>
# include <sdpstDef.h>

/*
 * 생성해야할 Bitmap 페이지 개수 산정
 *
 * 페이지 개수를 인자로 받아서 페이지들을 모두
 * 저장할 수 있는 상위개념의 페이지가 몇개가
 * 필요한지 계산한다. 
 *
 * ALIGN UP을 하기 위해 (aMaxSlotCnt) - 1 해줌.
 */
#define SDPST_EST_BMP_CNT_4NEWEXT( aPageCnt, aMaxSlotCnt ) \
    ( ( (aPageCnt) + (aMaxSlotCnt) - 1 ) / (aMaxSlotCnt) )


class sdpstSegDDL
{
public:
    static IDE_RC createSegment( idvSQL        * aStatistics,
                                 sdrMtx        * aMtx,
                                 scSpaceID       aSpaceID,
                                 sdpSegType      aSegType,
                                 sdpSegHandle  * aSegmentHandle );

    static IDE_RC dropSegment( idvSQL     * aStatistics,
                               sdrMtx     * aMtx,
                               scSpaceID    aSpaceID,
                               scPageID     aSegPID );


    static IDE_RC resetSegment( idvSQL        * aStatistics,
                                sdrMtx        * aMtx,
                                scSpaceID       aSpaceID,
                                sdpSegHandle  * aSegmentHandle,
                                sdpSegType      aSegType );


    static IDE_RC allocateExtents( idvSQL           * aStatistics,
                                   sdrMtxStartInfo  * aStartInfo,
                                   scSpaceID          aSpaceID,
                                   sdpSegHandle     * aSegHandle,
                                   UInt               aExtCount );

    static IDE_RC allocNewExtsAndPage( idvSQL           * aStatistics,
                                       sdrMtxStartInfo  * aStartInfo,
                                       scSpaceID          aSpaceID,
                                       sdpSegHandle     * aSegHandle,
                                       UInt               aExtCount,
                                       idBool             aNeedToUpdateHWM,
                                       sdRID            * aAllocFstExtRID,
                                       sdrMtx           * aMtx,
                                       UChar           ** aFstDataPage );
private:

    static IDE_RC resetSegment( idvSQL           * aStatistics,
                                sdrMtxStartInfo  * aStartInfo,
                                scSpaceID          aSpaceID,
                                sdpSegHandle     * aSegmentHandle );

    static IDE_RC freeAllExts( idvSQL       * aStatistics,
                               sdrMtx       * aMtx,
                               scSpaceID      aSpaceID,
                               sdpstSegHdr  * aSegHdr );

    static IDE_RC freeExtsExceptFst( idvSQL       * aStatistics,
                                     sdrMtx       * aMtx,
                                     scSpaceID      aSpaceID,
                                     sdpstSegHdr  * aSegHdr );

    static IDE_RC allocateSegment( idvSQL       * aStatistics,
                                   sdrMtx       * aMtx,
                                   scSpaceID      aSpaceID,
                                   sdpSegHandle * aSegHandle );

    static IDE_RC createDataPages( idvSQL               * aStatistics,
            sdrMtxStartInfo      * aStartInfo,
            scSpaceID              aSpaceID,
            sdpSegHandle         * aSegHandle,
            sdpstExtDesc         * aExtDesc,
            sdpstBfrAllocExtInfo * aBfrInfo,
            sdpstAftAllocExtInfo * aAftInfo );
    static IDE_RC createMetaPages( idvSQL               * aStatistics,
                                   sdrMtxStartInfo      * aStartInfo,
                                   scSpaceID              aSpaceID,
                                   sdpstSegCache        * aSegCache,
                                   sdpstExtDesc         * aExtDesc,
                                   sdpstBfrAllocExtInfo * aBfrInfo,
                                   sdpstAftAllocExtInfo * aAftInfo,
                                   scPageID             * aSegPID );

    static IDE_RC linkNewExtToLstBMPs( idvSQL                * aStatistics,
                                       sdrMtx                * aMtx,
                                       scSpaceID               aSpaceID,
                                       sdpSegHandle          * aSegHandle,
                                       sdpstExtDesc          * aExtDesc,
                                       sdpstBfrAllocExtInfo  * aBfrInfo,
                                       sdpstAftAllocExtInfo  * aAftInfo );

    static IDE_RC linkNewExtToExtDir( idvSQL                 * aStatistics,
                                       sdrMtx                * aMtx,
                                       scSpaceID               aSpaceID,
                                       scPageID                aSegPID,
                                       sdpstExtDesc          * aExtDesc,
                                       sdpstBfrAllocExtInfo  * aBfrInfo,
                                       sdpstAftAllocExtInfo  * aAftInfo,
                                       sdRID                 * aAllocExtRID );

    static IDE_RC linkNewExtToSegHdr( idvSQL                * aStatistics,
                                      sdrMtx                * aMtx,
                                      scSpaceID               aSpaceID,
                                      scPageID                aSegPID,
                                      sdpstExtDesc          * aExtDesc,
                                      sdpstBfrAllocExtInfo  * aBfrInfo,
                                      sdpstAftAllocExtInfo  * aAftInfo,
                                      sdRID                   aAllocExtRID,
                                      idBool                  aNeedToUpdateHWM,
                                      sdpstSegCache         * aSegCache );

    static IDE_RC checkAndUpdateHWM( idvSQL                 * aStatistics,
                                     scSpaceID       aSpaceID,
                                     scPageID        aSegPID,
                                     scPageID        aPageID,
                                     scPageID        aExtDirPID,
                                     SShort          aSlotNoInExtDir,
                                     sdpstSegCache * aSegCache );

    static IDE_RC tryToFormatPageAfterHWM( idvSQL           * aStatistics,
                                           sdrMtxStartInfo  * aStartInfo,
                                           scSpaceID          aSpaceID,
                                           sdpSegHandle     * aSegHandle,
                                           sdRID            * aFstExtRID,
                                           sdrMtx           * aMtx,
                                           UChar           ** aFstDatePage,
                                           idBool           * aFormatFlag );

};

#endif // _O_SDPST_SEG_DDL_H_

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
 * $Id: sdpscSegDDL.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 Create, Extend 연산관련
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_SEG_DDL_H_
# define _O_SDPSC_SEG_DDL_H_ 1

# include <sdpDef.h>
# include <sdpscDef.h>

# include <sdpscED.h>

class sdpscSegDDL
{
public:

    /* [ INTERFACE ] Segment 할당 */
    static IDE_RC createSegment( idvSQL                * aStatistics,
                                 sdrMtx                * aMtx,
                                 scSpaceID               aSpaceID,
                                 sdpSegType              aSegType,
                                 sdpSegHandle          * aSegmentHandle );

    /* Segment에 1개이상의 Extent를 할당한다. */
    static IDE_RC allocNewExts( idvSQL           * aStatistics,
                                sdrMtxStartInfo  * aStartInfo,
                                scSpaceID          aSpaceID,
                                sdpSegHandle     * aSegHandle,
                                scPageID           aCurExtDir,
                                sdpFreeExtDirType  aFreeListIdx,
                                sdRID            * aAllocExtRID,
                                scPageID         * aFstPIDOfExt,
                                scPageID         * aFstDataPIDOfExt );

    /* [INTERFACE] From 세그먼트에서 To 세그먼트로 Extent Dir를 옮긴다.
       Extent Dir이동시 from과 to의 extent 수가 틀리면 나머지는 freelist로 보낸다.  */
    static IDE_RC tryStealExts( idvSQL           * aStatistics,
                                sdrMtxStartInfo  * aStartInfo,
                                scSpaceID          aSpaceID,
                                sdpSegHandle     * aFrSegHandle,
                                scPageID           aFrSegPID,
                                scPageID           aFrCurExtDir,
                                sdpSegHandle     * aToSegHandle,
                                scPageID           aToSegPID,
                                scPageID           aToCurExtDir,
                                idBool           * aTrySuccess );

private:

    /* Segment를 할당한다 */
    static IDE_RC allocateSegment( idvSQL       * aStatistics,
                                   sdrMtx       * aMtx,
                                   scSpaceID      aSpaceID,
                                   sdpSegHandle * aSegHandle,
                                   sdpSegType     aSegType );

    /* 새로운 하나의 Extent를 Segment에 할당하는 연산을 완료 */
    static IDE_RC addAllocExtDesc( idvSQL             * aStatistics,
                                   sdrMtx             * aMtx,
                                   scSpaceID            aSpaceID,
                                   scPageID             aSegPID,
                                   sdpscExtDirInfo    * aCurExtDirInfo,
                                   sdRID              * aAllocExtRID,
                                   sdpscExtDesc       * aExtDesc,
                                   UInt               * aTotExtDescCnt );

    /* 새로운 하나의 Extent Dir.를 Segment에 할당하는 연산을 완료 */
    static IDE_RC addOrShrinkAllocExtDir( idvSQL                * aStatistics,
                                          sdrMtx                * aMtx,
                                          scSpaceID               aSpaceID,
                                          scPageID                aSegPID,
                                          sdpFreeExtDirType       aFreeListIdx,
                                          sdpscExtDirInfo       * aCurExtDirInfo,
                                          sdpscAllocExtDirInfo  * aAllocExtInfo );
};



#endif // _O_SDPSC_SEG_DDL_H_

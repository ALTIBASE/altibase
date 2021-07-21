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
 * $Id: sdpsfSegDDL.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Segment의 Create/Drop/Alter/Reset 연산의 STATIC
 * 인터페이스들을 관리한다.
 *
 ***********************************************************************/

#ifndef _O_SDPSF_SEG_DDL_H_
#define _O_SDPSF_SEG_DDL_H_ 1

#include <sdpDef.h>
#include <sdpsfDef.h>

class sdpsfSegDDL
{

public:
    /* [ INTERFACE ] Segment 할당 */
    static IDE_RC createSegment( idvSQL                * aStatistics,
                                 sdrMtx                * aMtx,
                                 scSpaceID               aSpaceID,
                                 sdpSegType              aSegType,
                                 sdpSegHandle          * aSegmentHandle );

    /* [ INTERFACE ] Segment 해제 */
    static IDE_RC dropSegment( idvSQL              * aStatistics,
                               sdrMtx              * aMtx,
                               scSpaceID             aSpaceID,
                               scPageID              aSegPID );

    /*  [ INTERFACE ] Segment 리셋  */
    static IDE_RC resetSegment( idvSQL           * aStatistics,
                                sdrMtx           * aMtx,
                                scSpaceID          aSpaceID,
                                sdpSegHandle     * aSegmentHandle,
                                sdpSegType         aSegType );

    static IDE_RC trim( idvSQL           *aStatistics,
                        sdrMtx           *aMtx,
                        scSpaceID         aSpaceID,
                        sdpsfSegHdr      *aSegHdr,
                        sdRID             aExtRID );

private:

    static IDE_RC allocateSegment( idvSQL          * aStatistics,
                                   sdrMtx          * aMtx,
                                   scSpaceID         aSpaceID,
                                   sdpSegType        aSegType,
                                   sdpSegHandle    * aSegHandle );
};


#endif // _O_SDPSF_SEG_DDL_H_

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
 
# ifndef _O_SDPST_ALLOC_EXT_INFO_H_
# define _O_SDPST_ALLOC_EXT_INFO_H_ 1

# include <sdpstDef.h>

class sdpstAllocExtInfo
{
public:
    static IDE_RC initialize( sdpstBfrAllocExtInfo *aBfrInfo );
    static IDE_RC initialize( sdpstAftAllocExtInfo *aAftInfo );
    static IDE_RC getAllocExtInfo( idvSQL                * aStatistics,
                                   scSpaceID               aSpaceID,
                                   scPageID                aSegPID,
                                   UInt                    aMaxExtCnt,
                                   sdpstExtDesc          * aExtDesc,
                                   sdpstBfrAllocExtInfo  * aBfrInfo,
                                   sdpstAftAllocExtInfo  * aAftInfo );

    static void dump( sdpstBfrAllocExtInfo      * aBfrInfo );
    static void dump( sdpstAftAllocExtInfo      * aAftInfo );
private:
    static sdpstPageRange selectPageRange( ULong aTotPages );

    static IDE_RC getBfrAllocExtInfo( idvSQL               * aStatistics,
                                      scSpaceID              aSpaceID,
                                      scPageID               aSegPID,
                                      UInt                   aMaxExtCnt,
                                      sdpstBfrAllocExtInfo * aBfrInfo );

    static void getAftAllocExtInfo( scPageID                 aSegPID,
                                    sdpstExtDesc           * aExtDesc, 
                                    sdpstBfrAllocExtInfo   * aBfrInfo,
                                    sdpstAftAllocExtInfo   * aAftInfo );
};

#endif // _O_SDPST_ALLOC_EXT_INFO_H_

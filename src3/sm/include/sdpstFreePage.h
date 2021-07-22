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
 * $Id: sdpstFreePage.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 가용공간 해제 연산 관련
 * 헤더파일이다. 
 *
 ***********************************************************************/

# ifndef _O_SDPST_FREE_PAGE_H_
# define _O_SDPST_FREE_PAGE_H_ 1

# include <sdpDef.h>
# include <sdpPhyPage.h>

class sdpstFreePage
{
public:
    static IDE_RC freePage( idvSQL           *aStatistics,
                            sdrMtx           *aMtx,
                            scSpaceID         aSpaceID,
                            sdpSegHandle     *aSegmentHandle,
                            UChar            *aPageHdr );

    static idBool isFreePage( UChar * aPagePtr );

    static IDE_RC checkAndUpdateHintItBMP( idvSQL         * aStatistics,
                                           scSpaceID        aSpaceID,
                                           scPageID         aSegPID,
                                           sdpstStack     * aRevStack,
                                           sdpstStack     * aItHintStack,
                                           idBool         * aIsTurnOn );

private:
    static IDE_RC makeOrderedStackFromRevStack( idvSQL      * aStatistics,
                                      scSpaceID     aSpaceID,
                                      scPageID      aSegPID,
                                      sdpstStack  * aRevStack,
                                      sdpstStack  * aOrderedStack );
};

#endif // _O_SDPST_FREE_PAGE_H_

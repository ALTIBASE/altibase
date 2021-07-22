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
 * $Id: sdpscAllocPage.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment에서 가용공간 할당 연산 관련
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_ALLOC_PAGE_H_
# define _O_SDPSC_ALLOC_PAGE_H_ 1

# include <sdpDef.h>
# include <sdpscDef.h>

class sdpscAllocPage
{

public:

    static IDE_RC createPage( idvSQL           * aStatistics,
                              scSpaceID          aSpaceID,
                              scPageID           aNewPageID,
                              sdpPageType        aPageType,
                              sdpParentInfo    * aParentInfo,
                              sdpscPageBitSet    aPageBitSet,
                              sdrMtx           * aMtx4Latch,
                              sdrMtx           * aMtx4Logging,
                              UChar           ** aNewPagePtr );

    static IDE_RC formatPageHdr( sdpPhyPageHdr    * aNewPagePtr,
                                 scPageID           aNewPageID,
                                 sdpPageType        aPageType,
                                 sdpParentInfo    * aParentInfo,
                                 sdpscPageBitSet    aPageBitSet,
                                 sdrMtx           * aMtx );


    static IDE_RC allocNewPage4Append( idvSQL              * aStatistics,
                                       sdrMtx              * aMtx,
                                       scSpaceID             aSpaceID,
                                       sdpSegHandle        * aSegHandle,
                                       sdRID                 aPrvAllocExtRID,
                                       scPageID              aFstPIDOfPrvAllocExt,
                                       scPageID              aPrvAllocPageID,
                                       idBool                aIsLogging,
                                       sdpPageType           aPageType,
                                       sdRID               * aAllocExtRID,
                                       scPageID            * aFstPIDOfAllocExt,
                                       scPageID            * aAllocPID,
                                       UChar              ** aAllocPagePtr );

    static IDE_RC prepareNewPage4Append(
                            idvSQL        * aStatistics,
                            sdrMtx        * aMtx,
                            scSpaceID       aSpaceID,
                            sdpSegHandle  * aSegHandle,
                            sdRID           aPrvAllocExtRID,
                            scPageID        aFstPIDOfPrvAllocExt,
                            scPageID        aPrvAllocPageID );
};


#endif // _O_SDPSC_ALLOC_PAGE_H_

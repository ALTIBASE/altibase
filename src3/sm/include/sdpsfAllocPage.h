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

# ifndef _O_SDPSF_ALLOCPAGE_H_
# define _O_SDPSF_ALLOCPAGE_H_ 1

# include <sdpsfDef.h>
# include <sdr.h>

class sdpsfAllocPage
{
public:
    static IDE_RC allocNewPage( idvSQL        *aStatistics,
                                sdrMtx        *aMtx,
                                scSpaceID      aSpaceID,
                                sdpSegHandle  *aSegHandle,
                                sdpsfSegHdr   *aSegHdr,
                                sdpPageType    aPageType,
                                UChar        **aPagePtr );

    static IDE_RC prepareNewPages( idvSQL            * aStatistics,
                                   sdrMtx            * aMtx,
                                   scSpaceID           aSpaceID,
                                   sdpSegHandle      * aSegHandle,
                                   UInt                aCountWanted );

    static IDE_RC allocPage( idvSQL        *aStatistics,
                             sdrMtx        *aMtx,
                             scSpaceID      aSpaceID,
                             sdpSegHandle * aSegHandle,
                             sdpPageType    aPageType,
                             UChar        **aPagePtr );

    static IDE_RC freePage( idvSQL            * aStatistics,
                            sdrMtx            * aMtx,
                            scSpaceID           aSpaceID,
                            sdpSegHandle      * aSegHandle,
                            UChar             * aPagePtr );

    static idBool isFreePage( UChar * aPagePtr );

    static IDE_RC addPageToFreeList( idvSQL          * aStatistics,
                                     sdrMtx          * aMtx,
                                     scSpaceID         aSpaceID,
                                     scPageID          aSegPID,
                                     UChar           * aPagePtr );

    static IDE_RC allocNewPage4Append( idvSQL               *aStatistics,
                                       sdrMtx               *aMtx,
                                       scSpaceID             aSpaceID,
                                       sdpSegHandle         *aSegHandle,
                                       sdRID                 aPrvAllocExtRID,
                                       scPageID              aFstPIDOfPrvAllocExt,
                                       scPageID              aPrvAllocPageID,
                                       idBool                aIsLogging,
                                       sdpPageType           aPageType,
                                       sdRID                *aAllocExtRID,
                                       scPageID             *aFstPIDOfAllocExt,
                                       scPageID             *aAllocPID,
                                       UChar               **aAllocPagePtr );
};

#endif /* _O_SDPSF_ALLOCPAGE_H_ */

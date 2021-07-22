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
 * $Id: sdpstItBMP.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Internal Bitmap 페이지의 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_IT_BMP_H_
# define _O_SDPST_IT_BMP_H_ 1

# include <sdrDef.h>
# include <sdpDef.h>
# include <sdpPhyPage.h>

# include <sdpstDef.h>
# include <sdpstFindPage.h>

class sdpstItBMP
{
public:
    inline static idBool isCandidateLfBMP( UChar      * aPagePtr,
                                           SShort       aSlotNo,
                                           sdpstMFNL    aTargetMFNL );

    inline static void setCandidateArray( UChar       * aPagePtr,
                                          UShort        aSlotNo,
                                          UShort        aArrIdx,
                                          void        * aArray );
    inline static UShort getStartSlotNo( UChar        * aPagePtr,
                                         UShort         aHintIdx );

    inline static SShort getSlotCnt( UChar     * aPagePtr,
                                     sdpstWM   * aHWM );

    static sdpstBMPOps  mItBMPOps;
};

inline idBool sdpstItBMP::isCandidateLfBMP( UChar      * aPagePtr,
                                            SShort       aSlotNo,
                                            sdpstMFNL    aTargetMFNL )
{
    sdpstBMPSlot    * sSlotPtr;

    IDE_ASSERT( aPagePtr != NULL );
    IDE_ASSERT( aSlotNo  != SDPST_INVALID_SLOTNO );

    sSlotPtr = sdpstBMP::getSlot( sdpstBMP::getHdrPtr( aPagePtr ), aSlotNo );
    return ( sSlotPtr->mMFNL >= aTargetMFNL ? ID_TRUE : ID_FALSE );
}

inline void sdpstItBMP::setCandidateArray( UChar       * aPagePtr,
                                           UShort        aSlotNo,
                                           UShort        aArrIdx,
                                           void        * aArray )
{
    sdpstPosItem    * sArray;
    sdpstBMPSlot    * sSlotPtr;

    IDE_DASSERT( aArray   != NULL );
    IDE_DASSERT( aPagePtr != NULL );

    sArray   = (sdpstPosItem *)aArray;
    sSlotPtr = sdpstBMP::getSlot( sdpstBMP::getHdrPtr( aPagePtr ), aSlotNo );

    sArray[aArrIdx].mNodePID = sSlotPtr->mBMP;
    sArray[aArrIdx].mIndex   = aSlotNo;
}

inline UShort sdpstItBMP::getStartSlotNo( UChar       * aPagePtr,
                                          UShort        aHintIdx )
{
    sdpstBMPHdr * sBMPHdr;
    UShort        sSlotNo;
    SShort        sFreeSlotCnt;

    IDE_ASSERT( aPagePtr != NULL );

    sBMPHdr = sdpstBMP::getHdrPtr( aPagePtr );

    sFreeSlotCnt = sBMPHdr->mSlotCnt - sBMPHdr->mFstFreeSlotNo;
    /* freeslot이 없을때도 mFstFreeSlotNo가 마지막 Slot을 가리키기 때문에
     *      * 총 개수에서 빼도 0이 나올수가 없다. */
    IDE_ASSERT( sFreeSlotCnt > 0 );

    sSlotNo = sdpstBMP::doHash( aHintIdx, sFreeSlotCnt );
    if ( sSlotNo < sBMPHdr->mFstFreeSlotNo )
    {
        sSlotNo = sBMPHdr->mFstFreeSlotNo;
    }                                                                           
    return sSlotNo;

}

inline SShort sdpstItBMP::getSlotCnt( UChar     * aPagePtr,
                                      sdpstWM   * aHWM )
{
    sdpstBMPHdr   * sBMPHdr;
    sdpstPosItem    sHWMPos;
    SShort          sSlotCnt;

    IDE_ASSERT( aPagePtr != NULL );
    IDE_ASSERT( aHWM     != NULL );

    sBMPHdr = sdpstBMP::getHdrPtr( aPagePtr );
    sHWMPos = sdpstStackMgr::getSeekPos( &aHWM->mStack, SDPST_ITBMP );

    if ( sHWMPos.mNodePID ==
            sdpPhyPage::getPageID( sdpPhyPage::getPageStartPtr(aPagePtr) ) )
    {
        if ( sHWMPos.mIndex + 1 < sBMPHdr->mSlotCnt )
        {
            sSlotCnt = sHWMPos.mIndex + 1;
        }
        else
        {
            sSlotCnt = sBMPHdr->mSlotCnt;
        }
    }
    else
    {
        sSlotCnt = sBMPHdr->mSlotCnt;
    }

    return sSlotCnt;
}

#endif // _O_SDPST_IT_BMP_H_

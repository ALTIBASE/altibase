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
 * $Id: sdpstBMP.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 BMP의 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_BMP_COMMON_H_
# define _O_SDPST_BMP_COMMON_H_ 1

# include <sdrDef.h>
# include <sdpDef.h>
# include <sdpPhyPage.h>

# include <sdpstDef.h>
# include <sdpstSH.h>
# include <sdpstFindPage.h>
# include <sdpstStackMgr.h>

#define SDPST_BMP_CHILD_TYPE( aBMPType ) \
    ( (sdpstBMPType)((UInt)(aBMPType) + 1) )
#define SDPST_BMP_PARENT_TYPE( aBMPType ) \
    ( (sdpstBMPType)((UInt)(aBMPType) - 1) )

class sdpstBMP
{
public:
    static IDE_RC createAndInitPages( idvSQL                * aStatistics,
                                      sdrMtxStartInfo       * aStartInfo,
                                      scSpaceID               aSpaceID,
                                      sdpstExtDesc          * aExtDesc,
                                      sdpstBMPType            aBMPType,
                                      sdpstBfrAllocExtInfo  * aBfrInfo,
                                      sdpstAftAllocExtInfo  * aAftInfo);

    static IDE_RC logAndInitBMPHdr( sdrMtx            * aMtx,
                                    sdpstBMPHdr       * aBMPHdr,
                                    sdpstBMPType        aBMPType,
                                    scPageID            aParentBMP,
                                    UShort              aSlotNoInParent,
                                    scPageID            aFstBMP,
                                    scPageID            aLstBMP,
                                    UShort              aFullBMPs,
                                    UShort              aMaxSlotCnt,
                                    idBool            * aNeedToChangeMFNL );

    static void  initBMPHdr( sdpstBMPHdr       * aBMPHdr,
                             sdpstBMPType        aBMPType,
                             scOffset            aBodyOffset,
                             scPageID            aParentBMP,
                             UShort              aSlotNoInParent,
                             scPageID            aFstBMP,
                             scPageID            aLstBMP,
                             UShort              aFullBMPs,
                             UShort              aMaxSlotCnt,
                             idBool            * aNeedToChangeMFNL );


    static void  initBMPHdr( sdpstBMPHdr       * aBMPHdr,
                             sdpstBMPType        aBMPType,
                             scPageID            aParentBMP,
                             UShort              aSlotNoInParent,
                             scPageID            aFstBMP,
                             scPageID            aLstBMP,
                             UShort              aFullBMPs,
                             UShort              aMaxSlotCnt,
                             idBool            * aNeedToChangeMFNL );

    static inline SShort getFstFreeSlotNo( UChar * sPagePtr );
    static inline UShort getFreeSlotCnt( sdpstBMPHdr * aBMPHdr );

    static inline sdpstBMPHdr * getHdrPtr( UChar   * aPagePtr );

    static IDE_RC logAndAddSlots( sdrMtx        * aMtx,
                                  sdpstBMPHdr   * aBMPHdr,
                                  scPageID        aFstBMP,
                                  scPageID        aLstBMP,
                                  UShort          aFullBMPs,
                                  idBool        * aNeedToChangeMFNL,
                                  sdpstMFNL     * aNewMFNL );

    static IDE_RC logAndUpdateMFNL( sdrMtx            * aMtx,
                                    sdpstBMPHdr       * aBMPHdr,
                                    SShort              aFmSlotNo,
                                    SShort              aToSlotNo,
                                    sdpstMFNL           aOldMFNL,
                                    sdpstMFNL           aNewMFNL,
                                    UShort              aPageCount,
                                    sdpstMFNL         * aNewBMPMFNL,
                                    idBool            * aNeedToChangeMFNL );

    static void updateMFNL( sdpstBMPHdr       * aBMPHdr,
                            SShort              aFmSlotNo,
                            SShort              aToSlotNo,
                            sdpstMFNL           aOldMFNL,
                            sdpstMFNL           aNewMFNL,
                            UShort              aPageCount,
                            sdpstMFNL         * aNewBMPMFNL,
                            idBool            * aNeedToChangeMFNL );

    static IDE_RC tryToChangeMFNL( idvSQL          * aStatistics,
                                   sdrMtx          * aMtx,
                                   scSpaceID         aSpaceID,
                                   scPageID          aCurBMP,
                                   SShort            aSlotNo,
                                   sdpstMFNL         aNewSlotMFNL,
                                   idBool          * aNeedToChangeMFNL,
                                   sdpstMFNL       * aNewMFNL,
                                   scPageID        * aParentBMP,
                                   SShort          * aSlotNoInParent );

    static inline SShort findSlotNoByBMP( sdpstBMPHdr  * aBMPHdr,
                                          scPageID       aBMP );

    static inline idBool isLstSlotNo ( UChar * aPagePtr, SShort aIndex );

    static void addSlots( sdpstBMPHdr       * aBMPHdr,
                          SShort              aLstSlotNo,
                          scPageID            aFstBMP,
                          scPageID            aLstBMP,
                          UShort              aFullBMPs,
                          idBool            * aNeedToChangeMFNL,
                          sdpstMFNL         * aNewMFNL );

    static inline sdpstBMPSlot * getMapPtr( sdpstBMPHdr * aBMPHdr );

    static inline SShort getNxtFreeSlot( sdpstBMPHdr  * aBMPHdr,
                                         SShort         aNxtSlotNo );

    static IDE_RC verifyBMP( sdpstBMPHdr * aBMPHdr );

    static inline UShort doHash( UShort aHintIdx, SShort aSlotCnt );

    static inline sdpstBMPSlot * getSlot( sdpstBMPHdr   * aBMPHdr,
                                          UShort          aSlotNo );

    static void addSlotsToMap( sdpstBMPSlot  * aMapPtr,
                               SShort          aSlotNo,
                               scPageID        aFstBMP,
                               scPageID        aLstBMP,
                               UShort          aFullBMPs );

    static void makeCandidateChild( sdpstSegCache    * aSegCache,
                                    UChar            * aPagePtr,
                                    sdpstBMPType       aBMPType,
                                    sdpstSearchType    aSearchType,
                                    sdpstWM          * aHWM,
                                    void             * aCandidateArray,
                                    UInt             * aCandidateCount );

    static inline UInt getMaxSlotCnt4Property();

    static IDE_RC dumpHdr( UChar * aPagePtr,
                           SChar * aOutBuf,
                           UInt    aOutSize );

    static IDE_RC dumpBody( UChar * aPagePtr,
                            SChar * aOutBuf,
                            UInt    aOutSize );

    static IDE_RC dump( UChar * aPagePtr );

public:
    static sdpstBMPOps  * mBMPOps[SDPST_BMP_TYPE_MAX];
};

/* aNxtSlotNo 이후에 가용공간이 있는 slotNo를 찾아 반환한다. */
inline SShort sdpstBMP::getNxtFreeSlot( sdpstBMPHdr     * aBMPHdr,
                                        SShort            aNxtSlotNo )
{
    SShort              sCurIdx;
    sdpstBMPSlot      * sSlot;
    sdpstMFNL           sTargetMFNL;
    SShort              sLoop;
    SShort              sNxtIdx;

    sNxtIdx     = SDPST_INVALID_SLOTNO;
    sTargetMFNL = SDPST_SEARCH_TARGET_MIN_MFNL( SDPST_SEARCH_NEWSLOT );

    sSlot = getMapPtr(aBMPHdr);

    for ( sLoop = 0, sCurIdx = aNxtSlotNo; sCurIdx < aBMPHdr->mSlotCnt; sLoop++ )
    {
        IDE_ASSERT( sCurIdx != SDPST_INVALID_SLOTNO );

        if ( sdpstFindPage::isAvailable( sSlot[sCurIdx].mMFNL, 
                                         sTargetMFNL ) == ID_TRUE )
        {
            sNxtIdx = sCurIdx;
            break;
        }

        sCurIdx++;
    }

    if ( sNxtIdx == SDPST_INVALID_SLOTNO )
    {
        sNxtIdx = sCurIdx--;
    }

    return sNxtIdx;
}

/* it-bmp 페이지에서의 탐색 시작 위치를 Hash 하여 얻는다 */
inline UShort sdpstBMP::doHash( UShort  aHintIdx, SShort  aSlotCnt )
{
    UShort  sSlotNo;

    IDE_ASSERT( aSlotCnt > 0 );

    sSlotNo = aHintIdx % aSlotCnt; // hash 사용

    return sSlotNo;
}

/* BMP의 map ptr을 반환한다. */
inline sdpstBMPSlot * sdpstBMP::getMapPtr( sdpstBMPHdr * aBMPHdr )
{
    IDE_ASSERT( aBMPHdr->mBodyOffset != 0 );
    return (sdpstBMPSlot*)
           (sdpPhyPage::getPageStartPtr((UChar*)aBMPHdr) +
            aBMPHdr->mBodyOffset);
}

/*
 * BMP에서 Internal Control Header의 Ptr 반환
 */
inline sdpstBMPHdr * sdpstBMP::getHdrPtr( UChar   * aPagePtr )
{
    sdpstSegHdr     * sSegHdr;
    sdpstBMPHdr     * sBMPHdr;

    if ( sdpPhyPage::getPhyPageType( aPagePtr ) == SDP_PAGE_TMS_SEGHDR )
    {
        sSegHdr = (sdpstSegHdr*)
                  sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
        sBMPHdr = &sSegHdr->mRtBMPHdr;
    }
    else
    {
        sBMPHdr = (sdpstBMPHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
    }
    return sBMPHdr;
}

/* sdpstBMPOps에서 사용한다. */
inline SShort sdpstBMP::getFstFreeSlotNo( UChar * aPagePtr )
{
    IDE_ASSERT( aPagePtr != NULL);
    return getHdrPtr(aPagePtr)->mFstFreeSlotNo;
}

/*
 * BMP에서의 가용한 Slot 개수 반환
 */
inline UShort sdpstBMP::getFreeSlotCnt( sdpstBMPHdr * aBMPHdr )
{
    IDE_ASSERT( aBMPHdr != NULL);
    return (UShort)( aBMPHdr->mMaxSlotCnt - aBMPHdr->mSlotCnt );
}

/* leaf slot을 반환한다 */
inline sdpstBMPSlot * sdpstBMP::getSlot( sdpstBMPHdr * aBMPHdr,
                                         UShort        aSlotNo )
{
    
    IDE_ASSERT( aBMPHdr != NULL );
    return &(getMapPtr( aBMPHdr )[ aSlotNo ] );
}

/* lf-bmp 페이지의 PID로 leaf slot 순번 탐색 */
inline SShort sdpstBMP::findSlotNoByBMP( sdpstBMPHdr  * aBMPHdr,
                                         scPageID       aBMP )
{
    UShort             sLoop;
    SShort             sSlotNo;
    sdpstBMPSlot     * sMapPtr;

    IDE_ASSERT( aBMPHdr != NULL );
    IDE_ASSERT( aBMP   != SD_NULL_PID );

    sSlotNo = SDPST_INVALID_SLOTNO;
    sMapPtr = getMapPtr( aBMPHdr );

    for( sLoop = 0; sLoop < aBMPHdr->mSlotCnt; sLoop++ )
    {
        if ( sMapPtr[sLoop].mBMP == aBMP )
        {
            sSlotNo = (SShort)sLoop;
            break; // found it !!
        }
    }
    return sSlotNo;
}

/* 마지막 slotNo 여부를 확인한다. */
inline idBool sdpstBMP::isLstSlotNo ( UChar * aPagePtr, SShort aIndex )
{
    return (aIndex == ( getHdrPtr( aPagePtr )->mSlotCnt - 1 ) ?
            ID_TRUE : ID_FALSE);
}

inline UInt sdpstBMP::getMaxSlotCnt4Property()
{
    return (UInt)
           ( (sdpPhyPage::getEmptyPageFreeSize() - ID_SIZEOF(sdpstBMPHdr)) /
             ID_SIZEOF(sdpstBMPSlot) );
}

#endif // _O_SDPST_BMP_COMMON_H_

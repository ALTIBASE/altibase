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
 * $Id: sdpstSH.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 Segment Header의 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_SH_H_
# define _O_SDPST_SH_H_ 1

# include <sdpDef.h>
# include <sdpstDef.h>

# include <sdpPhyPage.h>
# include <sdpstExtDir.h>
# include <sdpstBMP.h>

class sdpstSH
{
public:
    static IDE_RC getSegState( idvSQL        *aStatistics,
                               scSpaceID      aSpaceID,
                               scPageID       aSegPID,
                               sdpSegState   *aSegState );

    static IDE_RC createAndInitPage( idvSQL                * aStatistics,
                                     sdrMtxStartInfo       * aStartInfo,
                                     scSpaceID               aSpaceID,
                                     sdpstExtDesc          * aExtDesc,
                                     sdpstBfrAllocExtInfo  * aBfrInfo,
                                     sdpstAftAllocExtInfo  * aAftInfo,
                                     sdpstSegCache         * aSegCache,
                                     scPageID              * aSegPID );

    static IDE_RC getLstBMPInfo( idvSQL                * aStatistics,
                                 sdrMtxStartInfo       * aStartInfo,
                                 scSpaceID               aSpaceID,
                                 scPageID                aSegPID,
                                 UInt                    aMaxExtCnt,
                                 sdpstExtDesc          * aExtDesc,
                                 sdpstBfrAllocExtInfo  * aEstBMPs );


    static IDE_RC addNewExtDirPage( sdrMtx    * aMtx,
                                    UChar     * sSegHdrPagePtr,
                                    UChar     * sNewExtDIrPagePtr );

    static IDE_RC logAndLinkBMPs( sdrMtx        * aMtx,
                                  sdpstSegHdr   * sSegHdr,
                                  ULong           aAllocPageCnt,
                                  ULong           aMetaPageCnt,
                                  UShort          aNewRtBMPCnt,
                                  scPageID        aLstRtBMP,
                                  scPageID        aLstItBMP,
                                  scPageID        aLstLfBMP,
                                  scPageID      * aBfrLstRtBMP );

    static IDE_RC unlinkFstExtDirFromSegHdr( sdrMtx  * aMtx,
                                          UChar   * aSegHdrPagePtr,
                                          scPageID  aNextExtMapPID );

    static IDE_RC getSegInfo( idvSQL        * aStatistics,
                              scSpaceID       aSpaceID,
                              scPageID        aSegPID,
                              void          * aTableHeader,
                              sdpSegInfo    * aSegInfo );

    static IDE_RC updateWM4NewPages( idvSQL               * aStatistics,
                                     sdrMtx               * aMtx,
                                     scSpaceID              aSpaceID,
                                     scPageID               aSegPID,
                                     sdpstSegCache        * aSegCache,
                                     scPageID               aWMPID,
                                     UShort                 aNewPageCnt,
                                     UShort                 aMetaPageCnt,
                                     sdpstRangeSlot       * aRangeSlot,
                                     sdpstStack           * aNewEndPos,
                                     idBool               * aIsUpdateWM );

    static IDE_RC logAndUpdateWM( sdrMtx         * aMtx,
                                  sdpstWM        * aWM,
                                  scPageID         aWMPID,
                                  scPageID         aExtMapPID,
                                  SShort           aSlotNoInExtDir,
                                  sdpstStack     * aStack );

    static inline sdpstBMPHdr * getRtBMPHdr( UChar * aPagePtr );
    static inline sdpstExtDirHdr * getExtHdr( UChar * aPagePtr );
    static inline sdpstSegHdr* getHdrPtr( UChar * aPagePtr );

    static void initSegHdr ( sdpstSegHdr   * aSegHdreader,
                             scPageID        aSegHdrPID,
                             sdpSegType      aSegType,
                             UInt            aPageCntInExt,
                             UShort          aMaxSlotCntInExtDir,
                             UShort          aMaxSlotCntInRtBMP );

    static void linkBMPsToSegHdr( sdpstSegHdr  * aSegHdr,
                                  ULong          aAllocPageCnt,
                                  ULong          aMetaPageCnt,
                                  scPageID       aNewLstLfBMP,
                                  scPageID       aNewLstItBMP,
                                  scPageID       aNewLstRtBMP,
                                  UShort         aNewRtBMPCnt );

    static void updateWM( sdpstWM       * aWM,
                          scPageID        aWMPID,
                          scPageID        aExtMapPID,
                          SShort          aExtDescIdxInExtDir,
                          sdpstStack    * aPosStack );

    static IDE_RC setMetaPID( idvSQL     * aStatistics,
                              sdrMtx     * aMtx,
                              scSpaceID    aSpaceID,
                              scPageID     aSegPID,
                              UInt         aIndex,
                              scPageID     aMetaPID );

    static IDE_RC getMetaPID( idvSQL     * aStatistics,
                              scSpaceID    aSpaceID,
                              scPageID     aSegPID,
                              UInt         aIndex,
                              scPageID   * aMetaPID );

    static IDE_RC isExistPageAfterHWM( idvSQL           * aStatistics,
                                       scSpaceID          aSpaceID,
                                       scPageID           aSegPID,
                                       sdpstSegCache    * aSegCache,
                                       idBool           * aResult );

    static inline sdpstExtDirHdr * getExtDirHdr( UChar * aPagePtr );

    static inline UInt getTotExtDirCnt( sdpstSegHdr * aSegHdr );

    static inline scPageID getLstExtDir( sdpstSegHdr    * aSegHdr );

    static inline scPageID getFstExtDir( sdpstSegHdr    * aSegHdr );

    static inline IDE_RC setTotExtCnt( sdrMtx       * aMtx,
                                       sdpstSegHdr  * aSegHdr,
                                       ULong          aTotExtCnt );

    static inline IDE_RC setLstExtRID( sdrMtx       * aMtx,
                                       sdpstSegHdr  * aSegHdr,
                                       sdRID          aFreeIndexPageCnt );

    static inline IDE_RC setFreeIndexPageCnt( sdrMtx       * aMtx,
                                              sdpstSegHdr  * aSegHdr,
                                              ULong          aTotExtCnt );

    static IDE_RC dumpHdr( UChar * aPagePtr,
                           SChar * aOutBuf,
                           UInt    aOutSize );

    static IDE_RC dumpBody( UChar * aPagePtr,
                            SChar * aOutBuf,
                            UInt    aOutSize );

    static IDE_RC dump( UChar * aPagePtr );

private:
    static void initWM( sdpstWM   * aWM );
};

/*
 * 페이지의 임의의 포인터에서 Logical Header Ptr를 반환한다.
 */
inline sdpstSegHdr* sdpstSH::getHdrPtr( UChar * aPagePtr )
{
    IDE_ASSERT( aPagePtr != NULL );
    return (sdpstSegHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
}

/* Root BMP Header 반환 */
inline sdpstBMPHdr * sdpstSH::getRtBMPHdr( UChar * aPagePtr )
{
    return (sdpstBMPHdr*)(&(getHdrPtr( aPagePtr )->mRtBMPHdr));
}

/* ExtDir Header 반환 */
inline sdpstExtDirHdr * sdpstSH::getExtDirHdr( UChar * aPagePtr )
{
    return (sdpstExtDirHdr*)(&(getHdrPtr(aPagePtr)->mExtDirHdr));
}

/* ExtDir 개수 반환 */
inline UInt sdpstSH::getTotExtDirCnt( sdpstSegHdr * aSegHdr )
{
    IDE_ASSERT( aSegHdr != NULL );
    /* +1은 SegHdr에 포함된 ExtDir 개수 */
    return sdpDblPIDList::getNodeCnt(&aSegHdr->mExtDirBase) + 1;
}

/* Segment Header 페이지에서 마지막 Lst ExtDir PID를 반환한다. */
inline scPageID sdpstSH::getLstExtDir( sdpstSegHdr  * aSegHdr )
{
    scPageID        sExtDirPID;

    IDE_DASSERT( aSegHdr != NULL );

    if ( sdpDblPIDList::getNodeCnt( &aSegHdr->mExtDirBase ) > 0 )
    {
        sExtDirPID = sdpDblPIDList::getListTailNode( &aSegHdr->mExtDirBase );
    }
    else
    {
        sExtDirPID = sdpPhyPage::getPageID(
                            sdpPhyPage::getPageStartPtr( aSegHdr ) );
    }
    return sExtDirPID;
}

/* Segment Header 페이지에서 첫번째 ExtDir PID를 반환한다. */
inline scPageID sdpstSH::getFstExtDir( sdpstSegHdr  * aSegHdr )
{
    scPageID          sExtDirPID;

    IDE_DASSERT( aSegHdr != NULL );

    if ( sdpDblPIDList::getListHeadNode(&aSegHdr->mExtDirBase) == SD_NULL_PID )
    {
        sExtDirPID = sdpPhyPage::getPageID(
                            sdpPhyPage::getPageStartPtr( aSegHdr ) );
    }
    else
    {
        sExtDirPID = sdpDblPIDList::getListHeadNode(&aSegHdr->mExtDirBase);
    }

    return sExtDirPID;
}

inline IDE_RC sdpstSH::setTotExtCnt( sdrMtx       * aMtx,
                                     sdpstSegHdr  * aSegHdr,
                                     ULong          aTotExtCnt )
{
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aSegHdr->mTotExtCnt,
                                         &aTotExtCnt,
                                         ID_SIZEOF( ULong )) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

inline IDE_RC sdpstSH::setLstExtRID( sdrMtx       * aMtx,
                                     sdpstSegHdr  * aSegHdr,
                                     sdRID          aExtRID )
{
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aSegHdr->mLstExtRID,
                                         &aExtRID,
                                         ID_SIZEOF(sdRID) ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

inline IDE_RC sdpstSH::setFreeIndexPageCnt( sdrMtx       * aMtx,
                                            sdpstSegHdr  * aSegHdr,
                                            ULong          aFreeIndexPageCnt )
{
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aSegHdr->mFreeIndexPageCnt,
                                         &aFreeIndexPageCnt,
                                         ID_SIZEOF(ULong) ) != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

#endif // _O_SDPST_SH_H_

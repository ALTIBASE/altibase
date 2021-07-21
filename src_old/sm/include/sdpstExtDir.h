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
 * $Id: sdpstExtDir.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment의 ExtDir Page 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_ExtDir_H_
# define _O_SDPST_ExtDir_H_ 1

# include <sdpstDef.h>
# include <sdpstSH.h>
# include <sdpPhyPage.h>

class sdpstExtDir
{
public:
    static void initExtDesc( sdpstExtDesc *aExtDesc );
    static inline sdpstExtDirHdr * getHdrPtr( UChar   * aPagePtr );

    static inline UShort  getFreeSlotCnt( sdpstExtDirHdr * aPagePtr );

    static IDE_RC createAndInitPage( idvSQL               * aStatistics,
                                     sdrMtx               * aMtx,
                                     scSpaceID              aSpaceID,
                                     sdpstExtDesc         * aExtDesc,
                                     sdpstBfrAllocExtInfo * aBfrInfo,
                                     sdpstAftAllocExtInfo * aAftInfo,
                                     UChar               ** aPagePtr );

    static IDE_RC logAndInitExtDirHdr( sdrMtx           * aMtx,
                                       sdpstExtDirHdr   * aExtDirHdr,
                                       UShort             aMaxExtCnt );

    static void  initExtDirHdr( sdpstExtDirHdr  * aExtDirHdr,
                                UShort            aMaxExtCnt,
                                scOffset          aBodyOffset );

    static IDE_RC logAndAddExtDesc( sdrMtx         * aMtx,
                                    sdpstExtDirHdr * aExtDirHdr,
                                    sdpstExtDesc   * aExtDesc,
                                    sdRID          * aAllocExtRID );

    static inline sdpstExtDesc * getMapPtr( sdpstExtDirHdr * aExtDirHdr );

    static IDE_RC logAndClearExtDesc( sdrMtx          * aMtx,
                                      sdpstExtDirHdr  * aExtDirHdr,
                                      SShort            aExtDescIdx,
                                      scPageID          aExtFstPID,
                                      UInt              aLength );

    static IDE_RC getExtInfo( idvSQL      * aStatistics,
                              scSpaceID     aSpaceID,
                              sdRID         aExtRID,
                              sdpExtInfo  * aExtInfo );

    static IDE_RC getNxtAllocPage( idvSQL           * aStatistics,
                                   scSpaceID          aSpaceID,
                                   sdpSegInfo       * aSegInfo,
                                   sdpSegCacheInfo  * aSegCacheInfo,
                                   sdRID            * aExtRID,
                                   sdpExtInfo       * aExtInfo,
                                   scPageID         * aPageID );


    static IDE_RC getNxtPage( idvSQL           * aStatistics,
                              scSpaceID          aSpaceID,
                              sdpSegInfo       * aSegInfo,
                              sdpSegCacheInfo  * aSegCacheInfo,
                              sdRID            * aExtRID,
                              sdpExtInfo       * aExtInfo,
                              scPageID         * aPageID );

    static inline SShort calcOffset2SlotNo( sdpstExtDirHdr * aExtDirHdr,
                                            scOffset         aOffset );

    static inline scOffset calcSlotNo2Offset( sdpstExtDirHdr * aExtDirHdr,
                                              SShort           aExtDescIdx );

    static inline sdpstExtDesc * getFstExtDesc( sdpstExtDirHdr * aExtDirHdr );

    static sdRID getFstExtRID( sdpstExtDirHdr  * aExtDirHdr );

    static IDE_RC allocNewPageInExt( idvSQL         * aStatistics,
                                     sdrMtx         * aMtx,
                                     scSpaceID        aSpaceID,
                                     sdpSegHandle   * aSegHandle,
                                     sdRID            aPrvAllocExtRID,
                                     scPageID         aPrvAllocPageID,
                                     sdRID          * aAllocExtRID,
                                     scPageID       * aAllocPID,
                                     sdpParentInfo  * aParentInfo );

    static IDE_RC freeLstExt( idvSQL             * aStatistics,
                              sdrMtx             * aMtx,
                              scSpaceID            aSpaceID,
                              sdpstSegHdr        * aSegHdr,
                              sdpstExtDirHdr     * aExtDirHdr );

    static IDE_RC freeAllExtExceptFst( idvSQL             * aStatistics,
                                       sdrMtxStartInfo    * aStartInfo,
                                       scSpaceID            aSpaceID,
                                       sdpstSegHdr        * aSegHdr,
                                       sdpstExtDirHdr     * aExtDirHdr );

    static inline scPageID getNxtExtDir ( sdpstExtDirHdr   * aRtBMPHdr,
                                          scPageID           sSegPID );

    static void addExtDesc( sdpstExtDirHdr * aExtDirHdr,
                            SShort           aLstSlotNo,
                            sdpstExtDesc   * aExtDesc );

    static inline void clearExtDesc( sdpstExtDirHdr * aExtDirHdr,
                                     sdpstExtDesc   * aExtDesc );

    static IDE_RC getNxtExtRID( idvSQL       * aStatistics,
                                scSpaceID      aSpaceID,
                                scPageID       aSegHdrPID,
                                sdRID          aCurrExtRID,
                                sdRID        * aNxtExtRID );

    static inline sdpstExtDesc * getLstExtDesc( sdpstExtDirHdr * aExtDirHdr );

    static inline IDE_RC setExtDescCnt( sdrMtx          * aMtx,
                                        sdpstExtDirHdr  * aExtDirPageHdr,
                                        UShort            aExtDescCnt );

    static inline UInt getMaxSlotCnt4Property();

    static IDE_RC makeSeqNo( idvSQL           * aStatistics,
                             scSpaceID          aSpaceID,
                             scPageID           aSegPID,
                             sdRID              aExtRID,
                             scPageID           aPageID,
                             ULong            * aSeqNo );

    static IDE_RC calcLfBMPInfo( idvSQL          * aStatistics,
                                 scSpaceID         aSpaceID,
                                 sdpstExtDesc    * aExtDescPtr,
                                 scPageID          aPageID,
                                 sdpParentInfo   * aParentInfo );

    static IDE_RC makeExtRID( idvSQL          * aStatistics,
                              scSpaceID         aSpaceID,
                              scPageID          aExtDirPID,
                              SShort            aSlotNoInExtDir,
                              sdRID           * aExtRID );

    static IDE_RC formatPageUntilNxtLfBMP( idvSQL          * aStatistics,
                                           sdrMtxStartInfo * aStartInfo,
                                           scSpaceID         aSpaceID,
                                           sdpSegHandle    * aSegHandle,
                                           sdRID             aExtRID,
                                           scPageID        * aFstFmtPID,
                                           scPageID        * aLstFmtPID,
                                           scPageID        * aLstFmtExtDirPID,
                                           SShort          * aLstFmtSlotNoInExtDir );

    static inline idBool isFreePIDInExt( sdpstExtDesc  * aExtDesc,
                                         scPageID        aPrvAllocPageID );

    static IDE_RC dumpHdr( UChar * aPagePtr,
                           SChar * aOutBuf,
                           UInt    aOutSize );

    static IDE_RC dumpBody( UChar * aPagePtr,
                            SChar * aOutBuf,
                            UInt    aOutSize );

    static IDE_RC dump( UChar * aPagePtr );

private:

    static inline idBool isExistLfBMPInExt( sdpExtInfo * aExtInfo );

    static void addExtDescToMap( sdpstExtDesc * aExtMap,
                                 SShort         aLstSlotNo,
                                 sdpstExtDesc * aExtDesc );


    static inline void getNxtExtRID( sdpstExtDirHdr  * aExtDirHdr,
                                     SShort            aSlotNo,
                                     sdRID           * aAllocExtRID );

    static IDE_RC getFstExtRIDInNxtExtDir( idvSQL     * aStatistics,
                                           scSpaceID    aSpaceID,
                                           scPageID     aNxtExtMapPID,
                                           sdRID      * aFstExtRID );

    static IDE_RC getFstDataPIDOfExt( idvSQL         * aStatistics,
                                      scSpaceID        aSpaceID,
                                      sdRID            aAllocExtRID,
                                      scPageID       * aAllocPID,
                                      sdpParentInfo  * aParentInfo );


};

// Extent에 Leaf Bitmap 페이지를 포함하는지 여부 반환
inline idBool sdpstExtDir::isExistLfBMPInExt( sdpExtInfo * aExtInfo )
{
    IDE_ASSERT( aExtInfo != NULL );

    if ( (aExtInfo->mFstPID <= aExtInfo->mExtMgmtLfBMP ) &&
         (aExtInfo->mLstPID > aExtInfo->mExtMgmtLfBMP ) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}


/* 동일한 ExtDir에서 다음 ExtDesc의 위치를 반환한다. */
inline void sdpstExtDir::getNxtExtRID( sdpstExtDirHdr  * aExtDirHdr,
                                       SShort            aSlotNo,
                                       sdRID           * aAllocExtRID )
{
    sdpstExtDesc *sMapPtr;

    IDE_ASSERT( aExtDirHdr != NULL );
    IDE_ASSERT( aAllocExtRID != NULL );

    if ( aSlotNo == (aExtDirHdr->mExtCnt - 1))
    {
        // 마지막 Slot이므로 현재 ExtDir 페이지에서는
        // 다음 Extent Slot이 존재하지 않는다.
        *aAllocExtRID = SD_NULL_RID;
    }
    else
    {
        sMapPtr = getMapPtr( aExtDirHdr );
        *aAllocExtRID = sdpPhyPage::getRIDFromPtr( (UChar*)
                                                   &(sMapPtr[ aSlotNo + 1]) );
    }

    return;
}


/*
 * Extent에서 페이지를 할당할 수 있는지 여부를 반환한다.
 * 본 함수는 allocNewPage4Append 에서만 사용할 수 있다.
 * 왜냐하면, 인자로 주어진 PID이후가 Extent 내에 포함된다면,
 * Free Page임을 보장할 수 있기 때문이다.
 */
inline idBool sdpstExtDir::isFreePIDInExt( sdpstExtDesc    * aExtDesc,
                                           scPageID          aPageID )
{
    scPageID sFstPID = aExtDesc->mExtFstPID;
    scPageID sLstPID = aExtDesc->mExtFstPID + aExtDesc->mLength - 1;
    scPageID sNxtPID = aPageID + 1;

    if ( (sFstPID <= sNxtPID) && ( sLstPID >= sNxtPID) )
    {
        return ID_TRUE;
    }
    return ID_FALSE;
}

/* ExtDir 페이지에서 ExtDesc의 index를 계산한다. */
inline SShort sdpstExtDir::calcOffset2SlotNo( sdpstExtDirHdr * aExtDirHdr,
                                              scOffset         aOffset )
{
    UShort  sBodyOffset;

    if ( aExtDirHdr == NULL )
    {
        sBodyOffset = sdpPhyPage::getDataStartOffset( ID_SIZEOF(sdpstExtDirHdr) );
    }
    else
    {
        sBodyOffset = aExtDirHdr->mBodyOffset;
    }
    return (SShort)((aOffset - sBodyOffset) / ID_SIZEOF(sdpstExtDesc));
}

/* ExtDir 페이지에서 ExtDesc의 Offset을 계산한다. */
inline scOffset sdpstExtDir::calcSlotNo2Offset( sdpstExtDirHdr * aExtDirHdr,
                                                SShort           aExtDescIdx )
{
    UShort  sBodyOffset;

    if ( aExtDirHdr == NULL )
    {
        sBodyOffset = sdpPhyPage::getDataStartOffset( ID_SIZEOF(sdpstExtDirHdr) );
    }
    else
    {
        sBodyOffset = aExtDirHdr->mBodyOffset;
    }

    return (scOffset)(sBodyOffset + ( aExtDescIdx * ID_SIZEOF(sdpstExtDesc)));
}

/* ExtDir 페이지의 map ptr을 반환한다. */
inline sdpstExtDesc * sdpstExtDir::getMapPtr( sdpstExtDirHdr * aExtDirHdr )
{
    IDE_ASSERT( aExtDirHdr->mBodyOffset != 0 );
    return (sdpstExtDesc*)(sdpPhyPage::getPageStartPtr((UChar*)aExtDirHdr) +
                           aExtDirHdr->mBodyOffset);
}

/*
 * ExtDir Control Header의 Ptr 반환
 */
inline sdpstExtDirHdr * sdpstExtDir::getHdrPtr( UChar   * aPagePtr )
{
    sdpPageType      sPageType;
    sdpstExtDirHdr * sExtDirHdr;
    sdpstSegHdr    * sSegHdr;

    sPageType = (sdpPageType)sdpPhyPage::getPhyPageType( aPagePtr );
    if ( sPageType == SDP_PAGE_TMS_SEGHDR )
    {
        sSegHdr = (sdpstSegHdr*)
                  sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );

        sExtDirHdr = &sSegHdr->mExtDirHdr;
    }
    else
    {
        sExtDirHdr = (sdpstExtDirHdr*)
                     sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
    }
    return sExtDirHdr;
}

/* ExtDir 페이지에서의 가용한 Extent Slot 개수 반환 */
inline UShort  sdpstExtDir::getFreeSlotCnt( sdpstExtDirHdr * aExtDirHdr )
{
    IDE_ASSERT( aExtDirHdr != NULL );
    return (UShort)( aExtDirHdr->mMaxExtCnt - aExtDirHdr->mExtCnt );
}

/*
 * ExtDir 페이지에 기록할 수 있는 최대 Extent Slot의 개수반환
 */
inline UInt sdpstExtDir::getMaxSlotCnt4Property()
{
    return (UInt)
           ( (sdpPhyPage::getEmptyPageFreeSize() - ID_SIZEOF(sdpstExtDirHdr)) /
             ID_SIZEOF(sdpstExtDesc) );
}

inline void sdpstExtDir::clearExtDesc( sdpstExtDirHdr * aExtDirHdr,
                                       sdpstExtDesc   * aExtDesc )
{
    IDE_ASSERT( aExtDirHdr != NULL );
    IDE_ASSERT( aExtDesc != NULL );

    aExtDesc->mExtFstPID     = SD_NULL_PID;
    aExtDesc->mLength        = 0;
    aExtDesc->mExtMgmtLfBMP  = SD_NULL_PID;
    aExtDesc->mExtFstDataPID = SD_NULL_PID;

    IDE_ASSERT( aExtDirHdr->mExtCnt > 0 );
    aExtDirHdr->mExtCnt--;

    return;
}

/* 첫번째 Extent Slot을 반환한다. */
inline sdpstExtDesc * sdpstExtDir::getFstExtDesc( sdpstExtDirHdr * aExtDirHdr )
{
    return getMapPtr( aExtDirHdr );
}

inline sdpstExtDesc * sdpstExtDir::getLstExtDesc( sdpstExtDirHdr * aExtDirHdr )
{
    IDE_DASSERT( aExtDirHdr != NULL );
    return (sdpstExtDesc*)( sdpPhyPage::getPageStartPtr( aExtDirHdr ) +
                            aExtDirHdr->mBodyOffset +
                            (ID_SIZEOF(sdpstExtDesc) * 
                             (aExtDirHdr->mExtCnt - 1)) );
}

/***********************************************************************
 * Description : ext dir hdr에 ExtDesc Count설정
 **********************************************************************/
inline IDE_RC sdpstExtDir::setExtDescCnt( sdrMtx          * aMtx,
                                          sdpstExtDirHdr  * aExtDirPageHdr,
                                          UShort            aExtDescCnt )
{
    IDE_DASSERT( aExtDirPageHdr != NULL );
    IDE_DASSERT( aMtx != NULL );

    return sdrMiniTrans::writeNBytes( aMtx,
                                      (UChar*)&( aExtDirPageHdr->mExtCnt ),
                                      &aExtDescCnt,
                                      ID_SIZEOF( aExtDirPageHdr->mExtCnt ) );
}

inline scPageID sdpstExtDir::getNxtExtDir ( sdpstExtDirHdr  * aExtDirHdr,
                                            scPageID          aSegPID )
{
    sdpPhyPageHdr   * sPhyPageHdr;
    sdpstSegHdr     * sSegHdr;
    ULong             sNodeCnt;
    scPageID          sNxtExtDir = SD_NULL_PID;

    IDE_ASSERT( aExtDirHdr != NULL );

    sPhyPageHdr = sdpPhyPage::getHdr( sdpPhyPage::getPageStartPtr(aExtDirHdr) );

    /* SegHdr 페이지이면, PIDList의 첫번째 ExtDir을 가져온다.
     * 물론 SegHdr가 마지막 ExtDir 인 경우도 있다. */
    if ( sdpPhyPage::getPageType( sPhyPageHdr ) == SDP_PAGE_TMS_SEGHDR )
    {
        sSegHdr  = (sdpstSegHdr*)sdpPhyPage::getLogicalHdrStartPtr(
                            (UChar*)sPhyPageHdr );
        sNodeCnt = sdpDblPIDList::getNodeCnt( &sSegHdr->mExtDirBase );
        if ( sNodeCnt > 0 )
        {
            sNxtExtDir = sdpDblPIDList::getListHeadNode( &sSegHdr->mExtDirBase );
            IDE_ASSERT( sNxtExtDir != SD_NULL_PID );
        }
    }
    else
    {
        sNxtExtDir = sdpPhyPage::getNxtPIDOfDblList( sPhyPageHdr );
        if ( sNxtExtDir == aSegPID )
        {
            sNxtExtDir = SC_NULL_PID;
        }
    }

    return sNxtExtDir;
}

#endif // _O_SDPST_ExtDir_H_

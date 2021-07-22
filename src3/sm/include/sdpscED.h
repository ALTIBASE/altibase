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
 * $Id: sdpscED.h 82075 2018-01-17 06:39:52Z jina.kim $
 *
 * 본 파일은 Circular-List Managed Segment의 ExtDir Page 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPSC_EXT_DIR_H_
# define _O_SDPSC_EXT_DIR_H_ 1

# include <sdpPhyPage.h>
# include <sdpscDef.h>

class sdpscExtDir
{
public:

    static IDE_RC getCurExtDirInfo( idvSQL            * aStatistics,
                                    scSpaceID           aSpaceID,
                                    sdpSegHandle      * aSegHandle,
                                    scPageID            aCurExtDir,
                                    sdpscExtDirInfo   * aCurExtDirInfo );

    /* ExtDir Page 생성 및 초기화 */
    static IDE_RC createAndInitPage( idvSQL    * aStatistics,
                                     sdrMtx    * aMtx,
                                     scSpaceID   aSpaceID,
                                     scPageID    aNewExtDirPID,
                                     scPageID    aNxtExtDirPID,
                                     UShort      aMaxExtCntInExtDir,
                                     UChar    ** aPagePtr );

    /* ExtDir Page Control Header 초기화 및 write logging */
    static IDE_RC logAndInitCntlHdr( sdrMtx               * aMtx,
                                     sdpscExtDirCntlHdr   * aCntlHdr,
                                     scPageID               aNxtExtDirPID,
                                     UShort                 aMaxExtCnt );

    /* ExtDir Page Control Header 초기화 */
    static void  initCntlHdr( sdpscExtDirCntlHdr * aCntlHdr,
                              scPageID             aNxtExtDir,
                              scOffset             aMapOffset,
                              UShort               aMaxExtCnt );

    /* Segment Header로부터 마지막 ExtDir에 기록 */
    static IDE_RC logAndAddExtDesc( sdrMtx             * aMtx,
                                    sdpscExtDirCntlHdr * aCntlHdr,
                                    sdpscExtDesc       * aExtDesc );

    /* 직접 Extent로부터 새로운 페이지를 할당한다. */
    static IDE_RC allocNewPageInExt( idvSQL         * aStatistics,
                                     sdrMtx         * aMtx,
                                     scSpaceID        aSpaceID,
                                     sdpSegHandle   * aSegHandle,
                                     sdRID            aPrvAllocExtRID,
                                     scPageID         aFstPIDOfPrvAllocExt,
                                     scPageID         aPrvAllocPageID,
                                     sdRID          * aAllocExtRID,
                                     scPageID       * aFstPIDOfAllocExt,
                                     scPageID       * aAllocPID,
                                     sdpParentInfo  * aParentInfo );

    static IDE_RC tryAllocExtDir( idvSQL               * aStatistics,
                                  sdrMtxStartInfo      * aStartInfo,
                                  scSpaceID              aSpaceID,
                                  sdpscSegCache        * aSegCache,
                                  scPageID               aSegPID,
                                  sdpFreeExtDirType      aFreeListIdx,
                                  scPageID               aNxtExtDirPID,
                                  sdpscAllocExtDirInfo * aAllocExtDirInfo );

    static IDE_RC checkNxtExtDir4Steal( idvSQL               * aStatistics,
                                        sdrMtxStartInfo      * aStartInfo,
                                        scSpaceID              aSpaceID,
                                        scPageID               aSegHdrPID,
                                        scPageID               aNxtExtDirPID,
                                        sdpscAllocExtDirInfo * aAllocExtDirInfo );

    static IDE_RC setNxtExtDir( idvSQL          * aStatistics,
                                sdrMtx          * aMtx,
                                scSpaceID         aSpaceID,
                                scPageID          aToLstExtDir,
                                scPageID          aPageID );

    static IDE_RC makeExtDirFull( idvSQL     * aStatistics,
                                  sdrMtx     * aMtx,
                                  scSpaceID    aSpaceID,
                                  scPageID     aExtDir );
    
    /* extent map에 extslot을 기록한다. */
    static void addExtDesc( sdpscExtDirCntlHdr * aCntlHdr,
                            SShort               aLstDescIdx,
                            sdpscExtDesc       * aExtDesc );

    // Write 목적으로 ExtDir 페이지의 Control Header를 fix한다.
    static IDE_RC fixAndGetCntlHdr4Write(
                      idvSQL              * aStatistics,
                      sdrMtx              * aMtx,
                      scSpaceID             aSpaceID,
                      scPageID              aExtDirPID,
                      sdpscExtDirCntlHdr ** aCntlHdr );

    // Read 목적으로 ExtDir 페이지의 Control Header를 fix한다.
    static IDE_RC fixAndGetCntlHdr4Read(
                      idvSQL              * aStatistics,
                      sdrMtx              * aMtx,
                      scSpaceID             aSpaceID,
                      scPageID              aExtDirPID,
                      sdpscExtDirCntlHdr ** aCntlHdr );

    // Control Header가 속해있는 페이지를 release한다.
    static IDE_RC releaseCntlHdr( idvSQL              * aStatistics,
                                  sdpscExtDirCntlHdr  * aCntlHdr );


    /* [ INTERFACE ] 다음 ExtDesc의 RID를 반환한다. */
    static IDE_RC getNxtExtRID( idvSQL       *aStatistics,
                                scSpaceID     aSpaceID,
                                scPageID      aSegHdrPID,
                                sdRID         aCurrExtRID,
                                sdRID        *aNxtExtRID );

    /* [ INTERFACE ] Extent 정보를 반환한다. */
    static IDE_RC getExtInfo( idvSQL       *aStatistics,
                              scSpaceID     aSpaceID,
                              sdRID         aExtRID,
                              sdpExtInfo   *aExtInfo );

    /* [ INTERFACE ] Sequential Scan시 다음 페이지 반환 */
    static IDE_RC getNxtAllocPage( idvSQL           * aStatistics,
                                   scSpaceID          aSpaceID,
                                   sdpSegInfo       * aSegInfo,
                                   sdpSegCacheInfo  * /*aSegCacheInfo*/,
                                   sdRID            * aExtRID,
                                   sdpExtInfo       * aExtInfo,
                                   scPageID         * aPageID );

    static IDE_RC markSCN4ReCycle( idvSQL        * aStatistics,
                                   scSpaceID       aSpaceID,
                                   scPageID        aSegPID,
                                   sdpSegHandle  * aSegHandle,
                                   sdRID           aFstExtRID,
                                   sdRID           aLstExtRID,
                                   smSCN         * aCommitSCN );


    /* BUG-31055 Can not reuse undo pages immediately after it is used to 
     *           aborted transaction */
    /* [ INTERFACE ] Extent들을 Tablespace로 반환함.
     * Abort된 Transaction이 사용한 Extent들을 즉시 재활용하기 위함. */
    static IDE_RC shrinkExts( idvSQL            * aStatistics,
                              scSpaceID           aSpaceID,
                              scPageID            aSegPID,
                              sdpSegHandle      * aSegHandle,
                              sdrMtxStartInfo   * aStartInfo,
                              sdpFreeExtDirType   aFreeListIdx,
                              sdRID               aFstExtRID,
                              sdRID               aLstExtRID );
    /* BUG-42975 undo tablespace를 undo segment가 전부 사용하면 tss segment가 
                 확장을 할수가 없다. 이 경우 undo segment에서 steal을 해야 하며
                 steal한 extCnt 가 ExtDir에서 관리하는 ExtCnt보다 크면 큰경우 ExtDir을 shrink 한다.*/ 
    static IDE_RC shrinkExtDir( idvSQL                * aStatistics,
                                sdrMtx                * aMtx,
                                scSpaceID               aSpaceID,
                                sdpSegHandle          * aToSegHandle,
                                sdpscAllocExtDirInfo  * aAllocExtDirInfo );
  
    static IDE_RC setSCNAtAlloc( idvSQL        * aStatistics,
                                 scSpaceID       aSpaceID,
                                 sdRID           aExtRID,
                                 smSCN         * aTransBSCN );

    /* ExtDir 페이지의 map ptr을 반환한다. */
    static inline sdpscExtDirMap * getMapPtr( sdpscExtDirCntlHdr * aCntlHdr );

    static inline SShort calcOffset2DescIdx(
                                       sdpscExtDirCntlHdr * aCntlHdr,
                                       scOffset             aOffset );

    static inline scOffset calcDescIdx2Offset(
                                       sdpscExtDirCntlHdr * aCntlHdr,
                                       SShort               aExtDescIdx );

    /* ExtDir Control Header의 Ptr 반환 */
    static inline sdpscExtDirCntlHdr * getHdrPtr( UChar   * aPagePtr );

    /* ExtDir 페이지에서의 가용한 ExtDesc 개수 반환 */
    static inline UShort  getFreeDescCnt( sdpscExtDirCntlHdr * aCntlHdr );

    static void setLatestCSCN( sdpscExtDirCntlHdr * aCntlHdr,
                               smSCN              * aCommitSCN );

    static void setFstDskViewSCN( sdpscExtDirCntlHdr * aCntlHdr,
                                  smSCN              * aMyFstDskViewSCN );

    static inline sdpscExtDesc *
                   getExtDescByIdx( sdpscExtDirCntlHdr * aExtDirCntlHdr,
                                    UShort               aExtDescIdx );

    static IDE_RC setNxtExtDir( sdrMtx             * aMtx,
                                sdpscExtDirCntlHdr * aExtDirCntlHdr,
                                scPageID             aNxtExtDir );

    static inline void initExtDesc( sdpscExtDesc  * aExtDesc );

private:

    /* ExtDir에 ExtDesc을 기록한다. */
    static void addExtDescToMap( sdpscExtDirMap  * aMapPtr,
                                 SShort            aLstIdx,
                                 sdpscExtDesc    * aExtDesc );

    /* Extent에서 페이지를 할당할 수 있는지 여부를 반환한다. */
    static inline idBool isFreePIDInExt( UInt         aPageCntInExt,
                                         scPageID     aFstPIDOfAllocExt,
                                         scPageID     aLstAllocPageID );

    /* 다음 ExtDesc의 위치를 반환한다. */
    static inline void getNxtExt( sdpscExtDirCntlHdr  * aExtDirCntlHdr,
                                  sdRID                 aCurExtRID,
                                  sdRID               * aNxtExtRID,
                                  sdpscExtDesc        * aNxtExtDesc );

    static IDE_RC checkExtDirState4Reuse(
                                    idvSQL           * aStatistics,
                                    scSpaceID          aSpaceID,
                                    scPageID           aSegPID,
                                    scPageID           aCurExtDir,
                                    smSCN            * aMyFstDskViewSCN,
                                    smSCN            * aSysMinDskViewSCN,
                                    sdpscExtDirState * aExtDirState,
                                    sdRID            * aAllocExtRID,
                                    sdpscExtDesc     * aExtDesc,
                                    scPageID         * aNxtExtDir,
                                    UInt             * aExtCntInExtDir );

    static IDE_RC getNxtExt4Alloc( idvSQL       * aStatistics,
                                   scSpaceID      aSpaceID,
                                   sdRID          aCurExtRID,
                                   sdRID        * aNxtExtRID,
                                   scPageID     * aFstPIDOfExt,
                                   scPageID     * aFstDataPIDOfNxtExt );

    static IDE_RC getExtDescInfo( idvSQL        * aStatistics,
                                  scSpaceID       aSpaceID,
                                  scPageID        aExtDirPID,
                                  UShort          aIdx,
                                  sdRID         * aExtRID,
                                  sdpscExtDesc  * aExtDescPtr );

    static inline idBool isExeedShrinkThreshold( sdpscSegCache * aSegCache );

    static inline void initExtDirInfo( sdpscExtDirInfo  * aExtDirInfo );
    static inline void initAllocExtDirInfo(
                                       sdpscAllocExtDirInfo  * aAllocExtDirInfo );
};

inline idBool sdpscExtDir::isExeedShrinkThreshold( sdpscSegCache * aSegCache )
{
    // BUG-27329 CodeSonar::Uninitialized Variable (2)
    UInt sSizeThreshold = ID_UINT_MAX;

    switch( aSegCache->mCommon.mSegType )
    {
        case SDP_SEG_TYPE_TSS:
            sSizeThreshold = smuProperty::getTSSegSizeShrinkThreshold();
            break;
        case SDP_SEG_TYPE_UNDO:
            sSizeThreshold = smuProperty::getUDSegSizeShrinkThreshold();
            break;
            // BUG-27329 CodeSonar::Uninitialized Variable (2)
            // SDP_SEG_TYPE_TSS, SDP_SEG_TYPE_UNDO 이외의 경우는 올 수 없다.
            IDE_ASSERT(0);
        default:
            break;
    }

    if ( aSegCache->mCommon.mSegSizeByBytes >= sSizeThreshold )
    {
        return ID_TRUE;
    }

    return ID_FALSE;
}

/* 동일한 ExtDir에서 다음 ExtDesc의 위치를 반환한다. */
inline void sdpscExtDir::getNxtExt( sdpscExtDirCntlHdr  * aExtDirCntlHdr,
                                    sdRID                 aCurExtRID,
                                    sdRID               * aNxtExtRID,
                                    sdpscExtDesc        * aNxtExtDesc )
{
    UShort              sIdx;
    sdpscExtDesc      * sNxtExtDescPtr;
    sdRID               sNxtExtRID;

    IDE_ASSERT( aExtDirCntlHdr != NULL );
    IDE_ASSERT( aNxtExtRID     != NULL );
    IDE_ASSERT( aNxtExtDesc    != NULL );

    sIdx = calcOffset2DescIdx( aExtDirCntlHdr,
                               SD_MAKE_OFFSET( aCurExtRID ));

    if ( sIdx == (aExtDirCntlHdr->mExtCnt - 1))
    {
        // 마지막 Desc이므로 현재 ExtDir 페이지에서는
        // 다음 ExtDesc이 존재하지 않는다.
        sNxtExtRID   = SD_NULL_RID;
    }
    else
    {
        sNxtExtDescPtr = getExtDescByIdx( aExtDirCntlHdr, sIdx+1 );
        sNxtExtRID     = sdpPhyPage::getRIDFromPtr( sNxtExtDescPtr );

        *aNxtExtDesc   = *sNxtExtDescPtr; // copy 해준다.
    }

    *aNxtExtRID  = sNxtExtRID;

    return;
}


/* ExtDir 페이지에서 ExtDesc의 index를 계산한다. */
inline SShort sdpscExtDir::calcOffset2DescIdx( sdpscExtDirCntlHdr * aCntlHdr,
                                               scOffset             aOffset )
{
    UShort  sMapOffset;

    if ( aCntlHdr == NULL )
    {
        sMapOffset = sdpPhyPage::getDataStartOffset( ID_SIZEOF(sdpscExtDirCntlHdr) );
    }
    else
    {
        sMapOffset = aCntlHdr->mMapOffset;
    }
    return (SShort)((aOffset - sMapOffset) / ID_SIZEOF(sdpscExtDesc));
}

/* ExtDir 페이지에서 ExtDesc의 Offset을 계산한다. */
inline scOffset sdpscExtDir::calcDescIdx2Offset( sdpscExtDirCntlHdr * aCntlHdr,
                                                 SShort               aExtDescIdx )
{
    UShort  sMapOffset;

    if ( aCntlHdr == NULL )
    {
        sMapOffset = sdpPhyPage::getDataStartOffset( ID_SIZEOF(sdpscExtDirCntlHdr) );
    }
    else
    {
        sMapOffset = aCntlHdr->mMapOffset;
    }

    return (scOffset)(sMapOffset + ( aExtDescIdx * ID_SIZEOF(sdpscExtDesc)));
}

/* ExtDir 페이지의 map ptr을 반환한다. */
inline sdpscExtDirMap * sdpscExtDir::getMapPtr( sdpscExtDirCntlHdr * aCntlHdr )
{
    IDE_ASSERT( aCntlHdr->mMapOffset != 0 );

    return (sdpscExtDirMap*)
        (sdpPhyPage::getPageStartPtr((UChar*)aCntlHdr) +
          aCntlHdr->mMapOffset);
}

/* ExtDir Control Header의 Ptr 반환 */
inline sdpscExtDirCntlHdr * sdpscExtDir::getHdrPtr( UChar   * aPagePtr )
{
    return (sdpscExtDirCntlHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
}

/* ExtDir 페이지에서의 가용한 ExtDesc 개수 반환 */
inline UShort  sdpscExtDir::getFreeDescCnt( sdpscExtDirCntlHdr * aCntlHdr )
{
    IDE_ASSERT( aCntlHdr->mMaxExtCnt >= aCntlHdr->mExtCnt );
    return (UShort)( aCntlHdr->mMaxExtCnt - aCntlHdr->mExtCnt );
}

/* aPageID가 속한 Extent에 aPageID이후로 Free Page가 있는지 조사한다. */
inline idBool sdpscExtDir::isFreePIDInExt( 
                                 UInt         aPageCntInExt,
                                 scPageID     aFstPIDOfAllocExt,
                                 scPageID     aLstAllocPageID )
{
    UInt sAllocPageCntInExt = aLstAllocPageID - aFstPIDOfAllocExt + 1;

    IDE_ASSERT( aPageCntInExt >= SDP_MIN_EXTENT_PAGE_CNT );

    if( sAllocPageCntInExt < aPageCntInExt )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

/* Extent Dir. Map에서 aExtDescIdx번째 ExtDesc를 반환한다. */
inline sdpscExtDesc * sdpscExtDir::getExtDescByIdx(
                                   sdpscExtDirCntlHdr * aExtDirCntlHdr,
                                   UShort               aExtDescIdx )
{
    sdpscExtDirMap * sExtMapPtr = getMapPtr( aExtDirCntlHdr );

    return &(sExtMapPtr->mExtDesc[ aExtDescIdx ] );
}


/***********************************************************************
 *
 * Description : Extent Desc 초기화
 *
 * aExtDesc  - [OUT] Extent Desc.포인터
 *
 ***********************************************************************/
inline void sdpscExtDir::initExtDesc( sdpscExtDesc  * aExtDesc )
{
    aExtDesc->mExtFstPID     = SD_NULL_PID;
    aExtDesc->mLength        = 0;
    aExtDesc->mExtFstDataPID = SD_NULL_PID;
}

/***********************************************************************
 *
 * Description : ExtDir 정보 자료구조
 *
 * aExtDirInfo - [OUT] ExtDirInfo 자료구조 포인터
 *
 ***********************************************************************/
inline void sdpscExtDir::initExtDirInfo( sdpscExtDirInfo  * aExtDirInfo )
{
    IDE_ASSERT( aExtDirInfo != NULL );

    aExtDirInfo->mExtDirPID    = SD_NULL_PID;
    aExtDirInfo->mNxtExtDirPID = SD_NULL_PID;
    aExtDirInfo->mIsFull       = ID_FALSE;
    aExtDirInfo->mTotExtCnt    = 0;
    aExtDirInfo->mMaxExtCnt    = 0;
}

/***********************************************************************
 *
 * Description : 세그먼트 생성 혹은 확장 연산시 필요한 정보를 저장하는 자료구조 초기화
 *
 * aAllocExtInfo - [IN] Extent 확장정보 포인터
 *
 ***********************************************************************/
inline void sdpscExtDir::initAllocExtDirInfo( sdpscAllocExtDirInfo  * aAllocExtDirInfo )
{
    IDE_ASSERT( aAllocExtDirInfo != NULL );

    aAllocExtDirInfo->mIsAllocNewExtDir  = ID_FALSE;
    aAllocExtDirInfo->mNewExtDirPID      = SD_NULL_PID;
    aAllocExtDirInfo->mNxtPIDOfNewExtDir = SD_NULL_PID;
    aAllocExtDirInfo->mShrinkExtDirPID   = SD_NULL_PID;

    initExtDesc( &aAllocExtDirInfo->mFstExtDesc );

    aAllocExtDirInfo->mTotExtCntOfSeg    = 0;
    aAllocExtDirInfo->mFstExtDescRID     = SD_NULL_RID;

    aAllocExtDirInfo->mExtCntInShrinkExtDir = 0;
}

#endif // _O_SDPSC_EXT_DIR_H_

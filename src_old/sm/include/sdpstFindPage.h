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
 * $Id: sdpstFindPage.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 가용공간 탐색 연산 관련
 * 헤더 파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_FIND_PAGE_H_
# define _O_SDPST_FIND_PAGE_H_ 1

# include <sdpDef.h>
# include <sdpReq.h>

class sdpstFindPage
{
public:
    static IDE_RC findInsertablePage( idvSQL           * aStatistics,
                                      sdrMtx           * aMtx,
                                      scSpaceID          aSpaceID,
                                      sdpSegHandle     * aSegmentHandle,
                                      void             * aTableInfo,
                                      sdpPageType        aPageType,
                                      UInt               aRowSize,
                                      idBool          /* aNeedKeySlot*/,
                                      UChar           ** aPagePtr,
                                      UChar            * aCTSlotNo );

    static IDE_RC searchFreeSpace( idvSQL            * aStatistics,
                                   sdrMtx            * aMtx,
                                   scSpaceID           aSpaceID,
                                   sdpSegHandle      * aSegHandle,
                                   UInt                aRowSize,
                                   sdpPageType         aPageType,
                                   sdpstSearchType     aSearchType,
                                   UChar            ** aPagePtr,
                                   UChar             * aCTSlotNo,
                                   scPageID          * aNewHintDataPID );

    static idBool needToChangePageFN( sdpPhyPageHdr    * aPageHdr,
                                  sdpSegHandle     * aSegHandle,
                                  sdpstPBS         * aNewPageBitSet );

    static inline idBool isAvailable( sdpstMFNL  aSrcMFNL,
                                      sdpstMFNL  aTargetMFNL );

    static IDE_RC checkSizeAndAllocCTS(
                                     idvSQL          * aStatistics,
                                     sdrMtx          * aMtx,
                                     scSpaceID         aSpaceID,
                                     sdpSegHandle    * aSegHandle,
                                     UChar           * aPagePtr,
                                     UInt              aRowSize,
                                     idBool          * aCanAlloc,
                                     UChar           * aCTSlotNo );

private:
    static IDE_RC checkAndSearchHintDataPID( idvSQL             * aStatistics,
                                             sdrMtx             * aMtx,
                                             scSpaceID            aSpaceID,
                                             sdpSegHandle       * aSegHandle,
                                             void               * aTableInfo,
                                             scPageID             aHintDataPID,
                                             UInt                 aRowSize,
                                             UChar             ** aPagePtr,
                                             UChar              * aCTSlotNo,
                                             scPageID           * aNewHintDataPID );

    static IDE_RC tryToAllocExtsAndPage( idvSQL           * aStatistics,
                                   sdrMtx           * aMtx4Latch,
                                   scSpaceID          aSpaceID,
                                   sdpSegHandle     * aSegHandle,
                                   sdpstSearchType    aSearchType,
                                   sdpPageType        aPageType,
                                   sdpstStack       * aStack,
                                   UChar           ** aFstDataPagePtr );

    static IDE_RC searchSpaceInRtBMP( idvSQL            * aStatistics,
                                      sdrMtx            * aMtx,
                                      scSpaceID           aSpaceID,
                                      sdpSegHandle      * aSegHandle,
                                      sdpstStack        * aStack,
                                      sdpstSearchType     aSearchType,
                                      idBool            * aNeedToExtendExt );

    static IDE_RC searchSpaceInItBMP( idvSQL          * aStatistics,
                                      sdrMtx          * aMtx,
                                      scSpaceID         aSpaceID,
                                      scPageID          aItBMP, 
                                      sdpstSearchType   aSearchType,
                                      sdpstSegCache   * aSegCache,
                                      idBool          * aGoNextIt,
                                      sdpstPosItem    * aArrLeafBMP,
                                      UInt            * aCandidateCount );

    static IDE_RC searchSpaceInLfBMP( idvSQL          * aStatistics,
                                      sdrMtx          * aMtx,
                                      scSpaceID         aSpaceID,
                                      sdpSegHandle    * aSegHandle,
                                      scPageID          aLeafBMP,
                                      sdpstStack      * aStack,
                                      sdpPageType       aPageType,
                                      sdpstSearchType   aSearchType,
                                      UInt              aRecordSize,
                                      UChar          ** aPagePtr,
                                      UChar           * aCTSlotNo );

    static IDE_RC searchPagesInCandidatePage(
                                idvSQL               * aStatistics,
                                sdrMtx               * aMtx,
                                scSpaceID              aSpaceID,
                                sdpSegHandle         * aSegHandle,
                                scPageID               aLeafBMP,
                                sdpstCandidatePage   * aArrData,
                                sdpPageType            aPageType,
                                UInt                   aCandidateDataCount,
                                sdpstSearchType        aSearchType,
                                UInt                   aRecordSize,
                                sdbWaitMode            aWaitMode,
                                UChar               ** aPagePtr,
                                UChar                * aCTSlotNo );

private:

    inline static idBool isPageInsertable( sdpPhyPageHdr *aPageHdr,
                                           UInt           aPctUsed );

    inline static idBool isPageUpdateOnly( sdpPhyPageHdr *aPageHdr,
                                           UInt           aPctFree );

};

// 탐색대상에 부합하는 leaf slot이 존재하는지 판단한다.
inline idBool sdpstFindPage::isAvailable( sdpstMFNL  aSrcMFNL,
                                          sdpstMFNL  aTargetMFNL )
{
    return ( aSrcMFNL >= aTargetMFNL ? ID_TRUE : ID_FALSE);
}

/***********************************************************************
 * Page 상태가 Insert High Limit 이하로 떨어졌는지 Check한다.
 ***********************************************************************/
inline idBool sdpstFindPage::isPageInsertable( sdpPhyPageHdr *aPageHdr,
                                               UInt           aPctUsed )
{
    idBool sIsInsertable = ID_FALSE;
    UInt   sUsedSize     = 0;
    UInt   sPctUsedSize;

    IDE_DASSERT( aPageHdr != NULL );
    IDE_DASSERT( aPctUsed < 100 );

    if( sdpPhyPage::canMakeSlotEntry( aPageHdr ) == ID_TRUE )
    {
        sUsedSize = SD_PAGE_SIZE -
            sdpPhyPage::getTotalFreeSize( aPageHdr )    -
            smLayerCallback::getTotAgingSize( aPageHdr ) +
            ID_SIZEOF(sdpSlotEntry);

        sPctUsedSize = ( SD_PAGE_SIZE * aPctUsed )  / 100 ;

        if( sUsedSize < sPctUsedSize )
        {
            sIsInsertable = ID_TRUE;
        }

    }

    return sIsInsertable;
}

/* Page 상태가 Update Only인지를 Check한다. */
inline idBool sdpstFindPage::isPageUpdateOnly( sdpPhyPageHdr *aPageHdr,
                                               UInt           aPctFree )
{
    UInt   sPctFreeSize;
    UInt   sFreeSize     = 0;
    idBool sIsUpdateOnly = ID_FALSE;

    IDE_DASSERT( aPageHdr != NULL );
    IDE_DASSERT( aPctFree < 100 );

    if( sdpPhyPage::canMakeSlotEntry( aPageHdr ) == ID_TRUE )
    {
        sFreeSize = sdpPhyPage::getTotalFreeSize( aPageHdr ) +
            smLayerCallback::getTotAgingSize( aPageHdr );

        sPctFreeSize  = ( SD_PAGE_SIZE * aPctFree )  / 100 ;

        if( sFreeSize < sPctFreeSize )
        {
            sIsUpdateOnly = ID_TRUE;
        }
    }

    return sIsUpdateOnly;
}



#endif // _O_SDPST_FIND_PAGE_H_

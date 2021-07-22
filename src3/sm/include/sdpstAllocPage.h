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
 * $Id: sdpstAllocPage.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * 본 파일은 Treelist Managed Segment에서 가용공간 할당 연산 관련
 * 헤더파일이다.
 *
 ***********************************************************************/

# ifndef _O_SDPST_ALLOC_PAGE_H_
# define _O_SDPST_ALLOC_PAGE_H_ 1

# include <sdpDef.h>
# include <sdpstDef.h>

class sdpstAllocPage
{
public:

    static IDE_RC allocateNewPage( idvSQL             * aStatistics,
                                   sdrMtx             * aMtx,
                                   scSpaceID            aSpaceID,
                                   sdpSegHandle       * aSegmentHandle,
                                   sdpPageType          aPageType,
                                   UChar             ** aNewPagePtr );

    static IDE_RC prepareNewPages( idvSQL             * aStatistics,
                                   sdrMtx             * aMtx,
                                   scSpaceID            aSpaceID,
                                   sdpSegHandle       * aSegmentHandle,
                                   UInt                 aCountWanted );


    static IDE_RC tryToChangeMFNLAndItHint(
                                   idvSQL               * aStatistics,
                                   sdrMtx               * aMtx,
                                   scSpaceID              aSpaceID,
                                   sdpSegHandle         * aSegHandle,
                                   sdpstChangeMFNLPhase   aChangePhase,
                                   scPageID               aChildPID,
                                   SShort                 aPBSNo4lf,
                                   void                 * aChangeState,
                                   scPageID               aCurPID,
                                   SShort                 aSlotNoInParent,
                                   sdpstStack           * aRevStack );

    static IDE_RC checkAndCreatePages( idvSQL             * aStatistics,
                                           sdrMtx             * aMtx,
                                           scSpaceID            aSpaceID,
                                           sdpSegHandle       * aSegHandle,
                                           sdpstStack         * aStack,
                                           scPageID             aLeafBMP,
                                           sdpstCandidatePage * aDataPage,
                                           sdpPageType          aPageType,
                                           UChar             ** aNewPagePtr );

    static IDE_RC createPage( idvSQL           * aStatistics,
                              sdrMtx           * aMtx,
                              scSpaceID          aSpaceID,
                              sdpSegHandle     * aSegHandle,
                              scPageID           aNewPageID,
                              ULong              aSeqNo,
                              sdpPageType        aPageType,
                              scPageID           aParentPID,
                              SShort             aPBSNoInParent,
                              sdpstPBS           aPBS,
                              UChar           ** aNewPagePtr );

    static IDE_RC formatPageHdr( sdrMtx           * aMtx,
                                 sdpSegHandle     * aSegHandle,
                                 sdpPhyPageHdr    * aNewPagePtr,
                                 scPageID           aNewPageID,
                                 ULong              aSeqNo,
                                 sdpPageType        aPageType,
                                 scPageID           aParentPID,
                                 SShort             aPBSNo,
                                 sdpstPBS           aPBS );

    static IDE_RC tryToCheckDataPage( idvSQL         * aStatistics,
                                      sdrMtx         * aMtx,
                                      sdrSavePoint   * aMtxSP,
                                      scSpaceID        aSpaceID,
                                      scPageID         aLeafBMP,
                                      UChar          * aDataPagePtr,
                                      SShort           aDataPBSNo,
                                      UInt             aFixedSize,
                                      UInt             aRowSize,
                                      UInt             aInsHighLimitPct,
                                      UChar         ** aPagePtr );

    static IDE_RC updatePageState( idvSQL             * aStatistics,
                                   sdrMtx             * aMtx,
                                   scSpaceID            aSpaceID,
                                   sdpSegHandle       * aSegHandle,
                                   UChar              * aPagePtr );

    static IDE_RC updatePageFN( idvSQL         * aStatistics,
                                sdrMtx         * aMtx,
                                scSpaceID        aSpaceID,
                                sdpSegHandle   * aSegHandle,
                                UChar          * aDataPagePtr );

    static IDE_RC updatePageFNtoFull( idvSQL         * aStatistics,
                                      sdrMtx         * aMtx,
                                      scSpaceID        aSpaceID,
                                      sdpSegHandle   * aSegHandle,
                                      UChar          * aDataPagePtr );

    static sdpstMFNL calcMFNL( UShort  * aMFNLtbl );

    static IDE_RC makeOrderedStackFromDataPage( idvSQL       * aStatistics,
                             scSpaceID      aSpaceID,
                             scPageID       aSegPID,
                             scPageID       aPageID,
                             sdpstStack   * aOrderedStack );

    static IDE_RC updateBMPUntilHWM( idvSQL           * aStatistics,
                                     scSpaceID          aSpaceID,
                                     scPageID           aSegPID,
                                     sdRID              aLstAllocExtRIDOfFM,
                                     scPageID           aLstAllocPIDOfFM,
                                     sdrMtxStartInfo  * aStartInfo );

    static IDE_RC formatDataPagesInExt(
                                    idvSQL          * aStatistics,
                                    sdrMtxStartInfo * aStartInfo,
                                    scSpaceID         aSpaceID,
                                    sdpSegHandle    * aSegHandle,
                                    sdpstExtDesc    * aExtDesc,
                                    sdRID             aBeginRID,
                                    scPageID          aBeginPID );
};


#endif // _O_SDPST_ALLOC_PAGE_H_

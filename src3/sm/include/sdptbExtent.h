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
 * $Id: sdptbExtent.h 27220 2008-07-23 14:56:22Z newdaily $
 *
 * TBS에서 extent를 할당하고 해제하는 루틴에 관련된 함수들이다.
 ***********************************************************************/

# ifndef _O_SDPTB_EXTENT_H_
# define _O_SDPTB_EXTENT_H_ 1

#include <sdptb.h>
#include <sdpst.h>

class sdptbExtent {
public:
    static IDE_RC initialize(){ return IDE_SUCCESS; }
    static IDE_RC destroy(){ return IDE_SUCCESS; }

    /* tablespace로부터 extent를 할당받는다.*/
    static IDE_RC allocExts( idvSQL         * aStatistics,
                             sdrMtxStartInfo* aStartInfo,
                             scSpaceID        aSpaceID,
                             UInt             aOrgNrExts,
                             sdpExtDesc     * aExtDesc );

    static IDE_RC allocTmpExt( idvSQL      * aStatistics,
                               scSpaceID     aSpaceID,
                               sdpExtDesc  * aExtSlot );

    /* ExtDir 페이지를 할당한다. */
    static IDE_RC tryAllocExtDir( idvSQL             * aStatistics,
                                  sdrMtxStartInfo    * aStartInfo,
                                  scSpaceID            aSpaceID,
                                  sdpFreeExtDirType    aFreeListIdx,
                                  scPageID           * aExtDirPID );

    /* ExtDir 페이지를 해제한다. */
    static IDE_RC freeExtDir( idvSQL             * aStatistics,
                              sdrMtx             * aMtx,
                              scSpaceID            aSpaceID,
                              sdpFreeExtDirType    aFreeListIdx,
                              scPageID             aExtDirPID );

    /* GG에서 extent들의 할당을 시도한다 */
    static IDE_RC tryAllocExtsInGG( idvSQL             * aStatistics,
                                    sdrMtxStartInfo    * aStartInfo,
                                    sdptbSpaceCache    * aCache,  
                                    sdFileID             aFID,
                                    UInt                 aOrgNrExts,
                                    scPageID           * aExtFstPID,
                                    UInt               * aNrDone); 

    /* LG에서 extent들의 할당을 시도한다 */
    static IDE_RC allocExtsInLG( idvSQL                  * aStatistics,
                                 sdrMtx                  * aMtx,
                                 sdptbSpaceCache         * aSpaceCache,  
                                 sdptbGGHdr              * aGGPtr,
                                 scPageID                  aLGHdrPID,
                                 UInt                      aOrgNrExts, 
                                 scPageID                * aExtPID,
                                 UInt                    * aNrDone, 
                                 UInt                    * aFreeInLG);

    /* space cache로부터 현재 할당이 가능할 가능성이 있는 FID를 얻는다*/
    static IDE_RC getAvailFID( sdptbSpaceCache         * aCache,
                           smFileID                 * aFID);

    /* deallcation LG hdr에 free가 있다면 switching을 한다.*/
    static IDE_RC trySwitch( sdrMtx                    *   aMtx,
                             sdptbGGHdr                *   aGGHdrPtr,
                             idBool                    *   aRet,
                             sdptbSpaceCache           *   aCache );

    static IDE_RC prepareCachedFreeExts( idvSQL           * aStatistics,
                                         sctTableSpaceNode* aSpaceNode );

    static IDE_RC freeExt( idvSQL           *  aStatistics,
                           sdrMtx           *  aMtx,
                           scSpaceID           aSpaceID,
                           scPageID            aExtFstPID,
                           UInt             *  aNrDone );
    static IDE_RC freeTmpExt( scSpaceID           aSpaceID,
                              scPageID            aExtFstPID );

    /*[INTERFACE]  TBS에 extent를 반납한다. */
    /* LG에 extent들을 반납한다.*/
    static IDE_RC freeExts( idvSQL           *  aStatistics,
                            sdrMtx           *  aMtx,
                            scSpaceID           aSpaceID,
                            ULong            *  aExtFstPIDArr,
                            UInt               aArrCount );

    static IDE_RC freeExtsInLG( idvSQL           *  aStatistics,
                                sdrMtx           *  aMtx,
                                scSpaceID           aSpaceID,
                                sdptbSortExtSlot *  aSortedExts,
                                UInt                aNrSortedExts,
                                UInt                aBeginIndex,
                                UInt             *  aEndIndex,
                                UInt             *  aNrDone);

    static IDE_RC autoExtDatafileOnDemand( idvSQL          *   aStatistics,
                                           UInt                aSpaceID,
                                           sdptbSpaceCache *   aCache,
                                           void            *   aTransForMtx,
                                           UInt                aNeededPageCnt);

    static void allocByBitmapIndex( sdptbLGHdr * aLGHdr,
                                    UInt          aIndex);

    static void freeByBitmapIndex( sdptbLGHdr *  aLGHdr,
                                   UInt          aIndex );

    static IDE_RC isFreeExtPage( idvSQL   * aStatistics,
                                 scSpaceID  aSpaceID,
                                 scPageID   aPageID,
                                 idBool   * aIsFreeExt);

    /* BUG-47666 X$DATAFILE에 Freeness Of GG 표현 */
    static idBool getFreenessOfGG( sddTableSpaceNode  * aTBSNode,
                                   sdFileID             aFID )
    {
        sdptbSpaceCache * sSpaceCache = sddDiskMgr::getSpaceCache( aTBSNode );

        if ( aFID <= sSpaceCache->mMaxGGID )
        {
            return sdptbBit::getBit( (void *)sSpaceCache->mFreenessOfGGs, aFID );
        }

        return ID_FALSE;
    };
};

#endif // _O_SDPTB_EXTENT_H_

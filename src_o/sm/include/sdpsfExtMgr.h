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

# ifndef _O_SDPSF_EXTMGR_H_
# define _O_SDPSF_EXTMGR_H_ 1

# include <sdpsfDef.h>
# include <sdpsfExtDirPage.h>
# include <sdr.h>

class sdpsfExtMgr
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC allocExt( idvSQL          * aStatistics,
                            sdrMtxStartInfo * aStartInfo,
                            scSpaceID         aSpaceID,
                            sdpsfSegHdr     * aSegHdr );

    static IDE_RC extend( idvSQL          * aStatistics,
                          sdrMtxStartInfo * aStartInfo,
                          scSpaceID         aSpaceID,
                          sdpsfSegHdr     * aSegHdr,
                          sdpSegHandle    * aSegHandle,
                          UInt              aNxtExtCnt );


    static IDE_RC getExtDesc( idvSQL       *aStatistics,
                              scSpaceID     aSpaceID,
                              sdRID         aExtRID,
                              sdpsfExtDesc *aExtDescPtr );

    static IDE_RC getExtInfo( idvSQL       *aStatistics,
                              scSpaceID     aSpaceID,
                              sdRID         aExtRID,
                              sdpExtInfo   *aExtInfo );

    static IDE_RC freeAllExts( idvSQL          *aStatistics,
                               sdrMtx          *aMtx,
                               scSpaceID        aSpaceID,
                               sdpsfSegHdr     *aSegHdr );

    static IDE_RC freeExtsExceptFst( idvSQL              *aStatistics,
                                     sdrMtx              *aMtx,
                                     scSpaceID            aSpaceID,
                                     sdpsfSegHdr         *aSegHdr );

    static IDE_RC allocPage( idvSQL               *aStatistics,
                             sdrMtx               *aAllocMtx,
                             sdrMtx               *aCrtMtx,
                             scSpaceID             aSpaceID,
                             sdpsfSegHdr          *aSegHdr,
                             sdpSegHandle         *aSegHandle,
                             UInt                  aNextExtCnt,
                             sdpPageType           aPageType,
                             scPageID             *aPageID,
                             UChar               **aAllocPagePtr );

    static IDE_RC allocNewPage( idvSQL             *aStatistics,
                                sdrMtx             *aMtx,
                                scSpaceID           aSpaceID,
                                sdpsfSegHdr        *aSegHdr,
                                sdpSegHandle       *aSegHandle,
                                UInt                aNxtExtCnt,
                                sdRID               aPrvAllocExtRID,
                                scPageID            aFstPIDOfPrvExtAllocExt,
                                scPageID            aPrvAllocPageID,
                                sdRID              *aAllocExtRID,
                                scPageID           *aFstPIDOfAllocExt,
                                scPageID           *aAllocPID,
                                idBool              aIsAppendMode );

    static IDE_RC getExtListInfo( idvSQL   *aStatistics,
                                  scSpaceID aSpaceID,
                                  scPageID  aSegPID,
                                  UInt     *aPageCntInExt,
                                  sdRID    *aFstExtRID,
                                  sdRID    *aLstExtRID );

    static IDE_RC getNxtAllocPage( idvSQL           * aStatistics,
                                   scSpaceID          aSpaceID,
                                   sdpSegInfo       * aSegInfo,
                                   sdpSegCacheInfo  * aSegCacheInfo,
                                   sdRID            * aExtRID,
                                   sdpExtInfo       * aExtInfo,
                                   scPageID         * aPageID );

    static IDE_RC freeAllNxtExt( idvSQL       *aStatistics,
                                 sdrMtx       *aMtx,
                                 scSpaceID     aSpaceID,
                                 sdpsfSegHdr  *aSegHdr,
                                 sdRID         aExtRID );

    static IDE_RC getNxtExt4Alloc( idvSQL       * aStatistics,
                                   scSpaceID      aSpaceID,
                                   sdpsfSegHdr  * aSegHdr,
                                   sdRID          aCurExtRID,
                                   sdRID        * aNxtExtRID,
                                   scPageID     * aFstPIDOfExt,
                                   scPageID     * aFstFreePIDOfNxtExt );

    static IDE_RC getNxtExt4Scan( idvSQL       * aStatistics,
                                  scSpaceID      aSpaceID,
                                  scPageID       aSegHdrPID,
                                  sdRID          aCurExtRID,
                                  sdRID        * aNxtExtRID,
                                  scPageID     * aFstPIDOfNxtExt,
                                  scPageID     * aFstDataPIDOfNxtExt );

    static IDE_RC getNxtExtRID( idvSQL       * aStatistics,
                                scSpaceID      aSpaceID,
                                scPageID       aSegHdrPID,
                                sdRID          aCurExtRID,
                                sdRID        * aNxtExtRID);

    static IDE_RC allocMutliExt( idvSQL           * aStatistics,
                                 sdrMtxStartInfo  * aStartInfo,
                                 scSpaceID          aSpaceID,
                                 sdpSegHandle     * aSegHandle,
                                 UInt               aExtCount );


/* inline Function */
public:
    inline static ULong getExtCnt( sdpsfSegHdr *aSegHdr );
    inline static sdRID getFstExt( sdpsfSegHdr *aSegHdr );
    inline static sdRID getLstExt( sdpsfSegHdr *aSegHdr );

    inline static IDE_RC fixAndGetExtDesc4Update(
                                    idvSQL          * aStatistics,
                                    sdrMtx          * aMtx,
                                    scSpaceID         aSpaceID,
                                    sdRID             aRID,
                                    sdpsfExtDesc  **  aExtDesc );

private:
    inline static idBool isFreePIDInExt( sdpsfSegHdr *aSegHdr,
                                         scPageID     aFstPIDOfAllocExt,
                                         scPageID     aLstAllocPageID );
};

/* Segment의 Extent갯수를 리턴한다. */
inline ULong sdpsfExtMgr::getExtCnt( sdpsfSegHdr *aSegHdr )
{
    return aSegHdr->mTotExtCnt;
}

/* Extent List의 첫번째 Extent를 리턴한다. */
inline sdRID sdpsfExtMgr::getFstExt( sdpsfSegHdr *aSegHdr )
{
    return sdpsfExtDirPage::getFstExtRID( &aSegHdr->mExtDirCntlHdr );
}

inline sdRID sdpsfExtMgr::getLstExt( sdpsfSegHdr *aSegHdr )
{
    return aSegHdr->mAllocExtRID;
}

/* aRID가 가리키는 ExtDesc를 구한다. */
inline IDE_RC sdpsfExtMgr::fixAndGetExtDesc4Update(
    idvSQL          * aStatistics,
    sdrMtx          * aMtx,
    scSpaceID         aSpaceID,
    sdRID             aRID,
    sdpsfExtDesc   ** aExtDesc )
{
    IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                          aSpaceID,
                                          aRID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          aMtx,
                                          (UChar**)aExtDesc )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExtDesc = NULL;

    return IDE_FAILURE;
}

/* aPageID가 속한 Extent에 aPageID이후로 Free Page가 있는지 조사한다. */
inline idBool sdpsfExtMgr::isFreePIDInExt( sdpsfSegHdr *aSegHdr,
                                           scPageID     aFstPIDOfAllocExt,
                                           scPageID     aLstAllocPageID )
{
    UInt sAllocPageCntInExt = aLstAllocPageID - aFstPIDOfAllocExt + 1;

    if( sAllocPageCntInExt >= aSegHdr->mPageCntInExt )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

#endif // _O_SDPSF_EXTMGR_H_


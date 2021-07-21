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
 *
 * Description :
 *
 * 본 파일은 extent directory page에 대한 구현 파일이다.
 *
 **********************************************************************/

#include <sdr.h>
#include <sdpDef.h>
#include <sdpsfExtDirPage.h>
#include <sdpsfExtent.h>
#include <sdpsfSH.h>
//XXX sdptb를 바로 참조하면 안됩니다.
#include <sdptbExtent.h>
#include <smErrorCode.h>

/***********************************************************************
 * Description: ExtDirPage를 초기화한다.
 *
 * aMtx           - [IN] Min Transaction Pointer
 * aExtDirCntlHdr - [IN] ExtDirCntlHdr
 * aMaxExtDescCnt - [IN] 최대 Extent 갯수
 **********************************************************************/
IDE_RC sdpsfExtDirPage::initialize( sdrMtx*              aMtx,
                                    sdpsfExtDirCntlHdr*  aExtDirCntlHdr,
                                    UShort               aMaxExtDescCnt )
{
    scOffset sFstExtDescOffset;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );
    IDE_ASSERT( aMaxExtDescCnt != 0 );

    IDE_TEST( setExtDescCnt( aMtx,
                             aExtDirCntlHdr,
                             0 )
              != IDE_SUCCESS );

    IDE_TEST( setMaxExtDescCnt( aMtx,
                                aExtDirCntlHdr,
                                aMaxExtDescCnt ) != IDE_SUCCESS );

    sFstExtDescOffset = sdpPhyPage::getOffsetFromPagePtr( (UChar*)aExtDirCntlHdr ) +
        ID_SIZEOF( sdpsfExtDirCntlHdr ) ;

    IDE_TEST( setFstExtDescOffset( aMtx,
                                   aExtDirCntlHdr,
                                   sFstExtDescOffset )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 **********************************************************************/
IDE_RC sdpsfExtDirPage::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : ExtDirPage를 생성한다. 생성된 ExtDirPage의 Pointer와
 *               Header를 구해준다.
 *
 * aStatistics    - [IN] 통계정보
 * aMtx           - [IN] Mini Transaction Pointer
 * aSpaceID       - [IN] TableSpace ID
 * aPageID        - [IN] ExtDirPage ID
 *
 * aPagePtr       - [OUT] Create된 ExtDirPage Pointer가 설정된다.
 * aRetExtDirHdr  - [OUT] ExtDirPage의 ExtDirCntlHdr가 설정된다.
 **********************************************************************/
IDE_RC sdpsfExtDirPage::create( idvSQL              * aStatistics,
                                sdrMtx              * aMtx,
                                scSpaceID             aSpaceID,
                                scPageID              aPageID,
                                UChar              ** aPagePtr,
                                sdpsfExtDirCntlHdr ** aRetExtDirHdr )
{
    sdpsfExtDirCntlHdr * sExtDirHdr;
    UChar              * sPagePtr;

    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aSpaceID      != 0 );
    IDE_ASSERT( aPageID       != SD_NULL_PID );
    IDE_ASSERT( aPagePtr      != NULL );
    IDE_ASSERT( aRetExtDirHdr != NULL );

    IDE_TEST( sdpPhyPage::create( aStatistics,
                                  aSpaceID,
                                  aPageID,
                                  NULL,    /* Parent Info */
                                  0,       /* Page State */
                                  SDP_PAGE_FMS_EXTDIR,
                                  SM_NULL_OID, // SegMeta
                                  SM_NULL_INDEX_ID,
                                  aMtx,    /* Create Mtx */
                                  aMtx,    /* Page Init Mtx */
                                  &sPagePtr )
              != IDE_SUCCESS );

    sExtDirHdr = sdpsfExtDirPage::getExtDirCntlHdr( sPagePtr );

    IDE_TEST( initialize( aMtx,
                          sExtDirHdr,
                          getMaxExtDescCntInExtDirPage() )
              != IDE_SUCCESS );

    *aPagePtr      = sPagePtr;
    *aRetExtDirHdr = sExtDirHdr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr      = NULL;
    *aRetExtDirHdr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : <aSpaceID, aPageID>에 해당하는 Page에 대해서 XLatch를
 *               잡고 페이지의 포인터와 ExtDirCntlHeader를 리턴한다.
 *
 * aStatistics    - [IN] 통계정보
 * aMtx           - [IN] Mini Transaction Pointer
 * aSpaceID       - [IN] TableSpace ID
 * aPageID        - [IN] ExtDirPage ID
 *
 * aPagePtr       - [OUT] ExtDirPage Pointer가 설정된다.
 * aRetExtDirHdr  - [OUT] ExtDirPage의 ExtDirCntlHdr가 설정된다.
 **********************************************************************/
IDE_RC sdpsfExtDirPage::getPage4Update( idvSQL              * aStatistics,
                                        sdrMtx              * aMtx,
                                        scSpaceID             aSpaceID,
                                        scPageID              aPageID,
                                        UChar              ** aPagePtr,
                                        sdpsfExtDirCntlHdr ** aRetExtDirHdr )
{
    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aSpaceID      != 0 );
    IDE_ASSERT( aPageID       != SD_NULL_PID );
    IDE_ASSERT( aPagePtr      != NULL );
    IDE_ASSERT( aRetExtDirHdr != NULL );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          aSpaceID,
                                          aPageID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          aPagePtr,
                                          NULL /*aTrySuccess*/,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );

    *aRetExtDirHdr = sdpsfExtDirPage::getExtDirCntlHdr( *aPagePtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr      = NULL;
    *aRetExtDirHdr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : <aSpaceID, aPageID>에 해당하는 Page를 Buffer에 Fix한다.
 *
 * aStatistics    - [IN] 통계정보
 * aSpaceID       - [IN] TableSpace ID
 * aPageID        - [IN] ExtDirPage ID
 *
 * aPagePtr       - [OUT] ExtDirPage Pointer가 설정된다.
 * aRetExtDirHdr  - [OUT] ExtDirPage의 ExtDirCntlHdr가 설정된다.
 **********************************************************************/
IDE_RC sdpsfExtDirPage::fixPage( idvSQL              * aStatistics,
                                 scSpaceID             aSpaceID,
                                 scPageID              aPageID,
                                 UChar              ** aPagePtr,
                                 sdpsfExtDirCntlHdr ** aRetExtDirHdr )
{
    IDE_ASSERT( aSpaceID      != 0 );
    IDE_ASSERT( aPageID       != SD_NULL_PID );
    IDE_ASSERT( aPagePtr      != NULL );
    IDE_ASSERT( aRetExtDirHdr != NULL );

    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aPageID,
                                          aPagePtr )
              != IDE_SUCCESS );

    *aRetExtDirHdr = sdpsfExtDirPage::getExtDirCntlHdr( *aPagePtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr      = NULL;
    *aRetExtDirHdr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 새로운 ExtDesc를 이 aExtDirHdr의 끝에 추가한다. 만약 공간
 *               이 없다면 aNewExtDesc를 NULL로 설정하고 있다면 추가한
 *               ExtDesc의 시작주소를 넘겨준다.
 *
 * aMtx           - [IN] Mini Transaction Pointer
 * aExtDirHdr     - [IN] ExtDirPage Header
 * aFstPID        - [IN] 새로운 Extent의 첫번째 페이지
 * aFlag          - [IN] ExtDesc의 Flag 정보.
 *
 * aNewExtDesc    - [OUT] If 공간이 없다면 NULL, else 새로 Add된 Extent Desc
 *                        의 시작 주소.
 **********************************************************************/
IDE_RC sdpsfExtDirPage::addNewExtDescAtLst( sdrMtx             * aMtx,
                                            sdpsfExtDirCntlHdr * aExtDirHdr,
                                            scPageID             aFstPID,
                                            UInt                 aFlag,
                                            sdpsfExtDesc      ** aNewExtDesc )
{
    UShort        sExtCnt;
    sdpsfExtDesc *sNewExtDesc;

    IDE_ASSERT( aMtx          != NULL );
    IDE_ASSERT( aExtDirHdr    != NULL );
    IDE_ASSERT( aFstPID       != SD_NULL_PID );
    IDE_ASSERT( aNewExtDesc   != NULL );

    if( isFull( aExtDirHdr ) == ID_FALSE )
    {
        sExtCnt = aExtDirHdr->mExtDescCnt;

        /* 새로운 Extent가 추가되었기때문에 ExtDesc갯수를 1증가
         * 시켜준다. */
        IDE_TEST( setExtDescCnt( aMtx,
                                 aExtDirHdr,
                                 sExtCnt + 1 )
                  != IDE_SUCCESS );

        sNewExtDesc  = getLstExtDesc( aExtDirHdr );

        /* 새로운 Extent의 시작 페이지를 설정한다. */
        IDE_TEST( sdpsfExtent::setFirstPID( sNewExtDesc,
                                            aFstPID,
                                            aMtx )
                  != IDE_SUCCESS );

        IDE_TEST( sdpsfExtent::setFlag( sNewExtDesc,
                                        aFlag,
                                        aMtx )
                  != IDE_SUCCESS );

        *aNewExtDesc = sNewExtDesc;
    }
    else
    {
        /* 공간이 없다. */
        *aNewExtDesc = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aNewExtDesc = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Segment에서 aCurExtRID 다음에 존재하는 Extent RID와
 *               ExtDesc를 구한다.
 *
 * aExtDirHdr     - [IN] Extent Directory Page Pointer
 * aCurExtRID     - [IN] 현재 Extent RID
 * aNxtExtRID     - [IN] 다음 Extent RID
 * aExtDesc       - [OUT] 다음 ExtDesc 가 복사될 버퍼
 **********************************************************************/
IDE_RC sdpsfExtDirPage::getNxtExt( sdpsfExtDirCntlHdr * aExtDirHdr,
                                   sdRID                aCurExtRID,
                                   sdRID              * aNxtExtRID,
                                   sdpsfExtDesc       * aExtDesc )
{
    sdRID sLstExtRID;

    IDE_ASSERT( aExtDirHdr  != NULL );
    IDE_ASSERT( aCurExtRID  != SD_NULL_RID );
    IDE_ASSERT( aNxtExtRID  != NULL );
    IDE_ASSERT( aExtDesc    != NULL );

    sLstExtRID = getLstExtRID( aExtDirHdr );

    IDE_ASSERT( aCurExtRID <= sLstExtRID );

    if( sLstExtRID == aCurExtRID )
    {
        sdpsfExtent::init( aExtDesc );

        *aNxtExtRID = SD_NULL_RID;
    }
    else
    {
        *aNxtExtRID = aCurExtRID + ID_SIZEOF( sdpsfExtDesc );
        *aExtDesc   = *getExtDesc( aExtDirHdr, *aNxtExtRID );
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : aExtDirPID가 가리키는 페이지에서 OUT인자의 정보를 가져
 *               온다. aExtDirPID가 가리키는 페이지는 Extent Directory
 *               Page이어야 한다.
 *
 * aStatistics    - [IN] 통계정보
 * aSpaceID       - [IN] TableSpace ID
 * aExtDirPID     - [IN] Extent Directory PageID
 *
 * aExtCnt        - [OUT] aExtDirPID가 가리키는 페이지에 존재하는 Extent
 *                        갯수
 * aFstExtRID     - [OUT] 첫번째 Extent RID
 * aFstExtDescPtr - [OUT] 첫번재 Extent Desc가 복사될 버퍼
 * aLstExtRID     - [OUT] Extent Directory Page내의 마지막 Extent RID
 * aNxtExtDirPID  - [OUT] aExtDirPID의 다음 ExtDirPageID
 **********************************************************************/
IDE_RC sdpsfExtDirPage::getPageInfo( idvSQL        * aStatistics,
                                     scSpaceID       aSpaceID,
                                     scPageID        aExtDirPID,
                                     UShort        * aExtCnt,
                                     sdRID         * aFstExtRID,
                                     sdpsfExtDesc  * aFstExtDescPtr,
                                     sdRID         * aLstExtRID,
                                     scPageID      * aNxtExtDirPID )
{
    sdpsfExtDesc       * sFstExtDescPtr;
    UChar              * sExtDirPagePtr;
    sdpsfExtDirCntlHdr * sExtDirCntlHdr;
    sdpPhyPageHdr      * sPhyHdrOfExtPage;
    SInt                 sState = 0;

    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aExtDirPID     != SD_NULL_PID );
    IDE_ASSERT( aExtCnt        != NULL );
    IDE_ASSERT( aFstExtRID     != NULL );
    IDE_ASSERT( aFstExtDescPtr != NULL );
    IDE_ASSERT( aLstExtRID     != NULL );
    IDE_ASSERT( aNxtExtDirPID  != NULL );

    IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                          aSpaceID,
                                          aExtDirPID,
                                          (UChar**)&sExtDirPagePtr )
              != IDE_SUCCESS );

    IDE_ASSERT( sExtDirPagePtr != NULL );

    sPhyHdrOfExtPage = sdpPhyPage::getHdr( sExtDirPagePtr );

    IDE_ASSERT( sdpPhyPage::getPageType( sPhyHdrOfExtPage )
                == SDP_PAGE_FMS_EXTDIR );

    sExtDirCntlHdr   = getExtDirCntlHdr( sExtDirPagePtr );
    sFstExtDescPtr   = getFstExtDesc( sExtDirCntlHdr );

    if( sExtDirCntlHdr->mExtDescCnt != 0 )
    {
        *aFstExtRID = SD_MAKE_RID( aExtDirPID, getFstExtOffset() );
        *aLstExtRID = SD_MAKE_RID( aExtDirPID, getFstExtOffset() +
                                   ( sExtDirCntlHdr->mExtDescCnt - 1 ) * ID_SIZEOF( sdpsfExtDesc ) );
    }
    else
    {
        *aFstExtRID = SD_NULL_RID;
        *aLstExtRID = SD_NULL_RID;
    }

    *aFstExtDescPtr = *sFstExtDescPtr;
    *aNxtExtDirPID  = sdpPhyPage::getNxtPIDOfDblList( sPhyHdrOfExtPage );
    *aExtCnt        = sExtDirCntlHdr->mExtDescCnt;

    sState = 0;
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                       sExtDirPagePtr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sExtDirPagePtr )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aExtDirCntlHdr이 가리키는 페이지의 끝에서 ExtDesc를 제거
 *               한다.
 *
 * aStatistics    - [IN] 통계 정보
 * aMtx           - [IN] Mini Transaction Pointer
 * aSpaceID       - [IN] TableSpace ID
 * aSegHdr        - [IN] Segment Header
 * aExtDirHdr     - [IN] Extent Direct Control Header
 **********************************************************************/
IDE_RC sdpsfExtDirPage::freeLstExt( idvSQL             * aStatistics,
                                    sdrMtx             * aMtx,
                                    scSpaceID            aSpaceID,
                                    sdpsfSegHdr        * aSegHdr,
                                    sdpsfExtDirCntlHdr * aExtDirCntlHdr )
{
    sdpsfExtDesc     * sLstExtDesc;
    UInt               sFreeExtCnt;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );

    sLstExtDesc = getLstExtDesc( aExtDirCntlHdr );

    IDE_ASSERT( sLstExtDesc->mFstPID != 0 );

    IDE_TEST( sdptbExtent::freeExt( aStatistics,
                                    aMtx,
                                    aSpaceID,
                                    sLstExtDesc->mFstPID,
                                    &sFreeExtCnt )
              != IDE_SUCCESS );

    IDE_TEST( setExtDescCnt( aMtx,
                             aExtDirCntlHdr,
                             aExtDirCntlHdr->mExtDescCnt - 1 )
              != IDE_SUCCESS );

    IDE_TEST( sdpsfSH::setTotExtCnt( aMtx,
                                     aSegHdr,
                                     aSegHdr->mTotExtCnt - 1 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aExtDirCntlHdr가 가리키는 페이지에서 첫번째 Extent
 *               제외하고 모든 Extent를 free한다.
 *
 * aStatistics    - [IN] 통계 정보
 * aStartInfo     - [IN] Mini Transaction Start Info
 * aSpaceID       - [IN] TableSpace ID
 * aSegHdr        - [IN] Segment Header Pointer
 * aExtDirCntlHdr - [IN] Extent Direct Control Header
 **********************************************************************/
IDE_RC sdpsfExtDirPage::freeAllExtExceptFst( idvSQL             * aStatistics,
                                             sdrMtxStartInfo    * aStartInfo,
                                             scSpaceID            aSpaceID,
                                             sdpsfSegHdr        * aSegHdr,
                                             sdpsfExtDirCntlHdr * aExtDirCntlHdr )
{
    UShort             sExtDescCnt;
    sdrMtx             sFreeMtx;
    SInt               sState = 0;
    UChar            * sExtDirPagePtr;
    UChar            * sSegHdrPagePtr;

    IDE_ASSERT( aStartInfo     != NULL );
    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );

    sExtDirPagePtr = sdpPhyPage::getPageStartPtr( aExtDirCntlHdr );
    sSegHdrPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );

    /* 첫번째 ExtDesc는 ExtDirPage가 있는 Extent이기때문에 가장 나중에 Free해야 한다.
     * 하여 Extent Dir Page의 Extent를 역순으로 Free한다. */
    sExtDescCnt    = aExtDirCntlHdr->mExtDescCnt;

    while( sExtDescCnt > 1 )
    {
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sFreeMtx,
                                       aStartInfo,
                                       ID_FALSE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        sExtDescCnt--;

        IDE_TEST( freeLstExt( aStatistics,
                              &sFreeMtx,
                              aSpaceID,
                              aSegHdr,
                              aExtDirCntlHdr ) != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::setDirtyPage( &sFreeMtx,
                                              sExtDirPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::setDirtyPage( &sFreeMtx,
                                              sSegHdrPagePtr )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sFreeMtx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sFreeMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aExtRID가 가리키는 Extent이후의 모든 Extent를 free한다.
 *
 * aStatistics    - [IN] 통계 정보
 * aStartInfo     - [IN] Mini Transaction Start Info
 * aSpaceID       - [IN] TableSpace ID
 * aSegHdr        - [IN] Segment Header
 * aExtDirHdr     - [IN] Extent Direct Control Header
 * aExtRID        - [IN] Extent RID
 **********************************************************************/
IDE_RC sdpsfExtDirPage::freeAllNxtExt( idvSQL             * aStatistics,
                                       sdrMtxStartInfo    * aStartInfo,
                                       scSpaceID            aSpaceID,
                                       sdpsfSegHdr        * aSegHdr,
                                       sdpsfExtDirCntlHdr * aExtDirCntlHdr,
                                       sdRID                aExtRID )
{
    UShort             sExtDescCnt;
    sdrMtx             sFreeMtx;
    SInt               sState = 0;
    UChar            * sExtDirPagePtr;
    UChar            * sSegHdrPagePtr;
    sdRID              sLstExtRID;

    IDE_ASSERT( aStartInfo     != NULL );
    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );
    IDE_ASSERT( aExtRID        != SD_NULL_RID );

    sExtDirPagePtr = sdpPhyPage::getPageStartPtr( aExtDirCntlHdr );
    sSegHdrPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );

    /* 첫번째 ExtDesc는 ExtDirPage가 있는 Extent이기때문에 가장 나중에 Free해야 한다.
     * 하여 Extent Dir Page의 Extent를 역순으로 Free한다. */
    sExtDescCnt = aExtDirCntlHdr->mExtDescCnt;

    while( sExtDescCnt > 0 )
    {
        sLstExtRID = sdpsfExtDirPage::getLstExtRID( aExtDirCntlHdr );

        if( sLstExtRID == aExtRID )
        {
            break;
        }

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sFreeMtx,
                                       aStartInfo,
                                       ID_FALSE,/*Undoable(PROJ-2162)*/
                                       SM_DLOG_ATTR_DEFAULT )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_TEST( freeLstExt( aStatistics,
                              &sFreeMtx,
                              aSpaceID,
                              aSegHdr,
                              aExtDirCntlHdr ) != IDE_SUCCESS );

        sExtDescCnt--;

        IDE_TEST( sdrMiniTrans::setDirtyPage( &sFreeMtx,
                                              sExtDirPagePtr )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::setDirtyPage( &sFreeMtx,
                                              sSegHdrPagePtr )
                  != IDE_SUCCESS );

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sFreeMtx ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sFreeMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Debug를 위해서 Console에 ExtentDesc정보를 출력한다.
 *
 * aExtDirCntlHdr- [IN] Extent Direct Control Header
 **********************************************************************/
IDE_RC sdpsfExtDirPage::dump( sdpsfExtDirCntlHdr * aExtDirCntlHdr )
{
    UInt          i;
    sdpsfExtDesc *sExtDesc;
    UInt          sExtCnt;

    IDE_ERROR( aExtDirCntlHdr != NULL );

    sExtCnt = aExtDirCntlHdr->mExtDescCnt;

    for( i = 0; i < sExtCnt; i++ )
    {
        sExtDesc = getNthExtDesc( aExtDirCntlHdr, i );
        ideLog::log( IDE_SERVER_0, 
                     "%uth,"
                     "FstPID: %u,"
                     "Flag:%u",
                     i,
                     sExtDesc->mFstPID,
                     sExtDesc->mFlag );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

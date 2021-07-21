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
 * $Id: sdpsfFreePIDList.cpp 25804 2008-04-29 01:23:17Z bskim $
 *
 * 본 파일은 Segment에서 가용공간 탐색 연산의 STATIC 인터페이스들을 관리한다.
 *
 **********************************************************************/

# include <sdr.h>
# include <smErrorCode.h>
# include <sdpReq.h>
# include <sdpTableSpace.h>

# include <sdpsfDef.h>
# include <sdpsfSH.h>
# include <sdpsfVerifyAndDump.h>
# include <sdpsfFreePIDList.h>
# include <sdpsfFindPage.h>

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC sdpsfFreePIDList::initialize()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC sdpsfFreePIDList::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Free Page List에서 aRecordSize가 들어갈수 페이지에
 *               XLatch를 건다. walk, unlink을 주어진 갯수이상으로 하게
 *               되면 포기한다.
 *
 * Caution:
 *  1. aSegHdr에 XLatch가 걸려 있어야 한다.
 *
 * aStatistics      - [IN] 통계 정보
 * aMtx             - [IN] Mini Transaction Pointer
 * aSpaceID         - [IN] SpaceID
 * aSegHdr          - [IN] Segment Header
 * aRecordSize      - [IN] Record Size
 * aNeedKeySlot     - [IN] KeySlot이 필요하면 ID_TRUE, else ID_FALSE.
 *
 * aPagePtr         - [OUT] Free Page Ptr
 *
 ***********************************************************************/
IDE_RC sdpsfFreePIDList::walkAndUnlink( idvSQL               *aStatistics,
                                        sdrMtx               *aMtx,
                                        scSpaceID             aSpaceID,
                                        sdpsfSegHdr          *aSegHdr,
                                        UInt                  aRecordSize,
                                        idBool                aNeedKeySlot,
                                        UChar               **aPagePtr,
                                        UChar                *aCTSlotIdx )
{
    UChar             *sPagePtr;
    UChar             *sPrvPagePtr;
    /* BUGBUG: Property 정해야 함.
     * smuProperty::getWalkCntInDiskTblFreePIDList() */
    UInt               sWalkCnt = 5;
    scPageID           sNxtPageID;
    idBool             sCurPageLatched;
    idBool             sPrvPageLatched;
    scPageID           sPageID;
    UInt               i;
    SInt               sState = 0;
    sdpPhyPageHdr     *sPhyPageHdr;
    idBool             sRemFrmList;
    idBool             sCanAllocSlot;
    sdrMtxStartInfo    sStartInfo;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSpaceID     != 0 );
    IDE_ASSERT( aSegHdr      != NULL );
    IDE_ASSERT( aRecordSize  < SD_PAGE_SIZE );
    IDE_ASSERT( aRecordSize  > 0 );
    IDE_ASSERT( aPagePtr     != NULL );

    *aPagePtr = NULL;

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    sPageID = sdpSglPIDList::getHeadOfList( &(aSegHdr->mFreePIDList) );

    sPrvPagePtr     = NULL;
    sPrvPageLatched = ID_FALSE;

    /* BUGBUG : 몇개의 페이지까지 Walking할 것인가 ? */
    for( i = 0; i < (sWalkCnt) && (sPageID != SD_NULL_PID); i++ )
    {
        /* Page에 대해서 Fix시킨다. */
        IDE_TEST( sdbBufferMgr::fixPageByPID( aStatistics,
                                              aSpaceID,
                                              sPageID,
                                              &sPagePtr )
                  != IDE_SUCCESS );
        sState = 1;

        sPhyPageHdr = sdpPhyPage::getHdr( sPagePtr );

        IDE_ASSERT( sPhyPageHdr->mLinkState ==  SDP_PAGE_LIST_LINK );

        sNxtPageID  = sdpPhyPage::getNxtPIDOfSglList( sPhyPageHdr );

        sdbBufferMgr::latchPage( aStatistics,
                                 sPagePtr,
                                 SDB_X_LATCH,
                                 SDB_WAIT_NO,
                                 &sCurPageLatched  );

        sRemFrmList = ID_FALSE;

        if( sCurPageLatched == ID_TRUE )
        {
            sState = 2;

            IDE_TEST( sdpsfFindPage::checkSizeAndAllocCTS(
                                     aStatistics,
                                     &sStartInfo,
                                     sPagePtr,
                                     aRecordSize,
                                     aNeedKeySlot,
                                     &sCanAllocSlot,
                                     &sRemFrmList,
                                     aCTSlotIdx ) != IDE_SUCCESS );

            if ( sCanAllocSlot == ID_TRUE )
            {
                IDE_TEST( sdrMiniTrans::pushPage( aMtx,
                                                  sPagePtr,
                                                  SDB_X_LATCH )
                          != IDE_SUCCESS );

                *aPagePtr = sPagePtr;
                break;
            }

            /* 이전 Page에 대해서 Latch가 잡혔을 경우만 Unlink를 시도한다. */
            if( sRemFrmList == ID_TRUE )
            {
                if ( (sPrvPageLatched == ID_TRUE) || (i == 0) )
                {
                    IDE_TEST( removePage( aStatistics,
                                          aMtx,
                                          aSegHdr,
                                          sPrvPagePtr,
                                          sPagePtr )
                              != IDE_SUCCESS );

                    sdbBufferMgr::unlatchPage( sPagePtr );
                    sCurPageLatched = ID_FALSE;
                }
            }
        }

        if( sPrvPageLatched == ID_TRUE )
        {
            sdbBufferMgr::unlatchPage( sPrvPagePtr );
            sPrvPageLatched = ID_FALSE;
        }

        if( sPrvPagePtr != NULL )
        {
            IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                               sPrvPagePtr )
                      != IDE_SUCCESS );
            sPrvPagePtr = NULL;
        }

        sState = 0;

        sPrvPageLatched = sCurPageLatched;
        sPrvPagePtr     = sPagePtr;
        sPageID         = sNxtPageID;
    }

    if( sPrvPageLatched == ID_TRUE )
    {
        sdbBufferMgr::unlatchPage( sPrvPagePtr );
        sPrvPageLatched = ID_FALSE;
    }

    if( sPrvPagePtr != NULL )
    {
        IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                           sPrvPagePtr )
                  != IDE_SUCCESS );
        sPrvPagePtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sPrvPageLatched == ID_TRUE )
    {
        sdbBufferMgr::unlatchPage( sPrvPagePtr );
        sPrvPageLatched = ID_FALSE;
    }

    if( sPrvPagePtr != NULL )
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                             sPrvPagePtr )
                    == IDE_SUCCESS );
        sPrvPagePtr = NULL;
    }

    switch( sState )
    {
        case 2:
            sdbBufferMgr::unlatchPage( sPagePtr );
        case 1:

            IDE_ASSERT( sdbBufferMgr::unfixPage( aStatistics,
                                                 sPagePtr )
                        == IDE_SUCCESS );

        default:
            break;
    }


    *aPagePtr    = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPagePtr을 Free Page List에 앞에 붙인다.
 *
 * Caution:
 *  1. aSegHdr에 XLatch가 걸려 있어야 한다.
 *
 * aMtx      - [IN] Mini Transaction Pointer
 * aSegHdr   - [IN] Segment Header
 * aPagePtr  - [IN] Page Pointer
 *
 ***********************************************************************/
IDE_RC sdpsfFreePIDList::addPage( sdrMtx          * aMtx,
                                  sdpsfSegHdr     * aSegHdr,
                                  UChar           * aPagePtr )
{
    sdpSglPIDListBase *sFreePIDList;
    UChar             *sSegPagePtr;
    sdpPhyPageHdr     *sPageHdr;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSegHdr      != NULL );
    IDE_ASSERT( aPagePtr     != NULL );

    sSegPagePtr  = sdpPhyPage::getPageStartPtr( aSegHdr );

    sFreePIDList = sdpsfSH::getFreePIDList( aSegHdr );

    sPageHdr = (sdpPhyPageHdr*)aPagePtr;

    IDE_TEST( sdrMiniTrans::setDirtyPage( aMtx,
                                          sSegPagePtr )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::setDirtyPage( aMtx,
                                          aPagePtr )
              != IDE_SUCCESS );

    /* 페이지를 Free Page List앞에 추가한다. */
    IDE_TEST( sdpSglPIDList::addNode2Head(
                  sFreePIDList,
                  sdpPhyPage::getSglPIDListNode( sPageHdr ),
                  aMtx ) != IDE_SUCCESS );

    IDE_ASSERT( sPageHdr->mLinkState == SDP_PAGE_LIST_UNLINK );

    IDE_TEST( sdpPhyPage::setLinkState( sPageHdr,
                                        SDP_PAGE_LIST_LINK,
                                        aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aPagePtr을 Free Page List에 앞에 붙인다.
 *
 * Caution:
 *  1. aSegHdr에 XLatch가 걸려 있어야 한다.
 *  2. aPrvPagePtr에 XLatch가 걸려 있어야 한다.
 *  3. aTgtPagePtr에 XLatch가 걸려 있어야 한다.
 *  4. 이 함수가 return되면 rollback되지 않는다.
 *
 * aStatistics      - [IN] 통계 정보
 * aMtx             - [IN] Mini Transaction Pointer
 * aSegHdr          - [IN] Segment Hdr
 * aPrvPagePtr      - [IN] 제거하려는 Page의 이전 페이지 Pointer
 * aTgtPagePtr      - [IN] 제거하려는 Page Pointer
 ***********************************************************************/
IDE_RC sdpsfFreePIDList::removePage( idvSQL             * aStatistics,
                                     sdrMtx             * aMtx,
                                     sdpsfSegHdr        * aSegHdr,
                                     UChar              * aPrvPagePtr,
                                     UChar              * aTgtPagePtr )
{
    scPageID           sRmvPageID;
    sdpSglPIDListNode *sRmvPageNode;
    sdpSglPIDListBase *sFreePIDList;
    sdrMtx             sRmvMtx;
    SInt               sState = 0;
    UChar             *sSegPagePtr;
    sdrMtxStartInfo    sStartInfo;
    sdpPhyPageHdr     *sPhyPageHdr;

    IDE_ASSERT( aMtx         != NULL );
    IDE_ASSERT( aSegHdr      != NULL );
    IDE_ASSERT( aTgtPagePtr  != NULL );

    sStartInfo.mTrans   = sdrMiniTrans::getTrans( aMtx );
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

    sSegPagePtr  = sdpPhyPage::getPageStartPtr( aSegHdr );
    sFreePIDList = sdpsfSH::getFreePIDList( aSegHdr );

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sRmvMtx,
                                   &sStartInfo,
                                   ID_FALSE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdrMiniTrans::setDirtyPage( &sRmvMtx,
                                          sSegPagePtr )
              != IDE_SUCCESS );

    if( aPrvPagePtr != NULL )
    {
        IDE_TEST( sdrMiniTrans::setDirtyPage( &sRmvMtx,
                                              aPrvPagePtr )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::setDirtyPage( &sRmvMtx,
                                          aTgtPagePtr )
              != IDE_SUCCESS );


    sPhyPageHdr = sdpPhyPage::getHdr( aTgtPagePtr );

    if( sPhyPageHdr->mPageState != SDP_PAGE_BIT_UPDATE_ONLY )
    {
        IDE_TEST( sdpPhyPage::setState( sPhyPageHdr,
                                        (UShort)SDPSF_PAGE_USED_UPDATE_ONLY,
                                        &sRmvMtx )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdpPhyPage::setLinkState( sPhyPageHdr,
                                        SDP_PAGE_LIST_UNLINK,
                                        &sRmvMtx )
              != IDE_SUCCESS );

    if( aPrvPagePtr != NULL )
    {
        IDE_TEST( sdpSglPIDList::removeNode( sFreePIDList,
                                             aPrvPagePtr,
                                             aTgtPagePtr,
                                             &sRmvMtx,
                                             &sRmvPageID,
                                             &sRmvPageNode )
                  != IDE_SUCCESS );
    }
    else
    {
        /* PrevPage가 Null인 것은 제거하려는 페이지가
         * Header페이지이다. */
        IDE_TEST( sdpSglPIDList::removeNodeAtHead( sFreePIDList,
                                                   aTgtPagePtr,
                                                   &sRmvMtx,
                                                   &sRmvPageID,
                                                   &sRmvPageNode )
                  != IDE_SUCCESS );

    }

    IDE_ASSERT( sdpPhyPage::getPageID( aTgtPagePtr ) == sRmvPageID );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sRmvMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sRmvMtx )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}



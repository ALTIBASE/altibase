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
 **********************************************************************/

# include <smErrorCode.h>
# include <sdr.h>
# include <sdpReq.h>

# include <sdpsfDef.h>
# include <sdpsfSH.h>
# include <sdpsfVerifyAndDump.h>
# include <sdpSglPIDList.h>
# include <sdpTableSpace.h>
# include <sdpsfUFmtPIDList.h>

IDE_RC sdpsfUFmtPIDList::initialize()
{
    return IDE_SUCCESS;
}

IDE_RC sdpsfUFmtPIDList::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : aPagePtr을 UnFormat PID List의 Head에 추가한다.
 *
 * Caution:
 *  1. aSegHdr에 XLatch가 걸려 있어야 한다.
 *  2. aPagePtr에 XLatch가 걸려 있어야 한다.
 *
 * aSegHdr    - [IN] Segment Header
 * aPagePtr   - [IN] Page Ptr
 * aMtx       - [IN] Mini Transaction Pointer
 *
 ***********************************************************************/
IDE_RC sdpsfUFmtPIDList::add2Head( sdrMtx             * aMtx,
                                   sdpsfSegHdr        * aSegHdr,
                                   UChar              * aPagePtr )
{
    sdpSglPIDListBase *sUFmtPIDList;
    sdpPhyPageHdr     *sPageHdr;

    IDE_ASSERT( aMtx     != NULL );
    IDE_ASSERT( aSegHdr  != NULL );
    IDE_ASSERT( aPagePtr != NULL );

    sUFmtPIDList = sdpsfSH::getUFmtPIDList( aSegHdr );

    sPageHdr = sdpPhyPage::getHdr( aPagePtr );

    IDE_TEST( sdpPhyPage::setLinkState( sPageHdr,
                                        SDP_PAGE_LIST_LINK,
                                        aMtx )
              != IDE_SUCCESS );

    IDE_TEST( sdpSglPIDList::addNode2Head( sUFmtPIDList,
                                           sdpPhyPage::getSglPIDListNode( sPageHdr ),
                                           aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 * Description : UnFormat PID List의 Head에서 Page하나를 리스트에서
 *               제거한다.
 *
 * Caution:
 *  1. aSegHdr에 XLatch가 걸려 있어야 한다.
 *  2. 리턴될때 aPagePtr에 XLatch가 걸려있다.
 *
 * aStatistics    - [IN] 통계정보
 * aRmvMtx        - [IN] Head페이지를 PageList에서 제거할 Mini Transaction Pointer
 * aMtx           - [IN] Head페이지를 버퍼에 Fix하고 Latch를 획득하는
 *                       Mini Transaction Pointer
 * aSpaceID       - [IN] SpaceID
 * aSegHdr        - [IN] Segment Hdr
 * aPageType      - [IN] Page Type
 *
 * aAllocPID      - [OUT] Head Page ID
 * aPagePtr       - [OUT] Head Page Pointer로서 리턴될때 XLatch가 걸려있다
 *
 ***********************************************************************/
IDE_RC sdpsfUFmtPIDList::removeAtHead( idvSQL             * aStatistics,
                                       sdrMtx             * aRmvMtx,
                                       sdrMtx             * aMtx,
                                       scSpaceID            aSpaceID,
                                       sdpsfSegHdr        * aSegHdr,
                                       sdpPageType          aPageType,
                                       scPageID           * aAllocPID,
                                       UChar             ** aPagePtr )
{
    scPageID           sRmvPageID;
    sdpSglPIDListNode *sRmvPageNode;
    sdpSglPIDListBase *sUFmtPIDList;
    UChar             *sSegPagePtr;
    sdpPhyPageHdr     *sPageHdr;

    IDE_ASSERT( aRmvMtx    != NULL );
    IDE_ASSERT( aMtx       != NULL );
    IDE_ASSERT( aSpaceID   != 0 );
    IDE_ASSERT( aSegHdr    != NULL );
    IDE_ASSERT( aPageType  <  SDP_PAGE_TYPE_MAX );
    IDE_ASSERT( aAllocPID  != NULL );
    IDE_ASSERT( aPagePtr   != NULL );

    *aPagePtr  = NULL;
    *aAllocPID = SD_NULL_PID;

    /* 리스트가 비어 있는지 조사한다. */
    if( sdpsfUFmtPIDList::getPageCnt( aSegHdr ) != 0 )
    {
        sRmvPageID = getFstPageID( aSegHdr );

        /* SegHdr에서 Unformat Page List얻는다. */
        sUFmtPIDList = sdpsfSH::getUFmtPIDList( aSegHdr );

        /* 리스트에서 제거연산은 sRmvMtx가 하지만 제거된 페이지
         * 에 대한 create연산은 상위 함수에서 넘어온 aMtx가 하게
         * 하여 상위 함수에서 해당 페이지에 대해서 별도의 get페이지가
         * 발생하지 않도록 한다. */
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sRmvPageID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              aPagePtr,
                                              NULL /*aTrySuccess*/,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );

        IDE_TEST( sdpSglPIDList::removeNodeAtHead( sUFmtPIDList,
                                                   *aPagePtr,
                                                   aRmvMtx,
                                                   &sRmvPageID,
                                                   &sRmvPageNode )
                  != IDE_SUCCESS );

        sPageHdr = (sdpPhyPageHdr*)*aPagePtr;

        /* sdpPhyPage::initialize에서 Page List Node를 초기화 하기때문에
         * 먼저 PIDList에서 제거후에 Physical Page를 초기화 해야한다. */
        IDE_TEST( sdpPhyPage::logAndInit( sPageHdr,
                                          sRmvPageID,
                                          NULL, /* Parent Info */
                                          SDPSF_PAGE_USED_INSERTABLE,
                                          aPageType,
                                          sPageHdr->mTableOID,
                                          sPageHdr->mIndexID, /* index id */
                                          aRmvMtx )
                  != IDE_SUCCESS );

        IDE_TEST( sdpPhyPage::setLinkState( (sdpPhyPageHdr*)*aPagePtr,
                                            SDP_PAGE_LIST_UNLINK,
                                            aRmvMtx )
                  != IDE_SUCCESS );

        sSegPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );

        /* Segment Header Page를 Dirty */
        IDE_TEST( sdrMiniTrans::setDirtyPage( aRmvMtx,
                                              sSegPagePtr )
                  != IDE_SUCCESS );

        /* 리스트에서 제거된 Page를 Dirty로 등록 */
        IDE_TEST( sdrMiniTrans::setDirtyPage( aRmvMtx,
                                              *aPagePtr )
                  != IDE_SUCCESS );

        *aAllocPID = sRmvPageID;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aPagePtr  = NULL;
    *aAllocPID = SD_NULL_PID;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aSegHdr의 Unformat Page List의 첫번째 PageID를 구한다
 *
 * aSegHdr        - [IN] Segment Hdr
 ***********************************************************************/
scPageID sdpsfUFmtPIDList::getFstPageID( sdpsfSegHdr  * aSegHdr )
{
    sdpSglPIDListBase *sUFmtPIDList;

    IDE_ASSERT( aSegHdr != NULL );

    sUFmtPIDList = sdpsfSH::getUFmtPIDList( aSegHdr );

    return sdpSglPIDList::getHeadOfList( sUFmtPIDList );
}


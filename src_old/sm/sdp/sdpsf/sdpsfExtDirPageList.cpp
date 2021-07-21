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
#include <iduLatch.h>
#include <sdd.h>
#include <sdb.h>
#include <sdr.h>
#include <smu.h>
#include <smr.h>
#include <sct.h>
#include <sdpDef.h>
#include <sdpSglRIDList.h>
#include <sdpPhyPage.h>
#include <sdpReq.h>
#include <smErrorCode.h>
#include <sdpsfSH.h>
#include <sdptbExtent.h>
#include <sdpsfExtMgr.h>

#include <sdpsfExtDirPageList.h>

/***********************************************************************
 * Description : Segment의 ExtDIRPage PID List를 초기화 한다.
 *
 * aMtx    - [IN] Mini Transaction Pointer
 * aSegHdr - [IN] Segment Header
 *
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::initialize( sdrMtx      * aMtx,
                                        sdpsfSegHdr * aSegHdr )
{
    IDE_ASSERT( aMtx    != NULL );
    IDE_ASSERT( aSegHdr != 0 );

    IDE_TEST( sdpDblPIDList::initBaseNode( &aSegHdr->mExtDirPIDList,
                                           aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : 새로운 ExtDirPage를 할당한다.
 *
 *  1. 새로운 ExtDirPage를 위해서 Extent할당이 필요한지 검사한다.
 *     할당이 필요하면 할당한다.
 *
 *    - ExtDirPage를 할당시 Extent단위로 할당한다. 때문에 현재
 *     마지막 ExtDirPage가 속한 Extent에 Free Page가 없는지 조사한다.
 *     없다면 새로운 Extent를 할당해야 한다.
 *
 *  2. 새로운 ExtDirPage의 PageNo인 ExtDirPID를 결정한다.
 *    - 새로운 Extent가 할당되었다면 새 Extent의 첫번째 PID
 *    - 기존 Extent에 공간이 있었다면 마지막에 사용한  ExtDirPageID + 1
 *
 *  3. ExtDirPID에 해당하는 페이지를 Create하고 초기화 한다.
 *
 * Caution:
 *  1. aTbsHdr가 있는 페이지에 XLatch가 걸려 있어야 한다.
 *
 * aStatistics    - [IN] 통계정보
 * aMtx           - [IN] Mini Transaction Pointer
 * aSpaceID       - [IN] TableSpace ID
 * aSegHdr        - [IN] Segment Header
 * aExtDirPagePtr - [IN] Extent Directory Page Ptr
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::addNewExtDirPage2Tail( idvSQL              * aStatistics,
                                                   sdrMtx              * aMtx,
                                                   scSpaceID             aSpaceID,
                                                   sdpsfSegHdr         * aSegHdr,
                                                   UChar               * aExtDirPagePtr )
{
    sdpsfExtDirCntlHdr  * sExtDirCntlHdr;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aExtDirPagePtr != NULL );

    sExtDirCntlHdr = sdpsfExtDirPage::getExtDirCntlHdr( aExtDirPagePtr );

    IDE_TEST( sdpsfExtDirPage::initialize(
                  aMtx,
                  sExtDirCntlHdr,
                  aSegHdr->mMaxExtCntInExtDirPage )
              != IDE_SUCCESS );

    /* 할당된 ExtDirPage를 Ext Dir PID List에 추가한다. */
    IDE_TEST( addPage2Tail( aStatistics,
                            aMtx,
                            aSegHdr,
                            sExtDirCntlHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 연속되어 있는 aAddExtDescCnt만큼 연속된 Extent들을 ExtDir
 *               Page에 추가한다.
 *
 * aStatistics        - [IN] 통계정보
 * aMtx               - [IN] Mini Transaction Pointer.
 * aSpaceID           - [IN] Table Space ID
 * aSegHdr            - [IN] Segment Header
 * aFstExtPID         - [IN] Add될 Extent들에 속해 있는 첫번째 PID
 *
 * aExtDirCntlHdr     - [OUT] Extent DirPage Control Header
 * aNewExtDescRID     - [OUT] 새로운 Extent RID
 * aNewExtDescPtr     - [OUT] 새로운 Extent Desc Pointer
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::addExtDesc( idvSQL              * aStatistics,
                                        sdrMtx              * aMtx,
                                        scSpaceID             aSpaceID,
                                        sdpsfSegHdr         * aSegHdr,
                                        scPageID              aFstExtPID,
                                        sdpsfExtDirCntlHdr ** aExtDirCntlHdr,
                                        sdRID               * aNewExtDescRID,
                                        sdpsfExtDesc       ** aNewExtDescPtr )
{
    sdpsfExtDirCntlHdr *sExtDirHdr;
    scPageID            sExtDirPID;
    UChar              *sExtDirPagePtr;
    sdpsfExtDesc       *sNewExtDescPtr;
    UInt                sFlag;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSpaceID       != 0 );
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aFstExtPID     != SD_NULL_PID );
    IDE_ASSERT( aExtDirCntlHdr != NULL );
    IDE_ASSERT( aNewExtDescRID != NULL );
    IDE_ASSERT( aNewExtDescPtr != NULL );

    /* 새로 할당된 ExtDirPage를 ExtDirPageList에 추가한다. */
    if( isNeedNewExtDirPage( aSegHdr ) == ID_TRUE )
    {
        sExtDirPID = aFstExtPID;
        sFlag      = SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_TRUE;

        IDE_TEST( sdpsfExtDirPage::create( aStatistics,
                                           aMtx,
                                           aSpaceID,
                                           sExtDirPID,
                                           &sExtDirPagePtr,
                                           &sExtDirHdr )
                  != IDE_SUCCESS );

        IDE_TEST( addNewExtDirPage2Tail( aStatistics,
                                         aMtx,
                                         aSpaceID,
                                         aSegHdr,
                                         sExtDirPagePtr )
              != IDE_SUCCESS );

        IDE_TEST( sdpsfSH::setFmtPageCnt( aMtx,
                                          aSegHdr,
                                          aSegHdr->mFmtPageCnt + 1 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( getLstExtDirPage4Update( aStatistics,
                                           aMtx,
                                           aSpaceID,
                                           aSegHdr,
                                           &sExtDirPagePtr,
                                           &sExtDirHdr )
                  != IDE_SUCCESS );

        sFlag = SDP_SF_EXTDESC_FST_IS_EXTDIRPAGE_FALSE;
    }

    /* Fix된 ExtDirPage 끝에 Add를 시도한다. */
    IDE_TEST( sdpsfExtDirPage::addNewExtDescAtLst(
                  aMtx,
                  sExtDirHdr,
                  aFstExtPID,
                  sFlag,
                  &sNewExtDescPtr )
              != IDE_SUCCESS );

    IDE_ASSERT( sNewExtDescPtr != NULL );

    /* Extent의 갯수를 설정 */
    IDE_TEST( sdpsfSH::setTotExtCnt( aMtx,
                                     aSegHdr,
                                     aSegHdr->mTotExtCnt + 1 )
              != IDE_SUCCESS );

    *aNewExtDescRID = sdpPhyPage::getRIDFromPtr( sNewExtDescPtr );
    *aNewExtDescPtr = sNewExtDescPtr;
    *aExtDirCntlHdr = sExtDirHdr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aNewExtDescRID = SD_NULL_RID;
    *aNewExtDescPtr = NULL;
    *aExtDirCntlHdr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aExtDirCntlHdr를 ExtDirPID List 끝에 추가한다.
 *
 * Caution:
 *   1. aTbsHdr가 속한 페이지에 XLatch가 걸려 있어야 한다.
 *   2. aExtDirCntlHdr가 속한 페이지에 XLath가 걸려 있어 한다.
 *
 * aStatistics       - [IN] 통계정보
 * aMtx              - [IN] Mini Transaction Pointer.
 * aSegHdr           - [IN] Segment Hdr
 * aNewExtDirCntlHdr - [IN] XLatch가 걸린 ExtDirCntl Header
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::addPage2Tail( idvSQL               * aStatistics,
                                          sdrMtx               * aMtx,
                                          sdpsfSegHdr          * aSegHdr,
                                          sdpsfExtDirCntlHdr   * aNewExtDirCntlHdr )
{
    sdpPhyPageHdr       * sPageHdr;
    UChar               * sPagePtr;
    sdpDblPIDListBase   * sExtDirPIDList;

    IDE_ASSERT( aMtx               != NULL );
    IDE_ASSERT( aSegHdr            != NULL );
    IDE_ASSERT( aNewExtDirCntlHdr  != NULL );

    sExtDirPIDList  = &aSegHdr->mExtDirPIDList;
    sPagePtr        = sdpPhyPage::getPageStartPtr( aNewExtDirCntlHdr );
    sPageHdr        = sdpPhyPage::getHdr( sPagePtr );

    IDE_TEST( sdpDblPIDList::insertTailNode( aStatistics,
                                             sExtDirPIDList,
                                             sdpPhyPage::getDblPIDListNode( sPageHdr ),
                                             aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : ExtDirPageList에서 aExtDirCntlHdr가 가리키는 페이지를
 *               제거한다.
 *
 * aStatistics       - [IN] 통계정보
 * aMtx              - [IN] Mini Transaction Pointer.
 * aSegHdr           - [IN] Segment Hdr
 * aExtDirCntlHdr    - [IN] 제거될 페이지의 ExtDir Control Header
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::unlinkPage( idvSQL               * aStatistics,
                                        sdrMtx               * aMtx,
                                        sdpsfSegHdr          * aSegHdr,
                                        sdpsfExtDirCntlHdr   * aExtDirCntlHdr )
{
    sdpPhyPageHdr * sExtDirPhyPageHdr;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );

    sExtDirPhyPageHdr = sdpPhyPage::getHdr( (UChar*)aExtDirCntlHdr );

    IDE_TEST( sdpDblPIDList::removeNode( aStatistics,
                                         &aSegHdr->mExtDirPIDList,
                                         sdpPhyPage::getDblPIDListNode( sExtDirPhyPageHdr ),
                                         aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aSegHdr의 마지막 ExtDirPage를 Buffer에 Fix하고 XLatch를
 *               잡아서 넘겨준다. aSegHdr에 ExtDirCntlHdr가 있어서 만약
 *               에 aSegHdr만이 존재한다면 SegHdr 페이지의 ExtPageCntlHdr
 *               를 넘겨준다. aSegHdr는 이미 XLatch가 잡혀있는 상태이기
 *               때문에 SegDirty만 해주면 된다.
 *
 * aStatistics       - [IN] 통계정보
 * aMtx              - [IN] Mini Transaction Pointer.
 * aSpaceID          - [IN] Space ID
 * aSegHdr           - [IN] Segment Header
 * aExtDirPagePtr    - [IN] Extent DirPage Ptr
 * aExtDirCntlHdr    - [IN] aExtDirPagePtr의 Extent Dir Control Header
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::getLstExtDirPage4Update( idvSQL               * aStatistics,
                                                     sdrMtx               * aMtx,
                                                     scSpaceID              aSpaceID,
                                                     sdpsfSegHdr          * aSegHdr,
                                                     UChar               ** aExtDirPagePtr,
                                                     sdpsfExtDirCntlHdr  ** aExtDirCntlHdr )
{
    sdpsfExtDirCntlHdr  * sExtDirHdr;
    UChar               * sExtDirPagePtr;
    scPageID              sLstExtDirPID;

    IDE_ASSERT( aMtx           != NULL );
    IDE_ASSERT( aSpaceID       != 0 )
    IDE_ASSERT( aSegHdr        != NULL );
    IDE_ASSERT( aExtDirPagePtr != NULL );
    IDE_ASSERT( aExtDirCntlHdr != NULL );

    sLstExtDirPID = sdpDblPIDList::getListTailNode( &aSegHdr->mExtDirPIDList );

    if( sLstExtDirPID != aSegHdr->mSegHdrPID )
    {
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              aSpaceID,
                                              sLstExtDirPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sExtDirPagePtr,
                                              NULL /*aTrySuccess*/,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );

        IDE_ASSERT( sExtDirPagePtr != NULL );

        sExtDirHdr = sdpsfExtDirPage::getExtDirCntlHdr( sExtDirPagePtr );
    }
    else
    {
        sExtDirPagePtr = sdpPhyPage::getPageStartPtr( aSegHdr );
        sExtDirHdr     = &aSegHdr->mExtDirCntlHdr;

        IDE_TEST( sdrMiniTrans::setDirtyPage( aMtx,
                                              sExtDirPagePtr )
                  != IDE_SUCCESS );
    }

    *aExtDirCntlHdr = sExtDirHdr;
    *aExtDirPagePtr = sExtDirPagePtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aExtDirCntlHdr = NULL;
    *aExtDirPagePtr = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : aSegHdr의 ExtDirPageList에 대한 정보를 Dump한다.
 *
 * aSegHdr - [IN] Segment Header
 ***********************************************************************/
IDE_RC sdpsfExtDirPageList::dump( sdpsfSegHdr * aSegHdr )
{
    sdpDblPIDListBase * sExtDirPIDList;

    IDE_ASSERT( aSegHdr != NULL );

    sExtDirPIDList  = &aSegHdr->mExtDirPIDList;;

    ideLog::log( IDE_SERVER_0, 
                 " Add Page: %u,"
                 " %u,"
                 " %u",
                 sdpDblPIDList::getNodeCnt( sExtDirPIDList ),
                 sExtDirPIDList->mBase.mPrev,
                 sExtDirPIDList->mBase.mNext );

    return IDE_SUCCESS;
}

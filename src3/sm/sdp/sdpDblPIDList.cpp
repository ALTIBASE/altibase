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
 * $Id: sdpDblPIDList.cpp 25608 2008-04-11 09:52:13Z kclee $
 *
 * Description :
 *
 * File-Based PID Double Linked-List 구현
 *
 **********************************************************************/

#include <sdr.h>
#include <sdpDblPIDList.h>
#include <sdpPhyPage.h>

/***********************************************************************
 * Description : page list의 base 노드를 초기화
 *
 * + 2nd.code desgin
 *   - base 노드의 page ID를 얻는다.
 *   - base 노드의 mNodeCnt를 0으로 로깅한다. (SDR_4BYTE)
 *   - base 노드의 mHead를 Page ID를 self Page ID로 로깅한다.(SDR_4BYTE)
 *   - base 노드의 mTail를 Page ID를 self Page ID로 로깅한다.(SDR_4BYTE)
 ***********************************************************************/
IDE_RC sdpDblPIDList::initBaseNode(sdpDblPIDListBase  * aBaseNode,
                                   sdrMtx             * aMtx)
{
    scPageID sPID;
    UChar*   sPagePtr;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    // get base page ID
    sPagePtr = sdpPhyPage::getPageStartPtr((UChar*)aBaseNode);
    sPID     = sdpPhyPage::getPageID(sPagePtr);

    // set length
    IDE_TEST( setNodeCnt(aBaseNode, 0, aMtx) != IDE_SUCCESS );
    // set head-node
    IDE_TEST( setNxtOfNode(&aBaseNode->mBase, sPID, aMtx) != IDE_SUCCESS );
    // set tail-node
    IDE_TEST( setPrvOfNode(&aBaseNode->mBase, sPID, aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : page list의 head에 노드를 추가
 *
 * + 2nd. code design
 *   => sdpDblPIDList::insertNodeAfter를 호출함
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertHeadNode(idvSQL            * aStatistics,
                                     sdpDblPIDListBase * aBaseNode,
                                     sdpDblPIDListNode * aNewNode,
                                     sdrMtx            * aMtx)
{

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    IDE_TEST( insertNodeAfter(aStatistics,
                              aBaseNode,
                              &aBaseNode->mBase,
                              aNewNode,
                              aMtx) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :  page list의 tail에 노드를 추가
 *
 * + 2nd. code design
 *   => sdpDblPIDList::insertNodeBefore를 호출함
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertTailNode(idvSQL              * aStatistics,
                                     sdpDblPIDListBase   * aBaseNode,
                                     sdpDblPIDListNode   * aNewNode,
                                     sdrMtx              * aMtx)
{

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );


    IDE_TEST( insertNodeBefore(aStatistics,
                               aBaseNode,
                               &aBaseNode->mBase,
                               aNewNode,
                               aMtx) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 * Description :  page list의 tail에 Node List를 추가
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertTailList( idvSQL             * aStatistics,
                                      sdpDblPIDListBase  * aBaseNode,
                                      sdpDblPIDListNode  * aFstNode,
                                      sdpDblPIDListNode  * aLstNode,
                                      ULong                aListLen,
                                      sdrMtx             * aMtx )
{

    scSpaceID       sSpaceID;
    scPageID        sBasePID;
    scPageID        sPrevPID;
    scPageID        sFstPID;
    scPageID        sLstPID;
    ULong           sListLen;
    idBool          sTrySuccess;
    UChar*          sPagePtr;      // base 노드의 page
    UChar*          sPrevPagePtr;  // 기준 노드의 previous page
    sdpDblPIDListNode* sPrevNode;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aFstNode  != NULL );
    IDE_DASSERT( aLstNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    sPagePtr = (UChar*)sdpPhyPage::getHdr((UChar*)aBaseNode);
    sSpaceID = sdpPhyPage::getSpaceID((UChar*)sPagePtr);
    sBasePID = sdpPhyPage::getPageID(sPagePtr);
    sPrevPID = getPrvOfNode(&aBaseNode->mBase);

    IDE_ASSERT(sSpaceID == (sdpPhyPage::getSpaceID(
                                (UChar*)sdpPhyPage::getHdr((UChar*)aFstNode))));
    IDE_ASSERT(sSpaceID == (sdpPhyPage::getSpaceID(
                                (UChar*)sdpPhyPage::getHdr((UChar*)aLstNode))));

    sFstPID  = sdpPhyPage::getPageID((UChar*)sdpPhyPage::getHdr((UChar*)aFstNode));
    sLstPID  = sdpPhyPage::getPageID((UChar*)sdpPhyPage::getHdr((UChar*)aLstNode));

    IDE_TEST( setNxtOfNode(aLstNode, sBasePID, aMtx) != IDE_SUCCESS );
    IDE_TEST( setPrvOfNode(aFstNode, sPrevPID, aMtx) != IDE_SUCCESS );

    // PREVNODE의 next 노드를 새로운 노드로 설정한다.
    if (sPrevPID != sBasePID )
    {
        sPrevPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                           sSpaceID,
                                                           sPrevPID );
        if( sPrevPagePtr == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  sSpaceID,
                                                  sPrevPID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  aMtx,
                                                  &sPrevPagePtr,
                                                  &sTrySuccess,
                                                  NULL /*IsCorruptPage*/ )
                      != IDE_SUCCESS );
            IDE_DASSERT( sPrevPagePtr != NULL );
        }

        IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sPrevPagePtr));
        sPrevNode = sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sPrevPagePtr);
    }
    else
    {
        sPrevNode = &aBaseNode->mBase;
    }

    IDE_TEST( setNxtOfNode(sPrevNode, sFstPID, aMtx) != IDE_SUCCESS );

    // 기준 노드의 prev 노드를 새로운 노드로 설정한다.
    IDE_TEST( setPrvOfNode(&aBaseNode->mBase, sLstPID, aMtx) != IDE_SUCCESS );

    // 새로운 노드 추가로 인한 base 노드의 리스트 개수 aListLen 증가
    sListLen = getNodeCnt(aBaseNode);
    IDE_TEST( setNodeCnt(aBaseNode, (sListLen + aListLen), aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/***********************************************************************
 * Description : page list의 특정 노드 뒤에 새로운 노드 추가
 *
 * + 2nd. code design
 *   - 기준 노드의 page ID와 space ID를 얻는다.
 *   - newnode의 page ID를 얻는다.
 *   - 기준 노드의 NEXTNODE를 얻는다.
 *   - newnode의 prev 노드를 기준 노드로 설정 (SDR_4BYTES)
 *   - newnode의 next 노드를 NEXTNODE로 설정 (SDR_4BYTES)
 *   - if newnode와 NEXTNODE와 동일한 page인 경우
 *     : !! assert
 *   - else 그렇지 않은 경우
 *     : NEXTNODE를 버퍼관리자에 요청하여 fix시킨후,
 *       포인터 얻음
 *     => NEXTNODE의 prev 노드를 newnode로 설정 (SDR_4BYTES)
 *   - 기준 노드의 next 노드를 newnode로 설정(SDR_4BYTES)
 *   - basenode의 길이를 구해서 +1 (SDR_4BYTES)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 *   2) node != newnode 확인
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertNodeAfter(idvSQL              * aStatistics,
                                      sdpDblPIDListBase   * aBaseNode,
                                      sdpDblPIDListNode   * aNode,
                                      sdpDblPIDListNode   * aNewNode,
                                      sdrMtx              * aMtx)
{

    scPageID        sPID;
    scPageID        sNewPID;
    scPageID        sNextPID;
    scSpaceID       sSpaceID;
    ULong           sListLen;
    idBool          sTrySuccess;
    UChar*          sPagePtr;
    UChar*          sNewPagePtr;
    UChar*          sNextPagePtr;
    sdpDblPIDListNode* sNextNode;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNode     != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNode     != aNewNode );

    // 기준 노드의 page ID를 얻는다. tnode
    sPagePtr    = sdpPhyPage::getPageStartPtr((UChar*)aNode);
    sSpaceID    = sdpPhyPage::getSpaceID(sPagePtr);
    sPID        = sdpPhyPage::getPageID(sPagePtr);

    // 새로운 노드의 page ID를 얻는다. node
    sNewPagePtr = sdpPhyPage::getPageStartPtr((UChar*)aNewNode);
    sNewPID     = sdpPhyPage::getPageID(sNewPagePtr);

    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sNewPagePtr));
    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(
                   (UChar*)sdpPhyPage::getHdr((UChar*)aNode)));

    // 새로운 노드의 prev/next를 설정한다.
    sNextPID = getNxtOfNode(aNode); // get NEXTNODE

    // 동일한 page일 경우 Assert!!
    IDE_ASSERT( sNewPID != sNextPID );

    // NEXTNODE를 fix 시킨다.
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          sSpaceID,
                                          sNextPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          &sNextPagePtr,
                                          &sTrySuccess,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );

    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sNextPagePtr));

    IDE_TEST( setPrvOfNode(aNewNode, sPID, aMtx) != IDE_SUCCESS ); // node->prev = tnode
    IDE_TEST( setNxtOfNode(aNewNode, sNextPID, aMtx) != IDE_SUCCESS ); // node->next = tnode->next

    // NEXTNODE의 prev 노드를 새로운 노드로 설정한다.
    if (sNextPagePtr != sdpPhyPage::getPageStartPtr((UChar*)aBaseNode))
    {
        sNextNode = sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sNextPagePtr);
    }
    else
    {
        sNextNode = &aBaseNode->mBase;
    }

    IDE_TEST( setPrvOfNode(sNextNode, sNewPID, aMtx) != IDE_SUCCESS ); // tnode->next->prev = node

    // 기준 노드의 next 노드를 새로운 노드로 설정한다.
    IDE_TEST( setNxtOfNode(aNode, sNewPID, aMtx) != IDE_SUCCESS ); // tnode->next = node

    // 새로운 노드 추가로 인한 base 노드의 리스트 개수 1 증가
    sListLen = getNodeCnt(aBaseNode);
    IDE_TEST( setNodeCnt(aBaseNode, (sListLen + 1), aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : page list의 특정 노드 앞에 새로운 노드 추가
 *
 * + 2nd. code design
 *   - 기준 노드의 page ID와 space id를 알아냄
 *   - newnode의 page ID를 얻음
 *   - 기준 노드의 PREVNODE를 얻음
 *   - newnode의 prev 노드를 PREVNODE로 설정 (SDR_4BYTES)
 *   - newnode의 next 노드를 기준 노드로 설정 (SDR_4BYTES)
 *   - if newnode와 PREVNODE와 동일한 page인 경우
 *     : !! assert
 *   - else
 *     : PREVNODE를 버퍼관리자에 요청하여 fix시킨후, 포인터 얻음
 *     => PREVNODE의 next 노드를 new 노드로 설정(SDR_4BYTES)
 *   - 기준 노드의 prev 노드를 newnode의 노드로 설정(SDR_4BYTES)
 *   - basenode의 길이를 구해서 +1 (SDR_4BYTES)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 *   2) node != newnode 확인
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertNodeBeforeLow(idvSQL             * aStatistics,
                                          sdpDblPIDListBase  * aBaseNode,
                                          sdpDblPIDListNode  * aNode,    // tnode
                                          sdpDblPIDListNode  * aNewNode, // node
                                          sdrMtx             * aMtx,
                                          idBool               aNeedGetPage)
{
    scPageID            sPID;
    scPageID            sNewPID;
    scPageID            sPrevPID;
    scSpaceID           sSpaceID;
    ULong               sListLen;
    idBool              sTrySuccess;
    UChar             * sPagePtr;      // 기준 노드의 page
    UChar             * sNewPagePtr;   // 새로운 노드의 page
    UChar             * sPrevPagePtr;  // 기준 노드의 previous page
    sdpDblPIDListNode * sPrevNode;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNode     != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNode     != aNewNode );

    // 기준 노드의 page ID를 얻는다. tnode
    sPagePtr = sdpPhyPage::getPageStartPtr((UChar*)aNode);
    sPID     = sdpPhyPage::getPageID(sPagePtr);
    sSpaceID = sdpPhyPage::getSpaceID(sPagePtr);

    // 새로운 노드의 page ID를 얻는다. node
    sNewPagePtr = sdpPhyPage::getPageStartPtr((UChar*)aNewNode);
    sNewPID     = sdpPhyPage::getPageID(sNewPagePtr);

    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sNewPagePtr));
    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(
                   (UChar*)sdpPhyPage::getHdr((UChar*)aNode)));

    // 새로운 노드의 prev/next를 설정한다.
    sPrevPID = getPrvOfNode(aNode); // get PREVNODE

    if( ( aNeedGetPage == ID_TRUE ) && ( sNewPID != sPrevPID ) )
    {
        // PREVNODE fix 시킨다.
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              sSpaceID,
                                              sPrevPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPrevPagePtr,
                                              &sTrySuccess,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );
    }
    else
    {
        sPrevPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                           sSpaceID,
                                                           sPrevPID );
        IDE_ASSERT( sPrevPagePtr != NULL );
    }

    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sPrevPagePtr));

    IDE_TEST( setNxtOfNode(aNewNode, sPID, aMtx) != IDE_SUCCESS );     // node->next = tnode
    IDE_TEST( setPrvOfNode(aNewNode, sPrevPID, aMtx) != IDE_SUCCESS ); // node->prev = tnode->prev

    // 새로운 노드 추가로 인한 base 노드의 리스트 개수 1 증가
    sListLen = getNodeCnt(aBaseNode);

    // PREVNODE의 next 노드를 새로운 노드로 설정한다.
    if (sPrevPagePtr != sdpPhyPage::getPageStartPtr((UChar*)aBaseNode) || sListLen != 0 )
    {
        sPrevNode = sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sPrevPagePtr);
    }
    else
    {
        sPrevNode = &aBaseNode->mBase;
    }

    IDE_TEST( setNxtOfNode(sPrevNode, sNewPID, aMtx) != IDE_SUCCESS ); // tnode->prev->next = node

    // 기준 노드의 prev 노드를 새로운 노드로 설정한다.
    IDE_TEST( setPrvOfNode(aNode, sNewPID, aMtx) != IDE_SUCCESS );  // tnode->prev = node

    IDE_TEST( setNodeCnt(aBaseNode, sListLen + 1, aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 *
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertNodeBefore(idvSQL             * aStatistics,
                                       sdpDblPIDListBase  * aBaseNode,
                                       sdpDblPIDListNode  * aNode,    // tnode
                                       sdpDblPIDListNode  * aNewNode, // node
                                       sdrMtx             * aMtx)
{


    IDE_TEST( insertNodeBeforeLow( aStatistics,
                                   aBaseNode,
                                   aNode,
                                   aNewNode,
                                   aMtx,
                                   ID_TRUE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 *
 ***********************************************************************/
IDE_RC sdpDblPIDList::insertNodeBeforeWithNoFix(idvSQL             * aStatistics,
                                                sdpDblPIDListBase  * aBaseNode,
                                                sdpDblPIDListNode  * aNode,
                                                sdpDblPIDListNode  * aNewNode,
                                                sdrMtx             * aMtx)
{


    IDE_TEST( insertNodeBeforeLow( aStatistics,
                                   aBaseNode,
                                   aNode,
                                   aNewNode,
                                   aMtx,
                                   ID_FALSE )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : page list에서 특정노드 제거
 *
 * + 2nd. code design
 *   - 삭제할 노드의의 PREVNODE/NEXTNODE의 page ID를 얻는다
 *   - if PREVNODE == 삭제할 노드 then
 *     :!! assert
 *   - else
 *     :PREVNODE를 fix 요청
 *   - PREVNODE의 next 노드를 NEXTNODE로 설정(SDR_4BYTES)
 *   - if NEXTNODE == 삭제할 노드 then
 *     :!! assert
 *   - else
 *     :NEXTNODE를 fix 요청
 *   - NEXTNODE의 prevnode를 PREVNODE로 설정 (SDR_4BYTES)
 *   - basenode의 길이를 구해서 1 감소 (SDR_4BYTES)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 *   2) basenode != node
 *   3) basenode, node에 대한 fix(x-latch) 확인
 ***********************************************************************/
IDE_RC sdpDblPIDList::removeNodeLow( idvSQL             * aStatistics,
                                     sdpDblPIDListBase  * aBaseNode,
                                     sdpDblPIDListNode  * aNode,
                                     sdrMtx             * aMtx,
                                     idBool               aCheckMtxStack )
{
    ULong               sListLen;
    idBool              sTrySuccess;
    scSpaceID           sSpaceID;
    scPageID            sPrevPID;
    scPageID            sNextPID;
    UChar              *sPagePtr;
    UChar              *sPrevPagePtr;
    UChar              *sNextPagePtr;
    sdpDblPIDListNode  *sNextNode;
    sdpDblPIDListNode  *sPrevNode;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNode     != NULL );
    IDE_DASSERT( aMtx      != NULL );

    // 기준 노드의 page ID와 space ID를 얻는다.
    sPagePtr = sdpPhyPage::getPageStartPtr((UChar*)aNode);
    sSpaceID = sdpPhyPage::getSpaceID(sPagePtr);

    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sPagePtr));

    // 기준 노드의 PREVNODE/NEXTNODE를 얻는다.
    sPrevPID = getPrvOfNode(aNode);
    sNextPID = getNxtOfNode(aNode);

    if(aCheckMtxStack == ID_FALSE)
    {
        // PREVNODE fix 시킨다.
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              sSpaceID,
                                              sPrevPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sPrevPagePtr,
                                              &sTrySuccess,
                                              NULL /*IsCorruptPage*/ )
                  != IDE_SUCCESS );

        // NEXTNODE fix 시킨다.
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              sSpaceID,
                                              sNextPID,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sNextPagePtr,
                                              &sTrySuccess,
                                              NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );
    }
    else
    {
        sPrevPagePtr = sdrMiniTrans::getPagePtrFromPageID(
            aMtx,
            sSpaceID,
            sPrevPID );
        if ( sPrevPagePtr == NULL )
        {
            // PREVNODE fix 시킨다.
            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  sSpaceID,
                                                  sPrevPID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  aMtx,
                                                  &sPrevPagePtr,
                                                  &sTrySuccess,
                                                  NULL /*IsCorruptPage*/ )
                      != IDE_SUCCESS );
        }

        sNextPagePtr = sdrMiniTrans::getPagePtrFromPageID(
            aMtx,
            sSpaceID,
            sNextPID );
        if (sNextPagePtr == NULL)
        {
            // NEXTNODE fix 시킨다.
            IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                                  sSpaceID,
                                                  sNextPID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_SINGLE_PAGE_READ,
                                                  aMtx,
                                                  &sNextPagePtr,
                                                  &sTrySuccess,
                                                  NULL /*IsCorruptPage*/ )
                      != IDE_SUCCESS );
        }
    }

    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sNextPagePtr));
    IDE_ASSERT(sSpaceID == sdpPhyPage::getSpaceID(sPrevPagePtr));

    // PREVNODE의 next 노드를 NEXTNODE로 설정
    if (sPrevPagePtr != sdpPhyPage::getPageStartPtr((UChar*)aBaseNode))
    {
        sPrevNode = sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sPrevPagePtr);
    }
    else
    {
        sPrevNode = &aBaseNode->mBase;
    }
    IDE_TEST( setNxtOfNode(sPrevNode, sNextPID, aMtx) != IDE_SUCCESS );

    // NEXTNODE의 prev 노드를 새로운 노드로 설정한다.
    if (sNextPagePtr != sdpPhyPage::getPageStartPtr((UChar*)aBaseNode))
    {
        sNextNode = sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sNextPagePtr);
    }
    else
    {
        sNextNode = &aBaseNode->mBase;
    }
    IDE_TEST( setPrvOfNode(sNextNode, sPrevPID, aMtx) != IDE_SUCCESS );

    // 노드 제거로 인한 base 노드의 리스트 개수 1 감소
    sListLen = getNodeCnt(aBaseNode);
    IDE_ASSERT( sListLen != 0 );
    IDE_TEST( setNodeCnt(aBaseNode, (sListLen - 1), aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 ***********************************************************************/
IDE_RC sdpDblPIDList::removeNode(idvSQL              * aStatistics,
                                 sdpDblPIDListBase   * aBaseNode,
                                 sdpDblPIDListNode   * aNode,
                                 sdrMtx              * aMtx)
{
    IDE_TEST( removeNodeLow(aStatistics,
                            aBaseNode,
                            aNode,
                            aMtx,
                            ID_FALSE)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :
 *
 ***********************************************************************/
IDE_RC sdpDblPIDList::removeNodeWithNoFix(idvSQL              * aStatistics,
                                          sdpDblPIDListBase   * aBaseNode,
                                          sdpDblPIDListNode   * aNode,
                                          sdrMtx              * aMtx)
{
    IDE_TEST( removeNodeLow(aStatistics,
                            aBaseNode,
                            aNode,
                            aMtx,
                            ID_TRUE)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : base 노드의 length 설정 및 logging
 * base 노드의 list length을 설정한다.
 ***********************************************************************/
IDE_RC sdpDblPIDList::setNodeCnt( sdpDblPIDListBase *aBaseNode,
                                  ULong              aLen,
                                  sdrMtx            *aMtx )
{

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&aBaseNode->mNodeCnt,
                                        &aLen,
                                        ID_SIZEOF(aLen) ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 노드의 prev page ID 설정 및 logging
 * aNode가 base 노드이면 tail 노드를 설정하고, 그렇지 않으면 next 노드를
 * 설정한다.
 ***********************************************************************/
IDE_RC sdpDblPIDList::setPrvOfNode(sdpDblPIDListNode  * aNode,
                                   scPageID             aPageID,
                                   sdrMtx             * aMtx)
{

    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&aNode->mPrev,
                                        &aPageID,
                                        ID_SIZEOF(aPageID) ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 노드의 prev page ID 설정 및 logging
 ***********************************************************************/
IDE_RC sdpDblPIDList::setNxtOfNode(sdpDblPIDListNode  * aNode,
                                   scPageID             aPageID,
                                   sdrMtx             * aMtx)
{

    IDE_DASSERT( aNode != NULL );
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&aNode->mNext,
                                        &aPageID,
                                        ID_SIZEOF(aPageID) ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 리스트의 모든 node 출력
 ***********************************************************************/
IDE_RC sdpDblPIDList::dumpList( scSpaceID  aSpaceID,
                                sdRID      aBaseNodeRID )
{
    UInt                i;
    ULong               sNodeCount;
    sdrMtx              sMtx;
    UChar             * sPagePtr;
    sdpDblPIDListBase * sBaseNode;
    idBool              sTrySuccess;
    scPageID            sCurPageID;
    scPageID            sBasePageID;
    SInt                sState = 0;

    IDE_ERROR( aBaseNodeRID != SD_NULL_RID );

    IDE_TEST( sdrMiniTrans::begin( NULL, /* idvSQL* */
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdbBufferMgr::getPageByRID(NULL, /* idvSQL* */
                                         aSpaceID,
                                         aBaseNodeRID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         &sMtx,
                                         (UChar**)&sBaseNode,
                                         &sTrySuccess) != IDE_SUCCESS );

    sBasePageID = SD_MAKE_PID(aBaseNodeRID);
    sNodeCount  = getNodeCnt(sBaseNode);
    sCurPageID = getListHeadNode(sBaseNode);

    for (i = 0; sCurPageID != sBasePageID; i++)
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL,
                                              aSpaceID,
                                              sCurPageID,
                                              &sPagePtr,
                                              &sTrySuccess) != IDE_SUCCESS );
        sState = 2;

        idlOS::printf("%"ID_UINT32_FMT"/%"ID_UINT32_FMT" : pageID %"ID_UINT32_FMT"\n",
                      i + 1,
                      sNodeCount,
                      sdpPhyPage::getPageID(sPagePtr));

        sCurPageID = getNxtOfNode(sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sPagePtr));

        sState = 1;
        IDE_TEST( sdbBufferMgr::unfixPage( NULL, /* idvSQL* */
                                           sPagePtr )
                  != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    IDE_ASSERT( i == sNodeCount ); // !! assert
    IDE_ASSERT( sState == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, sPagePtr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sdrMiniTrans::rollback( &sMtx )
                        == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 리스트의 dump check.
 ***********************************************************************/
IDE_RC sdpDblPIDList::dumpCheck( sdpDblPIDListBase  * aBaseNode,
                                 scSpaceID            aSpaceID,
                                 scPageID             aPageID )
{
    UInt                i;
    ULong               sNodeCount;
    UChar*              sPagePtr;
    idBool              sTrySuccess;
    scPageID            sCurPageID;
    UShort              sState = 0;

    IDE_ERROR( aBaseNode != NULL );

    sNodeCount  = getNodeCnt(aBaseNode);
    sCurPageID  = getListHeadNode(aBaseNode);

    for (i = 0;  i < sNodeCount; i++)
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL,
                                              aSpaceID,
                                              sCurPageID,
                                              &sPagePtr,
                                              &sTrySuccess) != IDE_SUCCESS );
        sState = 1;
        if( sCurPageID ==  aPageID)
        {
            IDE_ASSERT(0);
        }

        sCurPageID = getNxtOfNode(sdpPhyPage::getDblPIDListNode((sdpPhyPageHdr*)sPagePtr));

        sState = 0;

        IDE_TEST( sdbBufferMgr::unfixPage( NULL, /* idvSQL* */
                                           sPagePtr ) != IDE_SUCCESS );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if( sState != 0)
        {
            IDE_ASSERT( sdbBufferMgr::unfixPage(NULL, /* idvSQL* */
                                                sPagePtr)
                        == IDE_SUCCESS);
        }
    }
    return IDE_FAILURE;
}

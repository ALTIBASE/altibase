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
 * $Id: sdpDblRIDList.cpp 24150 2007-11-14 06:36:46Z bskim $
 *
 * Description :
 *
 * File-Based RID Linked-List 구현
 *
 **********************************************************************/

#include <sdr.h>
#include <sdpDblRIDList.h>
#include <sdpPhyPage.h>

/***********************************************************************
 * Description : RID list의 base 노드를 초기화
 *
 * + 2nd.code desgin
 *   - base 노드의 rid를 얻는다.
 *   - base 노드의 mNodeCnt를 0로 설정    (SDR_4BYTES)
 *   - base 노드의 mHead를 self RID를 설정(SDR_8BYTES)
 *   - base 노드의 mTail를 self RID를 설정(SDR_8BYTES)
 ***********************************************************************/
IDE_RC sdpDblRIDList::initList( sdpDblRIDListBase*    aBaseNode,
                                sdrMtx*               aMtx )
{
    sdRID sBaseRID;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    sBaseRID = getBaseNodeRID( aBaseNode );

    // set length
    IDE_TEST( setNodeCnt( aBaseNode, 0, aMtx ) != IDE_SUCCESS );

    // set head-node
    IDE_TEST( setNxtOfNode( &aBaseNode->mBase, sBaseRID, aMtx )
              != IDE_SUCCESS );

    // set tail-node
    IDE_TEST( setPrvOfNode( &aBaseNode->mBase, sBaseRID, aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : RID list의 head에 노드를 추가
 *
 * + 2nd. code design
 *   => sdRIDList::insertNodeAfter를 호출함
 ***********************************************************************/
IDE_RC sdpDblRIDList::insertHeadNode(idvSQL              * aStatistics,
                                     sdpDblRIDListBase   * aBaseNode,
                                     sdpDblRIDListNode   * aNewNode,
                                     sdrMtx              * aMtx)
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    // head rid 를 교체한다.
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
 * Description :  RID list의 tail에 노드를 추가
 *
 * + 2nd. code design
 *   - sdpDblRIDList::insertNodeBefore를 호출함
 ***********************************************************************/
IDE_RC sdpDblRIDList::insertTailNode( idvSQL             *aStatistics,
                                      sdpDblRIDListBase*  aBaseNode,
                                      sdpDblRIDListNode*  aNewNode,
                                      sdrMtx             *aMtx )
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    // tail rid 를 교체한다.
    IDE_TEST( insertNodeBefore( aStatistics,
                                aBaseNode,
                                &aBaseNode->mBase,
                                aNewNode,
                                aMtx ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : RID list의 특정 노드 뒤에 새로운 노드 추가
 *
 * + 2nd. code design
 *   - 기준 노드의 RID을 얻는다.
 *   - newnode의 RID를 얻는다.
 *   - 기준 노드의 NEXTNODE를 얻는다.
 *   - newnode의 prev 노드를 기준 노드로 설정 (SDR_8BYTES)
 *   - newnode의 next 노드를 NEXTNODE로 설정 (SDR_8BYTES)
 *   - if newnode와 NEXTNODE와 동일한 page인 경우
 *     : 직접 NEXTNODE의 ptr을 얻는다.
 *   - else 
 *     : NEXTNODE를 버퍼관리자에 요청하여 fix시킨후,
 *       포인터 얻음
 *     => NEXTNODE의 prevnode를 newnode로 설정 (SDR_8BYTE)
 *   - 기준 노드의 next 노드를 newnode로 설정(SDR_8BYTE)
 *   - basenode의 길이를 구해서 +1 (SDR_4BYTE)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 *   2) node != newnode 확인
 ***********************************************************************/
IDE_RC sdpDblRIDList::insertNodeAfter( idvSQL              * aStatistics,
                                       sdpDblRIDListBase   * aBaseNode,
                                       sdpDblRIDListNode   * aNode,
                                       sdpDblRIDListNode   * aNewNode,
                                       sdrMtx              * aMtx )
{
    scSpaceID           sSpaceID;
    sdRID               sRID;
    sdRID               sNewRID;
    sdRID               sNextRID;
    ULong               sNodeCnt;
    idBool              sTrySuccess;
    sdpDblRIDListNode*  sNextNode;
    UChar              *sPagePtr;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNode     != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNode     != aNewNode );

    // 기준 노드의 page ID를 얻는다.
    sRID    = sdpPhyPage::getRIDFromPtr( (UChar*)aNode );
    
    // 새로운 노드의 page ID를 얻는다.
    sNewRID = sdpPhyPage::getRIDFromPtr( (UChar*)aNewNode );
   
    // 새로운 노드의 prev/next를 설정한다.
    sNextRID = getNxtOfNode( aNode ); // get NEXTNODE

    sSpaceID = sdpPhyPage::getSpaceID( sdpPhyPage::getPageStartPtr( (UChar*)aBaseNode ) );
    
    // 동일한 page인 경우
    if( isSamePage( &sNewRID, &sNextRID ) == ID_TRUE ) 
    {
        sNextNode = (sdpDblRIDListNode*)( sdpPhyPage::getPageStartPtr( (UChar*)aNewNode )
                                         + SD_MAKE_OFFSET( sNextRID ) );
    }
    else
    {
        // NEXTNODE를 fix 시킨다.
        sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                       sSpaceID,
                                                       SD_MAKE_PID( sNextRID) );

        if( sPagePtr == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                  sSpaceID,
                                                  sNextRID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  aMtx,
                                                  (UChar**)&sNextNode,
                                                  &sTrySuccess )
                      != IDE_SUCCESS );
        }
        else
        {
            sNextNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sNextRID ) );
        }

    }

    IDE_TEST( setPrvOfNode( aNewNode, sRID, aMtx ) != IDE_SUCCESS );
    IDE_TEST( setNxtOfNode( aNewNode, sNextRID, aMtx ) != IDE_SUCCESS );

    // NEXTNODE의 prev 노드를 새로운 노드로 설정한다.
    IDE_TEST( setPrvOfNode( sNextNode, sNewRID, aMtx ) != IDE_SUCCESS );

    // 기준 노드의 next 노드를 새로운 노드로 설정한다.
    IDE_TEST( setNxtOfNode(aNode, sNewRID, aMtx) != IDE_SUCCESS );

    // 새로운 노드 추가로 인한 base 노드의 리스트 개수 1 증가
    sNodeCnt = getNodeCnt( aBaseNode );
    IDE_TEST( setNodeCnt( aBaseNode, ( sNodeCnt + 1 ), aMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/***********************************************************************
 * Description : RID list의 특정 노드 앞에 새로운 노드 추가
 *
 * + 2nd. code design
 *   - 기준 노드의 RID을 얻는다.
 *   - newnode의 RID를 얻는다.
 *   - 기준 노드의 PREVNODE를 얻는다.
 *   - newnode의 prev 노드를 PREVNODE로 설정 (SDR_8BYTES)
 *   - newnode의 next 노드를 기준 노드로 설정 (SDR_8BYTES)
 *   - if newnode와 PREVNODE와 동일한 page인 경우
 *     : 직접 PREVNODE의 ptr을 얻는다.
 *   - else 
 *     : PREVNODE를 버퍼관리자에 요청하여 fix시킨후,
 *       포인터 얻음
 *     => PREVNODE의 next 노드를 newnode로 설정 (SDR_8BYTE)
 *   - 기준 노드의 prev 노드를 newnode로 설정(SDR_8BYTE)
 *   - basenode의 길이를 구해서 +1 (SDR_4BYTE)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 *   2) node != newnode 확인
 ***********************************************************************/
IDE_RC sdpDblRIDList::insertNodeBefore(idvSQL           *aStatistics,
                                       sdpDblRIDListBase*   aBaseNode,
                                       sdpDblRIDListNode*   aNode,
                                       sdpDblRIDListNode*   aNewNode,
                                       sdrMtx*           aMtx)
{

    scSpaceID            sSpaceID;
    sdRID                sRID;
    sdRID                sNewRID;
    sdRID                sPrevRID;
    idBool               sTrySuccess;
    sdpDblRIDListNode*   sPrevNode;
    UChar              * sPagePtr;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNode     != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNode     != aNewNode );

    // 기준 노드의 page ID를 얻는다. (tnode)
    sRID    = sdpPhyPage::getRIDFromPtr( (UChar*)aNode );
    
    // 새로운 노드의 page ID를 얻는다. (node)
    sNewRID = sdpPhyPage::getRIDFromPtr( (UChar*)aNewNode );
   
    // 새로운 노드의 prev/next를 설정한다. (tnode->prev)
    sPrevRID = getPrvOfNode( aNode ); // get PREVNODE

    sSpaceID = sdpPhyPage::getSpaceID( sdpPhyPage::getPageStartPtr( (UChar*)aBaseNode ) );
    
    // 동일한 page인 경우
    if (isSamePage( &sNewRID, &sPrevRID ) == ID_TRUE ) 
    {
        sPrevNode = (sdpDblRIDListNode*)( sdpPhyPage::getPageStartPtr( (UChar*)aNewNode )
                                         + SD_MAKE_OFFSET( sPrevRID ) );
    }
    else
    {
        // PREVNODE를 fix 시킨다.
        // NEXTNODE를 fix 시킨다.
        sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                       sSpaceID,
                                                       SD_MAKE_PID( sPrevRID) );

        if( sPagePtr == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                  sSpaceID,
                                                  sPrevRID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  aMtx,
                                                  (UChar**)&sPrevNode,
                                                  &sTrySuccess ) != IDE_SUCCESS );
        }
        else
        {
            sPrevNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sPrevRID ) );
        }

    }

    // (node->next = tnode)
    IDE_TEST( setNxtOfNode( aNewNode, sRID, aMtx ) != IDE_SUCCESS );
    // (node->prev = tnode->prev)
    IDE_TEST( setPrvOfNode( aNewNode, sPrevRID, aMtx ) != IDE_SUCCESS );

    // PREVNODE의 next 노드를 새로운 노드로 설정한다. tnode->prev->next = node
    IDE_TEST( setNxtOfNode( sPrevNode, sNewRID, aMtx ) != IDE_SUCCESS );

    // 기준 노드의 prev 노드를 새로운 노드로 설정한다. tnode->prev = node
    IDE_TEST( setPrvOfNode( aNode, sNewRID, aMtx ) != IDE_SUCCESS );

    // 새로운 노드 추가로 인한 base 노드의 리스트 개수 1 증가
    IDE_TEST( setNodeCnt( aBaseNode, getNodeCnt( aBaseNode ) + 1, aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : RID list에서 특정노드 제거
 *
 * + 2nd. code design
 *   - 삭제할 노드의의 PREVNODE/NEXTNODE의 RID를 얻는다
 *   - if PREVNODE == 삭제할 노드 then
 *     :직접 PREVNODE의 ptr을 얻는다.
 *   - else
 *     :PREVNODE를 fix 요청 
 *   - PREVNODE의 next 노드를 NEXTNODE로 설정(SDR_8BYTES)
 *   - if NEXTNODE == 삭제할 노드 then
 *     :직접 NEXTNODE의 ptr을 얻는다.
 *   - else
 *     :NEXTNODE를 fix 요청
 *   -  NEXTNODE의 prevnode를 PREVNODE로 설정 (SDR_8BYTES)
 *   - basenode의 길이를 구해서 1 감소 (SDR_4BYTES)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 ***********************************************************************/
IDE_RC sdpDblRIDList::removeNode( idvSQL              * aStatistics,
                                  sdpDblRIDListBase   * aBaseNode,
                                  sdpDblRIDListNode   * aNode,
                                  sdrMtx              * aMtx )
{
    scSpaceID             sSpaceID;
    ULong                 sNodeCnt;
    idBool                sTrySuccess;
    sdRID                 sRID;
    sdRID                 sPrevRID;
    sdRID                 sNextRID;
    sdpDblRIDListNode    *sPrevNode;
    sdpDblRIDListNode    *sNextNode;
    UChar                *sPagePtr;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aNode     != NULL );
    IDE_DASSERT( aMtx      != NULL );

    IDE_DASSERT( getNodeCnt(aBaseNode) > 0 );

    // 기준 노드의 page ID와 space ID를 얻는다.
    // 기준 노드의 page ID와 space ID를 얻는다.
    sRID = sdpPhyPage::getRIDFromPtr( (UChar*)aNode );
    IDE_DASSERT(sRID != SD_NULL_RID);

    //  기준 노드의 PREVNODE/NEXTNODE를 얻는다.
    sPrevRID = getPrvOfNode( aNode );
    sNextRID = getNxtOfNode( aNode );

    sSpaceID = sdpPhyPage::getSpaceID(
        sdpPhyPage::getPageStartPtr( (UChar*)aBaseNode ) );

    // PREVNODE와 기존 노드가 동일하다면 ASSERT!!
    if (isSamePage( &sPrevRID, &sRID ) == ID_TRUE )
    {
        sPrevNode = (sdpDblRIDListNode*)( sdpPhyPage::getPageStartPtr( (UChar*)aNode )
                                          + SD_MAKE_OFFSET( sPrevRID ) );
    }
    else
    {
        // PREVNODE fix 시킨다.
        sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                       sSpaceID,
                                                       SD_MAKE_PID( sPrevRID) );
        if( sPagePtr == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                  sSpaceID,
                                                  sPrevRID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  aMtx,
                                                  (UChar**)&sPrevNode,
                                                  &sTrySuccess ) != IDE_SUCCESS );
        }
        else
        {
            sPrevNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sPrevRID ) );

        }
    }
    
    IDE_DASSERT( sPrevNode != NULL );

    // NEXTNODE와 기존 노드가 동일하다면
    if (isSamePage(&sNextRID, &sRID) == ID_TRUE)
    {
        sNextNode = (sdpDblRIDListNode*)(sdpPhyPage::getPageStartPtr((UChar*)aNode)
                                         + SD_MAKE_OFFSET(sNextRID));
    }
    else
    {
        // NEXTNODE fix 시킨다.
        sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                       sSpaceID,
                                                       SD_MAKE_PID( sNextRID) );
        if( sPagePtr == NULL )
        {
            IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                  sSpaceID,
                                                  sNextRID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  aMtx,
                                                  (UChar**)&sNextNode,
                                                  &sTrySuccess ) != IDE_SUCCESS );
        }
        else
        {
            sNextNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET(sNextRID));
        }
    }

    // PREVNODE의 next 노드를 NEXTNODE로 설정
    IDE_TEST( setNxtOfNode(sPrevNode, sNextRID, aMtx) != IDE_SUCCESS );
    
    // NEXTNODE의 prev 노드를 PREVNODE로 설정
    IDE_TEST( setPrvOfNode(sNextNode, sPrevRID, aMtx) != IDE_SUCCESS );

    // 노드 제거로 인한 base 노드의 리스트 개수 1 감소
    sNodeCnt = getNodeCnt(aBaseNode);
    IDE_ASSERT( sNodeCnt != 0 );
    setNodeCnt( aBaseNode, sNodeCnt - 1, aMtx );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
  리스트 내에서 SrcNode를 aDestNode 이후(NEXT)로 옮긴다.
*/
IDE_RC sdpDblRIDList::moveNodeInList( idvSQL             *  aStatistics,
                                      sdpDblRIDListBase  *  aBaseNode,
                                      sdpDblRIDListNode  *  aDestNode,
                                      sdpDblRIDListNode  *  aSrcNode,
                                      sdrMtx             *  aMtx )
{
    ULong  sNodeCnt;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx != NULL );
    IDE_DASSERT( aDestNode != NULL );
    IDE_DASSERT( aSrcNode != NULL );

    sNodeCnt = sdpDblRIDList::getNodeCnt( aBaseNode );
    
    IDE_TEST( removeNode( aStatistics,
                          aBaseNode,
                          aSrcNode,
                          aMtx ) != IDE_SUCCESS );
    
    IDE_TEST( insertNodeAfter( aStatistics,
                               aBaseNode,
                               aDestNode,
                               aSrcNode,
                               aMtx ) != IDE_SUCCESS );
    
    IDE_ASSERT( sNodeCnt == sdpDblRIDList::getNodeCnt( aBaseNode ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : base 노드의 length 설정 및 logging
 * base 노드의 list length을 설정한다.
 ***********************************************************************/
IDE_RC sdpDblRIDList::setNodeCnt(sdpDblRIDListBase*    aBaseNode,
                                 ULong                 aNodeCnt,
                                 sdrMtx*               aMtx)
{

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&aBaseNode->mNodeCnt,
                                        &aNodeCnt,
                                        ID_SIZEOF(aNodeCnt) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}


/***********************************************************************
 * Description : 노드의 prev RID 설정 및 logging
 ***********************************************************************/
IDE_RC sdpDblRIDList::setPrvOfNode( sdpDblRIDListNode  * aNode,
                                    sdRID                aPrevRID,
                                    sdrMtx             * aMtx )
{
    IDE_DASSERT( aNode    != NULL );
    IDE_DASSERT( aMtx     != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aNode->mPrev,
                                         &aPrevRID,
                                         ID_SIZEOF( aPrevRID ) )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

/***********************************************************************
 * Description : 노드의 next RID 설정 및 logging
 ***********************************************************************/
IDE_RC sdpDblRIDList::setNxtOfNode( sdpDblRIDListNode * aNode,
                                    sdRID               aNextRID,
                                    sdrMtx            * aMtx )
{

    IDE_DASSERT( aNode    != NULL );
    IDE_DASSERT( aMtx     != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aNode->mNext,
                                         &aNextRID,
                                         ID_SIZEOF( aNextRID ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 리스트의 모든 node 출력
 ***********************************************************************/
IDE_RC sdpDblRIDList::dumpList( scSpaceID aSpaceID,
                                sdRID     aBaseNodeRID )
{

    UInt                i;
    ULong               sNodeCount;
    sdrMtx              sMtx;
    UChar             * sPagePtr;
    idBool              sTrySuccess;
    sdpDblRIDListBase * sBaseNode;
    sdpDblRIDListNode * sNode;
    sdRID               sCurRID;
    sdRID               sBaseRID;
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

    IDE_TEST( sdbBufferMgr::getPageByRID( NULL, /* idvSQL* */
                                          aSpaceID,
                                          aBaseNodeRID, 
                                          SDB_S_LATCH,
                                          SDB_WAIT_NORMAL,
                                          &sMtx,
                                          (UChar**)&sBaseNode,
                                          &sTrySuccess ) != IDE_SUCCESS );

    sNodeCount  = getNodeCnt( sBaseNode );
    sCurRID     = getHeadOfList( sBaseNode );
    sBaseRID    = sdpPhyPage::getRIDFromPtr( (UChar*)&sBaseNode->mBase );

    for( i = 0; sCurRID != sBaseRID; i++ )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL,
                                              aSpaceID,
                                              SD_MAKE_PID(sCurRID),
                                              &sPagePtr,
                                              &sTrySuccess ) != IDE_SUCCESS );
        sState = 2;

        sNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sCurRID ) );

        idlOS::printf("%"ID_UINT32_FMT"/%"ID_UINT32_FMT" : sid %"ID_UINT32_FMT", pid %"ID_UINT32_FMT", offset %"ID_UINT32_FMT"\n",
                      i,
                      sNodeCount,
                      aSpaceID,
                      SD_MAKE_PID( sCurRID ),
                      SD_MAKE_OFFSET( sCurRID ));

        sCurRID = getNxtOfNode( sNode );

        sState = 1;
        IDE_TEST( sdbBufferMgr::unfixPage( NULL, /* idvSQL* */
                                           sPagePtr )
                  != IDE_SUCCESS );

    }

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

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
 * Description : from 노드부터 tail까지 한번에 제거
 *
 * + 2nd. code design
 *   - 삭제할 f노드의 rid를 구한다.
 *   - 삭제할 f노드의의 f-PREVNODE의 RID를 얻는다
 *   - 삭제할 t노드의 rid를 구한다.
 *   - 삭제할 t노드의의 t-NEXTNODE의 RID를 얻는다
 *   - if PREVNODE == 삭제할 노드 then
 *     :직접 PREVNODE의 ptr을 얻는다.
 *   - else
 *     :PREVNODE를 fix 요청 
 *   - PREVNODE의 next 노드를 NEXTNODE로 설정(SDR_8BYTES)
 *   - if NEXTNODE == 삭제할 노드 then
 *     :직접 NEXTNODE의 ptr을 얻는다.
 *   - else
 *     :NEXTNODE를 fix 요청
 *   -  NEXTNODE의 prevnode를 PREVNODE로 설정 (SDR_8BYTES)
 *   - basenode의 길이를 구해서 1 감소 (SDR_4BYTES)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 ***********************************************************************/
IDE_RC sdpDblRIDList::removeNodesAtOnce( idvSQL            * aStatistics,
                                         sdpDblRIDListBase * aBaseNode,
                                         sdpDblRIDListNode * aFromNode,
                                         sdpDblRIDListNode * aToNode,
                                         ULong               aNodeCount,
                                         sdrMtx            * aMtx )
{
    scSpaceID            sSpaceID;
    ULong                sNodeCnt;
    idBool               sTrySuccess;
    sdRID                sFRID;
    sdRID                sTNextRID;
    sdpDblRIDListNode  * sTNextNode;
    sdRID                sFPrevRID;
    sdpDblRIDListNode  * sFPrevNode;
    UChar              * sPagePtr;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aFromNode != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNodeCount != 0 );

    IDE_DASSERT(getNodeCnt(aBaseNode) > 0);

    sSpaceID = sdpPhyPage::getSpaceID(
        sdpPhyPage::getPageStartPtr( (UChar*) aBaseNode ) );

    if (aNodeCount > 1)
    {
        IDE_ASSERT( aFromNode != aToNode );

        sFPrevRID = sdpDblRIDList::getPrvOfNode(aFromNode);

        IDE_ASSERT(sFPrevRID != SD_NULL_RID);

        // 삭제할 f노드의 rid를 구한다.
        sFRID     = sdpPhyPage::getRIDFromPtr((UChar*)aFromNode);
        IDE_ASSERT(sFRID != SD_NULL_RID);

        // 제거할 t노드의 NEXTNODE를 얻는다.
        sTNextRID = getNxtOfNode(aToNode);

        // t-NEXTNODE fix 시킨다.
        if( isSamePage( &sTNextRID, &sFRID ) == ID_TRUE )
        {
            sTNextNode = (sdpDblRIDListNode*)( sdpPhyPage::getPageStartPtr( (UChar*)aFromNode )
                                               + SD_MAKE_OFFSET( sTNextRID ) );
        }
        else
        {
            sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                           sSpaceID,
                                                           SD_MAKE_PID( sTNextRID) );

            if( sPagePtr == NULL )
            {
                IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                      sSpaceID,
                                                      sTNextRID,
                                                      SDB_X_LATCH,
                                                      SDB_WAIT_NORMAL,
                                                      aMtx,
                                                      (UChar**)&sTNextNode,
                                                      &sTrySuccess ) != IDE_SUCCESS );
            }
            else
            {
                sTNextNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sTNextRID ) );
            }
        }

        IDE_DASSERT( sTNextNode != NULL );

        // f-PREVNODE fix 시킨다.
        if( isSamePage( &sFPrevRID, &sFRID ) == ID_TRUE )
        {
            sFPrevNode = (sdpDblRIDListNode*)( sdpPhyPage::getPageStartPtr( (UChar*)aFromNode )
                                               + SD_MAKE_OFFSET( sFPrevRID ));
        }
        else
        {
            sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                           sSpaceID,
                                                           SD_MAKE_PID( sFPrevRID) );

            if( sPagePtr == NULL )
            {
                IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                      sSpaceID,
                                                      sFPrevRID,
                                                      SDB_X_LATCH,
                                                      SDB_WAIT_NORMAL,
                                                      aMtx,
                                                      (UChar**)&sFPrevNode,
                                                      &sTrySuccess )
                      != IDE_SUCCESS );
            }
            else
            {
                sFPrevNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sFPrevRID ));
            }
        }

        IDE_DASSERT( sFPrevNode != NULL );

        // t-NEXTNODE의 prev 노드를 base 노드로 설정
        IDE_TEST( setPrvOfNode(sTNextNode, sFPrevRID, aMtx) != IDE_SUCCESS );
        // base 노드의 prev 노드를 t-NEXTNODE로 설정
        IDE_TEST( setNxtOfNode(sFPrevNode, sTNextRID, aMtx) != IDE_SUCCESS );
        // t노드의 next 노드를 SD_NULL_RID로 설정
        IDE_TEST( setNxtOfNode(aToNode, SD_NULL_RID, aMtx) != IDE_SUCCESS );
        // f노드의 prev 노드를 SD_NULL_RID로 설정
        IDE_TEST( setPrvOfNode(aFromNode, SD_NULL_RID, aMtx) != IDE_SUCCESS );

        // 노드 제거로 인한 base 노드의 리스트 개수 aNodeCount 감소
        sNodeCnt = getNodeCnt(aBaseNode);
        IDE_DASSERT( sNodeCnt != 0 );
        IDE_DASSERT( sNodeCnt >= aNodeCount );
        setNodeCnt(aBaseNode, (sNodeCnt - aNodeCount), aMtx);
    }
    else
    {
        IDE_DASSERT( aFromNode == aToNode );

        IDE_TEST( removeNode(aStatistics,
                             aBaseNode, aFromNode, aMtx) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : rid list를 tail에 추가
 *
 * + 2nd. code design
 *   - base 노드의 RID을 얻는다.
 *   - newnode의 RID를 얻는다.
 *   - base 노드의 PREVNODE를 얻는다.
 *   - newnode의 prev 노드를 PREVNODE로 설정 (SDR_8BYTES)
 *   - newnode의 next 노드를 base 노드로 설정 (SDR_8BYTES)
 *   - if newnode와 PREVNODE와 동일한 page인 경우
 *     : 직접 PREVNODE의 ptr을 얻는다.
 *   - else 
 *     : PREVNODE를 버퍼관리자에 요청하여 fix시킨후,
 *       포인터 얻음
 *     => PREVNODE의 next 노드를 newnode로 설정 (SDR_8BYTE)
 *   - base 노드의 prev 노드를 newnode로 설정(SDR_8BYTE)
 *   - basenode의 길이를 구해서 +1 (SDR_4BYTE)
 *
 * - 주의
 *   1) 인자가 모두 not null 확인
 *   2) node != newnode 확인
 ***********************************************************************/
IDE_RC sdpDblRIDList::insertNodesAtOnce(idvSQL               * aStatistics,
                                        sdpDblRIDListBase    * aBaseNode,
                                        sdpDblRIDListNode    * aFromNode,
                                        sdpDblRIDListNode    * aToNode,
                                        ULong                  aNodeCount,
                                        sdrMtx               * aMtx)
{

    scSpaceID             sSpaceID;
    sdRID                 sBaseRID;
    sdRID                 sNewFromRID;
    sdRID                 sNewToRID;
    sdRID                 sTailRID;
    ULong                 sNodeCnt;
    idBool                sTrySuccess;
    sdpDblRIDListNode   * sTailNode;
    UChar               * sPagePtr;

    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aFromNode != NULL );
    IDE_DASSERT( aToNode   != NULL );
    IDE_DASSERT( aMtx      != NULL );
    IDE_DASSERT( aNodeCount != 0 );

    sSpaceID = sdpPhyPage::getSpaceID( sdpPhyPage::getPageStartPtr( (UChar*)aBaseNode ));

    if( aNodeCount > 1 )
    {
        IDE_DASSERT( aFromNode != aToNode );

        sBaseRID    = sdpPhyPage::getRIDFromPtr( (UChar*)&aBaseNode->mBase );
        sNewFromRID = sdpPhyPage::getRIDFromPtr( (UChar*)aFromNode );
        sNewToRID   = sdpPhyPage::getRIDFromPtr( (UChar*)aToNode );

        // base 노드의 tail 노드(prev RID)를 얻는다.
        sTailRID = getTailOfList( aBaseNode );

        // 동일한 page인 경우
        if( isSamePage( &sNewToRID, &sTailRID ) == ID_TRUE ) 
        {
            sTailNode = (sdpDblRIDListNode*)(sdpPhyPage::getPageStartPtr( (UChar*)aToNode )
                                             + SD_MAKE_OFFSET( sTailRID ));
        }
        else
        {
            sPagePtr = sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                           sSpaceID,
                                                           SD_MAKE_PID( sTailRID) );

            if( sPagePtr == NULL )
            {
                // PREVNODE를 fix 시킨다.
                IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                      sSpaceID,
                                                      sTailRID,
                                                      SDB_X_LATCH,
                                                      SDB_WAIT_NORMAL,
                                                      aMtx,
                                                      (UChar**)&sTailNode,
                                                      &sTrySuccess )
                      != IDE_SUCCESS );
            }
            else
            {
                sTailNode = (sdpDblRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sTailRID ));

            }
        }
        IDE_DASSERT( sTailNode != NULL );

        IDE_TEST( setNxtOfNode( sTailNode, sNewFromRID, aMtx ) != IDE_SUCCESS );
        IDE_TEST( setPrvOfNode( &aBaseNode->mBase, sNewToRID, aMtx) != IDE_SUCCESS );

        IDE_TEST( setNxtOfNode( aToNode, sBaseRID, aMtx ) != IDE_SUCCESS );
        IDE_TEST( setPrvOfNode( aFromNode, sTailRID, aMtx ) != IDE_SUCCESS );

        // 새로운 노드 추가로 인한 base 노드의 리스트 개수 1 증가
        sNodeCnt = getNodeCnt( aBaseNode );
        IDE_TEST( setNodeCnt(aBaseNode, ( sNodeCnt + aNodeCount ), aMtx )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( aFromNode == aToNode );

        IDE_TEST( insertTailNode( aStatistics,
                                  aBaseNode, aFromNode, aMtx )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

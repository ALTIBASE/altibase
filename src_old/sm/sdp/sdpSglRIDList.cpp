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
 * $Id: sdpSglRIDList.cpp 24150 2007-11-14 06:36:46Z bskim $
 *
 * Description :
 *
 * File-Based RID Linked-List 구현
 *
 **********************************************************************/

#include <sdr.h>
#include <sdpSglRIDList.h>
#include <sdpPhyPage.h>

/***********************************************************************
 * Description : RID list의 Base 노드를 초기화
 *
 * + 2nd.code desgin
 *   - base 노드의 rid를 얻는다.
 *   - base 노드의 mNodeCnt를 0로 설정    (SDR_4BYTES)
 *   - base 노드의 mHead를 self RID를 설정(SDR_8BYTES)
 *   - base 노드의 mTail를 self RID를 설정(SDR_8BYTES)
 ***********************************************************************/
IDE_RC sdpSglRIDList::initList( sdpSglRIDListBase*    aBaseNode,
                                sdrMtx*               aMtx )
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    // Set Node Cnt
    IDE_TEST( setNodeCnt( aBaseNode, 0, aMtx ) != IDE_SUCCESS );

    // Set Head-Node
    IDE_TEST( setHeadOfList( aBaseNode, SD_NULL_RID, aMtx )
              != IDE_SUCCESS );

    // Set Tail-Node
    IDE_TEST( setTailOfList( aBaseNode, SD_NULL_RID, aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSglRIDList::initList( sdpSglRIDListBase*    aBaseNode,
                                sdRID                 aHeadRID,
                                sdRID                 aTailRID,
                                ULong                 aNodeCnt,
                                sdrMtx*               aMtx )
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    // Set Node Cnt
    IDE_TEST( setNodeCnt( aBaseNode, aNodeCnt, aMtx ) != IDE_SUCCESS );

    // Set Head-Node
    IDE_TEST( setHeadOfList( aBaseNode, aHeadRID, aMtx )
              != IDE_SUCCESS );

    // Set Tail-Node
    IDE_TEST( setTailOfList( aBaseNode, aTailRID, aMtx )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSglRIDList::initNode( sdpSglRIDListNode   * aNode,
                                sdrMtx              * aMtx )
{
    IDE_TEST( setNxtOfNode( aNode, SD_NULL_RID, aMtx )
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
IDE_RC sdpSglRIDList::addNode2Head( sdpSglRIDListBase   * aBaseNode,
                                    sdpSglRIDListNode   * aNewNode,
                                    sdrMtx              * aMtx )
{
    sdRID sRID;

    validate( aBaseNode );
    
    sRID = sdpPhyPage::getRIDFromPtr( aNewNode );

    IDE_TEST( addListToHead( aBaseNode,
                             sRID,
                             sRID,
                             aNewNode,
                             1,
                             aMtx ) != IDE_SUCCESS );

    validate( aBaseNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  RID list의 tail에 노드를 추가
 *
 * + 2nd. code design
 *   - sdpSglRIDList::insertNodeBefore를 호출함
 ***********************************************************************/
IDE_RC sdpSglRIDList::addNode2Tail( idvSQL             * aStatistics,
                                    scSpaceID            aSpaceID,
                                    sdpSglRIDListBase  * aBase,
                                    sdRID                aNodeRID,
                                    sdrMtx             * aMtx )
{
    idBool             sTrySuccess;
    sdpSglRIDListNode* sTailNode;

    IDE_DASSERT( aBase     != NULL );
    IDE_DASSERT( aMtx      != NULL );

    validate( aBase );

    if( aBase->mTail == SD_NULL_RID )
    {
        IDE_ASSERT( aBase->mHead == SD_NULL_PID );

        IDE_TEST( setHeadOfList( aBase, aNodeRID, aMtx )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Tail의 Next RID를 aBase->mTail로 설정한다. */
        IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                              aSpaceID,
                                              aBase->mTail,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              aMtx,
                                              (UChar**)&sTailNode,
                                              &sTrySuccess ) != IDE_SUCCESS );

        IDE_TEST( setNxtOfNode( sTailNode, aNodeRID, aMtx ) != IDE_SUCCESS );
    }

    IDE_TEST( setTailOfList( aBase, aNodeRID, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, getNodeCnt( aBase ) + 1, aMtx )
              != IDE_SUCCESS );

    validate( aBase );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  RID list의 tail에 노드를 추가
 *
 * + 2nd. code design
 *   - sdpSglRIDList::insertNodeBefore를 호출함
 ***********************************************************************/
IDE_RC sdpSglRIDList::addNode2Tail( idvSQL             * aStatistics,
                                    scSpaceID            aSpaceID,
                                    sdpSglRIDListBase  * aBase,
                                    sdpSglRIDListNode  * aNewNode,
                                    sdrMtx             * aMtx )
{
    UChar*             sNewPagePtr;
    sdRID              sNewRID;
    idBool             sTrySuccess;
    sdpSglRIDListNode* sTailNode;

    IDE_DASSERT( aBase     != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    validate( aBase );
    
    sNewPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aNewNode );
    sNewRID     = sdpPhyPage::getRIDFromPtr( (UChar*)aNewNode );

    /* set next of new node to be Null,
     * because the next of tail must be Null */
    IDE_TEST( setNxtOfNode( aNewNode, SD_NULL_RID, aMtx )
              != IDE_SUCCESS );

    if( aBase->mTail == SD_NULL_RID )
    {
        IDE_ASSERT( aBase->mHead == SD_NULL_PID );

        IDE_TEST( setHeadOfList( aBase, sNewRID, aMtx )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Tail Node가 추가되는 Node와 동일한 페이지에 존재한다면 */
        if( isSamePage( &sNewRID, &(aBase->mTail) ) == ID_TRUE )
        {
            sTailNode = (sdpSglRIDListNode*)( sNewPagePtr +
                                              SD_MAKE_OFFSET( aBase->mTail ) );
        }
        else
        {
            /* Tail의 Next RID를 aBase->mTail로 설정한다. */
            IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                                  aSpaceID,
                                                  aBase->mTail,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  aMtx,
                                                  (UChar**)&sTailNode,
                                                  &sTrySuccess ) != IDE_SUCCESS );
        }

        IDE_TEST( setNxtOfNode( sTailNode, sNewRID, aMtx ) != IDE_SUCCESS );
    }

    IDE_TEST( setTailOfList( aBase, sNewRID, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, getNodeCnt( aBase ) + 1, aMtx )
              != IDE_SUCCESS );

    validate( aBase );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 리스트의 Head에 List를 추가한다.
 ***********************************************************************/
IDE_RC sdpSglRIDList::addListToHead( sdpSglRIDListBase  * aBase,
                                     sdRID                aFstRID,
                                     sdRID                aLstRID,
                                     sdpSglRIDListNode  * aLstNode,
                                     ULong                aItemCnt,
                                     sdrMtx             * aMtx )
{
    IDE_DASSERT( aBase != NULL );
    IDE_DASSERT( aMtx      != NULL );

    validate( aBase );
    
    if( aBase->mHead == SD_NULL_RID )
    {
        IDE_ASSERT( aBase->mTail == SD_NULL_RID );

        IDE_TEST( setTailOfList( aBase,
                                 aLstRID,
                                 aMtx) != IDE_SUCCESS );
    }

    /* set next of new node to be Null,
     * because the next of tail must be Null */
    IDE_TEST( setNxtOfNode( aLstNode, aBase->mHead, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setHeadOfList( aBase, aFstRID, aMtx )
              != IDE_SUCCESS );

    /* Node갯수를 aItemCnt 증가 시킨다. */
    IDE_TEST( setNodeCnt( aBase, getNodeCnt( aBase ) + aItemCnt, aMtx )
              != IDE_SUCCESS );

    validate( aBase );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSglRIDList::removeNodeAtHead( idvSQL             * aStatistics,
                                        scSpaceID            aSpaceID,
                                        sdpSglRIDListBase  * aBase,
                                        sdrMtx             * aMtx,
                                        sdRID              * aRemovedRID,
                                        sdpSglRIDListNode ** aRemovedListNode )
{
    sdRID              sHeadRID;
    sdpSglRIDListNode* sHeadNode;

    validate( aBase );

    IDE_ASSERT( aRemovedListNode != NULL );

    IDE_ASSERT( getNodeCnt( aBase ) > 0 );

    *aRemovedRID      = SD_NULL_RID;
    *aRemovedListNode = NULL;

    sHeadRID    = getHeadOfList( aBase );

    IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                          aSpaceID,
                                          sHeadRID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          aMtx,
                                          (UChar**)&sHeadNode )
              != IDE_SUCCESS );

    if( aBase->mHead == aBase->mTail )
    {
        IDE_ASSERT( getNodeCnt( aBase ) == 1 );
        IDE_ASSERT( sHeadNode->mNext == SD_NULL_RID );

        IDE_TEST( setTailOfList( aBase,
                                 SD_NULL_RID,
                                 aMtx )
                  != IDE_SUCCESS );
    }

    IDE_TEST( setHeadOfList( aBase, sHeadNode->mNext, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, getNodeCnt( aBase ) - 1, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNxtOfNode( sHeadNode, SD_NULL_RID, aMtx )
              != IDE_SUCCESS );

    *aRemovedListNode = sHeadNode;
    *aRemovedRID      = sHeadRID;

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 리스트의 Head에 List를 추가한다.
 ***********************************************************************/
IDE_RC sdpSglRIDList::addListToTail( idvSQL             * aStatistics,
                                     sdpSglRIDListBase  * aBase,
                                     scSpaceID            aSpaceID,
                                     sdRID                aFstRID,
                                     sdRID                aLstRID,
                                     ULong                aItemCnt,
                                     sdrMtx             * aMtx )
{
    sdpSglRIDListNode *sTailNode;

    validate( aBase );

    IDE_DASSERT( aBase != NULL );
    IDE_DASSERT( aMtx      != NULL );

    IDE_DASSERT( validateNxtIsNULL( aStatistics,
                                    aSpaceID,
                                    aLstRID )
                 == IDE_SUCCESS );

    if( aBase->mTail == SD_NULL_RID )
    {
        IDE_ASSERT( aBase->mHead == SD_NULL_RID );

        IDE_TEST( setHeadOfList( aBase,
                                 aFstRID,
                                 aMtx) != IDE_SUCCESS );
    }
    else
    {
        /* Tail의 Next RID를 aBase->mTail로 설정한다. */
        IDE_TEST( sdbBufferMgr::getPageByRID( aStatistics,
                                              aSpaceID,
                                              aBase->mTail,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              aMtx,
                                              (UChar**)&sTailNode )
                  != IDE_SUCCESS );

        IDE_TEST( setNxtOfNode( sTailNode, aFstRID, aMtx ) != IDE_SUCCESS );
    }

    IDE_TEST( setTailOfList( aBase, aLstRID, aMtx )
              != IDE_SUCCESS );

    /* Node갯수를 aItemCnt 증가 시킨다. */
    IDE_TEST( setNodeCnt( aBase, getNodeCnt( aBase ) + aItemCnt, aMtx )
              != IDE_SUCCESS );

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 리스트의 모든 node 출력
 ***********************************************************************/
IDE_RC sdpSglRIDList::dumpList( scSpaceID aSpaceID,
                                sdRID     aBaseNodeRID )
{
    UInt                i;
    ULong               sNodeCount;
    sdrMtx              sMtx;
    UChar             * sPagePtr;
    idBool              sTrySuccess;
    sdpSglRIDListBase * sBaseNode;
    sdpSglRIDListNode * sNode;
    sdRID               sCurRID;
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

    for( i = 0; sCurRID != SD_NULL_RID; i++ )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL,
                                              aSpaceID,
                                              SD_MAKE_PID(sCurRID),
                                              &sPagePtr,
                                              &sTrySuccess ) != IDE_SUCCESS );
        sState = 2;

        sNode = (sdpSglRIDListNode*)( sPagePtr + SD_MAKE_OFFSET( sCurRID ) );

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

    IDE_ASSERT( i == sNodeCount );
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

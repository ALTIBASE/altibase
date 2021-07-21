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
 * $Id: sdpSglPIDList.cpp 25608 2008-04-11 09:52:13Z kclee $
 *
 * Description :
 *
 * File-Based PID Single Linked-List 구현
 *
 **********************************************************************/

#include <sdr.h>
#include <sdpSglPIDList.h>
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
IDE_RC sdpSglPIDList::initList( sdpSglPIDListBase*  aBase,
                                sdrMtx*             aMtx )
{
    IDE_DASSERT( aBase != NULL );
    IDE_DASSERT( aMtx      != NULL );

    /* set item count */
    IDE_TEST( setNodeCnt( aBase, 0, aMtx ) != IDE_SUCCESS );

    /* 초기에 PID List의 Head,Tail은 SD_NULL_PID 로 설정된다. */
    IDE_TEST( setHeadOfList( aBase, SD_NULL_PID,
                             aMtx) != IDE_SUCCESS );

    IDE_TEST( setTailOfList( aBase, SD_NULL_PID,
                             aMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Page List의 head에 노드를 추가
 ***********************************************************************/
IDE_RC sdpSglPIDList::addNode2Head( sdpSglPIDListBase   * aBase,
                                    sdpSglPIDListNode   * aNewNode,
                                    sdrMtx              * aMtx )
{
    UChar*      sNewPagePtr;
    scPageID    sNewPID;

    IDE_DASSERT( aBase != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    validate( aBase );

    sNewPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aNewNode );
    sNewPID     = sdpPhyPage::getPageID( sNewPagePtr );

    IDE_DASSERT( aBase->mHead != sNewPID );
    
    if( aBase->mHead == SD_NULL_PID )
    {
        IDE_ASSERT( aBase->mTail == SD_NULL_PID );

        IDE_TEST( setTailOfList( aBase,
                                 sNewPID,
                                 aMtx) != IDE_SUCCESS );
    }

    /* set next of new node to be Null,
     * because the next of tail must be Null */
    IDE_TEST( setNxtOfNode( aNewNode, aBase->mHead, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setHeadOfList( aBase, sNewPID, aMtx)
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, ( aBase->mNodeCnt + 1 ), aMtx)
              != IDE_SUCCESS );

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  Page List의 tail에 노드를 추가
 ***********************************************************************/
IDE_RC sdpSglPIDList::addNode2Tail( idvSQL             * aStatistics,
                                    sdpSglPIDListBase  * aBase,
                                    sdpSglPIDListNode  * aNewNode,
                                    sdrMtx             * aMtx )
{
    UChar*         sNewPagePtr;
    UChar*         sTailPagePtr;
    scPageID       sNewPID;
    idBool         sTrySuccess;
    sdpSglPIDListNode* sTailNode;
    scSpaceID      sSpaceID;

    IDE_DASSERT( aBase     != NULL );
    IDE_DASSERT( aNewNode  != NULL );
    IDE_DASSERT( aMtx      != NULL );

    validate( aBase );

    sNewPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aNewNode );
    sSpaceID    = sdpPhyPage::getSpaceID( (UChar*)sNewPagePtr );
    sNewPID     = sdpPhyPage::getPageID( sNewPagePtr );

    /* set next of new node to be Null,
     * because the next of tail must be Null */
    IDE_TEST( setNxtOfNode( aNewNode, SD_NULL_PID, aMtx )
              != IDE_SUCCESS );

    if( aBase->mTail == SD_NULL_PID )
    {
        IDE_ASSERT( aBase->mHead == SD_NULL_PID );

        IDE_TEST( setHeadOfList( aBase, sNewPID, aMtx )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Tail의 Next PID를 aBase->mTail로 설정한다. */
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              sSpaceID,
                                              aBase->mTail,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sTailPagePtr,
                                              &sTrySuccess,
                                              NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

        sTailNode = sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*) sTailPagePtr );

        IDE_TEST( setNxtOfNode( sTailNode, sNewPID, aMtx ) != IDE_SUCCESS );
    }

    IDE_TEST( setTailOfList( aBase, sNewPID, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, ( aBase->mNodeCnt + 1 ), aMtx)
              != IDE_SUCCESS );

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  Page List의 Head에 노드를 추가
 ***********************************************************************/
IDE_RC sdpSglPIDList::addList2Head( sdpSglPIDListBase  * aBase,
                                    sdpSglPIDListNode  * aFstNode,
                                    sdpSglPIDListNode  * aLstNode,
                                    ULong               aItemCnt,
                                    sdrMtx             * aMtx )
{
    UChar*             sFstPagePtr;
    UChar*             sLstPagePtr;
    scPageID           sFstPageID;
    scPageID           sLstPageID;

    validate( aBase );

    sFstPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aFstNode );
    sFstPageID  = sdpPhyPage::getPageID( sFstPagePtr );

    sLstPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aLstNode );
    sLstPageID  = sdpPhyPage::getPageID( sLstPagePtr );

    if( aBase->mHead == SD_NULL_PID )
    {
        IDE_ASSERT( aBase->mTail == SD_NULL_PID );

        IDE_TEST( setTailOfList( aBase, sLstPageID, aMtx )
                  != IDE_SUCCESS );
    }

    IDE_TEST( setNxtOfNode( aLstNode, aBase->mHead, aMtx ) != IDE_SUCCESS );

    IDE_TEST( setHeadOfList( aBase, sFstPageID, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, ( aBase->mNodeCnt + aItemCnt ), aMtx)
              != IDE_SUCCESS );

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  page list의 tail에 Node List를 추가
 ***********************************************************************/
IDE_RC sdpSglPIDList::addList2Tail( idvSQL             * aStatistics,
                                    sdpSglPIDListBase  * aBase,
                                    sdpSglPIDListNode  * aFstNode,
                                    sdpSglPIDListNode  * aLstNode,
                                    ULong                aItemCnt,
                                    sdrMtx             * aMtx )
{
    UChar*         sFstPagePtr;
    UChar*         sLstPagePtr;
    UChar*         sTailPagePtr;
    UChar*         sBasePagePtr;
    idBool         sTrySuccess;
    sdpSglPIDListNode* sTailNode;
    scSpaceID      sSpaceID;
    scPageID       sFstPageID;
    scPageID       sLstPageID;

    validate( aBase );

    sBasePagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aBase );
    sSpaceID     = sdpPhyPage::getSpaceID( (UChar*)sBasePagePtr );

    sFstPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aFstNode );
    sFstPageID  = sdpPhyPage::getPageID( sFstPagePtr );

    sLstPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aLstNode );
    sLstPageID  = sdpPhyPage::getPageID( sLstPagePtr );

    if( aBase->mTail == SD_NULL_PID )
    {
        IDE_ASSERT( aBase->mHead == SD_NULL_PID );

        IDE_TEST( setHeadOfList( aBase, sFstPageID, aMtx )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Tail의 Next PID를 aBase->mTail로 설정한다. */
        IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                              sSpaceID,
                                              aBase->mTail,
                                              SDB_X_LATCH,
                                              SDB_WAIT_NORMAL,
                                              SDB_SINGLE_PAGE_READ,
                                              aMtx,
                                              &sTailPagePtr,
                                              &sTrySuccess,
                                              NULL /*IsCorruptPage*/ ) != IDE_SUCCESS );

        sTailNode = sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*) sTailPagePtr );

        IDE_TEST( setNxtOfNode( sTailNode, sFstPageID, aMtx ) != IDE_SUCCESS );

    }

    IDE_TEST( setTailOfList( aBase, sLstPageID, aMtx )
              != IDE_SUCCESS );

    IDE_TEST( setNodeCnt( aBase, ( aBase->mNodeCnt + aItemCnt ), aMtx)
              != IDE_SUCCESS );

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSglPIDList::removeNodeAtHead( sdpSglPIDListBase  * aBase,
                                        UChar              * aHeadPagePtr,
                                        sdrMtx             * aMtx,
                                        scPageID           * aRemovedPID,
                                        sdpSglPIDListNode ** aRemovedListNode)
{
    scPageID           sHeadPID;
    sdpSglPIDListNode* sHeadNode;

    IDE_ASSERT( aRemovedListNode != NULL );

    IDE_ASSERT( getNodeCnt( aBase ) > 0 );

    validate( aBase );

    *aRemovedPID      = SD_NULL_RID;
    *aRemovedListNode = NULL;

    sHeadPID    = getHeadOfList( aBase );
    sHeadNode   = sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*) aHeadPagePtr );

    if( aBase->mHead == aBase->mTail )
    {
        IDE_ASSERT( getNodeCnt( aBase ) == 1 );
        IDE_ASSERT( sHeadNode->mNext == SD_NULL_PID );

        IDE_TEST( setTailOfList( aBase,
                                 SD_NULL_PID,
                                 aMtx ) != IDE_SUCCESS );
    }

    /* Head의 Next를 널로 하기전에 Base의 Head를 설정한다. */
    IDE_TEST( setHeadOfList( aBase, sHeadNode->mNext, aMtx )
              != IDE_SUCCESS );

    /* 제거될 Node의 Next를 NULL로 설정한다. */
    IDE_TEST( setNxtOfNode( sHeadNode, SD_NULL_PID, aMtx )
              != IDE_SUCCESS );

    /* Node갯수를 1감소 시킨다. */
    IDE_TEST( setNodeCnt( aBase, getNodeCnt( aBase ) - 1, aMtx )
              != IDE_SUCCESS );

    *aRemovedListNode = sHeadNode;
    *aRemovedPID      = sHeadPID;

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC sdpSglPIDList::removeNodeAtHead( idvSQL             * aStatistics,
                                        sdpSglPIDListBase  * aBase,
                                        sdrMtx             * aMtx,
                                        scPageID           * aRemovedPID,
                                        sdpSglPIDListNode ** aRemovedListNode )
{
    scPageID            sHeadPID;
    scSpaceID           sSpaceID;
    UChar             * sNewPagePtr;
    UChar             * sPagePtr;

    IDE_ASSERT( aRemovedListNode != NULL );

    IDE_ASSERT( getNodeCnt( aBase ) > 0 );

    validate( aBase );

    *aRemovedPID      = SD_NULL_RID;
    *aRemovedListNode = NULL;

    sNewPagePtr = sdpPhyPage::getPageStartPtr( (UChar*)aBase );
    sSpaceID    = sdpPhyPage::getSpaceID( (UChar*)sNewPagePtr );

    sHeadPID    = getHeadOfList( aBase );

    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          sSpaceID,
                                          sHeadPID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          (UChar**)&sPagePtr,
                                          NULL /*TrySuccess*/,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );


    IDE_TEST( removeNodeAtHead( aBase,
                                sPagePtr,
                                aMtx,
                                aRemovedPID,
                                aRemovedListNode )
              != IDE_SUCCESS );

    IDE_ASSERT( *aRemovedPID != SD_NULL_PID );

    validate( aBase );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdpSglPIDList::removeNode( sdpSglPIDListBase  * aBase,
                                  UChar              * aPrvPagePtr,
                                  UChar              * aRmvPagePtr,
                                  sdrMtx             * aMtx,
                                  scPageID           * aRemovedPID,
                                  sdpSglPIDListNode ** aRemovedListNode)
{
    scPageID           sHeadPID;
    scPageID           sTailPID;
    sdpSglPIDListNode* sPrvNode;
    sdpSglPIDListNode* sRmvNode;
    ULong              sNodeCnt;
    scPageID           sRmvPID;
    scPageID           sPrvPID;

    validate( aBase );

    sHeadPID = getHeadOfList( aBase );
    sTailPID = getTailOfList( aBase );
    sNodeCnt = getNodeCnt( aBase );

    sRmvPID  = sdpPhyPage::getPageID( aRmvPagePtr );
    sRmvNode = sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*) aRmvPagePtr );

    if( aPrvPagePtr == NULL )
    {
        IDE_ASSERT( sHeadPID == sRmvPID );

        /* Head는 제거될 Node의 다음 노드. */
        IDE_TEST( setHeadOfList( aBase, sRmvNode->mNext, aMtx )
                  != IDE_SUCCESS );

        sPrvPID = SD_NULL_PID;
    }
    else
    {
        sPrvNode = sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*) aPrvPagePtr );
        /* 제거될 Node의 Prev Node의 Next를 제거될 Node의 Next로 */
        IDE_TEST( setNxtOfNode( sPrvNode, sRmvNode->mNext, aMtx )
                  != IDE_SUCCESS );

        sPrvPID  = sdpPhyPage::getPageID( aPrvPagePtr );
    }

    /* 제거될 Node의 Next를 NULL */
    IDE_TEST( setNxtOfNode( sRmvNode, SD_NULL_PID, aMtx )
              != IDE_SUCCESS );

    if( sTailPID == sRmvPID )
    {
        IDE_TEST( setTailOfList( aBase, sPrvPID, aMtx )
                  != IDE_SUCCESS );
    }

    IDE_TEST( setNodeCnt( aBase, sNodeCnt - 1, aMtx )
              != IDE_SUCCESS );

    *aRemovedPID = sRmvPID;
    *aRemovedListNode  = sRmvNode;

    validate( aBase );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aRemovedPID = SD_NULL_PID;
    *aRemovedListNode  = NULL;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : base 노드의 length 설정 및 logging
 * base 노드의 list length을 설정한다.
 ***********************************************************************/
IDE_RC sdpSglPIDList::setNodeCnt( sdpSglPIDListBase  * aBase,
                                  ULong                aNodeCnt,
                                  sdrMtx             * aMtx )
{
    IDE_DASSERT( aBase != NULL );
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&aBase->mNodeCnt,
                                        &aNodeCnt,
                                        ID_SIZEOF(aNodeCnt) ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 노드의 prev page ID 설정 및 logging 
 * aNode가 base 노드이면 tail 노드를 설정하고, 그렇지 않으면 next 노드를
 * 설정한다.
 ***********************************************************************/
IDE_RC sdpSglPIDList::setHeadOfList( sdpSglPIDListBase*  aBase,
                                     scPageID            aPageID,
                                     sdrMtx*             aMtx )
{
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&( aBase->mHead ),
                                         &aPageID,
                                         ID_SIZEOF(aPageID) ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 노드의 prev page ID 설정 및 logging
 ***********************************************************************/
IDE_RC sdpSglPIDList::setTailOfList( sdpSglPIDListBase*    aBase,
                                     scPageID              aPageID,
                                     sdrMtx*               aMtx )
{
    IDE_DASSERT( aMtx != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                        (UChar*)&( aBase->mTail ),
                                        &aPageID,
                                        ID_SIZEOF(aPageID) ) != IDE_SUCCESS );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 노드의 prev page ID 설정 및 logging
 ***********************************************************************/
IDE_RC sdpSglPIDList::setNxtOfNode(sdpSglPIDListNode*   aNode,
                                   scPageID          aPageID,
                                   sdrMtx*           aMtx)
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
IDE_RC sdpSglPIDList::dumpList( scSpaceID  aSpaceID,
                                sdRID      aBaseRID )
{
    UInt                i;
    ULong               sNodeCount;
    sdrMtx              sMtx;
    UChar             * sPagePtr;
    sdpSglPIDListBase * sBaseNode;
    idBool              sTrySuccess;
    scPageID            sCurPageID;
    scPageID            sBasePageID;
    SInt                sState = 0;

    IDE_ERROR( aBaseRID != SD_NULL_RID );

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
                                         aBaseRID, 
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         &sMtx,
                                         (UChar**)&sBaseNode,
                                         &sTrySuccess) != IDE_SUCCESS );

    sBasePageID = SD_MAKE_PID(aBaseRID);
    sNodeCount  = getNodeCnt(sBaseNode);
    sCurPageID  = getHeadOfList(sBaseNode);

    for ( i = 0; sCurPageID != sBasePageID; i++ )
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

        sCurPageID = getNxtOfNode( sdpPhyPage::getSglPIDListNode((sdpPhyPageHdr*)sPagePtr) );

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
IDE_RC sdpSglPIDList::dumpCheck( sdpSglPIDListBase* aBase,
                                 scSpaceID          aSpaceID,
                                 scPageID           aPageID )
{
    UInt       i;
    ULong      sNodeCount;
    UChar*     sPagePtr;
    idBool     sTrySuccess;
    scPageID   sCurPageID;
    UShort     sState = 0;

    IDE_ERROR( aBase != NULL );

    sNodeCount = getNodeCnt( aBase );
    sCurPageID = getHeadOfList( aBase );

    for ( i = 0;  i < sNodeCount; i++ )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID( NULL,
                                              aSpaceID,
                                              sCurPageID,
                                              &sPagePtr,
                                              &sTrySuccess) != IDE_SUCCESS );
        sState = 1;
        if( sCurPageID == aPageID )
        {
            IDE_ASSERT(0);
        }

        sCurPageID = getNxtOfNode(
            sdpPhyPage::getSglPIDListNode( (sdpPhyPageHdr*)sPagePtr ) );

        sState = 0;

        IDE_TEST( sdbBufferMgr::unfixPage( NULL, /* idvSQL* */
                                           sPagePtr )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0)
    {
        IDE_ASSERT( sdbBufferMgr::unfixPage( NULL, /* idvSQL* */
                                             sPagePtr)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

void sdpSglPIDList::validate( sdpSglPIDListBase * aBase )
{
    if( getNodeCnt( aBase ) == 1 )
    {
        IDE_ASSERT( aBase->mHead == aBase->mTail );
        IDE_ASSERT( aBase->mHead != SD_NULL_PID );
    }

    if( getNodeCnt( aBase ) == 0 )
    {
        IDE_ASSERT( aBase->mHead == aBase->mTail );
        IDE_ASSERT( aBase->mHead == SD_NULL_PID );
    }

    if( aBase->mHead == SD_NULL_PID )
    {
        IDE_ASSERT( aBase->mTail == SD_NULL_PID );
        IDE_ASSERT( getNodeCnt( aBase ) == 0 );
    }

    if( aBase->mTail == SD_NULL_PID )
    {
        IDE_ASSERT( aBase->mHead == SD_NULL_PID );
        IDE_ASSERT( getNodeCnt( aBase ) == 0 );
    }

    if( aBase->mHead == aBase->mTail )
    {
        if( aBase->mHead == SD_NULL_PID )
        {
            IDE_ASSERT( getNodeCnt( aBase ) == 0 );
        }
        else
        {
            IDE_ASSERT( getNodeCnt( aBase ) == 1 );
        }
    }

}

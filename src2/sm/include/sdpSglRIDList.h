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
 * $Id: sdpSglRIDList.h 22687 2007-07-23 01:01:41Z bskim $
 *
 * Description :
 *
 * File Based RID Linked-List 관리자
 *
 *
 * # 개념
 *
 * tablespace의 논리적인 저장구조를 관리하기 위한 
 * 자료구조이다.
 *
 * # 구조
 *  + tablespace에 저장되는 각종 논리적인 RID list 관리
 *      - tablespace의 ext.desc free list   (base)
 *      - segment desc의 ext.desc.full list (base)
 *      - segment desc의 ext.desc.free list (base)
 *      - extent desc의 ext.desc.list       (node)
 *
 *    뿐만아니라,
 *
 *      - TSS RID list (base)
 *      - used USH list (base)
 *      - free USH list (base)
 *
 * 
 * # 구조
 *                 sdpSglRIDListBase
 *                  ______________
 *                  |____len_____|        
 *          ________|tail_|_head_|________
 *         |    ____/        \_____      |
 *         |  _/_________   _______\___  |
 *         |  |next|prev|<--|next|prev|  |
 *         -->|____|____|-->|____|____|<--
 *
 *         sdpSglRIDListNode
 *
 * RID 리스트는 동일한 tablespace에서만 유지 관리가 가능하며, 위와같은
 * circular double linked-list의 구조를 유지한다. 리스트에 대한 변경연산은
 * base 노드를 fix한 상태에서 이루어져야한다.
 * sdpPIDList와 달리 동일한 page내에 리스트의 여러 노드가 존재할 수 있다.
 *
 * # Releate data structure
 *
 * sdpSglRIDListBase 구조체
 * sdpSglRIDListNode 구조체
 *
 **********************************************************************/

#ifndef _O_SDP_SGL_RIDLIST_H_
#define _O_SDP_SGL_RIDLIST_H_ 1

#include <smDef.h>
#include <sdpDef.h>
#include <sdpPhyPage.h>

class sdpSglRIDList
{
public:

    /* RID list의 base 노드를 초기화 */
    static IDE_RC initList( sdpSglRIDListBase*    aBaseNode,
                            sdrMtx*               aMtx );

    static IDE_RC initList( sdpSglRIDListBase* aBaseNode,
                            sdRID              aHeadRID,
                            sdRID              aTailRID,
                            ULong              aNodeCnt,
                            sdrMtx*            aMtx );

    /* RID list의 head에 노드를 추가 */
    static IDE_RC addNode2Head( sdpSglRIDListBase   * aBaseNode,
                                sdpSglRIDListNode   * aNewNode,
                                sdrMtx              * aMtx );

    static IDE_RC addNode2Tail( idvSQL             * aStatistics,
                                scSpaceID            aSpaceID,
                                sdpSglRIDListBase  * aBase,
                                sdRID                aNodeRID,
                                sdrMtx             * aMtx );

    /* RID list의 tail에 노드를 추가 */
    static IDE_RC addNode2Tail( idvSQL             * aStatistics,
                                scSpaceID            aSpaceID,
                                sdpSglRIDListBase  * aBaseNode,
                                sdpSglRIDListNode  * aNewNode,
                                sdrMtx             * aMtx );

    static IDE_RC addListToHead( sdpSglRIDListBase  * aBase,
                                 sdRID                aFstRID,
                                 sdRID                aLstRID,
                                 sdpSglRIDListNode  * aLstNode,
                                 ULong                aItemCnt,
                                 sdrMtx             * aMtx );

    static IDE_RC addListToTail( idvSQL             * aStatistics,
                                 sdpSglRIDListBase  * aBase,
                                 scSpaceID            aSpaceID,
                                 sdRID                aFstRID,
                                 sdRID                aLstRID,
                                 ULong                aItemCnt,
                                 sdrMtx             * aMtx );

    static IDE_RC removeNodeAtHead( idvSQL             * aStatistics,
                                    scSpaceID            aSpaceID,
                                    sdpSglRIDListBase  * aBase,
                                    sdrMtx             * aMtx,
                                    sdRID              * aRemovedRID,
                                    sdpSglRIDListNode ** aRemovedListNode );

    static IDE_RC initNode( sdpSglRIDListNode   * aNode,
                            sdrMtx              * aMtx );

    /* 리스트의 모든 노드 얻기 */
    static IDE_RC dumpList( scSpaceID aSpaceID,
                            sdRID     aBaseNodeRID );

    /* base 노드의 head 노드 얻기 */
    static inline sdRID getBaseNodeRID(sdpSglRIDListBase*   aBaseNode);

    /* base 노드의 length 얻기 */
    static inline ULong getNodeCnt(sdpSglRIDListBase*   aBaseNode);

    /* base 노드의 head 노드 얻기 */
    static inline sdRID getHeadOfList(sdpSglRIDListBase*   aBaseNode);

    /* base 노드의 tail 노드 얻기 */
    static inline sdRID getTailOfList(sdpSglRIDListBase*   aBaseNode);

    /* 노드의 next 노드 얻기 */
    static inline sdRID getNxtOfNode(sdpSglRIDListNode*   aNode);

    static inline IDE_RC setHeadOfList( sdpSglRIDListBase * aBaseNode,
                                        sdRID               aHead,
                                        sdrMtx            * aMtx );

    static inline IDE_RC setTailOfList( sdpSglRIDListBase * aBaseNode,
                                        sdRID                aTail,
                                        sdrMtx             * aMtx );

    /* base 노드의 length 설정 및 logging */
    static inline IDE_RC setNodeCnt( sdpSglRIDListBase*  aBaseNode,
                                     ULong               aNodeCnt,
                                     sdrMtx*             aMtx );

    /* 노드의 next 노드 설정 및 logging */
    static inline IDE_RC setNxtOfNode( sdpSglRIDListNode*  aNode,
                                       sdRID               aNextRID,
                                       sdrMtx*             aMtx );

private:
    // rid가 동일한 page에 존재하는지 검사
    inline static idBool isSamePage( sdRID*     aLhs,
                                     sdRID*     aRhs );

    inline static void validate( sdpSglRIDListBase * aBase );

    // Validate Function
    inline static IDE_RC validateNxtIsNULL( idvSQL    * aStatistics,
                                            scSpaceID   aSpaceID,
                                            sdRID       aRID );
};

/***********************************************************************
 * Description : base->mBase ptr를 rid로 변환하여 반환
 ***********************************************************************/
inline sdRID sdpSglRIDList::getBaseNodeRID(sdpSglRIDListBase*  aBaseNode)
{
    IDE_DASSERT( aBaseNode != NULL );
    return sdpPhyPage::getRIDFromPtr((UChar*)aBaseNode);
}

/***********************************************************************
 * Description : 동일한 Page내의 rid인지 검사
 ***********************************************************************/
inline idBool sdpSglRIDList::isSamePage( sdRID* aLhs, sdRID* aRhs )
{
    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    return ((SD_MAKE_PID(*aLhs) == SD_MAKE_PID(*aRhs))) ? ID_TRUE : ID_FALSE;
}

/***********************************************************************
 * Description : Base 노드의 length 반환
 ***********************************************************************/
inline ULong sdpSglRIDList::getNodeCnt( sdpSglRIDListBase*   aBaseNode )
{
    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mNodeCnt;
}

/***********************************************************************
 * Description : 노드의 next 노드 반환 혹은 base 노드의 tail 노드 반환
 ***********************************************************************/
inline sdRID sdpSglRIDList::getNxtOfNode( sdpSglRIDListNode*  aNode)
{
    IDE_DASSERT( aNode != NULL );

    return aNode->mNext;
}

/***********************************************************************
 * Description : base 노드의 head 노드 반환
 ***********************************************************************/
inline sdRID sdpSglRIDList::getHeadOfList( sdpSglRIDListBase*   aBaseNode )
{

    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mHead;
}

/***********************************************************************
 * Description : base 노드의 tail 노드 반환
 ***********************************************************************/
inline sdRID sdpSglRIDList::getTailOfList( sdpSglRIDListBase*   aBaseNode )
{
    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mTail;
}

/***********************************************************************
 * Description : base 노드의 length 설정 및 logging
 * base 노드의 list length을 설정한다.
 ***********************************************************************/
inline IDE_RC sdpSglRIDList::setNodeCnt( sdpSglRIDListBase* aBaseNode,
                                         ULong              aNodeCnt,
                                         sdrMtx*            aMtx )
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aBaseNode->mNodeCnt,
                                         &aNodeCnt,
                                         ID_SIZEOF( aNodeCnt ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 노드의 Head RID 설정 및 logging
 ***********************************************************************/
inline IDE_RC sdpSglRIDList::setHeadOfList( sdpSglRIDListBase * aBaseNode,
                                            sdRID               aHead,
                                            sdrMtx            * aMtx )
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aBaseNode->mHead,
                                         &aHead,
                                         ID_SIZEOF( aHead ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 노드의 Tail RID 설정 및 logging
 ***********************************************************************/
inline IDE_RC sdpSglRIDList::setTailOfList( sdpSglRIDListBase  * aBaseNode,
                                            sdRID                aTail,
                                            sdrMtx             * aMtx )
{
    IDE_DASSERT( aBaseNode != NULL );
    IDE_DASSERT( aMtx      != NULL );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&aBaseNode->mTail,
                                         &aTail,
                                         ID_SIZEOF( aTail ) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 노드의 next RID 설정 및 logging
 ***********************************************************************/
inline IDE_RC sdpSglRIDList::setNxtOfNode( sdpSglRIDListNode * aNode,
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

inline void sdpSglRIDList::validate( sdpSglRIDListBase * aBase )
{
    if( getNodeCnt( aBase ) == 1 )
    {
        IDE_ASSERT( aBase->mHead == aBase->mTail );
        IDE_ASSERT( aBase->mHead != SD_NULL_RID );
    }

    if( getNodeCnt( aBase ) == 0 )
    {
        IDE_ASSERT( aBase->mHead == aBase->mTail );
        IDE_ASSERT( aBase->mHead == SD_NULL_RID );
    }

    if( getNodeCnt( aBase ) != 0 )
    {
        IDE_ASSERT( aBase->mHead != SD_NULL_RID );
        IDE_ASSERT( aBase->mTail != SD_NULL_RID );
    }
}

inline IDE_RC sdpSglRIDList::validateNxtIsNULL( idvSQL    * aStatistics,
                                                scSpaceID   aSpaceID,
                                                sdRID       aRID )
{
    sdpSglRIDListNode *sNode;
    idBool             sDummy;

    IDE_TEST( sdbBufferMgr::fixPageByRID( aStatistics,
                                          aSpaceID,
                                          aRID,
                                          (UChar**)&sNode,
                                          &sDummy )
              != IDE_SUCCESS );

    IDE_ASSERT( sNode->mNext == SD_NULL_RID );

    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics,
                                       (UChar*)sNode )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

#endif // _O_SDP_SGL_RIDLIST_H_


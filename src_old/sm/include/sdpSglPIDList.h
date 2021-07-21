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
 * $Id: sdpSglPIDList.h 24150 2007-11-14 06:36:46Z bskim $
 *
 * Description :
 *
 * File Based PID Linked-list 관리자
 *
 *
 * # 개념
 *
 * 각종 tablespace의 논리적인 page list를 관리하기 위한
 * 인터페이스
 *
 * # 목적
 *
 *  + tablespace에 저장되는 각종 논리적인 page list 관리
 *
 *      - tablespace의 seg.dir full list (base)
 *      - tablespace의 seg.dir free list (base)
 *      - tablespace의 ext.dir full list (base)
 *      - segment dir의 seg.dir list     (node)
 *      - segment desc의 used page list  (base)
 *      - extent  dir의 ext.dir list     (node)
 *      - persistent page의 page list    (node)
 *
 *
 * # 구조
 *              sdpPageListBase
 *                  ______________
 *                  |____len_____|
 *          ________|tail_|_head_|________
 *         |    ____/        \_____      |
 *         |  _/_________   _______\___  |
 *         |  |next|prev|<--|next|prev|  |
 *         -->|____|____|-->|____|____|<--
 *
 *         sdpPageListNode
 *
 * PID 리스트는 동일한 tablespace에서만 유지 관리가 가능하며, 위와같은
 * circular double linked-list의 구조를 유지한다. 리스트에 대한 변경연산은
 * base 노드를 fix한 상태에서 이루어져야한다.
 * 동일한 page내에 리스트의 여러 노드가 존재할 수 없다(ASSERT 처리)
 *
 * # Releate data structure
 *
 * sdpSglPIDListBase 구조체
 * sdpSglPIDListNode 구조체
 *
 **********************************************************************/

#ifndef _O_SDP_SGL_PIDLIST_H_
#define _O_SDP_SGL_PIDLIST_H_ 1

#include <sdr.h>
#include <sdpDef.h>

class sdpSglPIDList
{
public:
    static IDE_RC initList( sdpSglPIDListBase*  aBase,
                            sdrMtx*             aMtx );

    static IDE_RC addNode2Head( sdpSglPIDListBase   * aBase,
                                sdpSglPIDListNode   * aNewNode,
                                sdrMtx              * aMtx );

    static IDE_RC addList2Head( sdpSglPIDListBase  * aBase,
                                sdpSglPIDListNode  * aFstNode,
                                sdpSglPIDListNode  * aLstNode,
                                ULong               aItemCnt,
                                sdrMtx             * aMtx);

    static IDE_RC addNode2Tail( idvSQL             * aStatistics,
                                sdpSglPIDListBase  * aBase,
                                sdpSglPIDListNode  * aNewNode,
                                sdrMtx             * aMtx );

    static IDE_RC addList2Tail( idvSQL             * aStatistics,
                                sdpSglPIDListBase  * aBase,
                                sdpSglPIDListNode  * aFstNode,
                                sdpSglPIDListNode  * aLstNode,
                                ULong                aItemCnt,
                                sdrMtx             * aMtx );

    static IDE_RC removeNodeAtHead( idvSQL             * aStatistics,
                                    sdpSglPIDListBase  * aBase,
                                    sdrMtx             * aMtx,
                                    scPageID           * aRemovedPID,
                                    sdpSglPIDListNode ** aRemovedListNode );

    static IDE_RC removeNodeAtHead( sdpSglPIDListBase  * aBase,
                                    UChar              * aHeadPagePtr,
                                    sdrMtx             * aMtx,
                                    scPageID           * aRemovedPID,
                                    sdpSglPIDListNode ** aRemovedListNode);

    static IDE_RC removeNode( sdpSglPIDListBase  * aBase,
                              UChar              * aPrvPagePtr,
                              UChar              * aRmvPagePtr,
                              sdrMtx             * aMtx,
                              scPageID           * aRemovedPID,
                              sdpSglPIDListNode ** aRemovedListNode);

    static IDE_RC setNodeCnt( sdpSglPIDListBase  * aBase,
                              ULong                aNodeCnt,
                              sdrMtx             * aMtx );

    static IDE_RC setNxtOfNode(sdpSglPIDListNode * aNode,
                               scPageID            aPageID,
                               sdrMtx            * aMtx);

    static IDE_RC dumpList( scSpaceID  aSpaceID,
                            sdRID      aBaseRID );

    static IDE_RC dumpCheck( sdpSglPIDListBase* aBase,
                             scSpaceID          aSpaceID,
                             scPageID           aPageID );

    /* inline Function */
    static inline ULong getNodeCnt( sdpSglPIDListBase*   aBaseNode );

    static inline scPageID getNxtOfNode( sdpSglPIDListNode* aNode );

    static inline scPageID getHeadOfList( sdpSglPIDListBase* aBaseNode );
    static inline scPageID getTailOfList( sdpSglPIDListBase* aBaseNode );

    static IDE_RC setHeadOfList( sdpSglPIDListBase * aBase,
                                 scPageID            aPageID,
                                 sdrMtx            * aMtx );

    static IDE_RC setTailOfList( sdpSglPIDListBase * aBase,
                                 scPageID            aPageID,
                                 sdrMtx            * aMtx );

private:
    static void validate( sdpSglPIDListBase * aBase );
};


/***********************************************************************
 * Description : Base 노드의 length 반환
 ***********************************************************************/
inline ULong sdpSglPIDList::getNodeCnt( sdpSglPIDListBase*   aBaseNode )
{
    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mNodeCnt;
}

/***********************************************************************
 * Description : 노드의 next 노드 반환 혹은 base 노드의 tail 노드 반환
 ***********************************************************************/
inline scPageID sdpSglPIDList::getNxtOfNode( sdpSglPIDListNode*  aNode )
{
    IDE_DASSERT( aNode != NULL );

    return aNode->mNext;
}

/***********************************************************************
 * Description : base 노드의 head 노드 반환
 ***********************************************************************/
inline scPageID sdpSglPIDList::getHeadOfList(sdpSglPIDListBase* aBaseNode)
{
    IDE_DASSERT( aBaseNode != NULL );
    return aBaseNode->mHead;
}

/***********************************************************************
 * Description : base 노드의 tail 노드 반환
 ***********************************************************************/
inline scPageID sdpSglPIDList::getTailOfList(sdpSglPIDListBase* aBaseNode)
{
    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mTail;
}

#endif // _O_SDP_SGL_PIDLIST_H_


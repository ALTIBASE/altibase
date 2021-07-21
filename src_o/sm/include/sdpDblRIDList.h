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
 * $Id: sdpDblRIDList.h 22687 2007-07-23 01:01:41Z bskim $
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
 *                 sdpDblRIDListBase
 *                  ______________
 *                  |____len_____|
 *          ________|tail_|_head_|________
 *         |    ____/        \_____      |
 *         |  _/_________   _______\___  |
 *         |  |next|prev|<--|next|prev|  |
 *         -->|____|____|-->|____|____|<--
 *
 *         sdpDblRIDListNode
 *
 * RID 리스트는 동일한 tablespace에서만 유지 관리가 가능하며, 위와같은
 * circular double linked-list의 구조를 유지한다. 리스트에 대한 변경연산은
 * base 노드를 fix한 상태에서 이루어져야한다.
 * sdpPIDList와 달리 동일한 page내에 리스트의 여러 노드가 존재할 수 있다.
 *
 * # Releate data structure
 *
 * sdpDblRIDListBase 구조체
 * sdpDblRIDListNode 구조체
 *
 **********************************************************************/

#ifndef _O_SDP_DBL_RIDLIST_H_
#define _O_SDP_DBL_RIDLIST_H_ 1

#include <smDef.h>
#include <sdpDef.h>
#include <sdpPhyPage.h>

class sdpDblRIDList
{
public:

    /* RID list의 base 노드를 초기화 */
    static IDE_RC initList(sdpDblRIDListBase* aBaseNode,
                           sdrMtx*         aMtx);

    /* RID list의 head에 노드를 추가 */
    static IDE_RC insertHeadNode(idvSQL          *aStatistics,
                                 sdpDblRIDListBase*  aBaseNode,
                                 sdpDblRIDListNode*  aNewNode,
                                 sdrMtx*          aMtx);

    /* RID list의 tail에 노드를 추가 */
    static IDE_RC insertTailNode(idvSQL          *aStatistics,
                                 sdpDblRIDListBase*  aBaseNode,
                                 sdpDblRIDListNode*  aNewNode,
                                 sdrMtx*          aMtx);

    /* RID list의 특정 노드 뒤에 새로운 노드 추가 */
    static IDE_RC insertNodeAfter(idvSQL            *aStatistics,
                                  sdpDblRIDListBase*   aBaseNode,
                                  sdpDblRIDListNode*   aNode,
                                  sdpDblRIDListNode*   aNewNode,
                                  sdrMtx*           aMtx);

    /* 리스트 내에서 Node를 옮긴다. */
    static IDE_RC moveNodeInList( idvSQL          *  aStatistics,
                                  sdpDblRIDListBase*    aBaseNode,
                                  sdpDblRIDListNode*    aDestNode,
                                  sdpDblRIDListNode*    aSrcNode,
                                  sdrMtx          *  aMtx );

    /* RID list의 특정 노드 앞에 새로운 노드 추가 */
    static IDE_RC insertNodeBefore(idvSQL          *aStatistics,
                                   sdpDblRIDListBase*  aBaseNode,
                                   sdpDblRIDListNode*  aNode,
                                   sdpDblRIDListNode*  aNewNode,
                                   sdrMtx*          aMtx);

    /* RID list에서 특정노드 제거 */
    static IDE_RC removeNode(idvSQL             * aStatistics,
                             sdpDblRIDListBase  * aBaseNode,
                             sdpDblRIDListNode  * aNode,
                             sdrMtx             * aMtx);

    /* from 노드부터 tail까지 한번에 제거 */
    static IDE_RC removeNodesAtOnce(idvSQL               *aStatistics,
                                    sdpDblRIDListBase    *aBaseNode,
                                    sdpDblRIDListNode    *aFromNode,
                                    sdpDblRIDListNode    *aToNode,
                                    ULong                 aNodeCount,
                                    sdrMtx               *aMtx);

    /* rid list를 tail에 추가 */
    static IDE_RC insertNodesAtOnce(idvSQL            *aStatistics,
                                    sdpDblRIDListBase *aBaseNode,
                                    sdpDblRIDListNode *aFromNode,
                                    sdpDblRIDListNode *aToNode,
                                    ULong             aNodeCount,
                                    sdrMtx            *aMtx);

    /* base 노드의 head 노드 얻기 */
    static inline sdRID getBaseNodeRID(sdpDblRIDListBase*   aBaseNode);

    /* base 노드의 length 얻기 */
    static inline ULong getNodeCnt(sdpDblRIDListBase*   aBaseNode);

    /* base 노드의 head 노드 얻기 */
    static inline sdRID getHeadOfList(sdpDblRIDListBase*   aBaseNode);

    /* base 노드의 tail 노드 얻기 */
    static inline sdRID getTailOfList(sdpDblRIDListBase*   aBaseNode);

    /* 노드의 next 노드 얻기 */
    static inline sdRID getNxtOfNode(sdpDblRIDListNode*   aNode);

    /* 노드의 prev 노드 얻기 */
    static inline sdRID getPrvOfNode(sdpDblRIDListNode*   aNode);

    /* 리스트의 모든 노드 얻기 */
    static IDE_RC dumpList( scSpaceID aSpaceID,
                            sdRID     aBaseNodeRID );

private:

    // rid가 동일한 page에 존재하는지 검사
    static inline idBool isSamePage(sdRID*     aLhs,
                                    sdRID*     aRhs);

    /* base 노드의 length 설정 및 logging */
    static IDE_RC  setNodeCnt(sdpDblRIDListBase*  aBaseNode,
                              ULong               aNodeCnt,
                              sdrMtx*             aMtx);

    /* 노드의 next 노드 설정 및 logging */
    static IDE_RC  setNxtOfNode(sdpDblRIDListNode*  aNode,
                                sdRID            aNextRID,
                                sdrMtx*          aMtx);

    /* 노드의 prev 노드 설정 및 logging */
    static IDE_RC  setPrvOfNode(sdpDblRIDListNode*   aNode,
                                sdRID             aPrevRID,
                                sdrMtx*           aMtx);
};

/***********************************************************************
 * Description : base->mBase ptr를 rid로 변환하여 반환
 ***********************************************************************/
inline sdRID sdpDblRIDList::getBaseNodeRID(sdpDblRIDListBase*  aBaseNode)
{

    IDE_DASSERT( aBaseNode != NULL );

    return sdpPhyPage::getRIDFromPtr((UChar*)&aBaseNode->mBase);

}

/***********************************************************************
 * Description : 동일한 Page내의 rid인지 검사
 ***********************************************************************/
idBool sdpDblRIDList::isSamePage(sdRID* aLhs, sdRID* aRhs)
{

    IDE_DASSERT( aLhs != NULL );
    IDE_DASSERT( aRhs != NULL );

    return ((SD_MAKE_PID(*aLhs) == SD_MAKE_PID(*aRhs))) ? ID_TRUE : ID_FALSE;

}

/***********************************************************************
 * Description : Base 노드의 length 반환
 ***********************************************************************/
inline ULong sdpDblRIDList::getNodeCnt(sdpDblRIDListBase*   aBaseNode)
{
    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mNodeCnt;
}

/***********************************************************************
 * Description : 노드의 next 노드 반환 혹은 base 노드의 tail 노드 반환
 ***********************************************************************/
inline sdRID sdpDblRIDList::getNxtOfNode(sdpDblRIDListNode*  aNode)
{

    IDE_DASSERT( aNode != NULL );

    return aNode->mNext;

}

/***********************************************************************
 * Description : 노드의 prev 노드 반환 혹은 base 노드의 head 노드 반환
 ***********************************************************************/
inline sdRID sdpDblRIDList::getPrvOfNode(sdpDblRIDListNode*   aNode)
{

    IDE_DASSERT( aNode != NULL );

    return aNode->mPrev;

}


/***********************************************************************
 * Description : base 노드의 head 노드 반환
 ***********************************************************************/
inline sdRID sdpDblRIDList::getHeadOfList(sdpDblRIDListBase*   aBaseNode)
{

    IDE_DASSERT( aBaseNode != NULL );

    return getNxtOfNode(&aBaseNode->mBase);

}

/***********************************************************************
 * Description : base 노드의 tail 노드 반환
 ***********************************************************************/
inline sdRID sdpDblRIDList::getTailOfList(sdpDblRIDListBase*   aBaseNode)
{

    IDE_DASSERT( aBaseNode != NULL );

    return getPrvOfNode(&aBaseNode->mBase);

}

#endif // _O_SDP_DBL_RIDLIST_H_

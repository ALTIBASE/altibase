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
 * $Id: sdpDblPIDList.h 24150 2007-11-14 06:36:46Z bskim $
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
 * sdpDblPIDListBase 구조체
 * sdpDblPIDListNode 구조체
 *
 **********************************************************************/

#ifndef _O_SDP_DBL_PIDLIST_H_
#define _O_SDP_DBL_PIDLIST_H_ 1

#include <sdr.h>
#include <sdpDef.h>

class sdpDblPIDList
{
public:

    /* page list의 base 노드를 초기화 */
    static IDE_RC initBaseNode(sdpDblPIDListBase*    aBaseNode,  
                               sdrMtx*               aMtx);
    
    /* page list의 head에 노드를 추가 */
    static IDE_RC insertHeadNode(idvSQL           *aStatistics,
                                 sdpDblPIDListBase*   aBaseNode, 
                                 sdpDblPIDListNode*   aNewNode,
                                 sdrMtx*           aMtx);

    /* page list의 tail에 노드를 추가 */
    static IDE_RC insertTailNode(idvSQL           *aStatistics,
                                 sdpDblPIDListBase*   aBaseNode, 
                                 sdpDblPIDListNode*   aNewNode,
                                 sdrMtx*           aMtx);

    /* page list의 tail에 list를 추가 */
    static IDE_RC insertTailList(idvSQL              *aStatistics,
                                 sdpDblPIDListBase   *aBaseNode, 
                                 sdpDblPIDListNode   *aFstNode,
                                 sdpDblPIDListNode   *aLstNode,
                                 ULong                aListLen,
                                 sdrMtx*              aMtx);
    
    /* page list의 특정 노드 뒤에 새로운 노드 추가 */
    static IDE_RC insertNodeAfter(idvSQL*           aStatistics,
                                  sdpDblPIDListBase*   aBaseNode,
                                  sdpDblPIDListNode*   aNode,
                                  sdpDblPIDListNode*   aNewNode,
                                  sdrMtx*           aMtx);
    
    /* page list의 특정 노드 앞에 새로운 노드 추가 */
    static IDE_RC insertNodeBefore(idvSQL*           aStatistics,
                                   sdpDblPIDListBase*   aBaseNode,
                                   sdpDblPIDListNode*   aNode,
                                   sdpDblPIDListNode*   aNewNode,
                                   sdrMtx*           aMtx);

    static IDE_RC insertNodeBeforeWithNoFix(idvSQL*          aStatistics,
                                            sdpDblPIDListBase*  aBaseNode,
                                            sdpDblPIDListNode*  aNode,
                                            sdpDblPIDListNode*  aNewNode,
                                            sdrMtx*          aMtx);
    
    static IDE_RC insertNodeBeforeLow(idvSQL *aStatistics,
                                      sdpDblPIDListBase*  aBaseNode,
                                      sdpDblPIDListNode*  aNode,
                                      sdpDblPIDListNode*  aNewNode,
                                      sdrMtx*          aMtx,
                                      idBool           aNeedGetPage);
    
    /* page list에서 특정노드 제거 */
    static IDE_RC removeNode(idvSQL                *aStatistics,
                             sdpDblPIDListBase*     aBaseNode,
                             sdpDblPIDListNode*     aNode,
                             sdrMtx*                aMtx);
    
    static IDE_RC removeNodeLow(idvSQL          *aStatistics,
                                sdpDblPIDListBase*  aBaseNode,
                                sdpDblPIDListNode*  aNode,
                                sdrMtx*          aMtx,
                                idBool           aCheckMtxStack);
    
    static IDE_RC removeNodeWithNoFix(idvSQL          *aStatistics,
                                      sdpDblPIDListBase*  aBaseNode,
                                      sdpDblPIDListNode*  aNode,
                                      sdrMtx*          aMtx);
    
    static IDE_RC dumpCheck( sdpDblPIDListBase*  aBaseNode,
                             scSpaceID           aSpaceID,
                             scPageID            aPageID );

    /* base 노드의 length 얻기 */
    static inline ULong getNodeCnt(sdpDblPIDListBase*   aBaseNode);
    
    /* base 노드의 head 노드 얻기 */
    static inline scPageID getListHeadNode(sdpDblPIDListBase*   aBaseNode);
    
    /* base 노드의 tail 노드 얻기 */
    static inline scPageID getListTailNode(sdpDblPIDListBase*   aBaseNode);
    
    /* 노드의 next 노드 얻기 */
    static inline scPageID getNxtOfNode(sdpDblPIDListNode*   aNode);
    
    /* 노드의 prev 노드 얻기 */
    static inline scPageID getPrvOfNode(sdpDblPIDListNode*   aNode);
    
    /* list의 모든 노드 출력 */
    static IDE_RC dumpList( scSpaceID aSpaceID,
                            sdRID     aBaseNodeRID );
    
    /* 노드의 특정 prev page ID 설정 및 logging */
    static IDE_RC setPrvOfNode(sdpDblPIDListNode*  aNode,
                               scPageID            aPageID,
                               sdrMtx*             aMtx);
    
    /* 노드의 특정 next page ID 설정 및 logging */
    static IDE_RC setNxtOfNode(sdpDblPIDListNode*  aNode,
                               scPageID            aPageID,
                               sdrMtx*             aMtx);
    
    /* base 노드의 length 설정 및 logging */
    static IDE_RC setNodeCnt(sdpDblPIDListBase  *aBaseNode,
                             ULong               aLen,
                             sdrMtx             *aMtx);
};


/***********************************************************************
 * Description : Base 노드의 length 반환
 ***********************************************************************/
inline ULong sdpDblPIDList::getNodeCnt(sdpDblPIDListBase*   aBaseNode)
{
    IDE_DASSERT( aBaseNode != NULL );

    return aBaseNode->mNodeCnt;
}

/***********************************************************************
 * Description : 노드의 next 노드 반환 혹은 base 노드의 tail 노드 반환
 ***********************************************************************/
inline scPageID sdpDblPIDList::getNxtOfNode(sdpDblPIDListNode*  aNode)
{

    IDE_DASSERT( aNode != NULL );

    return aNode->mNext;
    
}

/***********************************************************************
 * Description : 노드의 prev 노드 반환 혹은 base 노드의 head 노드 반환
 ***********************************************************************/
inline scPageID sdpDblPIDList::getPrvOfNode(sdpDblPIDListNode* aNode)
{

    IDE_DASSERT( aNode != NULL );

    return aNode->mPrev;
    
}

/***********************************************************************
 * Description : base 노드의 head 노드 반환
 ***********************************************************************/
inline scPageID sdpDblPIDList::getListHeadNode(sdpDblPIDListBase* aBaseNode)
{

    IDE_DASSERT( aBaseNode != NULL );

    return getNxtOfNode(&aBaseNode->mBase);
    
}
    
/***********************************************************************
 * Description : base 노드의 tail 노드 반환
 ***********************************************************************/
inline scPageID sdpDblPIDList::getListTailNode(sdpDblPIDListBase* aBaseNode)
{
    
    IDE_DASSERT( aBaseNode != NULL );

    return getPrvOfNode(&aBaseNode->mBase);
    
}

#endif // _O_SDP_DBL_PIDLIST_H_


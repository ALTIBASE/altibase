/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/***********************************************************************
 * $Id$
 **********************************************************************/
#include <idu.h>
#include <iduVLogShmList.h>

void iduShmList::addBefore( iduShmTxInfo   * aShmTxInfo,
                            iduShmListNode * aTgtNode,
                            iduShmListNode * aNode )
{
    iduShmListNode * sTgtNodePrev =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK(aTgtNode->mAddrPrev);

    IDU_SHM_VALIDATE_ADDR_PTR( aNode->mAddrSelf, aNode );

    iduVLogShmList::writeAddBefore( aShmTxInfo,
                                    aTgtNode,
                                    sTgtNodePrev,
                                    aNode );

    aNode->mAddrNext = aTgtNode->mAddrSelf;
    aNode->mAddrPrev = aTgtNode->mAddrPrev;

    sTgtNodePrev->mAddrNext = aNode->mAddrSelf;
    aTgtNode->mAddrPrev = aNode->mAddrSelf;

}

void iduShmList::addAfter( iduShmTxInfo   * aShmTxInfo,
                           iduShmListNode * aTgtNode,
                           iduShmListNode * aNode )
{
    iduShmListNode * sTgtNodeNext =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK(aTgtNode->mAddrNext);

    IDU_SHM_VALIDATE_ADDR_PTR( aNode->mAddrSelf, aNode );

    iduVLogShmList::writeAddAfter( aShmTxInfo,
                                   aTgtNode,
                                   sTgtNodeNext,
                                   aNode );

    aNode->mAddrPrev = aTgtNode->mAddrSelf;
    aNode->mAddrNext = aTgtNode->mAddrNext;

    sTgtNodeNext->mAddrPrev = aNode->mAddrSelf;
    aTgtNode->mAddrNext = aNode->mAddrSelf;

}

void iduShmList::addAfterBySerialOP( iduShmTxInfo   * aShmTxInfo,
                                     iduShmListNode * aTgtNode,
                                     iduShmListNode * aNode )
{
    iduShmListNode * sTgtNodeNext =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK(aTgtNode->mAddrNext);

    iduVLogShmList::writeAddAfter( aShmTxInfo,
                                   aTgtNode,
                                   sTgtNodeNext,
                                   aNode);

    ID_SERIAL_BEGIN( aNode->mAddrPrev = aTgtNode->mAddrSelf );
    ID_SERIAL_END( aNode->mAddrNext = aTgtNode->mAddrNext );

    sTgtNodeNext->mAddrPrev = aNode->mAddrSelf;
    aTgtNode->mAddrNext = aNode->mAddrSelf;

}

void iduShmList::addFirstBySerialOP( iduShmTxInfo   * aShmTxInfo,
                                     iduShmListNode * aBase,
                                     iduShmListNode * aNode )
{
    addAfterBySerialOP( aShmTxInfo, aBase, aNode );
}

void iduShmList::addFirst( iduShmTxInfo   * aShmTxInfo,
                           iduShmListNode * aBase,
                           iduShmListNode * aNode )
{
    IDU_SHM_VALIDATE_ADDR_PTR( aNode->mAddrSelf, aNode );

    addAfter( aShmTxInfo, aBase, aNode );
}

void iduShmList::addLast( iduShmTxInfo   * aShmTxInfo,
                          iduShmListNode * aBase,
                          iduShmListNode * aNode )
{
    IDU_SHM_VALIDATE_ADDR_PTR( aNode->mAddrSelf, aNode );

    addBefore( aShmTxInfo, aBase, aNode );
}

void iduShmList::cutBetween( iduShmTxInfo   * aShmTxInfo,
                             iduShmListNode * aPrev,
                             iduShmListNode * aNext )
{
    iduVLogShmList::writeCutBetween( aShmTxInfo,
                                     aPrev,
                                     aNext);

    aPrev->mAddrNext = aNext->mAddrSelf;
    aNext->mAddrPrev = aPrev->mAddrSelf;
}

void iduShmList::addListFirst( iduShmTxInfo   * aShmTxInfo,
                               iduShmListNode * aBase,
                               iduShmListNode * aFirst,
                               iduShmListNode * aLast )
{
    iduShmListNode* sBaseNext =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK( aBase->mAddrNext );

    iduVLogShmList::writeAddListFirst( aShmTxInfo,
                                       aBase,
                                       sBaseNext,
                                       aFirst,
                                       aLast);

    aFirst->mAddrPrev = aBase->mAddrSelf;
    aLast->mAddrNext  = aBase->mAddrNext;

    sBaseNext->mAddrPrev = aLast->mAddrSelf;
    aBase->mAddrNext = aFirst->mAddrSelf;

}

void iduShmList::addListFirstBySerialOP( iduShmTxInfo   * aShmTxInfo,
                                         iduShmListNode * aBase,
                                         iduShmListNode * aFirst,
                                         iduShmListNode * aLast )
{
    iduShmListNode* sBaseNext =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK( aBase->mAddrNext );

    iduVLogShmList::writeAddListFirst( aShmTxInfo,
                                       aBase,
                                       sBaseNext,
                                       aFirst,
                                       aLast );

    ID_SERIAL_BEGIN( aFirst->mAddrPrev = aBase->mAddrSelf );
    ID_SERIAL_END( aLast->mAddrNext = aBase->mAddrNext );

    sBaseNext->mAddrPrev = aLast->mAddrSelf;
    aBase->mAddrNext = aFirst->mAddrSelf;

}

void iduShmList::remove( iduShmTxInfo   * aShmTxInfo,
                         iduShmListNode * aNode )
{
    iduShmListNode* sNodeNext =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK( aNode->mAddrNext );

    iduShmListNode* sNodePrev =
        ( iduShmListNode * )IDU_SHM_GET_ADDR_PTR_CHECK( aNode->mAddrPrev );

    iduVLogShmList::writeRemove( aShmTxInfo,
                                 aNode,
                                 sNodePrev,
                                 sNodeNext);

    sNodePrev->mAddrNext = aNode->mAddrNext;
    sNodeNext->mAddrPrev = aNode->mAddrPrev;

}

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

#ifndef _O_IDU_SHM_LIST_H_
#define _O_IDU_SHM_LIST_H_ 1

#include <iduShmDef.h>
#include <iduShmMgr.h>

class iduShmList
{
public:
    static void initBase( iduShmListNode *aBase,
                          idShmAddr       aSelfAddr,
                          idShmAddr       aAddrData );

    static idBool isEmpty( iduShmListNode *aNode );
    static idBool isNodeLinked( iduShmListNode *aNode );
    static void   initNode( iduShmListNode *aNode,
                            idShmAddr       aSelfAddr,
                            idShmAddr       aAddrData );
    static inline void setData( iduShmListNode *aNode, idShmAddr aAddr4Data );

    static inline idShmAddr getAddr( iduShmListNode *aNode );

    static inline idShmAddr getPrv( iduShmListNode *aNode );
    static inline idShmAddr getNxt( iduShmListNode *aNode );
    static inline idShmAddr getFst( iduShmListNode *aNode );
    static inline idShmAddr getLst( iduShmListNode *aNode );
    static inline idShmAddr getData( iduShmListNode *aNode );

    static inline iduShmListNode* getPrvPtr( iduShmListNode *aNode );
    static inline iduShmListNode* getNxtPtr( iduShmListNode *aNode );
    static inline iduShmListNode* getFstPtr( iduShmListNode *aNode );
    static inline iduShmListNode* getLstPtr( iduShmListNode *aNode );
    static inline void* getDataPtr( iduShmListNode *aNode );

    static void addBefore( iduShmTxInfo   * aShmTxInfo,
                           iduShmListNode * aTgtNode,
                           iduShmListNode * aNode );

    static void addAfter( iduShmTxInfo   * aShmTxInfo,
                          iduShmListNode * aTgtNode,
                          iduShmListNode * aNode );

    static void addAfterBySerialOP( iduShmTxInfo   * aShmTxInfo,
                                    iduShmListNode * aTgtNode,
                                    iduShmListNode * aNode );

    static void addFirstBySerialOP( iduShmTxInfo   * aShmTxInfo,
                                    iduShmListNode * aBase,
                                    iduShmListNode * aNode );

    static void addFirst( iduShmTxInfo   * aShmTxInfo,
                          iduShmListNode * aBase,
                          iduShmListNode * aNode );

    static void addLast( iduShmTxInfo   * aShmTxInfo,
                         iduShmListNode * aBase,
                         iduShmListNode * aNode );

    static void cutBetween( iduShmTxInfo   * aShmTxInfo,
                            iduShmListNode * aPrev,
                            iduShmListNode * aNext );

    static void addListFirst( iduShmTxInfo   * aShmTxInfo,
                              iduShmListNode * aBase,
                              iduShmListNode * aFirst,
                              iduShmListNode * aLast );

    static void addListFirstBySerialOP( iduShmTxInfo   * aShmTxInfo,
                                        iduShmListNode * aBase,
                                        iduShmListNode * aFirst,
                                        iduShmListNode * aLast );

    static void remove( iduShmTxInfo   * aShmTxInfo,
                        iduShmListNode * aNode );
};

inline void iduShmList::initBase( iduShmListNode *aNode,
                                  idShmAddr       aSelfAddr,
                                  idShmAddr       aAddrData )
{
    aNode->mAddrPrev = aSelfAddr;
    aNode->mAddrNext = aSelfAddr;
    aNode->mAddrData = aAddrData;
    aNode->mAddrSelf = aSelfAddr;
}

inline idBool iduShmList::isEmpty( iduShmListNode *aNode )
{
    if(( aNode->mAddrPrev == aNode->mAddrSelf ) &&
       ( aNode->mAddrNext == aNode->mAddrSelf ))
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline idBool iduShmList::isNodeLinked( iduShmListNode *aNode )
{
    if(( aNode->mAddrPrev != aNode->mAddrSelf ) &&
       ( aNode->mAddrNext != aNode->mAddrSelf ))
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline void iduShmList::initNode( iduShmListNode *aNode,
                                  idShmAddr       aSelfAddr,
                                  idShmAddr       aAddrData )
{
    aNode->mAddrPrev = IDU_SHM_NULL_ADDR;
    aNode->mAddrNext = IDU_SHM_NULL_ADDR;
    aNode->mAddrSelf = aSelfAddr;
    aNode->mAddrData = aAddrData;
}

inline void iduShmList::setData( iduShmListNode *aNode, idShmAddr aAddr4Data )
{
    aNode->mAddrData = aAddr4Data;
}

inline idShmAddr iduShmList::getAddr( iduShmListNode *aNode )
{
    return aNode->mAddrSelf;
}

inline idShmAddr iduShmList::getPrv( iduShmListNode *aNode )
{
    return aNode->mAddrPrev;
}

inline idShmAddr iduShmList::getNxt( iduShmListNode *aNode )
{
    return aNode->mAddrNext;
}

inline idShmAddr iduShmList::getFst( iduShmListNode *aBase )
{
    return aBase->mAddrNext;
}

inline idShmAddr iduShmList::getLst( iduShmListNode *aNode )
{
    return aNode->mAddrPrev;
}

inline iduShmListNode* iduShmList::getPrvPtr( iduShmListNode *aNode )
{
    return (iduShmListNode*)IDU_SHM_GET_ADDR_PTR_CHECK( iduShmList::getPrv( aNode ) );
}

inline iduShmListNode* iduShmList::getNxtPtr( iduShmListNode *aNode )
{
    return (iduShmListNode*)IDU_SHM_GET_ADDR_PTR_CHECK( iduShmList::getNxt( aNode ) );
}

inline iduShmListNode* iduShmList::getFstPtr( iduShmListNode *aNode )
{
    return (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( iduShmList::getFst( aNode ) );
}

inline iduShmListNode* iduShmList::getLstPtr( iduShmListNode *aNode )
{
    return (iduShmListNode*)IDU_SHM_GET_ADDR_PTR( iduShmList::getLst( aNode ) );
}

inline idShmAddr iduShmList::getData( iduShmListNode *aNode )
{
    return aNode->mAddrData;
}

inline void* iduShmList::getDataPtr( iduShmListNode *aNode )
{
    return (void*)IDU_SHM_GET_ADDR_PTR( aNode->mAddrData);
}

#endif  // _O_IDU_SHM_LIST_H_

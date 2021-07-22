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
 * $Id: 
 **********************************************************************/

#ifndef _O_RPD_REPLICATED_TRANS_GROUP_NODE_H_
#define _O_RPD_REPLICATED_TRANS_GROUP_NODE_H_ 1

#include <idl.h>

#include <smDef.h>

class rpdReplicatedTransGroupNode;
class rpdTransSlotNode;

typedef enum rpdReplicatedTransGroupOperation
{
    RPD_REPLICATED_TRANS_GROUP_NONE,
    RPD_REPLICATED_TRANS_GROUP_SEND,
    RPD_REPLICATED_TRANS_GROUP_DONT_SEND
} rpdReplicatedTransGroupOperation;

typedef struct rpdReplicatedTrans
{
    smTID       mReplicatedTransID;
    smSN        mBeginSN;
    smSN        mEndSN;
} rpdReplicatedTrans;

class rpdReplicatedTransGroupNode
{
    /* Variable */
private:
    UInt                    mTransTableSize;

    smTID                   mGroupTransID;
    smSN                    mBeginSN;
    smSN                    mEndSN;
    smTID                   mEndTransID;


    rpdTransSlotNode      * mTransSlotTable;
    UInt                    mTransSlotCount;

    rpdReplicatedTrans    * mReplicatedTrans;
    UInt                    mReplicatedTransCount;

    UInt                    mReplicatedTransGroupMaxCount;

    rpdReplicatedTransGroupOperation      mOperation;
    rpdReplicatedTransGroupNode         * mNext;

    iduMemPool            * mSlotNodePool;
    iduMemPool            * mReplicatedTransPool;

public:


    /* Function */
private:

    void            sortTransSlotNode( void );
    void            sortTrans( void );
    rpdReplicatedTrans *
                    getReplicatedTransByTransID( smTID     aTransID );

public:
    IDE_RC          initialize( iduMemPool  * aSlotNodePool,
                                iduMemPool  * aReplicatedTransPool,
                                UInt          aTransTableSize,
                                UInt          aReplicatedTransGroupMaxCount );
    void            finalize( void );

    void            insertCompleteTrans( smTID      aTransID,
                                         smSN       aBeginSN,
                                         smSN       aEndSN );

    rpdTransSlotNode * 
                    getTransSlotNodeByIndex( UInt     aIndex );

    UInt            getTransSlotIndexByIndex( UInt    aIndex );

    rpdTransSlotNode * 
                    getTransSlotNodeByTransSlotIndex( UInt     aTransSlotIndex );

    void            setNextReplicatedTransGroupNode( rpdReplicatedTransGroupNode   * aNode );

    rpdReplicatedTransGroupNode * 
                    getNextReplicatedTransGroupNode( void );

    UInt            getReplicatedTransCount( void );

    UInt            getTransSlotCount( void );

    void            setNextReplicatedTransGroupNodeInTransSlotNode( UInt   aTransSlotIndex,
                                                                    rpdReplicatedTransGroupNode * aNode );


    void            setOperation( rpdReplicatedTransGroupOperation     aOperation );
    rpdReplicatedTransGroupOperation
                    getOperation( void );

    smTID           getGroupTransID( void );

    smSN            getEndSN( void );

    idBool          isLastTrans( smTID  aTransID,
                                 smSN   aEndSN );

    idBool          isThisTransaction( smTID    aTransID,
                                       smSN     aSN );

    void            clear( void );

    IDE_RC          buildRecordForReplicatedTransGroupInfo( SChar               * aRepName,
                                                            void                * aHeader,
                                                            void                * aDumpObj,
                                                            iduFixedTableMemory * aMemory );

    IDE_RC          buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                           void                * aHeader,
                                                           void                * aDumpObj,
                                                           iduFixedTableMemory * aMemory );

void printNodeInfo( void );
};

#endif

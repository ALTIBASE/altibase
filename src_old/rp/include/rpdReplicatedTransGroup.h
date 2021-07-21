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

#ifndef _O_RPD_REPLICATED_TRANS_GROUP_H_
#define _O_RPD_REPLICATED_TRANS_GROUP_H_ 1

#include <idl.h>

#include <smDef.h>

#include <rpdReplicatedTransGroupNode.h>
#include <rpdAnalyzedTransTable.h>

class rpdReplicatedTransGroup
{
    /* Variable */
private:
    rpdReplicatedTransGroupNode     * mAnalyzingGroup;

    rpdReplicatedTransGroupNode       mHeadCompletedGroupList;
    rpdReplicatedTransGroupNode     * mTailCompletedGroupList;

    UInt                              mTransTableSize;

    rpdAnalyzedTransTable             mTransTable;

    UInt                              mReplicationTransGroupMaxCount;

    iduMutex                          mMutex;

    iduMemPool                        mReplicatedTransNodePool;
    iduMemPool                        mReplicatedTransPool;
    iduMemPool                        mSlotNodePool;

public:


    /* Function */
private:

public:
    IDE_RC          initialize( SChar   * aRepName,
                                UInt      aReplicationTransGroupMaxCount );
    void            finalize( void );

    IDE_RC          addCompletedReplicatedTransGroup( rpdReplicatedTransGroupOperation aOperation );
    void            removeCompleteReplicatedTransGroup( void );
    void            checkAndRemoveCompleteReplicatedTransGroup( smSN     aSN );

    idBool          isLastTransactionInFirstGroup( smTID     aTransID,
                                                   smSN      aEndSN );

    IDE_RC          checkAndAddReplicatedTransGroupToComplete( rpdReplicatedTransGroupOperation aOperation );

    void            insertCompleteTrans( smTID      aTransID,
                                         smSN       aBeginSN,
                                         smSN       aEndSN );

    UInt            getReplicatedTransCount( void );

    smTID           getReplicatedTransGroupTransID( smTID    aTransID );

    void            completedRead( void );

    void            clearReplicatedTransGroup( void );

    void            getReplicatedTransGroupInfo( smTID           aTransID,
                                                 smSN            aSN,
                                                 idBool        * aIsFirstGroup,
                                                 idBool        * aIsLastLog,
                                                 rpdReplicatedTransGroupOperation    * aGroupOperation );

    idBool          isThereGroupNode( void );

    IDE_RC          buildRecordForReplicatedTransGroupInfo( SChar               * aRepName,
                                                            void                * aHeader,
                                                            void                * aDumpObj,
                                                            iduFixedTableMemory * aMemory );

    IDE_RC          buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                           void                * aHeader,
                                                           void                * aDumpObj,
                                                           iduFixedTableMemory * aMemory );
};

#endif

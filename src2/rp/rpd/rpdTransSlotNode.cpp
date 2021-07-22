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

#include <rpdReplicatedTransGroupNode.h>
#include <rpdTransSlotNode.h>
#include <rpuProperty.h>
#include <rpcManager.h>

#include <smiMisc.h>

void rpdTransSlotNode::initialize( void )
{
    mTransSlotIndex = -1;
    mNext = NULL;
}

void rpdTransSlotNode::finalize( void )
{
    /* do nothing */
}

void rpdTransSlotNode::setTransInfo( UInt       aTransSlotIndex )
{
    mTransSlotIndex = aTransSlotIndex;
    mNext = NULL;
}


rpdReplicatedTransGroupNode * rpdTransSlotNode::getNextReplicatedTransGroupNode( void )
{
    return mNext;
}

void rpdTransSlotNode::setNextReplicatedTransGroupNode( rpdReplicatedTransGroupNode   * aNode )
{
    IDE_DASSERT( aNode != NULL );

    mNext = aNode;
}

UInt rpdTransSlotNode::getTransSlotIndex( void )
{
    return mTransSlotIndex;
}

IDE_RC rpdTransSlotNode::buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                                smTID                 aGroupTransID,
                                                                void                * aHeader,
                                                                void                * /* aDumpObj */,
                                                                iduFixedTableMemory * aMemory )
{
    rpdReplicatedTransSlotInfo        sInfo;

    sInfo.mRepName = aRepName;
    sInfo.mGroupTransID = aGroupTransID;
    sInfo.mTransSlotIndex = mTransSlotIndex;

    if ( mNext != NULL )
    {
        sInfo.mNextGroupTransID = mNext->getGroupTransID();
    }
    else
    {
        sInfo.mNextGroupTransID = SM_NULL_TID;
    }

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sInfo )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


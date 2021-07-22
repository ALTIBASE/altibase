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

#include <rpcManager.h>
#include <rpdReplicatedTransGroupNode.h>
#include <rpdTransSlotNode.h>
#include <rpuProperty.h>

#include <smiMisc.h>

IDE_RC rpdReplicatedTransGroupNode::initialize( iduMemPool  * aSlotNodePool,
                                                iduMemPool  * aReplicatedTransPool,
                                                UInt          aTransTableSize,
                                                UInt          aReplicatedTransGroupMaxCount )
{
    UInt    i = 0;
    idBool  sIsTransSlotTableAlloc = ID_FALSE;
    idBool  sIsReplicatedTransAlloc = ID_FALSE;

    IDE_DASSERT( aSlotNodePool != NULL );
    IDE_DASSERT( aReplicatedTransPool != NULL );

    mTransTableSize = aTransTableSize;

    mGroupTransID = SM_NULL_TID;
    mBeginSN = SM_SN_MAX;
    mEndSN = 0;
    mOperation = RPD_REPLICATED_TRANS_GROUP_NONE;

    mSlotNodePool = aSlotNodePool;
    mReplicatedTransPool = aReplicatedTransPool;

    IDU_FIT_POINT( "rpdReplicatedTransGroupNode::initialize::alloc::mTransSlotTable",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mSlotNodePool->alloc" );
    IDE_TEST( mSlotNodePool->alloc( (void**)&mTransSlotTable ) != IDE_SUCCESS );
    sIsTransSlotTableAlloc = ID_TRUE;
    mTransSlotCount = 0;

    IDU_FIT_POINT( "rpdReplicatedTransGroupNode::initialize::alloc::mReplicatedTrans",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mReplicatedTransPool->alloc" );
    IDE_TEST( mReplicatedTransPool->alloc( (void**)&mReplicatedTrans ) != IDE_SUCCESS );
    sIsReplicatedTransAlloc = ID_TRUE;
    mReplicatedTransCount = 0;

    mReplicatedTransGroupMaxCount = aReplicatedTransGroupMaxCount;

    for ( i = 0; i < mReplicatedTransGroupMaxCount; i++ )
    {
        mTransSlotTable[i].initialize();
    }

    mNext = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsTransSlotTableAlloc == ID_TRUE )
    {
        (void)mSlotNodePool->memfree( mTransSlotTable );
        mTransSlotTable = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( sIsReplicatedTransAlloc == ID_TRUE )
    {
        (void)mReplicatedTransPool->memfree( mReplicatedTrans );
        mReplicatedTrans = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdReplicatedTransGroupNode::finalize( void )
{
    UInt        i = 0;

    for ( i = 0; i < mReplicatedTransGroupMaxCount; i++ )
    {
        mTransSlotTable[i].finalize();
    }

    (void)mSlotNodePool->memfree( mTransSlotTable );
    mTransSlotTable = NULL;
    mTransSlotCount = 0;

    (void)mReplicatedTransPool->memfree( mReplicatedTrans );
    mReplicatedTrans = NULL;
    mReplicatedTransCount = 0;
}

void rpdReplicatedTransGroupNode::insertCompleteTrans( smTID      aTransID,
                                                       smSN       aBeginSN,
                                                       smSN       aEndSN )
{
    UInt                sTransSlotIndex = 0;
    rpdTransSlotNode  * sNode = NULL;

    IDE_DASSERT( mReplicatedTransCount < mReplicatedTransGroupMaxCount );

    if ( mGroupTransID == SM_NULL_TID )
    {
        mGroupTransID = aTransID;
    }
    else
    {
        /* do nothing */
    }

    if ( aBeginSN < mBeginSN )
    {
        mBeginSN = aBeginSN;
    }
    else
    {
        /* do nothing */
    }

    /* new log is always bigger than log that is already in group */
    mEndSN = aEndSN;
    mEndTransID = aTransID;

    mReplicatedTrans[mReplicatedTransCount].mReplicatedTransID = aTransID;
    mReplicatedTrans[mReplicatedTransCount].mBeginSN = aBeginSN;
    mReplicatedTrans[mReplicatedTransCount].mEndSN = aEndSN;
    mReplicatedTransCount++;

    sTransSlotIndex = aTransID % mTransTableSize;

    sNode = getTransSlotNodeByTransSlotIndex( sTransSlotIndex );

    if ( sNode == NULL )
    {
        mTransSlotTable[mTransSlotCount].setTransInfo( sTransSlotIndex );

        mTransSlotCount++;
    }
    else
    {
        /* do nothing */
    }

    sortTrans();
    sortTransSlotNode();
}

rpdTransSlotNode * rpdReplicatedTransGroupNode::getTransSlotNodeByIndex( UInt     aIndex )
{
    return &mTransSlotTable[aIndex];
}

UInt rpdReplicatedTransGroupNode::getTransSlotIndexByIndex( UInt    aIndex )
{
    return mTransSlotTable[aIndex].getTransSlotIndex();
}

rpdTransSlotNode * rpdReplicatedTransGroupNode::getTransSlotNodeByTransSlotIndex( UInt     aTransSlotIndex )
{
    SInt    sLow = 0;
    SInt    sHigh = 0;
    SInt    sMid = 0;
    UInt    sTransSlotIndex = 0;

    rpdTransSlotNode  * sNode = NULL;

    sLow = 0;
    sHigh = mTransSlotCount - 1;

    while ( sLow <= sHigh )
    {
        sMid = ( sHigh + sLow ) >> 1;

        sTransSlotIndex = mTransSlotTable[sMid].getTransSlotIndex();

        if ( sTransSlotIndex > aTransSlotIndex )
        {
            sHigh = sMid - 1;
        }
        else if ( sTransSlotIndex < aTransSlotIndex )
        {
            sLow = sMid + 1;
        }
        else
        {
            sNode = &mTransSlotTable[sMid];
            break;
        }
    }

    return sNode;
}

UInt rpdReplicatedTransGroupNode::getReplicatedTransCount( void )
{
    return mReplicatedTransCount;
}

UInt rpdReplicatedTransGroupNode::getTransSlotCount( void )
{
    return mTransSlotCount;
}

extern "C" SInt rpdReplicatedTransGroupNodeCompareTransSlotNode( const void * aElem1,
                                                                 const void * aElem2 )
{
    SInt    sRC = 0;
    UInt    sTransSlotIndex1 = 0;
    UInt    sTransSlotIndex2 = 0;

    sTransSlotIndex1 = ((rpdTransSlotNode*)aElem1)->getTransSlotIndex();
    sTransSlotIndex2 = ((rpdTransSlotNode*)aElem2)->getTransSlotIndex();

    if ( sTransSlotIndex1 > sTransSlotIndex2 )
    {
        sRC = 1;
    }
    else if ( sTransSlotIndex1 < sTransSlotIndex2 )
    {
        sRC = -1;
    }
    else
    {
        sRC = 0;
    }

    return sRC;
}

extern "C" SInt rpdReplicatedTransGroupNodeCompareTrans( const void * aElem1,
                                                         const void * aElem2 )
{
    SInt    sRC = 0;
    smTID   sTransID1 = 0;
    smTID   sTransID2 = 0;

    sTransID1 = ((rpdReplicatedTrans*)aElem1)->mReplicatedTransID;
    sTransID2 = ((rpdReplicatedTrans*)aElem2)->mReplicatedTransID;

    if ( sTransID1 > sTransID2 )
    {
        sRC = 1;
    }
    else if ( sTransID1 < sTransID2 )
    {
        sRC = -1;
    }
    else
    {
        sRC = 0;
    }

    return sRC;
}

void rpdReplicatedTransGroupNode::sortTransSlotNode( void )
{
    idlOS::qsort( mTransSlotTable,
                  mTransSlotCount,
                  ID_SIZEOF( rpdTransSlotNode ),
                  rpdReplicatedTransGroupNodeCompareTransSlotNode ); 
}

void rpdReplicatedTransGroupNode::sortTrans( void )
{
    idlOS::qsort( mReplicatedTrans,
                  mReplicatedTransCount,
                  ID_SIZEOF( rpdReplicatedTrans ),
                  rpdReplicatedTransGroupNodeCompareTrans );
}

void rpdReplicatedTransGroupNode::setOperation( rpdReplicatedTransGroupOperation    aOperation )
{
    mOperation = aOperation;
}

rpdReplicatedTransGroupOperation rpdReplicatedTransGroupNode::getOperation( void )
{
    return mOperation;
}

void rpdReplicatedTransGroupNode::setNextReplicatedTransGroupNode( rpdReplicatedTransGroupNode   * aNode )
{
    mNext = aNode;
}

rpdReplicatedTransGroupNode * rpdReplicatedTransGroupNode::getNextReplicatedTransGroupNode( void )
{
    return mNext;
}

void rpdReplicatedTransGroupNode::setNextReplicatedTransGroupNodeInTransSlotNode( UInt   aTransSlotIndex,
                                                                                  rpdReplicatedTransGroupNode * aNode )
{

    IDE_DASSERT( aNode != NULL );

    rpdTransSlotNode  * sTransSlotNode = NULL;

    sTransSlotNode = getTransSlotNodeByTransSlotIndex( aTransSlotIndex );

    IDE_DASSERT( sTransSlotNode != NULL );

    sTransSlotNode->setNextReplicatedTransGroupNode( aNode );
}

smTID rpdReplicatedTransGroupNode::getGroupTransID( void )
{
    return mGroupTransID;
}

smSN rpdReplicatedTransGroupNode::getEndSN( void )
{
    return mEndSN;
}

idBool rpdReplicatedTransGroupNode::isLastTrans( smTID  aTransID,
                                                 smSN   aEndSN )
{
    idBool      sRC = ID_FALSE;

    if ( ( mEndSN == aEndSN ) &&
         ( mEndTransID == aTransID ) )
    {
        sRC = ID_TRUE;
    }
    else
    {
        sRC = ID_FALSE;
    }

    return sRC;
}

IDE_RC rpdReplicatedTransGroupNode::buildRecordForReplicatedTransGroupInfo( SChar               * aRepName,
                                                                            void                * aHeader,
                                                                            void                * /* aDumpObj */,
                                                                            iduFixedTableMemory * aMemory )
{
    rpdReplicatedTransGroupInfo     sInfo;
    UInt                            i = 0;

    sInfo.mRepName = aRepName;
    sInfo.mGroupTransID = mGroupTransID;
    sInfo.mGroupTransBeginSN = mBeginSN;
    sInfo.mGroupTransEndSN = mEndSN;

    sInfo.mOperation = mOperation;

    for ( i = 0; i < mReplicatedTransCount; i++ )
    {
        sInfo.mTransID = mReplicatedTrans[i].mReplicatedTransID;
        sInfo.mBeginSN = mReplicatedTrans[i].mBeginSN;
        sInfo.mEndSN = mReplicatedTrans[i].mEndSN;

        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)&sInfo )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdReplicatedTransGroupNode::buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                                           void                * aHeader,
                                                                           void                * aDumpObj,
                                                                           iduFixedTableMemory * aMemory )
{
    UInt   i = 0;

    for ( i = 0; i < mTransSlotCount; i++ )
    {
        IDE_TEST( mTransSlotTable[i].buildRecordForReplicatedTransSlotInfo( aRepName,
                                                                            mGroupTransID,
                                                                            aHeader,
                                                                            aDumpObj,
                                                                            aMemory )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpdReplicatedTransGroupNode::clear( void )
{
    UInt    i = 0;

    mGroupTransID = SM_NULL_TID;
    mBeginSN = SM_SN_MAX;
    mEndSN = 0;
    mEndTransID = SM_NULL_TID;
    mOperation = RPD_REPLICATED_TRANS_GROUP_NONE;

    for ( i = 0; i < mReplicatedTransCount; i++ )
    {
        mReplicatedTrans[i].mReplicatedTransID = SM_NULL_TID;
        mReplicatedTrans[i].mBeginSN = SM_SN_NULL;
        mReplicatedTrans[i].mEndSN = SM_SN_NULL;
    }

    for ( i = 0; i < mTransSlotCount; i++ )
    {
        mTransSlotTable[i].initialize();
    }

    mReplicatedTransCount = 0;
    mTransSlotCount = 0;

    mNext = NULL;
}

rpdReplicatedTrans * rpdReplicatedTransGroupNode::getReplicatedTransByTransID( smTID     aTransID )
{
    SInt    sLow = 0;
    SInt    sHigh = 0;
    SInt    sMid = 0;

    smTID   sTransID = SM_NULL_TID;

    rpdReplicatedTrans      * sReplicatedTrans = NULL;

    sLow = 0;
    sHigh = mReplicatedTransCount - 1;
    
    while ( sLow <= sHigh )
    {
        sMid = ( sHigh + sLow ) >> 1;

        sTransID = mReplicatedTrans[sMid].mReplicatedTransID;

        if ( sTransID > aTransID )
        {
            sHigh = sMid - 1;
        }
        else if ( sTransID < aTransID )
        {
            sLow = sMid + 1;
        }
        else
        {
            sReplicatedTrans = &mReplicatedTrans[sMid];
            break;
        }
    }

    return sReplicatedTrans;
}

idBool rpdReplicatedTransGroupNode::isThisTransaction( smTID    aTransID,
                                                       smSN     aSN )
{
    rpdReplicatedTrans      * sTrans = NULL;
    idBool                    sRC = ID_FALSE;

    sTrans = getReplicatedTransByTransID( aTransID );

    if ( sTrans == NULL )
    {
        sRC = ID_FALSE;
    }
    else
    {
        if ( ( aSN >= sTrans->mBeginSN ) && 
             ( aSN <= sTrans->mEndSN ) )
        {
            sRC = ID_TRUE;
        }
        else
        {
            sRC = ID_FALSE;
        }
    }

    return sRC;
}



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

#include <rpError.h>

#include <rpuProperty.h>

#include <rpdTransSlotNode.h>
#include <rpdReplicatedTransGroup.h>

#include <smiMisc.h>

IDE_RC rpdReplicatedTransGroup::initialize( SChar   * aRepName,
                                            UInt      aReplicationTransGroupMaxCount )
{
    SChar       sName[IDU_MUTEX_NAME_LEN + 1] = { 0, };
    UInt        sStage = 0;

    IDU_FIT_POINT( "rpdReplicatedTransGroup::initialize::initialize::mReplicatedTransNodePool",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mReplicatedTransNodePool" );
    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_REPLICATED_TRANS_GROUP", aRepName );
    IDE_TEST( mReplicatedTransNodePool.initialize( IDU_MEM_RP_RPD,
                                                   sName,
                                                   2,
                                                   ID_SIZEOF( rpdReplicatedTransGroupNode ),
                                                   1, /* not use */
                                                   IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                                   ID_TRUE,							/* UseMutex */
                                                   IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                                   ID_FALSE,						/* ForcePooling */
                                                   ID_TRUE,							/* GarbageCollection */
                                                   ID_TRUE,                         /* HWCacheLine */
                                                   IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/)
             != IDE_SUCCESS);			
    sStage = 1;

    mReplicationTransGroupMaxCount = aReplicationTransGroupMaxCount;

    IDU_FIT_POINT( "rpdReplicatedTransGroup::initialize::initialize::mSlotNodePool",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mSlotNodePool" );
    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_SLOT_NODE", aRepName );
    IDE_TEST( mSlotNodePool.initialize( IDU_MEM_RP_RPD,
                                        sName,
                                        2,
                                        ID_SIZEOF( rpdTransSlotNode ) * mReplicationTransGroupMaxCount,
                                        1, /* not use */
                                        IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                        ID_TRUE,							/* UseMutex */
                                        IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                        ID_FALSE,							/* ForcePooling */
                                        ID_TRUE,							/* GarbageCollection */
                                        ID_TRUE,                          /* HWCacheLine */
                                        IDU_MEMPOOL_TYPE_LEGACY           /* mempool type*/ ) 
              != IDE_SUCCESS);			
    sStage = 2;

    IDU_FIT_POINT( "rpdReplicatedTransGroup::initialize::initialize::mReplicatedTransPool",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mReplicatedTransPool" );
    idlOS::snprintf( sName, IDU_MUTEX_NAME_LEN, "RP_%s_REPLICATED_TRANS", aRepName );
    IDE_TEST( mReplicatedTransPool.initialize( IDU_MEM_RP_RPD,
                                               sName,
                                               2,
                                               ID_SIZEOF( rpdReplicatedTrans ) * mReplicationTransGroupMaxCount,
                                               1, /* not use */
                                               IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                               ID_TRUE,							/* UseMutex */
                                               IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                               ID_FALSE,						/* ForcePooling */
                                               ID_TRUE,							/* GarbageCollection */
                                               ID_TRUE,                         /* HWCacheLine */
                                               IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/ ) 
              != IDE_SUCCESS);			
    sStage = 3;

    IDE_TEST( mHeadCompletedGroupList.initialize( &mSlotNodePool,
                                              &mReplicatedTransPool,
                                              mTransTableSize,
                                              mReplicationTransGroupMaxCount ) 
              != IDE_SUCCESS );
    sStage = 4;

    mTailCompletedGroupList = &mHeadCompletedGroupList;

    IDE_TEST( mTransTable.initialize() != IDE_SUCCESS );
    sStage = 5;

    mTransTableSize = smiGetTransTblSize();

    IDU_FIT_POINT( "rpdReplicatedTransGroup::initialize::alloc::mReplicatedTransPool",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "mReplicatedTransPool->alloc" );
    IDE_TEST( mReplicatedTransNodePool.alloc( (void**)&mAnalyzingGroup ) != IDE_SUCCESS );
    sStage = 6;

    IDE_TEST( mAnalyzingGroup->initialize( &mSlotNodePool,
                                           &mReplicatedTransPool,
                                           mTransTableSize,
                                           mReplicationTransGroupMaxCount ) 
              != IDE_SUCCESS );
    sStage = 7;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 7:
            mAnalyzingGroup->finalize();
        case 6:
            (void)mReplicatedTransNodePool.memfree( mAnalyzingGroup );
        case 5:
            mTransTable.finalize();
        case 4:
            mHeadCompletedGroupList.finalize();
        case 3:
            (void)mReplicatedTransPool.destroy( ID_FALSE );
        case 2:
            (void)mSlotNodePool.destroy( ID_FALSE );
        case 1:
            (void)mReplicatedTransNodePool.destroy( ID_FALSE );
        default:
            break;

    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdReplicatedTransGroup::finalize( void )
{
    clearReplicatedTransGroup();

    mHeadCompletedGroupList.finalize();
    mTransTable.finalize();

    if ( mAnalyzingGroup != NULL )
    {
        mAnalyzingGroup->finalize();

        if ( mReplicatedTransNodePool.memfree( mAnalyzingGroup ) != IDE_SUCCESS )
        {
            IDE_ERRLOG( IDE_RP_0 );
        }
        else
        {
            /* do nohting */
        }
        mAnalyzingGroup = NULL;
    }
    else
    {
        /* do nothing */
    }

    if ( mReplicatedTransNodePool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }
    
    if ( mSlotNodePool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }

    if ( mReplicatedTransPool.destroy( ID_FALSE ) != IDE_SUCCESS )
    {
        IDE_ERRLOG( IDE_RP_0 );
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpdReplicatedTransGroup::addCompletedReplicatedTransGroup( rpdReplicatedTransGroupOperation  aOperation )
{
    UInt        i = 0;
    UInt        sTransSlotCount = 0;
    UInt        sTransSlotIndex = 0;
    idBool      sIsAlloc = ID_FALSE;

    sTransSlotCount = mAnalyzingGroup->getTransSlotCount();
    for ( i = 0; i < sTransSlotCount; i++ )
    {
        sTransSlotIndex = mAnalyzingGroup->getTransSlotIndexByIndex( i );

        mTransTable.insertNewReplicatedTransGroup( sTransSlotIndex,
                                                   mAnalyzingGroup );
    }

    mAnalyzingGroup->setOperation( aOperation );

    mTailCompletedGroupList->setNextReplicatedTransGroupNode( mAnalyzingGroup );
    mTailCompletedGroupList = mAnalyzingGroup;

    mAnalyzingGroup = NULL;

    /* alloc new group node */
    IDU_FIT_POINT( "rpdReplicatedTransGroup::addCompletedReplicatedTransGroup::alloc::mAnalyzingGroup",
                   rpERR_ABORT_RP_INTERNAL_ARG,
                   "alloc_mAnalyzingGroup" );
    IDE_TEST( mReplicatedTransNodePool.alloc( (void**)&mAnalyzingGroup ) != IDE_SUCCESS );
    sIsAlloc = ID_TRUE;

    IDE_TEST( mAnalyzingGroup->initialize( &mSlotNodePool,
                                           &mReplicatedTransPool,
                                           mTransTableSize,
                                           mReplicationTransGroupMaxCount ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS; 
    
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloc == ID_TRUE )
    {
        (void)mReplicatedTransNodePool.memfree( mAnalyzingGroup );
        mAnalyzingGroup = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdReplicatedTransGroup::removeCompleteReplicatedTransGroup( void )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;

    sNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    /* add comment */
    mHeadCompletedGroupList.setNextReplicatedTransGroupNode( 
        sNode->getNextReplicatedTransGroupNode() );

    if ( sNode == mTailCompletedGroupList )
    {
        mTailCompletedGroupList = &mHeadCompletedGroupList;
    }
    else
    {
        /* do nothing */
    }

    mTransTable.removeReplicatedTransGroup( sNode );

    sNode->finalize();

    (void)mReplicatedTransNodePool.memfree( sNode );
}

IDE_RC rpdReplicatedTransGroup::checkAndAddReplicatedTransGroupToComplete( rpdReplicatedTransGroupOperation aOperation )
{
    if ( mAnalyzingGroup->getReplicatedTransCount() >= mReplicationTransGroupMaxCount )
    {
        IDE_TEST( addCompletedReplicatedTransGroup( aOperation ) != IDE_SUCCESS );
    }
    else
    {
        /* do nothing */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


void rpdReplicatedTransGroup::insertCompleteTrans( smTID      aTransID,
                                                   smSN       aBeginSN,
                                                   smSN       aEndSN )
{
    mAnalyzingGroup->insertCompleteTrans( aTransID,
                                          aBeginSN,
                                          aEndSN );
}

UInt rpdReplicatedTransGroup::getReplicatedTransCount( void )
{
    return mAnalyzingGroup->getReplicatedTransCount();
}

smTID rpdReplicatedTransGroup::getReplicatedTransGroupTransID( smTID    aTransID )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;
    smTID       sGroupTransID = SM_NULL_TID;

    sNode = mTransTable.getFirstReplicatedTransGroupNode( aTransID );

    IDE_DASSERT( sNode != NULL );

    sGroupTransID = sNode->getGroupTransID();

    return sGroupTransID;
}

idBool rpdReplicatedTransGroup::isLastTransactionInFirstGroup( smTID     aTransID,
                                                               smSN      aEndSN )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;
    idBool          sRC = ID_FALSE;

    sNode = mTransTable.getFirstReplicatedTransGroupNode( aTransID );

    IDE_DASSERT( sNode != NULL );

    sRC = sNode->isLastTrans( aTransID,
                              aEndSN );

    return sRC;
}

void rpdReplicatedTransGroup::clearReplicatedTransGroup( void )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;

    sNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    while ( sNode != NULL )
    {
        removeCompleteReplicatedTransGroup();

        sNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();
    }

    IDE_DASSERT( mHeadCompletedGroupList.getNextReplicatedTransGroupNode() == NULL );
    IDE_DASSERT( &mHeadCompletedGroupList == mTailCompletedGroupList );

    if ( mAnalyzingGroup != NULL )
    {
        mAnalyzingGroup->clear();
    }
    else
    {
        /* do nothing */
    }
}

IDE_RC rpdReplicatedTransGroup::buildRecordForReplicatedTransGroupInfo( SChar               * aRepName,
                                                                        void                * aHeader,
                                                                        void                * aDumpObj,
                                                                        iduFixedTableMemory * aMemory )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;

    sNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    while ( sNode != NULL )
    {
        IDE_TEST( sNode->buildRecordForReplicatedTransGroupInfo( aRepName,
                                                                 aHeader,
                                                                 aDumpObj,
                                                                 aMemory )
                  != IDE_SUCCESS );

        sNode = sNode->getNextReplicatedTransGroupNode();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC rpdReplicatedTransGroup::buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                                       void                * aHeader,
                                                                       void                * aDumpObj,
                                                                       iduFixedTableMemory * aMemory )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;

    sNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    while ( sNode != NULL )
    {
        IDE_TEST( sNode->buildRecordForReplicatedTransSlotInfo( aRepName,
                                                                aHeader,
                                                                aDumpObj,
                                                                aMemory )
                  != IDE_SUCCESS );

        sNode = sNode->getNextReplicatedTransGroupNode();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void rpdReplicatedTransGroup::checkAndRemoveCompleteReplicatedTransGroup( smSN     aSN )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;
    smSN            sEndSN = SM_SN_NULL;

    sNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    while ( sNode != NULL )
    {
        sEndSN = sNode->getEndSN();

        if ( aSN == sEndSN )
        {
            sNode = sNode->getNextReplicatedTransGroupNode();

            removeCompleteReplicatedTransGroup();
        }
        else
        {
            break;
        }
    }
}

void rpdReplicatedTransGroup::getReplicatedTransGroupInfo( smTID           aTransID,
                                                           smSN            aSN,
                                                           idBool        * aIsFirstGroup,
                                                           idBool        * aIsLastLog,
                                                           rpdReplicatedTransGroupOperation    * aGroupOperation )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;
    rpdReplicatedTransGroupNode     * sHeadNextNode = NULL;

    *aIsFirstGroup = ID_FALSE;
    *aGroupOperation = RPD_REPLICATED_TRANS_GROUP_NONE;

    sNode = mTransTable.getFirstReplicatedTransGroupNode( aTransID );
    sHeadNextNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    if ( sHeadNextNode == sNode )
    {
        if ( sNode->isThisTransaction( aTransID,
                                       aSN )
                == ID_TRUE )
        {
            *aIsFirstGroup = ID_TRUE;
            *aGroupOperation = sNode->getOperation();
            *aIsLastLog = sNode->isLastTrans( aTransID,
                                              aSN );
        }
        else
        {
            *aIsFirstGroup = ID_FALSE;
        }
    }
    else
    {
        *aIsFirstGroup = ID_FALSE;
    }
}

idBool rpdReplicatedTransGroup::isThereGroupNode( void )
{
    rpdReplicatedTransGroupNode     * sHeadNextNode = NULL;
    idBool              sIsGroupNode = ID_TRUE;

    sHeadNextNode = mHeadCompletedGroupList.getNextReplicatedTransGroupNode();

    if ( sHeadNextNode != NULL )
    {
        sIsGroupNode = ID_TRUE;
    }
    else
    {
        sIsGroupNode = ID_FALSE;
    }
    
    return sIsGroupNode;
}

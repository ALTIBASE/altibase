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

#include <rpdReplicatedTransGroupNode.h>
#include <rpdTransSlotNode.h>
#include <rpdAnalyzedTransTable.h>
#include <rpuProperty.h>

#include <smiMisc.h>

IDE_RC rpdAnalyzedTransTable::initialize( void )
{
    UInt        i = 0;
    UInt        sStage = 0;

    mTransTableSize = smiGetTransTblSize();

    IDU_FIT_POINT_RAISE( "rpdAnalyzedTransTable::initialize::malloc::mAnalyzedTransTable",
                         ERR_MEMORY_ALLOC_MANLALYZEDTRANSTABLE );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       mTransTableSize * ID_SIZEOF( rpdReplicatedTransGroupNodePtr ),
                                       (void**)&mAnalyzedTransTable,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_MANLALYZEDTRANSTABLE );
    sStage = 1;

    IDU_FIT_POINT_RAISE( "rpdAnalyzedTransTable::initialize::malloc::mLastAnalyzedTransTable",
                         ERR_MEMORY_ALLOC_MLASTANLALYZEDTRANSTABLE );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       mTransTableSize * ID_SIZEOF( rpdReplicatedTransGroupNodePtr ),
                                       (void**)&mLastAnalyzedTransTable,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_MLASTANLALYZEDTRANSTABLE );
    sStage = 2;

    for ( i = 0; i < mTransTableSize; i++ )
    {
        mAnalyzedTransTable[i] = NULL;
        mLastAnalyzedTransTable[i] = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_MANLALYZEDTRANSTABLE )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initialize",
                                  "mAnalyzedTransTable" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_MLASTANLALYZEDTRANSTABLE )
    {
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initialize",
                                  "mLastAnalyzedTransTable" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 2:
            (void)iduMemMgr::free( mLastAnalyzedTransTable );
            mLastAnalyzedTransTable = NULL;
        case 1:
            (void)iduMemMgr::free( mAnalyzedTransTable );
            mAnalyzedTransTable = NULL;
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdAnalyzedTransTable::finalize( void )
{
    IDE_DASSERT( mLastAnalyzedTransTable != NULL );
    IDE_DASSERT( mAnalyzedTransTable != NULL );

    (void)iduMemMgr::free( mLastAnalyzedTransTable );
    mLastAnalyzedTransTable = NULL;

    (void)iduMemMgr::free( mAnalyzedTransTable );
    mAnalyzedTransTable = NULL;
}

void rpdAnalyzedTransTable::insertNewReplicatedTransGroup( UInt          aTransSlotIndex,
                                                           rpdReplicatedTransGroupNode * aNode )
{
    rpdReplicatedTransGroupNode     * sNode = NULL;
    rpdReplicatedTransGroupNode     * sLastNode = NULL;

    sNode = mAnalyzedTransTable[aTransSlotIndex];

    if ( sNode == NULL )
    {
        mAnalyzedTransTable[aTransSlotIndex] = aNode;
        mLastAnalyzedTransTable[aTransSlotIndex] = aNode;
    }
    else
    {
        sLastNode = mLastAnalyzedTransTable[aTransSlotIndex];

        IDE_DASSERT( sLastNode != NULL );
        IDE_DASSERT( sLastNode != aNode );

        sLastNode->setNextReplicatedTransGroupNodeInTransSlotNode( aTransSlotIndex,
                                                                   aNode );
        mLastAnalyzedTransTable[aTransSlotIndex] = aNode;
    }
}

void rpdAnalyzedTransTable::removeReplicatedTransGroup( rpdReplicatedTransGroupNode * aGroupNode )
{
    UInt                sTransSlotCount = 0;
    UInt                i = 0;
    UInt                sSlotIndex = 0;
    rpdTransSlotNode  * sTransSlotNode = NULL;

    sTransSlotCount = aGroupNode->getTransSlotCount();
    for ( i = 0; i < sTransSlotCount; i++ )
    {
        sTransSlotNode = aGroupNode->getTransSlotNodeByIndex( i );
        sSlotIndex = sTransSlotNode->getTransSlotIndex();
        
        if ( mAnalyzedTransTable[sSlotIndex] == mLastAnalyzedTransTable[sSlotIndex] )
        {
            mLastAnalyzedTransTable[sSlotIndex] = NULL;
            mAnalyzedTransTable[sSlotIndex] = NULL;
        }
        else
        {
            mAnalyzedTransTable[sSlotIndex] = sTransSlotNode->getNextReplicatedTransGroupNode();
        }
    }
}

rpdReplicatedTransGroupNode * rpdAnalyzedTransTable::getFirstReplicatedTransGroupNode( smTID      aTransID )
{
    UInt sIndex = 0;

    sIndex = aTransID % mTransTableSize;

    return mAnalyzedTransTable[sIndex];
}


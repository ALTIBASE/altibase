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

#include <rpdTransTbl.h>
#include <rpdAnalyzingTransTable.h>

#include <smiMisc.h>

IDE_RC rpdAnalyzingTransTable::initialize( void )
{
    UInt            i = 0;
    idBool          sIsAlloc = ID_FALSE;

    mTransTableSize = smiGetTransTblSize();

    IDU_FIT_POINT_RAISE( "rpdAnalyzingTransTable::initialize::malloc::mTransTable",
                         ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       mTransTableSize * ID_SIZEOF( rpdAnalyzingTransNode ),
                                       (void**)&mTransTable,
                                       IDU_MEM_IMMEDIATE ) 
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC );
    sIsAlloc = ID_TRUE;

    for ( i = 0; i < mTransTableSize; i++ )
    {
        IDU_LIST_INIT( &(mTransTable[i].mItemMetaList) );
    }

    clearActiveTransTable();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "initialize",
                                  "mTransTable" ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsAlloc == ID_TRUE )
    {
        (void)iduMemMgr::free( mTransTable );
        mTransTable = NULL;
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdAnalyzingTransTable::finalize( void )
{
    if ( mTransTable != NULL )
    {
        (void)iduMemMgr::free( mTransTable );
        mTransTable = NULL;
    }
    else
    {
        /* do nothing */
    }
}

void rpdAnalyzingTransTable::clearTransNode( rpdAnalyzingTransNode  * aNode )
{
    iduListNode       * sNode = NULL;
    iduListNode       * sDummy = NULL;
    rpdItemMetaEntry  * sItemMetaEntry = NULL;

    aNode->mIsActive = ID_FALSE;
    aNode->mTransID = SM_NULL_TID;
    aNode->mBeginSN = SM_SN_NULL;
    aNode->mAllowToGroup = ID_TRUE;
    aNode->mIsReplicationTrans = ID_FALSE;
    aNode->mIsDDLTrans = ID_FALSE;

    IDU_LIST_ITERATE_SAFE( &(aNode->mItemMetaList), sNode, sDummy )
    {
        sItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
        IDU_LIST_REMOVE(sNode);

        (void)iduMemMgr::free(sItemMetaEntry->mLogBody);
        (void)iduMemMgr::free(sItemMetaEntry);
    }
}

void rpdAnalyzingTransTable::clearActiveTransTable( void )
{
    UInt        i = 0;

    for ( i = 0; i < mTransTableSize; i++ )
    {
        clearTransNode( &mTransTable[i] );
    }
}

UInt rpdAnalyzingTransTable::getIndex( smTID    aTransID )
{
    return aTransID % mTransTableSize;
}

void rpdAnalyzingTransTable::clearTransNodeFromTransID( smTID    aTransID )
{
    UInt        sIndex = 0;

    sIndex = getIndex( aTransID );

    clearTransNode( &mTransTable[sIndex] );
}

void rpdAnalyzingTransTable::setActiveTrans( smTID  aTransID,
                                             smSN   aBeginSN )
{
    UInt        sIndex = 0;

    sIndex = getIndex( aTransID );

    mTransTable[sIndex].mIsActive = ID_TRUE;
    mTransTable[sIndex].mTransID = aTransID;
    mTransTable[sIndex].mBeginSN = aBeginSN;
    mTransTable[sIndex].mAllowToGroup = ID_TRUE;
}

idBool rpdAnalyzingTransTable::isActiveTrans( smTID     aTransID )
{
    UInt        sIndex = 0;

    sIndex = getIndex( aTransID );

    return mTransTable[sIndex].mIsActive;
}

void rpdAnalyzingTransTable::setDisableToGroup( smTID        aTransID )
{
    UInt        sIndex = 0;

    sIndex = getIndex( aTransID );

    mTransTable[sIndex].mAllowToGroup = ID_FALSE;
}

idBool rpdAnalyzingTransTable::shouldBeGroup( smTID     aTransID )
{
    UInt        sIndex = 0;

    sIndex = getIndex( aTransID );

    return mTransTable[sIndex].mAllowToGroup;
}

smSN rpdAnalyzingTransTable::getBeginSN( smTID       aTransID )
{
    UInt        sIndex = 0;

    sIndex = getIndex( aTransID );

    return mTransTable[sIndex].mBeginSN;
}

void rpdAnalyzingTransTable::setReplicationTrans( smTID     aTransID )
{
    UInt    sIndex = 0;

    sIndex = getIndex( aTransID );

    mTransTable[sIndex].mIsReplicationTrans = ID_TRUE;
}

idBool rpdAnalyzingTransTable::isReplicationTrans( smTID    aTransID )
{
    UInt    sIndex = 0;

    sIndex = getIndex( aTransID );

    return mTransTable[sIndex].mIsReplicationTrans;
}

smTID rpdAnalyzingTransTable::getTransIDByIndex( UInt       aIndex )
{
    return mTransTable[aIndex].mTransID;
}

smSN rpdAnalyzingTransTable::getBeginSNByIndex( UInt     aIndex )
{
    return mTransTable[aIndex].mBeginSN;
}
    
void rpdAnalyzingTransTable::setDisableToGroupByIndex( UInt     aIndex )
{
    mTransTable[aIndex].mAllowToGroup = ID_FALSE;
}

idBool rpdAnalyzingTransTable::isActiveTransByIndex( UInt       aIndex )
{
    return mTransTable[aIndex].mIsActive;
}

idBool rpdAnalyzingTransTable::isReplicationTransByIndex( UInt aIndex )
{
    return mTransTable[aIndex].mIsReplicationTrans;
}

void rpdAnalyzingTransTable::setDDLTrans( smTID aTransID )
{
    UInt    sIndex = 0;

    sIndex = getIndex( aTransID );

    mTransTable[sIndex].mIsDDLTrans = ID_TRUE;
}

idBool rpdAnalyzingTransTable::isDDLTrans( smTID aTransID )
{
    UInt    sIndex = 0;

    sIndex = getIndex( aTransID );

    return mTransTable[sIndex].mIsDDLTrans;
}

IDE_RC rpdAnalyzingTransTable::addItemMetaEntry( smTID          aTID,
                                                 smiTableMeta * aItemMeta,
                                                 const void   * aItemMetaLogBody,
                                                 UInt           aItemMetaLogBodySize,
                                                 smSN           aLogSN )
{
    rpdItemMetaEntry * sItemMetaEntry = NULL;
    UInt               sIndex         = aTID % mTransTableSize;

    IDU_FIT_POINT_RAISE( "rpdAnalyzingTransTable::addItemMetaEntry::malloc::sItemMetaEntry",
                         ERR_MEMORY_ALLOC_ITEM_META_ENTRY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                       ID_SIZEOF(rpdItemMetaEntry),
                                       (void **)&sItemMetaEntry,
                                       IDU_MEM_IMMEDIATE )
                    != IDE_SUCCESS, ERR_MEMORY_ALLOC_ITEM_META_ENTRY );
    idlOS::memset( (void *)sItemMetaEntry, 0x00, ID_SIZEOF(rpdItemMetaEntry) );

    idlOS::memcpy( (void *)&sItemMetaEntry->mItemMeta,
                   (const void  *)aItemMeta,
                   ID_SIZEOF(smiTableMeta) );

    IDU_FIT_POINT_RAISE( "rpdAnalyzingTransTable::addItemMetaEntry::malloc::sItemMetaEntry->mLogBody",
                         ERR_MEMORY_ALLOC_ITEM_META_ENTRY );

    if ( aItemMetaLogBodySize > 0 )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_RP_RPD,
                                           (ULong)aItemMetaLogBodySize,
                                           (void **)&sItemMetaEntry->mLogBody,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS, ERR_MEMORY_ALLOC_LOG_BODY);
        idlOS::memcpy( (void *)sItemMetaEntry->mLogBody,
                       aItemMetaLogBody,
                       aItemMetaLogBodySize );
    }
    else
    {
        sItemMetaEntry->mLogBody = NULL;
    }
    sItemMetaEntry->mLogSN = aLogSN;
    // 이후에는 실패하지 않는다
    IDU_LIST_INIT_OBJ( &(sItemMetaEntry->mNode), sItemMetaEntry );
    IDU_LIST_ADD_LAST( &mTransTable[sIndex].mItemMetaList,
                       &(sItemMetaEntry->mNode) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC_ITEM_META_ENTRY );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpdAnalyingTransTable::addItemMetaEntry",
                                  "sItemMetaEntry" ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC_LOG_BODY );
    {
        IDE_ERRLOG( IDE_RP_0 );
        IDE_SET( ideSetErrorCode( rpERR_ABORT_MEMORY_ALLOC,
                                  "rpdAnalyzingTransTable::addItemMetaEntry",
                                  "sItemMetaEntry->mLogBody") );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sItemMetaEntry != NULL )
    {
        if ( sItemMetaEntry->mLogBody != NULL )
        {
            (void)iduMemMgr::free( sItemMetaEntry->mLogBody );
        }
        else
        {
            /* do nothing */
        }

        (void)iduMemMgr::free( sItemMetaEntry );
    }
    else
    {
        /* do nothing */
    }

    IDE_POP();

    return IDE_FAILURE;
}

void rpdAnalyzingTransTable::getFirstItemMetaEntry( smTID               aTID,
                                                    rpdItemMetaEntry ** aItemMetaEntry )
{
    iduListNode * sNode = NULL;
    iduListNode * sDummy = NULL;
    UInt          sIndex = aTID % mTransTableSize;

    *aItemMetaEntry = NULL;

    if ( IDU_LIST_IS_EMPTY( &mTransTable[sIndex].mItemMetaList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mTransTable[sIndex].mItemMetaList, sNode, sDummy )
        {
            *aItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
            break;
        }
    }
    else
    {
        /* do nothing */
    }

    IDE_DASSERT( *aItemMetaEntry != NULL );

    return;
}

void rpdAnalyzingTransTable::removeFirstItemMetaEntry( smTID aTID )
{
    iduListNode      * sNode = NULL;
    iduListNode      * sDummy = NULL;
    UInt               sIndex = aTID % mTransTableSize;
    rpdItemMetaEntry * sItemMetaEntry = NULL;

    if ( IDU_LIST_IS_EMPTY( &mTransTable[sIndex].mItemMetaList ) != ID_TRUE )
    {
        IDU_LIST_ITERATE_SAFE( &mTransTable[sIndex].mItemMetaList, sNode, sDummy )
        {
            sItemMetaEntry = (rpdItemMetaEntry *)sNode->mObj;
            IDU_LIST_REMOVE( sNode );

            (void)iduMemMgr::free( (void *)sItemMetaEntry->mLogBody );
            (void)iduMemMgr::free( (void *)sItemMetaEntry );
            break;
        }
    }
    else
    {
        /* do nothing */
    }

    return;
}

idBool rpdAnalyzingTransTable::existItemMeta( smTID aTID )
{
    UInt sIndex = aTID % mTransTableSize;
    idBool  sResult = ID_FALSE;

    if ( IDU_LIST_IS_EMPTY( &mTransTable[sIndex].mItemMetaList ) != ID_TRUE )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = ID_FALSE;
    }

    return sResult;
}


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
 * $id$
 **********************************************************************/

#include <ide.h>
#include <idl.h>

#include <mtc.h>

#include <dkpDef.h>

#include <dkdResultSetMetaCache.h>

/*
 *
 */ 
typedef struct dkdRemoteQueryListNode
{
    SChar * mRemoteQuery;

    UInt mColumnCount;
    dkpColumn * mSourceColumnArray;
    mtcColumn * mDestColumnArray;

    iduListNode mListNode;
    
} dkdRemoteQueryListNode;

/*
 *
 */
typedef struct dkdDatabaseLinkListNode
{
    SChar mDatabaseLinkName[ DK_NAME_LEN + 1 ];
    
    iduList mRemoteQueryListHead;

    iduListNode mListNode;

} dkdDatabaseLinkListNode;

/*
 *
 */ 
typedef struct dkdResultSetMetaCache
{
    iduMutex mMutex;

    iduList mDatabaseLinkListHead;

} dkdResultSetMetaCache;

typedef enum dkdResultSetMetaCacheState {

    DKD_RESULT_SET_META_CACHE_STATE_UNINITIALIZED = 0,
    DKD_RESULT_SET_META_CACHE_STATE_INITIALIZED
    
} dkdResultSetMetaCacheState ;

static dkdResultSetMetaCacheState gResultSetMetaCacheState =
    DKD_RESULT_SET_META_CACHE_STATE_UNINITIALIZED;

static dkdResultSetMetaCache gResultSetMetaCache;


/*
 *
 */
static IDE_RC allocRemoteQueryListNode( UInt aRemoteQueryLength,
                                        UInt aColumnCount,
                                        dkdRemoteQueryListNode ** aNode )
{
    dkdRemoteQueryListNode * sNode = NULL;
    SInt sStage = 0;

    IDU_FIT_POINT( "allocRemoteQueryListNode::malloc::Node" );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 ID_SIZEOF( *sNode ),
                                 (void **)&sNode,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sStage = 1;

    IDU_FIT_POINT( "allocRemoteQueryListNode::malloc::RemoteQuery" );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 ID_SIZEOF( SChar ) * (aRemoteQueryLength + 1),
                                 (void **)&(sNode->mRemoteQuery),
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sStage = 2;

    IDU_FIT_POINT( "allocRemoteQueryListNode::malloc::SourceColumnArray" );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 ID_SIZEOF( dkpColumn ) * aColumnCount,
                                 (void **)&(sNode->mSourceColumnArray),
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sStage = 3;

    IDU_FIT_POINT( "allocRemoteQueryListNode::malloc::DestColumnArray" );
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 ID_SIZEOF( mtcColumn ) * aColumnCount,
                                 (void **)&(sNode->mDestColumnArray),
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );

    *aNode = sNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    switch ( sStage )
    {
        case 3:
            (void)iduMemMgr::free( sNode->mSourceColumnArray );
        case 2:
            (void)iduMemMgr::free( sNode->mRemoteQuery );
        case 1:
            (void)iduMemMgr::free( sNode );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
 *
 */
static IDE_RC createRemoteQueryListNode( SChar * aRemoteQuery,
                                        UInt aRemoteQueryLength,
                                        UInt aColumnCount,
                                        dkpColumn * aSourceColumnArray,
                                        mtcColumn *aDestColumnArray,
                                        dkdRemoteQueryListNode ** aNode )
{
    dkdRemoteQueryListNode * sNode = NULL;
    UInt i = 0;
    
    IDE_TEST( allocRemoteQueryListNode( aRemoteQueryLength,
                                        aColumnCount,
                                        &sNode )
              != IDE_SUCCESS );
    
    (void)idlOS::memcpy( sNode->mRemoteQuery,
                         aRemoteQuery,
                         aRemoteQueryLength );
    sNode->mRemoteQuery[ aRemoteQueryLength ] = '\0';
    
    for ( i = 0; i < aColumnCount; i++ )
    {
        (void)idlOS::memcpy( &(sNode->mSourceColumnArray[i]),
                             &(aSourceColumnArray[i]),
                             ID_SIZEOF( dkpColumn ) );
        
        (void)idlOS::memcpy( &(sNode->mDestColumnArray[i]),
                             &(aDestColumnArray[i]),
                             ID_SIZEOF( mtcColumn ) );
    }

    IDU_LIST_INIT_OBJ( &(sNode->mListNode), sNode );

    sNode->mColumnCount = aColumnCount;
    
    *aNode = sNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */
static void destroyRemoteQueryListNode( dkdRemoteQueryListNode * aNode )
{
    IDE_PUSH();

    (void)iduMemMgr::free( aNode->mDestColumnArray );

    (void)iduMemMgr::free( aNode->mSourceColumnArray );

    (void)iduMemMgr::free( aNode->mRemoteQuery );
            
    (void)iduMemMgr::free( aNode );

    IDE_POP();
}

/*
 *
 */
static void freeRemoteQueryList( iduList * aHead )
{
    iduListNode * sIterator = NULL;
    iduListNode * sNextNode = NULL;
    dkdRemoteQueryListNode * sCurrentNode = NULL;

    IDU_LIST_ITERATE_SAFE( aHead, sIterator, sNextNode )
    {
        sCurrentNode = (dkdRemoteQueryListNode *)sIterator->mObj;

        IDU_LIST_REMOVE( sIterator );
        
        destroyRemoteQueryListNode( sCurrentNode );        
    }
}

/*
 *
 */
static void removeRemoteQueryListNode( iduList * aHead,
                                       SChar * aRemoteQuery )
{
    iduListNode * sIterator = NULL;
    iduListNode * sNextNode = NULL;
    dkdRemoteQueryListNode * sCurrentNode = NULL;

    IDU_LIST_ITERATE_SAFE( aHead, sIterator, sNextNode )
    {
        sCurrentNode = (dkdRemoteQueryListNode *)sIterator->mObj;
        
        if ( idlOS::strcasecmp( sCurrentNode->mRemoteQuery,
                                aRemoteQuery )
             == 0 )
        {
            IDU_LIST_REMOVE( sIterator );
            
            destroyRemoteQueryListNode( sCurrentNode );
            
            break;            
        }
        else
        {
            /* nothing to do */
        }
    }
}

/*
 *
 */
static dkdRemoteQueryListNode * searchRemoteQueryListNode(
    iduList * aHead,
    SChar * aRemoteQuery )
{
    iduListNode * sIterator = NULL;
    dkdRemoteQueryListNode * sCurrentNode = NULL;
    dkdRemoteQueryListNode * sFoundNode = NULL;    

    IDU_LIST_ITERATE( aHead, sIterator )
    {
        sCurrentNode = (dkdRemoteQueryListNode *)sIterator->mObj;
        
        if ( idlOS::strcasecmp( sCurrentNode->mRemoteQuery,
                            aRemoteQuery )
             == 0 )
        {
            sFoundNode = sCurrentNode;
            
            break;
        }
        else
        {
            /* nothing to do */
        }
    }

    return sFoundNode;
}

/*
 *
 */ 
static IDE_RC createDatabaseLinkListNode( SChar * aLinkName,    
                                          dkdDatabaseLinkListNode ** aNode )
{
    dkdDatabaseLinkListNode * sNode = NULL;
    SInt sStage = 0;
    
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_DK,
                                 ID_SIZEOF( *sNode ),
                                 (void **)&sNode,
                                 IDU_MEM_IMMEDIATE )
              != IDE_SUCCESS );
    sStage = 1;
    
    (void)idlOS::strncpy( sNode->mDatabaseLinkName,
                          aLinkName,
                          ID_SIZEOF( sNode->mDatabaseLinkName) );
    sNode->mDatabaseLinkName[ DK_NAME_LEN ] = '\0';
    
    IDU_LIST_INIT( &(sNode->mRemoteQueryListHead) );
    
    IDU_LIST_INIT_OBJ( &(sNode->mListNode), sNode );

    *aNode = sNode;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sStage )
    {
        case 1:
            (void)iduMemMgr::free( sNode );
        default:
            break;
    }
    IDE_POP();
    
    return IDE_FAILURE;
}

/*
 *
 */
static void destroyDatabaseLinkListNode( dkdDatabaseLinkListNode * aNode )
{
    IDE_PUSH();
    
    (void)iduMemMgr::free( aNode );

    IDE_POP();
}

/*
 *
 */
static void freeDatabaseLinkList( iduList * aHead )
{
    iduList * sIterator = NULL;
    iduList * sNodeNext = NULL;
    dkdDatabaseLinkListNode * sDatabaseLinkListNode = NULL;
    
    IDU_LIST_ITERATE_SAFE( aHead, sIterator, sNodeNext)
    {
        IDU_LIST_REMOVE( sIterator );
        
        sDatabaseLinkListNode = (dkdDatabaseLinkListNode *)sIterator->mObj;
        
        freeRemoteQueryList( &(sDatabaseLinkListNode->mRemoteQueryListHead) );
    
        destroyDatabaseLinkListNode( sDatabaseLinkListNode );
    }
}

/*
 *
 */
static void removeDatabaseLinkListNode( iduList * aHead,
                                        SChar * aLinkName )
{
    iduList * sIterator = NULL;
    iduList * sNodeNext = NULL;
    dkdDatabaseLinkListNode * sCurrentNode = NULL;

    IDU_LIST_ITERATE_SAFE( aHead, sIterator, sNodeNext )
    {
        sCurrentNode = (dkdDatabaseLinkListNode *)sIterator->mObj;

        if ( idlOS::strncasecmp(
                 sCurrentNode->mDatabaseLinkName,
                 aLinkName,
                 ID_SIZEOF( sCurrentNode->mDatabaseLinkName ) )
             == 0 )
        {
            IDU_LIST_REMOVE( sIterator );
        
            freeRemoteQueryList( &(sCurrentNode->mRemoteQueryListHead) );

            destroyDatabaseLinkListNode( sCurrentNode );
            
            break;
        }
        else
        {
            /* do nothing */
        }
    }
}

/*
 *
 */
static dkdDatabaseLinkListNode * searchDatabaseLinkListNode(
    iduList * aHead,
    SChar * aLinkName )
{
    iduList * sIterator = NULL;
    iduList * sNodeNext = NULL;
    dkdDatabaseLinkListNode * sCurrentNode = NULL;
    dkdDatabaseLinkListNode * sFoundNode = NULL;
    
    IDU_LIST_ITERATE_SAFE( aHead, sIterator, sNodeNext )
    {
        sCurrentNode = (dkdDatabaseLinkListNode *)sIterator->mObj;

        if ( idlOS::strncasecmp(
                 sCurrentNode->mDatabaseLinkName,
                 aLinkName,
                 ID_SIZEOF( sCurrentNode->mDatabaseLinkName ) )
             == 0 )
        {
            sFoundNode = sCurrentNode;
            break;
        }
        else
        {
            /* do nothing */
        }
    }

    return sFoundNode;
}


/*
 *
 */ 
IDE_RC dkdResultSetMetaCacheInitialize( void )
{
    if ( gResultSetMetaCacheState ==
         DKD_RESULT_SET_META_CACHE_STATE_UNINITIALIZED )
    {
        IDE_TEST( gResultSetMetaCache.mMutex.initialize(
                      (SChar *)"DKD_DATABASE_LINK_LIST_NODE_MUTEX",
                      IDU_MUTEX_KIND_POSIX,
                      IDV_WAIT_INDEX_NULL)
                  != IDE_SUCCESS );

        IDU_LIST_INIT( &(gResultSetMetaCache.mDatabaseLinkListHead) );
        
        gResultSetMetaCacheState = DKD_RESULT_SET_META_CACHE_STATE_INITIALIZED;
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkdResultSetMetaCacheFinalize( void )
{
    if ( gResultSetMetaCacheState ==
         DKD_RESULT_SET_META_CACHE_STATE_INITIALIZED )
    {
        gResultSetMetaCacheState =
            DKD_RESULT_SET_META_CACHE_STATE_UNINITIALIZED;

        IDE_ASSERT( gResultSetMetaCache.mMutex.lock( NULL ) == IDE_SUCCESS );
        
        freeDatabaseLinkList( &(gResultSetMetaCache.mDatabaseLinkListHead) );

        IDE_ASSERT( gResultSetMetaCache.mMutex.unlock() == IDE_SUCCESS );
        
        (void)gResultSetMetaCache.mMutex.destroy();
    }
    else
    {
        /* do nothing */
    }
    
    return IDE_SUCCESS;
}

/*
 *
 */ 
static IDE_RC checkInitialized( void )
{
    IDE_TEST_RAISE( gResultSetMetaCacheState !=
                    DKD_RESULT_SET_META_CACHE_STATE_INITIALIZED,
                    ERROR_UNINITIALIZED );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_UNINITIALIZED )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKM_DBLINK_DISABLED ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkdResultSetMetaCacheInsert( SChar * aLinkName,
                                    SChar * aRemoteQuery,
                                    UInt aColumnCount,
                                    dkpColumn * aSourceColumnArray,
                                    mtcColumn * aDestColumnArray )
{
    UInt sRemoteQueryLength;
    dkdDatabaseLinkListNode * sLinkNode = NULL;
    dkdRemoteQueryListNode * sQueryNode = NULL;    
    SInt sStage = 0;
    
    IDE_TEST( checkInitialized() != IDE_SUCCESS );
    
    sRemoteQueryLength = idlOS::strlen( aRemoteQuery );
    
    IDE_TEST_RAISE( sRemoteQueryLength == 0, ERROR_QUERY_LENGTH );
    IDE_TEST_RAISE( aColumnCount == 0, ERROR_COLUMN_COUNT );

    IDE_ASSERT( gResultSetMetaCache.mMutex.lock( NULL ) == IDE_SUCCESS );
    sStage = 1;
    
    sLinkNode = searchDatabaseLinkListNode(
        &(gResultSetMetaCache.mDatabaseLinkListHead),
        aLinkName );

    if ( sLinkNode != NULL )
    {
        removeRemoteQueryListNode( &(sLinkNode->mRemoteQueryListHead),
                                   aRemoteQuery );
    }
    else
    {
        IDE_TEST( createDatabaseLinkListNode( aLinkName,
                                              &sLinkNode )
                  != IDE_SUCCESS );
        
        IDU_LIST_ADD_LAST( &(gResultSetMetaCache.mDatabaseLinkListHead),
                           &(sLinkNode->mListNode) );
    }

    IDE_TEST( createRemoteQueryListNode( aRemoteQuery,
                                         sRemoteQueryLength,
                                         aColumnCount,
                                         aSourceColumnArray,
                                         aDestColumnArray,
                                         &sQueryNode )
              != IDE_SUCCESS );

    IDU_LIST_ADD_LAST( &(sLinkNode->mRemoteQueryListHead),
                       &(sQueryNode->mListNode) );
    
    sStage = 0; 
    IDE_ASSERT( gResultSetMetaCache.mMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERROR_QUERY_LENGTH )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKD_REMOTE_QUERY_EMPTY ) );
    }
    IDE_EXCEPTION( ERROR_COLUMN_COUNT )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKD_NO_COLUMN ) );
    }
    IDE_EXCEPTION_END;

    switch ( sStage )
    {
        case 1:
            IDE_ASSERT( gResultSetMetaCache.mMutex.unlock() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/*
 * dkdResultSetMetaCacheRelease() has to be called after this function succeed.
 */ 
IDE_RC dkdResultSetMetaCacheGetAndHold( SChar * aLinkName,
                                        SChar * aRemoteQuery,
                                        UInt * aColumnCount,
                                        dkpColumn ** aSourceColumnArray,
                                        mtcColumn ** aDestColumnArray )
{
    dkdDatabaseLinkListNode * sLinkNode = NULL;
    dkdRemoteQueryListNode * sQueryNode = NULL;
    UInt sColumnCount = 0;
    dkpColumn * sSourceColumnArray = NULL;
    mtcColumn * sDestColumnArray = NULL;
    
    IDE_TEST( checkInitialized() != IDE_SUCCESS );

    IDE_ASSERT( gResultSetMetaCache.mMutex.lock( NULL ) == IDE_SUCCESS );

    sLinkNode = searchDatabaseLinkListNode(
        &(gResultSetMetaCache.mDatabaseLinkListHead),
        aLinkName );

    if ( sLinkNode != NULL )
    {

        sQueryNode = searchRemoteQueryListNode(
            &(sLinkNode->mRemoteQueryListHead),
            aRemoteQuery );
        
        if ( sQueryNode != NULL )
        {
            sColumnCount = sQueryNode->mColumnCount;
            sSourceColumnArray = sQueryNode->mSourceColumnArray;
            sDestColumnArray = sQueryNode->mDestColumnArray;
        }
        else
        {
            /* do nothing */
        }
    }
    else
    {
        /* do nothing */
    }

    /* Unlock is done by dkdResultSetMetaCacheRelese() */
    
    *aColumnCount = sColumnCount;
    *aSourceColumnArray = sSourceColumnArray;
    *aDestColumnArray = sDestColumnArray;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkdResultSetMetaCacheRelese( void )
{
    IDE_TEST( checkInitialized() != IDE_SUCCESS );
    
    IDE_ASSERT( gResultSetMetaCache.mMutex.unlock() == IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
 *
 */ 
IDE_RC dkdResultSetMetaCacheDelete( SChar * aLinkName )
{
    IDE_TEST( checkInitialized() != IDE_SUCCESS );

    IDE_ASSERT( gResultSetMetaCache.mMutex.lock( NULL ) == IDE_SUCCESS );

    removeDatabaseLinkListNode( &(gResultSetMetaCache.mDatabaseLinkListHead),
                                aLinkName );
    
    IDE_ASSERT( gResultSetMetaCache.mMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 *
 */ 
IDE_RC dkdResultSetMetaCacheInvalid( SChar * aLinkName,
                                     SChar * aRemoteQuery )
{
    dkdDatabaseLinkListNode * sCurrentNode = NULL;

    IDE_TEST( checkInitialized() != IDE_SUCCESS );

    IDE_ASSERT( gResultSetMetaCache.mMutex.lock( NULL ) == IDE_SUCCESS );

    sCurrentNode = searchDatabaseLinkListNode(
        &(gResultSetMetaCache.mDatabaseLinkListHead),
        aLinkName );

    if ( sCurrentNode != NULL )
    {
        removeRemoteQueryListNode( &(sCurrentNode->mRemoteQueryListHead),
                                   aRemoteQuery );
    }
    else
    {
        /* do nothing */
    }

    IDE_ASSERT( gResultSetMetaCache.mMutex.unlock() == IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

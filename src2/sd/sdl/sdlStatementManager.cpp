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
 * $Id$ sdlStatementManager.cpp
 **********************************************************************/

#include <idl.h>
#include <sdi.h>
#include <sdl.h>
#include <sdlStatement.h>
#include <sdlStatementManager.h>

static inline void freeRemoteStatementInNode( const sdlRemoteNode     * aRemoteNode,
                                              sdiConnectInfo          * aConnectInfo,
                                              const UChar               aMode )
{
    iduListNode    * sIterStmt     = NULL;
    sdlRemoteStmt  * sRemoteStmt   = NULL;

    IDU_LIST_ITERATE( &aRemoteNode->mStmtListHead, sIterStmt )
    {
        sRemoteStmt = (sdlRemoteStmt *)sIterStmt->mObj;

        if ( sRemoteStmt->mStmt != NULL )
        {
            (void) sdl::freeStmt( aConnectInfo,
                                  sRemoteStmt,
                                  aMode,
                                  &(aConnectInfo->mLinkFailure) );
        }
    }
}

void sdlStatementManager::initializeStatement( sdiStatement * aSdStmt )
{
    IDU_LIST_INIT( &aSdStmt->mNodeListHead );
}

IDE_RC sdlStatementManager::getNodeContainer( sdiStatement   * aSdStmt,
                                              UInt             aNodeId,
                                              sdlRemoteNode ** aNode )
{
    iduListNode   * sIterator    = NULL;
    sdlRemoteNode * sNode        = NULL;
    idBool          sIsNodeAlloc = ID_FALSE;
    idBool          sIsNodeAdd   = ID_FALSE;

    IDU_LIST_ITERATE( &aSdStmt->mNodeListHead, sIterator )
    {
        if ( ((sdlRemoteNode *)sIterator)->mNodeId == aNodeId )
        {
            sNode = (sdlRemoteNode *)sIterator->mObj;
            break;
        }
    }

    if ( sNode == NULL )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SD_STMT,
                                           ID_SIZEOF( sdlRemoteNode ),
                                           (void **)&sNode,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS,
                        InsufficientMemory );
        sIsNodeAlloc = ID_TRUE;

        initRemoteNode( sNode );

        sNode->mNodeId = aNodeId;

        /* add to sdiStatement::mNodeListHead */
        IDU_LIST_ADD_AFTER( &aSdStmt->mNodeListHead, &sNode->mListNode );
        sIsNodeAdd = ID_TRUE;
    }

    *aNode = sNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InsufficientMemory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }    
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsNodeAlloc == ID_TRUE )
    {
        if ( sIsNodeAdd == ID_TRUE )
        {
            IDU_LIST_REMOVE( &sNode->mListNode );
        }
        (void)iduMemMgr::free( sNode );
        sNode = NULL;
    }

    IDE_POP();

    *aNode = NULL;
    
    return IDE_FAILURE;
}

IDE_RC sdlStatementManager::getStmtContainer( sdlRemoteNode  * aNode,
                                              sdlRemoteStmt ** aStmt )
{
    sdlRemoteStmt * sStmt        = NULL;
    iduListNode   * sIterator    = NULL;
    idBool          sIsStmtAlloc = ID_FALSE;
    idBool          sIsStmtAdd   = ID_FALSE;

    IDU_LIST_ITERATE( &aNode->mStmtListHead, sIterator )
    {
        if ( ( ((sdlRemoteStmt *)sIterator)->mFreeFlag & SDL_STMT_FREE_FLAG_IN_USE_MASK ) 
             == SDL_STMT_FREE_FLAG_IN_USE_FALSE )
        {
            sStmt = (sdlRemoteStmt *)sIterator->mObj;
            break;
        }
    }

    if ( sStmt == NULL )
    {
        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SD_STMT,
                                           ID_SIZEOF( sdlRemoteStmt ),
                                           (void **)&sStmt,
                                           IDU_MEM_IMMEDIATE )
                        != IDE_SUCCESS,
                        InsufficientMemory );
        sIsStmtAlloc = ID_TRUE;

        initRemoteStmt( sStmt );

        /* add to sdlRemoteNode::mStmtListHead */
        IDU_LIST_ADD_AFTER( &aNode->mStmtListHead, &sStmt->mListNode );
        sIsStmtAdd = ID_TRUE;
    }

    *aStmt = sStmt;

    return IDE_SUCCESS;

    IDE_EXCEPTION( InsufficientMemory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }    
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sIsStmtAlloc == ID_TRUE )
    {
        if ( sIsStmtAdd == ID_TRUE )
        {
            IDU_LIST_REMOVE( &sStmt->mListNode );
        }
        (void)iduMemMgr::free( sStmt );
        sStmt = NULL;
    }

    IDE_POP();

    *aStmt = NULL;
    
    return IDE_FAILURE;
}

IDE_RC sdlStatementManager::allocRemoteStatement( sdiStatement   * aSdStmt,
                                                  UInt             aNodeId,
                                                  sdlRemoteStmt ** aRemoteStmt )
{
    sdlRemoteNode * sNode   = NULL;
    sdlRemoteStmt * sStmt   = NULL;

    IDE_TEST_RAISE( aRemoteStmt == NULL, InternalError );

    IDE_DASSERT( *aRemoteStmt == NULL );
    *aRemoteStmt = NULL;

    IDE_TEST( getNodeContainer( aSdStmt, aNodeId, &sNode )
              != IDE_SUCCESS );

    IDE_TEST( getStmtContainer( sNode, &sStmt )
              != IDE_SUCCESS );

    sStmt->mFreeFlag &= ~SDL_STMT_FREE_FLAG_IN_USE_MASK;
    sStmt->mFreeFlag |= SDL_STMT_FREE_FLAG_IN_USE_TRUE;

    sStmt->mRemoteStmtId = sNode->mCurRemoteStmtId;
    sNode->mCurRemoteStmtId++;

    *aRemoteStmt = sStmt;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( InternalError );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InternalServerError ) );
    }    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void sdlStatementManager::setUnused( sdlRemoteStmt ** aRemoteStmt )
{
    if ( *aRemoteStmt != NULL )
    {
        (*aRemoteStmt)->mFreeFlag &= ~SDL_STMT_FREE_FLAG_IN_USE_MASK;
        (*aRemoteStmt)->mFreeFlag |= SDL_STMT_FREE_FLAG_IN_USE_FALSE;
        *aRemoteStmt = NULL;
    }
}

void sdlStatementManager::freeAllRemoteStatement( sdiStatement  * aSdStmt,
                                                  sdiClientInfo * aClientInfo,
                                                  UChar           aMode )
{
    iduListNode    * sIterNode     = NULL;
    sdlRemoteNode  * sRemoteNode   = NULL;
    sdiConnectInfo * sConnectInfo  = NULL;

    IDE_TEST_CONT( aClientInfo == NULL, SKIP );

    sConnectInfo = aClientInfo->mConnectInfo;

    IDU_LIST_ITERATE( &aSdStmt->mNodeListHead, sIterNode )
    {
        sRemoteNode = (sdlRemoteNode *)sIterNode->mObj;

        freeRemoteStatementInNode( sRemoteNode,
                                   sConnectInfo,
                                   aMode );
    }

    IDE_EXCEPTION_CONT( SKIP );
}

void sdlStatementManager::freeRemoteStatement( sdiStatement  * aSdStmt,
                                               sdiClientInfo * aClientInfo,
                                               UInt            aNodeId,
                                               UChar           aMode )
{
    iduListNode    * sIterNode     = NULL;
    sdlRemoteNode  * sRemoteNode   = NULL;
    sdiConnectInfo * sConnectInfo  = NULL;

    IDE_TEST_CONT( aClientInfo == NULL, SKIP );

    sConnectInfo = aClientInfo->mConnectInfo;

    IDU_LIST_ITERATE( &aSdStmt->mNodeListHead, sIterNode )
    {
        sRemoteNode = (sdlRemoteNode *)sIterNode->mObj;

        if ( aNodeId == sRemoteNode->mNodeId )
        {
            freeRemoteStatementInNode( sRemoteNode,
                                       sConnectInfo,
                                       aMode );
            break;
        }
    }

    IDE_EXCEPTION_CONT( SKIP );
}

void sdlStatementManager::finalizeStatement( sdiStatement * aSdStmt )
{
    iduListNode   * sIterNode     = NULL;
    iduListNode   * sIterNodeNext = NULL;
    iduListNode   * sIterStmt     = NULL;
    iduListNode   * sIterStmtNext = NULL;
    sdlRemoteNode * sNode         = NULL;
    sdlRemoteStmt * sStmt         = NULL;

    IDU_LIST_ITERATE_SAFE( &aSdStmt->mNodeListHead, sIterNode, sIterNodeNext )
    {
        sNode = (sdlRemoteNode *)sIterNode->mObj;

        IDU_LIST_ITERATE_SAFE( &sNode->mStmtListHead, sIterStmt, sIterStmtNext )
        {
            sStmt = (sdlRemoteStmt *)sIterStmt->mObj;

            /* mStat must freed befor freeStatement */
            IDE_DASSERT( sStmt->mStmt == NULL );
            sStmt->mStmt = NULL;

            (void)iduMemMgr::free( sStmt );
            sStmt = NULL;
        }
        IDU_LIST_INIT( &sNode->mStmtListHead );

        (void)iduMemMgr::free( sNode );
        sNode = NULL;
    }
    IDU_LIST_INIT( &aSdStmt->mNodeListHead );
}

static inline void findRemoteStatementInNode(
                      const sdlRemoteNode     * aRemoteNode,
                      const UInt                aRemoteStmtId,
                      sdlRemoteStmt          ** aRemoteStmt )
{
    iduListNode    * sIterStmt     = NULL;
    sdlRemoteStmt  * sRemoteStmt   = NULL;

    IDU_LIST_ITERATE( &aRemoteNode->mStmtListHead, sIterStmt )
    {
        sRemoteStmt = (sdlRemoteStmt *)sIterStmt->mObj;

        if ( sRemoteStmt->mRemoteStmtId == aRemoteStmtId )
        {
            *aRemoteStmt = sRemoteStmt;
            break;
        }
    }
}

void sdlStatementManager::findRemoteStatement(
                                       sdiStatement   * aSdStmt,
                                       UInt             aNodeId,
                                       UInt             aRemoteStmtId,
                                       sdlRemoteStmt ** aRemoteStmt )
{
    iduListNode    * sIterNode     = NULL;
    sdlRemoteNode  * sRemoteNode   = NULL;

    *aRemoteStmt = NULL;

    IDU_LIST_ITERATE( &aSdStmt->mNodeListHead, sIterNode )
    {
        sRemoteNode = (sdlRemoteNode *)sIterNode->mObj;

        if ( aNodeId == sRemoteNode->mNodeId )
        {
            findRemoteStatementInNode( sRemoteNode,
                                       aRemoteStmtId,
                                       aRemoteStmt );
            break;
        }
    }
}

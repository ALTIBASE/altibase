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
 * $Id$ sdlStatementManager.h
 **********************************************************************/

#ifndef _O_SDL_STATEMENT_MANAGER_H_
#define _O_SDL_STATEMENT_MANAGER_H_ 1

#include <sdiStatement.h>
#include <sdlStatement.h>
#include <qci.h>

class sdlStatementManager
{
public:
    static void   initRemoteNode( sdlRemoteNode * aRemoteNode );

    static void   initRemoteStmt( sdlRemoteStmt * aRemoteStmt );

    static void   initializeStatement( sdiStatement *aSdStmt );

    static IDE_RC allocRemoteStatement( sdiStatement   * aSdStmt,
                                        UInt             aNodeId,
                                        sdlRemoteStmt ** aRemoteStmt );

    static void   setUnused( sdlRemoteStmt ** aRemoteStmt );

    static void   freeAllRemoteStatement( sdiStatement  * aSdStmt,
                                          sdiClientInfo * aClientInfo,
                                          UChar           aMode );

    static void   freeRemoteStatement( sdiStatement  * aSdStmt,
                                       sdiClientInfo * aClientInfo,
                                       UInt            aNodeId,
                                       UChar           aMode );

    static void   finalizeStatement( sdiStatement * aSdStmt );

    /* PROJ-2728 Sharding LOB */
    static void   findRemoteStatement( sdiStatement   * aSdStmt,
                                       UInt             aNodeId,
                                       UInt             aRemoteStmtId,
                                       sdlRemoteStmt ** aRemoteStmt );

private:
    static IDE_RC getNodeContainer( sdiStatement   * aSdStmt,
                                    UInt             aNodeId,
                                    sdlRemoteNode ** aNode );

    static IDE_RC getStmtContainer( sdlRemoteNode  * aNode,
                                    sdlRemoteStmt ** aStmt );
};

inline void sdlStatementManager::initRemoteNode( sdlRemoteNode * aRemoteNode )
{
    aRemoteNode->mNodeId = 0;
    aRemoteNode->mCurRemoteStmtId = 0;

    /* init list */
    IDU_LIST_INIT( &aRemoteNode->mStmtListHead );
    IDU_LIST_INIT_OBJ( &aRemoteNode->mListNode, aRemoteNode );
}

inline void sdlStatementManager::initRemoteStmt( sdlRemoteStmt * aRemoteStmt )
{
    aRemoteStmt->mStmt     = NULL;
    aRemoteStmt->mFreeFlag = SDL_STMT_FREE_FLAG_IN_USE_FALSE;

    /* init list */
    IDU_LIST_INIT_OBJ( &aRemoteStmt->mListNode, aRemoteStmt );
}

#endif  // _O_SDL_STATEMENT_MANAGER_H_

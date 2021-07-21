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
 * $Id$ sdiStatementManager.cpp
 **********************************************************************/

#include <sdiStatementManager.h>
#include <sdlStatementManager.h>

void sdiStatementManager::initializeStatement( sdiStatement * aSdStmt )
{
    sdlStatementManager::initializeStatement( aSdStmt );
}

void sdiStatementManager::freeRemoteStatement( sdiStatement  * aSdStmt,
                                               sdiClientInfo * aClientInfo,
                                               UInt            aNodeId,
                                               UChar           aMode )
{
    sdlStatementManager::freeRemoteStatement( aSdStmt,
                                              aClientInfo,
                                              aNodeId,
                                              aMode );
}

void sdiStatementManager::freeAllRemoteStatement( sdiStatement  * aSdStmt,
                                                  sdiClientInfo * aClientInfo,
                                                  UChar           aMode )
{
    sdlStatementManager::freeAllRemoteStatement( aSdStmt,
                                                 aClientInfo,
                                                 aMode );
}

void sdiStatementManager::finalizeStatement( sdiStatement * aSdStmt )
{
    sdlStatementManager::finalizeStatement( aSdStmt );
}

void sdiStatementManager::findRemoteStatement(
                                       sdiStatement   * aSdStmt,
                                       UInt             aNodeId,
                                       UInt             aRemoteStmtId,
                                       sdlRemoteStmt ** aRemoteStmt )
{
    sdlStatementManager::findRemoteStatement( aSdStmt,
                                              aNodeId,
                                              aRemoteStmtId,
                                              aRemoteStmt );
}

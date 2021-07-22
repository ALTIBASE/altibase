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
 * $Id: qsoPkgStmts.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qso.h>
#include <qsoPkgStmts.h>
#include <qmo.h>
#include <qcuSqlSourceInfo.h>
#include <qcg.h>
#include <qsv.h>

IDE_RC qsoPkgStmts::optimizeBodyBlock(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsPkgStmtBlock      * sBLOCK = (qsPkgStmtBlock *)aProcStmts;
    qsPkgStmts          * sPkgStmt;
    qsProcStmts         * sProcStmt;
    qsExceptionHandlers * sExceptionHandler;
    qsProcStmtException * sExceptionBlock;         // BUG-37501

    // subprogram
    for( sPkgStmt = sBLOCK->subprograms ;
         sPkgStmt != NULL ;
         sPkgStmt = sPkgStmt->next )
    {
        if( sPkgStmt->stmtType != QS_OBJECT_MAX )
        {
            IDE_TEST( sPkgStmt->optimize( aStatement, sPkgStmt ) != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    
    // body
    for (sProcStmt = sBLOCK->bodyStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    // exception handler action statement
    if( sBLOCK->exception != NULL )
    {
        sExceptionBlock = (qsProcStmtException *)(sBLOCK->exception);

        for ( sExceptionHandler = sExceptionBlock->exceptionHandlers;
              sExceptionHandler != NULL;
              sExceptionHandler = sExceptionHandler->next )
        {
            sProcStmt = sExceptionHandler->actionStmt;
            IDE_TEST( sProcStmt->optimize( aStatement, sProcStmt ) != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsoPkgStmts::optimizeCreateProc( qcStatement * aStatement,
                                        qsPkgStmts  * aPkgStmts )
{
    qsPkgSubprograms * sPkgSubprogram;
    qsProcParseTree  * sParseTree;
    qsProcStmts      * sProcStmt;
    UInt               sUserID;

    sPkgSubprogram = ( qsPkgSubprograms * )aPkgStmts;
    sParseTree     = sPkgSubprogram->parseTree;
    sUserID        = QCG_GET_SESSION_USER_ID( aStatement );

    // PR-24408
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );
    
    sProcStmt = (qsProcStmts *)(sParseTree->block);
 
    if( sProcStmt != NULL )
    {  
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_FAILURE;
}

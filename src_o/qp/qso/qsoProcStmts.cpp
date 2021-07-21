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
 * $Id: qsoProcStmts.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qtc.h>
#include <qso.h>
#include <qsoProcStmts.h>
#include <qmo.h>
#include <qcuSqlSourceInfo.h>
#include <qcg.h>
#include <qsv.h>

IDE_RC qsoProcStmts::optimizeNone(
    qcStatement * /* aStatement */,
    qsProcStmts * /* aProcStmts */)
{
    /* Except in the case of below. 
     *
     * BLOCK
     * IF
     * THEN
     * ELSE
     * WHILE
     * FOR
     * CURSOR FOR
     * GOTO
     */

    /* nothing to do */

    return IDE_SUCCESS;

}

IDE_RC qsoProcStmts::optimizeBlock(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtBlock     * sBLOCK = (qsProcStmtBlock *)aProcStmts;
    qsProcStmts         * sProcStmt;
    qsExceptionHandlers * sExceptionHandler;
    qsProcStmtException * sExceptionBlock;   // BUG-37501

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

        for (sExceptionHandler = sExceptionBlock->exceptionHandlers;
             sExceptionHandler != NULL;
             sExceptionHandler = sExceptionHandler->next)
        {
            sProcStmt = sExceptionHandler->actionStmt;
            IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
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

IDE_RC qsoProcStmts::optimizeIf(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtIf    * sIF = (qsProcStmtIf *)aProcStmts;
    qsProcStmts     * sProcStmt;
    
    // THEN clause
    sProcStmt = sIF->thenStmt;
    if( sProcStmt != NULL )
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    // ELSE clause
    sProcStmt = sIF->elseStmt;
    if( sProcStmt != NULL )
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qsoProcStmts::optimizeThen(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtThen  * sTHEN = (qsProcStmtThen *)aProcStmts;
    qsProcStmts     * sProcStmt;
    
    for (sProcStmt = sTHEN->thenStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qsoProcStmts::optimizeElse(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtElse  * sELSE = (qsProcStmtElse *)aProcStmts;
    qsProcStmts     * sProcStmt;

    for (sProcStmt = sELSE->elseStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

IDE_RC qsoProcStmts::optimizeWhile(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtWhile     * sWHILE = (qsProcStmtWhile *)aProcStmts;
    qsProcStmts         * sProcStmt;

    // loop statements
    for (sProcStmt = sWHILE->loopStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qsoProcStmts::optimizeFor(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtFor   * sFOR = (qsProcStmtFor *)aProcStmts;
    qsProcStmts     * sProcStmt;

    // loop statements
    for (sProcStmt = sFOR->loopStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qsoProcStmts::optimizeCursorFor(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtCursorFor     * sCursorFor = (qsProcStmtCursorFor *)aProcStmts;
    qsProcStmts             * sProcStmt;

    // loop statements
    for (sProcStmt = sCursorFor->loopStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->optimize(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qsoProcStmts::optimizeGoto(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts )
{
/***********************************************************************
 *
 * Description : PROJ-1335, To fix BUG-12475
 *               GOTO의 optimization( 실제로는 validation )
 * Implementation :
 *     (1) 자신의 parent statement를 따라가면서
 *         동일한 label이 있는지 검색
 *     (2) 동일한 label이 존재한다면 labelID를 gotoStmt에 세팅
 *
 ***********************************************************************/

    qsProcStmtGoto   * sGOTO = (qsProcStmtGoto *)aProcStmts;
    qsProcStmts      * sParentStmt;
    qcuSqlSourceInfo   sqlInfo;
    qsLabels         * sLabel;
    idBool             sFind;

    sFind = ID_FALSE;

    for( sParentStmt = aProcStmts->parent;
         sParentStmt != NULL && sFind == ID_FALSE;
         sParentStmt = sParentStmt->parent  )
    {
        for( sLabel = sParentStmt->childLabels;
             sLabel != NULL && sFind == ID_FALSE;
             sLabel = sLabel->next )
        {
            if ( QC_IS_NAME_CASELESS_MATCHED( sLabel->namePos, sGOTO->labelNamePos ) )
            {
                sGOTO->labelID = sLabel->id;
                sFind = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    if( sFind == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sGOTO->labelNamePos );
        IDE_RAISE(GOTO_ERROR);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(GOTO_ERROR)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_ILLEGAL_GOTO,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

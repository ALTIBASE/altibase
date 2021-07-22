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
 * $Id: qsvCursor.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QSV_CURSOR_H_
#define _Q_QSV_CURSOR_H_ 1

#include <qc.h>
#include <qsParseTree.h>

#define QSV_SQL_CURSOR_NAME ("SQL")

class qsvCursor
{
public:
    static IDE_RC validateCursorDeclare(
        qcStatement * aStatement,
        qsCursors   * aCursor);

    static IDE_RC parseCursorFor(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC validateCursorFor( qcStatement     * aStatement,
                                     qsProcStmts     * aProcStmts,
                                     qsProcStmts     * aParentStmt,
                                     qsValidatePurpose aPurpose );

    static IDE_RC validateOpen( qcStatement     * aStatement,
                                qsProcStmts     * aProcStmts,
                                qsProcStmts     * aParentStmt,
                                qsValidatePurpose aPurpose );

    static IDE_RC validateFetch( qcStatement     * aStatement,
                                 qsProcStmts     * aProcStmts,
                                 qsProcStmts     * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC validateClose( qcStatement     * aStatement,
                                 qsProcStmts     * aProcStmts,
                                 qsProcStmts     * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC getCursorDefinition(
        qcStatement     * aStatement,
        qtcNode         * aOpenCursorSpec,
        qsCursors      ** aCursorDef );

    static IDE_RC validateIntoClause(
        qcStatement     * aStatement,
        qmsTarget       * aTargets,
        qmsInto         * aIntoVars );

    static IDE_RC parseOpen(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts );

private:
    // PROJ-1073 Package
    static IDE_RC getPkgCursorDefinition(
        qcStatement  * aStatement,
        qtcNode      * aCursorNode,
        qsCursors   ** aCursorDef,
        idBool       * aFind );

    // BUG-38164
    static IDE_RC getPkgLocalCursorDefinition(
        qcStatement     * aStatement,
        qtcNode         * aCursorNode,
        qsPkgStmtBlock  * aBlock,
        qsCursors      ** aCursorDef,
        idBool          * aFind );
};

#endif  // _Q_QSV_CURSOR_H_


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
 * $Id: qsoProcStmts.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _Q_QSO_PROC_STMTS_H_
#define _Q_QSO_PROC_STMTS_H_ 1

#include <qc.h>
#include <qsParseTree.h>

class qsoProcStmts
{
public:
    static IDE_RC optimizeNone(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeBlock(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeIf(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeThen(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeElse(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);
    
    static IDE_RC optimizeWhile(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeFor(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    static IDE_RC optimizeCursorFor(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);

    // PROJ-1335, To fix BUG-12475
    // goto는 optimize 단계에서 validation함.
    static IDE_RC optimizeGoto(
        qcStatement * aStatement,
        qsProcStmts * aProcStmts);
};

#endif  // _Q_QSO_PROC_STMTS_H_


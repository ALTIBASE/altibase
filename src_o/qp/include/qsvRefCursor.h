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
 * $Id: qsvRefCursor.h 15573 2006-04-15 02:50:33Z mhjeong $
 **********************************************************************/

#ifndef _Q_QSV_REF_CURSOR_H_
#define _Q_QSV_REF_CURSOR_H_ 1

#include <qc.h>
#include <qsParseTree.h>

class qsvRefCursor
{
public:

    static IDE_RC validateOpenFor( qcStatement       * aStatement,
                                   qsProcStmtOpenFor * aProcStmtOpenFor );

    static IDE_RC validateFetch( qcStatement     * aStatement,
                                 qsProcStmts     * aProcStmts,
                                 qsProcStmts     * aParentStmt,
                                 qsValidatePurpose aPurpose );

    static IDE_RC validateUsingParamClause(
        qcStatement  * aStatement,
        qsUsingParam * aUsingParams,
        UInt         * aUsingParamCount );

    static IDE_RC searchRefCursorVar(
        qcStatement  * aStatement,
        qtcNode      * aVariableNode,
        qsVariables ** aVariables, // BUG-38767
        idBool       * aFindVar );
};

#endif  // _Q_QSV_REF_CURSOR_H_


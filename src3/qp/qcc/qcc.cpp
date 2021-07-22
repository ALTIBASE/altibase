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
 

/********************************\***************************************
 * $Id: qcc.cpp 88963 2020-10-19 03:33:18Z jake.jang $
 **********************************************************************/

#include <idl.h>
#include <qcc.h>

IDE_RC qcc::parse(qcStatement * /* aStatement */)
{
#define IDE_FN "qcc::parse"

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qcc::parseError(qcStatement * /* aStatement */)
{
#define IDE_FN "qcc::parseError"

    IDE_RAISE(ERR_NOT_SUPPORTED_SYNTAX);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_SUPPORTED_SYNTAX);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCP_NOT_SUPPORTED_SYNTAX));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qcc::validate(qcStatement * aStatement)
{
#define IDE_FN "qcc::validate"

    if ( ( aStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_MASK )
         == QCI_STMT_MASK_DML )
    {
        // CHECK SEQUENCE ÀÏ °æ¿ì
        QC_SHARED_TMPLATE(aStatement)->smiStatementFlag |= SMI_STATEMENT_MEMORY_CURSOR;
    }
    else
    {
        // Nothing To Do
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qcc::optimize(qcStatement * /* aStatement */)
{
#define IDE_FN "qcc::optimize"

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qcc::execute(qcStatement * /* aStatement */)
{
#define IDE_FN "qcc::execute"

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qcc::executeError(qcStatement * /* aStatement */)
{
    IDE_RAISE(ERR_ABORT_INVALID_STMT_TYPE);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_ABORT_INVALID_STMT_TYPE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qcc::executeError",
                                  "This statement cannot be executed through the execution plan." ));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
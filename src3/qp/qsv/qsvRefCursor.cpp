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
 * $Id: qsvRefCursor.cpp 22178 2007-06-15 01:26:33Z orc $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcuSqlSourceInfo.h>
#include <qsParseTree.h>
#include <qsvRefCursor.h>
#include <qsv.h>
#include <qsvProcVar.h>
#include <qsvProcStmts.h>
#include <qcuSessionPkg.h>
#include <qmv.h>

extern mtdModule    mtdVarchar;
extern mtdModule    mtdChar;

IDE_RC
qsvRefCursor::validateOpenFor( qcStatement       * aStatement,
                               qsProcStmtOpenFor * aProcStmtOpenFor )
{
/***********************************************************************
 *
 *  Description : PROJ-1386 Dynamic SQL
 *               
 *  Implementation :
 *            (1) query string이 char나 varchar인지 검사
 *            (2) USING절에 대한 validation
 *
 ***********************************************************************/

    mtcColumn         * sMtcColumn;
    qcuSqlSourceInfo    sqlInfo;    

    // fix BUG-37495
    if( aProcStmtOpenFor->sqlStringNode->node.module == &qtc::subqueryModule )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &aProcStmtOpenFor->sqlStringNode->position );
        IDE_RAISE(ERR_INSUFFICIENT_ARGUEMNT);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( qtc::estimate( aProcStmtOpenFor->sqlStringNode,
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
              != IDE_SUCCESS );

    sMtcColumn = &(QC_SHARED_TMPLATE(aStatement)->tmplate.
                   rows[aProcStmtOpenFor->sqlStringNode->node.table].
                   columns[aProcStmtOpenFor->sqlStringNode->node.column]);

    if( ( sMtcColumn->module != &mtdVarchar ) &&
        ( sMtcColumn->module != &mtdChar ) )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &aProcStmtOpenFor->sqlStringNode->position );
        IDE_RAISE(ERR_INSUFFICIENT_ARGUEMNT);
    }
    else
    {
        // Nothing to do.
    }

    IDE_TEST( validateUsingParamClause(
                  aStatement,
                  aProcStmtOpenFor->usingParams,
                  &aProcStmtOpenFor->usingParamCount )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INSUFFICIENT_ARGUEMNT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INSUFFICIENT_ARGUEMNT_SQLTEXT,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qsvRefCursor::validateUsingParamClause(
    qcStatement  * aStatement,
    qsUsingParam * aUsingParams,
    UInt         * aUsingParamCount )
{
/***********************************************************************
 *
 *  Description : PROJ-1385 Dynamic-SQL
 *                USING ... 절에 대한 validation
 *  Implementation : parameter의 inouttype별로 구분
 *                  OUT, IN OUT 인 경우
 *                  (1)반드시 procedure variable이어야 함
 *                  (2) read만 가능한 variable의 경우 허용안함
 *
 ***********************************************************************/
    
    idBool              sFindVar;
    qsVariables       * sArrayVariable;
    qcuSqlSourceInfo    sqlInfo;
    qsUsingParam      * sCurrParam;
    qtcNode           * sCurrParamNode;

    *aUsingParamCount = 0;
    
    if( aUsingParams != NULL )
    {
        for( sCurrParam = aUsingParams;
             sCurrParam != NULL;
             sCurrParam = sCurrParam->next )
        {
            sCurrParamNode = sCurrParam->paramNode;

            if( sCurrParam->inOutType == QS_IN )
            {
                // in타입인 경우 특별한 검사를 하지 않음.
                IDE_TEST( qtc::estimate( sCurrParamNode,
                                         QC_SHARED_TMPLATE(aStatement), 
                                         aStatement,
                                         NULL,
                                         NULL,
                                         NULL )
                      != IDE_SUCCESS );
            }
            else
            {
                // (1) out, in out인 경우 반드시 procedure변수여야 함.
                // invalid out argument 에러를 냄.
                IDE_TEST(qsvProcVar::searchVarAndPara(
                         aStatement,
                         sCurrParamNode,
                         ID_TRUE, // for OUTPUT
                         &sFindVar,
                         &sArrayVariable)
                     != IDE_SUCCESS);

                if( sFindVar == ID_FALSE )
                {
                    IDE_TEST( qsvProcVar::searchVariableFromPkg(
                            aStatement,
                            sCurrParamNode,
                            &sFindVar,
                            &sArrayVariable )
                        != IDE_SUCCESS );
                }

                if (sFindVar == ID_FALSE)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sCurrParamNode->position );
                    IDE_RAISE(ERR_INVALID_OUT_ARGUMENT);
                }
                else
                {
                    // Nothing to do.
                }

                // lvalue에 psm변수가 존재하므로 lvalue flag를 씌움.
                // out인 경우에만 해당됨. array_index_variable인 경우
                // 없으면 만들어야 하기 때문.
                if( sCurrParam->inOutType == QS_OUT )
                {
                    sCurrParamNode->lflag |= QTC_NODE_LVALUE_ENABLE;
                }
                else
                {
                    // Nothing to do.
                }

                // (2) out, in out인 경우 outbinding_disable이면 에러
                IDE_TEST( qtc::estimate( sCurrParamNode,
                                         QC_SHARED_TMPLATE(aStatement), 
                                         aStatement,
                                         NULL,
                                         NULL,
                                         NULL )
                          != IDE_SUCCESS );
            
                if ( ( sCurrParamNode->lflag & QTC_NODE_OUTBINDING_MASK )
                     == QTC_NODE_OUTBINDING_DISABLE )
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sCurrParamNode->position );
                    IDE_RAISE(ERR_INVALID_OUT_ARGUMENT);
                }
                else
                {
                    // Nothing to do.
                }
            }

            (*aUsingParamCount)++;
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_OUT_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INVALID_OUT_ARGUEMNT_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC qsvRefCursor::searchRefCursorVar(
    qcStatement  * aStatement,
    qtcNode      * aVariableNode,
    qsVariables ** aVariables,
    idBool       * aFindVar )
{
/***********************************************************************
 *
 *  Description : PROJ-1386 Dynamic-SQL
 *
 *  Implementation : (1) variable search
 *                   (2) variable 찾았으면 ref cursor type인지 검사
 *                   (3) variable을 못찾았거나, ref cursor type이 아니면
 *                       not found
 *
 ***********************************************************************/
    
    mtcColumn         * sMtcColumn;
    // PROJ-1073 Package
    qcTemplate        * sTemplate;
    qsxPkgInfo        * sPkgInfo;
    
    IDE_TEST( qsvProcVar::searchVarAndPara(
                  aStatement,
                  aVariableNode,
                  ID_FALSE,
                  aFindVar,
                  aVariables )
              != IDE_SUCCESS );

    if( *aFindVar == ID_FALSE )
    {
        IDE_TEST( qsvProcVar::searchVariableFromPkg(
                      aStatement,
                      aVariableNode,
                      aFindVar,
                      aVariables )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nohting to do.
    }

    if( *aFindVar == ID_TRUE )
    {
        if( aVariableNode->node.objectID != 0 )
        {
            IDE_TEST( qsxPkg::getPkgInfo( aVariableNode->node.objectID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            // objectID의 template을 가져온다.
            sTemplate = sPkgInfo->tmplate;
        }
        else
        {
            sTemplate = QC_SHARED_TMPLATE(aStatement);
        }

        sMtcColumn = &(sTemplate->tmplate.
                       rows[aVariableNode->node.table].
                       columns[aVariableNode->node.column]);

        // 반드시 ref cursor type module이어야 함.
        if( sMtcColumn->module->id != MTD_REF_CURSOR_ID )
        {
            *aFindVar = ID_FALSE;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC
qsvRefCursor::validateFetch( qcStatement     * aStatement,
                             qsProcStmts     * aProcStmts,
                             qsProcStmts     * /*aParentStmt*/,
                             qsValidatePurpose /*aPurpose*/ )
{
/***********************************************************************
 *
 *  Description : PROJ-1386 Dynamic-SQL
 *                BUG-41707 chunking bulk collections of reference cursor    
 *
 *  Implementation :
 *            (1) into절에 대한 validation
 *            (2) limit절에 대한 validation
 *
 ***********************************************************************/

    qsProcStmtFetch * sProcStmtFetch = (qsProcStmtFetch*)aProcStmts;

    /* into절에 대한 validation */
    IDE_TEST( qsvProcStmts::validateIntoClauseForRefCursor( 
                   aStatement,
                   aProcStmts,
                   sProcStmtFetch->intoVariableNodes )
              != IDE_SUCCESS );

    /* limit절에 대한 validation */
    // limit절은 bulk collect into일 때만 사용 가능하다.
    if ( ( sProcStmtFetch->intoVariableNodes->bulkCollect == ID_TRUE ) && 
         ( sProcStmtFetch->limit != NULL ) )
    {
        IDE_TEST( qmv::validateLimit( aStatement,
                                      sProcStmtFetch->limit )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

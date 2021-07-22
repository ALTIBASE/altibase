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
 * $Id: qsvCursor.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcmUser.h>
#include <qcuSqlSourceInfo.h>
#include <qsvCursor.h>
#include <qsv.h>
#include <qsvProcVar.h>
#include <qsvProcStmts.h>
#include <qsvPkgStmts.h>
#include <qmv.h>
#include <qmvQuerySet.h>
#include <qsxRelatedProc.h>
#include <qdpPrivilege.h>
#include <qsxPkg.h>
#include <qdpRole.h>

/* PROJ-1584 DML Return Clause */
extern mtdModule mtdInteger;

IDE_RC qsvCursor::validateCursorDeclare(
    qcStatement * aStatement,
    qsCursors   * aCursor)
{
    qcStatement         * sStatement;
    qsProcParseTree     * sParseTree;
    qsVariableItems     * sProcPara = NULL;
    qsVariableItems     * sLastCursorPara = NULL;
    qcuSqlSourceInfo      sqlInfo;
    UInt                  sStage = 0;

    QSV_ENV_SET_SQL( aStatement, aCursor );

    aStatement->spvEnv->currStmt = (qsProcStmts *)aCursor->mCursorSql;

    sParseTree = aStatement->spvEnv->createProc;

    // BUG-27442
    // Validate length of Cursor name
    if ( aCursor->common.name.size > QC_MAX_OBJECT_NAME_LEN )
    {
        sStage = 1;
        sqlInfo.setSourceInfo( aStatement,
                               & aCursor->common.name );
        IDE_RAISE( ERR_MAX_NAME_LEN_OVERFLOW );
    }
    else
    {
        // Nothing to do.
    }

    if ( idlOS::strMatch( aCursor->common.name.stmtText + aCursor->common.name.offset,
                          aCursor->common.name.size,
                          QSV_SQL_CURSOR_NAME,
                          idlOS::strlen( QSV_SQL_CURSOR_NAME ) ) == 0 )
    {
        sStage = 1;
        sqlInfo.setSourceInfo( aStatement,
                               & aCursor->common.name );
        IDE_RAISE(ERR_CURSOR_NAME_IS_SQL);
    }
    else
    {
        // Nothing to do.
    }

    // BUG-38629
    // qsvProcStmts::parseCursor에서 parameter에 대한 validate을 수행함.
    if (aCursor->paraDecls != NULL)
    {
        // connect parameter list
        sLastCursorPara = aCursor->paraDecls;
        while (sLastCursorPara->next != NULL)
        {
            sLastCursorPara = sLastCursorPara->next;
        }

        if( sParseTree != NULL )
        {
            sLastCursorPara->next = sParseTree->paraDecls;
        }

        // fix BUG-32837
        sProcPara                        = aStatement->spvEnv->allParaDecls;
        aStatement->spvEnv->allParaDecls = aCursor->paraDecls;
    }
    else
    {
        // Nothing to do.
    }

    // validate statement
    sStatement                  = aCursor->statement;
    sStatement->myPlan->planEnv = NULL;
    sStatement->spvEnv          = aStatement->spvEnv;
    sStatement->mStatistics     = aStatement->mStatistics;
    sStatement->mRandomPlanInfo = aStatement->mRandomPlanInfo;

    IDE_TEST(sStatement->myPlan->parseTree->validate( sStatement ) != IDE_SUCCESS);

    // BUG-36988
    IDE_TEST( qsvProcStmts::queryTrans( aStatement, (qsProcStmts*)aCursor->mCursorSql )
              != IDE_SUCCESS );

    // BUG-37655
    // pragma restrict reference
    if( aStatement->spvEnv->createPkg != NULL )
    {
        IDE_TEST( qsvPkgStmts::checkPragmaRestrictReferencesOption( 
                      aStatement,
                      aStatement->spvEnv->currSubprogram,
                      aStatement->spvEnv->currStmt,
                      aStatement->spvEnv->isPkgInitializeSection )
                  != IDE_SUCCESS );

        // BUG-44615
        // Package spec인 경우에는 table info를 cursor 선언시에 미리 만든다.
        if ( aStatement->spvEnv->createPkg->objType == QS_PKG )
        {
            IDE_TEST( qmvQuerySet::makeTableInfo(
                          aStatement,
                          ((qmsParseTree*)aCursor->mCursorSql->parseTree)->querySet,
                          NULL,
                          &aCursor->tableInfo,
                          aCursor->common.objectID )
                     != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    if (aCursor->paraDecls != NULL)
    {
        // disconnect parameter list
        sLastCursorPara->next = NULL;

        // fix BUG-32837
        aStatement->spvEnv->allParaDecls = sProcPara;
    }
    else
    {
        // Nothing to do.
    }

    // BUG-38767
    // Cursor를 선언할 때 sqlIdx를 설정한다.
    if ( aStatement->spvEnv->createProc != NULL )
    {
        aCursor->sqlIdx = aStatement->spvEnv->createProc->procSqlCount++;
    }
    else
    {
        aCursor->sqlIdx = ID_UINT_MAX;
    }

    IDE_TEST( QC_QME_MEM(aStatement)->free( sStatement )
              != IDE_SUCCESS );
    aCursor->statement = NULL;

    QSV_ENV_INIT_SQL( aStatement );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_CURSOR_NAME_IS_SQL);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_CURSOR_NAME_IS_SQL_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_MAX_NAME_LEN_OVERFLOW)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCP_MAX_NAME_LENGTH_OVERFLOW,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    // BUG-38835
    // An error position of cursor in PSM should be shown to the user.
    if ( sStage == 0 )
    {
        // set original error code.
        qsxEnv::setErrorCode( QC_QSX_ENV(aStatement) );

        // BUG-43998
        // PSM 생성 오류 발생시 오류 발생 위치를 한 번만 출력하도록 합니다.
        if ( ideHasErrorPosition() == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &aCursor->pos );

            (void)sqlInfo.initWithBeforeMessage(
                aStatement->qmeMem );

            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sqlInfo.getBeforeErrMessage(),
                                sqlInfo.getErrMessage()));
            (void)sqlInfo.fini();
        }

        // set sophisticated error message.
        qsxEnv::setErrorMessage( QC_QSX_ENV(aStatement) );
    }
    else
    {
        // Noting to do.
    }

    QSV_ENV_INIT_SQL( aStatement );

    return IDE_FAILURE;
}

IDE_RC qsvCursor::parseCursorFor(qcStatement * /*aStatement*/,
                                 qsProcStmts * /*aProcStmts*/)
{
    return IDE_SUCCESS;
}

IDE_RC
qsvCursor::validateCursorFor( qcStatement     * aStatement,
                              qsProcStmts     * aProcStmts,
                              qsProcStmts     * aParentStmt,
                              qsValidatePurpose aPurpose )
{
    qsProcStmtCursorFor  * sCursorFor = (qsProcStmtCursorFor *)aProcStmts;
    qsProcStmts          * sProcStmt;
    qsCursors            * sCursorDef = NULL;
    qsVariables          * sVariable;
    qsAllVariables       * sOldAllVars;
    qsAllVariables       * sOldAllVarsForCursorVar;
    qtcNode              * sRowVarNode[2];
    qsProcStmtSql        * sSQL;
    qsUsingParam         * sCurrParam;
    qtcNode              * sCurrParamNode;
    qcuSqlSourceInfo       sqlInfo;
    // BUG-37981
    qsxPkgInfo           * sPkgInfo = NULL;
    qcTemplate           * sCursorTemplate;
    SInt                   sState = 0;
    // BUG-41243
    qsVariableItems      * sParaDecl      = NULL;
    UInt                   sParaDeclCount = 0;
    
    QS_SET_PARENT_STMT( aProcStmts, aParentStmt );

    aStatement->spvEnv->currStmt = aProcStmts;

    // validation of counter variable
    //  (1) counter cannot be specified with label name.
    //      label_name.counter_name : illegal
    //      counter_name            : legal
    //  (2) counter variable is read-only variable.
    //  (3) It do not need that
    //      a counter variable is a declared local variable in declare section.
    //  (4) A counter variable in cursor for loop must have row type.

    // make qsVariableItems for counter variable
    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qsVariables, &sVariable)
             != IDE_SUCCESS);

    sVariable->common.itemType      = QS_VARIABLE;
    SET_POSITION( sVariable->common.name,
                  sCursorFor->counterRowTypeNode->columnName );
    sVariable->common.table         = ID_USHORT_MAX;
    sVariable->common.column        = ID_USHORT_MAX;
    sVariable->common.objectID      = QS_EMPTY_OID;
    sVariable->common.next          = NULL;

    IDE_TEST( qtc::makeProcVariable( aStatement,
                                     sRowVarNode,
                                     & sCursorFor->openCursorSpecNode->columnName,
                                     NULL,
                                     QTC_PROC_VAR_OP_NONE )
              != IDE_SUCCESS );

    sVariable->variableTypeNode   = sRowVarNode[0];
    sVariable->defaultValueNode   = NULL;
    sVariable->variableType       = QS_ROW_TYPE;
    sVariable->inOutType          = QS_IN;    // read-only variable
    sVariable->typeInfo           = NULL;
    sVariable->nocopyType         = QS_COPY;

    // check cursor name
    IDE_TEST(getCursorDefinition(
                 aStatement,
                 sCursorFor->openCursorSpecNode,
                 &sCursorDef)
             != IDE_SUCCESS);

    if ( sCursorDef == NULL )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sCursorFor->openCursorSpecNode->position );
        IDE_RAISE(ERR_NOT_EXIST_CURSOR);
    }
    else
    {
        sSQL = sCursorDef->mCursorSql;

        // BUG-38767
        if(( sCursorDef->common.objectID == QS_EMPTY_OID ) &&
           ( sCursorDef->sqlIdx != ID_UINT_MAX ) )
        {
            sCursorFor->sqlIdx = sCursorDef->sqlIdx;
        }
        else
        {
            // package spec에 선언한 cursor이면 sqlIdx를 새로 할당한다.
            if ( aStatement->spvEnv->createProc != NULL )
            {
                sCursorFor->sqlIdx = aStatement->spvEnv->createProc->procSqlCount++;
            }
            else
            {
                sCursorFor->sqlIdx = ID_UINT_MAX;
            }
        }
    }

    // check cursor arguments
    for ( sParaDecl = sCursorDef->paraDecls;
          sParaDecl != NULL ;
          sParaDecl = sParaDecl->next )
    {
        sParaDeclCount++;
    }

    IDE_TEST( qsv::validateArgumentsWithoutParser( aStatement,
                                                   sCursorFor->openCursorSpecNode,
                                                   sCursorDef->paraDecls,
                                                   sParaDeclCount,
                                                   ID_FALSE /* No subquery is allowed */ )
              != IDE_SUCCESS );

    // To fix BUG-14279
    // tableInfo가 생성되어 있지 않으면 생성한 후
    // rowtype 생성.
    if( sCursorDef->tableInfo == NULL )
    {
        IDE_TEST( qmvQuerySet::makeTableInfo(
                      aStatement,
                      ((qmsParseTree*)sCursorDef->mCursorSql->parseTree)->querySet,
                      NULL,
                      &sCursorDef->tableInfo,
                      sCursorDef->common.objectID )
                 != IDE_SUCCESS);
    }
    else
    {
        // Nothing to do.
    }

    // PROJ-1075 cursorFor를 위한 rowtype변수를 생성하고,
    // counter로 사용할 변수의 table, column을 세팅한다.
    IDE_TEST( qsvProcVar::makeRowTypeVariable( aStatement,
                                               sCursorDef->tableInfo,
                                               sVariable )
              != IDE_SUCCESS );

    IDE_TEST(qsvProcStmts::connectAllVariables(
                 aStatement,
                 sCursorFor->common.parentLabels,
                 (qsVariableItems *)sVariable,
                 ID_TRUE,
                 &sOldAllVars )
             != IDE_SUCCESS);

    /* PROJ-2197 PSM Renewal
     * Cursor Definition에 있는 Cursor의 변수를 사용하기 위해서
     * Variablelist에 추가한다. */
    IDE_TEST( qsvProcStmts::connectAllVariables(
                  aStatement,
                  sCursorFor->common.parentLabels,
                  (qsVariableItems *)sCursorDef->paraDecls,
                  ID_TRUE,
                  &sOldAllVarsForCursorVar)
              != IDE_SUCCESS );

    // BUG-41262 PSM overloading
    // PSM overloading 은 parser 단계에서 처리한다.
    // 하지만 PSM 내부의 변수는 connectAllVariables 를 호출후에 접근이 가능하다. 
    for (sProcStmt = sCursorFor->loopStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST(sProcStmt->parse(aStatement, sProcStmt) != IDE_SUCCESS);
    }

    if( sSQL->usingParams != NULL )
    {
        // BUG-37981
        if( sCursorDef->common.objectID != QS_EMPTY_OID )
        {
            IDE_TEST( qsxPkg::getPkgInfo( sCursorDef->common.objectID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            sCursorTemplate = sPkgInfo->tmplate;

            QC_CONNECT_TEMPLATE_STACK(
                sCursorTemplate,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stackBuffer,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stackRemain );
            sCursorTemplate->stmt = QC_SHARED_TMPLATE(aStatement)->stmt;
            sState = 1;
        }
        else
        {
            sCursorTemplate = QC_SHARED_TMPLATE(aStatement);
        }

        for( sCurrParam = sSQL->usingParams;
             sCurrParam != NULL;
             sCurrParam = sCurrParam->next )
        {
            sCurrParamNode = sCurrParam->paramNode;

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

            IDE_TEST( qtc::estimate( sCurrParamNode,
                                     sCursorTemplate,  
                                     aStatement,
                                     NULL,
                                     NULL,
                                     NULL )
                      != IDE_SUCCESS );

            if ( ( sCurrParam->inOutType != QS_IN ) &&
                 ( ( sCurrParamNode->lflag & QTC_NODE_OUTBINDING_MASK )
                 == QTC_NODE_OUTBINDING_DISABLE ) )
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

        // BUG-37981
        if( sCursorDef->common.objectID != QS_EMPTY_OID )
        {
            QC_DISCONNECT_TEMPLATE_STACK( sCursorTemplate );
            sCursorTemplate->stmt = NULL; 
            sState = 0;
        }
        else
        {
            // Nothing to do.
        }
    }

    sCursorFor->rowVariable                       = sVariable;
    sCursorFor->counterRowTypeNode->node.table    = sVariable->common.table;
    sCursorFor->counterRowTypeNode->node.column   = sVariable->common.column;
    sCursorFor->counterRowTypeNode->node.objectID = sVariable->common.objectID;

    // PROJ-1075
    // fetch into variable도 calculate를 수행하기 때문에
    // estimate를 해서 mtcExecute를 세팅해야 한다.
    IDE_TEST( qtc::estimate( sCursorFor->counterRowTypeNode,
                             QC_SHARED_TMPLATE( aStatement ),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
              != IDE_SUCCESS);

    // check loop body
    (aStatement->spvEnv->loopCount)++;

    for (sProcStmt = sCursorFor->loopStmts;
         sProcStmt != NULL;
         sProcStmt = sProcStmt->next)
    {
        IDE_TEST( sProcStmt->validate(aStatement, sProcStmt, aProcStmts, aPurpose )
                  != IDE_SUCCESS);
    }

    (aStatement->spvEnv->loopCount)--;

    /* PROJ-2197 PSM Renewal */
    qsvProcStmts::disconnectAllVariables( aStatement, sOldAllVarsForCursorVar);
    qsvProcStmts::disconnectAllVariables( aStatement, sOldAllVars );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_CURSOR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_OUT_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INVALID_OUT_ARGUEMNT_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    // BUG-37981
    if( sState == 1 )
    {
        QC_DISCONNECT_TEMPLATE_STACK( sCursorTemplate );
        sCursorTemplate->stmt = NULL;
        sState = 0; 
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsvCursor::parseOpen(
    qcStatement * aStatement,
    qsProcStmts * aProcStmts)
{
    qsProcStmtOpen      * sOPEN = (qsProcStmtOpen *)aProcStmts;
    qtcNode             * sNode;

    // arguments of OPEN statement
    for ( sNode = (qtcNode *)(sOPEN->openCursorSpecNode->node.arguments);
          sNode != NULL;
          sNode = (qtcNode *)(sNode->node.next))
    {
        IDE_TEST(qmv::parseViewInExpression( aStatement, sNode )
                 != IDE_SUCCESS);
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}


IDE_RC
qsvCursor::validateOpen( qcStatement     * aStatement,
                         qsProcStmts     * aProcStmts,
                         qsProcStmts     * aParentStmt,
                         qsValidatePurpose /* aPurpose */ )
{
    qsProcStmtOpen      * sOPEN = (qsProcStmtOpen *)aProcStmts;
    qsCursors           * sCursorDef = NULL;
    qsProcStmtSql       * sSQL;
    qsUsingParam        * sCurrParam;
    qtcNode             * sCurrParamNode;
    qsAllVariables      * sOldAllVars;
    qcuSqlSourceInfo      sqlInfo;
    // BUG-37981
    qsxPkgInfo          * sPkgInfo = NULL;
    qcTemplate          * sCursorTemplate;
    SInt                  sState = 0;
    // BUG-41243
    qsVariableItems     * sParaDecl      = NULL;
    UInt                  sParaDeclCount = 0;
    
    QS_SET_PARENT_STMT( aProcStmts, aParentStmt );

    aStatement->spvEnv->currStmt = aProcStmts;

    // check cursor name
    IDE_TEST(getCursorDefinition(
                 aStatement,
                 sOPEN->openCursorSpecNode,
                 &sCursorDef)
             != IDE_SUCCESS);

    if ( sCursorDef == NULL )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sOPEN->openCursorSpecNode->position );
        IDE_RAISE(ERR_NOT_EXIST_CURSOR);
    }
    else
    {
        sSQL = sCursorDef->mCursorSql;

        // BUG-38767
        if(( sCursorDef->common.objectID == QS_EMPTY_OID ) &&
           ( sCursorDef->sqlIdx != ID_UINT_MAX ) )
        {
            sOPEN->sqlIdx = sCursorDef->sqlIdx;
        }
        else
        {
            // package spec에 선언한 cursor이면 sqlIdx를 새로 할당한다.
            if ( aStatement->spvEnv->createProc != NULL )
            {
                sOPEN->sqlIdx = aStatement->spvEnv->createProc->procSqlCount++;
            }
            else
            {
                sOPEN->sqlIdx = ID_UINT_MAX;
            }
        }
    }

    // check cursor arguments
    for ( sParaDecl = sCursorDef->paraDecls;
          sParaDecl != NULL ;
          sParaDecl = sParaDecl->next )
    {
        sParaDeclCount++;
    }

    IDE_TEST( qsv::validateArgumentsWithoutParser( aStatement,
                                                   sOPEN->openCursorSpecNode,
                                                   sCursorDef->paraDecls,
                                                   sParaDeclCount,
                                                   ID_FALSE /* No subquery is allowed */)
              != IDE_SUCCESS );

    IDE_TEST(qsvProcStmts::connectAllVariables(
                 aStatement,
                 sSQL->common.parentLabels, (qsVariableItems *)sCursorDef->paraDecls, ID_TRUE,
                 &sOldAllVars )
             != IDE_SUCCESS);

    if( sSQL->usingParams != NULL )
    {
        // BUG-37981
        if( sCursorDef->common.objectID != QS_EMPTY_OID )
        {
            IDE_TEST( qsxPkg::getPkgInfo( sCursorDef->common.objectID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            sCursorTemplate = sPkgInfo->tmplate;

            QC_CONNECT_TEMPLATE_STACK(
                sCursorTemplate,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stackBuffer,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stack,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stackCount,
                QC_SHARED_TMPLATE(aStatement)->tmplate.stackRemain );
            sCursorTemplate->stmt = QC_SHARED_TMPLATE(aStatement)->stmt;
            sState = 1;
        }
        else
        {
            sCursorTemplate = QC_SHARED_TMPLATE(aStatement);
        }

        for( sCurrParam = sSQL->usingParams;
             sCurrParam != NULL;
             sCurrParam = sCurrParam->next )
        {
            sCurrParamNode = sCurrParam->paramNode;

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

            IDE_TEST( qtc::estimate( sCurrParamNode,
                                     sCursorTemplate, 
                                     aStatement,
                                     NULL,
                                     NULL,
                                     NULL )
                      != IDE_SUCCESS );

            if ( ( sCurrParam->inOutType != QS_IN ) &&
                 ( ( sCurrParamNode->lflag & QTC_NODE_OUTBINDING_MASK )
                 == QTC_NODE_OUTBINDING_DISABLE ) )
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

        // BUG-37981
        if( sCursorDef->common.objectID != QS_EMPTY_OID )
        {
            QC_DISCONNECT_TEMPLATE_STACK( sCursorTemplate );
            sCursorTemplate->stmt = NULL; 
            sState = 0;
        }
        else
        {
            // Nothing to do.
        }
    }

    qsvProcStmts::disconnectAllVariables( aStatement, sOldAllVars );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_CURSOR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_OUT_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INVALID_OUT_ARGUEMNT_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    // BUG-37981
    if( sState == 1 )
    {
        QC_DISCONNECT_TEMPLATE_STACK( sCursorTemplate );
        sCursorTemplate->stmt = NULL;
        sState = 0; 
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsvCursor::validateFetch( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose /* aPurpose */ )
{
    qsProcStmtFetch     * sFETCH = (qsProcStmtFetch *)aProcStmts;
    qsCursors           * sCursorDef = NULL;
    qmsParseTree        * sSelectParseTree;
    qcuSqlSourceInfo      sqlInfo;
    
    QS_SET_PARENT_STMT( aProcStmts, aParentStmt );

    aStatement->spvEnv->currStmt = aProcStmts;

    // check cursor name
    IDE_TEST(getCursorDefinition( aStatement,
                                  sFETCH->cursorNode,
                                  &sCursorDef)
             != IDE_SUCCESS);

    if ( sCursorDef == NULL )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sFETCH->cursorNode->position );
        IDE_RAISE(ERR_NOT_EXIST_CURSOR);
    }
    else
    {
        // Nothing to do.
    }
    
    // check INTO clause
    sSelectParseTree = (qmsParseTree*)sCursorDef->mCursorSql->parseTree;
    
    IDE_TEST( qsvProcStmts::validateIntoClause( aStatement,
                                                sSelectParseTree->querySet->target,
                                                sFETCH->intoVariableNodes )
              != IDE_SUCCESS );
    
    /* BUG-41242 */
    // bulk collection에서만 limit을 사용할 수 있다.
    if ( sFETCH->intoVariableNodes->bulkCollect == ID_TRUE )
    {
        if ( sFETCH->limit != NULL )
        {
            IDE_TEST( qmv::validateLimit( aStatement,
                                          sFETCH->limit )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_CURSOR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC qsvCursor::validateClose( qcStatement * aStatement,
                                 qsProcStmts * aProcStmts,
                                 qsProcStmts * aParentStmt,
                                 qsValidatePurpose /* aPurpose */ )
{
    qsProcStmtClose     * sCLOSE = (qsProcStmtClose *)aProcStmts;
    qsCursors           * sCursorDef = NULL;
    qcuSqlSourceInfo      sqlInfo;
    
    QS_SET_PARENT_STMT( aProcStmts, aParentStmt );

    aStatement->spvEnv->currStmt = aProcStmts;

    // check cursor name
    IDE_TEST(getCursorDefinition(
                 aStatement,
                 sCLOSE->cursorNode,
                 &sCursorDef)
             != IDE_SUCCESS);

    if ( sCursorDef == NULL )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sCLOSE->cursorNode->position );
        IDE_RAISE(ERR_NOT_EXIST_CURSOR);
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_CURSOR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC qsvCursor::getCursorDefinition(
    qcStatement     * aStatement,
    qtcNode         * aCursorNode,
    qsCursors      ** aCursorDef)
{
    qsCursors           * sCursorDef = NULL;
    qsAllVariables      * sCurrVar;
    qsLabels            * sLabel;
    qsVariableItems     * sVariableItem;
    idBool                sFind      = ID_FALSE;
    qsVariableItems     * sCurrDeclItem;
    qcuSqlSourceInfo      sqlInfo;
    qsPkgStmtBlock      * sBLOCK;
    UInt                  sUserID;   // BUG-38164
    qtcNode             * sCursorNode = NULL;
    
    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    /* BUG-38164
       package 내에 선언 cursor의 경우, package 생성 시 userID를,
       procedure 내에 선언된 겨우, procedure 생성 시 userID를
       sUserID에 설정해 준다. */
    if ( aStatement->spvEnv->createProc != NULL )
    {
        sUserID = aStatement->spvEnv->createProc->userID;
        sCursorNode = aStatement->spvEnv->createProc->sqlCursorTypeNode;
    }
    else
    {
        IDE_DASSERT( aStatement->spvEnv->createPkg != NULL );
        sUserID = aStatement->spvEnv->createPkg->userID;
        sCursorNode = aStatement->spvEnv->createPkg->sqlCursorTypeNode;
    }
    
    for ( sCurrVar = aStatement->spvEnv->allVariables;
          sCurrVar != NULL && sFind == ID_FALSE;
          sCurrVar = sCurrVar->next )
    {
        if ( QC_IS_NULL_NAME(aCursorNode->tableName) != ID_TRUE )
        {
            // label_name.cursor_name
            for ( sLabel = sCurrVar->labels;
                  sLabel != NULL && sFind == ID_FALSE;
                  sLabel = sLabel->next )
            {
                if ( QC_IS_NAME_MATCHED( sLabel->namePos, aCursorNode->tableName ) )
                {
                    for ( sVariableItem = sCurrVar->variableItems;
                          (sVariableItem != NULL) &&
                          (sVariableItem != sCurrDeclItem) &&
                          (sFind == ID_FALSE);
                          sVariableItem = sVariableItem->next )
                    {
                        if ( ( sVariableItem->itemType == QS_CURSOR ) &&
                             QC_IS_NAME_MATCHED( sVariableItem->name, aCursorNode->columnName ) )
                        {
                            sCursorDef = (qsCursors *)sVariableItem;
                            sCursorDef->userID = sUserID;   // BUG-38164
                            sCursorDef->common.objectID = QS_EMPTY_OID;
                            sFind = ID_TRUE;
                            break;
                        }
                    }
                }
            }
        } /* QC_IS_NULL_NAME(aCursorNode->tableName) != ID_TRUE */
        else
        {
            // cursor_name
            for ( sVariableItem = sCurrVar->variableItems;
                  ( sVariableItem != NULL ) &&
                  ( sVariableItem != sCurrDeclItem ) &&
                  ( sFind == ID_FALSE );
                  sVariableItem = sVariableItem->next )
            {
                if ( ( sVariableItem->itemType == QS_CURSOR ) &&
                     QC_IS_NAME_MATCHED( sVariableItem->name, aCursorNode->columnName ) )
                {
                    sCursorDef = (qsCursors *)sVariableItem;
                    sCursorDef->userID = sUserID;   // BUG-38164       
                    sCursorDef->common.objectID = QS_EMPTY_OID;
                    sFind = ID_TRUE;
                    break;
                }
            }
        }
    } /* end for */

    // package의 subprogram일 때,
    // subprogram 내부 cursor 변수를 찾지 못했으면, 
    // package의 body와 spec에서 찾아봐야 한다.
    IDE_TEST_CONT( aStatement->spvEnv->createPkg == NULL , skip_search_cursor );

    if ( sFind == ID_FALSE )
    {
        sBLOCK = aStatement->spvEnv->createPkg->block;

        IDE_TEST( getPkgLocalCursorDefinition( aStatement,
                                               aCursorNode,
                                               sBLOCK,
                                               &sCursorDef,
                                               &sFind )
                  != IDE_SUCCESS );

        if ( sFind == ID_TRUE )
        {
            IDE_DASSERT( sCursorDef != NULL );
            sCursorDef->userID = aStatement->spvEnv->createPkg->userID;   // BUG-38164
            sCursorDef->common.objectID = QS_EMPTY_OID;
        }
        else
        {
            IDE_TEST_CONT( aStatement->spvEnv->createPkg->objType != QS_PKG_BODY , 
                           skip_search_cursor );

            IDE_DASSERT( aStatement->spvEnv->pkgPlanTree != NULL );
            IDE_DASSERT( aStatement->spvEnv->pkgPlanTree->objType == QS_PKG );
            IDE_DASSERT( aStatement->spvEnv->pkgPlanTree->pkgInfo != NULL );

            sBLOCK = aStatement->spvEnv->pkgPlanTree->block;

            IDE_TEST( getPkgLocalCursorDefinition( aStatement,
                                                   aCursorNode,
                                                   sBLOCK,
                                                   &sCursorDef,
                                                   &sFind )
                      != IDE_SUCCESS );

            if ( sFind == ID_TRUE )
            {
                IDE_DASSERT( sCursorDef != NULL );
                sCursorDef->userID = aStatement->spvEnv->pkgPlanTree->userID;   // BUG-38164
                sCursorDef->common.objectID = aStatement->spvEnv->pkgPlanTree->pkgOID;
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( skip_search_cursor );

    if ( sFind == ID_TRUE )
    {
        aCursorNode->node.table = sCursorDef->common.table;
        aCursorNode->node.column = sCursorDef->common.column;
        aCursorNode->node.objectID = sCursorDef->common.objectID;
    }
    else
    {
        if ( QC_IS_NULL_NAME(aCursorNode->tableName) != ID_TRUE )
        {
            IDE_TEST( getPkgCursorDefinition( aStatement,
                                              aCursorNode,
                                              &sCursorDef,
                                              &sFind )
                      != IDE_SUCCESS );

            if ( sFind == ID_TRUE )
            {
                aCursorNode->node.table = sCursorDef->common.table;
                aCursorNode->node.column = sCursorDef->common.column;
                aCursorNode->node.objectID = sCursorDef->common.objectID;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    }

    if ( sFind == ID_FALSE )
    {
        if ( idlOS::strMatch( aCursorNode->columnName.stmtText + aCursorNode->columnName.offset,
                              aCursorNode->columnName.size,
                              "SQL",
                              3 ) == 0 )
        {
            // BUG-41386
            aCursorNode->node.table  = sCursorNode->node.table;
            aCursorNode->node.column = sCursorNode->node.column;
            aCursorNode->node.objectID = sCursorNode->node.objectID;
        }
        else
        {
            (void)sqlInfo.setSourceInfo( aStatement,
                                         &aCursorNode->position );

            IDE_RAISE(ERR_NOT_EXIST_CURSOR);
        }
    }
    else
    {
        // BUG-37655
        // pragma restrict reference
        if ( aStatement->spvEnv->createPkg != NULL )
        {
            IDE_DASSERT( sCursorDef != NULL );

            if ( aStatement->spvEnv->createPkg->objType == QS_PKG_BODY )
            {
                IDE_TEST_RAISE( qsvPkgStmts::checkPragmaRestrictReferencesOption( 
                                    aStatement,
                                    aStatement->spvEnv->currSubprogram,
                                    & sCursorDef->mCursorSql->common,
                                    aStatement->spvEnv->isPkgInitializeSection )
                                != IDE_SUCCESS, ERR_VIOLATES_PRAGMA_CURSOR );
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    *aCursorDef = sCursorDef;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_CURSOR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_VIOLATES_PRAGMA_CURSOR );
    {
        // set original error code.
        qsxEnv::setErrorCode( QC_QSX_ENV(aStatement) );

        // BUG-43998
        // PSM 생성 오류 발생시 오류 발생 위치를 한 번만 출력하도록 합니다.
        if ( ideHasErrorPosition() == ID_FALSE )
        {
            (void)sqlInfo.setSourceInfo( aStatement,
                                         &aCursorNode->position );

            (void)sqlInfo.initWithBeforeMessage(
                aStatement->qmeMem );

            IDE_SET(
                ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                                sqlInfo.getBeforeErrMessage(),
                                sqlInfo.getErrMessage()));
            (void)sqlInfo.fini();
        }

        // set sophisticated error message.
        qsxEnv::setErrorMessage( QC_QSX_ENV(aStatement) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1073 Package
IDE_RC qsvCursor::getPkgCursorDefinition( qcStatement  * aStatement,
                                          qtcNode      * aCursorNode,
                                          qsCursors   ** aCursorDef, 
                                          idBool       * aFind )
{
    qsOID              sPkgOID;
    UInt               sPkgUserID;
    qsxPkgInfo       * sPkgInfo;
    idBool             sExists = ID_FALSE;
    qcmSynonymInfo     sSynonymInfo;
    qsPkgStmtBlock   * sBLOCK;
    qsVariableItems  * sVariable;
    qsCursors        * sCursorDef = NULL;
    qcuSqlSourceInfo   sqlInfo;
    idBool             sFind = ID_FALSE;
    
    *aFind = ID_FALSE;

    IDE_TEST( qcmSynonym::resolvePkg( aStatement,
                                      aCursorNode->userName,
                                      aCursorNode->tableName,
                                      &sPkgOID,
                                      &sPkgUserID,
                                      &sExists,
                                      &sSynonymInfo )
              != IDE_SUCCESS );

    if( sExists == ID_TRUE )
    {
        // synonym으로 참조되는 proc의 기록
        IDE_TEST( qsvPkgStmts::makePkgSynonymList(
                      aStatement,
                      &sSynonymInfo,
                      aCursorNode->userName,
                      aCursorNode->tableName,
                      sPkgOID )
                  != IDE_SUCCESS );

        IDE_TEST( qsvPkgStmts::makeRelatedObjects(
                      aStatement,
                      & aCursorNode->userName,
                      & aCursorNode->tableName,
                      & sSynonymInfo,
                      0,
                      QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree(
                      aStatement,
                      sPkgOID,
                      QS_PKG,
                      & aStatement->spvEnv->procPlanList )
                  != IDE_SUCCESS );

        IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                      &sPkgInfo )
                  != IDE_SUCCESS);

        /* BUG-45164 */
        IDE_TEST_RAISE( sPkgInfo->isValid != ID_TRUE, err_object_invalid );

        IDE_TEST( qdpRole::checkDMLExecutePSMPriv(
                      aStatement,
                      sPkgInfo->planTree->userID,
                      sPkgInfo->privilegeCount,
                      sPkgInfo->granteeID,
                      ID_FALSE,
                      NULL,
                      NULL )
                  != IDE_SUCCESS );

        IDE_DASSERT( sPkgInfo->objType == QS_PKG );

        // package에 선언된 cursor를 찾는다.
        sBLOCK = sPkgInfo->planTree->block;

        for( sVariable = sBLOCK->variableItems;
             sVariable != NULL;
             sVariable = sVariable->next )
        {
            if ( ( sVariable->itemType == QS_CURSOR ) &&
                 QC_IS_NAME_MATCHED( sVariable->name, aCursorNode->columnName ) )
            {
                sCursorDef = (qsCursors *)sVariable;
                sCursorDef->userID = sPkgInfo->planTree->userID;    // BUG-38164
                sCursorDef->common.objectID = sPkgInfo->planTree->pkgOID;
                sFind = ID_TRUE;
                break;
            }
        }

        if( sFind == ID_TRUE )
        {
            aCursorNode->node.table = sCursorDef->common.table;
            aCursorNode->node.column = sCursorDef->common.column;
            aCursorNode->node.objectID = sCursorDef->common.objectID;
        }
        else
        {
            (void)sqlInfo.setSourceInfo( aStatement,
                                         &aCursorNode->position );

            IDE_RAISE(ERR_NOT_EXIST_CURSOR);
        }
    }
    else
    {
        // Nothing to do.
    }

    *aFind = sFind;
    *aCursorDef = sCursorDef;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_CURSOR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */ 
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvCursor::getPkgLocalCursorDefinition( qcStatement     * aStatement,
                                               qtcNode         * aCursorNode,
                                               qsPkgStmtBlock  * aBlock,
                                               qsCursors      ** aCursorDef,
                                               idBool          * aFind )
{
/***********************************************************************
 *
 * Description : BUG-38164 
 *               package spec/body에서 선언한 cursor 검색
 *
 * Implementation :
 *    (1) cursor_name
 *    (2) my_package_name.cursor_name
 *           (a) package_name이 동일한지 확인
 *    (3) my_user_name.my_package_name.cursor_name
 *           (a) aCursorNode->userName에서 userID를 얻어온다
 *           (b) pkgParseTree의 userID와 동일한지 확인
 *           (c) package_name 동일한지 확인
 **********************************************************************/

    qsPkgParseTree      * sPkgParseTree;
    qsVariableItems     * sVariableItem;
    qsCursors           * sCursorDef = NULL;
    idBool                sFind      = ID_FALSE;
    UInt                  sUserID    = 0;
    
    IDE_TEST_CONT( QC_IS_NULL_NAME(aCursorNode->pkgName) == ID_FALSE,
                   skip_search_cursor );
    IDE_DASSERT( QC_IS_NULL_NAME(aCursorNode->columnName) != ID_TRUE );

    sPkgParseTree = aStatement->spvEnv->createPkg;

    IDE_TEST_CONT( aBlock->variableItems == NULL, skip_search_cursor );

    if ( QC_IS_NULL_NAME(aCursorNode->userName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME(aCursorNode->tableName) == ID_TRUE )
        {
            sFind = ID_TRUE;
        }
        else
        {
            /* tableName과 package name 동일하면
               sFind를 ID_TRUE로 셋팅한다. */
            if ( QC_IS_NAME_MATCHED( sPkgParseTree->pkgNamePos, aCursorNode->tableName ) )
            {
                sFind = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        } /* QC_IS_NULL_NAME(aCursorNode->tableName) != ID_TRUE */ 
    } /* QC_IS_NULL_NAME(aCursorNode->userName) == ID_TRUE */
    else
    {
        /* userID가 동일하고, tableName과 package name이 동일하면
           sFind을 ID_TRUE로 셋팅한다. */
        IDE_TEST( qcmUser::getUserID( aStatement,
                                      aCursorNode->userName,
                                      &sUserID )
                  != IDE_SUCCESS );

        if ( sUserID == sPkgParseTree->userID )
        {
            if ( QC_IS_NAME_MATCHED( sPkgParseTree->pkgNamePos, aCursorNode->tableName ) )
            {
                sFind = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }
    } /* QC_IS_NULL_NAME(aCursorNode->userName) == ID_FALSE */

    /* 위의에서 sFind가 ID_TRUE일 경우, package 내부에 선언된 cursor일 수도 있다.
       package 내부에 선언한 variableItems를 돌면서 
       columnName과 동일한 cursor가 존재하는지 찾는다..*/
    if ( sFind == ID_TRUE )
    {
        for ( sVariableItem = aBlock->variableItems;
              sVariableItem != NULL;
              sVariableItem = sVariableItem->next )
        {
            if ( sVariableItem->itemType == QS_CURSOR )
            {
                if ( QC_IS_NAME_MATCHED( sVariableItem->name, aCursorNode->columnName ) )
                {
                    sCursorDef = (qsCursors *)sVariableItem;
                    sFind = ID_TRUE;
                    break;
                }
                else
                {
                    sFind = ID_FALSE;
                }
            }
            else
            {
                sFind = ID_FALSE;
            }
        }
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( skip_search_cursor );

    *aFind      = sFind;
    *aCursorDef = sCursorDef;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

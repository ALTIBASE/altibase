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
 * $Id: qsv.cpp 90971 2021-06-09 00:29:36Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idx.h>
#include <cm.h>
#include <qsv.h>
#include <qsx.h>
#include <qcmUser.h>
#include <qcuSqlSourceInfo.h>
#include <qsvProcVar.h>
#include <qsvProcType.h>
#include <qsvProcStmts.h>
#include <qsvPkgStmts.h>
#include <qcmPkg.h>
#include <qcmProc.h>
#include <qsvEnv.h>
#include <qsxRelatedProc.h>
#include <qsxPkg.h>
#include <qcpManager.h>
#include <qdpPrivilege.h>
#include <qmv.h>
#include <qso.h>
#include <qcg.h>
#include <qcmSynonym.h>
#include <qmvQuerySet.h>
#include <qcgPlan.h>
#include <qdpRole.h>
#include <qtcCache.h>
#include <qmvShardTransform.h>
#include <sdi.h>

extern mtdModule mtdInteger;

extern mtfModule qsfMCountModule;
extern mtfModule qsfMDeleteModule;
extern mtfModule qsfMExistsModule;
extern mtfModule qsfMFirstModule;
extern mtfModule qsfMLastModule;
extern mtfModule qsfMNextModule;
extern mtfModule qsfMPriorModule;

extern qcNamePosition gSysName;
qcNamePosition gDBMSStandardName = {(SChar*)"DBMS_STANDARD", 0, 13};

IDE_RC qsv::parseCreateProc(qcStatement * aStatement)
{
#define IDE_FN "qsv::parseCreateProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::parseCreateProc"));

    qsProcParseTree    * sParseTree;
    qcuSqlSourceInfo     sqlInfo;

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    aStatement->spvEnv->createProc = sParseTree;
 
    /* BUG-30059 */
    if ( qdbCommon::containDollarInName( &(sParseTree->procNamePos) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &(sParseTree->procNamePos) );

        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    // validation시에 참조되기 때문에 임시로 지정한다.
    // PVO가 완료되면 정상적인 값으로 지정된다.
    sParseTree->stmtText    = aStatement->myPlan->stmtText;
    sParseTree->stmtTextLen = aStatement->myPlan->stmtTextLen;

    IDE_TEST(checkHostVariable( aStatement ) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateCreateProc(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateCreateProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateCreateProc"));

    qsProcParseTree    * sParseTree;
    qsLabels           * sCurrLabel;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist = ID_FALSE;
    UInt                 sUserID = QCG_GET_SESSION_USER_ID( aStatement );

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    // check already existing object
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->procNamePos,
                                sParseTree->objType,
                                &( sParseTree->userID ),
                                &( sParseTree->procOID ),
                                & sExist )
              != IDE_SUCCESS );

    /* 1. procOID가 있으면 같은 이름의 PSM이 있으므로 오류
     * 2. procOID는 QS_EMPTY_OID인데, OBJECT가 존재하면
     *    같은 이름의 PSM이 아닌 OBJECT가 존재 (예를 들면 TABLE) */
    if( sParseTree->procOID != QS_EMPTY_OID )
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->procNamePos );
        IDE_RAISE( ERR_DUP_PROC_NAME );
    }
    else
    {
        IDE_TEST_RAISE(sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME);
    }

    // check grant
    IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                              sParseTree->userID )
              != IDE_SUCCESS );

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->procNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );
 
    // validation parameter
    IDE_TEST(qsvProcVar::validateParaDef(aStatement, sParseTree->paraDecls)
             != IDE_SUCCESS);

    IDE_TEST(qsvProcVar::setParamModules(
                 aStatement,
                 sParseTree->paraDeclCount,
                 sParseTree->paraDecls,
                 (mtdModule***) & (sParseTree->paramModules),
                 &(sParseTree->paramColumns))
             != IDE_SUCCESS );

    // PR-24408
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );
    
    // PROJ-1685
    if( sParseTree->procType == QS_NORMAL )
    {
        // set label name (= procedure name)
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qsLabels, &sCurrLabel)
                 != IDE_SUCCESS);

        SET_POSITION( sCurrLabel->namePos, sParseTree->procNamePos );
        sCurrLabel->id      = qsvEnv::getNextId(aStatement->spvEnv);
        sCurrLabel->stmt    = NULL;
        sCurrLabel->next    = NULL;

        sParseTree->block->common.parentLabels = sCurrLabel;

        // parsing body
        IDE_TEST( sParseTree->block->common.parse(
                      aStatement, (qsProcStmts *)(sParseTree->block))
                  != IDE_SUCCESS);
 
        // validation body
        IDE_TEST( sParseTree->block->common.
                  validate( aStatement,
                            (qsProcStmts *)(sParseTree->block),
                            NULL,
                            QS_PURPOSE_PSM )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( validateExtproc( aStatement, sParseTree ) != IDE_SUCCESS );
    }

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
   
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION(ERR_DUP_PROC_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_DUPLICATED_PROC_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateReplaceProc(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateReplaceProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateReplaceProc"));

    qsProcParseTree    * sParseTree;
    qsLabels           * sCurrLabel;
    idBool               sExist = ID_FALSE;
    UInt                 sUserID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool               sIsUsed = ID_FALSE;
    SInt                 sObjectType = QS_OBJECT_MAX;

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    // check already exist object
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->procNamePos,
                                sParseTree->objType,
                                &( sParseTree->userID ),
                                &( sParseTree->procOID ),
                                & sExist )
              != IDE_SUCCESS );

    /* BUG-48290 shard object에 대한 DDL 차단 */
    IDE_TEST( sdi::checkShardObjectForDDL( aStatement, SDI_DDL_TYPE_PROCEDURE ) != IDE_SUCCESS );

    if( sExist == ID_TRUE )
    {
        if( sParseTree->procOID == QS_EMPTY_OID )
        {
            IDE_RAISE( ERR_EXIST_OBJECT_NAME );
        }
        else
        {
            /* BUG-39101
               객체의 type을 확인하여 parse tree의 type과 동일하지 않으면,
               동일한 객체명을 가진 다른 type의 psm 객체이므로
               에러메시지를 셋팅한다. */
            IDE_TEST( qcmProc::getProcObjType( aStatement,
                                               sParseTree->procOID,
                                               &sObjectType )
                      != IDE_SUCCESS );

            if ( sObjectType == sParseTree->objType )
            {
                // replace old procedure
                sParseTree->common.execute = qsx::replaceProcOrFunc;

                // check grant
                IDE_TEST( qdpRole::checkDDLAlterPSMPriv(
                              aStatement,
                              sParseTree->userID )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_RAISE( ERR_EXIST_OBJECT_NAME );
            }
        }
    }
    else
    {
        // create new procedure
        sParseTree->common.execute = qsx::createProcOrFunc;

        // check grant
        IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                                  sParseTree->userID )
                  != IDE_SUCCESS );
    }

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->procNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    // BUG-38800 Server startup시 Function-Based Index에서 사용 중인 Function을
    // recompile 할 수 있어야 한다.
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
         != QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
    {
        /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        IDE_TEST( qcmProc::relIsUsedProcByConstraint(
                      aStatement,
                      &(sParseTree->procNamePos),
                      sParseTree->userID,
                      &sIsUsed )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_CONSTRAINT );

        IDE_TEST( qcmProc::relIsUsedProcByIndex(
                      aStatement,
                      &(sParseTree->procNamePos),
                      sParseTree->userID,
                      &sIsUsed )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_INDEX );
    }
    else
    {
        // Nothing to do.
    }

    // validation parameter
    IDE_TEST(qsvProcVar::validateParaDef(aStatement, sParseTree->paraDecls)
             != IDE_SUCCESS);

    IDE_TEST(qsvProcVar::setParamModules(
                 aStatement,
                 sParseTree->paraDeclCount,
                 sParseTree->paraDecls,
                 (mtdModule***) & sParseTree->paramModules,
                 &(sParseTree->paramColumns) )
             != IDE_SUCCESS );

    // PR-24408
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

    // PROJ-1685
    if( sParseTree->procType == QS_NORMAL )
    {
        // set label name (= procedure name)
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qsLabels, &sCurrLabel)
                 != IDE_SUCCESS);
 
        SET_POSITION( sCurrLabel->namePos, sParseTree->procNamePos );
 
        sCurrLabel->id   = qsvEnv::getNextId(aStatement->spvEnv);
        sCurrLabel->stmt = NULL;
        sCurrLabel->next = NULL;
 
        sParseTree->block->common.parentLabels = sCurrLabel;

        // parsing body
        IDE_TEST( sParseTree->block->common.parse(
                      aStatement,
                      (qsProcStmts *)(sParseTree->block))
                  != IDE_SUCCESS);
        
        // validation body
        IDE_TEST( sParseTree->block->common.validate(
                      aStatement,
                      (qsProcStmts *)(sParseTree->block),
                      NULL,
                      QS_PURPOSE_PSM )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( validateExtproc( aStatement, sParseTree ) != IDE_SUCCESS );
    }

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_CONSTRAINT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_CONSTRAINT ) );
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_INDEX ) );
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateReplaceProcForRecompile(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateReplaceProcForRecompile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateReplaceProcForRecompile"));

    qsProcParseTree    * sParseTree;
    qsLabels           * sCurrLabel;
    idBool               sExist = ID_FALSE;
    UInt                 sUserID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool               sIsUsed = ID_FALSE;
    SInt                 sObjectType = QS_OBJECT_MAX;

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    // check already exist object
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->procNamePos,
                                sParseTree->objType,
                                &( sParseTree->userID ),
                                &( sParseTree->procOID ),
                                & sExist )
              != IDE_SUCCESS );

    if( sExist == ID_TRUE )
    {
        if( sParseTree->procOID == QS_EMPTY_OID )
        {
            IDE_RAISE( ERR_EXIST_OBJECT_NAME );
        }
        else
        {
            /* BUG-39101
               객체의 type을 확인하여 parse tree의 type과 동일하지 않으면,
               동일한 객체명을 가진 다른 type의 psm 객체이므로
               에러메시지를 셋팅한다. */
            IDE_TEST( qcmProc::getProcObjType( aStatement,
                                               sParseTree->procOID,
                                               &sObjectType )
                      != IDE_SUCCESS );

            if ( sObjectType == sParseTree->objType )
            {
                // replace old procedure
                sParseTree->common.execute = qsx::replaceProcOrFunc;

                // check grant
                IDE_TEST( qdpRole::checkDDLAlterPSMPriv(
                              aStatement,
                              sParseTree->userID )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_RAISE( ERR_EXIST_OBJECT_NAME );
            }
        }
    }
    else
    {
        // create new procedure
        sParseTree->common.execute = qsx::createProcOrFunc;

        // check grant
        IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                                  sParseTree->userID )
                  != IDE_SUCCESS );
    }

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->procNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    // BUG-38800 Server startup시 Function-Based Index에서 사용 중인 Function을
    // recompile 할 수 있어야 한다.
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
         != QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
    {
        /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        IDE_TEST( qcmProc::relIsUsedProcByConstraint(
                      aStatement,
                      &(sParseTree->procNamePos),
                      sParseTree->userID,
                      &sIsUsed )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_CONSTRAINT );

        IDE_TEST( qcmProc::relIsUsedProcByIndex(
                      aStatement,
                      &(sParseTree->procNamePos),
                      sParseTree->userID,
                      &sIsUsed )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_INDEX );
    }
    else
    {
        // Nothing to do.
    }

    // validation parameter
    IDE_TEST(qsvProcVar::validateParaDef(aStatement, sParseTree->paraDecls)
             != IDE_SUCCESS);

    IDE_TEST(qsvProcVar::setParamModules(
                 aStatement,
                 sParseTree->paraDeclCount,
                 sParseTree->paraDecls,
                 (mtdModule***) & sParseTree->paramModules,
                 &(sParseTree->paramColumns) )
             != IDE_SUCCESS );

    // PR-24408
    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

    // PROJ-1685
    if( sParseTree->procType == QS_NORMAL )
    {
        // set label name (= procedure name)
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qsLabels, &sCurrLabel)
                 != IDE_SUCCESS);
 
        SET_POSITION( sCurrLabel->namePos, sParseTree->procNamePos );
 
        sCurrLabel->id   = qsvEnv::getNextId(aStatement->spvEnv);
        sCurrLabel->stmt = NULL;
        sCurrLabel->next = NULL;
 
        sParseTree->block->common.parentLabels = sCurrLabel;

        // parsing body
        IDE_TEST( sParseTree->block->common.parse(
                      aStatement,
                      (qsProcStmts *)(sParseTree->block))
                  != IDE_SUCCESS);
        
        // validation body
        IDE_TEST( sParseTree->block->common.validate(
                      aStatement,
                      (qsProcStmts *)(sParseTree->block),
                      NULL,
                      QS_PURPOSE_PSM )
                  != IDE_SUCCESS);
    }
    else
    {
        IDE_TEST( validateExtproc( aStatement, sParseTree ) != IDE_SUCCESS );
    }

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_CONSTRAINT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_CONSTRAINT ) );
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_INDEX ) );
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );
    
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateCreateFunc(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateCreateFunc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateCreateFunc"));

    IDE_TEST( validateCreateProc( aStatement ) != IDE_SUCCESS );

    IDE_TEST( validateReturnDef( aStatement ) != IDE_SUCCESS );

    // check only in normal procedure.
    if ( ( aStatement->spvEnv->flag & QSV_ENV_RETURN_STMT_MASK )
         == QSV_ENV_RETURN_STMT_ABSENT )
    {
        IDE_RAISE( ERR_NOT_HAVE_RETURN_STMT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_HAVE_RETURN_STMT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_STMT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateReplaceFunc(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateReplaceFunc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateReplaceFunc"));


    IDE_TEST( validateReplaceProc( aStatement ) != IDE_SUCCESS );

    IDE_TEST( validateReturnDef( aStatement ) != IDE_SUCCESS );

    // check only in normal procedure.
    if ( ( aStatement->spvEnv->flag & QSV_ENV_RETURN_STMT_MASK )
         == QSV_ENV_RETURN_STMT_ABSENT )
    {
        IDE_RAISE( ERR_NOT_HAVE_RETURN_STMT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_HAVE_RETURN_STMT);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_STMT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateReturnDef( qcStatement * aStatement )
{
    qsProcParseTree    * sParseTree;
    mtcColumn          * sReturnTypeNodeColumn;
    qcuSqlSourceInfo     sqlInfo;
    qsVariables        * sReturnTypeVar;
    idBool               sFound;

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    sReturnTypeVar = sParseTree->returnTypeVar;

    // 적합성 검사. return타입은 반드시 있어야 함.
    IDE_DASSERT( sReturnTypeVar != NULL );

    // validation return data type
    if (sReturnTypeVar->variableType == QS_COL_TYPE)
    {
        // return value에서 column type을 허용하는 경우는 단 2가지.
        // (1) table_name.column_name%TYPE
        // (2) user_name.table_name.column_name%TYPE
        if (QC_IS_NULL_NAME(sReturnTypeVar->variableTypeNode->tableName) == ID_TRUE)
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnTypeVar->variableTypeNode->position );
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
        else
        {
            IDE_TEST(qsvProcVar::checkAttributeColType(
                         aStatement, sReturnTypeVar)
                     != IDE_SUCCESS);

            sReturnTypeVar->variableType = QS_PRIM_TYPE;
        }
    }
    else if (sReturnTypeVar->variableType == QS_PRIM_TYPE)
    {
        // Nothing to do.
    }
    else if (sReturnTypeVar->variableType == QS_ROW_TYPE)
    {
        if (QC_IS_NULL_NAME(sReturnTypeVar->variableTypeNode->userName) == ID_TRUE)
        {
            // PROJ-1075 rowtype 생성 허용.
            // (1) user_name.table_name%ROWTYPE
            // (2) table_name%ROWTYPE
            IDE_TEST(qsvProcVar::checkAttributeRowType( aStatement, sReturnTypeVar)
                     != IDE_SUCCESS);
        }
        else
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sReturnTypeVar->variableTypeNode->columnName );
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
    }
    else if ( sReturnTypeVar->variableType == QS_UD_TYPE )
    {
        IDE_TEST( qsvProcVar::makeUDTVariable( aStatement,
                                               sReturnTypeVar )
                  != IDE_SUCCESS );
    }
    else
    {
        // 적합성 검사. 이 이외의 타입은 절대로 올 수 없음.
        IDE_DASSERT(0);
    }

    IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement),
                          mtcColumn,
                          &(sParseTree->returnTypeColumn))
             != IDE_SUCCESS);

    sReturnTypeNodeColumn = QTC_STMT_COLUMN (aStatement,
                                             sReturnTypeVar->variableTypeNode);

    // to fix BUG-5773
    sReturnTypeNodeColumn->column.id     = 0;
    sReturnTypeNodeColumn->column.offset = 0;

    // sReturnTypeNodeColumn->flag = 0;
    // to fix BUG-13600 precision, scale을 복사하기 위해
    // column argument값은 저장해야 함.
    sReturnTypeNodeColumn->flag = sReturnTypeNodeColumn->flag & MTC_COLUMN_ARGUMENT_COUNT_MASK;
    sReturnTypeNodeColumn->type.languageId = sReturnTypeNodeColumn->language->id;

    /* PROJ-2586 PSM Parameters and return without precision
       아래 조건 중 하나만 만족하면 precision을 조정하기 위한 함수 호출.

       조건 1. QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 1이면서
       datatype의 flag에 QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT 일 것.
       조건 2. QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 2 */
    /* 왜 아래쪽에 복사했는가?
       alloc전에 하면, mtcColumn에 셋팅했던 flag 정보가 날라감. */
    if( ((QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 1) &&
         (((sReturnTypeVar->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK)
           == QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT))) /* 조건1 */ ||
        (QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 2) /* 조건2 */ )
    {
        IDE_TEST( setPrecisionAndScale( aStatement,
                                        sReturnTypeVar )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    /* BUG-44382 clone tuple 성능개선 */
    // 복사와 초기화가 필요함
    qtc::setTupleColumnFlag(
        QTC_STMT_TUPLE( aStatement, sReturnTypeVar->variableTypeNode ),
        ID_TRUE,
        ID_TRUE );

    mtc::copyColumn(sParseTree->returnTypeColumn,
                    sReturnTypeNodeColumn);

    // mtdModule 설정
    sParseTree->returnTypeModule = (mtdModule *)sReturnTypeNodeColumn->module;

    // PROJ-1685
    if( sParseTree->block == NULL )
    {
        // external procedure
        sFound = ID_FALSE;
 
        switch( sParseTree->returnTypeColumn->type.dataTypeId )
        {
            case MTD_BIGINT_ID:
            case MTD_BOOLEAN_ID:
            case MTD_SMALLINT_ID:
            case MTD_INTEGER_ID:
            case MTD_REAL_ID:
            case MTD_DOUBLE_ID:
            case MTD_CHAR_ID:
            case MTD_VARCHAR_ID:
            case MTD_NCHAR_ID:
            case MTD_NVARCHAR_ID:
            case MTD_NUMERIC_ID:
            case MTD_NUMBER_ID:
            case MTD_FLOAT_ID:
            case MTD_DATE_ID:
            case MTD_INTERVAL_ID:
                sFound = ID_TRUE;
                break;
            default:
                break;
        }
 
        if ( sFound == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &sParseTree->returnTypeVar->common.name );
 
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        // normal procedure
        sFound = ID_TRUE;
        
        switch( sParseTree->returnTypeColumn->type.dataTypeId )
        {
            case MTD_LIST_ID:
                // list type을 반환하는 user-defined function은 생성할 수 없다.
                sFound = ID_FALSE;
                break;
            default:
                break;
        }
 
        if ( sFound == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &sParseTree->returnTypeVar->common.name );
 
            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }
        else
        {
            // Nothing to do.
        }
    }

    /* PROJ-2452 Cache for DETERMINISTIC function */
    IDE_TEST( qtcCache::setIsCachableForFunction( sParseTree->paraDecls,
                                                  sParseTree->paramModules,
                                                  sParseTree->returnTypeModule,
                                                  &sParseTree->isDeterministic,
                                                  &sParseTree->isCachable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_SUPPORTED_DATATYPE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_SUPPORTED_DATATYPE_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsv::validateDropProc(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateDropProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateDropProc"));

    qsDropParseTree     * sParseTree;
    qcuSqlSourceInfo      sqlInfo;
    SInt                  sObjType;
    idBool                sIsUsed = ID_FALSE;

    sParseTree = (qsDropParseTree *)(aStatement->myPlan->parseTree);

    IDE_TEST(checkUserAndProcedure(
                 aStatement,
                 sParseTree->userNamePos,
                 sParseTree->procNamePos,
                 &(sParseTree->userID),
                 &(sParseTree->procOID) )
             != IDE_SUCCESS);

    /* BUG-48290 shard object에 대한 DDL 차단 */
    IDE_TEST( sdi::checkShardObjectForDDL( aStatement, SDI_DDL_TYPE_PROCEDURE_DROP ) != IDE_SUCCESS );

    // check grant
    IDE_TEST( qdpRole::checkDDLDropPSMPriv( aStatement,
                                            sParseTree->userID )
              != IDE_SUCCESS );
    
    if (sParseTree->procOID == QS_EMPTY_OID)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->procNamePos );
        IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
    }
    else
    {
        IDE_TEST( qcmProc::getProcObjType( aStatement,
                                           sParseTree->procOID,
                                           &sObjType )
                  != IDE_SUCCESS );

        /* BUG-39059
           찾은 객체의 type이 parsing 단계에서 parse tree에 셋팅된
           타입과 동일하지 않을 경우, 에러메시지를 셋팅한다. */
        if( sObjType != sParseTree->objectType )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sParseTree->procNamePos );
            IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
        }
        else
        {
            // Nothing to do.
        }
    }

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    IDE_TEST( qcmProc::relIsUsedProcByConstraint(
                  aStatement,
                  &(sParseTree->procNamePos),
                  sParseTree->userID,
                  &sIsUsed )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_CONSTRAINT );

    IDE_TEST( qcmProc::relIsUsedProcByIndex(
                  aStatement,
                  &(sParseTree->procNamePos),
                  sParseTree->userID,
                  &sIsUsed )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_INDEX );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION(ERR_NOT_EXIST_PROC_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_PROC_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_CONSTRAINT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_CONSTRAINT ) );
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_INDEX ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsv::validateAltProc(qcStatement * aStatement)
{
#define IDE_FN "qsv::validateAltProc"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::validateAltProc"));

    qsAlterParseTree        * sParseTree;
    qcuSqlSourceInfo          sqlInfo;
    SInt                      sObjType;

    sParseTree = (qsAlterParseTree *)(aStatement->myPlan->parseTree);

    IDE_TEST(checkUserAndProcedure(
                 aStatement,
                 sParseTree->userNamePos,
                 sParseTree->procNamePos,
                 &(sParseTree->userID),
                 &(sParseTree->procOID) )
             != IDE_SUCCESS);

    // check grant
    IDE_TEST( qdpRole::checkDDLAlterPSMPriv( aStatement,
                                             sParseTree->userID )
              != IDE_SUCCESS );

    if (sParseTree->procOID == QS_EMPTY_OID)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->procNamePos );
        IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
    }
    else
    {
        IDE_TEST( qcmProc::getProcObjType( aStatement,
                                           sParseTree->procOID,
                                           &sObjType )
                  != IDE_SUCCESS );

        /* BUG-39059
           찾은 객체의 type이 parsing 단계에서 parse tree에 셋팅된
           타입과 동일하지 않을 경우, 에러메시지를 셋팅한다. */
        if( sObjType != sParseTree->objectType )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sParseTree->procNamePos );
            IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PROC_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_PROC_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsv::parseExeProc(qcStatement * aStatement)
{
    qsExecParseTree     * sParseTree;
    idBool                sRecursiveCall     = ID_FALSE;
    qsxProcInfo         * sProcInfo;
    qsProcParseTree     * sProcPlanTree      = NULL;
    qcuSqlSourceInfo      sqlInfo;
    idBool                sExist             = ID_FALSE;
    idBool                sIsPkg             = ID_FALSE;
    qcmSynonymInfo        sSynonymInfo;
    // PROJ-1073 Package
    UInt                  sSubprogramID;
    qcNamePosition      * sUserName          = NULL;
    qcNamePosition      * sPkgName;
    qcNamePosition      * sProcName          = NULL;
    qsxPkgInfo          * sPkgInfo;
    idBool                sMyPkgSubprogram   = ID_FALSE;
    UInt                  sErrCode;
    // BUG-37655
    qsProcStmts         * sProcStmt;
    qsPkgStmts          * sCurrPkgStmt;
    qcNamePosition        sPos;
    qsxPkgObjectInfo    * sPkgBodyObjectInfo = NULL;
    qsOID                 sPkgBodyOID;
    // BUG-39194
    UInt                  sTmpUserID;
    // BUG-39084
    SChar               * sStmtText;
    qciStmtType           sTopStmtKind;
    // BUG-37820
    qsPkgSubprogramType   sSubprogramType;
    qsProcParseTree     * sCrtParseTree;
    UInt                  sOrgFlag;

    qsCallSpec          * sCallSpec;
    SChar                 sPkgNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                 sProcNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SInt                  sProcNameIdx = 0;
    const SChar         * sProcNameArry[] = { "SET_SHARD_TABLE_SHARDKEY",
                                              "SET_SHARD_TABLE_SOLO",
                                              "SET_SHARD_TABLE_CLONE",                                              
                                              "UNSET_SHARD_TABLE",
                                              "UNSET_SHARD_PROCEDURE",
                                              "SET_SHARD_PROCEDURE_SHARDKEY",
                                              "SET_SHARD_PROCEDURE_SOLO",
                                              "SET_SHARD_PROCEDURE_CLONE",
                                              NULL };

    qcNamePosition        sUserNamePosition;
    qcNamePosition        sProcNamePosition;

    IDE_DASSERT( QC_SHARED_TMPLATE(aStatement) != NULL );
    IDE_DASSERT( QC_SHARED_TMPLATE(aStatement)->stmt != NULL );
    IDE_DASSERT( QC_SHARED_TMPLATE(aStatement)->stmt->myPlan != NULL );
    IDE_DASSERT( QC_SHARED_TMPLATE(aStatement)->stmt->myPlan->parseTree != NULL );

    sTopStmtKind  = QC_SHARED_TMPLATE(aStatement)->stmt->myPlan->parseTree->stmtKind;
    sParseTree    = (qsExecParseTree *)(aStatement->myPlan->parseTree);
    sCurrPkgStmt  = aStatement->spvEnv->currSubprogram;
    sCrtParseTree = aStatement->spvEnv->createProc;

    // initialize
    sParseTree->returnModule     = NULL;
    sParseTree->returnTypeColumn = NULL;
    sParseTree->pkgBodyOID       = QS_EMPTY_OID;
    sParseTree->subprogramID     = QS_INIT_SUBPROGRAM_ID;
    sParseTree->isCachable       = ID_FALSE;    // PROJ-2452

    IDU_LIST_INIT( &sParseTree->refCurList );

    if ( aStatement->spvEnv->createPkg == NULL )
    {
        // check recursive call in create procedure statement
        if ( aStatement->spvEnv->createProc != NULL )
        {
            IDE_TEST(checkRecursiveCall(aStatement,
                                        &sRecursiveCall)
                     != IDE_SUCCESS);
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        /* PROJ-1073 Package
           create package 시 동일한 package에 속해있는 다른 subprogram을 호출하는지,
           subprogram이 자기 자신을 재귀호출하는지 검사한다. */
        IDE_TEST( checkMyPkgSubprogramCall( aStatement,
                                            &sMyPkgSubprogram,
                                            &sRecursiveCall )
                  != IDE_SUCCESS );
    }

    if (sRecursiveCall == ID_FALSE)
    {
        if( sMyPkgSubprogram == ID_FALSE )
        {
            /* PROJ-1073 Package
             *
             * procedure call의 경우
             * | USER   | TABLE    | COLUMN   | PKG |
             * |    x   | (USER)   | PSM NAME |   X |
             *
             * package의 procedure call의 경우
             * |  USER  | TABLE    | COLUMN   | PKG |
             * | (USER) | PKG NAME | PSM NAME |   X |
             *
             * 따라서 resolvePkg 호출한 다음 resolvePSM을 호출한다. */
            if ( QC_IS_NULL_NAME( sParseTree->callSpecNode->tableName) == ID_FALSE )
            {
                /* 1. Package의 PSM (sub program)인지 확인한다. */
                // PROJ-1073 Package
                IDE_TEST( qcmSynonym::resolvePkg(
                              aStatement,
                              sParseTree->callSpecNode->userName,
                              sParseTree->callSpecNode->tableName,
                              &(sParseTree->procOID),
                              &(sParseTree->userID),
                              &sExist,
                              &sSynonymInfo )
                          != IDE_SUCCESS );

                if( sExist == ID_TRUE )
                {
                    sUserName = &(sParseTree->callSpecNode->userName);
                    sPkgName  = &(sParseTree->callSpecNode->tableName);
                    sProcName = &(sParseTree->callSpecNode->columnName);

                    sIsPkg = ID_TRUE;

                    // DBMS_SHARD package중에 one transaction으로 수행하는
                    // 프로시저에 대해 flag 설정
                    QC_STR_COPY( sPkgNameStr, sParseTree->callSpecNode->tableName );
                    QC_STR_COPY( sProcNameStr, sParseTree->callSpecNode->columnName );

                    if ( idlOS::strMatch( sPkgNameStr,
                                          idlOS::strlen( sPkgNameStr ),
                                          "DBMS_SHARD",
                                          10 ) == 0 )
                    {
                        for( sProcNameIdx = 0; sProcNameArry[sProcNameIdx] != NULL; sProcNameIdx++ )
                        {           
                            if ( idlOS::strMatch( sProcNameStr,
                                                  idlOS::strlen( sProcNameStr ),
                                                  sProcNameArry[sProcNameIdx],
                                                  idlOS::strlen( sProcNameArry[sProcNameIdx] )) == 0 )
                            {
                                aStatement->mFlag &= ~QC_STMT_SHARD_DBMS_PKG_MASK;
                                aStatement->mFlag |= QC_STMT_SHARD_DBMS_PKG_TRUE;
                            }
                        }
                    }
                }
                else
                {
                    // user.package.proceudre나 variable일 때
                    // user가 없는 경우는 "User not found"가 나오는다.
                    // user가 제대로 된 상태에서 package가 존재하지 않으면, sExists가 false가 되고,
                    // error처리를 하지 않는다.
                    // 따라서, user가 존재하면서, package가 존재하지 않으면, error처리를 해야한다.
                    if( QC_IS_NULL_NAME( sParseTree->callSpecNode->userName ) != ID_TRUE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sParseTree->callSpecNode->tableName);
                        IDE_RAISE( ERR_NOT_EXIST_PKG_NAME );
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

            // BUG-46074 PSM Trigger with multiple triggering events
            // DBMS_STANDARD package의 subprogram을 동작하기 위한 코드
            // INSERTING으로 호출해도 DBMS_STANDARD.INSERTING을 호출한다.
            if( (sExist == ID_FALSE) &&
                (QC_IS_NULL_NAME(sParseTree->callSpecNode->tableName) == ID_TRUE) )
            {
                if ( qcmSynonym::resolvePkg(
                         aStatement,
                         gSysName,
                         gDBMSStandardName,
                         &(sParseTree->procOID),
                         &(sParseTree->userID),
                         &sExist,
                         &sSynonymInfo)
                     == IDE_SUCCESS )
                {
                    if ( sExist == ID_TRUE )
                    {
                        sUserName = &gSysName;
                        sPkgName  = &gDBMSStandardName;
                        sProcName = &(sParseTree->callSpecNode->columnName);

                        IDE_TEST( qsxPkg::getPkgInfo( sParseTree->procOID,
                                                      &sPkgInfo )
                                  != IDE_SUCCESS );

                        sOrgFlag = aStatement->spvEnv->flag;
                        aStatement->spvEnv->flag &= ~QSV_ENV_ESTIMATE_ARGU_MASK;
                        aStatement->spvEnv->flag |= QSV_ENV_ESTIMATE_ARGU_TRUE;

                        if ( (sPkgInfo != NULL) && 
                             (qsvPkgStmts::getPkgSubprogramID( aStatement,
                                                               sParseTree->callSpecNode,
                                                              *sUserName,
                                                              *sPkgName,
                                                              *sProcName,
                                                               sPkgInfo,
                                                              &sSubprogramID )
                              == IDE_SUCCESS) )
                        {
                            if ( sSubprogramID == QS_PSM_SUBPROGRAM_ID )
                            {
                                IDE_CLEAR();
                                sUserName = NULL;
                                sPkgName  = NULL;
                                sProcName = NULL;
                                sExist = ID_FALSE;
                            }
                            else
                            {
                                sIsPkg = ID_TRUE;
                            }
                        }
                        else
                        {
                            IDE_CLEAR();
                            sUserName = NULL;
                            sPkgName  = NULL;
                            sProcName = NULL;
                            sExist = ID_FALSE;
                        }

                        aStatement->spvEnv->flag = sOrgFlag;
                    }
                }
            }

            if( sExist == ID_FALSE )
            {
                /* 2. PSM 인지 확인한다. */
                if( qcmSynonym::resolvePSM(
                        aStatement,
                        sParseTree->callSpecNode->tableName,
                        sParseTree->callSpecNode->columnName,
                        &(sParseTree->procOID),
                        &(sParseTree->userID),
                        &sExist,
                        &sSynonymInfo)
                    == IDE_SUCCESS )
                {
                    if( sExist == ID_TRUE )
                    {
                        sUserName = &(sParseTree->callSpecNode->tableName);
                        sProcName = &(sParseTree->callSpecNode->columnName);
                    }
                    else
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sParseTree->callSpecNode->columnName);
                        IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
                    }
                }
                else
                {
                    sErrCode = ideGetErrorCode();
                    if( sErrCode == qpERR_ABORT_QCM_NOT_EXIST_USER )
                    {
                        if ( QC_IS_NULL_NAME( sParseTree->callSpecNode->tableName ) != ID_TRUE )
                        {
                            sPos.stmtText = sParseTree->callSpecNode->tableName.stmtText;
                            sPos.offset   = sParseTree->callSpecNode->tableName.offset;
                            sPos.size     = sParseTree->callSpecNode->columnName.offset + 
                                sParseTree->callSpecNode->columnName.size -
                                sParseTree->callSpecNode->tableName.offset;
                        }
                        else
                        {
                            sPos = sParseTree->callSpecNode->columnName;
                        }

                        sqlInfo.setSourceInfo( aStatement,
                                               & sPos );
                        IDE_RAISE( ERR_INVALID_IDENTIFIER );
                    }
                    else
                    {
                        IDE_RAISE( ERR_NOT_CHANGE_MSG );
                    }
                }
            }

            /* BUG-36728 Check Constraint, Function-Based Index에서 Synonym을 사용할 수 없어야 합니다. */
            if ( sSynonymInfo.isSynonymName == ID_TRUE )
            {
                sParseTree->callSpecNode->lflag &= ~QTC_NODE_SP_SYNONYM_FUNC_MASK;
                sParseTree->callSpecNode->lflag |= QTC_NODE_SP_SYNONYM_FUNC_TRUE;
            }
            else
            {
                /* Nothing to do */
            }

            /* PSM을 호출하는 경우 */
            if( sIsPkg == ID_FALSE )
            {
                sParseTree->subprogramID = QS_PSM_SUBPROGRAM_ID;

                // synonym으로 참조되는 proc의 기록
                IDE_TEST( qsvProcStmts::makeProcSynonymList(
                              aStatement,
                              &sSynonymInfo,
                              *sUserName,
                              *sProcName,
                              sParseTree->procOID )
                          != IDE_SUCCESS );

                // search or make related object list
                IDE_TEST(qsvProcStmts::makeRelatedObjects(
                             aStatement,
                             sUserName,
                             sProcName,
                             & sSynonymInfo,
                             0,
                             QS_PROC )
                         != IDE_SUCCESS);

                // make procOID list
                IDE_TEST(qsxRelatedProc::prepareRelatedPlanTree(
                             aStatement,
                             sParseTree->procOID,
                             0,
                             &(aStatement->spvEnv->procPlanList))
                         != IDE_SUCCESS);

                IDE_TEST(qsxProc::getProcInfo( sParseTree->procOID,
                                               &sProcInfo)
                         != IDE_SUCCESS);

                // PROJ-2717 Internal procedure
                //   Library를 load하지 못했거나, function을 제대로 찾지 못한경우 에러
                if ( sProcInfo->planTree->procType == QS_INTERNAL_C )
                {
                    sCallSpec = sProcInfo->planTree->expCallSpec;

                    IDE_TEST_RAISE( sCallSpec->libraryNode == NULL,
                                    ERR_LIB_NOT_FOUND );

                    IDE_TEST_RAISE( sCallSpec->libraryNode->mHandle == PDL_SHLIB_INVALID_HANDLE,
                                    ERR_LIB_NOT_FOUND );

                    IDE_TEST_RAISE( sCallSpec->libraryNode->mFunctionPtr == NULL,
                                    ERR_ENTRY_FUNCTION_NOT_FOUND );

                    IDE_TEST_RAISE( sCallSpec->functionPtr == NULL,
                                    ERR_FUNCTION_NOT_FOUND );
                }

                /* BUG-45164 */
                IDE_TEST_RAISE( sProcInfo->isValid != ID_TRUE, err_object_invalid );

                /* BUG-48443 */
                if ( ( aStatement->mFlag & QC_STMT_PACKAGE_RECOMPILE_MASK )
                     == QC_STMT_PACKAGE_RECOMPILE_TRUE )
                {
                    sProcInfo->isValid = ID_FALSE;
                }

                sProcPlanTree = sProcInfo->planTree;
                sStmtText     = sProcPlanTree->stmtText; 

                // PROJ-2646 shard analyzer enhancement
                if ( ( sdi::isShardCoordinator( aStatement ) == ID_TRUE ) ||
                     ( sdi::detectShardMetaChange( aStatement ) == ID_TRUE ) )
                {
                    // BUG-48435
                    // Synonym으로 procedure를 호출할 때는 synonym의
                    // user name과 procedure name을 사용한다.
                    if ( sSynonymInfo.isSynonymName == ID_TRUE )
                    {
                        sUserNamePosition.stmtText = (SChar*)&sSynonymInfo.objectOwnerName;
                        sUserNamePosition.offset   = 0;
                        sUserNamePosition.size     = idlOS::strlen(sSynonymInfo.objectOwnerName);

                        sProcNamePosition.stmtText = (SChar*)&sSynonymInfo.objectName;
                        sProcNamePosition.offset   = 0;
                        sProcNamePosition.size     = idlOS::strlen(sSynonymInfo.objectName);

                        sUserName = &sUserNamePosition;
                        sProcName = &sProcNamePosition;
                    }

                    IDE_TEST( sdi::getProcedureInfoWithPlanTree(
                                  aStatement,
                                  sProcPlanTree->userID,
                                  *sUserName,
                                  *sProcName,
                                  sProcPlanTree,
                                  &(sParseTree->mShardObjInfo) )
                              != IDE_SUCCESS );

                    if ( sParseTree->mShardObjInfo != NULL )
                    {
                        aStatement->mFlag &= ~QC_STMT_SHARD_OBJ_MASK;
                        aStatement->mFlag |= QC_STMT_SHARD_OBJ_EXIST;
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

                // BUG-37655
                // pragma restrict reference
                if( aStatement->spvEnv->createPkg != NULL )
                {
                    for( sProcStmt  = sProcInfo->planTree->block->bodyStmts;
                         sProcStmt != NULL;
                         sProcStmt  = sProcStmt->next )
                    {
                        IDE_TEST( qsvPkgStmts::checkPragmaRestrictReferencesOption(
                                      aStatement,
                                      aStatement->spvEnv->currSubprogram,
                                      sProcStmt,
                                      aStatement->spvEnv->isPkgInitializeSection )
                                  != IDE_SUCCESS );
                    }
                }

                // PROJ-1075 typeset은 execute 할 수 없음.
                if( sProcInfo->planTree->objType == QS_TYPESET )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
                }
                else
                {
                    // Nothing to do.
                }

                /* BUG-39770
                   package를 참조하는 객체를 related object로 가지거나,
                   package를 참조하는 psm객체를 참조하면
                   parallel로 실행되면 안 된다. */
                if ( sProcInfo->planTree->referToPkg == ID_TRUE )
                {
                    sParseTree->callSpecNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
                    sParseTree->callSpecNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;
                }
                else
                {
                    // Nothing to do.
                }

                // BUG-47971 Package global variable의 분산 실행을 제한합니다.
                if ( sProcInfo->planTree->referToPkgVar == ID_TRUE )
                {
                    aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
                    aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;

                    if ( aStatement->spvEnv->createProc != NULL )
                    {
                        aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
                    }
                }
            }
            else /* Package의 subprogram을 호출하는 경우 */
            {
                // PROJ-1073 Package
                IDE_TEST( qsvPkgStmts::makePkgSynonymList(
                              aStatement,
                              & sSynonymInfo,
                              *sUserName,
                              *sPkgName,
                              sParseTree->procOID )
                          != IDE_SUCCESS );

                // search or make related object list
                IDE_TEST(qsvPkgStmts::makeRelatedObjects(
                             aStatement,
                             sUserName,
                             sPkgName,
                             & sSynonymInfo,
                             0,
                             QS_PKG )
                         != IDE_SUCCESS);

                // make procOID list
                IDE_TEST(qsxRelatedProc::prepareRelatedPlanTree(
                             aStatement,
                             sParseTree->procOID,
                             QS_PKG,
                             &(aStatement->spvEnv->procPlanList))
                         != IDE_SUCCESS);

                IDE_TEST( qsxPkg::getPkgInfo( sParseTree->procOID,
                                              &sPkgInfo )
                          != IDE_SUCCESS );

                /* 1) sPkgInfo가 NULL이면 오류
                   2) sSubprogramID가 0이 아니나 sPkgBodyInfo가 NULL이면
                   => qpERR_ABORT_QSV_INVALID_OR_MISSING_SUBPROGRAM
                   3) sSubprogramID가 0이고 sPkgBodyInfo가 NULL이면
                   => qpERR_ABORT_QSX_NOT_EXIST_SUBPROGRAM_OR_VARIABLE */

                // 1) spec의 package info를 가져오지 못함.
                IDE_TEST( sPkgInfo == NULL );

                /* BUG-45164 */
                IDE_TEST_RAISE( sPkgInfo->isValid != ID_TRUE, err_object_invalid );

                IDE_DASSERT( sPkgInfo->planTree != NULL );

                sStmtText    = sPkgInfo->planTree->stmtText;

                /* BUG-39194
                   qsExecParseTree->callSpecNode의 정보가
                   package_name( 또는 synonym_name ).record_name.field_name이면 못 찾음 */
                sUserName = &(sPkgInfo->planTree->userNamePos);
                sPkgName  = &(sPkgInfo->planTree->pkgNamePos);

                if ( QC_IS_NULL_NAME(sParseTree->callSpecNode->userName) != ID_TRUE )
                {
                    if ( qcmUser::getUserID( aStatement,
                                             sParseTree->callSpecNode->userName,
                                             & sTmpUserID ) == IDE_SUCCESS )
                    {
                        IDE_DASSERT( sTmpUserID == sParseTree->userID );
                        sProcName = &(sParseTree->callSpecNode->columnName);
                    }
                    else
                    {
                        IDE_CLEAR();
                        sProcName = &(sParseTree->callSpecNode->tableName);
                    }
                }
                else
                {
                    sProcName = &(sParseTree->callSpecNode->columnName);
                }

                IDE_TEST( qsvPkgStmts::getPkgSubprogramID( aStatement,
                                                           sParseTree->callSpecNode,
                                                           *sUserName,
                                                           *sPkgName,
                                                           *sProcName,
                                                           sPkgInfo,
                                                           &sSubprogramID )
                          != IDE_SUCCESS );

                // 2) subprogram id를 가져오지 못함.
                IDE_TEST_RAISE( sSubprogramID == QS_PSM_SUBPROGRAM_ID,
                                ERR_NOT_EXIST_SUBPROGRAM );

                sParseTree->subprogramID  = sSubprogramID;

                /* BUG-38720
                   package body 존재 여부 확인 후 body의 qsxPkgInfo를 가져온다. */
                IDE_TEST( qcmPkg::getPkgExistWithEmptyByNamePtr( aStatement,
                                                                 sParseTree->userID,
                                                                 (SChar*)( sPkgInfo->planTree->pkgNamePos.stmtText +
                                                                           sPkgInfo->planTree->pkgNamePos.offset ),
                                                                 sPkgInfo->planTree->pkgNamePos.size,
                                                                 QS_PKG_BODY,
                                                                 & sPkgBodyOID )
                          != IDE_SUCCESS);

                // BUG-41412 Variable인지 먼저 확인한다.
                if ( sSubprogramID == QS_PKG_VARIABLE_ID )
                {
                    /* BUG-39194
                       package variable일 때 qsExecParseTree->leftNode가 있어야 한다. */
                    if ( sParseTree->leftNode == NULL )
                    {
                        if ( QC_IS_NULL_NAME(sParseTree->callSpecNode->pkgName) != ID_TRUE )
                        {
                            sPos = sParseTree->callSpecNode->pkgName;
                        }
                        else
                        {
                            sPos = sParseTree->callSpecNode->columnName;
                        }

                        sqlInfo.setSourceInfo( aStatement,
                                               & sPos );

                        IDE_RAISE( ERR_PKG_VARIABLE_NOT_HAVE_RETURN );
                    }
                    else
                    {
                        /* PROJ-2533
                           Member Function인 경우 spFunctionCallModle 인 경우가 있다.
                           Member Function과 동일한 이름의 variable이 있을 수 있음 */
                        if ( ( sParseTree->callSpecNode->node.module == & qsfMCountModule ) ||
                             ( sParseTree->callSpecNode->node.module == & qsfMDeleteModule) ||
                             ( sParseTree->callSpecNode->node.module == & qsfMExistsModule) ||
                             ( sParseTree->callSpecNode->node.module == & qsfMFirstModule ) ||
                             ( sParseTree->callSpecNode->node.module == & qsfMLastModule  ) ||
                             ( sParseTree->callSpecNode->node.module == & qsfMNextModule  ) ||
                             ( sParseTree->callSpecNode->node.module == & qsfMPriorModule ) ||
                             ( ( sParseTree->callSpecNode->node.module == & qtc::spFunctionCallModule) &&
                               ( ( (sParseTree->callSpecNode->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
                                 QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST ) ) )
                        {
                            // BUG-41412 Member Function인 경우 columnModule로 바꾸지 않는다.
                            // Nothing to do.
                        }
                        else
                        {
                            sParseTree->callSpecNode->node.module = & qtc::columnModule;
                        }

                        sParseTree->callSpecNode->lflag &= ~QTC_NODE_PROC_FUNCTION_MASK;
                        sParseTree->callSpecNode->lflag |= QTC_NODE_PROC_FUNCTION_FALSE;

                        sParseTree->common.validate = qsv::validateExecPkgAssign;
                        sParseTree->common.execute  = qsx::executePkgAssign;

                        // BUG-47971 Package global variable의 분산 실행을 제한합니다.
                        aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
                        aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;

                        if ( aStatement->spvEnv->createProc != NULL )
                        {
                            aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
                        }

                        IDE_CONT( IS_PACKAGE_VARIABLE );
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if ( sPkgBodyOID != QS_EMPTY_OID )
                {
                    IDE_TEST( qsxPkg::getPkgObjectInfo( sPkgBodyOID,
                                                        &sPkgBodyObjectInfo )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sPkgBodyObjectInfo == NULL, ERR_NOT_EXIST_PKG_BODY_NAME );
                    IDE_TEST_RAISE( sPkgBodyObjectInfo->pkgInfo == NULL, ERR_NOT_EXIST_PKG_BODY_NAME );

                    sParseTree->pkgBodyOID = sPkgBodyObjectInfo->pkgInfo->pkgOID;
                }
                else
                {
                    /* BUG-39094
                       DDL일 때는 package body가 없어도 성공해야 한다. */
                    if ( (sTopStmtKind & QCI_STMT_MASK_MASK) != QCI_STMT_MASK_DDL )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sParseTree->callSpecNode->tableName );
                        IDE_RAISE( ERR_NOT_EXIST_PKG_BODY_NAME );
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                IDE_TEST( qsvPkgStmts::getPkgSubprogramPlanTree(
                              aStatement,
                              sPkgInfo->planTree,
                              sPkgInfo->objType,
                              sSubprogramID,
                              &sProcPlanTree )
                          != IDE_SUCCESS );

                IDE_DASSERT( sProcPlanTree != NULL );

                sProcPlanTree->procOID    = sParseTree->procOID;
                sProcPlanTree->pkgBodyOID = sParseTree->pkgBodyOID;

                // BUG-37655
                // pragma restrict reference
                if( ( aStatement->spvEnv->createPkg != NULL ) &&
                    ( sCurrPkgStmt != NULL ) )
                {
                    IDE_DASSERT( sCrtParseTree != NULL );

                    if ( sCrtParseTree->block != NULL )
                    {
                        if ( sCrtParseTree->block->isAutonomousTransBlock == ID_FALSE )
                        {
                            if( sCurrPkgStmt->flag != QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED )
                            {
                                IDE_RAISE( ERR_VIOLATES_PRAGMA );
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
                    else
                    {
                        // Nothing to do.
                    }
                }

                /* BUG-39770
                   package를 참조하는 객체를 related object로 가지거나,
                   package를 참조하는 psm객체를 참조하면
                   parallel로 실행되면 안 된다. */
                sParseTree->callSpecNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
                sParseTree->callSpecNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;

                // BUG-47971 Package global variable의 분산 실행을 제한합니다.
                if ( sProcPlanTree->referToPkgVar == ID_TRUE )
                {
                    aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
                    aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;
                
                    if ( aStatement->spvEnv->createProc != NULL )
                    {
                        aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
                    }
                }

                if ( (aStatement->spvEnv->createProc != NULL) &&
                     (aStatement->spvEnv->createPkg == NULL) )
                {
                    aStatement->spvEnv->createProc->referToPkg = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
            }

            /* PROJ-1090 Function-based Index */
            sParseTree->isDeterministic = sProcPlanTree->isDeterministic;

            // validation parameter
            IDE_TEST( validateArgumentsWithParser( aStatement,
                                                   sStmtText,
                                                   sParseTree->callSpecNode,
                                                   sProcPlanTree->paraDecls,
                                                   sProcPlanTree->paraDeclCount,
                                                   &sParseTree->refCurList,
                                                   sParseTree->isRootStmt )
                      != IDE_SUCCESS );
        }
        else // package body subprogram
        {
            sParseTree->paramModules = NULL;
            sParseTree->paramColumns = NULL;

            // subprogram search
            IDE_TEST( qsvPkgStmts::getMyPkgSubprogramID(
                          aStatement,
                          sParseTree->callSpecNode,
                          &sSubprogramType,
                          &sSubprogramID )
                      != IDE_SUCCESS );

            // 2) subprogram id를 가져오지 못함.
            IDE_TEST_RAISE( sSubprogramID == QS_PSM_SUBPROGRAM_ID,
                            ERR_NOT_EXIST_SUBPROGRAM );

            sParseTree->subprogramID  = sSubprogramID;

            IDE_TEST( qsvPkgStmts::getPkgSubprogramPlanTree(
                          aStatement,
                          aStatement->spvEnv->createPkg,
                          aStatement->spvEnv->createPkg->objType,
                          sSubprogramID,
                          &sProcPlanTree )
                      != IDE_SUCCESS );

            if( sProcPlanTree == NULL )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sParseTree->callSpecNode->columnName );

                IDE_RAISE( ERR_NOT_DEFINE_SUBPROGRAM );
            }

            sProcPlanTree->pkgBodyOID = sParseTree->pkgBodyOID;

            if( ( aStatement->spvEnv->createPkg != NULL ) &&
                ( sCurrPkgStmt != NULL ) )
            {
                IDE_DASSERT( sCrtParseTree != NULL );

                if ( sCrtParseTree->block != NULL )
                {
                    if ( sCrtParseTree->block->isAutonomousTransBlock == ID_FALSE )
                    {
                        if( sCurrPkgStmt->flag != QS_PRAGMA_RESTRICT_REFERENCES_UNDEFINED )
                        {
                            IDE_RAISE( ERR_VIOLATES_PRAGMA );
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
                else
                {
                    // Nothing to do.
                }
            }

            /* PROJ-1090 Function-based Index */
            sParseTree->isDeterministic = sProcPlanTree->isDeterministic;

            /* BUG-39770
               package를 참조하는 객체를 related object로 가지거나,
               package를 참조하는 psm객체를 참조하면
               parallel로 실행되면 안 된다. */
            sParseTree->callSpecNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
            sParseTree->callSpecNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;
        } /* sMyPkgSubprogram == ID_TRUE */
    }
    else  /* sRecursiveCall == ID_TRUE */
    {
        if( aStatement->spvEnv->createProc != NULL )
        { 
            if ( ( sParseTree->callSpecNode->lflag & QTC_NODE_SP_PARAM_DEFAULT_VALUE_MASK )
                 == QTC_NODE_SP_PARAM_DEFAULT_VALUE_FALSE ) 
            {
                sParseTree->isDeterministic = aStatement->spvEnv->createProc->isDeterministic;
            }
            else
            {
                // BUG-41228
                // parameter의 default에 자기 자신을 호출(recursive call) 할 수 없다.
                sqlInfo.setSourceInfo( aStatement,
                                       & sParseTree->callSpecNode->columnName);
                IDE_RAISE( ERR_NOT_EXIST_PROC_NAME );
            }
        }
        else
        {
            IDE_DASSERT( 0 );
        }
    }

    IDE_EXCEPTION_CONT( IS_PACKAGE_VARIABLE );

    // Shard Transformation 수행
    IDE_TEST( qmvShardTransform::doTransform( aStatement )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_IDENTIFIER );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INVALID_IDENTIFIER,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_PKG_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_DEFINE_SUBPROGRAM );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_UNDEFINED_SUBPROGRAM, 
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_PROC_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_PROC_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_SUBPROGRAM);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_NOT_EXIST_SUBPROGRAM_AND_VARIABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_PKG_BODY_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG_BODY,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_VIOLATES_PRAGMA)
    {
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_VIOLATES_ASSOCIATED_PRAGMA) );
    }
    IDE_EXCEPTION( ERR_NOT_CHANGE_MSG )
    {
        // Do not set error code
        // reference : qtc::changeNodeFromColumnToSP();
    }
    IDE_EXCEPTION( ERR_PKG_VARIABLE_NOT_HAVE_RETURN )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PACKAGE_VARIABLE_NOT_HAVE_RETURN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */ 
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION( ERR_LIB_NOT_FOUND )
    {
        ideSetErrorCode( idERR_ABORT_IDX_LIBRARY_NOT_FOUND,
                         idxLocalSock::mHomePath,
                         IDX_LIB_DEFAULT_DIR,
                         sCallSpec->libraryNode->mLibPath );
    }
    IDE_EXCEPTION( ERR_ENTRY_FUNCTION_NOT_FOUND )
    {
        ideSetErrorCode( idERR_ABORT_IDX_ENTRY_FUNCTION_NOT_FOUND );
    }
    IDE_EXCEPTION( ERR_FUNCTION_NOT_FOUND )
    {
        ideSetErrorCode( idERR_ABORT_IDX_FUNCTION_NOT_FOUND );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::validateExeProc(qcStatement * aStatement)
{
    qtcNode             * sProcReturnTypeNode;
    qsxProcInfo         * sProcInfo = NULL;
    qsxPkgInfo          * sPkgInfo = NULL;
    mtcColumn           * sExeReturnTypeColumn;
    mtcColumn           * sProcReturnTypeColumn;
    qsExecParseTree     * sExeParseTree;
    qsProcParseTree     * sCrtParseTree = NULL;
    qsPkgParseTree      * sCrtPkgParseTree;
    qcTemplate          * sTmplate;
    mtcColumn           * sColumn;
    mtcColumn           * sCallSpecColumn;
    idBool                sIsAllowSubq;
    qsxProcPlanList       sProcPlan;
    qcuSqlSourceInfo      sqlInfo;
    qsxPkgInfo          * sPkgBodyInfo = NULL;
    // BUG-37820
    qsPkgStmts          * sCurrStmt    = NULL;
    // fix BUG-42521
    UInt                  i = 0;
    qtcNode             * sArgument = NULL;
    qsVariableItems     * sParaDecls = NULL;
    qsVariableItems     * sProcParaDecls = NULL;

    IDU_FIT_POINT_FATAL( "qsv::validateExeProc::__FT__" );

    sExeParseTree = (qsExecParseTree *)aStatement->myPlan->parseTree;
    sCurrStmt     = aStatement->spvEnv->currSubprogram;

    if (sExeParseTree->procOID == QS_EMPTY_OID) // recursive call
    {
        if( sExeParseTree->subprogramID == QS_PSM_SUBPROGRAM_ID )
        {
            sCrtParseTree = aStatement->spvEnv->createProc;
        }
        else
        {
            // subprogram이 자신을 재귀호출했을 때
            if ( (aStatement->spvEnv->isPkgInitializeSection == ID_FALSE) &&
                 (qsvPkgStmts::checkIsRecursiveSubprogramCall( sCurrStmt,
                                                               sExeParseTree->subprogramID )
                  == ID_TRUE) )
            {
                sCrtParseTree  = aStatement->spvEnv->createProc;
            }
            else
            {
                sCrtPkgParseTree = aStatement->spvEnv->createPkg;

                IDE_TEST( qsvPkgStmts::getPkgSubprogramPlanTree(
                              aStatement,
                              sCrtPkgParseTree,
                              sCrtPkgParseTree->objType,
                              sExeParseTree->subprogramID,
                              &sCrtParseTree )
                          != IDE_SUCCESS );
            }

            if( sCrtParseTree == NULL )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sExeParseTree->callSpecNode->columnName );

                IDE_RAISE( ERR_NOT_DEFINE_SUBPROGRAM );
            }
        }

        // in case of function
        if (sCrtParseTree->returnTypeVar != NULL)
        {
            if( sExeParseTree->subprogramID == QS_PSM_SUBPROGRAM_ID )
            {
                //fix PROJ-1596
                sTmplate = QC_SHARED_TMPLATE(aStatement->spvEnv->createProc->common.stmt);
            }
            else
            {
                sTmplate = QC_SHARED_TMPLATE(aStatement->spvEnv->createPkg->common.stmt);

                if (sExeParseTree->leftNode == NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sExeParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
                }
            }

            sColumn   =
                & (sTmplate->tmplate
                   .rows[sCrtParseTree->returnTypeVar->variableTypeNode->node.table]
                   .columns[sCrtParseTree->returnTypeVar->variableTypeNode->node.column]);

            sTmplate = QC_SHARED_TMPLATE(aStatement);

            // PROJ-1075
            // set return 
            // node는 이미 만들어져 있는 상태임.
            // primitive data type이 아닌 경우에 어떻게 되는지 체크해 보아야 함.
            sCallSpecColumn = &( sTmplate->tmplate.rows[sExeParseTree->callSpecNode->node.table]
                                 .columns[sExeParseTree->callSpecNode->node.column]);

            mtc::copyColumn(sCallSpecColumn, sColumn);
        }

        // validation argument
        IDE_TEST( validateArgumentsWithoutParser( aStatement,
                                                  sExeParseTree->callSpecNode,
                                                  sCrtParseTree->paraDecls,
                                                  sCrtParseTree->paraDeclCount,
                                                  ID_FALSE /* no subquery is allowed */)
                  != IDE_SUCCESS );

        /* PROJ-2452 Caching for DETERMINISTIC Function */
        sExeParseTree->isCachable = sCrtParseTree->isCachable;
    }
    else    // NOT recursive call
    {
        if( sExeParseTree->subprogramID != QS_PSM_SUBPROGRAM_ID )
        {
            // get parameter declaration

            IDE_TEST( qsxPkg::getPkgInfo( sExeParseTree->procOID,
                                          &sPkgInfo )
                      != IDE_SUCCESS);

            // check grant
            IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aStatement,
                                                       sPkgInfo->planTree->userID,
                                                       sPkgInfo->privilegeCount,
                                                       sPkgInfo->granteeID,
                                                       ID_FALSE,
                                                       NULL,
                                                       NULL)
                      != IDE_SUCCESS );

            /* BUG-38720
               package body 존재 여부 확인 후 body의 qsxPkgInfo를 가져온다. */
            if ( sExeParseTree->pkgBodyOID != QS_EMPTY_OID )
            {
                IDE_TEST( qsxPkg::getPkgInfo( sExeParseTree->pkgBodyOID,
                                              &sPkgBodyInfo)
                          != IDE_SUCCESS );

                if ( sPkgBodyInfo->isValid == ID_TRUE )
                {
                    sProcPlan.pkgBodyOID         = sPkgBodyInfo->pkgOID;
                    sProcPlan.pkgBodyModifyCount = sPkgBodyInfo->modifyCount;
                }
                else
                {
                    sProcPlan.pkgBodyOID         = QS_EMPTY_OID;
                    sProcPlan.pkgBodyModifyCount = 0;
                }
            }
            else
            {
                /* BUG-39094
                   package body가 없어도 객체는 생성되어야 한다. */
                sProcPlan.pkgBodyOID         = QS_EMPTY_OID;
                sProcPlan.pkgBodyModifyCount = 0;
            }

            IDE_TEST( qsvPkgStmts::getPkgSubprogramPlanTree(
                          aStatement,
                          sPkgInfo->planTree,
                          sPkgInfo->objType,
                          sExeParseTree->subprogramID,
                          &sCrtParseTree )
                      != IDE_SUCCESS );

            IDE_DASSERT( sCrtParseTree != NULL );

            // BUG-36973
            sProcPlan.objectID           = sPkgInfo->pkgOID;
            sProcPlan.modifyCount        = sPkgInfo->modifyCount;
            sProcPlan.objectType         = sPkgInfo->objType;
            sProcPlan.planTree           = (qsProcParseTree *) sCrtParseTree;
            sProcPlan.next = NULL;

            IDE_TEST( qcgPlan::registerPlanPkg( aStatement, &sProcPlan ) != IDE_SUCCESS );

            // no subquery is allowed on
            // 1. execute proc
            // 2. execute ? := func
            // 3. procedure invocation in a procedure or a function.
            // and stmtKind of these cases are set to QCI_STMT_MASK_SP mask.
            if ( (aStatement->spvEnv->flag & QSV_ENV_SUBQUERY_ON_ARGU_ALLOW_MASK)
                 == QSV_ENV_SUBQUERY_ON_ARGU_ALLOW_TRUE )
            {
                sIsAllowSubq = ID_TRUE;
            }
            else
            {
                sIsAllowSubq = ID_FALSE;
            }

            // validation parameter
            IDE_TEST( validateArgumentsAfterParser(
                          aStatement,
                          sExeParseTree->callSpecNode,
                          sCrtParseTree->paraDecls,
                          sIsAllowSubq )
                      != IDE_SUCCESS );

            // set parameter module array pointer.
            sExeParseTree->paraDeclCount = sCrtParseTree->paraDeclCount;

            // fix BUG-42521
            sProcParaDecls = sCrtParseTree->paraDecls;

            IDE_TEST( qsvProcVar::copyParamModules(
                          aStatement,
                          sExeParseTree->paraDeclCount,
                          sCrtParseTree->paramColumns,
                          (mtdModule***) & (sExeParseTree->paramModules),
                          &(sExeParseTree->paramColumns) )
                      != IDE_SUCCESS );

            if ( sCrtParseTree->returnTypeVar != NULL )
            {
                sProcReturnTypeNode = sCrtParseTree->returnTypeVar->variableTypeNode;

                if (sExeParseTree->leftNode == NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sExeParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
                }

                sExeReturnTypeColumn =
                    QTC_STMT_COLUMN( aStatement, sExeParseTree->callSpecNode );

                sProcReturnTypeColumn =
                    QTC_TMPL_COLUMN( sPkgInfo->tmplate,  sProcReturnTypeNode );

                //fix BUG-17410
                IDE_TEST( qsvProcType::copyColumn( aStatement,
                                                   sProcReturnTypeColumn,
                                                   sExeReturnTypeColumn,
                                                   (mtdModule **)&sExeReturnTypeColumn->module )
                          != IDE_SUCCESS );
            }
            else
            {
                if (sExeParseTree->leftNode != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sExeParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_PROCEDURE_HAVE_RETURN );
                }
            }

            /* PROJ-2452 Caching for DETERMINISTIC Function */
            sExeParseTree->isCachable = sCrtParseTree->isCachable;
        }
        else
        {
            // get parameter declaration
            IDE_TEST( qsxProc::getProcInfo( sExeParseTree->procOID,
                                            &sProcInfo )
                      != IDE_SUCCESS);

            // check grant
            IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aStatement,
                                                       sProcInfo->planTree->userID,
                                                       sProcInfo->privilegeCount,
                                                       sProcInfo->granteeID,
                                                       ID_FALSE,
                                                       NULL,
                                                       NULL)
                      != IDE_SUCCESS );

            sProcPlan.objectID           = sProcInfo->procOID;
            sProcPlan.modifyCount        = sProcInfo->modifyCount;
            sProcPlan.objectType         = sProcInfo->planTree->objType; 
            sProcPlan.planTree           = sProcInfo->planTree;
            sProcPlan.pkgBodyOID         = QS_EMPTY_OID;
            sProcPlan.pkgBodyModifyCount = 0;
            sProcPlan.next = NULL;

            // environment의 기록
            IDE_TEST( qcgPlan::registerPlanProc(
                          aStatement,
                          & sProcPlan )
                      != IDE_SUCCESS );

            // no subquery is allowed on
            // 1. execute proc
            // 2. execute ? := func
            // 3. procedure invocation in a procedure or a function.
            // and stmtKind of these cases are set to QCI_STMT_MASK_SP mask.
            if ( (aStatement->spvEnv->flag & QSV_ENV_SUBQUERY_ON_ARGU_ALLOW_MASK)
                 == QSV_ENV_SUBQUERY_ON_ARGU_ALLOW_TRUE )
            {
                sIsAllowSubq = ID_TRUE;
            }
            else
            {
                sIsAllowSubq = ID_FALSE;
            }

            // validation parameter
            IDE_TEST( validateArgumentsAfterParser(
                          aStatement,
                          sExeParseTree->callSpecNode,
                          sProcInfo->planTree->paraDecls,
                          sIsAllowSubq )
                      != IDE_SUCCESS);

            // set parameter module array pointer.
            sExeParseTree->paraDeclCount = sProcInfo->planTree->paraDeclCount;

            // fix BUG-42521
            sProcParaDecls = sProcInfo->planTree->paraDecls;

            IDE_TEST( qsvProcVar::copyParamModules(
                          aStatement,
                          sExeParseTree->paraDeclCount,
                          sProcInfo->planTree->paramColumns,
                          (mtdModule***) & (sExeParseTree->paramModules),
                          &(sExeParseTree->paramColumns) )
                      != IDE_SUCCESS );

            if ( sProcInfo->planTree->returnTypeVar != NULL )
            {
                sProcReturnTypeNode = sProcInfo->planTree->returnTypeVar->variableTypeNode;

                if (sExeParseTree->leftNode == NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sExeParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
                }

                sExeReturnTypeColumn =
                    QTC_STMT_COLUMN( aStatement, sExeParseTree->callSpecNode );

                sProcReturnTypeColumn =
                    QTC_TMPL_COLUMN( sProcInfo->tmplate,  sProcReturnTypeNode );

                //fix BUG-17410
                IDE_TEST( qsvProcType::copyColumn( aStatement,
                                                   sProcReturnTypeColumn,
                                                   sExeReturnTypeColumn,
                                                   (mtdModule **)&sExeReturnTypeColumn->module )
                          != IDE_SUCCESS );
            }
            else
            {
                if (sExeParseTree->leftNode != NULL)
                {
                    sqlInfo.setSourceInfo(
                        aStatement,
                        & sExeParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_PROCEDURE_HAVE_RETURN );
                }
            }

            /* PROJ-2452 Caching for DETERMINISTIC Function */
            sExeParseTree->isCachable = sProcInfo->planTree->isCachable;
        }
    }

    // fix BUG-42521
    if ( aStatement->myPlan->sBindParam != NULL )
    {
        for ( i = 0, sParaDecls = sProcParaDecls, sArgument = (qtcNode *)sExeParseTree->callSpecNode->node.arguments;
              ( i < sExeParseTree->paraDeclCount ) && ( sArgument != NULL ) && ( sParaDecls != NULL );
              i++, sParaDecls = sParaDecls->next, sArgument = (qtcNode *)sArgument->node.next )
        {
            if ( ( (sArgument->node.lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_EXIST ) &&
                 ( sArgument->node.module == &(qtc::valueModule) ) )
            {
                switch ( ((qsVariables *)sParaDecls)->inOutType )
                {
                    case QS_IN:
                        aStatement->myPlan->sBindParam[sArgument->node.column].param.inoutType = CMP_DB_PARAM_INPUT; 
                        break;
                    case QS_OUT:
                        aStatement->myPlan->sBindParam[sArgument->node.column].param.inoutType = CMP_DB_PARAM_OUTPUT; 
                        break;
                    case QS_INOUT:
                        aStatement->myPlan->sBindParam[sArgument->node.column].param.inoutType = CMP_DB_PARAM_INPUT_OUTPUT; 
                        break;
                    default:
                        break;
                }
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_DEFINE_SUBPROGRAM );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_UNDEFINED_SUBPROGRAM,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }

    IDE_EXCEPTION(ERR_FUNCTION_NOT_HAVE_RETURN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_VALUE_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_PROCEDURE_HAVE_RETURN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_HAVE_RETURN_VALUE_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::validateExeFunc(qcStatement * aStatement)
{
    qsExecParseTree     * sParseTree;
    qsxProcInfo         * sProcInfo;
    qsProcParseTree     * sCrtParseTree    = NULL;
    qcuSqlSourceInfo      sqlInfo;
    qsPkgParseTree      * sCrtPkgParseTree = NULL;
    qsxPkgInfo          * sPkgInfo         = NULL;
    qsVariables         * sReturnTypeVar   = NULL;

    IDU_FIT_POINT_FATAL( "qsv::validateExeFunc::__FT__" );

    sParseTree = (qsExecParseTree *)(aStatement->myPlan->parseTree);

    IDE_TEST( validateExeProc( aStatement ) != IDE_SUCCESS );

    if (sParseTree->procOID != QS_EMPTY_OID)
    {
        if( sParseTree->subprogramID == QS_PSM_SUBPROGRAM_ID )
        {
            IDE_TEST( qsxProc::getProcInfo( sParseTree->procOID,
                                            & sProcInfo )
                      != IDE_SUCCESS );

            sCrtParseTree = sProcInfo->planTree;
        }
        else
        {
            /* BUG-39094
               package body가 없어도 객체는 생성되어야 한다. */
            IDE_TEST( qsxPkg::getPkgInfo( sParseTree->procOID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            IDE_TEST( qsvPkgStmts::getPkgSubprogramPlanTree(
                          aStatement,
                          sPkgInfo->planTree,
                          sPkgInfo->objType,
                          sParseTree->subprogramID,
                          &sCrtParseTree )
                      != IDE_SUCCESS );

            IDE_DASSERT( sCrtParseTree != NULL );
        }

        sReturnTypeVar = sCrtParseTree->returnTypeVar;

        // if called fuction is procedure, then error.
        if ( sReturnTypeVar == NULL )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sParseTree->callSpecNode->columnName);
            IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
        }

        // copy information
        IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                mtcColumn,
                                &( sParseTree->returnTypeColumn ) )
                  != IDE_SUCCESS );

        IDE_TEST( qsvProcType::copyColumn( aStatement,
                                           sCrtParseTree->returnTypeColumn,
                                           sParseTree->returnTypeColumn,
                                           (mtdModule **)&sParseTree->returnModule )
                  != IDE_SUCCESS );

        /* function의 return값이 column으로 사용될 때, return char(1)인지 return char로 생성되었는지 구별 */
        if ( ( sReturnTypeVar->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK)
             == QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT )
        {
            sParseTree->callSpecNode->lflag &= ~QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK;
            sParseTree->callSpecNode->lflag |= QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT;
        }
        else
        {
            // Nothing to do.
        }

        // BUG-32392
        // sParseTree->returnTypeColumn의 내용이 변경되었으므로 qtc::fixAfterValidation에서
        // 반드시 해당 column의 내용을 보정해주어야 한다.
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sParseTree->callSpecNode->node.table].lflag
            &= ~MTC_TUPLE_ROW_SKIP_MASK;
        QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sParseTree->callSpecNode->node.table].lflag
            |= MTC_TUPLE_ROW_SKIP_FALSE;

        /* PROJ-2452 Caching for DETERMINISTIC Function */
        sParseTree->isCachable = sCrtParseTree->isCachable;
    }
    else    // recursive call
    {
        if( sParseTree->subprogramID != QS_PSM_SUBPROGRAM_ID )
        {
            // subprogram이 자신을 재귀호출했을 때
            if ( (aStatement->spvEnv->isPkgInitializeSection == ID_FALSE) &&
                 (qsvPkgStmts::checkIsRecursiveSubprogramCall(
                     aStatement->spvEnv->currSubprogram,
                     sParseTree->subprogramID )
                  == ID_TRUE) )
            {
                if ( aStatement->spvEnv->createProc->returnTypeVar == NULL )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
                }

                /* PROJ-2452 Caching for DETERMINISTIC Function */
                sParseTree->isCachable = aStatement->spvEnv->createProc->isCachable;
            }
            else
            {
                sCrtPkgParseTree = aStatement->spvEnv->createPkg;

                IDE_TEST( qsvPkgStmts::getPkgSubprogramPlanTree(
                              aStatement,
                              sCrtPkgParseTree,
                              sCrtPkgParseTree->objType,
                              sParseTree->subprogramID,
                              &sCrtParseTree )
                          != IDE_SUCCESS );

                sReturnTypeVar = sCrtParseTree->returnTypeVar;

                if ( sReturnTypeVar == NULL )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sParseTree->callSpecNode->columnName);
                    IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
                }

                // copy information
                IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ),
                                        mtcColumn,
                                        &( sParseTree->returnTypeColumn ) )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sCrtParseTree->returnTypeColumn == NULL , err_return_value_invalid );

                IDE_TEST( qsvProcType::copyColumn( aStatement,
                                                   sCrtParseTree->returnTypeColumn,
                                                   sParseTree->returnTypeColumn,
                                                   (mtdModule **)&sParseTree->returnModule )
                          != IDE_SUCCESS );

                /* function의 return값이 column으로 사용될 때, return char(1)인지 return char로 생성되었는지 구별 */
                if ( (sReturnTypeVar->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK)
                     == QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT )
                {
                    sParseTree->callSpecNode->lflag &= ~QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK;
                    sParseTree->callSpecNode->lflag |= QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT;
                }
                else
                {
                    // Nothing to do.
                }

                // BUG-32392
                // sParseTree->returnTypeColumn의 내용이 변경되었으므로 qtc::fixAfterValidation에서
                // 반드시 해당 column의 내용을 보정해주어야 한다.
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sParseTree->callSpecNode->node.table].lflag
                    &= ~MTC_TUPLE_ROW_SKIP_MASK;
                QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sParseTree->callSpecNode->node.table].lflag
                    |= MTC_TUPLE_ROW_SKIP_FALSE;

                /* PROJ-2452 Caching for DETERMINISTIC Function */
                sParseTree->isCachable = sCrtParseTree->isCachable;
            }
        }
        else
        {
            if ( aStatement->spvEnv->createProc->returnTypeVar == NULL )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sParseTree->callSpecNode->columnName);
                IDE_RAISE( ERR_FUNCTION_NOT_HAVE_RETURN );
            }

            /* PROJ-2452 Caching for DETERMINISTIC Function */
            sParseTree->isCachable = aStatement->spvEnv->createProc->isCachable;
        }
    }

    // fix BUG-42521
    if ( aStatement->myPlan->sBindParam != NULL )
    {
        if ( ( (sParseTree->leftNode->node.lflag & MTC_NODE_BIND_MASK) == MTC_NODE_BIND_EXIST ) &&
             ( sParseTree->leftNode->node.module == &(qtc::valueModule) ) )
        {
            aStatement->myPlan->sBindParam[sParseTree->leftNode->node.column].param.inoutType = CMP_DB_PARAM_OUTPUT; 
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

    IDE_EXCEPTION(ERR_FUNCTION_NOT_HAVE_RETURN);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_HAVE_RETURN_VALUE_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(err_return_value_invalid)
    {
        IDE_SET(ideSetErrorCode( qpERR_ABORT_QSV_SUBPROGRAM_INVALID ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsv::checkHostVariable( qcStatement * aStatement )
{

    UShort  sBindCount = 0;
    UShort  sBindTuple = 0;

    IDU_FIT_POINT_FATAL( "qsv::checkHostVariable::__FT__" );

    sBindTuple = QC_SHARED_TMPLATE(aStatement)->tmplate.variableRow;

    // check host variable
    if( sBindTuple == ID_USHORT_MAX )
    {
        // Nothing to do.
    }
    else
    {
        sBindCount = QC_SHARED_TMPLATE(aStatement)->tmplate.rows[sBindTuple].columnCount;
    }

    if( sBindCount > 0 )
    {
        IDE_RAISE( ERR_NOT_ALLOWED_HOST_VAR );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_ALLOWED_HOST_VAR);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QCV_NOT_ALLOWED_HOSTVAR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::checkUserAndProcedure(
    qcStatement     * aStatement,
    qcNamePosition    aUserName,
    qcNamePosition    aProcName,
    UInt            * aUserID,
    qsOID           * aProcOID)
{
#define IDE_FN "qsv::checkUserAndProcedure"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsv::checkUserAndProcedure"));

    // check user name
    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        IDE_TEST(qcmUser::getUserID(aStatement, aUserName, aUserID)
                 != IDE_SUCCESS);
    }

    // check procedure name
    IDE_TEST(qcmProc::getProcExistWithEmptyByName(
                 aStatement,
                 *aUserID,
                 aProcName,
                 aProcOID)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

// PROJ-1073 Package
IDE_RC qsv::checkUserAndPkg( 
    qcStatement    * aStatement,
    qcNamePosition   aUserName,
    qcNamePosition   aPkgName,
    UInt           * aUserID,
    qsOID          * aSpecOID,
    qsOID          * aBodyOID)
{
    // check user name
    if (QC_IS_NULL_NAME(aUserName) == ID_TRUE)
    {
        *aUserID = QCG_GET_SESSION_USER_ID(aStatement);
    }
    else
    {
        IDE_TEST(qcmUser::getUserID(aStatement, aUserName, aUserID)
                 != IDE_SUCCESS);
    }

    // check package name
    IDE_TEST( qcmPkg::getPkgExistWithEmptyByName(
                  aStatement,
                  *aUserID,
                  aPkgName,
                  QS_PKG,
                  aSpecOID )
              != IDE_SUCCESS );

    // check package body
    IDE_TEST( qcmPkg::getPkgExistWithEmptyByName(
                  aStatement,
                  *aUserID,
                  aPkgName,
                  QS_PKG_BODY,
                  aBodyOID)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::checkRecursiveCall( qcStatement * aStatement,
                                idBool      * aRecursiveCall )
{
    qsExecParseTree     * sExeParseTree;
    qsProcParseTree     * sCrtParseTree;

    IDU_FIT_POINT_FATAL( "qsv::checkRecursiveCall::__FT__" );

    sExeParseTree = (qsExecParseTree *)aStatement->myPlan->parseTree;

    *aRecursiveCall = ID_FALSE;

    if ( aStatement->spvEnv->createProc != NULL )
    {
        sCrtParseTree = aStatement->spvEnv->createProc;

        // check user name
        if ( QC_IS_NULL_NAME(sExeParseTree->callSpecNode->tableName) == ID_TRUE )
        {
            sExeParseTree->userID = QCG_GET_SESSION_USER_ID(aStatement);
        }
        else
        {
            if ( qcmUser::getUserID( aStatement,
                                     sExeParseTree->callSpecNode->tableName,
                                     & (sExeParseTree->userID) )
                 != IDE_SUCCESS )
            {
                IDE_CLEAR();
                IDE_CONT( NOT_RECURSIVECALL );
            }
            else
            {
                // Nothing to do.
            }
        }

        // check procedure name
        if ( sExeParseTree->userID == sCrtParseTree->userID )
        {
            if ( QC_IS_NAME_MATCHED( sExeParseTree->callSpecNode->columnName, sCrtParseTree->procNamePos ) )
            {
                *aRecursiveCall         = ID_TRUE;
                sExeParseTree->procOID  = QS_EMPTY_OID ;
            }
        }
    }

    IDE_EXCEPTION_CONT( NOT_RECURSIVECALL );

    return IDE_SUCCESS;
}

IDE_RC qsv::validateArgumentsWithParser(
    qcStatement     * aStatement,
    SChar           * aStmtText,
    qtcNode         * aCallSpec,
    qsVariableItems * aParaDecls,
    UInt              aParaDeclCount,
    iduList         * aRefCurList,
    idBool            aIsRootStmt )
{
    qtcNode             * sPrevArgu;
    qtcNode             * sCurrArgu;
    qtcNode             * sNewArgu;
    qtcNode             * sNewVar[2];
    mtcColumn           * sNewColumn;
    qsVariableItems     * sParaDecl;
    qsVariables         * sParaVar;
    UChar               * sDefaultValueStr;
    qdDefaultParseTree  * sDefaultParseTree;
    qcStatement         * sDefaultStatement;
    qcuSqlSourceInfo      sqlInfo;
    iduListNode         * sNode;
    // BUG-37235
    iduVarMemString       sSqlBuffer;
    SInt                  sSize = 0;
    SInt                  i     = 0;
    // BUG-41243
    qsvArgPassInfo      * sArgPassArray = NULL;
    idBool                sAlreadyAdded = ID_FALSE;

    IDU_FIT_POINT_FATAL( "qsv::validateArgumentsWithParser::__FT__" );

    // BUG-41243 Name-based Argument Passing
    //           + Too many Argument Validation
    IDE_TEST( validateNamedArguments( aStatement,
                                      aCallSpec,
                                      aParaDecls,
                                      aParaDeclCount,
                                      & sArgPassArray )
              != IDE_SUCCESS );

    // set DEFAULT of parameter
    sPrevArgu = NULL;
    sCurrArgu = (qtcNode *)(aCallSpec->node.arguments);

    for (sParaDecl = aParaDecls, i = 0;
         sParaDecl != NULL;
         sParaDecl = sParaDecl->next, i++)
    {
        sParaVar = (qsVariables*)sParaDecl;
        sAlreadyAdded = ID_FALSE;

        if( aIsRootStmt == ID_TRUE )
        {
            // ref cursor type이고 out/inout인 경우임
            // 이 경우 resultset이 됨.
            if( (sParaVar->variableType == QS_REF_CURSOR_TYPE) &&
                ( (sParaVar->inOutType == QS_OUT) ||
                  (sParaVar->inOutType == QS_INOUT) ) )
            {
                // ref cursor type이므로 반드시 typeInfo는 있어야함.
                IDE_DASSERT( sParaVar->typeInfo != NULL );

                IDE_TEST( qtc::createColumn(
                              aStatement,
                              (mtdModule*)sParaVar->typeInfo->typeModule,
                              &sNewColumn,
                              0, NULL, NULL, 1 )
                          != IDE_SUCCESS );

                // 가상의 ref cursor proc var를 만든다.
                IDE_TEST( qtc::makeInternalProcVariable( aStatement,
                                                         sNewVar,
                                                         NULL,
                                                         sNewColumn,
                                                         QTC_PROC_VAR_OP_NEXT_COLUMN )
                          != IDE_SUCCESS );

                sNewArgu = sNewVar[0];

                sNewArgu->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                sNewArgu->lflag |= QTC_NODE_OUTBINDING_ENABLE;

                IDE_TEST( QC_QMP_MEM(aStatement)->alloc( ID_SIZEOF(iduListNode),
                                                         (void**)&sNode )
                          != IDE_SUCCESS );

                IDU_LIST_INIT_OBJ( sNode, (void*)sNewArgu );

                IDU_LIST_ADD_LAST( aRefCurList, sNode );

                // connect argument
                if (sPrevArgu == NULL)
                {
                    aCallSpec->node.arguments = (mtcNode *)sNewArgu;
                }
                else
                {
                    sPrevArgu->node.next = (mtcNode *)sNewArgu;
                }

                sNewArgu->node.next = (mtcNode*)sCurrArgu;

                sCurrArgu = sNewArgu;

                // BUG-41243
                if ( sArgPassArray[i].mType != QSV_ARG_NOT_PASSED )
                {
                    // PassArray의 Type을 정할 때는 refCursor를 고려할 수 없다.
                    // 따라서 refCursor가 생략되었어도 PassArray에 설정된 경우가 있으므로
                    // 이 경우, 현재 Type을 다음에 다시 읽도록 해야 한다.
                    i--;
                }
                else
                {
                    // PassArray에 설정하지 않아도 된다.
                    // Nothing to do.
                }
                
                // Argument를 삽입했으므로, Default Argument 부분을 지나친다.
                sAlreadyAdded = ID_TRUE;
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

        // BUG-41243 마지막 Default Parameter에 대해서만 처리할 수 없다.
        // ArgPassArray를 통해 Default Parameter를 처리한다.
        if ( ( sArgPassArray[i].mType == QSV_ARG_NOT_PASSED ) && 
             ( sAlreadyAdded == ID_FALSE ) )
        {
            if (sParaVar->defaultValueNode == NULL)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & aCallSpec->position );
                IDE_RAISE( ERR_INSUFFICIENT_ARGUMENT );
            }

            // make argument node
            IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM( aStatement ), qcStatement, &sDefaultStatement )
                      != IDE_SUCCESS );

            QC_SET_STATEMENT(sDefaultStatement, aStatement, NULL);

            /****************************************************************
             * BUG-37235
             * 아래 예제와 같이 package 내부 변수를 default value로 했을 때,
             * 실행 시 변수를 찾기 위해서 default statement 생성 시
             * package 이름을 추가한다.
             *
             * ex)
             * create or replace package pkg1 as
             * v1 integer;
             * procedure proc1(p1 integer default v1);
             * end;
             * /
             * create or replackage package body pkg1 as
             * procedure proc1(p1 integer default v1) as 
             * begin
             * println('test');
             * end;
             * end;
             * /
             *
             * exec pkg1.proc1;
             ****************************************************************/

            // default statement를 저장할 임시 buffer 생성
            IDE_TEST( iduVarMemStringInitialize( &sSqlBuffer, QC_QME_MEM(aStatement), QSV_SQL_BUFFER_SIZE )
                      != IDE_SUCCESS );

            IDE_TEST( iduVarMemStringAppendLength( &sSqlBuffer, "DEFAULT ", 8 )
                      != IDE_SUCCESS );

            /* BUG-42037 */
            if ( ((sParaVar->defaultValueNode->lflag & QTC_NODE_PKG_VARIABLE_MASK)
                  == QTC_NODE_PKG_VARIABLE_TRUE) &&
                 (QC_IS_NULL_NAME( sParaVar->defaultValueNode->userName ) == ID_TRUE) &&
                 (QC_IS_NAME_MATCHED( sParaVar->defaultValueNode->tableName,
                                      aCallSpec->tableName )
                  == ID_FALSE) )
            {
                IDE_TEST( iduVarMemStringAppendLength( &sSqlBuffer,
                                                       aStatement->myPlan->stmtText +
                                                       aCallSpec->tableName.offset,
                                                       aCallSpec->tableName.size )
                          != IDE_SUCCESS );

                IDE_TEST( iduVarMemStringAppendLength( &sSqlBuffer,
                                                       ".", 1 )
                          != IDE_SUCCESS );

                IDE_TEST( iduVarMemStringAppendLength( &sSqlBuffer,
                                                       aStmtText +
                                                       sParaVar->defaultValueNode->position.offset,
                                                       sParaVar->defaultValueNode->position.size )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( iduVarMemStringAppendLength( &sSqlBuffer,
                                                       aStmtText +
                                                       sParaVar->defaultValueNode->position.offset,
                                                       sParaVar->defaultValueNode->position.size )
                          != IDE_SUCCESS );
            }

            sSize = iduVarMemStringGetLength( &sSqlBuffer );

            IDE_TEST( QC_QMP_MEM( aStatement )->alloc( sSize + 1,
                                                       (void**)&sDefaultValueStr)
                      != IDE_SUCCESS);

            IDE_TEST( iduVarMemStringExtractString( &sSqlBuffer, (SChar *)sDefaultValueStr, sSize )
                      != IDE_SUCCESS );

            IDE_TEST( iduVarMemStringFinalize( &sSqlBuffer )
                      != IDE_SUCCESS );

            sDefaultValueStr[sSize] = '\0';

            sDefaultStatement->myPlan->stmtText = (SChar*)sDefaultValueStr;
            sDefaultStatement->myPlan->stmtTextLen =
                idlOS::strlen((SChar*)sDefaultValueStr);

            IDE_TEST( qcpManager::parseIt( sDefaultStatement ) != IDE_SUCCESS );

            sDefaultParseTree =
                (qdDefaultParseTree*) sDefaultStatement->myPlan->parseTree;

            // set default expression
            sCurrArgu = sDefaultParseTree->defaultValue;

            // connect argument
            if (sPrevArgu == NULL)
            {
                sCurrArgu->node.next      = (mtcNode *)aCallSpec->node.arguments;
                aCallSpec->node.arguments = (mtcNode *)sCurrArgu;
            }
            else
            {
                sCurrArgu->node.next = sPrevArgu->node.next;
                sPrevArgu->node.next = (mtcNode *)sCurrArgu;
            }

            // mtcNode의 MTC_NODE_ARGUMENT_COUNT 값을 증가시킨다.
            aCallSpec->node.lflag++;
        }

        // next argument
        sPrevArgu = sCurrArgu;
        sCurrArgu = (qtcNode *)(sCurrArgu->node.next);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INSUFFICIENT_ARGUMENT);
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

IDE_RC qsv::validateArgumentsAfterParser(
    qcStatement     * aStatement,
    qtcNode         * aCallSpec,
    qsVariableItems * aParaDecls,
    idBool            aAllowSubquery )
{
    qtcNode             * sCurrArgu;
    qsVariableItems     * sParaDecl;
    qsVariables         * sParaVar;
    idBool                sFind;
    qsVariables         * sArrayVar = NULL;
    qcuSqlSourceInfo      sqlInfo;
    UShort                sTable;
    UShort                sColumn;

    IDU_FIT_POINT_FATAL( "qsv::validateArgumentsAfterParser::__FT__" );

    for (sParaDecl = aParaDecls,
             sCurrArgu = (qtcNode *)(aCallSpec->node.arguments);
         sParaDecl != NULL;
         sParaDecl = sParaDecl->next,
             sCurrArgu = (qtcNode *)(sCurrArgu->node.next))
    {
        sParaVar = (qsVariables*)sParaDecl;

        if (sParaVar->inOutType == QS_IN)
        {
            if ( ( aStatement->spvEnv->flag & QSV_ENV_ESTIMATE_ARGU_MASK )
                 == QSV_ENV_ESTIMATE_ARGU_FALSE )
            {
                IDE_TEST( qtc::estimate(
                              sCurrArgu,
                              QC_SHARED_TMPLATE( aStatement ),
                              aStatement,
                              NULL,
                              NULL,
                              NULL )
                          != IDE_SUCCESS );
            }

            if ( aAllowSubquery != ID_TRUE )
            {
                IDE_TEST_RAISE( qsv::checkNoSubquery(
                                    aStatement,
                                    sCurrArgu,
                                    & sqlInfo ) != IDE_SUCCESS,
                                ERR_SUBQ_NOT_ALLOWED_ON_ARGUMENT );
            }
        }
        else // QS_OUT or QS_INOUT
        {
            if ( ( aStatement->spvEnv->flag & QSV_ENV_ESTIMATE_ARGU_MASK )
                 == QSV_ENV_ESTIMATE_ARGU_FALSE )
            {
                IDE_TEST( qtc::estimate(
                              sCurrArgu,
                              QC_SHARED_TMPLATE( aStatement ),
                              aStatement,
                              NULL,
                              NULL,
                              NULL )
                          != IDE_SUCCESS );
            }

            if (sCurrArgu->node.module == &(qtc::columnModule))
            {
                if ( aStatement->spvEnv->createProc != NULL)
                {
                    /* BUG-38644
                       estimate에서 indirect calculate를 위한
                       정보가 있는 table/column 정보 보존 */
                    sTable  = sCurrArgu->node.table;
                    sColumn = sCurrArgu->node.column;

                    // search in variable or parameter list
                    IDE_TEST( qsvProcVar::searchVarAndPara(
                                  aStatement,
                                  sCurrArgu,
                                  ID_TRUE, // for OUTPUT
                                  & sFind,
                                  & sArrayVar )
                              != IDE_SUCCESS );

                    if ( sFind == ID_FALSE )
                    {
                        IDE_TEST( qsvProcVar::searchVariableFromPkg(
                                      aStatement,
                                      sCurrArgu,
                                      & sFind,
                                      & sArrayVar )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    sCurrArgu->node.table  = sTable;
                    sCurrArgu->node.column = sColumn;
                }
                else
                {
                    // PROJ-1386 Dynamic-SQL
                    // ref cursor를 위해 internal procedure variable을
                    // 만들었음.
                    if( ( sCurrArgu->lflag & QTC_NODE_INTERNAL_PROC_VAR_MASK )
                        == QTC_NODE_INTERNAL_PROC_VAR_EXIST )
                    {
                        sFind = ID_TRUE;
                    }
                    else
                    {
                        /* - BUG-38644
                           estimate에서 indirect calculate를 위한 정보가 있는 table/column 정보 보존
                           - BUG-42311 argument가 package 변수일 때 찾을 수 있어야 한다. */
                        sTable  = sCurrArgu->node.table;
                        sColumn = sCurrArgu->node.column;

                        /* BUG-44941
                           package의 initialize section에서 fclose함수의 argument가 동일한 package 변수이면 찾지 못 합니다. */
                        // search in variable or parameter list
                        IDE_TEST(qsvProcVar::searchVarAndPara(
                                     aStatement,
                                     sCurrArgu,
                                     ID_TRUE, // for OUTPUT
                                     &sFind,
                                     &sArrayVar)
                                 != IDE_SUCCESS);

                        if ( sFind != ID_TRUE )
                        {
                            /* search a package variable */
                            IDE_TEST( qsvProcVar::searchVariableFromPkg(
                                          aStatement,
                                          sCurrArgu,
                                          &sFind,
                                          &sArrayVar )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nohting to do.
                        }

                        sCurrArgu->node.table  = sTable;
                        sCurrArgu->node.column = sColumn;
                    }
                }

                if (sFind == ID_TRUE)
                {
                    if ( ( sCurrArgu->lflag & QTC_NODE_OUTBINDING_MASK )
                         == QTC_NODE_OUTBINDING_DISABLE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrArgu->position );
                        IDE_RAISE( ERR_INVALID_OUT_ARGUMENT );
                    }
                    else
                    {
                        // Nothing to do.
                    }

                    // out parameter인 경우 lvalue_enable로 세팅.
                    if( sParaVar->inOutType == QS_OUT )
                    {
                        sCurrArgu->lflag |= QTC_NODE_LVALUE_ENABLE;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrArgu->position );
                    IDE_RAISE( ERR_NOT_EXIST_VARIABLE );
                }
            }
            else if (sCurrArgu->node.module == &(qtc::valueModule))
            {
                // constant value or host variable
                if ( ( sCurrArgu->node.lflag & MTC_NODE_BIND_MASK )
                     == MTC_NODE_BIND_ABSENT ) // constant
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrArgu->position );
                    IDE_RAISE( ERR_INVALID_OUT_ARGUMENT );
                }
            }
            else
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrArgu->position );
                IDE_RAISE( ERR_INVALID_OUT_ARGUMENT );
            }
        }
    }

    if (sParaDecl != NULL)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aCallSpec->position );
        IDE_RAISE( ERR_INSUFFICIENT_ARGUMENT );
    }

    if (sCurrArgu != NULL)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aCallSpec->position );
        IDE_RAISE( ERR_TOO_MANY_ARGUMENT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SUBQ_NOT_ALLOWED_ON_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_ALLOWED_SUBQUERY_SQLTEXT,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INSUFFICIENT_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INSUFFICIENT_ARGUEMNT_SQLTEXT,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_TOO_MANY_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_TOO_MANY_ARGUEMNT_SQLTEXT,
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
    IDE_EXCEPTION(ERR_NOT_EXIST_VARIABLE)
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

IDE_RC qsv::validateArgumentsWithoutParser(
    qcStatement     * aStatement,
    qtcNode         * aCallSpec,
    qsVariableItems * aParaDecls,
    UInt              aParaDeclCount,
    idBool            aAllowSubquery )
{
    qtcNode             * sPrevArgu;
    qtcNode             * sCurrArgu;
    qsVariableItems     * sParaDecl;
    qsVariables         * sParaVar;
    idBool                sFind;
    qsVariables         * sArrayVar = NULL;
    qcuSqlSourceInfo      sqlInfo;
    UShort                sTable;
    UShort                sColumn;
    UInt                  i = 0;
    // BUG-41243
    qsvArgPassInfo      * sArgPassArray = NULL;

    // BUG-41243 Name-based Argument Passing
    //           + Too many Argument Validation
    IDE_TEST( validateNamedArguments( aStatement,
                                      aCallSpec,
                                      aParaDecls,
                                      aParaDeclCount,
                                      & sArgPassArray )
              != IDE_SUCCESS );

    // set DEFAULT of parameter
    sPrevArgu = NULL;
    sCurrArgu = (qtcNode *)(aCallSpec->node.arguments);

    for (sParaDecl = aParaDecls, i = 0;
         sParaDecl != NULL;
         sParaDecl = sParaDecl->next, i++)
    {
        sParaVar = (qsVariables*)sParaDecl;

        // BUG-41243 마지막 Default Parameter에 대해서만 처리할 수 없다.
        // ArgPassArray를 통해 Default Parameter를 처리한다.
        if ( sArgPassArray[i].mType == QSV_ARG_NOT_PASSED )
        {
            if (sParaVar->defaultValueNode == NULL)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & aCallSpec->position );
                IDE_RAISE( ERR_INSUFFICIENT_ARGUMENT );
            }

            // make argument node
            IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(aStatement), qtcNode, &sCurrArgu)
                     != IDE_SUCCESS);

            // set default expression
            idlOS::memcpy(sCurrArgu,
                          sParaVar->defaultValueNode,
                          ID_SIZEOF(qtcNode));
            sCurrArgu->node.next = NULL;

            // connect argument
            if (sPrevArgu == NULL)
            {
                sCurrArgu->node.next      = (mtcNode *)aCallSpec->node.arguments;
                aCallSpec->node.arguments = (mtcNode *)sCurrArgu;
            }
            else
            {
                sCurrArgu->node.next = sPrevArgu->node.next;
                sPrevArgu->node.next = (mtcNode *)sCurrArgu;
            }

            // mtcNode의 MTC_NODE_ARGUMENT_COUNT 값을 증가시킨다.
            aCallSpec->node.lflag++;
        }

        // next argument
        sPrevArgu = sCurrArgu;
        sCurrArgu = (qtcNode *)(sCurrArgu->node.next);
    }

    for (sParaDecl = aParaDecls,
             sCurrArgu = (qtcNode *)(aCallSpec->node.arguments);
         sParaDecl != NULL;
         sParaDecl = sParaDecl->next,
             sCurrArgu = (qtcNode *)(sCurrArgu->node.next))
    {
        sParaVar = (qsVariables*)sParaDecl;

        if (sParaVar->inOutType == QS_IN)
        {
            if ( ( aStatement->spvEnv->flag & QSV_ENV_ESTIMATE_ARGU_MASK )
                 == QSV_ENV_ESTIMATE_ARGU_FALSE )
            {
                IDE_TEST(qtc::estimate(
                             sCurrArgu,
                             QC_SHARED_TMPLATE(aStatement),
                             aStatement,
                             NULL,
                             NULL,
                             NULL )
                         != IDE_SUCCESS);
            }

            if ( aAllowSubquery != ID_TRUE )
            {
                IDE_TEST_RAISE( qsv::checkNoSubquery(
                                    aStatement,
                                    sCurrArgu,
                                    & sqlInfo ) != IDE_SUCCESS,
                                ERR_SUBQ_NOT_ALLOWED_ON_ARGUMENT );
            }
        }
        else // QS_OUT or QS_INOUT
        {
            if( ((qsExecParseTree *)(aStatement->myPlan->parseTree))->subprogramID
                != QS_PSM_SUBPROGRAM_ID )
            {
                if ( ( aStatement->spvEnv->flag & QSV_ENV_ESTIMATE_ARGU_MASK )
                     == QSV_ENV_ESTIMATE_ARGU_FALSE )
                {
                    IDE_TEST(qtc::estimate(
                                 sCurrArgu,
                                 QC_SHARED_TMPLATE(aStatement),
                                 aStatement,
                                 NULL,
                                 NULL,
                                 NULL )
                             != IDE_SUCCESS);
                }
            }
            else
            {
                // Nothing to do.
            }

            if (sCurrArgu->node.module == &(qtc::columnModule))
            {
                if ( aStatement->spvEnv->createProc != NULL)
                {
                    /* BUG-38644
                       estimate에서 indirect calculate를 위한
                       정보가 있는 table/column 정보 보존 */
                    sTable  = sCurrArgu->node.table;
                    sColumn = sCurrArgu->node.column;

                    // search in variable or parameter list
                    IDE_TEST(qsvProcVar::searchVarAndPara(
                                 aStatement,
                                 sCurrArgu,
                                 ID_TRUE, // for OUTPUT
                                 &sFind,
                                 &sArrayVar)
                             != IDE_SUCCESS);

                    if ( sFind == ID_FALSE )
                    {
                        IDE_TEST( qsvProcVar::searchVariableFromPkg(
                                      aStatement,
                                      sCurrArgu,
                                      &sFind,
                                      &sArrayVar )
                                  != IDE_SUCCESS )
                            }
                    else
                    {
                        // Nothing to do.
                    }

                    sCurrArgu->node.table  = sTable;
                    sCurrArgu->node.column = sColumn;
                }
                else
                {
                    sFind = ID_FALSE;
                }

                if (sFind == ID_TRUE)
                {
                    if ( ( sCurrArgu->lflag & QTC_NODE_OUTBINDING_MASK )
                         == QTC_NODE_OUTBINDING_DISABLE )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               & sCurrArgu->position );
                        IDE_RAISE( ERR_INVALID_OUT_ARGUMENT );
                    }

                    /*
                      if (sRecordVar != NULL)
                      {
                      sqlInfo.setSourceInfo( aStatement,
                      & sCurrArgu->position );
                      IDE_RAISE(ERR_INVALID_OUT_ARGUMENT);
                      }
                    */
                }
                else
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrArgu->position );
                    IDE_RAISE( ERR_NOT_EXIST_VARIABLE );
                }
            }
            else if (sCurrArgu->node.module == &(qtc::valueModule))
            {
                // constant value or host variable
                if ( ( sCurrArgu->node.lflag & MTC_NODE_BIND_MASK )
                     == MTC_NODE_BIND_ABSENT ) // constant
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sCurrArgu->position );
                    IDE_RAISE( ERR_INVALID_OUT_ARGUMENT );
                }
            }
            else
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sCurrArgu->position );
                IDE_RAISE( ERR_INVALID_OUT_ARGUMENT );
            }
        }
    }

    if (sParaDecl != NULL)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aCallSpec->position );
        IDE_RAISE( ERR_INSUFFICIENT_ARGUMENT );
    }

    if (sCurrArgu != NULL)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aCallSpec->position );
        IDE_RAISE( ERR_TOO_MANY_ARGUMENT );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SUBQ_NOT_ALLOWED_ON_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_ALLOWED_SUBQUERY_SQLTEXT,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INSUFFICIENT_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INSUFFICIENT_ARGUEMNT_SQLTEXT,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_TOO_MANY_ARGUMENT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_TOO_MANY_ARGUEMNT_SQLTEXT,
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
    IDE_EXCEPTION(ERR_NOT_EXIST_VARIABLE)
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

IDE_RC qsv::checkNoSubquery(
    qcStatement       * aStatement,
    qtcNode           * aNode,
    qcuSqlSourceInfo  * aSourceInfo)
{
    IDU_FIT_POINT_FATAL( "qsv::checkNoSubquery::__FT__" );

    if ( ( aNode->lflag & QTC_NODE_SUBQUERY_MASK )
         == QTC_NODE_SUBQUERY_EXIST )
    {
        aSourceInfo->setSourceInfo( aStatement,
                                    & aNode->position );
        IDE_RAISE( ERR_PASS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_PASS);
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

////////////////////////////////////////////////////////
// To Fix BUG-11921(A3) 11920(A4)
////////////////////////////////////////////////////////
//
// aNode는 스토어드 펑션의 call spec 노드이다.
// call spec 노드는 노드 자신은 펑션의 리턴값을 가리키고
// arguments는 펑션의 인자들을 가리킨다.
//
// 이 함수가 호출되고 나면 aNode->subquery에
// qcStatement가 설정되는데, 이 속에 펑션 실행을 위한
// qsExecParseTree 가 설정된다.
//
// 그리고 aNode->arguments에 사용자가 지정하지 않은
// 기본 인자들을 생성해준다.
// 이는 qtc::estimateInternal에서
// 펑션의 argument들에 대해서 recursive하게
// qtc::estimateInternal 을 호출할때 쓰인다.
IDE_RC qsv::createExecParseTreeOnCallSpecNode(
    qtcNode     * aCallSpecNode,
    mtcCallBack * aCallBack )

{
    qtcCallBackInfo     * sCallBackInfo; /* BUG-45994 - 컴파일러 최적화 회피 */
    qcStatement         * sStatement;    /* BUG-45994 */
    qsExecParseTree     * sParseTree = NULL;
    volatile UInt         sStage;
    qcNamePosition        sPosition;
    qcuSqlSourceInfo      sqlInfo;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qsv::createExecParseTreeOnCallSpecNode::__FT__" );

    sCallBackInfo = (qtcCallBackInfo*)((mtcCallBack*)aCallBack)->info;
    sStage        = 0; /* BUG-45994 - 컴파일러 최적화 회피 */

    if( sCallBackInfo->statement == NULL )
    {
        // PROJ-1502 PARTITIONED DISK TABLE
        // Nothing to do.
        // 이미 만들었음.
    }
    else
    {
        // make execute function parse tree
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(sCallBackInfo->statement),
                              qsExecParseTree,
                              &sParseTree)
                 != IDE_SUCCESS);

        IDE_TEST_RAISE(sParseTree == NULL, ERR_MEMORY_SHORTAGE);

        SET_EMPTY_POSITION(sPosition);
        QC_SET_INIT_PARSE_TREE(sParseTree, sPosition);

        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(sCallBackInfo->statement),
                              qtcNode,
                              &(sParseTree->leftNode))
                 != IDE_SUCCESS);

        // left 초기화
        QTC_NODE_INIT( sParseTree->leftNode );

        sParseTree->isRootStmt = ID_FALSE;
        sParseTree->mShardObjInfo = NULL;

        sParseTree->callSpecNode = aCallSpecNode;
        sParseTree->paramModules = NULL;
        sParseTree->common.parse    = qsv::parseExeProc;
        sParseTree->common.validate = qsv::validateExeFunc;
        sParseTree->common.optimize = qso::optimizeNone;
        sParseTree->common.execute  = qsx::executeProcOrFunc;

        sParseTree->isCachable = ID_FALSE;  // PROJ-2452

        // make execute function qcStatement
        IDE_TEST(STRUCT_ALLOC(QC_QMP_MEM(sCallBackInfo->statement),
                              qcStatement,
                              &sStatement)
                 != IDE_SUCCESS);

        QC_SET_STATEMENT(sStatement, sCallBackInfo->statement, sParseTree);
        sStatement->myPlan->parseTree->stmt     = sStatement;
        sStatement->myPlan->parseTree->stmtKind = QCI_STMT_EXEC_FUNC;

        // set member of qcStatement
        sStatement->myPlan->planEnv = sCallBackInfo->statement->myPlan->planEnv;
        sStatement->spvEnv = sCallBackInfo->statement->spvEnv;
        sStatement->myPlan->sBindParam = sCallBackInfo->statement->myPlan->sBindParam;
        sStage = 1;

        /* BUG-45994 */
        IDU_FIT_POINT_FATAL( "qsv::createExecParseTreeOnCallSpecNode::__FT__::STAGE1" );

        // for accessing procOID
        aCallSpecNode->subquery = sStatement;

        IDE_TEST_RAISE( ( aCallSpecNode->node.lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                        MTC_NODE_QUANTIFIER_TRUE,
                        ERR_NOT_AGGREGATION );

        sStatement->spvEnv->flag &= ~QSV_ENV_ESTIMATE_ARGU_MASK;
        sStatement->spvEnv->flag |= QSV_ENV_ESTIMATE_ARGU_TRUE;

        if( ( qci::getStartupPhase() != QCI_STARTUP_SERVICE ) &&
            ( qcg::isInitializedMetaCaches() != ID_TRUE ) )
        {
            sqlInfo.setSourceInfo( sStatement,
                                   & aCallSpecNode->columnName );
            IDE_RAISE( err_invalid_psm_not_in_service_phase );
        }

        IDE_TEST(sStatement->myPlan->parseTree->parse( sStatement ) != IDE_SUCCESS);

        // BUG-43061
        // Prevent abnormal server shutdown when validates a node like 'pkg.res2.c1.c1';
        if ( (sStatement->myPlan->parseTree->validate == qsv::validateExecPkgAssign) &&
             (sParseTree->callSpecNode->node.module == &qtc::columnModule) )
        {
            sqlInfo.setSourceInfo( sStatement,
                                   & aCallSpecNode->position );
            IDE_RAISE( ERR_INVALID_IDENTIFIER );
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST(sStatement->myPlan->parseTree->validate( sStatement ) != IDE_SUCCESS);

        sStatement->spvEnv->flag &= ~QSV_ENV_ESTIMATE_ARGU_MASK;
        sStatement->spvEnv->flag |= QSV_ENV_ESTIMATE_ARGU_FALSE;

        /* PROJ-1090 Function-based Index */
        if ( sParseTree->isDeterministic == ID_TRUE )
        {
            aCallSpecNode->lflag &= ~QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK;
            aCallSpecNode->lflag |= QTC_NODE_PROC_FUNC_DETERMINISTIC_TRUE;
        }
        else
        {
            aCallSpecNode->lflag &= ~QTC_NODE_PROC_FUNC_DETERMINISTIC_MASK;
            aCallSpecNode->lflag |= QTC_NODE_PROC_FUNC_DETERMINISTIC_FALSE;

            /* PROJ-2462 Result Cache */
            if ( sCallBackInfo->querySet != NULL )
            {
                sCallBackInfo->querySet->lflag &= ~QMV_QUERYSET_RESULT_CACHE_INVALID_MASK;
                sCallBackInfo->querySet->lflag |= QMV_QUERYSET_RESULT_CACHE_INVALID_TRUE;
            }
            else
            {
                /* Nothing to do */
            }
        }
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_psm_not_in_service_phase)
    {
        (void)sqlInfo.init(sStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_PSM_NOT_IN_SERVICE_PHASE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INVALID_IDENTIFIER );
    {
        (void)sqlInfo.init( sStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INVALID_IDENTIFIER,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION( ERR_MEMORY_SHORTAGE );
    IDE_SET(ideSetErrorCode(mtERR_FATAL_MEMORY_SHORTAGE));
    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    switch (sStage )
    {
        // sStatement->parseTree->validate may have
        // latched and added some procedures to sStatement->spvEnv,
        // which need to be unlocked in MM layer.
        case 1:

            // BUG-41758 replace pkg result worng
            // validate 과정에서 에러가 발생했을때 다시 복구해준다.
            sStatement->spvEnv->flag &= ~QSV_ENV_ESTIMATE_ARGU_MASK;
            sStatement->spvEnv->flag |= QSV_ENV_ESTIMATE_ARGU_FALSE;

            sCallBackInfo->statement->myPlan->planEnv =
                sStatement->myPlan->planEnv;
            sCallBackInfo->statement->spvEnv = sStatement->spvEnv;
            sCallBackInfo->statement->myPlan->sBindParam = sStatement->myPlan->sBindParam;
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

UShort
qsv::getResultSetCount( qcStatement * aStatement )
{
    qsExecParseTree * sParseTree;
    UShort            sResultSetCount = 0;
    iduListNode     * sIterator = NULL;

    sParseTree = (qsExecParseTree*)aStatement->myPlan->parseTree;

    IDU_LIST_ITERATE( &sParseTree->refCurList, sIterator )
    {
        sResultSetCount++;
    }

    return sResultSetCount;
}

// PROJ-1685
/* param property rule
   INDICATOR    LENGTH    MAXLEN
   ------------------------------------------
   IN           O            O         X
   IN OUT/OUT   O            O         O
   RETURN       O            X         X
*/
IDE_RC qsv::validateExtproc( qcStatement * aStatement, qsProcParseTree * aParseTree )
{
    UInt                 sUserID;
    qcuSqlSourceInfo     sqlInfo;
    qcmSynonymInfo       sSynonymInfo;
    qcmLibraryInfo       sLibraryInfo;
    qsProcParseTree    * sParseTree;
    idBool               sExist = ID_FALSE;
    qsCallSpec         * sCallSpec;
    qsCallSpecParam    * sExpParam;
    qsCallSpecParam    * sExpParam2;
    qsVariableItems    * sProcParam;
    idBool               sFound;
    mtcColumn          * sColumn;
    SInt                 sFileSpecSize;
    UInt                 i;

    IDU_FIT_POINT_FATAL( "qsv::validateExtproc::__FT__" );

    sParseTree = aParseTree;

    sCallSpec = sParseTree->expCallSpec;

    IDE_TEST( qcmSynonym::resolveLibrary( aStatement,
                                          sCallSpec->userNamePos,
                                          sCallSpec->libraryNamePos,
                                          & sLibraryInfo,
                                          & sUserID,
                                          & sExist,
                                          & sSynonymInfo )
              != IDE_SUCCESS );

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               &sCallSpec->libraryNamePos );
        IDE_RAISE( ERR_NOT_EXIST_LIBRARY_NAME );
    }


    sFileSpecSize = idlOS::strlen( sLibraryInfo.fileSpec );

    IDE_TEST( QC_QMP_MEM( aStatement)->alloc( sFileSpecSize + 1,
                                              (void**)&sCallSpec->fileSpec )
              != IDE_SUCCESS );

    idlOS::memcpy( sCallSpec->fileSpec,
                   sLibraryInfo.fileSpec,
                   sFileSpecSize );

    sCallSpec->fileSpec[sFileSpecSize] = '\0';

    IDE_TEST( qdpRole::checkDMLExecuteLibraryPriv( aStatement,
                                                   sLibraryInfo.libraryID,
                                                   sLibraryInfo.userID )
              != IDE_SUCCESS );

    IDE_TEST( qsvProcStmts::makeRelatedObjects(
                  aStatement,
                  & sCallSpec->userNamePos,
                  & sCallSpec->libraryNamePos,
                  & sSynonymInfo,
                  sLibraryInfo.libraryID,
                  QS_LIBRARY )
              != IDE_SUCCESS );

    // argument name of external procedure name has unsupported datatype name
    sProcParam = sParseTree->paraDecls;

    for( i = 0; i < sParseTree->paraDeclCount; i++ )
    {
        sFound = ID_FALSE;

        sColumn = &sParseTree->paramColumns[i];

        switch( sColumn->type.dataTypeId )
        {
            case MTD_BIGINT_ID:
            case MTD_BOOLEAN_ID:
            case MTD_SMALLINT_ID:
            case MTD_INTEGER_ID:
            case MTD_REAL_ID:
            case MTD_DOUBLE_ID:
            case MTD_CHAR_ID:
            case MTD_VARCHAR_ID:
            case MTD_NCHAR_ID:
            case MTD_NVARCHAR_ID:
            case MTD_NUMERIC_ID:
            case MTD_NUMBER_ID:
            case MTD_FLOAT_ID:
            case MTD_DATE_ID:
            case MTD_INTERVAL_ID:
                sFound = ID_TRUE;
                break;
            case MTD_BLOB_ID:
            case MTD_CLOB_ID:
            case MTD_BYTE_ID:       // PROJ-2717 Internal procedure
            case MTD_VARBYTE_ID:    // PROJ-2717 Internal procedure
                // BUG-39814 IN mode LOB Parameter in Extproc
                if ( ((qsVariables*)sProcParam)->inOutType == QS_IN )
                {
                    sFound = ID_TRUE;
                }
                else
                {
                    // Nothing to do.
                }
                break;
            default:
                break;
        }

        if( sFound == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   &sProcParam->name );

            IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
        }

        sProcParam = sProcParam->next;
    }

    if( sCallSpec->param != NULL )
    {
        // copy in/out/in out param type
        for( sProcParam = sParseTree->paraDecls;
             sProcParam != NULL; 
             sProcParam = sProcParam->next )
        {
            for( sExpParam = sCallSpec->param;
                 sExpParam != NULL;
                 sExpParam = sExpParam->next )
            {
                if ( QC_IS_NAME_MATCHED( sProcParam->name, sExpParam->paramNamePos ) )
                {
                    sExpParam->inOutType = ((qsVariables *)sProcParam)->inOutType;
                }
            }
        }

        // RETURN, for actual function return, must be last in the parameters clause 
        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramNamePos, "RETURN" ) &&
                 ( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) == ID_TRUE ) )
            {
                if( sExpParam->next != NULL )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sExpParam->paramNamePos );

                    IDE_RAISE( ERR_RETURN_MUST_BE_IN_THE_LAST );
                }
            }
        }

        // Incorrect Usage of RETURN in parameters clause
        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramNamePos, "RETURN" ) )
            {
                if( sParseTree->returnTypeVar == NULL )
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           &sExpParam->paramNamePos );

                    IDE_RAISE( ERR_INCORRECT_USAGE_OF_RETURN );
                }
            }
        }

        // Incorrect Usage of MAXLEN/LENGTH in parameters clause
        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            if( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) != ID_TRUE )
            { 
                if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramPropertyPos, "MAXLEN" ) )
                {
                    if( sExpParam->inOutType == QS_IN )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &sExpParam->paramPropertyPos );

                        IDE_RAISE( ERR_INCORRECT_USAGE_OF_MAXLEN );
                    }
                }
            }
        }

        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramNamePos, "RETURN" ) )
            {
                if( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) != ID_TRUE )
                { 
                    if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramPropertyPos, "MAXLEN" ) )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &sExpParam->paramPropertyPos );

                        IDE_RAISE( ERR_INCORRECT_USAGE_OF_MAXLEN );
                    }

                    if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramPropertyPos, "LENGTH" ) )
                    {
                        sqlInfo.setSourceInfo( aStatement,
                                               &sExpParam->paramPropertyPos );

                        IDE_RAISE( ERR_INCORRECT_USAGE_OF_LENGTH );
                    }

                }
            }
        }

        // Formal parameter missing in the parameters
        for( sProcParam = sParseTree->paraDecls; 
             sProcParam != NULL; 
             sProcParam = sProcParam->next )
        {
            sFound = ID_FALSE;

            for( sExpParam = sCallSpec->param;
                 sExpParam != NULL;
                 sExpParam = sExpParam->next )
            {
                if ( QC_IS_NAME_MATCHED( sProcParam->name, sExpParam->paramNamePos ) &&
                     ( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) == ID_TRUE ) )
                {
                    sFound = ID_TRUE;
                    break;
                }
            }

            if( sFound == ID_FALSE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sProcParam->name );

                IDE_RAISE( ERR_FORMAL_PARAMETER_MISSING );
            }
        }

        // external parameter name not found in formal parameter list 
        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            sFound = ID_FALSE;

            if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramNamePos, "RETURN" ) )
            {
                sFound = ID_TRUE;
                continue; // visit next parameter pos.
            }

            for( sProcParam = sParseTree->paraDecls;
                 sProcParam != NULL; 
                 sProcParam = sProcParam->next )
            {
                if ( QC_IS_NAME_MATCHED( sExpParam->paramNamePos, sProcParam->name ) )
                {
                    sFound = ID_TRUE;
                    break;
                }
            }

            if( sFound == ID_FALSE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sExpParam->paramNamePos );

                IDE_RAISE( ERR_EXTERNAL_PARAMETER_NAME_NOT_FOUND );
            }
        }


        // Multiple declarations in foreign function formal parameter list
        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            sFound = ID_FALSE;

            if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramNamePos, "RETURN" ) )
            {
                continue;
            }

            for( sExpParam2 = sExpParam->next;
                 sExpParam2 != NULL;
                 sExpParam2 = sExpParam2->next )
            {
                if ( QC_IS_NAME_MATCHED( sExpParam->paramNamePos, sExpParam2->paramNamePos ) )
                {
                    if( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) == ID_TRUE &&
                        QC_IS_NULL_NAME( sExpParam2->paramPropertyPos ) == ID_TRUE )
                    {
                        sFound = ID_TRUE;
                        break;
                    }
                    else 
                    {
                        if( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) != ID_TRUE &&
                            QC_IS_NULL_NAME( sExpParam2->paramPropertyPos ) != ID_TRUE )
                        {
                            if ( QC_IS_NAME_MATCHED( sExpParam->paramPropertyPos, sExpParam2->paramPropertyPos ) )
                            {
                                sFound = ID_TRUE;
                                break;
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
                }
            }

            if( sFound == ID_TRUE )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       &sExpParam->paramNamePos );

                IDE_RAISE( ERR_MULTIPLE_DECLARATIONS );
            }
        }

        // overwrite MAXLEN parameter
        for( sExpParam = sCallSpec->param;
             sExpParam != NULL;
             sExpParam = sExpParam->next )
        {
            if( QC_IS_NULL_NAME( sExpParam->paramPropertyPos ) == ID_TRUE )
            { 
                // Nothing to do.
            }
            else
            { 
                if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramPropertyPos, "MAXLEN" ) )
                {
                    sExpParam->inOutType = QS_IN;
                }
                else
                {
                    // Nothing to do.
                }
            }
        }
    }
    else
    {
        if( sParseTree->paraDeclCount > 0 )
        {
            IDE_TEST( QC_QMP_MEM( aStatement)->alloc( ID_SIZEOF( qsCallSpecParam ) * sParseTree->paraDeclCount,
                                                      (void**)&sCallSpec->param )
                      != IDE_SUCCESS );

            for( i = 0, 
                     sProcParam = sParseTree->paraDecls;
                 sProcParam != NULL; 
                 sProcParam = sProcParam->next )
            {
                sExpParam = &sCallSpec->param[i];

                SET_POSITION( sExpParam->paramNamePos, sProcParam->name );

                SET_EMPTY_POSITION( sExpParam->paramPropertyPos );

                sExpParam->inOutType = ((qsVariables *)sProcParam)->inOutType;

                if( sProcParam->next != NULL )
                {
                    sExpParam->next = &sCallSpec->param[i + 1];
                }
                else
                {
                    sExpParam->next = NULL;
                }

                i++;
            }
        }
    }

    sCallSpec->paramCount = 0;

    // copy table, column
    for( sExpParam = sCallSpec->param;
         sExpParam != NULL;
         sExpParam = sExpParam->next )
    {
        if ( QC_IS_STR_CASELESS_MATCHED( sExpParam->paramNamePos, "RETURN" ) )
        {
            IDE_DASSERT( sParseTree->returnTypeVar != NULL );

            sExpParam->table = sParseTree->returnTypeVar->common.table;
            sExpParam->column = sParseTree->returnTypeVar->common.column;
        }
        else
        {
            for( sProcParam = sParseTree->paraDecls;
                 sProcParam != NULL; 
                 sProcParam = sProcParam->next )
            {
                if ( QC_IS_NAME_MATCHED( sExpParam->paramNamePos, sProcParam->name ) )
                {
                    sExpParam->table = sProcParam->table;
                    sExpParam->column = sProcParam->column;

                    break;
                }
            }
        }

        sCallSpec->paramCount++;
    }

    aStatement->spvEnv->flag &= ~QSV_ENV_RETURN_STMT_MASK;
    aStatement->spvEnv->flag |= QSV_ENV_RETURN_STMT_EXIST;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_LIBRARY_NAME )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_LIBRARY_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_NOT_SUPPORTED_DATATYPE )
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_PROC_NOT_SUPPORTED_DATATYPE_SQLTEXT,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_RETURN_MUST_BE_IN_THE_LAST )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_RETURN_MUST_BE_IN_THE_LAST,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INCORRECT_USAGE_OF_RETURN )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INCORRECT_USAGE_OF_XXX,
                             "RETURN", sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INCORRECT_USAGE_OF_MAXLEN )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INCORRECT_USAGE_OF_XXX,
                             "MAXLEN", sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_INCORRECT_USAGE_OF_LENGTH )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INCORRECT_USAGE_OF_XXX,
                             "LENGTH", sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_FORMAL_PARAMETER_MISSING )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_FORMAL_PARAMETER_MISSING,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_MULTIPLE_DECLARATIONS )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_MULTIPLE_DECLARATIONS,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_EXTERNAL_PARAMETER_NAME_NOT_FOUND )
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_EXTERNAL_PARAMETER_NAME_NOT_FOUND,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-1073 Package
IDE_RC qsv::parseCreatePkg( qcStatement * aStatement )
{
    qsPkgParseTree    * sParseTree;
    qcuSqlSourceInfo    sqlInfo;

    sParseTree = ( qsPkgParseTree * )( aStatement->myPlan-> parseTree );

    aStatement->spvEnv->createPkg = sParseTree;

    if( qdbCommon::containDollarInName( &( sParseTree->pkgNamePos ) ) == ID_TRUE )
    {
        sqlInfo.setSourceInfo(
            aStatement,
            &( sParseTree->pkgNamePos ) );
        IDE_RAISE( CANT_USE_RESERVED_WORD );
    }

    sParseTree->stmtText = aStatement->myPlan->stmtText;
    sParseTree->stmtTextLen = aStatement->myPlan->stmtTextLen;

    IDE_TEST( checkHostVariable( aStatement ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( CANT_USE_RESERVED_WORD );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QDB_RESERVED_WORD_IN_OBJECT_NAME,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::validateCreatePkg( qcStatement * aStatement )
{
    /* 1. 객체의 이름 중복 확인
     * 2. 권한 확인
     * 3. parsing block
     * 4. validate block          */

    qsPkgParseTree     * sParseTree;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist = ID_FALSE;
    UInt                 sUserID = QCG_GET_SESSION_USER_ID( aStatement );

    sParseTree = (qsPkgParseTree *)(aStatement->myPlan->parseTree);

    // 1. check already existitng object
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->pkgNamePos,
                                sParseTree->objType,
                                &(sParseTree->userID),
                                &(sParseTree->pkgOID),
                                &sExist )
              != IDE_SUCCESS );
    if( sParseTree->pkgOID != QS_EMPTY_OID )
    {
        sqlInfo.setSourceInfo( aStatement, & sParseTree->pkgNamePos );
        IDE_RAISE( ERR_DUP_PKG_NAME );
    }
    else
    {
        IDE_TEST_RAISE(sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME);
    }

    // 2. check grant
    IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                              sParseTree->userID )
              != IDE_SUCCESS );

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->pkgNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

    // 3. parsing block
    IDE_TEST( sParseTree->block->common.parse(
                  aStatement,
                  (qsProcStmts *)(sParseTree->block) )
              != IDE_SUCCESS );

    // 4. validation block
    IDE_TEST( sParseTree->block->common.validate(
                  aStatement,
                  (qsProcStmts *)(sParseTree->block),
                  NULL,
                  QS_PURPOSE_PKG )
              != IDE_SUCCESS );

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_EXIST_OBJECT_NAME);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_EXIST_OBJECT_NAME));
    }
    IDE_EXCEPTION(ERR_DUP_PKG_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_DUPLICATE_PKG,
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_FAILURE;
}

IDE_RC qsv::validateReplacePkg( qcStatement * aStatement )
{
    /* 1. 객체의 이름 중복 확인
     * 2. 권한 확인
     * 3. parsing block
     * 4. validate block          */

    qsPkgParseTree     * sParseTree;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist = ID_FALSE;
    UInt                 sUserID = QCG_GET_SESSION_USER_ID( aStatement );
    idBool               sIsUsed = ID_FALSE;

    sParseTree = ( qsPkgParseTree * )( aStatement->myPlan->parseTree );

    // 1. check already existitng object
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->pkgNamePos,
                                sParseTree->objType,
                                &( sParseTree->userID ),
                                &( sParseTree->pkgOID ),
                                &sExist )
              != IDE_SUCCESS );
    if( sExist == ID_TRUE )
    {
        // 객체의 이름이 같은 다른 타입의 객체
        if( sParseTree->pkgOID == QS_EMPTY_OID )
        {
            IDE_TEST_RAISE( sExist == ID_TRUE, ERR_EXIST_OBJECT_NAME );
        }
        else
        {
            // replace
            sParseTree->common.execute = qsx::replacePkgOrPkgBody;

            // 2. check grant
            IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                                      sParseTree->userID )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // create new procedure
        sParseTree->common.execute = qsx::createPkg;

        // 2. check grant
        IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                                  sParseTree->userID )
                  != IDE_SUCCESS );
    }

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->pkgNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    // BUG-38800 Server startup시 Function-Based Index에서 사용 중인 Function을
    // recompile 할 수 있어야 한다.
    if ( ( aStatement->session->mQPSpecific.mFlag & QC_SESSION_INTERNAL_LOAD_PROC_MASK )
         != QC_SESSION_INTERNAL_LOAD_PROC_TRUE )
    {
        /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
        IDE_TEST( qcmProc::relIsUsedProcByConstraint(
                      aStatement,
                      &(sParseTree->pkgNamePos),
                      sParseTree->userID,
                      &sIsUsed )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_CONSTRAINT );

        IDE_TEST( qcmProc::relIsUsedProcByIndex(
                      aStatement,
                      &(sParseTree->pkgNamePos),
                      sParseTree->userID,
                      &sIsUsed )
                  != IDE_SUCCESS );
        IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_INDEX );
    }
    else
    {
        // Nothing to do.
    }

    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

    // 3. parsing block
    IDE_TEST( sParseTree->block->common.parse(
                  aStatement,
                  ( qsProcStmts * )( sParseTree->block ) )
              != IDE_SUCCESS );

    // 4. validation block
    IDE_TEST( sParseTree->block->common.validate(
                  aStatement,
                  ( qsProcStmts * )( sParseTree->block ),
                  NULL,
                  QS_PURPOSE_PKG )
              != IDE_SUCCESS );

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXIST_OBJECT_NAME );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_EXIST_OBJECT_NAME ) );
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_CONSTRAINT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_CONSTRAINT ) );
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_INDEX ) );
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_FAILURE;
}

IDE_RC qsv::validateCreatePkgBody( qcStatement * aStatement )
{
    /* 1. package spec의 존재여부 확인
     * 2. package body의 중복여부 확인
     * 3. 권한 체크
     * 4. parsing block
     * 5. validation block             */

    qsPkgParseTree     * sParseTree;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist       = ID_FALSE;
    qsOID                sPkgSpecOID;
    qsxPkgInfo         * sPkgSpecInfo;
    UInt                 sUserID      = QCG_GET_SESSION_USER_ID( aStatement );
    SInt                 sStage       = 0;

    sParseTree   = (qsPkgParseTree *)(aStatement->myPlan->parseTree);

    // package body는 package spec이 있어야만 만들 수 있다.
    // SYS_PACKAGES_에서 동일한 이름을 가진  row를 찾지 못하면 만들 수 없다.
    // 먼저, existObject에서 package spec이 있는지 찾는다.
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->pkgNamePos,
                                QS_PKG,
                                &(sParseTree->userID),
                                &sPkgSpecOID,
                                &sExist )
              != IDE_SUCCESS );

    // 객체가 존재한다.
    if( sExist == ID_TRUE )
    {
        // spec의 OID가 초기값과 동일하면 package가
        // 존재하지 않기 때문에 body 생성이 불가능하며,
        // spec이 없다는 error를 표시한다.
        if ( sPkgSpecOID == QS_EMPTY_OID )
        {
            sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
            IDE_RAISE( ERR_NOT_EXIST_PACKAGE_NAME );
        }
        else
        {
            /* BUG-38844
               create or replace package body일 때, spec에 대해 참조하기 때문에
               spec이 변경되는 것을 막기 위해 S latch를 잡아둔다.
               1) package body가 정상적으로 생성될 경우
               => qci::hardPrepare PVO 후에 aStatement->spvEnv->procPlanList에 대해 unlatch 수행
               2) package spec이 procPlanList에 달기 이전에 excpetion 발생할 경우
               => 현 함수의 exception 처리 시 package spec에 대해서 unlatch
               3) package spec이 procPlanList에 달린 이후 exception 발생
               => qci::hardPrepare의 exception 처리 시 procPlanList에 달린 객체와 같이 unlatch */
            IDE_TEST( qsxPkg::latchS( sPkgSpecOID )
                      != IDE_SUCCESS );
            sStage = 1;

            IDE_TEST( qsxPkg::getPkgInfo( sPkgSpecOID,
                                          & sPkgSpecInfo )
                      != IDE_SUCCESS );

            /* BUG-38348 Package spec이 invalid 상태일 수도 있다. */
            if ( sPkgSpecInfo->isValid == ID_TRUE )
            {
                /* BUG-38844
                   package spec을 spvEnv->procPlanList에 달아둔다. */
                IDE_TEST( qsvPkgStmts::makeRelatedPkgSpecToPkgBody( aStatement,
                                                                    sPkgSpecInfo )
                          != IDE_SUCCESS );

                /* BUG-41847
                   package spec의 related object를 body의 related object로 상속 */
                IDE_TEST( qsvPkgStmts::inheritRelatedObjectFromPkgSpec( aStatement,
                                                                        sPkgSpecInfo )
                          != IDE_SUCCESS );
                sStage = 0;

                // spec의 OID가 초기값이 아니면, spec이 존재하는 것이며,
                // body를 생성할 수 있다.
                IDE_TEST( qcmPkg::getPkgExistWithEmptyByName( aStatement,
                                                              sParseTree->userID,
                                                              sParseTree->pkgNamePos,
                                                              sParseTree->objType,
                                                              &( sParseTree->pkgOID ) )
                          != IDE_SUCCESS );

                // sParseTree->pkgOID가 초기값일 경우, spec 정보를 qsvEnv에 저장한다
                if( sParseTree->pkgOID == QS_EMPTY_OID)
                {
                    aStatement->spvEnv->pkgPlanTree = sPkgSpecInfo->planTree;
                }
                else
                {
                    // sParseTree->pkgOID가 초기값이 아닐 경우,
                    // 기존에 body가 존재하다는 의미이므로 body 중복 error를 표시한다.
                    sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
                    IDE_RAISE( ERR_DUP_PKG_NAME );
                }
            }
            else
            {
                sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
                IDE_RAISE( ERR_INVALID_PACKAGE_SPEC );
            }
        }
    }
    else
    {
        // 객체가 존재하지 않는다.
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
        IDE_RAISE( ERR_NOT_EXIST_PACKAGE_NAME );
    }

    // 2. check grant
    IDE_TEST( qdpRole::checkDDLCreatePSMPriv( aStatement,
                                              sParseTree->userID )
              != IDE_SUCCESS );

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->pkgNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    /* BUG-45306 PSM AUTHID 
       package body의 authid는 package spec의 authid와 동일하게 설정해야 한다. */
    sParseTree->isDefiner = aStatement->spvEnv->pkgPlanTree->isDefiner; 

    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

    // 3. parsing block
    IDE_TEST( sParseTree->block->common.parse(
                  aStatement,
                  (qsProcStmts *)(sParseTree->block) )
              != IDE_SUCCESS );

    // 4. validation block
    IDE_TEST( sParseTree->block->common.validate(
                  aStatement,
                  (qsProcStmts *)(sParseTree->block),
                  NULL,
                  QS_PURPOSE_PKG )
              != IDE_SUCCESS );

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PACKAGE_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_PACKAGE_SPECIFICATION,
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_DUP_PKG_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_DUPLICATE_PKG,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_PACKAGE_SPEC);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INVALID_PACKAGE_SPECIFICATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    /* BUG-38844
       1) sStage == 1
       => aStatement->spvEnv->procPlanList에 달기 이전이므로, 현 함수에서 unlatch
       2) sStage == 0
       => procPlanList에 달린 후이므로, qci::hardPrepare에서 exception 처리 시
       procPlanList에 달린 객체들과 함께 unlatch */
    if ( sStage == 1 )
    {
        sStage = 0;
        (void) qsxPkg::unlatch ( sPkgSpecOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsv::validateReplacePkgBody( qcStatement * aStatement )
{
    /* 1. package spec의 존재여부 확인
     * 2. package body의 중복여부 확인
     * 3. package spec의 정보 설정
     * 4. 권한 체크
     * 5. parsing block
     * 6. validation block             */

    qsPkgParseTree     * sParseTree;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist       = ID_FALSE;
    qsOID                sPkgSpecOID  = QS_EMPTY_OID;
    qsxPkgInfo         * sPkgSpecInfo;
    UInt                 sUserID      = QCG_GET_SESSION_USER_ID( aStatement );
    SInt                 sStage       = 0;

    sParseTree   = (qsPkgParseTree *)(aStatement->myPlan->parseTree);

    // package body는 package spec이 있어야만 만들 수 있다.
    // SYS_PACKAGES_에서 동일한 이름을 가진  row를 찾지 못하면 만들 수 없다.
    // 1. package spec의 존재 여부 확인
    IDE_TEST( qcm::existObject( aStatement,
                                ID_FALSE,
                                sParseTree->userNamePos,
                                sParseTree->pkgNamePos,
                                QS_PKG,
                                &(sParseTree->userID),
                                &sPkgSpecOID,
                                &sExist )
              != IDE_SUCCESS );

    // 객체가 존재한다.
    if( sExist == ID_TRUE )
    {
        // spec의 OID가 초기값과 동일하면 package가
        // 존재하지 않기 때문에 body 생성이 불가능하며,
        // spec이 없다는 error를 표시한다.
        if( sPkgSpecOID == QS_EMPTY_OID )
        {
            sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
            IDE_RAISE( ERR_NOT_EXIST_PACKAGE_NAME );
        }
        else
        {
            /* BUG-38348
               package spec이 rebuild 하다 실패하면,
               package spec는 있지만, invalid 상태이다.
               이 때는 body 역시 invalid가 되어야 한다.*/
            /* BUG-38844
               create or replace package body일 때, spec에 대해 참조하기 때문에
               spec이 변경되는 것을 막기 위해 S latch를 잡아둔다.
               1) package body가 정상적으로 생성될 경우
               => qci::hardPrepare PVO 후에 aStatement->spvEnv->procPlanList에 대해 unlatch 수행
               2) package spec이 procPlanList에 달기 이전에 excpetion 발생할 경우
               => 현 함수의 exception 처리 시 package spec에 대해서 unlatch
               3) package spec이 procPlanList에 달린 이후 exception 발생
               => qci::hardPrepare의 exception 처리 시 procPlanList에 달린 객체와 같이 unlatch */
            IDE_TEST( qsxPkg::latchS( sPkgSpecOID )
                      != IDE_SUCCESS );
            sStage = 1;

            IDE_TEST( qsxPkg::getPkgInfo( sPkgSpecOID,
                                          & sPkgSpecInfo )
                      != IDE_SUCCESS );

            if ( sPkgSpecInfo->isValid == ID_TRUE )
            {
                /*BUG-38844
                  package spec을 spvEnv->procPlanList에 달아둔다. */
                IDE_TEST( qsvPkgStmts::makeRelatedPkgSpecToPkgBody( aStatement,
                                                                    sPkgSpecInfo )
                          != IDE_SUCCESS );

                /* BUG-41847
                   package spec의 related object를 body의 related object로 상속 */
                IDE_TEST( qsvPkgStmts::inheritRelatedObjectFromPkgSpec( aStatement,
                                                                        sPkgSpecInfo )
                          != IDE_SUCCESS );
                sStage = 0;

                // spec의 OID가 초기값이 아니면, spec이 존재하는 것이며,
                // body를 생성할 수 있다.
                // 2. package body의 중복여부 확인
                IDE_TEST( qcmPkg::getPkgExistWithEmptyByName( aStatement,
                                                              sParseTree->userID,
                                                              sParseTree->pkgNamePos,
                                                              sParseTree->objType,
                                                              &( sParseTree->pkgOID ) )
                          != IDE_SUCCESS );

                if( sParseTree->pkgOID != QS_EMPTY_OID )
                {
                    // replace
                    sParseTree->common.execute = qsx::replacePkgOrPkgBody;
                }
                else
                {
                    // create new package body
                    sParseTree->common.execute = qsx::createPkgBody;
                }

                // 3. spec에 대한 정보 설정
                aStatement->spvEnv->pkgPlanTree = sPkgSpecInfo->planTree;

                // 4. check grant
                IDE_TEST( qdpRole::checkDDLCreatePSMPriv(
                              aStatement,
                              sParseTree->userID )
                          != IDE_SUCCESS );
            }
            else
            {
                sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
                IDE_RAISE( ERR_INVALID_PACKAGE_SPEC );
            }
        }
    }
    else
    {
        // 객체가 존재하지 않는다.
        sqlInfo.setSourceInfo( aStatement, &(sParseTree->pkgNamePos) );
        IDE_RAISE( ERR_NOT_EXIST_PACKAGE_NAME );
    }

    // BUG-42322
    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->pkgNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    /* BUG-45306 PSM AUTHID 
       package body의 authid는 package spec의 authid와 동일하게 설정해야 한다. */
    sParseTree->isDefiner = aStatement->spvEnv->pkgPlanTree->isDefiner; 

    QCG_SET_SESSION_USER_ID( aStatement, sParseTree->userID );

    // 5. parsing block
    IDE_TEST( sParseTree->block->common.parse(
                  aStatement,
                  (qsProcStmts *)(sParseTree->block) )
              != IDE_SUCCESS );

    // 6. validation block
    IDE_TEST( sParseTree->block->common.validate(
                  aStatement,
                  (qsProcStmts *)(sParseTree->block),
                  NULL,
                  QS_PURPOSE_PKG )
              != IDE_SUCCESS );

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_PACKAGE_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_PACKAGE_SPECIFICATION,
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_INVALID_PACKAGE_SPEC);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_INVALID_PACKAGE_SPECIFICATION,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    QCG_SET_SESSION_USER_ID( aStatement, sUserID );

    /* BUG-38844
       1) sStage == 1
       => aStatement->spvEnv->procPlanList에 달기 이전이므로, 현 함수에서 unlatch
       2) sStage == 0
       => procPlanList에 달린 후이므로, qci::hardPrepare에서 exception 처리 시
       procPlanList에 달린 객체들과 함께 unlatch */
    if ( sStage == 1 )
    {
        sStage = 0;
        (void) qsxPkg::unlatch ( sPkgSpecOID );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;
}

IDE_RC qsv::validateAltPkg( qcStatement * aStatement )
{
    qsAlterPkgParseTree * sParseTree;
    qcuSqlSourceInfo      sqlInfo;

    sParseTree = ( qsAlterPkgParseTree * )( aStatement->myPlan->parseTree );

    // 1. check userID and pkgOID
    IDE_TEST( checkUserAndPkg(
                  aStatement,
                  sParseTree->userNamePos,
                  sParseTree->pkgNamePos,
                  &( sParseTree->userID ),
                  &( sParseTree->specOID ),
                  &( sParseTree->bodyOID ) )
              != IDE_SUCCESS );

    if( sParseTree->specOID == QS_EMPTY_OID )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->pkgNamePos );
        IDE_RAISE( ERR_NOT_EXIST_PKG_NAME );
    }
    else
    {
        // Nothing to do.
    }

    if( sParseTree->bodyOID == QS_EMPTY_OID )
    {
        if( sParseTree->option == QS_PKG_BODY_ONLY )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & sParseTree->pkgNamePos );
            IDE_RAISE( ERR_NOT_EXIST_PKG_BODY_NAME );
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

    // 2. check grant
    IDE_TEST( qdpRole::checkDDLAlterPSMPriv( aStatement,
                                             sParseTree->userID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_PKG_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_PKG_BODY_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG_BODY,
                             sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::validateDropPkg( qcStatement * aStatement )
{
    qsAlterPkgParseTree * sParseTree;
    qcuSqlSourceInfo      sqlInfo;
    idBool                sIsUsed = ID_FALSE;

    sParseTree = ( qsAlterPkgParseTree * )( aStatement->myPlan->parseTree );

    // 1. check userID and pkgOID
    IDE_TEST( checkUserAndPkg(
                  aStatement,
                  sParseTree->userNamePos,
                  sParseTree->pkgNamePos,
                  &( sParseTree->userID ),
                  &( sParseTree->specOID ),
                  &( sParseTree->bodyOID ) )
              != IDE_SUCCESS );

    if( sParseTree->specOID == QS_EMPTY_OID )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sParseTree->pkgNamePos );
        IDE_RAISE( ERR_NOT_EXIST_PKG_NAME );
    }
    else
    {
        if( sParseTree->option == QS_PKG_BODY_ONLY )
        {
            if( sParseTree->bodyOID== QS_EMPTY_OID )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sParseTree->pkgNamePos );
                IDE_RAISE( ERR_NOT_EXIST_PKG_NAME );
            }
        }
    }

    // 2. check grant
    IDE_TEST( qdpRole::checkDDLDropPSMPriv( aStatement,
                                            sParseTree->userID )
              != IDE_SUCCESS );

    /* BUG-35445 Check Constraint, Function-Based Index에서 사용 중인 Function을 변경/제거 방지 */
    IDE_TEST( qcmProc::relIsUsedProcByConstraint(
                  aStatement,
                  &(sParseTree->pkgNamePos),
                  sParseTree->userID,
                  &sIsUsed )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_CONSTRAINT );

    IDE_TEST( qcmProc::relIsUsedProcByIndex(
                  aStatement,
                  &(sParseTree->pkgNamePos),
                  sParseTree->userID,
                  &sIsUsed )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sIsUsed == ID_TRUE, ERR_PROC_USED_BY_INDEX );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_PKG_NAME );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_NOT_EXIST_PKG,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_CONSTRAINT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_CONSTRAINT ) );
    }
    IDE_EXCEPTION( ERR_PROC_USED_BY_INDEX );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_PROC_USED_BY_INDEX ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* PROJ-1073 Package */
IDE_RC qsv::parseExecPkgAssign ( qcStatement * aQcStmt )
{
    qsExecParseTree * sParseTree;
    idBool            sExist = ID_FALSE;
    qcmSynonymInfo    sSynonymInfo;
    qcuSqlSourceInfo  sqlInfo;
    qsxPkgInfo      * sPkgInfo;
    idBool            sIsVariable = ID_FALSE;
    qsVariables     * sVariable   = NULL;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qsv::parseExecPkgAssign::__FT__" );

    sParseTree = (qsExecParseTree*)aQcStmt->myPlan->parseTree;

    /* BUG-45994 */
    IDU_FIT_POINT_FATAL( "qsv::parseExecPkgAssign::__FT__::STAGE1" );

    IDE_TEST( qcmSynonym::resolvePkg(
                  aQcStmt,
                  sParseTree->leftNode->userName,
                  sParseTree->leftNode->tableName,
                  &(sParseTree->procOID),
                  &(sParseTree->userID),
                  &sExist,
                  &sSynonymInfo)
              != IDE_SUCCESS );

    if( sExist == ID_TRUE )
    {
        // initialize section
        // synonym으로 참조되는 proc의 기록
        IDE_TEST( qsvPkgStmts::makePkgSynonymList(
                      aQcStmt,
                      & sSynonymInfo,
                      sParseTree->leftNode->userName,
                      sParseTree->leftNode->tableName,
                      sParseTree->procOID )
                  != IDE_SUCCESS );

        IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree( aQcStmt,
                                                          sParseTree->procOID,
                                                          QS_PKG,
                                                          &(aQcStmt->spvEnv->procPlanList) )
                  != IDE_SUCCESS );

        IDE_TEST( qsxPkg::getPkgInfo( sParseTree->procOID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );

        // 1) spec의 package info를 가져오지 못함.
        IDE_TEST( sPkgInfo == NULL );

        /* BUG-45164 */
        IDE_TEST_RAISE( sPkgInfo->isValid != ID_TRUE, err_object_invalid );

        /* BUG-38720
           package body 존재 여부 확인 후 body의 qsxPkgInfo를 가져온다. */
        IDE_TEST( qcmPkg::getPkgExistWithEmptyByName(
                      aQcStmt,
                      sParseTree->userID,
                      sParseTree->leftNode->tableName,
                      QS_PKG_BODY,
                      &(sParseTree->pkgBodyOID) )
                  != IDE_SUCCESS );

        IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aQcStmt,
                                                   sPkgInfo->planTree->userID,
                                                   sPkgInfo->privilegeCount,
                                                   sPkgInfo->granteeID,
                                                   ID_FALSE,
                                                   NULL,
                                                   NULL )
                  != IDE_SUCCESS );

        // PROJ-2533
        // exec pkg1.func(10) := 1 와 같이 subprogram은 left node에 올 수 없다.
        IDE_TEST( qsvProcVar::searchPkgVariable( aQcStmt,
                                                 sPkgInfo,
                                                 sParseTree->leftNode,
                                                 &sIsVariable,
                                                 &sVariable )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsVariable == ID_FALSE,
                        ERR_NOT_EXIST_SUBPROGRAM );

        IDU_LIST_INIT( &sParseTree->refCurList );
    }
    else
    {
        // left node를 찾지 못한 경우 invalid identifier error를 발생한다.
        sqlInfo.setSourceInfo( aQcStmt,
                               & sParseTree->leftNode->position);
        IDE_RAISE( ERR_INVALID_IDENTIFIER );
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_EXIST_SUBPROGRAM )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_NOT_EXIST_SUBPROGRAM_AND_VARIABLE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_IDENTIFIER );
    {
        (void)sqlInfo.init( aQcStmt->qmeMem );
        IDE_SET(
            ideSetErrorCode( qpERR_ABORT_QSV_INVALID_IDENTIFIER,
                             sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    // BUG-38883 print error position
    if ( ideHasErrorPosition() == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aQcStmt,
                               &sParseTree->leftNode->position);

        // set original error code.
        qsxEnv::setErrorCode( QC_QSX_ENV(aQcStmt) );

        (void)sqlInfo.initWithBeforeMessage(
            QC_QME_MEM(aQcStmt) );

        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                            sqlInfo.getBeforeErrMessage(),
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

        // set sophisticated error message.
        qsxEnv::setErrorMessage( QC_QSX_ENV(aQcStmt) );
    }
    else
    {
        // Nothing to do.
    }

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

/* PROJ-1073 Package */
IDE_RC qsv::validateExecPkgAssign ( qcStatement * aQcStmt )
{
    qsExecParseTree * sParseTree;
    qsxPkgInfo      * sPkgInfo     = NULL;
    qsxPkgInfo      * sPkgBodyInfo = NULL;
    qsxProcPlanList   sPlan;

    sParseTree = (qsExecParseTree*)aQcStmt->myPlan->parseTree;

    sParseTree->leftNode->lflag |= QTC_NODE_LVALUE_ENABLE;

    // exec pkg1.v1 := 100;
    IDE_TEST( qtc::estimate(
                  sParseTree->leftNode,
                  QC_SHARED_TMPLATE( aQcStmt ),
                  aQcStmt,
                  NULL,
                  NULL,
                  NULL )
              != IDE_SUCCESS );

    // BUG-42790
    // left에 오는 변수는  column 모듈이거나, host 변수여야함.
    IDE_ERROR_RAISE( ( ( sParseTree->leftNode->node.module ==
                         &qtc::columnModule ) ||
                       ( sParseTree->leftNode->node.module ==
                         &qtc::valueModule ) ),
                     ERR_UNEXPECTED_MODULE_ERROR );

    IDE_TEST( qtc::estimate(
                  sParseTree->callSpecNode,
                  QC_SHARED_TMPLATE( aQcStmt ),
                  aQcStmt,
                  NULL,
                  NULL,
                  NULL )
              != IDE_SUCCESS );

    // get parameter declaration
    IDE_TEST( qsxPkg::getPkgInfo( sParseTree->procOID,
                                  &sPkgInfo )
              != IDE_SUCCESS);

    // check grant
    IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aQcStmt,
                                               sPkgInfo->planTree->userID,
                                               sPkgInfo->privilegeCount,
                                               sPkgInfo->granteeID,
                                               ID_FALSE,
                                               NULL,
                                               NULL)
              != IDE_SUCCESS );

    if ( sParseTree->pkgBodyOID != QS_EMPTY_OID )
    {
        IDE_TEST( qsxPkg::getPkgInfo( sParseTree->pkgBodyOID,
                                      &sPkgBodyInfo )
                  != IDE_SUCCESS );

        sPlan.pkgBodyOID         = sPkgBodyInfo->pkgOID;
        sPlan.pkgBodyModifyCount = sPkgBodyInfo->modifyCount;
    }
    else
    {
        sPlan.pkgBodyOID         = QS_EMPTY_OID;
        sPlan.pkgBodyModifyCount = 0;
    }

    // BUG-36973
    sPlan.objectID     = sPkgInfo->pkgOID;
    sPlan.modifyCount  = sPkgInfo->modifyCount;
    sPlan.objectType   = sPkgInfo->objType;
    sPlan.planTree     = (qsPkgParseTree *)sPkgInfo->planTree;
    sPlan.next = NULL;

    IDE_TEST( qcgPlan::registerPlanPkg( aQcStmt, &sPlan ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNEXPECTED_MODULE_ERROR )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QMC_UNEXPECTED_ERROR,
                                  "qsv::validateExecPkgAssign",
                                  "The module is unexpected" ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::checkMyPkgSubprogramCall( qcStatement * aStatement,
                                      idBool      * aMyPkgSubprogram,
                                      idBool      * aRecursiveCall )
{
/*******************************************************************************
 *
 * Description :
 *    create package 시 subprogram에 대해 재귀호출인지 또는
 *    동일한 package에 존재하는 subprogram을 호출하는 검사
 *
 * Implementation :
 *    1. 재귀호출인지 검사
 *      a. qsExecParseTree->callSpecNode->columnName과
 *         aStatement->spvEnv->createProc->procNamePos가 동일하면 재귀호출
 *    2. 재귀호출이 아니면, 동일한 package 내의 subprogram 호출하는지 검사
 *      a. public subprogram에서 검사
 *         ( 즉, package spec에 선언되어 있는 subprogram 검사 )
 *        => public subprogram을 호출하는 경우,
 *           qsProcStmtSql->usingSubprograms에 달아놓는다.
 *      b. a에 없을 경우, private subprogram에 존재하는 검사
 *         ( package body만 정의된 subprogram )
 ******************************************************************************/

    qsExecParseTree  * sExeParseTree        = NULL;
    qsPkgParseTree   * sPkgParseTree        = NULL;
    qsPkgStmts       * sCurrStmt            = NULL;
    UInt               sSubprogramID        = QS_PSM_SUBPROGRAM_ID;
    qsPkgSubprogramType sSubprogramType     = QS_NONE_TYPE;
    idBool             sIsInitializeSection = ID_FALSE;
    idBool             sIsPossible          = ID_FALSE;
    idBool             sExist               = ID_FALSE;
    idBool             sIsRecursiveCall     = ID_FALSE;
    UInt               sErrCode;

    *aRecursiveCall      = ID_FALSE;
    *aMyPkgSubprogram    = ID_FALSE;
    sExeParseTree        = (qsExecParseTree *)(aStatement->myPlan->parseTree);
    sPkgParseTree        = aStatement->spvEnv->createPkg;
    sCurrStmt            = aStatement->spvEnv->currSubprogram;
    sIsInitializeSection = aStatement->spvEnv->isPkgInitializeSection;

    IDE_DASSERT( QC_IS_NULL_NAME( sExeParseTree->callSpecNode->columnName) != ID_TRUE );

    if ( QC_IS_NULL_NAME( sExeParseTree->callSpecNode->userName ) != ID_TRUE )
    {
        IDE_TEST( qcmUser::getUserID(aStatement,
                                     sExeParseTree->callSpecNode->userName,
                                     & ( sExeParseTree->userID ) )
                  != IDE_SUCCESS );
    }
    else /* sExeParseTree->callSpecNode->userName is NULL */
    {
        sExeParseTree->userID = QCG_GET_SESSION_USER_ID( aStatement );
    }

    if ( sExeParseTree->userID == sPkgParseTree->userID )
    {
        if ( QC_IS_NULL_NAME( sExeParseTree->callSpecNode->tableName ) != ID_TRUE )
        {
            if ( QC_IS_NAME_MATCHED( sExeParseTree->callSpecNode->tableName, sPkgParseTree->pkgNamePos ) )
            {
                sIsPossible = ID_TRUE;
            }
            else
            {
                sIsPossible = ID_FALSE;
            }
        }
        else /* sExeParseTree->callSpecNode->tableName is NULL */
        {
            sIsPossible = ID_TRUE;
        }
    }
    else
    {
        sIsPossible = ID_FALSE;
    }

    IDE_TEST_CONT( sIsPossible == ID_FALSE , IS_NOT_SAME_PACKAGE_OR_IS_RECURSIVECALL );

    // BUG-41262 PSM overloading
    if ( qsvPkgStmts::getMyPkgSubprogramID(
             aStatement,
             sExeParseTree->callSpecNode,
             &sSubprogramType,
             &sSubprogramID ) == IDE_SUCCESS )
    {
        IDE_TEST_CONT( sSubprogramID == QS_PSM_SUBPROGRAM_ID,
                       IS_NOT_SAME_PACKAGE_OR_IS_RECURSIVECALL );

        /* BUG-41847 package의 subprogram을 default value로 사용 가능하게 지원 */
        /* package spec에 선언된 varialbe에 default로 올 수 있는 subprogram
           - 다른 package subprogram */
        IDE_TEST_CONT( (sPkgParseTree->objType == QS_PKG) &&
                       (sIsInitializeSection == ID_FALSE) &&
                       (sCurrStmt == NULL),
                       IS_NOT_SAME_PACKAGE_OR_IS_RECURSIVECALL );

        /* package body에 선언된 varialbe에 default로 올 수 있는 subprogram
           - 다른 package subprogram
           - 동일 package의 public subprogram */
        IDE_TEST_CONT( (sPkgParseTree->objType == QS_PKG_BODY) &&
                       (sIsInitializeSection == ID_FALSE) &&
                       (sCurrStmt == NULL) &&
                       (sSubprogramType == QS_PRIVATE_SUBPROGRAM),
                       IS_NOT_SAME_PACKAGE_OR_IS_RECURSIVECALL );

        // RecursiveCall
        if ( (sIsInitializeSection != ID_TRUE) &&
             (qsvPkgStmts::checkIsRecursiveSubprogramCall(
                 sCurrStmt,
                 sSubprogramID )
              == ID_TRUE ) )
        {
            sIsRecursiveCall = ID_TRUE;
            sExeParseTree->subprogramID
                = ( (qsPkgSubprograms *) sCurrStmt )->subprogramID;
        }
        else
        {
            if ( sSubprogramType == QS_PUBLIC_SUBPROGRAM )
            {
                // BUG-37333
                IDE_TEST( qsvPkgStmts::makeUsingSubprogramList( aStatement,
                                                                sExeParseTree->callSpecNode )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_DASSERT( sSubprogramType != QS_NONE_TYPE );
            }
        }

        sExist                  = ID_TRUE;
        sExeParseTree->procOID  = QS_EMPTY_OID ;
    }
    else
    {
        sErrCode = ideGetErrorCode();

        IDE_CLEAR();

        if ( sErrCode == qpERR_ABORT_QSV_TOO_MANY_MATCH_THIS_CALL )
        {
            sExist = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    /* BUG-37235, BUG-41847
       default value에 subprogram name만 명시했을 경우, 실행 시 default value를 찾지 못 한다.
       따라서, default statement를 만들 때, package name을 명시 해 줘야 한다. */
    if ( (sExist == ID_TRUE) &&
         ((sExeParseTree->callSpecNode->lflag & QTC_NODE_SP_PARAM_DEFAULT_VALUE_MASK)
          == QTC_NODE_SP_PARAM_DEFAULT_VALUE_TRUE) )
    {
        sExeParseTree->callSpecNode->lflag &= ~QTC_NODE_PKG_VARIABLE_MASK;
        sExeParseTree->callSpecNode->lflag |= QTC_NODE_PKG_VARIABLE_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( IS_NOT_SAME_PACKAGE_OR_IS_RECURSIVECALL );

    *aMyPkgSubprogram = sExist;
    *aRecursiveCall   = sIsRecursiveCall;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::validateNamedArguments( qcStatement     * aStatement,
                                    qtcNode         * aCallSpec,
                                    qsVariableItems * aParaDecls,
                                    UInt              aParaDeclCnt,
                                    qsvArgPassInfo ** aArgPassArray )
{
/****************************************************************************
 * 
 * Description : BUG-41243 Name-based Argument Passing
 *               Name-based Argument 의 검증을 하고, 순서를 재배치한다.
 *
 *    Argument Passing에는 3가지가 있다.
 *
 *    1. Positional Argument Passing (기존 방식)
 *       - Parameter 순서대로 Argument 값을 지정
 *       - 순서가 바뀌면 사용자 의도와 다르게 작동
 *         (e.g.) exec PROC1( 1, 'aa', :a );
 *
 *    2. Name-based Argument Passing
 *       - Parameter 이름을 명시해 Argument 값을 지정
 *       - 순서가 바뀌어도 상관없음
 *         (e.g.) exec PROC1( P2=>'aa', P3=>:a, P1=>1 );
 *
 *    3. Mixed Argument Passing
 *       - 두 방식의 혼합형
 *       - 반드시 Positional으로 일부 지정한 후 Name-based로 나머지를 지정
 *         (e.g.) exec PROC1( 1, P3=>:a, P2=>'aa' );
 *
 *
 *    발생할 수 있는 에러는 다음과 같다.
 *
 *    - Positional > Name-based 순서를 어기며 지정하는 경우
 *      >> exec PROC1( 1, P3=>:a, 'aa' ); -- 다시 Positional Passing
 *                                ^  ^
 *    - 잘못된 Parameter 이름 지정
 *      >> exec PROC1( P2=>'aa', P3=>:a, P100=>1 ); -- P100은 없음
 *                                       ^  ^
 *    - 이미 Argument가 지정된 Parameter를 중복 지정하는 경우
 *      >> exec PROC1( 1, P1=>3, :a ); -- 이미 P1의 Argument 1이 지정
 *                        ^^
 *
 * Implementation :
 *
 *  - 정의된 Parameter의 참조 여부를 저장할 Array를 할당 (RefArray)
 *
 *  - CallSpecNode를 따라가면서,
 *    - 순차적 방식(Positional)은 순서대로 RefArray 값 설정
 *    - 명시적 방식(Named)은 ParaDecls를 탐색해 해당 위치 RefArray 값 설정
 *
 *  - CallSpecNode에서 Named 이후의 List를 Tmp List로 지정
 *  - 참조가 이미 된 ParaDecls를 따라가면서,
 *    - Tmp List에 알맞은 qtcNode 발견
 *    - 해당 qtcNode 다음 Node를 CallSpecNode에 이어 붙임
 *      (해당 qtcNode는 Name Node이므로, 다음 Node가 Argument Node)
 *
 * Note :
 *
 *   - 모두 끝났을 때, 참조하지 못한 ParaDecls가 발견될 수 있으나
 *     Argument Validation 나머지 작업에서 걸러낼 것이다.
 *   - RefArray는, Default Parameter 처리 할 때도 사용해야 하므로 반환한다.
 *    
 ****************************************************************************/

    qsvArgPassInfo  * sArgPassArray   = NULL;
    qtcNode         * sNewArgs        = NULL;
    qtcNode         * sCurrArg        = NULL;
    qsVariableItems * sParaDecl       = NULL;
    idBool            sIsNameFound    = ID_FALSE;
    UInt              sArgCount       = 0;
    UInt              i;
    qcuSqlSourceInfo  sqlInfo;

    IDU_FIT_POINT_FATAL( "qsv::validateNamedArguments::__FT__" );

    // 정의된 Parameter가 없으면 건너뛴다.
    IDE_TEST_CONT( aParaDeclCnt == 0, NO_NAMED_ARGUMENT );

    /****************************************************************
     * Argument Passing 여부를 저장할 배열 할당
     ***************************************************************/

    IDE_TEST( QC_QME_MEM(aStatement)->cralloc(
                  ID_SIZEOF(qsvArgPassInfo) * aParaDeclCnt,
                  (void**)& sArgPassArray )
              != IDE_SUCCESS );
    
    /****************************************************************
     * Positional Argument의 RefArray 설정
     ***************************************************************/
    
    for ( i = 0, sCurrArg = (qtcNode *)(aCallSpec->node.arguments);
          sCurrArg != NULL;
          i++, sCurrArg = (qtcNode *)(sCurrArg->node.next) )
    {
        if ( ( sCurrArg->lflag & QTC_NODE_SP_PARAM_NAME_MASK )
             == QTC_NODE_SP_PARAM_NAME_FALSE )
        {
            // Argument 개수를 세어나간다.
            sArgCount++;

            // Argument 개수가 aParaDeclCnt를 넘는 경우 에러
            IDE_TEST_RAISE( sArgCount > aParaDeclCnt, TOO_MANY_ARGUMENT );

            // PassInfo의 PassType을 POSITIONAL로 설정
            sArgPassArray[i].mType = QSV_ARG_PASS_POSITIONAL;
            // PassInfo의 PassArg를 sCurrArg 그대로 설정
            sArgPassArray[i].mArg  = sCurrArg;
        }
        else
        {
            // Named Argument가 존재한다는 걸 명시하고, 다음 단계로 넘어간다.
            sIsNameFound = ID_TRUE;
            break;
        }
    }

    // Name-based Argument가 없으면 건너뛴다.
    IDE_TEST_CONT( sIsNameFound == ID_FALSE, NO_NAMED_ARGUMENT );

    /****************************************************************
     * Name-based Argument의 RefArray 설정
     ***************************************************************/

    while ( sCurrArg != NULL )
    {
        // Named Argument만 나와야 하는데
        // 다시 Positional Argument가 나오면 에러.
        // (e.g.) exec proc1( 1, P2=>3, 3 );
        IDE_TEST_RAISE( ( sCurrArg->lflag & QTC_NODE_SP_PARAM_NAME_MASK )
                        == QTC_NODE_SP_PARAM_NAME_FALSE,
                        ARGUMENT_DOUBLE_MIXED_REFERNECE );

        // Argument 개수를 세어나간다.
        sArgCount++;

        // Argument 개수가 aParaDeclCnt를 넘는 경우 에러
        IDE_TEST_RAISE( sArgCount > aParaDeclCnt, TOO_MANY_ARGUMENT );

        // Named Argument의 Name을 가지고 ParaDecls을 검색한다.
        for ( i = 0, sParaDecl = aParaDecls;
              sParaDecl != NULL;
              i++, sParaDecl = sParaDecl->next )
        {
            if ( QC_IS_NAME_MATCHED( sParaDecl->name, sCurrArg->position ) )
            {
                // i값은 항상 Parameter 개수보다 적어야 한다.
                IDE_DASSERT( i < aParaDeclCnt );

                // Named Argument가 가리키고 있는 Parameter에
                // 이미 다른 Argument가 가리키고 있는 경우 에러.
                IDE_TEST_RAISE( sArgPassArray[i].mType != QSV_ARG_NOT_PASSED,
                                ARGUMENT_DUPLICATE_REFERENCE );

                // PassInfo의 PassType을 NAMED로 설정
                sArgPassArray[i].mType = QSV_ARG_PASS_NAMED;
                // PassInfo의 PassArg를 sCurrArg 다음 Node로 설정
                sArgPassArray[i].mArg  = (qtcNode *)(sCurrArg->node.next);

                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        // 일치하는 Parameter Name이 나오지 않은 경우 에러.
        IDE_TEST_RAISE( sParaDecl == NULL, ARGUMENT_WRONG_REFERENCE );

        // 다음 Node가 존재하지 않을 수 없다.
        IDE_DASSERT( sCurrArg->node.next != NULL );

        // 다음다음 Argument로 이동 = 다음 Argument의 Name Node로 이동
        sCurrArg = (qtcNode *)(sCurrArg->node.next->next);
    }

    /****************************************************************
     * Name-based Argument의 위치 재조정
     ***************************************************************/
    
    i        = 0;
    sNewArgs = NULL;
    sCurrArg = NULL;
    
    while ( i < aParaDeclCnt )
    {
        if ( sArgPassArray[i].mArg == NULL )
        {
            IDE_DASSERT( sArgPassArray[i].mType == QSV_ARG_NOT_PASSED );

            // Nothing to do.
        }
        else
        {
            if ( sNewArgs == NULL )
            {
                sNewArgs = sArgPassArray[i].mArg;
                sCurrArg = sNewArgs;
            }
            else
            {
                sCurrArg->node.next = (mtcNode *)sArgPassArray[i].mArg;
                sCurrArg            = (qtcNode *)sCurrArg->node.next;

                // next Node는 NULL로 초기화해야 한다.
                sCurrArg->node.next = NULL;
            }
        }

        i++;
    }

    // 재연결한 리스트를 연결
    aCallSpec->node.arguments = (mtcNode *)sNewArgs;

    IDE_EXCEPTION_CONT( NO_NAMED_ARGUMENT );

    // ParaRefArr 반환
    if ( aArgPassArray != NULL )
    {
        *aArgPassArray = sArgPassArray;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ARGUMENT_DOUBLE_MIXED_REFERNECE )
    {
        (void)sqlInfo.setSourceInfo( aStatement, & sCurrArg->position );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_NORMAL_PARAM_PASSING_AFTER_NAMED_NOTATION,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ARGUMENT_DUPLICATE_REFERENCE )
    {
        (void)sqlInfo.setSourceInfo( aStatement, & sCurrArg->position );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_DUPLICATE_PARAM_NAME_NOTATION,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ARGUMENT_WRONG_REFERENCE )
    {
        (void)sqlInfo.setSourceInfo( aStatement, & sCurrArg->position );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_WRONG_PARAM_NAME_NOTATION,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( TOO_MANY_ARGUMENT )
    {
        (void)sqlInfo.setSourceInfo( aStatement, & aCallSpec->position );
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_TOO_MANY_ARGUEMNT_SQLTEXT,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsv::setPrecisionAndScale( qcStatement * aStatement,
                                  qsVariables * aParamAndReturn )
{
/*****************************************************************************
 * Description :
 *     PROJ-2586 PSM Parameters and return without precision
 *
 *     아래의 type에 대해서 Precision, Scale 및 Size를 default 값으로 조정
 *         - CHAR( M )
 *         - VARCHAR( M )
 *         - NCHAR( M )
 *         - NVARCHAR( M )
 *         - BIT( M )
 *         - VARBIT( M )
 *         - BYTE( M )
 *         - VARBYTE( M )
 *         - NIBBLE( M )
 *         - FLOAT[ ( P ) ]
 *         - NUMBER[ ( P [ , S ] ) ]
 *         - NUMERIC[ ( P [ , S ] ) ]
 *         - DECIMAL[ ( P [ , S ] ) ]
 *             ㄴNUMERIC과 동일
 ****************************************************************************/

    qtcNode   * sNode        = NULL;
    mtcColumn * sColumn      = NULL;
    idBool      sNeedSetting = ID_TRUE;
    SInt        sPrecision   = 0;

    IDE_DASSERT( aStatement != NULL );
    IDE_DASSERT( aParamAndReturn != NULL );

    sNode = aParamAndReturn->variableTypeNode;
    sColumn = QTC_TMPL_COLUMN( QC_SHARED_TMPLATE( aStatement ), sNode );

    IDE_DASSERT( sColumn != NULL );
    IDE_DASSERT( sColumn->module != NULL );

    switch ( sColumn->module->id )
    {
        case MTD_CHAR_ID :
            sPrecision = QCU_PSM_CHAR_DEFAULT_PRECISION; 
            break;
        case MTD_VARCHAR_ID :
            sPrecision = QCU_PSM_VARCHAR_DEFAULT_PRECISION; 
            break;
        case MTD_NCHAR_ID :
            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sPrecision = QCU_PSM_NCHAR_UTF16_DEFAULT_PRECISION; 
            }
            else /* UTF8 */
            {
                /* 검증용 코드
                   mtdEstimate에서 UTF16 또는 UTF8이 아니면 에러 발생함. */
                IDE_DASSERT( mtl::mNationalCharSet->id == MTL_UTF8_ID );

                sPrecision = QCU_PSM_NCHAR_UTF8_DEFAULT_PRECISION; 
            }
            break;
        case MTD_NVARCHAR_ID :
            if ( mtl::mNationalCharSet->id == MTL_UTF16_ID )
            {
                sPrecision = QCU_PSM_NVARCHAR_UTF16_DEFAULT_PRECISION; 
            }
            else /* UTF8 */
            {
                /* 검증용 코드
                   mtdEstimate에서 UTF16 또는 UTF8이 아니면 에러 발생함. */
                IDE_DASSERT( mtl::mNationalCharSet->id == MTL_UTF8_ID );

                sPrecision = QCU_PSM_NVARCHAR_UTF8_DEFAULT_PRECISION; 
            }
            break;
        case MTD_BIT_ID :
            sPrecision = QS_BIT_PRECISION_DEFAULT; 
            break;
        case MTD_VARBIT_ID :
            sPrecision = QS_VARBIT_PRECISION_DEFAULT; 
            break;
        case MTD_BYTE_ID :
            sPrecision = QS_BYTE_PRECISION_DEFAULT; 
            break;
        case MTD_VARBYTE_ID :
            sPrecision = QS_VARBYTE_PRECISION_DEFAULT; 
            break;
        case MTD_NIBBLE_ID :
            sPrecision = MTD_NIBBLE_PRECISION_MAXIMUM; 
            break;
        case MTD_FLOAT_ID :
        case MTD_NUMBER_ID :
            sPrecision = MTD_FLOAT_PRECISION_MAXIMUM; 
            break;
        case MTD_NUMERIC_ID :
            // DECIMAL type은 NUMERIC type과 동일
            sPrecision = MTD_NUMERIC_PRECISION_MAXIMUM; 
            break;
        default :
            /* ECHAR 또는 EVARCHAR datatype의 경우,
               파서에서 mtcColumn 셋팅 시 에러 발생한다. */
            IDE_DASSERT( sColumn->module->id != MTD_ECHAR_ID );
            IDE_DASSERT( sColumn->module->id != MTD_EVARCHAR_ID );

            sNeedSetting = ID_FALSE;
    }

    if ( sNeedSetting == ID_TRUE )
    {
        IDE_TEST( mtc::initializeColumn( sColumn,
                                         sColumn->module,
                                         1,
                                         sPrecision,
                                         0 )
                  != IDE_SUCCESS );

        sColumn->flag &= ~MTC_COLUMN_SP_SET_PRECISION_MASK;
        sColumn->flag |= MTC_COLUMN_SP_SET_PRECISION_TRUE;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-42322
IDE_RC qsv::setObjectNameInfo( qcStatement        * aStatement,
                               qsObjectType         aObjectType,
                               UInt                 aUserID,
                               qcNamePosition       aObjectNamePos,
                               qsObjectNameInfo   * aObjectInfo )
{
    SChar           sBuffer[QC_MAX_OBJECT_NAME_LEN * 2 + 2];
    SInt            sObjectTypeLen = 0;
    SInt            sNameLen       = 0;

    IDE_DASSERT( aObjectNamePos.stmtText != NULL );
    IDE_DASSERT( aObjectInfo != NULL );

    switch( aObjectType )
    {
        case QS_PROC:
            sObjectTypeLen = idlOS::strlen("procedure");
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sObjectTypeLen + 1,
                                                     (void**)&aObjectInfo->objectType )
                      != IDE_SUCCESS);
            idlOS::memcpy( aObjectInfo->objectType , "procedure" , sObjectTypeLen );
            aObjectInfo->objectType[sObjectTypeLen] = '\0';
            break;
        case QS_FUNC:
            sObjectTypeLen = idlOS::strlen("function");
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sObjectTypeLen + 1,
                                                     (void**)&aObjectInfo->objectType )
                      != IDE_SUCCESS);
            idlOS::memcpy( aObjectInfo->objectType , "function" , sObjectTypeLen );
            aObjectInfo->objectType[sObjectTypeLen] = '\0';
            break;
        case QS_TYPESET:
            sObjectTypeLen = idlOS::strlen("typeset");
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sObjectTypeLen + 1,
                                                     (void**)&aObjectInfo->objectType )
                      != IDE_SUCCESS);
            idlOS::memcpy( aObjectInfo->objectType , "typeset" , sObjectTypeLen );
            aObjectInfo->objectType[sObjectTypeLen] = '\0';
            break;
        case QS_PKG:
            sObjectTypeLen = idlOS::strlen("package");
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sObjectTypeLen + 1,
                                                     (void**)&aObjectInfo->objectType )
                      != IDE_SUCCESS);
            idlOS::memcpy( aObjectInfo->objectType , "package" , sObjectTypeLen );
            aObjectInfo->objectType[sObjectTypeLen] = '\0';
            break;
        case QS_PKG_BODY:
            sObjectTypeLen = idlOS::strlen("package body");
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sObjectTypeLen + 1,
                                                     (void**)&aObjectInfo->objectType )
                      != IDE_SUCCESS);
            idlOS::memcpy( aObjectInfo->objectType , "package body" , sObjectTypeLen );
            aObjectInfo->objectType[sObjectTypeLen] = '\0';
            break;
        case QS_TRIGGER:
            sObjectTypeLen = idlOS::strlen("trigger");
            IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sObjectTypeLen + 1,
                                                     (void**)&aObjectInfo->objectType )
                      != IDE_SUCCESS);
            idlOS::memcpy( aObjectInfo->objectType , "trigger" , sObjectTypeLen );
            aObjectInfo->objectType[sObjectTypeLen] = '\0';
            break;
        default:
            IDE_DASSERT(0);
    }

    idlOS::memset( sBuffer, 0, ID_SIZEOF(sBuffer) );

    IDE_TEST( qcmUser::getUserName( aStatement,
                                    aUserID,
                                    sBuffer )
              != IDE_SUCCESS );

    // USER_NAME
    sNameLen = idlOS::strlen( sBuffer );
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sNameLen + 1,
                                             (void**)&aObjectInfo->userName )
              != IDE_SUCCESS);

    idlOS::memcpy( aObjectInfo->userName, sBuffer, sNameLen );
    aObjectInfo->userName[sNameLen] = '\0';

    idlOS::memcpy( sBuffer + idlOS::strlen(sBuffer), ".", 1 );

    QC_STR_COPY( sBuffer + idlOS::strlen(sBuffer), aObjectNamePos );

    sNameLen = idlOS::strlen( sBuffer );

    // USER_NAME.OBJECT_NAME
    IDE_TEST( QC_QMP_MEM(aStatement)->alloc( sNameLen + 1,
                                             (void**)&aObjectInfo->name )
              != IDE_SUCCESS);

    idlOS::memcpy( aObjectInfo->name, sBuffer, sNameLen );
    aObjectInfo->name[sNameLen] = '\0';

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::parseAB(qcStatement * /*aStatement*/ )
{
    return IDE_SUCCESS;

    //IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsv::validateAB(qcStatement * aStatement )
{
    qsProcParseTree    * sParseTree;

    sParseTree = (qsProcParseTree *)(aStatement->myPlan->parseTree);

    aStatement->spvEnv->createProc = sParseTree;

    sParseTree->userID = QCG_GET_SESSION_USER_ID( aStatement );

    IDE_TEST( setObjectNameInfo( aStatement,
                                 sParseTree->objType,
                                 sParseTree->userID,
                                 sParseTree->procNamePos,
                                 &sParseTree->objectNameInfo )
              != IDE_SUCCESS );

    sParseTree->stmtText    = aStatement->myPlan->stmtText;
    sParseTree->stmtTextLen = aStatement->myPlan->stmtTextLen;

    if ( sParseTree->block->common.parentLabels != NULL )
    {
        sParseTree->block->common.parentLabels->id = qsvEnv::getNextId(aStatement->spvEnv);
    }

    // parsing body
    IDE_TEST( sParseTree->block->common.parse(
                  aStatement, (qsProcStmts *)(sParseTree->block))
              != IDE_SUCCESS);

    // validation body
    IDE_TEST( sParseTree->block->common.
              validate( aStatement,
                        (qsProcStmts *)(sParseTree->block),
                        NULL,
                        QS_PURPOSE_PSM )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


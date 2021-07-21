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
 * $Id: qsvProcVar.cpp 87967 2020-07-07 05:02:47Z khkwak $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcmUser.h>
#include <qcmTableInfo.h>
#include <qcuSqlSourceInfo.h>
#include <qsv.h>
#include <qsvProcVar.h>
#include <qsvProcType.h>
#include <qsvProcStmts.h>
#include <qsxRelatedProc.h>
#include <qdpPrivilege.h>
#include <qsvPkgStmts.h>
#include <qsvCursor.h>
#include <qmvQTC.h>
#include <qdbCommon.h>
#include <qcmSynonym.h>
#include <qmvQuerySet.h>
#include <qcgPlan.h>
#include <qdpRole.h>

IDE_RC qsvProcVar::validateParaDef(
    qcStatement     * aStatement,
    qsVariableItems * aParaDecls)
{
    qsVariableItems     * sParaDef;
    qsVariableItems     * sNextParaDef;
    qsVariables         * sParaVar;
    qcuSqlSourceInfo      sqlInfo;
    UInt                  sParaCount = 0;
    // PROJ-1073 Package
    idBool                sValidPara = ID_FALSE;
    mtcColumn           * sColumn;
    qtcModule           * sQtcModule;
    idBool                sIsPkg = ID_FALSE;
    // BUG-38146
    qsCursors          * sCursor;

    if( aStatement->spvEnv->createPkg != NULL )
    {
        sIsPkg = ID_TRUE;
    }
    else
    {
        sIsPkg = ID_FALSE;
    }

    for (sParaDef = aParaDecls;
         sParaDef != NULL;
         sParaDef = sParaDef->next)
    {
        sParaVar = (qsVariables*)sParaDef;

        sParaCount++;

        // check parameter name
        for (sNextParaDef = sParaDef->next;
             sNextParaDef != NULL;
             sNextParaDef = sNextParaDef->next)
        {
            if ( QC_IS_NAME_MATCHED( sParaDef->name, sNextParaDef->name ) )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sNextParaDef->name );
                IDE_RAISE( ERR_DUP_PARA_NAME );
            }
        }

        // check data type
        if ( sParaVar->variableType == QS_ROW_TYPE )
        {
            /* BUG-38146 
               subprogram에서의 parameter의 datatype이
               cursor%rowtype ,
               package_name.cursor_name%rowtype ,
               user_name.package_name.cursor_name%rowtype 일 경우
               procedure에서의 parameter의 datatype이
               package_name.cursor_name%rowtype ,
               user_name.package_name.cursor_name%rowtype 일 경우 */
            IDE_TEST( searchCursor(
                          aStatement,
                          sParaVar->variableTypeNode,
                          &sValidPara,
                          &sCursor )
                      != IDE_SUCCESS );

            if ( sValidPara == ID_TRUE )
            {
                if ( sCursor->tableInfo == NULL )
                {
                    // To fix BUG-14279
                    // tableInfo가 생성되어 있지 않으면 생성한 후
                    // rowtype 생성.
                    IDE_TEST( qmvQuerySet::makeTableInfo(
                                  aStatement,
                                  ((qmsParseTree*)sCursor->mCursorSql->parseTree)->querySet,
                                  NULL,
                                  &sCursor->tableInfo,
                                  sCursor->common.objectID )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                sParaVar->variableTypeNode->node.table =
                    sParaVar->common.table;
                sParaVar->variableTypeNode->node.column =
                    sParaVar->common.column;
                sParaVar->variableTypeNode->node.objectID =
                    sParaVar->common.objectID;

                IDE_TEST( makeRowTypeVariable(
                              aStatement,
                              sCursor->tableInfo,
                              sParaVar )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( QC_IS_NULL_NAME(sParaVar->variableTypeNode->userName) == ID_TRUE )
                {
                    // PROJ-1075 rowtype 생성 허용.
                    // (1) user_name.table_name%ROWTYPE
                    // (2) table_name%ROWTYPE
                    IDE_TEST( checkAttributeRowType( aStatement, sParaVar )
                              != IDE_SUCCESS );
                }
                else
                {
                    sqlInfo.setSourceInfo( aStatement,
                                           & sParaVar->variableTypeNode->columnName );
                    IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
                }
            }
        }
        else if (sParaVar->variableType == QS_COL_TYPE)
        {
            // parameter에서 column type을 허용하는 경우
            // PROJ-1073 Package
            // (1),(2)은 package의 subprogram일때만 허용되고,
            // 나머지는 모두 허용
            // (1) variable%TYPE
            // (2) record.field%TYPE

            // (3) package.variable%TYPE
            // (4) package.variable.field%TYPE
            // (5) user.package.variable%TYPE
            // (6) user.package.record.field%type
            // (7) table_name.column_name%TYPE
            // (8) user_name.table_name.column_name%TYPE

            // procedure/function
            if ( ( QC_IS_NULL_NAME(sParaVar->variableTypeNode->tableName) == ID_TRUE ) &&
                 ( sIsPkg == ID_FALSE ) )
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sParaVar->variableTypeNode->columnName );
                IDE_RAISE( ERR_NOT_SUPPORTED_DATATYPE );
            }
            else
            {
                /* ex)
                   create or replace package pkg1 as
                   type rec1 is record(c1 integer);
                   v1 rec1;
                   procedure proc1(p1 v1%type);     <-- (1)
                   procedure proc2(p1 v1.c1%type);  <-- (2)
                   end;
                   /
                   package의 subprogram의 parameter는 위와 같이
                   동일한 package에 존재하는 variable을 %type으로 사용이 가능하다. */

                // PROC-1073 Package
                if( sIsPkg == ID_TRUE )
                {
                    // search local( package body, spec)
                    IDE_TEST( searchPkgLocalVarType( aStatement,
                                                     sParaVar,
                                                     &sValidPara,
                                                     &sColumn )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                    // 일반 procedure/function
                }

                /* ex)
                 * pkg1은 위의 예제 참고
                 * create or replace procedure proc1( p1 pkg1.v1%type ) as
                 * begin
                 * null;
                 * end;
                 * /
                 *
                 * create or replace package pkg2 as
                 * procedure proc1( p1 pkg1.v1%type );
                 * end;
                 * /  */
                if( sValidPara == ID_FALSE )
                {
                    IDE_TEST( checkPkgVarType( aStatement,
                                               sParaVar,
                                               &sValidPara,
                                               &sColumn )
                              != IDE_SUCCESS );
                }
                else
                {
                    // Nothing to do.
                }

                if( sValidPara == ID_TRUE )
                {
                    // primitive / record / rowtype별로 만들어야 함.
                    if( ( sColumn->type.dataTypeId >= MTD_UDT_ID_MIN ) &&
                        ( sColumn->type.dataTypeId <= MTD_UDT_ID_MAX ) )
                    {
                        sQtcModule = (qtcModule*)sColumn->module;

                        // row / record / associative array인 경우.
                        if( ( sColumn->type.dataTypeId == MTD_ROWTYPE_ID ) ||
                            ( sColumn->type.dataTypeId == MTD_RECORDTYPE_ID ) )
                        {
                            IDE_TEST( makeRecordVariable( aStatement,
                                                          sQtcModule->typeInfo,
                                                          sParaVar )
                                      != IDE_SUCCESS );
                        }
                        else if( sColumn->type.dataTypeId
                                 == MTD_ASSOCIATIVE_ARRAY_ID )
                        {
                            IDE_TEST( makeArrayVariable( aStatement,
                                                         sQtcModule->typeInfo,
                                                         sParaVar )
                                      != IDE_SUCCESS );
                        }
                        else if( sColumn->type.dataTypeId
                                 == MTD_REF_CURSOR_ID )
                        {
                            IDE_TEST( makeRefCurVariable( aStatement,
                                                          sQtcModule->typeInfo,
                                                          sParaVar )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // row, record, associative array, ref cursor 이외의 타입은 에러.
                            IDE_DASSERT(0);
                        }
                    }
                    else
                    {
                        // primitive type인 경우.
                        IDE_TEST( setPrimitiveDataType( aStatement,
                                                        sColumn,
                                                        sParaVar )
                                  != IDE_SUCCESS );
                    }
                }
            }

            if( sValidPara == ID_FALSE )
            {
                IDE_TEST(checkAttributeColType(aStatement, sParaVar)
                         != IDE_SUCCESS);
            }
        }
        else if ( sParaVar->variableType == QS_UD_TYPE )
        {
            IDE_TEST( makeUDTVariable( aStatement,
                                       sParaVar )
                      != IDE_SUCCESS );
        }
        else
        {
            // primitive type.
            // Nothing to do.
        }

        // set MTC_COLUMN_OUTBINDING_MASK
        if (sParaVar->inOutType == QS_IN)
        {
            sParaVar->variableTypeNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
            sParaVar->variableTypeNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
        }
        else
        {
            sParaVar->variableTypeNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
            sParaVar->variableTypeNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;
        }

        /* PROJ-2586 PSM Parameters and return without precision
           아래 조건 중 하나만 만족하면 precision을 조정하기 위한 함수 호출.

           조건 1. QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 1이면서
           datatype의 flag에 QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT 일 것.
           조건 2. QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 2 */

        if( ((QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 1) &&
             (((sParaVar->variableTypeNode->lflag & QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_MASK)
               == QTC_NODE_SP_PARAM_OR_RETURN_PRECISION_ABSENT))) /* 조건1 */ ||
            (QCU_PSM_PARAM_AND_RETURN_WITHOUT_PRECISION_ENABLE == 2) /* 조건2 */ )
        {
            IDE_TEST( qsv::setPrecisionAndScale( aStatement,
                                                 sParaVar )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
        
        /* BUG-44382 clone tuple 성능개선 */
        // 복사와 초기화가 필요함
        qtc::setTupleColumnFlag(
            QTC_STMT_TUPLE( aStatement, sParaVar->variableTypeNode ),
            ID_TRUE,
            ID_TRUE );

        // check default value
        if (sParaVar->defaultValueNode != NULL)
        {
            if (sParaVar->inOutType != QS_IN)
            {
                sqlInfo.setSourceInfo( aStatement,
                                       & sParaVar->defaultValueNode->columnName );
                IDE_RAISE( ERR_NOT_ALLOW_DEFAULT );
            }

            // BUG-41228
            sParaVar->defaultValueNode->lflag &= ~QTC_NODE_SP_PARAM_DEFAULT_VALUE_MASK;
            sParaVar->defaultValueNode->lflag |= QTC_NODE_SP_PARAM_DEFAULT_VALUE_TRUE;

            IDE_TEST(qdbCommon::validateDefaultDefinition(
                         aStatement, sParaVar->defaultValueNode)
                     != IDE_SUCCESS);

            sParaVar->defaultValueNode->lflag &= ~QTC_NODE_SP_PARAM_DEFAULT_VALUE_MASK;
            sParaVar->defaultValueNode->lflag |= QTC_NODE_SP_PARAM_DEFAULT_VALUE_FALSE;

        }
    }

    IDE_TEST_RAISE(sParaCount > MTC_NODE_ARGUMENT_COUNT_MAXIMUM,
                   ERR_MAX_PARAMETER_NUMBER);

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_MAX_PARAMETER_NUMBER);
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSV_PROC_MAX_PARA_NUM_INT_ARG1,
                                MTC_NODE_ARGUMENT_COUNT_MAXIMUM));
    }
    IDE_EXCEPTION(ERR_DUP_PARA_NAME);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_DUPLICATE_PARAMETER_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_SUPPORTED_DATATYPE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_SUPPORTED_DATATYPE_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_ALLOW_DEFAULT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_ALLOW_DEFAULT_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::setParamModules(
    qcStatement        * aStatement,
    UInt                 aParaDeclCount,
    qsVariableItems    * aParaDecls,
    mtdModule        *** aModules,
    mtcColumn         ** aParaColumns)
{
#define IDE_FN "qsv::setParamModules"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt               i;
    const mtdModule ** sModules;
    mtcColumn        * sParaColumn;
    mtcColumn        * sParaColumns;
    qsVariableItems  * sParaDecl;
    qsVariables      * sParaVar;

    if ( aParaDeclCount > 0 )
    {
        IDE_TEST(STRUCT_ALLOC_WITH_COUNT(QC_QMP_MEM(aStatement),
                                         mtdModule*,
                                         aParaDeclCount,
                                         &sModules)
                 != IDE_SUCCESS);

        IDE_TEST(STRUCT_ALLOC_WITH_COUNT(QC_QMP_MEM(aStatement),
                                         mtcColumn,
                                         aParaDeclCount,
                                         &sParaColumns)
                 != IDE_SUCCESS);

        for (i = 0, sParaDecl = aParaDecls;
             i < aParaDeclCount && sParaDecl != NULL;
             i++, sParaDecl = sParaDecl->next )
        {
            sParaVar = (qsVariables*)sParaDecl;
            sParaColumn = QTC_STMT_COLUMN( aStatement,
                                           sParaVar->variableTypeNode );
            mtc::copyColumn( (sParaColumns + i), sParaColumn );
            sModules[i] = sParaColumn->module;
        }

        *aModules     = (mtdModule **) sModules;
        *aParaColumns = sParaColumns;
    }
    else
    {
        *aModules     = NULL;
        *aParaColumns = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::copyParamModules(
    qcStatement        * aStatement,
    UInt                 aParaDeclCount,
    mtcColumn          * aSrcParaColumns,
    mtdModule        *** aDstModules,
    mtcColumn         ** aDstParaColumns )
{
#define IDE_FN "qsv::copyParamModules"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    UInt               i;
    const mtdModule ** sModules;
    mtcColumn        * sParaColumns;

    if ( aParaDeclCount > 0 )
    {
        IDE_TEST(STRUCT_ALLOC_WITH_COUNT(QC_QMP_MEM(aStatement),
                                         mtdModule*,
                                         aParaDeclCount,
                                         &sModules)
                 != IDE_SUCCESS);

        IDE_TEST(STRUCT_ALLOC_WITH_COUNT(QC_QMP_MEM(aStatement),
                                         mtcColumn,
                                         aParaDeclCount,
                                         &sParaColumns)
                 != IDE_SUCCESS);

        for (i = 0; i < aParaDeclCount; i++)
        {
            IDE_TEST( qsvProcType::copyColumn( aStatement,
                                               aSrcParaColumns + i,
                                               sParaColumns + i,
                                               (mtdModule **)&sModules[i] )
                      != IDE_SUCCESS );
        }

        *aDstModules     = (mtdModule **) sModules;
        *aDstParaColumns = sParaColumns;
    }
    else
    {
        *aDstModules     = NULL;
        *aDstParaColumns = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::checkAttributeColType(
    qcStatement     * aStatement,
    qsVariables     * aVariable )
{
    UInt                 sUserID;
    qcmTableInfo       * sTableInfo;
    smSCN                sTableSCN;
    UShort               sColOrder;
    idBool               sFind;
    idBool               sIsLobType;
    qcmColumn          * sColumn;
    qcuSqlSourceInfo     sqlInfo;
    qtcNode            * sTypeNode;

    idBool               sExist = ID_FALSE;
    void               * sTableHandle = NULL;
    UInt                 sTableType;

    qcmSynonymInfo       sSynonymInfo;

    sTypeNode          = aVariable->variableTypeNode;

    // user.table.col   %TYPE
    // table.col        %TYPE

    IDE_TEST(qcmSynonym::resolveTableViewQueue(
                 aStatement,
                 sTypeNode->userName,
                 sTypeNode->tableName,
                 &sTableInfo,
                 &sUserID,
                 &sTableSCN,
                 &sExist,
                 &sSynonymInfo,
                 &sTableHandle)
             != IDE_SUCCESS);

    if (sExist == ID_FALSE)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sTypeNode->tableName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }

    sTableType = smiGetTableFlag(sTableHandle) & SMI_TABLE_TYPE_MASK;
    
    // PROJ-2083 DUAL Table
    if ( sTableType != SMI_TABLE_FIXED )
    {
        // BUG-34492
        // create procedure시 호출되며 참조만 하므로
        // validation lock이면 충분하다.
        IDE_TEST(qcm::lockTableForDDLValidation(
                     aStatement,
                     sTableHandle,
                     sTableSCN)
                 != IDE_SUCCESS);

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanTable(
                      aStatement,
                      sTableHandle,
                      sTableSCN,
                      sTableInfo->tableOwnerID, /* BUG-45893 */
                      sTableInfo->name )        /* BUG-45893 */
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanSynonym(
                      aStatement,
                      & sSynonymInfo,
                      sTypeNode->userName,
                      sTypeNode->tableName,
                      sTableHandle,
                      NULL )
                  != IDE_SUCCESS );

        // make related object list
        IDE_TEST(qsvProcStmts::makeRelatedObjects(
                     aStatement,
                     & sTypeNode->userName,
                     & sTypeNode->tableName,
                     & sSynonymInfo,
                     sTableInfo->tableID,
                     QS_TABLE)
                 != IDE_SUCCESS);
    }   
    else
    {
        // BUG-46214
        // Fixed table이면 synonym만 처리한다.
        // Fixed table은 lock을 잡지 않고,
        // Related object에 추가하지도 않는다.
        IDE_TEST( qcgPlan::registerPlanSynonym(
                      aStatement,
                      & sSynonymInfo,
                      sTypeNode->userName,
                      sTypeNode->tableName,
                      sTableHandle,
                      NULL )
                  != IDE_SUCCESS );
    }

    // check column name
    IDE_TEST(qmvQTC::searchColumnInTableInfo(
                 sTableInfo,
                 sTypeNode->columnName,
                 &sColOrder,
                 &sFind,
                 &sIsLobType)
             != IDE_SUCCESS);

    if (sFind == ID_FALSE)
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sTypeNode->columnName );
        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
    }

    // set data type
    sColumn = &(sTableInfo->columns[sColOrder]);

    /* PROJ-1090 Function-based Index */
    if( (sColumn->flag & QCM_COLUMN_HIDDEN_COLUMN_MASK)
        == QCM_COLUMN_HIDDEN_COLUMN_TRUE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sTypeNode->columnName );
        IDE_RAISE( ERR_CANNOT_USE_HIDDEN_COLUMN );
    }
    else
    {
        // Nothing to do.
    }

    // column 타입 변수는 별도의 INTERMEDIATE tuple을 할당 받아야 한다.
    QC_SHARED_TMPLATE(aStatement)->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE] = ID_USHORT_MAX;

    IDE_TEST(setPrimitiveDataType( aStatement, sColumn->basicInfo, aVariable )
             != IDE_SUCCESS);

    // 더이상 컬럼이 할당되지 않아야 한다.
    QC_SHARED_TMPLATE(aStatement)->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE] = ID_USHORT_MAX;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION( ERR_CANNOT_USE_HIDDEN_COLUMN );
    {
        (void)sqlInfo.init( aStatement->qmeMem );
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDB_CANNOT_USE_HIDDEN_COLUMN,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::setPrimitiveDataType(
    qcStatement     * aStatement,
    mtcColumn       * aColumn,
    qsVariables     * aVariable )
{
    qcTemplate  * sTemplate;
    mtcTuple    * sMtcTuple;
    mtcColumn   * sColumn;
    qtcNode     * sNode;
    UShort        sCurrRowID;
    UInt          sColumnIndex;
    UInt          sOffset;

    sTemplate = QC_SHARED_TMPLATE(aStatement);
    sNode = aVariable->variableTypeNode;

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
     * Intermediate Tuple Row가 있고 비어 있지 않은 상태에서,
     * Intermediate Tuple Row에 Lob Column을 할당할 때,
     * (Old Offset + New Size) > Property 이면,
     * 새로운 Intermediate Tuple Row를 할당한다.
     */
    if( sTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE] != ID_USHORT_MAX )
    {
        sCurrRowID = sTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE];

        sMtcTuple = &(sTemplate->tmplate.rows[sCurrRowID]);
        if ( sMtcTuple->columnCount != 0 )
        {
            if ( ( aColumn->type.dataTypeId == MTD_BLOB_ID ) ||
                 ( aColumn->type.dataTypeId == MTD_CLOB_ID ) )
            {
                for( sColumnIndex = 0, sOffset = 0;
                     sColumnIndex < sMtcTuple->columnCount;
                     sColumnIndex++ )
                {
                    if ( sMtcTuple->columns[sColumnIndex].module != NULL )
                    {
                        sOffset = idlOS::align( sOffset,
                                                sMtcTuple->columns[sColumnIndex].module->align );
                        sOffset += sMtcTuple->columns[sColumnIndex].column.size;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                if ( (sOffset + (UInt)aColumn->column.size ) > QCU_INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT )
                {
                    IDE_TEST( qtc::nextRow( QC_QMP_MEM(aStatement),
                                            aStatement,
                                            sTemplate,
                                            MTC_TUPLE_TYPE_INTERMEDIATE )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               sNode,
                               aStatement,
                               sTemplate,
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    sColumn = &( sTemplate->tmplate.rows[sNode->node.table]
                 .columns[sNode->node.column]);

    mtc::copyColumn(sColumn, aColumn);

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
     * LOB Column을 LOB Value로 변환한다.
     */
    if ( ( aColumn->type.dataTypeId == MTD_BLOB_ID ) ||
         ( aColumn->type.dataTypeId == MTD_CLOB_ID ) )
    {
        IDE_TEST( mtc::initializeColumn( sColumn,
                                         aColumn->type.dataTypeId,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        // PROJ-2002 Column Security
        if ( ( sColumn->module->flag & MTD_ENCRYPT_TYPE_MASK )
             == MTD_ENCRYPT_TYPE_TRUE )
        {
            IDE_TEST( qtc::changeColumn4Decrypt( sColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }

    aVariable->common.table  = sNode->node.table;
    aVariable->common.column = sNode->node.column;
    aVariable->common.objectID = sNode->node.objectID;   // PROJ-1073 Package

    aVariable->variableType = QS_PRIM_TYPE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::checkAttributeRowType(
    qcStatement     * aStatement,
    qsVariables     * aVariable)
{
#define IDE_FN "qsvProcVar::checkAttributeRowType"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsvProcVar::checkAttributeRowType"));

    UInt                 sUserID;
    qtcNode            * sTypeNode;
    qcmTableInfo       * sTableInfo;
    smSCN                sTableSCN;
    qcuSqlSourceInfo     sqlInfo;
    idBool               sExist = ID_FALSE;
    void               * sTableHandle = NULL;
    UInt                 sTableType;

    qcmSynonymInfo       sSynonymInfo;

    // user.table   %ROWTYPE
    // table        %ROWTYPE

    sTypeNode = aVariable->variableTypeNode;

    IDE_TEST( qcmSynonym::resolveTableViewQueue(
                  aStatement,
                  sTypeNode->tableName,
                  sTypeNode->columnName,
                  &sTableInfo,
                  &sUserID,
                  &sTableSCN,
                  &sExist,
                  &sSynonymInfo,
                  &sTableHandle )
              != IDE_SUCCESS );

    if( sExist == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sTypeNode->columnName );
        IDE_RAISE( ERR_NOT_EXIST_TABLE );
    }

    sTableType = smiGetTableFlag( sTableHandle ) & SMI_TABLE_TYPE_MASK;
    
    // PROJ-2083 DUAL Table
    if ( sTableType != SMI_TABLE_FIXED )
    {
        // BUG-34492
        // create procedure시 호출되며 참조만 하므로
        // validation lock이면 충분하다.
        IDE_TEST( qcm::lockTableForDDLValidation(
                      aStatement,
                      sTableHandle,
                      sTableSCN )
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanTable(
                      aStatement,
                      sTableHandle,
                      sTableSCN,
                      sTableInfo->tableOwnerID, /* BUG-45893 */
                      sTableInfo->name )        /* BUG-45893 */
                  != IDE_SUCCESS );

        // environment의 기록
        IDE_TEST( qcgPlan::registerPlanSynonym(
                      aStatement,
                      & sSynonymInfo,
                      sTypeNode->tableName,
                      sTypeNode->columnName,
                      sTableHandle,
                      NULL )
                  != IDE_SUCCESS );

        // make related object list
        IDE_TEST( qsvProcStmts::makeRelatedObjects(
                      aStatement,
                      & sTypeNode->tableName,
                      & sTypeNode->columnName,
                      & sSynonymInfo,
                      sTableInfo->tableID,
                      QS_TABLE )
                  != IDE_SUCCESS );
    }
    else
    {
        // BUG-46214
        // Fixed table이면 synonym만 처리한다.
        // Fixed table은 lock을 잡지 않고,
        // Related object에 추가하지도 않는다.
        IDE_TEST( qcgPlan::registerPlanSynonym(
                      aStatement,
                      & sSynonymInfo,
                      sTypeNode->tableName,
                      sTypeNode->columnName,
                      sTableHandle,
                      NULL )
                  != IDE_SUCCESS );
    }

    // fix BUG-33916
    if( QC_SHARED_TMPLATE(aStatement)->tmplate.rowCount >=
        QC_SHARED_TMPLATE(aStatement)->tmplate.rowArrayCount )
    {
        IDE_TEST( qtc::increaseInternalTuple(
                      aStatement,
                      QC_SHARED_TMPLATE(aStatement)->tmplate.rowArrayCount )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Nothing to do */
    }

    // PROJ-1359 Trigger
    switch( aVariable->common.itemType )
    {
        case QS_VARIABLE:
            // set data type
            IDE_TEST( makeRowTypeVariable(
                          aStatement,
                          sTableInfo,
                          aVariable )
                      != IDE_SUCCESS );
            break;
        case QS_TRIGGER_NEW_VARIABLE :
        case QS_TRIGGER_OLD_VARIABLE :
            // set data type
            // PROJ-1075 trigger rowtype은 psm rowtype과
            // 호환이 되지 않기 때문에 호환이 되도록
            // rowtype을 생성
            IDE_TEST( makeTriggerRowTypeVariable(
                          aStatement,
                          sTableInfo,
                          aVariable )
                      != IDE_SUCCESS );
            break;
        default :
            IDE_DASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_EXIST_TABLE);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QCV_NOT_EXISTS_TABLE,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::makeRowTypeVariable(
    qcStatement     * aStatement,
    qcmTableInfo    * aTableInfo,
    qsVariables     * aVariable )
{
/***********************************************************************
 *
 * Description :
 *    rowtype 변수의 생성.
 *
 * Implementation :
 *    1. tableInfo를 이용하여 파싱후 단계의 qsTypes하나 생성
 *    2. qsTypes를 validation
 *    3. 이 type을 가지고 makeRecordVariable호출.
 *
 ***********************************************************************/

#define IDE_FN "qsvProcVar::makeRowTypeVariable"
    IDE_MSGLOG_FUNC(
        IDE_MSGLOG_BODY(IDE_FN));

    qsTypes        * sType;
    idBool           sTriggerVariable;
    qcuSqlSourceInfo sqlInfo;

    /* PROJ-1090 Function-based Index */
    if ( (aVariable->common.itemType == QS_TRIGGER_NEW_VARIABLE) ||
         (aVariable->common.itemType == QS_TRIGGER_OLD_VARIABLE) )
    {
        sTriggerVariable = ID_TRUE;
    }
    else
    {
        sTriggerVariable = ID_FALSE;
    }

    // type을 새로 하나 생성.
    // rowtype은 별도의 type reference공간이 없다.
    IDE_TEST( qsvProcType::makeRowType(
                  aStatement,
                  aTableInfo,
                  & aVariable->variableTypeNode->position,
                  sTriggerVariable,
                  & sType )
              != IDE_SUCCESS );

    // type validate
    IDE_TEST( qsvProcType::validateTypeDeclare(
                  aStatement,
                  sType,
                  & aVariable->variableTypeNode->position,
                  sTriggerVariable )
              != IDE_SUCCESS );

    // make recordtype
    IDE_TEST( makeRecordVariable( aStatement,
                                  sType,
                                  aVariable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // BUG-38883 print error position
    if ( ideHasErrorPosition() == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aVariable->common.name );

        // set original error code.
        qsxEnv::setErrorCode( QC_QSX_ENV(aStatement) );

        (void)sqlInfo.initWithBeforeMessage(
            QC_QME_MEM(aStatement) );

        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                            sqlInfo.getBeforeErrMessage(),
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

        // set sophisticated error message.
        qsxEnv::setErrorMessage( QC_QSX_ENV(aStatement) );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsvProcVar::makeTriggerRowTypeVariable(
    qcStatement     * aStatement,
    qcmTableInfo    * aTableInfo,
    qsVariables     * aVariable )
{
/***********************************************************************
 *
 * Description :
 *    PROJ-1359 Trigger
 *
 *    Trigger 의 Referencing Row를 처리하기 위해 임의로 정의된
 *    ROW TYPE으로 정의한 지역 변수에 대해 처리한다.
 *
 * Implementation :
 *
 *    일반 Table의 Validation과 동일하게 처리하고,
 *    PSM의 변수간의 접근을 위한 Node간의 정보만을 유사하게 처리한다.
 *
 *    PROJ-1075
 *    trigger의 rowtype은 psm rowtype과 호환이 안되므로
 *    psm호환용 rowtype을 한벌 더 만들고 실제로는 이를 사용함.
 *    * PROJ-1502에서 바뀐 점.
 *      trigger rowtype의 변수에 값을 복사할 때
 *      복사를 위한 table tuple을 더이상 할당하지 않는다.
 *      partition의 row가 올 수도 있기 때문임.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::makeTriggerRowTypeVariable"
    IDE_MSGLOG_FUNC(
        IDE_MSGLOG_BODY(IDE_FN));

    // PROJ-1502 PARTITIONED DISK TABLE
    // trigger row type은 기존의 psm row type과 동일한 루틴을 사용한다.

    IDE_TEST( makeRowTypeVariable( aStatement,
                                   aTableInfo,
                                   aVariable )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsvProcVar::validateLocalVariable(
    qcStatement * aStatement,
    qsVariables * aVariable )
{
#define IDE_FN "qsvProcVar::validateLocalVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsvProcVar::validateLocalVariable"));

    idBool               sValidVariable = ID_FALSE;
    qsCursors          * sCursor;
    mtcColumn          * sColumn = NULL;
    qtcModule          * sQtcModule;
    qcuSqlSourceInfo     sqlInfo;

    // BUG-27442
    // Validate length of Variable name
    if ( aVariable->common.name.size > QC_MAX_OBJECT_NAME_LEN )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aVariable->common.name );
        IDE_RAISE( ERR_MAX_NAME_LEN_OVERFLOW );
    }
    else
    {
        // Nothing to do.
    }

    // in case of
    // ------- primitive type
    //   (1) variable_name   primitive_type
    // ------- %TYPE
    //        : %TYPE must be applied to a variable, column, field or attribute.
    //          not parameter.
    //   (2) variable_name   variable_name%TYPE
    //   (3) variable_name   record_name%TYPE
    //   (4) variable_name   label_name.variable_name%TYPE
    //   (5) variable_name   label_name.record_name%TYPE
    //   (6) variable_name   table_name.column_name%TYPE
    //   (7) variable_name   label_name.record_name.field_name%TYPE
    //   (8) variable_name   user_name.table_name.column_name%TYPE
    // ------- %ROWTYPE
    //   (9) variable_name   cursor_name%ROWTYPE
    //  (10) variable_name   table_name%ROWTYPE
    //  (11) variable_name   label_name.cursor_name%ROWTYPE
    //  (12) variable_name   user_name.table_name%ROWTYPE

    // check data type
    if (aVariable->variableType == QS_PRIM_TYPE)
    {
        // (1) variable_name   primitive_type
        sValidVariable = ID_TRUE;
    }
    else if (aVariable->variableType == QS_COL_TYPE)
    {
        if( aStatement->spvEnv->createProc != NULL )
        {
            IDE_TEST( searchVarType( aStatement,
                                     aVariable,
                                     &sValidVariable,
                                     &sColumn )
                      != IDE_SUCCESS );
        }

        if( sValidVariable == ID_FALSE )
        {
            IDE_TEST( searchPkgLocalVarType( aStatement,
                                             aVariable,
                                             &sValidVariable,
                                             &sColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }

        if( ( sValidVariable == ID_FALSE ) &&
            ( QC_IS_NULL_NAME( aVariable->variableTypeNode->tableName )
              != ID_TRUE ) )
        {
            IDE_TEST( checkPkgVarType( aStatement,
                                       aVariable,
                                       &sValidVariable,
                                       &sColumn )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }

        if( sValidVariable == ID_TRUE )
        {
            // primitive / record / rowtype별로 만들어야 함.
            if( sColumn->type.dataTypeId >= MTD_UDT_ID_MIN &&
                sColumn->type.dataTypeId <= MTD_UDT_ID_MAX )
            {
                sQtcModule = (qtcModule*)sColumn->module;

                // row / record / associative array인 경우.
                if( sColumn->type.dataTypeId == MTD_ROWTYPE_ID ||
                    sColumn->type.dataTypeId == MTD_RECORDTYPE_ID )
                {
                    IDE_TEST( makeRecordVariable( aStatement,
                                                  sQtcModule->typeInfo,
                                                  aVariable )
                              != IDE_SUCCESS );
                }
                else if ( sColumn->type.dataTypeId
                          == MTD_ASSOCIATIVE_ARRAY_ID )
                {
                    IDE_TEST( makeArrayVariable( aStatement,
                                                 sQtcModule->typeInfo,
                                                 aVariable )
                              != IDE_SUCCESS );
                }
                else if ( sColumn->type.dataTypeId
                          == MTD_REF_CURSOR_ID )
                {
                    IDE_TEST( makeRefCurVariable( aStatement,
                                                  sQtcModule->typeInfo,
                                                  aVariable )
                              != IDE_SUCCESS );                    
                }
                else
                {
                    // row, record, associative array, ref cursor 이외의 타입은 에러.
                    IDE_DASSERT(0);
                }
            }
            else
            {
                // primitive type인 경우.
                IDE_TEST( setPrimitiveDataType( aStatement,
                                                sColumn,
                                                aVariable )
                          != IDE_SUCCESS);
            }
        }
        else
        {
            if( QC_IS_NULL_NAME( aVariable->variableTypeNode->tableName )
                != ID_TRUE )
            {
                IDE_TEST( checkAttributeColType(
                              aStatement,
                              aVariable )
                          != IDE_SUCCESS );

                sValidVariable = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }

        if( sValidVariable == ID_FALSE )
        {
            sqlInfo.setSourceInfo(
                aStatement,
                & aVariable->variableTypeNode->position );
            IDE_RAISE( ERR_NOT_FOUND_VAR );
        }
    }
    else if( aVariable->variableType == QS_ROW_TYPE )
    {
        //  (9) variable_name   cursor_name%ROWTYPE
        // (11) variable_name   label_name.cursor_name%ROWTYPE
        //      variable_name   package_name.cursro_name%ROWTYPE
        //      variable_name   user_name.package_name.cursor_name%ROWTYPE
        IDE_TEST( searchCursor(
                      aStatement,
                      aVariable->variableTypeNode,
                      &sValidVariable,
                      &sCursor )
                  != IDE_SUCCESS );

        if( sValidVariable == ID_TRUE )
        {
            if ( sCursor->tableInfo == NULL )
            {
                // To fix BUG-14279
                // tableInfo가 생성되어 있지 않으면 생성한 후
                // rowtype 생성.
                IDE_TEST( qmvQuerySet::makeTableInfo(
                              aStatement,
                              ((qmsParseTree*)sCursor->mCursorSql->parseTree)->querySet,
                              NULL,
                              &sCursor->tableInfo,
                              sCursor->common.objectID )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }

            aVariable->variableTypeNode->node.table =
                aVariable->common.table;
            aVariable->variableTypeNode->node.column =
                aVariable->common.column;
            aVariable->variableTypeNode->node.objectID =
                aVariable->common.objectID;

            IDE_TEST( makeRowTypeVariable(
                          aStatement,
                          sCursor->tableInfo,
                          aVariable )
                      != IDE_SUCCESS );
        }
        else // if( sValidVariable == ID_FALSE )
        {
            // (10) variable_name   table_name%ROWTYPE
            // (12) variable_name   user_name.table_name%ROWTYPE
            IDE_TEST( checkAttributeRowType( aStatement, aVariable )
                      != IDE_SUCCESS);

            sValidVariable = ID_TRUE;
        }

        if( sValidVariable == ID_FALSE )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aVariable->variableTypeNode->position );
            IDE_RAISE( ERR_WRONG_ROWTYPE_DEFINITION );
        }
    }
    // PROJ-1075 UDT
    // user-defined type은 초기에 QS_UD_TYPE으로 설정되며,
    // local또는 package에서 검색에 성공하면
    // QS_RECORD_TYPE 또는 QS_ASSOCIATIVE_ARRAY_TYPE으로 바뀐다.
    // 이 때 typenode도 새로 세팅한다.
    else if( aVariable->variableType == QS_UD_TYPE )
    {
        IDE_TEST( makeUDTVariable( aStatement,
                                   aVariable )
                  != IDE_SUCCESS );
    }

    // check CONSTANT and set MTC_COLUMN_OUTBINDING_MASK
    if( aVariable->inOutType == QS_IN )
    {
        if( aVariable->defaultValueNode == NULL )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aVariable->common.name );
            IDE_RAISE( ERR_NOT_CONSTANT );
        }

        // PROJ-1075 UDT
        // user-defined type은 constant가 될 수 없다.
        if( ( aVariable->variableType == QS_ROW_TYPE ) ||
            ( aVariable->variableType == QS_RECORD_TYPE ) ||
            ( aVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE ) )
        {
            sqlInfo.setSourceInfo( aStatement,
                                   & aVariable->common.name );
            IDE_RAISE( ERR_WRONG_CONSTANT);
        }

        aVariable->variableTypeNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
        aVariable->variableTypeNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
    }
    else
    {
        aVariable->variableTypeNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
        aVariable->variableTypeNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;
    }

    // check DEFAULT
    if( aVariable->defaultValueNode != NULL )
    {
        IDE_TEST(qdbCommon::validateDefaultDefinition(
                     aStatement, aVariable->defaultValueNode)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_NOT_FOUND_VAR);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_NOT_EXIST_VARIABLE_NAME_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_NOT_CONSTANT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_NOT_CONSTANT_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_WRONG_CONSTANT);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_WRONG_CONSTANT_SQLTEXT,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION(ERR_WRONG_ROWTYPE_DEFINITION);
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSV_PROC_WRONG_ROWTYPE_DEFINITION,
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

    // BUG-38883 print error position
    if ( ideHasErrorPosition() == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & aVariable->common.name );

        // set original error code.
        qsxEnv::setErrorCode( QC_QSX_ENV(aStatement) );

        (void)sqlInfo.initWithBeforeMessage(
            QC_QME_MEM(aStatement) );

        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QSX_SQLTEXT_WRAPPER,
                            sqlInfo.getBeforeErrMessage(),
                            sqlInfo.getErrMessage()));
        (void)sqlInfo.fini();

        // set sophisticated error message.
        qsxEnv::setErrorMessage( QC_QSX_ENV(aStatement) );
    }
    else
    {
        // Nothing to do.
    }

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsvProcVar::searchVarAndPara(
    qcStatement   * aStatement,
    qtcNode       * aVarNode,
    idBool          aSearchForLValue,
    idBool        * aIsFound,
    qsVariables  ** aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 structured data type 지원
 *               aVarNode에 맞는 psm variable을 검색하여 table, column을
 *               세팅한다.
 *
 * Implementation :
 *               qtcNode에는 총 3가지 position을 입력받을 수 있다.
 *               user_Name, table_Name, column_Name
 *
 *        (A). Argument가 없는 경우.(column/row/record/associative array type인 경우)
 *             ex) V1 := ... ;
 *                 LABEL1.V1 := ... ;
 *                 V1.I1 := ... ;
 *        (B). Argument가 있는 경우.(associative array type의 index를 쓰는 경우)
 *             ex) V1[1] := ... ;
 *                 V1[1].I1 := ... ;
 *
 *         To fix BUG-12622
 *         aSearchForLValue가 true인 경우는 output parameter를 찾는 것이다.
 *         trigger row에서 어떤 컬럼이 output용으로 사용되었는지 알기 위함임.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::searchVarAndPara"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsPkgParseTree * sPkgParseTree;

    *aIsFound     = ID_FALSE;
    *aVariable    = NULL;
    sPkgParseTree = aStatement->spvEnv->createPkg;

    if ( ( aVarNode->node.arguments == NULL ) &&
         ( ( (aVarNode->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
           QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT ) )
    {
        // (A). Argument가 없는 경우.(column/row/record/associative array type인 경우)
        // array variable output 이 필요하지 않다.
        IDE_TEST( searchVariableNonArg( aStatement,
                                        aVarNode,
                                        aIsFound,
                                        aVariable )
                  != IDE_SUCCESS );

        if ( *aIsFound == ID_FALSE )
        {
            IDE_TEST( searchParameterNonArg( aStatement,
                                             aVarNode,
                                             aSearchForLValue,
                                             aIsFound,
                                             aVariable )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1073 Package
        if ( *aIsFound == ID_FALSE )
        {
            IDE_TEST( searchPkgLocalVarNonArg( aStatement,
                                               sPkgParseTree,
                                               aVarNode,
                                               aIsFound,
                                               aVariable )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // (B). Argument가 있는 경우.(associative array type의 index를 쓰는 경우
        // array variable output이 필요하다.
        // column module에서 array variable을 이용하여 index에 해당하는 row를
        // 가져와야 하기 때문.
        IDE_TEST( searchVariableWithArg( aStatement,
                                         aVarNode,
                                         aIsFound,
                                         aVariable )
                  != IDE_SUCCESS );

        if ( *aIsFound == ID_FALSE )
        {
            IDE_TEST( searchParameterWithArg( aStatement,
                                              aVarNode,
                                              aIsFound,
                                              aVariable )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }

        // PROJ-1073 Package
        if ( *aIsFound == ID_FALSE )
        {
            IDE_TEST( searchPkgLocalVarWithArg( aStatement,
                                                sPkgParseTree,
                                                aVarNode,
                                                aIsFound,
                                                aVariable )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::searchVariableNonArg(
    qcStatement     * aStatement,
    qtcNode         * aVarNode,
    idBool          * aIsFound,
    qsVariables    ** aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 structured data type 지원
 *               aVarNode에 맞는 psm variable을 검색하여 table, column을
 *               세팅한다.
 *               IN/OUTBINDING flag도 세팅해 주어야 한다.
 *
 * Implementation :
 *               qtcNode에는 총 3가지 position을 입력받을 수 있다.
 *               user_Name, table_Name, column_Name
 *
 *        (A). Argument가 없는 경우.(column/row/record/associative array type인 경우)
 *               1. column_name
 *                 1) variable_name
 *                 2) parameter_name
 *               2. table_name.column_name
 *                 1) variable_name.field_name
 *                 2) label_name.variable_name
 *                 3) parameter_name.field_name
 *                 4) proc_name.parameter_name
 *               3. user_name.table_name.column_name
 *                 1) label_name.variable_name.field_name
 *                 2) proc_name.parameter_name.field_name
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::searchVariableNonArg"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsAllVariables      * sCurrVar;
    qsLabels            * sLabel;
    qsVariables         * sFoundVariable = NULL;
    qsVariableItems     * sCurrDeclItem;
    SChar               * sRealSQL = qsvProcVar::getValidSQL( aStatement );

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    *aIsFound = ID_FALSE;

    // search local variables
    if (QC_IS_NULL_NAME(aVarNode->userName) == ID_TRUE)
    {
        if (QC_IS_NULL_NAME(aVarNode->tableName) == ID_TRUE)
        {
            // 1.1) variable_name
            for( sCurrVar = aStatement->spvEnv->allVariables;
                 ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                 sCurrVar = sCurrVar->next )
            {
                IDE_TEST( searchVariableItems(
                              sCurrVar->variableItems,
                              sCurrDeclItem,
                              &aVarNode->columnName,
                              aIsFound,
                              &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    aVarNode->node.table = sFoundVariable->common.table;
                    aVarNode->node.column = sFoundVariable->common.column;
                    aVarNode->node.objectID = sFoundVariable->common.objectID;

                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                    if( sFoundVariable->inOutType == QS_IN )
                    {
                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
        else
        {
            // 2. table_name.column_name
            for( sCurrVar = aStatement->spvEnv->allVariables;
                 ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                 sCurrVar = sCurrVar->next )
            {
                //   1) variable_name.field_name
                IDE_TEST( searchVariableItems(
                              sCurrVar->variableItems,
                              sCurrDeclItem,
                              &aVarNode->tableName,
                              aIsFound,
                              &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                        ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                    {
                        IDE_TEST( searchFieldOfRecord(
                                      aStatement,
                                      sFoundVariable->typeInfo,
                                      sFoundVariable->variableTypeNode,
                                      aVarNode,
                                      ID_FALSE,
                                      aIsFound )
                                  != IDE_SUCCESS );

                        if ( *aIsFound == ID_TRUE )
                        {
                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                            if ( sFoundVariable->inOutType == QS_IN )
                            {
                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                        *aIsFound = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if( *aIsFound == ID_FALSE )
                {
                    //   2) label_name.variable_name
                    for( sLabel = sCurrVar->labels;
                         ( sLabel != NULL ) && ( *aIsFound == ID_FALSE );
                         sLabel = sLabel->next )
                    {
                        if (idlOS::strMatch(
                                //aStatement->stmtText + sLabel->namePos.offset,
                                sRealSQL + sLabel->namePos.offset,
                                sLabel->namePos.size,
                                aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                                aVarNode->tableName.size) == 0)
                        {
                            IDE_TEST( searchVariableItems(
                                          sCurrVar->variableItems,
                                          sCurrDeclItem,
                                          &aVarNode->columnName,
                                          aIsFound,
                                          &sFoundVariable )
                                      != IDE_SUCCESS );

                            if( *aIsFound == ID_TRUE )
                            {
                                aVarNode->node.table = sFoundVariable->common.table;
                                aVarNode->node.column = sFoundVariable->common.column;
                                aVarNode->node.objectID = sFoundVariable->common.objectID;

                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                    } // for .. label
                } // if ( *aIsFound == ID_FALSE )
            } // for .. allvariables
        }
    }
    else
    {
        // 3. user_name.table_name.column_name
        // 1) label_name.variable_name.field_name
        for( sCurrVar = aStatement->spvEnv->allVariables;
             ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
             sCurrVar = sCurrVar->next )
        {
            for( sLabel = sCurrVar->labels;
                 ( sLabel != NULL ) && ( *aIsFound == ID_FALSE );
                 sLabel = sLabel->next )
            {
                if (idlOS::strMatch(
                        sRealSQL + sLabel->namePos.offset,
                        sLabel->namePos.size,
                        aVarNode->userName.stmtText + aVarNode->userName.offset,
                        aVarNode->userName.size) == 0)
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->tableName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                            ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                        {
                            IDE_TEST( searchFieldOfRecord(
                                          aStatement,
                                          sFoundVariable->typeInfo,
                                          sFoundVariable->variableTypeNode,
                                          aVarNode,
                                          ID_FALSE,
                                          aIsFound )
                                      != IDE_SUCCESS );

                            if ( *aIsFound == ID_TRUE )
                            {
                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if ( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                            *aIsFound = ID_FALSE;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            } // for .. label
        } // for .. allVariables
    }

    if( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::searchParameterNonArg(
    qcStatement     * aStatement,
    qtcNode         * aVarNode,
    idBool            aSearchForLValue,
    idBool          * aIsFound,
    qsVariables    ** aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 structured data type 지원
 *               aVarNode에 맞는 psm variable을 검색하여 table, column을
 *               세팅한다.
 *               IN/OUTBINDING flag도 세팅해 주어야 한다.
 *
 * Implementation :
 *               qtcNode에는 총 3가지 position을 입력받을 수 있다.
 *               user_Name, table_Name, column_Name
 *
 *        (A). Argument가 없는 경우.(column/row/record/associative array type인 경우)
 *             parameter를 검색한다.
 *
 ***********************************************************************/

    qsProcParseTree     * sParseTree;
    qsVariables         * sFoundVariable = NULL;
    qsVariableItems     * sCurrDeclItem;
    qtcNode             * sNode;

    sParseTree = aStatement->spvEnv->createProc;

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    *aIsFound = ID_FALSE;       // TASK-3876 Code Sonar

    if( QC_IS_NULL_NAME(aVarNode->userName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME(aVarNode->tableName) == ID_TRUE )
        {
            // procedureName이 없는 경우 무조건 parameter 라고 가정.
            // 1. column_name
            //   2) parameter_name
            IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                           sCurrDeclItem,
                                           &aVarNode->columnName,
                                           aIsFound,
                                           &sFoundVariable )
                      != IDE_SUCCESS );

            if ( *aIsFound == ID_TRUE )
            {
                aVarNode->node.table = sFoundVariable->common.table;
                aVarNode->node.column = sFoundVariable->common.column;
                aVarNode->node.objectID = sFoundVariable->common.objectID;

                if( sFoundVariable->inOutType == QS_IN )
                {
                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                }
                else
                {
                    if( ( aSearchForLValue == ID_TRUE ) &&
                        ( sFoundVariable->variableType == QS_ROW_TYPE ) )
                    {
                        // To fix BUG-12622
                        // trigger row통째로 output으로 사용한 경우임.
                        // 각 컬럼 모두 flag세팅
                        for( sNode = (qtcNode *)sFoundVariable->variableTypeNode->node.arguments;
                             sNode != NULL;
                             sNode = (qtcNode*)sNode->node.next )
                        {
                            sNode->lflag &= ~QTC_NODE_LVALUE_MASK;
                            sNode->lflag |= QTC_NODE_LVALUE_ENABLE;
                        }        
                    }
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // 2. table_name.column_name
            //   3) parameter_name.field_name
            IDE_TEST( searchVariableItems(
                          aStatement->spvEnv->allParaDecls,
                          sCurrDeclItem,
                          &aVarNode->tableName,
                          aIsFound,
                          &sFoundVariable )
                      != IDE_SUCCESS );

            if ( *aIsFound == ID_TRUE )
            {
                if( sFoundVariable->variableType == QS_ROW_TYPE ||
                    sFoundVariable->variableType == QS_RECORD_TYPE )
                {
                    IDE_TEST( searchFieldOfRecord(
                                  aStatement,
                                  sFoundVariable->typeInfo,
                                  sFoundVariable->variableTypeNode,
                                  aVarNode,
                                  aSearchForLValue,
                                  aIsFound )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                        if ( sFoundVariable->inOutType == QS_IN )
                        {
                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                    *aIsFound = ID_FALSE;
                }
            }
            else
            {
                // Nothing to do.
            }

            if( ( *aIsFound == ID_FALSE ) && ( sParseTree != NULL ) )
            {
                // 4) procedure_name.parameter_name
                if (idlOS::strMatch(
                        sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                        sParseTree->procNamePos.size,
                        aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                        aVarNode->tableName.size) == 0)
                {
                    IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                                   sCurrDeclItem,
                                                   &aVarNode->columnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );
                    if( *aIsFound == ID_TRUE )
                    {
                        aVarNode->node.table = sFoundVariable->common.table;
                        aVarNode->node.column = sFoundVariable->common.column;
                        aVarNode->node.objectID = sFoundVariable->common.objectID;

                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                        if( sFoundVariable->inOutType == QS_IN )
                        {
                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
            } // if( *aIsFound == ID_FALSE )
        }
    }
    else
    {
        // 3.2) procedure_name.parameter_name.field_name
        if( sParseTree != NULL )
        {
            if (idlOS::strMatch(
                    sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                    sParseTree->procNamePos.size,
                    aVarNode->userName.stmtText + aVarNode->userName.offset,
                    aVarNode->userName.size) == 0)
            {
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aVarNode->tableName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    if( sFoundVariable->variableType == QS_ROW_TYPE ||
                        sFoundVariable->variableType == QS_RECORD_TYPE )
                    {
                        IDE_TEST( searchFieldOfRecord(
                                      aStatement,
                                      sFoundVariable->typeInfo,
                                      sFoundVariable->variableTypeNode,
                                      aVarNode,
                                      ID_FALSE,
                                      aIsFound )
                                  != IDE_SUCCESS );

                        if ( *aIsFound == ID_TRUE )
                        {
                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                            if ( sFoundVariable->inOutType == QS_IN )
                            {
                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                        *aIsFound = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            } // if ( idlOS::strMatch( ...
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            *aIsFound = ID_FALSE;
        }
    }
    
    if( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchVariableWithArg(
    qcStatement     * aStatement,
    qtcNode         * aVarNode,
    idBool          * aIsFound,
    qsVariables    ** aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 structured data type 지원
 *               aVarNode에 맞는 psm variable을 검색하여 table, column을
 *               세팅한다.
 *               IN/OUTBINDING flag도 세팅해 주어야 한다.
 *
 * Implementation :
 *               qtcNode에는 총 3가지 position을 입력받을 수 있다.
 *               user_Name, table_Name, column_Name
 *
 *        (B). Argument가 있는 경우.(associative array type의 index를 쓰는 경우)
 *               1. column_name[ expression ]
 *                 1) variable_name[ index ]
 *                 2) parameter_name[ index ]
 *               2. table_name.column_name[ expression ]
 *                 1) label_name.variable_name[ index ]
 *                 2) proc_name.parameter_name[ index ]
 *               3. column_name[ expression].pkg_name
 *                 1) variable_name[ index ].column_name
 *                 2) parameter_name[ index ].column_name
 *               4. user_name.table_name[ expression ].column_name
 *                 1) label_name.variable_name[ index ].column_name
 *                 2) proc_name.parameter_name[ index ].column_name
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::searchVariableWithArg"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsAllVariables      * sCurrVar;
    qsLabels            * sLabel;
    qsVariables         * sFoundVariable = NULL;
    mtcColumn           * sColumn;
    qtcModule           * sQtcModule;
    qsVariableItems     * sCurrDeclItem;
    SChar               * sRealSQL = qsvProcVar::getValidSQL( aStatement );

    *aIsFound = ID_FALSE;

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    // search local variables
    if (QC_IS_NULL_NAME(aVarNode->userName) == ID_TRUE)
    {
        if (QC_IS_NULL_NAME(aVarNode->tableName) == ID_TRUE)
        {
            if ( QC_IS_NULL_NAME(aVarNode->pkgName) == ID_TRUE )
            { 
                // 1. column_name[ expression ]
                //   1) variable_name[ index ]
                for (sCurrVar = aStatement->spvEnv->allVariables;
                     (sCurrVar != NULL) && (*aIsFound == ID_FALSE);
                     sCurrVar = sCurrVar->next)
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->columnName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            aVarNode->node.table =
                                sFoundVariable->variableTypeNode->node.arguments->table;
                            aVarNode->node.column =
                                sFoundVariable->variableTypeNode->node.arguments->column;
                            aVarNode->node.objectID =
                                sFoundVariable->variableTypeNode->node.arguments->objectID;

                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                            if ( sFoundVariable->inOutType == QS_IN )
                            {
                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }       
                }
            } // pkgName is null.
            else
            {
                // 3. column_name[ expression].pkg_name
                //   1) variable_name[ index ].column_name
                for ( sCurrVar = aStatement->spvEnv->allVariables;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                      sCurrVar = sCurrVar->next )
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->columnName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            sColumn = QTC_STMT_COLUMN( aStatement,
                                                       (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                            if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                            {
                                sQtcModule = (qtcModule*)sColumn->module;

                                IDE_TEST( searchFieldOfRecord(
                                              aStatement,
                                              sQtcModule->typeInfo,
                                              (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                              aVarNode,
                                              ID_FALSE,
                                              aIsFound )
                                          != IDE_SUCCESS );

                                if ( *aIsFound == ID_TRUE )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                    if ( sFoundVariable->inOutType == QS_IN )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            } // pkgName is not null.
        } // tableName is null.
        else
        {
            // 2. table_name.column_name[ expression ]
            //   1) label_name.variable_name[ index ]
            for( sCurrVar = aStatement->spvEnv->allVariables;
                 ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                 sCurrVar = sCurrVar->next )
            {
                for( sLabel = sCurrVar->labels;
                     ( sLabel != NULL ) && ( *aIsFound == ID_FALSE );
                     sLabel = sLabel->next )
                {
                    if (idlOS::strMatch(
                            sRealSQL + sLabel->namePos.offset,
                            sLabel->namePos.size,
                            aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                            aVarNode->tableName.size) == 0)
                    {
                        IDE_TEST( searchVariableItems(
                                      sCurrVar->variableItems,
                                      sCurrDeclItem,
                                      &aVarNode->columnName,
                                      aIsFound,
                                      &sFoundVariable )
                                  != IDE_SUCCESS );

                        if( *aIsFound == ID_TRUE )
                        {
                            if( sFoundVariable->variableType ==
                                QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                aVarNode->node.table =
                                    sFoundVariable->variableTypeNode->node.arguments->table;
                                aVarNode->node.column =
                                    sFoundVariable->variableTypeNode->node.arguments->column;
                                aVarNode->node.objectID =
                                    sFoundVariable->variableTypeNode->node.arguments->objectID;

                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                } // for .. label
            } // for .. allVariables
        } // tableName is not null.
    } // userName is null.
    else
    {
        // 4. user_name.table_name[ expression ].column_name
        //   1) label_name.variable_name[ index ].column_name

        for ( sCurrVar = aStatement->spvEnv->allVariables;
              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
              sCurrVar = sCurrVar->next )
        {
            for ( sLabel = sCurrVar->labels;
                  ( sLabel != NULL ) && ( *aIsFound == ID_FALSE );
                  sLabel = sLabel->next )
            {
                if ( idlOS::strMatch(
                         //aStatement->stmtText + sLabel->namePos.offset,
                         sRealSQL + sLabel->namePos.offset,
                         sLabel->namePos.size,
                         aVarNode->userName.stmtText + aVarNode->userName.offset,
                         aVarNode->userName.size) == 0)
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->tableName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            // variableTypeNode->node.arguments에 해당하는
                            // column의 module이 반드시 record type이어야 함.
                            sColumn = QTC_STMT_COLUMN(
                                aStatement,
                                (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                            if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                            {
                                sQtcModule = (qtcModule*)sColumn->module;

                                IDE_TEST( searchFieldOfRecord(
                                              aStatement,
                                              sQtcModule->typeInfo,
                                              (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                              aVarNode,
                                              ID_FALSE,
                                              aIsFound )
                                          != IDE_SUCCESS );

                                if ( *aIsFound == ID_TRUE )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                    if ( sFoundVariable->inOutType == QS_IN )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
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
            } // for .. label
        } // for .. allVariables
    } // userName is not null

    if( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::searchParameterWithArg(
    qcStatement     * aStatement,
    qtcNode         * aVarNode,
    idBool          * aIsFound,
    qsVariables    ** aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 structured data type 지원
 *               aVarNode에 맞는 psm variable을 검색하여 table, column을
 *               세팅한다.
 *               IN/OUTBINDING flag도 세팅해 주어야 한다.
 *
 * Implementation :
 *               qtcNode에는 총 3가지 position을 입력받을 수 있다.
 *               user_Name, table_Name, column_Name
 *
 *        (B). Argument가 있는 경우.(associative array type의 index를 쓰는 경우)
 *               1. column_name[ expression ]
 *                 1) variable_name[ index ]
 *                 2) parameter_name[ index ]
 *               2. table_name.column_name[ expression ]
 *                 1) label_name.variable_name[ index ]
 *                 2) proc_name.parameter_name[ index ]
 *               3. column_name[ expression].pkg_name
 *                 1) variable_name[ index ].column_name
 *                 2) parameter_name[ index ].column_name
 *               4. user_name.table_name[ expression ].column_name
 *                 1) label_name.variable_name[ index ].column_name
 *                 2) proc_name.parameter_name[ index ].column_name
 *
 *         위 순서대로 psm variable을 검색한다. 단, parameter검색은 맨 마지막에 한다.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::searchParameterWithArg"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsProcParseTree     * sParseTree;
    qsVariables         * sFoundVariable = NULL;
    mtcColumn           * sColumn;
    qtcModule           * sQtcModule;
    qsVariableItems     * sCurrDeclItem;

    sParseTree = aStatement->spvEnv->createProc;

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;
    *aIsFound = ID_FALSE;       // TASK-3876 Code Sonar

    if( QC_IS_NULL_NAME(aVarNode->userName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME(aVarNode->tableName) == ID_TRUE )
        {
            if ( QC_IS_NULL_NAME(aVarNode->pkgName) == ID_TRUE )
            {
                // procedureName이 없는 경우 무조건 parameter라고 가정.
                // 1. column_name[ expression ]
                //   2) parameter_name[ index ]
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aVarNode->columnName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    if ( sFoundVariable->variableType ==
                         QS_ASSOCIATIVE_ARRAY_TYPE )
                    {
                        aVarNode->node.table =
                            sFoundVariable->variableTypeNode->node.arguments->table;
                        aVarNode->node.column =
                            sFoundVariable->variableTypeNode->node.arguments->column;
                        aVarNode->node.objectID =
                            sFoundVariable->variableTypeNode->node.arguments->objectID;

                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                        if ( sFoundVariable->inOutType == QS_IN )
                        {
                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }// pkgName is null
            else
            {
                // 3. column_name[ expression].pkg_name     : table_name 없음.
                //   2) parameter_name[ index ].column_name
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aVarNode->columnName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    if ( sFoundVariable->variableType ==
                         QS_ASSOCIATIVE_ARRAY_TYPE )
                    {
                        // variableTypeNode->node.arguments에 해당하는
                        // column의 module이 반드시 record type이어야 함.
                        sColumn = QTC_STMT_COLUMN(
                            aStatement,
                            (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                        if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                        {
                            sQtcModule = (qtcModule*)sColumn->module;

                            IDE_TEST( searchFieldOfRecord(
                                          aStatement,
                                          sQtcModule->typeInfo,
                                          (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                          aVarNode,
                                          ID_FALSE,
                                          aIsFound )
                                      != IDE_SUCCESS );

                            if ( *aIsFound == ID_TRUE )
                            {
                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if ( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            } // pkgName is not null
        } // tableName is null
        else
        {
            // 2. table_name.column_name[ expression ]
            //   2) proc_name.parameter_name[ index ]
            if( sParseTree != NULL )
            {
                if (idlOS::strMatch(
                        sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                        sParseTree->procNamePos.size,
                        aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                        aVarNode->tableName.size) == 0)
                {
                    IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                                   sCurrDeclItem,
                                                   &aVarNode->columnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType ==
                             QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            aVarNode->node.table =
                                sFoundVariable->variableTypeNode->node.arguments->table;
                            aVarNode->node.column =
                                sFoundVariable->variableTypeNode->node.arguments->column;
                            aVarNode->node.objectID =
                                sFoundVariable->variableTypeNode->node.arguments->objectID;

                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                            if ( sFoundVariable->inOutType == QS_IN )
                            {
                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
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
            else
            {
                *aIsFound = ID_FALSE;
            }
        }
    }
    else
    {
        if ( sParseTree != NULL )
        {
            // 4. user_name.table_name[ expression ].column_name
            //   2) proc_name.parameter_name[ index ].column_name
            if (idlOS::strMatch(
                    sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                    sParseTree->procNamePos.size,
                    aVarNode->userName.stmtText + aVarNode->userName.offset,
                    aVarNode->userName.size) == 0)
            {
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aVarNode->tableName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    if ( sFoundVariable->variableType ==
                         QS_ASSOCIATIVE_ARRAY_TYPE )
                    {
                        // variableTypeNode->node.arguments에 해당하는
                        // column의 module이 반드시 record type이어야 함.
                        sColumn = QTC_STMT_COLUMN(
                            aStatement,
                            (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                        if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                        {
                            sQtcModule = (qtcModule*)sColumn->module;

                            IDE_TEST( searchFieldOfRecord(
                                          aStatement,
                                          sQtcModule->typeInfo,
                                          (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                          aVarNode,
                                          ID_FALSE,
                                          aIsFound )
                                      != IDE_SUCCESS );

                            if ( *aIsFound == ID_TRUE )
                            {
                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if ( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                        sFoundVariable = NULL;
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
        else
        {
            *aIsFound = ID_FALSE;
        }
    } // userName is not null

    if( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::searchVarType( qcStatement * aStatement,
                                  qsVariables * aVariable,
                                  idBool      * aIsFound,
                                  mtcColumn  ** aColumn )
{
/***********************************************************************
 *
 * Description : PROJ-1075 variable의 type을 검색.
 *               variable_name%TYPE 의 변수 선언이 왔을 경우
 *
 * Implementation :
 *        다음과 같은 순서로 변수를 검색함.
 *        (1) variable_name
 *        (2) record_name.field_name
 *        (3) label_name.variable_name
 *        (4) label_name.record_name.field_name
 *
 *        변수에서 못찾으면 parameter임.
 *        (5) parameter_name
 *        (6) parameter_name.field_name
 *        (7) procedure_name.parameter_name
 *        (8) procedure_name.parameter_name.field_name
 *
 *        * array 변수 같은 경우 다음과 같은 방법으로 타입을 가져올
 *         수 없음.
 *        ex) V1 arrvar[1]%TYPE;
 *            V2 arrvar[2].i1%TYPE;
 *         위와 같이 타입선언이 불가능함. 따라서 여기서는 오로지
 *         record/rowtype에 대한 고려만 해주면 됨.
 *
 *        * 변수 자기자신의 선언부분을 넘어서서 검색하지 않도록
 *          해야 함.
 *        ex) V1 INTEGER;
 *            V2 V3%TYPE; -- 여기서는 V1까지밖에 볼 수 없음. 에러.
 *            V3 INTEGER;
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::searchVarType"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsProcParseTree     * sParseTree;
    qsAllVariables      * sCurrVar;
    qsVariableItems     * sCurrDeclItem;
    qsLabels            * sLabel;
    qtcNode             * sNode;
    qsVariables         * sFoundVariable;
    SChar               * sRealSQL = qsvProcVar::getValidSQL( aStatement );

    sParseTree = aStatement->spvEnv->createProc;

    *aIsFound  = ID_FALSE;
    *aColumn   = NULL;

    if( sParseTree == NULL )
    {
        IDE_CONT( PACKAGE_VARIABLE );
    }

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    sNode      = aVariable->variableTypeNode;

    sFoundVariable = NULL;

    // 적합성 검사. columnName은 반드시 있어야 함.
    IDE_DASSERT( QC_IS_NULL_NAME(sNode->columnName) == ID_FALSE );

    if( QC_IS_NULL_NAME(sNode->userName) == ID_TRUE )
    {
        // userName이 없음
        // (1) variable_name / parameter name
        // (2) record_name.field_name
        // (3) label_name.variable_name

        if( QC_IS_NULL_NAME( sNode->tableName ) == ID_TRUE )
        {
            // userName, tableName이 없음.
            // (1) variable_name / parameter name
            for( sCurrVar = aStatement->spvEnv->allVariables;
                 sCurrVar != NULL && *aIsFound == ID_FALSE;
                 sCurrVar = sCurrVar->next )
            {
                IDE_TEST( searchVariableItems(
                              sCurrVar->variableItems,
                              sCurrDeclItem,
                              &sNode->columnName,
                              aIsFound,
                              &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    *aColumn = QC_SHARED_TMPLATE(aStatement)->tmplate
                        .rows[sFoundVariable->variableTypeNode->node.table]
                        .columns
                        + sFoundVariable->variableTypeNode->node.column;
                }
            }
        }
        else
        {
            // userName은 없고 tableName은 있음
            for( sCurrVar = aStatement->spvEnv->allVariables;
                 sCurrVar != NULL && *aIsFound == ID_FALSE;
                 sCurrVar = sCurrVar->next )
            {
                // (2) record_name.field_name
                IDE_TEST( searchVariableItems(
                              sCurrVar->variableItems,
                              sCurrDeclItem,
                              &sNode->tableName,
                              aIsFound,
                              &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                        ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                    {
                        IDE_TEST( searchFieldTypeOfRecord(
                                      sFoundVariable,
                                      &sNode->columnName,
                                      aIsFound,
                                      QC_SHARED_TMPLATE(aStatement),
                                      aColumn )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if( *aIsFound == ID_FALSE )
                {
                    // (3) label_name.variable_name
                    for( sLabel = sCurrVar->labels;
                         sLabel != NULL && *aIsFound == ID_FALSE;
                         sLabel = sLabel->next )
                    {
                        if( idlOS::strMatch(
                                //aStatement->stmtText + sLabel->namePos.offset,
                                sRealSQL + sLabel->namePos.offset,
                                sLabel->namePos.size,
                                sNode->tableName.stmtText + sNode->tableName.offset,
                                sNode->tableName.size) == 0)
                        {
                            IDE_TEST( searchVariableItems(
                                          sCurrVar->variableItems,
                                          sCurrDeclItem,
                                          &sNode->columnName,
                                          aIsFound,
                                          &sFoundVariable )
                                      != IDE_SUCCESS );

                            if( *aIsFound == ID_TRUE )
                            {
                                *aColumn = QC_SHARED_TMPLATE(aStatement)->tmplate
                                    .rows[sFoundVariable->variableTypeNode->node.table]
                                    .columns
                                    + sFoundVariable->variableTypeNode->node.column;
                            }
                        }
                    } // for .. label
                } // if not found 
            } // for .. allVariables
        }
    }
    else
    {
        // userName, tableName, columnName 셋다 갖고 있음.
        //(4) label_name.record_name.field_name
        for( sCurrVar = aStatement->spvEnv->allVariables;
             sCurrVar != NULL && *aIsFound == ID_FALSE;
             sCurrVar = sCurrVar->next )
        {
            for( sLabel = sCurrVar->labels;
                 sLabel != NULL && *aIsFound == ID_FALSE;
                 sLabel = sLabel->next )
            {
                if( idlOS::strMatch(
                        //aStatement->stmtText + sLabel->namePos.offset,
                        sRealSQL + sLabel->namePos.offset,
                        sLabel->namePos.size,
                        sNode->userName.stmtText + sNode->userName.offset,
                        sNode->userName.size) == 0)
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &sNode->tableName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                            ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                        {
                            IDE_TEST( searchFieldTypeOfRecord(
                                          sFoundVariable,
                                          &sNode->columnName,
                                          aIsFound,
                                          QC_SHARED_TMPLATE(aStatement),
                                          aColumn )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
        }
    }

    // 여기까지 못찾았으면 parameter검색
    if( *aIsFound == ID_FALSE )
    {
        if( QC_IS_NULL_NAME(sNode->userName) == ID_TRUE )
        {
            if( QC_IS_NULL_NAME(sNode->tableName) == ID_TRUE )
            {
                // procedureName이 없는 경우 무조건 parameter 라고 가정.
                // (5) parameter_name
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &sNode->columnName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    *aColumn = QC_SHARED_TMPLATE(aStatement)->tmplate
                        .rows[sFoundVariable->variableTypeNode->node.table]
                        .columns
                        + sFoundVariable->variableTypeNode->node.column;
                }
            }
            else
            {
                // (6) parameter_name.field_name
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &sNode->tableName,
                                               aIsFound,
                                               &sFoundVariable)
                          != IDE_SUCCESS);

                if ( *aIsFound == ID_TRUE )
                {
                    if( sFoundVariable->variableType == QS_ROW_TYPE ||
                        sFoundVariable->variableType == QS_RECORD_TYPE )
                    {
                        IDE_TEST( searchFieldTypeOfRecord(
                                      sFoundVariable,
                                      &sNode->columnName,
                                      aIsFound,
                                      QC_SHARED_TMPLATE(aStatement),
                                      aColumn )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                }

                if( *aIsFound == ID_FALSE )
                {
                    // (7) procedure_name.parameter_name
                    if( idlOS::strMatch(
                            sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                            sParseTree->procNamePos.size,
                            sNode->tableName.stmtText + sNode->tableName.offset,
                            sNode->tableName.size) == 0)
                    {
                        IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                                       sCurrDeclItem,
                                                       &sNode->columnName,
                                                       aIsFound,
                                                       &sFoundVariable )
                                  != IDE_SUCCESS );

                        if( *aIsFound == ID_TRUE )
                        {
                            *aColumn = QC_SHARED_TMPLATE(aStatement)->tmplate
                                .rows[sFoundVariable->variableTypeNode->node.table]
                                .columns
                                + sFoundVariable->variableTypeNode->node.column;
                        }
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
            // (8) procedure_name.parameter_name.field_name
            if( idlOS::strMatch(
                    sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                    sParseTree->procNamePos.size,
                    sNode->userName.stmtText + sNode->userName.offset,
                    sNode->userName.size) == 0)
            {
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &sNode->tableName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                        ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                    {
                        IDE_TEST( searchFieldTypeOfRecord(
                                      sFoundVariable,
                                      &sNode->columnName,
                                      aIsFound,
                                      QC_SHARED_TMPLATE(aStatement),
                                      aColumn )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            } // if ( idlOS::strMatch( ...
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

    IDE_EXCEPTION_CONT( PACKAGE_VARIABLE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::searchArrayVar( qcStatement  * aStatement,
                                   qtcNode      * aArrNode,
                                   idBool       * aIsFound,
                                   qsVariables ** aFoundVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 Array Variable 검색.
 *               member function estimate시 사용.
 *
 * Implementation :
 *               다음과 같은 경우만 존재.
 *               (1) var_name
 *               (2) param_name
 *               (3) label_name.var_name
 *               (4) proc_name.param_name
 *               변수 검색으로 끝이 아니라 반드시 array type변수인지
 *               체크해 주어야 한다.
 *
 ***********************************************************************/

    qsProcParseTree     * sParseTree;
    qsAllVariables      * sCurrVar;
    qsVariableItems     * sCurrDeclItem;
    qsLabels            * sLabel;
    SChar               * sRealSQL = qsvProcVar::getValidSQL( aStatement );

    sParseTree    = aStatement->spvEnv->createProc;

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    *aIsFound = ID_FALSE;
    *aFoundVariable = NULL;

    // 적합성 검사. varName은 반드시 있어야 함.
    IDE_DASSERT( QC_IS_NULL_NAME( aArrNode->tableName ) == ID_FALSE );

    if ( sParseTree != NULL )
    {
        if( QC_IS_NULL_NAME( aArrNode->userName ) == ID_TRUE )
        {
            // (1) var_name 의 경우.
            for (sCurrVar = aStatement->spvEnv->allVariables;
                 sCurrVar != NULL && *aIsFound == ID_FALSE;
                 sCurrVar = sCurrVar->next)
            {
                IDE_TEST( searchVariableItems( sCurrVar->variableItems,
                                               sCurrDeclItem,
                                               &aArrNode->tableName,
                                               aIsFound,
                                               aFoundVariable)
                          != IDE_SUCCESS);
            }

            if( *aIsFound == ID_FALSE )
            {
                // (2) param_name 의 경우.
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aArrNode->tableName,
                                               aIsFound,
                                               aFoundVariable )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // (3) label_name.var_name 의 경우.
            for (sCurrVar = aStatement->spvEnv->allVariables;
                 sCurrVar != NULL && *aIsFound == ID_FALSE;
                 sCurrVar = sCurrVar->next)
            {
                for (sLabel = sCurrVar->labels;
                     sLabel != NULL && *aIsFound == ID_FALSE;
                     sLabel = sLabel->next)
                {
                    if (idlOS::strMatch(
                            sRealSQL + sLabel->namePos.offset,
                            sLabel->namePos.size,
                            sRealSQL + aArrNode->userName.offset,
                            aArrNode->userName.size) == 0)
                    {
                        IDE_TEST( searchVariableItems( sCurrVar->variableItems,
                                                       sCurrDeclItem,
                                                       &aArrNode->tableName,
                                                       aIsFound,
                                                       aFoundVariable)
                                  != IDE_SUCCESS);
                    }
                } // for .. label
            } // for .. allVariables

            if( *aIsFound == ID_FALSE )
            {
                // (4) proc_name.param_name 의 경우.
                if (idlOS::strMatch(
                        sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                        sParseTree->procNamePos.size,
                        sRealSQL + aArrNode->userName.offset,
                        aArrNode->userName.size) == 0)
                {
                    IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                                   sCurrDeclItem,
                                                   &aArrNode->tableName,
                                                   aIsFound,
                                                   aFoundVariable )
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
        }

        /* BUG-38243 set objectID */
        if ( *aIsFound == ID_TRUE )
        {
            (*aFoundVariable)->common.objectID = QS_EMPTY_OID;
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

    /* BUG-38243
       subprogram 에서 array tpye형 varirable을 찾지 못 했으면,
       package local에서 array type형 variable을 찾아봐야 한다.
       package의 경우, package body에서 먼저 찾고, 그 후 package spec에서 찾는다.
       만약, local에서 찾지 못 했으면,
       다른 package spec에 있는 array type형 variable을 찾는다. */
    if ( (*aIsFound == ID_FALSE) &&
         (aStatement->spvEnv->createPkg != NULL) )
    {
        IDE_TEST( searchPkgLocalArrayVar( aStatement,
                                          aArrNode,
                                          aIsFound,
                                          aFoundVariable )
                  != IDE_SUCCESS );
    }
    else
    {
        /* procedure 객체일 경우는,
           package local에 대한 탐색을 할 필요가 없다. */
        // Nothing to do.
    }

    if ( *aIsFound == ID_FALSE )
    {
        IDE_TEST( searchPkgArrayVar( aStatement,
                                     aArrNode,
                                     aIsFound,
                                     aFoundVariable )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // 마지막으로 array type 변수인지 검사.
    if( *aIsFound == ID_TRUE )
    {
        if( (*aFoundVariable)->variableType !=
            QS_ASSOCIATIVE_ARRAY_TYPE )
        {
            // 만약 array type이 아니라면 못찾은걸로 바꾼다.
            *aIsFound = ID_FALSE;
            *aFoundVariable = NULL;
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

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}


IDE_RC qsvProcVar::searchVariableItems(
    qsVariableItems * aVariableItems,
    qsVariableItems * aCurrDeclItem,
    qcNamePosition  * aVarName,
    idBool          * aIsFound,
    qsVariables    ** aFoundVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 variable 검색.
 *
 * Implementation :
 *       (1) aVariable은 NULL이 오는 경우와 아닌 경우로 나뉨
 *       (2) NULL인 경우 : block내 변수 검색용
 *       (3) NULL이 아닌 경우 : 변수 선언시 변수 검색용
 *
 ***********************************************************************/

    qsVariableItems     * sVariableItem;

    IDU_FIT_POINT_FATAL( "qsvProcVar::searchVariableItems::__FT__" );

    *aIsFound = ID_FALSE;

    for ( sVariableItem = aVariableItems;
          ( sVariableItem != NULL ) &&
              ( sVariableItem != aCurrDeclItem );
          sVariableItem = sVariableItem->next)
    {
        if ( ( sVariableItem->itemType == QS_VARIABLE ) ||
             ( sVariableItem->itemType == QS_TRIGGER_NEW_VARIABLE ) ||
             ( sVariableItem->itemType == QS_TRIGGER_OLD_VARIABLE ) )
        {
            if (idlOS::strMatch(
                    sVariableItem->name.stmtText + sVariableItem->name.offset,
                    sVariableItem->name.size,
                    aVarName->stmtText + aVarName->offset,
                    aVarName->size) == 0)
            {
                *aFoundVariable = (qsVariables *)sVariableItem;

                *aIsFound = ID_TRUE;

                break;
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC qsvProcVar::searchFieldTypeOfRecord(
    qsVariables     * aRecordVariable,
    qcNamePosition  * aFieldName,
    idBool          * aIsFound,
    qcTemplate      * aTemplate,
    mtcColumn      ** aColumn )
{
/***********************************************************************
 *
 * Description : PROJ-1075 record variable에서 field type을 추출.
 *               mtcColumn을 가져온다.
 *
 * Implementation :
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::searchFieldTypeOfRecord"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qcmColumn       * sCurrColumn;
    qsTypes         * sType;
    UShort            sColOrder;
    qtcNode         * sNode;
    UShort            i;
    sType = aRecordVariable->typeInfo;

    *aIsFound = ID_FALSE;

    IDE_DASSERT( sType != NULL );

    for( sCurrColumn = sType->columns,
             sColOrder = 0;
         sCurrColumn != NULL;
         sCurrColumn = sCurrColumn->next,
             sColOrder++)
    {
        if ( QC_IS_STR_CASELESS_MATCHED( (*aFieldName), sCurrColumn->name ) )
        {
            // BUG-15414
            // record variable에 중복된 alias name이 존재하더라도
            // 찾고자하는 column name에만 중복이 없으면 된다.
            IDE_TEST_RAISE( *aIsFound == ID_TRUE,
                            ERR_DUP_ALIAS_NAME );

            sNode = (qtcNode *)aRecordVariable->variableTypeNode->node.arguments;
            for (i = 0; i < sColOrder; i++)
            {
                sNode = (qtcNode *)sNode->node.next;
            }

            *aColumn = aTemplate->tmplate
                .rows[sNode->node.table]
                .columns
                + sNode->node.column;

            *aIsFound = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUP_ALIAS_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME));
    }
    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;

#undef IDE_FN
}


IDE_RC qsvProcVar::searchCursor(
    qcStatement     * aStatement,
    qtcNode         * aNode,
    idBool          * aIsFound,
    qsCursors      ** aCursorDef)
{
#define IDE_FN "qsvProcVar::searchCursor"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY("qsvProcVar::searchCursor"));

    IDE_RC sRC;

    sRC = qsvCursor::getCursorDefinition(aStatement, aNode, aCursorDef);

    if (sRC == IDE_SUCCESS)
    {
        // To fix BUG-14279
        // return value가 IDE_SUCCESS라도 cursorDef가 없는 경우가 존재.
        // SQL% .. 시리즈를 검색하는 경우는 결과는 SUCCESS이지만 cursorDef
        // 는 없는 상태가 됨
        // ex) SQL%ROWCOUNT , SQL% ... 시리즈들
        if( *aCursorDef == NULL )
        {
            *aIsFound = ID_FALSE;
        }
        else
        {
            *aIsFound = ID_TRUE;
        }
    }
    else
    {
        IDE_CLEAR();
        *aIsFound = ID_FALSE;
    }

    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qsvProcVar::searchFieldOfRecord(
    qcStatement     * aStatement,
    qsTypes         * aType,
    qtcNode         * aRecordVarNode,
    qtcNode         * aVariable,
    idBool            aSearchForLValue,
    idBool          * aIsFound)
{

    qcmColumn       * sCurrColumn;
    UShort            sColOrder;
    qtcNode         * sNode;
    UShort            i;
    qcNamePosition    sFieldName;
    qcuSqlSourceInfo  sqlInfo;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qsvProcVar::searchFieldOfRecord::__FT__" );

    *aIsFound = ID_FALSE;

    IDE_DASSERT( aType != NULL );

    /********************************************************
     *  No | userName | tableName | columnName | pkgName
     * ------------------------------------------------
     * (1) |          |   package | subprogram |
     * (2) |     user |   package | subprogram |
     * (3) |          |   package |   variable |
     * (4) |  package |    record |      field |
     * (5) |     user |   package |   variable |
     * (6) |     user |   package |     record | field
     * (7) |          |    record |      field |
     * (8) |          |           |     record | field
     *
     * 위의 표에서 보듯, field에 대한 정보는
     * columnName, pkgName에 존재한다.
     * 그 중 4개의 Name이 모두 NULL이 아닐 때 pkgName은
     * 확실히 field의 정보가 존재한다.
     *********************************************************/

    if( QC_IS_NULL_NAME( aVariable->pkgName ) == ID_TRUE )
    {
        sFieldName = aVariable->columnName;
    }
    else
    {
        sFieldName = aVariable->pkgName;
    }

    for( sCurrColumn = aType->columns,
             sColOrder = 0;
         sCurrColumn != NULL;
         sCurrColumn = sCurrColumn->next,
             sColOrder++)
    {
        if ( QC_IS_STR_CASELESS_MATCHED( sFieldName, sCurrColumn->name ) )
        {
            // BUG-15414
            // record variable에 중복된 alias name이 존재하더라도
            // 찾고자하는 column name에만 중복이 없으면 된다.
            IDE_TEST_RAISE( *aIsFound == ID_TRUE,
                            ERR_DUP_ALIAS_NAME );

            sNode = (qtcNode *)aRecordVarNode->node.arguments;
            for (i = 0; i < sColOrder; i++)
            {
                sNode = (qtcNode *)sNode->node.next;
            }

            aVariable->node.table = sNode->node.table;
            aVariable->node.column = sNode->node.column;
            aVariable->node.objectID = sNode->node.objectID;

            sNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
            sNode->lflag |= QTC_NODE_VALIDATE_TRUE;

            // To fix BUG-12622
            // output으로 사용된 컬럼에 LVALUE MASK를 씌움.
            if( aSearchForLValue == ID_TRUE )
            {
                sNode->lflag &= ~QTC_NODE_LVALUE_MASK;
                sNode->lflag |= QTC_NODE_LVALUE_ENABLE;
            }
            else
            {
                // Nothing to do.
            }

            *aIsFound = ID_TRUE;
        }
        else
        {
            // Nothing to do.
        }
    }

    /* BUG-39194
       table, record 및 array type의 variable은 있으나,
       table, record 및 array에 속한 field(또는 column)를 찾지 못하면
       column not found error를 내준다. */
    if ( *aIsFound == ID_FALSE )
    {
        sqlInfo.setSourceInfo( aStatement,
                               & sFieldName );

        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
    }
    else
    {
        // Nohting to do.
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_DUP_ALIAS_NAME );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QDB_DUPLICATE_COLUMN_NAME));
    }
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    *aIsFound = ID_FALSE;

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::makeUDTVariable( qcStatement * aStatement,
                                    qsVariables * aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 UDT variable을 생성
 *
 * Implementation :
 *         (1) type check.
 *         (2) 검색된 type분류별로 변수를 생성.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::makeUDTVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qsTypes           * sType;
    qtcNode           * sTypeNode;

    sTypeNode = aVariable->variableTypeNode;

    IDE_TEST( qsvProcType::checkTypes( aStatement,
                                       (qsVariableItems*)aVariable,
                                       &sTypeNode->userName,
                                       &sTypeNode->tableName,
                                       &sTypeNode->columnName,
                                       &sType )
              != IDE_SUCCESS );

    switch( sType->variableType )
    {
        case QS_RECORD_TYPE:
        {
            IDE_TEST( makeRecordVariable( aStatement,
                                          sType,
                                          aVariable )
                      != IDE_SUCCESS );
        }
        break;

        case QS_ASSOCIATIVE_ARRAY_TYPE:
        {
            IDE_TEST( makeArrayVariable( aStatement,
                                         sType,
                                         aVariable )
                      != IDE_SUCCESS );
        }
        break;

        case QS_REF_CURSOR_TYPE:
        {
            if( ( aStatement->spvEnv->createPkg != NULL ) &&
                ( aStatement->spvEnv->createProc == NULL ) )
            {
                IDE_RAISE( ERR_NOT_DECLARED_REF_CURSOR_VARIABLE );
            }
            else
            {
                IDE_TEST( makeRefCurVariable( aStatement,
                                              sType,
                                              aVariable )
                          != IDE_SUCCESS );
            }
        }
        break;

        default:
        {
            IDE_DASSERT(0);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_DECLARED_REF_CURSOR_VARIABLE )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_CANNOT_DECLARE_REF_CURSOR_VARIABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::makeRecordVariable( qcStatement * aStatement,
                                       qsTypes     * aType,
                                       qsVariables * aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 record/row type 변수를 type정보를 이용하여  생성.
 *
 * Implementation :
 *        (1) makeArgumentsForRowTypeNode함수를 호출하여 변수 생성.
 *        (2) 변수의 타입 정보 및 table, column 세팅.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::makeRecordVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    qtcNode         * sTypeNode;

    sTypeNode = aVariable->variableTypeNode;

    IDE_TEST( makeArgumentsForRowTypeNode( aStatement,
                                           aType,
                                           sTypeNode,
                                           aVariable->common.table )
              != IDE_SUCCESS );

    //variable정보 조정
    aVariable->typeInfo = aType;
    aVariable->variableType = aType->variableType;

    aVariable->common.table = sTypeNode->node.table;
    aVariable->common.column = sTypeNode->node.column;
    aVariable->common.objectID = sTypeNode->node.objectID;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::makeArrayVariable(
    qcStatement     * aStatement,
    qsTypes         * aType,
    qsVariables     * aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 array 변수 생성
 *
 * Implementation :
 *           (1) Lob Column이 너무 크면 Row 할당
 *           (2) array변수 생성
 *           (3) 대응되는 row 변수 생성
 *           (4) 만약 row변수가 record type이라면 확장
 *
 *         * array type 변수의 구성.
 *           (array)
 *             |
 *           (row)  [tuple_1]
 *             |                row와 col은 동일 tuple내에 있음.
 *           (col1) - (col2) - (col3) [tuple_1]
 *
 *           만약 row가 record type이라면 columnNode까지 구성함.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::makeArrayVariable"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UShort            sTable;
    qcTemplate      * sQcTemplate;
    mtcTemplate     * sMtcTemplate;
    mtcTuple        * sMtcTuple;
    qtcNode         * sTypeNode;
    qtcNode         * sRowNode;
    qcmColumn       * sRowColumn;
    qtcModule       * sRowModule;
    qcmColumn       * sColumnList;
    UShort            sCurrRowID;
    UInt              sColumnIndex;
    UInt              sOffset;
    UInt              sNewSize;
    idBool            sIsLobFound = ID_FALSE;
    qcuSqlSourceInfo  sqlInfo;

    sQcTemplate = QC_SHARED_TMPLATE(aStatement);
    sMtcTemplate = &(sQcTemplate->tmplate);
    sTypeNode = aVariable->variableTypeNode;

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
     * Intermediate Tuple Row가 있고 비어 있지 않은 상태에서,
     * Intermediate Tuple Row에 Lob Column을 할당할 때,
     * (Old Offset + New Size) > Property 이면,
     * 새로운 Intermediate Tuple Row를 할당한다.
     */
    if( sQcTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE] != ID_USHORT_MAX )
    {
        sCurrRowID = sQcTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE];

        sMtcTuple = &(sQcTemplate->tmplate.rows[sCurrRowID]);
        if ( sMtcTuple->columnCount != 0 )
        {
            for ( sColumnList = aType->columns;
                  sColumnList != NULL;
                  sColumnList = sColumnList->next )
            {
                if ( ( sColumnList->basicInfo->type.dataTypeId == MTD_BLOB_ID ) ||
                     ( sColumnList->basicInfo->type.dataTypeId == MTD_CLOB_ID ) )
                {
                    sIsLobFound = ID_TRUE;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sIsLobFound == ID_TRUE )
            {
                for( sColumnIndex = 0, sOffset = 0;
                     sColumnIndex < sMtcTuple->columnCount;
                     sColumnIndex++ )
                {
                    if ( sMtcTuple->columns[sColumnIndex].module != NULL )
                    {
                        sOffset = idlOS::align( sOffset,
                                                sMtcTuple->columns[sColumnIndex].module->align );
                        sOffset += sMtcTuple->columns[sColumnIndex].column.size;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                for ( sColumnList = aType->columns, sNewSize = 0;
                      sColumnList != NULL;
                      sColumnList = sColumnList->next )
                {
                    sNewSize += sColumnList->basicInfo->column.size;
                }

                if ( (sOffset + sNewSize) > QCU_INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT )
                {
                    IDE_TEST( qtc::nextRow( QC_QMP_MEM(aStatement),
                                            aStatement,
                                            sQcTemplate,
                                            MTC_TUPLE_TYPE_INTERMEDIATE )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    // (2) array 변수 생성
    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               sTypeNode,
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn(
                  sMtcTemplate->rows[sTypeNode->node.table].columns
                  + sTypeNode->node.column,
                  (mtdModule*)aType->typeModule,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                sTypeNode )
              != IDE_SUCCESS );

    // 첫번째 컬럼은 index 컬럼이므로 next가 row컬럼이다.
    sRowColumn = aType->columns->next;

    // (3) 대응되는 row 변수 생성.
    // array 에 대응되는 rowNode를 생성.
    IDE_TEST( qtc::makeInternalColumn(
                  aStatement, 0, 0, &sRowNode )
              != IDE_SUCCESS );

    // rowNode를 arrayNode의 arguments에 연결.
    // (array)
    //   |
    // (row)
    sTypeNode->node.arguments = (mtcNode*)sRowNode;


    if( sRowColumn->basicInfo->module->id < MTD_UDT_ID_MIN ||
        sRowColumn->basicInfo->module->id > MTD_UDT_ID_MAX )
    {
        // rowColumn이 primitive data type인 경우.
        // primitive data type은 column Node가 없다.
        // (array)
        //   |
        // (row)     이 구조가 최종 상태

        // tuple을 통째로 하나 할당.
        IDE_TEST( qtc::nextTable( &(sTable),
                                  aStatement,
                                  NULL,
                                  ID_TRUE,
                                  MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS);

        // 해당 tuple의 첫번째 컬럼이 rowColumn이다.
        sRowNode->node.table = sTable;
        sRowNode->node.column = 0;
        sRowNode->node.objectID = sTypeNode->node.objectID; 

        // alloc mtcColumn, mtcExecute
        IDE_TEST(qtc::allocIntermediateTuple( aStatement,
                                              sMtcTemplate,
                                              sTable,
                                              1 )
                 != IDE_SUCCESS);

        mtc::copyColumn( &(sMtcTemplate->rows[sTable].columns[0]),
                         sRowColumn->basicInfo);

        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
         * LOB Column을 LOB Value로 변환한다.
         */
        if ( ( sRowColumn->basicInfo->type.dataTypeId == MTD_BLOB_ID ) ||
             ( sRowColumn->basicInfo->type.dataTypeId == MTD_CLOB_ID ) )
        {
            IDE_TEST( mtc::initializeColumn( &(sMtcTemplate->rows[sTable].columns[0]),
                                             sRowColumn->basicInfo->type.dataTypeId,
                                             0,
                                             0,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2002 Column Security
            if ( (sMtcTemplate->rows[sTable].columns[0].module->flag
                  & MTD_ENCRYPT_TYPE_MASK)
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                IDE_TEST( qtc::changeColumn4Decrypt(
                              &(sMtcTemplate->rows[sTable].columns[0]) )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }

        // set execute
        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sRowNode )
                  != IDE_SUCCESS );

        // tuple offset 조정.
        qtc::resetTupleOffset( sMtcTemplate,
                               sTable );

        // nothing to do for tuple
        sMtcTemplate->rows[sTable].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
        sMtcTemplate->rows[sTable].lflag |= MTC_TUPLE_ROW_SKIP_TRUE;
    }
    else
    {
        // (4) 만약 row변수가 record type이라면 확장
        // record type은 column Node가 있다.
        // (array)
        //   |
        // (row)
        //   |
        // (col1) - (col2) - (col3) - ... 이 구조가 최종 상태.
        IDE_TEST_RAISE( ( sRowColumn->basicInfo->module->id != MTD_RECORDTYPE_ID ) &&
                        ( sRowColumn->basicInfo->module->id != MTD_ROWTYPE_ID ) &&
                        ( sRowColumn->basicInfo->module->id != MTD_ASSOCIATIVE_ARRAY_ID ) ,
                        err_unsupported_array_element_type );

        sRowModule = (qtcModule*)sRowColumn->basicInfo->module;

        // tuple을 할당받고 rowNode에 columnNode를 붙이고
        // column execute 세팅.
        IDE_TEST( makeArgumentsForRowTypeNode( aStatement,
                                               sRowModule->typeInfo,
                                               sRowNode,
                                               ID_USHORT_MAX ) // table
                  != IDE_SUCCESS );
    }

    // variable 정보 조정.
    aVariable->typeInfo = aType;
    aVariable->variableType = aType->variableType;

    aVariable->common.table = sTypeNode->node.table;
    aVariable->common.column = sTypeNode->node.column;
    aVariable->common.objectID = sTypeNode->node.objectID;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_unsupported_array_element_type );
    {
        sqlInfo.setSourceInfo( aStatement, &sRowColumn->namePos );
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSV_UNSUPPORTED_ARRAY_ELEMENT_TYPE,
                                  sqlInfo.getErrMessage() ) );
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsvProcVar::makeRefCurVariable(
    qcStatement     * aStatement,
    qsTypes         * aType,
    qsVariables     * aVariable )
{
/***********************************************************************
 *
 * Description : PROJ-1386 ref cursor 변수 생성
 *
 * Implementation :
 *
 ***********************************************************************/
    qcTemplate      * sQcTemplate;
    mtcTemplate     * sMtcTemplate;
    mtcTuple        * sMtcTuple;
    qtcNode         * sTypeNode;
    qcmColumn       * sColumnList;
    UShort            sCurrRowID;
    UInt              sColumnIndex;
    UInt              sOffset;
    UInt              sNewSize;
    idBool            sIsLobFound = ID_FALSE;

    sQcTemplate = QC_SHARED_TMPLATE(aStatement);
    sMtcTemplate = &(sQcTemplate->tmplate);
    sTypeNode = aVariable->variableTypeNode;

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
     * Intermediate Tuple Row가 있고 비어 있지 않은 상태에서,
     * Intermediate Tuple Row에 Lob Column을 할당할 때,
     * (Old Offset + New Size) > Property 이면,
     * 새로운 Intermediate Tuple Row를 할당한다.
     */
    if( sQcTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE] != ID_USHORT_MAX )
    {
        sCurrRowID = sQcTemplate->tmplate.currentRow[MTC_TUPLE_TYPE_INTERMEDIATE];

        sMtcTuple = &(sQcTemplate->tmplate.rows[sCurrRowID]);
        if ( sMtcTuple->columnCount != 0 )
        {
            for ( sColumnList = aType->columns;
                  sColumnList != NULL;
                  sColumnList = sColumnList->next )
            {
                if ( ( sColumnList->basicInfo->type.dataTypeId == MTD_BLOB_ID ) ||
                     ( sColumnList->basicInfo->type.dataTypeId == MTD_CLOB_ID ) )
                {
                    sIsLobFound = ID_TRUE;
                    break;
                }
                else
                {
                    /* Nothing to do */
                }
            }

            if ( sIsLobFound == ID_TRUE )
            {
                for( sColumnIndex = 0, sOffset = 0;
                     sColumnIndex < sMtcTuple->columnCount;
                     sColumnIndex++ )
                {
                    if ( sMtcTuple->columns[sColumnIndex].module != NULL )
                    {
                        sOffset = idlOS::align( sOffset,
                                                sMtcTuple->columns[sColumnIndex].module->align );
                        sOffset += sMtcTuple->columns[sColumnIndex].column.size;
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                }

                for ( sColumnList = aType->columns, sNewSize = 0;
                      sColumnList != NULL;
                      sColumnList = sColumnList->next )
                {
                    sNewSize += sColumnList->basicInfo->column.size;
                }

                if ( (sOffset + sNewSize) > QCU_INTERMEDIATE_TUPLE_LOB_OBJECT_LIMIT )
                {
                    IDE_TEST( qtc::nextRow( QC_QMP_MEM(aStatement),
                                            aStatement,
                                            sQcTemplate,
                                            MTC_TUPLE_TYPE_INTERMEDIATE )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( qtc::nextColumn( QC_QMP_MEM(aStatement),
                               sTypeNode,
                               aStatement,
                               QC_SHARED_TMPLATE(aStatement),
                               MTC_TUPLE_TYPE_INTERMEDIATE,
                               1 )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn(
                  sMtcTemplate->rows[sTypeNode->node.table].columns
                  + sTypeNode->node.column,
                  (mtdModule*)aType->typeModule,
                  0,
                  0,
                  0 )
              != IDE_SUCCESS );

    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement, 
                                                sTypeNode )
              != IDE_SUCCESS );

    // variable 정보 조정.
    aVariable->typeInfo = aType;
    aVariable->variableType = aType->variableType;

    aVariable->common.table    = sTypeNode->node.table;
    aVariable->common.column   = sTypeNode->node.column;
    aVariable->common.objectID = sTypeNode->node.objectID;

    // BUG-38767
    // resIdx는 array type에서 사용했으나, cursor type은 sqlIdx로 사용한다.
    if ( aStatement->spvEnv->createProc != NULL )
    {
        aVariable->resIdx = aStatement->spvEnv->createProc->procSqlCount++;
    }
    else
    {
        aVariable->resIdx = ID_UINT_MAX;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC qsvProcVar::makeArgumentsForRowTypeNode( qcStatement * aStatement,
                                                qsTypes     * aType,
                                                qtcNode     * aRowNode,
                                                UShort        aTable )
{
/***********************************************************************
 *
 * Description : PROJ-1075 record/row type node를 type정보를 이용하여 생성.
 *
 * Implementation :
 *           (1) tuple을 하나 할당받음
 *           (2) 첫번째는 자기 자신, 두번째이후로 내부컬럼.
 *           (3) [rowtype][col1][col2][col3] 형태로 구성.
 *                  |
 *               col1,col2, col3의 size, offet을 포괄함.
 *
 ***********************************************************************/
#define IDE_FN "qsvProcVar::makeArgumentsForRowTypeNode"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UShort            sTable;
    UShort            sColumn;
    qtcNode         * sPrevNode = NULL;
    qtcNode         * sCurrNode;
    mtcTemplate     * sMtcTemplate;
    qcmColumn       * sColumnDef;

    sMtcTemplate = & QC_SHARED_TMPLATE(aStatement)->tmplate;

    // row 타입 변수를 리빌드 하는 경우 기존의 tuple을 그대로 사용해야 한다.
    if( aTable == ID_USHORT_MAX )
    {
        // tuple을 통째로 하나 할당.
        IDE_TEST( qtc::nextTable( &(sTable),
                                  aStatement,
                                  NULL,
                                  ID_TRUE,
                                  MTC_COLUMN_NOTNULL_TRUE )
                  != IDE_SUCCESS); // PR-13597
    }
    else
    {
        sTable = aRowNode->node.table;
    }

    // 해당 tuple의 첫번째 컬럼이 record type자신이 된다.
    aRowNode->node.table = sTable;
    aRowNode->node.column = 0;
    aRowNode->node.objectID = aType->common.objectID; 

    // 두번째 컬럼부터는 내부 컬럼으로 세팅
    for( sColumn = 1; sColumn < aType->columnCount + 1; sColumn++ )
    {
        // make node for one field
        IDE_TEST( qtc::makeInternalColumn(
                      aStatement, sTable, sColumn, &sCurrNode )
                  != IDE_SUCCESS );

        // connect
        if( sColumn == 1 )
        {
            aRowNode->node.arguments = (mtcNode *)sCurrNode;
            sPrevNode                 = sCurrNode;
        }
        else
        {
            sPrevNode->node.next      = (mtcNode *)sCurrNode;
            sPrevNode                 = sCurrNode;
        }
    }

    // alloc mtcColumn, mtcExecute
    IDE_TEST( qtc::allocIntermediateTuple( aStatement,
                                           sMtcTemplate,
                                           sTable,
                                           sColumn )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn(
                  sMtcTemplate->rows[aRowNode->node.table].columns
                  + aRowNode->node.column,
                  (mtdModule*)aType->typeModule,
                  1,
                  aType->typeSize,
                  0 )
              != IDE_SUCCESS );

    // row/record type variable이므로,
    // offset을 0으로 초기화 해준다.
    sMtcTemplate->rows[aRowNode->node.table].columns->column.offset = 0;


    IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                aRowNode )
              != IDE_SUCCESS );

    // set mtcColumn, mtcExecute
    for( sColumn = 1,
             sCurrNode = (qtcNode *)aRowNode->node.arguments,
             sColumnDef = aType->columns;
         sColumn < sMtcTemplate->rows[sTable].columnCount;
         sColumn++,
             sCurrNode = (qtcNode *)sCurrNode->node.next,
             sColumnDef = sColumnDef->next)
    {
        // copy size, type, module
        mtc::copyColumn( &(sMtcTemplate->rows[sTable].columns[sColumn]),
                         sColumnDef->basicInfo);

        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
         * LOB Column을 LOB Value로 변환한다.
         */
        if ( ( sColumnDef->basicInfo->type.dataTypeId == MTD_BLOB_ID ) ||
             ( sColumnDef->basicInfo->type.dataTypeId == MTD_CLOB_ID ) )
        {
            IDE_TEST( mtc::initializeColumn( &(sMtcTemplate->rows[sTable].columns[sColumn]),
                                             sColumnDef->basicInfo->type.dataTypeId,
                                             0,
                                             0,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-2002 Column Security
            if ( ( sMtcTemplate->rows[sTable].columns[sColumn].module->flag
                   & MTD_ENCRYPT_TYPE_MASK )
                 == MTD_ENCRYPT_TYPE_TRUE )
            {
                IDE_TEST( qtc::changeColumn4Decrypt(
                              & sMtcTemplate->rows[sTable].columns[sColumn] )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
        }

        // set execute
        IDE_TEST( qtc::estimateNodeWithoutArgument( aStatement,
                                                    sCurrNode )
                  != IDE_SUCCESS );
    }

    // rowoffset 조정
    // 추후 cloneTemplate할때 공간을 할당받는다.
    sMtcTemplate->rows[sTable].rowOffset =
        sMtcTemplate->rows[sTable].rowMaximum= aType->typeSize;

    // nothing to do for tuple
    sMtcTemplate->rows[sTable].lflag &= ~MTC_TUPLE_ROW_SKIP_MASK;
    sMtcTemplate->rows[sTable].lflag |= MTC_TUPLE_ROW_SKIP_TRUE;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

SChar *
qsvProcVar::getValidSQL( qcStatement * aStatement )
{
    SChar * sSQL = NULL;

    if( aStatement->spvEnv->createProc == NULL )
    {
        // BUG-37364
        if( aStatement->spvEnv->createPkg == NULL )
        {
            sSQL = aStatement->myPlan->stmtText;
        }
        else
        {
            sSQL = aStatement->spvEnv->createPkg->stmtText;
        }
    }
    else
    {
        sSQL = aStatement->spvEnv->createProc->stmtText;
    }

    return sSQL;
}

// PROJ-1073 Package
IDE_RC qsvProcVar::checkPkgVarType( qcStatement  * aStatement,
                                    qsVariables  * aVariable,
                                    idBool       * aValidVariable,
                                    mtcColumn   ** aColumn )
{
    qtcNode        * sNode           = aVariable->variableTypeNode;
    qsOID            sPkgOID;
    UInt             sPkgUserID;
    qsxPkgInfo     * sPkgInfo;
    idBool           sExists         = ID_FALSE;
    mtcColumn      * sColumn         = NULL;
    qcmSynonymInfo   sSynonymInfo;

    IDE_TEST( qcmSynonym::resolvePkg( aStatement,
                                      sNode->userName,
                                      sNode->tableName,
                                      &sPkgOID,
                                      &sPkgUserID,
                                      &sExists,
                                      &sSynonymInfo )
              != IDE_SUCCESS );

    if( sExists == ID_TRUE )
    {
        IDE_TEST( qsvPkgStmts::makePkgSynonymList( aStatement,
                                                   &sSynonymInfo,
                                                   sNode->userName,
                                                   sNode->tableName,
                                                   sPkgOID )
                  != IDE_SUCCESS );

        IDE_TEST( qsvPkgStmts::makeRelatedObjects( aStatement,
                                                   & sNode->userName,
                                                   & sNode->tableName,
                                                   & sSynonymInfo,
                                                   0,
                                                   QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree( aStatement,
                                                          sPkgOID,
                                                          QS_PKG,
                                                          &(aStatement->spvEnv->procPlanList) )
                  != IDE_SUCCESS );

        IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );

        /* BUG-45164 */
        IDE_TEST_RAISE( sPkgInfo->isValid != ID_TRUE, err_object_invalid );

        IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aStatement,
                                                   sPkgInfo->planTree->userID,
                                                   sPkgInfo->privilegeCount,
                                                   sPkgInfo->granteeID,
                                                   ID_FALSE,
                                                   NULL,
                                                   NULL )
                  != IDE_SUCCESS );

        IDE_TEST( searchVarFromPkg( aStatement,
                                    sPkgInfo,
                                    aVariable,
                                    aValidVariable,
                                    &sColumn )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    // BUG-40727 올바른 table, column, objectID는 이후에 설정한다.
    sNode->node.table    = 0;
    sNode->node.column   = 0;
    sNode->node.objectID = 0;

    *aColumn = sColumn;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */ 
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalVarTypeInternal( qcStatement     * aStatement,
                                                  qsPkgParseTree  * aPkgParseTree,
                                                  qtcNode         * aVarNode,
                                                  qcTemplate      * aTemplate,
                                                  idBool          * aIsFound,
                                                  mtcColumn      ** aColumn )
{
    qsVariableItems * sCurrDeclItem  = NULL;
    qsVariables     * sFoundVariable = NULL;
    UInt              sUserID        = QC_EMPTY_USER_ID;

    sCurrDeclItem = aStatement->spvEnv->currDeclItem;
    *aIsFound     = ID_FALSE;
    *aColumn      = NULL;

    IDE_DASSERT( QC_IS_NULL_NAME( aVarNode->columnName ) == ID_FALSE );

    if( QC_IS_NULL_NAME ( aVarNode->pkgName ) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE )
        {
            if( QC_IS_NULL_NAME( aVarNode->tableName ) == ID_TRUE )
            {
                // ( 1 ) variable_name / parameter name
                IDE_TEST( searchVariableItems( aPkgParseTree->block->variableItems,
                                               sCurrDeclItem,
                                               &aVarNode->columnName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    *aColumn = aTemplate->tmplate.rows[sFoundVariable->
                                                       variableTypeNode->node.table].columns +
                        sFoundVariable->variableTypeNode->node.column;
                }
            } /* QC_IS_NULL_NAME( aVarNode->tableName ) == ID_FALSE */
            else
            {
                // ( 2 ) myPkg_name.variable_name
                if ( QC_IS_NAME_MATCHED( aPkgParseTree->pkgNamePos,
                                         aVarNode->tableName )
                     == ID_TRUE )
                {
                    IDE_TEST( searchVariableItems( aPkgParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   &aVarNode->columnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        *aColumn = aTemplate->tmplate.rows[sFoundVariable->
                                                           variableTypeNode->node.table].columns +
                            sFoundVariable->variableTypeNode->node.column;
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

                if ( *aIsFound == ID_FALSE )
                {
                    // ( 3 ) record_name.field_name
                    IDE_TEST( searchVariableItems( aPkgParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   &aVarNode->tableName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                            ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                        {
                            IDE_TEST( searchFieldTypeOfRecord( sFoundVariable,
                                                               &aVarNode->columnName,
                                                               aIsFound,
                                                               aTemplate,
                                                               aColumn )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            IDE_RAISE( err_not_reference_variable );
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
            } /* QC_IS_NULL_NAME( aVarNode->tableName ) == ID_TRUE */
        } /* QC_IS_NULL_NAME( aVarNode->userName ) == ID_FALSE */
        else
        {
            // ( 4 ) myUser_name.myPkg_name.variable_name
            if ( qcmUser::getUserID( aStatement,
                                     aVarNode->userName,
                                     &sUserID )
                 == IDE_SUCCESS )
            {
                if ( QC_IS_NAME_MATCHED( aPkgParseTree->pkgNamePos,
                                         aVarNode->tableName )
                     == ID_TRUE )
                {
                    IDE_TEST( searchVariableItems( aPkgParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   &aVarNode->columnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    // userName, tableName이 없음.
                    // (1) variable_name / parameter name
                    if( *aIsFound == ID_TRUE )
                    {
                        *aColumn = aTemplate->tmplate.rows[sFoundVariable->
                                                           variableTypeNode->node.table].columns +
                            sFoundVariable->variableTypeNode->node.column;
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
                IDE_CLEAR();

                // ( 5 ) myPkg_name.variable_name.field_name
                if ( QC_IS_NAME_MATCHED( aPkgParseTree->pkgNamePos,
                                         aVarNode->userName )
                     == ID_TRUE )
                {
                    IDE_TEST( searchVariableItems( aPkgParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   &aVarNode->tableName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                            ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                        {
                            IDE_TEST( searchFieldTypeOfRecord( sFoundVariable,
                                                               &aVarNode->columnName,
                                                               aIsFound,
                                                               aTemplate,
                                                               aColumn )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            IDE_RAISE( err_not_reference_variable );
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
        }  /* QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE */ 
    } /* QC_IS_NULL_NAME( aVarNode->pkgName ) == ID_FALSE */
    else
    {
        // ( 6 ) myUser_name.myPkg_name.variable_name.field_name
        if ( qcmUser::getUserID( aStatement,
                                 aVarNode->userName,
                                 &sUserID )
             == IDE_SUCCESS )
        {
            if ( QC_IS_NAME_MATCHED( aPkgParseTree->pkgNamePos,
                                     aVarNode->tableName )
                 == ID_TRUE )
            {
                IDE_TEST( searchVariableItems( aPkgParseTree->block->variableItems,
                                               sCurrDeclItem,
                                               &aVarNode->columnName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                        ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                    {
                        IDE_TEST( searchFieldTypeOfRecord( sFoundVariable,
                                                           &aVarNode->pkgName,
                                                           aIsFound,
                                                           aTemplate,
                                                           aColumn )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                        IDE_RAISE( err_not_reference_variable );
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
        else
        {
            IDE_CLEAR();
        }
    } /* QC_IS_NULL_NAME( aVarNode->pkgNamePos ) == ID_TRUE */

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_not_reference_variable );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSV_INVALID_REFERENCE_VARIABLE));
    }
    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalVarType( qcStatement  * aStatement,
                                          qsVariables  * aVariable,
                                          idBool       * aIsFound,
                                          mtcColumn   ** aColumn )
{
    qsPkgParseTree * sPkgSpecParseTree = NULL;
    qsPkgParseTree * sPkgBodyParseTree = NULL;
    qcTemplate     * sPkgSpecTemplate  = NULL;
    qcTemplate     * sPkgBodyTemplate  = NULL;
    qtcNode        * sNode             = NULL;

    sNode     = aVariable->variableTypeNode;
    *aIsFound = ID_FALSE;

    if( aStatement->spvEnv->createPkg == NULL )
    {
        IDE_CONT( NOMAL_PROCEDURE );
    }
    else
    {
        if ( aStatement->spvEnv->createPkg->objType == QS_PKG )
        {
            sPkgSpecParseTree = aStatement->spvEnv->createPkg;
            sPkgBodyParseTree = NULL;

            sPkgSpecTemplate = QC_SHARED_TMPLATE(aStatement); 
            sPkgBodyTemplate = NULL;
        }
        else
        {
            IDE_DASSERT( aStatement->spvEnv->createPkg->objType == QS_PKG_BODY );

            sPkgSpecParseTree = aStatement->spvEnv->pkgPlanTree;
            sPkgBodyParseTree = aStatement->spvEnv->createPkg;

            sPkgSpecTemplate = sPkgSpecParseTree->pkgInfo->tmplate;
            sPkgBodyTemplate = QC_SHARED_TMPLATE(aStatement); 
        }
    }

    if ( sPkgBodyParseTree != NULL )
    {
        IDE_TEST( searchPkgLocalVarTypeInternal( aStatement,
                                                 sPkgBodyParseTree,
                                                 sNode,
                                                 sPkgBodyTemplate,
                                                 aIsFound,
                                                 aColumn)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( *aIsFound == ID_FALSE )
    {
        IDE_TEST( searchPkgLocalVarTypeInternal( aStatement,
                                                 sPkgSpecParseTree,
                                                 sNode,
                                                 sPkgSpecTemplate,
                                                 aIsFound,
                                                 aColumn)
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    IDE_EXCEPTION_CONT( NOMAL_PROCEDURE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchNodeFromPkg(
    qcStatement    * aStatement, 
    qsxPkgInfo     * aPkgInfo, 
    qtcNode        * aQtcNode,
    idBool         * aValidVariable,
    mtcColumn     ** aColumn)
{
    qsPkgParseTree    * sPlanTree = aPkgInfo->planTree;
    qsPkgStmtBlock    * sBLOCK = sPlanTree->block;
    qsVariables       * sFoundVariable = NULL;
    qcTemplate        * sTemplate;
    SChar               sUserName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    idBool              sIsFound = ID_FALSE;

    sUserName[ QC_MAX_OBJECT_NAME_LEN ] = '\0';

    sTemplate = aPkgInfo->tmplate;

    if( QC_IS_NULL_NAME( aQtcNode->pkgName ) == ID_TRUE )
    {
        // package.variable
        if( QC_IS_NULL_NAME( aQtcNode->userName ) == ID_TRUE )
        {
            IDE_DASSERT( sPlanTree != NULL );

            IDE_TEST( searchVariableItems( sBLOCK->variableItems,
                                           NULL,
                                           &aQtcNode->columnName,
                                           &sIsFound,
                                           &sFoundVariable )
                      != IDE_SUCCESS );

            if( sIsFound == ID_TRUE )
            {
                aQtcNode->node.table    = sFoundVariable->common.table;
                aQtcNode->node.column   = sFoundVariable->common.column;
                aQtcNode->node.objectID = sPlanTree->pkgOID;
                *aValidVariable = ID_TRUE;

                *aColumn = sTemplate->tmplate
                    .rows[sFoundVariable->variableTypeNode->node.table]
                    .columns
                    + sFoundVariable->variableTypeNode->node.column;
            }
        }
        else
        {
            IDE_DASSERT( sPlanTree != NULL );

            if( QC_IS_NULL_NAME( sPlanTree->userNamePos ) == ID_TRUE )
            {
                IDE_TEST( qcmUser::getUserName( aStatement,
                                                sPlanTree->userID,
                                                sUserName )
                          != IDE_SUCCESS );
            }
            else
            {
                QC_STR_COPY( sUserName, sPlanTree->userNamePos );
            }

            // check user
            if ( idlOS::strMatch( aQtcNode->userName.stmtText + aQtcNode->userName.offset,
                                  aQtcNode->userName.size,
                                  sUserName,
                                  idlOS::strlen( sUserName ) ) != 0 )
            {
                // package.record.field
                // check package
                IDE_TEST( searchVariableItems( sBLOCK->variableItems,
                                               NULL,
                                               &aQtcNode->tableName,
                                               &sIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS);

                if( sIsFound == ID_TRUE )
                {
                    // check field
                    if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                        ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                    {
                        *aValidVariable = ID_TRUE;
                        aQtcNode->node.objectID = sPlanTree->pkgOID;

                        IDE_TEST( searchFieldTypeOfRecord( sFoundVariable,
                                                           &aQtcNode->columnName,
                                                           aValidVariable,
                                                           sTemplate,
                                                           aColumn )
                                  != IDE_SUCCESS );
                    } 
                    // BUG-40727
                    // 변수 타입이 row/record 타입이 아니면 찾지 못한 것이다.
                    else
                    {
                        sIsFound = ID_FALSE;
                    }
                }
                else
                {
                    sIsFound = ID_FALSE;
                }
            }
            // user.package.variable 
            else
            {
                IDE_TEST( searchVariableItems( sBLOCK->variableItems,
                                               NULL,
                                               &aQtcNode->columnName,
                                               &sIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS);

                if( sIsFound == ID_TRUE )
                {
                    aQtcNode->node.table    = sFoundVariable->common.table;
                    aQtcNode->node.column   = sFoundVariable->common.column;
                    aQtcNode->node.objectID = sPlanTree->pkgOID;
                    *aValidVariable         = ID_TRUE;

                    *aColumn = sTemplate->tmplate
                        .rows[sFoundVariable->variableTypeNode->node.table]
                        .columns
                        + sFoundVariable->variableTypeNode->node.column;
                }
                else
                {
                    sIsFound = ID_FALSE; 
                }
            }
        }
    }
    // user.package.record.field
    else
    {
        IDE_TEST( searchVariableItems( sBLOCK->variableItems,
                                       NULL,
                                       &aQtcNode->columnName,
                                       &sIsFound,
                                       &sFoundVariable )
                  != IDE_SUCCESS );

        if( sIsFound == ID_TRUE )
        {
            if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
            {
                *aValidVariable = ID_TRUE;

                // cehck field
                IDE_TEST( searchFieldTypeOfRecord( sFoundVariable,
                                                   &aQtcNode->pkgName,
                                                   aValidVariable,
                                                   sTemplate,
                                                   aColumn )
                          != IDE_SUCCESS );
            }
            else
            {
                sIsFound = ID_FALSE;
            }
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

IDE_RC qsvProcVar::searchVarFromPkg(
    qcStatement    * aStatement, 
    qsxPkgInfo     * aPkgInfo, 
    qsVariables    * aVariable,
    idBool         * aValidVariable,
    mtcColumn     ** aColumn)
{
    qtcNode         * sNode = aVariable->variableTypeNode;

    IDE_TEST( qsvProcVar::searchNodeFromPkg( aStatement,
                                             aPkgInfo,
                                             sNode,
                                             aValidVariable,
                                             aColumn )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalVarNonArg(
    qcStatement    * aStatement,
    qsPkgParseTree * aPkgParseTree, 
    qtcNode        * aVarNode,
    idBool         * aIsFound,
    qsVariables   ** aVariable )
{
    qsPkgParseTree  * sBodyParseTree;
    qsPkgParseTree  * sSpecParseTree;
    qsVariables     * sFoundVariable = NULL;
    qsOID             sPkgOID = QS_EMPTY_OID;
    idBool            sIsNeedChkField = ID_FALSE;
    UInt              sUserID;
    qsVariableItems * sCurrDeclItem;

    qcNamePosition  * sColumnName;

    sCurrDeclItem  = aStatement->spvEnv->currDeclItem;
    sBodyParseTree = NULL;
    sSpecParseTree = NULL;
    *aIsFound      = ID_FALSE;       // TASK-3876 Code Sonar

    if( aPkgParseTree != NULL )
    {
        if( aPkgParseTree->objType == QS_PKG )
        {
            sSpecParseTree = aPkgParseTree;
        }
        else
        {
            sBodyParseTree = aPkgParseTree;
            sSpecParseTree = aStatement->spvEnv->pkgPlanTree;
        }
    }
    else
    {
        // 일반 procedure에서 package 변수를 사용한 경우
        // Nothing to do.
    }

    if( ( sBodyParseTree == NULL ) &&
        ( sSpecParseTree == NULL ) )
    {
        IDE_CONT( NORMAL_PROCEDURE );
    }
    else
    {
        // Nothing to do.
    }

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    if ( QC_IS_NULL_NAME( aVarNode->userName ) == ID_FALSE )
    {
        if ( qcmUser::getUserID( aStatement,
                                 aVarNode->userName,
                                 &sUserID )
             != IDE_SUCCESS )
        {
            sUserID = ID_UINT_MAX;
            IDE_CLEAR();
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sUserID = ID_UINT_MAX;
    }

    if( QC_IS_NULL_NAME( aVarNode->pkgName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE )
        {
            if( QC_IS_NULL_NAME( aVarNode->tableName ) == ID_TRUE )
            {
                /* Package Local */
                sColumnName = &aVarNode->columnName;

                // variable
                // 1. body 변수
                // 2. spec 변수
                // 다른 package 인지 확인할 필요가 없다.
                // ----------------------------------
                // | USER | TABLE | COLUMN | PACKAGE
                // ----------------------------------
                // |   X  |   X   |   O    |   X
                if( sBodyParseTree != NULL )
                {
                    IDE_TEST( searchVariableItems(
                                  sBodyParseTree->block->variableItems,
                                  sCurrDeclItem,
                                  sColumnName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );
                }

                if( ( *aIsFound == ID_FALSE ) &&
                    ( sSpecParseTree != NULL ) )
                {
                    // Spec 변수인지 확인한다.
                    IDE_TEST( searchVariableItems( sSpecParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if( sBodyParseTree != NULL )  // 즉, body가 존재한다.
                        {
                            sPkgOID = sSpecParseTree->pkgOID;
                        }
                        else
                        {
                            sPkgOID = QS_EMPTY_OID;
                        }
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                    }
                }
                else
                {
                    // Nothing to do.
                    // package body에서 찾았다.
                }
            } // table name is null
            else
            {
                // varialbe.field   
                // myPackage.variable 
                // ----------------------------------
                // | USER | TABLE | COLUMN | PACKAGE
                // ----------------------------------
                // |   X  |   O   |   O    |   X

                // Local
                if( sBodyParseTree != NULL )
                {
                    sColumnName = &aVarNode->tableName;

                    // variable.field
                    IDE_TEST( searchVariableItems( sBodyParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        sIsNeedChkField = ID_TRUE;
                    }

                    // myPackage.variable
                    if( *aIsFound == ID_FALSE )
                    {
                        if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) )
                        {
                            sColumnName = &aVarNode->columnName;

                            IDE_TEST( searchVariableItems( sBodyParseTree->block->variableItems,
                                                           sCurrDeclItem,
                                                           sColumnName,
                                                           aIsFound,
                                                           &sFoundVariable )
                                      != IDE_SUCCESS );
                        }
                        else
                        {
                            // Nothing to do.
                            // 외부 package의 변수
                        } 
                    }
                } // body parse tree is not null

                if( ( *aIsFound == ID_FALSE ) &&
                    ( sSpecParseTree != NULL ) )
                {
                    sColumnName = &aVarNode->tableName;

                    // Spec 변수인지 확인한다.
                    IDE_TEST( searchVariableItems( sSpecParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if(sBodyParseTree != NULL ) // 즉, body는 존재한다.
                        {
                            sPkgOID = sSpecParseTree->pkgOID;
                        }
                        else
                        {
                            sPkgOID = QS_EMPTY_OID;
                        }

                        sIsNeedChkField = ID_TRUE;
                    }

                    // myPackage.variable
                    if( *aIsFound == ID_FALSE )
                    {
                        if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) )
                        {
                            sColumnName = &aVarNode->columnName;

                            // Spec 변수인지 확인한다.
                            IDE_TEST( searchVariableItems( sSpecParseTree->block->variableItems,
                                                           sCurrDeclItem,
                                                           sColumnName,
                                                           aIsFound,
                                                           &sFoundVariable )
                                      != IDE_SUCCESS );

                            if( *aIsFound == ID_TRUE )
                            {
                                if( sBodyParseTree != NULL )  // 즉, body가 존재한다.
                                {
                                    sPkgOID = sSpecParseTree->pkgOID;
                                }
                                else
                                {
                                    sPkgOID = QS_EMPTY_OID;
                                }
                            }
                            else
                            {
                                // Not Found
                            }
                        }
                        else
                        {
                            // Nothing to do.
                            // 외부 package의 변수
                        }
                    }
                } // spec parse tree is not null
            } // table name in not null
        }
        else
        {
            // myPackage.variable.field
            // myUser.myPackage.variable
            // ----------------------------------
            // | USER | TABLE | COLUMN | PACKAGE
            // ----------------------------------
            // |  O   |   O   |   O    |   X

            // check my package name
            if( sBodyParseTree != NULL )
            {
                // myPackage.variable.field
                if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->userName ) )
                {
                    sColumnName = &aVarNode->tableName;

                    IDE_TEST( searchVariableItems( sBodyParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        sIsNeedChkField = ID_TRUE;
                    }
                }
                // myUser.myPackage.variable
                else
                {
                    if( sUserID == sBodyParseTree->userID )
                    {
                        if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) )
                        {
                            sColumnName = &aVarNode->columnName;

                            IDE_TEST( searchVariableItems( sBodyParseTree->block->variableItems,
                                                           sCurrDeclItem,
                                                           sColumnName,
                                                           aIsFound,
                                                           &sFoundVariable )
                                      != IDE_SUCCESS );
                        }
                    }
                }
            } // body parse tree is not null

            if( ( *aIsFound == ID_FALSE ) &&
                ( sSpecParseTree != NULL ) )
            {
                // myPackage.variable.field
                if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->userName ) )
                {
                    sColumnName = &aVarNode->tableName;

                    IDE_TEST( searchVariableItems( sSpecParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if( sBodyParseTree != NULL ) // 즉, body는 존재한다.
                        {
                            sPkgOID = sSpecParseTree->pkgOID;
                        }
                        else
                        {
                            sPkgOID = QS_EMPTY_OID;
                        }

                        sIsNeedChkField = ID_TRUE;
                    }
                }
                // myUser.myPackage.variable
                else
                {
                    if ( sUserID == sSpecParseTree->userID )
                    {
                        if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) )
                        {
                            sColumnName = &aVarNode->columnName;

                            IDE_TEST( searchVariableItems( sSpecParseTree->block->variableItems,
                                                           sCurrDeclItem,
                                                           sColumnName,
                                                           aIsFound,
                                                           &sFoundVariable )
                                      != IDE_SUCCESS );

                            if( *aIsFound == ID_TRUE )
                            {
                                if( sBodyParseTree != NULL )  // 즉, body가 존재한다.
                                {
                                    sPkgOID = sSpecParseTree->pkgOID;
                                }
                                else
                                {
                                    sPkgOID = QS_EMPTY_OID;
                                }
                            }
                            else
                            {
                                // Not Found
                            }

                        }
                        else
                        {
                            // Nothing to do.
                            // 다른 package 변수
                        }
                    }
                    else
                    {
                        // Nothing to do.
                        // 다른 package 변수
                    }
                }
            } // spec parse tree is not null
        } // user name is not null
    } // pkg name is null
    else
    {
        // myUser.myPackage.variable.field
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE
        // ----------------------------------
        // |  O   |   O   |   O    |   O

        sColumnName = &aVarNode->columnName;

        if( sBodyParseTree != NULL )
        {
            if( sUserID == sBodyParseTree->userID )
            {
                if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) )
                {
                    IDE_TEST( searchVariableItems( sBodyParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        sIsNeedChkField = ID_TRUE;
                    }
                }
            }
        } // body parse tree is not null

        if( ( *aIsFound == ID_FALSE ) &&
            ( sSpecParseTree != NULL ) )
        {
            if( sUserID == sSpecParseTree->userID )
            {
                if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) )
                {
                    IDE_TEST( searchVariableItems( sSpecParseTree->block->variableItems,
                                                   sCurrDeclItem,
                                                   sColumnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if( *aIsFound == ID_TRUE )
                    {
                        if( sBodyParseTree != NULL ) // 즉, body는 존재한다.
                        {
                            sPkgOID = sSpecParseTree->pkgOID;
                        }
                        else
                        {
                            sPkgOID = QS_EMPTY_OID;
                        }

                        sIsNeedChkField = ID_TRUE;
                    }
                }
                else
                {
                    // Nothing to do.
                    // 다른 package 변수
                }
            }
            else
            {
                // Nothing to do.
                // 다른 package 변수
            }
        } // spec parse tree is not null
    } // pkg name is not null

    IDE_EXCEPTION_CONT( NORMAL_PROCEDURE );

    if( *aIsFound == ID_TRUE )
    {
        if( sIsNeedChkField == ID_TRUE )
        {
            // search field
            IDE_TEST( searchFieldOfRecord( aStatement,
                                           sFoundVariable->typeInfo,
                                           sFoundVariable->variableTypeNode,
                                           aVarNode,
                                           ID_FALSE,
                                           aIsFound )
                      != IDE_SUCCESS );

            aVarNode->node.objectID = sPkgOID;
        }
        else 
        {
            IDE_DASSERT( sFoundVariable != NULL );

            aVarNode->node.table    = sFoundVariable->common.table;
            aVarNode->node.column   = sFoundVariable->common.column;
            aVarNode->node.objectID = sPkgOID;
        }

        // searchFieldOfRecord에 의해서 aIsFound가 변경될 수 있으므로
        // aIsFound도 다시 검사한다.
        if ( *aIsFound == ID_TRUE )
        {
            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

            if ( sFoundVariable->inOutType == QS_IN )
            {
                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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

        /* BUG-37235 */
        aVarNode->lflag &= ~QTC_NODE_PKG_VARIABLE_MASK;
        aVarNode->lflag |= QTC_NODE_PKG_VARIABLE_TRUE;

        /* BUG-39770 */
        aVarNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
        aVarNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;

        // BUG-47971 Package global variable의 분산 실행을 제한합니다.
        if ( sFoundVariable->inOutType != QS_IN )
        {
            aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
            aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;

            if ( aStatement->spvEnv->createProc != NULL )
            {
                aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
            }
        }

        *aVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgVarWithArg(
    qcStatement  * aStatement,
    qsxPkgInfo   * aPkgInfo,
    qtcNode      * aVarNode,
    idBool       * aIsFound,
    qsVariables ** aVariable )
{
    qsPkgParseTree    * sParseTree;
    qsVariables       * sFoundVariable = NULL;
    qsVariableItems   * sCurrVar;
    mtcColumn         * sColumn;
    qtcModule         * sQtcModule;
    SChar               sUserName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qcTemplate        * sTemplate;
    mtcTuple          * sMtcTuple;
    qcuSqlSourceInfo    sqlInfo;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qsvProcVar::searchPkgVarWithArg::__FT__" );

    sUserName[ QC_MAX_OBJECT_NAME_LEN ] = '\0';

    *aIsFound  = ID_FALSE;
    sTemplate = aPkgInfo->tmplate;

    sParseTree = aPkgInfo->planTree;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    if( QC_IS_NULL_NAME( aVarNode->pkgName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE )
        {
            // package.variable[ index ]
            // ----------------------------------
            // | USER | TABLE | COLUMN | PACKAGE
            // ----------------------------------
            // |   X  |   O   |   O    |   X
            for( sCurrVar = sParseTree->block->variableItems;
                 ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                 sCurrVar = sCurrVar->next )
            {
                // check variable 
                if( idlOS::strMatch(
                        sCurrVar->name.stmtText + sCurrVar->name.offset,
                        sCurrVar->name.size,
                        aVarNode->columnName.stmtText + aVarNode->columnName.offset,
                        aVarNode->columnName.size) == 0 )
                {
                    sFoundVariable = (qsVariables *)sCurrVar;
                    sFoundVariable->common.objectID = sParseTree->pkgOID;
                    *aIsFound = ID_TRUE;

                    if ( sFoundVariable->variableType ==
                         QS_ASSOCIATIVE_ARRAY_TYPE )
                    {
                        aVarNode->node.table =
                            sFoundVariable->variableTypeNode->node.arguments->table;
                        aVarNode->node.column =
                            sFoundVariable->variableTypeNode->node.arguments->column;
                        aVarNode->node.objectID = sFoundVariable->common.objectID;

                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;
 
                        if( sFoundVariable->inOutType == QS_IN )
                        {
                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        // BUG-42640
                        sqlInfo.setSourceInfo( aStatement,
                                               & aVarNode->columnName );
                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                    }
                }
            }
        }
        else  // userName is not null
        {
            if( QC_IS_NULL_NAME( aVarNode->columnName ) == ID_TRUE )
            {
                // package.variable[ index ]
                // ----------------------------------
                // | USER | TABLE | COLUMN | PACKAGE
                // ----------------------------------
                // |  O   |   O   |   X    |   X

                for( sCurrVar = sParseTree->block->variableItems;
                     ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                     sCurrVar = sCurrVar->next )
                {
                    // check variable
                    if( idlOS::strMatch(
                            sCurrVar->name.stmtText + sCurrVar->name.offset,
                            sCurrVar->name.size,
                            aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                            aVarNode->tableName.size) == 0 )
                    {
                        sFoundVariable = ( qsVariables * )sCurrVar;
                        sFoundVariable->common.objectID =  sParseTree->pkgOID;
                        *aIsFound = ID_TRUE;

                        if( sFoundVariable->variableType ==
                            QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            aVarNode->node.table =
                                sFoundVariable->variableTypeNode->node.arguments->table;
                            aVarNode->node.column =
                                sFoundVariable->variableTypeNode->node.arguments->column;
                            aVarNode->node.objectID = sFoundVariable->common.objectID;

                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                            if( sFoundVariable->inOutType == QS_IN )
                            {
                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            // BUG-42640
                            sqlInfo.setSourceInfo( aStatement,
                                                   & aVarNode->tableName );
                            IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                        }
                    }
                }
            }
            else
            {
                // package.variable[ index ].column
                // ----------------------------------
                // | USER | TABLE | COLUMN | PACKAGE
                // ----------------------------------
                // |  O   |   O   |   O    |   X

                if( QC_IS_NULL_NAME( sParseTree->userNamePos ) == ID_TRUE )
                {
                    IDE_TEST( qcmUser::getUserName( aStatement,
                                                    sParseTree->userID,
                                                    sUserName )
                              != IDE_SUCCESS );
                }
                else
                {
                    QC_STR_COPY( sUserName, sParseTree->userNamePos );
                }

                //check use
                if ( idlOS::strMatch( aVarNode->userName.stmtText + aVarNode->userName.offset,
                                      aVarNode->userName.size,
                                      sUserName,
                                      idlOS::strlen( sUserName ) ) != 0 )
                {
                    // package.arr[index].column
                    for( sCurrVar = sParseTree->block->variableItems;
                         ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                         sCurrVar = sCurrVar->next )
                    {
                        // check variable
                        if( idlOS::strMatch(
                                sCurrVar->name.stmtText + sCurrVar->name.offset,
                                sCurrVar->name.size,
                                aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                                aVarNode->tableName.size) == 0 )
                        {
                            sFoundVariable = ( qsVariables * )sCurrVar;
                            sFoundVariable->common.objectID = aPkgInfo->planTree->pkgOID;
                            *aIsFound = ID_TRUE;

                            if( sFoundVariable->variableType ==
                                QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                sMtcTuple = ( sTemplate->tmplate.rows ) +
                                    ((qtcNode*)sFoundVariable->variableTypeNode->node.arguments)->node.table;

                                sColumn = QTC_TUPLE_COLUMN( sMtcTuple,
                                                            (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                                if( sColumn->module->id == MTD_RECORDTYPE_ID )
                                {
                                    sQtcModule = (qtcModule*)sColumn->module;

                                    IDE_TEST( searchFieldOfRecord(
                                                  aStatement,
                                                  sQtcModule->typeInfo,
                                                  (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                                  aVarNode,
                                                  ID_FALSE,
                                                  aIsFound )
                                              != IDE_SUCCESS );

                                    aVarNode->node.objectID = sFoundVariable->common.objectID;

                                    if ( *aIsFound == ID_TRUE )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                        if ( sFoundVariable->inOutType == QS_IN )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                        }
                                        else
                                        {
                                            // Nothing to do.
                                        }
                                    }
                                    else
                                    {
                                        // BUG-42640
                                        sqlInfo.setSourceInfo( aStatement,
                                                               & aVarNode->columnName );
                                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                                    }
                                }
                                else
                                {
                                    // BUG-42640
                                    sqlInfo.setSourceInfo( aStatement,
                                                           & aVarNode->tableName );
                                    IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                                }
                            }
                            else
                            {
                                // BUG-42640
                                sqlInfo.setSourceInfo( aStatement,
                                                       & aVarNode->tableName );
                                IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                            }
                        }
                    }
                } // userName is package name
                else
                {
                    // user.package.variable[ index ]
                    // ----------------------------------
                    // | USER | TABLE | COLUMN | PACKAGE
                    // ----------------------------------
                    // |  O   |   O   |   O    |   X

                    for( sCurrVar = sParseTree->block->variableItems;
                         ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                         sCurrVar = sCurrVar->next )
                    {
                        // check variable
                        if( idlOS::strMatch(
                                sCurrVar->name.stmtText + sCurrVar->name.offset,
                                sCurrVar->name.size,
                                aVarNode->columnName.stmtText + aVarNode->columnName.offset,
                                aVarNode->columnName.size) == 0 )
                        {
                            sFoundVariable = ( qsVariables * )sCurrVar;
                            sFoundVariable->common.objectID =  sParseTree->pkgOID;
                            *aIsFound = ID_TRUE;

                            if( sFoundVariable->variableType ==
                                QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                aVarNode->node.table =
                                    sFoundVariable->variableTypeNode->node.arguments->table;
                                aVarNode->node.column =
                                    sFoundVariable->variableTypeNode->node.arguments->column;
                                aVarNode->node.objectID = sFoundVariable->common.objectID;

                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                // BUG-42640
                                sqlInfo.setSourceInfo( aStatement,
                                                       & aVarNode->columnName );
                                IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                            }
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                } // userName is user Name
            }
        }
    } // pkgName is null
    else
    {
        // user.package.variable[ index ].column
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE
        // ----------------------------------
        // |  O   |   O   |   O    |   O 

        for( sCurrVar = sParseTree->block->variableItems;
             ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
             sCurrVar = sCurrVar->next )
        {
            // check variable
            if( idlOS::strMatch(
                    sCurrVar->name.stmtText + sCurrVar->name.offset,
                    sCurrVar->name.size,
                    aVarNode->columnName.stmtText + aVarNode->columnName.offset,
                    aVarNode->columnName.size) == 0 )
            {
                sFoundVariable = ( qsVariables * )sCurrVar;
                sFoundVariable->common.objectID = aPkgInfo->planTree->pkgOID;
                *aIsFound = ID_TRUE;

                if( sFoundVariable->variableType ==
                    QS_ASSOCIATIVE_ARRAY_TYPE )
                {
                    sMtcTuple = ( sTemplate->tmplate.rows ) +
                        ((qtcNode*)sFoundVariable->variableTypeNode->node.arguments)->node.table;

                    sColumn = QTC_TUPLE_COLUMN( sMtcTuple,
                                                (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                    if( sColumn->module->id == MTD_RECORDTYPE_ID )
                    {
                        sQtcModule = (qtcModule*)sColumn->module;

                        IDE_TEST( searchFieldOfRecord(
                                      aStatement,
                                      sQtcModule->typeInfo,
                                      (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                      aVarNode,
                                      ID_FALSE,
                                      aIsFound )
                                  != IDE_SUCCESS );

                        aVarNode->node.objectID = sFoundVariable->common.objectID;

                        if ( *aIsFound == ID_TRUE )
                        {
                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                            if ( sFoundVariable->inOutType == QS_IN )
                            {
                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            // BUG-42640
                            sqlInfo.setSourceInfo( aStatement,
                                                   & aVarNode->pkgName );
                            IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                        }
                    }
                    else
                    {
                        // BUG-42640
                        sqlInfo.setSourceInfo( aStatement,
                                               & aVarNode->columnName );
                        IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                    }
                }
                else
                {
                    // BUG-42640
                    sqlInfo.setSourceInfo( aStatement,
                                           & aVarNode->columnName );
                    IDE_RAISE( ERR_NOT_EXIST_COLUMN );
                }
            }
        }
    }

    if( *aIsFound == ID_TRUE )
    {
        /* BUG-39770 */
        aVarNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
        aVarNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;

        // BUG-47971 Package global variable의 분산 실행을 제한합니다.
        if ( sFoundVariable->inOutType != QS_IN )
        {
            aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
            aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;

            if ( aStatement->spvEnv->createProc != NULL )
            {
                aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
            }
        }

        *aVariable = sFoundVariable;
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    // BUG-42640
    IDE_EXCEPTION(ERR_NOT_EXIST_COLUMN)
    {
        (void)sqlInfo.init(aStatement->qmeMem);
        IDE_SET(
            ideSetErrorCode(qpERR_ABORT_QMV_NOT_EXISTS_COLUMN,
                            sqlInfo.getErrMessage() ));
        (void)sqlInfo.fini();
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    *aIsFound = ID_FALSE;

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchVariableFromPkg(
    qcStatement   * aStatement,
    qtcNode       * aVarNode,
    idBool        * aIsFound,
    qsVariables  ** aVariable )
{
    qsOID              sPkgOID;
    UInt               sPkgUserID;
    qsxPkgInfo       * sPkgInfo;
    idBool             sExists         = ID_FALSE;
    qcmSynonymInfo     sSynonymInfo;

    *aIsFound = ID_FALSE;
    *aVariable = NULL;

    //search variable declared in package
    IDE_TEST( qcmSynonym::resolvePkg( aStatement,
                                      aVarNode->userName,
                                      aVarNode->tableName,
                                      &sPkgOID,
                                      &sPkgUserID,
                                      &sExists,
                                      &sSynonymInfo )
              != IDE_SUCCESS );

    if( sExists == ID_TRUE )
    {
        // synonym으로 참조되는 proc의 기록
        IDE_TEST( qsvPkgStmts::makePkgSynonymList( aStatement,
                                                   &sSynonymInfo,
                                                   aVarNode->userName,
                                                   aVarNode->tableName,
                                                   sPkgOID )
                  != IDE_SUCCESS );

        IDE_TEST( qsvPkgStmts::makeRelatedObjects( aStatement,
                                                   &aVarNode->userName,
                                                   &aVarNode->tableName,
                                                   & sSynonymInfo,
                                                   0,
                                                   QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree( aStatement,
                                                          sPkgOID,
                                                          QS_PKG,
                                                          &(aStatement->spvEnv->procPlanList))
                  != IDE_SUCCESS );

        IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );

        /* BUG-45164 */
        IDE_TEST_RAISE( sPkgInfo->isValid != ID_TRUE, err_object_invalid );

        IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aStatement,
                                                   sPkgInfo->planTree->userID,
                                                   sPkgInfo->privilegeCount,
                                                   sPkgInfo->granteeID,
                                                   ID_FALSE,
                                                   NULL,
                                                   NULL )
                  != IDE_SUCCESS );

        IDE_TEST( searchPkgVariable( aStatement,
                                     sPkgInfo,
                                     aVarNode,
                                     aIsFound,
                                     aVariable )
                  != IDE_SUCCESS );

        /* BUG-39770 */
        if ( (*aIsFound == ID_TRUE) &&
             (aStatement->spvEnv->createProc != NULL) &&
             (aStatement->spvEnv->createPkg == NULL) )
        {
            aStatement->spvEnv->createProc->referToPkg = ID_TRUE;
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

    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */ 
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgVariable( qcStatement  * aStatement,
                                      qsxPkgInfo   * aPkgInfo,
                                      qtcNode      * aVarNode,
                                      idBool       * aIsFound,
                                      qsVariables ** aVariable )
{
    IDU_FIT_POINT_FATAL( "qsvProcVar::searchPkgVariable::__FT__" );

    if ( ( aVarNode->node.arguments == NULL ) &&
         ( ( (aVarNode->lflag) & QTC_NODE_SP_ARRAY_INDEX_VAR_MASK ) ==
           QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT ) )
    {
        // (A). 괄호 스타일이 아니면서 Argument가 없는 변수인 경우.
        //      (column/row/record/associative array type인 경우)
        IDE_TEST( searchPkgVarNonArg( aStatement,
                                      aPkgInfo,
                                      aVarNode,
                                      aIsFound,
                                      aVariable )
                  != IDE_SUCCESS );
    }
    else
    {
        // (B). 괄호 스타일의 변수인 경우.(associative array type의 index를 쓰는 경우)
        IDE_TEST( searchPkgVarWithArg( aStatement,
                                       aPkgInfo,
                                       aVarNode,
                                       aIsFound,
                                       aVariable )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgVarNonArg( qcStatement  * aStatement,
                                       qsxPkgInfo   * aPkgInfo,
                                       qtcNode      * aVarNode,
                                       idBool       * aIsFound,
                                       qsVariables ** aVariable )
{
    qsPkgParseTree  * sParseTree;
    qsVariables     * sFoundVariable = NULL;
    SChar             sUserName[ QC_MAX_OBJECT_NAME_LEN + 1 ];
    qsOID             sPkgOID = QS_EMPTY_OID;
    idBool            sIsNeedChkField = ID_FALSE;
    qcNamePosition  * sVarName;

    IDE_FT_BEGIN();

    IDU_FIT_POINT_FATAL( "qsvProcVar::searchPkgVarNonArg::__FT__" );

    sParseTree     = aPkgInfo->planTree ;

    // 다른 package의 변수는 spec꺼만 사용가능하다.
    IDE_DASSERT( aPkgInfo->objType == QS_PKG );

    sUserName[ QC_MAX_OBJECT_NAME_LEN ] = '\0';

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;
    *aIsFound = ID_FALSE;       // TASK-3876 Code Sonar

    if( QC_IS_NULL_NAME( aVarNode->pkgName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE )
        {

            IDE_DASSERT( QC_IS_NULL_NAME( aVarNode->tableName ) != ID_TRUE );
            IDE_DASSERT( QC_IS_NULL_NAME( aVarNode->columnName ) != ID_TRUE );

            // package.variable
            // ----------------------------------
            // | USER | TABLE | COLUMN | PACKAGE
            // ----------------------------------
            // |   X  |   O   |   O    |   X

            sVarName = &aVarNode->columnName;

            IDE_TEST( searchVariableItems( sParseTree->block->variableItems,
                                           NULL,
                                           sVarName,
                                           aIsFound,
                                           &sFoundVariable )
                      != IDE_SUCCESS );
        }
        else // QC_IS_NULL_NAME( aVarNode->userName ) != ID_TRUE
        {
            // user.package.variable 
            // package.variable.field
            // ----------------------------------
            // | USER | TABLE | COLUMN | PACKAGE
            // ----------------------------------
            // |  O   |   O   |   O    |   X

            IDE_DASSERT( QC_IS_NULL_NAME( aVarNode->tableName ) != ID_TRUE );
            IDE_DASSERT( QC_IS_NULL_NAME( aVarNode->columnName ) != ID_TRUE );

            if( QC_IS_NULL_NAME( sParseTree->userNamePos ) == ID_TRUE )
            {
                IDE_TEST( qcmUser::getUserName( aStatement,
                                                sParseTree->userID,
                                                sUserName )
                          != IDE_SUCCESS );
            }
            else
            {
                QC_STR_COPY( sUserName, sParseTree->userNamePos );
            }

            //check use
            if ( idlOS::strMatch( aVarNode->userName.stmtText + aVarNode->userName.offset,
                                  aVarNode->userName.size,
                                  sUserName,
                                  idlOS::strlen( sUserName ) ) != 0 )
            {
                // package.variable.field
                sVarName = &aVarNode->tableName;

                IDE_TEST( searchVariableItems( sParseTree->block->variableItems,
                                               NULL,
                                               sVarName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if( *aIsFound == ID_TRUE )
                {
                    if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                        ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
                    {
                        sIsNeedChkField = ID_TRUE;
                    }
                    else
                    {
                        IDE_RAISE( err_not_reference_variable )
                            }
                }
            }
            else
            {
                // user.package.variable
                sVarName = &aVarNode->columnName;

                IDE_TEST( searchVariableItems( sParseTree->block->variableItems,
                                               NULL,
                                               sVarName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );
            }
        } // end of else
    }
    else
    {
        // user.package.variable.field
        // ----------------------------------
        // | USER | TABLE | COLUMN | PACKAGE
        // ----------------------------------
        // |  O   |   O   |   O    |   O

        // USER, TABLE, COLUMN, PACKAGE가 모두 있으면 반드시 field를 검사한다.
        sIsNeedChkField = ID_TRUE;

        sVarName = &aVarNode->columnName;

        IDE_TEST( searchVariableItems( sParseTree->block->variableItems,
                                       NULL,
                                       sVarName,
                                       aIsFound,
                                       &sFoundVariable )
                  != IDE_SUCCESS );

        if( *aIsFound == ID_TRUE )
        {
            sPkgOID = sParseTree->pkgOID;

            if( ( sFoundVariable->variableType == QS_ROW_TYPE ) ||
                ( sFoundVariable->variableType == QS_RECORD_TYPE ) )
            {
                sIsNeedChkField = ID_TRUE;
            }
            else
            {
                IDE_RAISE( err_not_reference_variable )
                    }
        }
    }

    if( *aIsFound == ID_TRUE )
    {
        sPkgOID = aPkgInfo->planTree->pkgOID;

        if( sIsNeedChkField == ID_TRUE )
        {
            // search field
            IDE_TEST( searchFieldOfRecord( aStatement,
                                           sFoundVariable->typeInfo,
                                           sFoundVariable->variableTypeNode,
                                           aVarNode,
                                           ID_FALSE,
                                           aIsFound )
                      != IDE_SUCCESS);

            aVarNode->node.objectID = sPkgOID;
        }
        else
        {
            aVarNode->node.table    = sFoundVariable->common.table;
            aVarNode->node.column   = sFoundVariable->common.column;
            aVarNode->node.objectID = sPkgOID;
        }

        // searchFieldOfRecord에 의해서 aIsFound가 변경될 수 있으므로
        // aIsFound도 다시 검사한다.
        if ( *aIsFound == ID_TRUE )
        {
            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

            if ( sFoundVariable->inOutType == QS_IN )
            {
                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
            }
            else
            {
                // Nothing to do.
            }

            /* BUG-39770 */
            aVarNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
            aVarNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;

            // BUG-47971 Package global variable의 분산 실행을 제한합니다.
            if ( sFoundVariable->inOutType != QS_IN )
            {
                aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
                aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;

                if ( aStatement->spvEnv->createProc != NULL )
                {
                    aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
                }
            }
        }
        else
        {
            // Nothing to do.
        }

        *aVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    IDE_FT_END();

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_not_reference_variable );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSV_INVALID_REFERENCE_VARIABLE));
    }
    IDE_EXCEPTION_SIGNAL()
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_FAULT_TOLERATED ) );
    }
    IDE_EXCEPTION_END;

    IDE_FT_EXCEPTION_BEGIN();

    *aIsFound = ID_FALSE;

    IDE_FT_EXCEPTION_END();

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalVarWithArg(
    qcStatement    * aStatement,
    qsPkgParseTree * aPkgParseTree,
    qtcNode        * aVarNode,
    idBool         * aIsFound,
    qsVariables   ** aVariable )
{
    qsPkgParseTree      * sBodyParseTree;
    qsPkgParseTree      * sSpecParseTree;
    qsVariables         * sFoundVariable = NULL;
    qsVariableItems     * sCurrVar;
    mtcColumn           * sColumn;
    qcTemplate          * sTemplate;
    mtcTuple            * sMtcTuple;
    qtcModule           * sQtcModule;
    UInt                  sUserID;

    sBodyParseTree = NULL;
    sSpecParseTree = NULL;
    *aIsFound      = ID_FALSE;       // TASK-3876 Code Sonar

    if( aPkgParseTree != NULL )
    {
        if( aPkgParseTree->objType == QS_PKG )
        {
            sSpecParseTree = aPkgParseTree;
        }
        else
        {
            sBodyParseTree = aPkgParseTree;
            sSpecParseTree = aStatement->spvEnv->pkgPlanTree;
        }
    }
    else
    {
        // 일반 procedure에서 package 변수를 사용한 경우
    }


    if( ( sBodyParseTree == NULL ) &&
        ( sSpecParseTree == NULL ) )
    {
        IDE_RAISE( NORMAL_PROCEDURE );
    }
    else
    {
        // Nothing to do.
    }

    if ( QC_IS_NULL_NAME( aVarNode->userName ) == ID_FALSE )
    {
        if ( qcmUser::getUserID( aStatement,
                                 aVarNode->userName,
                                 &sUserID )
             != IDE_SUCCESS )
        {
            sUserID = ID_UINT_MAX;
            IDE_CLEAR();
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sUserID = ID_UINT_MAX;
    }

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    if( QC_IS_NULL_NAME( aVarNode->pkgName) == ID_TRUE )
    {
        if( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE )
        {
            if( QC_IS_NULL_NAME( aVarNode->tableName ) == ID_TRUE )
            {
                // columnName[index]
                //      -----------------------
                //     |  U  |  T  |  C  |  P  |
                //      -----------------------
                //     |  X  |  X  |  O  |  X  |
                //      -----------------------

                if( sBodyParseTree != NULL )
                {
                    // array[index]
                    for( sCurrVar = sBodyParseTree->block->variableItems;
                         ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                         sCurrVar = sCurrVar->next )
                    {
                        // check variable
                        if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) )
                        {
                            sFoundVariable = (qsVariables *)sCurrVar;
                            sFoundVariable->common.objectID = QS_EMPTY_OID;
                            *aIsFound = ID_TRUE;

                            if ( sFoundVariable->variableType ==
                                 QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                aVarNode->node.table =
                                    sFoundVariable->variableTypeNode->node.arguments->table;
                                aVarNode->node.column =
                                    sFoundVariable->variableTypeNode->node.arguments->column;
                                aVarNode->node.objectID = sFoundVariable->common.objectID;

                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                    }
                }

                if( ( *aIsFound == ID_FALSE ) &&
                    ( sSpecParseTree != NULL ) )
                {
                    // Spec 변수인지 확인한다.
                    for( sCurrVar = sSpecParseTree->block->variableItems;
                         ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                         sCurrVar = sCurrVar->next )
                    {
                        // check variable
                        if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) )
                        {
                            sFoundVariable = (qsVariables *)sCurrVar;
                            sFoundVariable->common.objectID =  sSpecParseTree->pkgOID;
                            *aIsFound = ID_TRUE;

                            if ( sFoundVariable->variableType ==
                                 QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                aVarNode->node.table =
                                    sFoundVariable->variableTypeNode->node.arguments->table;
                                aVarNode->node.column =
                                    sFoundVariable->variableTypeNode->node.arguments->column;
                                aVarNode->node.objectID = sFoundVariable->common.objectID;

                                aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                if( sFoundVariable->inOutType == QS_IN )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                    aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                }
                                else
                                {
                                    // Nothing to do.
                                }
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                    }
                }
                else
                {
                    // Nothing to do.
                    // body에서 variable을 찾았다.
                }
            } // tableName is null;
            else
            {
                //TAG : table_name.column_name [ index ]
                //      -----------------------
                //     |  U  |  T  |  C  |  P  |
                //      -----------------------
                //     |  X  |  O  |  O  |  X  |
                //      -----------------------

                if( sBodyParseTree != NULL )
                {
                    if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                    {
                        for ( sCurrVar = sBodyParseTree->block->variableItems;
                              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                              sCurrVar = sCurrVar->next )
                        {
                            // check variable
                            if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                            {
                                sFoundVariable = ( qsVariables * )sCurrVar;
                                sFoundVariable->common.objectID = QS_EMPTY_OID;
                                *aIsFound = ID_TRUE;

                                if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                {
                                    aVarNode->node.table =
                                        sFoundVariable->variableTypeNode->node.arguments->table;
                                    aVarNode->node.column =
                                        sFoundVariable->variableTypeNode->node.arguments->column;
                                    aVarNode->node.objectID =
                                        sFoundVariable->common.objectID;

                                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                    if ( sFoundVariable->inOutType == QS_IN )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
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
                }

                if ( ( *aIsFound == ID_FALSE ) &&
                     ( sSpecParseTree != NULL ) )
                {
                    if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                    {
                        for ( sCurrVar = sSpecParseTree->block->variableItems;
                              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                              sCurrVar = sCurrVar->next )
                        {
                            // check variable
                            if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                            {
                                sFoundVariable = ( qsVariables * )sCurrVar;
                                sFoundVariable->common.objectID = sSpecParseTree->pkgOID;
                                *aIsFound = ID_TRUE;

                                if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                {
                                    aVarNode->node.table =
                                        sFoundVariable->variableTypeNode->node.arguments->table;
                                    aVarNode->node.column =
                                        sFoundVariable->variableTypeNode->node.arguments->column;
                                    aVarNode->node.objectID = sCurrVar->objectID;

                                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                    if ( sFoundVariable->inOutType == QS_IN )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                    }
                                    else
                                    {
                                        // Nothing to do.
                                    }
                                }
                                else
                                {
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                    }
                } // ( *aIsFound == ID_FALSE ) && ( sSpecParseTree != NULL )
            } // tableName is not null
        } // userName is null
        else
        {
            // userName.tableName[index].columnName
            // userName.tableName.columnName[index]
            //      -----------------------
            //     |  U  |  T  |  C  |  P  |
            //      -----------------------
            //     |  O  |  O  |  O  |  X  |
            //      -----------------------
            // e.g) package.variable[index].field
            //      user.package.variable[index]
            if ( sBodyParseTree != NULL )
            {
                // myPkg.arr[index].column
                if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->userName ) == ID_TRUE )
                {
                    for( sCurrVar = sBodyParseTree->block->variableItems;
                         ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                         sCurrVar = sCurrVar->next )
                    {
                        if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->tableName ) == ID_TRUE)
                        {
                            sFoundVariable = ( qsVariables * )sCurrVar;
                            sFoundVariable->common.objectID = QS_EMPTY_OID;
                            *aIsFound = ID_TRUE;

                            if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                sColumn = QTC_STMT_COLUMN( aStatement,
                                                           (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );
                                if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                                {
                                    sQtcModule = (qtcModule *)sColumn->module;

                                    IDE_TEST( searchFieldOfRecord(
                                                  aStatement,
                                                  sQtcModule->typeInfo,
                                                  (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                                  aVarNode,
                                                  ID_FALSE,
                                                  aIsFound )
                                              != IDE_SUCCESS );

                                    aVarNode->node.objectID = sFoundVariable->common.objectID;

                                    if ( *aIsFound == ID_TRUE )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                        if ( sFoundVariable->inOutType == QS_IN )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                    }
                }
                // myUser.myPkg.arr[index]
                else
                {
                    if ( sUserID == sBodyParseTree->userID )
                    {
                        if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                        {
                            for ( sCurrVar = sBodyParseTree->block->variableItems;
                                  ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                                  sCurrVar = sCurrVar->next )
                            {
                                // check variable
                                if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                                {
                                    sFoundVariable = ( qsVariables * )sCurrVar;
                                    sFoundVariable->common.objectID = QS_EMPTY_OID;
                                    *aIsFound = ID_TRUE;

                                    if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                    {
                                        aVarNode->node.table =
                                            sFoundVariable->variableTypeNode->node.arguments->table;
                                        aVarNode->node.column =
                                            sFoundVariable->variableTypeNode->node.arguments->column;
                                        aVarNode->node.objectID =
                                            sFoundVariable->common.objectID;

                                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                        if ( sFoundVariable->inOutType == QS_IN )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
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
            } // body parse tree is not null
            else
            {
                // Nothing to do.
            }

            if ( ( *aIsFound == ID_FALSE ) &&
                 ( sSpecParseTree != NULL ) )
            {
                // myPkg.arr[index].column
                if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->userName ) == ID_TRUE)
                {
                    for( sCurrVar = sSpecParseTree->block->variableItems;
                         ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                         sCurrVar = sCurrVar->next )
                    {
                        if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->tableName ) )
                        {
                            sFoundVariable = ( qsVariables * )sCurrVar;
                            sFoundVariable->common.objectID = sSpecParseTree->pkgOID;
                            *aIsFound = ID_TRUE;

                            if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                if( sSpecParseTree != aStatement->spvEnv->createPkg )
                                {
                                    sTemplate = sSpecParseTree->pkgInfo->tmplate;

                                    sMtcTuple = ( sTemplate->tmplate.rows ) +
                                        ((qtcNode*)sFoundVariable->variableTypeNode->node.arguments)->node.table;

                                    sColumn = QTC_TUPLE_COLUMN( sMtcTuple,
                                                                (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );
                                }
                                else
                                {
                                    // create package 중
                                    sColumn = QTC_STMT_COLUMN( aStatement,
                                                               (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );
                                }

                                if( sColumn->module->id == MTD_RECORDTYPE_ID )
                                {
                                    sQtcModule = (qtcModule *)sColumn->module;

                                    IDE_TEST( searchFieldOfRecord(
                                                  aStatement,
                                                  sQtcModule->typeInfo,
                                                  (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                                  aVarNode,
                                                  ID_FALSE,
                                                  aIsFound )
                                              != IDE_SUCCESS );

                                    if( sSpecParseTree != aStatement->spvEnv->createPkg )
                                    {
                                        aVarNode->node.objectID = sFoundVariable->common.objectID;
                                    }
                                    else
                                    {
                                        aVarNode->node.objectID = QS_EMPTY_OID;
                                    }

                                    if ( *aIsFound == ID_TRUE )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                        if ( sFoundVariable->inOutType == QS_IN )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                    }
                }
                // myUser.myPkg.arr[index]
                else
                {
                    if ( sUserID == sSpecParseTree->userID )
                    {
                        if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE)
                        {
                            // Spec 변수인지 확인한다.
                            for ( sCurrVar = sSpecParseTree->block->variableItems;
                                  ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                                  sCurrVar = sCurrVar->next )
                            {
                                // check variable
                                if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                                {
                                    sFoundVariable = ( qsVariables * )sCurrVar;
                                    sFoundVariable->common.objectID = sSpecParseTree->pkgOID;
                                    *aIsFound = ID_TRUE;

                                    if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                    {
                                        aVarNode->node.table =
                                            sFoundVariable->variableTypeNode->node.arguments->table;
                                        aVarNode->node.column =
                                            sFoundVariable->variableTypeNode->node.arguments->column;
                                        aVarNode->node.objectID = sFoundVariable->common.objectID;

                                        aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                        aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                        if ( sFoundVariable->inOutType == QS_IN )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                            aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
                                        }
                                        else
                                        {
                                            // Nothing to do.
                                        }
                                    }
                                    else
                                    {
                                        *aIsFound = ID_FALSE;
                                        sFoundVariable = NULL;
                                    }
                                }
                                else
                                {
                                    // Nothing to do.
                                }   
                            } // for
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
            } // spec parse tree is not null
            else
            {
                // Nothing to do.
            }
        }// userName is not null
    } // pkgName is null
    else
    {
        // columnName[index].pkgName
        //      -----------------------
        //     |  U  |  T  |  C  |  P  |
        //      -----------------------
        //     |  X  |  X  |  O  |  O  |
        //      -----------------------

        if ( ( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE ) &&
             ( QC_IS_NULL_NAME( aVarNode->tableName ) == ID_TRUE ) )
        {
            if ( sBodyParseTree != NULL )
            {
                for ( sCurrVar = sBodyParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                      sCurrVar = sCurrVar->next )
                {

                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE)
                    {
                        sFoundVariable = ( qsVariables * )sCurrVar;
                        sFoundVariable->common.objectID = QS_EMPTY_OID;
                        *aIsFound = ID_TRUE;

                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            sColumn = QTC_STMT_COLUMN( aStatement,
                                                       (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                            if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                            {
                                sQtcModule = (qtcModule *)sColumn->module;

                                IDE_TEST( searchFieldOfRecord(
                                              aStatement,
                                              sQtcModule->typeInfo,
                                              (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                              aVarNode,
                                              ID_FALSE,
                                              aIsFound )
                                          != IDE_SUCCESS );

                                aVarNode->node.objectID = sFoundVariable->common.objectID;

                                if ( *aIsFound == ID_TRUE )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                    if ( sFoundVariable->inOutType == QS_IN )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
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
                // nothing to do.
            }

            if ( ( *aIsFound == ID_FALSE ) &&
                 ( sSpecParseTree != NULL ) )
            {
                // Spec 변수인지 확인한다.
                for ( sCurrVar = sSpecParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                      sCurrVar = sCurrVar->next )
                {
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                    {
                        sFoundVariable = (qsVariables *)sCurrVar;
                        sFoundVariable->common.objectID = sSpecParseTree->pkgOID;
                        *aIsFound = ID_TRUE;

                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            if ( sSpecParseTree != aStatement->spvEnv->createPkg )
                            {
                                sTemplate = sSpecParseTree->pkgInfo->tmplate;

                                sMtcTuple = ( sTemplate->tmplate.rows ) +
                                    ((qtcNode*)sFoundVariable->variableTypeNode->node.arguments)->node.table;

                                sColumn = QTC_TUPLE_COLUMN( sMtcTuple,
                                                            ((qtcNode*)sFoundVariable->variableTypeNode->
                                                             node.arguments) );
                            }
                            else
                            {
                                // create package 중
                                sColumn = QTC_STMT_COLUMN( aStatement,
                                                           (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );
                            }

                            if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                            {
                                sQtcModule = (qtcModule *)sColumn->module;

                                IDE_TEST( searchFieldOfRecord(
                                              aStatement,
                                              sQtcModule->typeInfo,
                                              (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                              aVarNode,
                                              ID_FALSE,
                                              aIsFound )
                                          != IDE_SUCCESS );

                                if ( sSpecParseTree != aStatement->spvEnv->createPkg )
                                {
                                    aVarNode->node.objectID = sFoundVariable->common.objectID;
                                }
                                else
                                {
                                    aVarNode->node.objectID = QS_EMPTY_OID;
                                }

                                if ( *aIsFound == ID_TRUE )
                                {
                                    aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                    aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                    if ( sFoundVariable->inOutType == QS_IN )
                                    {
                                        aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                        aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    } // QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->userName )
                    else
                    {
                        // Nothing to do.
                    }   
                } // for
            } // ( *aIsFound == ID_FALSE ) && ( sSpecParseTree != NULL )
            else
            {
                // Nothing to do.
            }   
        } // userName is null and tableName is null
        else
        {
            // userName.tableName.columnName[index].pkgName
            //      -----------------------
            //     |  U  |  T  |  C  |  P  |
            //      -----------------------
            //     |  O  |  O  |  O  |  O  |
            //      -----------------------
            if ( sBodyParseTree != NULL )
            {
                if ( sUserID == sBodyParseTree->userID )
                {
                    if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                    {
                        for ( sCurrVar = sBodyParseTree->block->variableItems;
                              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                              sCurrVar = sCurrVar->next )
                        {
                            if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                            {
                                sFoundVariable = ( qsVariables * )sCurrVar;
                                sFoundVariable->common.objectID = QS_EMPTY_OID;
                                *aIsFound = ID_TRUE;

                                if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                {
                                    sColumn = QTC_STMT_COLUMN( aStatement,
                                                               (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );

                                    if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                                    {
                                        sQtcModule = (qtcModule *)sColumn->module;

                                        IDE_TEST( searchFieldOfRecord(
                                                      aStatement,
                                                      sQtcModule->typeInfo,
                                                      (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                                      aVarNode,
                                                      ID_FALSE,
                                                      aIsFound )
                                                  != IDE_SUCCESS );

                                        aVarNode->node.objectID = sFoundVariable->common.objectID;

                                        if ( *aIsFound == ID_TRUE )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                            if ( sFoundVariable->inOutType == QS_IN )
                                            {
                                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                        *aIsFound = ID_FALSE;
                                        sFoundVariable = NULL;
                                    }
                                }
                                else
                                {
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        } // for
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
            } // body parse tree is not null
            else
            {
                // Nothing to do.
            }

            if ( ( *aIsFound == ID_FALSE ) &&
                 ( sSpecParseTree != NULL ) )
            {
                if ( sUserID == sSpecParseTree->userID )
                {
                    if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                    {
                        // Spec 변수인지 확인한다.
                        for ( sCurrVar = sSpecParseTree->block->variableItems;
                              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                              sCurrVar = sCurrVar->next )
                        {
                            // check variable
                            if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                            {
                                sFoundVariable = ( qsVariables * )sCurrVar;
                                sFoundVariable->common.objectID = sSpecParseTree->pkgOID;
                                *aIsFound = ID_TRUE;

                                if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                {
                                    if ( sSpecParseTree != aStatement->spvEnv->createPkg )
                                    {
                                        sTemplate = sSpecParseTree->pkgInfo->tmplate;

                                        sMtcTuple = ( sTemplate->tmplate.rows ) +
                                            ((qtcNode*)sFoundVariable->variableTypeNode->node.arguments)->node.table;

                                        sColumn = QTC_TUPLE_COLUMN( sMtcTuple,
                                                                    (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );
                                    }
                                    else
                                    {
                                        // create package 중
                                        sColumn = QTC_STMT_COLUMN( aStatement,
                                                                   (qtcNode*)sFoundVariable->variableTypeNode->node.arguments );
                                    }

                                    if ( sColumn->module->id == MTD_RECORDTYPE_ID )
                                    {
                                        sQtcModule = (qtcModule *)sColumn->module;

                                        IDE_TEST( searchFieldOfRecord(
                                                      aStatement,
                                                      sQtcModule->typeInfo,
                                                      (qtcNode*)sFoundVariable->variableTypeNode->node.arguments,
                                                      aVarNode,
                                                      ID_FALSE,
                                                      aIsFound )
                                                  != IDE_SUCCESS );

                                        if ( sSpecParseTree != aStatement->spvEnv->createPkg )
                                        {
                                            aVarNode->node.objectID = sFoundVariable->common.objectID;
                                        }
                                        else
                                        {
                                            aVarNode->node.objectID = QS_EMPTY_OID;
                                        }

                                        if ( *aIsFound == ID_TRUE )
                                        {
                                            aVarNode->lflag &= ~QTC_NODE_VALIDATE_MASK;
                                            aVarNode->lflag |= QTC_NODE_VALIDATE_TRUE;

                                            if ( sFoundVariable->inOutType == QS_IN )
                                            {
                                                aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
                                                aVarNode->lflag |= QTC_NODE_OUTBINDING_DISABLE;
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
                                        *aIsFound = ID_FALSE;
                                        sFoundVariable = NULL;
                                    }
                                }
                                else
                                {
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        } // for
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
            } // spec parse tree is not null
            else
            {
                // Nothing to do.
            }
        } // userName is not null and tableName is not null
    } // pkgName is not null

    IDE_EXCEPTION_CONT( NORMAL_PROCEDURE );

    if ( *aIsFound == ID_TRUE )
    {
        /* BUG-37235 */
        aVarNode->lflag &= ~QTC_NODE_PKG_VARIABLE_MASK;
        aVarNode->lflag |= QTC_NODE_PKG_VARIABLE_TRUE;

        /* BUG-39770 */
        aVarNode->lflag &= ~QTC_NODE_PKG_MEMBER_MASK;
        aVarNode->lflag |= QTC_NODE_PKG_MEMBER_EXIST;

        // BUG-47971 Package global variable의 분산 실행을 제한합니다.
        if ( sFoundVariable->inOutType != QS_IN )
        {
            aStatement->spvEnv->flag &= ~QSV_ENV_PKG_VAR_EXIST_MASK;
            aStatement->spvEnv->flag |= QSV_ENV_PKG_VAR_EXIST_TRUE;

            if ( aStatement->spvEnv->createProc != NULL )
            {
                aStatement->spvEnv->createProc->referToPkgVar = ID_TRUE;
            }
        }

        *aVariable = sFoundVariable;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalArrayVarInternal( qcStatement     * aStatement,
                                                   qsPkgParseTree  * aPkgParseTree,
                                                   qsVariableItems * aVariableItems,
                                                   qtcNode         * aArrNode,
                                                   idBool          * aIsFound,
                                                   qsVariables    ** aFoundVariable )
{
/********************************************************************************************
 * Description : BUG-38243 support array method at package.
 *               package local에 선언된 array type variable 검색
 *
 * Implementation : 
 *               다음과 같은 경우만 존재                    ( qtcNode에서 qcNamePosition )
 *               (1) arrayVar_name                        -- tableName
 *               (2) myPkg_name.arrayVar_name             -- userName.tableName
 *               (3) myUser_name.myPkg_name.arrayVar_name -- userName.tableName.columnName
 *      qsvProcVar::searchArrayVar에서 찾은 변수에 대해 array type인지 마지막으로 확인.
 ********************************************************************************************/

    qcNamePosition * sUserName;
    qcNamePosition * sPkgName;
    qcNamePosition * sArrName;
    idBool           sIsFound  = ID_FALSE;
    UInt             sUserID   = 0;

    /* pkgName이 null일 때는 method function 정보가 qtcNode의 columnName에 존재하지만,
       반대의 경우, pkgName에 method function 정보가 존재한다.
       따라서 pkgName에 대해서 NULL인지 아닌지 구분해야 한다. */
    if ( QC_IS_NULL_NAME( aArrNode->pkgName ) == ID_TRUE )
    {
        sUserName = NULL;
        sPkgName = &aArrNode->userName;
        sArrName = &aArrNode->tableName;
    }
    else
    {
        sUserName = &aArrNode->userName;
        sPkgName = &aArrNode->tableName;
        sArrName = &aArrNode->columnName;
    }

    if ( sUserName == NULL )
    {
        if ( QC_IS_NULL_NAME( (*sPkgName) ) == ID_TRUE )
        {
            sIsFound = ID_TRUE;
        } /* QC_IS_NULL_NAME( (*sPkgName) ) == ID_TRUE */
        else
        {
            if ( QC_IS_NAME_MATCHED( (*sPkgName), aPkgParseTree->pkgNamePos ) )
            {
                sIsFound = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }
        }/* QC_IS_NULL_NAME( (*sPkgNmae) ) == ID_FALSE */
    } /* sUserName == NULL */
    else
    {
        IDE_TEST( qcmUser::getUserID( aStatement, 
                                      (*sUserName),
                                      &sUserID )
                  != IDE_SUCCESS );

        if ( sUserID == aPkgParseTree->userID )
        {
            if ( QC_IS_NAME_MATCHED( (*sPkgName), aPkgParseTree->pkgNamePos ) )
            {
                sIsFound = ID_TRUE;
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
    } /* sUserName != NULL */

    if( sIsFound == ID_TRUE )
    {
        IDE_TEST( searchVariableItems( aVariableItems,
                                       aStatement->spvEnv->currDeclItem,
                                       sArrName,
                                       aIsFound,
                                       aFoundVariable )
                  != IDE_SUCCESS );
    }
    else
    {
        *aIsFound       = sIsFound;
        *aFoundVariable = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgArrayVar( qcStatement  * aStatement,
                                      qtcNode      * aArrNode, 
                                      idBool       * aIsFound,
                                      qsVariables ** aFoundVariable )
{
/********************************************************************************************
 * Description : BUG-38243 support array method at package.
 *               자신이 속한 package가 아닌
 *               다른 package 객체에 선언된 array type variable 검색
 *               qsvProcVarr::searchArrayVar에서 호출됨.
 *
 * Implementation :
 *               다음과 같은 경우만 존재                    ( qtcNode에서 qcNamePosition )
 *               (1) pkg_name.arrayVar_name                   -- tableName
 *               (2) user_name.pkg_name.arrayVar_name         -- userName.tableName
 *      qsvProcVar::searchArrayVar에서 찾은 변수에 대해 array type인지 마지막으로 확인.
 ********************************************************************************************/

    qcNamePosition   sUserName;
    qcNamePosition   sPkgName;
    qcNamePosition   sArrName;
    qsOID            sPkgOID;
    UInt             sPkgUserID;
    qsxPkgInfo     * sPkgInfo;
    idBool           sExists         = ID_FALSE;
    qcmSynonymInfo   sSynonymInfo;
    idBool           sIsFound        = ID_FALSE;
    qsVariables    * sFoundVariable  = NULL;

    /* pkgName이 null일 때는 method function 정보가 qtcNode의 columnName에 존재하지만,
       반대의 경우, pkgName에 method function 정보가 존재한다.
       따라서 pkgName에 대해서 NULL인지 아닌지 구분해야 한다. */
    if ( QC_IS_NULL_NAME( aArrNode->pkgName ) == ID_TRUE )
    {
        SET_EMPTY_POSITION( sUserName );
        sPkgName = aArrNode->userName;
        sArrName = aArrNode->tableName;
    }
    else
    {
        sUserName = aArrNode->userName;
        sPkgName = aArrNode->tableName;
        sArrName = aArrNode->columnName;
    }

    IDE_TEST( qcmSynonym::resolvePkg( aStatement,
                                      sUserName,
                                      sPkgName,
                                      &sPkgOID,
                                      &sPkgUserID,
                                      &sExists,
                                      &sSynonymInfo )
              != IDE_SUCCESS );

    if ( sExists == ID_TRUE )
    {
        IDE_TEST( qsvPkgStmts::makePkgSynonymList( 
                      aStatement,
                      &sSynonymInfo,
                      sUserName,
                      sPkgName,
                      sPkgOID )
                  != IDE_SUCCESS );

        IDE_TEST( qsvPkgStmts::makeRelatedObjects(
                      aStatement,
                      & sUserName,
                      & sPkgName,
                      & sSynonymInfo,
                      0,
                      QS_PKG )
                  != IDE_SUCCESS );

        IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree(
                      aStatement,
                      sPkgOID,
                      QS_PKG,
                      &(aStatement->spvEnv->procPlanList) )
                  != IDE_SUCCESS );

        IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                      &sPkgInfo )
                  != IDE_SUCCESS );

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

        IDE_TEST( searchVariableItems( sPkgInfo->planTree->block->variableItems,
                                       aStatement->spvEnv->currDeclItem,
                                       & sArrName,
                                       & sIsFound,
                                       & sFoundVariable )
                  != IDE_SUCCESS );

        if ( sIsFound == ID_TRUE )
        {
            sFoundVariable->common.objectID = sPkgInfo->pkgOID;

            *aFoundVariable = sFoundVariable;
        }
        else
        {
            // Nothing to do.
        } 

        *aIsFound = sIsFound;
    }
    else
    {
        *aIsFound = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */ 
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalArrayVar( qcStatement     * aStatement,
                                           qtcNode         * aArrNode,
                                           idBool          * aIsFound,
                                           qsVariables    ** aFoundVariable ) 
{
/********************************************************************************************
 * Description : BUG-38243 support array method at package.
 *               package local에 선언된 array type variable 검색
 *               qsvProcVarr::searchArrayVar에서 호출됨.
 *
 * Implementation :
 *              (1) create package spec
 *                     a. package spec에서만 탐색
 *              (2) create package body
 *                     a. package body 탐색
 *                     b. package spec 탐색
 *      qsvProcVar::searchArrayVar에서 찾은 변수에 대해 array type인지 마지막으로 확인.
 ********************************************************************************************/

    qsPkgParseTree * sPkgSpecParseTree;
    qsPkgParseTree * sPkgBodyParseTree;
    idBool           sIsFound       = ID_FALSE;
    qsVariables    * sFoundVariable = NULL;

    if ( aStatement->spvEnv->pkgPlanTree == NULL ) 
    {
        /* aStatement->spvEnv->pkgPlanTree은
           create package body 중에 package body에 대응하는
           package spec 정보를 셋팅해 놓은 변수이다.
           따라서 aStatement->spvEnv->pkgPlanTree가 NULL은
           create package의 validate 과정이라는 의미이다.*/
        IDE_DASSERT( aStatement->spvEnv->createPkg != NULL );

        sPkgSpecParseTree = aStatement->spvEnv->createPkg;
        sPkgBodyParseTree = NULL;
    }
    else
    {
        IDE_DASSERT( aStatement->spvEnv->createPkg != NULL );
        IDE_DASSERT( aStatement->spvEnv->pkgPlanTree != NULL );

        sPkgSpecParseTree = aStatement->spvEnv->pkgPlanTree;
        sPkgBodyParseTree = aStatement->spvEnv->createPkg;
    }

    if ( sPkgBodyParseTree != NULL )
    { 
        IDE_TEST( searchPkgLocalArrayVarInternal( 
                      aStatement,
                      sPkgBodyParseTree,
                      sPkgBodyParseTree->block->variableItems,
                      aArrNode,
                      &sIsFound,
                      &sFoundVariable)
                  != IDE_SUCCESS );

        if ( sIsFound == ID_TRUE )
        {
            sFoundVariable->common.objectID = QS_EMPTY_OID;
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

    if ( sIsFound == ID_FALSE )
    {
        IDE_TEST( searchPkgLocalArrayVarInternal( 
                      aStatement,
                      sPkgSpecParseTree,
                      sPkgSpecParseTree->block->variableItems,
                      aArrNode,
                      &sIsFound,
                      &sFoundVariable )
                  != IDE_SUCCESS );

        if ( sIsFound == ID_TRUE )
        {
            if ( sPkgSpecParseTree == aStatement->spvEnv->createPkg )
            {
                sFoundVariable->common.objectID = QS_EMPTY_OID;
            }
            else
            {
                sFoundVariable->common.objectID = sPkgSpecParseTree->pkgOID;
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

    if ( sIsFound == ID_TRUE )
    {
        *aFoundVariable = sFoundVariable;
    }
    else
    {
        // Nothing to do.
    }

    *aIsFound = sIsFound;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aIsFound = ID_FALSE;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchVarAndParaForArray( qcStatement      * aStatement,
                                             qtcNode          * aVarNode,
                                             idBool           * aIsFound,
                                             qsVariables     ** aVariable,
                                             const mtfModule **aModule  )
{
/***********************************************************************
 *
 * Description : PROJ-2533
 *               aVarNode에 맞는 psm array variable을 검색한다.
 *
 * Implementation :
 *               qtcNode에는 총 4가지 position을 입력받을 수 있다.
 *               user_Name, table_Name, column_Name, pkg_Name
 *
 *         associative array type의 index를 쓰는 경우
 *             ex) V1[1] / V1(1)
 *
 ***********************************************************************/
    qsPkgParseTree * sPkgParseTree = NULL;

    *aIsFound     = ID_FALSE;
    *aVariable    = NULL;
    sPkgParseTree = aStatement->spvEnv->createPkg;

    // Argument가 있는 경우.(associative array type의 index를 쓰는 경우
    // array variable output이 필요하다.
    // column module에서 array variable을 이용하여 index에 해당하는 row를
    // 가져와야 하기 때문.
    IDE_TEST( searchVariableForArray( aStatement,
                                      aVarNode,
                                      aIsFound,
                                      aVariable,
                                      aModule )
              != IDE_SUCCESS );

    if ( *aIsFound == ID_FALSE )
    {
        IDE_TEST( searchParameterForArray( aStatement,
                                           aVarNode,
                                           aIsFound,
                                           aVariable,
                                           aModule )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( *aIsFound == ID_FALSE )
    {
        IDE_TEST( searchPkgLocalVarForArray( aStatement,
                                             sPkgParseTree,
                                             aVarNode,
                                             aIsFound,
                                             aVariable,
                                             aModule )
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

IDE_RC qsvProcVar::searchVariableForArray( qcStatement      * aStatement,
                                           qtcNode          * aVarNode,
                                           idBool           * aIsFound,
                                           qsVariables     ** aVariable,
                                           const mtfModule ** aModule )
{
/***********************************************************************
 *
 * Description : PROJ-2533
 *               aVarNode에 맞는 psm array variable을 검색한다.
 *               만약, array type의 member function인 경우 모듈 탐색한다.
 * Implementation :
 *        올 수 있는 associative array type
 *               1. column_name( expression ) 또는 ()
 *                 1) variable_name( index )
 *                 2) parameter_name( index )
 *               2. table_name.column_name( expression ) 또는 ()
 *                 1) label_name.variable_name( index )
 *                 2) proc_name.parameter_name( index )
 *                 3) variable_name.member_function( index )
 *                 4) parameter_name.member_function( index )
 *               3. user_name.table_name.column_name( index ) 또는 ()
 *                  1) label_name.variable_name.member_function( index )
 *                  2) label_name.parameter_name.member_function( index )
 *
 ***********************************************************************/
    qsAllVariables      * sCurrVar        = NULL;
    qsLabels            * sLabel          = NULL;
    qsVariables         * sFoundVariable  = NULL;
    qsVariableItems     * sCurrDeclItem   = NULL;
    SChar               * sRealSQL        = qsvProcVar::getValidSQL( aStatement );
    idBool                sIsMemberFunc   = ID_FALSE;
    const mtfModule     * sModule         = NULL;
    idBool                sIsArrayMemFunc = ID_FALSE;

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;

    // search local variables
    if (QC_IS_NULL_NAME(aVarNode->userName) == ID_TRUE)
    {
        if (QC_IS_NULL_NAME(aVarNode->tableName) == ID_TRUE)
        {
            if ( QC_IS_NULL_NAME(aVarNode->pkgName) == ID_TRUE )
            { 
                // 1. column_name[ expression ]
                //   1) variable_name[ index ]
                for (sCurrVar = aStatement->spvEnv->allVariables;
                     (sCurrVar != NULL) && (*aIsFound == ID_FALSE);
                     sCurrVar = sCurrVar->next)
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->columnName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType != QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        sFoundVariable = NULL;
                    }       
                }
            } // pkgName is null.
            else
            {
                // Nothing to do.
            }
        } // tableName is null.
        else
        {
            // 2. table_name.column_name[ expression ]
            // 2-3) variable_name.member_function( index )
            IDE_TEST( qsf::moduleByName( &sModule,
                                         &sIsMemberFunc,
                                         (void*)(aVarNode->columnName.stmtText +
                                                 aVarNode->columnName.offset),
                                         aVarNode->columnName.size )
                      != IDE_SUCCESS );

            for ( sCurrVar = aStatement->spvEnv->allVariables;
                  ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                  sCurrVar = sCurrVar->next )
            {
                if ( sIsMemberFunc == ID_TRUE )
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->tableName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    // Nothing to do.
                }

                for ( sLabel = sCurrVar->labels;
                      ( sLabel != NULL ) && ( *aIsFound == ID_FALSE );
                      sLabel = sLabel->next )
                {
                    if (idlOS::strMatch(
                            sRealSQL + sLabel->namePos.offset,
                            sLabel->namePos.size,
                            aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                            aVarNode->tableName.size) == 0)
                    {
                        IDE_TEST( searchVariableItems(
                                      sCurrVar->variableItems,
                                      sCurrDeclItem,
                                      &aVarNode->columnName,
                                      aIsFound,
                                      &sFoundVariable )
                                  != IDE_SUCCESS );

                        if ( *aIsFound == ID_TRUE )
                        {
                            if ( sFoundVariable->variableType != QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                } // for .. label
            } // for .. allVariables
        } // tableName is not null.
    } // userName is null.
    else
    {
        // 3. user_name.table_name.column_name( index )
        //   1) label_name.variable_name.member_function( index )
        IDE_TEST( qsf::moduleByName( &sModule,
                                     &sIsMemberFunc,
                                     (void*)(aVarNode->columnName.stmtText +
                                             aVarNode->columnName.offset),
                                     aVarNode->columnName.size )
                  != IDE_SUCCESS );

        for ( sCurrVar = aStatement->spvEnv->allVariables;
              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE ) &&
                  ( sIsMemberFunc == ID_TRUE );
              sCurrVar = sCurrVar->next )
        {
            for ( sLabel = sCurrVar->labels;
                  ( sLabel != NULL ) && ( *aIsFound == ID_FALSE );
                  sLabel = sLabel->next )
            {
                if ( idlOS::strMatch(
                         sRealSQL + sLabel->namePos.offset,
                         sLabel->namePos.size,
                         aVarNode->userName.stmtText + aVarNode->userName.offset,
                         aVarNode->userName.size) == 0)
                {
                    IDE_TEST( searchVariableItems(
                                  sCurrVar->variableItems,
                                  sCurrDeclItem,
                                  &aVarNode->tableName,
                                  aIsFound,
                                  &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            } // for .. label
        } // for .. allVariables
    } // userName is not null

    if ( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;

        if ( sIsArrayMemFunc == ID_TRUE )
        {
            *aModule = sModule;
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchParameterForArray( qcStatement      * aStatement,
                                            qtcNode          * aVarNode,
                                            idBool           * aIsFound,
                                            qsVariables     ** aVariable,
                                            const mtfModule ** aModule )
{
/***********************************************************************
 *
 * Description : PROJ-2533
 *               aVarNode에 맞는 psm array variable을 검색한다.
 *               만약, array type의 member function인 경우 모듈 탐색한다.
 * Implementation :
 *        올 수 있는 associative array type
 *               1. column_name( expression ) 또는 () 
 *                 1) variable_name( index )
 *                 2) parameter_name( index )
 *               2. table_name.column_name( expression ) 또는 ()
 *                 1) label_name.variable_name( index )
 *                 2) proc_name.parameter_name( index )
 *                 3) variable_name.member_function( index )
 *                 4) parameter_name.member_function( index )
 *               3. user_name.table_name.column_name( index ) 또는 ()
 *                  1) label_name.variable_name.member_function( index )
 *                  2) label_name.parameter_name.member_function( index )
 *
 ***********************************************************************/
    qsProcParseTree     * sParseTree      = NULL;
    qsVariables         * sFoundVariable  = NULL;
    qsVariableItems     * sCurrDeclItem   = NULL;
    idBool                sIsMemberFunc   = ID_FALSE;
    const mtfModule     * sModule         = NULL;
    idBool                sIsArrayMemFunc = ID_FALSE;

    sParseTree = aStatement->spvEnv->createProc;

    // To fix BUG-14129
    // 현재 validate중인 declare item 전까지만 검색을 해야 함
    sCurrDeclItem = aStatement->spvEnv->currDeclItem;

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;
    *aIsFound = ID_FALSE;       // TASK-3876 Code Sonar

    if ( QC_IS_NULL_NAME(aVarNode->userName) == ID_TRUE )
    {
        if ( QC_IS_NULL_NAME(aVarNode->tableName) == ID_TRUE )
        {
            if ( QC_IS_NULL_NAME(aVarNode->pkgName) == ID_TRUE )
            {
                // procedureName이 없는 경우 무조건 parameter라고 가정.
                // 1. column_name[ expression ]
                //   2) parameter_name[ index ]
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aVarNode->columnName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    if ( sFoundVariable->variableType !=
                         QS_ASSOCIATIVE_ARRAY_TYPE )
                    {
                        *aIsFound = ID_FALSE;
                        sFoundVariable = NULL;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
                else
                {
                    sFoundVariable = NULL;
                }
            } // pkgName is null
            else
            {
                // Nothing to do.
            } // pkgName is not null
        } // tableName is null
        else
        {
            // 2. table_name.column_name[ expression ]
            if ( sParseTree != NULL )
            {
                IDE_TEST( qsf::moduleByName( &sModule,
                                             &sIsMemberFunc,
                                             (void*)(aVarNode->columnName.stmtText +
                                                     aVarNode->columnName.offset),
                                             aVarNode->columnName.size )
                          != IDE_SUCCESS );

                // 2-4) parameter_name.member_function( index )
                if ( sIsMemberFunc == ID_TRUE )
                {
                    IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                                   sCurrDeclItem,
                                                   &aVarNode->tableName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    // Nothing to do.
                }

                // 2-2) proc_name.parameter_name( index )
                if ( ( idlOS::strMatch(
                           sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                           sParseTree->procNamePos.size,
                           aVarNode->tableName.stmtText + aVarNode->tableName.offset,
                           aVarNode->tableName.size) == 0 ) &&
                     ( *aIsFound == ID_FALSE ) )
                {
                    IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                                   sCurrDeclItem,
                                                   &aVarNode->columnName,
                                                   aIsFound,
                                                   &sFoundVariable )
                              != IDE_SUCCESS );

                    if ( *aIsFound == ID_TRUE )
                    {
                        if ( sFoundVariable->variableType != QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                        else
                        {
                            // Nothing to do.
                        }
                    }
                    else
                    {
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    // Nothing to do.
                }
            }
            else
            {
                *aIsFound = ID_FALSE;
            }
        }
    }
    else
    {
        if ( sParseTree != NULL )
        {
            // 3. user_name.table_name.column_name( expression )
            //   2) proc_name.parameter_name.member_function( index )
            IDE_TEST( qsf::moduleByName( &sModule,
                                         &sIsMemberFunc,
                                         (void*)(aVarNode->columnName.stmtText +
                                                 aVarNode->columnName.offset),
                                         aVarNode->columnName.size )
                      != IDE_SUCCESS );

            if ( ( idlOS::strMatch(
                       sParseTree->procNamePos.stmtText + sParseTree->procNamePos.offset,
                       sParseTree->procNamePos.size,
                       aVarNode->userName.stmtText + aVarNode->userName.offset,
                       aVarNode->userName.size) == 0 ) &&
                 ( sIsMemberFunc == ID_TRUE ) )
            {
                IDE_TEST( searchVariableItems( aStatement->spvEnv->allParaDecls,
                                               sCurrDeclItem,
                                               &aVarNode->tableName,
                                               aIsFound,
                                               &sFoundVariable )
                          != IDE_SUCCESS );

                if ( *aIsFound == ID_TRUE )
                {
                    if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                    {
                        sIsArrayMemFunc = ID_TRUE;
                    }
                    else
                    {
                        *aIsFound = ID_FALSE;
                        sFoundVariable = NULL;
                    }
                }
                else
                {
                    sFoundVariable = NULL;
                }
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            *aIsFound = ID_FALSE;
        }
    } // userName is not null

    if ( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;

        if ( sIsArrayMemFunc == ID_TRUE )
        {
            *aModule = sModule;
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchPkgLocalVarForArray( qcStatement      * aStatement,
                                              qsPkgParseTree   * aPkgParseTree,
                                              qtcNode          * aVarNode,
                                              idBool           * aIsFound,
                                              qsVariables     ** aVariable,
                                              const mtfModule ** aModule )
{
/************************************************************
 *
 * Description : PROJ-2533
 *               aVarNode에 맞는 local package의 array variable을 검색한다.
 *               만약, array type의 member function인 경우 모듈 탐색한다. 
 *
 * Implementation :
 *    이 함수에 올 수 있는 경우
 *       1. column_name (list) 또는 ()
 *           1) specVariable_name
 *           2) bodyVariable_name
 *       2. table_name.column_name (list) 또는 ()
 *           1) specVariable_name.member_function
 *           2) bodyVariable_name.member_function
 *           3) pkg_name.specVariable_name
 *           4) pkg_name.bodyVariable_name
 *       3. user_name.table_name.column_name (list) 또는 ()
 *           1) pkg_name.specVariable_name.member_function
 *           2) pkg_name.bodyVariable_name.member_function
 *           3) user_name.pkg_name.specVariable_name
 *           4) user_name.pkg_name.bodyVariable_name
 *
 *************************************************************/
    qsPkgParseTree      * sBodyParseTree  = NULL;
    qsPkgParseTree      * sSpecParseTree  = NULL;
    qsVariables         * sFoundVariable  = NULL;
    qsVariableItems     * sCurrVar        = NULL;
    UInt                  sUserID         = 0;
    idBool                sIsMemberFunc   = ID_FALSE;
    const mtfModule     * sModule         = NULL;
    idBool                sIsArrayMemFunc = ID_FALSE;

    sBodyParseTree = NULL;
    sSpecParseTree = NULL;

    if ( aPkgParseTree != NULL )
    {
        if ( aPkgParseTree->objType == QS_PKG )
        {
            sSpecParseTree = aPkgParseTree;
        }
        else
        {
            sBodyParseTree = aPkgParseTree;
            sSpecParseTree = aStatement->spvEnv->pkgPlanTree;
        }
    }
    else
    {
        // 일반 procedure에서 package 변수를 사용한 경우
    }


    IDE_TEST_CONT ( ( sBodyParseTree == NULL ) &&
                    ( sSpecParseTree == NULL ),
                    SKIP_SEARCH );

    IDE_TEST_CONT( QC_IS_NULL_NAME( aVarNode->pkgName ) == ID_FALSE,
                   SKIP_SEARCH );

    if ( QC_IS_NULL_NAME( aVarNode->userName ) == ID_FALSE )
    {
        if ( qcmUser::getUserID( aStatement,
                                 aVarNode->userName,
                                 &sUserID )
             != IDE_SUCCESS )
        {
            sUserID = ID_UINT_MAX;
            IDE_CLEAR();
        }
        else
        {
            // Nothing to do.
        }
    }
    else
    {
        sUserID = ID_UINT_MAX;
    }

    // initialize
    aVarNode->lflag &= ~QTC_NODE_OUTBINDING_MASK;
    aVarNode->lflag |= QTC_NODE_OUTBINDING_ENABLE;
    *aIsFound = ID_FALSE;       // TASK-3876 Code Sonar

    if ( QC_IS_NULL_NAME( aVarNode->userName ) == ID_TRUE )
    {
        if ( QC_IS_NULL_NAME( aVarNode->tableName ) == ID_TRUE )
        {
            if ( sBodyParseTree != NULL )
            {
                //1-2) bodyVariable_name()
                for ( sCurrVar = sBodyParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                      sCurrVar = sCurrVar->next )
                {
                    // check variable
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                    {
                        sFoundVariable = (qsVariables *)sCurrVar;
                        sFoundVariable->common.objectID = QS_EMPTY_OID;

                        if ( sFoundVariable->variableType ==
                             QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
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

            if ( ( *aIsFound == ID_FALSE ) &&
                 ( sSpecParseTree != NULL ) )
            {
                // 1-1) specVariable_name()
                // Spec 변수인지 확인한다.
                for ( sCurrVar = sSpecParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                      sCurrVar = sCurrVar->next )
                {
                    // check variable
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                    {
                        sFoundVariable = (qsVariables *)sCurrVar;
                        sFoundVariable->common.objectID = sSpecParseTree->pkgOID;

                        if ( sFoundVariable->variableType ==
                             QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
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
        } // tableName is null;
        else
        {
            //TAG : table_name.column_name ()
            //      -----------------------
            //     |  U  |  T  |  C  |  P  |
            //      -----------------------
            //     |  X  |  O  |  O  |  X  |
            //      -----------------------

            if ( sBodyParseTree != NULL )
            {
                // 2-2) bodyVariable_name.member_function()
                IDE_TEST( qsf::moduleByName( &sModule,
                                             &sIsMemberFunc,
                                             (void*)(aVarNode->columnName.stmtText +
                                                     aVarNode->columnName.offset),
                                             aVarNode->columnName.size )
                          != IDE_SUCCESS );

                for ( sCurrVar = sBodyParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE ) &&
                          ( sIsMemberFunc == ID_TRUE );
                      sCurrVar = sCurrVar->next )
                {
                    // check variable
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->tableName ) == ID_TRUE )
                    {
                        sFoundVariable = (qsVariables *)sCurrVar;
                        sFoundVariable->common.objectID = QS_EMPTY_OID;

                        if ( sFoundVariable->variableType ==
                             QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_TRUE;
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                // 2-4) pkg_name.bodyVariable_name()
                if ( ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) ) &&
                     ( *aIsFound == ID_FALSE ) )
                {
                    for ( sCurrVar = sBodyParseTree->block->variableItems;
                          ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                          sCurrVar = sCurrVar->next )
                    {
                        // check variable
                        if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                        {
                            sFoundVariable = ( qsVariables * )sCurrVar;
                            sFoundVariable->common.objectID = QS_EMPTY_OID;

                            if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                *aIsFound = ID_TRUE;
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
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
            }
            else
            {
                // Nothing to do.
            }

            if ( ( *aIsFound == ID_FALSE ) &&
                 ( sSpecParseTree != NULL ) )
            {
                // 2-1) specVariable_name.member_function()
                IDE_TEST( qsf::moduleByName( &sModule,
                                             &sIsMemberFunc,
                                             (void*)(aVarNode->columnName.stmtText +
                                                     aVarNode->columnName.offset),
                                             aVarNode->columnName.size )
                          != IDE_SUCCESS );

                for ( sCurrVar = sSpecParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE ) &&
                          ( sIsMemberFunc == ID_TRUE );
                      sCurrVar = sCurrVar->next )
                {
                    // check variable
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->tableName ) == ID_TRUE )
                    {
                        sFoundVariable = (qsVariables *)sCurrVar;
                        sFoundVariable->common.objectID = sSpecParseTree->pkgOID;

                        if ( sFoundVariable->variableType ==
                             QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_TRUE;
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }

                // 2-3) pkg_name.specVariable_name()
                if ( ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) ) &&
                     ( *aIsFound == ID_FALSE ) )
                {
                    for ( sCurrVar = sSpecParseTree->block->variableItems;
                          ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                          sCurrVar = sCurrVar->next )
                    {
                        // check variable
                        if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                        {
                            sFoundVariable = ( qsVariables * )sCurrVar;
                            sFoundVariable->common.objectID = sSpecParseTree->pkgOID;
                            *aIsFound = ID_TRUE;

                            if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                            {
                                *aIsFound = ID_TRUE;
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
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
            } // ( *aIsFound == ID_FALSE ) && ( sSpecParseTree != NULL )
            else
            {
                // Nothing to do.
            }
        } // tableName is not null
    } // userName is null
    else
    {
        // userName.tableName.columnName[index]
        //      -----------------------
        //     |  U  |  T  |  C  |  P  |
        //      -----------------------
        //     |  O  |  O  |  O  |  X  |
        //      -----------------------

        if ( sBodyParseTree != NULL )
        {
            if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->userName ) == ID_TRUE )
            {
                // 3-2) mypkg_name.bodyVariable_name.member_function()
                IDE_TEST( qsf::moduleByName( &sModule,
                                             &sIsMemberFunc,
                                             (void*)(aVarNode->columnName.stmtText +
                                                     aVarNode->columnName.offset),
                                             aVarNode->columnName.size )
                          != IDE_SUCCESS );

                for ( sCurrVar = sBodyParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE ) &&
                          ( sIsMemberFunc == ID_TRUE );
                      sCurrVar = sCurrVar->next )
                {
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->tableName ) == ID_TRUE )
                    {
                        sFoundVariable = ( qsVariables * )sCurrVar;
                        sFoundVariable->common.objectID = QS_EMPTY_OID;

                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_TRUE;
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            // myUser.myPkg.arr[index]
            else
            {
                // 3-4) user_name.pkg_name.bodyVariable_name()
                if ( sUserID == sBodyParseTree->userID )
                {
                    if ( QC_IS_NAME_MATCHED( sBodyParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                    {
                        for ( sCurrVar = sBodyParseTree->block->variableItems;
                              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                              sCurrVar = sCurrVar->next )
                        {
                            // check variable
                            if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                            {
                                sFoundVariable = ( qsVariables * )sCurrVar;
                                sFoundVariable->common.objectID = QS_EMPTY_OID;

                                if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                {
                                    *aIsFound = ID_TRUE;
                                }
                                else
                                {
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
                                }
                            }
                            else
                            {
                                *aIsFound = ID_FALSE;
                                sFoundVariable = NULL;
                            }
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
        } // body parse tree is not null
        else
        {
            // Nothing to do.
        }

        // 3-1) pkg_name.specVariable_name.member_function()
        if ( ( *aIsFound == ID_FALSE ) &&
             ( sSpecParseTree != NULL ) )
        {

            if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->userName ) == ID_TRUE )
            {
                IDE_TEST( qsf::moduleByName( &sModule,
                                             &sIsMemberFunc,
                                             (void*)(aVarNode->columnName.stmtText +
                                                     aVarNode->columnName.offset),
                                             aVarNode->columnName.size )
                          != IDE_SUCCESS );

                for ( sCurrVar = sSpecParseTree->block->variableItems;
                      ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE ) &&
                          ( sIsMemberFunc == ID_TRUE );
                      sCurrVar = sCurrVar->next )
                {
                    if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->tableName ) == ID_TRUE )
                    {
                        sFoundVariable = ( qsVariables * )sCurrVar;
                        sFoundVariable->common.objectID = sSpecParseTree->pkgOID;

                        if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                        {
                            *aIsFound = ID_TRUE;
                            sIsArrayMemFunc = ID_TRUE;
                        }
                        else
                        {
                            *aIsFound = ID_FALSE;
                            sFoundVariable = NULL;
                        }
                    }
                    else
                    {
                        // Nothing to do.
                    }
                }
            }
            // myUser.myPkg.arr[index]
            else
            {
                // 3-3) user_name.pkg_name.specVariable_name()
                if ( sUserID == sSpecParseTree->userID )
                {
                    if ( QC_IS_NAME_MATCHED( sSpecParseTree->pkgNamePos, aVarNode->tableName ) == ID_TRUE )
                    {
                        // Spec 변수인지 확인한다.
                        for ( sCurrVar = sSpecParseTree->block->variableItems;
                              ( sCurrVar != NULL ) && ( *aIsFound == ID_FALSE );
                              sCurrVar = sCurrVar->next )
                        {
                            // check variable
                            if ( QC_IS_NAME_MATCHED( sCurrVar->name, aVarNode->columnName ) == ID_TRUE )
                            {
                                sFoundVariable = ( qsVariables * )sCurrVar;
                                sFoundVariable->common.objectID = sSpecParseTree->pkgOID;

                                if ( sFoundVariable->variableType == QS_ASSOCIATIVE_ARRAY_TYPE )
                                {
                                    *aIsFound = ID_TRUE;
                                }
                                else
                                {
                                    *aIsFound = ID_FALSE;
                                    sFoundVariable = NULL;
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
                }   
                else
                {
                    // Nothing to do.
                }
            }
        } // spec parse tree is not null
        else
        {
            // Nothing to do.
        }
    }// userName is not null

    IDE_EXCEPTION_CONT( SKIP_SEARCH );

    if ( *aIsFound == ID_TRUE )
    {
        *aVariable = sFoundVariable;

        if ( sIsArrayMemFunc == ID_TRUE )
        {
            *aModule = sModule;
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::searchVariableFromPkgForArray( qcStatement      * aStatement,
                                                  qtcNode          * aVarNode,
                                                  idBool           * aIsFound,
                                                  qsVariables     ** aVariable,
                                                  const mtfModule ** aModule )
{
    qsOID              sPkgOID       = QS_EMPTY_OID;
    UInt               sPkgUserID    = 0;
    qsxPkgInfo       * sPkgInfo      = NULL;
    idBool             sExists       = ID_FALSE;
    qcmSynonymInfo     sSynonymInfo;
    idBool             sIsMemberFunc = ID_FALSE;
    const mtfModule  * sModule       = NULL;

    *aIsFound = ID_FALSE;
    *aVariable = NULL;

    IDE_TEST( qsf::moduleByName( &sModule,
                                 &sIsMemberFunc,
                                 (void*)(aVarNode->columnName.stmtText +
                                         aVarNode->columnName.offset),
                                 aVarNode->columnName.size )
              != IDE_SUCCESS );

    if ( sIsMemberFunc == ID_TRUE )
    { 
        IDE_TEST( qsvProcVar::searchPkgArrayVar( aStatement,
                                                 aVarNode,
                                                 aIsFound,
                                                 aVariable )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }

    if ( *aIsFound == ID_FALSE )
    {
        //search variable declared in package
        IDE_TEST( qcmSynonym::resolvePkg( aStatement,
                                          aVarNode->userName,
                                          aVarNode->tableName,
                                          &sPkgOID,
                                          &sPkgUserID,
                                          &sExists,
                                          &sSynonymInfo )
                  != IDE_SUCCESS );

        if ( sExists == ID_TRUE )
        {
            // synonym으로 참조되는 proc의 기록
            IDE_TEST( qsvPkgStmts::makePkgSynonymList( aStatement,
                                                       &sSynonymInfo,
                                                       aVarNode->userName,
                                                       aVarNode->tableName,
                                                       sPkgOID )
                      != IDE_SUCCESS );

            IDE_TEST( qsvPkgStmts::makeRelatedObjects( aStatement,
                                                       &aVarNode->userName,
                                                       &aVarNode->tableName,
                                                       & sSynonymInfo,
                                                       0,
                                                       QS_PKG )
                      != IDE_SUCCESS );

            IDE_TEST( qsxRelatedProc::prepareRelatedPlanTree( aStatement,
                                                              sPkgOID,
                                                              QS_PKG,
                                                              &(aStatement->spvEnv->procPlanList))
                      != IDE_SUCCESS );

            IDE_TEST( qsxPkg::getPkgInfo( sPkgOID,
                                          &sPkgInfo )
                      != IDE_SUCCESS );

            /* BUG-45164 */
            IDE_TEST_RAISE( sPkgInfo->isValid != ID_TRUE, err_object_invalid );

            IDE_TEST( qdpRole::checkDMLExecutePSMPriv( aStatement,
                                                       sPkgInfo->planTree->userID,
                                                       sPkgInfo->privilegeCount,
                                                       sPkgInfo->granteeID,
                                                       ID_FALSE,
                                                       NULL,
                                                       NULL )
                      != IDE_SUCCESS );

            IDE_TEST( searchPkgVarWithArg( aStatement,
                                           sPkgInfo,
                                           aVarNode,
                                           aIsFound,
                                           aVariable )
                      != IDE_SUCCESS );

            if ( *aIsFound == ID_TRUE )
            {
                if ( (*aVariable)->variableType != QS_ASSOCIATIVE_ARRAY_TYPE )
                {
                    *aIsFound = ID_FALSE;
                    *aVariable = NULL;
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
    } // *aIsFound == ID_FALSE
    else
    {
        *aModule = sModule;
    } // *aIsFound == ID_TRUE

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_object_invalid );   /* BUG-45164 */ 
    {
        IDE_SET( ideSetErrorCode(qpERR_ABORT_QSX_PLAN_INVALID) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

//BUG-46032
IDE_RC qsvProcVar::makeInternalArrayVariable( qcStatement      * aStatement,
                                           qtcNode          * aNode,
                                           qsVariables      * aParentArray,
                                           qsVariables     ** aChildArray )
{
    qtcNode          * sInnerNode;
    qtcNode          * sTypeNode[2];
    qsVariables      * sVariable;
    qtcModule        * sRowModule;
    qsTypes          * sType;

    sRowModule = (qtcModule *)aParentArray->typeInfo->columns->next->basicInfo->module;
    IDE_DASSERT( sRowModule->module.id == MTD_ASSOCIATIVE_ARRAY_ID );

    // 노드 생성
    IDE_TEST( STRUCT_ALLOC( QC_QMP_MEM(aStatement),
                            qtcNode,
                            &sInnerNode )
              != IDE_SUCCESS);

    idlOS::memcpy( sInnerNode, aNode, ID_SIZEOF(qtcNode) );
 
    QTC_NODE_INIT( aNode );

    aNode->node.module    = sInnerNode->node.module;
    aNode->node.next      = sInnerNode->node.next;
    aNode->node.orgNode   = (mtcNode *)sInnerNode;
    aNode->node.arguments = NULL;
    aNode->position       = sInnerNode->position;
    aNode->columnName     = sInnerNode->position;

    aNode->lflag = sInnerNode->lflag;

    aNode->lflag &= ~QTC_NODE_SP_NESTED_ARRAY_MASK;
    aNode->lflag |= QTC_NODE_SP_NESTED_ARRAY_TRUE;

    aNode->lflag &= ~QTC_NODE_PROC_VAR_ESTIMATE_MASK;
    aNode->lflag |= QTC_NODE_PROC_VAR_ESTIMATE_FALSE;

    aNode->lflag &= ~QTC_NODE_SP_ARRAY_INDEX_VAR_MASK;
    aNode->lflag |= QTC_NODE_SP_ARRAY_INDEX_VAR_ABSENT;

    // 중간 array variable 만들기
    // qsTypes 생성
    IDE_TEST( qsvProcType::makeArrayTypeColumnByModule( QC_QME_MEM(aStatement),
                                                        sRowModule,
                                                        &sType)
              != IDE_SUCCESS);

    // qsVariables 생성
    IDE_TEST(STRUCT_ALLOC(QC_QME_MEM(aStatement), qsVariables, &sVariable)
             != IDE_SUCCESS);

    sVariable->common.itemType      = QS_VARIABLE;
    SET_POSITION( sVariable->common.name,
                  aNode->position );
    sVariable->common.table         = ID_USHORT_MAX;
    sVariable->common.column        = ID_USHORT_MAX;
    sVariable->common.objectID      = QS_EMPTY_OID;
    sVariable->common.next          = NULL;

    IDE_TEST( qtc::makeProcVariable( aStatement,
                                     sTypeNode,
                                     & aNode->columnName,
                                     NULL,
                                     QTC_PROC_VAR_OP_NEXT_COLUMN )
              != IDE_SUCCESS );

    sVariable->variableTypeNode   = sTypeNode[0];
    sVariable->defaultValueNode   = NULL;
    sVariable->variableType       = QS_UD_TYPE;
    sVariable->inOutType          = QS_INOUT;
    sVariable->typeInfo           = NULL;
    sVariable->nocopyType         = QS_NOCOPY;

    IDE_TEST( qsvProcVar::makeArrayVariable( aStatement,
                                             sType,
                                             sVariable)
              != IDE_SUCCESS );

    *aChildArray = sVariable;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsvProcVar::makeRecordColumnByName( qcStatement      * aStatement,
                                           qtcNode          * aRecord,
                                           qcNamePosition   * aFieldName,
                                           qtcNode         ** aColumn )
{
    /****************************************************************
     * BUG-46174
     *
     * 아래의 규칙에 따라 레코드의 필드 이름을 채운다.
     *
     *  No | userName | tableName | columnName | pkgName | arguments
     * --------------------------------------------------------------
     *  1  |          |           |  record    |         |
     *  1' |          |  record   |  field     |         |
     *
     *  2  |          |  package  |  record    |         |
     *  2' |  package |  record   |  field     |         |
     *
     *  3  |  user    |  package  |  record    |         |
     *  3' |  user    |  package  |  record    |  field  |
     *
     *  4  |          |           |  record    |         |  index
     *  4' |          |           |  record    |  field  |  index
     *
     *  5  |          |  package  |  record    |         |  index
     *  5' |  package |  record   |  field     |         |  index
     *
     *  6  |  user    |  package  |  record    |         |  index
     *  6' |  user    |  package  |  record    |  field  |  index
     *
     *********************************************************/

    IDE_TEST( qtc::makeProcVariable( aStatement,
                                     aColumn,
                                     &(aRecord->position),
                                     NULL,
                                     QTC_PROC_VAR_OP_NONE )
              != IDE_SUCCESS);

    if ( aRecord->node.arguments != NULL )
    {
        // array의 index 연결
        aColumn[0]->node.arguments = aRecord->node.arguments;

        aColumn[0]->lflag &= ~QTC_NODE_SP_ARRAY_INDEX_VAR_MASK;
        aColumn[0]->lflag |= QTC_NODE_SP_ARRAY_INDEX_VAR_EXIST;

        aColumn[0]->node.lflag &= ~MTC_NODE_ARGUMENT_COUNT_MASK;
        aColumn[0]->node.lflag |= 1;
    }

    if ( QC_IS_NULL_NAME( aRecord->userName ) == ID_TRUE ) 
    {
        if ( QC_IS_NULL_NAME( aRecord->tableName ) == ID_TRUE )
        {
            if ( aRecord->node.arguments == NULL )
            {
                // 1) C -> T.C ( C is field name position )
                SET_POSITION( aColumn[0]->tableName, aRecord->columnName );
                SET_POSITION( aColumn[0]->columnName, *aFieldName );
            }
            else
            {
                // 4) C[idx] -> C[idx].P ( P is field name position )
                SET_POSITION( aColumn[0]->columnName, aRecord->columnName );
                SET_POSITION( aColumn[0]->pkgName, *aFieldName );
            }
        }
        else
        {
            // 2) T.C      -> U.T.C ( C is field name position )
            // 5) T.C[idx] -> U.T[idx].C ( C is field name position )
            SET_POSITION( aColumn[0]->userName, aRecord->tableName );
            SET_POSITION( aColumn[0]->tableName, aRecord->columnName );
            SET_POSITION( aColumn[0]->columnName, *aFieldName );
        }
    }
    else
    {
        // 3) U.T.C      -> U.T.C.P ( P is field name position )
        // 6) U.T.C[idx] -> U.T.C[idx].P ( P is field name position )
        SET_POSITION( aColumn[0]->userName, aRecord->userName );
        SET_POSITION( aColumn[0]->tableName, aRecord->tableName );
        SET_POSITION( aColumn[0]->columnName, aRecord->columnName );
        SET_POSITION( aColumn[0]->pkgName, *aFieldName );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

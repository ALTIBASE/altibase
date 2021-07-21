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
 * $Id$
 *
 * Description :
 *
 *     ALTIBASE SHARD manage ment function
 *
 * Syntax :
 *    SHARD_SET_SHARD_RAGNE( user_name     VARCHAR,
 *                           table_name    VARCHAR,
 *                           value         VARCHAR,
 *                           sub_value     VARCHAR,
 *                           node_name     VARCHAR,
 *                           set_func_type VARCHAR )
 *    RETURN 0
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>
#include <sdiZookeeper.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName sdfFunctionName[1] = {
    { NULL, 28, (void*)"SHARD_SET_SHARD_RANGE_GLOBAL" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetShardRangeGlobalModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_SetShardRangeGlobal( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetShardRangeGlobal,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC sdfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[6] =
    {
        &mtdVarchar, // user_name
        &mtdVarchar, // table_name
        &mtdVarchar, // value
        &mtdVarchar, // sub_value
        &mtdVarchar, // node_name
        &mtdVarchar, // set_function_type 'H'=HASH,'R'=RANGE,'L'=LIST,'P'=COMPOSITE
    };
    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 6,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = sdfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC sdfCalculate_SetShardRangeGlobal( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Set Shard Range String
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    mtdCharType             * sUserName;
    SChar                     sUserNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sTableName;
    SChar                     sTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sNodeName;
    SChar                     sNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdCharType             * sValue;
    SChar                     sValueStr[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    mtdCharType             * sSubValue;
    SChar                     sSubValueStr[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    mtdCharType             * sSetFuncType;
    SChar                     sSetFuncTypeStr[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    SChar                     sPartitionName[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmTablePartitionType     sTablePartitionType = QCM_PARTITIONED_TABLE;

    sdiClientInfo           * sClientInfo  = NULL;
    ULong                     sSMN = ID_ULONG(0);
    idBool                    sIsTableFound = ID_FALSE;
    sdiTableInfo              sTableInfo;
    UInt                      sExecCount = 0;
    SChar                   * sSplitType;
    idBool                    sIsAlive = ID_FALSE;
    SChar                     sObjectValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1]; 
    SChar                     sSqlStr[SDF_QUERY_LEN];
    
    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

    sObjectValue[0] = '\0';

    // BUG-46366
    IDE_TEST_RAISE( ( QC_SMI_STMT(sStatement)->getTrans() == NULL ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DML ) == QCI_STMT_MASK_DML ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DCL ) == QCI_STMT_MASK_DCL ),
                    ERR_INSIDE_QUERY );

    // Check Privilege
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(sStatement) != QCI_SYS_USER_ID,
                    ERR_NO_GRANT );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) || // user name
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) || // table name
         ( aStack[3].column->module->isNull( aStack[3].column,
                                             aStack[3].value ) == ID_TRUE ) || // value
         ( aStack[5].column->module->isNull( aStack[5].column,
                                             aStack[5].value ) == ID_TRUE ) || // node name
         ( aStack[6].column->module->isNull( aStack[6].column,
                                             aStack[6].value ) == ID_TRUE ) )  // set function type
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------

        // user name
        sUserName = (mtdCharType*)aStack[1].value;

        IDE_TEST_RAISE( sUserName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_USER_NAME_TOO_LONG );
        idlOS::strncpy( sUserNameStr,
                        (SChar*)sUserName->value,
                        sUserName->length );
        sUserNameStr[sUserName->length] = '\0';

        // object name
        sTableName = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sTableName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_TABLE_NAME_TOO_LONG );
        idlOS::strncpy( sTableNameStr,
                        (SChar*)sTableName->value,
                        sTableName->length );
        sTableNameStr[sTableName->length] = '\0';

        // value
        sValue = (mtdCharType*)aStack[3].value;

        IDE_TEST_RAISE( sValue->length > SDI_RANGE_VARCHAR_MAX_PRECISION,
                        ERR_SHARD_MAX_VALUE_TOO_LONG );
        idlOS::strncpy( sValueStr,
                        (SChar*)sValue->value,
                        sValue->length );
        sValueStr[sValue->length] = '\0';
        
        // PARTITION VALUE를 입력과 동일하게 ' 붙여 준다.
        if ( sValueStr[0] == '\'' )
        {
            // INPUT ARG ('''A''') => 'A' => '''A'''
            idlOS::snprintf( sObjectValue,
                             SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                             "''%s''",
                             sValueStr );
        }
        else
        {
            // INPUT ARG ('A') => A => 'A'
            idlOS::snprintf( sObjectValue,
                             SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                             "'%s'",
                             sValueStr );
        }

        // sub value
        sSubValue = (mtdCharType*)aStack[4].value;

        IDE_TEST_RAISE( sSubValue->length > SDI_RANGE_VARCHAR_MAX_PRECISION,
                        ERR_SHARD_MAX_VALUE_TOO_LONG );
        idlOS::strncpy( sSubValueStr,
                        (SChar*)sSubValue->value,
                        sSubValue->length );
        sSubValueStr[sSubValue->length] = '\0';

        // shard node name
        sNodeName = (mtdCharType*)aStack[5].value;

        IDE_TEST_RAISE( sNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                        ERR_SHARD_GROUP_NAME_TOO_LONG );
        idlOS::strncpy( sNodeNameStr,
                        (SChar*)sNodeName->value,
                        sNodeName->length );
        sNodeNameStr[sNodeName->length] = '\0';

        // set function type
        sSetFuncType = (mtdCharType*)aStack[6].value;

        IDE_TEST_RAISE( sSetFuncType->length != 1,
                        ERR_ARGUMENT_NOT_APPLICABLE );
        idlOS::strncpy( sSetFuncTypeStr,
                        (SChar*)sSetFuncType->value,
                        sSetFuncType->length );
        sSetFuncTypeStr[sSetFuncType->length] = '\0';

        if ( sSetFuncTypeStr[0] == 'H' )
        {
            sSplitType = (SChar*)"HASH";
        }
        else if ( sSetFuncTypeStr[0] == 'R' )
        {
            sSplitType = (SChar*)"RANGE";

        }
        else if ( sSetFuncTypeStr[0] == 'L' )
        {
             sSplitType = (SChar*)"LIST";
        }
        else if ( sSetFuncTypeStr[0] == 'P' )
        {
             sSplitType = (SChar*)"COMPOSITE";
        }
        else
        {
            IDE_RAISE( ERR_INVALID_RANGE_FUNCTION );
        }

        //---------------------------------
        // Validation
        //---------------------------------
        
        IDE_TEST( sdi::getIncreasedSMNForMetaNode( ( QC_SMI_STMT( sStatement ) )->getTrans() , 
                                                   &sSMN ) 
                  != IDE_SUCCESS );

        sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
        sOldStmt                = QC_SMI_STMT(sStatement);
        QC_SMI_STMT(sStatement) = &sSmiStmt;
        sState = 1;

        IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                  sOldStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );
        sState = 2;


        // SetShardRange 는 등록된 Procedure 혹은 Partitioned Table만 대상으로한다.
        IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( sStatement ),
                                     sUserNameStr,
                                     sTableNameStr,
                                     sSMN, //sMetaNodeInfo.mShardMetaNumber,
                                     &sTableInfo,
                                     &sIsTableFound )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                        ERR_NOT_EXIST_TABLE );

        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;

        if ( sTableInfo.mObjectType == 'T' )
        {
            switch (sdi::getShardObjectType(&sTableInfo))
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_TEST_RAISE( aStack[4].column->module->isNull( aStack[4].column,
                                                           aStack[4].value ) != ID_TRUE, ERR_ARGUMENT_NOT_APPLICABLE );                   
                    /* HASH, RANGE, LIST 의 경우에는 파티션 이름을 알아와서 Ranges_에 넣어줘야 한다. */
                    IDE_TEST( sdm::getPartitionNameByValue( QC_SMI_STMT( sStatement ),
                                                            sUserNameStr,
                                                            sTableNameStr,
                                                            sObjectValue,
                                                            &sTablePartitionType,
                                                            sPartitionName ) != IDE_SUCCESS );

                    if ( idlOS::strncmp( sPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 ) == 0 )
                    {
                        // partitioned table의 경우 range value와 partition의 경계값이 맞는 것이 없으면 에러를 반환한다.
                        IDE_TEST_RAISE( sTablePartitionType != QCM_NONE_PARTITIONED_TABLE, ERR_SHARD_PARTITION_VALUE_NOT_EXIST );
                    }
                    else
                    {
                        /* do nothing */
                    }
                    break;
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                    IDE_TEST_RAISE( sdi::getShardObjectType(&sTableInfo) != SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT, ERR_ARGUMENT_NOT_APPLICABLE );
                    /* COMPOSITE 은 파티션 이름과 매핑하지 않고, Ranges_에는 SDM_NA_STR로 입력된다. */
                    idlOS::strncpy( sPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 );

                    break;
                case SDI_CLONE_DIST_OBJECT:
                case SDI_SOLO_DIST_OBJECT:
                case SDI_NON_SHARD_OBJECT:
                default:
                    IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
                    break;
            }
        }
        else
        {
            idlOS::strncpy( sPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 );
        }

        //---------------------------------
        // Check Session Property
        //---------------------------------
        IDE_TEST_RAISE( QCG_GET_SESSION_IS_AUTOCOMMIT( sStatement ) == ID_TRUE,
                        ERR_COMMIT_STATE );

        IDE_TEST_RAISE( QCG_GET_SESSION_GTX_LEVEL( sStatement ) < 2,
                        ERR_GTX_LEVEL );

        IDE_TEST_RAISE ( QCG_GET_SESSION_TRANSACTIONAL_DDL( sStatement) != ID_TRUE,
                         ERR_DDL_TRANSACTION );

        //---------------------------------
        // Insert Meta to All Node
        //---------------------------------
        if ( sSetFuncTypeStr[0] == 'P' )
        {
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "exec dbms_shard.set_shard_%s_local( "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT" )",
                             sSplitType,
                             sUserNameStr,
                             sTableNameStr,
                             sValueStr,
                             sSubValueStr,
                             sNodeNameStr );
        }
        else
        {
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "exec dbms_shard.set_shard_%s_local( "
                             QCM_SQL_VARCHAR_FMT", "
                             QCM_SQL_VARCHAR_FMT", "
                             "%s, "
                             QCM_SQL_VARCHAR_FMT" )",
                             sSplitType,
                             sUserNameStr,
                             sTableNameStr,
                             sObjectValue,
                             sNodeNameStr );
        }

        IDE_TEST( sdi::compareDataAndSessionSMN( sStatement ) != IDE_SUCCESS );

        sdi::setShardMetaTouched( sStatement->session );

        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(sStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );

        IDE_TEST( sdi::checkShardLinker( sStatement ) != IDE_SUCCESS );

        sClientInfo = sStatement->session->mQPSpecific.mClientInfo;

        // 전체 Node 대상으로 Meta Update 실행
        if ( sClientInfo != NULL )
        {
            IDE_TEST( sdi::shardExecDirectNested( sStatement,
                                            NULL,
                                            (SChar*)sSqlStr,
                                            (UInt) idlOS::strlen( sSqlStr ),
                                            SDI_INTERNAL_OP_NORMAL,
                                            &sExecCount)
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sExecCount == 0,
                            ERR_INVALID_SHARD_NODE );
        }
        else
        {
            IDE_RAISE( ERR_NODE_NOT_EXIST );
        }
    }

    // revert job all remove
    if ( sdiZookeeper::isExistRevertJob() == ID_TRUE )
    {
        (void) sdiZookeeper::removeRevertJob();
        IDE_DASSERT( sdiZookeeper::isExistRevertJob() != ID_TRUE );
    }

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLUSTER_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_ZKC_DEADNODE_EXIST ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION( ERR_INVALID_RANGE_FUNCTION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_INVALID_RANGE_FUNCTION ) );
    }
    IDE_EXCEPTION( ERR_NOT_EXIST_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_TABLE_NOT_EXIST ) );
    }
    IDE_EXCEPTION( ERR_COMMIT_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_CANNOT_EXECUTE_IN_AUTOCOMMIT_MODE ) );
    }
    IDE_EXCEPTION( ERR_GTX_LEVEL )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_GTX_LEVEL ) );
    }
    IDE_EXCEPTION( ERR_INSIDE_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_PSM_INSIDE_QUERY ) );
    }
    IDE_EXCEPTION( ERR_SHARD_USER_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_USER_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_SHARD_TABLE_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_TABLE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_SHARD_GROUP_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_SHARD_MAX_VALUE_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_MAX_VALUE_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_SHARD_PARTITION_VALUE_NOT_EXIST );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_PARTITION_VALUE_NOT_EXIST, sValueStr, sTableNameStr ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION( ERR_DDL_TRANSACTION );
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_INSUFFICIENT_ATTRIBUTE,
                                 "TRANSACTIONAL_DDL = 1" ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_SD_1);
            }
            else
            {
                /* Nothing to do */
            }
        case 1:
            QC_SMI_STMT(sStatement) = sOldStmt;
        default:
            break;
    }
    
    if ( sdiZookeeper::isExistRevertJob() == ID_TRUE )
    {
        (void) sdiZookeeper::executeRevertJob( ZK_REVERT_JOB_REPL_ITEM_DROP );
        (void) sdiZookeeper::removeRevertJob();
        IDE_DASSERT( sdiZookeeper::isExistRevertJob() != ID_TRUE );
    }

    IDE_POP();

    return IDE_FAILURE;
}

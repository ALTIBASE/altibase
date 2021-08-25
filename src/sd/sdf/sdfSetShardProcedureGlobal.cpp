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
 *     ALTIBASE SHARD management function
 *
 * Syntax :
 *    SHARD_SET_SHARD_PROCEDURE( user_name VARCHAR,
 *                               proc_name VARCHAR,
 *                               split_method VARCHAR,
 *                               shard_key_parameter_name VARCHAR,
 *                               sub_split_method VARCHAR,
 *                               sub_shard_key_parameter_name VARCHAR,
 *                               default_node_name VARCHAR )
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
    { NULL, 32, (void*)"SHARD_SET_SHARD_PROCEDURE_GLOBAL" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetShardProcedureGlobalModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_SetShardProcedureGlobal( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetShardProcedureGlobal,
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
    const mtdModule* sModules[7] =
    {
        &mtdVarchar, // user_name
        &mtdVarchar, // table_name
        &mtdVarchar, // split_method
        &mtdVarchar, // key_column_name
        &mtdVarchar, // sub_split_method
        &mtdVarchar, // sub_key_column_name
        &mtdVarchar  // default_node_name
    };
    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 7,
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

IDE_RC sdfCalculate_SetShardProcedureGlobal( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Set Shard Table Info
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
    mtdCharType             * sKeyColumnName;
    SChar                     sKeyColumnNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sSplitMethod;
    SChar                     sSplitMethodStr[1 + 1];
    mtdCharType             * sSubKeyColumnName;
    SChar                     sSubKeyColumnNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sSubSplitMethod;
    SChar                     sSubSplitMethodStr[1 + 1];
    mtdCharType             * sDefaultNodeName;
    SChar                     sDefaultNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];

    sdiClientInfo           * sClientInfo  = NULL;
    UInt                      sExecCount = 0;
    idBool                    sIsAlive = ID_FALSE;
    SChar                     sSqlStr[SDF_QUERY_LEN];
    
    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

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
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) ||
         ( aStack[3].column->module->isNull( aStack[3].column,
                                             aStack[3].value ) == ID_TRUE ) )
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

        // table name
        sTableName = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sTableName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_TABLE_NAME_TOO_LONG );
        idlOS::strncpy( sTableNameStr,
                        (SChar*)sTableName->value,
                        sTableName->length );
        sTableNameStr[sTableName->length] = '\0';

        // split method
        sSplitMethod = (mtdCharType*)aStack[3].value;
        IDE_TEST_RAISE( sSplitMethod->length > 1,
                        ERR_INVALID_SHARD_SPLIT_METHOD_NAME );

        IDE_TEST( sdi::getSplitMethodCharByStr((SChar*)sSplitMethod->value, &(sSplitMethodStr[0])) != IDE_SUCCESS );
        sSplitMethodStr[1] = '\0';

        if ( ( sSplitMethodStr[0] == 'C' ) ||
             ( sSplitMethodStr[0] == 'S' ) )
        {
            // clone 및 solo table은 shard key와 sub split method, sub shard key 그리고 default node가 null이어야 한다.
            IDE_TEST_RAISE( ( ( aStack[4].column->module->isNull( aStack[4].column,
                                                                  aStack[4].value ) != ID_TRUE ) || // shard key
                              ( aStack[5].column->module->isNull( aStack[5].column,
                                                                  aStack[5].value ) != ID_TRUE ) || // sub split method
                              ( aStack[6].column->module->isNull( aStack[6].column,
                                                                  aStack[6].value ) != ID_TRUE ) || // sub shard key
                              ( aStack[7].column->module->isNull( aStack[7].column,
                                                                  aStack[7].value ) != ID_TRUE ) ), // default node
                            ERR_ARGUMENT_NOT_APPLICABLE );
        }
        else
        {
            // default node는 null일 수 있다.
            IDE_TEST_RAISE( aStack[4].column->module->isNull( aStack[4].column,
                                                              aStack[4].value ) == ID_TRUE, // shard key
                            ERR_ARGUMENT_NOT_APPLICABLE );
        }

        // key column name
        sKeyColumnName = (mtdCharType*)aStack[4].value;

        IDE_TEST_RAISE( sKeyColumnName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_KEYCOLUMN_NAME_TOO_LONG );
        idlOS::strncpy( sKeyColumnNameStr,
                        (SChar*)sKeyColumnName->value,
                        sKeyColumnName->length );
        sKeyColumnNameStr[sKeyColumnName->length] = '\0';

        /* PROJ-2655 Composite shard key */
        sSubSplitMethod = (mtdCharType*)aStack[5].value;

        IDE_TEST_RAISE( sSubSplitMethod->length > 1,
                        ERR_INVALID_SHARD_SPLIT_METHOD_NAME );

        if ( sSubSplitMethod->length == 1 )
        {   
            if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                  "H", 1 ) == 0 )
            {
                sSubSplitMethodStr[0] = 'H';
                sSubSplitMethodStr[1] = '\0';
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "R", 1 ) == 0 )
            {
                sSubSplitMethodStr[0] = 'R';
                sSubSplitMethodStr[1] = '\0';
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "C", 1 ) == 0 )
            {
                /* Sub-shard key의 split method는 clone일 수 없다. */
                IDE_RAISE( ERR_UNSUPPORTED_SUB_SHARD_KEY_SPLIT_TYPE );
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "S", 1 ) == 0 )
            {
                /* Sub-shard key의 split method는 solo일 수 없다. */
                IDE_RAISE( ERR_UNSUPPORTED_SUB_SHARD_KEY_SPLIT_TYPE );
            }
            else if ( idlOS::strMatch( (SChar*)sSubSplitMethod->value, sSubSplitMethod->length,
                                       "L", 1 ) == 0 )
            {
                sSubSplitMethodStr[0] = 'L';
                sSubSplitMethodStr[1] = '\0';
            }
            else
            {
                IDE_RAISE( ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
            }

            // sub-shard key의 split method가 null이 아닌 경우에는 반드시 sub-shard key가 세팅 되어야 한다.
            IDE_TEST_RAISE( aStack[6].column->module->isNull( aStack[6].column,
                                                              aStack[6].value ) == ID_TRUE, // sub shard key
                            ERR_ARGUMENT_NOT_APPLICABLE );
        }
        else
        {
            sSubSplitMethodStr[0] = '\0';
        }

        // sub key column name
        sSubKeyColumnName = (mtdCharType*)aStack[6].value;

        IDE_TEST_RAISE( sSubKeyColumnName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_KEYCOLUMN_NAME_TOO_LONG );
        idlOS::strncpy( sSubKeyColumnNameStr,
                        (SChar*)sSubKeyColumnName->value,
                        sSubKeyColumnName->length );
        sSubKeyColumnNameStr[sSubKeyColumnName->length] = '\0';

        if ( sSubKeyColumnName->length > 0 )
        {
            // Shard key와 sub-shard key는 같은 column일 수 없다.
            IDE_TEST_RAISE( idlOS::strMatch( (SChar*)sKeyColumnName->value, sKeyColumnName->length,
                                             (SChar*)sSubKeyColumnName->value, sSubKeyColumnName->length ) == 0,
                            ERR_DUPLICATED_SUB_SHARD_KEY_NAME );

            IDE_TEST_RAISE( sSubSplitMethod->length != 1, ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
        }
        else
        {
            // Nothing to do.
        }

        // default node name
        sDefaultNodeName = (mtdCharType*)aStack[7].value;

        IDE_TEST_RAISE( sDefaultNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                        ERR_SHARD_GROUP_NAME_TOO_LONG );
        idlOS::strncpy( sDefaultNodeNameStr,
                        (SChar*)sDefaultNodeName->value,
                        sDefaultNodeName->length );
        sDefaultNodeNameStr[sDefaultNodeName->length] = '\0';

        //---------------------------------
        // Check Session Property
        //---------------------------------
        IDE_TEST_RAISE( QCG_GET_SESSION_IS_AUTOCOMMIT( sStatement ) == ID_TRUE,
                        ERR_COMMIT_STATE );

        IDE_TEST_RAISE( QCG_GET_SESSION_GTX_LEVEL( sStatement ) < 2,
                        ERR_GTX_LEVEL );

        IDE_TEST_RAISE ( QCG_GET_SESSION_TRANSACTIONAL_DDL( sStatement) != ID_TRUE,
                         ERR_DDL_TRANSACTION );
        
        /* PROJ-2757 Advanced Global DDL */
        IDE_TEST_RAISE ( QCG_GET_SESSION_GLOBAL_DDL( sStatement) == ID_TRUE,
                         ERR_GLOBAL_DDL );
        
        //---------------------------------
        // Insert Meta to All Node
        //---------------------------------

        IDE_TEST( sdi::compareDataAndSessionSMN( sStatement ) != IDE_SUCCESS );

        sdi::setShardMetaTouched( sStatement->session );
        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(sStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );

        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "exec dbms_shard.set_shard_procedure_local( "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT" )",
                         sUserNameStr,
                         sTableNameStr,
                         sSplitMethodStr,
                         sKeyColumnNameStr,
                         sSubSplitMethodStr,
                         sSubKeyColumnNameStr,
                         sDefaultNodeNameStr );

        // Zookeeper에서 Node 정보를 받아 온다.
        // IDE_TEST( sdi::zookeeper_get_node_list() != IDE_SUCCESS );
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
    IDE_EXCEPTION( ERR_SHARD_KEYCOLUMN_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_KEYCOLUMN_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_SPLIT_METHOD_NAME ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
    }
    IDE_EXCEPTION( ERR_SHARD_GROUP_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION( ERR_DUPLICATED_SUB_SHARD_KEY_NAME )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SUB_SHARD_KEY_NAME ) );
    }
    IDE_EXCEPTION( ERR_UNSUPPORTED_SUB_SHARD_KEY_SPLIT_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_UNSUPPORTED_SUB_SHARD_KEY_SPLIT_TYPE ) );
    }
    IDE_EXCEPTION( ERR_DDL_TRANSACTION );
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_INSUFFICIENT_ATTRIBUTE,
                                 "TRANSACTIONAL_DDL = 1" ));
    }
    IDE_EXCEPTION( ERR_GLOBAL_DDL );
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_INSUFFICIENT_ATTRIBUTE,
                                 "GLOBAL_DDL = 0" ));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

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
 *    SHARD_SET_SHARD_TABLE_LOCAL( user_name VARCHAR,
 *                                 table_name VARCHAR,
 *                                 split_method VARCHAR,
 *                                 shard_key_column_name VARCHAR,
 *                                 sub_split_method VARCHAR,
 *                                 sub_shard_key_column_name VARCHAR,
 *                                 default_node_name VARCHAR )
 *    RETURN 0
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName sdfFunctionName[1] = {
    { NULL, 21, (void*)"SHARD_SET_SHARD_TABLE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetShardTableModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_SetShardTable( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetShardTable,
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

IDE_RC sdfCalculate_SetShardTable( mtcNode*     aNode,
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
    mtdCharType             * sTableName = NULL;
    SChar                     sTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sKeyColumnName = NULL;
    SChar                     sKeyColumnNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sSplitMethod = NULL;
    SChar                     sSplitMethodStr[1 + 1];
    mtdCharType             * sSubKeyColumnName = NULL;
    SChar                     sSubKeyColumnNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sSubSplitMethod = NULL;
    SChar                     sSubSplitMethodStr[1 + 1];
    mtdCharType             * sDefaultNodeName = NULL;
    SChar                     sDefaultNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    SChar                     sDefaultPartitionNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    qcmTablePartitionType     sTablePartitionType = QCM_PARTITIONED_TABLE;

    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt = NULL;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;
    sdiSplitMethod 			  sSdSplitMethod = SDI_SPLIT_NONE;

    SChar                     sBackupTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    sdiLocalMetaInfo          sLocalMetaInfo;
    UInt                      sErrCode;
    SChar                     sSqlStr[SDF_QUERY_LEN];
    
    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( ( sStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
         QC_SESSION_SHARD_META_TOUCH_TRUE )
    {
        sIsOldSessionShardMetaTouched = ID_TRUE;
    }

    // BUG-46366
    IDE_TEST_RAISE( ( QC_SMI_STMT(sStatement)->getTrans() == NULL ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DML ) == QCI_STMT_MASK_DML ) ||
                    ( ( sStatement->myPlan->parseTree->stmtKind & QCI_STMT_MASK_DCL ) == QCI_STMT_MASK_DCL ),
                    ERR_INSIDE_QUERY );

    // Check Privilege
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_ID(sStatement) != QCI_SYS_USER_ID,
                    ERR_NO_GRANT );

    if ( SDU_SHARD_LOCAL_FORCE != 1 )
    {
        /* Shard Local Operation은 internal 에서만 수행되어야 한다.  */
        IDE_TEST_RAISE( ( QCG_GET_SESSION_IS_SHARD_INTERNAL_LOCAL_OPERATION( sStatement ) != ID_TRUE ) &&
                        ( ( sStatement->session->mQPSpecific.mFlag & QC_SESSION_ALTER_META_MASK )
                             != QC_SESSION_ALTER_META_ENABLE),
                        ERR_INTERNAL_OPERATION );
    }

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
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

        /* statement 생성해서 해야함 그렇지 않으면 아래 함수에서 assert 발생. */
        IDE_TEST( sdm::getSplitMethodByPartition( QC_SMI_STMT( sStatement ),
                                                  sUserNameStr, 
                                                  sTableNameStr, 
                                                  &sSdSplitMethod ) != IDE_SUCCESS );

        // split method
        if ( aStack[3].column->module->isNull( aStack[3].column,
                                               aStack[3].value ) == ID_TRUE )
        {
            /* Table의 SplitMethod가 NONE일 경우 split method는 NULL이면 안된다. */
            IDE_TEST_RAISE( sSdSplitMethod == SDI_SPLIT_NONE, ERR_DO_NOT_MATCH_SPLIT_METHOD );
            /* NULL 이면 partition 정보를 가져와서 자동으로 설정한다. */
            IDE_TEST( sdi::getSplitMethodCharByType(sSdSplitMethod, &(sSplitMethodStr[0])) != IDE_SUCCESS );
            sSplitMethodStr[1] = '\0';
        }
        else
        {
            sSplitMethod = (mtdCharType*)aStack[3].value;
            IDE_TEST_RAISE( sSplitMethod->length > 1,
                            ERR_INVALID_SHARD_SPLIT_METHOD_NAME );
            IDE_TEST( sdi::getSplitMethodCharByStr((SChar*)sSplitMethod->value, &(sSplitMethodStr[0])) != IDE_SUCCESS );
            sSplitMethodStr[1] = '\0';
        }

        if ( ( sSplitMethodStr[0] == 'C' ) ||
             ( sSplitMethodStr[0] == 'S' ) )
        {
            // clone 및 solo table은 shard key와 sub split method, sub shard key 및 default node가 null이어야 한다.
            IDE_TEST_RAISE( ( ( aStack[4].column->module->isNull( aStack[4].column,
                                                                  aStack[4].value ) != ID_TRUE ) || // shard key
                              ( aStack[5].column->module->isNull( aStack[5].column,
                                                                  aStack[5].value ) != ID_TRUE ) || // sub split method
                              ( aStack[6].column->module->isNull( aStack[6].column,
                                                                  aStack[6].value ) != ID_TRUE ) || // sub shard key
                              ( aStack[7].column->module->isNull( aStack[7].column,
                                                                  aStack[7].value ) != ID_TRUE ) ), // default node
                            ERR_ARGUMENT_NOT_APPLICABLE );
            sKeyColumnNameStr[0] = '\0';
            if ( sSplitMethodStr[0] == 'C' ) /* CLONE */
            {
                sSdSplitMethod = SDI_SPLIT_CLONE;
            }
            else /* SOLO */
            {
                sSdSplitMethod = SDI_SPLIT_SOLO;
            }
        }
        else
        {
            //shard key
            if (aStack[4].column->module->isNull(aStack[4].column, aStack[4].value) == ID_TRUE)
            {
                /* NULL 이면 partition 정보를 가져와서 자동으로 설정한다. */
                IDE_TEST( sdm::getShardKeyStrByPartition( QC_SMI_STMT( sStatement ),
                                                          sUserNameStr,
                                                          sTableNameStr,
                                                          QC_MAX_OBJECT_NAME_LEN + 1,
                                                          sKeyColumnNameStr ) != IDE_SUCCESS );
            }
            else
            {
                // key column name
                sKeyColumnName = (mtdCharType*)aStack[4].value;

                IDE_TEST_RAISE( sKeyColumnName->length > QC_MAX_OBJECT_NAME_LEN,
                                ERR_SHARD_KEYCOLUMN_NAME_TOO_LONG );
                idlOS::strncpy( sKeyColumnNameStr,
                                (SChar*)sKeyColumnName->value,
                                sKeyColumnName->length );
                sKeyColumnNameStr[sKeyColumnName->length] = '\0';
            }
        }

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

        /* Null String을 입력하면 Default Partition 정보를 Return 한다 */
        IDE_TEST( sdm::getPartitionNameByValue( QC_SMI_STMT( sStatement ),
                                                sUserNameStr,
                                                sTableNameStr,
                                                (SChar *)"",
                                                &sTablePartitionType,
                                                sDefaultPartitionNameStr ) != IDE_SUCCESS );

        switch ( sTablePartitionType )
        {
            case QCM_PARTITIONED_TABLE:
                // default partition name을 가져온 경우에는 길이를 점검한다.
                IDE_TEST_RAISE( (SInt)idlOS::strlen(sDefaultPartitionNameStr) > QC_MAX_OBJECT_NAME_LEN,
                                ERR_SHARD_PARTITION_NAME_TOO_LONG );
                break;
            case QCM_TABLE_PARTITION:
                IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
                break;
            case QCM_NONE_PARTITIONED_TABLE:
                // 일반 테이블일 경우에는 SDM_NA_STR가 입력된다.
                IDE_TEST_RAISE( idlOS::strncmp( sDefaultPartitionNameStr, SDM_NA_STR,
                                                IDL_MIN(QC_MAX_OBJECT_NAME_LEN, SDM_NA_STR_SIZE) ) != 0,
                                ERR_DO_NOT_MATCH_SPLIT_METHOD )

                break;
            default:
                IDE_DASSERT(0);
        }

        //---------------------------------
        // Validation
        //---------------------------------
        IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
        /* Table의 SplitMethod가 None이거나 Composite Type일 경우  k-safety는 0 이어야 한다. */
        IDE_TEST_RAISE( (sLocalMetaInfo.mKSafety != 0) && ( sSdSplitMethod == SDI_SPLIT_NONE ) , ERR_KSAFETY_TABLE );
        IDE_TEST_RAISE( (sLocalMetaInfo.mKSafety != 0) && ( sSubSplitMethod->length == 1 ) , ERR_KSAFETY_COMPOSITE );

        //---------------------------------
        // Begin Statement for meta
        //---------------------------------

        sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
        sOldStmt                = QC_SMI_STMT(sStatement);
        QC_SMI_STMT(sStatement) = &sSmiStmt;
        sState = 1;

        IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                  sOldStmt,
                                  sSmiStmtFlag )
                  != IDE_SUCCESS );
        sState = 2;

        //---------------------------------
        // Insert Meta
        //---------------------------------

        IDE_TEST( sdm::insertTable( sStatement,
                                    (SChar*)sUserNameStr,
                                    (SChar*)sTableNameStr,
                                    (SChar*)sSplitMethodStr,
                                    (SChar*)sKeyColumnNameStr,
                                    (SChar*)sSubSplitMethodStr,
                                    (SChar*)sSubKeyColumnNameStr,
                                    (SChar*)sDefaultNodeNameStr,
                                    (SChar*)sDefaultPartitionNameStr,
                                    &sRowCnt )
                  != IDE_SUCCESS );

        //---------------------------------
        // End Statement
        //---------------------------------

        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;

        IDE_TEST_RAISE( sRowCnt == 0,
                        ERR_INVALID_SHARD_NODE );

        if ( SDU_SHARD_LOCAL_FORCE != 1 )
        {
            
            //---------------------------------
            // Create _BAK_ Table
            //---------------------------------
            idlOS::snprintf( sBackupTableNameStr,
                             QC_MAX_OBJECT_NAME_LEN+1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableNameStr );

            sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
            sOldStmt                = QC_SMI_STMT(sStatement);
            QC_SMI_STMT(sStatement) = &sSmiStmt;
            sState = 1;

            IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                      sOldStmt,
                                      sSmiStmtFlag )
                      != IDE_SUCCESS );
            sState = 2;

            // bak table drop
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "drop table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" cascade ",
                             sUserNameStr,
                             sBackupTableNameStr );

            ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s", sSqlStr);

            if ( sdf::executeRemoteSQLWithNewTrans( sStatement,
                                                    sLocalMetaInfo.mNodeName,
                                                    sSqlStr,
                                                    ID_TRUE )
                      != IDE_SUCCESS )
            {
                sErrCode = ideGetErrorCode();

                if ( sErrCode == qpERR_ABORT_QCV_NOT_EXISTS_TABLE )
                {
                    IDE_CLEAR();
                }
                else
                {
                    IDE_RAISE( ERR_ALREDAY_EXIST_ERROR_MSG );
                }
            }
            
            // bak table create
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "create table "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" "
                             "from table schema "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" "
                             "using prefix "QCM_SQL_STRING_SKIP_FMT" shard backup",
                             sUserNameStr,
                             sBackupTableNameStr,
                             sUserNameStr,
                             sTableNameStr,
                             SDI_BACKUP_TABLE_PREFIX );
            
            ideLog::log(IDE_SD_17,"[SHARD INTERNAL SQL]: %s", sSqlStr);

            IDE_TEST( sdf::executeRemoteSQLWithNewTrans( sStatement,
                                                         sLocalMetaInfo.mNodeName,
                                                         sSqlStr,
                                                         ID_TRUE )
                      != IDE_SUCCESS );

            sState = 1;
            IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

            sState = 0;
            QC_SMI_STMT(sStatement) = sOldStmt;
        }

        if ( ( SDU_SHARD_LOCAL_FORCE != 1 ) &&
             ( sSdSplitMethod != SDI_SPLIT_NONE ) )
        {
            /* TASK-7307 DML Data Consistency in Shard
             *   ALTER TABLE table SHARD SPLIT/CLONE/SOLO */
            IDE_TEST( sdm::alterShardFlag( sStatement,
                                           sUserNameStr,
                                           sTableNameStr,
                                           sSdSplitMethod,
                                           ID_TRUE ) /* isNewTrans */
                      != IDE_SUCCESS );

            sState = 0;
            QC_SMI_STMT(sStatement) = sOldStmt;

            /* 지정된 default Node와 내 NodeName이 일치하면
             * default partition을 USABLE로 설정한다 */
            if ( ( sSdSplitMethod != SDI_SPLIT_CLONE &&
                   sSdSplitMethod != SDI_SPLIT_SOLO ) &&
                 ( sTablePartitionType == QCM_PARTITIONED_TABLE ) &&
                 ( idlOS::strncmp( sLocalMetaInfo.mNodeName, 
                                   sDefaultNodeNameStr, 
                                   SDI_NODE_NAME_MAX_SIZE + 1 ) == 0 ) )
            {
                IDE_TEST( sdm::alterUsable( sStatement,
                                            sUserNameStr,
                                            sTableNameStr,
                                            sDefaultPartitionNameStr,
                                            ID_TRUE, /* isUsable */
                                            ID_TRUE  /* isNewTrans */ )
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
    
    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_KSAFETY_TABLE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_TABLE_TYPE , sTableNameStr ) );
    }
    IDE_EXCEPTION( ERR_KSAFETY_COMPOSITE  )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_COMPOSITE_TABLE ) );
    }
    IDE_EXCEPTION( ERR_DO_NOT_MATCH_SPLIT_METHOD )
    {
        IDE_SET( ideSetErrorCode(sdERR_ABORT_SDM_DO_NOT_MATCH_SPLIT_METHOD) );
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
    IDE_EXCEPTION( ERR_SHARD_PARTITION_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_PARTITION_NAME_TOO_LONG ) );
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
    IDE_EXCEPTION( ERR_INTERNAL_OPERATION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdfCalculate_SetShardTable",
                                  "SHARD_INTERNAL_LOCAL_OPERATION is not 1" ) );
    }
    IDE_EXCEPTION( ERR_ALREDAY_EXIST_ERROR_MSG )
    {
        // nothing to do
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

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( sIsOldSessionShardMetaTouched == ID_TRUE )
    {
        sdi::setShardMetaTouched( sStatement->session );
    }
    else
    {
        sdi::unsetShardMetaTouched( sStatement->session );
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

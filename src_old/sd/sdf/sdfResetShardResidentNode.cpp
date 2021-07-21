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
 *    SHARD_RESET_SHARD_RESIDENT_NODE( user_name     VARCHAR,
 *                                     table_name    VARCHAR,
 *                                     old_node_name VARCHAR,
 *                                     new_node_name VARCHAR,
 *                                     value         VARCHAR,
 *                                     sub_value     VARCHAR,
 *                                     is_default    INT )
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
    { NULL, 31, (void*)"SHARD_RESET_SHARD_RESIDENT_NODE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfResetShardResidentNodeModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_ResetShardResidentNode( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_ResetShardResidentNode,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC sdfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    const mtdModule* sModules[7] =
    {
        &mtdVarchar, // user_name
        &mtdVarchar, // table_name
        &mtdVarchar, // old_node_name
        &mtdVarchar, // new_node_name
        &mtdVarchar, // value
        &mtdVarchar, // sub_value
        &mtdInteger, // is_default
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

IDE_RC sdfCalculate_ResetShardResidentNode( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Reset Shard Resident node
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
    mtdCharType             * sOldNodeName;
    SChar                     sOldNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdCharType             * sNewNodeName;
    SChar                     sNewNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdCharType             * sValue;
    SChar                     sValueStr[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    mtdCharType             * sSubValue;
    SChar                     sSubValueStr[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;
    SChar                     sPartitionName[ QC_MAX_OBJECT_NAME_LEN + 1];
    qcmTablePartitionType     sTablePartitionType = QCM_PARTITIONED_TABLE;
    SChar                     sObjectValue[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    
    mtdIntegerType            sIsDefault;

    sdiShardObjectType         sShardTableType = SDI_NON_SHARD_OBJECT;

    sdiTableInfo              sTableInfo;
    idBool                    sIsTableFound = ID_FALSE;
    sdiGlobalMetaInfo         sMetaNodeInfo = { ID_ULONG(0) };
    sdiLocalMetaInfo          sLocalMetaInfo;
    idBool                    sIsUsable     = ID_FALSE;

    sdiInternalOperation      sInternalOP = SDI_INTERNAL_OP_NOT;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;
    sOldStmt     = QC_SMI_STMT(sStatement);
    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

    /* BUG-47623 샤드 메타 변경에 대한 trc 로그중 commit 로그를 작성하기전에 DASSERT 로 죽는 경우가 있습니다. */
    if ( ( sStatement->session->mQPSpecific.mFlag & QC_SESSION_SHARD_META_TOUCH_MASK ) ==
         QC_SESSION_SHARD_META_TOUCH_TRUE )
    {
        sIsOldSessionShardMetaTouched = ID_TRUE;
    }

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
                                             aStack[3].value ) == ID_TRUE ) )  // old node name
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

        // old node name
        sOldNodeName = (mtdCharType*)aStack[3].value;

        IDE_TEST_RAISE( sOldNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                        ERR_SHARD_GROUP_NAME_TOO_LONG );
        idlOS::strncpy( sOldNodeNameStr,
                        (SChar*)sOldNodeName->value,
                        sOldNodeName->length );
        sOldNodeNameStr[sOldNodeName->length] = '\0';

        // new node name
        sNewNodeName = (mtdCharType*)aStack[4].value;

        IDE_TEST_RAISE( sNewNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                        ERR_SHARD_GROUP_NAME_TOO_LONG );
        idlOS::strncpy( sNewNodeNameStr,
                        (SChar*)sNewNodeName->value,
                        sNewNodeName->length );
        sNewNodeNameStr[sNewNodeName->length] = '\0';

        // value
        sValue = (mtdCharType*)aStack[5].value;

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
        sSubValue = (mtdCharType*)aStack[6].value;

        IDE_TEST_RAISE( sSubValue->length > SDI_RANGE_VARCHAR_MAX_PRECISION,
                        ERR_SHARD_MAX_VALUE_TOO_LONG );
        idlOS::strncpy( sSubValueStr,
                        (SChar*)sSubValue->value,
                        sSubValue->length );
        sSubValueStr[sSubValue->length] = '\0';

        // is_default
        sIsDefault = *(mtdIntegerType*)aStack[7].value;
        IDE_TEST_RAISE( ( sIsDefault > 1 ) ||
                        ( sIsDefault < 0 ),
                        ERR_ARGUMENT_NOT_APPLICABLE );

        // Internal Option
        sInternalOP = (sdiInternalOperation)QCG_GET_SESSION_SHARD_INTERNAL_LOCAL_OPERATION( sStatement );

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

        /* get TableInfo */
        IDE_TEST( sdm::getGlobalMetaInfoCore( QC_SMI_STMT(sStatement),
                                              &sMetaNodeInfo ) != IDE_SUCCESS );
       
        IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( sStatement ),
                                     sUserNameStr,
                                     sTableNameStr,
                                     sMetaNodeInfo.mShardMetaNumber,
                                     &sTableInfo,
                                     &sIsTableFound )
                  != IDE_SUCCESS );

        /* check Table Type */
        sShardTableType = sdi::getShardObjectType( &sTableInfo );
        
        /* Table의 Shard Type에 맞게 Value를 가져온다. */
        if ( sTableInfo.mObjectType == 'T' )
        {
            switch( sShardTableType )
            {
                case SDI_SINGLE_SHARD_KEY_DIST_OBJECT:
                    IDE_TEST( sdm::getPartitionNameByValue( QC_SMI_STMT( sStatement ),
                                                            sUserNameStr,
                                                            sTableNameStr,
                                                            sObjectValue,
                                                            &sTablePartitionType,
                                                            sPartitionName ) != IDE_SUCCESS );
                    
                    if ( idlOS::strncmp( sPartitionName, SDM_NA_STR, QC_MAX_OBJECT_NAME_LEN + 1 ) == 0 )
                    {
                        // partitioned table의 경우 range value와 partition의 경계값이 맞는 것이 없으면 에러를 반환한다.
                        IDE_TEST_RAISE( sTablePartitionType != QCM_NONE_PARTITIONED_TABLE, ERR_ARGUMENT_NOT_APPLICABLE );
                    }
                    break;
                case SDI_SOLO_DIST_OBJECT:
                case SDI_CLONE_DIST_OBJECT:
                    /* Solo/Clone은 Partition이 없다. */
                    sPartitionName[0] = '\0';
                    break;
                case SDI_COMPOSITE_SHARD_KEY_DIST_OBJECT:
                case SDI_NON_SHARD_OBJECT:
                default:
                    IDE_RAISE( ERR_SHARD_TYPE );
                    break;
            }
        }
        else
        {
            idlOS::strncpy( sPartitionName,
                            SDM_NA_STR,
                            QC_MAX_OBJECT_NAME_LEN + 1 );
        }

        //---------------------------------
        // Update Meta
        //---------------------------------
        if ( sIsDefault != 0 )
        {
            IDE_TEST(sdm::updateDefaultNodeAndPartititon( sStatement,
                                                          sUserNameStr,
                                                          sTableNameStr,
                                                          sNewNodeNameStr,
                                                          sPartitionName,
                                                          &sRowCnt,
                                                          sInternalOP ) != IDE_SUCCESS);
            IDE_TEST_RAISE( sRowCnt == 0,
                            ERR_INVALID_SHARD_INFO_D );
        }
        else
        {
            IDE_TEST( sdm::updateRange( sStatement,
                                        (SChar*)sUserNameStr,
                                        (SChar*)sTableNameStr,
                                        (SChar*)sOldNodeNameStr,
                                        (SChar*)sNewNodeNameStr,
                                        (SChar*)sPartitionName,
                                        (SChar*)sObjectValue,
                                        (SChar*)sSubValueStr,
                                        &sRowCnt,
                                        sInternalOP )
                      != IDE_SUCCESS );
            IDE_TEST_RAISE( sRowCnt == 0,
                            ERR_INVALID_SHARD_INFO_R );
        }

        //---------------------------------
        // End Statement
        //---------------------------------

        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;

        /* TASK-7307 DML Data Consistency in Shard */
        if ( ( SDU_SHARD_LOCAL_FORCE != 1 ) &&
             ( sTableInfo.mObjectType == 'T' ) )
        {
            IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

            /* 지정된 OldNode와 내 NodeName이 일치하면 unusable로 변경
             * 지정된 NewNode와 내 NodeName이 일치하면 usable로 변경 */
            if ( idlOS::strncmp(sLocalMetaInfo.mNodeName, sOldNodeNameStr, SDI_NODE_NAME_MAX_SIZE + 1) == 0 )
            {
                sIsUsable = ID_FALSE;
            }
            else if ( idlOS::strncmp(sLocalMetaInfo.mNodeName, sNewNodeNameStr, SDI_NODE_NAME_MAX_SIZE + 1) == 0 )
            {
                sIsUsable = ID_TRUE;
            }
            else
            {
                IDE_CONT( normal_exit );
            }

            IDE_TEST( sdm::alterUsable( sStatement,
                                        sUserNameStr,
                                        sTableNameStr,
                                        sPartitionName,
                                        sIsUsable,
                                        ID_FALSE ) // IsNewTrans
                      != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do */
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_SHARD_TYPE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdfCalculate_ResetShardResidentNode",
                                  "Invalid Shard Type" ) );
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
    IDE_EXCEPTION( ERR_INVALID_SHARD_INFO_D );
    {

        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_META_CHANGE_ARG,
                                  sUserNameStr,
                                  sTableNameStr,
                                  sNewNodeNameStr,
                                  sPartitionName,
                                  "",
                                  "",
                                  "" ) );
    }
    IDE_EXCEPTION( ERR_INVALID_SHARD_INFO_R );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_META_CHANGE_ARG,
                                  sUserNameStr,
                                  sTableNameStr,
                                  sOldNodeNameStr,
                                  sNewNodeNameStr,
                                  sPartitionName,
                                  sValueStr,
                                  "" ) );
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
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

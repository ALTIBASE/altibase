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
 *    SHARD_SET_SHARD_PARTITION_NODE( user_name      VARCHAR,
 *                                    table_name     VARCHAR,
 *                                    partition_name VARCHAR,
 *                                    node_name      VARCHAR )
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
    { NULL, 37, (void*)"SHARD_SET_SHARD_PARTITION_NODE_GLOBAL" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetShardPartitionNodeGlobalModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_SetShardPartitionNodeGlobal( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetShardPartitionNodeGlobal,
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
    const mtdModule* sModules[4] =
    {
        &mtdVarchar, // user_name
        &mtdVarchar, // table_name
        &mtdVarchar, // partition_name
        &mtdVarchar, // node_name
    };
    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 4,
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

IDE_RC sdfCalculate_SetShardPartitionNodeGlobal( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Set Shard Partition Node
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
    mtdCharType             * sPartitionName;
    SChar                     sPartitionNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                     sValueStr[SDI_RANGE_VARCHAR_MAX_PRECISION + 1];
    idBool                    sIsDefault = ID_FALSE;

    sdiClientInfo           * sClientInfo  = NULL;
    UShort                    i = 0;
    UInt                      sExecCount = 0;

    ULong                     sSMN = ID_ULONG(0);

    SChar                   * sDefaultNodeNameStr = NULL;
    SChar                   * sDefaultPartitionNamePtr = NULL;
    SChar                     sBakTableName[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                   * sReplNamePtr    = NULL;
    SChar                   * sBakNodeNamePtr = NULL;
    sdiLocalMetaInfo          sLocalMetaInfo;
    sdiTableInfo              sTableInfo;
    sdiNodeInfo               sNodeInfo;
    idBool                    sIsTableFound = ID_FALSE;

    sdiReplicaSetInfo         sReplicaSetInfo;
    sdiReplicaSetInfo         sFoundReplicaSetInfo;

    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
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

    IDE_TEST_RAISE((aStack[1].column->module->isNull (aStack[1].column,
                                                      aStack[1].value) == ID_TRUE) || // user name
                   (aStack[2].column->module->isNull (aStack[2].column,
                                                      aStack[2].value) == ID_TRUE) || // table name
                   (aStack[3].column->module->isNull (aStack[3].column,
                                                      aStack[3].value) == ID_TRUE) || // partition name
                   (aStack[5].column->module->isNull (aStack[4].column,
                                                      aStack[4].value) == ID_TRUE), // node name
                   ERR_ARGUMENT_NOT_APPLICABLE);

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

    // partition name
    sPartitionName = (mtdCharType*)aStack[3].value;

    IDE_TEST_RAISE( sPartitionName->length > QC_MAX_OBJECT_NAME_LEN,
                    ERR_SHARD_MAX_PARTITION_TOO_LONG );
    idlOS::strncpy( sPartitionNameStr,
                    (SChar*)sPartitionName->value,
                    sPartitionName->length );
    sPartitionNameStr[sPartitionName->length] = '\0';

    // shard node name
    sNodeName = (mtdCharType*)aStack[4].value;

    IDE_TEST_RAISE( sNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                    ERR_SHARD_GROUP_NAME_TOO_LONG );
    idlOS::strncpy( sNodeNameStr,
                    (SChar*)sNodeName->value,
                    sNodeName->length );
    sNodeNameStr[sNodeName->length] = '\0';

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

    IDE_TEST( sdi::compareDataAndSessionSMN( sStatement ) != IDE_SUCCESS );

    sdi::setShardMetaTouched( sStatement->session );

    IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(sStatement)->getTrans()->getTransID() ) != IDE_SUCCESS );
    IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );
    
    // Zookeeper에서 Node 정보를 받아 온다.
    // IDE_TEST( sdi::zookeeper_get_node_list() != IDE_SUCCESS );
    IDE_TEST( sdi::checkShardLinker( sStatement ) != IDE_SUCCESS );

    sClientInfo = sStatement->session->mQPSpecific.mClientInfo;

    // 전체 Node 대상으로 Meta Update 실행
    if ( sClientInfo != NULL )
    {
        IDE_TEST( sdm::getPartitionValueByName( QC_SMI_STMT( sStatement ),
                                                sUserNameStr,
                                                sTableNameStr,
                                                sPartitionNameStr,
                                                SDI_RANGE_VARCHAR_MAX_PRECISION + 1,
                                                sValueStr,
                                                &sIsDefault ) != IDE_SUCCESS);

        /* DefaultPartition 변경의 경우 먼저 Default Partition의 RP를 제거하고 진행하여야 한다. */
        if ( sIsDefault == ID_TRUE )
        {
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

            IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( sStatement ),
                                         sUserNameStr,
                                         sTableNameStr,
                                         sSMN,
                                         &sTableInfo,
                                         &sIsTableFound )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsTableFound == ID_FALSE,
                            ERR_NOT_EXIST_TABLE );

            IDE_TEST( sdm::getAllReplicaSetInfoSortedByPName( QC_SMI_STMT( sStatement ),
                                                              &sReplicaSetInfo,
                                                              sSMN ) 
                      != IDE_SUCCESS );

            sState = 1;
            IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

            sState = 0;
            QC_SMI_STMT(sStatement) = sOldStmt;

            idlOS::snprintf( sBakTableName,
                             QC_MAX_OBJECT_NAME_LEN+1,
                             "%s%s",
                             SDI_BACKUP_TABLE_PREFIX,
                             sTableNameStr );

            if ( sTableInfo.mDefaultNodeId != SDI_NODE_NULL_ID )
            {
                // Default Replication Drop
                if ( idlOS::strncmp( sTableInfo.mDefaultPartitionName, 
                                     SDM_NA_STR, 
                                     QC_MAX_OBJECT_NAME_LEN + 1 ) != 0 )
                {
                    sDefaultPartitionNamePtr = sTableInfo.mDefaultPartitionName;
                    sNodeInfo.mCount = 0;

                    IDE_TEST( sdi::getInternalNodeInfo( QC_SMI_STMT(sStatement)->getTrans(),
                                                        &sNodeInfo,
                                                        ID_TRUE,
                                                        sSMN )
                              != IDE_SUCCESS );

                    for ( i = 0; i < sNodeInfo.mCount; i++ )
                    {
                        if ( sNodeInfo.mNodes[i].mNodeId == sTableInfo.mDefaultNodeId )
                        {
                            sDefaultNodeNameStr = sNodeInfo.mNodes[i].mNodeName;
                            break;
                        }
                    }

                    // Default RP를 먼저 제거
                    IDE_TEST( sdm::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );

                    for ( i = 0; i < sLocalMetaInfo.mKSafety; i++ )
                    {
                        /* 내가 보내는 ReplName을 찾아야 한다. */
                        IDE_TEST( sdm::findSendReplInfoFromReplicaSet( &sReplicaSetInfo,
                                                                       sDefaultNodeNameStr,
                                                                       &sFoundReplicaSetInfo )
                                  != IDE_SUCCESS );

                        /* Meta Update 구문 수행 중에는 항상 모든 Node가 Valid 하기 때문에
                         * RP Set은 하나만 나와야 한다. */
                        IDE_TEST_RAISE( sFoundReplicaSetInfo.mCount != 1 , ERR_NODE_STATE );

                        if ( i == 0 ) /* First */
                        {
                            sReplNamePtr    = sFoundReplicaSetInfo.mReplicaSets[0].mFirstReplName;
                            sBakNodeNamePtr = sFoundReplicaSetInfo.mReplicaSets[0].mFirstBackupNodeName;
                        }
                        else if ( i == 1 ) /* Second */
                        {
                            sReplNamePtr    = sFoundReplicaSetInfo.mReplicaSets[0].mSecondReplName;
                            sBakNodeNamePtr = sFoundReplicaSetInfo.mReplicaSets[0].mSecondBackupNodeName;
                        }

                        if ( idlOS::strncmp( sBakNodeNamePtr, SDM_NA_STR, SDI_NODE_NAME_MAX_SIZE + 1 ) != 0 )
                        {
                            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                             "alter replication %s drop table from %s.%s partition %s "
                                             "to %s.%s partition %s",
                                             sReplNamePtr,     /* REPLICATION_NAME  */
                                             sUserNameStr,     /* Src UserName      */
                                             sTableNameStr,    /* Src TableName     */
                                             sDefaultPartitionNamePtr, /* Src PartitionName */
                                             sUserNameStr,     /* Dst UserName      */
                                             sBakTableName, /* Dst TableName     */
                                             sDefaultPartitionNamePtr /* Dst PartitionName */ );

                            IDE_TEST( sdi::shardExecDirect( sStatement,
                                                            sDefaultNodeNameStr,
                                                            (SChar*)sSqlStr,
                                                            (UInt) idlOS::strlen( sSqlStr ),
                                                            SDI_INTERNAL_OP_NORMAL,
                                                            &sExecCount,
                                                            NULL,
                                                            NULL,
                                                            0,
                                                            NULL )
                                      != IDE_SUCCESS );

                            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                             "alter replication %s drop table from %s.%s partition %s "
                                             "to %s.%s partition %s",
                                             sReplNamePtr,     /* REPLICATION_NAME  */
                                             sUserNameStr,     /* Src UserName      */
                                             sBakTableName,    /* Src TableName     */
                                             sDefaultPartitionNamePtr, /* Src PartitionName */
                                             sUserNameStr,     /* Dst UserName      */
                                             sTableNameStr, /* Dst TableName     */
                                             sDefaultPartitionNamePtr /* Dst PartitionName */ );

                            IDE_TEST( sdi::shardExecDirect( sStatement,
                                                            sBakNodeNamePtr,
                                                            (SChar*)sSqlStr,
                                                            (UInt) idlOS::strlen( sSqlStr ),
                                                            SDI_INTERNAL_OP_NORMAL,
                                                            &sExecCount,
                                                            NULL,
                                                            NULL,
                                                            0,
                                                            NULL )
                                      != IDE_SUCCESS );
                        }
                    }
                }
            }
        }

        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "exec dbms_shard.set_shard_partition_node_local( "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT", "
                         QCM_SQL_VARCHAR_FMT" )",
                         sUserNameStr,
                         sTableNameStr,
                         sPartitionNameStr,
                         sNodeNameStr );

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
    IDE_EXCEPTION( ERR_NODE_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdfCalculate_SetShardPartitionNodeGlobal",
                                  "ReplicaSet Count is not 1" ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
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
    IDE_EXCEPTION( ERR_INVALID_SHARD_NODE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_SHARD_NODE ) );
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
    IDE_EXCEPTION( ERR_SHARD_MAX_PARTITION_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_PARTITION_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
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

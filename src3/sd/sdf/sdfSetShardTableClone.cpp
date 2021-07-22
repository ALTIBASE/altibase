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
 *    SHARD_SET_SHARD_TABLE_CLONE( user_name                  VARCHAR,
 *                                 table_name                 VARCHAR,
 *                                 reference_node_name        VARCHAR,
 *                                 replication_parallel_count INTEGER)  
 *    RETURN 0
 *
 **********************************************************************/
#include <sdm.h>
#include <sdf.h>
#include <smi.h>
#include <qcg.h>
#include <sdiZookeeper.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName sdfFunctionName[1] = {
    { NULL, 27, (void*)"SHARD_SET_SHARD_TABLE_CLONE" }
};

static IDE_RC executeSetShardClone( qcStatement * aStatement,
                                    SChar       * aUserNameStr,
                                    SChar       * aTableNameStr );

static IDE_RC executeTruncateTableToNode( qcStatement * aStatement,
                                          SChar       * aUserNameStr,
                                          SChar       * aTableNameStr,
                                          SChar       * sRefNodeNameStr );

static IDE_RC executeSyncClone( qcStatement * aStatement,
                                iduList     * aNodeInfoList,
                                SChar       * aUserNameStr,
                                SChar       * aTableNameStr,
                                SChar       * aRefNodeNameStr,
                                UInt          aReplParallelCnt );

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetShardTableCloneModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_SetShardTableClone( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetShardTableClone,
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
        &mtdVarchar, // reference_node_name
        &mtdInteger  // replication_parallel_count     
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

IDE_RC sdfCalculate_SetShardTableClone( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    qcStatement             * sStatement = NULL;
    mtdCharType             * sUserName = NULL;
    mtdCharType             * sTableName = NULL;
    SChar                     sUserNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    SChar                     sTableNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    sdiSplitMethod            sSdSplitMethod = SDI_SPLIT_NONE;
    SChar                     sSplitMethodStr[1 + 1];
    ULong                     sSMN = ID_ULONG(0);
    idBool                    sIsAlive = ID_FALSE;
    mtdCharType             * sRefNodeName = NULL;
    SChar                     sRefNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdIntegerType            sReplParallelCnt;
    iduList                 * sNodeInfoList = NULL;
    UInt                      sNodeCount = 0;
    UInt                      sPartitionCnt = 0;
    UInt                      i = 0;
    UInt                      sExecCount = 0;
    ULong                     sNumResult = 0;
    idBool                    sFetchResult = ID_FALSE;
    idBool                    sExistIndex = ID_FALSE;
    sdiClientInfo           * sClientInfo  = NULL;
    sdiConnectInfo          * sConnectInfo = NULL;
    sdiLocalMetaInfo          sLocalMetaInfo;
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

        // reference node name
        if ( aStack[3].column->module->isNull( aStack[3].column,
                                               aStack[3].value ) == ID_TRUE )
        {
            // current set reference node name
            IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
            
            idlOS::strncpy( sRefNodeNameStr,
                            (SChar*)sLocalMetaInfo.mNodeName,
                            SDI_NODE_NAME_MAX_SIZE);
            sRefNodeNameStr[SDI_NODE_NAME_MAX_SIZE] = '\0';
        }
        else
        {        
            sRefNodeName = (mtdCharType*)aStack[3].value;

            IDE_TEST_RAISE( sRefNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                            ERR_SHARD_GROUP_NAME_TOO_LONG );
            idlOS::strncpy( sRefNodeNameStr,
                            (SChar*)sRefNodeName->value,
                            sRefNodeName->length );
            sRefNodeNameStr[sRefNodeName->length] = '\0';
        }
        
        // replication_parallel_count
        sReplParallelCnt = *(mtdIntegerType*)aStack[4].value;
         
        // split method
        /* NULL 이면 partition 정보를 가져와서 자동으로 설정한다. */
        sSdSplitMethod = SDI_SPLIT_CLONE;
        
        IDE_TEST( sdi::getSplitMethodCharByType(sSdSplitMethod, &(sSplitMethodStr[0]))
                  != IDE_SUCCESS );
        sSplitMethodStr[1] = '\0';

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
        // check table name
        //---------------------------------
        IDE_TEST_RAISE(( sTableName->length +
                         idlOS::strlen(SDI_BACKUP_TABLE_PREFIX) ) > QC_MAX_OBJECT_NAME_LEN,
                       ERR_SHARD_TABLE_NAME_TOO_LONG );

        //---------------------------------
        // Insert Meta to All Node
        //---------------------------------
        IDE_TEST( sdi::compareDataAndSessionSMN( sStatement )
                  != IDE_SUCCESS );

        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(sStatement)->getTrans()->getTransID() )
                  != IDE_SUCCESS );

        sdi::setShardMetaTouched( sStatement->session );
        
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );

        IDE_TEST( sdiZookeeper::getAllNodeInfoList( &sNodeInfoList,
                                                    &sNodeCount )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdi::getIncreasedSMNForMetaNode( ( QC_SMI_STMT( sStatement ) )->getTrans() , 
                                                   &sSMN ) 
                  != IDE_SUCCESS );

        IDE_TEST( sdi::checkShardLinker( sStatement ) != IDE_SUCCESS );
                
        //---------------------------------
        // check partition cnt, pk, view, node
        //---------------------------------        
        IDE_TEST( sdm::validateShardObjectTable( QC_SMI_STMT(sStatement),
                                                 sUserNameStr,
                                                 sTableNameStr,
                                                 NULL, // partition and node name list
                                                 sSdSplitMethod,
                                                 sPartitionCnt,
                                                 &sExistIndex )
                  != IDE_SUCCESS );

        IDE_TEST( sdf::validateNode( sStatement,
                                     NULL, // partition and node name list
                                     sRefNodeNameStr,
                                     sSdSplitMethod )
                  != IDE_SUCCESS );

        //---------------------------------
        // check schema
        //---------------------------------
        // table
        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "SELECT DIGEST( DBMS_METADATA.GET_DDL('TABLE', "
                         "'%s', '%s'), 'SHA-256') A FROM DUAL ",
                         sTableNameStr,
                         sUserNameStr );
        
        IDE_TEST( sdf::checkShardObjectSchema( sStatement,
                                               sSqlStr )
                  != IDE_SUCCESS );
        
        // index
        if ( sExistIndex == ID_TRUE )
        {
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "SELECT DIGEST( DBMS_METADATA.GET_DEPENDENT_DDL('INDEX', "
                             "'%s', '%s'), 'SHA-256') A FROM DUAL ",
                             sTableNameStr,
                             sUserNameStr );

            IDE_TEST( sdf::checkShardObjectSchema( sStatement,
                                                   sSqlStr )
                      != IDE_SUCCESS );
        }

        //---------------------------------
        // check reference node option 
        //---------------------------------
        if ( aStack[3].column->module->isNull( aStack[3].column,
                                               aStack[3].value ) == ID_TRUE )
        {
            sClientInfo = sStatement->session->mQPSpecific.mClientInfo;
            
            if ( sClientInfo != NULL )
            {
                sConnectInfo = sClientInfo->mConnectInfo;
            
                for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
                {
                    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                     "SELECT COUNT(*) FROM "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" ",
                                     sUserNameStr,
                                     sTableNameStr );
                    
                    IDE_TEST( sdi::shardExecDirect( sStatement,
                                                    sConnectInfo->mNodeName,
                                                    (SChar*)sSqlStr,
                                                    (UInt) idlOS::strlen(sSqlStr),
                                                    SDI_INTERNAL_OP_NORMAL,
                                                    &sExecCount,
                                                    &sNumResult,
                                                    NULL,
                                                    ID_SIZEOF(sNumResult),
                                                    &sFetchResult )
                              != IDE_SUCCESS );

                    IDE_TEST_RAISE( sFetchResult == ID_FALSE, ERR_NO_DATA_FOUND );

                    IDE_TEST_RAISE( sNumResult > 0, ERR_RECORD_EXIST );
                }
            }
            else
            {
                IDE_RAISE( ERR_NODE_NOT_EXIST );
            }   
        }
        
        //---------------------------------
        // SET_SHARD_TABLE
        //---------------------------------
        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "exec dbms_shard.set_shard_table_local( '"
                         QCM_SQL_STRING_SKIP_FMT"', '"
                         QCM_SQL_STRING_SKIP_FMT"', "
                         QCM_SQL_VARCHAR_FMT", "
                         "'','','','' )",
                         sUserNameStr,
                         sTableNameStr,
                         sSplitMethodStr );
        
        IDE_TEST( sdf::runRemoteQuery( sStatement,
                                       sSqlStr )
                  != IDE_SUCCESS );
        
        //---------------------------------
        // SET_SHARD_CLONE
        //---------------------------------
        IDE_TEST( executeSetShardClone( sStatement,
                                        sUserNameStr,
                                        sTableNameStr )
                  != IDE_SUCCESS );
        
        //---------------------------------
        // truncate table to node
        //---------------------------------
        IDE_TEST( executeTruncateTableToNode( sStatement,
                                              sUserNameStr,
                                              sTableNameStr,
                                              sRefNodeNameStr )
                  != IDE_SUCCESS );
        
        //---------------------------------
        //  clone resharding 
        //---------------------------------
        IDE_TEST( executeSyncClone( sStatement,
                                    sNodeInfoList,
                                    sUserNameStr,
                                    sTableNameStr,
                                    sRefNodeNameStr,
                                    sReplParallelCnt )
                  != IDE_SUCCESS );
    }
    
    // revert job all remove
    if ( sdiZookeeper::isExistRevertJob() == ID_TRUE )
    {
        (void) sdiZookeeper::removeRevertJob();
        IDE_DASSERT( sdiZookeeper::isExistRevertJob() != ID_TRUE );
    }
    
    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "COMMIT " );

    IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                          sSqlStr,
                                          sStatement->session->mMmSession )
              != IDE_SUCCESS );
    
    *(mtdIntegerType*)aStack[0].value = 0;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CLUSTER_STATE )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_ZKC_DEADNODE_EXIST ) );
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
                                 "TANSACTIONAL_DDL = 1" ));
    }
    IDE_EXCEPTION( ERR_SHARD_GROUP_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_RECORD_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_RECORD_EXIST ));
    }
    IDE_EXCEPTION( ERR_NO_DATA_FOUND )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR,
                                  "sdfCalculate_SetShardTableClone",
                                  "get row count fetch no data found" ) );
    }
    IDE_EXCEPTION( ERR_NODE_NOT_EXIST )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDM_SHARD_NODE_NOT_EXIST ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sdiZookeeper::isExistRevertJob() == ID_TRUE )
    {
        (void) sdiZookeeper::executeRevertJob( ZK_REVERT_JOB_REPL_ITEM_DROP );
        (void) sdiZookeeper::executeRevertJob( ZK_REVERT_JOB_REPL_DROP );
        (void) sdiZookeeper::executeRevertJob( ZK_REVERT_JOB_TABLE_ALTER );
        (void) sdiZookeeper::removeRevertJob();
        IDE_DASSERT( sdiZookeeper::isExistRevertJob() != ID_TRUE );
    }

    (void) executeTruncateTableToNode( sStatement,
                                       sUserNameStr,
                                       sTableNameStr,
                                       sRefNodeNameStr );
    
    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "ROLLBACK " );

    (void) qciMisc::runDCLforInternal( sStatement,
                                       sSqlStr,
                                       sStatement->session->mMmSession );
    IDE_POP();
    
    return IDE_FAILURE;
}

IDE_RC executeSetShardClone( qcStatement * aStatement,
                             SChar       * aUserNameStr,
                             SChar       * aTableNameStr )
{    
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    SChar            sSqlStr[SDF_QUERY_LEN];
    UInt             i = 0;

    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
    
    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;
            
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "exec dbms_shard.set_shard_clone_local( '"
                             QCM_SQL_STRING_SKIP_FMT"', '"
                             QCM_SQL_STRING_SKIP_FMT"', '"
                             QCM_SQL_STRING_SKIP_FMT"' )",
                             aUserNameStr,
                             aTableNameStr,
                             sConnectInfo->mNodeName );
        
            IDE_TEST( sdf::runRemoteQuery( aStatement,
                                           sSqlStr )
                      != IDE_SUCCESS );    
        }
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC executeTruncateTableToNode( qcStatement * aStatement,
                                   SChar       * aUserNameStr,
                                   SChar       * aTableNameStr,
                                   SChar       * sRefNodeNameStr )
{
    sdiClientInfo  * sClientInfo  = NULL;
    sdiConnectInfo * sConnectInfo = NULL;
    smiStatement   * sOldStmt = NULL;
    SChar            sSqlStr[SDF_QUERY_LEN];
    smiStatement     sSmiStmt;
    UInt             sSmiStmtFlag = 0;
    SInt             sState = 0;
    sdiLocalMetaInfo sLocalMetaInfo;
    UInt             i = 0;

    // cf> rollback 되지 않음.
    IDE_TEST( sdi::getLocalMetaInfo( &sLocalMetaInfo ) != IDE_SUCCESS );
   
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;
        
    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
            
    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;
            
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {   
            if ( idlOS::strMatch( sConnectInfo->mNodeName,
                                  idlOS::strlen( sConnectInfo->mNodeName ),
                                  sRefNodeNameStr,
                                  idlOS::strlen( sRefNodeNameStr ) ) != 0 )
            {
                idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                 "TRUNCATE TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT" ",
                                 aUserNameStr,
                                 aTableNameStr );       
                        
                IDE_TEST( sdf::executeRemoteSQLWithNewTrans( aStatement,
                                                             sConnectInfo->mNodeName,
                                                             sSqlStr,
                                                             ID_TRUE )
                          != IDE_SUCCESS );
            }
        }
    }
    
    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;
        
    return IDE_SUCCESS;
    
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
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }

    IDE_POP();
    
    return IDE_FAILURE;
}

IDE_RC executeSyncClone( qcStatement * aStatement,
                         iduList     * aNodeInfoList,
                         SChar       * aUserNameStr,
                         SChar       * aTableNameStr,
                         SChar       * aRefNodeNameStr,
                         UInt          aReplParallelCnt )
{
    SChar              sSqlStr[SDF_QUERY_LEN];
    SChar              sReplName[QC_MAX_OBJECT_NAME_LEN + 1];
    sdiClientInfo    * sClientInfo  = NULL;
    sdiConnectInfo   * sConnectInfo = NULL;
    sdiLocalMetaInfo * sTargetMetaInfo = NULL;
    smiStatement     * sOldStmt = NULL;
    smiStatement       sSmiStmt;
    UInt               sSmiStmtFlag = 0;
    UInt               sAccessMode = 0;
    idBool             sIsReferenceNode = ID_FALSE;
    UInt               sAccessState = 0;
    UInt               sState = 0;
    UInt               i = 0;
    
    //---------------------------------
    // ACCESS READ ONLY
    //---------------------------------
    IDE_TEST( sdf::getCurrentAccessMode( aStatement,
                                         aTableNameStr,
                                         &sAccessMode )
              != IDE_SUCCESS );

    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ ONLY ",
                     aUserNameStr,
                     aTableNameStr );

    IDE_TEST( sdf::remoteSQLWithNewTrans( aStatement,
                                          sSqlStr )
              != IDE_SUCCESS );
    sAccessState = 1;

    //---------------------------------
    // CLONE DATA SYNC
    //---------------------------------
    sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
    sOldStmt                = QC_SMI_STMT(aStatement);
    QC_SMI_STMT(aStatement) = &sSmiStmt;
    sState = 1;

    IDE_TEST( sSmiStmt.begin( aStatement->mStatistics,
                              sOldStmt,
                              sSmiStmtFlag )
              != IDE_SUCCESS );
    sState = 2;

    
    sClientInfo = aStatement->session->mQPSpecific.mClientInfo;
    
    if ( sClientInfo != NULL )
    {
        sConnectInfo = sClientInfo->mConnectInfo;

        // node n-1/2 개의 replciation을 생성 및 동기화
        for ( i = 0; i < sClientInfo->mCount; i++, sConnectInfo++ )
        {
            sIsReferenceNode = sdi::isSameNode( aRefNodeNameStr,
                                                sConnectInfo->mNodeName );

            if ( sIsReferenceNode != ID_TRUE )
            {
                // REPL_SET_CLONE_NODE1_NODE2
                idlOS::snprintf( sReplName, SDF_QUERY_LEN,
                                 "REPL_SET_CLONE_%s_%s",
                                 aRefNodeNameStr,
                                 sConnectInfo->mNodeName );
            
                //---------------------------------
                // CREATE REPLICATION
                //---------------------------------
                // sender
                sTargetMetaInfo =
                    sdiZookeeper::getNodeInfo( aNodeInfoList,
                                               sConnectInfo->mNodeName );
            
                IDE_TEST( sdf::createReplicationForInternal(
                              aStatement,
                              aRefNodeNameStr,
                              sReplName,
                              sTargetMetaInfo->mInternalRPHostIP,
                              sTargetMetaInfo->mInternalRPPortNo )
                          != IDE_SUCCESS );
        
                // receiver
                sTargetMetaInfo =
                    sdiZookeeper::getNodeInfo( aNodeInfoList,
                                               aRefNodeNameStr );

                IDE_TEST( sdf::createReplicationForInternal(
                              aStatement,
                              sConnectInfo->mNodeName,
                              sReplName,
                              sTargetMetaInfo->mInternalRPHostIP,
                              sTargetMetaInfo->mInternalRPPortNo )
                          != IDE_SUCCESS );
            
                //---------------------------------
                // ADD REPLICATION ITEM
                //---------------------------------
                // sender
                IDE_TEST( sdm::addReplTable( aStatement,
                                             aRefNodeNameStr,
                                             sReplName,
                                             aUserNameStr,
                                             aTableNameStr,
                                             SDM_NA_STR,
                                             SDM_REPL_CLONE,
                                             ID_TRUE )
                          != IDE_SUCCESS );

                // receiver
                IDE_TEST( sdm::addReplTable( aStatement,
                                             sConnectInfo->mNodeName,
                                             sReplName,
                                             aUserNameStr,
                                             aTableNameStr,
                                             SDM_NA_STR,
                                             SDM_REPL_CLONE,
                                             ID_TRUE )
                          != IDE_SUCCESS );
                //---------------------------------
                // SYNC REPLICATION 
                //---------------------------------
                //sender
                IDE_TEST( sdf::startReplicationForInternal(
                              aStatement,
                              aRefNodeNameStr,
                              sReplName,
                              aReplParallelCnt )
                          != IDE_SUCCESS );

            
                //---------------------------------
                // DROP REPLICATION
                //---------------------------------
                // sender
                IDE_TEST( sdf::dropReplicationForInternal(
                              aStatement,
                              aRefNodeNameStr,
                              sReplName )
                          != IDE_SUCCESS );

                // receiver
                IDE_TEST( sdf::dropReplicationForInternal(
                              aStatement,
                              sConnectInfo->mNodeName,
                              sReplName )
                          != IDE_SUCCESS );

            }
        }
    }
    
    sState = 1;
    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

    sState = 0;
    QC_SMI_STMT(aStatement) = sOldStmt;

    //---------------------------------
    // REVERT ACCESS MODE
    //---------------------------------
    switch (sAccessMode)
    {
        case 1: // Write
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ WRITE ",
                             aUserNameStr,
                             aTableNameStr );
            break;
        case 2: // Read
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ READ ",
                             aUserNameStr,
                             aTableNameStr );
            break;
        case 3: // Append
            idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                             "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ APPEND ",
                             aUserNameStr,
                             aTableNameStr );
            break;
        default:
            IDE_DASSERT(0);
            break;
    }
            
    IDE_TEST( sdf::remoteSQLWithNewTrans( aStatement,
                                          sSqlStr )
              != IDE_SUCCESS );
    sAccessState = 0;

    return IDE_SUCCESS;

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
            QC_SMI_STMT(aStatement) = sOldStmt;
        default:
            break;
    }
    
    if ( sAccessState == 1 )
    {
        switch (sAccessMode)
        {
            case 1: // Write
                idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                 "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ WRITE ",
                                 aUserNameStr,
                                 aTableNameStr );
                break;
            case 2: // Read
                idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                 "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ READ ",
                                 aUserNameStr,
                                 aTableNameStr );
                break;
            case 3: // Append
                idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                 "ALTER TABLE "QCM_SQL_STRING_SKIP_FMT"."QCM_SQL_STRING_SKIP_FMT"  ACCESS READ APPEND ",
                                 aUserNameStr,
                                 aTableNameStr );
                break;
            default:
                IDE_DASSERT(0);
                break;
        }
            
        (void)sdf::remoteSQLWithNewTrans( aStatement,
                                          sSqlStr );
    }
    
    IDE_POP();

    return IDE_FAILURE;
}

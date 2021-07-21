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
 *    SHARD_SET_SHARD_PROCEDURE_CLONE( user_name    VARCHAR,
 *                                     proc_name    VARCHAR,
 *                                     proc_replace VARCHAR);
 *                                        
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
    { NULL, 31, (void*)"SHARD_SET_SHARD_PROCEDURE_CLONE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetShardProcedureCloneModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_SetShardProcedureClone( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

IDE_RC executeSetShardClone( qcStatement    * aStatement,
                             SChar          * aUserNameStr,
                             SChar          * aProcNameStr );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetShardProcedureClone,
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
    const mtdModule* sModules[3] =
    {
        &mtdVarchar, // user_name
        &mtdVarchar, // proc_name
        &mtdVarchar  // proc_replace
    };
    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
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

IDE_RC sdfCalculate_SetShardProcedureClone( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    qcStatement             * sStatement;
    mtdCharType             * sUserName;
    SChar                     sUserNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    mtdCharType             * sProcName;
    SChar                     sProcNameStr[QC_MAX_OBJECT_NAME_LEN + 1];
    idBool                    sIsAlive = ID_FALSE;
    SChar                     sSqlStr[SDF_QUERY_LEN];
    mtdCharType             * sProcReplace = NULL;
    SChar                     sProcReplaceStr[2];
    sdiGlobalMetaInfo         sMetaNodeInfo = { ID_ULONG(0) };
    sdiTableInfo              sTableInfo;
    idBool                    sIsTableFound = ID_FALSE;

    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    
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
        sProcName = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sProcName->length > QC_MAX_OBJECT_NAME_LEN,
                        ERR_SHARD_TABLE_NAME_TOO_LONG );
        idlOS::strncpy( sProcNameStr,
                        (SChar*)sProcName->value,
                        sProcName->length );
        sProcNameStr[sProcName->length] = '\0';
        
        // proc replace
        sProcReplace = (mtdCharType*)aStack[3].value;
        idlOS::strncpy( sProcReplaceStr,
                        (SChar*)sProcReplace->value,
                        sProcReplace->length );
        sProcReplaceStr[sProcReplace->length] = '\0';        

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
        // zookeeper lock and etc
        //---------------------------------
        IDE_TEST( sdi::compareDataAndSessionSMN( sStatement ) != IDE_SUCCESS );
        
        IDE_TEST( sdiZookeeper::getShardMetaLock( QC_SMI_STMT(sStatement)->getTrans()->getTransID() )
                  != IDE_SUCCESS );

        sdi::setShardMetaTouched( sStatement->session );
                
        IDE_TEST( sdiZookeeper::checkAllNodeAlive( &sIsAlive ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsAlive != ID_TRUE, ERR_CLUSTER_STATE );

        IDE_TEST( sdi::checkShardLinker( sStatement ) != IDE_SUCCESS );
        
        //---------------------------------
        // check exist shard meta
        //---------------------------------
        if ( sProcReplaceStr[0] == 'Y' )
        {
            sSmiStmtFlag = SMI_STATEMENT_NORMAL | SMI_STATEMENT_MEMORY_CURSOR;
            sOldStmt                = QC_SMI_STMT(sStatement);
            QC_SMI_STMT(sStatement) = &sSmiStmt;
            sState = 1;

            IDE_TEST( sSmiStmt.begin( sStatement->mStatistics,
                                      sOldStmt,
                                      sSmiStmtFlag )
                      != IDE_SUCCESS );
            sState = 2;

            IDE_TEST( sdi::getGlobalMetaInfoCore( QC_SMI_STMT( sStatement ),
                                                  &sMetaNodeInfo )
                      != IDE_SUCCESS );
            
            IDE_TEST( sdm::getTableInfo( QC_SMI_STMT( sStatement ),
                                         sUserNameStr,
                                         sProcNameStr,
                                         sMetaNodeInfo.mShardMetaNumber,
                                         &sTableInfo,
                                         &sIsTableFound )
                      != IDE_SUCCESS );

            sState = 1;
            IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

            sState = 0;
            QC_SMI_STMT(sStatement) = sOldStmt;

            //---------------------------------
            // UNSET_SHARD_PROCEDURE_BY_ID
            //---------------------------------
            if ( sIsTableFound == ID_TRUE )
            {
                idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                                 "EXEC DBMS_SHARD.UNSET_SHARD_PROCEDURE_BY_ID( %"ID_UINT32_FMT" ) ",
                                 sTableInfo.mShardID );

                IDE_TEST( sdf::runRemoteQuery( sStatement,
                                               sSqlStr )
                          != IDE_SUCCESS );
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            IDE_TEST_RAISE( sProcReplaceStr[0] != 'N', ERR_PROC_REPLACE_OPTION );
        }
        
        //---------------------------------
        // check procedure, exist node
        //---------------------------------
        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "SELECT DIGEST( DBMS_METADATA.GET_DDL('PROCEDURE', "
                         "'%s', '%s'), 'SHA-256') A FROM DUAL ",
                         sProcNameStr,
                         sUserNameStr );
        
        IDE_TEST( sdf::checkShardObjectSchema( sStatement,
                                               sSqlStr )
                  != IDE_SUCCESS );
        
        //---------------------------------
        // SET_SHARD_PROCEDURE
        //---------------------------------
        idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                         "exec dbms_shard.set_shard_procedure_local( '"
                         QCM_SQL_STRING_SKIP_FMT"', '"
                         QCM_SQL_STRING_SKIP_FMT"', "
                         "'C', "
                         "'', "
                         "'', "
                         "'', "
                         "'' )",
                         sUserNameStr,
                         sProcNameStr );

        IDE_TEST( sdf::runRemoteQuery( sStatement,
                                       sSqlStr )
                  != IDE_SUCCESS );

        //---------------------------------
        // SET_SHARD_CLONE
        //---------------------------------
        IDE_TEST( executeSetShardClone( sStatement,
                                        sUserNameStr,
                                        sProcNameStr )
                  != IDE_SUCCESS );
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
    IDE_EXCEPTION( ERR_SHARD_USER_NAME_TOO_LONG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_USER_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_SHARD_TABLE_NAME_TOO_LONG )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_TABLE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION( ERR_DDL_TRANSACTION )
    {
        IDE_SET(ideSetErrorCode( sdERR_ABORT_SDC_INSUFFICIENT_ATTRIBUTE,
                                 "TRANSACTIONAL_DDL = 1" ));
    }
    IDE_EXCEPTION( ERR_PROC_REPLACE_OPTION )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_INVALID_OPTION,
                                  sProcReplaceStr ) );
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
        case 1:
            QC_SMI_STMT(sStatement) = sOldStmt;
        default:
            break;
    }

    idlOS::snprintf( sSqlStr, SDF_QUERY_LEN,
                     "ROLLBACK " );

    (void) qciMisc::runDCLforInternal( sStatement,
                                       sSqlStr,
                                       sStatement->session->mMmSession );

        
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC executeSetShardClone( qcStatement    * aStatement,
                             SChar          * aUserNameStr,
                             SChar          * aProcNameStr )
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
                             "EXEC DBMS_SHARD.SET_SHARD_CLONE_LOCAL( '"
                             QCM_SQL_STRING_SKIP_FMT"', '"
                             QCM_SQL_STRING_SKIP_FMT"', '"
                             QCM_SQL_STRING_SKIP_FMT"' )",
                             aUserNameStr,
                             aProcNameStr,
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


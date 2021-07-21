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
 *    SHARD_SET_LOCAL_NODE( shard_node_id                integer
 *                          node_name                    VARCHAR,
 *                          host_ip                      VARCHAR,
 *                          port                         integer,
 *                          internal_host_ip             VARCHAR,
 *                          internal_port_no             integer,
 *                          internal_replication_host_ip VARCHAR,
 *                          internal_replication_port_no integer,
 *                          conn_type           integer
 *                          )
 *    RETURN 0
 *
 **********************************************************************/

#include <cm.h>
#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName sdfFunctionName[1] = {
    { NULL, 20, (void*)"SHARD_SET_LOCAL_NODE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetLocalNodeModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_SetLocalNode( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetLocalNode,
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
    const mtdModule* sModules[9] =
    {
        &mtdInteger, // shard_node_id
        &mtdVarchar, // node name
        &mtdVarchar, // host ip
        &mtdInteger, // port
        &mtdVarchar, // internal host ip
        &mtdInteger, // internal port
        &mtdVarchar, // internal replication host ip
        &mtdInteger, // internal replication port
        &mtdInteger  // conn type
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 9,
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
                                     &mtdInteger,
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

IDE_RC sdfCalculate_SetLocalNode( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     Set Local Node
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    mtdIntegerType            sShardNodeID;
    mtdCharType             * sNodeName;
    SChar                     sNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdCharType             * sHostIP;
    SChar                     sHostIPStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sPort;
    mtdCharType             * sInternalHostIP;
    SChar                     sInternalHostIPStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sInternalPort;
    mtdCharType             * sInternalReplHostIP;
    SChar                     sInternalReplHostIPStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sInternalReplPort;
    mtdIntegerType            sConnType = SDI_NODE_CONNECT_TYPE_DEFAULT;
    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsInCluster = ID_FALSE;
    SChar                     sCommitStr[7] = {0,};

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
                                             aStack[1].value ) == ID_TRUE ) || /* shard_node_id */
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) || /* node_name */
         ( aStack[3].column->module->isNull( aStack[3].column,
                                             aStack[3].value ) == ID_TRUE ) || /* host_ip */
         ( aStack[4].column->module->isNull( aStack[4].column,
                                             aStack[4].value ) == ID_TRUE ) || /* port_no */
         ( aStack[5].column->module->isNull( aStack[5].column,
                                             aStack[5].value ) == ID_TRUE ) || /* internal_host_ip */
         ( aStack[6].column->module->isNull( aStack[6].column,
                                             aStack[6].value ) == ID_TRUE ) || /* internal_port_no */
         ( aStack[7].column->module->isNull( aStack[7].column,
                                             aStack[7].value ) == ID_TRUE ) || /* internal_replication_port_no */
         ( aStack[8].column->module->isNull( aStack[8].column,
                                             aStack[8].value ) == ID_TRUE )    /* internal_replication_port_no */
       )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------
        sShardNodeID = *(mtdIntegerType*)aStack[1].value;
        IDE_TEST_RAISE( ( sShardNodeID > 9200 ) ||
                        ( sShardNodeID < 1 ),
                        ERR_SHARD_NODE_ID );

        sNodeName = (mtdCharType*)aStack[2].value;
        IDE_TEST_RAISE( sNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                       ERR_SHARD_NODE_NAME_TOO_LONG );
        idlOS::strncpy( sNodeNameStr,
                       (SChar*)sNodeName->value,
                       sNodeName->length );
        sNodeNameStr[sNodeName->length] = '\0';

        sHostIP = (mtdCharType*)aStack[3].value;

        IDE_TEST_RAISE( sHostIP->length > SDI_SERVER_IP_SIZE,
                        ERR_IPADDRESS );
        idlOS::strncpy( sHostIPStr,
                        (SChar*)sHostIP->value,
                        sHostIP->length );
        sHostIPStr[sHostIP->length] = '\0';

        IDE_TEST_RAISE( cmiIsValidIPFormat(sHostIPStr) != ID_TRUE, ERR_IPADDRESS );

        sPort = *(mtdIntegerType*)aStack[4].value;
        IDE_TEST_RAISE( ( sPort > ID_USHORT_MAX ) ||
                        ( sPort <= 0 ),
                        ERR_PORT );

        sInternalHostIP = (mtdCharType*)aStack[5].value;

        IDE_TEST_RAISE( sInternalHostIP->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );

        idlOS::strncpy( sInternalHostIPStr,
                        (SChar*)sInternalHostIP->value,
                        sInternalHostIP->length );
        sInternalHostIPStr[sInternalHostIP->length] = '\0';

        IDE_TEST_RAISE( cmiIsValidIPFormat(sInternalHostIPStr) != ID_TRUE, ERR_IPADDRESS );

        sInternalPort = *(mtdIntegerType*)aStack[6].value;

        IDE_TEST_RAISE( ( sInternalPort > ID_USHORT_MAX ) ||
                        ( sInternalPort <= 0 ),
                        ERR_PORT );

        sInternalReplHostIP = (mtdCharType*)aStack[7].value;

        IDE_TEST_RAISE( sInternalReplHostIP->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );

        idlOS::strncpy( sInternalReplHostIPStr,
                        (SChar*)sInternalReplHostIP->value,
                        sInternalReplHostIP->length );
        sInternalReplHostIPStr[sInternalReplHostIP->length] = '\0';

        IDE_TEST_RAISE( cmiIsValidIPFormat(sInternalReplHostIPStr) != ID_TRUE, ERR_IPADDRESS );

        sInternalReplPort = *(mtdIntegerType*)aStack[8].value;

        IDE_TEST_RAISE( ( sInternalReplPort > ID_USHORT_MAX ) ||
                        ( sInternalReplPort <= 0 ),
                        ERR_PORT );

        if ( aStack[9].column->module->isNull( aStack[9].column,
                                               aStack[9].value ) == ID_TRUE ) /* conn_type */
        {
            sConnType = SDI_NODE_CONNECT_TYPE_TCP;
        }
        else
        {
            sConnType = *(mtdIntegerType*)aStack[9].value;

            switch ( sConnType )
            {
                case SDI_NODE_CONNECT_TYPE_TCP :
                case SDI_NODE_CONNECT_TYPE_IB  :
                    /* Nothing to do */
                    break;

                default :
                    IDE_RAISE( ERR_SHARD_UNSUPPORTED_INTERNAL_CONNTYPE );
                    break;
            }
        }

        //---------------------------------
        // Begin Statement for shard meta
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

        IDE_TEST( sdm::checkIsInCluster( sStatement,
                                         &sIsInCluster ) != IDE_SUCCESS );

        IDE_TEST_RAISE( sIsInCluster == ID_TRUE, ERR_ALREADY_IN_CLUSTER );

        /* update local meta info */
        IDE_TEST( sdm::updateLocalMetaInfo( sStatement,
                                   (sdiShardNodeId)sShardNodeID,
                                   (SChar*)sNodeNameStr,
                                   (SChar*)sHostIPStr,
                                   (UInt)sPort,
                                   (SChar*)sInternalHostIPStr,
                                   (UInt)sInternalPort,
                                   (SChar*)sInternalReplHostIPStr,
                                   (UInt)sInternalReplPort,
                                   (UInt)sConnType,
                                   &sRowCnt )
                  != IDE_SUCCESS );

        //---------------------------------
        // End Statement
        //---------------------------------

        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sState = 1;

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;

        IDE_TEST_RAISE( sRowCnt != 1, ERR_SHARD_META );

        //---------------------------------
        // Commit
        //---------------------------------

        idlOS::snprintf( sCommitStr, idlOS::strlen("COMMIT") + 1, "COMMIT" );

        IDE_TEST( qciMisc::runDCLforInternal( sStatement,
                                              sCommitStr,
                                              sStatement->session->mMmSession )
                  != IDE_SUCCESS );
    }

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSIDE_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_PSM_INSIDE_QUERY ) );
    }
    IDE_EXCEPTION( ERR_SHARD_UNSUPPORTED_INTERNAL_CONNTYPE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_UNSUPPORTED_META_CONNTYPE, sConnType ) );
    }
    IDE_EXCEPTION( ERR_SHARD_NODE_ID );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION( ERR_SHARD_NODE_NAME_TOO_LONG );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_SHARD_NODE_NAME_TOO_LONG ) );
    }
    IDE_EXCEPTION( ERR_IPADDRESS );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSF_INVALID_IPADDRESS ) );
    }
    IDE_EXCEPTION( ERR_PORT );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSF_INVALID_PORT ) );
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_SHARD_META );
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDF_SHARD_LOCAL_META_ERROR));
    }
    IDE_EXCEPTION( ERR_NO_GRANT )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QRC_NO_GRANT ) );
    }
    IDE_EXCEPTION( ERR_ALREADY_IN_CLUSTER )
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDI_ALREADY_IN_CLUSTER ) );
    }
    IDE_EXCEPTION_END;

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

    return IDE_FAILURE;
}

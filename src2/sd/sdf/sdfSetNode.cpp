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
 *    SHARD_SET_NODE( node_name             VARCHAR,
 *                    remote_host           VARCHAR,
 *                    remote_port           integer,
 *                    internal_remote_host  VARCHAR,
 *                    internal_remote_port  integer,
 *                    conn_type             integer,
 *                    node_id               integer )
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
    { NULL, 14, (void*)"SHARD_SET_NODE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfSetNodeModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};


IDE_RC sdfCalculate_SetNode( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_SetNode,
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
        &mtdVarchar, // node name
        &mtdVarchar, // host ip
        &mtdInteger, // port
        &mtdVarchar, // alternate host ip
        &mtdInteger, // alternate port
        &mtdInteger, // conn type
        &mtdInteger  // node id
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

IDE_RC sdfCalculate_SetNode( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     Set Data Node
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    UInt                      sNodeID;
    mtdCharType             * sNodeName;
    SChar                     sNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdCharType             * sRemoteHost;
    SChar                     sRemoteHostStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sPort;
    mtdCharType             * sInternalRemoteHost;
    SChar                     sInternalRemoteHostStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sInternalPort;
    mtdIntegerType            sConnType = SDI_NODE_CONNECT_TYPE_DEFAULT;
    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;

    sdiInternalOperation      sInternalOP = SDI_INTERNAL_OP_NOT;

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

        // shard node name
        sNodeName = (mtdCharType*)aStack[1].value;

        IDE_TEST_RAISE( sNodeName->length > SDI_NODE_NAME_MAX_SIZE,
                       ERR_SHARD_NODE_NAME_TOO_LONG );
        idlOS::strncpy( sNodeNameStr,
                       (SChar*)sNodeName->value,
                       sNodeName->length );
        sNodeNameStr[sNodeName->length] = '\0';

        // remote host
        sRemoteHost = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sRemoteHost->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );
        idlOS::strncpy( sRemoteHostStr,
                        (SChar*)sRemoteHost->value,
                        sRemoteHost->length );
        sRemoteHostStr[sRemoteHost->length] = '\0';

        // port no
        sPort = *(mtdIntegerType*)aStack[3].value;
        IDE_TEST_RAISE( ( sPort > ID_USHORT_MAX ) ||
                        ( sPort <= 0 ),
                        ERR_PORT );

        // internal remote host
        if ( aStack[4].column->module->isNull( aStack[4].column,
                                               aStack[4].value ) == ID_TRUE )
        {
            sInternalRemoteHostStr[0] = '\0';
        }
        else
        {
            sInternalRemoteHost = (mtdCharType*)aStack[4].value;

            IDE_TEST_RAISE( sInternalRemoteHost->length > IDL_IP_ADDR_MAX_LEN,
                            ERR_IPADDRESS );
            idlOS::strncpy( sInternalRemoteHostStr,
                            (SChar*)sInternalRemoteHost->value,
                            sInternalRemoteHost->length );
            sInternalRemoteHostStr[sInternalRemoteHost->length] = '\0';
        }

        // internal port no
        if ( aStack[5].column->module->isNull( aStack[5].column,
                                               aStack[5].value ) == ID_TRUE )
        {
            sInternalPort = 0;
        }
        else
        {
            sInternalPort = *(mtdIntegerType*)aStack[5].value;
            IDE_TEST_RAISE( ( sInternalPort > ID_USHORT_MAX ) ||
                            ( sInternalPort <= 0 ),
                            ERR_PORT );
        }

        // conn_type
        if ( aStack[6].column->module->isNull( aStack[6].column,
                                               aStack[6].value ) == ID_TRUE )
        {
            sConnType = SDI_NODE_CONNECT_TYPE_TCP;
        }
        else
        {
            sConnType = *(mtdIntegerType*)aStack[6].value;

            switch ( sConnType )
            {
                case SDI_NODE_CONNECT_TYPE_TCP :
                case SDI_NODE_CONNECT_TYPE_IB  :
                    /* Nothing to do */
                    break;

                default :
                    IDE_RAISE( ERR_SHARD_UNSUPPORTED_META_CONNTYPE );
                    break;
            }
        }

        // node_id
        if ( aStack[7].column->module->isNull( aStack[7].column,
                                               aStack[7].value ) == ID_TRUE )
        {
            sNodeID = SDI_NODE_NULL_ID;
        }
        else
        {
            sNodeID = *(mtdIntegerType*)aStack[7].value;
            IDE_TEST_RAISE( ( sNodeID > 9999 ) ||
                            ( sNodeID < 1 ),
                            ERR_ARGUMENT_NOT_APPLICABLE );
        }

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

        //---------------------------------
        // Insert Nodes Meta
        //---------------------------------

        IDE_TEST( sdm::insertNode( sStatement,
                                   &sNodeID,
                                   (SChar*)sNodeNameStr,
                                   (UInt)sPort,
                                   (SChar*)sRemoteHostStr,
                                   (UInt)sInternalPort,
                                   (SChar*)sInternalRemoteHostStr,
                                   (UInt)sConnType,
                                   &sRowCnt )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sRowCnt == 0,
                        ERR_EXIST_SHARD_NODE );
        //---------------------------------
        // Insert Replica Sets Meta And Reorganize
        //---------------------------------
        /* Failback 수행시 ReplicaSet은 추가되지 않고 자동으로 조정된다. */
        switch ( sInternalOP )
        {
            case SDI_INTERNAL_OP_NOT:
            case SDI_INTERNAL_OP_NORMAL:
                IDE_TEST( sdm::insertReplicaSet( sStatement,
                                                 sNodeID,
                                                 (SChar*)sNodeNameStr,
                                                 &sRowCnt )
                          != IDE_SUCCESS );

                IDE_TEST_RAISE( sRowCnt == 0,
                                ERR_INSERT_REPLICA_SETS );

                IDE_TEST( sdm::reorganizeReplicaSet( sStatement,
                                                    (SChar*)sNodeNameStr,
                                                    &sRowCnt )
                          != IDE_SUCCESS );
                break;
            case SDI_INTERNAL_OP_FAILOVER:
            case SDI_INTERNAL_OP_FAILBACK:
                break;
            default:
                IDE_DASSERT( 0 );
                break;
        }

        //---------------------------------
        // End Statement
        //---------------------------------

        sState = 1;
        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );

        sState = 0;
        QC_SMI_STMT(sStatement) = sOldStmt;
    }

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INSIDE_QUERY )
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QSX_PSM_INSIDE_QUERY ) );
    }
    IDE_EXCEPTION( ERR_SHARD_UNSUPPORTED_META_CONNTYPE );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDF_UNSUPPORTED_META_CONNTYPE, sConnType ) );
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
    IDE_EXCEPTION( ERR_EXIST_SHARD_NODE );
    {
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDF_AREADY_EXIST_SHARD_NODE));
    }
    IDE_EXCEPTION( ERR_INSERT_REPLICA_SETS );
    {
        IDE_SET( ideSetErrorCode( sdERR_ABORT_SDC_UNEXPECTED_ERROR , "sdfSetShardNode", "replica set insert error") );
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

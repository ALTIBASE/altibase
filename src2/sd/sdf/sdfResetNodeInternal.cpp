/** 
 *  Copyright (c) 1999~2018, Altibase Corp. and/or its affiliates. All rights reserved.
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
 *    RESET_NODE_INTERNAL( node_name         in varchar(40),
 *                         host_ip           in varchar(16),
 *                         port_no           in integer,
 *                         alternate_host_ip in varchar(16),
 *                         alternate_port_no in integer,
 *                         conn_type         in integer )
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
    { NULL, 25, (void*)"SHARD_RESET_NODE_INTERNAL" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfResetNodeInternalModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_ResetNodeInternal( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_ResetNodeInternal,
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
        &mtdVarchar, // node name
        &mtdVarchar, // host ip
        &mtdInteger, // port
        &mtdVarchar, // alternate host ip
        &mtdInteger, // alternate port
        &mtdInteger  // conn type
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

IDE_RC sdfCalculate_ResetNodeInternal( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Reset Data Node Internal Access Information
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    mtdCharType             * sNodeName;
    SChar                     sNodeNameStr[SDI_NODE_NAME_MAX_SIZE + 1];
    mtdCharType             * sActiveHost;
    SChar                     sActiveHostStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdCharType             * sAlternateHost;
    SChar                     sAlternateHostStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sPort;
    mtdIntegerType            sAlternatePort;
    mtdIntegerType            sConnType = SDI_NODE_CONNECT_TYPE_DEFAULT;
    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sStatement->mFlag &= ~QC_STMT_SHARD_META_CHANGE_MASK;
    sStatement->mFlag |= QC_STMT_SHARD_META_CHANGE_TRUE;

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
                                             aStack[3].value ) == ID_TRUE ) ||
         ( aStack[6].column->module->isNull( aStack[6].column,
                                             aStack[6].value ) == ID_TRUE ) )
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

        IDE_TEST_RAISE( sNodeName->length > SDI_CHECK_NODE_NAME_MAX_SIZE,
                        ERR_SHARD_NODE_NAME_TOO_LONG );
        idlOS::strncpy( sNodeNameStr,
                        (SChar*)sNodeName->value,
                        sNodeName->length );
        sNodeNameStr[sNodeName->length] = '\0';

        // host ip
        sActiveHost = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sActiveHost->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );
        if ( sActiveHost->length > 0 )
        {
            idlOS::strncpy( sActiveHostStr,
                            (SChar*)sActiveHost->value,
                            sActiveHost->length );
            sActiveHostStr[sActiveHost->length] = '\0';
        }
        else
        {
            sActiveHostStr[0] = '\0';
        }

        // port no
        if ( aStack[3].column->module->isNull( aStack[3].column,
                                               aStack[3].value ) == ID_TRUE )
        {
            sPort = 0;
        }
        else
        {
            sPort = *(mtdIntegerType*)aStack[3].value;
            IDE_TEST_RAISE( ( sPort > ID_USHORT_MAX ) ||
                            ( sPort <= 0 ),
                            ERR_PORT );
        }

        // alternate host ip
        sAlternateHost = (mtdCharType*)aStack[4].value;

        IDE_TEST_RAISE( sAlternateHost->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );
        if ( sAlternateHost->length > 0 )
        {
            idlOS::strncpy( sAlternateHostStr,
                            (SChar*)sAlternateHost->value,
                            sAlternateHost->length );
            sAlternateHostStr[sAlternateHost->length] = '\0';
        }
        else
        {
            sAlternateHostStr[0] = '\0';
        }

        // alternate port no
        if ( aStack[5].column->module->isNull( aStack[5].column,
                                               aStack[5].value ) == ID_TRUE )
        {
            sAlternatePort = 0;
        }
        else
        {
            sAlternatePort = *(mtdIntegerType*)aStack[5].value;
            IDE_TEST_RAISE( ( sAlternatePort > ID_USHORT_MAX ) ||
                            ( sAlternatePort <= 0 ),
                            ERR_PORT );
        }

        // conn type
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

        IDE_TEST( sdm::updateNodeInternal( sStatement,
                                           (SChar*)sNodeNameStr,
                                           (SChar*)sActiveHostStr,
                                           (UInt)sPort,
                                           (SChar*)sAlternateHostStr,
                                           (UInt)sAlternatePort,
                                           sConnType,
                                           &sRowCnt )
                  != IDE_SUCCESS );

        //---------------------------------
        // End Statement
        //---------------------------------

        IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
        sState = 1;

        QC_SMI_STMT(sStatement) = sOldStmt;
        sState = 0;

        IDE_TEST_RAISE( sRowCnt == 0,
                        ERR_EXIST_SHARD_NODE );
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
        IDE_SET(ideSetErrorCode(sdERR_ABORT_SDF_INVALID_SHARD_NODE));
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
            /* fall through */
        case 1:
            QC_SMI_STMT(sStatement) = sOldStmt;
            /* fall through */
        default:
            break;
    }

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

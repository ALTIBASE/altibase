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
 *    SHARD_RESET_NODE( node_name    VARCHAR,
 *                      remote_host  VARCHAR,
 *                      remote_port  integer,
 *                      is_alternate integer )
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
    { NULL, 16, (void*)"SHARD_RESET_NODE" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfResetNodeModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_ResetNode( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_ResetNode,
    NULL,
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
        &mtdVarchar, // node name
        &mtdVarchar, // host ip
        &mtdInteger, // port
        &mtdInteger  // is alternate
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

IDE_RC sdfCalculate_ResetNode( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *      Reset Data Node
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
    SChar                     sNodeNameStr[QC_MAX_NAME_LEN + 1];
    mtdCharType             * sRemoteHost;
    SChar                     sRemoteHostStr[IDL_IP_ADDR_MAX_LEN + 1];
    mtdIntegerType            sPort;
    mtdIntegerType            sIsAlternate;
    idBool                    sIsAlternateFlag;
    UInt                      sRowCnt = 0;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;

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
         ( aStack[4].column->module->isNull( aStack[4].column,
                                             aStack[4].value ) == ID_TRUE ) )
    {
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        //---------------------------------
        // Argument Validation
        //---------------------------------

        // is alternate
        sIsAlternate = *(mtdIntegerType*)aStack[4].value;
        if ( sIsAlternate != 0 )
        {
            sIsAlternateFlag = ID_TRUE;
        }
        else
        {
            sIsAlternateFlag = ID_FALSE;
        }

        if ( sIsAlternateFlag == ID_FALSE )
        {
            IDE_TEST_RAISE(
                ( aStack[2].column->module->isNull( aStack[2].column,
                                                    aStack[2].value ) == ID_TRUE ) ||
                ( aStack[3].column->module->isNull( aStack[3].column,
                                                    aStack[3].value ) == ID_TRUE ),
                ERR_ARGUMENT_NOT_APPLICABLE );
        }
        else
        {
            // alternate인 경우 ip/port가 null일 수 있음
            // Nothing to do.
        }

        // shard node name
        sNodeName = (mtdCharType*)aStack[1].value;

        IDE_TEST_RAISE( sNodeName->length > QC_MAX_NAME_LEN,
                       ERR_SHARD_NODE_NAME_TOO_LONG );
        idlOS::strncpy( sNodeNameStr,
                        (SChar*)sNodeName->value,
                        sNodeName->length );
        sNodeNameStr[sNodeName->length] = '\0';

        // remote host
        sRemoteHost = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( sRemoteHost->length > IDL_IP_ADDR_MAX_LEN,
                        ERR_IPADDRESS );
        if ( sRemoteHost->length > 0 )
        {
            idlOS::strncpy( sRemoteHostStr,
                            (SChar*)sRemoteHost->value,
                            sRemoteHost->length );
            sRemoteHostStr[sRemoteHost->length] = '\0';
        }
        else
        {
            sRemoteHostStr[0] = '\0';
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
            IDE_TEST_RAISE( ( sPort > 65535 ) ||
                            ( sPort <= 0 ),
                            ERR_PORT );
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

        IDE_TEST( sdm::updateNode( sStatement,
                                   (SChar*)sNodeNameStr,
                                   sIsAlternateFlag,
                                   (UInt)sPort,
                                   (SChar*)sRemoteHostStr,
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
                        ERR_EXIST_SHARD_NODE );
    }

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

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

    switch ( sState )
    {
        case 2:
            if ( sSmiStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
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

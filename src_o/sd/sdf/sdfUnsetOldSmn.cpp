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
 *    SHARD_UNSET_OLD_NODE( smn bigint default null )
 *    RETURN 0
 *
 **********************************************************************/

#include <sdf.h>
#include <sdm.h>
#include <smi.h>
#include <qcg.h>

extern mtdModule mtdBigint;
extern mtdModule mtdInteger;

static mtcName sdfFunctionName[1] = {
    { NULL, 19, (void*)"SHARD_UNSET_OLD_SMN" }
};

static IDE_RC sdfEstimate( mtcNode*        aNode,
                           mtcTemplate*    aTemplate,
                           mtcStack*       aStack,
                           SInt            aRemain,
                           mtcCallBack*    aCallBack );

mtfModule sdfUnsetOldSmnModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    sdfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    sdfEstimate
};

IDE_RC sdfCalculate_UnsetOldSmn( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute sdfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    sdfCalculate_UnsetOldSmn,
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
    const mtdModule* sModules[1] =
    {
        &mtdBigint // SMN
    };
    const mtdModule* sModule = &mtdInteger; // return(dummy value)

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
    {   
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }
    else
    {
        // No argument.
        // Nothing to do.
    }

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

IDE_RC sdfCalculate_UnsetOldSmn( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     Unset Old Smn
 *
 * Implementation :
 *      Argument Validation
 *      Begin Statement for meta
 *      Check Privilege
 *      End Statement
 *
 ***********************************************************************/

    qcStatement             * sStatement;
    smiStatement            * sOldStmt;
    smiStatement              sSmiStmt;
    UInt                      sSmiStmtFlag;
    SInt                      sState = 0;
    ULong                   * sSMN = NULL;
    idBool                    sIsOldSessionShardMetaTouched = ID_FALSE;

    sStatement   = ((qcTemplate*)aTemplate)->stmt;

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
    // Delete Meta
    //---------------------------------
    
    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_FALSE ) )
    {
        sSMN = (ULong*)aStack[1].value;
    }
    else
    {
        // Argument is NULL.
        // sSMN is NULL(initiali value).
        // Nothing to do.
    }

    IDE_TEST( sdm::deleteOldSMN( sStatement,
                                 sSMN )
              != IDE_SUCCESS );

    //---------------------------------
    // End Statement
    //---------------------------------

    IDE_TEST( sSmiStmt.end( SMI_STATEMENT_RESULT_SUCCESS ) != IDE_SUCCESS );
    sState = 1;

    sState = 0;
    QC_SMI_STMT(sStatement) = sOldStmt;

    *(mtdIntegerType*)aStack[0].value = 0;

    return IDE_SUCCESS;

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

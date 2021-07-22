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
 **********************************************************************/

#include <qsf.h>
#include <qci.h>
#include <qcg.h>
#include <qsxEnv.h>

extern mtdModule mtdInteger;

static mtcName qsfIsLastArrayBoundFunctionName[1] = {
    { NULL, 22, (void*)"SP_IS_LAST_ARRAY_BOUND" }
};

static IDE_RC qsfIsLastArrayBoundEstimate( mtcNode*     aNode,
                                        mtcTemplate* aTemplate,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        mtcCallBack* aCallBack );

IDE_RC qsfIsLastArrayBoundCalculate(mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

mtfModule qsfIsLastArrayBoundModule = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    qsfIsLastArrayBoundFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfIsLastArrayBoundEstimate
};

const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfIsLastArrayBoundCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfIsLastArrayBoundEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         /* aRemain */,
                                mtcCallBack* /* aCallBack */)
{
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 0,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfIsLastArrayBoundCalculate(mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
    qcStatement         * sStatement;
    qciArrayBindContext   sContext;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sStatement = ((qcTemplate*)aTemplate)->stmt;

    sContext.mMmStatement = QC_MM_STMT( sStatement );

    // Get Array Bind Information
    IDE_TEST( qci::mSessionCallback.mGetArrayBindInfo( & sContext )
              != IDE_SUCCESS );

    if ( sContext.mIsArrayBound == ID_TRUE )
    {
        if ( sContext.mCurrRowCnt == sContext.mTotalRowCnt )
        {
            *(mtdIntegerType*)aStack[0].value = 1;
        }
        else
        {
            *(mtdIntegerType*)aStack[0].value = 0;
        }
    }
    else
    {
        *(mtdIntegerType*)aStack[0].value = 0;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

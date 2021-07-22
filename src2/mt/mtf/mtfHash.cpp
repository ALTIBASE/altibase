/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id$
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfHash;

extern mtdModule mtdBigint;

static mtcName mtfHashFunctionName[1] = {
    { NULL, 4, (void*)"HASH" }
};

static IDE_RC mtfHashEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               mtcCallBack* aCallBack );

mtfModule mtfHash = {
    1|MTC_NODE_OPERATOR_FUNCTION|MTC_NODE_EAT_NULL_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfHashFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfHashEstimate
};

static IDE_RC mtfHashCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfHashCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfHashEstimate( mtcNode*     aNode,
                        mtcTemplate* aTemplate,
                        mtcStack*    aStack,
                        SInt      /* aRemain */,
                        mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[1] = { & mtdBigint };
    
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( (( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1) ||
                    (( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( (aStack[1].column->module->id == MTD_LIST_ID ) ||
                    (aStack[1].column->module->id == MTD_ROWTYPE_ID ) ||
                    (aStack[1].column->module->id == MTD_RECORDTYPE_ID ) ||
                    (aStack[1].column->module->id == MTD_ASSOCIATIVE_ARRAY_ID ),
                    ERR_CONVERSION_NOT_APPLICABLE );

    IDE_TEST_RAISE( aStack[1].column->module->hash == NULL,
                    ERR_NO_HASH_FUNC );

    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
    {
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments->next,
                                            aTemplate,
                                            aStack + 2,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
    
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION( ERR_NO_HASH_FUNC );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_UNEXPECTED_ERROR,
                            "mtfHashEstimate",
                            "hash function not found" ));
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfHashCalculate( mtcNode*     aNode,
                         mtcStack*    aStack,
                         SInt         aRemain,
                         void*        aInfo,
                         mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Hash Calculate
 *
 * Implementation :
 *
 ***********************************************************************/

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    *(mtdBigintType*)aStack[0].value =
        (mtdBigintType)aStack[1].column->module->hash(
            mtc::hashInitialValue,
            aStack[1].column,
            aStack[1].value );
        
    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
    {
        *(mtdBigintType*)aStack[0].value =
            *(mtdBigintType*)aStack[0].value % *(mtdBigintType*)aStack[2].value;
    }
    else
    {
        // Nothing to do.
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

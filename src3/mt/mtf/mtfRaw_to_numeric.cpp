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
 * $Id: mtfRaw_to_numeric.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfRaw_to_numeric;

extern mtdModule mtdVarbyte;
extern mtdModule mtdInteger;
extern mtdModule mtdFloat;

static mtcName mtfRaw_to_numericFunctionName[2] = {
    { mtfRaw_to_numericFunctionName + 1, 14, (void*)"RAW_TO_NUMERIC" },
    { NULL,                              12, (void*)"RAW_TO_FLOAT" }
};

static IDE_RC mtfRaw_to_numericEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfRaw_to_numeric = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRaw_to_numericFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRaw_to_numericEstimate
};

static IDE_RC mtfRaw_to_numericCalculate( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRaw_to_numericCalculate,
    NULL,
    mtx::calculateNA,    
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRaw_to_numericEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt      /* aRemain */,
                                  mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdFloat;
    sModules[1] = &mtdVarbyte;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules + 1 )
             != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     MTD_FLOAT_PRECISION_MAXIMUM,
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

IDE_RC mtfRaw_to_numericCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : RAW_TO_NUMERIC Calculate
 *
 * Implementation :
 *    RAW_TO_NUMERIC( varbyte )
 ***********************************************************************/
    
    const mtdModule* sModule;
    mtdNumericType*  sResult;
    mtdByteType*     sValue;
    UInt             sByteLen;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sResult = (mtdNumericType*)aStack[0].value;

    sModule = aStack[1].column->module;
    
    if ( sModule->isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE )
    {
        // 입력된 자료가 NULL인 경우
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue = (mtdByteType*)aStack[1].value;
        sByteLen = sValue->length;
        IDE_TEST_RAISE( sByteLen > MTD_FLOAT_SIZE_MAXIMUM, ERR_TOO_LONG )
        
        idlOS::memcpy( sResult, sValue->value, sByteLen );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

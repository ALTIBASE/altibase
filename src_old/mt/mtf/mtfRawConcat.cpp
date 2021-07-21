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
 * $Id: mtfRawConcat.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfRawConcat;

extern mtdModule mtdVarbyte;

static mtcName mtfRawConcatFunctionName[1] = {
    { NULL, 10, (void*)"RAW_CONCAT" }
};

static IDE_RC mtfRawConcatEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfRawConcat = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRawConcatFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRawConcatEstimate
};

static IDE_RC mtfRawConcatCalculate( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRawConcatCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRawConcatEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt      /* aRemain */,
                             mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];

    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdVarbyte;
    sModules[1] = &mtdVarbyte;
    sModules[2] = &mtdVarbyte;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules + 1 )
              != IDE_SUCCESS );

    sPrecision = aStack[1].column->precision + aStack[2].column->precision;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    if ( sPrecision > MTD_VARBYTE_PRECISION_MAXIMUM )
    {
        sPrecision = MTD_VARBYTE_PRECISION_MAXIMUM;
    }
    else
    {
        /* Nothing to do */
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     sPrecision,
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

IDE_RC mtfRawConcatCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Concat Calculate
 *
 * Implementation :
 *    RAW_CONCAT( byte1, byte2 )
 *
 *    aStack[0] : 첫번째 byte값에 두번째 byte값을 연결한 값
 *    aStack[1] : byte1 ( 첫번째 byte값 )
 *    aStack[2] : byte2 ( 두번째 byte값 )
 *
 ***********************************************************************/
    
    mtdByteType*   sResult;
    mtdByteType*   sFirstString;
    mtdByteType*   sSecondString;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sResult = (mtdByteType*)aStack[0].value;
    sFirstString = (mtdByteType*)aStack[1].value;
    sSecondString = (mtdByteType*)aStack[2].value;
    
    IDE_TEST_RAISE( (SInt)sFirstString->length + (SInt)sSecondString->length >
                    aStack[0].column->precision,
                    ERR_INVALID_LENGTH );

    // 첫번째 byte값을 결과에 복사
    idlOS::memcpy( sResult->value,
                   sFirstString->value,
                   sFirstString->length );

    // 첫번째 byte값 복사한 다음 위치에 두번째 byte값 복사
    idlOS::memcpy( sResult->value + sFirstString->length,
                   sSecondString->value,
                   sSecondString->length );
    
    sResult->length = sFirstString->length + sSecondString->length;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

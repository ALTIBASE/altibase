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
 * $Id: mtfConcat.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfConcat;

static mtcName mtfConcatFunctionName[2] = {
    { mtfConcatFunctionName+1, 2, (void*)"||" },
    { NULL,                    6, (void*)"CONCAT" }
};

static IDE_RC mtfConcatEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfConcat = {
    1|MTC_NODE_OPERATOR_FUNCTION|
        MTC_NODE_PRINT_FMT_INFIX_SP,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfConcatFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfConcatEstimate
};

static IDE_RC mtfConcatCalculate( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfConcatCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfConcatEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt      /* aRemain */,
                          mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];
    const mtdModule* sConcatResultModule;
    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[1],
                                            aStack[2].column->module )
              != IDE_SUCCESS );

    // concat의 두 인자는 char 아니면 varchar 타입이다.
    // 둘다 char 타입이면 결과는 char가 되고,
    // 아닌 경우엔 varchar 타입이 된다.
    IDE_TEST( mtf::getComparisonModule( &sConcatResultModule,
                                        sModules[0]->no,
                                        sModules[1]->no )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sConcatResultModule == NULL,
                    ERR_CONVERSION_NOT_APPLICABLE );

    sModules[0] = sConcatResultModule;
    sModules[1] = sConcatResultModule;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    sPrecision = aStack[1].column->precision + aStack[2].column->precision;

    if( (sConcatResultModule->id == MTD_NCHAR_ID) ||
        (sConcatResultModule->id == MTD_NVARCHAR_ID) )
    {
        if( mtl::mNationalCharSet->id == MTL_UTF8_ID )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecute;

            sPrecision = IDL_MIN( sPrecision,
                                  (SInt)MTD_UTF8_NCHAR_PRECISION_MAXIMUM );
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecute;

            sPrecision = IDL_MIN( sPrecision,
                                  (SInt)MTD_UTF16_NCHAR_PRECISION_MAXIMUM );
        }
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        if( sPrecision > MTD_CHAR_PRECISION_MAXIMUM )
        {
            sPrecision = MTD_CHAR_PRECISION_MAXIMUM;
        }
    }

    //IDE_TEST( sModules[0]->estimate( aStack[0].column, 1, sPrecision, 0 )
    //          != IDE_SUCCESS );
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

    IDE_EXCEPTION( ERR_CONVERSION_NOT_APPLICABLE );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConcatCalculate( mtcNode*     aNode,
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
 *    CONCAT( char1, char2 )
 *
 *    aStack[0] : 첫번째 문자값에 두번째 문자값을 연결한 값
 *    aStack[1] : char1 ( 첫번째 문자값 )
 *    aStack[2] : char2 ( 두번째 문자값 )
 *
 ***********************************************************************/
    
    mtdCharType*   sResult;
    mtdCharType*   sFirstString;
    mtdCharType*   sSecondString;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sResult = (mtdCharType*)aStack[0].value;
    sFirstString = (mtdCharType*)aStack[1].value;
    sSecondString = (mtdCharType*)aStack[2].value;
    
    IDE_TEST_RAISE( (UInt)sFirstString->length + (UInt)sSecondString->length >
                    MTD_CHAR_PRECISION_MAXIMUM,
                    ERR_INVALID_LENGTH );

    // 첫번째 문자값을 결과에 복사
    idlOS::memcpy( sResult->value,
                   sFirstString->value,
                   sFirstString->length );

    // 첫번째 문자값 복사한 다음 위치에 두번째 문자값 복사
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
 

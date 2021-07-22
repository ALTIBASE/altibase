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
 * $Id: mtfDigits.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfDigits;

extern mtdModule mtdChar;
extern mtdModule mtdInteger;
extern mtdModule mtdNumeric;

static mtcName mtfDigitsFunctionName[1] = {
    { NULL, 6, (void*)"DIGITS" }
};

static IDE_RC mtfDigitsEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfDigits = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDigitsFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDigitsEstimate
};

static IDE_RC mtfDigitsCalculate( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDigitsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDigitsEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt         /* aRemain */,
                          mtcCallBack* /* aCallBack */ )
{
    SInt    sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( aStack[1].column->module->id == MTD_SMALLINT_ID )
    {
        sPrecision = 5;
    }
    else if ( aStack[1].column->module->id == MTD_INTEGER_ID )
    {
        sPrecision = 10;
    }
    else if ( aStack[1].column->module->id == MTD_BIGINT_ID )
    {
        sPrecision = 19;
    }
    else
    {
        IDE_RAISE ( ERR_CONVERT );
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdChar,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDigitsCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : 
 *
 * Implementation :
 *    BINARY_LENGTH( char )
 *
 *    aStack[0] : 입력된 문자열의 길이 
 *    aStack[1] : char ( 입력된 문자열 )
 *
 ***********************************************************************/

    mtdCharType*           sResult;
    mtdSmallintType        sSmallNum;
    mtdIntegerType         sIntNum;
    mtdBigintType          sBigNum;
    SChar                  sTemp[38];

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
        
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdCharType*)aStack[0].value;
        sTemp[0] = '\0';

        if ( aStack[1].column->module->id == MTD_SMALLINT_ID )
        {
            sSmallNum = *(mtdSmallintType*)aStack[1].value;
            if ( sSmallNum < 0 )
            {
                sSmallNum = sSmallNum * (-1);
            }

            idlVA::appendFormat( sTemp, 6, "%05"ID_INT32_FMT, sSmallNum );
            idlOS::memcpy( sResult->value,
                           sTemp,
                           5 );
            sResult->length = 5;
        }
        else if ( aStack[1].column->module->id == MTD_INTEGER_ID )
        {
            sIntNum = *(mtdIntegerType*)aStack[1].value;
            if ( sIntNum < 0 )
            {
                sIntNum = sIntNum * (-1);
            }

            idlVA::appendFormat( sTemp, 11, "%010"ID_INT32_FMT, sIntNum );
            idlOS::memcpy( sResult->value,
                           sTemp,
                           10 );
            sResult->length = 10;
        }
        else if ( aStack[1].column->module->id == MTD_BIGINT_ID )
        {
            sBigNum = *(mtdBigintType*)aStack[1].value;
            if ( sBigNum < 0 )
            {
                sBigNum = sBigNum * (-1);
            }

            idlVA::appendFormat( sTemp, 20, "%019"ID_INT64_FMT, sBigNum );
            idlOS::memcpy( sResult->value,
                           sTemp,
                           19 );
            sResult->length = 19;
        }
        else
        {
            IDE_RAISE ( ERR_CONVERT );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_CONVERSION_NOT_APPLICABLE));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

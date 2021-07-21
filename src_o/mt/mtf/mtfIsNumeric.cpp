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

extern mtfModule mtfIsNumeric;

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;

static mtcName mtfIsNumericFunctionName[2] = {
    { mtfIsNumericFunctionName+1, 10, (void*)"IS_NUMERIC" },
    { NULL,                         9, (void*)"ISNUMERIC"  }
};

static IDE_RC mtfIsNumericEstimate( mtcNode     * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack    * sStack,
                                    SInt          aRemain,
                                    mtcCallBack * aCallBack );

mtfModule mtfIsNumeric = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfIsNumericFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfIsNumericEstimate
};

IDE_RC mtfIsNumericCalculate( mtcNode     * aNodeg,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfIsNumericCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfIsNumericEstimate( mtcNode     * aNode,
                             mtcTemplate * aTemplate,
                             mtcStack    * aStack,
                             SInt          /* aRemain */,
                             mtcCallBack * aCallBack )
{
    const mtdModule * sModules[1];

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdVarchar;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfIsNumericCalculate( mtcNode     * aNode,
                              mtcStack    * aStack,
                              SInt          aRemain,
                              void        * aInfo,
                              mtcTemplate * aTemplate )
{
    mtdCharType     * sCharValue;
    mtdNumericType  * sNumeric;
    UChar             sNumericBuffer[MTD_NUMERIC_SIZE_MAXIMUM];

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value )
        == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sCharValue = (mtdCharType*)aStack[1].value;
        sNumeric = (mtdNumericType*)sNumericBuffer;

        if ( mtc::makeNumeric( sNumeric,
                               MTD_NUMERIC_MANTISSA_MAXIMUM,
                               sCharValue->value,
                               sCharValue->length )
             == IDE_SUCCESS )
        {
            *(mtdIntegerType*)aStack[0].value = 1;
        }
        else
        {
            *(mtdIntegerType*)aStack[0].value = 0;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

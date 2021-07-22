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
 * $Id: mtfNumOr.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfNumOr;
extern mtdModule mtdBigint;
extern mtdModule mtdInteger;

static mtcName mtfNumOrFunctionName[1] = {
    { NULL, 5, (void*)"NUMOR" }
};

static IDE_RC mtfNumOrEstimate( mtcNode     * aNode,
                                mtcTemplate * aTemplate,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                mtcCallBack * aCallBack );

mtfModule mtfNumOr = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfNumOrFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNumOrEstimate
};

IDE_RC mtfNumOrCalculate( mtcNode     * aNode,
                          mtcStack    * aStack,
                          SInt          aRemain,
                          void        * aInfo,
                          mtcTemplate * aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNumOrCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNumOrEstimate( mtcNode     * aNode,
                         mtcTemplate * aTemplate,
                         mtcStack    * aStack,
                         SInt       /* aRemain */,
                         mtcCallBack * aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdBigint,
        &mtdBigint,
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNumOrCalculate( mtcNode     * aNode,
                          mtcStack    * aStack,
                          SInt          aRemain,
                          void        * aInfo,
                          mtcTemplate * aTemplate )
{
    mtdBigintType sResult = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult =
            *(mtdBigintType*)aStack[1].value |
            *(mtdBigintType*)aStack[2].value;

        IDE_TEST_RAISE( sResult > MTD_BIGINT_MAXIMUM ||
                        sResult < MTD_BIGINT_MINIMUM,
                        ERR_VALUE_OVERFLOW );

        *(mtdBigintType*)aStack[0].value = sResult;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

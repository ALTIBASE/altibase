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
 * $Id: mtfBitXor.cpp 14592 2005-12-09 01:38:06Z jhseong $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfBitXor;

extern mtdModule mtdVarbit;

static mtcName mtfBitXorFunctionName[1] = {
    { NULL, 6, (void*)"BITXOR" }
};

static IDE_RC mtfBitXorEstimate( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 mtcCallBack* aCallBack );

mtfModule mtfBitXor = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfBitXorFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfBitXorEstimate
};

IDE_RC mtfBitXorCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfBitXorCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfBitXorEstimate( mtcNode*     aNode,
                          mtcTemplate* aTemplate,
                          mtcStack*    aStack,
                          SInt      /* aRemain */,
                          mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdVarbit,
        &mtdVarbit
    };

    SInt       sPrecision      = 0;
    SInt       sNode1Precision = 0;
    SInt       sNode2Precision = 0;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sNode1Precision = aStack[1].column->precision;
    sNode2Precision = aStack[2].column->precision;

    sPrecision = sNode1Precision > sNode2Precision ? 
                 sNode1Precision : sNode2Precision;

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
                                     & mtdVarbit,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfBitXorCalculate( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
    SInt sIterator;    
    SInt sFence;
    SInt sFence2;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( ( aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
        ( aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        if( aStack[1].column->precision > aStack[2].column->precision )
        {
            sFence = BIT_TO_BYTE( aStack[2].column->precision );

            for( sIterator = 0; sIterator < sFence; sIterator++ )
            {
                ((mtdBitType *)aStack[0].value)->value[sIterator] = 
                    ((mtdBitType *)aStack[1].value)->value[sIterator] ^ 
                    ((mtdBitType *)aStack[2].value)->value[sIterator];
            }

            sFence2 = BIT_TO_BYTE( aStack[1].column->precision );

            for( sIterator = sFence; sIterator < sFence2; sIterator++ )
            {
                ((mtdBitType *)aStack[0].value)->value[sIterator] =
                    ((mtdBitType *)aStack[1].value)->value[sIterator] ^ 0x00;
            }

            ((mtdBitType *)aStack[0].value)->length = aStack[1].column->precision;
        }
        else
        {
            sFence = BIT_TO_BYTE( aStack[1].column->precision );

            for( sIterator = 0; sIterator < sFence; sIterator++ )
            {
                ((mtdBitType *)aStack[0].value)->value[sIterator] = 
                    ((mtdBitType *)aStack[1].value)->value[sIterator] ^ 
                    ((mtdBitType *)aStack[2].value)->value[sIterator];
            }

            sFence2 = BIT_TO_BYTE( aStack[2].column->precision );

            for( sIterator = sFence; sIterator < sFence2; sIterator++ )
            {
                ((mtdBitType *)aStack[0].value)->value[sIterator] =
                    ((mtdBitType *)aStack[2].value)->value[sIterator] ^ 0x00;
            }

            ((mtdBitType *)aStack[0].value)->length = aStack[2].column->precision;
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

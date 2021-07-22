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
 * $Id: mtfOr.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfAnd;

extern mtfModule mtfOr;

extern mtxModule mtxOr; /* PROJ-2632 */

static mtcName mtfOrFunctionName[1] = {
    { NULL, 2, (void*)"OR" }
};

static IDE_RC mtfOrEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             mtcCallBack* aCallBack );

mtfModule mtfOr = {
    1|MTC_NODE_OPERATOR_OR|MTC_NODE_LOGICAL_CONDITION_TRUE|
        MTC_NODE_PRINT_FMT_INFIX_SP,
    ~0,
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfOrFunctionName,
    &mtfAnd,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfOrEstimate
};

IDE_RC mtfOrCalculate(     mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfOrCalculate,
    NULL,
    mtxOr.mCommon,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfOrEstimate( mtcNode*     aNode,
                      mtcTemplate* aTemplate,
                      mtcStack*    aStack,
                      SInt         /* aRemain */,
                      mtcCallBack* /* aCallBack*/ )
{
    extern mtdModule mtdBoolean;

    mtcNode* sNode;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    for( sNode  = aNode->arguments;
         sNode != NULL;
         sNode  = sNode->next )
    {
        IDE_TEST_RAISE( ( sNode->lflag & ( MTC_NODE_LOGICAL_CONDITION_MASK |
                                          MTC_NODE_COMPARISON_MASK        ) )
                        == ( MTC_NODE_LOGICAL_CONDITION_FALSE |
                             MTC_NODE_COMPARISON_FALSE        ),
                        ERR_CONVERSION_NOT_APPLICABLE );
    }

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
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

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfOrCalculate( mtcNode*     aNode,
                       mtcStack*    aStack,
                       SInt         aRemain,
                       void*,
                       mtcTemplate* aTemplate )
{
    mtdBooleanType sValue;
    mtcNode*       sNode;
    
    IDE_TEST_RAISE( aRemain < 2, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );
    aStack++;
    aRemain--;
    sValue = MTD_BOOLEAN_FALSE;
    
    for( sNode  = aNode->arguments;
         sValue != MTD_BOOLEAN_TRUE && sNode != NULL;
         sNode  = sNode->next )
    {
        IDE_TEST( aTemplate->rows[sNode->table].
                  execute[sNode->column].calculate(                     sNode,
                                                                       aStack,
                                                                      aRemain,
           aTemplate->rows[sNode->table].execute[sNode->column].calculateInfo,
                                                                    aTemplate )
                  != IDE_SUCCESS );
        sValue = mtf::orMatrix[sValue][*(mtdBooleanType*)aStack[0].value];
    }
    
    *(mtdBooleanType*)aStack[-1].value = sValue;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

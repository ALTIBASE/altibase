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
 * $Id: mtfNotExists.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfExists;

extern mtfModule mtfNotExists;

static mtcName mtfNotExistsFunctionName[1] = {
    { NULL, 9, (void*)"NOTEXISTS" }
};

static IDE_RC mtfNotExistsEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfNotExists = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_COMPARISON_TRUE|
        MTC_NODE_PRINT_FMT_MISC,
    ~(MTC_NODE_INDEX_MASK),
    0.49,  // TODO : default selectivity 
    mtfNotExistsFunctionName,
    &mtfExists,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNotExistsEstimate
};

IDE_RC mtfNotExistsCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNotExistsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNotExistsEstimate( mtcNode*     aNode,
                             mtcTemplate* aTemplate,
                             mtcStack*    aStack,
                             SInt         /* aRemain */,
                             mtcCallBack* /* aCallBack */ )
{
    extern mtdModule mtdBoolean;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    //IDE_TEST( mtdBoolean.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( aNode->arguments->lflag & MTC_NODE_OPERATOR_MASK )
                    != MTC_NODE_OPERATOR_SUBQUERY,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    /* PROJ-2448 Subquery caching */
    IDE_TEST( aTemplate->setCalcSubquery( aNode, aTemplate ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfNotExistsCalculate( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*,
                              mtcTemplate* aTemplate )
{
    mtcStack * sStack;
    SInt       sRemain;
    
    sStack  = aTemplate->stack;
    sRemain = aTemplate->stackRemain;

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*)( (UChar*) aTemplate->rows[aNode->table].row
                              + aStack->column->column.offset );

    /* PROJ-2448 Subquery caching */
    IDE_TEST( aTemplate->rows[aNode->arguments->table].execute[aNode->arguments->column].calculate(
                 aNode->arguments,
                 aStack,
                 aRemain,
                 aTemplate->rows[aNode->arguments->table].
                 execute[aNode->arguments->column].calculateInfo,
                 aTemplate )
             != IDE_SUCCESS);

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    aTemplate->stack       = sStack;
    aTemplate->stackRemain = sRemain;

    return IDE_FAILURE;
}

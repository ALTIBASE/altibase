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
 * $Id: mtvFloat2Char.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvFloat2Char;

extern mtdModule mtdChar;
extern mtdModule mtdFloat;

extern mtxModule mtxFromFloatTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Float2Char( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

mtvModule mtvFloat2Char = {
    &mtdChar,
    &mtdFloat,
    MTV_COST_DEFAULT | MTV_COST_GROUP_PENALTY | MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Float2Char,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxFromFloatTo.mGetExecute( mtdChar.id, mtdChar.id );

    //IDE_TEST( mtdChar.estimate( aStack[0].column, 1, 45, 0 )
    //          != IDE_SUCCESS );

    // PROJ-1365, fix BUG-12944
    // 계산된 결과값 또는 사용자에게 바로 입력받는 값은
    // precision이 40일 수 있으므로 38일 때 보다 2자리를 더 필요로 한다.
    // 최대 45자리에서 47자리로 수정함.
    // 1(부호) + 40(precision) + 1(.) + 1(E) + 1(exponent부호) + 3(exponent값)
    // = 47이 된다.
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdChar,
                                     1,
                                     47,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Float2Char( mtcNode*,
                                mtcStack*    aStack,
                                SInt,
                                void*,
                                mtcTemplate* )
{
    mtdNumericType* sFloat;
    mtdCharType*    sChar;
    UInt            sLength;

    sFloat   = (mtdNumericType*)aStack[1].value;
    sChar = (mtdCharType*)aStack[0].value;

    // PROJ-1365, fix BUG-12944
    // 계산된 결과값 또는 사용자에게 바로 입력받는 값은
    // precision이 40일 수 있으므로 38일 때 보다 2자리를 더 필요로 한다.
    // 최대 45자리에서 47자리로 수정함.
    // 1(부호) + 40(precision) + 1(.) + 1(E) + 1(exponent부호) + 3(exponent값)
    // = 47이 된다.
    IDE_TEST( mtv::float2String( sChar->value, 47, &sLength, sFloat )
              != IDE_SUCCESS );
    sChar->length = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

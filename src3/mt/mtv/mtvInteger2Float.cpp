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
 * $Id: mtvInteger2Float.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvInteger2Float;

extern mtdModule mtdFloat;
extern mtdModule mtdInteger;

extern mtxModule mtxFromIntegerTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Integer2Float( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

mtvModule mtvInteger2Float = {
    &mtdFloat,
    &mtdInteger,
    MTV_COST_DEFAULT,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Integer2Float,
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
        = mtxFromIntegerTo.mGetExecute( mtdFloat.id, mtdFloat.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     1,
                                     10, // int max value(2147483647)
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Integer2Float( mtcNode*,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* )
{
    mtdBigintType sBigint;

    if( *(mtdIntegerType*)aStack[1].value == MTD_INTEGER_NULL )
    {
        mtdFloat.null( aStack[0].column,
                       aStack[0].value );
    }
    else
    {
        sBigint = *(mtdIntegerType*)aStack[1].value;

        IDE_TEST( mtv::nativeN2Numeric( sBigint,
                                        (mtdNumericType*)aStack[0].value )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

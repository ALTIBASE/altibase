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
 * $Id: mtvBigint2Numeric.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvBigint2Numeric;

extern mtdModule mtdNumeric;
extern mtdModule mtdBigint;

extern mtxModule mtxFromBigintTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Bigint2Numeric( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

mtvModule mtvBigint2Numeric = {
    &mtdNumeric,
    &mtdBigint,
    MTV_COST_DEFAULT+1,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Bigint2Numeric,
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
    aStack[0].column =  aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxFromBigintTo.mGetExecute( mtdNumeric.id, mtdNumeric.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNumeric,
                                     1,
                                     19, // bigint max value(9223372036854775807)
                                     0 )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Bigint2Numeric( mtcNode*,
                                    mtcStack*    aStack,
                                    SInt,
                                    void*,
                                    mtcTemplate* )
{
    if( *(mtdBigintType*)aStack[1].value == MTD_BIGINT_NULL )
    {
        mtdNumeric.null( aStack[0].column,
                         aStack[0].value );
    }
    else
    {
        IDE_TEST( mtv::nativeN2Numeric( *(mtdBigintType*)aStack[1].value,
                                        (mtdNumericType*)aStack[0].value )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

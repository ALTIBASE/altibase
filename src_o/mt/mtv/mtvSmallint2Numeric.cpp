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
 * $Id: mtvSmallint2Numeric.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvSmallint2Numeric;

extern mtdModule mtdNumeric;
extern mtdModule mtdSmallint;

extern mtxModule mtxFromSmallintTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Smallint2Numeric( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

mtvModule mtvSmallint2Numeric = {
    &mtdNumeric,
    &mtdSmallint,
    MTV_COST_DEFAULT+1,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Smallint2Numeric,
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
        = mtxFromSmallintTo.mGetExecute( mtdNumeric.id, mtdNumeric.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNumeric,
                                     1,
                                     5,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Smallint2Numeric( mtcNode*,
                                      mtcStack*    aStack,
                                      SInt,
                                      void*,
                                      mtcTemplate* )
{
    mtdBigintType sBigint;

    if( *(mtdSmallintType*)aStack[1].value == MTD_SMALLINT_NULL )
    {
        mtdNumeric.null( aStack[0].column,
                         aStack[0].value );
    }
    else
    {
        sBigint  = *(mtdSmallintType*)aStack[1].value;

        IDE_TEST( mtv::nativeN2Numeric( sBigint,
                                        (mtdNumericType*)aStack[0].value )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

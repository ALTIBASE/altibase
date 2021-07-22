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
 * $Id: mtvBigint2Smallint.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvBigint2Smallint;

extern mtdModule mtdSmallint;
extern mtdModule mtdBigint;

extern mtxModule mtxFromBigintTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Bigint2Smallint( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule mtvBigint2Smallint = {
    &mtdSmallint,
    &mtdBigint,
    MTV_COST_DEFAULT|MTV_COST_ERROR_PENALTY * 2,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Bigint2Smallint,
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
        = mtxFromBigintTo.mGetExecute( mtdSmallint.id, mtdSmallint.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdSmallint,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Bigint2Smallint( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* )
{
    if( *(mtdBigintType*)aStack[1].value == MTD_BIGINT_NULL )
    {
        mtdSmallint.null( aStack[0].column,
                          aStack[0].value );
    }
    else
    {
        IDE_TEST_RAISE( ( *(mtdBigintType*)aStack[1].value > MTD_SMALLINT_MAXIMUM ) ||
                        ( *(mtdBigintType*)aStack[1].value < MTD_SMALLINT_MINIMUM ),
                        ERR_VALUE_OVERFLOW );
        *(mtdSmallintType*)aStack[0].value = *(mtdBigintType*)aStack[1].value;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

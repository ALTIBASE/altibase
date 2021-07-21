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
 * $Id: mtvSmallint2Real.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvSmallint2Real;

extern mtdModule mtdReal;
extern mtdModule mtdSmallint;

extern mtxModule mtxFromSmallintTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Smallint2Real( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

mtvModule mtvSmallint2Real = {
    &mtdReal,
    &mtdSmallint,
    MTV_COST_DEFAULT | MTV_COST_ERROR_PENALTY | MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Smallint2Real,
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
        = mtxFromSmallintTo.mGetExecute( mtdReal.id, mtdReal.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdReal,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Smallint2Real( mtcNode*,
                                   mtcStack*    aStack,
                                   SInt,
                                   void*,
                                   mtcTemplate* )
{
    mtdDoubleType sDouble;

    if( *(mtdSmallintType*)aStack[1].value == MTD_SMALLINT_NULL )
    {
        mtdReal.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        sDouble = *(mtdSmallintType*)aStack[1].value;
        *(mtdRealType*)aStack[0].value = sDouble;
        IDE_TEST_RAISE( mtdReal.isNull( aStack[0].column,
                                        aStack[0].value )
                        == ID_TRUE, ERR_VALUE_OVERFLOW );

        // To fix BUG-12281
        // underflow °Ë»ç
        if( ( idlOS::fabs( sDouble ) < MTD_REAL_MINIMUM ) &&
            ( *(mtdRealType*)aStack[0].value != 0 ) )
        {
            *(mtdRealType*)aStack[0].value = 0;
        }
        else
        {
            // Nothing to do
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_VALUE_OVERFLOW ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

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
 * $Id: mtvDouble2Float.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvDouble2Float;

extern mtdModule mtdFloat;
extern mtdModule mtdDouble;

extern mtxModule mtxFromDoubleTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Double2Float( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvDouble2Float = {
    &mtdFloat,
    &mtdDouble,
    MTV_COST_DEFAULT|MTV_COST_ERROR_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Double2Float,
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
        = mtxFromDoubleTo.mGetExecute( mtdFloat.id, mtdFloat.id );

    //IDE_TEST( mtdFloat.estimate( aStack[0].column, 1, 38, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdFloat,
                                     1,
                                     38,
                                     0 )
              != IDE_SUCCESS );

    
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Double2Float( mtcNode*,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* )
{
    mtdNumericType* sFloat;
    mtdDoubleType   sDouble;

    if( mtdDouble.isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE )
    {
        mtdFloat.null( aStack[0].column,
                       aStack[0].value );
    }
    else
    {
        sFloat  = (mtdNumericType*)aStack[0].value;
        sDouble = *(mtdDoubleType*)aStack[1].value;
        
        IDE_TEST( mtc::makeNumeric( sFloat, sDouble )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

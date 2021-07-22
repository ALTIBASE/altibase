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
 * $Id: mtvNumeric2Double.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>
#include <ida.h>

#include <mtdTypes.h>

extern mtvModule mtvNumeric2Double;

extern mtdModule mtdDouble;
extern mtdModule mtdNumeric;

extern mtxModule mtxFromNumericTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Numeric2Double(  mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule mtvNumeric2Double = {
    &mtdDouble,
    &mtdNumeric,
    MTV_COST_DEFAULT|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Numeric2Double,
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
        = mtxFromNumericTo.mGetExecute( mtdDouble.id, mtdDouble.id );

    //IDE_TEST( mtdDouble.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Numeric2Double( mtcNode*,
                                    mtcStack*    aStack,
                                    SInt,
                                    void*,
                                    mtcTemplate* )
{
    mtdNumericType* sNumeric;
    mtdDoubleType*  sDouble;
    
    if( mtdNumeric.isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE )
    {
        mtdDouble.null( aStack[0].column,
                        aStack[0].value );
    }
    else
    {
        sNumeric = (mtdNumericType*)aStack[1].value;
        sDouble  = (mtdDoubleType*)aStack[0].value;
        
        mtc::numeric2Double( sDouble, sNumeric );
        
        IDE_TEST_RAISE( mtdDouble.isNull( aStack[0].column,
                                          aStack[0].value )
                        == ID_TRUE, ERR_VALUE_OVERFLOW );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

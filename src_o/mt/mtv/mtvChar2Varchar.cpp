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
 * $Id: mtvChar2Varchar.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvChar2Varchar;

extern mtdModule mtdVarchar;
extern mtdModule mtdChar;

extern mtxModule mtxFromCharTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Char2Varchar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvChar2Varchar = {
    &mtdVarchar,
    &mtdChar,
    MTV_COST_DEFAULT,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Char2Varchar,
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
        = mtxFromCharTo.mGetExecute( mtdVarchar.id, mtdVarchar.id );

    /*
    IDE_TEST( mtdVarchar.estimate( aStack[0].column,
                                   1,
                                   aStack[1].column->precision,
                                   0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Char2Varchar( mtcNode*,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* )
{
    ((mtdCharType*)aStack[0].value)->length =
                                       ((mtdCharType*)aStack[1].value)->length;
    
    idlOS::memcpy( ((mtdCharType*)aStack[0].value)->value,
                   ((mtdCharType*)aStack[1].value)->value,
                   ((mtdCharType*)aStack[1].value)->length );
    
    return IDE_SUCCESS;
}
 

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
 * $Id: mtvNvarchar2Nchar.cpp 26126 2008-05-23 07:21:56Z copyrei $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvNvarchar2Nchar;

extern mtdModule mtdNchar;
extern mtdModule mtdNvarchar;

extern mtxModule mtxFromNvarcharTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Nvarchar2Nchar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvNvarchar2Nchar = {
    &mtdNchar,
    &mtdNvarchar,
    MTV_COST_DEFAULT|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Nvarchar2Nchar,
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
        = mtxFromNvarcharTo.mGetExecute( mtdNchar.id, mtdNchar.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNchar,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Nvarchar2Nchar( mtcNode*,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* )
{
    ((mtdNcharType*)aStack[0].value)->length =
                                       ((mtdNcharType*)aStack[1].value)->length;
    
    idlOS::memcpy( ((mtdNcharType*)aStack[0].value)->value,
                   ((mtdNcharType*)aStack[1].value)->value,
                   (((mtdNcharType*)aStack[1].value)->length) );
    
    return IDE_SUCCESS;
}
 

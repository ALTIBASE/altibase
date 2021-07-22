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
 * $Id: mtvNchar2Char.cpp 26126 2008-05-23 07:21:56Z copyrei $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtvModule mtvNchar2Char;

extern mtdModule mtdChar;
extern mtdModule mtdNchar;

extern mtxModule mtxFromNcharTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Nchar2Char( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate );

mtvModule mtvNchar2Char = {
    &mtdChar,
    &mtdNchar,
    MTV_COST_DEFAULT|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Nchar2Char,
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
        = mtxFromNcharTo.mGetExecute( mtdChar.id, mtdChar.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdChar,
                                     1,
                                     mtl::mDBCharSet->maxPrecision( aStack[1].column->precision ),
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Nchar2Char( mtcNode*,
                                mtcStack*    aStack,
                                SInt,
                                void*,
                                mtcTemplate* )
{
    mtdCharType*     sResult;
    mtdNcharType*    sSource;

    sSource  = (mtdNcharType*)aStack[1].value;
    sResult  = (mtdCharType*)aStack[0].value;

    IDE_TEST( mtdNcharInterface::toChar( aStack,
                                (const mtlModule *) mtl::mNationalCharSet,
                                (const mtlModule *) mtl::mDBCharSet,
                                sSource,
                                sResult )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

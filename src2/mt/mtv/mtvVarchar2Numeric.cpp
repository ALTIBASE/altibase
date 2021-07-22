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
 * $Id: mtvVarchar2Numeric.cpp 47933 2011-06-30 02:01:37Z et16 $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvVarchar2Numeric;

extern mtdModule mtdNumeric;
extern mtdModule mtdVarchar;

extern mtxModule mtxFromVarcharTo; /* BUG-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varchar2Numeric( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule mtvVarchar2Numeric = {
    &mtdNumeric,
    &mtdVarchar,
    (MTV_COST_DEFAULT | MTV_COST_GROUP_PENALTY)+1,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varchar2Numeric,
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
        = mtxFromVarcharTo.mGetExecute( mtdNumeric.id, mtdNumeric.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNumeric,
                                     0,   // arguments
                                     0,   // precision
                                     0 )  // scale
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varchar2Numeric( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* )
{
    mtdCharType*    sVarchar;
    mtdNumericType* sNumeric;

    if( mtdVarchar.isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE )
    {
        mtdNumeric.null( aStack[0].column,
                         aStack[0].value );
    }
    else
    {
        sVarchar = (mtdCharType*)aStack[1].value;
        sNumeric = (mtdNumericType*)aStack[0].value;
        IDE_TEST( mtc::makeNumeric( sNumeric,
                                    MTD_FLOAT_MANTISSA_MAXIMUM,
                                    sVarchar->value,
                                    sVarchar->length )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

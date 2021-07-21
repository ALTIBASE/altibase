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
 * $Id: mtvVarbyte2Varchar.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvVarbyte2Varchar;

extern mtdModule mtdVarbyte;
extern mtdModule mtdVarchar;

extern mtxModule mtxFromVarbyteTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varbyte2Varchar( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule mtvVarbyte2Varchar = {
    &mtdVarchar,
    &mtdVarbyte,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varbyte2Varchar,
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
        = mtxFromVarbyteTo.mGetExecute( mtdVarchar.id, mtdVarchar.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     1,
                                     aStack[1].column->precision * 2,
                                     0 )
              != IDE_SUCCESS );

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varbyte2Varchar( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* )
{
    mtdByteType*      sVarbyte;
    mtdCharType*      sVarchar;
    SChar             sVarcharValue;
    SChar             sByteValue;
    SInt              sByteIterator;
    SInt              sVarcharIterator;

    sVarchar = (mtdCharType*)aStack[0].value;
    sVarbyte = (mtdByteType*)aStack[1].value;
    
    if( mtdVarbyte.isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE )
    {
        mtdVarchar.null( aStack[0].column,
                         aStack[0].value );
    }
    else
    {
        for( sByteIterator = 0, sVarcharIterator = 0;
             sByteIterator < sVarbyte->length;
             sByteIterator++, sVarcharIterator += 2)
        {
            sByteValue = sVarbyte->value[sByteIterator];

            sVarcharValue = (sByteValue & 0xF0) >> 4;
            sVarchar->value[sVarcharIterator+0] =
                (sVarcharValue < 10) ? (sVarcharValue + '0') : (sVarcharValue + 'A' - 10);
            
            sVarcharValue = (sByteValue & 0x0F);
            sVarchar->value[sVarcharIterator+1] =
                (sVarcharValue < 10) ? (sVarcharValue + '0') : (sVarcharValue + 'A' - 10);
        }
        
        sVarchar->length = sVarcharIterator;
    }
    
    return IDE_SUCCESS;
}
 

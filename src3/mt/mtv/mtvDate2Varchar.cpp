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
 * $Id: mtvDate2Varchar.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtvModule mtvDate2Varchar;

extern mtdModule mtdVarchar;
extern mtdModule mtdDate;

extern mtxModule mtxFromDateTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Date2Varchar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvDate2Varchar = {
    &mtdVarchar,
    &mtdDate,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Date2Varchar,
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
        = mtxFromDateTo.mGetExecute( mtdVarchar.id, mtdVarchar.id );

    /*
    IDE_TEST( mtdVarchar.estimate( aStack[0].column,
                                   1,
                                   MTC_TO_CHAR_MAX_PRECISION,
                                   0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarchar,
                                     1,
                                     MTC_TO_CHAR_MAX_PRECISION,
                                     0 )
              != IDE_SUCCESS );

    
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Date2Varchar( mtcNode*,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtdDateType* sDate;
    mtdCharType* sVarchar;
    UInt         sLength;
    SInt         sFormatLen;

    IDE_ASSERT( aTemplate != NULL );
    IDE_ASSERT( aTemplate->dateFormat != NULL );

    if( mtdDate.isNull( aStack[1].column,
                        aStack[1].value ) == ID_TRUE )
    {
        mtdVarchar.null( aStack[0].column,
                         aStack[0].value );
    }
    else
    {
        sDate      = (mtdDateType*)aStack[1].value;
        sVarchar   = (mtdCharType*)aStack[0].value;
        sLength    = 0;
        sFormatLen = idlOS::strlen( aTemplate->dateFormat );
 
        IDE_TEST( mtdDateInterface::toChar( sDate,
                                            sVarchar->value,
                                            &sLength,
                                            aStack[0].column->precision,
                                            (UChar*)aTemplate->dateFormat,
                                            sFormatLen )
                  != IDE_SUCCESS );
        
        // PROJ-1436
        // dateFormat을 참조했음을 표시한다.
        aTemplate->dateFormatRef = ID_TRUE;
        
        sVarchar->length = (UShort)sLength;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

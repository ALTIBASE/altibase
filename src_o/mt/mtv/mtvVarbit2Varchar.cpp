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
 * $Id: mtvVarbit2Varchar.cpp 13146 2005-08-12 09:20:06Z leekmo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvVarbit2Varchar;

extern mtdModule mtdVarbit;
extern mtdModule mtdVarchar;
extern mtdModule mtdChar;

extern mtxModule mtxFromVarbitTo; /* PROJ-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varbit2Varchar( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvVarbit2Varchar = {
    &mtdVarchar,
    &mtdVarbit,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varbit2Varchar,
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
        = mtxFromVarbitTo.mGetExecute( mtdVarchar.id, mtdVarchar.id );

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

IDE_RC mtvCalculate_Varbit2Varchar( mtcNode*,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* )
{
    UInt  sBitLength  = 0;
    UInt  sByteLength = 0;
    UInt  sBitCnt     = 0;
    UInt  sIndex      = 0;
    UInt  sByteIndex  = 0;
    UInt  sIterator   = 0;
    UChar sChr        = 0;
    //UChar sIsEmpty    = 1;

    mtdBitType* sVarbit;
    
    sBitLength = ((mtdBitType*)aStack[1].value)->length;
    sByteLength = BIT_TO_BYTE( ((mtdBitType*)aStack[1].value)->length);

    IDE_TEST_RAISE( sBitLength > MTD_CHAR_PRECISION_MAXIMUM, 
                    ERR_INVALID_LENGTH );
    
    if( mtdVarbit.isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE )
    {
        mtdVarchar.null( aStack[0].column,
                         aStack[0].value );
    }
    else
    {
        sVarbit = (mtdBitType*)aStack[1].value;

        for( sByteIndex = 0; sByteIndex < sByteLength; sByteIndex++ )
        {
            for( sIterator = 0; sIterator < 8; sIterator++ )
            {
                if( sBitCnt > sBitLength - 1 )
                {
                    break;
                }

                sChr = ( ( sVarbit->value[sByteIndex] << sIterator) >> 7) & 0x1;

                /*
                * 00001111의 경우 앞의 0000도 같이 변환이 되어야 한다. 
                *
                * insert into tab_bit values ( bit'11111111' );
                * insert into tab_bit values ( bit'00001111' );
                * insert into tab_bit values ( bit'11110000' );
                * select to_char( a ) from tab_bit where a like '1111%';
                * select to_char( a ) from tab_bit where a like '11110%';
                * select to_char( a ) from tab_bit where a like '0%';
                *
                */
                //if( sChr != 0 || sIsEmpty == 0 )
                {
                    //sIsEmpty = 0;
                    ((mtdCharType*)aStack[0].value)->value[sIndex] = sChr + '0';
                    sIndex++;
                }
                sBitCnt++;
            }
        }
        ((mtdCharType*)aStack[0].value)->length = sIndex;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

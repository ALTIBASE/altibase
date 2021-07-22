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
 * $Id: mtvVarchar2Varbyte.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvVarchar2Varbyte;

extern mtdModule mtdVarbyte;
extern mtdModule mtdVarchar;

extern mtxModule mtxFromVarcharTo; /* BUG-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varchar2Varbyte( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate );

mtvModule mtvVarchar2Varbyte = {
    &mtdVarbyte,
    &mtdVarchar,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varchar2Varbyte,
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
        = mtxFromVarcharTo.mGetExecute( mtdVarbyte.id, mtdVarbyte.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdVarbyte,
                                     1,
                                     ( aStack[1].column->precision + 1 ) / 2,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varchar2Varbyte( mtcNode*,
                                     mtcStack*    aStack,
                                     SInt,
                                     void*,
                                     mtcTemplate* )
{
    static const UChar sHex[256] = {
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
         0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 10, 11, 12, 13, 14, 15, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99
    };
    mtdByteType*      sVarbytes;
    mtdCharType*      sVarchar;
    SInt              sVarcharIterator;
    SInt              sVarcharFence;
    SInt              sBytesIterator;
    
    sVarbytes = (mtdByteType*)aStack[0].value;
    sVarchar  = (mtdCharType*)aStack[1].value;
    
    if ( sVarchar->length == 0 )
    {
        sVarbytes->length = 0;
    }
    else
    {
        sVarcharFence = ( sVarchar->length / 2 ) * 2;
        for ( sVarcharIterator = 0, sBytesIterator = 0;
              sVarcharIterator < sVarcharFence;
              sVarcharIterator += 2, sBytesIterator++ )
        {
            IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator+0]] > 15,
                            ERR_INVALID_LITERAL );
            IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator+1]] > 15,
                            ERR_INVALID_LITERAL );
            
            sVarbytes->value[sBytesIterator] =
                               sHex[sVarchar->value[sVarcharIterator+0]] * 16
                                  + sHex[sVarchar->value[sVarcharIterator+1]] ;
        }
        if ( ( sVarchar->length % 2 ) != 0 )
        {
           IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator]] > 15,
                           ERR_INVALID_LITERAL );
            sVarbytes->value[sBytesIterator] =
                                  sHex[sVarchar->value[sVarcharIterator]] * 16;
            sBytesIterator++;
        }
        sVarbytes->length = sBytesIterator;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

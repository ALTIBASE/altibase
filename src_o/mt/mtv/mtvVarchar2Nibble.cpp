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
 * $Id: mtvVarchar2Nibble.cpp 13146 2005-08-12 09:20:06Z sungminee $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvVarchar2Nibble;

extern mtdModule mtdNibble;
extern mtdModule mtdVarchar;

extern mtxModule mtxFromVarcharTo; /* BUG-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varchar2Nibble( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

mtvModule mtvVarchar2Nibble = {
    &mtdNibble,
    &mtdVarchar,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varchar2Nibble,
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
        = mtxFromVarcharTo.mGetExecute( mtdNibble.id, mtdNibble.id );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdNibble,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varchar2Nibble( mtcNode*,
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
    //mtcColumn*        sColumn;
    mtdNibbleType*    sNibble;
    mtdCharType*      sVarchar;
    SInt              sVarcharIterator;
    SInt              sNibbleIterator;
    SInt              sVarcharFence;
    
    //sColumn  = aStack[0].column;
    sNibble  = (mtdNibbleType*)aStack[0].value;
    sVarchar = (mtdCharType*)aStack[1].value;


    if( mtdVarchar.isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE )
    {
        mtdNibble.null( aStack[0].column,
                        aStack[0].value );
    }
    else
    {
        sVarcharFence = ( sVarchar->length / 2 ) * 2;
        
        for( sVarcharIterator = 0, sNibbleIterator = 0;
             sVarcharIterator < sVarcharFence;
             sVarcharIterator += 2, sNibbleIterator++ )
        {
            IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator+0]] > 15,
                            ERR_INVALID_LITERAL );
            IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator+1]] > 15,
                            ERR_INVALID_LITERAL );
            
            sNibble->value[sNibbleIterator] =
                sHex[sVarchar->value[sVarcharIterator+0]] * 16
                + sHex[sVarchar->value[sVarcharIterator+1]] ;
        }
        
        if( ( sVarchar->length % 2 ) != 0 )
        {
           IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator]] > 15,
                           ERR_INVALID_LITERAL );
            sNibble->value[sNibbleIterator] =
                sHex[sVarchar->value[sVarcharIterator]] * 16;
        }
        else
        {
            // Nothing to do.
        }
        
        sNibble->length = sVarchar->length;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

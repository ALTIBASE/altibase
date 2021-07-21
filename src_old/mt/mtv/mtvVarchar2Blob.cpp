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
 * $Id: mtvVarchar2Blob.cpp 13146 2005-08-12 09:20:06Z leekmo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvVarchar2Blob;

extern mtdModule mtdBlob;
extern mtdModule mtdVarchar;

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varchar2Blob( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvVarchar2Blob = {
    &mtdBlob,
    &mtdVarchar,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varchar2Blob,
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

    /*
    IDE_TEST( mtdBlob.estimate( aStack[0].column,
                                1,
                                ( aStack[1].column->precision + 1 ) / 2,
                                0 )
              != IDE_SUCCESS );
    */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBlob,
                                     1,
                                     ( aStack[1].column->precision + 1 ) / 2,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varchar2Blob( mtcNode*,
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
    mtdBlobType*      sBlob;
    mtdCharType*      sVarchar;
    SInt              sVarcharIterator;
    SInt              sVarcharFence;
    SInt              sBlobIterator;
    
    sBlob    = (mtdBlobType*)aStack[0].value;
    sVarchar = (mtdCharType*)aStack[1].value;
    
    if( sVarchar->length == 0 )
    {
        mtdBlob.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
        if ( ( sVarchar->length % 2 ) != 0 )
        {
            IDE_TEST_RAISE( aStack[0].column->precision < ((SInt)(sVarchar->length / 2) + 1),
                            ERR_CONVERT );
        }
        else
        {
            IDE_TEST_RAISE( aStack[0].column->precision < (SInt)(sVarchar->length / 2),
                            ERR_CONVERT );
        }

        sVarcharFence = ( sVarchar->length / 2 ) * 2;
        for( sVarcharIterator = 0, sBlobIterator = 0;
             sVarcharIterator < sVarcharFence;
             sVarcharIterator += 2, sBlobIterator++ )
        {
            IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator+0]] > 15,
                            ERR_INVALID_LITERAL );
            IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator+1]] > 15,
                            ERR_INVALID_LITERAL );
            
            sBlob->value[sBlobIterator] =
                sHex[sVarchar->value[sVarcharIterator+0]] * 16
                + sHex[sVarchar->value[sVarcharIterator+1]] ;
        }
        if( ( sVarchar->length % 2 ) != 0 )
        {
           IDE_TEST_RAISE( sHex[sVarchar->value[sVarcharIterator]] > 15,
                           ERR_INVALID_LITERAL );
            sBlob->value[sBlobIterator] =
                sHex[sVarchar->value[sVarcharIterator]] * 16;
            sBlobIterator++;
        }
        sBlob->length = sBlobIterator;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    }
    IDE_EXCEPTION( ERR_CONVERT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

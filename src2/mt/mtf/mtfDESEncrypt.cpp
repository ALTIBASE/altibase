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
 * $Id: mtfDESEncrypt.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idsDES.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfDESEncrypt;

extern mtdModule mtdVarchar;

static mtcName mtfDESEncryptFunctionName[1] = {
    { NULL, 10, (void*)"DESENCRYPT" }
};

static IDE_RC mtfDESEncryptEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

mtfModule mtfDESEncrypt = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDESEncryptFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDESEncryptEstimate
};

static IDE_RC mtfDESEncryptCalculate( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDESEncryptCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDESEncryptEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt      /* aRemain */,
                              mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &mtdVarchar;
    sModules[1] = &mtdVarchar;

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdVarchar,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDESEncryptCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : DESEncrypt Calculate
 *
 * Implementation :
 *     DES + CBC(Cipher Block Chaining)
 *
 *     C  : Cipher Text Block
 *     P  : Plain Text Block
 *     E  : Encryption (DES algorithm)
 *     k  : key
 *     i  : 8 byte Block order
 *
 *     C(i) = Ek( P(i) XOR C(i-1) )
 *
 *     reference : APPLIED CRYPTOGRAPHY, by Bruce Schneier (Book)
 *
 ***********************************************************************/
    
    mtdCharType*     sEncryptValue;
    mtdCharType*     sKey;
    mtdCharType*     sData;
    idsDES           sDes;
    SInt             sBlockCount;
    SInt             i;
    UChar            sCipherBlock[8];

    idlOS::memset( sCipherBlock, 0, 8 );
        
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        ( aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value )== ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sData = (mtdCharType*)aStack[1].value;
        sKey = (mtdCharType*)aStack[2].value;
        sEncryptValue = (mtdCharType*)aStack[0].value;

        IDE_TEST_RAISE(
            ( (sData->length % 8) != 0 ) ||
            ( sKey->length < 8 ), ERR_INVALID_LENGTH );

        sBlockCount = sData->length / 8;
                
        sDes.setkey( (UChar*)sKey->value, IDS_ENCODE );

        for( i = 0; i < sBlockCount; i++ )
        {
            sDes.blockXOR( sCipherBlock, (UChar*)sData->value + (i * 8) );
            
            sDes.des( sCipherBlock,
                      (UChar*)sEncryptValue->value + (i * 8) );
            idlOS::memcpy( sCipherBlock, (UChar*)sEncryptValue->value + (i * 8), 8 );
            
        }
        sEncryptValue->length = sData->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {    
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

 

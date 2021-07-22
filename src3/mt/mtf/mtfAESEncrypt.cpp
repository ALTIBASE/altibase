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
 * $Id: mtfAESEncrypt.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <idsAES.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfAESEncrypt;

extern mtdModule mtdVarchar;

static mtcName mtfAESEncryptFunctionName[1] = {
    { NULL, 10, (void*)"AESENCRYPT" }
};

static IDE_RC mtfAESEncryptEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

mtfModule mtfAESEncrypt = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfAESEncryptFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfAESEncryptEstimate
};

static IDE_RC mtfAESEncryptCalculate( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfAESEncryptCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAESEncryptEstimate( mtcNode*     aNode,
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

IDE_RC mtfAESEncryptCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : AESEncrypt Calculate
 *
 * Implementation :
 *
 ***********************************************************************/
    
    mtdCharType*     sEncryptValue;
    mtdCharType*     sKey;
    mtdCharType*     sData;

    SInt             sBlockCount;
    SInt             sBits = 128;
    SInt             i;
    
    idsAESCTX        sCtx;
    UChar            sKeyBuffer[32] = {0,};
    UChar            sCipherBlock[16] = {0,};
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( ( aStack[1].column->module->isNull( aStack[1].column,
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
        sKey =  (mtdCharType*)aStack[2].value;
        sEncryptValue = (mtdCharType*)aStack[0].value;

        IDE_TEST_RAISE(
            ( (sData->length % 16) != 0 ) ||
            ( sKey->length < 16 ), ERR_INVALID_LENGTH );

        sBlockCount = sData->length / 16;
                
        if ( sKey->length < 24 )
        {
            sBits = 128;
        }
        else if ( sKey->length < 32 )
        {
            sBits = 192;
        }
        else
        {
            sBits = 256;
        }
        
        if ( sKey->length < 32 )
        {
            idlOS::memcpy( (void*)sKeyBuffer, sKey->value, sKey->length );
        }
        else
        {
            idlOS::memcpy( (void*)sKeyBuffer, sKey->value, 32 );
        }
        
        idsAES::setKey( &sCtx,
                        (UChar*)sKeyBuffer,
                        sBits,
                        IDS_AES_ENCRYPT );

        for ( i = 0; i < sBlockCount; i++ )
        {
            idsAES::blockXOR( sCipherBlock, (UChar*)sData->value + ( i * 16 ) );
            idsAES::encrypt( &sCtx,
                             sCipherBlock,
                             (UChar*)sEncryptValue->value + ( i * 16 ) );
            idlOS::memcpy( sCipherBlock,
                           (UChar*)sEncryptValue->value + ( i * 16 ),
                            16 );
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

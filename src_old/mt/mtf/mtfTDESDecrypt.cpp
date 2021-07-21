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
 * $Id$
 **********************************************************************/

#include <idsDES.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfTDESDecrypt;

extern mtdModule mtdVarchar;
extern mtdModule mtdSmallint;

static mtcName mtfTDESDecryptFunctionName[2] = {
    { mtfTDESDecryptFunctionName + 1, 11, (void*)"TDESDECRYPT" },
    { NULL,                           17, (void*)"TRIPLE_DESDECRYPT" }
};

static IDE_RC mtfTDESDecryptEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

mtfModule mtfTDESDecrypt = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTDESDecryptFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTDESDecryptEstimate
};

static IDE_RC mtfTDESDecryptCalculate( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTDESDecryptCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTDESDecryptEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt      /* aRemain */,
                               mtcCallBack* aCallBack )
{
    const mtdModule* sModules[4];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ) ||
                    ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 4 ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    sModules[0] = &mtdVarchar;
    sModules[1] = &mtdVarchar;
    sModules[2] = &mtdSmallint;
    sModules[3] = &mtdVarchar;

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

IDE_RC mtfTDESDecryptCalculate( mtcNode*     aNode,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                void*        aInfo,
                                mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : TDESDecrypt Calculate
 *
 *     TDESDECRYPT( cipher_text, key, keying_option, initial_vector )
 *
 *     keying_option 0 : Key1 and Key2 are independent, and Key3 = Key1.
 *                   1 : All three keys are independent.
 *
 *     ex) TDESDECRYPT( 'XXXXXXXX', 'password', 0, '0123456789ABCDEF' )
 *
 * Implementation :
 *     DES + CBC(Cipher Block Chaning)
 *
 *     C : Cipher Text Block
 *     P : Plain Text Block
 *     D : Decryption (DES algorithm)
 *     k  : key
 *     i  : 8 byte Block order
 *
 *     P(i) = C(i-1) XOR Dk( C(i) )
 *     
 *     reference : APPLIED CRYPTOGRAPHY, by Bruce Schneier (Book)
 *
 ***********************************************************************/
    
    mtcNode         * sNode;
    mtdCharType     * sDecryptValue;
    mtdCharType     * sKey;
    mtdCharType     * sData;
    mtdCharType     * sIV;
    mtdSmallintType * sWhich;
    idsDES            sDes;
    SInt              sBlockCount;
    UChar             sIVBlock[8];
    UChar             sPlainBlock1[8];
    UChar             sPlainBlock2[8];
    UChar           * sKey1;
    UChar           * sKey2;
    UChar           * sKey3;
    SInt              i;
      
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( ( aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
        ( aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sDecryptValue = (mtdCharType*)aStack[0].value;
        sData = (mtdCharType*)aStack[1].value;
        sKey = (mtdCharType*)aStack[2].value;

        IDE_TEST_RAISE( ( (sData->length % 8) != 0 ) || ( sKey->length < 16 ),
                        ERR_INVALID_LENGTH );

        sBlockCount = sData->length / 8;

        // default keying option
        sKey1 = (UChar*)sKey->value;
        sKey2 = (UChar*)sKey->value + 8;
        sKey3 = (UChar*)sKey->value;

        // default initial vector
        idlOS::memset( sIVBlock, 0, 8 );

        sNode = aNode->arguments;  // plain text
        sNode = sNode->next;       // key
        sNode = sNode->next;       // keying option
        
        if ( sNode != NULL )
        {
            if ( aStack[3].column->module->isNull( aStack[3].column,
                                                   aStack[3].value ) == ID_TRUE )
            {
                // default keying option
            }
            else
            {
                sWhich = (mtdSmallintType*)aStack[3].value;

                if ( *sWhich == (mtdSmallintType)0 )
                {
                    // default keying option
                }
                else if ( *sWhich == (mtdSmallintType)1 )
                {
                    IDE_TEST_RAISE( sKey->length < 24, ERR_INVALID_LENGTH );
                
                    sKey1 = (UChar*)sKey->value;
                    sKey2 = (UChar*)sKey->value + 8;
                    sKey3 = (UChar*)sKey->value + 16;
                }
                else
                {
                    IDE_RAISE( ERR_INVALID_FUNCTION_ARGUMENT );
                }
            }

            sNode = sNode->next;   // initial vector

            if ( sNode != NULL )
            {
                if ( aStack[4].column->module->isNull( aStack[4].column,
                                                       aStack[4].value ) == ID_TRUE )
                {
                    // default initial vector
                }
                else
                {
                    sIV = (mtdCharType*)aStack[4].value;
                
                    IDE_TEST_RAISE( sIV->length < 8, ERR_INVALID_LENGTH );
                
                    idlOS::memcpy( sIVBlock, sIV->value, 8 );
                }
            }
            else
            {
                // default initial vector
            }
        }
        else
        {
            // default keying option
            // default initial vector
        }
        
        sDes.setkey( sKey3, IDS_DECODE );
        sDes.des( (UChar*)sData->value, sPlainBlock1 );

        sDes.setkey( sKey2, IDS_ENCODE );
        sDes.des( sPlainBlock1, sPlainBlock2 );
        
        sDes.setkey( sKey1, IDS_DECODE );
        sDes.des( sPlainBlock2, (UChar*)sDecryptValue->value );
        
        sDes.blockXOR( (UChar*)sDecryptValue->value,
                       sIVBlock );
        
        for( i = 1; i < sBlockCount; i++ )
        {
            sDes.setkey( sKey3, IDS_DECODE );
            sDes.des( (UChar*)sData->value + (i * 8), sPlainBlock1 );

            sDes.setkey( sKey2, IDS_ENCODE );
            sDes.des( sPlainBlock1, sPlainBlock2 );
            
            sDes.setkey( sKey1, IDS_DECODE );
            sDes.des( sPlainBlock2, (UChar*)sDecryptValue->value + (i * 8) );
            
            sDes.blockXOR( (UChar*)sDecryptValue->value + (i * 8),
                           (UChar*)sData->value + ((i-1) * 8) );
        }
        
        sDecryptValue->length = sData->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LENGTH );
    {    
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LENGTH));
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

 

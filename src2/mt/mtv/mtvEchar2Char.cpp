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
 * $Id: mtvChar2Varchar.cpp 29282 2008-11-13 08:03:38Z mhjeong $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvEchar2Char;

extern mtdModule mtdChar;
extern mtdModule mtdEchar;

static IDE_RC mtvEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

IDE_RC mtvCalculate_Echar2Char( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                void        * aInfo,
                                mtcTemplate * aTemplate );

mtvModule mtvEchar2Char = {
    &mtdChar,
    &mtdEchar,
    MTV_COST_DEFAULT|MTV_COST_SMALL_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Echar2Char,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtvEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          /* aRemain */,
                           mtcCallBack * /* aCallBack */ )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdChar,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Echar2Char( mtcNode     * aNode,
                                mtcStack    * aStack,
                                SInt          /* aRemain */,
                                void        * /* aInfo */,
                                mtcTemplate * aTemplate )
{
    mtdCharType    * sCharValue;
    mtdEcharType   * sEcharValue;
    mtcColumn      * sEncColumn;
    mtcEncryptInfo   sDecryptInfo;
    UShort           sPlainLength;
    UChar          * sPlain;
    UChar            sDecryptedBuf[MTD_ECHAR_DECRYPT_BUFFER_SIZE];
    
    sCharValue   = (mtdCharType*)aStack[0].value;    
    sEcharValue  = (mtdEcharType*)aStack[1].value;
    sPlain       = sDecryptedBuf;
    
    if ( aNode != NULL )
    {
        sEncColumn = aTemplate->rows[aNode->baseTable].columns
            + aNode->baseColumn;

        IDE_ASSERT_MSG( sEncColumn->module->id == MTD_ECHAR_ID,
                        "sEncColumn->module->id : %"ID_UINT32_FMT"\n",
                        sEncColumn->module->id );
    
        if ( sEncColumn->mColumnAttr.mEncAttr.mPolicy[0] != '\0' )
        {
            if( sEcharValue->mCipherLength > 0 )
            {
                IDE_TEST( aTemplate->getDecryptInfo( aTemplate,
                                                     aNode->baseTable,
                                                     aNode->baseColumn,
                                                     & sDecryptInfo )
                          != IDE_SUCCESS );
                
                IDE_TEST( aTemplate->decrypt( & sDecryptInfo,
                                              sEncColumn->mColumnAttr.mEncAttr.mPolicy,
                                              sEcharValue->mValue,
                                              sEcharValue->mCipherLength,
                                              sPlain,
                                              & sPlainLength )
                          != IDE_SUCCESS );
                
                IDE_ASSERT_MSG( sPlainLength <= sEncColumn->precision,
                                "sPlainLength :%"ID_UINT32_FMT"\n"
                                "sEncColumn->precision : %"ID_UINT32_FMT"\n",
                                sPlainLength, sEncColumn->precision);

                idlOS::memcpy( sCharValue->value,
                               sPlain,
                               sPlainLength );

                sCharValue->length = sPlainLength;                
            }
            else
            {
                sCharValue->length = 0;
            }
        }        
        else
        {
            sCharValue->length = sEcharValue->mCipherLength;

            if( sCharValue->length > 0 )
            {                
                idlOS::memcpy( sCharValue->value,
                               sEcharValue->mValue,
                               sEcharValue->mCipherLength );
            }
            else
            {
                // Nothing to do.
            }
        }
    }
    else
    {
        // convert for server
        sCharValue->length = sEcharValue->mCipherLength;

        if( sCharValue->length > 0 )
        {
            idlOS::memcpy( sCharValue->value,
                           sEcharValue->mValue,
                           sEcharValue->mCipherLength );
        }
        else
        {
            // Nothing to do.
        }
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;    
}
 

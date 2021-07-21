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

extern mtvModule mtvEvarchar2Varchar;

extern mtdModule mtdVarchar;
extern mtdModule mtdEvarchar;

static IDE_RC mtvEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

IDE_RC mtvCalculate_Evarchar2Varchar( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

mtvModule mtvEvarchar2Varchar = {
    &mtdVarchar,
    &mtdEvarchar,
    MTV_COST_DEFAULT|MTV_COST_SMALL_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Evarchar2Varchar,
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
                                     & mtdVarchar,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Evarchar2Varchar( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          /* aRemain */,
                                      void        * /* aInfo */,
                                      mtcTemplate * aTemplate )
{
    mtdCharType    * sVarcharValue;
    mtdEcharType   * sEvarcharValue;
    mtcColumn      * sEncColumn;
    mtcEncryptInfo   sDecryptInfo;
    UShort           sPlainLength;
    UChar          * sPlain;
    UChar            sDecryptedBuf[MTD_ECHAR_DECRYPT_BUFFER_SIZE];
    
    sVarcharValue  = (mtdCharType*)aStack[0].value;
    sEvarcharValue = (mtdEcharType*)aStack[1].value;
    sPlain         = sDecryptedBuf;    

    if ( aNode != NULL )
    {
        sEncColumn = aTemplate->rows[aNode->baseTable].columns
            + aNode->baseColumn;

        IDE_ASSERT_MSG( sEncColumn->module->id == MTD_EVARCHAR_ID,
                        "sEncColumn->module->id : %"ID_UINT32_FMT"\n",
                        sEncColumn->module->id );
        
        if ( sEncColumn->mColumnAttr.mEncAttr.mPolicy[0] != '\0' )
        {
            if( sEvarcharValue->mCipherLength > 0)
            {
                IDE_TEST( aTemplate->getDecryptInfo( aTemplate,
                                                     aNode->baseTable,
                                                     aNode->baseColumn,
                                                     & sDecryptInfo )
                          != IDE_SUCCESS );
                
                IDE_TEST( aTemplate->decrypt( & sDecryptInfo,
                                              sEncColumn->mColumnAttr.mEncAttr.mPolicy,
                                              sEvarcharValue->mValue,
                                              sEvarcharValue->mCipherLength,
                                              sPlain,
                                              & sPlainLength )
                          != IDE_SUCCESS );
                
                IDE_ASSERT_MSG( sPlainLength <= sEncColumn->precision,
                                "sPlainLength : %"ID_UINT32_FMT"\n"
                                "sEncColumn->precision : %"ID_UINT32_FMT"\n",
                                sPlainLength, sEncColumn->precision );

                idlOS::memcpy( sVarcharValue->value,
                               sPlain,
                               sPlainLength );

                sVarcharValue->length = sPlainLength;       
            }
            else
            {
                sVarcharValue->length = 0;
            }            
        }
        else
        {
            sVarcharValue->length = sEvarcharValue->mCipherLength;

            if( sVarcharValue->length > 0 )
            {
                idlOS::memcpy( sVarcharValue->value,
                               sEvarcharValue->mValue,
                               sVarcharValue->length );
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
        sVarcharValue->length = sEvarcharValue->mCipherLength;
        
        if( sVarcharValue->length > 0 )
        {
            idlOS::memcpy( sVarcharValue->value,
                           sEvarcharValue->mValue,
                           sVarcharValue->length );
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
 

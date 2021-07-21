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

extern mtvModule mtvVarchar2Evarchar;

extern mtdModule mtdVarchar;
extern mtdModule mtdEvarchar;

static IDE_RC mtvEstimate( mtcNode     * aNode,
                           mtcTemplate * aTemplate,
                           mtcStack    * aStack,
                           SInt          aRemain,
                           mtcCallBack * aCallBack );

IDE_RC mtvCalculate_Varchar2Evarchar( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

mtvModule mtvVarchar2Evarchar = {
    &mtdEvarchar,
    &mtdVarchar,
    MTV_COST_DEFAULT,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varchar2Evarchar,
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
    UInt  sECCSize;
    
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    IDE_TEST( aTemplate->getECCSize( aStack[1].column->precision,
                                     & sECCSize )
              != IDE_SUCCESS );

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdEvarchar,
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS );
    
    IDE_TEST( mtc::initializeEncryptColumn(
                  aStack[0].column,
                  (SChar*) "",                 // default policy
                  aStack[1].column->precision, // encrypted size
                  sECCSize )                   // ECC size
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varchar2Evarchar( mtcNode     * /* aNode */,
                                      mtcStack    * aStack,
                                      SInt          /* aRemain */,
                                      void        * /* aInfo */,
                                      mtcTemplate * aTemplate )
{
    mtcECCInfo     sInfo;
    mtdEcharType * sEvarcharValue;    
    mtdCharType  * sVarcharValue;

    sEvarcharValue = (mtdEcharType*)aStack[0].value;
    sVarcharValue  = (mtdCharType*)aStack[1].value;    
    
    sEvarcharValue->mCipherLength = sVarcharValue->length;

    if( sEvarcharValue->mCipherLength > 0 )
    {
        idlOS::memcpy( sEvarcharValue->mValue,
                       sVarcharValue->value,
                       sEvarcharValue->mCipherLength );

        IDE_TEST( aTemplate->getECCInfo( aTemplate,
                                         & sInfo )
                  != IDE_SUCCESS );
        
        IDE_TEST( aTemplate->encodeECC(
                      & sInfo,
                      sVarcharValue->value,
                      sVarcharValue->length,
                      sEvarcharValue->mValue + sEvarcharValue->mCipherLength,
                      & sEvarcharValue->mEccLength )
                  != IDE_SUCCESS );
    }
    else
    {        
        sEvarcharValue->mEccLength = 0;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

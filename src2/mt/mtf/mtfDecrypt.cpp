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
 * $Id: mtfGetClobLocator.cpp 15573 2006-04-15 02:50:33Z mhjeong $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfDecrypt;

extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdEchar;
extern mtdModule mtdEvarchar;

static mtcName mtfDecryptFunctionName[1] = {
    { NULL, 8, (void*)"_DECRYPT" }
};

static IDE_RC mtfDecryptEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfDecrypt = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // TODO : default selectivity
    mtfDecryptFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDecryptEstimate
};

IDE_RC mtfDecryptCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDecryptCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDecryptEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         /* aRemain */,
                           mtcCallBack* /* aCallBack */ )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST_RAISE( ( aStack[1].column->module->id != MTD_ECHAR_ID ) &&
                    ( aStack[1].column->module->id != MTD_EVARCHAR_ID ),
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if ( aStack[1].column->module->id == MTD_ECHAR_ID )
    {
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdChar,
                                         1,
                                         aStack[1].column->precision,
                                         0 )
                  != IDE_SUCCESS );
    }
    else /* if ( aStack[1].column->module->id == MTD_EVARCHAR_ID ) */
    {
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdVarchar,
                                         1,
                                         aStack[1].column->precision,
                                         0 )
                  != IDE_SUCCESS );
    }

    // echar와 evarchar가 같다.
    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDecryptCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
    mtdCharType      * sCharValue;
    mtdEcharType     * sEcharValue;
    mtcNode          * sEncNode;
    mtcColumn        * sEncColumn;
    mtcEncryptInfo     sDecryptInfo;
    UShort             sPlainLength;
    UChar            * sPlain;
    UChar              sDecryptedBuf[MTD_ECHAR_DECRYPT_BUFFER_SIZE];

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    //-------------------------------------------------
    // decrypt : echar -> char
    //-------------------------------------------------
    
    sCharValue   = (mtdCharType*)aStack[0].value;
    sEcharValue  = (mtdEcharType*)aStack[1].value;
    sPlain       = sDecryptedBuf;
        
    sEncNode = aNode->arguments;
    sEncColumn = aTemplate->rows[sEncNode->baseTable].columns
        + sEncNode->baseColumn;
    
    if ( sEncColumn->mColumnAttr.mEncAttr.mPolicy[0] != '\0' )
    {
        if( sEcharValue->mCipherLength > 0 )
        {
            IDE_TEST( aTemplate->getDecryptInfo( aTemplate,
                                                 sEncNode->baseTable,
                                                 sEncNode->baseColumn,
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
                            "sPlainLength : %"ID_UINT32_FMT"\n"
                            "sEncColumn->precision : %"ID_UINT32_FMT"\n",
                            sPlainLength, sEncColumn->precision );

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
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

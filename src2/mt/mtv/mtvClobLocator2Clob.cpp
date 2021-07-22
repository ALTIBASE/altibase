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
 * $Id: mtvClob2ClobLocator.cpp 13146 2005-08-12 09:20:06Z sungminee $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtvModule mtvClobLocator2Clob;

extern mtdModule mtdClob;
extern mtdModule mtdClobLocator;

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_ClobLocator2Clob( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

mtvModule mtvClobLocator2Clob = {
    &mtdClob,
    &mtdClobLocator,
    MTV_COST_DEFAULT|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_ClobLocator2Clob,
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

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdClob,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_ClobLocator2Clob( mtcNode*,
                                      mtcStack*    aStack,
                                      SInt,
                                      void*,
                                      mtcTemplate* aTemplate )
{
    mtdClobLocatorType   sLocator = MTD_LOCATOR_NULL;
    mtdClobType        * sClobValue;
    UInt                 sLobLength;
    UInt                 sReadLength;
    idBool               sIsNull;

    const mtlModule    * sLanguage;
    idBool               sTruncated = ID_FALSE;
    UInt                 sSize;

    sLanguage = aStack[0].column->language;
    
    sLocator = *(mtdClobLocatorType *)aStack[1].value;

    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLobLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );

    if ( sIsNull == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        /* BUG-47732  empty lob을 PSM 내부에서 select 할 때 에러 발생 */
        if ( sLobLength <= MTD_LOB_EMPTY_LENGTH )
        {
            sClobValue = (mtdClobType*)aStack[0].value;
            sClobValue->length = MTD_LOB_EMPTY_LENGTH;
        }
        else
        {
            // BUG-38842
            // clob to varchar conversion시 지정한 길이만큼만 변환한다.
            if ( MTU_CLOB_TO_VARCHAR_PRECISION < sLobLength )
            {
                sLobLength = MTU_CLOB_TO_VARCHAR_PRECISION;

                // 문자가 짤릴 수 있다.
                sTruncated = ID_TRUE;
            }
            else
            {
                // Nothing to do.
            }

            IDE_TEST_RAISE( (UInt)aStack[0].column->precision < sLobLength,
                            ERR_CONVERT );

            sClobValue = (mtdClobType*)aStack[0].value;

            IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                    sLocator,
                                    0,
                                    sLobLength,
                                    sClobValue->value,
                                    & sReadLength )
                      != IDE_SUCCESS );

            if ( sTruncated == ID_FALSE )
            {
                sClobValue->length = (SLong)sReadLength;
            }
            else
            {
                IDE_TEST( mtf::truncIncompletedString(
                              sClobValue->value,
                              sReadLength,
                              & sSize,
                              sLanguage )
                          != IDE_SUCCESS );

                sClobValue->length = (UShort)sSize;
            }
        }
    }

    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CONVERT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_CONVERSION_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );

    return IDE_FAILURE;
}

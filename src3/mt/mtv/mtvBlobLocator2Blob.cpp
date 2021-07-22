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
 * $Id: mtvBlob2BlobLocator.cpp 13146 2005-08-12 09:20:06Z leekmo $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>

extern mtvModule mtvBlobLocator2Blob;

extern mtdModule mtdBlob;
extern mtdModule mtdBlobLocator;

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_BlobLocator2Blob( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

mtvModule mtvBlobLocator2Blob = {
    &mtdBlob,
    &mtdBlobLocator,
    MTV_COST_DEFAULT|MTV_COST_LOSS_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_BlobLocator2Blob,
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
                                     & mtdBlob,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_BlobLocator2Blob( mtcNode*,
                                      mtcStack*    aStack,
                                      SInt,
                                      void*,
                                      mtcTemplate* aTemplate )
{
    mtdBlobLocatorType   sLocator = MTD_LOCATOR_NULL;
    mtdBlobType        * sBlobValue;
    UInt                 sLobLength;
    UInt                 sReadLength;
    idBool               sIsNull;

    sLocator = *(mtdBlobLocatorType *)aStack[1].value;

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
        IDE_TEST_RAISE( (UInt)aStack[0].column->precision < sLobLength,
                        ERR_CONVERT );

        sBlobValue = (mtdBlobType*)aStack[0].value;

        IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                sLocator,
                                0,
                                sLobLength,
                                sBlobValue->value,
                                & sReadLength )
                  != IDE_SUCCESS );

        sBlobValue->length = (SLong)sReadLength;
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

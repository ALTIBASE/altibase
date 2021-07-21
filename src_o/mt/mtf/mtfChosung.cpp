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
 * $Id: mtfChosung.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#define MTF_CHOSUNG_NUM 19

UShort mtfChosungTbl[MTF_CHOSUNG_NUM+1][2] = {
    { 0xb0a1, 0xa4a1 },
    { 0xb1ee, 0xa4a2 },
    { 0xb3aa, 0xa4a4 },
    { 0xb4d9, 0xa4a7 },
    { 0xb5fb, 0xa4a8 },
    { 0xb6f3, 0xa4a9 },
    { 0xb8b6, 0xa4b1 },
    { 0xb9d9, 0xa4b2 },
    { 0xbafc, 0xa4b3 },
    { 0xbbe7, 0xa4b5 },
    { 0xbdce, 0xa4b6 },
    { 0xbec6, 0xa4b7 },
    { 0xc0da, 0xa4b8 },
    { 0xc2a5, 0xa4b9 },
    { 0xc2f7, 0xa4ba },
    { 0xc4ab, 0xa4bb },
    { 0xc5b8, 0xa4bc },
    { 0xc6c4, 0xa4bd },
    { 0xc7cf, 0xa4be },
    { 0xffff, 0xffff } /* end marker */
};

extern mtfModule mtfChosung;

static mtcName mtfChosungFunctionName[1] = {
    { NULL, 7, (void*)"CHOSUNG" }
};

UShort mtfGetWansungCode   ( UChar  aByte, UInt * aFirst );
void   mtfExtractChosung   ( UShort aWansungCode, UChar ** aOutBuf );

static IDE_RC mtfChosungEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

IDE_RC mtfChosungCalculate(mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

mtfModule mtfChosung = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfChosungFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfChosungEstimate
};

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfChosungCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfChosungEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                            aStack[1].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[0],
                                     1,
                                     aStack[1].column->precision,
                                     0 )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UShort mtfGetWansungCode( UChar aByte, UInt * aFirst )
{
    UInt   sFirst = *aFirst;
    UShort sReturnCode;

    if ( aByte < 0x80 )
    {
        *aFirst = 0;

        return aByte;
    }
    else if ( sFirst == 0 )
    {
        *aFirst = aByte;

        return 0;
    }
    else
    {
        sReturnCode = (sFirst << 8) + aByte;
        *aFirst = 0;

        return sReturnCode;
    }
}

void mtfExtractChosung( UShort aWansungCode, UChar ** aOutBuf )
{
    UChar  sWansungUpper;
    UChar  sWansungLower;
    UShort sWansungChosung;

    UShort sTest1;
    UShort sTest2;
    SInt   sLow;
    SInt   sMed;
    SInt   sUpp;


    if( aWansungCode == 0 ) return;

    sWansungUpper = ( aWansungCode >> 8 ) & 0xff;
    sWansungLower = ( aWansungCode & 0xff );

    if( sWansungUpper == 0 )
    {
        *(*aOutBuf) = sWansungLower;
        (*aOutBuf)++;
    }
    else
    {
        sWansungChosung = aWansungCode;

        if ( sWansungUpper >= 0xb0 && sWansungUpper <= 0xC8
             && sWansungLower >= 0xa1 && sWansungLower <= 0xfe )
        {
            sLow = 0;
            sUpp = MTF_CHOSUNG_NUM - 1;

            while ( sLow <= sUpp )
            {
                sMed   = ( sLow + sUpp ) / 2;
                sTest1 = mtfChosungTbl[sMed][0];
                sTest2 = mtfChosungTbl[sMed+1][0];

                if ( aWansungCode < sTest1 )
                {
                    sUpp = sMed - 1;
                }
                else if ( aWansungCode >= sTest2 )
                {
                    sLow = sMed + 1;
                }
                else
                {
                    sWansungChosung = mtfChosungTbl[sMed][1];
                    break;
                }
            }
        }

        **aOutBuf = (UChar)((sWansungChosung >> 8) & 0xff);
        (*aOutBuf)++;
        **aOutBuf = (UChar)(sWansungChosung & 0xff);
        (*aOutBuf)++;
    }
}


IDE_RC mtfChosungCalculate(mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
    mtdCharType*   sValue;
    mtdCharType*   sVarchar;
    UChar*         sTarget;
    UChar*         sIterator;
    UChar*         sFence;

    UShort         sCode;
    UInt           sFirst;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue   = (mtdCharType*)aStack[0].value;
        sVarchar = (mtdCharType*)aStack[1].value;
        
        // extract chosung
        sValue->length = sVarchar->length;

        sFirst  = 0;
        for( sTarget   = sValue->value,
             sIterator = sVarchar->value,
             sFence    = sIterator + sVarchar->length;
             sIterator < sFence;
             sIterator++ )
        {
            sCode = mtfGetWansungCode( *sIterator, &sFirst );
            mtfExtractChosung( sCode, &sTarget );
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

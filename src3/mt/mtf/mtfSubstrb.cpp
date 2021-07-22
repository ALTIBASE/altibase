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
 * $Id: mtfSubstrb.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

extern mtfModule mtfSubstrb;

extern mtdModule mtdInteger;
extern mtdModule mtdBigint;
extern mtdModule mtdVarchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdClob;

static mtcName mtfSubstrbFunctionName[1] = {
    { NULL, 7, (void*)"SUBSTRB" }
};

static IDE_RC mtfSubstrbEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfSubstrb = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSubstrbFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfSubstrbEstimate
};

IDE_RC mtfSubstrbCalculate2( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

IDE_RC mtfSubstrbCalculate3( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

IDE_RC mtfSubstrbCalculateXlobLocator2( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

IDE_RC mtfSubstrbCalculateXlobLocator3( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfSubstringCalculateClobValue2( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfSubstringCalculateClobValue3( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate );

const mtcExecute mtfExecute2 = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstrbCalculate2,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecute3 = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstrbCalculate3,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteXlobLocator2 = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstrbCalculateXlobLocator2,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteXlobLocator3 = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstrbCalculateXlobLocator3,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecuteClobValue2 = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateClobValue2,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecuteClobValue3 = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateClobValue3,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubstrbEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];
    SInt             sPrecision;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID )
    {
        // PROJ-1362
        sModules[0] = &mtdVarchar;
        sModules[1] = &mtdBigint;
        sModules[2] = &mtdBigint;

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments->next,
                                            aTemplate,
                                            aStack + 2,
                                            aCallBack,
                                            sModules + 1 )
                  != IDE_SUCCESS );

        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocator2;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocator3;
        }

        // fix BUG-25560
        // precison을 clob의 최대 크기로 설정
        /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
         * Lob Locator는 VARCHAR로 가능한 최대 크기로 설정
         */
        sPrecision = MTD_VARCHAR_PRECISION_MAXIMUM;
    }
    else if ( aStack[1].column->module->id == MTD_CLOB_ID )
    {
        if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
        {
            // PROJ-1362
            IDE_TEST( mtf::getLobFuncResultModule( &sModules[0],
                                                   aStack[1].column->module )
                      != IDE_SUCCESS );
            sModules[1] = &mtdBigint;
            sModules[2] = &mtdBigint;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocator2;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocator3;
            }

            // fix BUG-25560
            // precison을 clob의 최대 크기로 설정
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
             * Lob Column이면 VARCHAR로 가능한 최대 크기로 설정한다.
             */
            sPrecision = MTD_VARCHAR_PRECISION_MAXIMUM;
        }
        else
        {
            sModules[0] = &mtdClob;
            sModules[1] = &mtdBigint;
            sModules[2] = &mtdBigint;

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteClobValue2;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteClobValue3;
            }

            // fix BUG-25560
            // precison을 clob의 최대 크기로 설정
            /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원
             * Lob Value이면 VARCHAR의 최대 크기와 Lob Value의 크기 중 작은 것으로 설정한다.
             */
            sPrecision = IDL_MIN( MTD_VARCHAR_PRECISION_MAXIMUM,
                                  aStack[1].column->precision );
        }
    }
    else
    {
        IDE_TEST( mtf::getCharFuncResultModule( &sModules[0],
                                                aStack[1].column->module )
                  != IDE_SUCCESS );
        sModules[1] = &mtdInteger;
        sModules[2] = &mtdInteger;

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecute2;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecute3;
        }

        sPrecision = aStack[1].column->precision;
    }

    // PROJ-1579 NCHAR
    if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
        (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
    {
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdNvarchar,
                                         1,
                                         sPrecision,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,  // BUG-16501
                                         1,
                                         sPrecision,
                                         0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSubstrbCalculate2( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    mtdCharType     * sValue;
    mtdCharType     * sVarchar;
    mtdIntegerType    sStart;
    const mtlModule * sLanguage;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sIndexLast;
    mtdIntegerType    sAdjustStart;
    mtdIntegerType    sAdjustSourceStart;
    mtdIntegerType    sLength;
    mtlNCRet          sRet;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue    = (mtdCharType*)aStack[0].value;
        sVarchar  = (mtdCharType*)aStack[1].value;
        sStart    = *(mtdIntegerType*)aStack[2].value;
        sLanguage = aStack[1].column->language;

        if( sStart > 0 )
        {
            sStart--;
        }
        else if( sStart != 0 )
        {
            sStart += sVarchar->length;
        }
        if( sStart < 0 || sVarchar->length <= sStart )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            sValue->length     = sVarchar->length - sStart;
            sLength            = sValue->length;
            sSourceIndex       = sVarchar->value;
            sSourceFence       = sSourceIndex + sStart;
            sAdjustSourceStart = 0;
            sRet               = NC_INVALID;

            while( sSourceIndex < sSourceFence )
            {
                sIndexLast = sSourceIndex;
                sRet       = sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
            }

            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustStart = sSourceIndex - sIndexLast;
                sLength     -= sAdjustStart;
                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustStart );
                    sAdjustSourceStart = sAdjustStart;
                }
                else
                {
                    sValue->length -= sAdjustStart;
                }
            }
            else
            {
                sAdjustStart = 0;
            }

            if ( (sLanguage->id == MTL_UTF16_ID) && (sValue->length % 2 != 0) )
            {
                sValue->length--;
                sLength--;
            }
            else
            {
                //nothing to do
            }
            
            if ( sLength > 0 )
            {
                idlOS::memcpy( sValue->value + sAdjustSourceStart,
                               sVarchar->value + sStart + sAdjustStart,
                               sLength );
            }
            else
            {
                //nothing to do
            }
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSubstrbCalculate3( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
    mtdCharType     * sValue;
    mtdCharType     * sVarchar;
    mtdIntegerType    sStart;
    mtdIntegerType    sLength;
    const mtlModule * sLanguage;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sIndexLast;
    mtdIntegerType    sAdjustStart;
    mtdIntegerType    sAdjustSourceStart;
    mtdIntegerType    sAdjustEnd;
    mtlNCRet          sRet;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue             = (mtdCharType*)aStack[0].value;
        sVarchar           = (mtdCharType*)aStack[1].value;
        sStart             = *(mtdIntegerType*)aStack[2].value;
        sLength            = *(mtdIntegerType*)aStack[3].value;
        sLanguage          = aStack[1].column->language;
        sAdjustSourceStart = 0;

        if( sStart > 0 )
        {
            sStart--;
        }
        else if( sStart != 0 )
        {
            sStart += sVarchar->length;
        }
        if( sStart < 0 || sVarchar->length <= sStart || sLength <= 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            sSourceIndex = sVarchar->value;
            sSourceFence = sSourceIndex + sStart;
            sRet         = NC_INVALID;
            
            if( sLength > sVarchar->length - sStart )
            {
                sLength = sVarchar->length - sStart;
            }
            sValue->length = sLength;

            while( sSourceIndex < sSourceFence )
            {
                sIndexLast = sSourceIndex;
                sRet       = sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
            }

            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustStart = sSourceIndex - sIndexLast;
                sLength     -= sAdjustStart;
               
                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustStart );
                    sAdjustSourceStart = sAdjustStart;
                    sSourceIndex      -= sAdjustStart;
                }
                else
                {
                    sValue->length -= sAdjustStart;
                }
            }
            else
            {
                sAdjustStart = 0;
            }

            // BUG-37527
            if ( sLength > 0 )
            {
                sSourceFence = sVarchar->value + sValue->length + sStart;
                sRet         = NC_INVALID;
            
                while( sSourceIndex < sSourceFence )
                {
                    sIndexLast = sSourceIndex;
                    sRet       = sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
                }

                if ( sRet == NC_MB_INCOMPLETED )
                {
                    sAdjustEnd = sSourceFence - sIndexLast;
                    sLength   -= sAdjustEnd;
                    if ( sLanguage->id != MTL_UTF16_ID )
                    {
                        idlOS::memset( sValue->value + sValue->length - sAdjustEnd,
                                       *sLanguage->specialCharSet[MTL_SP_IDX],
                                       sAdjustEnd );
                    }
                    else
                    {
                        sValue->length -= sAdjustEnd;
                    }
                }

                if ( (sLanguage->id == MTL_UTF16_ID) && (sValue->length % 2 != 0) )
                {
                    sValue->length--;
                    sLength--;
                }
                else
                {
                    //nothing to do
                }
           
                if ( sLength > 0 )
                {
                    idlOS::memcpy( sValue->value + sAdjustSourceStart,
                                   sVarchar->value + sStart + sAdjustStart,
                                   sLength );
                }
                else
                {
                    //nothing to do
                }
            }
            else
            {
                // Nothing to do.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfSubstrbCalculateXlobLocator2( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    mtdCharType        * sValue;
    mtdBigintType        sStart;
    mtdBigintType        sLength;
    mtdClobLocatorType   sLocator = MTD_LOCATOR_NULL;
    UInt                 sLobLength;
    idBool               sIsNull;
    UInt                 sReadLength;
    const mtlModule    * sLanguage;
    UChar              * sFence;
    UChar              * sBufferFence;
    UChar                sBuffer[MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION];
    UChar              * sIndexLast;
    UInt                 sBufferOffset;
    UInt                 sBufferMount;
    UInt                 sBufferSize;
    UChar              * sIndex;
    mtdBigintType        sAdjustStart;
    mtdBigintType        sAdjustSourceStart;
    mtlNCRet             sRet;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sLocator = *(mtdClobLocatorType*)aStack[1].value;
    
    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLobLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );
    
    if ( (sIsNull == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue    = (mtdCharType*)aStack[0].value;
        sStart    = *(mtdBigintType*)aStack[2].value;
        sLength   = aStack[0].column->precision;
        sLanguage = aStack[1].column->language;
        
        if( sStart > 0 )
        {
            sStart--;
        }
        else
        {
            // Nothing to do.
        }
        
        if( (sStart < 0) || (sLobLength <= sStart) || (sLength <= 0) )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            sBufferSize        = 0;
            sBufferOffset      = 0;
            sAdjustSourceStart = 0;
            sRet               = NC_INVALID;
            
            if ( sStart + sLength > sLobLength )
            {
                sLength = sLobLength - sStart;
            }
            else
            {
                // Nothing to do.
            }

            while ( sBufferOffset < sStart )
            {
                if ( sBufferOffset + MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION > sStart )
                {
                    sBufferMount = sStart - sBufferOffset;
                    sBufferSize  = sBufferMount;
                }
                else
                {
                    sBufferMount = MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION;
                    sBufferSize  = MTC_LOB_BUFFER_SIZE;
                }

                IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                        sLocator,
                                        sBufferOffset,
                                        sBufferMount,
                                        sBuffer,
                                        &sReadLength )
                          != IDE_SUCCESS );
                
                sIndex       = sBuffer;
                sFence       = sIndex + sBufferSize;
                sBufferFence = sIndex + sBufferMount;
               
                while ( sIndex < sFence )
                {
                    sIndexLast = sIndex;
                    sRet       = sLanguage->nextCharPtr( &sIndex, sBufferFence );
                }
                sBufferOffset += ( sIndex - sBuffer );
            }

            sValue->length = sLength;

            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustStart   = sIndex - sIndexLast;
                sLength       -= sAdjustStart;
                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustStart );
                    sAdjustSourceStart = sAdjustStart;
                }
                else
                {
                    sValue->length = sLength;
                }
            }
            else
            {
                sAdjustStart = 0;
            }

            if ( (sLanguage->id == MTL_UTF16_ID) && (sValue->length % 2 != 0) )
            {
                sValue->length--;
                sLength--;
            }
            else
            {
                //nothing to do
            }

            if ( sLength > 0 )
            {
                IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                        sLocator,
                                        sStart + sAdjustSourceStart,
                                        sLength,
                                        sValue->value + sAdjustStart,
                                        &sReadLength )
                          != IDE_SUCCESS );
            }
            else
            {
                //nothing to do
            }
        }
    }

    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}

IDE_RC mtfSubstrbCalculateXlobLocator3( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    mtdCharType        * sValue;
    mtdBigintType        sStart;
    mtdBigintType        sLength;
    mtdClobLocatorType   sLocator = MTD_LOCATOR_NULL;
    UInt                 sLobLength;
    idBool               sIsNull;
    UInt                 sReadLength;
    UChar              * sFence;
    UChar              * sBufferFence;
    UChar                sBuffer[MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION];
    UInt                 sBufferOffset;
    UInt                 sBufferMount;
    UInt                 sBufferSize;
    UChar              * sIndex;
    UChar              * sIndexLast;
    mtdBigintType        sAdjustStart;
    mtdBigintType        sAdjustSourceStart;
    mtdIntegerType       sAdjustEnd;
    const mtlModule    * sLanguage;
    mtlNCRet             sRet;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    sLocator = *(mtdClobLocatorType*)aStack[1].value;
    
    IDE_TEST( mtc::getLobLengthLocator( sLocator,
                                        & sIsNull,
                                        & sLobLength,
                                        mtc::getStatistics(aTemplate) )
              != IDE_SUCCESS );
    
    if ( (sIsNull == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) ||
         (aStack[3].column->module->isNull( aStack[3].column,
                                            aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue    = (mtdCharType*)aStack[0].value;
        sStart    = *(mtdBigintType*)aStack[2].value;
        sLength   = *(mtdBigintType*)aStack[3].value;
        sLanguage = aStack[1].column->language;
        
        if( sStart > 0 )
        {
            sStart--;
        }
        else
        {
            // Nothing to do.
        }
        
        if( (sStart < 0) || (sLobLength <= sStart) || (sLength <= 0) )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            IDE_TEST_RAISE( sLength > aStack[0].column->precision,
                            ERR_EXCEED_MAX );

            if ( sStart + sLength > sLobLength )
            {
                sLength = sLobLength - sStart;
            }
            else
            {
                // Nothing to do.
            }

            sBufferOffset      = 0;
            sBufferSize        = 0;
            sAdjustSourceStart = 0;
            sRet               = NC_INVALID;
            
            while ( sBufferOffset < sStart )
            {
                if ( sBufferOffset + MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION > sStart )
                {
                    sBufferMount = sStart - sBufferOffset;
                    sBufferSize  = sBufferMount;
                }
                else
                {
                    sBufferMount = MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION;
                    sBufferSize  = MTC_LOB_BUFFER_SIZE;
                }

                IDE_TEST( mtc::readLob( mtc::getStatistics(aTemplate), /* idvSQL* */
                                        sLocator,
                                        sBufferOffset,
                                        sBufferMount,
                                        sBuffer,
                                        &sReadLength )
                          != IDE_SUCCESS );
                
                sIndex       = sBuffer;
                sFence       = sIndex + sBufferSize;
                sBufferFence = sIndex + sBufferMount;

                while ( sIndex < sFence )
                {
                    sIndexLast = sIndex;
                    sRet       = sLanguage->nextCharPtr( & sIndex, sBufferFence );
                }
                sBufferOffset += ( sIndex - sBuffer );
            }

            sValue->length = sLength;
            
            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustStart   = sIndex - sIndexLast;
                sLength       -= sAdjustStart;

                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustStart );
                    sAdjustSourceStart = sAdjustStart;
                }
                else
                {
                    sValue->length = sLength;

                }
            }
            else
            {
                sAdjustStart = 0;
            }

            // BUG-37527
            if ( sLength > 0 )
            {
                IDE_TEST( mtc::readLob( mtc::getStatistics(aTemplate), /* idvSQL* */
                                        sLocator,
                                        sStart + sAdjustSourceStart,
                                        sLength,
                                        sValue->value + sAdjustStart,
                                        &sReadLength )
                          != IDE_SUCCESS );

                sIndex = sValue->value + sAdjustStart;
                sFence = sIndex + sLength;
                sRet   = NC_INVALID;

                while ( sIndex < sFence )
                {
                    sIndexLast = sIndex;
                    sRet       = sLanguage->nextCharPtr( &sIndex, sFence );
                }

                if ( sRet == NC_MB_INCOMPLETED )
                {
                    sAdjustEnd = sFence - sIndexLast;
                    sLength   -= sAdjustEnd;
                    if ( sLanguage->id == MTL_UTF16_ID )
                    {
                        sValue->length -= sAdjustEnd;
                    }
                    else
                    {
                        idlOS::memset( sValue->value + sValue->length - sAdjustEnd,
                                       *sLanguage->specialCharSet[MTL_SP_IDX],
                                       sAdjustEnd );
                    }
                }
                else
                {
                    //nothing to do
                }

                if ( (sLanguage->id == MTL_UTF16_ID) && (sValue->length % 2 != 0) )
                {
                    sValue->length--;
                }
                else
                {
                    //nothing to do
                }
            }
            else
            {
                //nothing to do
            }
        }
    }

    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }
    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}

IDE_RC mtfSubstringCalculateClobValue2( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate )
{
    mtdCharType     * sValue;
    mtdClobType     * sClobValue;
    mtdBigintType     sStart;
    mtdBigintType     sLength;
    const mtlModule * sLanguage;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sIndexLast;
    mtdBigintType     sAdjustStart;
    mtdBigintType     sAdjustSourceStart;
    mtlNCRet          sRet;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue     = (mtdCharType *)aStack[0].value;
        sClobValue = (mtdClobType *)aStack[1].value;
        sStart     = *(mtdBigintType *)aStack[2].value;
        sLanguage  = aStack[1].column->language;

        if ( sStart > 0 )
        {
            sStart--;
        }
        else if ( sStart != 0 )
        {
            sStart += sClobValue->length;
        }
        if ( (sStart < 0) || (sClobValue->length <= sStart) )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            sSourceIndex       = sClobValue->value;
            sSourceFence       = sSourceIndex + sStart;
            sAdjustSourceStart = 0;
            sRet               = NC_INVALID;

            sLength            = sClobValue->length - sStart;
            sValue->length     = (UShort) sLength;

            while ( sSourceIndex < sSourceFence )
            {
                sIndexLast = sSourceIndex;
                sRet       = sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
            }

            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustStart = sSourceIndex - sIndexLast;
                sLength     -= sAdjustStart;
                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustStart );
                    sAdjustSourceStart = sAdjustStart;
                }
                else
                {
                    sValue->length -= (UShort) sAdjustStart;
                }
            }
            else
            {
                sAdjustStart = 0;
            }

            if ( (sLanguage->id == MTL_UTF16_ID) && (sValue->length % 2 != 0) )
            {
                sValue->length--;
                sLength--;
            }
            else
            {
                //nothing to do
            }

            if ( sLength > 0 )
            {
                IDE_TEST_RAISE( sLength > (SLong) aStack[0].column->precision,
                                ERR_EXCEED_MAX );

                idlOS::memcpy( sValue->value + sAdjustSourceStart,
                               sClobValue->value + sStart + sAdjustStart,
                               sLength );
            }
            else
            {
                //nothing to do
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_idnReachEnd ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSubstringCalculateClobValue3( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate )
{
    mtdCharType     * sValue;
    mtdClobType     * sClobValue;
    mtdBigintType     sStart;
    mtdBigintType     sLength;
    const mtlModule * sLanguage;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sIndexLast;
    mtdBigintType     sAdjustStart;
    mtdBigintType     sAdjustSourceStart;
    mtdBigintType     sAdjustEnd;
    mtlNCRet          sRet;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) ||
         (aStack[3].column->module->isNull( aStack[3].column,
                                            aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue     = (mtdCharType *)aStack[0].value;
        sClobValue = (mtdClobType *)aStack[1].value;
        sStart     = *(mtdBigintType *)aStack[2].value;
        sLength    = *(mtdBigintType *)aStack[3].value;
        sLanguage  = aStack[1].column->language;

        if ( sStart > 0 )
        {
            sStart--;
        }
        else if ( sStart != 0 )
        {
            sStart += sClobValue->length;
        }
        if ( (sStart < 0) || (sClobValue->length <= sStart) || (sLength <= 0) )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            sSourceIndex       = sClobValue->value;
            sSourceFence       = sSourceIndex + sStart;
            sAdjustSourceStart = 0;
            sRet               = NC_INVALID;

            if ( sLength > sClobValue->length - sStart )
            {
                sLength = sClobValue->length - sStart;
            }
            else
            {
                //nothing to do
            }
            sValue->length     = (UShort) sLength;

            while ( sSourceIndex < sSourceFence )
            {
                sIndexLast = sSourceIndex;
                sRet       = sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
            }

            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustStart = sSourceIndex - sIndexLast;
                sLength     -= sAdjustStart;
                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustStart );
                    sAdjustSourceStart = sAdjustStart;
                    sSourceIndex      -= sAdjustStart;
                }
                else
                {
                    sValue->length -= (UShort) sAdjustStart;
                }
            }
            else
            {
                sAdjustStart = 0;
            }

            sSourceFence = sClobValue->value + sValue->length + sStart;
            sRet         = NC_INVALID;

            while ( sSourceIndex < sSourceFence )
            {
                sIndexLast = sSourceIndex;
                sRet       = sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
            }

            if ( sRet == NC_MB_INCOMPLETED )
            {
                sAdjustEnd = sSourceFence - sIndexLast;
                sLength   -= sAdjustEnd;
                if ( sLanguage->id != MTL_UTF16_ID )
                {
                    idlOS::memset( sValue->value + sValue->length - sAdjustEnd,
                                   *sLanguage->specialCharSet[MTL_SP_IDX],
                                   sAdjustEnd );
                }
                else
                {
                    sValue->length -= (UShort) sAdjustEnd;
                }
            }
            else
            {
                //nothing to do
            }

            if ( (sLanguage->id == MTL_UTF16_ID) && (sValue->length % 2 != 0) )
            {
                sValue->length--;
                sLength--;
            }
            else
            {
                //nothing to do
            }

            if ( sLength > 0 )
            {
                IDE_TEST_RAISE( sLength > (SLong) aStack[0].column->precision,
                                ERR_EXCEED_MAX );

                idlOS::memcpy( sValue->value + sAdjustSourceStart,
                               sClobValue->value + sStart + sAdjustStart,
                               sLength );
            }
            else
            {
                //nothing to do
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_idnReachEnd ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

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
 * $Id: mtfCharacter_length.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfCharacter_length;

extern mtdModule mtdBigint;

extern mtlModule mtlAscii;

static mtcName mtfCharacter_lengthFunctionName[3] = {
    { mtfCharacter_lengthFunctionName+1, 16, (void*)"CHARACTER_LENGTH" },
    { mtfCharacter_lengthFunctionName+2, 11, (void*)"CHAR_LENGTH"      },
    { NULL,               6, (void*)"LENGTH"           }
};

static IDE_RC mtfCharacter_lengthEstimate( mtcNode*     aNode,
                                           mtcTemplate* aTemplate,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           mtcCallBack* aCallBack );

mtfModule mtfCharacter_length = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfCharacter_lengthFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfCharacter_lengthEstimate
};

// PROJ-1755
static IDE_RC mtfCharacter_lengthCalculate( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate );

static IDE_RC mtfCharacter_lengthCalculate4MB( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfCharacter_lengthCalculateClobValue( mtcNode*     aNode,
                                                     mtcStack*    aStack,
                                                     SInt         aRemain,
                                                     void*        aInfo,
                                                     mtcTemplate* aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfCharacter_lengthCalculateClobValue4MB( mtcNode*     aNode,
                                                        mtcStack*    aStack,
                                                        SInt         aRemain,
                                                        void*        aInfo,
                                                        mtcTemplate* aTemplate );

// PROJ-1755
static IDE_RC mtfCharacter_lengthCalculateXlobLocator( mtcNode*     aNode,
                                                       mtcStack*    aStack,
                                                       SInt         aRemain,
                                                       void*        aInfo,
                                                       mtcTemplate* aTemplate );

static IDE_RC mtfCharacter_lengthCalculateXlobLocator4MB( mtcNode*     aNode,
                                                          mtcStack*    aStack,
                                                          SInt         aRemain,
                                                          void*        aInfo,
                                                          mtcTemplate* aTemplate );

// PROJ-1755
const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCharacter_lengthCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecute4MB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCharacter_lengthCalculate4MB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecuteClobValue = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCharacter_lengthCalculateClobValue,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecuteClobValue4MB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCharacter_lengthCalculateClobValue4MB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

// PROJ-1755
const mtcExecute mtfExecuteXlobLocator = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCharacter_lengthCalculateXlobLocator,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteXlobLocator4MB = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfCharacter_lengthCalculateXlobLocator4MB,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfCharacter_lengthEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt      /* aRemain */,
                                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[1];
    mtcColumn      * sIndexColumn;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // BUG-40992 FATAL when using _prowid
    // 인자의 경우 mtcStack 의 column 값을 이용하면 된다.
    sIndexColumn     = aStack[1].column;

    if ( aStack[1].column->module->id == MTD_CLOB_LOCATOR_ID )
    {
        // PROJ-1362
        if( sIndexColumn->language == &mtlAscii )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                mtfExecuteXlobLocator;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                mtfExecuteXlobLocator4MB;
        }
    }
    else if ( aStack[1].column->module->id == MTD_CLOB_ID )
    {
        if ( aTemplate->isBaseTable( aTemplate, aNode->arguments->table ) == ID_TRUE )
        {
            // PROJ-1362
            IDE_TEST( mtf::getLobFuncResultModule( &sModules[0],
                                                   aStack[1].column->module )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sModules )
                      != IDE_SUCCESS );

            if ( sIndexColumn->language == &mtlAscii )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] =
                    mtfExecuteXlobLocator;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] =
                    mtfExecuteXlobLocator4MB;
            }
        }
        else
        {
            if ( sIndexColumn->language == &mtlAscii )
            {
                /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
                aTemplate->rows[aNode->table].execute[aNode->column] =
                    mtfExecuteClobValue;
            }
            else
            {
                /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
                aTemplate->rows[aNode->table].execute[aNode->column] =
                    mtfExecuteClobValue4MB;
            }
        }
    }
    else
    {
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

        if( sIndexColumn->language == &mtlAscii )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                mtfExecute;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                mtfExecute4MB;
        }
    }

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdBigint,
                                     0,
                                     0,
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

IDE_RC mtfCharacter_lengthCalculate( mtcNode*     aNode,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     void*        aInfo,
                                     mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1755
 *               Character_length Calculate
 *
 * Implementation :
 *    CHAR_LENGTH( char )
 *
 *    aStack[0] : 입력된 문자열의 길이 
 *    aStack[1] : char ( 입력된 문자열 )
 *
 ***********************************************************************/

    mtdCharType   * sCharInput;

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
        sCharInput = (mtdCharType*)aStack[1].value;
        *(mtdBigintType*)aStack[0].value = sCharInput->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCharacter_lengthCalculate4MB( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Ascii Calculate
 *
 * Implementation :
 *    CHAR_LENGTH( char )
 *
 *    aStack[0] : 입력된 문자열의 길이
 *    aStack[1] : char ( 입력된 문자열 )
 *
 ***********************************************************************/

    const mtlModule * sLanguage;
    mtdBigintType     sLength;
    UChar           * sIndex;
    UChar           * sFence;

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
        //------------------------------------------
        // 입력된 문자열을 Language Type에 맞게 한 문자씩 읽어
        // 문자열 길이를 구함
        //------------------------------------------

        sLanguage = aStack[1].column->language;
        sLength = 0;

        sIndex = ((mtdCharType*)aStack[1].value)->value;
        sFence = sIndex + ((mtdCharType*)aStack[1].value)->length;

        while ( sIndex < sFence )
        {
            (void)sLanguage->nextCharPtr( & sIndex, sFence );
            sLength++;
        }

        *(mtdBigintType*)aStack[0].value = sLength;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCharacter_lengthCalculateClobValue( mtcNode     * aNode,
                                              mtcStack    * aStack,
                                              SInt          aRemain,
                                              void        * aInfo,
                                              mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Ascii Calculate
 *
 * Implementation :
 *    CHAR_LENGTH( clob )
 *
 *    aStack[0] : 입력된 문자열의 길이
 *    aStack[1] : clob ( 입력된 문자열 )
 *
 ***********************************************************************/

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        *(mtdBigintType *)aStack[0].value = ((mtdClobType *)aStack[1].value)->length;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCharacter_lengthCalculateClobValue4MB( mtcNode     * aNode,
                                                 mtcStack    * aStack,
                                                 SInt          aRemain,
                                                 void        * aInfo,
                                                 mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Character_length Calculate
 *
 * Implementation :
 *    CHAR_LENGTH( clob )
 *
 *    aStack[0] : 입력된 문자열의 길이
 *    aStack[1] : clob ( 입력된 문자열 )
 *
 ***********************************************************************/

    const mtlModule * sLanguage;
    mtdBigintType     sLength;
    mtdClobType     * sCLobValue;
    UChar           * sIndex;
    UChar           * sFence;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        //------------------------------------------
        // 입력된 문자열을 Language Type에 맞게 한 문자씩 읽어
        // 문자열 길이를 구함
        //------------------------------------------

        sLanguage = aStack[1].column->language;
        sLength = 0;

        sCLobValue = (mtdClobType *)aStack[1].value;
        sIndex = sCLobValue->value;
        sFence = sIndex + sCLobValue->length;

        while ( sIndex < sFence )
        {
            (void)sLanguage->nextCharPtr( & sIndex, sFence );
            sLength++;
        }

        *(mtdBigintType *)aStack[0].value = sLength;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfCharacter_lengthCalculateXlobLocator( mtcNode*     aNode,
                                                mtcStack*    aStack,
                                                SInt         aRemain,
                                                void*        aInfo,
                                                mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1755
 *               Character_length Calculate
 *
 * Implementation :
 *    CHAR_LENGTH( char )
 *
 *    aStack[0] : 입력된 문자열의 길이 
 *    aStack[1] : lob locator
 *
 ***********************************************************************/

    mtdClobLocatorType sLocator = MTD_LOCATOR_NULL;
    UInt               sLobLength;
    idBool             sIsNull;
    
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
    
    if ( sIsNull == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        *(mtdBigintType*)aStack[0].value = sLobLength;
    }
    
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}

IDE_RC mtfCharacter_lengthCalculateXlobLocator4MB( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1362
 *               Character_length Calculate
 *
 * Implementation :
 *    CHAR_LENGTH( char )
 *
 *    aStack[0] : 입력된 문자열의 길이 
 *    aStack[1] : lob locator
 *
 ***********************************************************************/

    const mtlModule  * sLanguage;
    mtdBigintType      sLength;
    UChar              sBuffer[MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION];
    UInt               sBufferOffset;
    UInt               sBufferMount;
    UInt               sBufferSize;
    UChar            * sIndex;
    UChar            * sFence;
    UChar            * sBufferFence;
    UInt               sReadLength;

    mtdClobLocatorType sLocator = MTD_LOCATOR_NULL;
    UInt               sLobLength;
    idBool             sIsNull;
    
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
    
    if ( sIsNull == ID_TRUE )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sLanguage = aStack[1].column->language;
        sLength = 0;        

        //------------------------------------------
        // 입력된 문자열을 Language Type에 맞게 한 문자씩 읽어
        // 문자열 길이를 구함
        //------------------------------------------

        sBufferOffset = 0;
        
        while ( sBufferOffset < sLobLength )
        {
            // 버퍼를 읽는다.
            if ( sBufferOffset + MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION > sLobLength )
            {
                sBufferMount = sLobLength - sBufferOffset;
                sBufferSize = sBufferMount;
            }
            else
            {
                sBufferMount = MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION;
                sBufferSize = MTC_LOB_BUFFER_SIZE;
            }
            
            //ideLog::log( IDE_QP_0, "[character_length] offset=%d", sBufferOffset );
            
            IDE_TEST( mtc::readLob( mtc::getStatistics( aTemplate ), /* idvSQL* */
                                    sLocator,
                                    sBufferOffset,
                                    sBufferMount,
                                    sBuffer,
                                    &sReadLength )
                      != IDE_SUCCESS );

            // 버퍼에서 문자열 길이를 구한다.
            sIndex = sBuffer;
            sFence = sIndex + sBufferSize;
            sBufferFence = sIndex + sBufferMount;
            
            while ( sIndex < sFence ) 
            {
                (void)sLanguage->nextCharPtr( & sIndex, sBufferFence );
                    
                sLength++;
            }

            sBufferOffset += ( sIndex - sBuffer );            
        }
        
        *(mtdBigintType*)aStack[0].value = sLength;
    }
    
    IDE_TEST( aTemplate->closeLobLocator( sLocator )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    (void) aTemplate->closeLobLocator( sLocator );
    
    return IDE_FAILURE;
}

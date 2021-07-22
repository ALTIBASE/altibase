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
 * $Id: mtfSubstring.cpp 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfSubstring;

extern mtdModule mtdInteger;
extern mtdModule mtdBigint;
extern mtdModule mtdVarchar;
extern mtdModule mtdNvarchar;
extern mtdModule mtdClob;

static mtcName mtfSubstringFunctionName[2] = {
    { mtfSubstringFunctionName+1, 9, (void*)"SUBSTRING" },
    { NULL,              6, (void*)"SUBSTR"    }
};

static IDE_RC mtfSubstringEstimate( mtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

mtfModule mtfSubstring = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfSubstringFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfSubstringEstimate
};

static IDE_RC mtfSubstringCalculateFor2Args( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static IDE_RC mtfSubstringCalculateFor3Args( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

static IDE_RC mtfSubstringCalculateXlobLocatorFor2Args( mtcNode*     aNode,
                                                        mtcStack*    aStack,
                                                        SInt         aRemain,
                                                        void*        aInfo,
                                                        mtcTemplate* aTemplate );

static IDE_RC mtfSubstringCalculateXlobLocatorFor3Args( mtcNode*     aNode,
                                                        mtcStack*    aStack,
                                                        SInt         aRemain,
                                                        void*        aInfo,
                                                        mtcTemplate* aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfSubstringCalculateClobValueFor2Args( mtcNode     * aNode,
                                                      mtcStack    * aStack,
                                                      SInt          aRemain,
                                                      void        * aInfo,
                                                      mtcTemplate * aTemplate );

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
static IDE_RC mtfSubstringCalculateClobValueFor3Args( mtcNode     * aNode,
                                                      mtcStack    * aStack,
                                                      SInt          aRemain,
                                                      void        * aInfo,
                                                      mtcTemplate * aTemplate );

static IDE_RC mtfSubstringCalculateNcharFor3Args( mtcNode*     aNode,
                                             mtcStack*    aStack,
                                             SInt         aRemain,
                                             void*        aInfo,
                                             mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteXlobLocatorFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateXlobLocatorFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteXlobLocatorFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateXlobLocatorFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteNcharFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateNcharFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecuteClobValueFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateClobValueFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
const mtcExecute mtfExecuteClobValueFor3Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfSubstringCalculateClobValueFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfSubstringEstimate( mtcNode*     aNode,
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
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocatorFor2Args;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocatorFor3Args;
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
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocatorFor2Args;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteXlobLocatorFor3Args;
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
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteClobValueFor2Args;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteClobValueFor3Args;
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

        // PROJ-1579 NCHAR
        if( (aStack[1].column->module->id == MTD_NCHAR_ID) ||
            (aStack[1].column->module->id == MTD_NVARCHAR_ID) )
        {
            if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecuteFor2Args;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecuteNcharFor3Args;
            }
        }
        else
        {
            if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecuteFor2Args;
            }
            else
            {
                aTemplate->rows[aNode->table].execute[aNode->column] = 
                                                        mtfExecuteFor3Args;
            }
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

IDE_RC mtfSubstringCommon( const mtlModule * aLanguage,
                           mtcStack        * aStack,
                           UChar           * aResult,
                           UShort            aResultMaxLen,
                           UShort          * aResultLen,
                           UChar           * aSource,
                           UShort            aSourceLen,
                           SInt              aStart,
                           SInt              aLength )
{
/***********************************************************************
 *
 * Description : Substring
 *
 * Implementation :
 *
 ***********************************************************************/

    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UChar           * sStartIndex;
    UChar           * sEndIndex;
    UShort            sSourceLength;
    UShort            i;

    //-----------------------------------------
    // 전체 문자 개수 구함
    //-----------------------------------------        

    sSourceIndex = aSource;
    sSourceFence = sSourceIndex + aSourceLen;
    sSourceLength = 0;

    //-----------------------------------------
    // 문자열에서 시작 문자가 몇번째 문자인지 알아냄
    //-----------------------------------------

    if( aStart > 0 )
    {
        // substring('abcde', 0)이나 substring('abcde', 1)이나 같다.
        // 그지 같은 오라클 스팩이 그러해서 따라한다.
        // 내부 구현은 0부터이다.
        aStart--;
    }
    else if( aStart < 0 )
    {
        while ( sSourceIndex < sSourceFence )
        {
            (void)aLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
            sSourceLength++;
        }
        aStart = aStart + sSourceLength;
    }
    else
    {
        // nothing to do
    }

    //-----------------------------------------
    // 결과 저장
    //-----------------------------------------

    if ( aStart < 0 )
    {
        // 시작 문자 위치가 문자열의 범위를 넘어선 경우,
        // - 첫번째 문자보다도 시작 문자 위치가 작은 경우
        // - 마지막 문자보다 시작 문자 위치가 큰 경우
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        // 시작 index를 찾음
        sStartIndex = aSource;

        for ( i = 0; i < aStart; i++ )
        {
            if ( sStartIndex < sSourceFence )
            {
                (void)aLanguage->nextCharPtr( &sStartIndex, sSourceFence );
            }
            else
            {
                /* Nothing to do */
            }

            /* BUG-42544 enhancement substring function */
            if ( sStartIndex >= sSourceFence )
            {
                aStack[0].column->module->null( aStack[0].column, aStack[0].value );
                IDE_CONT( normal_exit );
            }
            else
            {
                /* Nothing to do */
            }
        }

        // 끝 index를 찾음
        sEndIndex = sStartIndex;

        for ( i = 0 ; i < aLength ; i++ )
        {
            (void)aLanguage->nextCharPtr( &sEndIndex, sSourceFence );

            if ( sEndIndex == sSourceFence )
            {
                break;
            }
            else
            {
                // nothing to do
            }
        }

        //-----------------------------------------
        // start position부터 n개의 길이 string을 copy하여 결과에 저장
        //-----------------------------------------
        
        *aResultLen = sEndIndex - sStartIndex;
        
        IDE_TEST_RAISE( *aResultLen > aResultMaxLen,
                        ERR_EXCEED_MAX );
            
        idlOS::memcpy( aResult,
                       sStartIndex,
                       *aResultLen );
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {        
        IDE_SET(ideSetErrorCode( idERR_ABORT_idnReachEnd ));
    }    
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}



IDE_RC mtfSubstringCalculateFor2Args( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Substring Calculate
 *
 * Implementation :
 *    SUBSTR( string, m )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 끝까지 모든 문자 반환
 *    aStack[1] : string ( 입력된 문자열 )
 *    aStack[2] : n ( 시작점 )
 *
 *    ex ) SUBSTR( 'SALESMAN', 6 ) ==> 'MAN'
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdIntegerType    sStart;
    UChar           * sStartIndex;
    const mtlModule * sLanguage;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    UShort            sSourceLength = 0;
    UShort            i;

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
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sStart    = *(mtdIntegerType*)aStack[2].value;
        sLanguage = aStack[1].column->language;

        //-----------------------------------------
        // 전체 문자 개수 구함
        //-----------------------------------------        

        sSourceIndex = sSource->value;
        sSourceFence = sSourceIndex + sSource->length;

        sSourceLength = 0;

        //-----------------------------------------
        // 문자열에서 시작 문자가 몇번째 문자인지 알아냄
        //-----------------------------------------

        if( sStart > 0 )
        {
            // substring('abcde', 0)이나 substring('abcde', 1)이나 같다.
            // 그지 같은 오라클 스팩이 그러해서 따라한다.
            // 내부 구현은 0부터이다.
            sStart--;
        }
        else if( sStart < 0 )
        {
            while ( sSourceIndex < sSourceFence )
            {
                (void)sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
                sSourceLength++;
            }
            // 음수인 경우
            sStart = sStart + sSourceLength;
        }
        else
        {
            // nothing to do
        }

        //-----------------------------------------
        // 결과 저장
        //-----------------------------------------
        
        if ( sStart < 0 )
        {
            // 시작 문자 위치가 문자열의 범위를 넘어선 경우,
            // - 첫번째 문자보다도 시작 문자 위치가 작은 경우
            // - 마지막 문자보다 시작 문자 위치가 큰 경우
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // 시작 문자 위치가 문자 전체 개수보다 작거나 같은  경우
            
            // 시작 index를 찾음
            sStartIndex = sSource->value;
       
            for ( i = 0; i < sStart; i++ )
            {
                if ( sStartIndex < sSourceFence )
                {
                    (void)sLanguage->nextCharPtr( &sStartIndex, sSourceFence );
                }
                else
                {
                    /* Nothing to do */
                }

                /* BUG-42544 enhancement substring function */
                if ( sStartIndex >= sSourceFence )
                {
                    aStack[0].column->module->null( aStack[0].column, aStack[0].value );
                    IDE_CONT( normal_exit );
                }
                else
                {
                    /* Nothing to do */
                }
            }

            //-----------------------------------------
            // 시작 index부터 끝까지의 string을 copy하여 결과에 저장
            //-----------------------------------------
            
            sResult->length = sSource->length - ( sStartIndex - sSource->value );
            idlOS::memcpy( sResult->value,
                           sStartIndex,
                           sResult->length );
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSubstringCalculateFor3Args( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Substring Calculate
 *
 * Implementation :
 *    SUBSTR( string, m, n )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 n개의 문자 반환
 *    aStack[1] : 입력된 문자열
 *    aStack[2] : 시작 ( eg. m )
 *    aStack[3] : 반환 문자 개수   ( eg. n )
 *
 *    ex) SUBSTR('SALESMAN', 1, 5 ) ==> 'SALES'
 *
 ***********************************************************************/
    
    mtdCharType     * sResult;
    mtdCharType     * sSource;
    mtdIntegerType    sStart;
    mtdIntegerType    sLength;
    //const mtlModule * sLanguage;

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
        sResult   = (mtdCharType*)aStack[0].value;
        sSource   = (mtdCharType*)aStack[1].value;
        sStart    = *(mtdIntegerType*)aStack[2].value;
        sLength   = *(mtdIntegerType*)aStack[3].value;
        //sLanguage = aStack[1].column->language;

        if( sLength <= 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
            return IDE_SUCCESS;
        }
        else
        {
            // nothing to do
        }

        IDE_TEST( mtfSubstringCommon( aStack[1].column->language,
                                      aStack,
                                      sResult->value,
                                      aStack[0].column->precision,
                                      &sResult->length,
                                      sSource->value,
                                      sSource->length,
                                      sStart,
                                      sLength )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSubstringCalculateXlobLocatorFor2Args( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1362
 *               Clob Substring Calculate
 *
 * Implementation :
 *    SUBSTR( string, m )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 끝까지 모든 문자 반환
 *    aStack[1] : string ( 입력된 문자열 )
 *    aStack[2] : n ( 시작점 )
 *
 *    ex ) SUBSTR( 'SALESMAN', 6 ) ==> 'MAN'
 *
 ***********************************************************************/
    
    const mtlModule  * sLanguage;
    mtdCharType      * sResult;
    mtdBigintType      sStart;
    UInt               sStartIndex;
    UShort             sSize;
    UChar            * sIndex;
    UChar            * sIndexPrev;
    UChar            * sFence;    
    UChar            * sBufferFence;    
    UChar              sBuffer[MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION];
    UInt               sBufferOffset;
    UInt               sBufferMount;
    UInt               sBufferSize;
    mtdClobLocatorType sLocator = MTD_LOCATOR_NULL;
    UInt               sLobLength;
    idBool             sIsNull;
    UInt               sReadLength;

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
                                            aStack[2].value ) == ID_TRUE ))
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdCharType*)aStack[0].value;
        sStart    = *(mtdBigintType*)aStack[2].value;
        sLanguage = aStack[1].column->language;
        
        sResult->length = 0;
        
        //-----------------------------------------
        // 문자열에서 시작 문자가 몇번째 문자인지 알아냄
        //-----------------------------------------
        
        if( sStart > 0 )
        {
            // substring('abcde', 0)이나 substring('abcde', 1)이나 같다.
            // 그지 같은 오라클 스팩이 그러해서 따라한다.
            // 내부 구현은 0부터이다.
            sStart--;
        }
        else
        {
            // Nothing to do.
        }
        
        //-----------------------------------------
        // 결과 저장
        //-----------------------------------------
        
        if ( ( sLobLength <= sStart ) || ( sStart < 0 ) )
        {
            // 시작 문자 위치가 문자열의 범위를 넘어선 경우,
            // - 첫번째 문자보다도 시작 문자 위치가 작은 경우
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // 시작 index를 찾음
            sStartIndex = 0;
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
                
                //ideLog::log( IDE_QP_0, "[substring] offset=%d", sBufferOffset );
                
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
                    sIndexPrev = sIndex;
                    
                    (void)sLanguage->nextCharPtr( & sIndex, sBufferFence );
                    
                    if ( sStartIndex >= sStart )
                    {
                        sSize = sIndex - sIndexPrev;
                        
                        IDE_TEST_RAISE( sResult->length + sSize >
                                        aStack[0].column->precision,
                                        ERR_EXCEED_MAX );
                        
                        // 복사한다.
                        idlOS::memcpy( sResult->value + sResult->length,
                                       sIndexPrev,
                                       sSize );
                        sResult->length += sSize;
                    }
                    else
                    {
                        sStartIndex++;
                    }
                }
                
                sBufferOffset += ( sIndex - sBuffer );
            }

            if ( sStartIndex < sStart )
            {
                // 시작 index를 찾지 못했다.
                aStack[0].column->module->null( aStack[0].column,
                                                aStack[0].value );
            }
            else
            {
                // Nothing to do.
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

IDE_RC mtfSubstringCalculateXlobLocatorFor3Args( mtcNode*     aNode,
                                                 mtcStack*    aStack,
                                                 SInt         aRemain,
                                                 void*        aInfo,
                                                 mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : PROJ-1362
 *               Clob Substring Calculate
 *
 * Implementation :
 *    SUBSTR( string, m, n )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 n개의 문자 반환
 *    aStack[1] : 입력된 문자열
 *    aStack[2] : 시작 ( eg. m )
 *    aStack[3] : 반환 문자 개수   ( eg. n )
 *
 *    ex) SUBSTR('SALESMAN', 1, 5 ) ==> 'SALES'
 *
 ***********************************************************************/
    
    const mtlModule  * sLanguage;
    mtdCharType      * sResult;
    mtdBigintType      sStart;
    mtdBigintType      sLength;
    UInt               sStartIndex;
    UInt               sEndIndex;
    UShort             sSize;
    UChar            * sIndex;
    UChar            * sIndexPrev;
    UChar            * sFence;    
    UChar            * sBufferFence;    
    UChar              sBuffer[MTC_LOB_BUFFER_SIZE + MTL_MAX_PRECISION];
    UInt               sBufferOffset;
    UInt               sBufferMount;
    UInt               sBufferSize;
    mtdClobLocatorType sLocator = MTD_LOCATOR_NULL;
    UInt               sLobLength;
    idBool             sIsNull;
    UInt               sReadLength;

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
        sResult   = (mtdCharType*)aStack[0].value;
        sStart    = *(mtdBigintType*)aStack[2].value;
        sLength   = *(mtdBigintType*)aStack[3].value;
        sLanguage = aStack[1].column->language;
        
        sResult->length = 0;
        
        if( sLength <= 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            //-----------------------------------------
            // 문자열에서 시작 문자가 몇번째 문자인지 알아냄
            //-----------------------------------------
        
            if( sStart > 0 )
            {
                // substring('abcde', 0)이나 substring('abcde', 1)이나 같다.
                // 그지 같은 오라클 스팩이 그러해서 따라한다.
                // 내부 구현은 0부터이다.
                sStart--;
            }
            else
            {
                // Nothing to do.
            }
        
            //-----------------------------------------
            // 결과 저장
            //-----------------------------------------
        
            if ( ( sLobLength <= sStart ) || ( sStart < 0 ) )
            {
                // 시작 문자 위치가 문자열의 범위를 넘어선 경우,
                // - 첫번째 문자보다도 시작 문자 위치가 작은 경우
                aStack[0].column->module->null( aStack[0].column,
                                                aStack[0].value );
            }
            else
            {
                // 시작 index를 찾음
                sStartIndex = 0;
                sEndIndex = 0;
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
                
                    //ideLog::log( IDE_QP_0, "[substring] offset=%d", sBufferOffset );
                    
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
                        sIndexPrev = sIndex;
                        
                        (void)sLanguage->nextCharPtr( & sIndex, sBufferFence );
                        
                        if ( sStartIndex >= sStart )
                        {
                            sSize = sIndex - sIndexPrev;
                            
                            IDE_TEST_RAISE( sResult->length + sSize >
                                            aStack[0].column->precision,
                                            ERR_EXCEED_MAX );
                            
                            // 복사한다.
                            idlOS::memcpy( sResult->value + sResult->length,
                                           sIndexPrev,
                                           sSize );
                            sResult->length += sSize;

                            sEndIndex++;
                            
                            if ( sEndIndex >= sLength )
                            {
                                break;
                            }
                            else
                            {
                                // Nothing to do.
                            }
                        }
                        else
                        {
                            sStartIndex++;
                        }
                    }
                    
                    if ( sEndIndex >= sLength )
                    {
                        break;
                    }
                    else
                    {
                        // Nothing to do.
                    }
                            
                    sBufferOffset += ( sIndex - sBuffer );
                }
                
                if ( sStartIndex < sStart )
                {
                    // 시작 index를 찾지 못했다.
                    aStack[0].column->module->null( aStack[0].column,
                                                    aStack[0].value );
                }
                else
                {
                    // Nothing to do.
                }
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

IDE_RC mtfSubstringCalculateNcharFor3Args( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Substring Calculate for NCHAR
 *
 * Implementation :
 *    SUBSTR( string, m, n )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 n개의 문자 반환
 *    aStack[1] : 입력된 문자열
 *    aStack[2] : 시작 ( eg. m )
 *    aStack[3] : 반환 문자 개수   ( eg. n )
 *
 *    ex) SUBSTR('SALESMAN', 1, 5 ) ==> 'SALES'
 *
 ***********************************************************************/
    
    const mtlModule  * sLanguage;
    mtdNcharType     * sResult;
    mtdNcharType     * sSource;
    mtdIntegerType     sStart;
    mtdIntegerType     sLength;
    UShort             sResultMaxLen;

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
        sResult   = (mtdNcharType*)aStack[0].value;
        sSource   = (mtdNcharType*)aStack[1].value;
        sStart    = *(mtdIntegerType*)aStack[2].value;
        sLength   = *(mtdIntegerType*)aStack[3].value;
        sLanguage = aStack[1].column->language;

        if( sLength <= 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
            return IDE_SUCCESS;
        }
        else
        {
            // nothing to do
        }

        sResultMaxLen = sLanguage->maxPrecision(aStack[0].column->precision);

        IDE_TEST( mtfSubstringCommon( aStack[1].column->language,
                                      aStack,
                                      sResult->value,
                                      sResultMaxLen,
                                      &sResult->length,
                                      sSource->value,
                                      sSource->length,
                                      sStart,
                                      sLength )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSubstringCalculateClobValueFor2Args( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Substring Calculate For CLOB Value
 *
 * Implementation :
 *    SUBSTR( clob_value, m )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 끝까지 모든 문자 반환
 *    aStack[1] : clob_value ( 입력된 문자열 )
 *    aStack[2] : m ( 시작점 )
 *
 *    ex ) SUBSTR( CLOB'SALESMAN', 6 ) ==> 'MAN'
 *
 ***********************************************************************/

    mtdCharType     * sResult;
    mtdClobType     * sSource;
    mtdBigintType     sStart;
    const mtlModule * sLanguage;
    UChar           * sStartIndex;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    SLong             sSourceLength = 0;
    SLong             sCopySize;
    SLong             i;

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
        sResult   = (mtdCharType *)aStack[0].value;
        sSource   = (mtdClobType *)aStack[1].value;
        sStart    = *(mtdBigintType *)aStack[2].value;
        sLanguage = aStack[1].column->language;

        //-----------------------------------------
        // 전체 문자 개수 구함
        //-----------------------------------------

        sSourceIndex = sSource->value;
        sSourceFence = sSourceIndex + sSource->length;

        sSourceLength = 0;

        //-----------------------------------------
        // 문자열에서 시작 문자가 몇번째 문자인지 알아냄
        //-----------------------------------------

        if ( sStart > 0 )
        {
            // substring('abcde', 0)이나 substring('abcde', 1)이나 같다.
            // 그지 같은 오라클 스팩이 그러해서 따라한다.
            // 내부 구현은 0부터이다.
            sStart--;
        }
        else if ( sStart < 0 )
        {
            while ( sSourceIndex < sSourceFence )
            {
                (void)sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
                sSourceLength++;
            }
            // 음수인 경우
            sStart = sStart + sSourceLength;
        }
        else
        {
            // nothing to do
        }

        //-----------------------------------------
        // 결과 저장
        //-----------------------------------------

        if ( sStart < 0 )
        {
            // 시작 문자 위치가 문자열의 범위를 넘어선 경우,
            // - 첫번째 문자보다도 시작 문자 위치가 작은 경우
            // - 마지막 문자보다 시작 문자 위치가 큰 경우
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // 시작 문자 위치가 문자 전체 개수보다 작거나 같은  경우

            // 시작 index를 찾음
            sStartIndex = sSource->value;

            for ( i = 0; i < sStart; i++ )
            {
                if ( sStartIndex < sSourceFence )
                {
                    (void)sLanguage->nextCharPtr( &sStartIndex, sSourceFence );
                }
                else
                {
                    /* Nothing to do */
                }

                /* BUG-42544 enhancement substring function */
                if ( sStartIndex >= sSourceFence )
                {
                    aStack[0].column->module->null( aStack[0].column, aStack[0].value );
                    IDE_CONT( normal_exit );
                }
                else
                {
                    /* Nothing to do */
                }
            }

            //-----------------------------------------
            // 시작 index부터 끝까지의 string을 copy하여 결과에 저장
            //-----------------------------------------

            sCopySize = sSource->length - (sStartIndex - sSource->value);
            IDE_TEST_RAISE( sCopySize > (SLong) aStack[0].column->precision,
                            ERR_EXCEED_MAX );

            sResult->length = (UShort)sCopySize;
            idlOS::memcpy( sResult->value,
                           sStartIndex,
                           sResult->length );
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_idnReachEnd ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfSubstringCalculateClobValueFor3Args( mtcNode     * aNode,
                                               mtcStack    * aStack,
                                               SInt          aRemain,
                                               void        * aInfo,
                                               mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : Substring Calculate For CLOB Value
 *
 * Implementation :
 *    SUBSTR( clob_value, m, n )
 *
 *    aStack[0] : 입력된 문자열의 m번째 문자부터 n개의 문자 반환
 *    aStack[1] : clob_value ( 입력된 문자열 )
 *    aStack[2] : m ( 시작점 )
 *    aStack[3] : n ( 반환 문자 수 )
 *
 *    ex ) SUBSTR( CLOB'SALESMAN', 1, 5 ) ==> 'SALES'
 *
 ***********************************************************************/

    mtdCharType     * sResult;
    mtdClobType     * sSource;
    mtdBigintType     sStart;
    mtdBigintType     sLength;
    const mtlModule * sLanguage;
    UChar           * sStartIndex;
    UChar           * sEndIndex;
    UChar           * sSourceIndex;
    UChar           * sSourceFence;
    SLong             sSourceLength = 0;
    SLong             sCopySize;
    SLong             i;

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
        sResult   = (mtdCharType *)aStack[0].value;
        sSource   = (mtdClobType *)aStack[1].value;
        sStart    = *(mtdBigintType *)aStack[2].value;
        sLength   = *(mtdBigintType *)aStack[3].value;
        sLanguage = aStack[1].column->language;

        if ( sLength <= 0 )
        {
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
            return IDE_SUCCESS;
        }
        else
        {
            // nothing to do
        }

        //-----------------------------------------
        // 전체 문자 개수 구함
        //-----------------------------------------

        sSourceIndex = sSource->value;
        sSourceFence = sSourceIndex + sSource->length;

        sSourceLength = 0;

        //-----------------------------------------
        // 문자열에서 시작 문자가 몇번째 문자인지 알아냄
        //-----------------------------------------

        if ( sStart > 0 )
        {
            // substring('abcde', 0)이나 substring('abcde', 1)이나 같다.
            // 그지 같은 오라클 스팩이 그러해서 따라한다.
            // 내부 구현은 0부터이다.
            sStart--;
        }
        else if ( sStart < 0 )
        {
            while ( sSourceIndex < sSourceFence )
            {
                (void)sLanguage->nextCharPtr( &sSourceIndex, sSourceFence );
                sSourceLength++;
            }
            // 음수인 경우
            sStart = sStart + sSourceLength;
        }
        else
        {
            // nothing to do
        }

        //-----------------------------------------
        // 결과 저장
        //-----------------------------------------

        if ( sStart < 0 )
        {
            // 시작 문자 위치가 문자열의 범위를 넘어선 경우,
            // - 첫번째 문자보다도 시작 문자 위치가 작은 경우
            // - 마지막 문자보다 시작 문자 위치가 큰 경우
            aStack[0].column->module->null( aStack[0].column,
                                            aStack[0].value );
        }
        else
        {
            // 시작 문자 위치가 문자 전체 개수보다 작거나 같은  경우

            // 시작 index를 찾음
            sStartIndex = sSource->value;

            for ( i = 0; i < sStart; i++ )
            {
                if ( sStartIndex < sSourceFence )
                {
                    (void)sLanguage->nextCharPtr( &sStartIndex, sSourceFence );
                }
                else
                {
                    /* Nothing to do */
                }

                /* BUG-42544 enhancement substring function */
                if ( sStartIndex >= sSourceFence )
                {
                    aStack[0].column->module->null( aStack[0].column, aStack[0].value );
                    IDE_CONT( normal_exit );
                }
                else
                {
                    /* Nothing to do */
                }
            }

            // 끝 index를 찾음
            sEndIndex = sStartIndex;

            for ( i = 0 ; i < sLength ; i++ )
            {
                (void)sLanguage->nextCharPtr( &sEndIndex, sSourceFence );

                if ( sEndIndex == sSourceFence )
                {
                    break;
                }
                else
                {
                    // nothing to do
                }
            }

            //-----------------------------------------
            // 시작 index부터 n개의 길이 string을 copy하여 결과에 저장
            //-----------------------------------------

            sCopySize = sEndIndex - sStartIndex;
            IDE_TEST_RAISE( sCopySize > (SLong) aStack[0].column->precision,
                            ERR_EXCEED_MAX );

            sResult->length = (UShort)sCopySize;
            idlOS::memcpy( sResult->value,
                           sStartIndex,
                           sResult->length );
        }
    }

    IDE_EXCEPTION_CONT( normal_exit );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_EXCEED_MAX );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_idnReachEnd ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


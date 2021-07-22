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
 * $Id: mtfRegExpInstr.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <idErrorCode.h>
#include <mtdTypes.h>

#include <mtfRegExp.h>

extern mtdModule mtdInteger;
extern mtdModule mtdBinary;
extern mtdModule mtdVarchar;

static mtcName mtfRegExpInstrFunctionName[1] = {
    { NULL, 12, (void*)"REGEXP_INSTR" }
};

extern IDE_RC mtfCompileExpression( mtcNode            * aPatternNode,
                                    mtcTemplate        * aTemplate,
                                    mtfRegExpression  ** aCompiledExpression,
                                    mtcCallBack        * aCallBack );

static IDE_RC mtfRegExpInstrEstimate( mtcNode*     aNode,
                                      mtcTemplate* aTemplate,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      mtcCallBack* aCallBack );

static UShort mtfRegExpGetCharCount( const mtlModule*  aLanguage,
                                     const UChar*      aSource,
                                     UShort            aLength );

IDE_RC mtfRegExpFindPosition( const mtlModule*  aLanguage,
                              UShort*           aPosition,
                              mtfRegExpression* aExp,
                              const UChar*      aSource,
                              UShort            aSourceLen,
                              SShort            aStart,
                              UShort            aOccurrence );

mtfModule mtfRegExpInstr = {
    2|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfRegExpInstrFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRegExpInstrEstimate
};

static IDE_RC mtfRegExpInstrCalculateFor2Args( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfRegExpInstrCalculateFor3Args( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfRegExpInstrCalculateFor4Args( mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfRegExpInstrCalculateFor2ArgsFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

static IDE_RC mtfRegExpInstrCalculateFor3ArgsFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

static IDE_RC mtfRegExpInstrCalculateFor4ArgsFast( mtcNode*     aNode,
                                                   mtcStack*    aStack,
                                                   SInt         aRemain,
                                                   void*        aInfo,
                                                   mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRegExpInstrCalculateFor2Args,
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
    mtfRegExpInstrCalculateFor3Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor4Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRegExpInstrCalculateFor4Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfRegExpInstrEstimate( mtcNode*     aNode,
                               mtcTemplate* aTemplate,
                               mtcStack*    aStack,
                               SInt      /* aRemain */,
                               mtcCallBack* aCallBack )
{
    const mtdModule   * sModules[4];
    
    UInt                sPrecision;
    mtcNode           * sPatternNode;
    mtfRegExpression  * sCompiledExpression;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 2 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 4,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdVarchar;
    sModules[1] = sModules[0];
    sModules[2] = &mtdInteger;
    sModules[3] = &mtdInteger;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    /* 결과를 저장함 */
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    /* regexp의 compiled pattern을 저장함 */
    sPrecision = MTF_REG_EXPRESSION_SIZE( aStack[2].column->precision );
    
    IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                     & mtdBinary,
                                     1,
                                     sPrecision,
                                     0 )
              != IDE_SUCCESS );
    
    if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
         == MTC_NODE_REESTIMATE_FALSE )
    {
        if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor2Args;
        }
        else if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor3Args;
        }
        else if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 4 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                                                        mtfExecuteFor4Args;
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        sPatternNode = mtf::convertedNode( aNode->arguments->next,
                                           aTemplate );

        if ( ( sPatternNode == aNode->arguments->next )
             &&
             ( ( aTemplate->rows[sPatternNode->table].lflag & MTC_TUPLE_TYPE_MASK )
               == MTC_TUPLE_TYPE_CONSTANT ) )
        {
            IDE_TEST ( mtfCompileExpression( sPatternNode,
                                             aTemplate,
                                             &sCompiledExpression,
                                             aCallBack )
                       != IDE_SUCCESS );
            
            if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 2 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                    mtfRegExpInstrCalculateFor2ArgsFast;
            }
            else if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 3 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                    mtfRegExpInstrCalculateFor3ArgsFast;
            }
            else if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 4 )
            {
                aTemplate->rows[aNode->table].execute[aNode->column].calculate =
                    mtfRegExpInstrCalculateFor4ArgsFast;
            }
            else
            {
                // nothing to do
            }
            
            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo =
                sCompiledExpression;

            // 더이상 사용하지 않음
            IDE_TEST( mtc::initializeColumn( aStack[0].column + 1,
                                             & mtdBinary,
                                             1,
                                             0,
                                             0 )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do.
        }
    }
    
    if ( ( MTC_NODE_IS_DEFINED_VALUE( aNode->arguments->next ) == ID_TRUE )
         &&
         ( ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK )
             == MTC_TUPLE_TYPE_CONSTANT ) ||
           ( ( aTemplate->rows[aNode->arguments->next->table].lflag
               & MTC_TUPLE_TYPE_MASK )
             == MTC_TUPLE_TYPE_INTERMEDIATE ) ) )
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_TRUE;
            
        // BUG-38070 undef type으로 re-estimate하지 않는다.
        if ( ( aTemplate->variableRow != ID_USHORT_MAX ) &&
             ( ( aNode->lflag & MTC_NODE_BIND_MASK ) == MTC_NODE_BIND_EXIST ) )
        {
            if ( aTemplate->rows[aTemplate->variableRow].
                 columns->module->id == MTD_UNDEF_ID )
            {
                aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
                aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
            }
            else
            {
                // nothing to do
            }
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        aNode->lflag &= ~MTC_NODE_REESTIMATE_MASK;
        aNode->lflag |= MTC_NODE_REESTIMATE_FALSE;
    }

    /* BUG-44740 mtfRegExpression 재사용을 위해 Tuple Row를 초기화한다. */
    aTemplate->rows[aNode->table].lflag &= ~MTC_TUPLE_ROW_MEMSET_MASK;
    aTemplate->rows[aNode->table].lflag |= MTC_TUPLE_ROW_MEMSET_TRUE;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRegExpFindPosition( const mtlModule*  aLanguage,
                              UShort*           aPosition,
                              mtfRegExpression* aExp,
                              const UChar*      aSource,
                              UShort            aSourceLen,
                              SShort            aStart,
                              UShort            aOccurrence )
{
    const SChar *sBeginStr;
    const SChar *sEndStr;
    
    UChar       *sSourceIndex;
    UChar       *sSourceFence;
    UShort       sCnt = 1;
    UShort       sPosition = 0;
    SShort       sStart;
    
    *aPosition = 0;
    
    // 탐색을 시작할 위치 조정
    sSourceIndex = (UChar*) aSource;
    sSourceFence = sSourceIndex + aSourceLen;
    
    sStart = aStart;
    while ( sStart - 1 > 0 )
    {
        // TASK-3420 문자열 처리 정책 개선
        (void)aLanguage->nextCharPtr( & sSourceIndex, sSourceFence );
        
        --sStart;
    }
    
    // BUG-45386
    // 'ABC', 'B*' 이런 경우 endOfLine에서도 매치되어야 한다.
    // 따라서 sSourceIndex가 sSourceFence인 경우에도 search를 한다.
    while ( sSourceIndex <= sSourceFence )
    {
        if ( mtfRegExp::search( aExp,
                                (const SChar *)sSourceIndex,
                                aSourceLen - (UShort)( (vULong)sSourceIndex - (vULong)aSource ),
                                &sBeginStr,
                                &sEndStr ) == ID_TRUE )
        {
            /* 'ABCDEF','B*' 이러한 경우 sEndStr이 주어진 sSourceIndex와 같을수 있다. */
            if ( sSourceIndex != (UChar*)sEndStr )
            {
                sSourceIndex = (UChar*)sEndStr;
            }
            else
            {
                sSourceIndex++;
            }
        
            if ( aOccurrence == sCnt )
            {
                sPosition = (UShort)( (vULong)sBeginStr - (vULong)aSource ) + 1;
                break;
            }
            else
            {
                sCnt++;
            }
        }
        else
        {
            break;
        }
    }
    
    /* 글자수를 구한다. */
    if ( sPosition > 0 )
    {
        if ( aSourceLen < sPosition )
        {
            *aPosition = mtfRegExpGetCharCount( aLanguage, aSource, aSourceLen ) + 1;
        }
        else
        {
            *aPosition = mtfRegExpGetCharCount( aLanguage, aSource, sPosition );
        }
    }
    else
    {
        /* Nothing to do */
    }
    
    return IDE_SUCCESS;
}

/* aLength 까지의 문자 수를 반환 한다. */
UShort mtfRegExpGetCharCount( const mtlModule*  aLanguage,
                              const UChar*      aSource,
                              UShort            aLength )
{
    UChar  * sSrcCharIndex;
    UChar  * sSrcFence;
    UShort   sCharCount = 0;

    sSrcCharIndex = (UChar *)aSource;
    sSrcFence     = sSrcCharIndex + aLength;
    
    while ( sSrcCharIndex < sSrcFence )
    {
        (void)aLanguage->nextCharPtr( &sSrcCharIndex, sSrcFence );
        sCharCount++;
    }
    
    return sCharCount;
}

IDE_RC mtfRegExpInstrCalculateFor2Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    mtdCharType      * sVarchar1;
    mtdCharType      * sVarchar2;
    UShort             sPosition;
    
    const mtcColumn  * sColumn;
    mtdBinaryType    * sTempValue;
    mtfRegExpression * sCompiledExpression;
    UChar            * sPattern;
    UShort             sPatternLength;

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
        sVarchar1 = (mtdCharType*)aStack[1].value;
        sVarchar2 = (mtdCharType*)aStack[2].value;

        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sTempValue      = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);
        sCompiledExpression = (mtfRegExpression*)(sTempValue->mValue);

        sPattern = sVarchar2->value;
        sPatternLength = sVarchar2->length;

        /* BUG-45213 valgrin warning
         * SortTemp와 같이 mtrNode로 쌓일 경우 항상 새로운 Row가
         * 할당되므로 이전 sCompiledExpression->patternLen 비교해봐야
         * 의미가 없다 따라서 이럴경우 그냥 Compile한다.
         */
        if ( ( aTemplate->rows[aNode->table].lflag & MTC_TUPLE_PLAN_MTR_MASK )
             == MTC_TUPLE_PLAN_MTR_TRUE )
        {
            IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                             (const SChar *)sPattern,
                                             sPatternLength )
                      != IDE_SUCCESS );
        }
        else
        {
            // pattern compile
            if ( sPatternLength != sCompiledExpression->patternLen )
            {
                IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                 (const SChar *)sPattern,
                                                 sPatternLength )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( idlOS::memcmp( sPattern,
                                    sCompiledExpression->patternBuf,
                                    sPatternLength ) != 0 )
                {
                    IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                     (const SChar *)sPattern,
                                                     sPatternLength )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        IDE_TEST( mtfRegExpFindPosition( aStack[1].column->language,
                                         &sPosition,
                                         sCompiledExpression,
                                         sVarchar1->value,
                                         sVarchar1->length,
                                         1,
                                         1 )
                 != IDE_SUCCESS )
        
        *(mtdIntegerType*)aStack[0].value = sPosition;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRegExpInstrCalculateFor3Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    mtdCharType      * sVarchar1;
    mtdCharType      * sVarchar2;
    UShort             sPosition;
    SInt               sStart = 0;
    SInt               sSourceCharCount = 0;
    
    const mtcColumn  * sColumn;
    mtdBinaryType    * sTempValue;
    mtfRegExpression * sCompiledExpression;
    UChar            * sPattern;
    UShort             sPatternLength;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sVarchar1 = (mtdCharType*)aStack[1].value;
        sVarchar2 = (mtdCharType*)aStack[2].value;
        sStart = *(mtdIntegerType*)aStack[3].value;

        IDE_TEST_RAISE( ( sStart <= 0 ),
                        ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );

        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sTempValue      = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);
        sCompiledExpression = (mtfRegExpression*)(sTempValue->mValue);

        sPattern = sVarchar2->value;
        sPatternLength = sVarchar2->length;

        // Source의 문자 개수를 구함
        sSourceCharCount = mtfRegExpGetCharCount( aStack[1].column->language,
                                                  sVarchar1->value,
                                                  sVarchar1->length );

        /* BUG-45213 valgrin warning
         * SortTemp와 같이 mtrNode로 쌓일 경우 항상 새로운 Row가
         * 할당되므로 이전 sCompiledExpression->patternLen 비교해봐야
         * 의미가 없다 따라서 이럴경우 그냥 Compile한다.
         */
        if ( ( aTemplate->rows[aNode->table].lflag & MTC_TUPLE_PLAN_MTR_MASK )
             == MTC_TUPLE_PLAN_MTR_TRUE )
        {
            IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                             (const SChar *)sPattern,
                                             sPatternLength )
                      != IDE_SUCCESS );
        }
        else
        {
            // pattern compile
            if ( sPatternLength != sCompiledExpression->patternLen )
            {
                IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                 (const SChar *)sPattern,
                                                 sPatternLength )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( idlOS::memcmp( sPattern,
                                    sCompiledExpression->patternBuf,
                                    sPatternLength ) != 0 )
                {
                    IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                     (const SChar *)sPattern,
                                                     sPatternLength )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        /* BUG-34232
         * sStart( 탐색을 시작할 위치 )가 입력받은 문자열 길이보다 큰 경우,
         * 에러를 발생시키는 대신 0을 리턴하도록 수정 */
        if ( sStart > sSourceCharCount )
        {
            sPosition = 0;
        }
        else
        {
            IDE_TEST( mtfRegExpFindPosition( aStack[1].column->language,
                                             &sPosition,
                                             sCompiledExpression,
                                             sVarchar1->value,
                                             sVarchar1->length,
                                             sStart,
                                             1 )
                      != IDE_SUCCESS )
        }

        *(mtdIntegerType*)aStack[0].value = sPosition;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                              sStart ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfRegExpInstrCalculateFor4Args( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate )
{
    mtdCharType      * sVarchar1;
    mtdCharType      * sVarchar2;
    UShort             sPosition;
    SInt               sStart = 0;
    SInt               sOccurrence = 0;
    SInt               sSourceCharCount = 0;
    
    const mtcColumn  * sColumn;
    mtdBinaryType    * sTempValue;
    mtfRegExpression * sCompiledExpression;
    UChar            * sPattern;
    UShort             sPatternLength;

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
        sVarchar1 = (mtdCharType*)aStack[1].value;
        sVarchar2 = (mtdCharType*)aStack[2].value;
        sStart = *(mtdIntegerType*)aStack[3].value;
        sOccurrence = *(mtdIntegerType*)aStack[4].value;

        IDE_TEST_RAISE( ( sStart <= 0 ),
                        ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );

        IDE_TEST_RAISE ( ( sOccurrence <= 0 ),
                         ERR_ARGUMENT4_VALUE_OUT_OF_RANGE );

        sColumn = aTemplate->rows[aNode->table].columns + aNode->column;
        sTempValue      = (mtdBinaryType*)
        ((UChar*)aTemplate->rows[aNode->table].row + sColumn[1].column.offset);
        sCompiledExpression = (mtfRegExpression*)(sTempValue->mValue);

        sPattern = sVarchar2->value;
        sPatternLength = sVarchar2->length;

        // Source의 문자 개수를 구함
        sSourceCharCount = mtfRegExpGetCharCount( aStack[1].column->language,
                                                  sVarchar1->value,
                                                  sVarchar1->length );

        /* BUG-45213 valgrin warning
         * SortTemp와 같이 mtrNode로 쌓일 경우 항상 새로운 Row가
         * 할당되므로 이전 sCompiledExpression->patternLen 비교해봐야
         * 의미가 없다 따라서 이럴경우 그냥 Compile한다.
         */
        if ( ( aTemplate->rows[aNode->table].lflag & MTC_TUPLE_PLAN_MTR_MASK )
             == MTC_TUPLE_PLAN_MTR_TRUE )
        {
            IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                             (const SChar *)sPattern,
                                             sPatternLength )
                      != IDE_SUCCESS );
        }
        else
        {
            // pattern compile
            if ( sPatternLength != sCompiledExpression->patternLen )
            {
                IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                 (const SChar *)sPattern,
                                                 sPatternLength )
                          != IDE_SUCCESS );
            }
            else
            {
                if ( idlOS::memcmp( sPattern,
                                    sCompiledExpression->patternBuf,
                                    sPatternLength ) != 0 )
                {
                    IDE_TEST( mtfRegExp::expCompile( sCompiledExpression,
                                                     (const SChar *)sPattern,
                                                     sPatternLength )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }
            }
        }

        /* BUG-34232
         * sStart( 탐색을 시작할 위치 )가 입력받은 문자열 길이보다 큰 경우,
         * 에러를 발생시키는 대신 0을 리턴하도록 수정 */
        if ( sStart > sSourceCharCount )
        {
            sPosition = 0;
        }
        else
        {
            IDE_TEST( mtfRegExpFindPosition( aStack[1].column->language,
                                             &sPosition,
                                             sCompiledExpression,
                                             sVarchar1->value,
                                             sVarchar1->length,
                                             sStart,
                                             sOccurrence )
                      != IDE_SUCCESS )
        }

        *(mtdIntegerType*)aStack[0].value = sPosition;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                              sStart ) );

    IDE_EXCEPTION( ERR_ARGUMENT4_VALUE_OUT_OF_RANGE );
    {
        IDE_SET( ideSetErrorCode(mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                                 sOccurrence ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC mtfRegExpInstrCalculateFor2ArgsFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    mtdCharType      * sVarchar1;
    UShort             sPosition;

    mtfRegExpression * sCompiledExpression;
    
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
        sVarchar1 = (mtdCharType*)aStack[1].value;
        
        IDE_DASSERT( aInfo != NULL );
        sCompiledExpression = (mtfRegExpression*)aInfo;
        
        IDE_TEST( mtfRegExpFindPosition( aStack[1].column->language,
                                         &sPosition,
                                         sCompiledExpression,
                                         sVarchar1->value,
                                         sVarchar1->length,
                                         1,
                                         1 )
                  != IDE_SUCCESS )
        
        *(mtdIntegerType*)aStack[0].value = sPosition;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRegExpInstrCalculateFor3ArgsFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    mtdCharType      * sVarchar1;
    UShort             sPosition;
    SInt               sStart = 0;
    SInt               sSourceCharCount = 0;
    
    mtfRegExpression * sCompiledExpression;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sVarchar1 = (mtdCharType*)aStack[1].value;
        sStart = *(mtdIntegerType*)aStack[3].value;
        
        IDE_TEST_RAISE( ( sStart <= 0 ),
                        ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );
        
        IDE_DASSERT( aInfo != NULL );
        sCompiledExpression = (mtfRegExpression*)aInfo;

        // Source의 문자 개수를 구함
        sSourceCharCount = mtfRegExpGetCharCount( aStack[1].column->language,
                                                  sVarchar1->value,
                                                  sVarchar1->length );
        
        /* BUG-34232
         * sStart( 탐색을 시작할 위치 )가 입력받은 문자열 길이보다 큰 경우,
         * 에러를 발생시키는 대신 0을 리턴하도록 수정 */
        if ( sStart > sSourceCharCount )
        {
            sPosition = 0;
        }
        else
        {
            IDE_TEST( mtfRegExpFindPosition( aStack[1].column->language,
                                             &sPosition,
                                             sCompiledExpression,
                                             sVarchar1->value,
                                             sVarchar1->length,
                                             sStart,
                                             1 )
                      != IDE_SUCCESS )
        }
        
        *(mtdIntegerType*)aStack[0].value = sPosition;
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                              sStart ) );
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfRegExpInstrCalculateFor4ArgsFast( mtcNode*     aNode,
                                            mtcStack*    aStack,
                                            SInt         aRemain,
                                            void*        aInfo,
                                            mtcTemplate* aTemplate )
{
    mtdCharType      * sVarchar1;
    UShort             sPosition;
    SInt               sStart = 0;
    SInt               sOccurrence = 0;
    SInt               sSourceCharCount = 0;
    
    mtfRegExpression * sCompiledExpression;
    
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
        sVarchar1 = (mtdCharType*)aStack[1].value;

        sStart = *(mtdIntegerType*)aStack[3].value;
        sOccurrence = *(mtdIntegerType*)aStack[4].value;

        IDE_TEST_RAISE( ( sStart <= 0 ),
                        ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );

        IDE_TEST_RAISE ( ( sOccurrence <= 0 ),
                         ERR_ARGUMENT4_VALUE_OUT_OF_RANGE );
        
        IDE_DASSERT( aInfo != NULL );
        sCompiledExpression = (mtfRegExpression*)aInfo;

        // Source의 문자 개수를 구함
        sSourceCharCount = mtfRegExpGetCharCount( aStack[1].column->language,
                                                  sVarchar1->value,
                                                  sVarchar1->length );
        
        /* BUG-34232
         * sStart( 탐색을 시작할 위치 )가 입력받은 문자열 길이보다 큰 경우,
         * 에러를 발생시키는 대신 0을 리턴하도록 수정 */
        if ( sStart > sSourceCharCount )
        {
            sPosition = 0;
        }
        else
        {
                IDE_TEST( mtfRegExpFindPosition( aStack[1].column->language,
                                                 &sPosition,
                                                 sCompiledExpression,
                                                 sVarchar1->value,
                                                 sVarchar1->length,
                                                 sStart,
                                                 sOccurrence )
                          != IDE_SUCCESS )
        }
        
        *(mtdIntegerType*)aStack[0].value = sPosition;
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT3_VALUE_OUT_OF_RANGE );
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                              sStart ) );
    
    IDE_EXCEPTION( ERR_ARGUMENT4_VALUE_OUT_OF_RANGE );
    IDE_SET( ideSetErrorCode(mtERR_ABORT_ARGUMENT_VALUE_OUT_OF_RANGE,
                             sOccurrence ) );
 
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

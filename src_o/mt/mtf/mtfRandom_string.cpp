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
 * $Id: mtfRandom_string.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

/***********************************************************************
 * BUG-39461 support RANDOM_STRING Function
 *
 * RANDOM_STRING( aOption, aLength )
 *
 * aOption
 * 'u', 'U' - alpha characters 중 대문자로 Random 하게 생성
 * 'l', 'L' - alpha characters 중 소문자로 Random 하게 생성
 * 'a', 'A' - 대 소 문자를 구분하지 않고 Random 하게 생성
 * 'x', 'X' - 대분자와 숫자로 Random 하게 생성
 * 'p', 'P' - ASCII Code 0x20(' ') ~ 0x7E( ~ ) 까지 95 개의 문자
 *
 * aLength - 생성할 문자 길이( MAX 4000 )
 *
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtv.h>
#include <mtk.h>

extern mtfModule mtfRandomString;
extern mtdModule mtdVarchar;
extern mtdModule mtdChar;
extern mtdModule mtdInteger;

static mtcName mtfRandomStringFunctionName[1] = {
    { NULL, 13, (void *)"RANDOM_STRING" }
};

static IDE_RC mtfRandomStringEstimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

mtfModule mtfRandomString = {
    1 | MTC_NODE_OPERATOR_FUNCTION | MTC_NODE_VARIABLE_TRUE,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfRandomStringFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfRandomStringEstimate
};

static IDE_RC mtfRandomStringCalculateFast( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate );

static IDE_RC mtfRandomStringCalculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfRandomStringCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

#define RANDOM_STRING_MAX       (4000)
#define RANDOM_STRING_OPTION_U  (1)
#define RANDOM_STRING_OPTION_L  (2)
#define RANDOM_STRING_OPTION_A  (3)
#define RANDOM_STRING_OPTION_X  (4)
#define RANDOM_STRING_OPTION_P  (5)

static IDE_RC mtfRandomStringGetOption( mtcColumn   * aOptColumn,
                                        void        * aOptValue,
                                        vSLong      * aOption )
{
    vSLong        sOption;
    mtdCharType * sOptValue;

    sOptValue = ( mtdCharType * )aOptValue;

    /* aOption 은 기본적으로 'u' option을 사용한다 */
    if ( aOptColumn->module->isNull( aOptColumn, aOptValue ) == ID_TRUE )
    {
        sOption = RANDOM_STRING_OPTION_U;
    }
    else
    {
        IDE_TEST_RAISE( sOptValue->length != 1, ERR_INVALID_FUNCTION_ARGUMENT );

        if ( ( sOptValue->value[0] == 'l' ) || ( sOptValue->value[0] == 'L' ) )
        {
            sOption = RANDOM_STRING_OPTION_L;
        }
        else if ( ( sOptValue->value[0] == 'a' ) || ( sOptValue->value[0] == 'A' ) )
        {
            sOption = RANDOM_STRING_OPTION_A;
        }
        else if ( ( sOptValue->value[0] == 'x' ) || ( sOptValue->value[0] == 'X' ) )
        {
            sOption = RANDOM_STRING_OPTION_X;
        }
        else if ( ( sOptValue->value[0] == 'p' ) || ( sOptValue->value[0] == 'P' ) )
        {
            sOption = RANDOM_STRING_OPTION_P;
        }
        else
        {
            sOption = RANDOM_STRING_OPTION_U;
        }
    }

    *aOption = sOption;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtfRandomStringGetLength( mtcColumn      * aLengthColumn,
                                        void           * aLengthValue,
                                        SInt           * aLength )
{
    mtdIntegerType sLength = 0;

    /* Length 는 Null일 경우 Error 처리 */
    if ( aLengthColumn->module->isNull( aLengthColumn, aLengthValue ) == ID_TRUE )
    {
        IDE_RAISE( ERR_INVALID_FUNCTION_ARGUMENT );
    }
    else
    {
        sLength = *(mtdIntegerType *)aLengthValue;

        if ( sLength < 0 )
        {
            sLength = 0;
        }
        else if ( sLength > RANDOM_STRING_MAX )
        {
            sLength = RANDOM_STRING_MAX;
        }
        else
        {
            /* Nothing to do */
        }
    }

    *aLength = sLength;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtfRandomStringEstimate( mtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          /* aRemain */,
                                       mtcCallBack * aCallBack )
{
    mtcNode         * sMtc1;
    mtcNode         * sMtc2;
    mtcColumn       * sColumn;
    mtdIntegerType    sLength = 0;
    vSLong            sOption = 0;
    void            * sTempValue;
    const mtdModule * sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if ( ( aNode->lflag & MTC_NODE_REESTIMATE_MASK )
         == MTC_NODE_REESTIMATE_FALSE )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

        sModules[0] = &mtdChar;
        sModules[1] = &mtdInteger;

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         &mtdVarchar,
                                         1,
                                         RANDOM_STRING_MAX,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        /* arguments 1 Option */
        sMtc1 = mtf::convertedNode( aNode->arguments, aTemplate );

        /* arguments 2 Length */
        sMtc2 = mtf::convertedNode( aNode->arguments->next, aTemplate );

        if ( ( ( sMtc1 == aNode->arguments ) &&
               ( ( aTemplate->rows[sMtc1->table].lflag & MTC_TUPLE_TYPE_MASK )
                 == MTC_TUPLE_TYPE_CONSTANT ) ) &&
             ( ( sMtc2 == aNode->arguments->next ) &&
               ( ( aTemplate->rows[sMtc2->table].lflag & MTC_TUPLE_TYPE_MASK )
               == MTC_TUPLE_TYPE_CONSTANT ) ) )
        {
            /* arguments 2 Option */
            sColumn = &( aTemplate->rows[sMtc1->table].columns[sMtc1->column]);
            sTempValue = (void *)mtd::valueForModule( (smiColumn*)sColumn,
                                                      aTemplate->rows[sMtc1->table].row,
                                                      MTD_OFFSET_USE,
                                                      sColumn->module->staticNull );

            IDE_TEST( mtfRandomStringGetOption( sColumn, sTempValue, &sOption )
                      != IDE_SUCCESS );

            /* arguments 2 Length */
            sColumn = &( aTemplate->rows[sMtc2->table].columns[sMtc2->column]);
            sTempValue = (void *)mtd::valueForModule( (smiColumn*)sColumn,
                                                      aTemplate->rows[sMtc2->table].row,
                                                      MTD_OFFSET_USE,
                                                      sColumn->module->staticNull );

            IDE_TEST( mtfRandomStringGetLength( sColumn, sTempValue, &sLength )
                      != IDE_SUCCESS );

            IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                             &mtdVarchar,
                                             1,
                                             sLength,
                                             0 )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column].calculate = mtfRandomStringCalculateFast;
            aTemplate->rows[aNode->table].execute[aNode->column].calculateInfo = (void *)sOption;
        }
        else
        {
            /* Nothing to do */
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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }
    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtfRandomStringMakeString( SInt          aOption,
                                         SInt          aLength,
                                         mtdCharType * aString )
{
    SInt sMod    = 0;
    SInt sPos    = 0;
    SInt sValue  = 0;
    SInt i;

    /* 소문자 26, 대문자 26, 숫자 10 */
    const SChar * sRandomStrings = "abcdefghijklmnopqrstuvwxyz"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "0123456789";
    switch ( aOption )
    {
        case RANDOM_STRING_OPTION_U:
            sMod = 26; /* UPPER CASE */
            sPos = 26;
            break;
        case RANDOM_STRING_OPTION_L:
            sMod = 26; /* LOWER CASE */
            sPos = 0;
            break;
        case RANDOM_STRING_OPTION_A:
            sMod = 52; /* UPPER CASE + LOWER CASE */
            sPos = 0;
            break;
        case RANDOM_STRING_OPTION_X:
            sMod = 36; /* UPPER CASE + NUMBER */
            sPos = 26;
            break;
        case RANDOM_STRING_OPTION_P:
            sMod = 95; /* ASCII Code 0x20( ' ' ) ~ 0x7E( ~ ) */
            sPos = 0;
            break;
        default:
            IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
            break;
    }

    if ( aLength > 0 )
    {
        for ( i = 0; i < aLength; i++ )
        {
            sValue = (SInt)idlOS::rand() % sMod;

            if ( aOption != RANDOM_STRING_OPTION_P )
            {
                aString->value[i] = sRandomStrings[sPos + sValue];
            }
            else
            {
                aString->value[i] = (UChar)( 0x20 + sValue );
            }
        }

        aString->length = aLength;
    }
    else
    {
        aString->length = 0;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtfRandomStringCalculate( mtcNode     * aNode,
                                        mtcStack    * aStack,
                                        SInt          aRemain,
                                        void        * aInfo,
                                        mtcTemplate * aTemplate )
{
    vSLong        sOption = 0;
    SInt          sLength = 0;
    mtdCharType * sChar;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST( mtfRandomStringGetOption( aStack[1].column, aStack[1].value, &sOption )
              != IDE_SUCCESS );

    IDE_TEST( mtfRandomStringGetLength( aStack[2].column, aStack[2].value, &sLength )
              != IDE_SUCCESS );

    sChar = (mtdCharType *)aStack[0].value;

    IDE_TEST( mtfRandomStringMakeString( (SInt)sOption, sLength, sChar )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC mtfRandomStringCalculateFast( mtcNode     * aNode,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            void        * aInfo,
                                            mtcTemplate * aTemplate )
{
    SInt          sLength = 0;
    SInt          sOption = 0;
    mtdCharType * sChar;

    IDE_TEST_RAISE( aRemain < 1, ERR_STACK_OVERFLOW );

    aStack->column = aTemplate->rows[aNode->table].columns + aNode->column;
    aStack->value  = (void*) mtd::valueForModule( (smiColumn*) aStack->column,
                                                  aTemplate->rows[aNode->table].row,
                                                  MTD_OFFSET_USE,
                                                  aStack->column->module->staticNull );

    sOption = (SInt)(vSLong)aInfo;
    sLength = (SInt)aStack[0].column->precision;

    sChar = ( mtdCharType * )aStack[0].value;

    IDE_TEST( mtfRandomStringMakeString( sOption, sLength, sChar )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_STACK_OVERFLOW));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


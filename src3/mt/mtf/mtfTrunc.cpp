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
 * $Id: mtfTrunc.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfTrunc;

extern mtdModule mtdDate;
extern mtdModule mtdFloat;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfTruncFunctionName[1] = {
    { NULL, 5, (void*)"TRUNC" }
};

static IDE_RC mtfTruncEstimate( mtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

mtfModule mtfTrunc = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTruncFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTruncEstimate
};

static IDE_RC mtfTruncCalculateFloatFor1Arg(  mtcNode*     aNode,
                                              mtcStack*    aStack,
                                              SInt         aRemain,
                                              void*        aInfo,
                                              mtcTemplate* aTemplate );

static IDE_RC mtfTruncCalculateFloatFor2Args(  mtcNode*     aNode,
                                               mtcStack*    aStack,
                                               SInt         aRemain,
                                               void*        aInfo,
                                               mtcTemplate* aTemplate );

static IDE_RC mtfTruncCalculateFor1Arg( mtcNode*     aNode,
                                        mtcStack*    aStack,
                                        SInt         aRemain,
                                        void*        aInfo,
                                        mtcTemplate* aTemplate );

static IDE_RC mtfTruncCalculateFor2Args( mtcNode*     aNode,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         void*        aInfo,
                                         mtcTemplate* aTemplate );

static const mtcExecute mtfExecuteFloatFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTruncCalculateFloatFor1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteFloatFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTruncCalculateFloatFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTruncCalculateFor1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTruncCalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTruncEstimate( mtcNode*     aNode,
                         mtcTemplate* aTemplate,
                         mtcStack*    aStack,
                         SInt      /* aRemain */,
                         mtcCallBack* aCallBack )
{
    const mtdModule* sFloatModules[2];
    const mtdModule* sDateModules[2];

    /* BUG-44091 where 절에 round(), trunc() 오는 경우 비정상 종료합니다.  */
    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );
    
    if ( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
    {
        mtc::makeFloatConversionModule( aStack + 1, &sFloatModules[0] );
    }
    else
    {
        mtc::makeFloatConversionModule( aStack + 1, &sFloatModules[0] );
        mtc::makeFloatConversionModule( aStack + 2, &sFloatModules[1] );
    }

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    if( ( aStack[1].column->module->flag & MTD_GROUP_MASK ) == MTD_GROUP_DATE )
    {
        sDateModules[0] = &mtdDate;
        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            IDE_TEST( mtf::getCharFuncResultModule( &sDateModules[1],
                                                    aStack[1].column->module )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sDateModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteFor1Arg;
        }
        else
        {
            // PROJ-1579 NCHAR
            IDE_TEST( mtf::getCharFuncCharResultModule( &sDateModules[1],
                                                    aStack[2].column->module )
                      != IDE_SUCCESS );

            IDE_TEST( mtf::makeConversionNodes( aNode,
                                                aNode->arguments,
                                                aTemplate,
                                                aStack + 1,
                                                aCallBack,
                                                sDateModules )
                      != IDE_SUCCESS );

            aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteFor2Args;
        }

        //IDE_TEST( mtdDate.estimate( aStack[0].column, 0, 0, 0 )
        //          != IDE_SUCCESS );
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdDate,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sFloatModules )
                  != IDE_SUCCESS );

        if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                mtfExecuteFloatFor1Arg;
        }
        else
        {
            aTemplate->rows[aNode->table].execute[aNode->column] =
                mtfExecuteFloatFor2Args;
        }

        //IDE_TEST( mtdFloat.estimate( aStack[0].column, 0, 0, 0 )
        //          != IDE_SUCCESS );
        IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                         & mtdFloat,
                                         0,
                                         0,
                                         0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTruncCalculateFloatFor1Arg( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Trunc Calculate
 *
 * Implementation :
 *    aStack[0] : 소수점 아래를 버린 숫자 
 *    aStack[1] : 입력 숫자
 *
 *    ex) TRUNC( 15.79 ) ==> 15
 *
 ***********************************************************************/
    
    mtdNumericType * sMtdZeroValue;
    UChar            sMtdZeroBuff[20] = { 1, 128 , 0, };
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( mtdFloat.isNull( aStack[1].column,
                         aStack[1].value ) == ID_TRUE )
    {
        mtdFloat.null( aStack[0].column,
                       aStack[0].value );
    }
    else
    {
        sMtdZeroValue = (mtdNumericType*)sMtdZeroBuff;
        IDE_TEST( mtc::truncFloat( (mtdNumericType*)aStack[0].value,
                                   (mtdNumericType*)aStack[1].value,
                                   sMtdZeroValue )
                  != IDE_SUCCESS)
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfTruncCalculateFloatFor2Args( mtcNode*     aNode,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       void*        aInfo,
                                       mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Trunc Calculate
 *
 * Implementation :
 *    TRUNC( number_a, number_b )
 *
 *    aStack[0] : number_a를 소수점 아래 number_b번째 자리에서 버림한 결과
 *    aStack[1] : number_a ( 입력 숫자 )
 *    aStack[2] : number_b ( 버릴 소수점 아래 위치 )
 *
 *    ex) TRUNC(15.79 ,1 ) ==> 15.7
 *
 ***********************************************************************/
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( (mtdFloat.isNull( aStack[1].column,
                          aStack[1].value ) == ID_TRUE) ||
        (mtdFloat.isNull( aStack[2].column,
                          aStack[2].value ) == ID_TRUE) )
    {
        mtdFloat.null( aStack[0].column,
                       aStack[0].value );
    }
    else
    {
        IDE_TEST( mtc::truncFloat( (mtdNumericType*)aStack[0].value,
                                   (mtdNumericType*)aStack[1].value,
                                   (mtdNumericType*)aStack[2].value )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtfTruncCalculateFor1Arg(  mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Trunc Calculate
 *
 * Implementation :
 *    aStack[0] : 
 *    aStack[1] : 읽어온 스트링
 *
 ***********************************************************************/
    
    mtdDateType* sValue;
    mtdDateType* sDate;

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
        sValue   = (mtdDateType*)aStack[0].value;
        sDate    = (mtdDateType*)aStack[1].value;

        IDE_TEST( mtdDateInterface::makeDate(sValue,
                                             mtdDateInterface::year(sDate),
                                             mtdDateInterface::month(sDate),
                                             mtdDateInterface::day(sDate),
                                             0, 0, 0, 0)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTruncCalculateFor2Args(  mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Trunc Calculate
 *
 * Implementation :
 *    aStack[0] : 
 *    aStack[1] : 읽어온 스트링
 *
 ***********************************************************************/
    
    mtdDateType*     sValue;
    mtdDateType*     sDate;
    mtdCharType*     sVarchar;
    const mtlModule* sLanguage;
    SInt             sYear;
    SInt             sMonth;
    SInt             sDay;

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
        sValue   = (mtdDateType*)aStack[0].value;
        sDate    = (mtdDateType*)aStack[1].value;
        sVarchar = (mtdCharType*)aStack[2].value;
        sLanguage = aStack[2].column->language;

        sYear = mtdDateInterface::year(sDate);
        sMonth = mtdDateInterface::month(sDate);
        sDay = mtdDateInterface::day(sDate);

        UChar sLastDays[13] = { 0, 31, 28, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31 };

        // date가 윤년일 경우
        if ( mtdDateInterface::isLeapYear( sYear ) == ID_TRUE )
        {
             sLastDays[2] = 29;
        }
        else
        {
            /* Nothing to do */
        }

        if( sLanguage->extractSet->matchCentury( sVarchar->value,
                                                 sVarchar->length ) == 0 )
        {
            if ( sYear <= 0 )
            {
                sYear = ( sYear / 100 ) * 100 - 99;
            }
            else
            {
                sYear = ( ( sYear - 1 ) / 100 ) * 100 + 1;
            }

            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 sYear,
                                                 1, 1, 0, 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchYear( sVarchar->value,
                                                   sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 1, 1, 0, 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchQuarter( sVarchar->value,
                                                      sVarchar->length ) == 0 )
        {
            sMonth = (SInt) idlOS::ceil( ( (SDouble) sMonth / 3 ) ) * 3 - 2;
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 sMonth, 
                                                 1, 0, 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchMonth( sVarchar->value,
                                                    sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 mtdDateInterface::month(sDate),
                                                 1, 0, 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchWeek( sVarchar->value,
                                                   sVarchar->length ) == 0 )
        {
            /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            if ( ( sYear == 1582 ) &&
                 ( sMonth == 10 ) &&
                 ( ( sDay == 15 ) || ( sDay == 16 ) ) )
            {
                sDay -= 10;
            }
            else
            {
                /* Nothing to do */
            }

            sDay -= ( mtc::dayOfWeek( sYear, sMonth, sDay ) );
            if ( sDay < 1 )
            {
                sMonth--;
                if ( sMonth < 1 )
                {
                    sYear--;
                    sMonth = 12;
                }
                sDay += sLastDays[sMonth];
            }

            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 sYear,
                                                 sMonth,
                                                 sDay,
                                                 0, 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchDay( sVarchar->value,
                                                  sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 mtdDateInterface::month(sDate),
                                                 mtdDateInterface::day(sDate),
                                                 0, 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchHour( sVarchar->value,
                                                   sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 mtdDateInterface::month(sDate),
                                                 mtdDateInterface::day(sDate),
                                                 mtdDateInterface::hour(sDate),
                                                 0, 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchMinute( sVarchar->value,
                                                     sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 mtdDateInterface::month(sDate),
                                                 mtdDateInterface::day(sDate),
                                                 mtdDateInterface::hour(sDate),
                                                 mtdDateInterface::minute(sDate),
                                                 0, 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchSecond( sVarchar->value,
                                                     sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 mtdDateInterface::month(sDate),
                                                 mtdDateInterface::day(sDate),
                                                 mtdDateInterface::hour(sDate),
                                                 mtdDateInterface::minute(sDate),
                                                 mtdDateInterface::second(sDate),
                                                 0)
                      != IDE_SUCCESS );
        }
        else if( sLanguage->extractSet->matchMicroSec( sVarchar->value,
                                                       sVarchar->length ) == 0 )
        {
            IDE_TEST( mtdDateInterface::makeDate(sValue,
                                                 mtdDateInterface::year(sDate),
                                                 mtdDateInterface::month(sDate),
                                                 mtdDateInterface::day(sDate),
                                                 mtdDateInterface::hour(sDate),
                                                 mtdDateInterface::minute(sDate),
                                                 mtdDateInterface::second(sDate),
                                                 mtdDateInterface::microSecond(sDate))
                      != IDE_SUCCESS );
        }
        /* BUG-45730 ROUND, TRUNC 함수에서 DATE 포맷 IW 추가 지원 */
        else if( sLanguage->extractSet->matchISOWeek( sVarchar->value,
                                                      sVarchar->length ) == 0 )
        {
            /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            if ( ( sYear == 1582 ) &&
                 ( sMonth == 10 ) &&
                 ( ( sDay == 15 ) || ( sDay == 16 ) ) )
            {
                sDay -= 10;
            }
            else
            {
                /* Nothing to do */
            }

            /* 일요일부터 0, 1, 2, 3, 4, 5, 6 순서를 6, 0, 1, 2, 3, 4, 5 으로 변경 */
            sDay -= ( ( mtc::dayOfWeek( sYear, sMonth, sDay ) + 6 ) % 7 );

            if ( sDay < 1 )
            {
                sMonth--;

                if ( sMonth < 1 )
                {
                    sYear--;

                    sMonth = 12;
                }
                else
                {
                    /* Nothing to do */
                }

                sDay += sLastDays[sMonth];
            }
            else
            {
                /* Nothing to do */
            }

            IDE_TEST( mtdDateInterface::makeDate( sValue,
                                                  sYear,
                                                  sMonth,
                                                  sDay,
                                                  0,
                                                  0,
                                                  0,
                                                  0 )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_RAISE(ERR_INVALID_LITERAL);
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

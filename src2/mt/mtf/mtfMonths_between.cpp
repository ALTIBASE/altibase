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
 * $Id: mtfMonths_between.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfMonths_between;
extern mtdModule mtdDate;
extern mtdModule mtdInteger;
extern mtdModule mtdDouble;

static mtcName mtfMonths_betweenFunctionName[1] = {
    { NULL, 14, (void*)"MONTHS_BETWEEN" }
};

static IDE_RC mtfMonths_betweenEstimate( mtcNode*     aNode,
                                         mtcTemplate* aTemplate,
                                         mtcStack*    aStack,
                                         SInt         aRemain,
                                         mtcCallBack* aCallBack );

mtfModule mtfMonths_between = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfMonths_betweenFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfMonths_betweenEstimate
};

static IDE_RC mtfMonths_betweenCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfMonths_betweenCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfMonths_betweenEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt      /* aRemain */,
                                  mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdDate;
    sModules[1] = &mtdDate;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDouble,
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

IDE_RC mtfMonths_betweenCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Months_between Calculate
 *
 * Implementation : 
 *    MONTHS_BETWEEN( startdate, enddate )
 *
 *    aStack[0] : startdate - enddate를 개월 수로 보여준다.
 *    aStack[1] : startdate
 *    aStack[2] : enddate
 *
 *    ex) MONTHS_BETWEEN ( '30-SEP-2005', '31-DEC-2005') ==> -3
 *
 ***********************************************************************/
    
    SShort             sStartYear = 0;
    UShort             sStartMonth = 0;
    UShort             sStartDay = 0;
    UShort             sStartHour = 0;
    UShort             sStartMinute = 0;
    UShort             sStartSecond = 0;
    UInt               sStartMicroSecond = 0;

    SShort             sEndYear = 0;
    UShort             sEndMonth = 0;
    UShort             sEndDay = 0;
    UShort             sEndHour = 0;
    UShort             sEndMinute = 0;
    UShort             sEndSecond = 0;
    UInt               sEndMicroSecond = 0;

    SDouble            sDiffYear = 0;
    SDouble            sDiffMonth = 0;
    SDouble            sDiffDay = 0;
    SDouble            sDiffHour = 0;
    SDouble            sDiffMinute = 0;
    SDouble            sDiffSecond = 0;
    SDouble            sDiffMicroSecond = 0;

    mtdDateType      * sStartDate;
    mtdDateType      * sEndDate;
    
    UChar sStartDateLastDays[13] = { 0, 31, 28, 31, 30, 31, 30,
                                        31, 31, 30, 31, 30, 31 };
    UChar sEndDateLastDays[13] = { 0, 31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31 };

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
        sStartDate = (mtdDateType*)aStack[1].value;
        sEndDate = (mtdDateType*)aStack[2].value;
        *(mtdDoubleType*)aStack[0].value = 0;

        sStartYear = mtdDateInterface::year(sStartDate);
        sStartMonth = mtdDateInterface::month(sStartDate);
        sStartDay = mtdDateInterface::day(sStartDate);
        sStartHour = mtdDateInterface::hour(sStartDate);
        sStartMinute = mtdDateInterface::minute(sStartDate);
        sStartSecond = mtdDateInterface::second(sStartDate);
        sStartMicroSecond = mtdDateInterface::microSecond(sStartDate);

        sEndYear = mtdDateInterface::year(sEndDate);
        sEndMonth = mtdDateInterface::month(sEndDate);
        sEndDay = mtdDateInterface::day(sEndDate);
        sEndHour = mtdDateInterface::hour(sEndDate);
        sEndMinute = mtdDateInterface::minute(sEndDate);
        sEndSecond = mtdDateInterface::second(sEndDate);
        sEndMicroSecond = mtdDateInterface::microSecond(sEndDate);

        // 모든 fmt에 대해서 startdate - enddate 를 구한다.
        sDiffYear = sStartYear - sEndYear;
        sDiffMonth = sStartMonth - sEndMonth;
        sDiffDay = sStartDay - sEndDay;
        sDiffHour = sStartHour - sEndHour;
        sDiffMinute = sStartMinute - sEndMinute;
        sDiffSecond = sStartSecond - sEndSecond;
        sDiffMicroSecond = sStartMicroSecond - sEndMicroSecond;

        // startdate가 윤년일 경우
        if ( mtdDateInterface::isLeapYear( sStartYear ) == ID_TRUE )
        {
            sStartDateLastDays[2] = 29;
        }
        else
        {
            /* Nothing to do */
        }

        // enddate가 윤년일 경우
        if ( mtdDateInterface::isLeapYear( sEndYear ) == ID_TRUE )
        {
            sEndDateLastDays[2] = 29;
        }
        else
        {
            /* Nothing to do */
        }

        // 일이 같거나, 다른 달의 마지막 날일 경우에도 정수로 출력되어야 함.
        // 이 때에는 hour, minute, ... 에 상관없이 무조건 정수로 출력됨.
        if ( ( sStartDay == sEndDay ) || 
               ( ( sStartDay == sStartDateLastDays[sStartMonth] ) &&
               ( sEndDay == sEndDateLastDays[sEndMonth] ) ) )
        {
            *(mtdDoubleType*)aStack[0].value = sDiffMonth + sDiffYear * 12;
        }
        else
        {
            *(mtdDoubleType*)aStack[0].value = ( sDiffYear * 12 ) + 
                                               ( sDiffMonth ) +
                                               ( sDiffDay / 31 ) +
                                               ( sDiffHour / 24 / 31 ) +
                                               ( sDiffMinute / 60 / 24 / 31 ) +
                                               ( sDiffSecond / 60 / 60 / 24 / 31 ) +
                                               ( sDiffMicroSecond / 1000000 / 60 / 60 / 24 / 31 );
                                
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

 

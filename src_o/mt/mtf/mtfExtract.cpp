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
 * $Id: mtfExtract.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfExtract;

extern mtdModule mtdDate;
extern mtdModule mtdInteger;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;
extern mtdModule mtdInterval;

static mtcName mtfExtractFunctionName[2] = {
    { mtfExtractFunctionName+1, 7, (void*) "EXTRACT"  },
    { NULL, 8, (void*) "DATEPART" }
};

static IDE_RC mtfExtractEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfExtract = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfExtractFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfExtractEstimate
};

static IDE_RC mtfExtractCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfExtractCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfExtractEstimate( mtcNode*     aNode,
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

    // BUG-28092
    if ( aStack[1].column->module->id == MTD_INTERVAL_ID )
    {
        sModules[0] = &mtdInterval;
    }
    else
    {
        sModules[0] = &mtdDate;
    }

    // PROJ-1579 NCHAR
    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[1],
                                                aStack[2].column->module )
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
                                     &mtdInteger,
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

IDE_RC mtfExtractCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Extract Calculate
 *
 * Implementation :
 *    EXTRACT( date, char )
 *
 *    aStack[0] : Date 내용 중 입력값에 해당되는 자료만 추출한 값
 *    aStack[1] : Date 내용
 *    aStack[2] : 입력값
 *
 *    ex) extract( join_date, 'MONTH' )
 *        -----------------------------
 *        11
 *
 ***********************************************************************/
    
    mtdCharType      * sInput       = NULL;
    const mtlModule  * sLanguage    = NULL;
    mtdDateType      * sDate        = NULL;
    mtdIntervalType  * sInterval    = NULL;
    SInt               sYear        = 0;
    SInt               sMonth       = 0;
    SInt               sWeekOfMonth = 0;
    SInt               sDay         = 0;
    SInt               sHour        = 0;
    SInt               sMinute      = 0;
    SInt               sSecond      = 0;
    SInt               sMicroSecond = 0;
    SInt               sSign        = 1;

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
        // BUG - 28092
        if ( aStack[1].column->module->id == MTD_INTERVAL_ID )
        {
            sInterval = (mtdIntervalType*)aStack[1].value;

            if ( (sInterval->second < 0) || (sInterval->microsecond < 0 ))
            {
                sSign                  = -1;
                sInterval->second      = -sInterval->second;
                sInterval->microsecond = -sInterval->microsecond;
            }
            else
            {
                //nothing to do
            }

            /* BUG-43938 INTERVAL Type과 DATE Type의 Day 개념이 다릅니다. */
            sDay         = (SInt)( sInterval->second / ID_LONG(86400) );

            sHour        = (SInt)( ( sInterval->second % ID_LONG(86400) ) / 3600 );
            sMinute      = (SInt)( ( sInterval->second % ID_LONG(3600) ) / 60 );
            sSecond      = (SInt)( sInterval->second % ID_LONG(60) );
            sMicroSecond = (SInt)sInterval->microsecond;
        }
        else
        {
            sDate = (mtdDateType*)aStack[1].value;

            sYear        = mtdDateInterface::year( sDate );
            sMonth       = mtdDateInterface::month( sDate );
            sDay         = mtdDateInterface::day( sDate );

            sHour        = mtdDateInterface::hour( sDate );
            sMinute      = mtdDateInterface::minute( sDate );
            sSecond      = mtdDateInterface::second( sDate );
            sMicroSecond = mtdDateInterface::microSecond( sDate );
        }

        sInput = (mtdCharType*)aStack[2].value;
        sLanguage = aStack[2].column->language;

        if( sLanguage->extractSet->matchCentury( sInput->value,
                                                 sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 입력값이 century인 경우,
            // 읽어온 year 값을 이용하여 계산
            if ( sYear <= 0 )
            {
                *(mtdIntegerType *)aStack[0].value = ( sYear / 100 ) - 1;
            }
            else
            {
                *(mtdIntegerType *)aStack[0].value = ( sYear + 99 ) / 100;
            }
        }
        else if( sLanguage->extractSet->matchYear( sInput->value,
                                                   sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 입력값이 year인 경우,
            // 읽어온 date 내용에서 year값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sYear;
        }
        else if( sLanguage->extractSet->matchQuarter( sInput->value,
                                                      sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 입력값이 quarter인 경우,
            // 읽어온 month 값을 이용하여 계산
            *(mtdIntegerType*)aStack[0].value = (SInt) idlOS::ceil( ( (SDouble) sMonth / 3 ) ); 
        }
        else if( sLanguage->extractSet->matchMonth( sInput->value,
                                                    sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 입력값이 month인 경우,
            // 읽어온 date 내용에서 month값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sMonth;
        }
        else if( sLanguage->extractSet->matchWeek( sInput->value,
                                                   sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 입력값이 week인 경우,
            // 년중 몇번째 주인지 계산한다.
            // 1월 1일부터 그 주 토요일까지 1주로 계산한다.

            *(mtdIntegerType*)aStack[0].value = mtc::weekOfYear( sYear, sMonth, sDay );
        }
        else if( sLanguage->extractSet->matchWeekOfMonth( sInput->value,
                                                          sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 입력값이 weekofmonth인 경우,
            // 월중 몇번째 주인지 계산한다.
            // 1일부터 그 주 토요일까지 1주로 계산한다.

            sWeekOfMonth = (SInt) idlOS::ceil( (SDouble) ( sDay + 
                                                    mtc::dayOfWeek( sYear, sMonth, 1 ) ) / 7 );

            *(mtdIntegerType*)aStack[0].value = sWeekOfMonth;
        }
        else if( sLanguage->extractSet->matchDay( sInput->value,
                                                  sInput->length ) == 0 )
        {
            // 입력값이 day인 경우,
            // 읽어온 date 내용에서 day값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sDay;
        }
        else if( sLanguage->extractSet->matchDayOfYear( sInput->value,
                                                        sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 년중 몇번째 날인지 계산.
            *(mtdIntegerType*)aStack[0].value = mtc::dayOfYear( sYear, sMonth, sDay );
        }
        else if( sLanguage->extractSet->matchDayOfWeek( sInput->value,
                                                      sInput->length ) == 0 )
        {
            /* BUG-43938 INTERVAL Type에는 Week, Month, Year 개념이 없습니다. */
            IDE_TEST_RAISE( aStack[1].column->module->id == MTD_INTERVAL_ID,
                            ERR_INVALID_LITERAL );

            // 주중 몇번째 날인지 계산.
            // 일요일:1, 토요일:7
            *(mtdIntegerType*)aStack[0].value = mtc::dayOfWeek( sYear, sMonth, sDay ) + 1;
        }
        else if( sLanguage->extractSet->matchHour( sInput->value,
                                                   sInput->length ) == 0 )
        {
            // 입력값이 hour인 경우,
            // 읽어온 date 내용에서 hour값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sHour;
        }
        else if( sLanguage->extractSet->matchMinute( sInput->value,
                                                     sInput->length ) == 0 )
        {
            // 입력값이 minute인 경우,
            // 읽어온 date 내용에서 minute값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sMinute;
        }
        else if( sLanguage->extractSet->matchSecond( sInput->value,
                                                     sInput->length ) == 0 )
        {
            // 입력값이 second인 경우,
            // 읽어온 date 내용에서 second값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sSecond;
        }
        else if( sLanguage->extractSet->matchMicroSec(sInput->value,
                                                      sInput->length) == 0 )
        {
            // 입력값이 microsecond인 경우,
            // 읽어온 date 내용에서 microsecond값을 결과에 저장
            *(mtdIntegerType*)aStack[0].value = sMicroSecond;
        }
        else
        {
            IDE_RAISE(ERR_INVALID_LITERAL);
        }

        // BUG - 28092
        *(mtdIntegerType*)aStack[0].value *= sSign;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}


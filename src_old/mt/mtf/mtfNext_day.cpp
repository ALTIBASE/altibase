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
 * $Id: mtfNext_day.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfNext_day;

extern mtdModule mtdDate;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfNext_dayFunctionName[1] = {
    { NULL, 8, (void*)"NEXT_DAY" }
};

static IDE_RC mtfNext_dayEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfNext_day = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfNext_dayFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfNext_dayEstimate
};

static IDE_RC mtfNext_dayCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfNext_dayCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfNext_dayEstimate( mtcNode*     aNode,
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

    //IDE_TEST( mtdDate.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDate,
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

IDE_RC mtfNext_dayCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Next_day Calculate
 *
 * Implementation :
 *    NEXT_DAY( date, char )
 *
 *    aStack[0] : 입력된 날짜( date ) 이후에 알고 싶은 요일( char )의 날짜
 *    aStack[1] : date ( 입력 날짜 )
 *    aStack[2] : char ( 알고 싶은 요일 )
 *
 *    ex) NEXT_DAY( codingDay, 'SUNDAY' )
 *        ==> 2005/06/07 00:00:00 2005/06/12 00:00:00
 *
 ***********************************************************************/
    
    mtdDateType*     sResult;
    mtdDateType*     sDate;
    mtdCharType*     sDay;
    mtdIntervalType  sInterval;
    SInt             sFound;
    SInt             sWday;
    const mtlModule* sLanguage;

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
        sResult   = (mtdDateType*)aStack[0].value;
        sDate     = (mtdDateType*)aStack[1].value;
        sDay      = (mtdCharType*)aStack[2].value;
        sLanguage = aStack[2].column->language;

        if( sLanguage->nextDaySet->matchSunDay( sDay->value,
                                                sDay->length ) == 0 )
        {
            sFound = 0;
        }
        else if( sLanguage->nextDaySet->matchMonDay( sDay->value,
                                                     sDay->length ) == 0 )
        {
            sFound = 1;
        }
        else if( sLanguage->nextDaySet->matchTueDay( sDay->value,
                                                     sDay->length ) == 0 )
        {
            sFound = 2;
        }
        else if( sLanguage->nextDaySet->matchWedDay( sDay->value,
                                                     sDay->length ) == 0 )
        {
            sFound = 3;
        }
        else if( sLanguage->nextDaySet->matchThuDay( sDay->value,
                                                     sDay->length ) == 0 )
        {
            sFound = 4;
        }
        else if( sLanguage->nextDaySet->matchFriDay( sDay->value,
                                                     sDay->length ) == 0 )
        {
            sFound = 5;
        }
        else if( sLanguage->nextDaySet->matchSatDay( sDay->value,
                                                     sDay->length ) == 0 )
        {
            sFound = 6;
        }
        else
        {
            IDE_RAISE( ERR_INVALID_LITERAL );
        }

        IDE_TEST( mtdDateInterface::convertDate2Interval( sDate,
                                                         &sInterval )
                  != IDE_SUCCESS );

        // PROJ-1066 date 연산범위 확장에 의해 주석처리
        // BUG-11047 fix
        // sT가 -1이고 errno가 세팅되면 범위를 초과한 것이므로 
        // 에러를 발생시킨다.
        // IDE_TEST_RAISE( sT == -1, ERR_NOT_APPLICABLE )

        // 주어진 날짜의 요일을 구한다.  
        sWday = mtc::dayOfWeek(mtdDateInterface::year(sDate),
                               mtdDateInterface::month(sDate),
                               mtdDateInterface::day(sDate) );
        if( sFound > sWday ){
            sInterval.second += ( sFound - sWday ) * 86400;
        }else{
            sInterval.second += ( sFound + 7 - sWday ) * 86400;
        }

        /* BC에서 AD로 넘어가는 경우, MicroSecond를 음수에서 양수로 바꾼다. */
        if ( ( sInterval.second > 0 ) && ( sInterval.microsecond < 0 ) )
        {
            sInterval.second--;
            sInterval.microsecond += 1000000;
        }
        else
        {
            /* Nothing to do */
        }

        IDE_TEST_RAISE( ( sInterval.second > MTD_DATE_MAX_YEAR_PER_SECOND ) ||
                        ( sInterval.second < MTD_DATE_MIN_YEAR_PER_SECOND ) ||
                        ( ( sInterval.second == MTD_DATE_MIN_YEAR_PER_SECOND ) &&
                          ( sInterval.microsecond < 0 ) ),
                        ERR_INVALID_YEAR );

        IDE_TEST( mtdDateInterface::convertInterval2Date( &sInterval,
                                                           sResult )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

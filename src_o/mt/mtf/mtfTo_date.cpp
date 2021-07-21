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
 * $Id: mtfTo_date.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfTo_date;

extern mtdModule mtdDate;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfTo_dateFunctionName[1] = {
    { NULL, 7, (void*)"TO_DATE" }
};

static IDE_RC mtfTo_dateEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );

mtfModule mtfTo_date = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfTo_dateFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfTo_dateEstimate
};

static IDE_RC mtfTo_dateCalculateFor1Arg( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

static IDE_RC mtfTo_dateCalculateFor2Args( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

const mtcExecute mtfExecuteFor1Arg = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_dateCalculateFor1Arg,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

const mtcExecute mtfExecuteFor2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfTo_dateCalculateFor2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfTo_dateEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) < 1 ||
                    ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    if( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) == 1 )
    {
        sModules[0] = &mtdDate;

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

        aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecuteFor1Arg;
    }
    else
    {
        if( aStack[2].column->language != NULL )
        {
            // PROJ-1579 NCHAR
            IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[0],
                                                    aStack[2].column->module )
                      != IDE_SUCCESS );
        }
        else
        {
            // PROJ-1579 NCHAR
            IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[0],
                                                    aStack[1].column->module )
                      != IDE_SUCCESS );
        }

        sModules[1] = sModules[0];

        IDE_TEST( mtf::makeConversionNodes( aNode,
                                            aNode->arguments,
                                            aTemplate,
                                            aStack + 1,
                                            aCallBack,
                                            sModules )
                  != IDE_SUCCESS );

        aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

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

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfTo_dateCalculateFor1Arg( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : To_Date Calculate
 *
 * Implementation :
 *    TO_DATE( char )
 *
 *    aStack[0] : CHAR, VARCHAR 타입의 char를 DATE 타입으로 변환한 값 
 *    aStack[1] : date
 *
 *    ex) TO_DATE( '09-JUN-2005' ) ==> DATE 타입의 09-JUN-2005
 *
 ***********************************************************************/
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( mtdDate.isNull( aStack[1].column,
                        aStack[1].value ) == ID_TRUE )
    {
        mtdDate.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        *(mtdDateType*)aStack[0].value = *(mtdDateType*)aStack[1].value;
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

/*
static IDE_RC mtfCalculateNumber_To_date( UChar** aString,
                                          UInt*   aLength,
                                          UInt    aMax,
                                          UInt*   aValue )
{
    IDE_TEST_RAISE( *aLength < 1, ERR_INVALID_LITERAL );

    if( *aLength < aMax )
    {
        aMax = *aLength;
    }

    IDE_TEST_RAISE( **aString < '0' || **aString > '9', ERR_INVALID_LITERAL );

    for( *aValue = 0;
         aMax    > 0;
         aMax--, (*aString)++, (*aLength)-- )
    {
        if( **aString < '0' || **aString > '9' )
        {
            break;
        }
        *aValue = *aValue * 10 + **aString - '0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
*/

IDE_RC mtfTo_dateCalculateFor2Args( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : To_Date Calculate
 *
 * Implementation :
 *    TO_DATE( date, 'fmt' )
 *
 *    aStack[0] : CHAR, VARCHAR 타입의 char를 DATE 타입으로 변환한 값 
 *    aStack[1] : date
 *    aStack[2] : fmt ( char의 날짜 형식 )
 *
 *    ex) TO_CHAR( '2005-06-09 00:00:00, 'YYYY-MM-DD HH:MI::SS' )
 *       ==> DATE 타입의 2005-06-09
 *
 ***********************************************************************/

    mtdDateType*   sValue;
    mtdCharType*   sVarchar1;
    mtdCharType*   sVarchar2;
    PDL_Time_Value sTimevalue;
    struct tm      sLocaltime;
    time_t         sTime;

    SShort         sRealYear;
    UShort         sRealMonth;
    
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
        sValue    = (mtdDateType*)aStack[0].value;
        sVarchar1 = (mtdCharType*)aStack[1].value;
        sVarchar2 = (mtdCharType*)aStack[2].value;

        // 날짜 초기화
        sValue->year = ID_SSHORT_MAX;
        sValue->mon_day_hour = 0;
        sValue->min_sec_mic = 0;

        // '일'은 1로 설정
        sValue->mon_day_hour &= ~MTD_DATE_DAY_MASK;
        sValue->mon_day_hour |= 1 << MTD_DATE_DAY_SHIFT;

        IDE_TEST( mtdDateInterface::toDate(sValue,
                                           sVarchar1->value,
                                           sVarchar1->length,
                                           sVarchar2->value,
                                           sVarchar2->length)
                  != IDE_SUCCESS );

        // fix BUG-18787
        // 년 또는 월이 세팅이 안된 경우에 현재 날짜로 다시 세팅해줘야 함.
        if( (sValue->year == ID_SSHORT_MAX ) ||
            (mtdDateInterface::month(sValue)) == 0 )
        {
            sTimevalue = idlOS::gettimeofday();
            sTime      = (time_t)sTimevalue.sec();
            idlOS::localtime_r( &sTime, &sLocaltime );

            if( sValue->year == ID_SSHORT_MAX )
            {
                sRealYear = (SShort)sLocaltime.tm_year + 1900;
            }
            else
            {
                sRealYear = (SShort)sValue->year;
            }

            if( (UShort)(mtdDateInterface::month(sValue)) == 0)
            {
                sRealMonth = (UShort)sLocaltime.tm_mon + 1;
            }
            else
            {
                sRealMonth = (UShort)mtdDateInterface::month(sValue);
            }

            // year, month, day의 조합이 올바른지 체크하고,
            // sValue에 다시 세팅해준다.
            IDE_TEST( mtdDateInterface::checkYearMonthDayAndSetDateValue(
                                            sValue,
                                            (SShort)sRealYear,
                                            (UChar)sRealMonth,
                                            mtdDateInterface::day(sValue) )
                      != IDE_SUCCESS );
        }
        else
        {
            // Nothing to do
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
 

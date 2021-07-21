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
 * $Id: mtfAdd_months.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfAdd_months;

extern mtdModule mtdDate;
extern mtdModule mtdInteger;

static mtcName mtfAdd_monthsFunctionName[1] = {
    { NULL, 10, (void*)"ADD_MONTHS" }
};

static IDE_RC mtfAdd_monthsEstimate( mtcNode*     aNode,
                                     mtcTemplate* aTemplate,
                                     mtcStack*    aStack,
                                     SInt         aRemain,
                                     mtcCallBack* aCallBack );

mtfModule mtfAdd_months = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfAdd_monthsFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfAdd_monthsEstimate
};

IDE_RC mtfAdd_monthsCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfAdd_monthsCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfAdd_monthsEstimate( mtcNode*     aNode,
                              mtcTemplate* aTemplate,
                              mtcStack*    aStack,
                              SInt      /* aRemain */,
                              mtcCallBack* aCallBack )
{
    static const mtdModule* sModules[2] = {
        &mtdDate,
        &mtdInteger
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

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

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfAdd_monthsCalculate( mtcNode*     aNode,
                               mtcStack*    aStack,
                               SInt         aRemain,
                               void*        aInfo,
                               mtcTemplate* aTemplate )
{
    UChar sStartLastDays[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    UChar sEndLastDays[13]   = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    UChar sDay               = 0;

    mtdDateType* sValue;
    mtdDateType* sDate;
    SInt         sYear;
    SInt         sMonth;
    SInt         sDiffYear;
    SInt         sDiffMonth;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( ( aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
        ( aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sValue     = (mtdDateType*)aStack[0].value;
        sDate      = (mtdDateType*)aStack[1].value;
        sDiffMonth = *(mtdIntegerType*)aStack[2].value;
        sDiffYear  = sDiffMonth / 12;
        sDiffMonth = sDiffMonth % 12;
        sYear      = mtdDateInterface::year(sDate);
        sMonth     = mtdDateInterface::month(sDate);
        sDay       = mtdDateInterface::day(sDate);

        /* BUG-36296 date의 일(day)이 해당 월의 마지막 일(day)인 경우, number개월을 더한 후에도 마지막 일(day)이다. */
        if ( mtdDateInterface::isLeapYear( sYear ) == ID_TRUE )
        {
            sStartLastDays[2] = 29;
        }
        else
        {
            /* Nothing to do */
        }

        sYear     += sDiffYear;
        sMonth    += sDiffMonth;
        if( sMonth > 12 )
        {
            sMonth -= 12;
            sYear++;
        }
        if( sMonth < 1 )
        {
            sMonth += 12;
            sYear--;
        }
        IDE_TEST_RAISE( sYear >  32767, ERR_VALUE_OVERFLOW );
        IDE_TEST_RAISE( sYear < -32767, ERR_VALUE_OVERFLOW );
        
        *sValue = *sDate;
        IDE_TEST( mtdDateInterface::setYear(sValue, sYear) != IDE_SUCCESS );
        IDE_TEST( mtdDateInterface::setMonth(sValue, sMonth) != IDE_SUCCESS );
        
        /* BUG-36296 date의 일(day)이 해당 월의 마지막 일(day)인 경우, number개월을 더한 후에도 마지막 일(day)이다. */
        if ( mtdDateInterface::isLeapYear( sYear ) == ID_TRUE )
        {
            sEndLastDays[2] = 29;
        }
        else
        {
            /* Nothing to do */
        }

        if ( ( sDay == sStartLastDays[mtdDateInterface::month( sDate )] ) ||
             ( sDay >= sEndLastDays[sMonth] ) )
        {
            IDE_TEST( mtdDateInterface::setDay( sValue, sEndLastDays[sMonth] ) != IDE_SUCCESS );
        }
        else
        {
            /* BUG-36296 1582년 10월 4일(목)에서 10월 15일(금)으로 바로 건너뛴다. */
            if ( ( sYear == 1582 ) &&
                 ( sMonth == 10 ) &&
                 ( 4 < sDay ) && ( sDay < 15 ) )
            {
                IDE_TEST( mtdDateInterface::setDay( sValue, 15 ) != IDE_SUCCESS );
            }
            else
            {
                /* Nothing to do */
            }
        }
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

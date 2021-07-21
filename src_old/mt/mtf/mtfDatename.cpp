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
 * $Id: mtfDatename.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfDatename;

extern mtdModule mtdDate;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfDatenameFunctionName[1] = {
    { NULL, 8, (void*)"DATENAME" }
};

static IDE_RC mtfDatenameEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfDatename = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDatenameFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDatenameEstimate
};

static IDE_RC mtfDatenameCalculate( mtcNode*     aNode,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          void*        aInfo,
                                          mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDatenameCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDatenameEstimate( mtcNode*     aNode,
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

    IDE_TEST_RAISE( aStack[2].column->precision >
                    MTC_TO_CHAR_MAX_PRECISION,
                    ERR_TO_CHAR_MAX_PRECISION );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModules[1],
                                     1,
                                     10,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION( ERR_TO_CHAR_MAX_PRECISION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_TO_CHAR_MAX_PRECISION, 
                            MTC_TO_CHAR_MAX_PRECISION));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDatenameCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Datename Calculate
 *
 * Implementation :
 *    DATENAME( date, fmt )
 *
 *    aStack[0] : 입력된 날짜형의 이름을 출력함.
 *    aStack[1] : date
 *    aStack[2] : fmt ( char의 날짜 형식 )
 *
 *    ex) DATENAME( '19-OCT-2005', 'DAY')
 *       ==> 'WEDNESDAY'
 *
 ***********************************************************************/
    
    mtdCharType*     sResult;
    mtdDateType*     sDate;
    mtdCharType*     sDateFmt;
    UShort           sMonthIndex;
    UShort           sDayIndex;
    SShort           sYear;
    UShort           sMonth;
    UShort           sDay;
    SChar            sBuffer[15];
    SInt             sBufferCur = 0;
    SInt             sResultLen = 0;
    SInt             sBufferFence = 11;
  
    static const char* sMONTHName[12] = {
        "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
        "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };
    static const char* sMonthName[12] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    static const char* smonthName[12] = {
        "january", "february", "march", "april", "may", "june",
        "july", "august", "september", "october", "november", "december"
    };
 
    static const char* sMONName[12] = {
        "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
        "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
    };
    static const char* sMonName[12] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static const char* smonName[12] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };

    static const char* sDAYName[7] = {
        "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"
    };
    static const char* sDayName[7] = {
        "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
    };
    static const char* sdayName[7] = {
        "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"
    };

    static const char* sDYName[7] = {
        "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
    };
    static const char* sDyName[7] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char* sdyName[7] = {
        "sun", "mon", "tue", "wed", "thu", "fri", "sat"
    };
    
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
        sResult = (mtdCharType*)aStack[0].value;
        sDate = (mtdDateType*)aStack[1].value;
        sDateFmt = (mtdCharType*)aStack[2].value;
        sYear = mtdDateInterface::year(sDate);
        sMonth = mtdDateInterface::month(sDate);
        sDay = mtdDateInterface::day(sDate);

        sMonthIndex = sMonth - 1;
        sDayIndex = mtc::dayOfWeek(sYear, sMonth, sDay);

        sBuffer[0] = '\0';

        if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "MONTH", 5 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sMONTHName[sMonthIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "Month", 5 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sMonthName[sMonthIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "month", 5 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", smonthName[sMonthIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "MON", 3 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sMONName[sMonthIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "Mon", 3 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sMonName[sMonthIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "mon", 3 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", smonName[sMonthIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "DAY", 3 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sDAYName[sDayIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "Day", 3 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sDayName[sDayIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "day", 3 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sdayName[sDayIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "DY", 2 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sDYName[sDayIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "Dy", 2 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sDyName[sDayIndex] );
        }
        else if ( ( idlOS::strMatch( sDateFmt->value, sDateFmt->length,
                                "dy", 2 ) == 0 ) )
        {
            sBufferCur = idlVA::appendFormat( sBuffer, sBufferFence,
                                              "%s", sdyName[sDayIndex] );
        }
        else
	{
	    IDE_RAISE ( ERR_INVALID_LITERAL );
	}

        sResultLen = IDL_MIN( sBufferCur, sBufferFence - 1 );
        idlOS::memcpy( sResult->value, sBuffer, sResultLen );
        sResult->length = sResultLen;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

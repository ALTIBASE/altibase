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
 * $Id: mtvVarchar2Date.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtl.h>

#include <mtdTypes.h>
#include <mtuProperty.h>

extern mtvModule mtvVarchar2Date;

extern mtdModule mtdDate;
extern mtdModule mtdVarchar;

extern mtxModule mtxFromVarcharTo; /* BUG-2632 */

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

IDE_RC mtvCalculate_Varchar2Date( mtcNode*     aNode,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  void*        aInfo,
                                  mtcTemplate* aTemplate );

mtvModule mtvVarchar2Date = {
    &mtdDate,
    &mtdVarchar,
    MTV_COST_DEFAULT|MTV_COST_GROUP_PENALTY|MTV_COST_ERROR_PENALTY,
    mtvEstimate
};

static const mtcExecute mtvExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtvCalculate_Varchar2Date,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static IDE_RC mtvEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt,
                           mtcCallBack* )
{
    aStack[0].column = aTemplate->rows[aNode->table].columns+aNode->column;

    aTemplate->rows[aNode->table].execute[aNode->column] = mtvExecute;

    /* PROJ-2632 */
    aTemplate->rows[aNode->table].execute[aNode->column].mSerialExecute
        = mtxFromVarcharTo.mGetExecute( mtdDate.id, mtdDate.id );

    //IDE_TEST( mtdDate.estimate( aStack[0].column, 0, 0, 0 )
    //          != IDE_SUCCESS );
    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     & mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC mtvCalculate_Varchar2Date( mtcNode*,
                                  mtcStack*    aStack,
                                  SInt,
                                  void*,
                                  mtcTemplate* aTemplate )
{
    mtdCharType* sVarchar;
    mtdDateType* sDate;

    PDL_Time_Value sTimevalue;
    struct tm      sLocaltime;
    time_t         sTime;

    SShort         sRealYear;
    UShort         sRealMonth;
    
 
    IDE_ASSERT( aTemplate != NULL );
    IDE_ASSERT( aTemplate->dateFormat != NULL );

    if( mtdVarchar.isNull( aStack[1].column,
                           aStack[1].value ) == ID_TRUE )
    {
        mtdDate.null( aStack[0].column,
                      aStack[0].value );
    }
    else
    {
        sVarchar = (mtdCharType*)aStack[1].value;
        sDate    = (mtdDateType*)aStack[0].value;

        idlOS::memset((void*)sDate, 0x00, ID_SIZEOF(mtdDateType));

        // 날짜 초기화
        sDate->year = ID_SSHORT_MAX;
        sDate->mon_day_hour = 0;
        sDate->min_sec_mic = 0;

        // '일'은 1로 설정
        sDate->mon_day_hour &= ~MTD_DATE_DAY_MASK;
        sDate->mon_day_hour |= 1 << MTD_DATE_DAY_SHIFT;

        IDE_TEST(
            mtdDateInterface::toDate(sDate,
                                     sVarchar->value,
                                     sVarchar->length,
                                     (UChar*)aTemplate->dateFormat,
                                     idlOS::strlen( aTemplate->dateFormat ) )
                  != IDE_SUCCESS );
        
        // PROJ-1436
        // dateFormat을 참조했음을 표시한다.
        aTemplate->dateFormatRef = ID_TRUE;

        // 년 또는 월이 세팅이 안된 경우에 현재 날짜로 다시 세팅해줘야 함.
        if( (sDate->year == ID_SSHORT_MAX ) ||
            (mtdDateInterface::month(sDate)) == 0 )
        {
            sTimevalue = idlOS::gettimeofday();
            sTime      = (time_t)sTimevalue.sec();
            idlOS::localtime_r( &sTime, &sLocaltime );

            if( sDate->year == ID_SSHORT_MAX )
            {
                sRealYear = (SShort)sLocaltime.tm_year + 1900;
            }
            else
            {
                sRealYear = (SShort)sDate->year;
            }

            if( (UShort)(mtdDateInterface::month(sDate)) == 0)
            {
                sRealMonth = (UShort)sLocaltime.tm_mon + 1;
            }
            else
            {
                sRealMonth = (UShort)mtdDateInterface::month(sDate);
            }

            // year, month, day의 조합이 올바른지 체크하고,
            // sDate에 다시 세팅해준다.
            IDE_TEST( mtdDateInterface::checkYearMonthDayAndSetDateValue(
                          sDate,
                          (SShort)sRealYear,
                          (UChar)sRealMonth,
                          mtdDateInterface::day(sDate) )
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

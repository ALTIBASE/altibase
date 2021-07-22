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
 * $Id: mtfUnixToDate.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtcDef.h>

extern mtfModule mtfUnixToDate;

extern mtdModule mtdBigint;
extern mtdModule mtdDate;

static mtcName mtfUnixToDateFunctionName[1] = {
    { NULL, 12, (void*)"UNIX_TO_DATE" }
};

static IDE_RC mtfUnixToDateEstimate( mtcNode     * aNode,
                                     mtcTemplate * aTemplate,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     mtcCallBack * aCallBack );

mtfModule mtfUnixToDate = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfUnixToDateFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfUnixToDateEstimate
};

static IDE_RC mtfUnixToDateCalculate( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfUnixToDateCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfUnixToDateEstimate( mtcNode     * aNode,
                              mtcTemplate * aTemplate,
                              mtcStack    * aStack,
                              SInt       /* aRemain */,
                              mtcCallBack * aCallBack )
{
    const mtdModule* sModules[1];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdBigint;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );


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

IDE_RC mtfUnixToDateCalculate( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : UnixToDate Calculate
 *
 * Implementation :
 *    unix_to_date( 1239079710 )
 *
 *    aStack[0] : Bigint 값을 Date Type 으로 변환한 Date 값
 *    aStack[1] : Unix Time 의 Bigint 값
 *
 *    ex) unix_to_date( 1239079710 ) ==> 2009-04-07 04:48:30
 *
 ***********************************************************************/

    struct tm       sGlobaltime;
    time_t          sTime;
    mtdBigintType   sSec;
    mtdDateType   * sDate;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( mtdBigint.isNull( aStack[1].column, aStack[1].value ) == ID_TRUE )
    {
        mtdDate.null( aStack[0].column, aStack[0].value );
    }
    else
    {
        sDate = (mtdDateType*)aStack[0].value;
        sSec  = *(mtdBigintType*)aStack[1].value;

        if ( sSec <= 0 )
        {
            sTime = 0;
        }
        else
        {
            if ( sSec >= MTF_MAX_UNIX_DATE )
            {
                sTime = (time_t)MTF_MAX_UNIX_DATE;
            }
            else
            {
                sTime = (time_t)sSec;
            }
        }

        /* Time 객체로 변환 */
        idlOS::gmtime_r( &sTime, &sGlobaltime );

        /* Date 객체로 변환 */
        IDE_TEST( mtdDateInterface::makeDate( sDate,
                                              (SShort)sGlobaltime.tm_year + 1900,
                                              sGlobaltime.tm_mon + 1,
                                              sGlobaltime.tm_mday,
                                              sGlobaltime.tm_hour,
                                              sGlobaltime.tm_min,
                                              sGlobaltime.tm_sec,
                                              0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

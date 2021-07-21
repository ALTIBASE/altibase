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
 * $Id: mtfConv_tz.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtz.h>
#include <mtv.h>

extern mtdModule mtdDate;

static mtcName mtfConvertTimezoneFunctionName[1] = {
    { NULL, 13, (void*)"CONV_TIMEZONE" }
};

static IDE_RC mtfConvertTimezoneEstimate( mtcNode*     aNode,
                                          mtcTemplate* aTemplate,
                                          mtcStack*    aStack,
                                          SInt         aRemain,
                                          mtcCallBack* aCallBack );

mtfModule mtfConvertTimezone = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfConvertTimezoneFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfConvertTimezoneEstimate
};

static IDE_RC mtfConvertTimezoneCalculate( mtcNode*     aNode,
                                           mtcStack*    aStack,
                                           SInt         aRemain,
                                           void*        aInfo,
                                           mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfConvertTimezoneCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfConvertTimezoneEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt      /* aRemain */,
                                   mtcCallBack* aCallBack )
{
/***********************************************************************
 *
 * Description : mtfConvertTimezoneEstimate Estimate
 *
 * Implementation :
 *    CONV_TIMEZONE( date, varchar(40), varchar(40) )
 *
 *    aStack[0] : 결과 date
 *    aStack[1] : date ( 입력 날짜 )
 *    aStack[2] : 소스 타임존 (Asia/Seoul , KST, +09:00 )
 *    aStack[3] : 대상 타임존 (Asia/Seoul , KST, +09:00 )
 *
 *    ex) CONV_TIMEZONE( sysdate, 'KST', '-08:00' )
 *
 ***********************************************************************/

    const mtdModule* sModules[3];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdDate;

    // PROJ-1579 NCHAR
    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[1],
                                                aStack[2].column->module )
              != IDE_SUCCESS );

    IDE_TEST( mtf::getCharFuncCharResultModule( &sModules[2],
                                                aStack[3].column->module )
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
                                     & mtdDate,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_NOT_AGGREGATION ) );
    }

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_INVALID_FUNCTION_ARGUMENT ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfConvertTimezoneCalculate( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : mtfConvertTimezoneCalculate Calculate
 *
 * Implementation :
 *    CONV_TIMEZONE( date, varchar(40), varchar(40) )
 *
 *    aStack[0] : 결과 date
 *    aStack[1] : date ( 입력 날짜 )
 *    aStack[2] : 소스 타임존 (Asia/Seoul , KST, +09:00 )
 *    aStack[3] : 대상 타임존 (Asia/Seoul , KST, +09:00 )
 *
 *    ex) CONV_TIMEZONE( sysdate, 'KST', '-08:00' )
 *
 *    SRC TZ +09:00 DEST TZ -08:00
 *    +09:00의 보수 -09:00 + (-08:00) = -17:00이 총 offset이다.
 *    09 -09 -08 = 24 - 08 =16
 *    오전 09시 --> 어제 오후 16
 *
 ***********************************************************************/
    
    mtdDateType*     sResult;
    mtdDateType*     sDate;
    mtdCharType*     sSrcTimezoneStringArg;
    mtdCharType*     sDstTimezoneStringArg;

    SLong            sSrcTimezoneSecond;
    SLong            sDstTimezoneSecond;
    SLong            sTimeZoneOffsetSecond;
    SChar            sSrcTimezoneString[MTC_TIMEZONE_NAME_LEN + 1];
    SChar            sDstTimezoneString[MTC_TIMEZONE_NAME_LEN + 1];

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( ( aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE ) ||
        ( aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE ) ||
        ( aStack[3].column->module->isNull( aStack[3].column,
                                            aStack[3].value ) == ID_TRUE ) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult   = (mtdDateType*)aStack[0].value;
        sDate     = (mtdDateType*)aStack[1].value;
        sSrcTimezoneStringArg = (mtdCharType*)aStack[2].value;
        sDstTimezoneStringArg = (mtdCharType*)aStack[3].value;

        IDE_TEST_RAISE( sSrcTimezoneStringArg->length > MTC_TIMEZONE_NAME_LEN,
                        ERR_TOO_LONG_TIMEZONE_STRING );

        IDE_TEST_RAISE( sDstTimezoneStringArg->length > MTC_TIMEZONE_NAME_LEN,
                        ERR_TOO_LONG_TIMEZONE_STRING );

        idlOS::memcpy( sSrcTimezoneString,
                       sSrcTimezoneStringArg->value,
                       sSrcTimezoneStringArg->length );
        sSrcTimezoneString[sSrcTimezoneStringArg->length] = 0;

        idlOS::memcpy( sDstTimezoneString,
                       sDstTimezoneStringArg->value,
                       sDstTimezoneStringArg->length );
        sDstTimezoneString[sDstTimezoneStringArg->length] = 0;

        IDE_TEST( mtz::getTimezoneSecondAndString( sSrcTimezoneString,
                                                   &sSrcTimezoneSecond,
                                                   NULL )
                  != IDE_SUCCESS );

        IDE_TEST( mtz::getTimezoneSecondAndString( sDstTimezoneString,
                                                   &sDstTimezoneSecond,
                                                   NULL )
                  != IDE_SUCCESS );

        sTimeZoneOffsetSecond = ( ( sSrcTimezoneSecond * -1 ) + sDstTimezoneSecond );

        IDE_TEST( mtdDateInterface::addSecond( sResult,
                                               sDate,
                                               sTimeZoneOffsetSecond,
                                               0 )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_TOO_LONG_TIMEZONE_STRING );
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_TZ_LENGTH_EXCEED ) );
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

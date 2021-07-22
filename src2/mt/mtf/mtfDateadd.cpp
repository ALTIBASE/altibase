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
 * $Id: mtfDateadd.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

#include <mtdTypes.h>

extern mtfModule mtfDateadd;

extern mtdModule mtdInteger;
extern mtdModule mtdBigint;
extern mtdModule mtdDate;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfDateaddFunctionName[1] = {
    { NULL, 7, (void*)"DATEADD" }
};

static IDE_RC mtfDateaddEstimate( mtcNode*     aNode,
                                  mtcTemplate* aTemplate,
                                  mtcStack*    aStack,
                                  SInt         aRemain,
                                  mtcCallBack* aCallBack );


mtfModule mtfDateadd = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDateaddFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDateaddEstimate
};

IDE_RC mtfDateaddCalculate( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDateaddCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDateaddEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt      /* aRemain */,
                           mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdDate;
    sModules[1] = &mtdBigint;

    // PROJ-1579 NCHAR
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

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC mtfDateaddCalculate(  mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Dateadd Calculate
 *
 * Implementation :
 *    aStack[0] :
 *    aStack[1] : date
 *    aStack[2] : 증가시킬 수
 *    aStack[3] : 증가시킬 date fmt
 *
 ***********************************************************************/

    mtdDateType*     sResult;
    mtdDateType*     sDate;
    mtdCharType*     sVarchar;
    const mtlModule* sLanguage;
    SLong            sNumber = 0;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) )
    {
        aStack[0].column->module->null( aStack[0].column,
                                        aStack[0].value );
    }
    else
    {
        sResult = (mtdDateType*)aStack[0].value;
        sDate    = (mtdDateType*)aStack[1].value;
        sNumber  = *(mtdBigintType*)aStack[2].value;
        sVarchar = (mtdCharType*)aStack[3].value;
        sLanguage = aStack[3].column->language;

        if( sLanguage->extractSet->matchCentury( sVarchar->value,
                                                 sVarchar->length ) == 0 )
        {
            if ( sNumber > 100 ||
                 sNumber < -100 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMonth( sResult, sDate, sNumber*12*100 )
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchYear( sVarchar->value,
                                                   sVarchar->length ) == 0 )
        {
            if ( sNumber > 10000 ||
                 sNumber < -10000 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMonth( sResult, sDate, sNumber*12 )
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchQuarter( sVarchar->value,
                                                      sVarchar->length ) == 0 )
        {
            if ( sNumber > 40000 ||
                 sNumber < -40000 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMonth( sResult, sDate, sNumber*3 )
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchMonth( sVarchar->value,
                                                    sVarchar->length ) == 0 )
        {
            if ( sNumber > 120000 ||
                 sNumber < -120000 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMonth( sResult, sDate, sNumber )
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchWeek( sVarchar->value,
                                                   sVarchar->length ) == 0 )
        {
            if ( sNumber > 600000 ||
                 sNumber < -600000 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addDay( sResult, sDate, sNumber*7 )
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchDay( sVarchar->value,
                                                  sVarchar->length ) == 0 )
        {
            if ( sNumber > 3660000 ||
                 sNumber < -3660000 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addDay( sResult, sDate, sNumber )
                          != IDE_SUCCESS ); 
            }
        }
        else if( sLanguage->extractSet->matchHour( sVarchar->value,
                                                   sVarchar->length ) == 0 )
        {
            if ( sNumber > 87840000 ||
                 sNumber < -87840000 )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMicroSecond( sResult, sDate, sNumber*1000000*60*60)
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchMinute( sVarchar->value,
                                                     sVarchar->length ) == 0 )
        {
            if ( sNumber > ID_LONG(5270400000) ||
                 sNumber < ID_LONG(-5270400000) )
            {
                IDE_RAISE ( ERR_INVALID_YEAR );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMicroSecond( sResult, sDate, sNumber*1000000*60)
                          != IDE_SUCCESS ); 
            }
        }
        else if( sLanguage->extractSet->matchSecond( sVarchar->value,
                                                     sVarchar->length ) == 0 )
        {
            // 초일 경우 68년 이내의 값으로 한정한다.
            if ( sNumber > ID_LONG(2185574400) || 
                 sNumber < ID_LONG(-2185574400) )
            {
                IDE_RAISE ( ERR_DATEDIFF_OUT_OF_RANGE_IN_SECOND );
            }
            else
            {
                IDE_TEST( mtdDateInterface::addMicroSecond( sResult, sDate, sNumber*1000000 )
                          != IDE_SUCCESS );
            }
        }
        else if( sLanguage->extractSet->matchMicroSec( sVarchar->value,
                                                       sVarchar->length ) == 0 )
        {
            // 마이크로 초일 경우 30일 이내의 값으로 한정한다.
            if ( sNumber > ID_LONG(2592000000000) ||
                 sNumber < ID_LONG(-2592000000000) )
            {
                IDE_RAISE ( ERR_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND );
            }
            else
            {
                IDE_TEST ( mtdDateInterface::addMicroSecond( sResult, sDate, sNumber )
                           != IDE_SUCCESS );
            } 
        }
        else
        {
            IDE_RAISE(ERR_INVALID_LITERAL);
        }
    } 
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION( ERR_INVALID_YEAR );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_YEAR));

    IDE_EXCEPTION( ERR_DATEDIFF_OUT_OF_RANGE_IN_SECOND );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_SECOND));

    IDE_EXCEPTION( ERR_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_DATEDIFF_OUT_OF_RANGE_IN_MICROSECOND));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


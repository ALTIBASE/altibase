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
 * $Id: mtfTo_interval.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>

extern mtfModule mtfToInterval;
extern mtdModule mtdDouble;
extern mtdModule mtdInterval;

#define MTF_TO_INTERVAL_CMP_UNIT_STR(aStr, aUnitNo, aLen, aReturn )     \
    if ( idlOS::strncasecmp( (aStr),                                    \
                             UNIT_STR_TOINTV[(aUnitNo)],                \
                             (aLen) ) == 0 )                            \
    {                                                                   \
        (aReturn) = (aUnitNo);                                          \
    }

#define TOINTV_UNIT_YEAR        (0)
#define TOINTV_UNIT_MONTH       (1)
#define TOINTV_UNIT_DAY         (2)
#define TOINTV_UNIT_HOUR        (3)
#define TOINTV_UNIT_MIN         (4)
#define TOINTV_UNIT_SEC         (5)

#define TOINTV_UNIT_YEAR_LEN    (4)
#define TOINTV_UNIT_DAY_LEN     (3)
#define TOINTV_UNIT_MONTH_LEN   (5)
#define TOINTV_UNIT_HOUR_LEN    (4)
#define TOINTV_UNIT_MIN_LEN     (6)
#define TOINTV_UNIT_SEC_LEN     (6)
#define TOINTV_UNIT_ALIAS_LEN   (2)

static const SChar * UNIT_STR_TOINTV[6] =
{
    "YEAR",
    "MONTH",
    "DAY",
    "HOUR",
    "MINUTE",
    "SECOND"
};

static const SDouble SEC_PER_UNIT_TOINTV[6] =
{ 31536000.0, 2592000.0, 86400.0, 3600.0, 60.0, 1.0 };

static mtcName mtfToIntervalFunctionName[2] = {
    { mtfToIntervalFunctionName + 1, 11, (void*)"TO_INTERVAL" },
    { NULL,                          15, (void*)"NUMTODSINTERVAL" }
};

static IDE_RC mtfToIntervalEstimate( mtcNode     * aNode,
                                     mtcTemplate * aTemplate,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     mtcCallBack * aCallBack );

IDE_RC mtfToIntervalCalculate( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate );

mtfModule mtfToInterval = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfToIntervalFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfToIntervalEstimate
};

static const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfToIntervalCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static void mtfToIntervalGetUnit( void * aUnitValue,
                                  SInt * aUnit )
{
    SInt            sUnit = -1;
    mtdCharType *   sUnitValue;
    SInt            sUnitLen;
    SChar       *   sValueStr;

    sUnitValue = (mtdCharType *)aUnitValue;
    sUnitLen   = (SInt)sUnitValue->length;
    sValueStr  = (SChar *)sUnitValue->value;

    switch ( sUnitLen )
    {
    case TOINTV_UNIT_ALIAS_LEN:
        if ( (sValueStr[0] == 'Y' || sValueStr[0] == 'y') &&
             (sValueStr[1] == 'Y' || sValueStr[1] == 'y') )
        {
            sUnit = TOINTV_UNIT_YEAR;
        }
        else if ( (sValueStr[0] == 'M' || sValueStr[0] == 'm') &&
                  (sValueStr[1] == 'M' || sValueStr[1] == 'm') )
        {
            sUnit = TOINTV_UNIT_MONTH;
        }
        else if ( (sValueStr[0] == 'D' || sValueStr[0] == 'd') &&
                  (sValueStr[1] == 'D' || sValueStr[1] == 'd') )
        {
            sUnit = TOINTV_UNIT_DAY;
        }
        else if ( (sValueStr[0] == 'H' || sValueStr[0] == 'h') &&
                  (sValueStr[1] == 'H' || sValueStr[1] == 'h') )
        {
            sUnit = TOINTV_UNIT_HOUR;
        }
        else if ( (sValueStr[0] == 'M' || sValueStr[0] == 'm') &&
                  (sValueStr[1] == 'I' || sValueStr[1] == 'i') )
        {
            sUnit = TOINTV_UNIT_MIN;
        }
        else if ( (sValueStr[0] == 'S' || sValueStr[0] == 's') &&
                  (sValueStr[1] == 'S' || sValueStr[1] == 's') )
        {
            sUnit = TOINTV_UNIT_SEC;
        }
        else
        {
            // nothing to do
        }
        break;
    case TOINTV_UNIT_DAY_LEN:
        MTF_TO_INTERVAL_CMP_UNIT_STR( (const SChar*)sUnitValue->value,
                                      TOINTV_UNIT_DAY,
                                      TOINTV_UNIT_DAY_LEN,
                                      sUnit );
        break;
    case TOINTV_UNIT_YEAR_LEN:
        MTF_TO_INTERVAL_CMP_UNIT_STR( (const SChar*)sUnitValue->value,
                                      TOINTV_UNIT_YEAR,
                                      TOINTV_UNIT_YEAR_LEN,
                                      sUnit );
        if ( sUnit == TOINTV_UNIT_YEAR )
        {
            break;
        }
        else
        {
            // nothing to do
        }
        MTF_TO_INTERVAL_CMP_UNIT_STR( (const SChar*)sUnitValue->value,
                                      TOINTV_UNIT_HOUR,
                                      TOINTV_UNIT_HOUR_LEN,
                                      sUnit );
        break;
    case TOINTV_UNIT_MONTH_LEN:
        MTF_TO_INTERVAL_CMP_UNIT_STR( (const SChar*)sUnitValue->value,
                                      TOINTV_UNIT_MONTH,
                                      TOINTV_UNIT_MONTH_LEN,
                                      sUnit );
        break;
    case TOINTV_UNIT_MIN_LEN:
        MTF_TO_INTERVAL_CMP_UNIT_STR( (const SChar*)sUnitValue->value,
                                      TOINTV_UNIT_MIN,
                                      TOINTV_UNIT_MIN_LEN,
                                      sUnit );
        if ( sUnit == TOINTV_UNIT_MIN )
        {
            break;
        }
        else
        {
            // nothing to do
        }

        MTF_TO_INTERVAL_CMP_UNIT_STR( (const SChar*)sUnitValue->value,
                                      TOINTV_UNIT_SEC,
                                      TOINTV_UNIT_SEC_LEN,
                                      sUnit );
        break;
    default:
        break;
    }

    *aUnit = sUnit;
}

IDE_RC mtfToIntervalEstimate( mtcNode     * aNode,
                              mtcTemplate * aTemplate,
                              mtcStack    * aStack,
                              SInt       /* aRemain */,
                              mtcCallBack * aCallBack )
{
    const mtdModule* sModules[2];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdDouble;

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
                                     & mtdInterval,
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

IDE_RC mtfToIntervalCalculate( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate )
{
    mtdIntervalType     sResult;
    mtdDoubleType       sNumber;
    SInt                sUnit;
    SDouble             sSecond;
    SDouble             sIntegralPart;
    SDouble             sFractionalPart;


    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate)
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
        // 시간 단위를 얻는다.
        mtfToIntervalGetUnit( aStack[2].value, &sUnit );
        IDE_TEST_RAISE( sUnit < 0, ERR_ARGUMENT_NOT_APPLICABLE );

        sNumber = *(mtdDoubleType*)aStack[1].value;

        // 입력값을 초 단위로 환산한다.
        sSecond = sNumber * SEC_PER_UNIT_TOINTV[sUnit];

        IDE_TEST_RAISE( mtdDouble.isNull( mtdDouble.column, &sSecond )
                        == ID_TRUE,
                        ERR_VALUE_OVERFLOW );

        // 정수부와 소수부를 구한다.
        sFractionalPart = modf( sSecond, &sIntegralPart );
        IDE_TEST_RAISE( (sIntegralPart > 9e18) || (sIntegralPart < -9e18),
                        ERR_VALUE_OVERFLOW );

        sResult.second      = (SLong)sIntegralPart;

        sFractionalPart     = modf( sFractionalPart * 1e6, &sIntegralPart );
        sResult.microsecond = (SLong)sIntegralPart;

        // 오차 보정 (반올림)
        if ( sFractionalPart >= 0.5 )
        {
            sResult.microsecond++;

            if ( sResult.microsecond == 1000000 )
            {
                sResult.second++;
                sResult.microsecond = 0;
            }
            else
            {
                // Nothing to do.
            }
        }
        else if ( sFractionalPart <= -0.5 )
        {
            sResult.microsecond--;

            if ( sResult.microsecond == -1000000 )
            {
                sResult.second--;
                sResult.microsecond = 0;
            }
            else
            {
                // Nothing to do.
            }
        }
        else
        {
            // Nothing to do.
        }

        *(mtdIntervalType *)aStack[0].value = sResult;
    } 

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE )
    IDE_SET( ideSetErrorCode( mtERR_ABORT_ARGUMENT_NOT_APPLICABLE ) );

    IDE_EXCEPTION( ERR_VALUE_OVERFLOW );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_VALUE_OVERFLOW));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

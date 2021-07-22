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
 * $Id: mtfDateToUnix.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtcDef.h>

extern mtfModule mtfDateToUnix;

extern mtdModule mtdBigint;
extern mtdModule mtdDate;

static mtcName mtfDateToUnixFunctionName[1] = {
    { NULL, 12, (void*)"DATE_TO_UNIX" }
};

static IDE_RC mtfDateToUnixEstimate( mtcNode     * aNode,
                                     mtcTemplate * aTemplate,
                                     mtcStack    * aStack,
                                     SInt          aRemain,
                                     mtcCallBack * aCallBack );

mtfModule mtfDateToUnix = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0, // default selectivity (비교 연산자가 아님)
    mtfDateToUnixFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDateToUnixEstimate
};

static IDE_RC mtfDateToUnixCalculate( mtcNode     * aNode,
                                      mtcStack    * aStack,
                                      SInt          aRemain,
                                      void        * aInfo,
                                      mtcTemplate * aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDateToUnixCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDateToUnixEstimate( mtcNode     * aNode,
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

    sModules[0] = &mtdDate;

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModules )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = mtfExecute;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdBigint,
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

IDE_RC mtfDateToUnixCalculate( mtcNode     * aNode,
                               mtcStack    * aStack,
                               SInt          aRemain,
                               void        * aInfo,
                               mtcTemplate * aTemplate )
{
/***********************************************************************
 *
 * Description : DateToUnix Calculate
 *
 * Implementation :
 *   date_to_unix( 2009-04-07 04:48:30 )
 *
 *    aStack[0] : Unix Date 의 Bigint 값
 *    aStack[1] : Date 값
 *
 *    ex) date_to_unix ( 2009-04-07 04:48:30 ) ==> 1239079710
 *
 ***********************************************************************/

    mtdBigintType   * sBigint;
    mtdDateType     * sDate;
    mtdIntervalType   sSec;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( mtdDate.isNull( aStack[1].column, aStack[1].value ) == ID_TRUE )
    {
        mtdBigint.null( aStack[0].column, aStack[0].value );
    }
    else
    {
        sBigint = (mtdBigintType*)aStack[0].value;
        sDate   = (mtdDateType*)aStack[1].value;

        IDE_TEST( mtdDateInterface::convertDate2Interval( sDate, &sSec ) 
                  != IDE_SUCCESS );

        if ( sSec.second <= MTF_BASE_UNIX_DATE )
        {
            *sBigint = 0;
        }
        else
        {
            sSec.second -= MTF_BASE_UNIX_DATE;

            if ( sSec.second >=  MTF_MAX_UNIX_DATE )
            {
                *sBigint = MTF_MAX_UNIX_DATE;
            }
            else
            {
                *sBigint = sSec.second;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

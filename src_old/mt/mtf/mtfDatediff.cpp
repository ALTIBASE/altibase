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
 * $Id: mtfDatediff.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 **********************************************************************/

#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <mtf.h>
#include <mtk.h>
#include <mtv.h>

extern mtfModule mtfDatediff;
extern mtdModule mtdDate;
extern mtdModule mtdBigint;
extern mtdModule mtdChar;
extern mtdModule mtdVarchar;

static mtcName mtfDatediffFunctionName[1] = {
    { NULL, 8, (void*)"DATEDIFF" }
};

static IDE_RC mtfDatediffEstimate( mtcNode*     aNode,
                                   mtcTemplate* aTemplate,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   mtcCallBack* aCallBack );

mtfModule mtfDatediff = {
    1|MTC_NODE_OPERATOR_FUNCTION,
    ~(MTC_NODE_INDEX_MASK),
    1.0,  // default selectivity (비교 연산자가 아님)
    mtfDatediffFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    mtfDatediffEstimate
};

static IDE_RC mtfDatediffCalculate( mtcNode*     aNode,
                                   mtcStack*    aStack,
                                   SInt         aRemain,
                                   void*        aInfo,
                                   mtcTemplate* aTemplate );

const mtcExecute mtfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtfDatediffCalculate,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC mtfDatediffEstimate( mtcNode*     aNode,
                            mtcTemplate* aTemplate,
                            mtcStack*    aStack,
                            SInt      /* aRemain */,
                            mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3];

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    sModules[0] = &mtdDate;
    sModules[1] = &mtdDate;

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
                                     & mtdBigint,
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

IDE_RC mtfDatediffCalculate( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description : Datediff Calculate
 *
 * Implementation : 
 *    DATEDIFF( startdate, enddate, fmt )
 *
 *    aStack[0] : enddate - startdate를 지정한 fmt로 보여준다.
 *    aStack[1] : startdate
 *    aStack[2] : enddate
 *    aStack[3] : fmt 
 *
 *    ex) DATEDIFF ( '28-DEC-1980', '21-OCT-2005', 'DAY') ==> 9064
 *
 ***********************************************************************/
    
    mtdCharType      * sDateFmt = NULL;
    const mtlModule  * sLanguage = NULL;
    mtdDateType      * sStartDate;
    mtdDateType      * sEndDate;
    mtdDateField       sDateField = MTD_DATE_DIFF_CENTURY;

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
        sStartDate = (mtdDateType*)aStack[1].value;
        sEndDate = (mtdDateType*)aStack[2].value;
        sLanguage = aStack[3].column->language;
        sDateFmt = (mtdCharType*)aStack[3].value;

        if( sLanguage->extractSet->matchCentury( sDateFmt->value,
                                                 sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_CENTURY;
        }
        else if( sLanguage->extractSet->matchYear( sDateFmt->value,
                                              sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_YEAR;
        }
        else if( sLanguage->extractSet->matchQuarter( sDateFmt->value,
                                                      sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_QUARTER;
        }
        else if( sLanguage->extractSet->matchMonth( sDateFmt->value,
                                                    sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_MONTH;
        }
        else if( sLanguage->extractSet->matchWeek( sDateFmt->value,
                                                     sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_WEEK;
        }
        else if( sLanguage->extractSet->matchDay( sDateFmt->value,
                                                    sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_DAY;
        }
        else if( sLanguage->extractSet->matchHour( sDateFmt->value,
                                              sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_HOUR;
        }
        else if( sLanguage->extractSet->matchMinute( sDateFmt->value,
                                              sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_MINUTE;
        }
        else if( sLanguage->extractSet->matchSecond( sDateFmt->value,
                                              sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_SECOND;
        }
        else if( sLanguage->extractSet->matchMicroSec( sDateFmt->value,
                                              sDateFmt->length ) == 0 )
        {
            sDateField = MTD_DATE_DIFF_MICROSEC;
        }
        else
        {
            IDE_RAISE(ERR_INVALID_LITERAL);
        }

        IDE_TEST( mtdDateInterface::dateDiff( (mtdBigintType *)aStack[0].value,
                                              sStartDate,
                                              sEndDate,
                                              sDateField )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_INVALID_LITERAL );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_LITERAL));

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

 

/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: qsfGetSystemStats.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *
 * Syntax :
 *    GET_SYSTEM_STATS (
 *       STATNAME         VARCHAR(100),
 *       STATVALUE        DOUBLE )
 *    RETURN Integer
 *
 **********************************************************************/

#include <qdpPrivilege.h>
#include <qcmUser.h>
#include <qsf.h>
#include <qsxEnv.h>
#include <smiDef.h>
#include <smiStatistics.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdDouble;
extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 19, (void*)"SP_GET_SYSTEM_STATS" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfGetSystemStatsModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_GetSystemStats( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_GetSystemStats,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[2] =
    {
        &mtdVarchar,
        &mtdDouble
    };

    const mtdModule* sModule = &mtdInteger;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

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

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_GetSystemStats( mtcNode*     aNode,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    void*        aInfo,
                                    mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     GetSystemStats
 *
 * Implementation :
 *      Argument Validation
 *      CALL GetSystemStats
 *
 ***********************************************************************/

    mtdCharType          * sStatName;
    mtdDoubleType        * sStatValuePtr;
    mtdBigintType          sStatValue2;
    mtdIntegerType       * sReturnPtr;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /**********************************************************
     * Argument Validation
     *                     aStack, Id, NotNull, InOutType, ReturnParameter
     ***********************************************************/
    IDE_TEST( qsf::getArg( aStack, 0, ID_TRUE, QS_IN, (void**)&sReturnPtr )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 1, ID_TRUE, QS_IN, (void**)&sStatName  )    != IDE_SUCCESS );
    IDE_TEST( qsf::getArg( aStack, 2, ID_FALSE, QS_OUT, (void**)&sStatValuePtr ) != IDE_SUCCESS );


    if ( smiStatistics::isValidSystemStat() == ID_TRUE )
    {
        // smiStatistics.cpp gDBMSStatColDesc 의 이름과 동일해야 한다.
        if ( idlOS::strMatch( (const SChar*)sStatName->value,
                               sStatName->length,
                               "SREAD_TIME",
                               10 ) == 0 )
        {
            IDE_TEST( smiStatistics::getSystemStatSReadTime(
                                        ID_FALSE,
                                        sStatValuePtr )
                    != IDE_SUCCESS );
        }
        else if ( idlOS::strMatch( (const SChar*)sStatName->value,
                                    sStatName->length,
                                    "MREAD_TIME",
                                    10 ) == 0 )
        {
            IDE_TEST( smiStatistics::getSystemStatMReadTime(
                                        ID_FALSE,
                                        sStatValuePtr )
                    != IDE_SUCCESS );
        }
        else if ( idlOS::strMatch( (const SChar*)sStatName->value,
                                    sStatName->length,
                                    "MREAD_PAGE_COUNT",
                                    16 ) == 0 )
        {
            IDE_TEST( smiStatistics::getSystemStatDBFileMultiPageReadCount(
                                        ID_FALSE,
                                        &sStatValue2 )
                    != IDE_SUCCESS );

            *sStatValuePtr = sStatValue2;
        }
        else if ( idlOS::strMatch( (const SChar*)sStatName->value,
                                    sStatName->length,
                                    "HASH_TIME",
                                    9 ) == 0 )
        {
            IDE_TEST( smiStatistics::getSystemStatHashTime( sStatValuePtr ) != IDE_SUCCESS );
        }
        else if ( idlOS::strMatch( (const SChar*)sStatName->value,
                                    sStatName->length,
                                    "COMPARE_TIME",
                                    12 ) == 0 )
        {
            IDE_TEST( smiStatistics::getSystemStatCompareTime( sStatValuePtr ) != IDE_SUCCESS );
        }
        else if ( idlOS::strMatch( (const SChar*)sStatName->value,
                                    sStatName->length,
                                    "STORE_TIME",
                                    10 ) == 0 )
        {
            IDE_TEST( smiStatistics::getSystemStatStoreTime( sStatValuePtr ) != IDE_SUCCESS );
        }
        else
        {
            // nothing to do
        }
    }
    else
    {
        aStack[2].column->module->null( aStack[2].column,
                                        aStack[2].value );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

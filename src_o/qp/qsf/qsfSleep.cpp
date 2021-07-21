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
 * $Id: qsfSleep.cpp 18910 2006-11-13 01:56:34Z shsuh $
 **********************************************************************/

#include <idl.h>
#include <mte.h>
#include <mtc.h>
#include <mtd.h>
#include <qsf.h>
#include <mtk.h>
#include <mtv.h>
#include <mtdTypes.h>
#include <qtc.h>
#include <qsxUtil.h>

extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 8, (void*)"SP_SLEEP" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfSleepModule = {
    1|MTC_NODE_OPERATOR_MISC,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpSleep(
                            mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

IDE_RC qsfCalculate_SpSleep2Args( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpSleep,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

static const mtcExecute qsfExecute2Args = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpSleep2Args,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt      /* aRemain */,
                    mtcCallBack* aCallBack )
{
    UInt              sArgumentCount;
    const mtdModule * sModule[2] = { &mtdInteger, &mtdInteger };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) > 2,
                    ERR_INVALID_FUNCTION_ARGUMENT );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sModule )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     ID_SIZEOF( mtdIntegerType ),
                                     0 )
              != IDE_SUCCESS );

    sArgumentCount = ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK );

    if ( sArgumentCount == 1 )
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;
    }
    else
    {
        aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute2Args;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpSleep(
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
    qcStatement     * sStatement;
    mtdIntegerType    sSeconds;
    SInt              i;

    sStatement      = ((qcTemplate*)aTemplate)->stmt ;

    IDU_FIT_POINT( "qsfSleep::qsfCalculate_SpSleep" );
    // PSM 내에서만 호출이 가능함
    // Trigger에서는 호출되어서는 안됨
    IDE_TEST_RAISE( ( (QC_SMI_STMT(sStatement))->isDummy() == ID_FALSE ) &&
                    ( QC_SMI_STMT_SESSION_IS_JOB( sStatement ) == ID_FALSE ),
                    err_sleep_not_allowed );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        IDE_RAISE( err_argument_not_applicable );
    }

    sSeconds = *(mtdIntegerType *)aStack[1].value;

    if( sSeconds <= 0 )
    {
        IDE_RAISE( err_argument_not_applicable );
    }

    // fix BUG-17426
    for( i = 0; i < sSeconds; i++ )
    {
        *(mtdIntegerType *)aStack[0].value = idlOS::sleep( 1 );

        IDE_TEST( iduCheckSessionEvent( sStatement->mStatistics ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sleep_not_allowed );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_FUNCTION_NOT_ALLOWED));
    }
    IDE_EXCEPTION( err_argument_not_applicable );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// BUG-46481 Sleep during msec
IDE_RC qsfCalculate_SpSleep2Args( mtcNode     * aNode,
                                  mtcStack    * aStack,
                                  SInt          aRemain,
                                  void        * aInfo,
                                  mtcTemplate * aTemplate )
{
    qcStatement     * sStatement;
    mtdIntegerType    sSeconds;
    mtdIntegerType    sUSeconds;
    SInt              i;
    PDL_Time_Value    sWaitTime;

    sStatement      = ((qcTemplate*)aTemplate)->stmt ;

    // PSM 내에서만 호출이 가능함
    // Trigger에서는 호출되어서는 안됨
    IDE_TEST_RAISE( ( (QC_SMI_STMT(sStatement))->isDummy() == ID_FALSE ) &&
                    ( QC_SMI_STMT_SESSION_IS_JOB( sStatement ) == ID_FALSE ),
                    err_sleep_not_allowed );

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if ( (aStack[1].column->module->isNull( aStack[1].column,
                                            aStack[1].value ) == ID_TRUE) ||
         (aStack[2].column->module->isNull( aStack[2].column,
                                            aStack[2].value ) == ID_TRUE) )
    {
        IDE_RAISE( err_argument_not_applicable );
    }
    else
    {
        /* Nothing to do  */
    }

    sSeconds  = *(mtdIntegerType *)aStack[1].value;
    sUSeconds = *(mtdIntegerType *)aStack[2].value;

    if ( (sSeconds < 0) || (sUSeconds < 0) || (sUSeconds > 999999) )
    {
        IDE_RAISE( err_argument_not_applicable );
    }
    else
    {
        if ( ( sSeconds == 0 ) && ( sUSeconds == 0 ) )
        {
            IDE_RAISE( err_argument_not_applicable );
        }
        else
        {
            /* Nothing to do */
        }
    }

    // fix BUG-17426
    for ( i = 0; i < sSeconds; i++ )
    {
        *(mtdIntegerType *)aStack[0].value = idlOS::sleep( 1 );

        IDE_TEST( iduCheckSessionEvent( sStatement->mStatistics ) != IDE_SUCCESS );
    }

    if ( sUSeconds > 0 )
    {
        sWaitTime.set( 0, (sUSeconds) );
        *(mtdIntegerType *)aStack[0].value = idlOS::sleep( sWaitTime );
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_sleep_not_allowed );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_FUNCTION_NOT_ALLOWED));
    }
    IDE_EXCEPTION( err_argument_not_applicable );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


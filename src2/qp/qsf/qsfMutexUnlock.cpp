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
 * $Id: qsfMutexUnlock.cpp 18910 2006-11-13 01:56:34Z shsuh $
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

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
 
static mtcName qsfFunctionName[1] = {
    { NULL, 15, (void*)"SP$MUTEX$UNLOCK" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfMutexUnlockModule = {
    1|MTC_NODE_OPERATOR_MISC,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_SpMutexUnlock( 
                            mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpMutexUnlock,
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
#define IDE_FN "IDE_RC qsfEstimate"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    const mtdModule *sModule[1] = { &mtdVarchar };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 1,
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

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsfCalculate_SpMutexUnlock( 
                     mtcNode*     aNode,
                     mtcStack*    aStack,
                     SInt         aRemain,
                     void*        aInfo,
                     mtcTemplate* aTemplate )
{
#define IDE_FN "IDE_RC qsfCalculate_SpMutexUnlock"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement * sStatement;
    mtdCharType * sArg;
    SChar         sBuffer[17];
    SLong         sAddress;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;

    IDE_TEST_RAISE( ( (QC_SMI_STMT(sStatement))->isDummy() == ID_FALSE ) &&
                    ( QC_SMI_STMT_SESSION_IS_JOB( sStatement ) == ID_FALSE ),
                    err_mutex_unlock_not_allowed );
    
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
    
    sArg = (mtdCharType *)aStack[1].value;

    if( sArg->length > 16 )
    {
        IDE_RAISE( err_argument_not_applicable );
    }

    idlOS::memcpy( sBuffer, sArg->value, sArg->length );

    sBuffer[sArg->length] = '\0';

    sAddress = idlOS::strtol( sBuffer, 0, 16 );

    *(mtdIntegerType *)aStack[0].value = idlOS::thread_mutex_unlock( (PDL_thread_mutex_t *)sAddress );

    ideLog::log( IDE_QP_0, 
                 "[UNLOCK] %s %llu rc: %d errno: %d\n",
                 sBuffer, 
                 sAddress, 
                 *(mtdIntegerType *)aStack[0].value, 
                 errno );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mutex_unlock_not_allowed );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_FUNCTION_NOT_ALLOWED));
    }
    IDE_EXCEPTION( err_argument_not_applicable );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

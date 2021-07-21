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
 * $id$
 **********************************************************************/

#include <idl.h>
#include <ide.h>

#include <qci.h>
#include <mt.h>

#include <dkm.h>
#include <dkiSession.h>

#include <dkDef.h>
#include <dkErrorCode.h>

#include <dkifUtil.h>

/*
 * VARCHAR REMOTE_GET_ERROR_INFO( dblink_name IN VARCHAR,
 *                                statement_id IN BIGINT )
 *
 * Only this function is used in function or procedure.
 */

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
extern mtdModule mtdBigint;

static mtcName gFunctionName[1] =
{
    { NULL, 21, (void *)"REMOTE_GET_ERROR_INFO" }
};

/*
 * Followings are dony by this function.
 *   Is in a function or a procedure body?
 *   Do type conversion
 *   Check parameters' value
 *   Extract parameters' value
 *   Call dkm interface for this main functionality.
 *   Prepare return value and set
 *
 * aStack[0] is return value ( VARCHAR ).
 * aStack[1] is dblink_name.
 * aStack[2] is statement_id.
 */ 
static IDE_RC dkifCalculateFunction( mtcNode * aNode,
                                     mtcStack * aStack,
                                     SInt aRemain,
                                     void * aInfo,
                                     mtcTemplate * aTemplate )
{
    SChar sDblinkName[ DK_NAME_LEN + 1 ] = { };
    SLong sStatementId = 0;
    void * sQcStatement = NULL;
    dkiSession * sDkiSession = NULL;
    mtdCharType * sValue = NULL;
    dkmSession * sSession = NULL;
    
    /* BUG-36527 */
    IDE_TEST_RAISE( aRemain < 3, ERR_STACK_OVERFLOW );
    
    IDE_TEST( qciMisc::getQcStatement( aTemplate, &sQcStatement )
              != IDE_SUCCESS );
    IDE_TEST_RAISE( sQcStatement == NULL, ERR_STATEMENT_IS_NULL );
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    IDE_TEST( dkifUtilCheckNullColumn( aStack, 1 ) != IDE_SUCCESS );
    IDE_TEST( dkifUtilCheckNullColumn( aStack, 2 ) != IDE_SUCCESS );

    IDE_TEST( dkifUtilCopyDblinkName( (mtdCharType *)aStack[1].value,
                                      sDblinkName )
              != IDE_SUCCESS );

    sStatementId = *( (mtdBigintType *)aStack[2].value );
    
    IDE_TEST( qciMisc::getDatabaseLinkSession( sQcStatement,
                                               (void **)&sDkiSession )
              != IDE_SUCCESS );

    sValue = (mtdCharType *)aStack[0].value;

    sSession = dkiSessionGetDkmSession( sDkiSession );
    IDE_TEST( dkmCheckSessionAndStatus( sSession ) != IDE_SUCCESS );
    
    IDE_TEST( dkmCalculateGetRemoteErrorInfo(
                  sQcStatement,
                  sSession,
                  sDblinkName,
                  sStatementId,
                  (SChar *)sValue->value,
                  MTD_VARCHAR_PRECISION_MAXIMUM,
                  &(sValue->length) )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );        
    }
    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
}

static const mtcExecute gFunctionExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    dkifCalculateFunction,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};

/*
 * Followings are done by this function.
 *   Check number of parameters
 *   Make type conversion for parameters
 *   Initialize return value's space(?)
 *   Register execute module
 */ 
static IDE_RC dkifEstimateFunction( mtcNode * aNode,
                                    mtcTemplate * aTemplate,
                                    mtcStack * aStack,
                                    SInt /* aRemain */,
                                    mtcCallBack * aCallBack )
{
    const mtdModule * sArgumentModules[] =
    {
        &mtdVarchar,
        &mtdBigint,
    };

    const mtdModule* sReturnModule = &mtdVarchar;
    
    IDE_TEST( dkifUtilCheckNodeFlag( aNode->lflag, 2) != IDE_SUCCESS );

    IDE_TEST( mtf::makeConversionNodes( aNode,
                                        aNode->arguments,
                                        aTemplate,
                                        aStack + 1,
                                        aCallBack,
                                        sArgumentModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sReturnModule,
                                     1,
                                     MTD_VARCHAR_PRECISION_MAXIMUM,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = gFunctionExecute;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    IDE_ERRLOG( DK_TRC_LOG_ERROR );
    
    return IDE_FAILURE;
    
}

/*
 * Function module.
 */ 
mtfModule dkifRemoteGetErrorInfo =
{
    1 | MTC_NODE_OPERATOR_MISC | MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    gFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    dkifEstimateFunction
};

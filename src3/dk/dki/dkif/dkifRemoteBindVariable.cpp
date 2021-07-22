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
 * INTEGER REMOTE_BIND_VARIABLE( dblink_name IN VARCHAR,
 *                               statement_id IN BIGINT,
 *                               bind_variable_index IN INTEGER,
 *                               value IN VARCHAR )
 *
 * Only this function is used in function or procedure.
 */

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
extern mtdModule mtdBigint;

static mtcName gFunctionName[1] =
{
    { NULL, 20, (void *)"REMOTE_BIND_VARIABLE" }
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
 * aStack[0] is return value ( INTEGER ).
 * aStack[1] is dblink_name.
 * aStack[2] is statement_id.
 * aStack[3] is bind_variable_index
 * aStack[4] is value.
 */ 
static IDE_RC dkifCalculateFunction( mtcNode * aNode,
                                     mtcStack * aStack,
                                     SInt aRemain,
                                     void * aInfo,
                                     mtcTemplate * aTemplate )
{
    SChar sDblinkName[ DK_NAME_LEN + 1] = { };
    SLong sStatementId = 0;
    SInt sBindVariableIndex = 0;
    SChar * sValue = NULL;
    SInt sResult = 0;
    SInt sStage = 0;
    void * sQcStatement = NULL;
    dkiSession * sDkiSession = NULL;
    dkmSession * sSession = NULL;
    
    /* BUG-36527 */
    IDE_TEST_RAISE( aRemain < 5, ERR_STACK_OVERFLOW );
    
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
    IDE_TEST( dkifUtilCheckNullColumn( aStack, 3 ) != IDE_SUCCESS );
    IDE_TEST( dkifUtilCheckNullColumn( aStack, 4 ) != IDE_SUCCESS );

    IDE_TEST( dkifUtilCopyDblinkName( (mtdCharType *)aStack[1].value,
                                      sDblinkName )
              != IDE_SUCCESS );

    IDE_TEST( dkifUtilConvertMtdCharToCString( (mtdCharType *)aStack[4].value,
                                               &sValue )
              != IDE_SUCCESS );
    sStage = 1;

    sStatementId = *( (mtdBigintType *)aStack[2].value );
    
    sBindVariableIndex = *( (mtdIntegerType *)aStack[3].value );
    
    IDE_TEST( qciMisc::getDatabaseLinkSession( sQcStatement,
                                               (void **)&sDkiSession )
              != IDE_SUCCESS );

    sSession = dkiSessionGetDkmSession( sDkiSession );
    IDE_TEST( dkmCheckSessionAndStatus( sSession ) != IDE_SUCCESS );
    
    IDE_TEST( dkmCalculateBindVariable(
                  sQcStatement,
                  sSession,
                  sDblinkName,
                  sStatementId,
                  sBindVariableIndex,
                  sValue )
              != IDE_SUCCESS );
    
    *( (mtdIntegerType *)aStack[0].value ) = sResult;
    
    sStage = 0;
    (void)dkifUtilFreeCString( sValue );
    
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

    IDE_PUSH();
        
    switch ( sStage )
    {
        case 1:
            (void)dkifUtilFreeCString( sValue );
        case 0:
        default:
            break;
    }

    IDE_POP();

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
        &mtdInteger,
        &mtdVarchar
    };

    const mtdModule* sReturnModule = &mtdInteger;
    
    IDE_TEST( dkifUtilCheckNodeFlag( aNode->lflag, 4) != IDE_SUCCESS );

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
                                     0,
                                     0,
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
mtfModule dkifRemoteBindVariable =
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

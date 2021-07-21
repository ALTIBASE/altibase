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
 * CHAR( char_size ) DBLINK_GET_COLUMN_VALUE_CHAR ( dblink_name IN VARCHAR,
 *                                                  statement_id IN BIGINT,
 *                                                  column_idx IN INTEGER,
 *                                                  char_size IN INTEGER )
 *
 * char_size is a constant value.
 * 
 * Only this function is used in function or procedure.
 */

extern mtdModule mtdVarchar;
extern mtdModule mtdChar;
extern mtdModule mtdInteger;
extern mtdModule mtdBigint;

static mtcName gFunctionName[1] =
{
    { NULL, 28, (void *)"REMOTE_GET_COLUMN_VALUE_CHAR" }
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
 * aStack[0] is return value ( CHAR ).
 * aStack[1] is dblink_name.
 * aStack[2] is statement_id.
 * aStack[3] is column_idx.
 * aStack[4] is char_size.
 */ 
static IDE_RC dkifCalculateFunction( mtcNode * aNode,
                                     mtcStack * aStack,
                                     SInt aRemain,
                                     void * aInfo,
                                     mtcTemplate * aTemplate )
{
    SChar sDblinkName[ DK_NAME_LEN + 1] = {0,};
    SLong sStatementId = 0;
    SInt sColumnIndex = 0;
    void * sQcStatement = NULL;
    dkiSession * sDkiSession = NULL;
    mtcColumn * sColumn = NULL;
    void * sValue = NULL;
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

    IDE_TEST( dkifUtilCopyDblinkName( (mtdCharType *)aStack[1].value,
                                      sDblinkName )
              != IDE_SUCCESS );

    sStatementId = *( (mtdBigintType *)aStack[2].value );

    sColumnIndex = *( (mtdIntegerType *)aStack[3].value );
    
    IDE_TEST( qciMisc::getDatabaseLinkSession( sQcStatement,
                                               (void **)&sDkiSession )
              != IDE_SUCCESS );

    sSession = dkiSessionGetDkmSession( sDkiSession );
    IDE_TEST( dkmCheckSessionAndStatus( sSession ) != IDE_SUCCESS );
    
    IDE_TEST( dkmCalculateGetColumnValue(
                  sQcStatement,
                  sSession,
                  sDblinkName,
                  sStatementId,
                  sColumnIndex,
                  &sColumn,
                  &sValue )
              != IDE_SUCCESS );

    /* BUG-36527 */
    IDE_TEST_RAISE( sColumn->module->id != MTD_CHAR_ID, ERR_WRONG_TYPE );

    /*
     * return value
     */ 
    idlOS::memcpy( aStack[0].value,
                   sValue,
                   sColumn->module->actualSize( sColumn, sValue ) );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_STACK_OVERFLOW )
    {
        IDE_SET( ideSetErrorCode( mtERR_ABORT_STACK_OVERFLOW ) );        
    }
    IDE_EXCEPTION( ERR_STATEMENT_IS_NULL )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DKI_STATEMENT_IS_NULL ) );
    }
    IDE_EXCEPTION( ERR_WRONG_TYPE )
    {
        IDE_SET( ideSetErrorCode( dkERR_ABORT_DK_MISMATCHED_COLUMN_TYPE ) );
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
        &mtdInteger,
        &mtdInteger,
    };

    const mtdModule * sReturnModule = &mtdChar;
    
    SInt sSize;

    IDE_TEST( dkifUtilCheckNodeFlag( aNode->lflag, 4) != IDE_SUCCESS );

    IDE_TEST( dkifUtilGetIntegerValueFromNthArgument( aTemplate,
                                                      aNode,
                                                      4,
                                                      &sSize )
              != IDE_SUCCESS );
    
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
                                     sSize,
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
mtfModule dkifRemoteGetColumnValueChar =
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

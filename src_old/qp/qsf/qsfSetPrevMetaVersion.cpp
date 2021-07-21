/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: qsfPrint.cpp 67758 2014-12-01 09:59:34Z khkwak $
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
#include <qcmDatabase.h>

extern mtdModule mtdInteger;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 18, (void*)"SP_SET_PREVMETAVER" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfSetPrevMetaVerModule = {
    1|MTC_NODE_OPERATOR_MISC,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};

IDE_RC qsfCalculate_SpSetPrevMetaVer( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpSetPrevMetaVer,
    NULL,
    mtx::calculateNA,
    mtk::estimateRangeNA,
    mtk::extractRangeNA
};


IDE_RC qsfEstimate( mtcNode*     aNode,
                    mtcTemplate* aTemplate,
                    mtcStack*    aStack,
                    SInt         /* aRemain */,
                    mtcCallBack* aCallBack )
{
    const mtdModule* sModules[3] =
    {
        &mtdInteger,
        &mtdInteger,
        &mtdInteger
    };

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 3,
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
                                     &mtdBoolean,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpSetPrevMetaVer( mtcNode*     aNode,
                                      mtcStack*    aStack,
                                      SInt         aRemain,
                                      void*        aInfo,
                                      mtcTemplate* aTemplate )
{
    qcStatement          * sStatement = NULL;
    mtdIntegerType         sMajorVer; 
    mtdIntegerType         sMinorVer;
    mtdIntegerType         sPatchVer;

    sStatement = ((qcTemplate*)aTemplate)->stmt ;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    /**************************
     * Arguemnt validate
     **************************/
    if ( ( aStack[1].column->module->isNull( aStack[1].column,
                                             aStack[1].value ) == ID_TRUE ) ||
         ( aStack[2].column->module->isNull( aStack[2].column,
                                             aStack[2].value ) == ID_TRUE ) ||
         ( aStack[3].column->module->isNull( aStack[3].column,
                                             aStack[3].value ) == ID_TRUE ) )
    {
        IDE_RAISE( err_argument_not_applicable );
    }
    else
    {
        sMajorVer = *(mtdIntegerType *)aStack[1].value;
        sMinorVer = *(mtdIntegerType *)aStack[2].value;
        sPatchVer = *(mtdIntegerType *)aStack[3].value;
    }

    /**************************
     * check privilege 
     **************************/
    IDE_TEST_RAISE( QCG_GET_SESSION_USER_IS_SYSDBA(sStatement) != ID_TRUE,
                    ERR_NO_GRANT_ALTER_DATABASE );

    /**************************
     * update
     **************************/
    IDE_TEST( qcmDatabase::setMetaPrevVersion( sStatement,
                                               (UInt)sMajorVer,
                                               (UInt)sMinorVer,
                                               (UInt)sPatchVer)
              != IDE_SUCCESS );

    *(mtdBooleanType *)aStack[0].value = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_argument_not_applicable );
    {
    IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_NO_GRANT_ALTER_DATABASE );
    {
        IDE_SET( ideSetErrorCode( qpERR_ABORT_QDP_INSUFFICIENT_PRIVILEGES,
                                  QCM_PRIV_NAME_SYSTEM_ALTER_DATABASE_STR ) );
    }
    IDE_EXCEPTION_END;

    *(mtdBooleanType *)aStack[0].value = 1;
    
    return IDE_FAILURE;
}


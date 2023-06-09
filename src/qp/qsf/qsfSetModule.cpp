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
 * $Id: qsfSetModule.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
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
#include <qsvEnv.h>

extern mtdModule mtdInteger;
extern mtdModule mtdVarchar;

static mtcName qsfFunctionName[1] = {
    { NULL, 13, (void*)"SP_SET_MODULE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfSetModuleModule = {
    1|MTC_NODE_OPERATOR_MISC,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};

IDE_RC qsfCalculate_SpSetModule( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_SpSetModule,
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
    qcStatement     * sStatement;
    const mtdModule * sModule[2] = { &mtdVarchar, &mtdVarchar };
    
    sStatement = ((qcTemplate*)aTemplate)->stmt;
    // 반드시 psm 내부에서만 사용.
    IDE_TEST_RAISE( ( sStatement->spvEnv->createProc == NULL ) &&
                    ( sStatement->spvEnv->createPkg == NULL ),
                    ERR_NOT_ALLOWED );

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
                                        sModule )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     &mtdInteger,
                                     0,
                                     0,
                                     0 )
              != IDE_SUCCESS );

    aTemplate->rows[aNode->table].execute[aNode->column] = qsfExecute;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_ALLOWED );
    IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_FUNCTION_NOT_ALLOWED));

    IDE_EXCEPTION( ERR_NOT_AGGREGATION );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_NOT_AGGREGATION));

    IDE_EXCEPTION( ERR_INVALID_FUNCTION_ARGUMENT );
    IDE_SET(ideSetErrorCode(mtERR_ABORT_INVALID_FUNCTION_ARGUMENT));

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC qsfCalculate_SpSetModule( mtcNode*     aNode,
                                 mtcStack*    aStack,
                                 SInt         aRemain,
                                 void*        aInfo,
                                 mtcTemplate* aTemplate )
{
    qcStatement     * sStatement;
    

    mtdCharType     * sModule;
    mtdCharType     * sAction;
    
    UInt              sArg1Length = 0;
    UInt              sArg2Length = 0;
    UInt              sModuleStrLength = 0;
    UInt              sActionStrLength = 0;
    
    const mtlModule * sModuleStrLanguage;
    const mtlModule * sActionStrLanguage;
    
    sStatement      = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    sModule = (mtdCharType *)aStack[1].value;
    sAction = (mtdCharType *)aStack[2].value;
    
    sModuleStrLanguage = aStack[1].column->language;
    sActionStrLanguage = aStack[2].column->language;
    
    sArg1Length = IDL_MIN( sModule->length, QC_MAX_OBJECT_NAME_LEN );
    sArg2Length = IDL_MIN( sAction->length, QC_MAX_OBJECT_NAME_LEN );
    
    IDE_TEST( mtf::truncIncompletedString( sModule->value,
                                           sArg1Length,
                                           &sModuleStrLength,
                                           sModuleStrLanguage )
              != IDE_SUCCESS );
    
    IDE_TEST( QCG_SET_SESSION_MODULE_INFO( sStatement,
                                           (SChar *)sModule->value,
                                           sModuleStrLength )
              != IDE_SUCCESS );
    
    IDE_TEST( mtf::truncIncompletedString( sAction->value,
                                           sArg2Length,
                                           &sActionStrLength,
                                           sActionStrLanguage )
              != IDE_SUCCESS );

    IDE_TEST( QCG_SET_SESSION_ACTION_INFO( sStatement,
                                           (SChar *)sAction->value,
                                           sActionStrLength )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}
 

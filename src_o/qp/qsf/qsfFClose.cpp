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
 * $Id: qsfFClose.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE을 close하는 함수
 *
 * Syntax :
 *     FILE_CLOSE( file FILE_TYPE );
 *     return FILE_TYPE
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <qsxEnv.h>
#include <iduFileStream.h>
#include <qcuSessionObj.h>

extern mtdModule mtsFile;

static mtcName qsfFunctionName[1] = {
    { NULL, 10, (void*)"FILE_CLOSE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFCloseModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FClose( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FClose,
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

    const mtdModule* sModule = &mtsFile;

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
                                        &sModule )
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

#undef IDE_FN
}

IDE_RC qsfCalculate_FClose( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_close calculate
 *
 * Implementation :
 *     1. argument가 null인 경우 아무것도 하지 않고 success
 *     2. file을 close하고, filelist에서 제거
 *     3. return value는 null로 세팅
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FClose"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement    * sStatement;
    qcSession      * sSession;    
    mtsFileType    * sFileHandle;
    idBool           sExist;
    
    sStatement    = ((qcTemplate*)aTemplate)->stmt;

    sSession  = sStatement->spxEnv->mSession;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        // Nothing to do
    }
    else
    {
        sFileHandle = (mtsFileType*)aStack[1].value;
        
        IDE_TEST( qcuSessionObj::delOpenedFile(
            (qcSessionObjInfo*)(sSession->mQPSpecific.mSessionObj),
            sFileHandle->fp,
            &sExist )
                  != IDE_SUCCESS );

        if( sExist == ID_TRUE )
        {
            IDE_TEST( iduFileStream::closeFile( sFileHandle->fp )
                      != IDE_SUCCESS );
        }
        else
        {
            // FCLOSE_ALL 등으로 이미 파일이 닫혀있는 경우임.
            // Nothing to do
        }
    }

    aStack[0].column->module->null( aStack[0].column,
                                    aStack[0].value );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

 

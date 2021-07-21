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
 * $Id: qsfFFlush.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE buffer의 데이터를 flush하는 함수
 *
 * Syntax :
 *     FILE_FLUSH( file FILE_TYPE );
 *     RETURN BOOLEAN
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <iduFileStream.h>

extern mtdModule mtsFile;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 10, (void*)"FILE_FLUSH" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFFlushModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FFlush( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FFlush,
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

    const mtdModule* sArgModule = &mtsFile;
    const mtdModule* sRetModule = &mtdBoolean;

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
                                        &sArgModule )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sRetModule,
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

IDE_RC qsfCalculate_FFlush( mtcNode*     aNode,
                            mtcStack*    aStack,
                            SInt         aRemain,
                            void*        aInfo,
                            mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_flush calculate
 *
 * Implementation :
 *     1. filehandle이 null이면 open되지 않은 상태로 간주. error
 *     2. flush함수 호출
 *     3. return value는 dummy, TRUE로 세팅
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FFlush"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtsFileType    * sFileHandle;
    mtdBooleanType * sReturnValue;
    
    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );
    
    if( aStack[1].column->module->isNull( aStack[1].column,
                                          aStack[1].value ) == ID_TRUE )
    {
        // INVALID FILEHANDLE ERROR
        IDE_RAISE(ERR_INVALID_FILEHANDLE);
    }
    else
    {
        // Nothing to do
    }

    sFileHandle = (mtsFileType*)aStack[1].value;
    sReturnValue = (mtdBooleanType*)aStack[0].value;

    *sReturnValue = MTD_BOOLEAN_TRUE;

    IDE_TEST( iduFileStream::flushFile( sFileHandle->fp )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_FILEHANDLE);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_FILEHANDLE));
    }
        
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

 

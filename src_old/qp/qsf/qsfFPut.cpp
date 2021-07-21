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
 * $Id: qsfFPut.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE에 string을 기록하는 함수
 *
 * Syntax :
 *     FILE_PUT( file FILE_TYPE, buffer VARCHAR, autoflush BOOLEAN )
 *     RETURN BOOLEAN
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <iduFileStream.h>

extern mtdModule mtdVarchar;
extern mtdModule mtsFile;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 8, (void*)"FILE_PUT" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFPutModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FPut( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FPut,
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

    const mtdModule* sModules[3] =
    {
        &mtsFile,
        &mtdVarchar,
        &mtdBoolean
    };

    const mtdModule* sModule = &mtdBoolean;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

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

IDE_RC qsfCalculate_FPut( mtcNode*     aNode,
                          mtcStack*    aStack,
                          SInt         aRemain,
                          void*        aInfo,
                          mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_put calculate
 *
 * Implementation :
 *     1. filehandle이 null인 경우  error
 *     2. buffer가 null인 경우 아무것도 하지 않고 success
 *     3. autoflush가 null인 경우 error
 *     4. open mode가 w, a가 아닌 경우 error
 *     5. putString함수 호출하여 파일에 기록
 *     6. autoflush가 TRUE이면 flush
 *     7. return value는 dummy로, TRUE로 세팅
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FPut"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement    * sStatement;
    mtsFileType    * sFileHandle;
    mtdCharType    * sCharValue;
    mtdBooleanType * sFlush;
    mtdBooleanType * sReturnValue;
    SChar          * sCharBuffer;
    
    sStatement    = ((qcTemplate*)aTemplate)->stmt;

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

    if( aStack[3].column->module->isNull( aStack[3].column,
                                          aStack[3].value ) == ID_TRUE )
    {
        IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
    }
    else
    {
        // Nothing to do.
    }

    sFileHandle = (mtsFileType*)aStack[1].value;

    if( ( sFileHandle->mode[0] != 'w' ) &&
        ( sFileHandle->mode[0] != 'a' ) )
    {
        IDE_RAISE(WRITE_ERROR);
    }
    else
    {
        // Nothing to do.
    }
    
    sCharValue = (mtdCharType*)aStack[2].value;
    sFlush      = (mtdBooleanType*)aStack[3].value;      
    sReturnValue = (mtdBooleanType*)aStack[0].value;

    *sReturnValue = MTD_BOOLEAN_TRUE;
    
    if( aStack[2].column->module->isNull( aStack[2].column,
                                          aStack[2].value ) == ID_TRUE )
    {
        return IDE_SUCCESS;
    }
    else
    {
        // Nothing to do.
    }

    IDU_FIT_POINT( "qsfFPut::qsfCalculate_FPut::alloc::CharBuffer" );
    IDE_TEST( sStatement->qmxMem->alloc(
                         sCharValue->length + 1,
                         (void**)&sCharBuffer )
              != IDE_SUCCESS );

    idlOS::strncpy( sCharBuffer,
                    (SChar*)sCharValue->value,
                    sCharValue->length );

    *(sCharBuffer + sCharValue->length) = '\0';

    IDE_TEST( iduFileStream::putString( sCharBuffer,
                                        sFileHandle->fp )
              != IDE_SUCCESS );

    if( *sFlush == MTD_BOOLEAN_TRUE )
    {
        IDE_TEST( iduFileStream::flushFile( sFileHandle->fp )
                  != IDE_SUCCESS );
    }
    else
    {
        // Nothing to do.
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_FILEHANDLE);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_FILEHANDLE));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( WRITE_ERROR );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_WRITE_ERROR));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

 

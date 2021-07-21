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
 * $Id: qsfFGetLine.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE에서 한 line을 얻어오는 함수
 *
 * Syntax :
 *     FILE_GETLINE( file FILE_TYPE,linesize INTEGER );
 *     RETURN VARCHAR
 *
 **********************************************************************/

#include <qsf.h>
#include <qc.h>
#include <iduFileStream.h>

#define QSF_MAX_LINESIZE     (32768)
#define QSF_DEFAULT_LINESIZE  (1024)

extern mtdModule mtdVarchar;
extern mtdModule mtdInteger;
extern mtdModule mtsFile;

static mtcName qsfFunctionName[1] = {
    { NULL, 12, (void*)"FILE_GETLINE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFGetLineModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FGetLine( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FGetLine,
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

    const mtdModule* sModules[2] =
    {
        &mtsFile,
        &mtdInteger
    };

    const mtdModule* sModule = &mtdVarchar;

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
                                        sModules )
              != IDE_SUCCESS );

    aStack[0].column = aTemplate->rows[aNode->table].columns + aNode->column;

    // varchar모듈로 estimate해야함.. precision은 32768

    IDE_TEST( mtc::initializeColumn( aStack[0].column,
                                     sModule,
                                     1,
                                     QSF_MAX_LINESIZE,
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

IDE_RC qsfCalculate_FGetLine( mtcNode*     aNode,
                              mtcStack*    aStack,
                              SInt         aRemain,
                              void*        aInfo,
                              mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_getline calculate
 *
 * Implementation :
 *     1. filehandle이 null인 경우 open상태가 아니므로 error
 *     2. openmode가 'r'이 아닌 경우 error
 *     3. 최대 linesize를 넘거나, 1보다 작으면 error
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FGetLine"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtsFileType    * sFileHandle;
    mtdIntegerType   sLength;
    mtdCharType    * sReturnValue;

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
        sFileHandle = (mtsFileType*)aStack[1].value;
    }

    IDE_TEST_RAISE( sFileHandle->mode[0] != 'r', READ_ERROR );
        
    if( aStack[2].column->module->isNull( aStack[2].column,
                                          aStack[2].value ) == ID_TRUE )
    {
        sLength = QSF_DEFAULT_LINESIZE;
    }
    else
    {
        sLength = *(mtdIntegerType*)aStack[2].value;
        if( ( sLength < 1 ) || ( sLength >= QSF_MAX_LINESIZE ) )
        {
            // argument not applicable
            IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
        }
        else
        {
            // Nothing to do.
        }
    }

    sReturnValue = (mtdCharType*)aStack[0].value;

    IDE_TEST( iduFileStream::getString( (SChar*)sReturnValue->value,
                                        sLength,
                                        sFileHandle->fp )
              != IDE_SUCCESS );
    
    sReturnValue->length = idlOS::strlen((SChar*)sReturnValue->value);
        
    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_FILEHANDLE);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_FILEHANDLE));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( READ_ERROR );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_READ_ERROR));
    }
        
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

 

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
 * $Id: qsfFRemove.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE을 삭제하는 함수
 *
 * Syntax :
 *     FILE_REMOVE( path VARCHAR, filename VARCHAR );
 *     RETURN BOOLEAN
 *
 **********************************************************************/

#include <qsf.h>
#include <qcmDirectory.h>
#include <qdpPrivilege.h>
#include <iduFileStream.h>
#include <qdpRole.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 11, (void*)"FILE_REMOVE" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFRemoveModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FRemove( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FRemove,
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
        &mtdVarchar,
        &mtdVarchar,
    };

    const mtdModule* sModule = &mtdBoolean;

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

IDE_RC qsfCalculate_FRemove( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_remove calculate
 *
 * Implementation :
 *     1. argument가 null인 경우  error
 *     2. directory path를 meta에서 얻어옴
 *     3. write 권한이 있는지 검사
 *     4. open mode가 w, a가 아닌 경우 error
 *     5. path, filename, directory indicator를 조합하여 path생성
 *     6. remove함수 호출
 *     7. return value는 dummy로, TRUE로 세팅
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FRemove"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement    * sStatement;
    mtdCharType    * sPathValue;  // Path
    mtdCharType    * sFilenameValue;  // Filename
    mtdBooleanType * sReturnValue;
    SChar          * sFilePath;
    smiStatement   * sDummyStmt;
    smiStatement     sStmt;
    qcmDirectoryInfo sDirInfo;
    idBool           sExist;
    UInt             sSmiStmtFlag = 0;
    SInt             sStage = 0;
    
    sStatement    = ((qcTemplate*)aTemplate)->stmt;

    IDE_TEST( mtf::postfixCalculate( aNode,
                                     aStack,
                                     aRemain,
                                     aInfo,
                                     aTemplate )
              != IDE_SUCCESS );

    if( (aStack[1].column->module->isNull( aStack[1].column,
                                           aStack[1].value ) == ID_TRUE) ||
        (aStack[2].column->module->isNull( aStack[2].column,
                                           aStack[2].value ) == ID_TRUE) )
    {
        // Value Error
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        // Nothing to do.
    }
    
    sReturnValue = (mtdBooleanType*)aStack[0].value;

    *sReturnValue = MTD_BOOLEAN_TRUE;
        
    // path
    sPathValue = (mtdCharType*)aStack[1].value;
        
    // 메타에서 실제 path를 얻어옴
    sDummyStmt = QC_SMI_STMT(sStatement);

    sSmiStmtFlag &= ~SMI_STATEMENT_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_UNTOUCHABLE;
        
    sSmiStmtFlag &= ~SMI_STATEMENT_CURSOR_MASK;
    sSmiStmtFlag |= SMI_STATEMENT_MEMORY_CURSOR;

    QC_SMI_STMT(sStatement) = &sStmt;
    sStage = 1;
        
    IDE_TEST( sStmt.begin( mtc::getStatistics( aTemplate ), sDummyStmt, sSmiStmtFlag )
              != IDE_SUCCESS);
    sStage = 2;
        
    IDE_TEST( qcmDirectory::getDirectory( sStatement,
                                          (SChar*)sPathValue->value,
                                          sPathValue->length,
                                          &sDirInfo,
                                          &sExist )
              != IDE_SUCCESS );

        
    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_INVALID_PATH );

    // check grant
    IDE_TEST_RAISE( qdpRole::checkDMLWriteDirectoryPriv(
                        sStatement,
                        sDirInfo.directoryID,
                        sDirInfo.userID )
                    != IDE_SUCCESS, ERR_ACCESS_DENIED );

    sStage = 1;
    IDE_TEST( sStmt.end(SMI_STATEMENT_RESULT_SUCCESS)
              != IDE_SUCCESS );

    sStage = 0;
    QC_SMI_STMT(sStatement) = sDummyStmt;

    // filename
    sFilenameValue = (mtdCharType*)aStack[2].value;

    IDE_TEST( qsf::makeFilePath( sStatement->qmxMem,
                                 &sFilePath,
                                 sDirInfo.directoryPath,
                                 sFilenameValue )
              != IDE_SUCCESS );
            
    // iduFileStream::removefile호출
    IDE_TEST( iduFileStream::removeFile( sFilePath )
              != IDE_SUCCESS );
         
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_PATH );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_PATH));
    }
    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION( ERR_ACCESS_DENIED );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_DIRECTORY_ACCESS_DENIED));
    }
    
    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 2:
            if ( sStmt.end(SMI_STATEMENT_RESULT_FAILURE) != IDE_SUCCESS )
            {
                IDE_ERRLOG(IDE_QP_1);
            }
        case 1:
            QC_SMI_STMT(sStatement) = sDummyStmt;
        default:
            break;
    }
    
    return IDE_FAILURE;
    
#undef IDE_FN
}

 

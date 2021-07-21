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
 * $Id: qsfFRename.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE을 rename(mv)하는 함수
 *
 * Syntax :
 *     FILE_RENAME( srcpath VARCHAR, srcfilename VARCHAR,
 *                  destpath VARCHAR, destfilename VARCAHR,
 *                  overwrite BOOLEAN );
 *     RETURN BOOLEAN
 *
 **********************************************************************/

#include <qsf.h>
#include <qcmDirectory.h>
#include <qdpPrivilege.h>
#include <iduFileStream.h>
#include <qcg.h>
#include <qdpRole.h>

extern mtdModule mtdVarchar;
extern mtdModule mtdBoolean;

static mtcName qsfFunctionName[1] = {
    { NULL, 11, (void*)"FILE_RENAME" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFRenameModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FRename( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FRename,
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

    const mtdModule* sModules[5] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdBoolean
    };

    const mtdModule* sModule = &mtdBoolean;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 5,
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

IDE_RC qsfCalculate_FRename( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_rename calculate
 *
 * Implementation :
 *     1. argument가 null인 경우  error
 *     2. directory path를 얻어옴
 *     3. src, dest path에 대해 write권한 체크
 *     5. path, filename, directory indicator를 조합하여 path생성
 *     6. rename 함수 호출
 *     7. return value에 TRUE세팅
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FRename"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement    * sStatement;
    mtdCharType    * sSrcPathValue;
    mtdCharType    * sSrcFilenameValue;
    mtdCharType    * sDestPathValue;
    mtdCharType    * sDestFilenameValue;
    idBool           sOverWrite;
    mtdBooleanType * sReturnValue;
    SChar          * sSrcFilePath;
    SChar          * sDestFilePath;
    qcmDirectoryInfo sSrcDirInfo;
    qcmDirectoryInfo sDestDirInfo;
    idBool           sExist;
    
    smiStatement   * sDummyStmt;
    smiStatement     sStmt;
    
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
                                           aStack[2].value ) == ID_TRUE) ||
        (aStack[3].column->module->isNull( aStack[3].column,
                                           aStack[3].value ) == ID_TRUE) ||
        (aStack[4].column->module->isNull( aStack[4].column,
                                           aStack[4].value ) == ID_TRUE) ||
        (aStack[5].column->module->isNull( aStack[5].column,
                                           aStack[5].value ) == ID_TRUE) )
    {
        // Value Error
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        // Nothing to do.
    }
    
    sReturnValue = (mtdBooleanType*)aStack[0].value;
    *sReturnValue = MTD_BOOLEAN_NULL;
                
        
    // path
    sSrcPathValue = (mtdCharType*)aStack[1].value;
    sDestPathValue = (mtdCharType*)aStack[3].value;
        
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
                                          (SChar*)sSrcPathValue->value,
                                          sSrcPathValue->length,
                                          &sSrcDirInfo,
                                          &sExist )
              != IDE_SUCCESS );
        
    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_INVALID_PATH );

    IDE_TEST_RAISE( qdpRole::checkDMLWriteDirectoryPriv(
                        sStatement,
                        sSrcDirInfo.directoryID,
                        sSrcDirInfo.userID )
                    != IDE_SUCCESS, ERR_ACCESS_DENIED );    

    IDE_TEST( qcmDirectory::getDirectory( sStatement,
                                          (SChar*)sDestPathValue->value,
                                          sDestPathValue->length,
                                          &sDestDirInfo,
                                          &sExist )
              != IDE_SUCCESS );
    
    IDE_TEST_RAISE( sExist == ID_FALSE, ERR_INVALID_PATH );

    IDE_TEST_RAISE( qdpRole::checkDMLWriteDirectoryPriv(
                        sStatement,
                        sDestDirInfo.directoryID,
                        sDestDirInfo.userID )
                    != IDE_SUCCESS, ERR_ACCESS_DENIED );    

    sStage = 1;
    IDE_TEST( sStmt.end(SMI_STATEMENT_RESULT_SUCCESS)
              != IDE_SUCCESS );

    sStage = 0;
    QC_SMI_STMT(sStatement) = sDummyStmt;

    // src file
    sSrcFilenameValue = (mtdCharType*)aStack[2].value;

    IDE_TEST( qsf::makeFilePath( sStatement->qmxMem,
                                 &sSrcFilePath,
                                 sSrcDirInfo.directoryPath,
                                 sSrcFilenameValue )
              != IDE_SUCCESS );
    
    // dest file        
    sDestFilenameValue = (mtdCharType*)aStack[4].value;
    
    IDE_TEST( qsf::makeFilePath( sStatement->qmxMem,
                                 &sDestFilePath,
                                 sDestDirInfo.directoryPath,
                                 sDestFilenameValue )
              != IDE_SUCCESS );

    //overwrite
    IDE_TEST( aStack[5].column->module->isTrue( &sOverWrite,
                                                aStack[5].column,
                                                aStack[5].value )
              != IDE_SUCCESS );

    IDE_TEST( iduFileStream::renameFile( sSrcFilePath,
                                         sDestFilePath,
                                         sOverWrite )
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

 

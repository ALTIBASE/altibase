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
 * $Id: qsfFOpen.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE을 open하는 함수
 *
 * Syntax :
 *    FILE_OPEN( path VARCHAR, filename VARCHAR, openmode VARCHAR );
 *    RETURN FILE_TYPE
 *
 **********************************************************************/

#include <qsf.h>
#include <qsxEnv.h>
#include <qcmDirectory.h>
#include <qdpPrivilege.h>
#include <iduFileStream.h>
#include <qcuSessionObj.h>
#include <qdpRole.h>

extern mtdModule mtdVarchar;
extern mtdModule mtsFile;

static mtcName qsfFunctionName[1] = {
    { NULL, 9, (void*)"FILE_OPEN" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFOpenModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};


IDE_RC qsfCalculate_FOpen( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FOpen,
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
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar
    };

    const mtdModule* sModule = &mtsFile;

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

IDE_RC qsfCalculate_FOpen( mtcNode*     aNode,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           void*        aInfo,
                           mtcTemplate* aTemplate )
{
/***********************************************************************
 *
 * Description :
 *     file_open calculate
 *
 * Implementation :
 *     1. argument가 null인 경우  error
 *     2. openmode가 'r', 'w', 'a' 가 아닌 경우 error
 *     3. directory path를 얻어옴
 *     4. open mode에 따라 read, write 권한 검사
 *     5. path, filename, directory indicator를 조합하여 path생성
 *     6. file을 open하고, return value에 세팅
 *     7. filelist에 open된 filehandle 삽입
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FOpen"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement    * sStatement;
    qcSession      * sSession;
    mtdCharType    * sPathValue;  // Path
    mtdCharType    * sFilenameValue;  // Filename
    mtdCharType    * sOpenmodeValue;  // Open mode
    mtsFileType    * sFileType;
    SChar          * sFilePath;
    smiStatement   * sDummyStmt;
    smiStatement     sStmt;
    qcmDirectoryInfo sDirInfo;
    idBool           sExist;
    UInt             sSmiStmtFlag = 0;
    SInt             sStage = 0;
    
    sStatement   = ((qcTemplate*)aTemplate)->stmt;

    sSession = sStatement->spxEnv->mSession;

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
                                           aStack[3].value ) == ID_TRUE) )
    {
        // Value Error
        IDE_RAISE( ERR_ARGUMENT_NOT_APPLICABLE );
    }
    else
    {
        sFileType = (mtsFileType*)aStack[0].value;

        // mode
        sOpenmodeValue = (mtdCharType*)aStack[3].value;

        // 길이가 1이 아니면 에러 ex) r, w, a, R, W, A 중 하나여야 함
        if( sOpenmodeValue->length == 1 )
        {
            /* BUG-39273
               open mode 대소문자 모두 지원 */
            if( ( sOpenmodeValue->value[0] == 'r' ) ||
                ( sOpenmodeValue->value[0] == 'w' ) ||
                ( sOpenmodeValue->value[0] == 'a' ) )
            {
                sFileType->mode[0] = sOpenmodeValue->value[0];
            }
            else if ( sOpenmodeValue->value[0] == 'R')
            {
                sFileType->mode[0] = 'r';
            }
            else if ( sOpenmodeValue->value[0] == 'W' )
            {
                sFileType->mode[0] = 'w';
            }
            else if ( sOpenmodeValue->value[0] == 'A' )
            {
                sFileType->mode[0] = 'a';
            }
            else
            {
                // error invalid mode
                IDE_RAISE( ERR_INVALID_MODE );
            }
        }
        else
        {
            // error invalid mode
            IDE_RAISE( ERR_INVALID_MODE );
        }
        sFileType->mode[1] = '\0';
        
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

        if( sFileType->mode[0] == 'r' )
        {
            IDE_TEST_RAISE( qdpRole::checkDMLReadDirectoryPriv(
                                sStatement,
                                sDirInfo.directoryID,
                                sDirInfo.userID )
                            != IDE_SUCCESS, ERR_ACCESS_DENIED );
        }
        else
        {
            IDE_TEST_RAISE( qdpRole::checkDMLWriteDirectoryPriv(
                                sStatement,
                                sDirInfo.directoryID,
                                sDirInfo.userID )
                            != IDE_SUCCESS, ERR_ACCESS_DENIED );
        }
    
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
        
        // iduFileStream::openfile호출
        IDE_TEST( iduFileStream::openFile( &sFileType->fp,
                                           sFilePath,
                                           sFileType->mode )
                  != IDE_SUCCESS );
        
        IDE_TEST( qcuSessionObj::addOpenedFile(
            (qcSessionObjInfo*)(sSession->mQPSpecific.mSessionObj),
                                      sFileType->fp )
                  != IDE_SUCCESS );
    }
        
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_PATH );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_PATH));
    }
    IDE_EXCEPTION( ERR_INVALID_MODE );
    {
        IDE_SET(ideSetErrorCode(qpERR_ABORT_QSF_INVALID_FILEOPEN_MODE));
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

 

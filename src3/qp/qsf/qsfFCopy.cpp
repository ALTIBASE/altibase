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
 * $Id: qsfFCopy.cpp 84991 2019-03-11 09:21:00Z andrew.shin $
 *
 * Description :
 *     PROJ-1371 PSM File Handling
 *     FILE을 copy하는 함수
 *
 * Syntax :
 *     FILE_COPY( location VARCHAR, filename VARCHAR,
 *                destdir VARCHAR, destfilename VARCHAR,
 *                startline INTEGER, endline INTEGER );
 *     RETURN BOOLEAN
 *
 **********************************************************************/

#include <qsf.h>
#include <qcmDirectory.h>
#include <qdpPrivilege.h>
#include <iduFileStream.h>
#include <qcg.h>
#include <qdpRole.h>

#define QSF_MAX_COPY_LINE_LEN   (32768)
#define QSF_ENDLINE_UNLIMITED   (0)

extern mtdModule mtdVarchar;
extern mtdModule mtdBoolean;
extern mtdModule mtdInteger;

static mtcName qsfFunctionName[1] = {
    { NULL, 9, (void*)"FILE_COPY" }
};

static IDE_RC qsfEstimate( mtcNode*     aNode,
                           mtcTemplate* aTemplate,
                           mtcStack*    aStack,
                           SInt         aRemain,
                           mtcCallBack* aCallBack );

mtfModule qsfFCopyModule = {
    1|MTC_NODE_OPERATOR_MISC|MTC_NODE_VARIABLE_TRUE,
    ~0,
    1.0,                    // default selectivity (비교 연산자 아님)
    qsfFunctionName,
    NULL,
    mtf::initializeDefault,
    mtf::finalizeDefault,
    qsfEstimate
};

IDE_RC qsfCalculate_FCopy( mtcNode*     aNode,
                             mtcStack*    aStack,
                             SInt         aRemain,
                             void*        aInfo,
                             mtcTemplate* aTemplate );

static const mtcExecute qsfExecute = {
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    mtf::calculateNA,
    qsfCalculate_FCopy,
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

    const mtdModule* sModules[6] =
    {
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdVarchar,
        &mtdInteger,
        &mtdInteger
    };

    const mtdModule* sModule = &mtdBoolean;

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_QUANTIFIER_MASK ) ==
                    MTC_NODE_QUANTIFIER_TRUE,
                    ERR_NOT_AGGREGATION );

    IDE_TEST_RAISE( ( aNode->lflag & MTC_NODE_ARGUMENT_COUNT_MASK ) != 6,
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

IDE_RC qsfCalculate_FCopy( mtcNode*     aNode,
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
 *     1. argument가 하나라도 null이면 에러(endline은 null인 경우 무한대로 간주)
 *     2. system path를 meta로부터 가져오고, 권한 검사 수행
 *     2.1 source는 read, destination은 write권한 검사
 *     3. path, filename, directory indicator를 조합하여 path생성
 *     4. startline~endline까지 file copy
 *
 ***********************************************************************/
    
#define IDE_FN "IDE_RC qsfCalculate_FCopy"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qcStatement    * sStatement;
    mtdCharType    * sSrcPathValue;
    mtdCharType    * sSrcFilenameValue;
    mtdCharType    * sDestPathValue;
    mtdCharType    * sDestFilenameValue;
    mtdIntegerType   sStartLine;
    mtdIntegerType   sEndLine;
    mtdBooleanType * sReturnValue;
    SChar          * sSrcFilePath;
    SChar          * sDestFilePath;
    SChar          * sLineBuff;
    qcmDirectoryInfo sSrcDirInfo;
    qcmDirectoryInfo sDestDirInfo;
    idBool           sExist;
    SInt             sCurrentLine;
    FILE*            sSrcFp;
    FILE*            sDestFp;

    IDE_RC           sRC;
    UInt             sErrorCode;
        
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
        sReturnValue = (mtdBooleanType*)aStack[0].value;
        *sReturnValue = MTD_BOOLEAN_TRUE;
        
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
        
        // check grant
        IDE_TEST_RAISE( qdpRole::checkDMLReadDirectoryPriv(
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

        // src filename, path
        sSrcFilenameValue = (mtdCharType*)aStack[2].value;

        IDE_TEST( qsf::makeFilePath( sStatement->qmxMem,
                                     &sSrcFilePath,
                                     sSrcDirInfo.directoryPath,
                                     sSrcFilenameValue )
                  != IDE_SUCCESS );
        
        // dest filename, path        
        sDestFilenameValue = (mtdCharType*)aStack[4].value;

        IDE_TEST( qsf::makeFilePath( sStatement->qmxMem,
                                     &sDestFilePath,
                                     sDestDirInfo.directoryPath,
                                     sDestFilenameValue )
                  != IDE_SUCCESS );
        
        sStartLine = *(mtdIntegerType*)aStack[5].value;
        if( sStartLine < 1 )
        {
            IDE_RAISE(ERR_ARGUMENT_NOT_APPLICABLE);
        }
        else
        {
            // Nothing to do.
        }
        
        if( aStack[6].column->module->isNull( aStack[6].column,
                                              aStack[6].value ) == ID_TRUE )
        {
            sEndLine = QSF_ENDLINE_UNLIMITED;
        }
        else
        {    
            sEndLine = *(mtdIntegerType*)aStack[6].value;
            
            IDE_TEST_RAISE( sStartLine > sEndLine,
                            ERR_ARGUMENT_NOT_APPLICABLE );
        }

        IDE_TEST_RAISE( idlOS::strMatch( sSrcFilePath,
                                         idlOS::strlen( sSrcFilePath ),
                                         sDestFilePath ,
                                         idlOS::strlen( sDestFilePath ) ) == 0,
                        ERR_ARGUMENT_NOT_APPLICABLE );

        IDU_FIT_POINT( "qsfFCopy::qsfCalculate_FCopy::alloc::LineBuff" );       
        IDE_TEST( sStatement->qmxMem->alloc(
                              QSF_MAX_COPY_LINE_LEN,
                              (void**)&sLineBuff )
                  != IDE_SUCCESS );
        
        IDE_TEST( iduFileStream::openFile( &sSrcFp,
                                          sSrcFilePath,
                                          (SChar*)"r" )
                 != IDE_SUCCESS );
        sStage = 10;

        IDE_TEST( iduFileStream::openFile( &sDestFp,
                                          sDestFilePath,
                                          (SChar*)"w" )
                 != IDE_SUCCESS );
        sStage = 11;

        sCurrentLine = 1;
        
        while( 1 )
        {
            // getline
            sRC = iduFileStream::getString( sLineBuff,
                                            QSF_MAX_COPY_LINE_LEN,
                                            sSrcFp );
            if( sRC != IDE_SUCCESS )
            {
                sErrorCode = ideGetErrorCode();

                if( sErrorCode == idERR_ABORT_IDU_FILE_NO_DATA_FOUND )
                {
                    IDE_CLEAR();
                    break;
                }
                else
                {
                    IDE_RAISE(ERR_IDU);
                }
            }
            else
            {
                // Nothing to do
            }
            
            if( sStartLine <= sCurrentLine )
            {
                IDE_TEST( iduFileStream::putString( sLineBuff,
                                                    sDestFp )
                          != IDE_SUCCESS );
            }
            else
            {
                // Nothing to do.
            }
            
            sCurrentLine++;
            if( ( sEndLine != QSF_ENDLINE_UNLIMITED ) &&
                ( sCurrentLine > sEndLine ) )
            {
                break;
            }
            else
            {
                // Nothing to do.
            }
        }

        sStage = 10;
        IDE_TEST( iduFileStream::closeFile( sDestFp )
                  != IDE_SUCCESS );

        sStage = 0;
        IDE_TEST( iduFileStream::closeFile( sSrcFp )
                  != IDE_SUCCESS );
    }
        
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
    IDE_EXCEPTION( ERR_IDU );

    IDE_EXCEPTION_END;

    switch(sStage)
    {
        case 11:
            iduFileStream::closeFile(sDestFp );
        case 10:
            iduFileStream::closeFile(sSrcFp );
            break;
            
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
 

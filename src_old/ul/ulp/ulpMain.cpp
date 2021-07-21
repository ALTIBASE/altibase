/**
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/***********************************************************************
 * APRE C/C++ precompiler
 *
 *    APRE 의 구조는 크게 파서, 코드생성기, 그리고 APRE library로 구성된다.
 *
 *    1. 파서
 *          - ulpPreprocl/y    ; Macro 파싱을 담당함.
 *          - ulpPreprocIfl/y  ; Macro 조건절의 파싱을 담당함.
 *          - ulpCompl/y       ; 내장 SQL구문과 C언어 파싱을 담당함.
 *    1.1. 파싱 처리 과정
 *          - ulpPreprocl/y 에서 Macro 파싱을 하면서 #if와 같은 조건절을 만나면 ulpPreprocIfl/y
 *            파서를 호출하여 결과값을 얻어온다.
 *          - ulpPreprocl/y 에서 Macro 파싱을 마치면 macro처리를 완료한 .pp 파일(임시파일)이 생성된다.
 *          - 그런뒤 ulpCompl/y 파서는 .pp를 input으로 내장 SQL구문과 C언어 파싱을 한다.
 *            (ulpCompl/y 파서 에서도 부분적으로 macro 파싱을 한다.)
 *          - ulpCompl/y 파싱이 완료되면 .c/.cpp 파일이 생성된며 .pp 파일은 제거된다.
 *
 *    2. 코드생성기
 *          - ulpGenCode.cpp   ; 파서에서 파싱중 코드생성모듈의 함수들을 호출하여
 *                               코드를 변환하여 결과파일에 쓴다.
 *
 *    3. APRE library
 *          - ./lib 에 있는 모든 코드들을 포함하며, apre에서 전처리를 마친 변환된코드에서
 *            사용되는 interface들을 포함한다. 내부적으로 ODBC cli함수를 호출하여 SQL내장구문들의
 *            기능을 실행한다.
 *
 ***********************************************************************/

#include <ulpMain.h>

/* extern for parser functions */
extern int doPPparse  ( SChar *aFilename );   // parser for preprocessor
extern int doCOMPparse( SChar *aFilename );   // parser for precompiler


int main( SInt argc, SChar **argv )
{
    SInt sI;
    iduMemory       sUlpMem;
    iduMemoryStatus sUlpMempos;

    gUlpProgOption.ulpInit();

    /*********************************
     * 1. Command-line option 처리
     *      : utpProgOption 객체에서 처리.
     *********************************/

    /* 주어진 option들을 처리한다. */
    if ( gUlpProgOption.ulpParsingProgOption( argc, argv ) != IDE_SUCCESS )
    {
        idlOS::exit( 1 );
    }

    /* Copywrite message를 보여준다. */
    if( gUlpProgOption.mOptSilent == ID_FALSE )
    {
        gUlpProgOption.ulpPrintCopyright();
    }

    /* initialize iduMemory */
    sUlpMem.init(IDU_MEM_OTHER);
    gUlpMem = &sUlpMem;

    for( sI = 0 ; sI < gUlpProgOption.mInFileCnt ; sI++ )
    {

        gUlpCodeGen.ulpInit();
        gUlpMacroT.ulpInit();
        gUlpScopeT.ulpInit();
        gUlpStructT.ulpInit();

        gUlpProgOption.ulpAddPreDefinedMacro();

        /* 현재 처리할 내장SQL구문 프로그램 file 설정 & 중간file 설정 & 결과file 설정. */
        gUlpProgOption.ulpSetInOutFiles( gUlpProgOption.mInFileList[ sI ] );

        /*********************************
        * 2. Do Preprocessing
        *********************************/

        /* preprocessing output 중간 file을 연다. */
        gUlpCodeGen.ulpGenOpenFile( gUlpProgOption.mTmpFile );
        gUlpErrDelFile = ERR_DEL_TMP_FILE;

        IDE_ASSERT(gUlpMem->getStatus( &sUlpMempos ) == IDE_SUCCESS);

        /* Preprocessing을 수행한다.*/
        doPPparse( gUlpProgOption.mInFile );

        IDE_ASSERT(gUlpMem->setStatus( &sUlpMempos ) == IDE_SUCCESS);

        /* ulpCodeGen 객체를 초기화 시켜준다. */
        gUlpCodeGen.ulpGenClearAll();

        if ( gUlpProgOption.mDebugMacro == ID_TRUE )
        {
            // macro table을 보여준다.
            gUlpMacroT.ulpMPrint();
        }

        /*********************************
        * 4. Do Precompiling
        *********************************/

        /* BUG-28061 : preprocessing을마치면 marco table을 초기화하고, *
         *             ulpComp 에서 재구축한다.                       */
        // macro table 초기화.
        gUlpMacroT.ulpFinalize();
        gUlpProgOption.ulpAddPreDefinedMacro();

        /* precompiling output file을 연다. */
        gUlpCodeGen.ulpGenOpenFile( gUlpProgOption.mOutFile );
        gUlpErrDelFile = ERR_DEL_ALL_FILE;

        IDE_ASSERT(gUlpMem->getStatus( &sUlpMempos ) == IDE_SUCCESS);

        /* precompiling을 수행한다. */
        doCOMPparse( gUlpProgOption.mTmpFile );

        IDE_ASSERT(gUlpMem->setStatus( &sUlpMempos ) == IDE_SUCCESS);

        /* ulpCodeGen 객체를 초기화 시켜준다. */
        gUlpCodeGen.ulpGenClearAll();

        if ( gUlpProgOption.mDebugPP != ID_TRUE )
        {
            /* 중간 file을 제거한다. */
            IDE_TEST_RAISE ( idlOS::remove( gUlpProgOption.mTmpFile ) != 0,
                             ERR_FILE_TMP_REMOVE );
        }

        /* BUG-33025 Predefined types should be able to be set by user in APRE */
        gUlpCodeGen.ulpGenRemovePredefine(gUlpProgOption.mOutFile);
        gUlpCodeGen.ulpGenCloseFile();

        if ( gUlpProgOption.mDebugSymbol == ID_TRUE )
        {
            // struct table 과 symbol table 을 보여준다.
            gUlpStructT.ulpPrintStructT();
            gUlpScopeT.ulpPrintAllSymT();
        }

        gUlpCodeGen.ulpFinalize();
        gUlpMacroT.ulpFinalize();
        gUlpScopeT.ulpFinalize();
        gUlpStructT.ulpFinalize();

        gUlpErrDelFile = ERR_DEL_FILE_NONE;
    }

    gUlpProgOption.ulpFreeNCharVarNameList();

    /* iduMemory free */
    gUlpMem->destroy();

    // bug-27661 : sun 장비에서 apre main함수 exit 할때 SEGV
    idlOS::exit(0);

    IDE_EXCEPTION ( ERR_FILE_TMP_REMOVE );
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_DELETE_ERROR,
                         gUlpProgOption.mTmpFile, errno );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    // bug-27661 : sun 장비에서 apre main함수 exit 할때 SEGV
    idlOS::exit(-1);
}

void ulpFinalizeError()
{
    SChar *sFileName;

    if ( gUlpProgOption.mDebugPP != ID_TRUE )
    {
        switch ( gUlpErrDelFile )
        {
            case ERR_DEL_FILE_NONE:
                // do nothing 
                break;
            case ERR_DEL_TMP_FILE :
                sFileName = gUlpProgOption.mTmpFile;
                IDE_TEST_RAISE ( idlOS::remove( sFileName ) != 0,
                                 ERR_FILE_TMP_REMOVE );
                break;
            case ERR_DEL_ALL_FILE :
                sFileName = gUlpProgOption.mTmpFile;
                IDE_TEST_RAISE ( idlOS::remove( sFileName ) != 0,
                                 ERR_FILE_TMP_REMOVE );
                sFileName = gUlpProgOption.mOutFile;
                IDE_TEST_RAISE ( idlOS::remove( sFileName ) != 0,
                                 ERR_FILE_TMP_REMOVE );
                break;
            default:
                IDE_ASSERT(0);
                break;
        }
    }

    idlOS::exit(1);

    IDE_EXCEPTION ( ERR_FILE_TMP_REMOVE );
    {
        ulpErrorMgr mErrorMgr;
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_DELETE_ERROR,
                         sFileName, errno );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
    }
    IDE_EXCEPTION_END;

    idlOS::exit(1);
}

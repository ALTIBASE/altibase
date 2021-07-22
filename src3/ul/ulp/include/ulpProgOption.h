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

#ifndef _UTP_PROGOPTION_H_
#define _UTP_PROGOPTION_H_ 1

#include <idl.h>
#include <ide.h>
#include <iduVersion.h>
#include <iduList.h>
#include <ulpErrorMgr.h>
#include <ulpMacro.h>


typedef struct ulpHeaderInfo
{
    SChar mFileName[MAX_INCLUDE_PATH_LEN + MAX_FILE_NAME_LEN];
    ulpHEADERSTATE  mState;
    idBool          mIsCInclude;
} ulpHeaderInfo;

/**********************************************************
 * DESCRIPTION :
 *
 * Precompiler 의 command-line option 정보를 갖고 있는 class
 * & include header file에 대한 정보도 관리함.
 **********************************************************/
class ulpProgOption
{
    private:
        /* file extension string 정보 */
        SChar mFileExtName[MAX_FILE_EXT_LEN];
        /* Error 처리를 위한 자료구조 */
        ulpErrorMgr mErrorMgr;

    public:
        /*** Member functions ***/
        ulpProgOption();

        void ulpInit();

        /* command-line option들에 대한 파싱 */
        IDE_RC ulpParsingProgOption( SInt aArgc, SChar **aArgv );
        /* Preprocessing 후의 임시 file에 대한 이름을 설정함 */
        void ulpSetTmpFile();
        /* 마지막 생성 file에 대한 이름을 설정함 */
        void ulpSetOutFile();
        /* input/tmp/output file에 대한 이름을 설정함 */
        void ulpSetInOutFiles( SChar *aInFile );

        /* mIncludePathList의 path에 해당 header 파일이 존재하는지 확인함.*/
        IDE_RC ulpLookupHeader( SChar *aFileName, idBool aIsCInc );

        IDE_RC ulpPushIncList( SChar *aFileName, idBool aIsCInc );

        void   ulpPopIncList( void );

        SChar *ulpGetIncList( void );

        idBool ulpIsHeaderCInclude( void );

        IDE_RC ulpPrintCopyright();

        void ulpPrintVersion();

        void ulpPrintKeyword();

        void ulpPrintHelpMsg();
        /* mSysIncludePathList에 standard header file path를 설정함. */
        void ulpSetSysIncludePath();

        /* NCHAR 관련 함수 */
        void ulpFreeNCharVarNameList();

        void ulpAddPreDefinedMacro();

        /* BUG-28026 : add keywords */
        /* Keywords sorting function */
        void ulpQuickSort4Keyword( SChar **aKeywords, SInt aLeft, SInt aRight );

        /*** Member variables ***/

        /* 입력받은 내장SQL구문 파일의 리스트 */
        SChar mInFileList[MAX_INPUT_FILE_NUM][MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
        /* 현재처리 중인 내장SQL구문 프로그램 파일 */
        SChar mInFile[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
        /* 중간 저장 파일이름 */
        SChar mTmpFile[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
        /* 최종 생성 파일이름 */
        SChar mOutFile[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
        SChar mOutPath[MAX_FILE_PATH_LEN];
        /* name list of predefined names */
        SChar mDefineList[MAX_DEFINE_NUM][MAX_DEFINE_NAME_LEN];
        /* current working path */
        SChar mCurrentPath[MAX_FILE_PATH_LEN];
        /* start working path */
        SChar mStartPath[MAX_FILE_PATH_LEN];

        /** Variables for Header file **/
        /* include option path 리스트 */
        SChar mIncludePathList[MAX_HEADER_FILE_NUM][MAX_INCLUDE_PATH_LEN];
        /* preprocessor에서 처리 되는 include file 리스트 */
        ulpHeaderInfo mIncludeFileList[MAX_HEADER_FILE_NUM];
        /* include option path의 개수 */
        SInt  mIncludePathCnt;
        /* 처리중인 include file index */
        SInt  mIncludeFileIndex;

        /* the include path list for standard header files */
        SChar mSysIncludePathList[MAX_HEADER_FILE_NUM][MAX_INCLUDE_PATH_LEN];
        /* include path의 개수 for standard header files */
        SInt  mSysIncludePathCnt;
        /******************************/

        SChar mSpillValue[MAX_FILE_NAME_LEN];
        /* 입력받은 내장SQL구문 프로그램 파일의 개수 */
        SInt  mInFileCnt;
        /* 명시된 define 개수 */
        SInt  mDefineCnt;

        /* -mt option for multi-threaded app. */
        idBool mOptMt;
        /* -o option for speciping output file path*/
        idBool mOptOutPath;
        /* -t */
        idBool mOptFileExt;
        /* -include */
        idBool mOptInclude;
        /* -align */
        idBool mOptAlign;
        /* -n */
        idBool mOptNotNullPad;
        /* -spill */
        idBool mOptSpill;
        /* -atc */
        idBool mOptAtc;
        /* -silent */
        idBool mOptSilent;
        /* -unsafe_null */
        idBool mOptUnsafeNull;
        /* -parse */
        idBool mOptParse;
        ulpPARSEOPT mOptParseInfo;
        /* -define */
        idBool mOptDefine;
        /* -debug macro */
        idBool mDebugMacro;
        /* -debug symbol */
        idBool mDebugSymbol;
        /* -pp */
        idBool mDebugPP;

        /* Version 정보 */
        const SChar *mVersion;
        /* 파일 확장자 정보 */
        const SChar *mExtEmSQLFile;

        /* NCHAR */
        idBool  mNcharVar;
        iduList mNcharVarNameList;
        idBool  mNCharUTF16;

        /* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */
        idBool  mOptLineMacro;
};

#endif

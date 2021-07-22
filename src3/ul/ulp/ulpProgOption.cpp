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

#include <idp.h>
#include <ulpProgOption.h>
#include <ulpMacroTable.h>
#include <ulpKeywords.h>

/* extern for -define option*/
extern ulpMacroTable  gUlpMacroT;

/* BUG-33025 Predefined types should be able to be set by user in APRE
 * to decide whether PARSE_FULL or NOT
 */
extern ulpProgOption gUlpProgOption;

/* Constructor */
ulpProgOption::ulpProgOption()
{
}

void ulpProgOption::ulpInit()
{
/* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
    mVersion      = VERSION_STR;
    mExtEmSQLFile = "sc";

    mInFileCnt = 0;
    mIncludePathCnt = 0;
    mIncludeFileIndex = -1;
    mSysIncludePathCnt = 0;
    mDefineCnt = 0;
    mOptMt      = ID_FALSE;
    mOptOutPath = ID_FALSE;
    mOptFileExt = ID_FALSE;
    mOptInclude = ID_FALSE;
    mOptAlign   = ID_FALSE;
    mOptNotNullPad = ID_FALSE;
    mOptSpill   = ID_FALSE;
    mOptAtc     = ID_FALSE;
    mOptSilent  = ID_FALSE;
    mOptUnsafeNull = ID_FALSE;
    mOptParse   = ID_FALSE;
    mOptParseInfo = PARSE_PARTIAL;
    mOptDefine  = ID_FALSE;
    mDebugMacro = ID_FALSE;
    mDebugSymbol= ID_FALSE;
    mDebugPP    = ID_FALSE;
    idlOS::strcpy(mFileExtName, "c");
    idlOS::getcwd(mStartPath, MAX_FILE_PATH_LEN);
    idlOS::strcpy(mCurrentPath, mStartPath);
    ulpSetSysIncludePath();

    /* NCHAR */
    mNcharVar   = ID_FALSE;
    mNCharUTF16 = ID_FALSE;
    IDU_LIST_INIT( &mNcharVarNameList );

    /* BUG-42357 The -lines option is added to apre. (INC-31008) */
    mOptLineMacro = ID_FALSE;
}

/* standard header file path 를 저장하는 함수이나,     *
 * macro parser에서 error발생 여지가 있어 주석으로 막음. */
void ulpProgOption::ulpSetSysIncludePath()
{
/*    idlOS::snprintf(mSysIncludePathList[0],
                    MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                    "/usr/local/include");
    idlOS::snprintf(mSysIncludePathList[1],
                    MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                    "/usr/include");

    mSysIncludePathCnt += 2;*/
}

/* command-line option들에 대한 파싱 함수 */
IDE_RC ulpProgOption::ulpParsingProgOption( SInt aArgc, SChar **aArgv )
{
    SChar* sStrPos;
    SChar  sTmpPathBuf[MAX_INCLUDE_PATH_LEN];
    UInt   sLen;
    SInt   sI;
    SInt   sOutPathLen;

    SChar* sToken          = NULL;
    SChar* sSavePos        = NULL;
    SChar* sName           = NULL;
    SInt   sNameLen        = 0;
    iduListNode* sListNode = NULL;

    IDE_TEST_RAISE( aArgc < 2, EXC_PRINT_HELP );


    for( sI = 1 ; sI < aArgc ; sI++ )
    {
        if (idlOS::strcmp(aArgv[sI], "-h") == 0 ||
            idlOS::strcmp(aArgv[sI], "-H") == 0)
        {
            IDE_RAISE( EXC_PRINT_HELP );
        }
        else if (idlOS::strcmp(aArgv[sI], "-v") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-V") == 0)
        {
            ulpPrintVersion();
            idlOS::exit( 1 );
        }
        else if (idlOS::strcmp(aArgv[sI], "-t") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-T") == 0)
        {
            if (sI+1 < aArgc)
            {
                if (sI+2 == aArgc)
                {
                    IDE_RAISE( EXC_PRINT_HELP );
                }
                else
                {
                    if (idlOS::strcmp(aArgv[sI+1], "-t") == 0 ||
                        idlOS::strcmp(aArgv[sI+1], "-T") == 0)
                    {
                        IDE_RAISE( ERR_DUPLICATE_OPTION );
                    }
                    else if (mOptFileExt == ID_TRUE)
                    {
                        IDE_RAISE( ERR_DUPLICATE_OPTION );
                    }
                    else if (idlOS::strcasecmp(aArgv[sI+1], "c") == 0)
                    {
                        mOptFileExt = ID_TRUE;
                        idlOS::strcpy(mFileExtName, "c");
                        sI++;
                    }
                    else if (idlOS::strcasecmp(aArgv[sI+1], "cpp") == 0)
                    {
                        mOptFileExt = ID_TRUE;
                        idlOS::strcpy(mFileExtName, "cpp");
                        sI++;
                    }
                    else
                    {
                        IDE_RAISE( EXC_PRINT_HELP );
                    }
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        else if (idlOS::strcmp(aArgv[sI], "-include") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-INCLUDE") == 0)
        {
            if (sI+1 < aArgc)
            {
                if (sI+2 == aArgc)
                {
                    IDE_RAISE( EXC_PRINT_HELP );
                }
                else
                {
                    sLen = idlOS::strlen( aArgv[sI+1] );
                    /* include 문자열이 MAX_INCLUDE_PATH_LEN 보다 크면 ERROR */
                    IDE_TEST_RAISE( sLen >= MAX_INCLUDE_PATH_LEN , ERR_STRLEN_OVERFLOW );
                    idlOS::strncpy( sTmpPathBuf, aArgv[sI+1], MAX_INCLUDE_PATH_LEN );

                    sStrPos = idlOS::strchr(sTmpPathBuf, ',');
                    for( ; sStrPos != NULL ; ++mIncludePathCnt )
                    {
                        *sStrPos = '\0';
                        idlOS::strncpy(mIncludePathList[mIncludePathCnt],
                                       sTmpPathBuf, MAX_INCLUDE_PATH_LEN);
                        idlOS::strncpy(sTmpPathBuf, sStrPos+1, MAX_INCLUDE_PATH_LEN);
                        sStrPos = idlOS::strchr(sTmpPathBuf, ',');
                    }
                    idlOS::strncpy(mIncludePathList[mIncludePathCnt++],
                                   sTmpPathBuf, MAX_INCLUDE_PATH_LEN);
                    sI++;
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        /* BUG-27099 : code review 지적사항들 */
        else if ( (aArgv[sI][0] == '-') && (aArgv[sI][1] == 'I') )
        {
            if (sI + 1 < aArgc)
            {
                sLen = idlOS::strlen( aArgv[sI] );
                /* include 문자열이 MAX_INCLUDE_PATH_LEN 보다 크면 ERROR */
                IDE_TEST_RAISE( sLen - 2 >= MAX_INCLUDE_PATH_LEN , ERR_STRLEN_OVERFLOW );

                if( sLen > 2 )
                {
                    idlOS::strncpy( mIncludePathList[mIncludePathCnt++],
                                    aArgv[sI]+2, MAX_INCLUDE_PATH_LEN );
                }
                else
                {
                    /* do nothing or print error? */
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        else if (idlOS::strcmp(aArgv[sI], "-o") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-O") == 0)
        {
            if (sI+1 < aArgc)
            {
                if (sI+2 == aArgc)
                {
                    IDE_RAISE( EXC_PRINT_HELP );
                }
                else
                {
                    if (idlOS::strcmp(aArgv[sI+1], "-o") == 0 ||
                        idlOS::strcmp(aArgv[sI+1], "-O") == 0 ||
                        mOptOutPath == ID_TRUE )
                    {
                        IDE_RAISE( ERR_DUPLICATE_OPTION );
                    }
                    else
                    {
                        idlOS::strncpy( mOutPath, aArgv[sI+1], MAX_FILE_PATH_LEN );
                        sOutPathLen = idlOS::strlen( mOutPath );
                        if ( mOutPath[sOutPathLen-1] == IDL_FILE_SEPARATOR )
                        {
                            mOutPath[sOutPathLen-1] = '\0';
                        }
                        mOptOutPath = ID_TRUE;
                        sI++;
                    }
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        else if (idlOS::strcmp(aArgv[sI], "-mt") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-MT") == 0)
        {
            if (mOptMt == ID_TRUE)
            {
                IDE_RAISE( ERR_DUPLICATE_OPTION );
            }
            else
            {
                mOptMt = ID_TRUE;
            }
        }
#ifdef _AIX
        else if (idlOS::strcmp(aArgv[sI], "-align") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-ALIGN") == 0)
        {
            if (mOptAlign == ID_TRUE)
            {
                IDE_RAISE( ERR_DUPLICATE_OPTION );
            }
            else
            {
                mOptAlign = ID_TRUE;
            }
        }
#endif
        else if (idlOS::strcmp(aArgv[sI], "-n") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-N") == 0)
        {
            if (mOptNotNullPad == ID_TRUE)
            {
                IDE_RAISE( ERR_DUPLICATE_OPTION );
            }
            else
            {
                mOptNotNullPad = ID_TRUE;
            }
        }
        else if (idlOS::strcmp(aArgv[sI] , "-spill") == 0 ||
                 idlOS::strcmp(aArgv[sI] , "-SPILL") == 0)
        {
            if (mOptSpill == ID_TRUE)
            {
                IDE_RAISE( ERR_DUPLICATE_OPTION );
            }
            else
            {
                idlOS::strncpy(mSpillValue , aArgv[sI+1], MAX_FILE_NAME_LEN);
                mOptSpill = ID_TRUE;
                sI++;
            }
        }
        else if (idlOS::strcmp(aArgv[sI], "-atc") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-ATC") == 0)
        {
            if (mOptAtc == ID_TRUE)
            {
                IDE_RAISE( ERR_DUPLICATE_OPTION );
            }
            else
            {
                mOptAtc = ID_TRUE;
            }
        }
        else if(idlOS::strcmp(aArgv[sI], "-silent") == 0 )
        {
            mOptSilent = ID_TRUE;
        }
        else if (idlOS::strcmp(aArgv[sI], "-unsafe_null") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-UNSAFE_NULL") == 0)
        {
            mOptUnsafeNull = ID_TRUE;
        }
        else if (idlOS::strcmp(aArgv[sI], "-parse") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-PARSE") == 0)
        {
            if (sI+1 < aArgc)
            {
                if (sI+2 == aArgc)
                {
                    IDE_RAISE( EXC_PRINT_HELP );
                }
                else
                {
                    if (idlOS::strcmp(aArgv[sI+1], "-parse") == 0 ||
                        idlOS::strcmp(aArgv[sI+1], "-PARSE") == 0)
                    {
                        IDE_RAISE( ERR_DUPLICATE_OPTION );
                    }
                    else if ( mOptParse == ID_TRUE )
                    {
                        IDE_RAISE( ERR_DUPLICATE_OPTION );
                    }
                    else if (idlOS::strcasecmp(aArgv[sI+1], "none") == 0)
                    {
                        mOptParse = ID_TRUE;
                        mOptParseInfo = PARSE_NONE;
                        sI++;
                    }
                    else if (idlOS::strcasecmp(aArgv[sI+1], "partial") == 0)
                    {
                        mOptParse = ID_TRUE;
                        mOptParseInfo = PARSE_PARTIAL;
                        sI++;
                    }
                    else if (idlOS::strcasecmp(aArgv[sI+1], "full") == 0)
                    {
                        mOptParse = ID_TRUE;
                        mOptParseInfo = PARSE_FULL;
                        sI++;
                    }
                    else
                    {
                        IDE_RAISE( EXC_PRINT_HELP );
                    }
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        /* BUG-27099 : code review 지적사항들 */
        else if ( (aArgv[sI][0] == '-') && (aArgv[sI][1] == 'D') )
        {
            if (sI + 1 < aArgc)
            {
                sLen = idlOS::strlen( aArgv[sI] );
                /* include 문자열이 MAX_INCLUDE_PATH_LEN 보다 크면 ERROR */
                IDE_TEST_RAISE( sLen - 2 >= MAX_DEFINE_NAME_LEN , ERR_STRLEN_OVERFLOW );

                if( sLen > 2 )
                {
                    idlOS::strncpy( mDefineList[mDefineCnt++],
                                    aArgv[sI]+2,
                                    MAX_DEFINE_NAME_LEN);
                }
                else
                {
                    /* do nothing or print error? */
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        else if (idlOS::strcmp(aArgv[sI], "-keyword") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-KEYWORD") == 0)
        {
            ulpPrintKeyword();
            idlOS::exit( 1 );
        }
        else if (idlOS::strcmp(aArgv[sI], "-pp") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-PP") == 0)
        {
            mDebugPP = ID_TRUE;
        }
        else if (idlOS::strcmp(aArgv[sI], "-debug") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-DEBUG") == 0)
        {
            if (sI+1 < aArgc)
            {
                if (sI+2 == aArgc)
                {
                    IDE_RAISE( EXC_PRINT_HELP );
                }
                else
                {
                    if( idlOS::strcasecmp(aArgv[sI+1],"macro") == 0 )
                    {
                        mDebugMacro = ID_TRUE;
                        sI++;
                        if( idlOS::strcasecmp(aArgv[sI+1],"symbol") == 0 )
                        {
                            mDebugSymbol = ID_TRUE;
                            sI++;
                        }
                    }
                    else if ( idlOS::strcasecmp(aArgv[sI+1],"symbol") == 0 )
                    {
                        mDebugSymbol = ID_TRUE;
                        sI++;
                        if( idlOS::strcasecmp(aArgv[sI+1],"macro") == 0 )
                        {
                            mDebugMacro = ID_TRUE;
                            sI++;
                        }
                    }
                    else
                    {
                        IDE_RAISE( EXC_PRINT_HELP );
                    }
                }
            }
            else
            {
                IDE_RAISE( EXC_PRINT_HELP );
            }
        }
        // nchar용 host 변수명들을 option으로 줄수 있다
        // ex) sesc -nchar_var name1,name2,name3
        // 단 변수이름들 사이에 공백은 허용하지 않는다
        else if (idlOS::strcmp(aArgv[sI], "-nchar_var") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-NCHAR_VAR") == 0)
        {

            mNcharVar = ID_TRUE;
            sI++;
            sToken = idlOS::strtok_r(aArgv[sI], ",", &sSavePos);

            while (sToken != NULL)
            {
                // add var_name to list
                sNameLen = idlOS::strlen(sToken) + 1;
                sName = (SChar*)idlOS::malloc(sNameLen);
                IDE_TEST_RAISE(sName == NULL, ERR_MEMORY_ALLOC);
                idlOS::strcpy(sName, sToken);
                sListNode =
                        (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
                IDE_TEST_RAISE(sListNode == NULL, ERR_MEMORY_ALLOC);
                IDU_LIST_INIT_OBJ(sListNode, sName);
                IDU_LIST_ADD_LAST(&mNcharVarNameList, sListNode);

                sToken = idlOS::strtok_r(NULL, ",", &sSavePos);
            }
        }
        // utf8변환은 지원 안함
        // nchar default: nls_use 이고 nchar_utf16 옵션 주면 utf16
        else if (idlOS::strcmp(aArgv[sI], "-nchar_utf16") == 0 ||
                 idlOS::strcmp(aArgv[sI], "-NCHAR_UTF16") == 0)
        {
            mNCharUTF16 = ID_TRUE;
        }
        /* BUG-42357 The -lines option is added to apre. (INC-31008) */
        else if ( (idlOS::strcmp( aArgv[sI], "-lines" ) == 0) ||
                  (idlOS::strcmp( aArgv[sI], "-LINES" ) == 0) )
        {
            mOptLineMacro = ID_TRUE;
        }
        else
        {
            /* 각각 나머지 나열된 내장SQL구문 프로그램 file들을 복사한다. */
            sLen = idlOS::strlen( aArgv[sI] );
            IDE_TEST_RAISE( sLen >= ( MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN ) ,
                            ERR_STRLEN_OVERFLOW );
            idlOS::strncpy( mInFileList[mInFileCnt] , aArgv[sI],
                            ( MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN ));
            mInFileCnt++;

            sStrPos = idlOS::strrchr(aArgv[sI], '.');
            if ( ( sStrPos == NULL ) ||
                 ( sStrPos != NULL && idlOS::strcmp(sStrPos+1, mExtEmSQLFile) != 0 ) )
            {
                IDE_RAISE( ERR_INVALID_INPUT_FILENAME );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( EXC_PRINT_HELP );
    {
        ulpPrintHelpMsg();
    }
    IDE_EXCEPTION( ERR_DUPLICATE_OPTION );
    {
        ulpSetErrorCode(&mErrorMgr,
                         ulpERR_ABORT_COMP_Option_Duplicated_Error,
                         aArgv[sI]);
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr );
    }
    IDE_EXCEPTION( ERR_STRLEN_OVERFLOW );
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_COMP_Option_String_Overflow_Error,
                         aArgv[sI+1]);
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
    }
    IDE_EXCEPTION( ERR_INVALID_INPUT_FILENAME );
    {
        ulpSetErrorCode( &mErrorMgr,
                          ulpERR_ABORT_COMP_Invalid_Input_fileName_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
    }
    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        if (sName != NULL)
        {
            idlOS::free(sName);
        }

        ulpSetErrorCode(&mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error);
        ulpPrintfErrorCode(stderr, &mErrorMgr);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* Preprocessing 후의 임시 file에 대한 이름을 설정함 */
void ulpProgOption::ulpSetTmpFile()
{
    SChar* sStrPos;
    SChar  sTmpStr[ MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN ];

    idlOS::strcpy(sTmpStr, mInFile);
    // BUGBUG: 확장자가 없고, 경로명에 '.'이 포함된 경우 오동작 할 수 있다.
    sStrPos = idlOS::strrchr(sTmpStr, '.');
    // 확장자가 없을 경우 '.' 추가
    if (sStrPos == NULL)
    {
        sStrPos = sTmpStr + idlOS::strlen(sTmpStr);
        *(sStrPos++) = '.';
    }
    *(sStrPos+1) = 'p';
    *(sStrPos+2) = 'p';
    *(sStrPos+3) = '\0';
    sStrPos = idlOS::strrchr(sTmpStr, IDL_FILE_SEPARATOR);
    if (sStrPos != NULL)
    {
        idlOS::strcpy(mTmpFile, sStrPos+1);
    }
    else
    {
        idlOS::strcpy(mTmpFile, sTmpStr);
    }
}


/* 마지막 생성 file에 대한 이름을 설정함 */
void ulpProgOption::ulpSetOutFile()
{
    SChar  *sStrPos;
    SChar  *sSlashPos;
    SChar  sDotRemovedFilePath[ MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN ];

    sStrPos   = NULL;
    sSlashPos = NULL;

    idlOS::strncpy(sDotRemovedFilePath, mInFile,
                   MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN - 1);
    // BUGBUG: 확장자가 없고, 경로명에 '.'이 포함된 경우 오동작 할 수 있다.
    sStrPos = idlOS::strrchr(sDotRemovedFilePath, '.');

    if ( sStrPos != NULL )
    {
        *sStrPos = '\0';
    }

    if(mOptOutPath == ID_TRUE)
    {
        /* BUG-29502 : -o option 사용하면 target .sc path가 잘못설정됨. */
        // source file 의 path를 제외한 file 이름만 뽑아내야한다.
        // ex) apre -o ./out ./src/tmp.sc
        // output file path is "./out/tmp.c"
        sSlashPos = idlOS::strrchr(sDotRemovedFilePath, IDL_FILE_SEPARATOR);

        idlOS::sprintf( mOutFile, "%s%c%s.%s",
                        mOutPath, IDL_FILE_SEPARATOR,
                        ( sSlashPos != NULL )?sSlashPos+1:sDotRemovedFilePath,
                        mFileExtName);
    }
    else
    {
        // ex) apre ./src/tmp.sc
        // output file path is "./src/tmp.c"
        idlOS::sprintf( mOutFile, "%s.%s",
                        sDotRemovedFilePath,
                        mFileExtName );
    }
}


/* input/tmp/output file에 대한 이름을 설정함 */
void ulpProgOption::ulpSetInOutFiles(SChar* aInFile)
{
    idlOS::strncpy( mInFile, aInFile, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );

    ulpSetTmpFile();
    ulpSetOutFile();
}

/*
 *    <Header file 찾는 순서>
 *
 * 1. 현재 directory.
 * 2. INCLUDE option 에서 명시된 directory.
 * 3. directory for standard header files.
 *
 */
IDE_RC ulpProgOption::ulpLookupHeader( SChar *aFileName, idBool aIsCInc )
{
    SInt   sCnt;
    SChar  sTmpFileName[MAX_FILE_NAME_LEN];
    SChar  sTmpBuf[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
    FILE   *sFp;
    SChar  *sPos;

    /* .h를 생략한 경우 .h를 append 해서 파일을 찾는다. */
    sPos = idlOS::strrchr(aFileName, '.');
    if ( sPos == NULL )
    {
        idlOS::snprintf(sTmpFileName,
                        MAX_FILE_NAME_LEN,
                        "%s%s", aFileName, ".h");
    }
    else
    {
        idlOS::snprintf(sTmpFileName, MAX_FILE_NAME_LEN, aFileName);
    }

    /* 1. current path에서 찾음 */
    /* BUG-35273 ALTI-PCM-003 Coding Convention Violation in ulpProgOption.cpp */
    idlOS::snprintf( sTmpBuf,
                     MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                     ".%c%s",
                     IDL_FILE_SEPARATOR,
                     sTmpFileName);

    if( ( sFp = idlOS::fopen(sTmpBuf, "r")) != NULL )
    {
        idlOS::fclose(sFp);
        ulpPushIncList( sTmpBuf, aIsCInc );

        //idlOS::strcpy(g_infile, tmp_buf2);
        return IDE_SUCCESS;
    }

    /* 2. 사용자가 지정한 include path에서 차례대로 찾음 */
    for ( sCnt = 0 ; sCnt < mIncludePathCnt ; sCnt++ )
    {
        idlOS::snprintf( sTmpBuf, MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                         "%s%s%s", mIncludePathList[sCnt],
                         IDL_FILE_SEPARATORS, sTmpFileName );
        if( (sFp = idlOS::fopen(sTmpBuf, "r")) != NULL )
        {
            idlOS::fclose(sFp);
            ulpPushIncList( sTmpBuf, aIsCInc );

            // set g_current_path
            if ( mIncludePathList[sCnt][0] == '/' )
            {
                // 절대경로인 경우
                idlOS::strcpy(mCurrentPath, mIncludePathList[sCnt]);
            }
            else
            {
                // 상대경로인 경우
                /* BUG-35273 ALTI-PCM-003 Coding Convention Violation in ulpProgOption.cpp */
                idlOS::sprintf( mCurrentPath,
                                "%s%c%s",
                                mStartPath,
                                IDL_FILE_SEPARATOR,
                                mIncludePathList[sCnt] );
            }

            return IDE_SUCCESS;
        }
    }

    /* 3. Standard header files path에서 차례대로 찾음 */
    for ( sCnt = 0 ; sCnt < mSysIncludePathCnt ; sCnt++ )
    {
        idlOS::snprintf( sTmpBuf,
                         MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                         "%s%s%s",
                         mSysIncludePathList[sCnt],
                         IDL_FILE_SEPARATORS,
                         sTmpFileName );

        if( (sFp = idlOS::fopen(sTmpBuf, "r")) != NULL )
        {
            idlOS::fclose(sFp);
            ulpPushIncList( sTmpBuf, aIsCInc );

            return IDE_SUCCESS;
        }
    }

    /* BUG-33025 Predefined types should be able to be set by user in APRE */
    /* 4. path for aprePredefinedTypes.h - $ALTIBASE_HOME/include */
    if ( (gUlpProgOption.mOptParseInfo == PARSE_FULL) &&
         (idlOS::strcmp(sTmpFileName, PREDEFINE_HEADER) == 0) )
    {
        idlOS::snprintf( sTmpBuf,
                         MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                         "%s%sinclude%s%s",
                         idlOS::getenv( IDP_HOME_ENV ),
                         IDL_FILE_SEPARATORS,
                         IDL_FILE_SEPARATORS,
                         sTmpFileName );

        if( (sFp = idlOS::fopen(sTmpBuf, "r")) != NULL )
        {
            idlOS::fclose(sFp);
            ulpPushIncList( sTmpBuf, aIsCInc );

            return IDE_SUCCESS;
        }
    }

    IDE_RAISE( ERR_INCLUDE_FILE_OPEN );

    IDE_EXCEPTION( ERR_INCLUDE_FILE_OPEN );
    {
        /*ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_NOT_FOUND,
                         sTmpBuf );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);*/

    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* mIncludeFileList 배열에 새로 처리중인 include file 정보 저장함. */
IDE_RC ulpProgOption::ulpPushIncList( SChar *aFileName, idBool aIsCInc )
{
    IDE_TEST_RAISE ( mIncludeFileIndex >= MAX_HEADER_FILE_NUM-1,
                     ERR_INCLUDE_DEPTH_TOO_LARGE );
    mIncludeFileIndex ++;
    idlOS::snprintf( mIncludeFileList[mIncludeFileIndex].mFileName,
                     MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN,
                     aFileName );

    mIncludeFileList[mIncludeFileIndex].mIsCInclude = aIsCInc;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCLUDE_DEPTH_TOO_LARGE );
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_COMP_Include_Depth_Too_Large_Error,
                         aFileName );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void   ulpProgOption::ulpPopIncList()
{
    if ( mIncludeFileIndex > -1 )
    {
        mIncludeFileIndex --;
    }
}

SChar *ulpProgOption::ulpGetIncList( void )
{
    SChar *sVal;

    if ( mIncludeFileIndex > -1 )
    {
        sVal = mIncludeFileList[mIncludeFileIndex].mFileName;
    }
    else
    {
        sVal = NULL;
    }

    return sVal;
}

idBool ulpProgOption::ulpIsHeaderCInclude( void )
{
    if( ( mIncludeFileIndex >= 0 ) && 
        ( mIncludeFileList[mIncludeFileIndex].mIsCInclude == ID_TRUE ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

IDE_RC ulpProgOption::ulpPrintCopyright()
{
    SChar         sBuf[MAX_BANNER_SIZE];
    SChar         sBannerFile[MAX_BANNER_SIZE];
    SInt          sCount;
    FILE        * sFP;
    SChar       * sAltiHome;
   
    /* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
    const SChar * sBanner = BAN_FILE_NAME;
    
    
    sAltiHome = idlOS::getenv(IDP_HOME_ENV);
    IDE_TEST_RAISE( sAltiHome == NULL, err_altibase_home);

    // make full path banner file name
    idlOS::memset(sBannerFile, 0, ID_SIZEOF(sBannerFile));
    idlOS::snprintf(sBannerFile, ID_SIZEOF(sBannerFile), "%s%c%s%c%s"
            , sAltiHome, IDL_FILE_SEPARATOR, "msg", IDL_FILE_SEPARATOR, sBanner);

    sFP = idlOS::fopen(sBannerFile, "r");
    IDE_TEST_RAISE( sFP == NULL, err_file_open );

    // BUG-32698 Codesonar warnings at UX&UL module on Window 32bit
    sCount = idlOS::fread( (void*) sBuf, 1, MAX_BANNER_SIZE-1, sFP );
    IDE_TEST_RAISE( sCount <= 0, err_file_read );

    sBuf[sCount] = '\0';
    idlOS::printf("%s", sBuf);
    idlOS::fflush(stdout);

    idlOS::fclose(sFP);

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_altibase_home );
    {
        // nothing to do
        // ignore error in printCopyright
    }
    IDE_EXCEPTION( err_file_open );
    {
        // nothing to do
        // ignore error in printCopyright
    }
    IDE_EXCEPTION( err_file_read );
    {
        idlOS::fclose(sFP);
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void ulpProgOption::ulpPrintVersion()
{
    SChar sVer[MAX_VERSION_LEN];

    idlOS::snprintf( sVer, MAX_VERSION_LEN,
                     "%s %s %s %s\n",
                     mVersion,
                     iduVersionString,
                     iduGetSystemInfoString(),
                     iduGetProductionTimeString() );

    idlOS::printf("%s", sVer);
    idlOS::fflush(stdout);
}

/* BUG-28026 : add keywords */
/* Keywords print function */
void ulpProgOption::ulpPrintKeyword()
{
    SChar **sKeywordC;
    SChar **sKeywordEMSQL;
    SInt    sIndexC;
    SInt    sIndexEMSQL;
    SInt    sI;

    sKeywordC     = NULL;
    sKeywordEMSQL = NULL;
    sIndexC     = ID_SIZEOF(gUlpKeywords4C) / MAX_KEYWORD_LEN;
    sIndexEMSQL = ID_SIZEOF(gUlpKeywords4Emsql) / MAX_KEYWORD_LEN;

    IDE_TEST_RAISE( (sKeywordC = (SChar **)idlOS::malloc( sIndexC * ID_SIZEOF(SChar *) ))
                    == NULL, ERR_MEMORY_ALLOC );
    IDE_TEST_RAISE( (sKeywordEMSQL = (SChar **)idlOS::malloc( sIndexEMSQL * ID_SIZEOF(SChar *) ))
                    == NULL, ERR_MEMORY_ALLOC );

    for ( sI=0 ; sI < sIndexC ; sI++)
    {
        sKeywordC[sI] = (SChar *)gUlpKeywords4C[sI];
    }
    for ( sI=0 ; sI < sIndexEMSQL ; sI++)
    {
        sKeywordEMSQL[sI] = (SChar *)gUlpKeywords4Emsql[sI];
    }

    ulpQuickSort4Keyword( sKeywordC, 0, sIndexC - 1 );
    ulpQuickSort4Keyword( sKeywordEMSQL, 0, sIndexEMSQL - 1 );

    idlOS::printf( "\n:: Keywords for C code ::\n " );
    for ( sI=0 ; sI < sIndexC ; sI++)
    {
        idlOS::printf( "%s ", sKeywordC[sI] );
    }

    idlOS::printf( "\n\n:: Keywords for Embedded SQL statement ::\n " );
    for ( sI=0 ; sI < sIndexEMSQL ; sI++)
    {
        idlOS::printf( "%s ", sKeywordEMSQL[sI] );
    }
    idlOS::printf( "\n\n");

    idlOS::fflush(stdout);
    idlOS::free(sKeywordC);
    idlOS::free(sKeywordEMSQL);

    return;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC );
    {
        if ( sKeywordC != NULL )
        {
            idlOS::free(sKeywordC);
        }

        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        IDE_ASSERT(0);
    }
    IDE_EXCEPTION_END;
}

void ulpProgOption::ulpPrintHelpMsg()
{
    const SChar * sHelpMsg =
        "=====================================================================\n"
        "APRE (Altibase Precompiler) C/C++ Precompiler HELP Screen\n"
        "=====================================================================\n"
        "Usage  :  apre [<options>] <filename>\n\n"
        "-h               : Display this help information.\n"
        "-t <c|cpp>       : Specify the file extension for the output file.\n"
        "                   c   - File extension is '.c' (default)\n"
        "                   cpp - File extension is '.cpp'\n"
        "-o <output_path> : Specify the directory path for the output file.\n"
        "                   (default : current directory)\n"
        "-mt              : When precompiling a multithreaded application,\n"
        "                   this option must be specified.\n"
        "-I<include_path> : Specify the directory paths for files included using APRE C/C++.\n"
        "                   (default : current directory)\n"
        "-parse <none|partial|full>\n"
        "                 : Control which non-SQL code is parsed.\n"
        "-D<define_name>  : Use to define a preprocessor symbol.\n"
        "-v               : Output the version of APRE.\n"
        "-n               : Specify when CHAR variables are not null-padded.\n"
        "-unsafe_null     : Specify to suppress errors when NULL values are fetched\n"
        "                   and indicator variables are not used.\n"
        "-align           : Specify when using alignment in AIX.\n"
        "-spill <values>  : Specify the register allocation spill area size.\n"
        "-keyword         : Display all reserved keywords.\n"
        "-debug <macro|symbol>\n"
        "                 : Use for debugging.\n"
        "                   macro   - Display macro table.\n"
        "                   symbol  - Display symbol table.\n"
        "-nchar_var <variable_name_list>\n"
        "                 : Process the specified variables using\n"
        "                   the Altibase national character set.\n"
        "-nchar_utf16     : Set client nchar encoding to UTF-16.\n"
        "-lines           : Add #line directives to the generated code.\n"
        "-silent          : No display Copyright\n\n"
        "======================================================================\n";

    idlOS::printf( sHelpMsg );
    idlOS::fflush(stdout);
}


void ulpProgOption::ulpFreeNCharVarNameList()
{
    iduListNode*    sIterator = NULL;
    iduListNode*    sNextNode = NULL;

    IDU_LIST_ITERATE_SAFE(&mNcharVarNameList, sIterator, sNextNode)
    {
        IDU_LIST_REMOVE( sIterator );
        idlOS::free(sIterator->mObj);
        idlOS::free(sIterator);
    }
    IDU_LIST_INIT(&mNcharVarNameList);
}

void ulpProgOption::ulpAddPreDefinedMacro()
{
    SInt sJ;

    /* some predefined defines */
    // 하위호환성
    gUlpMacroT.ulpMDefine( (SChar*)"SESC_INCLUDE", NULL, ID_FALSE );
    gUlpMacroT.ulpMDefine( (SChar*)"SESC_DECLARE", NULL, ID_FALSE );
    // new
    gUlpMacroT.ulpMDefine( (SChar*)"ALTIBASE_APRE", NULL, ID_FALSE );
    // 오라클의 경우 ORA_PROC을 predefine해서 사용한다.
    //gUlpMacroT.ulpMDefine( (SChar*)"ORA_PROC", NULL, ID_FALSE );

    for( sJ = 0 ; sJ < mDefineCnt ; sJ++ )
    {
        // macro symbol table에 추가함.
        gUlpMacroT.ulpMDefine( mDefineList[sJ], NULL, ID_FALSE );
    }
}


/* BUG-28026 : add keywords */
/* Keywords sorting function */
void ulpProgOption::ulpQuickSort4Keyword( SChar **aKeywords,
                                          SInt aLeft,
                                          SInt aRight )
{
    SInt  sL;
    SInt  sR;
    SChar *sTmpstrptr;
    SChar *sPivot;

    sL  = aLeft;
    sR  = aRight;

    sPivot = aKeywords[(aLeft+aRight)/2];

    do
    {
        while ( idlOS::strcmp( aKeywords[sL] , sPivot ) < 0 )
        {
            sL++;
        }
        while ( idlOS::strcmp( aKeywords[sR] , sPivot ) > 0 )
        {
            sR--;
        }
        if ( sL <= sR )
        {
            sTmpstrptr    = aKeywords[sL];
            aKeywords[sL] = aKeywords[sR];
            aKeywords[sR] = sTmpstrptr;
            sL++;
            sR--;
        }
    } while ( sL <= sR );

    //  recursion
    if ( aLeft < sR )
    {
        ulpQuickSort4Keyword( aKeywords, aLeft, sR );
    }
    if ( sL < aRight )
    {
        ulpQuickSort4Keyword( aKeywords, sL, aRight );
    }
}

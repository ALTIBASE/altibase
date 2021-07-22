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

#ifndef _ULP_GENCODE_H_
#define _ULP_GENCODE_H_ 1

#include <idl.h>
#include <ide.h>
#include <ulpProgOption.h>
#include <ulpErrorMgr.h>
#include <ulpSymTable.h>
#include <ulpMacro.h>
#include <ulpTypes.h>

/* WHENEVER 구문 정보 자료구조*/
typedef struct ulpGenWheneverDetail 
{
    SInt               mScopeDepth;
    ulpGENWHENEVERACT  mAction;
    // Action에 필요한 string 저장.
    SChar              mText[MAX_WHENEVER_ACTION_LEN];
} ulpGenWheneverDetail;

/* 현재 WHENEVER 구문 설정정보 */
typedef struct ulpWhenever
{
    ulpGENWHENEVERCOND   mCondition;  // 현재 처리대상인 Condition에 대한 정보를 갖는다.
    // 현재 Condition 에대한 action 정보를 갖는다.
    // [0] : SQL_NOTFOUND 상태에 대한 정보.
    // [1] : SQL_ERROR 상태에 대한 정보.
    ulpGenWheneverDetail mContent[2];
} ulpWhenever;

/* code변환을 위해 파싱중 호스트 변수 정보를 list로 저장한다. */
typedef struct ulpGenHostVarList
{
    SChar              mRealName[MAX_HOSTVAR_NAME_SIZE * 2];
    SChar              mRealIndName[MAX_HOSTVAR_NAME_SIZE * 2];
    SChar              mRealFileOptName[MAX_HOSTVAR_NAME_SIZE * 2];
    ulpHVarType        mInOutType;
    ulpHostDiagType    mDiagType;
    ulpSymTElement    *mValue;
    ulpSymTElement    *mInd;
} ulpGenHostVarList;

/* 현재 변환 하려는 내장 SQL구문에 대한 정보를 저장한다. */
typedef struct  ulpGenEmSQLInfo
{
    SChar       mConnName[MAX_HOSTVAR_NAME_SIZE];
    SChar       mCurName[MAX_HOSTVAR_NAME_SIZE];
    SChar       mStmtName[MAX_HOSTVAR_NAME_SIZE];
    ulpStmtType mStmttype;
    SChar       mIters[GEN_EMSQL_INFO_SIZE];
    UInt        mNumofHostvar;
    SChar       mSqlinfo[GEN_EMSQL_INFO_SIZE];
    SChar       mScrollcur[GEN_EMSQL_INFO_SIZE];

    UInt        mCursorScrollable;
    UInt        mCursorSensitivity;
    UInt        mCursorWithHold;

    // 내장 쿼리에서 cli로 넘겨주기위해 앞부분을 잘라낸 쿼리 pointer.
    SChar      *mQueryStr;
    SChar       mQueryHostValue[MAX_HOSTVAR_NAME_SIZE];

    /* host변수의 정보를 갖는 ulpSymbolNode 를 list형태로 저장한다. */
    iduList     mHostVar;
    // host변수 list에 저장된 변수들에 대한 array,struct,arraystruct type 정보저장.
    // (isarr, isstruct 관련 code 변환시 참조됨)
    ulpGENhvType       mHostValueType;

    /* PSM 여부 정보. */
    idBool mIsPSMExec;
    /* 여분의 string 정보를 저장함. */
    SChar *mExtraStr;
    /* SQL_ATTR_PARAM_STATUS_PTR 변수 정보 */
    SChar  mStatusPtr[MAX_HOSTVAR_NAME_SIZE];
    /* ATOMIC 정보.*/
    SChar  mAtomic[GEN_EMSQL_INFO_SIZE];
    /* ErrCode 변수 정보 */
    SChar  mErrCodePtr[MAX_HOSTVAR_NAME_SIZE];
    /* Multithread 여부 정보. */
    idBool mIsMT;

    /* TASK-7218 Handling Multiple Errors */
    SChar  mConditionNum[GEN_EMSQL_INFO_SIZE];
} ulpGenEmSQLInfo;

/* BUG-35518 Shared pointer should be supported in APRE */
typedef struct ulpSharedPtrInfo
{
    /* The name of shared pointer */
    SChar       mSharedPtrName[MAX_HOSTVAR_NAME_SIZE * 2];
    /* Whether it was first declaration */
    idBool      mIsFirstSharedPtr;
    /* Whether previous declaration was single array */
    idBool      mIsPrevArraySingle;
    /* Previous name of variable */
    SChar       mPrevName[MAX_HOSTVAR_NAME_SIZE * 2];
    /* Previous size of variable */
    SChar       mPrevSize[MAX_HOSTVAR_NAME_SIZE * 2];
    /* Previous type of variable */
    ulpHostType mPrevType;
} ulpSharedPtrInfo;

/* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */
typedef struct ulpGenCurFileInfo
{
    SInt  mFirstLineNo;
    SInt  mInsertedLineCnt;
    SChar mFileName[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN];
} ulpGenCurFileInfo;


/**********************************************************
 * DESCRIPTION :
 *
 * Embedded SQL 구문을 C언어로 변환시켜주는 모듈이며,
 * File 핸들과 file에 쓰기전 temprary하게 저장되는 버퍼를 관리한다.
 **********************************************************/
class ulpCodeGen
{

private:

    /* 변환된 query string이 저장된다. Initial size는 32k 이나,
       쿼리 size가 이보다 더 크면 2배로 realloc하여 사용된다.*/
    SChar *mQueryBuf;
    /* 10k 고정 size 이며, 10k가 꽉차면 자동적으로
       utpGenWriteFile 함수를 호출하여 file에 쓴다. */
    SChar mWriteBuf [ GEN_WRITE_BUF_SIZE ];

    ulpGenEmSQLInfo mEmSQLInfo;        /* 변환 하려는 내장 SQL구문에 대한 정보를 저장한다. */
    UInt mWriteBufOffset;              /* write buffer가 얼만큼 채워졌는지 나타냄 (0-base)*/
    UInt mQueryBufSize;                /* 현재 query buffer의 max size값을 갖음. */
    UInt mQueryBufOffset;              /* query buffer가 얼만큼 채워졌는지 나타냄 (0-base)*/

    /* 내장구문의 호스트 변수를 ?로 변환하는데 필요한 호스트변수 하나하나에 대한 ?개수 정보를 배열에 저장 */
    UInt *mHostVarNumArr;
    UInt mHostVarNumOffset;
    UInt mHostVarNumSize;

    FILE *mOutFilePtr;                 /* output file pointer */
    SChar mOutFileName[ID_MAX_FILE_NAME + 1];

    ulpErrorMgr mErrorMgr;             /* Error 처리를 위한 자료구조 */

    /* BUGBUG: 사실 scope table에서 관리해야 한다. */
    ulpWhenever mWhenever;             /* WHENEVER 구문 정보 */

    void ulpGenSnprintf( SChar *aBuf, UInt aSize, const SChar *aStr, SInt aType );

    /* BUG-33025 Predefined types should be able to be set by user in APRE */
    /* Flag to decide whether ulpGenWriteFile is called by doPPparse(first call) */
    UInt mIsPpFlag;


    /* BUG-35518 Shared pointer should be supported in APRE */
    ulpSharedPtrInfo mSharedPtrInfo;

    /* BUG-42357 */
    ulpGenCurFileInfo mCurFileInfoArr[MAX_HEADER_FILE_NUM];
    SInt              mCurFileInfoIdx;

public:


    IDE_RC ulpInit();

    IDE_RC ulpFinalize();

    ulpCodeGen();                      /* initialize */

    SChar *ulpGetQueryBuf();

    void ulpGenNString ( SChar *aStr, UInt aLen);  /* 특정 string을 해당 길이만큼 그대로 buffer에 쓴다. */

    void ulpGenString ( SChar *aStr );  /* 특정 string을 그대로 buffer에 쓴다. */

    void ulpGenPutChar ( SChar aCh ); /* 특정 charater를 buffer에 쓴다. */

    void ulpGenUnputChar ( void );  /* mWriteBufOffset를  감소시킨다. */

    IDE_RC ulpGenQueryString ( SChar *aStr );  /* 특정 query string을 그대로 mQueryBuf에 쓴다. */

    /* 특정 query string을 aLen 만큼 그대로 mQueryBuf에 쓴다. */
    //IDE_RC ulpGenQueryNString ( SChar *aStr, UInt aLen );

    void ulpGenComment( SChar *aStr );  /* 특정 string을 comment 형태로 buffer에 쓴다. */

    /* varchar선언 부분을 C code로 변환하여 buffer에 쓴다. */
    void ulpGenVarchar( ulpSymTElement *aSymNode );

    /* BUG-35518 Shared pointer should be supported in APRE */
    void ulpGenSharedPtr( ulpSymTElement *aSymNode );
    SChar *ulpConvertFromHostType( ulpHostType aHostType );

    /* Embedded SQL 구문에 대한 정보를 mEmSQLInfo 에 저장한다. */
    void ulpGenEmSQL( ulpGENSQLINFO aType, void *aValue );

    /* mEmSQLInfo정보와 mQueryBuf를 토대로 코드를 생성하여 mWriteBuf에 쓴다.  */
    void ulpGenEmSQLFlush( ulpStmtType aStmtType, idBool aIsPrintQuery );

    void ulpGenHostVar( ulpStmtType aStmtType, ulpGenHostVarList *aHostVar, UInt *aCnt );

    /* mEmSQLInfo->mHostVar에 host변수 정보를 갖는 utpSymbolNode 를 추가한다. */
    IDE_RC ulpGenAddHostVarList( SChar          *aRealName,
                                 ulpSymTElement *aNode,
                                 SChar          *aIndName,
                                 ulpSymTElement *aIndNode,
                                 SChar          *aFileOptName,
                                 ulpHVarType     aIOType );

    /* mWriteBuf의 data를 file에 쓴다. */
    IDE_RC ulpGenWriteFile( );

    IDE_RC ulpGenOpenFile( SChar *aFileName);

    IDE_RC ulpGenCloseFile();

    /* BUG-33025 Predefined types should be able to be set by user in APRE */
    IDE_RC ulpGenRemovePredefine(SChar *aFileName);

    /* 처음 utpInitialize 가 호출 되었던 것처럼 초기화 된다. */
    void ulpGenClearAll();

    /* query buffer 를 초기화 해준다. */
    void ulpGenInitQBuff( void );

    /* mEmSQLInfo.mNumofHostvar 를 aNum 만큼 증가 시킨다. */
    void ulpIncHostVarNum( UInt aNum );

    /* mEmSQLInfo를 초기화 해준다. */
    void ulpClearEmSQLInfo();

    /* BUG-35518 Shared pointer should be supported in APRE */
    void ulpClearSharedPtrInfo();

    /* 내장 구문을 cli가 수행할수 있도록 변경해준다. */
    void ulpTransEmQuery ( SChar *aQueryBuf );

    /* WHENEVER 구문 상태정보 설정 함수 */
    void ulpGenSetWhenever( SInt aDepth,
                            ulpGENWHENEVERCOND aCond,
                            ulpGENWHENEVERACT aAct,
                            SChar *aText );

    void ulpGenPrintWheneverAct( ulpGenWheneverDetail *aWheneverDetail );

    void ulpGenResetWhenever( SInt aDepth );

    ulpWhenever *ulpGenGetWhenever( void );

    //
    void ulpGenAddHostVarArr( UInt aNum );

    void ulpGenCutQueryTail( SChar *aToken );

    void ulpGenCutQueryTail4PSM( SChar aCh );

    /* BUG-46824 */
    void ulpGenCutStringTail4PSM( SChar *aBuf, SChar aCh );

    void ulpGenRemoveQueryToken( SChar *aToken );

    // write some code at the beginning of the .cpp file.
    void ulpGenInitPrint( void );

    ulpGenEmSQLInfo *ulpGenGetEmSQLInfo( void );

    /* BUG-29479 : double 배열 사용시 precompile 잘못되는 경우발생함. */
    /*    호스트변수 이름을 인자로 받아 이름 맨뒤에 array index를 지정하는 문법인
     *   [...] 가 몇번 반복되는지 count해주는 함수. */
    SShort ulpGenBraceCnt4HV( SChar *aValueName, SInt aLen );

    /* host value의 정보들를 bitset으로 설정해준다. */
    void ulpGenGetHostValInfo( idBool          aIsField,
                               ulpSymTElement *aHVNode,
                               ulpSymTElement *aINDNode,
                               UInt           *aHVInfo,
                               SShort          aBraceCnt4HV,
                               SShort          aBraceCnt4Ind
                             );

    /* for dbugging */
    void ulpGenDebugPrint( ulpSymTElement *aSymNode );

    /* BUG-35518 Shared pointer should be supported in APRE */
    /* get shared pointer name in ulpCompy.y */
    SChar *ulpGetSharedPtrName();

    /* BUG-42357 The -lines option is added to apre. (INC-31008) */
    void   ulpGenSetCurFileInfo( SInt   aFstLineNo,
                                 SInt   aInsLineCnt,
                                 SChar *aFileNm );
    void   ulpGenResetCurFileInfo();
    void   ulpGenAddSubHeaderFilesLineCnt();
    idBool ulpGenIsHeaderFile();
    SInt   ulpGenMakeLineMacroStr( SChar *aBuffer, UInt aBuffSize = MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN );
    void   ulpGenPrintLineMacro();
};

#endif

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

#include <ulpGenCode.h>
#include <sqlcli.h>

extern ulpProgOption gUlpProgOption;
extern int           COMPlineno; /* BUG-42357 */
extern void ulpFinalizeError();

/* BUG-33183 The structure of funtion which transforms embedded query will be reformed in APRE */
typedef enum
{
    STATE_DEFAULT = 0,
    STATE_HOSTVAR,
    STATE_QUOTED
}ulpTransEmQueryStateFlag;

/* Constructor */
ulpCodeGen::ulpCodeGen()
{

}

/* 
 * DESCRIPTION :
 *
 *  ulpCodeGen생성자에서 호출되며, 버퍼 할당하고 변수 초기화함.
 */
IDE_RC ulpCodeGen::ulpInit()
{
    mOutFilePtr = NULL;
    idlOS::memset( mOutFileName, 0, ID_SIZEOF(mOutFileName) );

    mQueryBuf = (SChar *)idlOS::malloc( GEN_INIT_QUERYBUF_SIZE * ID_SIZEOF(SChar) );   /* 32k */
    IDE_TEST_RAISE( mQueryBuf == NULL, ERR_MEMORY_ALLOC );
    idlOS::memset( mQueryBuf, 0, GEN_INIT_QUERYBUF_SIZE * ID_SIZEOF(SChar) );
    idlOS::memset( mWriteBuf, 0, GEN_WRITE_BUF_SIZE * ID_SIZEOF(SChar) );
    mWriteBufOffset = 0;
    mQueryBufSize = GEN_INIT_QUERYBUF_SIZE;        /* 32k */
    mQueryBufOffset = 0;
    mWhenever.mCondition  = GEN_WHEN_NONE;

    mHostVarNumArr = (UInt *)idlOS::malloc( INIT_NUMOF_HOSTVAR * ID_SIZEOF(UInt) );
    IDE_TEST_RAISE( mHostVarNumArr == NULL, ERR_MEMORY_ALLOC );
    mHostVarNumOffset = 0;
    mHostVarNumSize   = INIT_NUMOF_HOSTVAR;

    idlOS::memset( &mEmSQLInfo, 0, ID_SIZEOF(ulpGenEmSQLInfo) );
    IDU_LIST_INIT(&(mEmSQLInfo.mHostVar));
    ulpClearEmSQLInfo();
    /* BUG-35518 Shared pointer should be supported in APRE */
    ulpClearSharedPtrInfo();

    mEmSQLInfo.mIsMT = ID_FALSE;

    /* BUG-33025 Predefined types should be able to be set by user in APRE */
    mIsPpFlag = 0;

    /* BUG-42357 */
    mCurFileInfoIdx = -1;

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_MEMORY_ALLOC);
    {
        if (mQueryBuf != NULL)
        {
            idlOS::free(mQueryBuf);
            mQueryBuf = NULL;
        }

        ulpSetErrorCode( &mErrorMgr,
                          ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 
 * DESCRIPTION :
 *
 *  ulpCodeGen 소멸자에서 호출되며, file을 닫고 버퍼를 해제함.
 */
IDE_RC ulpCodeGen::ulpFinalize()
{
    /* 1. close file*/
    if ( mOutFilePtr != NULL )
    {
        if ( ulpGenCloseFile() == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
    }

    /* 2. mQueryBuf 메모리 해제 */
    if ( mQueryBuf != NULL )
    {
        idlOS::free(mQueryBuf);
        mQueryBuf = NULL;
    }

    /* 3. mHostVar 해제 */
    ulpClearEmSQLInfo();

    if ( mHostVarNumArr != NULL )
    {
        idlOS::free(mHostVarNumArr);
        mHostVarNumArr = NULL;
    }

    return IDE_SUCCESS;

}


SChar *ulpCodeGen::ulpGetQueryBuf()
{
    return mQueryBuf;
}


/*
 * DESCRIPTION :
 *
 *  mEmSQLInfo 초기화.
 */
void ulpCodeGen::ulpClearEmSQLInfo()
{
    iduListNode*    sIterator = NULL;
    iduListNode*    sNextNode = NULL;

    mEmSQLInfo.mConnName[0]     = '\0';
    mEmSQLInfo.mStmttype        = S_UNKNOWN;
    mEmSQLInfo.mCurName[0]      = '\0';
    mEmSQLInfo.mStmtName[0]     = '\0';
    mEmSQLInfo.mIters[0]        = '\0';
    mEmSQLInfo.mNumofHostvar    = 0;
    mEmSQLInfo.mSqlinfo[0]      = '\0';
    mEmSQLInfo.mScrollcur[0]    = '\0';

    mEmSQLInfo.mCursorScrollable  = SQL_NONSCROLLABLE;
    mEmSQLInfo.mCursorSensitivity = SQL_INSENSITIVE;
    mEmSQLInfo.mCursorWithHold    = SQL_CURSOR_HOLD_OFF;

    mEmSQLInfo.mQueryStr        = NULL;
    mEmSQLInfo.mQueryHostValue[0] = '\0';
    mEmSQLInfo.mIsPSMExec       = ID_FALSE;
    mEmSQLInfo.mExtraStr        = NULL;
    mEmSQLInfo.mStatusPtr[0]    = '\0';
    mEmSQLInfo.mAtomic[0]       = '\0';
    mEmSQLInfo.mErrCodePtr[0]   = '\0';
    mEmSQLInfo.mHostValueType   = GEN_GENERAL;

    mHostVarNumOffset = 0;

    /* mHostVar list 모두 해제함. */
    IDU_LIST_ITERATE_SAFE(&(mEmSQLInfo.mHostVar), sIterator, sNextNode)
    {
        IDU_LIST_REMOVE( sIterator );
        idlOS::free(sIterator->mObj);
        idlOS::free(sIterator);
    }
    IDU_LIST_INIT(&(mEmSQLInfo.mHostVar));
}

/* BUG-35518 Shared pointer should be supported in APRE */
void ulpCodeGen::ulpClearSharedPtrInfo()
{
    mSharedPtrInfo.mSharedPtrName[0] = '\0';
    mSharedPtrInfo.mIsFirstSharedPtr = ID_TRUE;
    mSharedPtrInfo.mIsPrevArraySingle = ID_TRUE;
    mSharedPtrInfo.mPrevName[0] = '\0';
    mSharedPtrInfo.mPrevSize[0] = '\0';
    mSharedPtrInfo.mPrevType = H_UNKNOWN;
}

/*
 * DESCRIPTION :
 *
 *  특정 string을 그대로 buffer에 쓴다. (재귀함수)
 */
void ulpCodeGen::ulpGenNString ( SChar *aStr, UInt aLen)
{
    UInt sEmptyLen;
    /* mWriteBuf의 빈공간 byte 크기 */
    sEmptyLen = GEN_WRITE_BUF_SIZE - mWriteBufOffset;

    if ( sEmptyLen < aLen )
    {   /* 빈공간의 크기가 쓰려하는 길이보다 작을경우, 일단 빈공간 만큼만 버퍼에 쓴다. */
        if ( sEmptyLen > 0 )
        {
            ulpGenSnprintf( mWriteBuf + mWriteBufOffset, sEmptyLen, aStr, GEN_WRITE_BUF );
        }
        /* 버퍼가 꽉찼으므로, 파일에 flush한다. mWriteBufOffset은 0으로 초기화된다. */
        if ( ulpGenWriteFile() == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
        /* 재귀적으로 반복하여 버퍼에 쓴다. */
        ulpGenNString ( (SChar *)(aStr + sEmptyLen), aLen - sEmptyLen );
    }
    else
    {   /* 버퍼공간이 충분하면 버퍼에 쓴다. */
        if ( aLen > 0 )
        {
            ulpGenSnprintf( mWriteBuf + mWriteBufOffset, sEmptyLen, aStr, GEN_WRITE_BUF );
        }
    }
}

/* 
 * DESCRIPTION :
 *
 *  특정 string을 그대로 buffer에 쓴다. (재귀함수)
 */
void ulpCodeGen::ulpGenString ( SChar *aStr )
{
    UInt sLen;
    UInt sEmptyLen;
    /* mWriteBuf의 빈공간 byte 크기 */
    sEmptyLen = GEN_WRITE_BUF_SIZE - mWriteBufOffset;
    sLen = idlOS::strlen( aStr );

    if ( sEmptyLen < sLen )
    {   /* 빈공간의 크기가 쓰려하는 길이보다 작을경우, 일단 빈공간 만큼만 버퍼에 쓴다. */
        if ( sEmptyLen > 0 )
        {
            ulpGenSnprintf( mWriteBuf + mWriteBufOffset, sEmptyLen, aStr, GEN_WRITE_BUF );
        }
        /* 버퍼가 꽉찼으므로, 파일에 flush한다. mWriteBufOffset은 0으로 초기화된다. */
        if ( ulpGenWriteFile() == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
        /* 재귀적으로 반복하여 버퍼에 쓴다. */
        ulpGenString ( aStr + sEmptyLen );
    }
    else
    {   /* 버퍼공간이 충분하면 버퍼에 쓴다. */
        if ( sLen > 0 )
        {
            ulpGenSnprintf( mWriteBuf + mWriteBufOffset, sEmptyLen, aStr, GEN_WRITE_BUF );
        }
    }
}


/* 
 * DESCRIPTION :
 *
 *  특정 string을 comment 형태로 buffer에 쓴다.
 */
//  ex>  <string>     =>       /*<string>*/
void ulpCodeGen::ulpGenComment( SChar *aStr )
{
    UInt sEmptyLen;
    UInt sCommLLen;
    UInt sCommRLen;
    UInt sLen;
    SChar *sCh;
    idBool sIsChange;
    SChar sCommL[10] = "/* ";
    SChar sCommR[10] = " */";
    sCommLLen = idlOS::strlen( sCommL );
    sCommRLen = idlOS::strlen( sCommR );

    sIsChange = ID_FALSE;
    sCh  = NULL;
    sLen = idlOS::strlen(aStr);

    /* mWriteBuf의 빈공간 byte 크기 */
    sEmptyLen = GEN_WRITE_BUF_SIZE - mWriteBufOffset;

    /* 1. string안에 주석 기호가 있으면 '*'를 '#'문자로 바꿔준다. */
    while ( (sCh = idlOS::strstr(aStr, "*/")) != NULL )
    {
        idlOS::strncpy (sCh,"#",1);
        sIsChange = ID_TRUE;
    }

    while ( (sCh = idlOS::strstr(aStr, "/*")) != NULL )
    {
        idlOS::strncpy (sCh+1,"#",1);
    }


    /* 2. 버퍼가 거의 꽉차있어 sCommL을 쓸 수 없을경우 처리. */
    if ( sEmptyLen < sCommLLen )
    {
        if ( ulpGenWriteFile() == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
    }

    /* 3. sCommL을 버퍼에 쓴다. */
    ulpGenNString ( sCommL, sCommLLen );

    /* 4. 주석안의 string을 버퍼에 쓴다. */
    ulpGenNString ( aStr, sLen );

    sEmptyLen = GEN_WRITE_BUF_SIZE - mWriteBufOffset;
    /* 5. 버퍼가 거의 꽉차있어 sCommR을 쓸 수 없을경우 처리. */
    if ( sEmptyLen < sCommRLen )
    {
        if ( ulpGenWriteFile() == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
    }

    /* 6. sCommR을 버퍼에 쓴다. */
    ulpGenNString ( sCommR, sCommRLen );


    /* #로 변경된 query buffer 를 다시 복구한다. (HINT처리를 위함.) */
    if ( sIsChange == ID_TRUE )
    {
        while ( (sCh = idlOS::strstr(aStr, "#/")) != NULL )
        {
            idlOS::strncpy (sCh,"*",1);
        }

        while ( (sCh = idlOS::strstr(aStr, "/#")) != NULL )
        {
            idlOS::strncpy (sCh+1,"*",1);
        }
    }

}


/*
 * DESCRIPTION :
 *
 *  인자로 받은 aStr을 버퍼에 쓰며, offest을 쓰는 길이 만큼 증가시켜 준다.
 */
void ulpCodeGen::ulpGenSnprintf( SChar *aBuf, UInt aSize, const SChar *aStr, SInt aType )
{
    UInt sLen;

    // 함수명은 ulpGenSnprintf인데 실제로는 strncpy를 호출하네;;
    // snprintf 를 사용하면 안된다. 왜냐하면, snprintf는
    // 항상 제일 뒤에 null을 포함시키기 때문이다.
    idlOS::strncpy( aBuf, aStr, aSize);

    sLen = idlOS::strlen( aStr );

    switch ( aType )
    {
        case GEN_WRITE_BUF :
            // aSize는 write buffer의 남은 공간이며,
            // 쓸 string 길이 sLen가 더 클경우 offset을 aSize만큼 더함.
            if( sLen > aSize )
            {
                mWriteBufOffset += aSize;
            }
            else
            {
                mWriteBufOffset += sLen;
            }
            break;
        case GEN_QUERY_BUF :
            if( sLen > aSize )
            {
                mQueryBufOffset += aSize;
            }
            else
            {
                mQueryBufOffset += sLen;
            }
            break;
        default:
            IDE_ASSERT(0);
            break;
    }
}

/*
 * DESCRIPTION :
 *
 *  varchar선언 부분을 C code로 변환하여 buffer에 쓴다.
 */
void ulpCodeGen::ulpGenVarchar( ulpSymTElement *aSymNode )
{
    const SChar *sVarcharStr;
    SChar sTmpStr[ MAX_HOSTVAR_NAME_SIZE * 2 ];
    UInt  sVarcharLen;

    sVarcharStr = "struct { SQLLEN len; char arr";
    sVarcharLen = idlOS::strlen( sVarcharStr );
    /* write버퍼에 쓰자. */
    ulpGenNString ( (SChar *)sVarcharStr, sVarcharLen);

    sVarcharLen = MAX_HOSTVAR_NAME_SIZE * 2;

    // mAlloc  은 * 로 선언된 변수일경우 참이 되며,
    // mIsarray는 []로 선언된 변수일경우 참이된다.
    // mPointer는 몇중 포인터/배열 인지에 대한 정보를 저장함.
    if( aSymNode -> mAlloc != ID_TRUE )
    {   // varchar xxx[...][...]...; 일경우
        if( aSymNode->mIsarray == ID_TRUE )
        {
            if ( (aSymNode->mArraySize2[0] == '\0') &&
                 (aSymNode->mPointer == 1) )
            {
                idlOS::snprintf( sTmpStr, sVarcharLen, "[%s]; } %s;\n",
                                 aSymNode->mArraySize, aSymNode->mName );
            }
            else
            {
                idlOS::snprintf( sTmpStr, sVarcharLen, "[%s]; } %s[%s];\n",
                                 aSymNode->mArraySize2, aSymNode->mName, aSymNode->mArraySize );
            }
        }
        else
        {
            idlOS::snprintf( sTmpStr, sVarcharLen, "; } %s;\n", aSymNode->mName );
        }
    }
    else
    {   // varchar *...xxx...; 일경우
        if ( aSymNode->mArraySize[0] == '\0' )
        {
            if( aSymNode->mPointer == 1 )
            {
                idlOS::snprintf( sTmpStr, sVarcharLen, "[1]; } *%s;\n",
                                 aSymNode->mName );
            }
            else
            {
                // 1 < mPointer
                idlOS::snprintf( sTmpStr, sVarcharLen, "[1]; } **%s;\n",
                                 aSymNode->mName );
            }
        }
        else
        {
            idlOS::snprintf( sTmpStr, sVarcharLen, "[1]; } *%s[%s];\n",
                             aSymNode->mName, aSymNode->mArraySize );
        }
    }

    /* write버퍼에 쓰자. */
    ulpGenNString ( sTmpStr, idlOS::strlen(sTmpStr) );
}


/*
 * DESCRIPTION :
 *
 *  특정 query string을 그대로 mQueryBuf에 쓴다. (재귀함수)
 */
IDE_RC ulpCodeGen::ulpGenQueryString ( SChar *aStr )
{
    UInt sLen;
    UInt sEmptyLen;
    /* mQueryBuf 빈공간 byte 크기 */
    sEmptyLen = mQueryBufSize - mQueryBufOffset;
    sLen = idlOS::strlen( aStr );

    if ( sEmptyLen < sLen )
    {   /* 빈공간의 크기가 쓰려하는 길이보다 작을경우, size를 두배로 하여 realloc. */
        mQueryBuf = (SChar *)idlOS::realloc( mQueryBuf, mQueryBufSize * 2 );
        IDE_TEST_RAISE( mQueryBuf == NULL, ERR_MEMORY_ALLOC );
        mQueryBufSize *= 2;
        /* 재귀적으로 호출하여 버퍼 size가 작으면 relooac한다. */
        if( ulpGenQueryString ( aStr ) == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
    }
    else
    {   /* 버퍼공간이 충분하면 버퍼에 쓴다. */
        if ( sLen > 0 )
        {
            ulpGenSnprintf( mQueryBuf + mQueryBufOffset, sEmptyLen, aStr, GEN_QUERY_BUF );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_MEMORY_ALLOC);
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 
 * DESCRIPTION :
 *
 *  Embedded SQL 구문에 대한 정보를 mEmSQLInfo 에 저장한다.
 */
void ulpCodeGen::ulpGenEmSQL( ulpGENSQLINFO aType, void *aValue )
{
    switch( aType )
    {
        case GEN_CONNNAME :
            idlOS::snprintf( mEmSQLInfo.mConnName, MAX_HOSTVAR_NAME_SIZE, (SChar *) aValue );
            break;
        case GEN_CURNAME :
            idlOS::snprintf( mEmSQLInfo.mCurName, MAX_HOSTVAR_NAME_SIZE, (SChar *) aValue );
            break;
        case GEN_STMTNAME :
            idlOS::snprintf( mEmSQLInfo.mStmtName, MAX_HOSTVAR_NAME_SIZE, (SChar *) aValue );
            break;
        case GEN_STMTTYPE :
            mEmSQLInfo.mStmttype = *((ulpStmtType *) aValue);
            break;
        case GEN_ITERS :
            idlOS::snprintf( mEmSQLInfo.mIters, GEN_EMSQL_INFO_SIZE, (SChar *) aValue );
            break;
        case GEN_NUMHOSTVAR :
            mEmSQLInfo.mNumofHostvar = *((UInt*)aValue);
            break;
        case GEN_SQLINFO :
            idlOS::snprintf( mEmSQLInfo.mSqlinfo, GEN_EMSQL_INFO_SIZE, (SChar *) aValue );
            break;
        case GEN_SCROLLCUR :
            idlOS::snprintf( mEmSQLInfo.mScrollcur, GEN_EMSQL_INFO_SIZE, (SChar *) aValue );
            break;

        case GEN_CURSORSCROLLABLE :
            mEmSQLInfo.mCursorScrollable = *((UInt *)aValue);
            break;
        case GEN_CURSORSENSITIVITY :
            mEmSQLInfo.mCursorSensitivity = *((UInt *)aValue);
            break;
        case GEN_CURSORWITHHOLD :
            mEmSQLInfo.mCursorWithHold = *((UInt *)aValue);
            break;

        case GEN_QUERYSTR :
            mEmSQLInfo.mQueryStr = (SChar *)aValue;
            break;
        case GEN_QUERYHV :
            idlOS::snprintf( mEmSQLInfo.mQueryHostValue, MAX_HOSTVAR_NAME_SIZE,
                             (SChar *) aValue );
            break;
        case GEN_PSMEXEC :
            mEmSQLInfo.mIsPSMExec = *((idBool*)aValue);
            break;
        case GEN_EXTRASTRINFO :
            mEmSQLInfo.mExtraStr = (SChar *)aValue;
            break;
        case GEN_STATUSPTR :
            idlOS::snprintf( mEmSQLInfo.mStatusPtr, MAX_HOSTVAR_NAME_SIZE,
                             (SChar *) aValue );
            break;
        case GEN_ATOMIC :
            idlOS::snprintf( mEmSQLInfo.mAtomic, GEN_EMSQL_INFO_SIZE,
                             (SChar *) aValue );
            break;
        case GEN_ERRCODEPTR :
            idlOS::snprintf( mEmSQLInfo.mErrCodePtr, MAX_HOSTVAR_NAME_SIZE,
                             (SChar *) aValue );
            break;
        case GEN_MT :
            mEmSQLInfo.mIsMT = *((idBool*)aValue);
            break;
        case GEN_HVTYPE :
            mEmSQLInfo.mHostValueType = *((ulpGENhvType*)aValue);
            break;
        case GEN_CONDITIONNUM :
            idlOS::snprintf( mEmSQLInfo.mConditionNum, GEN_EMSQL_INFO_SIZE, (SChar *) aValue );
            break;
        default :
            /* 그외의 정보는 없다. */
            IDE_ASSERT(0);
            break;
    } 
}

/* 
 * DESCRIPTION :
 *
 *   mEmSQLInfo.mNumofHostvar 를 aNum 만큼 증가 시킨다.
 */
void ulpCodeGen::ulpIncHostVarNum( UInt aNum )
{
    mEmSQLInfo.mNumofHostvar += aNum;
}


/* 
 * DESCRIPTION :
 *
 *  mEmSQLInfo정보와 mQueryBuf를 토대로 코드를 생성하여 mWriteBuf에 쓴다.
 */
void ulpCodeGen::ulpGenEmSQLFlush( ulpStmtType aStmtType, idBool aIsPrintQuery )
{
    /* 각 정보에 대한 변환 코드 임시 저장소, 2k 면 충분하지 않을까? */
    SChar              sTmpStr[MAX_HOSTVAR_NAME_SIZE * 2];
    UInt               sCnt;
    iduListNode       *sIterator = NULL;
    ulpGenHostVarList *sHostVar  = NULL;
    ulpWhenever       *sWhenever = NULL;

    /* 1. basic code 생성. */
    ulpGenString( (SChar *)"\n" );

    ulpGenPrintLineMacro();
    ulpGenString( (SChar *)"{\n" );

    ulpGenPrintLineMacro();
    ulpGenString( (SChar *)"    struct ulpSqlstmt ulpSqlstmt;\n" );

    ulpGenPrintLineMacro();
    ulpGenString ( (SChar *)"    memset(&ulpSqlstmt, 0, sizeof(ulpSqlstmt));\n" );

    if ( mEmSQLInfo.mNumofHostvar > 0 )
    {
        ulpGenPrintLineMacro();
        idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                         "    ulpHostVar ulpHostVar[%d];\n",
                         mEmSQLInfo.mNumofHostvar );
        ulpGenString ( sTmpStr );
        ulpGenPrintLineMacro();
        ulpGenString ( (SChar *)"    ulpSqlstmt.hostvalue = ulpHostVar;\n" );
    }

    if( aStmtType != S_UNKNOWN )
    {
        ulpGenPrintLineMacro();
        idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                         "    ulpSqlstmt.stmttype = %d;\n",
                         aStmtType );
        ulpGenString ( sTmpStr );
    }
    else
    {
        // invalid stmt type
        IDE_ASSERT(0);
    }

    if( mEmSQLInfo.mStmtName[0] != '\0' )
    {
        ulpGenPrintLineMacro();
        idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                         "    ulpSqlstmt.stmtname = (char*) \"%s\";\n",
                         mEmSQLInfo.mStmtName );
        ulpGenString ( sTmpStr );
    }
    else
    {
        ulpGenPrintLineMacro();
        ulpGenString ( (SChar *)"    ulpSqlstmt.stmtname = NULL;\n" );
    }

    if( mEmSQLInfo.mCurName[0] != '\0' )
    {
        ulpGenPrintLineMacro();
        idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                         "    ulpSqlstmt.curname = (char*) \"%s\";\n",
                         mEmSQLInfo.mCurName );
        ulpGenString ( sTmpStr );
    }

    if( mEmSQLInfo.mIsMT == ID_TRUE )
    {
        ulpGenPrintLineMacro();
        ulpGenString ( (SChar *)"    ulpSqlstmt.ismt = 1;\n" );
    }
    else
    {
        ulpGenPrintLineMacro();
        ulpGenString ( (SChar *)"    ulpSqlstmt.ismt = 0;\n" );
    }

    /* 2. mEmSQLInfo를 토대로 code 생성 한다. */
    switch( aStmtType )
    {
        case S_Connect:
            if ( gUlpProgOption.mNCharUTF16 == ID_TRUE )
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.sqlinfo = 1;\n" );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.sqlinfo = 0;\n" );
            }
        case S_Disconnect:
            if( mEmSQLInfo.mNumofHostvar > 0 )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.numofhostvar = %d;\n",
                                 mEmSQLInfo.mNumofHostvar );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.numofhostvar = 0;\n" );
            }
            break;
        case S_AlterSession:
            if( mEmSQLInfo.mExtraStr != NULL )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.extrastr = \"%s\";\n",
                                 mEmSQLInfo.mExtraStr );
                ulpGenString ( sTmpStr );
            }
            break;
        case S_FailOver:
            if( mEmSQLInfo.mSqlinfo[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.sqlinfo = %s;\n", mEmSQLInfo.mSqlinfo );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.sqlinfo = 0;\n" );
            }
            break;
        case S_GetStmtDiag:
        case S_GetConditionDiag:
            /* TASK-7218 Handling Multiple Errors */
            if( mEmSQLInfo.mNumofHostvar > 0 )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.numofhostvar = %d;\n",
                                 mEmSQLInfo.mNumofHostvar );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.numofhostvar = 0;\n" );
            }

            if( mEmSQLInfo.mConditionNum[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.iters = %s;\n", mEmSQLInfo.mConditionNum );
                ulpGenString ( sTmpStr );
            }
            break;
        default:
            if( mEmSQLInfo.mNumofHostvar > 0 )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.numofhostvar = %d;\n",
                                 mEmSQLInfo.mNumofHostvar );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.numofhostvar = 0;\n" );
            }

            if ( mEmSQLInfo.mStatusPtr[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.statusptr = %s;\n",
                                 mEmSQLInfo.mStatusPtr );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.statusptr = NULL;\n" );
            }

            if ( mEmSQLInfo.mErrCodePtr[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.errcodeptr = %s;\n",
                                 mEmSQLInfo.mErrCodePtr );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.errcodeptr = NULL;\n" );
            }

            if (mEmSQLInfo.mAtomic[0] != '\0')
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.isatomic = 1;\n" );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.isatomic = 0;\n" );
            }

            if( aIsPrintQuery == ID_TRUE )
            {
                if ( mEmSQLInfo.mQueryHostValue[0] != '\0' )
                {
                    ulpGenPrintLineMacro();
                    idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                     "    ulpSqlstmt.stmt = (char *)%s;\n",
                                     mEmSQLInfo.mQueryHostValue );
                    ulpGenString ( sTmpStr );
                }
                else
                {
                    if ( mQueryBuf != NULL )
                    {
                        ulpGenPrintLineMacro();
                        ulpGenString ( (SChar *)"    ulpSqlstmt.stmt = (char *)" );

                        if ( mEmSQLInfo.mIsPSMExec == ID_TRUE )
                        {
                            ulpGenString ( (SChar *)"\"execute \" " );
                        }

                        if ( mEmSQLInfo.mQueryStr != NULL )
                        {
                            ulpTransEmQuery ( mEmSQLInfo.mQueryStr );
                        }
                        else
                        {
                            ulpTransEmQuery ( mQueryBuf );
                        }
                        ulpGenString ( (SChar *)";\n" );
                    }
                }
            }
            else
            {
                /* do nothing */
            }

            if( mEmSQLInfo.mIters[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.iters = %s;\n", mEmSQLInfo.mIters );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.iters = 0;\n" );
            }

            if( mEmSQLInfo.mSqlinfo[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                                 "    ulpSqlstmt.sqlinfo = %s;\n", mEmSQLInfo.mSqlinfo );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.sqlinfo = 0;\n" );
            }

            if( mEmSQLInfo.mScrollcur[0] != '\0' )
            {
                ulpGenPrintLineMacro();
                idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2, 
                                 "    ulpSqlstmt.scrollcur = %s;\n",
                                 mEmSQLInfo.mScrollcur );
                ulpGenString ( sTmpStr );
            }
            else
            {
                ulpGenPrintLineMacro();
                ulpGenString ( (SChar *)"    ulpSqlstmt.scrollcur = 0;\n" );
            }

            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2, 
                             "    ulpSqlstmt.cursorscrollable = %d;\n",
                             mEmSQLInfo.mCursorScrollable );
            ulpGenString ( sTmpStr );

            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2, 
                             "    ulpSqlstmt.cursorsensitivity = %d;\n",
                             mEmSQLInfo.mCursorSensitivity );
            ulpGenString ( sTmpStr );

            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2, 
                             "    ulpSqlstmt.cursorwithhold = %d;\n",
                             mEmSQLInfo.mCursorWithHold );
            ulpGenString ( sTmpStr );

            break;
    }

    ulpGenPrintLineMacro();
    ulpGenString ( (SChar *)"    ulpSqlstmt.esqlopts    = _esqlopts;\n" );
    // 내장구문 에러결과를 위한 코드생성
    ulpGenPrintLineMacro();
    ulpGenString ( (SChar *)"    ulpSqlstmt.sqlcaerr    = &sqlca;\n" );
    ulpGenPrintLineMacro();
    ulpGenString ( (SChar *)"    ulpSqlstmt.sqlcodeerr  = &SQLCODE;\n" );
    ulpGenPrintLineMacro();
    ulpGenString ( (SChar *)"    ulpSqlstmt.sqlstateerr = ulpGetSqlstate();\n" );

    // 호스트 변수 관련 코드생성
    for ( sCnt = 0, (sIterator) = (&(mEmSQLInfo.mHostVar))->mNext;
          (sIterator) != &(mEmSQLInfo.mHostVar);
          (sIterator) = (sIterator)->mNext, sCnt++ )
    {
        sHostVar = (ulpGenHostVarList *)sIterator->mObj;
        ulpGenHostVar( aStmtType, sHostVar, &sCnt );
    }


    if( mEmSQLInfo.mConnName[0] != '\0' )
    {
        if( mEmSQLInfo.mConnName[0] == ':' )
        {
            // :host변수 일경우 처리.
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                             "    ulpDoEmsql( (char*) %s, &ulpSqlstmt, NULL );\n",
                             mEmSQLInfo.mConnName + 1 );
        }
        else
        {
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                             "    ulpDoEmsql( (char*) \"%s\", &ulpSqlstmt, NULL );\n",
                             mEmSQLInfo.mConnName );
        }

        ulpGenPrintLineMacro();
        ulpGenString ( sTmpStr );
    }
    else
    {
        ulpGenPrintLineMacro();
        ulpGenString ((SChar *)"    ulpDoEmsql( NULL, &ulpSqlstmt, NULL );\n" );
    }

    // WHENEVER 구문 코드생성.
    sWhenever = ulpGenGetWhenever();
    switch( sWhenever->mCondition )
    {
        case GEN_WHEN_NONE:
            break;
        case GEN_WHEN_NOTFOUND:
            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                             "    if(sqlca.sqlcode == SQL_NO_DATA) " );
            ulpGenString ( sTmpStr );
            ulpGenPrintWheneverAct( &(sWhenever->mContent[GEN_WHEN_NOTFOUND]) );
            break;
        case GEN_WHEN_SQLERROR:
            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                             "    if(sqlca.sqlcode == SQL_ERROR) " );
            ulpGenString ( sTmpStr );
            ulpGenPrintWheneverAct( &(sWhenever->mContent[GEN_WHEN_SQLERROR]) );
            break;
        case GEN_WHEN_NOTFOUND_SQLERROR:
            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                             "    if(sqlca.sqlcode == SQL_NO_DATA) " );
            ulpGenString ( sTmpStr );
            ulpGenPrintWheneverAct( &(sWhenever->mContent[GEN_WHEN_NOTFOUND]) );

            ulpGenPrintLineMacro();
            idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE * 2,
                             "    if(sqlca.sqlcode == SQL_ERROR) " );
            ulpGenString ( sTmpStr );
            ulpGenPrintWheneverAct( &(sWhenever->mContent[GEN_WHEN_SQLERROR]) );
            break;
    }

    ulpGenPrintLineMacro();
    ulpGenString ( (SChar *)"}\n" );

    ulpGenPrintLineMacro();
}


void ulpCodeGen::ulpGenGetHostValInfo( idBool          aIsField,
                                       ulpSymTElement *aHVNode,
                                       ulpSymTElement *aINDNode,
                                       UInt           *aHVInfo,
                                       SShort          aBraceCnt4HV,
                                       SShort          aBraceCnt4Ind
                                     )
{
    idlOS::memset(aHVInfo, 0, ID_SIZEOF(UInt));
    if( aHVNode != NULL )
    {
        *aHVInfo|=GEN_HVINFO_IS_SYMNODE;
        if( aHVNode -> mIsstruct == ID_TRUE )
        {
            *aHVInfo|=GEN_HVINFO_IS_STRUCT;
        }
        if ( (aHVNode -> mType == H_VARCHAR) ||
             (aHVNode -> mType == H_NVARCHAR) )
        {
            *aHVInfo|=GEN_HVINFO_IS_VARCHAR;
        }
        if( aIsField == ID_TRUE )
        {
            if( aHVNode -> mPointer == 1 )
            {
                *aHVInfo|=GEN_HVINFO_IS_1POINTER;
            }
            if( aHVNode -> mPointer >= 2 )
            {
                *aHVInfo|=GEN_HVINFO_IS_2POINTER;
            }
        }
        else
        {
            if( aHVNode -> mPointer - aBraceCnt4HV == 1 )
            {
                *aHVInfo|=GEN_HVINFO_IS_1POINTER;
            }
            if( aHVNode -> mPointer - aBraceCnt4HV >= 2 )
            {
                *aHVInfo|=GEN_HVINFO_IS_2POINTER;
            }
        }
        if( aHVNode -> mIsarray == ID_TRUE )
        {
            *aHVInfo|=GEN_HVINFO_IS_ARRAY;
        }
        if( aHVNode -> mArraySize[0] != '\0' )
        {
            *aHVInfo|=GEN_HVINFO_IS_ARRAYSIZE1;
        }
        if( aHVNode -> mArraySize2[0] != '\0' )
        {
            *aHVInfo|=GEN_HVINFO_IS_ARRAYSIZE2;
        }
        if( aHVNode -> mAlloc == ID_TRUE )
        {
            *aHVInfo|=GEN_HVINFO_IS_DALLOC;
        }
        if( aHVNode -> mIssign == ID_TRUE )
        {
            *aHVInfo|=GEN_HVINFO_IS_SIGNED;
        }
        if( (aHVNode->mType == H_CLOB_FILE) ||
            (aHVNode->mType == H_BLOB_FILE) )
        {
            *aHVInfo|=GEN_HVINFO_IS_LOB;
        }
        if( aHVNode->mMoreInfo == 1 )
        {
            *aHVInfo|=GEN_HVINFO_IS_MOREINFO;
        }
        switch( aHVNode->mType )
        {
            case H_CLOB:
            case H_BLOB:
            case H_NUMERIC:
            case H_NIBBLE:
            case H_BIT:
            case H_BYTES:
            case H_VARBYTE:
            case H_BINARY:
            case H_BINARY2:  /* BUG-46418 */
            case H_CHAR:
            case H_VARCHAR:
            case H_NCHAR:
            case H_NVARCHAR:
            case H_CLOB_FILE:
            case H_BLOB_FILE:
                *aHVInfo|=GEN_HVINFO_IS_STRTYPE; 
                break;
            default:
                break;
        }
    }
    if( aINDNode != NULL )
    {
        *aHVInfo|=GEN_HVINFO_IS_INDNODE;
        if( aINDNode-> mIsstruct == ID_TRUE )
        {
            *aHVInfo|=GEN_HVINFO_IS_INDSTRUCT;
        }
        if( aINDNode -> mPointer - aBraceCnt4Ind > 0 )
        {
            *aHVInfo|=GEN_HVINFO_IS_INDPOINTER;
        }
    }
}


void ulpCodeGen::ulpGenHostVar( ulpStmtType aStmtType, ulpGenHostVarList *aHostVar, UInt *aCnt )
{
    SInt   sI;
    UInt   sHostValInfo, sHostValInfo4Field;
    UInt   sSLength = 0;
    SShort sBraceCnt4HV;
    SShort sBraceCnt4Ind;
    SChar  sTmpStr[GEN_WRITE_BUF_SIZE];  // 10K
    SChar  sHVFieldOpt[GEN_FIELD_OPT_LEN];
    SChar  sINDFieldOpt[GEN_FIELD_OPT_LEN];
    idBool sIsField;

    /*BUG-28414*/
    ulpSymTElement *sSymNode      = aHostVar->mValue;
    ulpSymTElement *sFieldNode;
    ulpSymTNode    *sFieldSymTNode;
    ulpSymTElement *sIndSymNode   = aHostVar->mInd;
    ulpSymTElement *sIndFieldNode = NULL;
    ulpSymTNode    *sIndFieldSymTNode;

    /* BUG-29479 : double 배열 사용시 precompile 잘못되는 경우발생함. */
    // 내장구문내에서 사용중인 호스트 변수의 brace 수를 얻어온다.
    // ex> :hostval[10]    => brace count = 1
    // ex> :hostval[10][0] => brace count = 2
    sBraceCnt4HV  = ulpGenBraceCnt4HV( aHostVar->mRealName,
                                       idlOS::strlen(aHostVar->mRealName) );
    sBraceCnt4Ind = ulpGenBraceCnt4HV( aHostVar->mRealIndName,
                                       idlOS::strlen(aHostVar->mRealIndName) );

    sIsField = ID_FALSE;
    // get host variable info.
    ulpGenGetHostValInfo( sIsField, sSymNode, sIndSymNode, &sHostValInfo,
                          sBraceCnt4HV, sBraceCnt4Ind );

    switch( aStmtType )
    {
        case S_Connect:
            if( GEN_VERIFY_BIT(sHostValInfo, GEN_HVINFO_IS_SYMNODE|GEN_HVINFO_IS_VARCHAR) )
            {
                PRINT_LineMacro();
                PRINT_mHostVar("(void*)%s.arr",*aCnt,aHostVar->mRealName);
            }
            else
            {
                PRINT_LineMacro();
                PRINT_mHostVar("(void*)%s",*aCnt,aHostVar->mRealName );
            }
            WRITEtoFile(sTmpStr,sSLength);
            break;
        case S_FailOver:
            PRINT_LineMacro();
            PRINT_mHostVar("(void*)%s",*aCnt,aHostVar->mRealName );
            PRINT_LineMacro();
            PRINT_mType   ("%d",*aCnt,sSymNode -> mType );
            WRITEtoFile(sTmpStr,sSLength);
            break;
        default:
            if( !GEN_VERIFY_BIT(sHostValInfo, GEN_HVINFO_IS_STRUCT) )
            {   // is not a struct
                //====================
                // 1. mHostVar 코드생성.
                //====================

                /* BUG-29479 : double 배열 사용시 precompile 잘못되는 경우발생함. */
                /*
                변수가 포인터 타입일 지라도, 내장구문안에서 :<variable name>[index] 이라고 표현되면
                더이상 포인터 타입이 아닐수 있다.
                ex>
                int *a;
                int b[10];
                exec sql insert into t1 values ( :a[0], :b[0] );
                =>a[0] 와 b[0]는 pointer type이 아니라 int type이다.
                따라서, 내장 구문 안에 사용되는 array or pointer 변수뒤에 [index] 가 왔을경우를 위해
                sSymNode -> mPointer( pointer depth ) 에 sBraceCnt4HV( brace수 )를 빼줘야한다.
                */
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_VARCHAR |
                                        GEN_HVINFO_IS_1POINTER|
                                        GEN_HVINFO_IS_2POINTER) )
                {
                    case (GEN_HVINFO_IS_VARCHAR):
                        PRINT_LineMacro();
                        PRINT_mHostVar("(void*)&(%s.arr)",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_1POINTER):
                    case (GEN_HVINFO_IS_2POINTER):
                        PRINT_LineMacro();
                        PRINT_mHostVar("(void*)(%s)",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER):
                        PRINT_LineMacro();
                        PRINT_mHostVar("(void*)%s.arr",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                        PRINT_LineMacro();
                        PRINT_mHostVar("(void*)%s[0].arr",*aCnt, aHostVar->mRealName );
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_mHostVar("(void*)&(%s)",*aCnt, aHostVar->mRealName );
                        break;
                }

                //====================
                // 2. mHostInd, mVcInd 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_VARCHAR |
                                        GEN_HVINFO_IS_1POINTER|
                                        GEN_HVINFO_IS_2POINTER|
                                        GEN_HVINFO_IS_INDNODE |
                                        GEN_HVINFO_IS_INDPOINTER) )
                {
                    case (GEN_HVINFO_IS_INDNODE):
                    case (GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_INDNODE):
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_INDNODE):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)&(%s)",*aCnt, aHostVar->mRealIndName );
                        break;
                    case (GEN_HVINFO_IS_INDNODE|GEN_HVINFO_IS_INDPOINTER):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)(%s)",*aCnt, aHostVar->mRealIndName );
                        break;
                    case (GEN_HVINFO_IS_VARCHAR):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)&(%s.len)",*aCnt, aHostVar->mRealName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("NULL",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_INDNODE):
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_INDNODE):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)&(%s)",*aCnt, aHostVar->mRealIndName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("(SQLLEN *)&(%s.len)",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_INDNODE|GEN_HVINFO_IS_INDPOINTER):
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_INDNODE|
                          GEN_HVINFO_IS_INDPOINTER):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)(%s)",*aCnt, aHostVar->mRealIndName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("(SQLLEN *)&(%s.len)",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_INDNODE|GEN_HVINFO_IS_INDPOINTER):
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_INDNODE|GEN_HVINFO_IS_INDPOINTER):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)(%s)",*aCnt, aHostVar->mRealIndName );
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)&(%s.len)",*aCnt, aHostVar->mRealName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("NULL",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)&(%s[0].len)",*aCnt, aHostVar->mRealName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("NULL",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_INDNODE):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)&(%s)",*aCnt, aHostVar->mRealIndName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("NULL",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER|
                          GEN_HVINFO_IS_INDNODE|GEN_HVINFO_IS_INDPOINTER):
                        PRINT_LineMacro();
                        PRINT_mHostInd("(SQLLEN *)(%s)",*aCnt, aHostVar->mRealIndName );
                        PRINT_LineMacro();
                        PRINT_mVcInd  ("NULL",*aCnt);
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_mHostInd("NULL",*aCnt);
                        break;
                }

                //====================
                // 3. isarr,arrsize 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_1POINTER  |
                                        GEN_HVINFO_IS_2POINTER  |
                                        GEN_HVINFO_IS_ARRAYSIZE1|
                                        GEN_HVINFO_IS_DALLOC    |
                                        GEN_HVINFO_IS_STRTYPE) )
                {
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_STRTYPE):
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC):
                        PRINT_LineMacro();
                        PRINT_isarr  ("1",*aCnt);
                        PRINT_LineMacro();
                        PRINT_arrsize("sizeof(%s)/sizeof(%s[0])",*aCnt,
                                      aHostVar->mRealName, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE1|GEN_HVINFO_IS_STRTYPE):
                    case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE1|GEN_HVINFO_IS_STRTYPE|
                          GEN_HVINFO_IS_DALLOC):
                        PRINT_LineMacro();
                        PRINT_isarr  ("1",*aCnt);
                        PRINT_LineMacro();
                        PRINT_arrsize("%s",*aCnt, sSymNode -> mArraySize );
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_isarr("0",*aCnt);
                        break;
                }


                //====================
                // 4. mType 코드생성.
                //====================
                PRINT_LineMacro();
                PRINT_mType("%d",*aCnt, sSymNode -> mType );

                /* TASK-7218 Handling Multiple Errors
                 * mDiagType 코드 생성 */
                if ( aStmtType == S_GetStmtDiag ||
                     aStmtType == S_GetConditionDiag )
                {
                    PRINT_LineMacro();
                    PRINT_mDiagType("%d",*aCnt, aHostVar -> mDiagType );
                }

                //====================
                // 5. isstruct 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_VARCHAR|
                                        GEN_HVINFO_IS_2POINTER) )
                {
                    case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                        PRINT_LineMacro();
                        PRINT_isstruct  ("1",*aCnt);
                        PRINT_LineMacro();
                        PRINT_structsize("sizeof(%s[0])",*aCnt, aHostVar->mRealName );
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_isstruct  ("0",*aCnt);
                        break;
                }


                //====================
                // 6. mSizeof 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_STRTYPE) )
                {
                    case (GEN_HVINFO_IS_STRTYPE):
                        PRINT_LineMacro();
                        PRINT_mSizeof("0",*aCnt);
                        break;
                    default:
                        switch( sSymNode -> mType )
                        {
                            case H_INTEGER:
                            case H_INT:
                                PRINT_LineMacro();
                                PRINT_mSizeof("sizeof(int)",*aCnt);
                                break;
                            case H_BLOBLOCATOR:
                            case H_CLOBLOCATOR:
                                PRINT_LineMacro();
                                PRINT_mSizeof("8",*aCnt);
                                break;
                            case H_LONG:
                                PRINT_LineMacro();
                                PRINT_mSizeof("sizeof(long)",*aCnt);
                                break;
                            case H_LONGLONG:
                                PRINT_LineMacro();
                                PRINT_mSizeof("sizeof(long long)",*aCnt);
                                break;
                            case H_SHORT:
                                PRINT_LineMacro();
                                PRINT_mSizeof("sizeof(short)",*aCnt);
                                break;
                            case H_FLOAT:
                                PRINT_LineMacro();
                                PRINT_mSizeof("sizeof(float)",*aCnt);
                                break;
                            case H_DOUBLE:
                                PRINT_LineMacro();
                                PRINT_mSizeof("sizeof(double)",*aCnt);
                                break;
                            default:
                                PRINT_LineMacro();
                                PRINT_mSizeof("0",*aCnt);
                                break;
                        }
                }


                //====================
                // 7. mLen 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_STRTYPE   |
                                        GEN_HVINFO_IS_1POINTER  |
                                        GEN_HVINFO_IS_2POINTER  |
                                        GEN_HVINFO_IS_ARRAYSIZE1|
                                        GEN_HVINFO_IS_ARRAYSIZE2|
                                        GEN_HVINFO_IS_DALLOC) )
                {
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mLen("%s",*aCnt, sSymNode->mArraySize);
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_ARRAYSIZE1|
                          GEN_HVINFO_IS_ARRAYSIZE2):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2|
                          GEN_HVINFO_IS_DALLOC):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2|
                          GEN_HVINFO_IS_ARRAYSIZE1):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2|
                          GEN_HVINFO_IS_ARRAYSIZE1|GEN_HVINFO_IS_DALLOC):
                        PRINT_LineMacro();
                        PRINT_mLen("%s",*aCnt, sSymNode->mArraySize2);
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_DALLOC):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_DALLOC):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_DALLOC|
                          GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mLen("MAX_CHAR_PTR",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mLen("sizeof(%s[0])",*aCnt, sSymNode->mName);
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_mLen("0",*aCnt);
                        break;
                }


                //====================
                // 8. mMaxlen 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_STRTYPE   |
                                        GEN_HVINFO_IS_VARCHAR   |
                                        GEN_HVINFO_IS_2POINTER  |
                                        GEN_HVINFO_IS_ARRAYSIZE1|
                                        GEN_HVINFO_IS_DALLOC) )
                {
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_DALLOC):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_DALLOC|
                          GEN_HVINFO_IS_ARRAYSIZE1):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_DALLOC|
                          GEN_HVINFO_IS_2POINTER):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_DALLOC|
                          GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("MAX_CHAR_PTR",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("sizeof(%s.arr)",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER|
                          GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("sizeof(%s[0].arr)",*aCnt, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC|GEN_HVINFO_IS_ARRAYSIZE1):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC|GEN_HVINFO_IS_2POINTER):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC|GEN_HVINFO_IS_2POINTER|
                          GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("MAX_CHAR_PTR",*aCnt);
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("sizeof(%s)/(%s)",*aCnt,
                                      aHostVar->mRealName, sSymNode->mArraySize );
                        break;
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("sizeof(%s)/sizeof(%s[0])",*aCnt,
                                      aHostVar->mRealName, aHostVar->mRealName );
                        break;
                    case (GEN_HVINFO_IS_STRTYPE):
                    case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_ARRAYSIZE1):
                        PRINT_LineMacro();
                        PRINT_mMaxlen("sizeof(%s)",*aCnt, aHostVar->mRealName );
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_mMaxlen("0",*aCnt);
                        break;
                }


                //====================
                // 9. mUnsign 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_SIGNED) )
                {
                    case (GEN_HVINFO_IS_SIGNED):
                        PRINT_LineMacro();
                        PRINT_mUnsign("(short) 0",*aCnt);
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_mUnsign("(short) 1",*aCnt);
                        break;
                }


                //====================
                // 10. mInOut 코드생성.
                //====================
                switch ( aHostVar -> mInOutType )
                {
                    case HV_IN_TYPE:
                        PRINT_LineMacro();
                        PRINT_mInOut("(short) 0",*aCnt);
                        break;
                    case HV_OUT_TYPE:
                        PRINT_LineMacro();
                        PRINT_mInOut("(short) 1",*aCnt);
                        break;
                    case HV_INOUT_TYPE:
                        PRINT_LineMacro();
                        PRINT_mInOut("(short) 2",*aCnt);
                        break;
                    case HV_OUT_PSM_TYPE:
                        PRINT_LineMacro();
                        PRINT_mInOut("(short) 3",*aCnt);
                        break;
                    default:
                        break;
                }


                //====================
                // 11. mIsDynAlloc 코드생성.
                //====================
                switch ( GEN_SUBSET_BIT(sHostValInfo,
                                        GEN_HVINFO_IS_DALLOC) )
                {
                    case (GEN_HVINFO_IS_DALLOC):
                        PRINT_LineMacro();
                        PRINT_mIsDynAlloc("(short) 1",*aCnt);
                        break;
                    default:
                        PRINT_LineMacro();
                        PRINT_mIsDynAlloc("(short) 0",*aCnt);
                        break;
                }


                //====================
                // 12. mMoreInfo 코드생성.
                //====================
                if( GEN_VERIFY_BIT(sHostValInfo, GEN_HVINFO_IS_MOREINFO) )
                {
                    PRINT_LineMacro();
                    PRINT_mMoreInfo("1",*aCnt);
                }


                //====================
                // 13. mFileopt 코드생성.
                //====================
                if( GEN_VERIFY_BIT(sHostValInfo, GEN_HVINFO_IS_LOB) )
                {
                    PRINT_LineMacro();
                    PRINT_mFileopt("(SQLUINTEGER *)&(%s)",*aCnt,
                                   aHostVar->mRealFileOptName );
                }

                /* 파일에 쓴다. */
                WRITEtoFile(sTmpStr,sSLength);
            }
            else
            {
                sFieldSymTNode = sSymNode->mStructLink->mChild->mInOrderList;
                if( GEN_VERIFY_BIT(sHostValInfo, GEN_HVINFO_IS_INDSTRUCT) )
                {
                    sIndFieldSymTNode = sIndSymNode->mStructLink->mChild->mInOrderList;
                    if( GEN_VERIFY_BIT(sHostValInfo,GEN_HVINFO_IS_INDPOINTER) )
                    {
                        idlOS::snprintf(sINDFieldOpt, GEN_FIELD_OPT_LEN, "->");
                    }
                    else
                    {
                        idlOS::snprintf(sINDFieldOpt, GEN_FIELD_OPT_LEN, ".");
                    }
                }
                else
                {
                    sIndFieldSymTNode = NULL;
                }

                if( GEN_VERIFY_BIT(sHostValInfo,GEN_HVINFO_IS_1POINTER) ||
                    GEN_VERIFY_BIT(sHostValInfo,GEN_HVINFO_IS_2POINTER) )
                {
                    idlOS::snprintf(sHVFieldOpt, GEN_FIELD_OPT_LEN, "->");
                }
                else
                {
                    idlOS::snprintf(sHVFieldOpt, GEN_FIELD_OPT_LEN, ".");
                }

                sIsField = ID_TRUE;

                // structure일경우
                // struct field들을 순회하며 코드생성.
                for( sI = 0
                     ; sI < sSymNode->mStructLink->mChild->mCnt
                     ; sI++,
                      (*aCnt)++,
                       sFieldSymTNode = sFieldSymTNode->mInOrderNext
                   )
                {
                    if ( sFieldSymTNode == NULL )
                    {
                        break;
                    }
                    sFieldNode = &(sFieldSymTNode->mElement);

                    /* BUG-26381 [valgrind bug] */
                    if ( sIndFieldSymTNode != NULL )
                    {
                        sIndFieldNode = &(sIndFieldSymTNode->mElement);
                    }
                    else
                    {
                        sIndFieldNode = NULL;
                    }

                    // get field info.
                    ulpGenGetHostValInfo( sIsField, sFieldNode, sIndFieldNode, &sHostValInfo4Field,
                                          sBraceCnt4HV, sBraceCnt4Ind );

                    //====================
                    // 1. mHostVar 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_VARCHAR |
                                            GEN_HVINFO_IS_1POINTER|
                                            GEN_HVINFO_IS_2POINTER) )
                    {

                        case (GEN_HVINFO_IS_1POINTER):
                        case (GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mHostVar( "(void*)(%s%s%s)",*aCnt,
                                            aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            break;
                        case (GEN_HVINFO_IS_VARCHAR):
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER):
                            PRINT_LineMacro();
                            PRINT_mHostVar( "(void*)%s%s%s.arr",*aCnt,
                                            aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName  );
                            break;
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mHostVar( "(void*)%s%s%s[0].arr",*aCnt,
                                            aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName  );
                            break;
                        default:
                            PRINT_LineMacro();
                            PRINT_mHostVar( "(void*)&(%s%s%s)",*aCnt,
                                            aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            break;
                    }


                    //====================
                    // 2. mHostInd, mVcInd 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_VARCHAR |
                                            GEN_HVINFO_IS_1POINTER|
                                            GEN_HVINFO_IS_2POINTER|
                                            GEN_HVINFO_IS_INDNODE) )
                    {
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mHostInd( "(SQLLEN *)&(%s%s%s[0].len)",*aCnt,
                                            aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            PRINT_LineMacro();
                            PRINT_mVcInd( "NULL",*aCnt );
                            break;
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_INDNODE):
                            PRINT_LineMacro();
                            PRINT_mHostInd( "(SQLLEN *)&(%s%s%s)",*aCnt,
                                            aHostVar->mRealIndName, sINDFieldOpt, sIndFieldNode->mName );
                            PRINT_LineMacro();
                            PRINT_mVcInd( "NULL",*aCnt );
                            break;
                        case (GEN_HVINFO_IS_VARCHAR):
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER):
                            PRINT_LineMacro();
                            PRINT_mHostInd( "(SQLLEN *)&(%s%s%s.len)",*aCnt,
                                            aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            PRINT_LineMacro();
                            PRINT_mVcInd( "NULL",*aCnt );
                            break;
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_INDNODE):
                        case (GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_INDNODE):
                            PRINT_LineMacro();
                            PRINT_mHostInd( "(SQLLEN *)&(%s%s%s)",*aCnt,
                                            aHostVar->mRealIndName, sINDFieldOpt, sIndFieldNode->mName );
                            PRINT_LineMacro();
                            PRINT_mVcInd( "(SQLLEN *)&(%s%s%s.len)",*aCnt,
                                          aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            break;
                        case (GEN_HVINFO_IS_INDNODE):
                        case (GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_INDNODE):
                        case (GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_INDNODE):
                            PRINT_LineMacro();
                            PRINT_mHostInd( "(SQLLEN *)&(%s%s%s)",*aCnt,
                                            aHostVar->mRealIndName, sINDFieldOpt, sIndFieldNode->mName );
                            break;
                        default:
                            PRINT_LineMacro();
                            PRINT_mHostInd( "NULL",*aCnt );
                            break;
                    }


                    //====================
                    // 3. isstruct,structsize 코드생성.
                    //====================
                    if ( ((ulpGenHostVarList *)(&(mEmSQLInfo.mHostVar))->mNext->mObj == aHostVar) &&
                         (mEmSQLInfo.mHostValueType != GEN_ARRAY ) )
                    {
                        PRINT_LineMacro();
                        PRINT_isstruct( "1",*aCnt );
                        if( GEN_VERIFY_BIT(sHostValInfo,GEN_HVINFO_IS_1POINTER) ||
                            GEN_VERIFY_BIT(sHostValInfo,GEN_HVINFO_IS_2POINTER) )
                        {
                            PRINT_LineMacro();
                            PRINT_structsize( "sizeof(%s[0])",*aCnt, aHostVar->mRealName );
                        }
                        else
                        {
                            PRINT_LineMacro();
                            PRINT_structsize( "sizeof(%s)",*aCnt, aHostVar->mRealName );
                        }
                    }
                    else
                    {   // struct 변수가 여러개 온다면 isstruct는 0이다.
                        PRINT_LineMacro();
                        PRINT_isstruct( "0",*aCnt );
                    }


                    //====================
                    // 4. isarr, arrsize 코드생성.
                    //====================
                    if( GEN_VERIFY_BIT(sHostValInfo,GEN_HVINFO_IS_ARRAY) )
                    {
                        PRINT_LineMacro();
                        PRINT_isarr  ("1",*aCnt);
                        PRINT_LineMacro();
                        PRINT_arrsize("%s",*aCnt, sSymNode->mArraySize );
                    }
                    else
                    {
                        // structure 이지만 모든 field들이 array일 경우.
                        if ( mEmSQLInfo.mHostValueType == GEN_ARRAY )
                        {
                            switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                                    GEN_HVINFO_IS_2POINTER  |
                                                    GEN_HVINFO_IS_ARRAYSIZE1|
                                                    GEN_HVINFO_IS_STRTYPE) )
                            {
                                case (GEN_HVINFO_IS_STRTYPE):
                                case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_ARRAYSIZE1):
                                    PRINT_LineMacro();
                                    PRINT_isarr("0",*aCnt);
                                    break;
                                case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER):
                                    PRINT_LineMacro();
                                    PRINT_isarr  ("1",*aCnt);
                                    PRINT_LineMacro();
                                    PRINT_arrsize("sizeof(%s.%s)/sizeof(%s.%s[0])",*aCnt,
                                                  aHostVar->mRealName, sFieldNode->mName,
                                                  aHostVar->mRealName, sFieldNode->mName );
                                    break;
                                default:
                                    PRINT_LineMacro();
                                    PRINT_isarr  ( "1",*aCnt );
                                    PRINT_LineMacro();
                                    PRINT_arrsize( "%s",*aCnt, sFieldNode->mArraySize );
                                    break;
                            }
                        }
                        else
                        {
                            PRINT_LineMacro();
                            PRINT_isarr("0",*aCnt);
                        }
                    }


                    //====================
                    // 5. mType 코드생성.
                    //====================
                    PRINT_LineMacro();
                    PRINT_mType( "%d",*aCnt, sFieldNode -> mType );


                    //====================
                    // 6. mSizeof 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_STRTYPE) )
                    {
                        case (GEN_HVINFO_IS_STRTYPE):
                            PRINT_LineMacro();
                            PRINT_mSizeof("0",*aCnt);
                            break;
                        default:
                            switch( sFieldNode -> mType )
                            {
                                case H_INTEGER:
                                case H_INT:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("sizeof(int)",*aCnt);
                                    break;
                                case H_BLOBLOCATOR:
                                case H_CLOBLOCATOR:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("8",*aCnt);
                                    break;
                                case H_LONG:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("sizeof(long)",*aCnt);
                                    break;
                                case H_LONGLONG:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("sizeof(long long)",*aCnt);
                                    break;
                                case H_SHORT:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("sizeof(short)",*aCnt);
                                    break;
                                case H_FLOAT:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("sizeof(float)",*aCnt);
                                    break;
                                case H_DOUBLE:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("sizeof(double)",*aCnt);
                                    break;
                                default:
                                    PRINT_LineMacro();
                                    PRINT_mSizeof("0",*aCnt);
                                    break;
                            }
                    }


                    //====================
                    // 7. mLen 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_STRTYPE   |
                                            GEN_HVINFO_IS_1POINTER  |
                                            GEN_HVINFO_IS_2POINTER  |
                                            GEN_HVINFO_IS_ARRAYSIZE1|
                                            GEN_HVINFO_IS_ARRAYSIZE2|
                                            GEN_HVINFO_IS_DALLOC) )
                    {
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_ARRAYSIZE1):
                            PRINT_LineMacro();
                            PRINT_mLen("%s",*aCnt, sFieldNode->mArraySize );
                            break;
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_ARRAYSIZE1|
                              GEN_HVINFO_IS_ARRAYSIZE2):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2|
                              GEN_HVINFO_IS_DALLOC):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2|
                              GEN_HVINFO_IS_ARRAYSIZE1):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_ARRAYSIZE2|
                              GEN_HVINFO_IS_ARRAYSIZE1|GEN_HVINFO_IS_DALLOC):
                            PRINT_LineMacro();
                            PRINT_mLen("%s",*aCnt, sFieldNode->mArraySize2 );
                            break;
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_1POINTER|GEN_HVINFO_IS_DALLOC):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_DALLOC):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER|GEN_HVINFO_IS_DALLOC|
                            GEN_HVINFO_IS_ARRAYSIZE1):
                            PRINT_LineMacro();
                            PRINT_mLen("MAX_CHAR_PTR",*aCnt);
                            break;
                        default:
                            PRINT_LineMacro();
                            PRINT_mLen("0",*aCnt);
                            break;
                    }


                    //====================
                    // 8. mMaxlen 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_STRTYPE   |
                                            GEN_HVINFO_IS_VARCHAR   |
                                            GEN_HVINFO_IS_2POINTER  |
                                            GEN_HVINFO_IS_DALLOC) )
                    {
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_DALLOC):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_DALLOC|
                              GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mMaxlen("MAX_CHAR_PTR",*aCnt);
                            break;
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR|GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mMaxlen("sizeof(%s%s%s[0].arr)",*aCnt,
                                          aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            break;
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_VARCHAR):
                            PRINT_LineMacro();
                            PRINT_mMaxlen("sizeof(%s%s%s.arr)",*aCnt,
                                          aHostVar->mRealName, sHVFieldOpt, sFieldNode->mName );
                            break;
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC):
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_DALLOC|GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mMaxlen("MAX_CHAR_PTR",*aCnt);
                            break;
                        case (GEN_HVINFO_IS_STRTYPE|GEN_HVINFO_IS_2POINTER):
                            PRINT_LineMacro();
                            PRINT_mMaxlen("sizeof(%s%s%s)/(%s)",*aCnt,
                                          aHostVar->mRealName,sHVFieldOpt,
                                          sFieldNode->mName,sFieldNode->mArraySize );
                            break;
                        case (GEN_HVINFO_IS_STRTYPE):
                            PRINT_LineMacro();
                            PRINT_mMaxlen("sizeof(%s%s%s)",*aCnt,
                                          aHostVar->mRealName,sHVFieldOpt,sFieldNode->mName );
                            break;
                        default:
                            PRINT_LineMacro();
                            PRINT_mMaxlen("0",*aCnt);
                            break;
                    }


                    //====================
                    // 9. mUnsign 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_SIGNED) )
                    {
                        case (GEN_HVINFO_IS_SIGNED):
                            PRINT_LineMacro();
                            PRINT_mUnsign("(short) 0",*aCnt);
                            break;
                        default:
                            PRINT_LineMacro();
                            PRINT_mUnsign("(short) 1",*aCnt);
                            break;
                    }


                    //====================
                    // 10. mInOut 코드생성.
                    //====================
                    switch ( aHostVar -> mInOutType )
                    {
                        case HV_IN_TYPE:
                            PRINT_LineMacro();
                            PRINT_mInOut("(short) 0",*aCnt);
                            break;
                        case HV_OUT_TYPE:
                            PRINT_LineMacro();
                            PRINT_mInOut("(short) 1",*aCnt);
                            break;
                        case HV_INOUT_TYPE:
                            PRINT_LineMacro();
                            PRINT_mInOut("(short) 2",*aCnt);
                            break;
                        default:
                            break;
                    }


                    //====================
                    // 11. mIsDynAlloc 코드생성.
                    //====================
                    switch ( GEN_SUBSET_BIT(sHostValInfo4Field,
                                            GEN_HVINFO_IS_DALLOC) )
                    {
                        case (GEN_HVINFO_IS_DALLOC):
                            PRINT_LineMacro();
                            PRINT_mIsDynAlloc("(short) 1",*aCnt);
                            break;
                        default:
                            PRINT_LineMacro();
                            PRINT_mIsDynAlloc("(short) 0",*aCnt);
                            break;
                    }


                    //====================
                    // 12. mMoreInfo 코드생성.
                    //====================
                    if( GEN_VERIFY_BIT(sHostValInfo4Field, GEN_HVINFO_IS_MOREINFO) )
                    {
                        PRINT_LineMacro();
                        PRINT_mMoreInfo("1",*aCnt);
                    }


                    //====================
                    // 13. mFileopt 코드생성.
                    //====================
                    if( GEN_VERIFY_BIT(sHostValInfo, GEN_HVINFO_IS_LOB) )
                    {
                        PRINT_LineMacro();
                        PRINT_mFileopt("(SQLUINTEGER *)&(%s)",*aCnt, aHostVar->mRealFileOptName );
                    }

                    /* BUG-26381 [valgrind bug] */
                    if ( sIndFieldSymTNode != NULL )
                    {   // 다음 field node로 이동.
                        sIndFieldSymTNode = sIndFieldSymTNode->mInOrderNext;
                    }

                    /* 파일에 쓴다. */
                    WRITEtoFile(sTmpStr,sSLength);
                }
                *aCnt -= 1;
            }
            /* 파일에 쓴다. */
            WRITEtoFile(sTmpStr,sSLength);
            break;
    }
}


/*
 * DESCRIPTION :
 *
 *  mEmSQLInfo->mHostVar에 host변수 정보를 갖는 ulpSymbolNode 를 추가한다.
 */
IDE_RC ulpCodeGen::ulpGenAddHostVarList( SChar          *aRealName,
                                         ulpSymTElement *aNode,
                                         SChar          *aIndName,
                                         ulpSymTElement *aIndNode,
                                         SChar          *aFileOptName,
                                         ulpHVarType     aIOType )
{
    iduListNode        *sListNode    = NULL;
    ulpGenHostVarList  *sHostVarList = NULL;

    sListNode = (iduListNode*)idlOS::malloc(ID_SIZEOF(iduListNode));
    IDE_TEST_RAISE( sListNode == NULL, ERR_MEMORY_ALLOC );

    sHostVarList = (ulpGenHostVarList *) idlOS::malloc( ID_SIZEOF(ulpGenHostVarList) );
    IDE_TEST_RAISE( sHostVarList == NULL, ERR_MEMORY_ALLOC );

    if ( aRealName[0] == ':' )
    {
        idlOS::snprintf( sHostVarList -> mRealName,
                         MAX_HOSTVAR_NAME_SIZE * 2,
                         "%s", aRealName+1 );
    }
    else
    {
        idlOS::snprintf( sHostVarList -> mRealName,
                         MAX_HOSTVAR_NAME_SIZE * 2,
                         "%s", aRealName );
    }

    if ( aIndNode != NULL )
    {
        idlOS::snprintf( sHostVarList -> mRealIndName,
                         MAX_HOSTVAR_NAME_SIZE * 2,
                         "%s", aIndName);
    }

    if ( aNode != NULL )
    {
        sHostVarList -> mValue     = aNode;
        sHostVarList -> mDiagType  = aNode -> mDiagType; /* TASK-7218 */
    }
    else
    {
        sHostVarList -> mValue     = NULL;
        sHostVarList -> mDiagType  = H_DIAG_UNKNOWN;
    }

    if ( aFileOptName != NULL )
    {
        idlOS::snprintf( sHostVarList -> mRealFileOptName,
                         MAX_HOSTVAR_NAME_SIZE * 2,
                         "%s", aFileOptName);
    }
    else
    {
        sHostVarList -> mRealFileOptName[0] = '\0';
    }

    sHostVarList -> mInd       = aIndNode;
    sHostVarList -> mInOutType = aIOType;

    IDU_LIST_INIT_OBJ(sListNode, sHostVarList);
    IDU_LIST_ADD_LAST(&(mEmSQLInfo.mHostVar), sListNode);

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_MEMORY_ALLOC);
    {
        if (sListNode != NULL)
        {    
            idlOS::free(sListNode);
        }

        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * DESCRIPTION :
 *
 *  mWriteBuf의 mWriteBufOffset까지의 data를 file에 쓴다.
 */
IDE_RC ulpCodeGen::ulpGenWriteFile( )
{
    /* BUG-33025 Predefined types should be able to be set by user in APRE */
    if ( (mIsPpFlag == 0) && (gUlpProgOption.mOptParseInfo == PARSE_FULL) )
    {
        idlOS::fprintf(mOutFilePtr, "#include <%s>\n", PREDEFINE_HEADER);
        mIsPpFlag = 1;
    }

    if ( mWriteBufOffset > 0)
    {
        IDE_TEST_RAISE( idlOS::fwrite( mWriteBuf,
                                       mWriteBufOffset,
                                       1,
                                       mOutFilePtr ) != (UInt)1,
                        ERR_FILE_WRITE );
    }
    /* offset 초기화. */
    mWriteBufOffset = 0;

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_FILE_WRITE);
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_WRITE_ERROR,
                         mOutFileName, errno );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * DESCRIPTION :
 *
 *  쓰고자하는 파일을 Open 한다.
 */
IDE_RC ulpCodeGen::ulpGenOpenFile( SChar *aFileName)
{
    mOutFilePtr = idlOS::fopen( aFileName, "w+" );
    IDE_TEST_RAISE( mOutFilePtr == NULL, ERR_FILE_OPEN );

    snprintf(mOutFileName, ID_SIZEOF(mOutFileName), "%s", aFileName);

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_FILE_OPEN);
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_OPEN_ERROR,
                         aFileName, errno );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * DESCRIPTION :
 *
 *  처리중인 파일을 Close 한다.
 */
IDE_RC ulpCodeGen::ulpGenCloseFile()
{
    if ( mOutFilePtr != NULL )
    {
        IDE_TEST_RAISE( idlOS::fclose( mOutFilePtr ) != 0,
                        ERR_FILE_CLOSE );
        mOutFilePtr = NULL;
        idlOS::memset( mOutFileName, 0, ID_SIZEOF(mOutFileName) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_FILE_CLOSE);
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_CLOSE_ERROR,
                         mOutFileName, errno );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * DESCRIPTION :
 *
 *  처음 ulpInitialize 가 호출 되었던 것처럼 객체가 초기화 된다.
 *  ( GEN_INIT_QUERYBUF_SIZE를 32k로 다시 초기화 하지는 않음. )
 */
void ulpCodeGen::ulpGenClearAll()
{
    idlOS::memset( mQueryBuf, 0, mQueryBufSize * ID_SIZEOF(SChar) );
    idlOS::memset( mWriteBuf, 0, GEN_WRITE_BUF_SIZE * ID_SIZEOF(SChar) );
    mWriteBufOffset = 0;
    mQueryBufOffset = 0;

    if ( mOutFilePtr != NULL )
    {
        if ( ulpGenCloseFile() == IDE_FAILURE )
        {
            IDE_ASSERT(0);
        }
    }

    /* BUG-42357 */
    mCurFileInfoIdx = -1;
}


/* 특정 charater를 buffer에 쓴다. */
void ulpCodeGen::ulpGenPutChar ( SChar aCh )
{
    UInt sEmptyLen;

    sEmptyLen = GEN_WRITE_BUF_SIZE - mWriteBufOffset;

    // 적어도 char하나는 들어갈수 있어야 되잔니?
    if( sEmptyLen > 1 )
    {
        mWriteBuf[ mWriteBufOffset++ ] = aCh;
    }
    else
    {
        ulpGenWriteFile();
        mWriteBuf[ mWriteBufOffset++ ] = aCh;
    }
}


/* mWriteBufOffset를  감소시킨다. */
void ulpCodeGen::ulpGenUnputChar ( void )
{
    mWriteBufOffset--;
}


/* Query buffer를 초기화해줌 */
void ulpCodeGen::ulpGenInitQBuff( void )
{
    if ( (mQueryBuf != NULL) &&
         (mQueryBufSize != 0) )
    {
        mQueryBufOffset = 0;
        mQueryBuf[0] = '\0';
    }
}


void ulpCodeGen::ulpTransEmQuery ( SChar *aQueryBuf )
{
/***********************************************************************
 *
 * Description :
 *    내장 구문을 cli가 수행할수 있도록 내장sql구문을 변경하여 write buffer에 써준다.
 *
 *    BUGBUG - 가능하다면 추후에 이복잡한 함수를 없애고 ulpComly.y에서
 *             파싱중 변환하도록 하는게 좋을듯...
 *
 * Implementation :
 *
 ***********************************************************************/
    UInt   sIndex;
    UInt   sVarNumIndex;
    UInt   sI;
    SChar  sCh;
    idBool sIsNull;

    /* BUG-33183 The structure of funtion which transforms embedded query will be reformed in APRE */
    ulpTransEmQueryStateFlag sStateFlag = STATE_DEFAULT;

    sIndex = 0;
    sVarNumIndex = 0;
    sI = 0;
    sIsNull    = ID_FALSE;

    /* BUG-33183 The structure of funtion which transforms embedded query will be reformed in APRE */
    ulpGenString ( (SChar *)"\"" );
    for( sIndex = 0 ; sIndex < mQueryBufOffset - (aQueryBuf - mQueryBuf) - 1 ; sIndex++ )
    {
        sCh = *(aQueryBuf + sIndex);
        switch ( sStateFlag )
        {
            case STATE_DEFAULT:
                if ( sCh == ':' )
                {
                    if (*(aQueryBuf + sIndex + 1) != '=')
                    {
                        /* 
                         * Change the host variable into ?
                         * A structure used as a host variable should be changed into  
                         * the number of ?s of their member variables
                         */
                        for ( sI = 0 ; sI++ < mHostVarNumArr[sVarNumIndex] ; )
                        {
                            ulpGenString ( (SChar *)"? " );
                            if ( sI < mHostVarNumArr[sVarNumIndex] )
                            {
                                ulpGenString ( (SChar *)", " );
                            }
                        }
                        sVarNumIndex++;
                        sStateFlag = STATE_HOSTVAR;
                    }
                    else
                    {
                        ulpGenPutChar ( sCh );
                    }
                }
                else if ( sCh == '\'' )
                {
                    ulpGenPutChar ( sCh );
                    sStateFlag = STATE_QUOTED;
                }
                else if ( sCh == '\n' )
                {
                    ulpGenString ( (SChar *)"\"\n\"" );
                }
                else if ( sCh == '"' )
                {
                    ulpGenString ( (SChar *)"\\\"" );
                }
                else if ( sCh == '\0' )
                {
                    sIsNull = ID_TRUE;
                }
                else
                {
                    ulpGenPutChar ( sCh );
                }
                break;

            case STATE_HOSTVAR:
                if ( (sCh >= '0' && sCh <= '9') || (sCh >= 'A' && sCh <= 'Z') ||
                     (sCh >= 'a' && sCh <= 'z') || (sCh == '[') || (sCh == ']') ||
                     (sCh == '_') || (sCh == '.') )
                {
                    /* allowed characters for host variable name : do nothing */
                }
                else if ( sCh == ':' )
                {
                    if ( *(aQueryBuf + sIndex + 1 ) != '=')
                    {
                        /* 
                         * Change the host variable into ?
                         * A structure used as a host variable should be changed into  
                         * the number of ?s of their member variables
                         */
                        ulpGenString ( (SChar *)", " );
                        for ( sI = 0 ; sI++ < mHostVarNumArr[sVarNumIndex] ; )
                        {
                            ulpGenString ( (SChar *)"? " );
                            if ( sI < mHostVarNumArr[sVarNumIndex] )
                            {
                                ulpGenString ( (SChar *)", " );
                            }
                        }                                                      
                        sVarNumIndex++;
                    }
                }
                else if ( sCh == '-' )
                {
                    if ( *(aQueryBuf + sIndex + 1 ) != '>')
                    {
                        /* end of host variable */
                        ulpGenPutChar ( sCh );
                        sStateFlag = STATE_DEFAULT;
                    }
                    else
                    {
                        /* skip checking > */
                        sIndex++;
                    }
                }
                else
                {
                    /* end of host variable */
                    ulpGenPutChar ( sCh );
                    sStateFlag = STATE_DEFAULT;
                }
                break;

            case STATE_QUOTED:
                if ( sCh == '\'' )
                {
                    if( *(aQueryBuf + sIndex + 1) == '\'' )
                    {
                        /* if '' : escaped ' */
                        ulpGenPutChar ( sCh );
                        ulpGenPutChar ( *(aQueryBuf + sIndex + 1) );
                        sIndex++;
                    }
                    else
                    {
                        /* end of quote */
                        ulpGenPutChar ( sCh );
                        sStateFlag = STATE_DEFAULT;
                    }
                }
                /* BUG-33859 escape \ for EMSQL */
                else if (sCh == '\\')
                {
                    ulpGenPutChar ( sCh );
                    ulpGenPutChar ( sCh );
                }
                else
                {
                    ulpGenPutChar ( sCh );
                }
                break;

            default:
                break;
        }

        if ( sIsNull == ID_TRUE )
        {
            break;
        }
    }
    ulpGenString ( (SChar *)"\"" );
}

/* WHENEVER 구문 상태정보 설정 함수 */
void ulpCodeGen::ulpGenSetWhenever( SInt aDepth,
                                    ulpGENWHENEVERCOND aCond,
                                    ulpGENWHENEVERACT aAct,
                                    SChar *aText )
{
    switch ( mWhenever.mCondition )
    {
        case GEN_WHEN_NONE:
            mWhenever.mCondition = aCond;
            mWhenever.mContent[aCond].mAction = aAct;
            mWhenever.mContent[aCond].mScopeDepth = aDepth;
            if ( aText != NULL )
            {
                idlOS::snprintf( mWhenever.mContent[aCond].mText,
                                 MAX_WHENEVER_ACTION_LEN,
                                 "%s",
                                 aText );
            }
            break;
        case GEN_WHEN_NOTFOUND:
            if ( aCond == GEN_WHEN_SQLERROR )
            {
                mWhenever.mCondition = GEN_WHEN_NOTFOUND_SQLERROR;
            }
            else
            {
                // do nothing
            }
            mWhenever.mContent[aCond].mAction = aAct;
            mWhenever.mContent[aCond].mScopeDepth = aDepth;
            if ( aText != NULL )
            {
                idlOS::snprintf( mWhenever.mContent[aCond].mText,
                                 MAX_WHENEVER_ACTION_LEN,
                                 "%s",
                                 aText );
            }
            break;
        case GEN_WHEN_SQLERROR:
            if ( aCond == GEN_WHEN_NOTFOUND )
            {
                mWhenever.mCondition = GEN_WHEN_NOTFOUND_SQLERROR;
            }
            else
            {
                // do nothing
            }
            mWhenever.mContent[aCond].mAction = aAct;
            mWhenever.mContent[aCond].mScopeDepth = aDepth;
            if ( aText != NULL )
            {
                idlOS::snprintf( mWhenever.mContent[aCond].mText,
                                 MAX_WHENEVER_ACTION_LEN,
                                 "%s",
                                 aText );
            }
            break;
        case GEN_WHEN_NOTFOUND_SQLERROR:
            mWhenever.mContent[aCond].mAction = aAct;
            mWhenever.mContent[aCond].mScopeDepth = aDepth;
            if ( aText != NULL )
            {
                idlOS::snprintf( mWhenever.mContent[aCond].mText,
                                 MAX_WHENEVER_ACTION_LEN,
                                 "%s",
                                 aText );
            }
            break;
        default:
            IDE_ASSERT(0);
            break;
    }
}


void ulpCodeGen::ulpGenPrintWheneverAct( ulpGenWheneverDetail *aWheneverDetail )
{
    switch( aWheneverDetail->mAction )
    {
        case GEN_WHEN_CONT:
            ulpGenString ( (SChar *)"{ }\n" );
            break;
        case GEN_WHEN_DO_FUNC:
            ulpGenString ( aWheneverDetail->mText );
            ulpGenString ( (SChar *)";\n" );
            break;
        case GEN_WHEN_DO_BREAK:
            ulpGenString ( (SChar *)"break;\n" );
            break;
        case GEN_WHEN_DO_CONT:
            ulpGenString ( (SChar *)"continue;\n" );
            break;
        case GEN_WHEN_GOTO:
            ulpGenString ( (SChar *)"goto " );
            ulpGenString ( aWheneverDetail->mText );
            ulpGenString ( (SChar *)";\n" );
            break;
        case GEN_WHEN_STOP:
            ulpGenString ( (SChar *)"exit(0);\n" );
            break;
        default:
            IDE_ASSERT(0);
            break;
    }
}

void ulpCodeGen::ulpGenResetWhenever( SInt aDepth )
{
    switch ( mWhenever.mCondition )
    {
        case GEN_WHEN_NONE:
            break;
        case GEN_WHEN_NOTFOUND:
            if ( mWhenever.mContent[GEN_WHEN_NOTFOUND].mScopeDepth
                 > aDepth )
            {
                mWhenever.mCondition = GEN_WHEN_NONE;
            }
            break;
        case GEN_WHEN_SQLERROR:
            if ( mWhenever.mContent[GEN_WHEN_SQLERROR].mScopeDepth
                 > aDepth )
            {
                mWhenever.mCondition = GEN_WHEN_NONE;
            }
            break;
        case GEN_WHEN_NOTFOUND_SQLERROR:
            if ( mWhenever.mContent[GEN_WHEN_NOTFOUND].mScopeDepth
                 > aDepth )
            {
                mWhenever.mCondition = GEN_WHEN_SQLERROR;
            }
            if ( mWhenever.mContent[GEN_WHEN_SQLERROR].mScopeDepth
                 > aDepth )
            {
                if( mWhenever.mCondition == GEN_WHEN_SQLERROR )
                {
                    mWhenever.mCondition = GEN_WHEN_NONE;
                }
                else
                {
                    mWhenever.mCondition = GEN_WHEN_NOTFOUND;
                }
            }
            break;
        default:
            IDE_ASSERT(0);
            break;
    }
}


ulpWhenever *ulpCodeGen::ulpGenGetWhenever( void )
{
    return &mWhenever;
}


void ulpCodeGen::ulpGenAddHostVarArr( UInt aNum )
{
    /* BUG-27748 : segmentation fault */
    if( mHostVarNumOffset >= mHostVarNumSize )
    {
        //realloc * 2
        mHostVarNumArr = (UInt *)idlOS::realloc( mHostVarNumArr,
                                                 mHostVarNumSize * 2 * ID_SIZEOF(UInt)  );
        IDE_TEST_RAISE( mHostVarNumArr == NULL, ERR_MEMORY_ALLOC );
        mHostVarNumSize *= 2;
    }
    mHostVarNumArr[mHostVarNumOffset++] = aNum;

    return;

    IDE_EXCEPTION( ERR_MEMORY_ALLOC )
    {
        ulpSetErrorCode( &mErrorMgr, ulpERR_ABORT_Memory_Alloc_Error );
        ulpPrintfErrorCode( stderr, &mErrorMgr);
        ulpFinalizeError();
    }
    IDE_EXCEPTION_END;
}

void ulpCodeGen::ulpGenCutQueryTail( SChar *aToken )
{
    SChar *sPos;

    if ( mEmSQLInfo.mQueryStr != NULL )
    {
        sPos = idlOS::strstr( mEmSQLInfo.mQueryStr, aToken );
    }
    else
    {
        sPos = idlOS::strstr( mQueryBuf, aToken );
    }

    if ( sPos != NULL )
    {
        *sPos = '\0';
    }
}

void ulpCodeGen::ulpGenCutQueryTail4PSM( SChar aCh )
{
    UInt   sOffset;

    for ( sOffset = mQueryBufOffset - 1 ;
          ( mQueryBuf[ sOffset ] != aCh ) && ( sOffset > 0 ) ;
          sOffset-- )
    {
        // do nothing
    }

    if( sOffset != 0 )
    {
        mQueryBuf[sOffset] = '\0';
    }
}

/* BUG-46824  
 * 문자열의 끝에서 지정된 문자가 발견될때까지 검색하고 검색전까지의 데이터는 잘라낸다.
 */
void ulpCodeGen::ulpGenCutStringTail4PSM( SChar *aBuf, SChar aCh )
{
    UInt sOffset = 0;

    for (sOffset=idlOS::strlen(aBuf); sOffset > 0; sOffset--)
    {
        if (aBuf[sOffset] == aCh)
        {
            break;
        }
    }

    if( sOffset > 0 )
    {
        aBuf[sOffset] = '\0';
    }
}

void ulpCodeGen::ulpGenRemoveQueryToken( SChar *aToken )
{
    SInt   sLen;
    SInt   sTokOffset;
    SInt   sPartOffset;
    SInt   sOffset;
    idBool sIsMatch;

    sIsMatch = ID_FALSE;
    sLen = idlOS::strlen(aToken);

    for ( sOffset = (SInt) mQueryBufOffset ; sOffset - sLen >= 0 ; sOffset-- )
    {
        for ( sTokOffset = sLen -1, sPartOffset = sOffset - 1 ;
              ( (sTokOffset >= 0) && (sPartOffset >= 0) ) &&
              (aToken[sTokOffset] == mQueryBuf[sPartOffset] ) ;
              sTokOffset--, sPartOffset--
            )
        {
            if ( sTokOffset == 0 )
            {
                /* BUG-28314 : 내장sql구문(select)을 잘못 변환하는 경우발생.  */
                // 퀴리 버퍼의 뒤에서부터 검색해가며 aToken과 같은 string을 찾아
                // 지워준다.
                sIsMatch = ID_TRUE;
                break;
            }
        }

        if( sIsMatch == ID_TRUE )
        {
            for( sTokOffset = sLen - 1 ; sTokOffset >= 0 ; sTokOffset-- )
            {
                mQueryBuf[ sOffset - sTokOffset - 1 ] = ' ';
            }
            break;
        }
    }
}


ulpGenEmSQLInfo *ulpCodeGen::ulpGenGetEmSQLInfo( void )
{
    return &mEmSQLInfo;
}


// write some code at the beginning of the .cpp file.
void ulpCodeGen::ulpGenInitPrint( void )
{
    const SChar *sStr1;
    const SChar *sStr2;
    const SChar *sStr3;
    SChar sTmpStr[ MAX_HOSTVAR_NAME_SIZE ];

    sStr1 =
            "/*********************************************\n"
            " *           Code generated by the           *\n";

    idlOS::snprintf( sTmpStr, MAX_HOSTVAR_NAME_SIZE,
                     " * %s(%s)*\n *********************************************/\n",
                     gUlpProgOption.mVersion,
                     iduVersionString );

    sStr2 = "\n#include <stdio.h>"
            "\n#include <string.h>"
            "\n#include <stdlib.h>\n";
    
    /* BUG-41471 Maintain one APRE source code for HDB and HDB-DA */
    sStr3 = INCLUDE_ULPLIBINTERFACE_STR;
    
    if( gUlpProgOption.mOptAtc != ID_TRUE )
    {
        ulpGenString( ( SChar * ) sStr1 );
        ulpGenString( ( SChar * ) sTmpStr );
        ulpGenString( ( SChar * ) sStr2 );
    }
    else
    {
        ulpGenString( ( SChar * ) sStr2 );
        ulpGenString( ( SChar * ) "#include <atc4ses.h>\n" );
    }

    if( gUlpProgOption.mOptAlign != ID_TRUE )
    {
        ulpGenString( ( SChar * ) sStr3 );
    }
    else
    {
        ulpGenString( ( SChar * ) "#pragma options align=power\n" );
        ulpGenString( ( SChar * ) sStr3 );
        ulpGenString( ( SChar * ) "#pragma options align=reset\n" );
    }

    /* BUG-23758 */
    ulpGenString( ( SChar *) "\n/* The variable _esqlopts specifies command-line options\n"
                             " * {-n, -unsafe_null, stmt caching, ...}\n"
                             " */" );

    // command line option setting
    idlOS::snprintf( sTmpStr, GEN_EMSQL_INFO_SIZE,
                     "\nstatic short _esqlopts[10] = {%d,%d,%d,%d,%d,%d,%d,%d,%d,%d};\n\n",
                     gUlpProgOption.mOptNotNullPad,
                     gUlpProgOption.mOptUnsafeNull,
                     1, // stmt cache info.
                     0,0,0,0,0,0,0 );
    ulpGenString( ( SChar * ) sTmpStr );

    if( gUlpProgOption.mOptMt == ID_TRUE )
    {
        mEmSQLInfo.mIsMT = ID_TRUE;
    }
}


/* BUG-29479 : double 배열 사용시 precompile 잘못되는 경우발생함. */
SShort ulpCodeGen::ulpGenBraceCnt4HV( SChar *aValueName, SInt aLen )
{
/***********************************************************************
 *
 * Description :
 *    호스트변수 이름을 인자로 받아 이름 맨뒤에 array index를 지정하는 문법인
 *   [...] 가 몇번 반복되는지 count해주는 함수.
 *   단, valuename[] 와같이 [...]안이 비어있을 경우 count하지 않는다.
 *
 *   사실, ulpCompl.l 에서 host 변수의 expr. 은 이중 [..][..] 을 지원하지 않는다.
 *   따라서 현제로서는 sBraceCnt값이 1을 넘지 못한다.
 *
 ***********************************************************************/
    SShort   sIsInBrace;    // nested []의 depth.
    SShort   sBraceCnt;     // 총 []의 수.
    idBool   sBreak;
    idBool   sIncBraceCnt;

    sBraceCnt    = 0;
    sIsInBrace   = 0;
    sBreak       = ID_FALSE;
    sIncBraceCnt = ID_FALSE;

    if( aLen > 0 )
    {
        // 변수 이름의 뒤에서 부터 parsing 한다.
        for( ; aLen > 0 ; aLen-- )
        {
            switch( aValueName[aLen-1] )
            {
                case ']':
                    sIsInBrace++;
                    break;
                case '[':
                    sIsInBrace--;
                    if ( (sIsInBrace   == 0) &&
                         (sIncBraceCnt == ID_TRUE) )
                    {
                        sBraceCnt++;
                        sIncBraceCnt = ID_FALSE;
                    }
                    break;
                case ' ':
                case '\t':
                    // [] 안에 공백만있으면 []는 아무런 의미없다.
                    break;
                default:
                    if (sIsInBrace==0)
                    {
                        sBreak = ID_TRUE;
                    }
                    else
                    {
                        sIncBraceCnt = ID_TRUE;
                    }
                    break;
            }

            if( sBreak == ID_TRUE )
            {
                break;
            }
        }
    }

    return sBraceCnt;
}


void ulpCodeGen::ulpGenDebugPrint( ulpSymTElement *aSymNode )
{
/***********************************************************************
 *
 * Description :
 *    debugging 용도의 호스트변수 정보 print해주는 함수.
 *
 ***********************************************************************/
    /*
    typedef struct ulpSymTElement
    {
        SChar            mName[MAX_HOSTVAR_NAME_SIZE];   // var name
        ulpHostType      mType;                          // variable type
        idBool           mIsTypedef;                     // typdef로 정의된 type 이름이냐?
        idBool           mIsarray;
        SChar            mArraySize[MAX_NUMBER_LEN];
        SChar            mArraySize2[MAX_NUMBER_LEN];
        idBool           mIsstruct;
        SChar            mStructName[MAX_HOSTVAR_NAME_SIZE];  // struct tag name
        ulpStructTNode  *mStructLink;             // struct type일경우 link
        idBool           mIssign;                 // unsigned or signed
        SShort           mPointer;
        idBool           mAlloc;                  // application에서 직접 alloc했는지 여부.
        UInt             mMoreInfo;               // Some additional infomation.
        idBool           mIsExtern;               // is extern variable?
    } ulpSymTElement;
    */

    /* BUG-35242 ALTI-PCM-002 Coding Convention Violation in UL module */
    idlOS::printf( "==== Print symbol info. ====\n" );
    idlOS::printf( "1. mName       = '%s'\n", aSymNode->mName );
    idlOS::printf( "2. mType       = '%d'\n", aSymNode->mType );
    idlOS::printf( "3. mIsTypedef  = '%s'\n", ( aSymNode->mIsTypedef == ID_TRUE ) ? (SChar*)"true" : (SChar*)"false" );
    idlOS::printf( "4. mIsarray    = '%s'\n", ( aSymNode->mIsarray == ID_TRUE ) ? (SChar*)"true" : (SChar*)"false" );
    idlOS::printf( "5. mArraySize  = '%s'\n", aSymNode->mArraySize );
    idlOS::printf( "6. mArraySize2 = '%s'\n", aSymNode->mArraySize2 );
    idlOS::printf( "7. mIsstruct   = '%s'\n", ( aSymNode->mIsstruct == ID_TRUE ) ? (SChar*)"true" : (SChar*)"false" );
    idlOS::printf( "8. mStructName = '%s'\n", aSymNode->mStructName );
    idlOS::printf( "9. mIssign     = '%s'\n", ( aSymNode->mIssign == ID_TRUE ) ? (SChar*)"true" : (SChar*)"false" );
    idlOS::printf( "10.mPointer    = '%d'\n", aSymNode->mPointer );
    idlOS::printf( "11.mAlloc      = '%s'\n", ( aSymNode->mAlloc == ID_TRUE ) ? (SChar*)"true" : (SChar*)"false" );
    idlOS::printf( "12.mMoreInfo   = '%d'\n", aSymNode->mMoreInfo );
    idlOS::printf( "13.mIsExtern   = '%s'\n", ( aSymNode->mIsExtern == ID_TRUE ) ? (SChar*)"true" : (SChar*)"false" );
    idlOS::printf( "=============================\n" );
}

/* BUG-33025 Predefined types should be able to be set by user in APRE */
IDE_RC ulpCodeGen::ulpGenRemovePredefine(SChar *aFileName)
{
    SInt sI;
    SChar sTempBuffer[100];
    SChar *sPredefinedString = (SChar*)"#include <"PREDEFINE_HEADER">";

    SInt sSizeofString = idlOS::strlen(sPredefinedString);
    mOutFilePtr = idlOS::fopen( aFileName, "r+" );
    IDE_TEST_RAISE( mOutFilePtr == NULL, ERR_FILE_OPEN );

    idlOS::snprintf(mOutFileName, ID_SIZEOF(mOutFileName), "%s", aFileName);

    while ( idlOS::fgets(sTempBuffer, 100, mOutFilePtr) != 0 )
    {
        if (idlOS::strncmp( sTempBuffer, sPredefinedString, sSizeofString) == 0)
        {
            idlOS::fseek(mOutFilePtr, (sSizeofString + 1) * (-1), SEEK_CUR);
            for ( sI = 0; sI < sSizeofString; sI++ )
            {
                idlOS::fprintf(mOutFilePtr, " ");
            }
            break;
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION (ERR_FILE_OPEN);
    {
        ulpSetErrorCode( &mErrorMgr,
                         ulpERR_ABORT_FILE_OPEN_ERROR,
                         aFileName, errno );
        ulpPrintfErrorCode( stderr,
                            &mErrorMgr);
        ulpFinalizeError();
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* BUG-35518 Shared pointer should be supported in APRE
 * Print converted sentences for shared pointer */
void ulpCodeGen::ulpGenSharedPtr( ulpSymTElement *aSymNode )
{
    SChar sStr[ MAX_HOSTVAR_NAME_SIZE * 2 ];

    SChar *sType = ulpConvertFromHostType( aSymNode->mType );

    UInt  sStrLen = MAX_HOSTVAR_NAME_SIZE * 2;

    sStr[0] = '\0'; /* Initialize this array to avoid codesonar warning */

    // mAlloc  은 * 로 선언된 변수일경우 참이 되며,
    // mIsarray는 []로 선언된 변수일경우 참이된다.
    // mPointer는 몇중 포인터/배열 인지에 대한 정보를 저장함.
    if( aSymNode -> mAlloc != ID_TRUE )
    {   
        if( aSymNode->mIsarray == ID_TRUE )
        {
            /* BUG-35518 : 1-dimensional array */
            if ( (aSymNode->mArraySize2[0] == '\0') &&
                 (aSymNode->mPointer == 1) )
            {
                /* BUG-35518 1-dimensional char array is printed without conversion */
                if ( aSymNode->mType == H_CHAR )
                {
                    if ( aSymNode->mIssign == ID_TRUE )
                    {
                        idlOS::snprintf( sStr, sStrLen, "\t%s %s[%s];\n", sType, aSymNode->mName, aSymNode->mArraySize );
                    }
                    else
                    {
                        idlOS::snprintf( sStr, sStrLen, "\tunsigned %s %s[%s];\n", sType, aSymNode->mName, aSymNode->mArraySize );
                    }
                }
                else
                {
                    /* BUG-35518 First declaration */
                    if ( mSharedPtrInfo.mIsFirstSharedPtr == ID_TRUE )
                    {
                        idlOS::snprintf( mSharedPtrInfo.mPrevName, sStrLen, aSymNode->mName );
                        idlOS::snprintf( mSharedPtrInfo.mPrevSize, sStrLen, aSymNode->mArraySize);
                        mSharedPtrInfo.mPrevType = aSymNode->mType;
                        idlOS::snprintf( sStr, sStrLen, "\t%s *%s = (%s*)ulpAlign((void*)%s, 8);\n",
                                         sType, aSymNode->mName, sType, mSharedPtrInfo.mSharedPtrName );
                        mSharedPtrInfo.mIsFirstSharedPtr = ID_FALSE;
                        mSharedPtrInfo.mIsPrevArraySingle = ID_TRUE;
                    }
                    else
                    {
                        /* BUG-35518 First declaration was 1-dimensional */
                        if ( mSharedPtrInfo.mIsPrevArraySingle == ID_TRUE )
                        {
                            idlOS::snprintf( sStr, sStrLen, "\t%s *%s = (%s*)ulpAlign((void*)(((char*)%s) + sizeof(%s)*(%s)), 8);\n",
                                             sType, aSymNode->mName, sType, mSharedPtrInfo.mPrevName, ulpConvertFromHostType(mSharedPtrInfo.mPrevType), mSharedPtrInfo.mPrevSize );
                        }
                        /* BUG-35518 First declaration was 2-dimensional */
                        else
                        {
                            idlOS::snprintf( sStr, sStrLen, "\t%s *%s = (%s*)ulpAlign((void*)(((char*)%s) + sizeof(%s_ses_td_demen_my)*(%s)), 8);\n",
                                             sType, aSymNode->mName, sType, mSharedPtrInfo.mPrevName, mSharedPtrInfo.mPrevName, mSharedPtrInfo.mPrevSize );
                        }
                        idlOS::snprintf( mSharedPtrInfo.mPrevName, sStrLen, aSymNode->mName );
                        idlOS::snprintf( mSharedPtrInfo.mPrevSize, sStrLen, aSymNode->mArraySize);
                        mSharedPtrInfo.mPrevType = aSymNode->mType;
                        mSharedPtrInfo.mIsFirstSharedPtr = ID_FALSE;
                        mSharedPtrInfo.mIsPrevArraySingle = ID_TRUE;
                    }
                }
            }
            /* BUG-35518 : 2-dimensional array */
            else
            {
                /* BUG-35518 First declaration */
                if ( mSharedPtrInfo.mIsFirstSharedPtr == ID_TRUE )
                {
                    idlOS::snprintf( mSharedPtrInfo.mPrevName, sStrLen, aSymNode->mName );
                    idlOS::snprintf( mSharedPtrInfo.mPrevSize, sStrLen, aSymNode->mArraySize);
                    mSharedPtrInfo.mPrevType = aSymNode->mType;
                    idlOS::snprintf( sStr, sStrLen, "\ttypedef char %s_ses_td_demen_my[%s];\n\t%s_ses_td_demen_my *%s = (%s_ses_td_demen_my*)ulpAlign((void*)%s, 8);\n",
                                     aSymNode->mName, aSymNode->mArraySize2, aSymNode->mName, aSymNode->mName, aSymNode->mName, mSharedPtrInfo.mSharedPtrName );
                    mSharedPtrInfo.mIsFirstSharedPtr = ID_FALSE;
                    mSharedPtrInfo.mIsPrevArraySingle = ID_FALSE;
                }
                else
                {
                    /* BUG-35518 First declaration was 1-dimensional */
                    if ( mSharedPtrInfo.mIsPrevArraySingle == ID_TRUE )
                    {
                        idlOS::snprintf( sStr, sStrLen, "\ttypedef char %s_ses_td_demen_my[%s];\n\t%s_ses_td_demen_my *%s = (%s_ses_td_demen_my*)ulpAlign((void*)(((char*)%s) + sizeof(%s)*(%s)), 8);\n",
                                         aSymNode->mName, aSymNode->mArraySize2, aSymNode->mName, aSymNode->mName, aSymNode->mName, 
                                         mSharedPtrInfo.mPrevName, ulpConvertFromHostType(mSharedPtrInfo.mPrevType), mSharedPtrInfo.mPrevSize );

                        idlOS::snprintf( mSharedPtrInfo.mPrevName, sStrLen, aSymNode->mName );
                        idlOS::snprintf( mSharedPtrInfo.mPrevSize, sStrLen, aSymNode->mArraySize);
                        mSharedPtrInfo.mPrevType = aSymNode->mType;
                        mSharedPtrInfo.mIsFirstSharedPtr = ID_FALSE;
                        mSharedPtrInfo.mIsPrevArraySingle = ID_FALSE;
                    }
                    /* BUG-35518 First declaration was 2-dimensional */
                    else
                    {
                        idlOS::snprintf( sStr, sStrLen, "\ttypedef char %s_ses_td_demen_my[%s];\n\t%s_ses_td_demen_my *%s = (%s_ses_td_demen_my*)ulpAlign((void*)(((char*)%s) + sizeof(%s_ses_td_demen_my)*(%s)), 8);\n",
                                         aSymNode->mName, aSymNode->mArraySize2, aSymNode->mName, aSymNode->mName, aSymNode->mName, 
                                         mSharedPtrInfo.mPrevName, mSharedPtrInfo.mPrevName, mSharedPtrInfo.mPrevSize );

                        idlOS::snprintf( mSharedPtrInfo.mPrevName, sStrLen, aSymNode->mName );
                        idlOS::snprintf( mSharedPtrInfo.mPrevSize, sStrLen, aSymNode->mArraySize);
                        mSharedPtrInfo.mPrevType = aSymNode->mType;
                        mSharedPtrInfo.mIsFirstSharedPtr = ID_FALSE;
                        mSharedPtrInfo.mIsPrevArraySingle = ID_FALSE;
                    }
                }
            }
        }
        /* BUG-35518 Declaration of other types is printed without conversion */
        else
        {
            if ( aSymNode->mIssign == ID_TRUE )
            {
                idlOS::snprintf( sStr, sStrLen, "\t%s %s;\n", sType, aSymNode->mName );
            }
            else
            {
                idlOS::snprintf( sStr, sStrLen, "\tunsigned %s %s;\n", sType, aSymNode->mName );
            }
        }
    }

    if( sStr != NULL )
        ulpGenNString ( sStr, idlOS::strlen(sStr) );
}

/* BUG-35518 Shared pointer should be supported in APRE 
 * Convert from stored hosttype to C type
 */
SChar *ulpCodeGen::ulpConvertFromHostType( ulpHostType aHostType )
{
    SChar *sTmpStr;
    switch ( aHostType )
    {
        case H_INT:
            sTmpStr = (SChar*)"int";
            break;
        case H_LONG:
            sTmpStr = (SChar*)"long";
            break;
        case H_LONGLONG:
            sTmpStr = (SChar*)"longlong";
            break;
        case H_SHORT:
            sTmpStr = (SChar*)"short";
            break;
        case H_CHAR:
            sTmpStr = (SChar*)"char";
            break;
        case H_FLOAT:
            sTmpStr = (SChar*)"float";
            break;
        case H_DOUBLE:
            sTmpStr = (SChar*)"double";
            break;
        case H_NIBBLE:
            sTmpStr = (SChar*)"SES_NIBBLE";
            break;
        case H_BYTES:
            sTmpStr = (SChar*)"SES_BYTES";
            break;
        case H_VARBYTE:
            sTmpStr = (SChar*)"SES_VARBYTE";
            break;
        case H_BINARY:
            sTmpStr = (SChar*)"SES_BINARY";
            break;
        default:
            ulpSetErrorCode( &mErrorMgr,
                             ulpERR_ABORT_Unknown_Type_For_Shared_Pointer );
            ulpPrintfErrorCode( stderr,
                                &mErrorMgr);
            ulpFinalizeError();
            IDE_ASSERT(0);
            break;
    }
    return sTmpStr;
}

/* BUG-35518 Shared pointer should be supported in APRE */
SChar *ulpCodeGen::ulpGetSharedPtrName()
{
    return mSharedPtrInfo.mSharedPtrName;
}

/* BUG-42357 [mm-apre] The -lines option is added to apre. (INC-31008) */
void ulpCodeGen::ulpGenSetCurFileInfo( SInt   aFstLineNo,
                                       SInt   aInsLineCnt,
                                       SChar *aFileNm )
{
    mCurFileInfoIdx++;

    mCurFileInfoArr[mCurFileInfoIdx].mFirstLineNo     = aFstLineNo;
    mCurFileInfoArr[mCurFileInfoIdx].mInsertedLineCnt = aInsLineCnt;
    (void)idlOS::strncpy( mCurFileInfoArr[mCurFileInfoIdx].mFileName,
                          aFileNm,
                          idlOS::strlen( aFileNm ) + 1 );
}

void ulpCodeGen::ulpGenResetCurFileInfo()
{
    mCurFileInfoArr[mCurFileInfoIdx].mFirstLineNo     = 0;
    mCurFileInfoArr[mCurFileInfoIdx].mInsertedLineCnt = 0;
    mCurFileInfoArr[mCurFileInfoIdx].mFileName[0]     = 0x00;

    mCurFileInfoIdx--;
}

void ulpCodeGen::ulpGenAddSubHeaderFilesLineCnt()
{
    mCurFileInfoArr[mCurFileInfoIdx - 1].mInsertedLineCnt +=
        (COMPlineno - mCurFileInfoArr[mCurFileInfoIdx].mFirstLineNo + 1);
}

idBool ulpCodeGen::ulpGenIsHeaderFile()
{
    return mCurFileInfoIdx > 0 ? ID_TRUE : ID_FALSE;
}

SInt ulpCodeGen::ulpGenMakeLineMacroStr( SChar *aBuffer, UInt aBuffSize )
{
    SInt sLen = 0;
    SInt sLineNo;

    sLineNo = (COMPlineno -
               mCurFileInfoArr[mCurFileInfoIdx].mFirstLineNo -
               mCurFileInfoArr[mCurFileInfoIdx].mInsertedLineCnt);

    sLen = idlOS::snprintf( aBuffer,
                            aBuffSize,
                            "#line %" ID_INT32_FMT " \"%s\"\n",
                            sLineNo,
                            mCurFileInfoArr[mCurFileInfoIdx].mFileName );

    return sLen;
}

void ulpCodeGen::ulpGenPrintLineMacro()
{
    SChar sLineMacroStr[MAX_FILE_PATH_LEN + MAX_FILE_NAME_LEN] = { 0, };

    if (gUlpProgOption.mOptLineMacro == ID_TRUE)
    {
        (void) ulpGenMakeLineMacroStr( sLineMacroStr );
        ulpGenString( sLineMacroStr );
    }
    else
    {
        /* do nothing */
    }
}


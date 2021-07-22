/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFileStream.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ideCallback.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <ideLog.h>
#include <iduFileStream.h>

IDE_RC iduFileStream::openFile( FILE** aFp,
                                SChar* aFilePath,
                                SChar* aMode )
{
/***********************************************************************
 *
 * Description : file을 open함
 *
 * Implementation :
 *    1. fopen 함수 호출
 *    2. 반환된 fp가 null이면 errno 검사
 *    3. errno에 따라서 적절한 error 반환
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::openFile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SInt sSystemErrno = 0;
    
    *aFp = idlOS::fopen( aFilePath, aMode );
    
    if( *aFp == NULL )
    {
        sSystemErrno = errno;

#if defined(WRS_VXWORKS)
        IDE_TEST_RAISE( sSystemErrno == ENOSPC, NO_SPACE_ERROR );
#else
        IDE_TEST_RAISE( (sSystemErrno == ENOSPC) ||
                        (sSystemErrno == EDQUOT),
                        NO_SPACE_ERROR );
#endif
        IDE_TEST_RAISE( (sSystemErrno == ENFILE) ||
                        (sSystemErrno == EMFILE),
                        OPEN_LIMIT_ERROR );
        
        IDE_RAISE(INVALID_OPERATION);
    }
    else
    {
        // Nothing to do
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(NO_SPACE_ERROR);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_DISK_SPACE_EXHAUSTED,
                                aFilePath,
                                0,
                                0));
    }
    IDE_EXCEPTION(OPEN_LIMIT_ERROR);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_EXCEED_OPEN_FILE_LIMIT,
                                aFilePath,
                                0,
                                0));
    }
    IDE_EXCEPTION(INVALID_OPERATION);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_OPERATION));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduFileStream::closeFile( FILE* aFp )
{
/***********************************************************************
 *
 * Description : file을 close함
 *
 * Implementation :
 *    1. fclose 함수 호출
 *    2. return value가 0이 아니면 에러
 *    3. errno에 따라서 적절한 error 반환
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::closeFile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SInt sRet;

    sRet = idlOS::fclose( aFp );

    IDE_TEST_RAISE( sRet != 0, CLOSE_ERROR );
        
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(CLOSE_ERROR);
    {
        // BUG-21760 PSM에서 생성한 파일에 대하여 close의 실패는
        // ABORT로 충분하다.
        IDE_SET(ideSetErrorCode(idERR_ABORT_FILE_CLOSE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduFileStream::getString( SChar* aStr,
                                 SInt   aLength,
                                 FILE* aFp )
{
/***********************************************************************
 *
 * Description : 한 line을 파일에서 읽어옴
 *
 * Implementation :
 *    1. fgets 함수 호출
 *    2. return value가 NULL이면 에러
 *    3. 파일의 끝이라면 no data found, 그렇지 않다면 invalid operation
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::getString"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SChar* sRet;

    sRet = idlOS::fgets( aStr, aLength, aFp );

    if( sRet == NULL )
    {
        if( idlOS::idlOS_feof( aFp ) )
        {
            // NO_DATA_FOUND
            IDE_RAISE(NO_DATA_FOUND);
        }
        else
        {
            IDE_RAISE(READ_ERROR);
        }
    }
    else
    {
        // Nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(NO_DATA_FOUND);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_NO_DATA_FOUND));
    }
    IDE_EXCEPTION(READ_ERROR);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_READ_ERROR));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduFileStream::putString( SChar* aStr,
                                 FILE* aFp )
{
/***********************************************************************
 *
 * Description : string을 파일에 기록
 *
 * Implementation :
 *    1. fputs 함수 호출
 *    2. return value가 EOF(-1)이면 에러
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::putString"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SInt sRet;

    sRet = idlOS::fputs( aStr, aFp );

    IDE_TEST_RAISE( sRet == EOF, WRITE_ERROR );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(WRITE_ERROR);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_WRITE_ERROR));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
#undef IDE_FN
}

IDE_RC iduFileStream::flushFile( FILE* aFp )
{
/***********************************************************************
 *
 * Description : string을 파일에 기록
 *
 * Implementation :
 *    1. fflush 함수 호출
 *    2. return value가 0이 아니라면 에러
 *    3. errno에 따라서 적절한 error 반환
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::flushFile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SInt sRet;
    SInt sSystemErrno = 0;
    
    IDE_DASSERT( aFp != NULL );
    
    sRet = idlOS::fflush( aFp );

    if( sRet != 0 )
    {
        sSystemErrno = errno;

#if !defined(VC_WINCE)
        IDE_TEST_RAISE(sSystemErrno == EBADF,
                       INVALID_FILEHANDLE);
#endif /* VC_WINCE */

#if defined(WRS_VXWORKS)
        IDE_TEST_RAISE( sSystemErrno == ENOSPC, NO_SPACE_ERROR );
#else
        IDE_TEST_RAISE((sSystemErrno == ENOSPC)||
                       (sSystemErrno == EDQUOT),
                       NO_SPACE_ERROR);
#endif
        
        IDE_RAISE(WRITE_ERROR);
    }
    else
    {
        // Nothing to do
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION(INVALID_FILEHANDLE);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_INVALID_FILEHANDLE));
    }
    IDE_EXCEPTION(WRITE_ERROR);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_WRITE_ERROR));
    }
    IDE_EXCEPTION(NO_SPACE_ERROR);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_DISK_SPACE_EXHAUSTED,
                                "filehandle in FFLUSH",
                                0,
                                0));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduFileStream::removeFile( SChar* aFilePath )
{
/***********************************************************************
 *
 * Description : 파일 삭제
 *
 * Implementation :
 *    1. remove 함수 호출
 *    2. return value가 0이 아니라면 에러
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::removeFile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SInt sRet;

    sRet = idlOS::remove( aFilePath );

    IDE_TEST_RAISE( sRet != 0, DELETE_FAILED );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION(DELETE_FAILED);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_DELETE_FAILED));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduFileStream::renameFile( SChar* aOldFilePath,
                                  SChar* aNewFilePath,
                                  idBool aOverWrite )
{
/***********************************************************************
 *
 * Description : 파일 이동(or rename)
 *
 * Implementation :
 *    1. overwrite가 세팅되어 있으면 aNewFilePath에 이미 존재하면 에러
 *    2. remove 함수 호출
 *    3. return value가 0이 아니라면 에러
 *
 ***********************************************************************/
#define IDE_FN "iduFileStream::renameFile"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    FILE*  sFp;
    idBool sExist;
    SInt   sRet;
    
    sFp = idlOS::fopen( aNewFilePath, "r" );
    if( sFp == NULL )
    {
        sExist = ID_FALSE;
    }
    else
    {
        sExist = ID_TRUE;
        idlOS::fclose( sFp );
    }
    
    if( aOverWrite == ID_TRUE )
    {
        // execute remove function
        sRet = idlOS::rename( aOldFilePath, aNewFilePath );
        if( sRet != 0 )
        {
            IDE_RAISE(RENAME_FAILED);
        }
        else
        {
            // Nothing to do
        }
    }
    else
    {
        // if newfilepath is exists, return error
        if( sExist == ID_TRUE )
        {
            IDE_RAISE(RENAME_FAILED);
        }
        else
        {    
            sRet = idlOS::rename( aOldFilePath, aNewFilePath );

            IDE_TEST_RAISE( sRet != 0, RENAME_FAILED );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(RENAME_FAILED);
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_IDU_FILE_RENAME_FAILED));
    }
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

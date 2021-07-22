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
 
/*********************************************************************
 * $Id$
 ********************************************************************/

#include <acp.h>
#include <ide.h>
#include <idl.h>
#include <idp.h>
#include <iduISQLTermInfoMgr.h>

IDL_EXTERN_C_BEGIN

#define IDU_MAX_LEN_PID     16

#define IDU_ISQL_TERMINFO_PATH_MAX  (4096)      // PATH_MAX

/*******************************************************
 * Description:
 *  isql pty 경로 정보를 저장하고 있는 파일의 절대경로를 
 *  획득하는 함수. 
 *  파일의 경로는 $ALTIBASE_XDB_HOME/conf/isql_terminal_info
 *
 * aFileNameBufLen - [IN]  파일이름을 저장할 버퍼의 길이
 * aFileNameBuf    - [OUT] 파일 이름을 저장하는 버퍼
 *******************************************************/
static void iduGetISQLTerminalInfoFilePath( SInt   aFileNameBufLen,
                                            SChar *aFileNameBuf )
{
    IDE_ASSERT( ( aFileNameBufLen > 0 ) && ( aFileNameBuf != NULL ) );

    (void)idlOS::snprintf(
            aFileNameBuf,
            aFileNameBufLen,
            "%s"IDL_FILE_SEPARATORS"conf"IDL_FILE_SEPARATORS"isql_terminal_info",
            idlOS::getenv( IDP_HOME_ENV ) );
}

/*******************************************************
 * Description:
 *    isql에서 호출하여 isql의 PID, PTY를
 *    특정 파일에 기록한다.
 *******************************************************/
IDE_RC iduSetISQLTerminalInfo( idBool * aIsCreatePtyFile )
{
#ifndef _WINDOWS
    SChar    sPtyName[IDU_ISQL_TERMINFO_PATH_MAX] = "";
    FILE   * sFpIsqlInfo         = NULL;
    SInt     sState             = 0;
    SChar    sSetFilePath[IDU_ISQL_TERMINFO_PATH_MAX];
    SChar  * sPtyNamePtr = NULL;

    sPtyNamePtr = ttyname( STDOUT_FILENO );
    if( sPtyNamePtr != NULL )
    {
        iduGetISQLTerminalInfoFilePath( IDU_ISQL_TERMINFO_PATH_MAX, sSetFilePath );
        idlOS::sprintf( sPtyName, "%s", sPtyNamePtr );
        IDE_ASSERT( idlOS::strlen( sPtyName ) != 0 );
    
        sFpIsqlInfo = idlOS::fopen( sSetFilePath, "w+" );
        IDE_TEST_RAISE( sFpIsqlInfo == NULL, err_file_open_isql_termianl_info_file );
        *aIsCreatePtyFile = ID_TRUE;
        sState = 1;
    
        (void) idlOS::fprintf( sFpIsqlInfo, "%d\n", idlOS::getpid() );

        (void) idlOS::fputs( sPtyName, sFpIsqlInfo );
    
        sState = 0;
        (void)idlOS::fclose( sFpIsqlInfo );
         
    }
    else
    {
        *aIsCreatePtyFile = ID_FALSE;
        // iduRemoveISQLTerminalInfoFile();
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_open_isql_termianl_info_file )
    {
        IDE_SET( ideSetErrorCode( idERR_IGNORE_FAIL_OPEN_ISQL_PTY_FILE ) );
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( idlOS::fclose( sFpIsqlInfo ) == 0 );
        *aIsCreatePtyFile = ID_FALSE;
    }

    return IDE_FAILURE;
#else
    return IDE_FAILURE;
#endif
}

/*******************************************************
 * Description:
 *   isql의 pty 경로를 획득하는 함수이다.   
 *
 * aIsqlPtyNameLen - [IN]  경로를 저장할 버퍼의 길이
 * aIsqlPtyName    - [OUT] 경로를 저장하는 버퍼
 * aPID            - [OUT]
 *******************************************************/
IDE_RC iduGetISQLTerminalInfo( SInt     aIsqlPtyNameLen,
                               SChar  * aIsqlPtyName,
                               SInt   * aIsqlPID )
{
#ifndef _WINDOWS
    FILE   * sFpIsqlInfo = NULL;
    SInt     sState     = 0;
    SChar    sPtyNameFilePath[IDU_ISQL_TERMINFO_PATH_MAX];
    SChar    sPIDStr[IDU_MAX_LEN_PID];

    iduGetISQLTerminalInfoFilePath( IDU_ISQL_TERMINFO_PATH_MAX, sPtyNameFilePath );
    
    sFpIsqlInfo = idlOS::fopen( sPtyNameFilePath, "r");
    IDE_TEST_RAISE( sFpIsqlInfo == NULL, err_file_open_isql_termianl_info_file );
    sState = 1;

    IDE_TEST( idlOS::fgets( sPIDStr, IDU_MAX_LEN_PID, sFpIsqlInfo)
              != sPIDStr );

    IDE_TEST( acpCharIsDigit( sPIDStr[0] ) != ACP_TRUE );

    *aIsqlPID = (SInt)idlOS::atoi( sPIDStr );

    IDE_TEST( idlOS::fgets( aIsqlPtyName, aIsqlPtyNameLen, sFpIsqlInfo )
              != aIsqlPtyName );

    sState = 0;
    (void)idlOS::fclose( sFpIsqlInfo );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_file_open_isql_termianl_info_file )
    {
        IDE_SET( ideSetErrorCode( idERR_IGNORE_FAIL_OPEN_ISQL_PTY_FILE ) );
    }
    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        IDE_ASSERT( idlOS::fclose( sFpIsqlInfo ) == 0 );
    }

    return IDE_FAILURE;
#else
    return IDE_FAILURE;
#endif
}

/*******************************************************
 * Description:
 *    $ALTIBASE_XDB_HOME/conf/isql_terminal_info 삭제
 *******************************************************/
void iduRemoveISQLTerminalInfoFile( void )
{
    SChar    sPtyNameFilePath[IDU_ISQL_TERMINFO_PATH_MAX] = "";

    iduGetISQLTerminalInfoFilePath( IDU_ISQL_TERMINFO_PATH_MAX,
                                    sPtyNameFilePath );

    if( idlOS::strlen( sPtyNameFilePath ) != 0 )
    {
        (void)idlOS::unlink( sPtyNameFilePath );
    }
}

IDL_EXTERN_C_END

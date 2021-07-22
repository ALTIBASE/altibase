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
 
#include <ide.h>
#include <idwService.h>
#include <idtContainer.h>


//Static Variable Initialize

#if defined(VC_WIN32) && !defined(VC_WINCE)
    SERVICE_STATUS_HANDLE   idwService::mHService      = NULL;
    SInt                    idwService::mServiceStatus = 0;
#endif  

    char*                   idwService::serviceName    = NULL;
    startFunction           idwService::mWinStart      = NULL;
    runFunction             idwService::mWinRun        = NULL;
    stopFunction            idwService::mWinStop       = NULL;


/*---------------------------------------------------------------
 * PROJ-1699
 *
 * Name : serviceStart()
 *
 * Description :
 * 1. 사용자 변수에서 서비스 이름을 읽어온다.
 * 2. SCM에 등록된 서비스에서 Start/Stop 등의 명령을 받을 수 있도록
 *    Dispatcher를 등록하고 실제 Altibase를 구동하기위해 
 *    serviceMain() 을 호출한다.
---------------------------------------------------------------*/

IDE_RC idwService::serviceStart()
{
#if defined(PDL_WIN32) && !defined(VC_WINCE)
    
    if ( (serviceName = idlOS::getenv(ALTIBASE_ENV_PREFIX"SERVICE")) == NULL )
    {
        //BUGBUG - 이벤트로그 남겨야 함
        //PS. 환경 변수를 읽어올 수 없음.
        return IDE_FAILURE;   
    }

	SERVICE_TABLE_ENTRY srvTE[]={
		{ serviceName, (LPSERVICE_MAIN_FUNCTION)serviceMain }, 
	    { NULL, NULL }
	};
	
	IDE_TEST( StartServiceCtrlDispatcher(srvTE) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
#else
    // UNIX,LINUX OTHER platform
    return IDE_FAILURE;
#endif  

}


/*---------------------------------------------------------------
 * PROJ-1699
 *
 * Name : serviceMain()
 *
 * Description :
 * Dispatcher에서 보내는 명령(Start/Stop)을 처리하기 위한 Handler를
 * 등록하고, Altibase 를 구동한다 (iSQL과의 통신은 별도로 필요하지
 * 않도록 처리되었다).
 *
---------------------------------------------------------------*/ 

IDE_RC idwService::serviceMain( SInt   /*argc*/,
                                SChar */*argv*/ )
{
#if defined(PDL_WIN32) && !defined(VC_WINCE)

    UInt     sOptionFlag      = 0;

	//ServiceHandle을 등록함.
	mHService = RegisterServiceCtrlHandler( serviceName, 
	                                        (LPHANDLER_FUNCTION)serviceHandler );
	
    if (mHService == 0)
    {
        //BUGBUG : 에러 메시지는 이벤트 로그로 남겨야 함		
        //GetLastError를 사용하여 ERROR_INVALID_NAME, 
        //ERROR_SERVICE_DOES_NOT_EXIST를 판별한뒤 이벤트 로그로 남김        
        return IDE_FAILURE;
    }

    /* BUG-47527 */
    IDE_TEST( idtContainer::initializeStatic(IDU_SERVER_TYPE) != IDE_SUCCESS );

    //Altibase Startup 
	setStatus( SERVICE_START_PENDING );
    IDE_TEST( idwService::mWinStart() != IDE_SUCCESS );
    
    //Listener를 생성
    setStatus( SERVICE_RUNNING );
    IDE_TEST( idwService::mWinRun() != IDE_SUCCESS );    

	setStatus( SERVICE_STOPPED );	
	
    IDE_TEST( idtContainer::destroyStatic() != IDE_SUCCESS );

	CloseHandle( mHService );  
	
	return IDE_SUCCESS;  
	
	IDE_EXCEPTION_END;
    {
        ideLog::logErrorMsg( IDE_SERVER_0 );
    }

    return IDE_FAILURE;
   
#else
    // UNIX,LINUX OTHER platform
    return IDE_FAILURE;
#endif  

}

/*---------------------------------------------------------------
 * PROJ-1699
 *
 * Name : serviceHandler(..)
 * Argument : opCode = 제어 명령(START/STOP)
 * Description :
 * Dispatcher에서 전달된 명령을 실제로 처리하는 함수이다.
 * SCM에서 STOP 명령 또는 시스템 종료시 전달되는 SHUTDOWN 명령을
 * 받아서, serviceStop 함수를 호출한다.
 * 각 명령을 처리한 후에 SCM에 현재 서비스 상태를 세팅한다.
---------------------------------------------------------------*/

#if defined(PDL_WIN32) && !defined(VC_WINCE)
void idwService::serviceHandler( SInt opCode )
#else
void idwService::serviceHandler( SInt /*opCode*/ )
#endif
{
#if defined(PDL_WIN32) && !defined(VC_WINCE)

  	if ( opCode == mServiceStatus )
  	{
		return ;
	}
	
	switch ( opCode )
	{
	case SERVICE_CONTROL_SHUTDOWN:            //SYSTEM SHUTDOWN
	case SERVICE_CONTROL_STOP:
		setStatus( SERVICE_STOP_PENDING );
		IDE_TEST( idwService::mWinStop() != IDE_SUCCESS );
		break;
	case SERVICE_CONTROL_INTERROGATE:
	default:
		setStatus(mServiceStatus);
		break;
	}
	
    IDE_EXCEPTION_END;
    {
        //BUGBUG - 이벤트로그로 관련 에러를 남겨야 함
        ideLog::logErrorMsg(IDE_SERVER_0);
    }
#else
    // UNIX,LINUX OTHER platform
#endif 
     
}

/*---------------------------------------------------------------
 * PROJ-1699
 *
 * Name : setStatus(..)
 * Argument : dwState = 서비스 상태
 * Description :
 * SCM에 상태 보고를 한다.
 * 
---------------------------------------------------------------*/

#if defined(PDL_WIN32) && !defined(VC_WINCE)
void idwService::setStatus( SInt dwState )
#else
void idwService::setStatus( SInt /*dwState*/ )
#endif    
{
#if defined(PDL_WIN32) && !defined(VC_WINCE)

    SERVICE_STATUS srvStatus;
    
	srvStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
	srvStatus.dwCurrentState            = dwState;
	srvStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
	srvStatus.dwWin32ExitCode           = 0;
	srvStatus.dwServiceSpecificExitCode = 0;
	srvStatus.dwCheckPoint              = 0;
	srvStatus.dwWaitHint                = 0;

	mServiceStatus                      = dwState;
	
	SetServiceStatus( mHService, &srvStatus );

#else
    // UNIX,LINUX OTHER platform

#endif 
   
}


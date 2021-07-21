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

#include <mmm.h>
#include <mmErrorCode.h>
#include <mmuProperty.h>


/* =======================================================
 * Action Function
 * => Check Environment
 * =====================================================*/

static IDE_RC mmmSetupFD()
{
#if defined(ALTIBASE_USE_VALGRIND)
    return IDE_SUCCESS;
#else
    SInt CurMaxHandle, MaxHandle;

    /* ------------------------------------------------------------------
     *  Maximum File Descriptor 설정
     * -----------------------------------------------------------------*/

    CurMaxHandle = idlVA::max_handles();
    idlVA::set_handle_limit();
    MaxHandle = idlVA::max_handles();

    if( MaxHandle > FD_SETSIZE )
    {
        ideLog::log(IDE_SERVER_0,
                    MM_TRC_MAX_HANDLE_EXCEED_FD_SETSIZE,
                    (SInt)MaxHandle,
                    (SInt)FD_SETSIZE);
        idlOS::exit(-1);
    }

    if( ID_SIZEOF(fd_set) < (SInt)(FD_SETSIZE / 8) )
    {
        ideLog::log(IDE_SERVER_0,
                    MM_TRC_SMALL_SIZE_OF_FD_SET,
                    (SInt)ID_SIZEOF(fd_set),
                    (SInt)(FD_SETSIZE / 8));
        idlOS::exit(-1);
    }

    IDE_TEST_RAISE(CurMaxHandle == -1, error_get_max_handle);
    IDE_TEST_RAISE(MaxHandle < 0, error_set_max_handle);

    // 핸들의 수보다 쓰레드 갯수가 더 많음 (개략측정)
    if ((UInt)MaxHandle < mmuProperty::getMaxClient())
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_MAX_HANDLE_EXCEED_MAX_CLIENT);
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION(error_get_max_handle);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_GET_MAX_HANDLE_FAILED);
        IDE_SET(ideSetErrorCode(mmERR_FATAL_MAXHANDLE_ERROR));
    }
    IDE_EXCEPTION(error_set_max_handle);
    {
        ideLog::log(IDE_SERVER_0, MM_TRC_GET_MAX_HANDLE_FAILED);
        IDE_SET(ideSetErrorCode(mmERR_FATAL_MAXHANDLE_ERROR));
    }

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
#endif
}

/************************************************************************ 
   BUG-16958
   file size limit 정책
   getrlimit(RLIMIT_FSIZE, ...)에 대해 32-bit system에서는
   4G가 아닌 2G를 리턴해야 한다. 왜냐하면 32-bit linux 시스템에서는
   getrlimit이 4G까지 사용할 수 있다해도 실제로는 2G밖에 사용할 수가 없다.
   단, 컴파일옵션에 _FILE_OFFSET_BITS=64를 주면 32bit system이라 하더라도
   2G 이상의 파일을 사용할 수 있다.
   따라서 system이 32-bit이냐, _FILE_OFFSET_BITS definition이 있느냐에 따라
   적당한 RLIMIT_FSIZE 값을 세팅한다.
**************************************************************************/
static IDE_RC mmmSetupOSFileSize()
{
#if (!defined(COMPILE_64BIT) && (_FILE_OFFSET_BITS != 64))
    struct rlimit limit;

    IDE_TEST_RAISE(idlOS::getrlimit(RLIMIT_FSIZE, &limit) != 0,
                   getrlimit_error);

    limit.rlim_cur = 0x7FFFFFFF;

    IDE_TEST_RAISE(idlOS::setrlimit(RLIMIT_FSIZE, &limit) != 0,
                   setrlimit_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(getrlimit_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_GETLIMIT_ERROR));
    }
    IDE_EXCEPTION(setrlimit_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_SETLIMIT_ERROR));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#else
    return IDE_SUCCESS;
#endif
}

static IDE_RC mmmCheckFileSize()
{
    struct rlimit  limit;
    /* ------------------------------------------------------------------
     *  [4] Log(DB) File Size 검사
     * -----------------------------------------------------------------*/
    IDE_TEST_RAISE(idlOS::getrlimit(RLIMIT_FSIZE, &limit) != 0,
                   getrlimit_error);

    // PROJ-1490 페이지리스트 다중화및 메모리반납
    //
    // 이 함수는 pre process단계에 체크되는 기능들을 모아놓은 것이다.
    // 데이터베이스 파일 크기에 대한 limit 체크는 smmManager에서 처리한다.
    // smmManager::initialize참고

    IDE_TEST_RAISE( mmuProperty::getLogFileSize() - 1 > limit.rlim_cur ,
                    check_OSFileLimit);

    return IDE_SUCCESS;
    IDE_EXCEPTION(check_OSFileLimit);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_OSFileSizeLimit_ERROR));
    }
    IDE_EXCEPTION(getrlimit_error);
    {
        IDE_SET(ideSetErrorCode(mmERR_FATAL_GETLIMIT_ERROR));
    }
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

static IDE_RC mmmCheckOSenv()
{
    /* ------------------------------------------------------------------
     *  [5] AIX에서의 환경변수 검사 및 Messasge 남기기
     *      관련버그 : PR-3900
     *       => 환경변수 리스트
     *          AIXTHREAD_MNRATIO=1:1
     *          AIXTHREAD_SCOPE=S
     *          AIXTHREAD_MUTEX_DEBUG=OFF
     *          AIXTHREAD_RWLOCK_DEBUG=OFF
     *          AIXTHREAD_COND_DEBUG=OFF
     *          SPINLOOPTIME=1000
     *          YIELDLOOPTIME=50
     *          MALLOCMULTIHEAP=1
     *
     *   [5] HP에서의 환경변수 검사
     *       => _M_ARENA_OPTS : 8 혹은 16으로 설정 요망. (BUG-13294, TASK-1733)
     *
     * -----------------------------------------------------------------*/
    {
        static const SChar *sEnvList[] =
        {
#if defined(IBM_AIX)
            "AIXTHREAD_MNRATIO",
            "AIXTHREAD_SCOPE",
            "AIXTHREAD_MUTEX_DEBUG",
            "AIXTHREAD_RWLOCK_DEBUG",
            "AIXTHREAD_COND_DEBUG",
            "SPINLOOPTIME",
            "YIELDLOOPTIME",
            "MALLOCMULTIHEAP",
#elif defined(HP_HPUX)
            "_M_ARENA_OPTS",
#endif
            NULL
        };
        SChar *sEnvVal;
        UInt   i;
        idBool sCheckSuccess;

        IDE_CALLBACK_SEND_SYM("[PREPARE] Check O/S Environment  Variables..");

        sCheckSuccess = ID_TRUE;
        for (i = 0; sEnvList[i] != NULL; i++)
        {
            UInt sLen = 0;

            sEnvVal = idlOS::getenv(sEnvList[i]);
            if (sEnvVal != NULL)
            {
                sLen = idlOS::strlen(sEnvVal);
            }

            if(sLen == 0)
            {
                ideLog::log(IDE_SERVER_0, MM_TRC_ENV_NOT_DEFINED, sEnvList[i]);
                sCheckSuccess = ID_FALSE;
            }
            else
            {
                ideLog::log(IDE_SERVER_0, MM_TRC_ENV_DEFINED, sEnvList[i], sEnvVal);
            }
        }

        if (sCheckSuccess != ID_TRUE)
        {
            IDE_CALLBACK_SEND_MSG("[WARNING!!!] check $ALTIBASE_HOME"IDL_FILE_SEPARATORS"trc"IDL_FILE_SEPARATORS"altibase_boot.log");
        }
        else
        {
            IDE_CALLBACK_SEND_MSG("[SUCCESS]");
        }
   }
    return IDE_SUCCESS;
}

static IDE_RC mmmPhaseActionInitOS(mmmPhase         /*aPhase*/,
                                   UInt             /*aOptionflag*/,
                                   mmmPhaseAction * /*aAction*/)
{
    IDE_TEST( mmmSetupFD() != IDE_SUCCESS);

    IDE_TEST( mmmSetupOSFileSize() != IDE_SUCCESS);

#if !defined(WRS_VXWORKS) && !defined(SYMBIAN)
    IDE_TEST( mmmCheckFileSize() != IDE_SUCCESS);
#endif

    IDE_TEST( mmmCheckOSenv()   != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* =======================================================
 *  Action Func Object for Action Descriptor
 * =====================================================*/

mmmPhaseAction gMmmActInitOS =
{
    (SChar *)"Initialize Operating System Parameters",
    0,
    mmmPhaseActionInitOS
};


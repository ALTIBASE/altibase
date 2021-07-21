/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduMutex.cpp 84268 2018-10-26 07:26:28Z donghyun1 $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduRunTimeInfo.h>
#include <iduMutexMgr.h>
#include <iduMutex.h>
#include <iduProperty.h>
#include <idp.h>
#include <idv.h>

#define IDU_ENV_SPIN_COUNT  "ALTIBASE_MUTEX_SPINLOCK_COUNT"

IDE_RC
iduMutex::initialize( const SChar     * aName,
                      iduMutexKind      aKind,
                      idvWaitIndex      aWaitEventID )
{
#define IDE_FN "SInt iduMutex::initialize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    static SChar *gUnknown = (SChar *)"UKNOWN_MUTEX_OBJ";

    SChar *sSpinDefaultEnv;

    SChar *sSpinUserEnv;

    SChar sNameBuf[512];

    UInt sSpinCount;

    if (aName == NULL)
    {
        aName = gUnknown;
    }

    /* ------------------------------------------------
     *  Property에 의해 POSIX로 되돌릴 수 있도록
     * ----------------------------------------------*/
#if defined(WRS_VXWORKS) || defined(VC_WINCE) || defined(ANDROID) || defined(SYMBIAN) || defined(X86_SOLARIS)   
    aKind = IDU_MUTEX_KIND_POSIX;
#else
    /* ------------------------------------------------
     * when kind is IDU_MUTEX_KIND_NATIVE2,3 ignore property
     * ----------------------------------------------*/
    if( ( aKind != IDU_MUTEX_KIND_NATIVE2 ) &&
        ( aKind != IDU_MUTEX_KIND_NATIVE_FOR_LOGGING ) )
    {
        if( iduProperty::getMutexType() == 0 )
        {
            aKind = IDU_MUTEX_KIND_POSIX;
        }
    }
#endif
    
#if defined(ALTIBASE_USE_VALGRIND)
    aKind = IDU_MUTEX_KIND_POSIX;
#endif

    IDE_TEST(iduMutexMgr::alloc(&mEntry, aKind) != IDE_SUCCESS);

    /* ------------------------------------------------
     *  1. MUTEX 인자 처리
     *
     *     A. 프로퍼티 MUTEX_SPIN_COUNT 에서 얻는다. 
     *     B. default 값은 아래의 환경변수에서 얻는다.
     *         ALTIBASE_MUTEX_SPINLOCK_COUNT = VALUE;
     *
     *     C. 이름이 지정되었을 경우 설정한다.
     *        ALTIBASE_MUTEX_SPINLOCK_COUNT_[NAME] = VALUE
     *
     * ----------------------------------------------*/

    switch( aKind )
    {
        case IDU_MUTEX_KIND_NATIVE2 :
            sSpinCount = iduProperty::getNativeMutexSpinCount();
            break;
        case IDU_MUTEX_KIND_NATIVE_FOR_LOGGING :
            // BUG-28856 Logging 병목제거
            // 높은 spin count를 가진 native mutex에서 Logging성능 향상
            sSpinCount = iduProperty::getNativeMutexSpinCount4Logging();
            break;
        default:
            sSpinCount = iduProperty::getMutexSpinCount();
            break;
    }

    idlOS::memset(sNameBuf, 0, 512);

    idlOS::strcpy(sNameBuf, IDU_ENV_SPIN_COUNT);
    idlOS::strcat(sNameBuf, "_");
    idlOS::strcat(sNameBuf, aName);

    sSpinUserEnv   = idlOS::getenv(sNameBuf);

    /* ------------------------------------------------
     *  Choose Env
     * ----------------------------------------------*/

    if (sSpinUserEnv != NULL)
    {
        sSpinCount = idlOS::strtol(sSpinUserEnv, 0, 10);
    }
    else
    {
        sSpinDefaultEnv = idlOS::getenv(IDU_ENV_SPIN_COUNT);

        if (sSpinDefaultEnv != NULL)
        {
            sSpinCount = idlOS::strtol(sSpinDefaultEnv, 0, 10);
        }
    }

    IDE_TEST(mEntry->initialize(aName,
                                aWaitEventID,
                                sSpinCount) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduMutex::destroy()
{
#define IDE_FN "SInt iduMutex::destroy()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    IDE_DASSERT(mEntry != NULL);
    
    if ( mEntry != NULL )
    {
        IDE_TEST(iduMutexMgr::free(mEntry) != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    return IDE_FAILURE;


#undef IDE_FN
}


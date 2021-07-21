/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduBridge.h 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idTypes.h>
#include <iduBridgeTime.h>

/* ------------------------------------------------
 *
 *  아래의 함수들은 C에서 C++ 함수를 호출하기 위한
 *  Bridge로서 정의된다. (iduBridgeForC.cpp)
 *
 * ----------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

    ULong  idlOS_getThreadID();
    SInt   idlOS_getProcessorCount();

    void   idlOS_sleep(ULong aSec, ULong aMicroSec);
    void   idlOS_thryield();

    SInt   idlOS_rand();
    /* wrapper for property read */
    UInt   iduBridge_getMutexSpinCount();
    UInt   iduBridge_getCheckMutexDurationTimeEnable();
    UInt   iduMutexSleepType(void);
    UInt   iduLatchMinSleep(void);
    UInt   iduLatchMaxSleep(void);
    UInt   iduLatchSleepType(void);
    
    /* idvTime related functions */
    /* return ID_SIZEOF(idvTime) */
    UInt   idv_SizeOf_IdvTime();
    /* wrapper for IDV_TIME_AVAILABLE() macro */
    idBool idv_TIME_AVAILABLE();
    /* wrapper for IDV_TIME_GET() */
    void   idv_TIME_GET( iduBridgeTime * aTime );
    /* wrapper for IDV_TIME_DIFF_MICRO() */
    ULong  idv_TIME_DIFF_MICRO(iduBridgeTime * aBeforeTime,
                               iduBridgeTime * aAfterTime);

    /*
     * TASK-2356
     * Altibase Wait Interface 시간통계정보 수집
     */
    UInt iduBridge_getTimedStatistics();
    void idv_BEGIN_WAIT_EVENT( void * aStatSQL,
                               void * aWeArgs );
    void idv_END_WAIT_EVENT( void  * aStatSQL,
                             void * aWeArgs );
    
    void assertForC(SChar *aCond, SChar *aFile, UInt aLine);

#    define IDE_CASSERT(a)  if ( !(a)) { assertForC(#a, __FILE__, __LINE__); }

#    if defined(DEBUG)
#     define IDE_DCASSERT(a) IDE_CASSERT(a)
#    else
#     define IDE_DCASSERT(a) ;
#    endif

#ifdef __cplusplus
}
#endif

/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idvTime.h 78754 2017-01-23 02:24:41Z kclee $
 *
 * Time을 측정하는 작업은 대단히 빨리 수행되어야 하기 때문에
 * 기존의 gettimeofday()와 같은 함수들은 시스템 호출인 이유로
 * 사용할 수 없다.
 *
 * 따라서, 해당 시스템에서 제공하는 특별한 함수를 이용하여야 하기 때문에
 * 해당 플랫폼에 대한 각각의 구현이 필요하다.
 *
 * 또한 수행성능이 대단히 중요하기 때문에 함수 호출조차도
 * 가능하면 inline 혹은 매크로로 대체할 수 있도록 한다.
 *
 **********************************************************************/

#ifndef _O_IDV_TIME_H_
# define _O_IDV_TIME_H_

#include <idl.h>

#define IDV_TIMER_LEVEL_NONE    0
#define IDV_TIMER_LEVEL_THREAD  1
#define IDV_TIMER_LEVEL_LIBRARY 2
#define IDV_TIMER_LEVEL_CLOCK   3

// BUG-27292 HP에서 timedwait()에서 SInt로 Time값을 받는 문제(2038년 문제)가 있어
// Max Time값이 SInt를 넘어서지 않도록 Max Time Interval값을 별도로 선언합니다.
#define IDV_MAX_TIME_INTERVAL_SEC  ( 60 * 60 * 24 * 30 )   // Sec (Max 1 Month)
#define IDV_MAX_TIME_INTERVAL_MSEC ( 1000 * 60 * 60 * 24 ) // Milli Sec (1 Day)
#define IDV_MAX_TIME_INTERVAL_USEC ( 1000000 * 60 )        // Micro Sec (1 Min)

// if ( idvManager::isServiceAvail )
//    if ( idvManager::getTimeType() == IDV_TIMER_LEVEL_CLOCK )
//       if ( getClock() )
//          1
//       ese
//          0
//    else
//       1
// else
//    0
#define IDV_TIME_AVAILABLE()  (idvManager::isServiceAvail() && ((idvManager::getTimeType()==IDV_TIMER_LEVEL_CLOCK)?idvManager::getClock():1))

#define IDV_TIME_INIT(val)    idvManager::mOP->mInit(val)
#define IDV_TIME_IS_INIT(val) idvManager::mOP->mIsInit(val)
#define IDV_TIME_IS_OLDER(a, b) idvManager::mOP->mIsOlder((a), (b))
#define IDV_TIME_GET(val)     idvManager::mOP->mGet(val)
#define IDV_TIME_DIFF_MICRO(before, after)  idvManager::mOP->mDiff((before), (after))
#define IDV_TIME_MICRO(val)   idvManager::mOP->mMicro(val)
#define IDV_TIME_DIFF_MICRO_SAFE(before, after)                \
     (IDL_LIKELY_FALSE(IDV_TIME_IS_OLDER((before), (after)))   \
     ?                                                         \
     idvManager::fixTimeDiff(before)                           \
     :                                                         \
     (IDV_TIME_DIFF_MICRO((before), (after))))

/* ------------------------------------------------
 *  Solaris, Wind River, WINCE, ANDROID, SYMBIAN
 * ----------------------------------------------*/

#if defined(SPARC_SOLARIS) || defined(X86_SOLARIS) || defined(WRS_VXWORKS) || \
    defined(VC_WINCE) || defined(ANDROID) || defined(SYMBIAN)

#define IDV_TIMER_DEFAULT_LEVEL       IDV_TIMER_LEVEL_LIBRARY

typedef struct idvTime
{
    union
    {
        timespec mSpec;
        ULong    mClock; // or Micro Second
    } iTime;
} idvTime;

/* ------------------------------------------------
 * HP 
 * ----------------------------------------------*/
// bug-23877 v$statement total_time is 0 when short SQL on hp-ux
// hp-ux에서의 time interval 구하는 방식 변경 (thread -> library)
// reason:
// 1. thread : hp timeslice 100 milisec + thread sleep (1 milisec) => inaccurate
// 2. clock  : not synchronized in multi-CPU 
// 3. library: the cost of gethrtime() is low (130 cycles) (cf)gettimeofday: 3750)
/* 
 * BUG-32878 The startup failure caused by setting TIMER_RUNNING_LEVEL=3 should be handled
 * Set default value of TIMER_RUNNING_LEVEL is reverted to IDV_TIMER_LEVEL_THREAD on HP platforms
   because of performance issue, even though IDV_TIMER_LEVEL_LIBRARY is supported by BUG-23877.
 * (only 5.7.1 and later versions)
 */
#elif defined(HP_HPUX) || defined(IA64_HP_HPUX)

#define IDV_TIMER_DEFAULT_LEVEL       IDV_TIMER_LEVEL_THREAD 

typedef struct idvTime
{
    union
    {
        timespec mSpec;
        ULong mClock; // or Micro Second
    } iTime;
} idvTime;

/* ------------------------------------------------
 *  Linux
 * ----------------------------------------------*/
#elif defined(OS_LINUX_KERNEL) && !defined(ANDROID)

#if defined(IA64_LINUX)
#define IDV_TIMER_DEFAULT_LEVEL       IDV_TIMER_LEVEL_THREAD
#else
#define IDV_TIMER_DEFAULT_LEVEL       IDV_TIMER_LEVEL_CLOCK
#endif

typedef struct idvTime
{
    union
    {
        timespec mSpec;
        ULong    mClock; // or Micro Second
    } iTime;
} idvTime;

/* ------------------------------------------------
 *  AIX
 * ----------------------------------------------*/
#elif defined(IBM_AIX)

#define IDV_TIMER_DEFAULT_LEVEL       IDV_TIMER_LEVEL_LIBRARY

typedef struct idvTime
{
    union
    {
        timebasestruct_t mSpec;
        ULong            mClock; // or Micro Second
    } iTime;
} idvTime;

#else

// Other Platforms

#define IDV_TIMER_DEFAULT_LEVEL       IDV_TIMER_LEVEL_THREAD

typedef struct idvTime
{
    union
    {
        ULong            mClock; // or Micro Second
    } iTime;
} idvTime;


#endif

IDL_EXTERN_C ULong   idvGetClockTickFromSystem();

/* ------------------------------------------------
 *  Wait
 * ----------------------------------------------*/
/*
typedef struct idvWait
{
    UInt  EVENT_ID;
    ULong TOTAL_WAITS;
    ULong TOTAL_TIMEOUTS;
    ULong TIME_WAITED;
    ULong AVERAGE_WAIT;
    ULong MAX_WAIT;
    ULong TIME_WAITED_MICRO;

} idvWait;
*/

#endif

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
 
/*******************************************************************************
 * $Id: acpTime.c 11312 2010-06-22 08:08:48Z djin $
 ******************************************************************************/

#include <acpAtomic.h>
#include <acpSpinLock.h>
#include <acpSleep.h>
#include <acpThr.h>
#include <acpTime.h>
#include <acpTimePrivate.h>

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)
/*
 * Leap year is any year divisible by four, but not by 100 unless also
 * divisible by 400
 */
/* BUG-36296 그레고리력의 윤년 규칙은 1583년부터 적용한다. 1582년 이전에는 4년마다 윤년이다. */
#define ACP_TIME_IS_LEAP_YEAR(y)                                        \
    ((!((y) % 4)) ? (((y) < 1583) || (!((y) % 400)) || ((y) % 100) ? 1 : 0) : 0)

ACP_INLINE void acpTimeAcpTimeToFileTime(acp_time_t aTime, FILETIME *aFileTime)
{
    aTime += ACP_TIME_DELTA_EPOCH_IN_USEC;
    aTime *= 10;
    aFileTime->dwLowDateTime  = (DWORD)aTime;
    aFileTime->dwHighDateTime = (DWORD)(aTime >> 32);
}

ACP_INLINE void acpTimeSystemTimeToAcpExpTime(SYSTEMTIME     *aSystemTime,
                                              acp_time_exp_t *aExpTime)
{
    static const acp_uint16_t sDayOffset[12] =
    {
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    };

    aExpTime->mSec       = aSystemTime->wSecond;
    aExpTime->mMin       = aSystemTime->wMinute;
    aExpTime->mHour      = aSystemTime->wHour;
    aExpTime->mDay       = aSystemTime->wDay;
    aExpTime->mMonth     = aSystemTime->wMonth;
    aExpTime->mYear      = aSystemTime->wYear;
    aExpTime->mDayOfWeek = aSystemTime->wDayOfWeek;
    aExpTime->mDayOfYear = sDayOffset[aExpTime->mMonth - 1] + aExpTime->mDay;

    /*
     * If this is a leap year, and we're past the 28th of Feb. (the
     * 58th day after Jan. 1), we'll increment our tm_yday by one.
     */
    if (ACP_TIME_IS_LEAP_YEAR(aExpTime->mYear) && (aExpTime->mDayOfYear > 58))
    {
        aExpTime->mDayOfYear++;
    }
}

ACP_EXPORT acp_time_t acpTimeNow(void)
{
    FILETIME     sFileTime;
    SYSTEMTIME   sSystemTime;

    GetSystemTime(&sSystemTime);
    SystemTimeToFileTime(&sSystemTime, &sFileTime);

    return acpTimeFromAbsFileTime(&sFileTime);
}

ACP_EXPORT void acpTimeGetLocalTime(acp_time_t      aTime,
                                    acp_time_exp_t *aLocalTime)
{
    FILETIME              sFileTime;
    SYSTEMTIME            sSystemTime;
    SYSTEMTIME            sLocalSystemTime;
    TIME_ZONE_INFORMATION sTimeZone;

    acpTimeAcpTimeToFileTime(aTime, &sFileTime);

    GetTimeZoneInformation(&sTimeZone);

    FileTimeToSystemTime(&sFileTime, &sSystemTime);
    SystemTimeToTzSpecificLocalTime(&sTimeZone,
                                    &sSystemTime,
                                    &sLocalSystemTime);

    acpTimeSystemTimeToAcpExpTime(&sLocalSystemTime, aLocalTime);

    aLocalTime->mUsec = (acp_sint32_t)acpTimeGetUsec(aTime);
}

ACP_EXPORT void acpTimeGetGmTime(acp_time_t      aTime,
                                 acp_time_exp_t *aGmTime)
{
    SYSTEMTIME sSystemTime;
    FILETIME   sFileTime;

    acpTimeAcpTimeToFileTime(aTime, &sFileTime);

    FileTimeToSystemTime(&sFileTime, &sSystemTime);

    acpTimeSystemTimeToAcpExpTime(&sSystemTime, aGmTime);

    aGmTime->mUsec = (acp_sint32_t)acpTimeGetUsec(aTime);
}

ACP_EXPORT acp_rc_t acpTimeMakeTime(const acp_time_exp_t* aExpTime,
                                    acp_time_t*           aTime)
{
    acp_time_t              sTime;
    SYSTEMTIME              sSystemTime;
    FILETIME                sFileTime;
    TIME_ZONE_INFORMATION   sTz;

    GetTimeZoneInformation(&sTz);

    sSystemTime.wMilliseconds   = aExpTime->mUsec / 1000;
    sSystemTime.wSecond         = aExpTime->mSec;
    sSystemTime.wMinute         = aExpTime->mMin;
    sSystemTime.wHour           = aExpTime->mHour;
    sSystemTime.wDay            = aExpTime->mDay;
    sSystemTime.wMonth          = aExpTime->mMonth;
    sSystemTime.wYear           = aExpTime->mYear;
    sSystemTime.wDayOfWeek      = aExpTime->mDayOfWeek;

    /*
     * Windows takes SystemTime as local time
     * The timezone bias must be added
     */
    SystemTimeToFileTime(&sSystemTime, &sFileTime);

    sTime  = sFileTime.dwLowDateTime;
    sTime |= ((acp_time_t)sFileTime.dwHighDateTime) << 32;

    sTime /= 10;
    sTime -= ACP_TIME_DELTA_EPOCH_IN_USEC;
    sTime += (acp_time_t)sTz.Bias * 60000000;
    sTime += (aExpTime->mUsec % 1000);
    
    *aTime = sTime;

    return ACP_RC_SUCCESS;
}


#endif  /* ALTI_CFG_OS_WINDOWS */

#if defined(ALTI_CFG_OS_WINDOWS) ||    \
    defined(ALTI_CFG_OS_LINUX)   ||    \
    defined(ALTI_CFG_OS_DARWIN)   ||    \
    defined(ALTI_CFG_OS_FREEBSD) ||    \
    defined(ALTI_CFG_OS_HPUX)    ||    \
    defined(ALTI_CFG_OS_AIX)

typedef struct acpTimeInit
{
    volatile acp_hrtime_t   mBaseTime;
    volatile acp_sint64_t   mBaseCounter;

# if defined(ALTI_CFG_OS_WINDOWS) || \
     defined(ALTI_CFG_OS_LINUX)   || \
     defined(ALTI_CFG_OS_DARWIN)  || \
     defined(ALTI_CFG_OS_FREEBSD) 
    volatile acp_sint64_t   mFreq;
    volatile acp_sint64_t   mPrevTime;
# endif

} acpTimeInit;

# if defined(ALTI_CFG_OS_HPUX) || defined(ALTI_CFG_OS_AIX)
static acpTimeInit gTimeInit = { 0, 0 };
# else
static acpTimeInit     gTimeInit = { 0, 0, 0, 0 };
static acp_spin_lock_t gSpinLock = ACP_SPIN_LOCK_INITIALIZER(-1);
# endif

static acp_thr_once_t gInitOnce = ACP_THR_ONCE_INITIALIZER;

static void acpTimeOnceInit(void)
{
# if defined(ALTI_CFG_OS_WINDOWS)
    LARGE_INTEGER sFreq;

    (void)QueryPerformanceFrequency(&sFreq);

    gTimeInit.mFreq        = (acp_sint64_t)sFreq.QuadPart;
    gTimeInit.mBaseCounter = acpTimeTicks();
# elif defined(ALTI_CFG_OS_LINUX)  || \
       defined(ALTI_CFG_OS_DARWIN) || \
       defined(ALTI_CFG_OS_FREEBSD)
    volatile acp_uint64_t sTicks;

    /* we need too much times to guess a frequency */
    sTicks = acpTimeTicks();
    acpSleepMsec(100);
    sTicks = acpTimeTicks() - sTicks;

    /* clock count per 1 sec == cpu frequency */
    gTimeInit.mFreq        = sTicks*10;
    gTimeInit.mBaseCounter = acpTimeTicks();
# elif defined(ALTI_CFG_OS_HPUX)
    gTimeInit.mBaseCounter = gethrtime();
# elif defined(ALTI_CFG_OS_AIX)
    timebasestruct_t sTime;

    /* mread_real_time(&sTime, TIMEBASE_SZ); */
    read_real_time(&sTime, TIMEBASE_SZ);
    time_base_to_time(&sTime, TIMEBASE_SZ);

    gTimeInit.mBaseCounter = (acp_sint64_t)
        ((sTime.tb_high*ACP_SINT64_LITERAL(1000000000)) + sTime.tb_low);
# endif

    /* micro to nano for base time */
    gTimeInit.mBaseTime = (acp_hrtime_t)(acpTimeNow()*1000);
}

#endif  /* ALTI_CFG_OS_WINDOWS || ALTI_CFG_OS_LINUX  || ALTI_CFG_OS_HPUX */

ACP_EXPORT void acpTimeHiResRevise(void)
{
#if defined(ALTI_CFG_OS_WINDOWS) || \
    defined(ALTI_CFG_OS_DARWIN)  || \
    defined(ALTI_CFG_OS_LINUX)   || \
    defined(ALTI_CFG_OS_FREEBSD)
    
    acp_hrtime_t sCurTime;
    acp_sint64_t sCurCounter;
    acp_uint64_t sCurFreq;

    acpThrOnce(&gInitOnce, acpTimeOnceInit);

    while (1)
    {
        sCurTime    = (acp_hrtime_t)(acpTimeNow() * 1000);
        sCurCounter = (acp_sint64_t)acpTimeTicks();

        if (((sCurCounter > gTimeInit.mBaseCounter) &&
             (sCurTime > gTimeInit.mBaseTime)))
        {
            break;
        }
        else
        {
            /* calculate again */
        }
    }

# if defined(ALTI_CFG_OS_LINUX)  || \
     defined(ALTI_CFG_OS_DARWIN) || \
     defined(ALTI_CFG_OS_FREEBSD)
    sCurFreq = (acp_hrtime_t)(
            (acp_double_t)(sCurCounter - gTimeInit.mBaseCounter) /
                          (sCurTime - gTimeInit.mBaseTime)) *
                              ACP_SINT64_LITERAL(1000000000);

    if (gTimeInit.mFreq != 0)
    {
        /* use the average value of two points as the current frequency */
        sCurFreq = (sCurFreq + gTimeInit.mFreq) / 2;
    }
    else
    {
        /* if gTimeInit.mFreq is 0, we dont need to use it. */
    }
# else
    sCurFreq = gTimeInit.mFreq;
# endif

    acpSpinLockLock(&gSpinLock);

    gTimeInit.mBaseTime    = sCurTime;
    gTimeInit.mBaseCounter = sCurCounter;
    gTimeInit.mFreq        = sCurFreq;

    acpSpinLockUnlock(&gSpinLock);
#endif  /* ALTI_CFG_OS_LINUX */
}

#if defined(ALTI_CFG_OS_TRU64)   ||  \
    defined(ALTI_CFG_OS_SOLARIS) ||  \
    defined(ALTI_CFG_OS_AIX) 
static acp_hrtime_t gPrevTime = 0;
static acp_spin_lock_t gSpinLock = ACP_SPIN_LOCK_INITIALIZER(-1);
#endif

ACP_EXPORT acp_hrtime_t acpTimeHiResNow(void)
{
#if defined(ALTI_CFG_OS_WINDOWS) || \
    defined(ALTI_CFG_OS_DARWIN)  || \
    defined(ALTI_CFG_OS_LINUX)   || \
    defined(ALTI_CFG_OS_FREEBSD)
    
    acp_hrtime_t sTime;
    acp_sint64_t sCurCounter;

    acp_sint64_t sBaseCounter;
    acp_sint64_t sBaseFreq;
    acp_sint64_t sPrevTime;
    acp_hrtime_t sBaseTime;

    acpThrOnce(&gInitOnce, acpTimeOnceInit);

    acpSpinLockLock(&gSpinLock);

    while (1)
    {
        sBaseCounter = gTimeInit.mBaseCounter;
        sBaseFreq    = gTimeInit.mFreq;
        sBaseTime    = gTimeInit.mBaseTime;
        sPrevTime    = gTimeInit.mPrevTime;

        sCurCounter = (acp_sint64_t)acpTimeTicks();

        if (sCurCounter < sBaseCounter)
        {
            acpSpinLockUnlock(&gSpinLock);
            acpThrYield();
            acpSpinLockLock(&gSpinLock);
        }
        else
        {
            break;
        }
    }

    sCurCounter -= sBaseCounter;
    sTime        = (acp_hrtime_t)((acp_double_t)sCurCounter/sBaseFreq) *
                   ACP_SINT64_LITERAL(1000000000) + sBaseTime;

    if (sPrevTime >= sTime)
    {
        sTime = gTimeInit.mPrevTime + 1;
    }
    else
    {
        /* do nothing, we got the correct value */
    }

    gTimeInit.mPrevTime = sTime;

    acpSpinLockUnlock(&gSpinLock);

    return (acp_hrtime_t)sTime;
#elif defined(ALTI_CFG_OS_HPUX)
    hrtime_t   sCurCounter;

    acp_hrtime_t sTime;
    acp_hrtime_t sBaseTime;
    acp_sint64_t sBaseCounter;

    acpThrOnce(&gInitOnce, acpTimeOnceInit);

    sCurCounter = gethrtime();

    sBaseCounter = gTimeInit.mBaseCounter;
    sBaseTime    = gTimeInit.mBaseTime;

    sTime = (acp_time_t)sCurCounter - sBaseCounter;

    return (acp_hrtime_t)(sBaseTime + sTime);
#elif defined(ALTI_CFG_OS_AIX)
    timebasestruct_t sNow;

    acp_sint32_t sRet;
    acp_hrtime_t sTimeDiff;
    acp_hrtime_t sRetTime;

    acpThrOnce(&gInitOnce, acpTimeOnceInit);

    /* supports monotonic increasement of register clock value */
    /* mread_real_time(&sNow, TIMEBASE_SZ); */
    read_real_time(&sNow, TIMEBASE_SZ);

    /* convert time structure information to real time */
    sRet = time_base_to_time(&sNow, TIMEBASE_SZ);

    if (sRet == 0)
    {
        sTimeDiff = (sNow.tb_high)*ACP_SINT64_LITERAL(1000000000) + sNow.tb_low;
        sRetTime  = gTimeInit.mBaseTime + (sTimeDiff - gTimeInit.mBaseCounter);
    }
    else
    {
        sRetTime = ACP_SINT64_LITERAL(0);
    }
    /* To ensure thread-safety */
    acpSpinLockLock(&gSpinLock);

    if (gPrevTime >= sRetTime)
    {
        sRetTime = gPrevTime + 1;
    }
    else
    {
        /* do nothing, this value is correct. */
    }

    gPrevTime = sRetTime;
    acpSpinLockUnlock(&gSpinLock);


    return (acp_hrtime_t)sRetTime;
#elif defined(ALTI_CFG_OS_TRU64) || defined(ALTI_CFG_OS_SOLARIS)
    struct timespec sTimeSpec;
    acp_sint32_t    sRet;
    acp_hrtime_t    sRetTime;

    sRet = clock_gettime(CLOCK_REALTIME, &sTimeSpec);

    if (sRet == 0)
    {
        sRetTime = (acp_time_t)
            (sTimeSpec.tv_sec*ACP_SINT64_LITERAL(1000000000) +
             sTimeSpec.tv_nsec);
    }
    else
    {
        sRetTime = ACP_SINT64_LITERAL(0);
    }

    /* To ensure thread-safety */
    acpSpinLockLock(&gSpinLock);

    if (gPrevTime >= sRetTime)
    {
        sRetTime = gPrevTime + 1;
    }
    else
    {
        /* do nothing, this value is correct. */
    }

    gPrevTime = sRetTime;
  
    acpSpinLockUnlock(&gSpinLock);

    return (acp_hrtime_t)sRetTime;
#endif
}

ACP_EXPORT acp_uint64_t acpTimeTicksEXPORT(void)
{
    return acpTimeTicks();
}

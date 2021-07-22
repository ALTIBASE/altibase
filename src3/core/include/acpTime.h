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
 * $Id: acpTime.h 11218 2010-06-10 08:13:05Z djin $
 ******************************************************************************/

#if !defined(_O_ACP_TIME_H_)
#define _O_ACP_TIME_H_

/**
 * @file
 * @ingroup CoreSys
 */

#include <acpTest.h>
#include <acpError.h>
#include <acpMemBarrier.h>

ACP_EXTERN_C_BEGIN


/**
 * number of microseconds since 00:00:00 january 1, 1970 UTC
 */
typedef acp_sint64_t acp_time_t;

/**
 * number of nanoseconds since 00:00:00 january 1, 1970 UTC
 */
typedef acp_sint64_t acp_hrtime_t;

/**
 * structure for broken-out time information
 * @see acpTimeGetLocalTime() acpTimeGetGmTime()
 */
typedef struct acp_time_exp_t
{
    acp_sint32_t mUsec;      /**< microseconds (0 - 999,999) */
    acp_sint32_t mSec;       /**< seconds (0 - 60)           */
    acp_sint32_t mMin;       /**< minutes (0 - 59)           */
    acp_sint32_t mHour;      /**< hours (0 - 23)             */
    acp_sint32_t mDay;       /**< day of month (1 - 31)      */
    acp_sint32_t mMonth;     /**< month of year (1 - 12)     */
    acp_sint32_t mYear;      /**< year                       */
    acp_sint32_t mDayOfWeek; /**< day of week (Sunday = 0)   */
    acp_sint32_t mDayOfYear; /**< day of year (1 - 366)      */
} acp_time_exp_t;

/**
 * indicates whether the time value is absolute or relative
 */
typedef enum acp_time_type_t
{
    ACP_TIME_ABS = 0, /**< the time value is absolute */
    ACP_TIME_REL      /**< the time value is relative */
} acp_time_type_t;

/**
 * no-wait timeout value
 */
#define ACP_TIME_IMMEDIATE ((acp_time_t)0)
/**
 * infinite timeout value
 */
#define ACP_TIME_INFINITE  ((acp_time_t)ACP_SINT64_MAX)

/*
 * MICRO SECONDS BASED FUNCTIONS
 */

/**
 * gets the second portion of the time value
 *
 * @param aTime microtime
 * @return second portion of time
 */
ACP_INLINE acp_sint64_t acpTimeGetSec(acp_time_t aTime)
{
    return ((aTime / ACP_SINT64_LITERAL(1000000)) % ACP_SINT64_LITERAL(60));
}

/**
 * gets the millisecond portion of the time value
 *
 * @param aTime microtime
 * @return millisecond portion of time
 */
ACP_INLINE acp_sint64_t acpTimeGetMsec(acp_time_t aTime)
{
    return ((aTime / ACP_SINT64_LITERAL(1000)) % ACP_SINT64_LITERAL(1000));
}

/**
 * gets the microsecond portion of the time value
 *
 * @param aTime microtime
 * @return microsecond portion of time
 */
ACP_INLINE acp_sint64_t acpTimeGetUsec(acp_time_t aTime)
{
    return (aTime % ACP_SINT64_LITERAL(1000000));
}

/**
 * gets the time value in seconds
 *
 * @param aTime microtime
 * @return second
 */
ACP_INLINE acp_sint64_t acpTimeToSec(acp_time_t aTime)
{
    return (aTime / ACP_SINT64_LITERAL(1000000));
}

/**
 * gets the time value in milliseconds
 *
 * @param aTime microtime
 * @return millisecond
 */
ACP_INLINE acp_sint64_t acpTimeToMsec(acp_time_t aTime)
{
    return (aTime / ACP_SINT64_LITERAL(1000));
}

/**
 * gets the time value in microseconds
 *
 * @param aTime microtime
 * @return microsecond
 */
ACP_INLINE acp_sint64_t acpTimeToUsec(acp_time_t aTime)
{
    return aTime;
}

/**
 * makes time value from seconds
 *
 * @param aSec second
 * @return time
 */
ACP_INLINE acp_time_t acpTimeFromSec(acp_sint64_t aSec)
{
    return ((acp_time_t)aSec * ACP_SINT64_LITERAL(1000000));
}

/**
 * makes time value from milliseconds
 *
 * @param aMsec millisecond
 * @return time
 */
ACP_INLINE acp_time_t acpTimeFromMsec(acp_sint64_t aMsec)
{

    return ((acp_time_t)aMsec * ACP_SINT64_LITERAL(1000));
}

/**
 * makes time value from microseconds
 *
 * @param aUsec microsecond
 * @return time
 */
ACP_INLINE acp_time_t acpTimeFromUsec(acp_sint64_t aUsec)
{
    return (acp_time_t)aUsec;
}

/**
 * makes time value from seconds and microseconds
 *
 * @param aSec second
 * @param aUsec microsecond
 * @return time
 */
ACP_INLINE acp_time_t acpTimeFrom(acp_sint64_t aSec, acp_sint64_t aUsec)
{
    return (acp_time_t)(aSec * ACP_SINT64_LITERAL(1000000) + aUsec);
}

/*
 * NANO SECONDS BASED FUNCTIONS
 */

/**
 * gets the second portion of the time value
 *
 * @param aTime nanotime
 * @return second portion of time
 */
ACP_INLINE acp_sint64_t acpTimeHiResGetSec(acp_hrtime_t aTime)
{
    return (aTime / ACP_SINT64_LITERAL(1000000000));
}

/**
 * gets the millisecond portion of the time value
 *
 * @param aTime nanotime
 * @return millisecond portion of time
 */
ACP_INLINE acp_sint64_t acpTimeHiResGetMsec(acp_hrtime_t aTime)
{
    return ((aTime / ACP_SINT64_LITERAL(1000000))
            % ACP_SINT64_LITERAL(1000));
}

/**
 * gets the microsecond portion of the time value
 *
 * @param aTime nanotime
 * @return microsecond portion of time
 */
ACP_INLINE acp_sint64_t acpTimeHiResGetUsec(acp_hrtime_t aTime)
{
    return ((aTime / ACP_SINT64_LITERAL(1000))
            % ACP_SINT64_LITERAL(1000000));
}

/**
 * gets the time value in seconds
 *
 * @param aTime nanotime
 * @return second
 */
ACP_INLINE acp_sint64_t acpTimeHiResToSec(acp_hrtime_t aTime)
{
    return (aTime / ACP_SINT64_LITERAL(1000000000));
}

/**
 * gets the time value in milliseconds
 *
 * @param aTime nanotime
 * @return millisecond
 */
ACP_INLINE acp_sint64_t acpTimeHiResToMsec(acp_hrtime_t aTime)
{
    return (aTime / ACP_SINT64_LITERAL(1000000));
}

/**
 * gets the time value in microseconds
 *
 * @param aTime nanotime
 * @return microsecond
 */
ACP_INLINE acp_sint64_t acpTimeHiResToUsec(acp_hrtime_t aTime)
{
    return (aTime / ACP_SINT64_LITERAL(1000));
}

/**
 * gets the time value in nanoseconds
 *
 * @param aTime time
 * @return nanosecond
 */
ACP_INLINE acp_sint64_t acpTimeHiResToNsec(acp_hrtime_t aTime)
{
    return aTime;
}

/**
 * makes time value from seconds
 *
 * @param aSec second
 * @return time
 */
ACP_INLINE acp_hrtime_t acpTimeHiResFromSec(acp_sint64_t aSec)
{
    return ((acp_hrtime_t)aSec * ACP_SINT64_LITERAL(1000000000));
}

/**
 * makes time value from milliseconds
 *
 * @param aMsec millisecond
 * @return time
 */
ACP_INLINE acp_hrtime_t acpTimeHiResFromMsec(acp_sint64_t aMsec)
{
    return ((acp_hrtime_t)aMsec * ACP_SINT64_LITERAL(1000000));
}

/**
 * makes time value from microseconds
 *
 * @param aUsec microsecond
 * @return time
 */
ACP_INLINE acp_hrtime_t acpTimeHiResFromUsec(acp_sint64_t aUsec)
{
    return ((acp_hrtime_t)aUsec * ACP_SINT64_LITERAL(1000));
}

/**
 * makes time value from seconds and microseconds
 *
 * @param aSec second
 * @param aNsec nanosecond
 * @return time
 */
ACP_INLINE acp_hrtime_t acpTimeHiResFrom(acp_sint64_t aSec, acp_sint64_t aNsec)
{
    return (acp_hrtime_t)(aSec * ACP_SINT64_LITERAL(1000000000) + aNsec);
}

/**
 * return the current high resolution time (nano seconds)
 *
 * @return current time
 */
ACP_EXPORT acp_hrtime_t acpTimeHiResNow(void);

/**
 * The clock-based #acpTimeHiResNow() sometimes returns unstable time value.
 * Developers can call this function to fix it correctly. (this function call
 * is the expensive one.)
 */
ACP_EXPORT void acpTimeHiResRevise(void);

/* REFERENCES :
 *   http://peter.kuscsik.com/wordpress/?p=14
 *   http://www.fftw.org/cycle.h
 */
/**
 * get cpu cycles(ticks) from time stamp counters.
 *
 * @return the current time stamp counter
 */

#if defined(ALTI_CFG_OS_WINDOWS)
ACP_INLINE acp_uint64_t acpTimeTicks(void)
{
    acp_uint64_t sRet = 0;
    LARGE_INTEGER sTicks;
    (void)QueryPerformanceCounter(&sTicks);
    sRet = (acp_uint64_t)sTicks.QuadPart;
    ACP_MEM_BARRIER();
    return sRet;
}

#else

ACP_EXPORT acp_uint64_t acpTimeTicksEXPORT(void);

# if defined(__GNUC__) && !defined(ACP_CFG_NOINLINE)

ACP_INLINE acp_uint64_t acpTimeTicks(void)
{
    acp_uint64_t sRet = 0;

#if defined(ALTI_CFG_CPU_X86) || defined(ALTI_CFG_CPU_X86_64)
#  if defined(ACP_CFG_COMPILE_32BIT)
    __asm__ __volatile__ ("rdtsc" : "=A"(sRet));
#  elif defined(ACP_CFG_COMPILE_64BIT)
    acp_uint32_t sLow;
    acp_uint32_t sHigh;

    __asm__ __volatile__ ("rdtsc" :"=a"(sLow),"=d"(sHigh));

    sRet  = (acp_uint64_t)sHigh << 32;
    sRet |= (acp_uint64_t)sLow;
#  endif  /* ALTI_CFG_BITTYPE */
#elif  defined(ALTI_CFG_CPU_IA64)
     __asm__ __volatile__ ("mov %0=ar.itc" : "=r"(sRet));
#elif defined(ALTI_CFG_CPU_POWERPC)
     acp_uint32_t sLow;
     acp_uint32_t sHigh;
     acp_uint32_t sTmp;

     do
     {
         __asm__ __volatile__ ("mftbu %0" : "=r"(sHigh));
         __asm__ __volatile__ ("mftb %0" : "=r"(sLow));
         __asm__ __volatile__ ("mftbu %0" : "=r"(sTmp));
     } while (sHigh != sTmp);

     sRet  = (acp_uint64_t)sHigh << 32;
     sRet |= (acp_uint64_t)sLow;
#elif defined(ALTI_CFG_CPU_SPARC)
     __asm__ __volatile__ (".byte 0x83, 0x41, 0x00, 0x00");
     __asm__ __volatile__ ("mov   %%g1, %0" : "=r" (sRet));
#elif defined(ALTI_CFG_CPU_ALPHA)
     __asm__ __volatile__ ("rpcc %0" : "=r" (sRet));

     /* alpha supports only 32bit cycle counter
      * lower 32 bit : cycles used by the current process
      * upper 32 bit : wall clock cycles
      */
     sRet = sRet & 0xffffffff;
#elif defined(ALTI_CFG_CPU_PARSIC)
     __asm__ __volatile__("mfctl 16, %0": "=r" (sRet));
#endif  /* ALTI_CFG_CPU_X86 */

     ACP_MEM_BARRIER();

     return sRet;
}

# else /* Not GNU-C or (GNU-C with NOINLINE directive) */

ACP_INLINE acp_uint64_t acpTimeTicks(void)
{
    return acpTimeTicksEXPORT();
}

# endif
#endif

#if defined(ALTI_CFG_OS_WINDOWS) && !defined(ACP_CFG_DOXYGEN)

ACP_EXPORT acp_time_t acpTimeNow(void);

ACP_EXPORT void       acpTimeGetLocalTime(acp_time_t      aTime,
                                          acp_time_exp_t *aLocalTime);
ACP_EXPORT void       acpTimeGetGmTime(acp_time_t      aTime,
                                       acp_time_exp_t *aGmTime);
ACP_EXPORT acp_rc_t   acpTimeMakeTime(const acp_time_exp_t* aExpTime,
                                      acp_time_t*           aTime);

#else

/**
 * returns the current time since Epoch (00:00:00 UTC, January 1, 1970)
 * measured in microseconds
 *
 * @return current time in microseconds
 */
ACP_INLINE acp_time_t acpTimeNow(void)
{
    struct timeval sTimeVal;
    struct timezone sTimeZone;
    acp_time_t     sRet;

    sRet = gettimeofday(&sTimeVal, &sTimeZone);

    if (sRet == 0)
    {
        sRet = (acp_time_t)(sTimeVal.tv_sec * ACP_SINT64_LITERAL(1000000) +
                            sTimeVal.tv_usec);
#if !defined( ALTI_CFG_OS_SOLARIS )
        /*
         * On Solaris gettimeofday() ignores the second argument.
         */
        sRet += (sTimeZone.tz_dsttime * 3600);
#endif
        return sRet;
    }
    else
    {
        return ACP_SINT64_LITERAL(0);
    }
}

/**
 * expands @a aTime to the #acp_time_exp_t structure in current timezone
 *
 * @param aTime microtime
 * @param aLocalTime pointer to the #acp_time_exp_t
 */
ACP_INLINE void acpTimeGetLocalTime(acp_time_t      aTime,
                                    acp_time_exp_t *aLocalTime)
{
    struct tm sTm;
    time_t    sTime = (time_t)acpTimeToSec(aTime);

    (void)localtime_r(&sTime, &sTm);

    aLocalTime->mUsec      = (acp_sint32_t)acpTimeGetUsec(aTime);
    aLocalTime->mSec       = sTm.tm_sec;
    aLocalTime->mMin       = sTm.tm_min;
    aLocalTime->mHour      = sTm.tm_hour;
    aLocalTime->mDay       = sTm.tm_mday;
    aLocalTime->mMonth     = sTm.tm_mon  + 1;
    aLocalTime->mYear      = sTm.tm_year + 1900;
    aLocalTime->mDayOfWeek = sTm.tm_wday;
    aLocalTime->mDayOfYear = sTm.tm_yday + 1;
}

/**
 * expands @a aTime to the #acp_time_exp_t structure without timezone adjustment
 *
 * @param aTime microtime
 * @param aGmTime pointer to the #acp_time_exp_t
 */
ACP_INLINE void acpTimeGetGmTime(acp_time_t      aTime,
                                 acp_time_exp_t *aGmTime)
{
    struct tm sTm;
    time_t    sTime = (time_t)acpTimeToSec(aTime);

    (void)gmtime_r(&sTime, &sTm);

    aGmTime->mUsec      = (acp_sint32_t)acpTimeGetUsec(aTime);
    aGmTime->mSec       = sTm.tm_sec;
    aGmTime->mMin       = sTm.tm_min;
    aGmTime->mHour      = sTm.tm_hour;
    aGmTime->mDay       = sTm.tm_mday;
    aGmTime->mMonth     = sTm.tm_mon  + 1;
    aGmTime->mYear      = sTm.tm_year + 1900;
    aGmTime->mDayOfWeek = sTm.tm_wday;
    aGmTime->mDayOfYear = sTm.tm_yday + 1;
}

/**
 * converts @a aGmTime to the #acp_time_t without timezone adjustment
 *
 * @param aGmTime pointer to the #acp_time_exp_t
 * valued in Greenwich mean time(GMT).
 * @param aTime pointer to store converted microtime
 * @return #ACP_RC_EFAULT when aGmTime or aTime is NULL
 *         #ACP_RC_EINVAL when aGmTime is not valid
 */
ACP_INLINE acp_rc_t acpTimeMakeTime(const acp_time_exp_t* aGmTime,
                                    acp_time_t*           aTime)
{
    struct tm  sTm;
    acp_time_t sTime;
    acp_rc_t   sRC;

    ACP_TEST_RAISE(NULL == aTime, HANDLE_FAULT);
    
    sTm.tm_sec          = aGmTime->mSec;
    sTm.tm_min          = aGmTime->mMin;
    sTm.tm_hour         = aGmTime->mHour;
    sTm.tm_mday         = aGmTime->mDay;
    sTm.tm_mon          = aGmTime->mMonth - 1;
    sTm.tm_year         = aGmTime->mYear  - 1900;
    sTm.tm_isdst        = -1;

    sTime = (acp_time_t)mktime(&sTm);
    ACP_TEST_RAISE(-1 == sTime, INVALID_GMTIME);

    sTime = sTime * ACP_SINT64_LITERAL(1000000) + aGmTime->mUsec;
    *aTime = (acp_time_t)sTime;
    return ACP_RC_SUCCESS;

    ACP_EXCEPTION(HANDLE_FAULT)
    {
        sRC = ACP_RC_EFAULT;
    }

    ACP_EXCEPTION(INVALID_GMTIME)
    {
        sRC = ACP_RC_EINVAL;
    }

    ACP_EXCEPTION_END;
    return sRC;
}

#endif

ACP_EXTERN_C_END

#endif

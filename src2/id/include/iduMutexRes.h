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
 
/***********************************************************************
 * $Id: $
 **********************************************************************/

/* ------------------------------------------------
 *      !!!!!!!!!!!!!WARNING!!!!!!!!!!!!!!
 *
 *  이 헤더화일은 C 소스코드와 호환이 되도록
 *  구현되어야 합니다.
 *  This source file should be compatible with
 *  C source, not C++.
 * ----------------------------------------------*/


/***********************************************************************
 * BUG-32511
 * Reducing the sizeof iduMutexEntry and iduLatchObj
 * to fit the size to the minimum that each platform needs.
 **********************************************************************/

#if !defined(_O_IDU_MUTEXRES_H_)
#define _O_IDU_MUTEXRES_H_

#define IDU_MUTEX_SLEEP (0)
#define IDU_MUTEX_YIELD (1)

#include <idTypes.h>

#if defined(ALTI_CFG_CPU_PARISC)
typedef struct iduNativeMutexObj
{
    int     sema[4];
} iduNativeMutexObj;
# else /* ALTI_CFG_CPU_PARISC */
typedef UInt iduNativeMutexObj;
# endif

/***********************************************************************
 * BUGBUG:
 * As there are C++ datatypes defined in PDL layer,
 * iduMutexResNative and iduMutexResPOSIX should be declared
 * for C and C++ with the same size.
 * This should be fixed, as a system header is included.
 * Substitution of PD layer with Core would be a solution.
 **********************************************************************/

#if defined(__cplusplus)

#include <idvTime.h>
#include <iduBridgeTime.h>

typedef struct iduMutexResNative
{
    iduNativeMutexObj  mMutexObj;        /* Native Mutex Object              */
    idBool             mIsLockTimeValid; /* mLastLockedTime이 유효한 값인지*/
    iduBridgeTime      mLastLockedTime;  /* 최근에 Lock잡은 시각           */
} iduMutexResNative;

typedef struct iduMutexResPOSIX
{
    PDL_thread_mutex_t mMutex;           /* Mutex 핸들 */
    idBool             mIsLockTimeValid; /* mLastLockedTime이 유효한 값인지 */
    idvTime            mLastLockedTime;  /* 최근에 Lock잡은 시각 */
} iduMutexResPOSIX;

#else /* cplusplus */

typedef struct iduMutexResNative
{
    iduNativeMutexObj  mMutexObj;
    idBool             mIsLockTimeValid;

    /* copied from iduBridgeTime.h BEGIN */
    struct {
        ULong mIdvTime[4];
    }  mLastLockedTime;
    /* copied from iduBridgeTime.h END */
} iduMutexResNative;

/***********************************************************************
 * BUGBUG:
 * PDL_thread_mutex_t is a C++ datatype!!!
 * Declaring once more and should be fixed.
 * This source was copied from OS.h
 **********************************************************************/

# if defined(ALTI_CFG_OS_WINDOWS)
typedef CRITICAL_SECTION PDL_thread_mutex_t;
# else
#  include <pthread.h>
#  include <sys/time.h>
typedef pthread_mutex_t PDL_thread_mutex_t;
# endif

typedef struct iduMutexResPOSIX
{
    PDL_thread_mutex_t mMutex;
    idBool             mIsLockTimeValid;
    /* copied from idvTime.h BEGIN */
# if defined(ALTI_CFG_OS_SOLARIS) || defined(ALTI_CFG_OS_HPUX)
    union {
        struct timespec mSpec;
        ULong       	mClock;
    } mLastLockedTime;
# elif defined(ALTI_CFG_OS_LINUX)
    union {
        struct timespec mSpec;
        ULong       	mClock;
    } mLastLockedTime;
# elif defined(ALTI_CFG_OS_AIX)
    union {
        timebasestruct_t    mSpec;
        ULong               mClock;
    } mLastLockedTime;
#else
    union {
        ULong               mClock;
    } mLastLockedTime;
# endif
    /* copied from idvTime.h END */
} iduMutexResPOSIX;

#endif /* cplusplus */

typedef union iduMutexRes
{
    iduMutexResNative   mNative;
    iduMutexResPOSIX    mPosix;
} iduMutexRes;

/* End of header */
#endif

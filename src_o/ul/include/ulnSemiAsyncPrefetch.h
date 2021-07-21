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

#ifndef _ULN_SEMI_ASYNC_PREFETCH_H_
#define _ULN_SEMI_ASYNC_PREFETCH_H_ 1

#include <uln.h>

typedef enum ulnAutoTuningState
{
    ULN_AUTO_TUNING_STATE_INIT,
    ULN_AUTO_TUNING_STATE_TUNING,
    ULN_AUTO_TUNING_STATE_STABLE,
    ULN_AUTO_TUNING_STATE_OPTIMAL,
    ULN_AUTO_TUNING_STATE_FLOODED,
    ULN_AUTO_TUNING_STATE_RESET,
    ULN_AUTO_TUNING_STATE_FAILED,
    ULN_AUTO_TUNING_STATE_END,
    ULN_AUTO_TUNING_STATE_MAX
} ulnAutoTuningState;

typedef enum ulnAutoTuningDecision
{
    ULN_AUTO_TUNING_DECISION_UNKNOWN  = -2,
    ULN_AUTO_TUNING_DECISION_INCREASE = -1,
    ULN_AUTO_TUNING_DECISION_STABLE   =  0,
    ULN_AUTO_TUNING_DECISION_DECREASE =  1,
} ulnAutoTuningDecision;

#define ULN_AUTO_TUNING_PREFETCH_ROWS_MIN            ULN_STMT_PREFETCH_AUTO_TUNING_MIN_DEFAULT
#define ULN_AUTO_TUNING_PREFETCH_ROWS_MAX            ULN_STMT_PREFETCH_AUTO_TUNING_MAX_DEFAULT

#define ULN_AUTO_TUNING_EPSILON                      (0.0000001)

/* semi-async prefetch auto-tuning parameters */
#define ULN_AUTO_TUNING_R_THRESHOLD_MIN              (1)
#define ULN_AUTO_TUNING_R_THRESHOLD_MAX              (3)
#define ULN_AUTO_TUNING_R_THRESHOLD_MAX_ON_OPTIMAL   (4)
#define ULN_AUTO_TUNING_OPTIMAL_THRESHOLD            (10)
#define ULN_AUTO_TUNING_R_0_IN_OPTIMAL_STATE         (3)
#define ULN_AUTO_TUNING_INCREASE_RATIO               (1.5)
#define ULN_AUTO_TUNING_DF_RATIO_MIN_PREFETCH_TIME   (10)
#define ULN_AUTO_TUNING_DF_RATIO_STABLE_THRESHOLD    (0.85)
#define ULN_AUTO_TUNING_FLOOD_THRESHOLD              (5)
#define ULN_AUTO_TUNING_FETCHED_BYTES_MAX            (2 * 1024 * 1024)

#if !defined(ALTI_CFG_OS_LINUX)
/* stub structure to compile under non-Linux including AIX, HP-UX, Solaris, Windows */
struct tcp_info
{
    acp_uint32_t tcpi_last_data_recv;
};
#endif /* not ALTI_CFG_OS_LINUX */

/**
 * semi-async prefetch auto-tuning
 *
 * [notation]
 *   d       : application load time (ms)
 *   f       : prefetch time (ms)
 *   r       : elapsed time from last data receive, network idle time (ms)
 *             r = { d - f    , if d > f  }
 *                 { 0        , otherwise }
 *   k       : socket read time from socket receive buffer (ms)
 *   t       : time
 *             (e.g. t-1 = previous time
 *                   t   = current time
 *                   t+1 = next time)
 * x_(t+1)   : predicted prefetch rows
 */

typedef struct ulnSemiAsyncPrefetchAutoTuning
{
    ulnStmt              *mStmt;

    /* statistics */
    acp_time_t            mBeginTimeToMeasure;     /* d, f time 을 측정하기 위한 시작 시간 */
    acp_time_t            mEndTimeToMeasure;       /* d, f time 을 측정하기 위한 끝 시간 */
    acp_sint64_t          mTotalElapsedTime;       /* d + k time */
    acp_sint64_t          mAppLoadTime;            /* d time */
    acp_sint64_t          mPrefetchTime;           /* f time */
    acp_double_t          mDFRatio;                /* d / f ratio */
    ulnAutoTuningDecision mDecision;               /* prefetch rows 증/감 결정 */
    acp_sint32_t          mAppReadRows;            /* d time 동안 application 에서 read 한 rows */
    acp_uint32_t          mFetchedRows;            /* f time 동안 fetched rows */
    acp_uint32_t          mNetworkIdleTime;        /* r time */
    acp_sint64_t          mSockReadTime;           /* k time */
    struct tcp_info       mTcpInfo;                /* TCP statistics in kernel */

    /* state */
    ulnAutoTuningState    mState;                  /* auto-tuning state */
    acp_uint32_t          mPrefetchRows;           /* x_(t+1) : 예측된 prefetch rows */
    acp_uint32_t          mLastStablePrefetchRows; /* 마지막으로 stable 한 prefetch rows */
    acp_bool_t            mIsTrialOneMore;         /* 정확한 feedback 을 위해 한번 더 적용할지의 여부 */
    acp_uint32_t          mDFRatioUnstableCount;   /* 폭주 상태를 검사하기 위해 d / f 비율상 unstable 회수 */
    acp_uint32_t          mConsecutiveCount;       /* 동일한 prefetch rows 로 예측한 연속 회수 */
    acp_uint32_t          mRZeroCountOnOptimal;    /* optimal 상태에서 r = 0 회수 */

    acp_bool_t            mIsAdjustedSockRcvBuf;   /* socket receive buffer 를 조절하였는지의 여부 */
    acp_uint32_t          mLastAdjustedSockRcvBufSize; /* auto-tuned prefetch rows 에 의해 마지막으로 조절된 socket receive buffer size */

    /* staticstics of last prediction */
    ulnAutoTuningState    mLastState;              /* last auto-tuning state */
    acp_uint32_t          mLastPrefetchRows;       /* last auto-tuned prefetch rows */
    acp_sint32_t          mLastCachedRows;         /* last cached row count */
    acp_sint32_t          mLastFetchedRows;        /* last fetched rows */
    acp_sint64_t          mLastAppLoadTime;        /* last d time */
    acp_sint64_t          mLastPrefetchTime;       /* last f time */
    acp_uint32_t          mLastNetworkIdleTime;    /* last r time */
    acp_sint64_t          mLastSockReadTime;       /* last k time */
    ulnAutoTuningDecision mLastDecision;           /* last decision */
} ulnSemiAsyncPrefetchAutoTuning;

/**
 * semi-async prefetch
 */
struct ulnSemiAsyncPrefetch
{
    acp_bool_t                     mIsAsyncSentFetch;

    ulnSemiAsyncPrefetchAutoTuning mAutoTuning;
};

/**
 * prefetch async. trace logging
 */
typedef struct ulnPrefetchAsyncStatTrcLog
{
    acp_bool_t      mIsTrcLogFile;
    acp_bool_t      mIsTrcLogStdOut;
    acp_char_t      mFileName[ACP_PATH_MAX_LENGTH + 1];
    acp_size_t      mEnvFileNameOffset;
    acp_thr_mutex_t mMutex;
} ulnPrefetchAsyncStatTrcLog;

extern ulnPrefetchAsyncStatTrcLog gPrefetchAsyncStatTrcLog;

ACI_RC ulnAllocSemiAsyncPrefetch(ulnSemiAsyncPrefetch **aSemiAsyncPrefetch);

void ulnFreeSemiAsyncPrefetch(ulnSemiAsyncPrefetch *aSemiAsyncPrefetch);

void ulnInitSemiAsyncPrefetch(ulnStmt *aStmt);

ACP_INLINE ulnSemiAsyncPrefetch *ulnGetSemiAsyncPrefetch(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    return sStmt->mSemiAsyncPrefetch;
}

ACP_INLINE ulnSemiAsyncPrefetchAutoTuning *ulnGetSemiAsyncPrefetchAutoTuning(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    ACE_DASSERT(sStmt != NULL);
    ACE_DASSERT(sStmt->mSemiAsyncPrefetch != NULL);

    return &sStmt->mSemiAsyncPrefetch->mAutoTuning;
}

ACP_INLINE void ulnSetAsyncSentFetch(ulnFnContext *aFnContext,
                                     acp_bool_t    aIsAsyncSentFetch)
{
    ulnSemiAsyncPrefetch *sSemiAsyncPrefetch = ulnGetSemiAsyncPrefetch(aFnContext);

    sSemiAsyncPrefetch->mIsAsyncSentFetch = aIsAsyncSentFetch;
}

ACP_INLINE acp_bool_t ulnIsAsyncSentFetch(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetch *sSemiAsyncPrefetch = ulnGetSemiAsyncPrefetch(aFnContext);

    ACI_TEST(sSemiAsyncPrefetch == NULL);

    return sSemiAsyncPrefetch->mIsAsyncSentFetch;

    ACI_EXCEPTION_END;

    return ACP_FALSE;
}

acp_bool_t ulnCanSemiAsyncPrefetch(ulnFnContext *aFnContext);

acp_bool_t ulnCanSemiAsyncPrefetchAutoTuning(ulnFnContext *aFnContext);

void ulnFetchBeginAutoTuning(ulnFnContext *aFnContext);

ACI_RC ulnFetchAutoTuning(ulnFnContext *aFnContext,
                          acp_uint32_t  aFetchedRows,
                          acp_uint32_t *aPredictedPrefetchRows);

ACP_INLINE acp_uint32_t ulnFetchGetAutoTunedPrefetchRows(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    return sAutoTuning->mPrefetchRows;
}

acp_bool_t ulnFetchIsAutoTunedSockRcvBuf(ulnDbc *aDbc);

void ulnFetchAutoTuningBeforeReceive(ulnFnContext *aFnContext);

void ulnFetchSkipAutoTuning(ulnFnContext *aFnContext);

void ulnFetchNextAutoTuning(ulnFnContext *aFnContext);

ACI_RC ulnFetchEndAutoTuning(ulnFnContext *aFnContext);

void ulnInitTraceLogPrefetchAsyncStat();
void ulnFinalizeTraceLogPrefetchAsyncStat();

void ulnTraceLogPrefetchAsyncStat(ulnFnContext     *aFnContext,
                                  const acp_char_t *aFormat,
                                  ...);

void ulnTraceLogPrefetchAsyncStatV(ulnFnContext     *aFnContext,
                                   const acp_char_t *aFormat,
                                   va_list           aArgs);

ACP_INLINE acp_bool_t ulnIsPrefetchAsyncStatTrcLog()
{
    return (gPrefetchAsyncStatTrcLog.mIsTrcLogFile   == ACP_TRUE) ||
           (gPrefetchAsyncStatTrcLog.mIsTrcLogStdOut == ACP_TRUE);
}

#define ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, aFormat, ...)               \
    do {                                                                       \
        if (ACP_LIKELY_FALSE(ulnIsPrefetchAsyncStatTrcLog() == ACP_TRUE)) {    \
            ulnTraceLogPrefetchAsyncStat(aFnContext, aFormat, ##__VA_ARGS__);  \
        }                                                                      \
        else {                                                                 \
            /* nothing to do */                                                \
        }                                                                      \
    } while (0)

#endif /* _ULN_SEMI_ASYNC_PREFETCH_H_ */


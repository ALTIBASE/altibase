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

#include <uln.h>
#include <ulnPrivate.h>

#include <ulnCache.h>
#include <ulnFetch.h>
#include <ulnFetchOpAsync.h>
#include <ulnSemiAsyncPrefetch.h>

typedef ACI_RC (*ulnFetchAutoTuningStateFunc)(ulnFnContext *);

static ACI_RC ulnFetchAutoTuningStateFuncInit   (ulnFnContext *);
static ACI_RC ulnFetchAutoTuningStateFuncTuning (ulnFnContext *);
static ACI_RC ulnFetchAutoTuningStateFuncFlooded(ulnFnContext *);
static ACI_RC ulnFetchAutoTuningStateFuncReset  (ulnFnContext *);
static ACI_RC ulnFetchAutoTuningStateFuncEnd    (ulnFnContext *);
static ACI_RC ulnFetchAutoTuningStateFuncAssert (ulnFnContext *);

static struct
{
    acp_char_t                  mStateChar; /* for trace logging */
    ulnFetchAutoTuningStateFunc mStateFunc;
} gAutoTuningStateTable[] = {
    { /* ULN_AUTO_TUNING_STATE_INIT    */ 'I', ulnFetchAutoTuningStateFuncInit    },
    { /* ULN_AUTO_TUNING_STATE_TUNING  */ 'T', ulnFetchAutoTuningStateFuncTuning  },
    { /* ULN_AUTO_TUNING_STATE_STABLE  */ 'S', ulnFetchAutoTuningStateFuncTuning  },
    { /* ULN_AUTO_TUNING_STATE_OPTIMAL */ 'O', ulnFetchAutoTuningStateFuncTuning  },
    { /* ULN_AUTO_TUNING_STATE_FLOODED */ 'F', ulnFetchAutoTuningStateFuncFlooded },
    { /* ULN_AUTO_TUNING_STATE_RESET   */ 'R', ulnFetchAutoTuningStateFuncReset   },
    { /* ULN_AUTO_TUNING_STATE_FAILED  */ 'X', ulnFetchAutoTuningStateFuncAssert  },
    { /* ULN_AUTO_TUNING_STATE_END     */ 'E', ulnFetchAutoTuningStateFuncEnd     }
};

ulnPrefetchAsyncStatTrcLog gPrefetchAsyncStatTrcLog;

ACI_RC ulnAllocSemiAsyncPrefetch(ulnSemiAsyncPrefetch **aSemiAsyncPrefetch)
{
    ACE_DASSERT(aSemiAsyncPrefetch != NULL);

    ACI_TEST(acpMemCalloc((void **)aSemiAsyncPrefetch,
                          ACI_SIZEOF(ulnSemiAsyncPrefetch),
                          8) != ACP_RC_SUCCESS);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

void ulnFreeSemiAsyncPrefetch(ulnSemiAsyncPrefetch *aSemiAsyncPrefetch)
{
    ACE_DASSERT(aSemiAsyncPrefetch != NULL);

    acpMemFree(aSemiAsyncPrefetch);
    aSemiAsyncPrefetch = NULL;
}

static void ulnInitPrefetchAutoTuning(ulnSemiAsyncPrefetchAutoTuning *aAutoTuning,
                                      ulnStmt                        *aStmt)
{
    ACE_DASSERT(aAutoTuning != NULL);
    ACE_DASSERT(aStmt != NULL);

    aAutoTuning->mStmt                       = aStmt;

    aAutoTuning->mBeginTimeToMeasure         = 0;
    aAutoTuning->mEndTimeToMeasure           = 0;
    aAutoTuning->mTotalElapsedTime           = 0;
    aAutoTuning->mAppLoadTime                = 0;
    aAutoTuning->mPrefetchTime               = 0;
    aAutoTuning->mDFRatio                    = 0;
    aAutoTuning->mDecision                   = ULN_AUTO_TUNING_DECISION_UNKNOWN;
    aAutoTuning->mAppReadRows                = 0;
    aAutoTuning->mFetchedRows                = 0;
    aAutoTuning->mNetworkIdleTime            = 0;
    aAutoTuning->mSockReadTime               = 0;

    acpMemSet(&aAutoTuning->mTcpInfo, 0, ACI_SIZEOF(aAutoTuning->mTcpInfo));

    aAutoTuning->mState                      = ULN_AUTO_TUNING_STATE_INIT;
    aAutoTuning->mPrefetchRows               = ULN_CACHE_OPTIMAL_PREFETCH_ROWS;
    aAutoTuning->mLastStablePrefetchRows     = ULN_CACHE_OPTIMAL_PREFETCH_ROWS;
    aAutoTuning->mIsTrialOneMore             = ACP_FALSE;
    aAutoTuning->mDFRatioUnstableCount       = 0;
    aAutoTuning->mConsecutiveCount           = 0;
    aAutoTuning->mRZeroCountOnOptimal        = 0;

    aAutoTuning->mIsAdjustedSockRcvBuf       = ACP_FALSE;
    aAutoTuning->mLastAdjustedSockRcvBufSize = 0;

    aAutoTuning->mLastState                  = ULN_AUTO_TUNING_STATE_INIT;
    aAutoTuning->mLastPrefetchRows           = ULN_CACHE_OPTIMAL_PREFETCH_ROWS;
    aAutoTuning->mLastCachedRows             = 0;
    aAutoTuning->mLastFetchedRows            = 0;
    aAutoTuning->mLastAppLoadTime            = 0;
    aAutoTuning->mLastPrefetchTime           = 0;
    aAutoTuning->mLastNetworkIdleTime        = 0;
    aAutoTuning->mLastSockReadTime           = 0;
    aAutoTuning->mLastDecision               = ULN_AUTO_TUNING_DECISION_UNKNOWN;
}

void ulnInitSemiAsyncPrefetch(ulnStmt *aStmt)
{
    ACE_DASSERT(aStmt->mSemiAsyncPrefetch != NULL);

    aStmt->mSemiAsyncPrefetch->mIsAsyncSentFetch = ACP_FALSE;

    ulnInitPrefetchAutoTuning(&aStmt->mSemiAsyncPrefetch->mAutoTuning, aStmt);
}

/**
 * Semi-async prefetch 기능이 동작될 수 있는지 여부를 확인한다.
 */
acp_bool_t ulnCanSemiAsyncPrefetch(ulnFnContext *aFnContext)
{
    ulnDbc    *sDbc = NULL;
    ulnStmt   *sStmt = aFnContext->mHandle.mStmt;
    ulnCursor *sCursor = ulnStmtGetCursor(sStmt);

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACI_TEST_RAISE(ulnStmtGetAttrPrefetchAsync(sStmt) != ALTIBASE_PREFETCH_ASYNC_PREFERRED, NOT_ALLOWED);

    ACI_TEST_RAISE(ulnCursorGetType(sCursor) != SQL_CURSOR_FORWARD_ONLY, NOT_ALLOWED);

    ACI_TEST_RAISE(ulnStmtGetAttrConcurrency(sStmt) != SQL_CONCUR_READ_ONLY, NOT_ALLOWED);

    ACI_TEST_RAISE((ulnDbcGetCmiLinkImpl(sDbc) != CMI_LINK_IMPL_TCP) &&
                   (ulnDbcGetCmiLinkImpl(sDbc) != CMI_LINK_IMPL_SSL), NOT_ALLOWED);
 
    return ACP_TRUE;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION(NOT_ALLOWED);
    ACI_EXCEPTION_END;

    return ACP_FALSE;
}

/**
 * Auto-tuning 을 동작할 수 있는지 여부를 확인한다. 만약, auto-tuning 도중
 * 에러가 발생하여 auto-tuning 기능을 end 하면 ULN_AUTO_TUNING_STATE_END 상태
 * (last state = ULN_AUTO_TUNING_STATE_FAILED)가 된다.
 */
acp_bool_t ulnCanSemiAsyncPrefetchAutoTuning(ulnFnContext *aFnContext)
{
    ulnStmt *sStmt = aFnContext->mHandle.mStmt;

    ACI_TEST_RAISE(ulnStmtGetAttrPrefetchAutoTuning(sStmt) == ACP_FALSE, NOT_ALLOWED);

    ACI_TEST_RAISE(sStmt->mSemiAsyncPrefetch == NULL, NOT_ALLOWED);

    ACI_TEST_RAISE(sStmt->mSemiAsyncPrefetch->mAutoTuning.mState == ULN_AUTO_TUNING_STATE_FAILED,
                   NOT_ALLOWED);

    ACI_TEST_RAISE(sStmt->mSemiAsyncPrefetch->mAutoTuning.mState == ULN_AUTO_TUNING_STATE_END &&
                   sStmt->mSemiAsyncPrefetch->mAutoTuning.mLastState == ULN_AUTO_TUNING_STATE_FAILED,
                   NOT_ALLOWED);

    return ACP_TRUE;

    ACI_EXCEPTION(NOT_ALLOWED);
    ACI_EXCEPTION_END;

    return ACP_FALSE;
}

/**
 * 사용자가 설정한 auto-tuning 관련 parameter 들을 trace logging 한다.
 */
static void ulnTraceAutoTuningParameters(ulnFnContext *aFnContext)
{
    ulnStmt      *sStmt;
    ulnCache     *sCache;
    acp_uint32_t  sAttrSockRcvBufBlockRatio;
    acp_uint32_t  sAttrPrefetchRows;
    acp_uint32_t  sAttrPrefetchAutoTuningInit;
    acp_uint32_t  sAttrPrefetchAutoTuningMin;
    acp_uint32_t  sAttrPrefetchAutoTuningMax;
    acp_uint32_t  sSingleRowSize;

    ACE_DASSERT(aFnContext != NULL);

    sStmt                       = aFnContext->mHandle.mStmt;
    sCache                      = ulnStmtGetCache(sStmt);
    sAttrSockRcvBufBlockRatio   = ulnDbcGetAttrSockRcvBufBlockRatio(sStmt->mParentDbc);
    sAttrPrefetchRows           = ulnStmtGetAttrPrefetchRows(sStmt);
    sAttrPrefetchAutoTuningInit = ulnStmtGetAttrPrefetchAutoTuningInit(sStmt);
    sAttrPrefetchAutoTuningMin  = ulnStmtGetAttrPrefetchAutoTuningMin(sStmt);
    sAttrPrefetchAutoTuningMax  = ulnStmtGetAttrPrefetchAutoTuningMax(sStmt);
    sSingleRowSize              = ulnCacheGetSingleRowSize(sCache);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : ALTIBASE_SOCK_RCVBUF_BLOCK_RATIO = %"ACI_UINT32_FMT"",
                                   sAttrSockRcvBufBlockRatio);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : ALTIBASE_PREFETCH_ROWS = %"ACI_UINT32_FMT"",
                                   sAttrPrefetchRows);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : ALTIBASE_PREFETCH_AUTO_TUNING_INIT = %"ACI_UINT32_FMT"",
                                   sAttrPrefetchAutoTuningInit);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : ALTIBASE_PREFETCH_AUTO_TUNING_MIN = %"ACI_UINT32_FMT"",
                                   sAttrPrefetchAutoTuningMin);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : ALTIBASE_PREFETCH_AUTO_TUNING_MAX = %"ACI_UINT32_FMT"",
                                   sAttrPrefetchAutoTuningMax);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : estimated single row size = %"ACI_UINT32_FMT"",
                                   sSingleRowSize);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                   "auto-tuning : [ d = application load time, f = prefetch time, r = 0 or d - f, k = socket read time ]",
                                   sSingleRowSize);
}

/**
 * Auto-tuning 통계 정보를 버린다. 단, 이전에 예측한 prefetch rows 와
 * 상태 정보는 유지한다.
 */
static void ulnPurgeStat(ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    ACE_DASSERT(aAutoTuning != NULL);

    /* purge the statistics */
    aAutoTuning->mBeginTimeToMeasure    = 0;
    aAutoTuning->mEndTimeToMeasure      = 0;
    aAutoTuning->mTotalElapsedTime      = 0;
    aAutoTuning->mAppLoadTime           = 0;
    aAutoTuning->mPrefetchTime          = 0;
    aAutoTuning->mDFRatio               = 0;
    aAutoTuning->mAppReadRows           = 0;
    aAutoTuning->mFetchedRows           = 0;
    aAutoTuning->mNetworkIdleTime       = 0;
    aAutoTuning->mSockReadTime          = 0;
    
    acpMemSet(&aAutoTuning->mTcpInfo, 0, ACI_SIZEOF(aAutoTuning->mTcpInfo));

    /* purge the last statistics */
    aAutoTuning->mLastState             = ULN_AUTO_TUNING_STATE_TUNING;
    aAutoTuning->mLastPrefetchRows      = 0;
    aAutoTuning->mLastCachedRows        = 0;
    aAutoTuning->mLastFetchedRows       = 0;
    aAutoTuning->mLastAppLoadTime       = 0;
    aAutoTuning->mLastPrefetchTime      = 0;
    aAutoTuning->mLastNetworkIdleTime   = 0;
    aAutoTuning->mLastSockReadTime      = 0;
    aAutoTuning->mLastDecision          = ULN_AUTO_TUNING_DECISION_UNKNOWN;
}

/**
 * Network idle time (r time) 으로 d time 과 f time 을 비교 추정한다.
 */
static ulnAutoTuningDecision ulnCompareAppLoadAndPrefetchTime(ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    acp_uint32_t sRThresholdMin = ULN_AUTO_TUNING_R_THRESHOLD_MIN;
    acp_uint32_t sRThresholdMax = ULN_AUTO_TUNING_R_THRESHOLD_MAX;

    ACE_DASSERT(aAutoTuning != NULL);

    if (aAutoTuning->mLastState == ULN_AUTO_TUNING_STATE_OPTIMAL)
    {
        sRThresholdMax = ULN_AUTO_TUNING_R_THRESHOLD_MAX_ON_OPTIMAL;
    }
    else
    {
        /* not optimal yet */
    }

    if (aAutoTuning->mNetworkIdleTime < sRThresholdMin)
    {
        return ULN_AUTO_TUNING_DECISION_INCREASE;
    }
    else if (aAutoTuning->mNetworkIdleTime > sRThresholdMax)
    {
        return ULN_AUTO_TUNING_DECISION_DECREASE;
    }

    return ULN_AUTO_TUNING_DECISION_STABLE;
}

/**
 * Auto-tuned prefetch rows 를 기반으로 최대 확보되어야 하는
 * socket receive buffer 를 설정한다.
 */
static ACI_RC ulnAdjustAutoTunedSockRcvBuf(ulnFnContext                   *aFnContext,
                                           ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    ulnDbc       *sDbc;
    ulnCache     *sCache;
    acp_uint32_t  sSingleRowSize;
    acp_double_t  sRowsPerBlock;
    acp_uint32_t  sAutoTunedSockRcvBufSize;

    ACE_DASSERT(aFnContext != NULL);
    ACE_DASSERT(aAutoTuning != NULL);
    ACE_DASSERT(aAutoTuning->mStmt != NULL);
    ACE_DASSERT(aAutoTuning->mStmt->mParentDbc != NULL);

    sDbc           = aAutoTuning->mStmt->mParentDbc,
    sCache         = ulnStmtGetCache(aAutoTuning->mStmt);
    sSingleRowSize = ulnCacheGetSingleRowSize(sCache);
    sRowsPerBlock  = (acp_double_t)CMI_BLOCK_DEFAULT_SIZE / (acp_double_t)sSingleRowSize;

    /**
     * caculate socket receive buffer size that should ensure from auto-tuned prefetch rows
     *
     *                                        n_fetch
     *   s_sock = n_fetch * (5 + s_row) + (------------- + 1) * 16
     *                                      n_per_block
     *
     *   s_sock      : socket receive buffer that should ensure
     *   n_fetch     : prefetch rows
     *   s_row       : single row size
     *   n_per_block : rows per a CM block
     * 
     *   cf) 5  = OpID(1) + row length(4)
     *       16 = CM header size
     *
     */
    sAutoTunedSockRcvBufSize = 
        (aAutoTuning->mPrefetchRows * (5 + sSingleRowSize)) +
        ((aAutoTuning->mPrefetchRows / sRowsPerBlock) + 1) * CMP_HEADER_SIZE;

    /* CM block size is 32K unit */
    if ((sAutoTunedSockRcvBufSize % CMI_BLOCK_DEFAULT_SIZE) > 0)
    {
        sAutoTunedSockRcvBufSize =
            ((sAutoTunedSockRcvBufSize / CMI_BLOCK_DEFAULT_SIZE) + 1) * CMI_BLOCK_DEFAULT_SIZE;
    }
    else
    {
        /* already 32K unit */
    }

    sAutoTunedSockRcvBufSize =
        ACP_MAX(sAutoTunedSockRcvBufSize,
                CMI_BLOCK_DEFAULT_SIZE * ULN_SOCK_RCVBUF_BLOCK_RATIO_DEFAULT);

    if (aAutoTuning->mLastAdjustedSockRcvBufSize != sAutoTunedSockRcvBufSize)
    {
        /* set socket receive buffer size */
        ACI_TEST(cmiSetLinkRcvBufSize(sDbc->mLink,
                                      sAutoTunedSockRcvBufSize) != ACI_SUCCESS);

        ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                       "auto-tuning : socket receive buffer change = %"ACI_INT32_FMT"",
                                       sAutoTunedSockRcvBufSize);

        aAutoTuning->mLastAdjustedSockRcvBufSize = sAutoTunedSockRcvBufSize;
        aAutoTuning->mIsAdjustedSockRcvBuf = ACP_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

#define ULN_SET_PREDICTED_PREFETCH_ROWS(aAutoTuning, aPredictedPrefetchRows)   \
    do {                                                                       \
        if (aPredictedPrefetchRows < ULN_AUTO_TUNING_PREFETCH_ROWS_MIN) {      \
            aAutoTuning->mPrefetchRows = ULN_AUTO_TUNING_PREFETCH_ROWS_MIN;    \
        }                                                                      \
        else if (aPredictedPrefetchRows > ULN_AUTO_TUNING_PREFETCH_ROWS_MAX) { \
            aAutoTuning->mPrefetchRows = ULN_AUTO_TUNING_PREFETCH_ROWS_MAX;    \
        }                                                                      \
        else {                                                                 \
            aAutoTuning->mPrefetchRows = (acp_uint32_t)aPredictedPrefetchRows; \
        }                                                                      \
    }                                                                          \
    while (0)

#define ULN_CHECK_TO_INCREASE_PREFETCH_ROWS(aAutoTuning)                       \
    do {                                                                       \
        if (aAutoTuning->mPrefetchRows < ULN_AUTO_TUNING_PREFETCH_ROWS_MAX &&  \
            aAutoTuning->mPrefetchRows == aAutoTuning->mLastPrefetchRows) {    \
            aAutoTuning->mPrefetchRows++;                                      \
        }                                                                      \
        else {                                                                 \
            /* nothing to do */                                                \
        }                                                                      \
    }                                                                          \
    while (0)

/**
 * d < f (r = 0) 인 경우이므로 prefetch rows 를 증가시킨다.
 */
static void ulnFetchAutoTuningIncrease(ulnFnContext                   *aFnContext,
                                       ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    acp_double_t sPredictedPrefetchRows;

    ACE_DASSERT(aFnContext != NULL);
    ACE_DASSERT(aAutoTuning != NULL);

    if (aAutoTuning->mLastDecision != ULN_AUTO_TUNING_DECISION_INCREASE)
    {
        aAutoTuning->mRZeroCountOnOptimal = 0;
    }
    else
    {
        /* nothing to do */
    }

    /* allow transient local error (r = 0) in the optimal state */
    if (aAutoTuning->mLastState == ULN_AUTO_TUNING_STATE_OPTIMAL &&
        aAutoTuning->mRZeroCountOnOptimal < ULN_AUTO_TUNING_R_0_IN_OPTIMAL_STATE)
    {
        /* transient local error, so keep the last predicted prefetch rows */
        aAutoTuning->mRZeroCountOnOptimal++;

        ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                       "auto-tuning : transient local error (r = 0) on optimal");
    }
    else
    {
        if (aAutoTuning->mState == ULN_AUTO_TUNING_STATE_STABLE)
        {
            /* stable, so keep the last predicted prefetch rows */
        }
        else
        {
            sPredictedPrefetchRows = aAutoTuning->mLastPrefetchRows *
                                     ULN_AUTO_TUNING_INCREASE_RATIO;

            ULN_SET_PREDICTED_PREFETCH_ROWS(aAutoTuning, sPredictedPrefetchRows);
            ULN_CHECK_TO_INCREASE_PREFETCH_ROWS(aAutoTuning);

            /* increase immediately, if d = 0 and f = 0 */
            if (aAutoTuning->mAppLoadTime > 0 && aAutoTuning->mPrefetchTime > 0)
            {
                /* should get more accurate feedback(error) */
                aAutoTuning->mIsTrialOneMore = ACP_TRUE;
            }
            else
            {
                /* increase rapidly */
            }

            aAutoTuning->mState = ULN_AUTO_TUNING_STATE_TUNING;
        }

        aAutoTuning->mRZeroCountOnOptimal = 0;
    }
}

/**
 * d > f (r > r_threshold_max) 인 경우이므로 prefetch rows 를 감소시킨다.
 */
static void ulnFetchAutoTuningDecrease(ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    acp_sint64_t sNextAppLoadTime; /* estimated next application load time */
    acp_double_t sPredictedPrefetchRows;

    /* variables to calculate a intersection from systems of linear equations */
    acp_sint64_t x1, x2;
    acp_sint64_t y1_f, y2_f, y1_d, y2_d;
    acp_double_t a_f, a_d, c_f, c_d;

    ACE_DASSERT(aAutoTuning != NULL);

    if (aAutoTuning->mLastFetchedRows == 0)
    {
        /**
         * try to reduce by r ratio, if don't have statistics,
         *
         *              d - r
         *   x_(t+1) = ------- * x_t
         *                d
         *
         */

        sPredictedPrefetchRows =
            ((acp_double_t)(aAutoTuning->mAppLoadTime - aAutoTuning->mNetworkIdleTime) /
             (acp_double_t)(aAutoTuning->mAppLoadTime)) * aAutoTuning->mLastPrefetchRows;

        ULN_SET_PREDICTED_PREFETCH_ROWS(aAutoTuning, sPredictedPrefetchRows);
    }
    else
    {
        /**
         * cacluate a intersection from systems of linear equations
         *
         *   x : prefetch rows
         *   y : elapsed time (d : application load time, f : prefetch time)
         *
         * 1. data :
         *
         *   (x1, y1_f), (x2, y2_f)
         *   (x1, y1_d), (x2, y2_d)
         *
         * 2. cacluate a, c coefficients from data, respectivley :
         *
         *   [ y = ax + c ]
         *
         *        y2 - y1
         *   a = --------- , c = y - ax
         *        x2 - x1
         *
         * 3. finally, calculate x-coordinate of the intersection :
         *
         *   a_d * x + c_d = a_f * x + c_f
         *
         *        c_d - c_f
         *   x = -----------              <--- x_(t+1)
         *        a_f - a_d
         */

        /* rows per ms * fetched rows */
        sNextAppLoadTime =
            ((acp_double_t)(aAutoTuning->mAppLoadTime + aAutoTuning->mSockReadTime) /
             (acp_double_t)aAutoTuning->mAppReadRows) * aAutoTuning->mFetchedRows;

        /* 아래 변수들은 수식 연산에 사용되므로 코드 가독성을 위해 coding convention 을 따르지 않는다. */
        x2   = aAutoTuning->mLastFetchedRows;
        x1   = aAutoTuning->mFetchedRows;
        y2_f = aAutoTuning->mLastPrefetchTime;
        y1_f = aAutoTuning->mPrefetchTime;
        y2_d = aAutoTuning->mAppLoadTime + aAutoTuning->mSockReadTime;
        y1_d = sNextAppLoadTime;

        if ((x2 - x1) != 0)
        {
            a_f  = (acp_double_t)(y2_f - y1_f) / (acp_double_t)(x2 - x1);
            c_f  = y2_f - (a_f * x2);

            a_d  = (acp_double_t)(y2_d - y1_d) / (acp_double_t)(x2 -x1);
            c_d  = y2_d - (a_d * x2);

            if (acpMathFastFabs(a_f - a_d) > ULN_AUTO_TUNING_EPSILON)
            {
                sPredictedPrefetchRows = (c_d - c_f) / (a_f - a_d);
            }
            else
            {
                sPredictedPrefetchRows = 0;
            }
        }
        else
        {
            sPredictedPrefetchRows = 0;
        }

        if ((sPredictedPrefetchRows < ULN_AUTO_TUNING_PREFETCH_ROWS_MIN) ||
            (sPredictedPrefetchRows > ULN_AUTO_TUNING_PREFETCH_ROWS_MAX))
        {
            /* simple reduction because of calculation exception */
            sPredictedPrefetchRows =
                ((acp_double_t)(aAutoTuning->mAppLoadTime - aAutoTuning->mNetworkIdleTime) /
                 (acp_double_t)(aAutoTuning->mAppLoadTime)) * aAutoTuning->mLastPrefetchRows;
        }
        else
        {
            /* nothing to do */
        }

        ULN_SET_PREDICTED_PREFETCH_ROWS(aAutoTuning, sPredictedPrefetchRows);
    }

    aAutoTuning->mState = ULN_AUTO_TUNING_STATE_TUNING;
}

/**
 * Stable 상태 또는 flooded 상태인지 체크한다.
 */
static void ulnFetchAutoTuningCheckState(ulnFnContext                   *aFnContext,
                                         ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    acp_uint64_t sFetchedBytes;

    ACE_DASSERT(aFnContext != NULL);
    ACE_DASSERT(aAutoTuning != NULL);

    /* check d / f ratio */
    if (aAutoTuning->mPrefetchTime >= ULN_AUTO_TUNING_DF_RATIO_MIN_PREFETCH_TIME)
    {
        if (aAutoTuning->mDFRatio < ULN_AUTO_TUNING_DF_RATIO_STABLE_THRESHOLD)
        {
            if (aAutoTuning->mDFRatioUnstableCount <= 0)
            {
                aAutoTuning->mLastStablePrefetchRows = aAutoTuning->mLastPrefetchRows;
                aAutoTuning->mDFRatioUnstableCount = 1;
            }
            else if (aAutoTuning->mLastStablePrefetchRows != aAutoTuning->mLastPrefetchRows)
            {
                aAutoTuning->mDFRatioUnstableCount++;
            }
            else
            {
                /* nothing to do */
            }

            ULN_PREFETCH_ASYNC_STAT_TRCLOG(
                    aFnContext,
                    "auto-tuning : flooded #%"ACI_INT32_FMT" (last stable prefetch rows = %"ACI_INT32_FMT")",
                    aAutoTuning->mDFRatioUnstableCount,
                    aAutoTuning->mLastStablePrefetchRows);

            if (aAutoTuning->mDFRatioUnstableCount >= ULN_AUTO_TUNING_FLOOD_THRESHOLD)
            {
                ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : FLOODED");

                /* apply the last stable prefetch rows */
                aAutoTuning->mPrefetchRows = aAutoTuning->mLastStablePrefetchRows;

                aAutoTuning->mState = ULN_AUTO_TUNING_STATE_FLOODED;
                aAutoTuning->mDFRatioUnstableCount = 0;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            aAutoTuning->mState = ULN_AUTO_TUNING_STATE_STABLE;
            aAutoTuning->mLastStablePrefetchRows = aAutoTuning->mLastPrefetchRows;

            if (aAutoTuning->mDFRatioUnstableCount > 0)
            {
                aAutoTuning->mDFRatioUnstableCount--;
            }
            else
            {
                /* nothing to do */
            }
        }
    }
    else
    {
        aAutoTuning->mDFRatioUnstableCount = 0;
    }

    /* check fetched bytes */
    if (aAutoTuning->mState != ULN_AUTO_TUNING_STATE_FLOODED)
    {
        sFetchedBytes = ulnStmtGetFetchedBytesFromServer(aAutoTuning->mStmt);
        if (sFetchedBytes > ULN_AUTO_TUNING_FETCHED_BYTES_MAX)
        {
            ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext,
                                           "auto-tuning : FLOODED (%"ACI_UINT64_FMT" bytes)",
                                           sFetchedBytes);

            /* apply the last stable prefetch rows */

            aAutoTuning->mState = ULN_AUTO_TUNING_STATE_FLOODED;
            aAutoTuning->mDFRatioUnstableCount = 0;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }
}

/**
 * Auto-tuning 을 시작한다.
 */
void ulnFetchBeginAutoTuning(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : begin");

    /* initialze, if auto-tuning was failed */
    if (sAutoTuning->mState == ULN_AUTO_TUNING_STATE_FAILED ||
        sAutoTuning->mLastState == ULN_AUTO_TUNING_STATE_FAILED)
    {
        ulnInitPrefetchAutoTuning(sAutoTuning, sAutoTuning->mStmt);
    }
    else
    {
        /* nothing to do */
    }

    ulnTraceAutoTuningParameters(aFnContext);
}

/**
 * Socket receive buffer 로부터 read (or receive) 이전에 호출하여 d time 을 측정한다.
 */
void ulnFetchAutoTuningBeforeReceive(ulnFnContext *aFnContext)
{
    ulnStmt                        *sStmt = aFnContext->mHandle.mStmt;
    ulnCache                       *sCache = ulnStmtGetCache(sStmt);
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    sAutoTuning->mEndTimeToMeasure = acpTimeNow();

    sAutoTuning->mAppLoadTime =
        acpTimeToMsec(sAutoTuning->mEndTimeToMeasure - sAutoTuning->mBeginTimeToMeasure);

    if (sAutoTuning->mLastCachedRows > 0)
    {
        sAutoTuning->mAppReadRows =
            sAutoTuning->mLastCachedRows - ulnCacheGetRowCount(sCache);
    }
    else
    {
        /* first auto-tuning */
    }
}

/**
 * Auto-tuning 을 수행하여 predicted prefetch rows 를 반환한다.
 */
ACI_RC ulnFetchAutoTuning(ulnFnContext *aFnContext,
                          acp_uint32_t  aFetchedRows,
                          acp_uint32_t *aPredictedPrefetchRows)
{
    ulnDbc                         *sDbc = NULL;
    ulnStmt                        *sStmt = aFnContext->mHandle.mStmt;
    ulnCache                       *sCache = ulnStmtGetCache(sStmt);
    ulnCursor                      *sCursor = ulnStmtGetCursor(sStmt);
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);
    acp_uint32_t                    sCursorSize = ulnCursorGetSize(sCursor);
    acp_uint32_t                    sAttrSockRcvBufBlockRatio;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACE_DASSERT(aPredictedPrefetchRows != NULL);
    ACE_DASSERT(sAutoTuning != NULL);
    ACE_DASSERT(sAutoTuning->mState < ULN_AUTO_TUNING_STATE_MAX);

    sAttrSockRcvBufBlockRatio = ulnDbcGetAttrSockRcvBufBlockRatio(sDbc);

    /* 1. update, if last auto-tuned prefetch rows is ULN_CACHE_OPTIMAL_PREFETCH_ROWS(0) */
    if (sAutoTuning->mLastPrefetchRows == ULN_CACHE_OPTIMAL_PREFETCH_ROWS)
    {
        sAutoTuning->mPrefetchRows     = aFetchedRows;
        sAutoTuning->mLastPrefetchRows = aFetchedRows;
    }
    else
    {
        /* nothing to do */
    }

    sAutoTuning->mFetchedRows = aFetchedRows;
    sAutoTuning->mDecision = ULN_AUTO_TUNING_DECISION_UNKNOWN;

    /* 2. call state function for each auto-tuning state */
    ACI_TEST(gAutoTuningStateTable[sAutoTuning->mState].mStateFunc(aFnContext) != ACI_SUCCESS);

    ACE_DASSERT(sAutoTuning->mState < ULN_AUTO_TUNING_STATE_MAX);

    /* 3. prefetch rows limit */
    if (sAutoTuning->mPrefetchRows != ULN_CACHE_OPTIMAL_PREFETCH_ROWS)
    {
        sAutoTuning->mPrefetchRows = ACP_MAX(sAutoTuning->mPrefetchRows, sStmt->mAttrPrefetchAutoTuningMin);
        sAutoTuning->mPrefetchRows = ACP_MIN(sAutoTuning->mPrefetchRows, sStmt->mAttrPrefetchAutoTuningMax);

        sAutoTuning->mPrefetchRows = ACP_MAX(sAutoTuning->mPrefetchRows, sCursorSize);
    }
    else
    {
        /* may be ULN_CACHE_OPTIMAL_PREFETCH_ROWS(0) on first prediction */
    }

    /* 4. adjust socket receive buffer that should ensure */
    if (sAttrSockRcvBufBlockRatio == 0)
    {
        ACI_TEST(ulnAdjustAutoTunedSockRcvBuf(aFnContext, sAutoTuning) != ACI_SUCCESS);
    }
    else
    {
        /* fixed socket receive buffer size */
    }

    /* 5. trace logging */
    if (ACP_LIKELY_FALSE(ulnIsPrefetchAsyncStatTrcLog() == ACP_TRUE))
    {
        ulnTraceLogPrefetchAsyncStat(
                aFnContext,
                "auto-tuning : d = %3"ACI_INT64_FMT" ms (%6"ACI_INT32_FMT" rows), r = %2"ACI_UINT32_FMT" ms, k = %2"ACI_INT64_FMT" ms, f = %3"ACI_INT64_FMT" ms (%6"ACI_UINT32_FMT" rows = %7"ACI_UINT64_FMT" bytes), d/f = %f, predicted = %6"ACI_UINT32_FMT" (%2"ACI_INT32_FMT"), state = %c",
                sAutoTuning->mAppLoadTime,
                sAutoTuning->mAppReadRows,
                sAutoTuning->mNetworkIdleTime,
                sAutoTuning->mSockReadTime,
                sAutoTuning->mPrefetchTime,
                aFetchedRows,
                ulnStmtGetFetchedBytesFromServer(sStmt),
                sAutoTuning->mDFRatio,
                sAutoTuning->mPrefetchRows,
                sAutoTuning->mDecision,
                gAutoTuningStateTable[sAutoTuning->mState].mStateChar);
    }
    else
    {
        /* nothing to do */
    }

    /* 6. save statistics */
    sAutoTuning->mLastPrefetchRows    = sAutoTuning->mPrefetchRows;
    sAutoTuning->mLastCachedRows      = ulnCacheGetRowCount(sCache);
    sAutoTuning->mLastFetchedRows     = aFetchedRows;
    sAutoTuning->mLastAppLoadTime     = sAutoTuning->mAppLoadTime;
    sAutoTuning->mLastPrefetchTime    = sAutoTuning->mPrefetchTime;
    sAutoTuning->mLastNetworkIdleTime = sAutoTuning->mNetworkIdleTime;
    sAutoTuning->mLastSockReadTime    = sAutoTuning->mSockReadTime;
    sAutoTuning->mLastDecision        = sAutoTuning->mDecision;
    sAutoTuning->mLastState           = sAutoTuning->mState;

    /* 7. begin to measure new elapsed time */
    sAutoTuning->mBeginTimeToMeasure = acpTimeNow();

    /* 8. final auto-tuned prefetch rows */
    *aPredictedPrefetchRows = sAutoTuning->mPrefetchRows;

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : error and stopped");

    sAutoTuning->mState = ULN_AUTO_TUNING_STATE_FAILED;

    return ACI_FAILURE;
}

/**
 * Auto-tuning 의 first prediction 으로서,
 * ALTIBASE_PREFETCH_AUTO_TUNING_INIT 또는 ALTIBASE_PREFETCH_ROWS 속성 값으로 시작한다.
 */
static ACI_RC ulnFetchAutoTuningStateFuncInit(ulnFnContext *aFnContext)
{
    ulnStmt                        *sStmt;
    ulnCache                       *sCache;
    ulnCursor                      *sCursor;
    acp_uint32_t                    sAttrPrefetchAutoTuningInit;
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    sStmt                       = sAutoTuning->mStmt;
    sCache                      = ulnStmtGetCache(sStmt);
    sCursor                     = ulnStmtGetCursor(sStmt);
    sAttrPrefetchAutoTuningInit = ulnStmtGetAttrPrefetchAutoTuningInit(sStmt);

    if (sAttrPrefetchAutoTuningInit == 0)
    {
        /* ALTIBASE_PREFETCH_AUTO_TUNING_INIT = 0 */

        ulnFetchCalcPrefetchRowsAsync(sCache,
                                      sCursor,
                                      &sAutoTuning->mPrefetchRows);
    }
    else
    {
        /* ALTIBASE_PREFETCH_AUTO_TUNING_INIT > 0 */

        sAutoTuning->mPrefetchRows = sAttrPrefetchAutoTuningInit;
    }

    sAutoTuning->mState = ULN_AUTO_TUNING_STATE_TUNING;

    return ACI_SUCCESS;
}

/**
 * Network idle time 을 측정한다.
 */
static ACI_RC ulnFetchAutoTuningMeasureNetworkIdleTime(ulnSemiAsyncPrefetchAutoTuning *aAutoTuning)
{
    ACE_DASSERT(aAutoTuning != NULL);

    /* 1. get r time */
    ACI_TEST(cmiGetLinkInfo(aAutoTuning->mStmt->mParentDbc->mLink,
                            (acp_char_t *)&aAutoTuning->mTcpInfo,
                            ACI_SIZEOF(struct tcp_info),
                            CMI_LINK_INFO_TCP_KERNEL_STAT) != ACI_SUCCESS);

    aAutoTuning->mNetworkIdleTime = aAutoTuning->mTcpInfo.tcpi_last_data_recv;

    /* 2. measure f, k time */
    aAutoTuning->mEndTimeToMeasure = acpTimeNow();
    aAutoTuning->mTotalElapsedTime =
        acpTimeToMsec(aAutoTuning->mEndTimeToMeasure - aAutoTuning->mBeginTimeToMeasure);

    aAutoTuning->mSockReadTime = aAutoTuning->mTotalElapsedTime - aAutoTuning->mAppLoadTime;
    aAutoTuning->mPrefetchTime = aAutoTuning->mTotalElapsedTime - aAutoTuning->mNetworkIdleTime;

    if (aAutoTuning->mPrefetchTime > 0)
    {
        aAutoTuning->mDFRatio = (acp_double_t)aAutoTuning->mAppLoadTime /
                                (acp_double_t)aAutoTuning->mPrefetchTime;
    }
    else
    {
        aAutoTuning->mPrefetchTime = 0;
        aAutoTuning->mDFRatio = 0;
    }

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Tuning 상태로서, network idle time 을 기반으로 최적의 prefetch rows 를 찾는다.
 */
static ACI_RC ulnFetchAutoTuningStateFuncTuning(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    /* 1. change tuning state first, because the network environment might be changed */
    sAutoTuning->mState = ULN_AUTO_TUNING_STATE_TUNING;

    /* 2. measure network idle time */
    ACI_TEST(ulnFetchAutoTuningMeasureNetworkIdleTime(sAutoTuning) != ACI_SUCCESS);

    /* 3. decide auto-tuning direction through the statistics */
    sAutoTuning->mDecision = ulnCompareAppLoadAndPrefetchTime(sAutoTuning);

    /* 4. apply one more, if needs */
    if (sAutoTuning->mIsTrialOneMore == ACP_TRUE)
    {
        ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : keep");

        sAutoTuning->mIsTrialOneMore = ACP_FALSE;

        /* keep the last predicted prefetch rows */
        ACI_RAISE(NORMAL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    /* 5. check state */
    ulnFetchAutoTuningCheckState(aFnContext, sAutoTuning);

    /* 6. apply the last stable prefetch rows, if flooded state */
    if (sAutoTuning->mState == ULN_AUTO_TUNING_STATE_FLOODED)
    {
        /* apply the last stable prefetch rows */
        sAutoTuning->mPrefetchRows = sAutoTuning->mLastStablePrefetchRows;

        ACI_RAISE(NORMAL_EXIT);
    }
    else
    {
        /* nothing to do */
    }

    /* 7. auto-tuning by decision */
    if (sAutoTuning->mDecision == ULN_AUTO_TUNING_DECISION_INCREASE)
    {
        /* 7-1. d < f : should increase (r = 0) */
        ulnFetchAutoTuningIncrease(aFnContext, sAutoTuning);
    }
    else if (sAutoTuning->mDecision == ULN_AUTO_TUNING_DECISION_DECREASE)
    {
        /* 7-2. d > f : should decrease (r > r_threshold_max) */
        ulnFetchAutoTuningDecrease(sAutoTuning);
    }
    else
    {
        /* 7-3. d = f : optimized at this time (r_threshold_min <= r <= r_threshold_max) */
        sAutoTuning->mState = ULN_AUTO_TUNING_STATE_STABLE;
    }

    /* 8. check optimal state */
    if (sAutoTuning->mLastPrefetchRows == sAutoTuning->mPrefetchRows)
    {
        sAutoTuning->mConsecutiveCount++;

        if (sAutoTuning->mConsecutiveCount >= ULN_AUTO_TUNING_OPTIMAL_THRESHOLD)
        {
            sAutoTuning->mState = ULN_AUTO_TUNING_STATE_OPTIMAL;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* stable or tuning state */
        sAutoTuning->mConsecutiveCount = 0;
    }

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Flooded 상태로서, auto-tuning 을 포기하고 last stable prefetch rows 를 적용한다.
 */
static ACI_RC ulnFetchAutoTuningStateFuncFlooded(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    /* 1. measure network idle time */
    ACI_TEST(ulnFetchAutoTuningMeasureNetworkIdleTime(sAutoTuning) != ACP_RC_SUCCESS);

    /* 2. decide auto-tuning direction through the statistics */
    sAutoTuning->mDecision = ulnCompareAppLoadAndPrefetchTime(sAutoTuning);

    /* 3. apply the last stable prefetch rows */
    sAutoTuning->mPrefetchRows = sAutoTuning->mLastStablePrefetchRows;

    sAutoTuning->mConsecutiveCount = 0;

    return ACI_SUCCESS;

    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Reset 상태로서, auto-tuning 통계 정보를 초기화한다.
 * 비동기 요청 이후 미리 read 할 경우 auto-tuning 통계 정보가 부정확하기 때문에 초기화 한다.
 */
static ACI_RC ulnFetchAutoTuningStateFuncReset(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    ulnPurgeStat(sAutoTuning);

    if (sAutoTuning->mLastState != ULN_AUTO_TUNING_STATE_OPTIMAL)
    {
        sAutoTuning->mConsecutiveCount    = 0;
        sAutoTuning->mRZeroCountOnOptimal = 0;
    }
    else
    {
        /* nothing to do */
    }

    if (sAutoTuning->mLastState != ULN_AUTO_TUNING_STATE_FLOODED)
    {
        sAutoTuning->mDFRatioUnstableCount = 0;
    }
    else
    {
        /* nothing to do */
    }

    /* keep the last predicted prefetch rows */
    sAutoTuning->mIsTrialOneMore = ACP_TRUE;

    if (sAutoTuning->mLastState != ULN_AUTO_TUNING_STATE_OPTIMAL &&
        sAutoTuning->mLastState != ULN_AUTO_TUNING_STATE_FLOODED)
    {
        sAutoTuning->mState = ULN_AUTO_TUNING_STATE_TUNING;
    }
    else
    {
        /* optimal or flooded state */
    }

    return ACI_SUCCESS;
}

/**
 * Auto-tuning 종료 후 다시 시작할 경우,
 * 마지막 auto-tuned prefetch rows 를 적용하고 통계/상태 정보는 초기화 한다.
 */
static ACI_RC ulnFetchAutoTuningStateFuncEnd(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    ACE_DASSERT(sAutoTuning != NULL);

    /* purge auto-tuning statistics */
    ulnPurgeStat(sAutoTuning);

    sAutoTuning->mIsTrialOneMore       = ACP_FALSE;
    sAutoTuning->mConsecutiveCount     = 0;
    sAutoTuning->mRZeroCountOnOptimal  = 0;
    sAutoTuning->mDFRatioUnstableCount = 0;

    /* purge the last stable prefetch rows */
    sAutoTuning->mLastStablePrefetchRows = 0;

    sAutoTuning->mState = ULN_AUTO_TUNING_STATE_TUNING;

    return ACI_SUCCESS;
}

/**
 * 해당 상태로 auto-tuning 할 경우 assert 를 발생시킨다.
 */
static ACI_RC ulnFetchAutoTuningStateFuncAssert(ulnFnContext *aFnContext)
{
    ACP_UNUSED(aFnContext);

    ACE_ASSERT(0);

    return ACI_SUCCESS;
}

/**
 * 다른 프로토콜에 의해 이미 read 한 경우 통계 정보가 유용하지 않으므로 reset 한다.
 */
void ulnFetchSkipAutoTuning(ulnFnContext *aFnContext)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);

    /* don't need to reset after auto-tuning end */
    ACI_TEST_RAISE(sAutoTuning->mState == ULN_AUTO_TUNING_STATE_END, NORMAL_EXIT);

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : skip");

    sAutoTuning->mLastState = sAutoTuning->mState;
    sAutoTuning->mState = ULN_AUTO_TUNING_STATE_RESET;

    ACI_EXCEPTION_CONT(NORMAL_EXIT);
}

/**
 * 다음 result set 에 대해 auto-tuing 할 경우 호출한다.
 */
void ulnFetchNextAutoTuning(ulnFnContext *aFnContext)
{
    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : begin (next result set)");

    ulnTraceAutoTuningParameters(aFnContext);
}

/**
 * Server cursor close 될 경우 auto-tuning 을 종료한다.
 */
ACI_RC ulnFetchEndAutoTuning(ulnFnContext *aFnContext)
{
    ulnDbc                         *sDbc = NULL;
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning = ulnGetSemiAsyncPrefetchAutoTuning(aFnContext);
    acp_uint32_t                    sSockRcvBufBlockRatio;

    ULN_FNCONTEXT_GET_DBC(aFnContext, sDbc);

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_TEST_RAISE(sDbc == NULL, InvalidHandleException);

    ACE_DASSERT(sAutoTuning != NULL);

    /* ignore, if already ended */
    ACI_TEST_RAISE(sAutoTuning->mState == ULN_AUTO_TUNING_STATE_END, NORMAL_EXIT);

    if (sAutoTuning->mIsAdjustedSockRcvBuf == ACP_TRUE)
    {
        sSockRcvBufBlockRatio = ulnDbcGetAttrSockRcvBufBlockRatio(sDbc);

        ACI_TEST(ulnDbcSetSockRcvBufBlockRatio(aFnContext,
                                               sSockRcvBufBlockRatio)
                != ACI_SUCCESS);

        ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : restored for the adjusted socket receive buffer");

        sAutoTuning->mLastAdjustedSockRcvBufSize = 0;
        sAutoTuning->mIsAdjustedSockRcvBuf = ACP_FALSE;
    }
    else
    {
        /* not changed */
    }

    sAutoTuning->mLastState = sAutoTuning->mState;
    sAutoTuning->mState = ULN_AUTO_TUNING_STATE_END;

    ULN_PREFETCH_ASYNC_STAT_TRCLOG(aFnContext, "auto-tuning : end");

    ACI_EXCEPTION_CONT(NORMAL_EXIT);

    return ACI_SUCCESS;

    /* BUG-46052 codesonar Null Pointer Dereference */
    ACI_EXCEPTION(InvalidHandleException)
    {
        ULN_FNCONTEXT_SET_RC(aFnContext, SQL_INVALID_HANDLE);
    }
    ACI_EXCEPTION_END;

    return ACI_FAILURE;
}

/**
 * Auto-tuning 에 의해 socket receive buffer 가 조절되었는지 여부를 확인한다.
 */
acp_bool_t ulnFetchIsAutoTunedSockRcvBuf(ulnDbc *aDbc)
{
    ulnSemiAsyncPrefetchAutoTuning *sAutoTuning;

    ACE_DASSERT(aDbc != NULL);

    ACI_TEST(aDbc->mAsyncPrefetchStmt == NULL);
    ACI_TEST(aDbc->mAsyncPrefetchStmt->mSemiAsyncPrefetch == NULL);

    sAutoTuning = &aDbc->mAsyncPrefetchStmt->mSemiAsyncPrefetch->mAutoTuning;

    return sAutoTuning->mIsAdjustedSockRcvBuf;

    ACI_EXCEPTION_END;

    return ACP_FALSE;
}

void ulnInitTraceLogPrefetchAsyncStat()
{
    acp_char_t *sEnvStr = NULL;

    if (acpEnvGet("ALTIBASE_PREFETCH_ASYNC_TRCLOG_FILE", &sEnvStr) == ACP_RC_SUCCESS)
    {
        gPrefetchAsyncStatTrcLog.mIsTrcLogFile = ACP_TRUE;

        acpSnprintf(gPrefetchAsyncStatTrcLog.mFileName,
                    ACP_PATH_MAX_LENGTH,
                    "%s",
                    sEnvStr);
        gPrefetchAsyncStatTrcLog.mEnvFileNameOffset =
            acpCStrLen(gPrefetchAsyncStatTrcLog.mFileName,
                       ACI_SIZEOF(gPrefetchAsyncStatTrcLog.mFileName));

        ACE_ASSERT(acpThrMutexCreate(&gPrefetchAsyncStatTrcLog.mMutex,
                                     ACP_THR_MUTEX_DEFAULT) == ACP_RC_SUCCESS);
    }
    else
    {
        gPrefetchAsyncStatTrcLog.mIsTrcLogFile      = ACP_FALSE;
        gPrefetchAsyncStatTrcLog.mFileName[0]       = '\0';
        gPrefetchAsyncStatTrcLog.mEnvFileNameOffset = 0;
    }

    if (acpEnvGet("ALTIBASE_PREFETCH_ASYNC_TRCLOG_STDOUT", &sEnvStr) == ACP_RC_SUCCESS)
    {
        gPrefetchAsyncStatTrcLog.mIsTrcLogStdOut = ACP_TRUE;
    }
    else
    {
        gPrefetchAsyncStatTrcLog.mIsTrcLogStdOut = ACP_FALSE;
    }
}

void ulnFinalizeTraceLogPrefetchAsyncStat()
{
    if (gPrefetchAsyncStatTrcLog.mIsTrcLogFile == ACP_TRUE)
    {
        (void)acpThrMutexDestroy(&gPrefetchAsyncStatTrcLog.mMutex);
    }
    else
    {
        /* nothing to do */
    }
}

void ulnTraceLogPrefetchAsyncStat(ulnFnContext     *aFnContext,
                                  const acp_char_t *aFormat,
                                  ...)
{
    va_list sArgs;

    va_start(sArgs, aFormat);

    ulnTraceLogPrefetchAsyncStatV(aFnContext, aFormat, sArgs);

    va_end(sArgs);
}

void ulnTraceLogPrefetchAsyncStatV(ulnFnContext     *aFnContext,
                                   const acp_char_t *aFormat,
                                   va_list           aArgs)
{
    acp_std_file_t sFile;
    acp_sint32_t   sPid;
    acp_uint64_t   sThrId;
    acp_char_t     sHandleMsg[80];
    acp_time_t     sTimevalue;
    acp_time_exp_t sLocalTime;
    acp_uint32_t   sState = 0;
    va_list        sArgsStdOut;

    ACE_DASSERT(aFnContext != NULL);
    ACE_DASSERT(aFormat != NULL);

    if (gPrefetchAsyncStatTrcLog.mIsTrcLogStdOut == ACP_TRUE)
    {
        va_copy(sArgsStdOut, aArgs);

        acpVprintf(aFormat, sArgsStdOut);
        acpPrintf("\n");
    }
    else
    {
        /* nothing to do */
    }

    ACI_TEST_RAISE(gPrefetchAsyncStatTrcLog.mIsTrcLogFile == ACP_FALSE, SKIP_TRACE_FILE);

    sPid = acpProcGetSelfID();
    sThrId = acpThrGetSelfID();

    if (aFnContext->mHandle.mObj != NULL)
    {
        if (aFnContext->mObjType == ULN_OBJ_TYPE_DBC)
        {
            acpSnprintf(sHandleMsg,
                        ACI_SIZEOF(sHandleMsg),
                        "[C-%p]",
                        aFnContext->mHandle.mDbc);
        }
        else if (aFnContext->mObjType == ULN_OBJ_TYPE_STMT)
        {
            acpSnprintf(sHandleMsg,
                        ACI_SIZEOF(sHandleMsg),
                        "[C-%p][S-%p]",
                        aFnContext->mHandle.mStmt->mParentDbc,
                        aFnContext->mHandle.mStmt);
        }
        else
        {
            sHandleMsg[0] = '\0';
        }
    }
    else
    {
        sHandleMsg[0] = '\0';
    }

    /* e.g.) altibase_cli_prefetch_async_PID_mmddhh.log */
    sTimevalue = acpTimeNow();
    acpTimeGetLocalTime(sTimevalue, &sLocalTime);

    acpSnprintf(gPrefetchAsyncStatTrcLog.mFileName + gPrefetchAsyncStatTrcLog.mEnvFileNameOffset,
                ACI_SIZEOF(gPrefetchAsyncStatTrcLog.mFileName) - gPrefetchAsyncStatTrcLog.mEnvFileNameOffset,
                "_%"ACI_INT32_FMT"_%02"ACI_INT32_FMT"%02"ACI_INT32_FMT"%02"ACI_INT32_FMT".log",
                sPid,
                sLocalTime.mMonth,
                sLocalTime.mDay,
                sLocalTime.mHour);

    ACE_ASSERT(acpThrMutexLock(&gPrefetchAsyncStatTrcLog.mMutex) == ACP_RC_SUCCESS);
    sState = 1;

    ACI_TEST(acpStdOpen(&sFile,
                        gPrefetchAsyncStatTrcLog.mFileName,
                        ACP_STD_OPEN_APPEND_TEXT) != ACP_RC_SUCCESS);

    /* e.g.) [hh:mm:ss:ms][PID:3636][Thread-2][C-0x12340000][S-0x56780000] trace logging message */
    (void)acpFprintf(&sFile,
                     "[%"ACI_INT32_FMT"/%02"ACI_INT32_FMT"/%02"ACI_INT32_FMT" %02"ACI_INT32_FMT":%02"ACI_INT32_FMT":%02"ACI_INT32_FMT".%03"ACI_INT32_FMT"]"
                     "[PID-%"ACI_INT32_FMT"]"
                     "[Thread-%"ACI_UINT64_FMT"]"
                     "%s ",
                     sLocalTime.mYear,
                     sLocalTime.mMonth,
                     sLocalTime.mDay,
                     sLocalTime.mHour,
                     sLocalTime.mMin,
                     sLocalTime.mSec,
                     sLocalTime.mUsec / 1000,
                     sPid,
                     sThrId,
                     sHandleMsg);

    (void)acpVfprintf(&sFile, aFormat, aArgs);

    (void)acpFprintf(&sFile, "\n");

    (void)acpStdFlush(&sFile);
    (void)acpStdClose(&sFile);

    sState = 0;
    ACE_ASSERT(acpThrMutexUnlock(&gPrefetchAsyncStatTrcLog.mMutex) == ACP_RC_SUCCESS);

    ACI_EXCEPTION_CONT(SKIP_TRACE_FILE);

    return;

    ACI_EXCEPTION_END;

    switch (sState)
    {
        case 1:
            ACE_ASSERT(acpThrMutexUnlock(&gPrefetchAsyncStatTrcLog.mMutex) == ACP_RC_SUCCESS);
            /* fall through */
        default:
            break;
    }
}


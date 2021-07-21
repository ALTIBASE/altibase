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
 * $Id: 
 *
 **********************************************************************/

#ifndef ACI_PROPERTY_H
#define ACI_PROPERTY_H 1

#include <acp.h>

ACP_EXTERN_C_BEGIN

#define ACI_SCALABILITY_CLIENT_CPU    1
#define ACI_SCALABILITY_CLIENT_MAX   32
#define ACI_SERVICE_STACK_SIZE 1048576
#define ACI_DEFAULT_STACK_SIZE 1048576

typedef struct aciProperty
{
    acp_uint32_t   mXLatchUseSignal;
    acp_uint32_t   mMutexType;
    acp_uint32_t   mCheckMutexDurationTimeEnable;
    acp_uint32_t   mTimedStatistics; 
    acp_uint32_t   mLatchType;
    acp_uint32_t   mMutexSpinCount;
    acp_uint32_t   mNativeMutexSpinCount;
    acp_uint32_t   mNativeMutexSpinCount4Logging;
    acp_uint32_t   mLatchSpinCount;
    acp_uint32_t   mProtocolDump;
    acp_uint32_t   mServerTrcFlag;
    acp_uint32_t   mSmTrcFlag;
    acp_uint32_t   mQpTrcFlag;
    acp_uint32_t   mJobTrcFlag; /* BUG-49108 */
    acp_uint32_t   mSdTrcFlag;  /* BUG-46138 */
    acp_uint32_t   mRpTrcFlag;
    acp_uint32_t   mDlTrcFlag;
    acp_uint32_t   mLkTrcFlag;
    acp_uint32_t   mXaTrcFlag; /* bug-24840 divide xa log */
    acp_uint32_t   mSourceInfo;
    acp_uint32_t   mAllMsglogFlush;
    /* direct io enabled */
    acp_uint32_t   mDirectIOEnabled;

    /* Query Prepare Memory Maxmimum Limit Size. */
    acp_uint64_t   mQMPMemMaxSize;
    
    /* Query Execute Memory Maximum Limit Size. */
    acp_uint64_t   mQMXMemMaxSize;

    /* Query Profile */
    acp_uint32_t   mQueryProfFlag;
    acp_uint32_t   mQueryProfBufSize;
    acp_uint32_t   mQueryProfBufFullSkip;
    acp_uint32_t   mQueryProfBufFlushSize;
    acp_uint32_t   mQueryProfFileSize;

    /*
     * Direct I/O시 file offset및 data size를 Align할 Page크기*
     */
    acp_uint32_t   mDirectIOPageSize;
    acp_uint32_t   mEnableRecoveryTest;

    /*
     * BUG-21487    Mutex Leak List출력을 property화 해야합니다.
     */
    acp_uint32_t   mShowMutexLeakList;

    /* BUG-20789
     * SM 소스에서 contention이 발생할 수 있는 부분에
     * 다중화를 할 경우 CPU하나당 몇 클라이언트를 예상할 지를 나타내는 상수
     * SM_SCALABILITY는 CPU개수 곱하기 이값이다. *
     */
    acp_uint32_t mScalabilityPerCPU;

    acp_uint32_t mMaxScalability;


    /* To Fix PR-13963 */
    acp_uint32_t   mInspectionLargeHeapThreshold;
    acp_bool_t     mInspectionLargeHeapThresholdInitialized;

    /* fix BUG-21266 */
    acp_uint32_t   mDefaultThreadStackSize;

    acp_uint32_t   mServiceThreadStackSize;

    /* fix BUG-21547 */
    acp_uint32_t   mUseMemoryPool;


} aci_property_t;

/*
 * Reference to global property object
 */
extern aci_property_t gAciProperty;

ACP_INLINE acp_uint32_t aciPropertyGetXLatchUseSignal(void)
{
    return gAciProperty.mXLatchUseSignal;
}

ACP_INLINE  acp_uint32_t aciPropertyGetMutexType(void)
{
    return gAciProperty.mMutexType;
}

ACP_INLINE  acp_uint32_t aciPropertyGetCheckMutexDurationTimeEnable(void)
{
    return gAciProperty.mCheckMutexDurationTimeEnable;
}

ACP_INLINE  acp_uint32_t aciPropertyGetTimedStatistics(void)
{
    return gAciProperty.mTimedStatistics;
}

ACP_INLINE acp_uint32_t aciPropertyGetLatchType(void)
{
    return gAciProperty.mLatchType;
}

ACP_INLINE acp_uint32_t aciPropertyGetMutexSpinCount(void)
{
    return gAciProperty.mMutexSpinCount;
}

ACP_INLINE acp_uint32_t aciPropertyGetNativeMutexSpinCount(void)
{
    return gAciProperty.mNativeMutexSpinCount;
}

ACP_INLINE acp_uint32_t aciPropertyGetNativeMutexSpinCount4Logging(void)
{
    return gAciProperty.mNativeMutexSpinCount4Logging;
}

ACP_INLINE acp_uint32_t aciPropertyGetLatchSpinCount(void)
{
    return gAciProperty.mLatchSpinCount;
}

ACP_INLINE acp_uint32_t aciPropertyGetProtocolDump(void)
{
    return gAciProperty.mProtocolDump;
}

ACP_INLINE acp_uint32_t aciPropertyGetServerTrcFlag(void)
{
    return gAciProperty.mServerTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetSmTrcFlag(void)
{
    return gAciProperty.mSmTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetQpTrcFlag(void)
{
    return gAciProperty.mQpTrcFlag;
}

/* BUG-49108 */
ACP_INLINE acp_uint32_t aciPropertyGetJobTrcFlag(void)
{
    return gAciProperty.mJobTrcFlag;
}

/* BUG-46138 */
ACP_INLINE acp_uint32_t aciPropertyGetSdTrcFlag(void)
{
    return gAciProperty.mSdTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetRpTrcFlag(void)
{
    return gAciProperty.mRpTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetDlTrcFlag(void)
{
    return gAciProperty.mDlTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetLkTrcFlag(void)
{
    return gAciProperty.mLkTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetXaTrcFlag(void)
{
    return gAciProperty.mXaTrcFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetSourceInfo(void)
{
    return gAciProperty.mSourceInfo;
}

ACP_INLINE acp_uint32_t aciPropertyGetAllMsglogFlush(void)
{
    return gAciProperty.mAllMsglogFlush;
}

ACP_INLINE acp_uint64_t aciPropertyGetPrepareMemoryMax(void)
{
    return gAciProperty.mQMPMemMaxSize;   
}

ACP_INLINE acp_uint64_t  aciPropertyGetExecuteMemoryMax(void)
{
    return gAciProperty.mQMXMemMaxSize;
}

ACP_INLINE acp_uint32_t  aciPropertyGetDirectIOEnabled(void)
{
    return gAciProperty.mDirectIOEnabled;
}

ACP_INLINE    acp_uint32_t   aciPropertyGetQueryProfFlag(void)
{
    return gAciProperty.mQueryProfFlag;
}

ACP_INLINE acp_uint32_t aciPropertyGetQueryProfBufSize(void)
{
    return gAciProperty.mQueryProfBufSize;
}

ACP_INLINE acp_uint32_t aciPropertyGetQueryProfBufFullSkip(void)
{
    return gAciProperty.mQueryProfBufFullSkip;
}

ACP_INLINE acp_uint32_t aciPropertyGetQueryProfBufFlushSize(void)
{
    return gAciProperty.mQueryProfBufFlushSize;
}

ACP_INLINE acp_uint32_t aciPropertyGetQueryProfFileSize(void)
{
    return gAciProperty.mQueryProfFileSize;
}

ACP_INLINE acp_uint32_t aciPropertyGetDirectIOPageSize(void)
{
    return gAciProperty.mDirectIOPageSize;
}

ACP_INLINE acp_uint32_t aciPropertyGetEnableRecTest(void)
{
    return gAciProperty.mEnableRecoveryTest;
}

ACP_INLINE acp_uint32_t aciPropertyGetShowMutexLeakList(void)
{
    return gAciProperty.mShowMutexLeakList;
}

ACP_INLINE acp_uint32_t aciPropertyGetScalability4CPU(void)
{
    return gAciProperty.mScalabilityPerCPU;
}

ACP_INLINE acp_uint32_t aciPropertyGetMaxScalability(void)
{
    return gAciProperty.mMaxScalability;
}

ACP_EXTERN_C_END

#endif /*  ACI_PROPERTY_H */


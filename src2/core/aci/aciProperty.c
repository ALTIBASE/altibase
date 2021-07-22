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
 * $Id: aciProperty.c 10969 2010-04-29 09:04:59Z gurugio $
 **********************************************************************/
#include <acp.h>
#include <aciProperty.h>

aci_property_t gAciProperty =
{
    0,                         /* mXLatchUseSignal */
    0,                         /* mMutexType */
    0,                         /* mCheckMutexDurationTimeEnable */
    0,                         /* mTimedStatistics */
    0,                         /* mLatchType */
    0,                         /* mMutexSpinCount */
    0,                         /* mNativeMutexSpinCount */
    0,                         /* mNativeMutexSpinCount4Logging */
    0,                         /* mLatchSpinCount */
    0,                         /* mProtocolDump */ 
    0,                         /* mServerTrcFlag */
    0,                         /* mSmTrcFlag */
    0,                         /* mQpTrcFlag */
    0,                         /* mRpTrcFlag */ 
    0,                         /* mDlTrcFlag */
    0,                         /* mLkTrcFlag */
    0,                         /* mXaTrcFlag */
    0,                         /* mSourceInfo */
    0,                         /* mAllMsglogFlush */ 
    0,                         /* mDirectIOEnabled */
    0UL,                       /* mQMPMemMaxSize */ 
    0UL,                       /* mQMXMemMaxSize */ 
    0,                         /* mQueryProfFlag */
    0,                         /* mQueryProfBufSize */
    0,                         /* mQueryProfBufFullSkip */
    0,                         /* mQueryProfBufFlushSize */
    0,                         /* mQueryProfFileSize */
    0,                         /* mDirectIOPageSize */
    0,                         /* mEnableRecoveryTest */
    0,                         /* mShowMutexLeakList */
    ACI_SCALABILITY_CLIENT_CPU,/* mScalabilityPerCPU */
    ACI_SCALABILITY_CLIENT_MAX,/* mMaxScalability */
    0,                         /* mInspectionLargeHeapThreshold */
    ACP_FALSE,                 /* mInspectionLargeHeapThresholdInitialized */
    ACI_DEFAULT_STACK_SIZE,    /* mDefaultThreadStackSize */
    ACI_SERVICE_STACK_SIZE,    /* mServiceThreadStackSize */
    1                          /* mUseMemoryPool */
};

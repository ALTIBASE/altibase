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
 * $Id$
 **********************************************************************/

#ifndef _O_IDV_AUDIT_DEF_H_
#define _O_IDV_AUDIT_DEF_H_ 1

#include <idl.h>
#include <idu.h>

#define IDV_MAX_IP_LEN 64
#define IDV_MAX_NAME_LEN 64
#define IDV_MAX_CLIENT_TYPE_LEN 40
#define IDV_MAX_CLIENT_APP_LEN 128
#define IDV_MAX_ACTION_LEN 64

struct idvSQL;
struct idvSession;

typedef struct idvAuditTrail
{
    /* User Info */
    ULong    mClientPID;          /* Process ID of the client */
    SChar    mClientType[IDV_MAX_CLIENT_TYPE_LEN + 1];     /* Type of the client, ex) CLI-64LE */
    SChar    mClientAppInfo[IDV_MAX_CLIENT_APP_LEN + 1];
    SChar    mLoginIP[IDV_MAX_IP_LEN + 1];
    SChar    mLoginID[IDV_MAX_NAME_LEN + 1];

    /* Session & Statement Info */
    UInt     mTransactionID;      /* mmcTransID */
    UInt     mSessionID;          /* mmcSessID */
    UInt     mStatementID;        /* mmcStmtID */
    UInt     mXaSessionFlag;      /* XA or not */
    UInt     mExecutionFlag;      /* Execution flag */
    UInt     mFetchFlag;          /* Fetch flag */
    UInt     mAutoCommitFlag;     /* Auto commit flag */
    
    /* Audit Info */
    UInt     mActionCode;
    SChar    mAction[IDV_MAX_ACTION_LEN + 1];
    
    /* Elapsed Time */
    ULong    mParseTime;
    ULong    mValidateTime;
    ULong    mOptimizeTime;
    ULong    mExecuteTime;
    ULong    mFetchTime;
    ULong    mSoftPrepareTime;
    ULong    mTotalTime;

    /* Stat Info */
    ULong    mUseMemory;
    ULong    mExecuteSuccessCount;
    ULong    mExecuteFailureCount;
    ULong    mProcessRow;

    /* Query Info */
    UInt     mReturnCode;
    
} idvAuditTrail; 

typedef struct idvAuditDescInfo
{
    void *mData;
    UInt  mLength;
} idvAuditDescInfo;

typedef struct idvAuditHeader
{
    UInt     mSize;
    ULong    mTime;
} idvAuditHeader;


/* BUG-39760 Enable AltiAudit to write log into syslog */
typedef enum idvAuditOutputMethod
{
    IDV_AUDIT_OUTPUT_NORMAL, //as it was
    IDV_AUDIT_OUTPUT_SYSLOG, //use syslog() 
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL0, //syslog() & facility:local0   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL1, //syslog() & facility:local1   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL2, //syslog() & facility:local2   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL3, //syslog() & facility:local3   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL4, //syslog() & facility:local4   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL5, //syslog() & facility:local5   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL6, //syslog() & facility:local6   
    IDV_AUDIT_OUTPUT_SYSLOG_LOCAL7, //syslog() & facility:local7   
    IDV_AUDIT_OUTPUT_MAX
} idvAuditOutputMethod;

#endif

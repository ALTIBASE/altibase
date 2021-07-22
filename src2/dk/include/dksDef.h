/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKS_DEF_H_
#define _O_DKS_DEF_H_ 1

#include <idl.h>
#include <cm.h>

#include <dkDef.h>
#include <dkaDef.h>
#include <dkuProperty.h>
#include <dktDef.h>
#include <dknLink.h>

#define DKS_LINKER_SESSION_TYPE_CONTROL         (0)
#define DKS_LINKER_SESSION_TYPE_DATA            (1)

#define DKS_LINKER_CONTROL_SESSION_ID           (0x00000000)
#define DKS_LINKER_NOTIFY_SESSION_ID            (ID_SINT_MAX)

#define DKS_ALTILINKER_CONNECT_HOST_NAME        DK_LOCAL_IP
#define DKS_ALTILINKER_CONNECT_PORT_NO          DKA_ALTILINKER_PORT_NO

#define DKS_MEM_POOL_DEFAULT_ELEM_CNT           (32)

#define DKS_AUTO_COMMIT_MODE_OFF                (0)
#define DKS_AUTO_COMMIT_MODE_ON                 (1)

#define DKS_CONNECTION_TRY_MAX_CNT              (10)


/****************************************************
 * Linker control session status
 ***************************************************/
typedef enum 
{
    /* linker session common */
    DKS_CTRL_SESSION_STATUS_NON = 0,
    DKS_CTRL_SESSION_STATUS_CREATED,
    DKS_CTRL_SESSION_STATUS_CONNECTED,
    DKS_CTRL_SESSION_STATUS_DISCONNECTED,
    DKS_CTRL_SESSION_STATUS_DESTROYED,
    /* control session only */
    DKS_CTRL_SESSION_STATUS_LOCKED,
    DKS_CTRL_SESSION_STATUS_UNLOCKED
} dksCtrlSessionStatus;

/****************************************************
 * Linker data session status
 ***************************************************/
typedef enum 
{
    /* linker session common */
    DKS_DATA_SESSION_STATUS_NON = 0,
    DKS_DATA_SESSION_STATUS_CREATED,
    DKS_DATA_SESSION_STATUS_CONNECTED,
    DKS_DATA_SESSION_STATUS_DISCONNECTED,
    DKS_DATA_SESSION_STATUS_DESTROYED
} dksDataSessionStatus;

/****************************************************
 * Remote Node Session Status 
 ***************************************************/
typedef enum 
{
    DKS_REMOTE_NODE_SESSION_STATUS_CONNECTED = 0,
    DKS_REMOTE_NODE_SESSION_STATUS_DISCONNECTED
} dksRemoteNodeSessionStatus;

typedef enum
{
    DKS_SESSION_TYPE_NONE    = 0,
    DKS_SESSION_TYPE_CONTROL,   // control session
    DKS_SESSION_TYPE_NOTIFY,    // notifier session
    DKS_SESSION_TYPE_DATA       // data session
} dksSessionType;

/* Define DK session including cm protocol */
typedef struct dksSession
{
    idBool              mLinkerDataSessionIsCreated;
    idBool              mIsNeedToDisconnect;
    idBool              mIsXA;
    UShort              mPortNo;
    dknLink            *mLink;
} dksSession;

/* Define linker control session */
typedef struct dksCtrlSession
{
    dksSession            mSession;
    dksCtrlSessionStatus  mStatus;
    UInt                  mRefCnt;
    iduMutex              mMutex;
} dksCtrlSession;

/* Define linker data session */
typedef struct dksDataSession
{
    dksSession            mSession;
    UInt                  mUserId;
    UInt                  mId;                    /* PV */
    UInt                  mGlobalTxId;            /* PV */
    UInt                  mLocalTxId;             /* PV */
    dksDataSessionStatus  mStatus;                /* PV */
    UInt                  mRefCnt;
    dktAtomicTxLevel      mAtomicTxLevel;         /* session property */
    UInt                  mAutoCommitMode;        /* session property */
    iduListNode           mNode;
} dksDataSession;

/* Define linker session info */
typedef struct dksLinkerSessionInfo
{
    UInt                mId;
    UInt                mStatus;
    UInt                mType;
} dksLinkerSessionInfo;

/* Define linker control session info */
typedef struct dksCtrlSessionInfo
{
    dksCtrlSessionStatus mStatus;
    UInt                 mReferenceCount;
} dksCtrlSessionInfo;

/* Define linker data session info */
typedef struct dksDataSessionInfo
{
    UInt                 mId;
    dksDataSessionStatus mStatus;
    UInt                 mGlobalTxId;
    UInt                 mLocalTxId;
} dksDataSessionInfo;

/* Define remote node session status info */
typedef struct dksRemoteNodeInfo
{
    dksRemoteNodeSessionStatus mStatus;
    UShort                     mSessionID;
} dksRemoteNodeInfo;


#endif  /* _O_DKS_DEF_H_ */

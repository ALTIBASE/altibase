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

#ifndef _O_IDV_PROFILE_DEF_H_
#define _O_IDV_PROFILE_DEF_H_ 1

#include <idl.h>
#include <idu.h>

struct idvSQL;
struct idvSession;

#define IDV_PROF_TYPE_STMT_FLAG 0x0001
#define IDV_PROF_TYPE_BIND_FLAG 0x0002
#define IDV_PROF_TYPE_PLAN_FLAG 0x0004
#define IDV_PROF_TYPE_SESS_FLAG 0x0008
#define IDV_PROF_TYPE_SYSS_FLAG 0x0010
#define IDV_PROF_TYPE_MEMS_FLAG 0x0020

#define IDV_PROF_TYPE_MAX_FLAG (IDV_PROF_TYPE_STMT_FLAG | IDV_PROF_TYPE_BIND_FLAG | \
                                IDV_PROF_TYPE_PLAN_FLAG | IDV_PROF_TYPE_SESS_FLAG | \
                                IDV_PROF_TYPE_SYSS_FLAG | IDV_PROF_TYPE_MEMS_FLAG )


/* BUG-737745 */
#define IDV_PROF_MAX_CLIENT_TYPE_LEN 40
#define IDV_PROF_MAX_CLIENT_APP_LEN 128

typedef struct idvProfDescInfo
{
    void  *mData;
    UInt   mLength;
}idvProfDescInfo;

// proj_2160 cm_type removal
typedef struct idvProfBind
{
    UInt        mParamNo;
    UInt        mType;
    void       *mData;
} idvProfBind;

typedef UInt   (*idvWriteCallbackA5)(void            *aBindData,
                                   idvProfDescInfo *aInfo,
                                   UInt             aInfoBegin,
                                   UInt             aInfoMax,
                                   UInt             aCurrSize);

typedef UInt   (*idvWriteCallback)(idvProfBind     *aBindData,
                                   idvProfDescInfo *aInfo,
                                   UInt             aInfoBegin,
                                   UInt             aCurrSize);

typedef enum idvProfType
{
    IDV_PROF_TYPE_NONE = 0,
    IDV_PROF_TYPE_STMT,
    IDV_PROF_TYPE_BIND_A5,
    IDV_PROF_TYPE_PLAN,
    IDV_PROF_TYPE_SESS,
    IDV_PROF_TYPE_SYSS,
    IDV_PROF_TYPE_MEMS,
    IDV_PROF_TYPE_BIND,
        
    IDV_PROF_MAX
        
}idvProfType;

typedef struct idvProfHeader
{
    UInt        mSize;
    idvProfType mType;
    ULong       mTime;
}idvProfHeader;
    

typedef struct idvProfStmtInfo
{
    /* additinal info */
    ULong   mClientPID;
    ULong   mTotalTime;
    ULong   mParseTime;
    ULong   mValidateTime;
    ULong   mOptimizeTime;
    ULong   mExecuteTime;
    ULong   mFetchTime;
    ULong   mSoftPrepareTime;

    UInt    mExecutionFlag;
    UInt    mFetchFlag;
    UInt    mSID;  // session   id
    UInt    mSSID; // statement id
    UInt    mTxID;
    UInt    mUserID;

    SChar   mClientType[IDV_PROF_MAX_CLIENT_TYPE_LEN + 1]; 
    SChar   mClientAppInfo[IDV_PROF_MAX_CLIENT_APP_LEN + 1];

    UInt    mXaSessionFlag;
} idvProfStmtInfo;

typedef struct idvProfSystem
{
    /* additinal info?? */
    
    /* real data*/
    idvSession *mSessData;
} idvProfSystem;

#endif

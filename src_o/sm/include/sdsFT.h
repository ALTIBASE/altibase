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
 * $Id$
 **********************************************************************/

/***********************************************************************
 * PROJ-2102 The Fast Secondary Buffer
 **********************************************************************/

#ifndef _O_SDS_FT_H_
#define _O_SDS_FT_H_ 1

#include <smDef.h>

/***********************************************
 *  BCB를 위한 fixed table 정의
 ***********************************************/
typedef struct sdsBCBStat
{
    UInt  mSBCBID;
    UInt  mSpaceID;
    UInt  mPageID;
    UInt  mHashBucketNo;
    SInt  mCPListNo;
    UInt  mRecvLSNFileNo;
    UInt  mRecvLSNOffset;
    UInt  mState;
    UInt  mBCBMutexLocked;
    UInt  mPageLSNFileNo;
    UInt  mPageLSNOffset;
} sdsBCBStat;

typedef struct sdsBCBStatArg
{
    void                *mHeader;
    iduFixedTableMemory *mMemory;
} sdsBCBStatArg;

/***********************************************************************
 *  Flush Mgr 위한 fixed table 정의
 ***********************************************************************/
typedef struct sdsFlushMgrStat
{
    /* flusher의 개수*/
    UInt    mFluserCount;
    /* checkpoint list count */
    UInt    mCPList;
    /* 현재 요청한 job 갯수 */
    UInt    mReqJobCount;
    /* flush 대상의 페이지 수 */
    UInt    mReplacementFlushPages;
    /* checkpoint flush 대상 pages 수 */
    ULong   mCheckpointFlushPages;
    /* checkpoint recoveryLSN이 가장 작은 SBCB.BCBID */
    UInt    mMinBCBID;
    /* checkpoint recoveryLSN이 가장 작은 SBCB.SpaceID */
    UInt    mMinBCBSpaceID;
    /* checkpoint recoveryLSN이 가장 작은 SBCB.PageID */
    UInt    mMinBCBPageID;
} sdsFlushMgrStat;

/***********************************************************************
 *  File node 위한 fixed table 정의
 ***********************************************************************/
typedef struct sdsFileNodeStat
{
    SChar*    mName;

    smLSN     mRedoLSN;
    smLSN     mCreateLSN;

    UInt      mSmVersion;

    ULong     mFilePages;
    idBool    mIsOpened;
    UInt      mState;
} sdsFileNodeStat;

class sdsFT
{
public:

private:

public:

private:

};

#endif //_O_SDS_FT_H_

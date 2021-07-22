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
 * $Id: rpxPJMgr.h 85246 2019-04-16 06:49:48Z donghyun1 $
 **********************************************************************/


#ifndef _O_RPX_PJ_MGR_H_  // Parallel Job Manager
#define _O_RPX_PJ_MGR_H_ 1

#include <idl.h>
#include <ideErrorMgr.h>
#include <iduMutex.h>

#include <rpxSender.h>
#include <rpdMeta.h>

class rpxPJChild;

class rpxPJMgr : public idtBaseThread
{
private:
    rpxPJChild  ** mChildArray;

    SInt           mChildCount;
    iduMutex       mMutex;
    idBool       * mExitFlag;
    idBool         mIsError;

    iduMemPool     mMemPool;

    iduMutex       mJobMutex;

    smiStatement * mStatement; 

    iduList        mSyncList;
    
    void      removeTotalSyncItems();

public:
    rpxPJMgr();
    ~rpxPJMgr(){};
    idBool    mPJMgrExitFlag;
    IDE_RC    initialize( SChar        * aRepName,
                          smiStatement * aStatement,
                          rpdMeta      * aMeta,
                          RP_SOCKET_TYPE aSocketType,
                          SChar        * aIPAddress,
                          UInt           aPortNo,
                          rpIBLatency    aIBLatency,
                          idBool       * aExitFlag,
                          SInt           aParallelFactor,
                          smiStatement * aParallelSmiStmts );
    void      destroy();
    void      run();

    IDE_RC    allocSyncItem( rpdMetaItem * aTable );     
    ULong     getSyncedCount( SChar *aTableName );

    idBool    getError()  { return mIsError; }
    IDE_RC    lock()      { return mMutex.lock(NULL /*idvSQL* */); }
    IDE_RC    unlock()    { return mMutex.unlock(); }

};

#endif

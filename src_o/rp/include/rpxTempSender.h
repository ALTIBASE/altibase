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
 * $Id: rpxTempSender.h 87362 2020-04-27 08:12:46Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_RPC_TEMP_SYNC_MGR_H_
#define _O_RPC_TEMP_SYNC_MGR_H_ 1

#include <idl.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <smrDef.h>
#include <qci.h>
#include <rp.h>
#include <rpdMeta.h>
#include <rpnComm.h>
#include <rpnMessenger.h>

class rpxTempSender : public idtBaseThread
{
/* function */
public:
    rpxTempSender() : idtBaseThread() {};
    virtual ~rpxTempSender() {};

    IDE_RC initialize( cmiProtocolContext * aProtocolContext,
                       rpdReplications    * aReplication,
                       rpdVersion         * aVersion,
                       rpdReplSyncItem   ** aTempSyncItemList );
    void   finalize();

    void   destroy();

    IDE_RC initializeThread();
    void   finalizeThread();


    void  setExit( idBool aExitFlag ) 
    { 
        IDE_ASSERT( mExitMutex.lock( NULL ) == IDE_SUCCESS );
        mExitFlag = aExitFlag; 
        IDE_ASSERT( mExitMutex.unlock( ) == IDE_SUCCESS );
    }
    idBool  isExit( ) 
    {
        idBool sExitFlag;
        IDE_ASSERT( mExitMutex.lock( NULL ) == IDE_SUCCESS );

        sExitFlag = mExitFlag;

        IDE_ASSERT( mExitMutex.unlock( ) == IDE_SUCCESS );
        return sExitFlag; 
    }
    IDE_RC  waitThreadJoin( idvSQL *aStatistics );

    void   run();

private:
    IDE_RC setTableOIDAtSyncItemList( smiStatement * aSmiStmt );
    IDE_RC tempSyncStart( smiTrans * aTrans );

/* variable */
public:
    iduListNode      mNode;
private:
    cmiProtocolContext * mProtocolContext;
    rpdMeta              mLocalMeta;
    rpdMeta              mRemoteMeta;
    rpdVersion           mRemoteVersion;
    rpdReplSyncItem    * mTempSyncItemList;

    iduMutex             mExitMutex;
    idBool               mExitFlag;

    idvSession           mSession;
    ULong                mEventFlag;
    idvSQL               mStatistics;
    idBool               mIsThreadDead;
    iduMutex             mThreadJoinMutex;
    iduCond              mThreadJoinCV;
};

#endif  /* _O_RPC_TEMP_SYNC_MGR_H_ */

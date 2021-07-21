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
* $Id: 
***********************************************************************/

#ifndef _O_RPX_DDL_EXECUTOR_H_
#define _O_RPX_DDL_EXECUTOR_H_ 1

#include <idu.h>
#include <idl.h>
#include <iduMemory.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <smi.h>

#include <rp.h>
#include <rpxSender.h>
#include <rpcHBT.h>
#include <rpcDDLSyncManager.h>

class rpxDDLExecutor : public idtBaseThread
{
/* Variable */
public:
    iduListNode      mNode;

private:
    rpdVersion           mVersion;
    idBool               mIsThreadDead;
    iduMutex             mThreadJoinMutex;
    iduCond              mThreadJoinCV;
    void               * mHBTResource;     /* for HBT */
    cmiProtocolContext * mProtocolContext;
    idvSession           mSession;
    ULong                mEventFlag;
    idvSQL               mStatistics;
    idBool               mExitFlag;
    SChar                mRepName[QC_MAX_NAME_LEN + 1];

    /* Functions */
public:
    rpxDDLExecutor() : idtBaseThread() {};
    virtual ~rpxDDLExecutor() {};

    IDE_RC initializeThread();
    void   finalizeThread();

    void   run();

    IDE_RC initialize( cmiProtocolContext * aProtocolContext, rpdVersion * aVersion );
    void   finalize( void );

    void   destroy( void );

    IDE_RC  waitThreadJoin( idvSQL *aStatistics );

    void   setExitFlag( idBool aExitFlag ) 
    { 
        mExitFlag = aExitFlag; 
    }

    idvSQL * getStatistics( void )
    {
        return &mStatistics;
    }

    SChar * getRepName( void )
    {
        return mRepName;
    }

    idBool isExit( void ) 
    { 
        return mExitFlag; 
    }

private:
};

#endif

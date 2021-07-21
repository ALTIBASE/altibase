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
 * $Id: rpcHBT.h 82877 2018-04-24 00:34:35Z returns $
 **********************************************************************/

/*
 *  rpcHBT is the Heart Beat Thread for Altibase Replication.
 *  HBT can detect whether each Hosts Network is dead or not.
 *  All Methods of HBT is used by rpcManager & rpcSender.
 */

#ifndef _O_RPC_HBT_H_
#define _O_RPC_HBT_H_ 1

#include <idl.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <rp.h>
#include <rpdLogAnalyzer.h>
#include <rpdMeta.h>

#include <rpnSocket.h>
#include <rpnPoll.h>

typedef enum
{
    RP_HOST_STATUS_NONE       = 0,
    RP_HOST_STATUS_INIT       = 1,
    RP_HOST_STATUS_CONNECTING = 2,
    RP_HOST_STATUS_CONNECTED  = 3,
    RP_HOST_STATUS_ERROR      = 4,
    RP_HOST_STATUS_INIT_ERROR = 5,

    RP_HOST_STATUS_MAX

} rpcHostStatus;

typedef struct rpcHostRsc
{
    SInt                 mRefCount;
    rpcHostStatus        mStatus;

    rpnSocket            mSocket;

    SChar                mHostIP[QCI_MAX_NAME_LEN + 1];
    SInt                 mPort;

    UInt                 mWaterMark;    /* error on over > REPLICATION_HBT_DETECT_HIGHWATER_MARK */
    idBool               mFault;        /* if ID_TRUE, there are some problems */

    idBool               mShouldBeCleaned;  /* when detect connection event, try to unregistHost, 
                                               it should not delete in unregistHost function */

    iduListNode          mNode;
    UInt                 mErrorNumber;
} rpcHostRsc ;

class rpcHBT : public idtBaseThread
{
    static rpcHBT    *mSelf;
    static iduMutex   mMutex;
    static iduCond    mCond;
    static idBool     mKillFlag;

    static void        initHostRsc( rpcHostRsc * aRsc, 
                                    SChar      * aIP, 
                                    SInt         aPort );
    static void        finalHostRsc(rpcHostRsc *);
    static rpcHostRsc* findHost(SChar *aIP, SInt aPort);

    static void insertList( rpcHostRsc * aRsc );
    static void deleteList( rpcHostRsc * aRsc );

    void   status(rpcHostRsc *);
    IDE_RC readyDetection(rpcHostRsc *aHost);
    IDE_RC proceedDetection(rpcHostRsc *aHost);
    IDE_RC errorDetection(rpcHostRsc *aHost);

    IDE_RC detectFault(rpcHostRsc *aHost);

public:
    rpcHBT();
    IDE_RC initialize();
    void   destroy();

    void   run();
    void   stop();              // Á¾·á called by mmiMgr
    static IDE_RC lock()     { return mMutex.lock(NULL /* idvSQL* */);   }
    static IDE_RC unlock()   { return mMutex.unlock(); }

    static IDE_RC  registHost( void **aRsc, SChar *aIP, SInt aPort );
    static void    unregistHost(void *aRsc);
    static idBool  checkFault(void *aRsc) { return ((rpcHostRsc *)aRsc)->mFault;}

private:
    iduList         mHostRscList;
    rpnPoll         mPoll;

    void            runHBT( UInt    * aSpentTimeSec );

    void            initializeHostRscList( void );
    void            connectFromHostRsc( rpcHostRsc * aRsc );
    void            connectFromHostRscList( void );
    void            prepareToDetectConnectionEvent( void );
    void            detectConnectionEvent( UInt          aTimeoutSec,
                                           UInt        * aSpentTimeSec );
    void            cleanupHostRscList( void );
    void            checkFaultFromHostRscList( void );
};

#endif  /* _O_RPC_SENDER_H_ */

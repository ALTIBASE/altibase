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
 **********************************************************************/

#ifndef _O_RPX_RECEIVER_APPLIER_H_
#define _O_RPX_RECEIVER_APPLIER_H_ 1

#include <idl.h>
#include <idtBaseThread.h>

#include <rpDef.h>
#include <rpxReceiver.h>
#include <rpxReceiverApply.h>

struct rpxReceiverParallelApplyInfo;

typedef enum rpxReceiverApplyStatus
{
    RECV_APPLIER_STATUS_INITIALIZE  = 0,
    RECV_APPLIER_STATUS_WORKING     = 1,
    RECV_APPLIER_STATUS_DEQUEUEING  = 2,
    RECV_APPLIER_STATUS_WAITING     = 3,
    RECV_APPLIER_STATUS_STOP        = 4
} rpxReceiverApplyStatus;

class rpxReceiverApplier : public idtBaseThread
{
    /* Variable */
private:
    rpxReceiver           * mReceiver;
    SChar                 * mRepName;
    rpxReceiverApply        mApply;
    UInt                    mMyIndex;
    iduMemAllocator       * mAllocator;
    rpReceiverStartMode     mStartMode;

    rpdQueue                mQueue;

    idBool                  mExitFlag;

    iduMutex                mMutex;
    iduCond                 mCV;
    idBool                  mIsWait;

    rpxReceiverApplyStatus  mStatus;

    smSN                    mProcessedSN;
    smSN                    mLastCommitSN;

    UInt                    mProcessedLogCount;

    SInt                    mAssignedXLogCount;

    idBool                  mIsDoneInitializeThread;

    iduMutex                mThreadJoinMutex;
    iduCond                 mThreadJoinCV;

    rprSNMapMgr           * mSNMapMgr;

public:

    /* Function */
private:
    void        run( void );
    IDE_RC      processXLog( rpdXLog    * aXLog,
                             idBool     * aIsEnd );
    void        updateProcessedSN( rpdXLog  * aXLog );

    void        releaseQueue( void );
public:
    rpxReceiverApplier() : idtBaseThread() { };
    virtual ~rpxReceiverApplier() { };

    void      initialize( rpxReceiver           * aReceiver,
                          SChar                 * aRepName,
                          UInt                    aApplierIndex,
                          rpReceiverStartMode     aStartMode,
                          iduMemAllocator       * aAllocator );

    void        finalize( void );

    IDE_RC      initializeThread( void );
    void        finalizeThread( void );

    void        enqueue( rpdXLog     * aXLog );
    IDE_RC      dequeue( rpdXLog    ** aXLog );

    void        setTransactionFlagReplReplicated( void );
    void        setTransactionFlagReplRecovery( void );
    void        setTransactionFlagCommitWriteWait( void );
    void        setTransactionFlagCommitWriteNoWait( void );

    void        setFlagToSendAckForEachTransactionCommit( void );
    void        setFlagNotToSendAckForEachTransactionCommit( void );

    void        setApplyPolicyCheck( void );
    void        setApplyPolicyForce( void );
    void        setApplyPolicyByProperty( void );

    void        setSNMapMgr( rprSNMapMgr * aSNMapMgr );

    smSN        getProcessedSN( void );
    smSN        getLastCommitSN( void );

    SInt        getATransCntFromTransTbl( void );

    UInt        getAssingedXLogCount( void );

    smSN        getRestartSN( void );

    void        setExitFlag( void );

    IDE_RC      allocRangeColumn( UInt   aCount );

    void        setParallelApplyInfo( rpxReceiverParallelApplyInfo * aApplierInfo );

    IDE_RC      buildRecordForReplReceiverParallelApply( void                * aHeader,
                                                         void                * aDumpObj,
                                                         iduFixedTableMemory * aMemory );

    IDE_RC      buildRecordForReplReceiverTransTbl( void                    * aHeader,
                                                    void                    * aDumpObj,
                                                    iduFixedTableMemory     * aMemory,
                                                    UInt                      aParallelID );

    UInt        getQueSize( void );

    void        wakeup( void );
};

inline UInt rpxReceiverApplier::getQueSize()
{
    return mQueue.getSize();
}

#endif

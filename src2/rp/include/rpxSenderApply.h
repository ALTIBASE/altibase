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
 

#ifndef _O_RPX_SENDER_APPLY_H_
#define _O_RPX_SENDER_APPLY_H_ 1

#include <idl.h>
#include <idtBaseThread.h>

#include <cm.h>
#include <smrDef.h>
#include <smi.h>
#include <smiLogRec.h>

#include <rp.h>
#include <rpuProperty.h>
#include <rpnComm.h>
#include <rpdSenderInfo.h>
#include <rpxSender.h>

class rpxSender;
class rpnMessenger;

typedef struct rpxRestartSN
{
    smSN mRestartSN;
    iduListNode   mNode;
} rpxRestartSN;

class rpxSenderApply : public idtBaseThread
{
public:

    rpxSenderApply();
    virtual ~rpxSenderApply() {};
    /* PROJ-1915 start flag를 전달 하여 off-line sender로 동작 할때
     * updateXSN을 meta에 반영 하지 않는다.
     */
    IDE_RC initialize(rpxSender        *aSender,
                      idvSQL           *aOpStatistics,
                      idvSession      * aStatSession,
                      rpdSenderInfo    *aSenderInfo,
                      rpnMessenger    * aMessenger,
                      rpdMeta          *aMeta,
                      void             *aRsc,   // BUG-29689 HBT Check
                      UInt             *aRetryCount,
                      idBool           *aNetworkError,
                      idBool           *aApplyFaultFlag,
                      idBool           *aSenderStopFlag,
                      idBool            aIsSupportRecovery,
                      RP_SENDER_TYPE   *aSenderType,
                      UInt              aParallelID,
                      RP_SENDER_STATUS *aStatus,
                      RP_SOCKET_TYPE    aSocketType);

    IDE_RC updateXSN(smSN);
    void   destroy();
    void   shutdown();

    /* BUG-38533 numa aware thread initialize */
    IDE_RC initializeThread();
    void   finalizeThread();

    IDE_RC insertRestartSNforRecovery(smSN aRestartSN);
    void   getRestartSNforRecovery(smSN* aRestartSN, smSN aFlushSN);
    void   getMinRestartSNFromAllApply( smSN* aRestartSN );
    void   run();

    idBool isExit() { return mExitFlag; }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * Handshake 중 SenderApply 중지
     */
    inline idBool isSuspended() { return mIsSuspended; }
    inline void   resume()      { mIsSuspended = ID_FALSE; }
    smSN          getMinRestartSN();
    inline idBool isParallelParent()
    {
        return ((*mSenderType == RP_PARALLEL)&&
                (mParallelID == RP_PARALLEL_PARENT_ID)) ? ID_TRUE : ID_FALSE;
    }
    inline idBool isParallelChild()
    {
        return ((*mSenderType == RP_PARALLEL)&&
                (mParallelID != RP_PARALLEL_PARENT_ID)) ? ID_TRUE : ID_FALSE;
    }

private:
    rpxSender        *mSender;
    SChar            *mRepName;         //[QC_MAX_OBJECT_NAME_LEN+1]
    rpXLogAck         mReceivedAck;
    rpdSenderInfo    *mSenderInfo;      //only one sender info's pointer
    rpnMessenger    * mMessenger;
    rpdMeta          *mMeta;
    void             *mRsc;             // BUG-29689 HBT Check
    UInt             *mRetryCount;
    idBool           *mRetryError;
    idBool           *mApplyFaultFlag;
    idBool           *mSenderStopFlag;
    idBool            mExitFlag;
    SInt              mRole;            // PROJ-1537
    smSN              mPrevRestartSN;

    /*PROJ-1608*/
    iduList           mRestartSNList;
    idBool            mIsSupportRecovery;
    iduMemPool        mRestartSNPool;

    smSN              mMinRestartSN;

    idBool            mIsSuspended;

    /* PROJ-1915 */
    RP_SENDER_TYPE   *mSenderType;
    UInt              mParallelID;

    RP_SENDER_STATUS *mStatus;

    /* BUG-31545 수행시간 통계정보 */
    idvSQL          * mOpStatistics;
    idvSession      * mStatSession;

    smSN              mEagerUpdatedRestartSNGap;

    RP_SOCKET_TYPE    mSocketType;

    /* Function */
    IDE_RC            checkHBT( void );
};

#endif  /* _O_RPX_SENDER_APPLY_H_ */

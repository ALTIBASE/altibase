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
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDU_SHM_MSG_MGR_H_)
#define _O_IDU_SHM_MSG_MGR_H_ 1

#include <iduProperty.h>
#include <idtBaseThread.h>
#include <idTypes.h>
#include <iduCond.h>
#include <iduShmDef.h>
#include <iduVersion.h>
#include <idwPMMgr.h>

#define IDU_SHM_MAX_MSG   256

typedef enum iduShmMsgReqMode
{
    IDU_SHM_MSG_NULL,
    IDU_SHM_MSG_SYNC,
    IDU_SHM_MSG_ASYNC
} iduShmMsgReqMode;

typedef enum iduStReqState
{
    IDU_SHM_MSG_STATE_REQUEST,
    IDU_SHM_MSG_STATE_REQUEST_BY_DAEMON,
    IDU_SHM_MSG_STATE_PROCESS,
    IDU_SHM_MSG_STATE_FINISH,
    IDU_SHM_MSG_STATE_CHECKED_BY_REQUESTOR,
    IDU_SHM_MSG_STATE_ERROR
} iduStReqState;

typedef struct iduStShmMsg
{
    union
    {
        idShmAddr           mAddrSelf;
        iduShmListNode      mNode;
    };

    idGblThrID          mTgtThrID;
    idGblThrID          mReqThrID;
    UInt                mErrorCode;
    iduStReqState       mState;
    iduShmMsgReqMode    mReqMode;
} iduStShmMsg;

typedef struct iduStShmMsgMgr
{
    idShmAddr       mAddrSelf;
    iduShmLatch     mLatch;
    iduShmListNode  mBaseNode4MsgLst;
    UInt            mUserMsgSize;
    iduStShmMemPool mMsgMemPool;
} iduStShmMsgMgr;

typedef IDE_RC (*iduShmMsgProcessFunc)( idvSQL       * aStatistics,
                                        iduShmTxInfo * aShmTxInfo,
                                        void         * aReqMsg );

class iduShmMsgMgr : public idtBaseThread
{
public:
    iduShmMsgMgr();
    virtual ~iduShmMsgMgr();

    IDE_RC initialize( idvSQL               * aStatistics,
                       iduShmTxInfo         * aShmTxInfo,
                       iduMemoryClientIndex   aIndex,
                       SChar                * aStrName,
                       UInt                   aMsgSize,
                       iduShmMsgProcessFunc   aMsgProcessFunc,
                       idShmAddr              aAddr4StMsgMgr,
                       iduStShmMsgMgr       * aStShmMsgMgr );

    IDE_RC destroy( idvSQL * aStatistics, iduShmTxInfo  * aShmTxInfo );

    IDE_RC attach( idvSQL             * aStatistics,
                   iduStShmMsgMgr     * aStShmMsgMgr,
                   iduMemoryClientIndex aIndex,
                   SChar              * aStrName,
                   iduShmMsgProcessFunc aProcessFunc );

    IDE_RC detach( idvSQL * aStatistics );

    IDE_RC sendRequest( idvSQL             * aStatistics,
                        iduShmTxInfo       * aShmTxInfo,
                        void               * aMsg,
                        iduShmMsgReqMode     aReqMode );

    IDE_RC recvRequest( idvSQL             * aStatistics,
                        iduShmTxInfo       * aShmTxInfo,
                        void              ** aReqMsg );

    inline void setMsgState( iduStReqState    aMsgState,
                             void           * aReqMsg );

    inline void setMsgErrCode( UInt   aErrorCode,
                               void * aReqMsg );

    IDE_RC  startThread();
    IDE_RC  shutdown( idvSQL * aStatistics );
    virtual void run();

private:
    inline void initShmMsg( iduShmTxInfo       * aShmTxInfo,
                            idShmAddr            aAddrSelf,
                            idGblThrID           aTgtThrID,
                            iduShmMsgReqMode     aReqMode,
                            iduStShmMsg        * aShmMsg );

    inline IDE_RC lock( idvSQL* aStatistics );
    inline IDE_RC unlock();

private:
    iduShmMsgProcessFunc  mMsgProcessFunc;
    iduStShmMsgMgr      * mStShmMsgMgr;
    iduShmProcInfo      * mStProcInfo;
    iduCond               mCV;
    iduMemoryClientIndex  mIndex;
    iduMutex              mMutex;
    idBool                mFinish;
    idGblThrID            mTgtThrID;
    UInt                  mMsgSize;
};

inline IDE_RC iduShmMsgMgr::lock( idvSQL* aStatistics )
{
    return mMutex.lock( aStatistics );
}

inline IDE_RC iduShmMsgMgr::unlock()
{
    return mMutex.unlock();
}

void iduShmMsgMgr::initShmMsg( iduShmTxInfo       * aShmTxInfo,
                               idShmAddr            aAddrSelf,
                               idGblThrID           aTgtThrID,
                               iduShmMsgReqMode     aReqMode,
                               iduStShmMsg        * aShmMsg )
{
    iduShmList::initNode( &aShmMsg->mNode,
                          aAddrSelf,
                          IDU_SHM_NULL_ADDR );

    aShmMsg->mTgtThrID  = aTgtThrID;
    aShmMsg->mReqThrID  = idwPMMgr::getGblThrID( aShmTxInfo );

    aShmMsg->mReqMode   = aReqMode;
    aShmMsg->mErrorCode = 0;
    aShmMsg->mState     = IDU_SHM_MSG_STATE_REQUEST;

    if( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        aShmMsg->mState = IDU_SHM_MSG_STATE_REQUEST_BY_DAEMON;
    }
}

void iduShmMsgMgr::setMsgState( iduStReqState    aMsgState,
                                void           * aReqMsg )
{
    iduStShmMsg * sShmMsg = (iduStShmMsg*)aReqMsg - 1;

    IDL_MEM_BARRIER;
    sShmMsg->mState = aMsgState;
}

void iduShmMsgMgr::setMsgErrCode( UInt             aErrorCode,
                                  void           * aReqMsg )
{
    iduStShmMsg * sShmMsg = (iduStShmMsg*)aReqMsg - 1;

    IDL_MEM_BARRIER;
    sShmMsg->mErrorCode = aErrorCode;
}

#endif /* _O_IDU_SHM_MSG_MGR_H_ */

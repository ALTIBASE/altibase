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

#if !defined(_O_IDW_PM_MGR_H_)
#define _O_IDW_PM_MGR_H_ 1

#include <idwDef.h>
#include <idu.h>
#include <iduShmList.h>
#include <idrShmTxPendingOp.h>

typedef struct idwShmProcFTInfo
{
    idShmAddr        mSelfAddr;
    ULong            mPID;
    iduShmProcState  mState;
    UInt             mLPID;
    iduShmProcType   mType;
    UInt             mThreadCnt;
    iduShmTxInfo     mMainTxInfo;
    UInt             mLogBuffSize;
    UInt             mLogOffset;
    key_t            mSemKey;
    SInt             mSemID;
    iduShmLatch      mLatch;
} idwShmProcFTInfo;

class idwPMMgr
{
public:
    static IDE_RC initialize( iduShmProcType aProcType );
    static IDE_RC destroy();

    static IDE_RC attach( iduShmProcType aProcType );
    static IDE_RC detach();

    static IDE_RC registerProc( idvSQL            * aStatistics,
                                UInt                aProcessID,
                                iduShmProcType      aProcType,
                                iduShmProcInfo   ** aProcInfo );

    static IDE_RC unRegisterProc( idvSQL         * aStatistics,
                                  iduShmProcInfo * aProcInfo );

    static IDE_RC allocThrInfo( idvSQL          * aStatistics,
                                UInt              aFlag,
                                iduShmTxInfo  ** aThrInfo );

    static IDE_RC freeThrInfo( idvSQL          * aStatistics,
                               iduShmTxInfo   * aThrInfo );

    static inline void setRestartProcCallback( iduProcRestartFunc   aRestartProcFunc );

    static iduShmProcMgrInfo * getProcMgrInfo(){ return mProcMgrInfo;};
    static IDE_RC getDeadProc( iduShmProcInfo  ** aProcInfo );
    static iduShmProcInfo* getProcInfo( idLPID aLPID );
    static inline iduShmProcInfo* getProcInfo();
    static inline idGblThrID getGblThrID( iduShmTxInfo * aShmTxInfo );
    static IDE_RC acquireSem4Proc( iduShmProcInfo  * aProcInfo );
    static IDE_RC tryAcquireSem4Proc( iduShmProcInfo  * aProcInfo,
                                      idBool       * aAcquired );
    static IDE_RC releaseSem4Proc( iduShmProcInfo  * aProcInfo );

    static inline void initThrInfo( iduShmProcInfo * aPRInfo,
                                    idShmAddr        aSelfAddr,
                                    UInt             aFlag,
                                    idBool           aIsHead,
                                    iduShmTxInfo  * aThrInfo );

    static inline iduShmProcType getProcType();
    static inline ULong getPID( idvSQL * aStatistics );

    static inline IDE_RC lockProcess( idvSQL * aStatistics, iduShmProcInfo  * aProcInfo );
    static inline IDE_RC unlockProcess( idvSQL * aStatistics, iduShmProcInfo  * aProcInfo );

    static inline idBool isLogEnabled( iduShmTxInfo * aThrInfo );
    static inline void   setLogMode( iduShmTxInfo * aThrInfo, idBool aLogMode );
    static inline void   setProcState( iduShmProcInfo * aDeadProcInfo,
                                       iduShmProcState  aProcState );

    static inline iduShmProcInfo * getMainShmProcInfo()
    {
        return mProcInfo4MainProcess;
    }

    static inline iduShmTxInfo * getMainShmThrInfo( idvSQL * aStatistics );

    static iduShmProcInfo * getDaemonProcInfo();

    static IDE_RC checkProcAliveByLPID( idLPID aLPID, idBool * aIsAlive );
    static IDE_RC checkDaemonProcAlive( idBool * aIsAlive );
    static IDE_RC getNxtAliveProc( iduShmProcInfo ** aProcInfo );
    static IDE_RC getNxtDeadProc( iduShmProcInfo ** aProcInfo );
    static IDE_RC getAliveProcCnt( UInt * aAliveProcCnt );

    static inline ULong getPID4Proc( iduShmProcInfo * aPRInfo )
    {
        return aPRInfo->mPID;
    }

    static inline ULong getLPID4Proc( iduShmProcInfo * aPRInfo )
    {
        return aPRInfo->mLPID;
    }

    static inline iduShmProcState  getProcState( idvSQL * aStatistics );
    static inline iduShmProcInfo * getProcInfo( idvSQL * aStatistics );

    static inline void validateThrID( idGblThrID aTgtThrID );

    static iduShmTxInfo * getTxInfo( UInt aLProcID, UInt aShmTxID );

    static IDE_RC unRegisterProcByWatchDog( idvSQL         * aStatistics,
                                            iduShmProcInfo * aProcInfo );

    static inline void disableResisterNewProc();
    static inline void enableResisterNewProc();

    static UInt getRegistProcCnt();

    static inline idBool isDaemonProc( idvSQL * aStatistics );

public:
    static acp_sem_t             mSemHandle;
    static iduShmProcMgrInfo   * mProcMgrInfo;
    static iduShmProcInfo      * mProcInfo4MainProcess;
    static iduShmProcType        mProcType;
};

inline void idwPMMgr::initThrInfo( iduShmProcInfo * aPRInfo,
                                   idShmAddr        aSelfAddr,
                                   UInt             aFlag,
                                   idBool           aIsHead,
                                   iduShmTxInfo   * aThrInfo )
{
    iduShmTxInfo   * sLstThrInfo;
    idShmAddr        sAddr4Obj;
    iduShmListNode * sPrvNode;

    aThrInfo->mSelfAddr = aSelfAddr;

    sAddr4Obj = IDU_SHM_GET_ADDR_SUBMEMBER( aSelfAddr, iduShmTxInfo, mNode );

    aThrInfo->mLPID = aPRInfo->mLPID;

    if( aIsHead == ID_TRUE )
    {
        iduShmList::initBase( &aThrInfo->mNode,
                              sAddr4Obj,   /* Self Addr */
                              aSelfAddr ); /* Data Addr */

        aThrInfo->mThrID = IDW_THRID_MAKE( aPRInfo->mLPID, 0 );
    }
    else
    {
        sPrvNode = iduShmList::getPrvPtr( &aPRInfo->mMainTxInfo.mNode );

        sLstThrInfo = (iduShmTxInfo*)iduShmList::getDataPtr( sPrvNode );
        IDE_ASSERT( sLstThrInfo != NULL );

        iduShmList::initNode( &aThrInfo->mNode,
                              sAddr4Obj /* Self Addr */,
                              aSelfAddr /* Data Addr */);

        aThrInfo->mThrID =
            IDW_THRID_MAKE( aThrInfo->mLPID,
                            IDW_THRID_THRNUM( sLstThrInfo->mThrID ) + 1 );
    }

    aThrInfo->mLogOffset   = 0;
    aThrInfo->mFlag        = aFlag;
    aThrInfo->mState       = IDU_SHM_THR_STATE_RUN;

    aThrInfo->mLatchStack.mCurSize = 0;
    aThrInfo->mSetLatchStack.mCurSize = 0;

    idrShmTxPendingOp::initializePendingOpList( aThrInfo );
}

inline iduShmProcInfo* idwPMMgr::getProcInfo( idLPID aLPID )
{
    return mProcMgrInfo->mPRTable + aLPID;
}

inline iduShmProcInfo* idwPMMgr::getProcInfo()
{
    return mProcInfo4MainProcess;
}

inline IDE_RC idwPMMgr::lockProcess( idvSQL          * aStatistics,
                                     iduShmProcInfo  * aProcInfo )
{
    return iduShmLatchAcquire( aStatistics,
                               getMainShmThrInfo( aStatistics ),
                               &aProcInfo->mLatch );
}

inline IDE_RC idwPMMgr::unlockProcess( idvSQL         * aStatistics,
                                       iduShmProcInfo * aProcInfo )
{
    return iduShmLatchRelease( getMainShmThrInfo( aStatistics ),
                               &aProcInfo->mLatch );
}

inline iduShmProcType idwPMMgr::getProcType()
{
    return mProcType;
}

inline ULong idwPMMgr::getPID( idvSQL * aStatistics )
{
    return ((iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics ))->mPID;
}

inline idBool idwPMMgr::isLogEnabled( iduShmTxInfo   * aThrInfo )
{
    if( ( aThrInfo->mFlag & IDU_SHM_THR_LOGGING_MASK ) ==
        IDU_SHM_THR_LOGGING_ON )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;

    }
}

inline void idwPMMgr::setLogMode( iduShmTxInfo * aThrInfo, idBool aLogMode )
{
    aThrInfo->mFlag &= ~IDU_SHM_THR_LOGGING_MASK;

    if( aLogMode == ID_TRUE )
    {
        aThrInfo->mFlag |= IDU_SHM_THR_LOGGING_ON;
    }
    else
    {
        aThrInfo->mFlag |= IDU_SHM_THR_LOGGING_OFF;
    }
}

inline void idwPMMgr::setProcState( iduShmProcInfo * aDeadProcInfo,
                                    iduShmProcState  aProcState )
{
    aDeadProcInfo->mState = aProcState;
}

inline iduShmProcInfo * idwPMMgr::getProcInfo( idvSQL * aStatistics )
{
    iduShmProcInfo *sProcInfo;
    sProcInfo = (iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics );
    IDE_ASSERT( sProcInfo != NULL );
    return sProcInfo;
}

inline iduShmProcState idwPMMgr::getProcState( idvSQL * aStatistics )
{
    iduShmProcInfo * sProcInfo;

    sProcInfo = getProcInfo( aStatistics );

    return sProcInfo->mState;
}

inline idGblThrID idwPMMgr::getGblThrID( iduShmTxInfo * aShmTxInfo )
{
    return aShmTxInfo->mThrID;
}

inline void idwPMMgr::validateThrID( idGblThrID aTgtThrID )
{
    IDE_ASSERT( IDW_THRID_LPID( aTgtThrID ) < IDU_MAX_PROCESS_COUNT );
}

inline void idwPMMgr::setRestartProcCallback( iduProcRestartFunc aRestartProcFunc )
{
    IDE_ASSERT( mProcInfo4MainProcess != NULL );

    mProcInfo4MainProcess->mRestartProcFunc = aRestartProcFunc;
}

inline iduShmTxInfo * idwPMMgr::getMainShmThrInfo( idvSQL * aStatistics )
{
    iduShmProcInfo * sProcInfo;

    sProcInfo = (iduShmProcInfo*)IDV_WP_GET_PROC_INFO( aStatistics );
    IDE_ASSERT( sProcInfo != NULL );
    return &sProcInfo->mMainTxInfo;
}

inline void idwPMMgr::disableResisterNewProc()
{
    mProcMgrInfo->mFlag = mProcMgrInfo->mFlag & ~IDU_SHM_PROC_REGISTER_MASK;
}

inline void idwPMMgr::enableResisterNewProc()
{
    mProcMgrInfo->mFlag |= IDU_SHM_PROC_REGISTER_ON;
}

inline idBool idwPMMgr::isDaemonProc( idvSQL * aStatistics )
{
    iduShmProcInfo * sProcInfo;

    sProcInfo = getProcInfo( aStatistics );

    return (sProcInfo->mType == IDU_PROC_TYPE_DAEMON) ? ID_TRUE : ID_FALSE;
}
#endif /* _O_IDW_PM_MGR_H_ */

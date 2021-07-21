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
 * $id$
 **********************************************************************/

#ifndef _O_DKS_SESSION_MGR_H_
#define _O_DKS_SESSION_MGR_H_ 1

#include <dks.h>
#include <idkAtomic.h>


class dksSessionMgr
{
private:
    static dksCtrlSession   mCtrlSession;
    static dksDataSession   mNotifyDataSession;
    static UInt             mDataSessionCnt;
    static iduList          mDataSessionList;
    static iduMutex         mDksMutex;

public:
    /* Initialize or finalize */
    static IDE_RC       initializeStatic();
    static IDE_RC       initCtrlSession();
    static IDE_RC       initNotifyDataSession();
    static IDE_RC       finalizeStatic();

    /* Create or destroy linker data session */
    static IDE_RC       createDataSession( UInt             aSessionId,
                                           dksDataSession  *aDataSession );
    static IDE_RC       destroyDataSession( dksDataSession  *aDataSession );

    /* Connect or disconnect */
    static IDE_RC       connectDataSession( dksDataSession  *aDataSession );
    static IDE_RC       invalidateLinkerDataSession( dksDataSession  *aDataSession );
    static IDE_RC       disconnectDataSession( dksDataSession  *aDataSession );
    static IDE_RC       checkAndDisconnectDataSession( dksDataSession  *aDataSession );

    static IDE_RC       connectControlSession( UShort aPortNo );
    static IDE_RC       connectNotifyDataSession( UShort aPortNo );
    static IDE_RC       disconnectControlSession();
    static IDE_RC       disconnectNotifyDataSession();
    
    static IDE_RC       checkConnection( dksSession  *aDksSession,
                                         idBool      *aIsConnected );

    /* Close session */
    static IDE_RC       closeCtrlSession();
    static void         closeNotifyDataSession();
    static IDE_RC       closeAllDataSessions();
    static IDE_RC       closeDataSession( dksDataSession *aSession );
    static IDE_RC       closeRemoteNodeSession( idvSQL          *aStatistics,
                                                dksDataSession  *aSession,
                                                SChar           *aDblinkName );

    /* Get information for performance view */
    static IDE_RC       getLinkerSessionInfo( dksLinkerSessionInfo ** aInfo, UInt * aSessionCnt );
    static IDE_RC       getLinkerCtrlSessionInfo( dksCtrlSessionInfo *aInfo );
    static IDE_RC       getLinkerDataSessionInfo( dksDataSessionInfo ** aInfo, UInt * aSessionCnt );
    static inline UInt  getLinkerSessionCount();

    /* Get lock or unlock for linker data session list */
    static IDE_RC       lock();
    static IDE_RC       unlock();

    /* Get dksSession from a linker session */
    static inline dksSession*  getCtrlDksSession();
    static inline dksSession*  getNotifyDataDksSession();
    static IDE_RC       getDataDksSession( UInt          aSessionId,
                                           dksSession  **aDksSession );

    /* Get linker control session */
    static inline dksCtrlSession*  getLinkerCtrlSession();
    static inline dksDataSession*  getLinkerNotifyDataSession();

    static inline UInt      getDataSessionCount();

    /* Get or set data session information */
    static inline iduList*  getDataSessionList();   

    static inline UInt      getDataSessionId( dksDataSession *aSession );

    static inline void      setDataSessionLocalTxId( dksDataSession *aSession,
                                                     UInt            aLocalTxId );
    static inline void      setDataSessionGlobalTxId( dksDataSession * aSession,
                                                      UInt             aGlobalTxId );
    static inline dktAtomicTxLevel   getDataSessionAtomicTxLevel( dksDataSession *aSession );
    
private:
    static IDE_RC createCtrlSessionProtocol( dksSession * aSession );
    static IDE_RC createNotifyDataSessionProtocol( dksSession * aSession );
    static IDE_RC createLinkerDataSession( dksDataSession * aDataSession );
    
    static IDE_RC connect( dksSession * aDksSession );
    static IDE_RC disconnect( dksSession * aDksSession, idBool aIsReuse );

    static IDE_RC connectControlSessionInternal( UShort aPortNo );
    static IDE_RC disconnectControlSessionInternal( void );
    static IDE_RC closeCtrlSessionInternal( void );
};

inline dksSession*  dksSessionMgr::getCtrlDksSession()
{
    return &mCtrlSession.mSession;
}

inline dksSession*  dksSessionMgr::getNotifyDataDksSession()
{
    return &mNotifyDataSession.mSession;
}

inline dksCtrlSession *  dksSessionMgr::getLinkerCtrlSession()
{
    return &mCtrlSession;
}

inline dksDataSession *  dksSessionMgr::getLinkerNotifyDataSession()
{
    return &mNotifyDataSession;
}

inline UInt dksSessionMgr::getLinkerSessionCount()
{
    return mDataSessionCnt + 1;
}

inline UInt dksSessionMgr::getDataSessionCount()
{
    return mDataSessionCnt;
}

inline UInt dksSessionMgr::getDataSessionId( dksDataSession *aSession )
{
    return aSession->mId;
}

inline iduList*  dksSessionMgr::getDataSessionList()
{
    return &mDataSessionList;
}

inline void dksSessionMgr::setDataSessionLocalTxId( dksDataSession * aSession,
                                                    UInt             aLocalTxId )
{
    aSession->mLocalTxId = aLocalTxId;
}

inline void dksSessionMgr::setDataSessionGlobalTxId( dksDataSession * aSession,
                                                     UInt             aGlobalTxId )
{
    aSession->mGlobalTxId = aGlobalTxId;
}

inline dktAtomicTxLevel dksSessionMgr::getDataSessionAtomicTxLevel( dksDataSession *aSession )
{
    return aSession->mAtomicTxLevel;
}

#endif /* _O_DKS_SESSION_MGR_H_ */

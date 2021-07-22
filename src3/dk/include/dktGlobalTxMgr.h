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

#ifndef _O_DKT_GLOBAL_TX_MGR_H_
#define _O_DKT_GLOBAL_TX_MGR_H_ 1

#include <dkt.h>
#include <dktGlobalCoordinator.h>
#include <dktNotifier.h>

/* BUG-48501 */
typedef struct dktGlobalCoordinatorList
{
    iduList  mHead;
    iduLatch mLatch;
    UInt     mCnt;
} dktGlobalCoordinatorList;

class dktGlobalTxMgr
{
private:

    static iduMemPool                 mGlobalCoordinatorPool;

    static SLong                      mUniqueGlobalTxSeq;

    /* DK 에 생성되어 존재하는 모든 global coordinator 들의 리스트 */
    static dktGlobalCoordinatorList * mGlobalCoordinatorList;

    static dktNotifier                mNotifier;  /* PROJ-2569 2PC */

public:

    static void lockRead( UInt aIdx );
    static void lockWrite( UInt aIdx );
    static void unlock( UInt aIdx );

    static UChar         mMacAddr[ACP_SYS_MAC_ADDR_LEN];
    static UInt          mInitTime;
    static SInt          mProcessID;
    /* Initialize / Finalize */
    static IDE_RC       initializeStatic();
    static IDE_RC       finalizeStatic();

    /* Create / Destroy global coordinator */
    static IDE_RC       createGlobalCoordinator( dksDataSession        * aSession,
                                                 UInt                    aLocalTxId,
                                                 dktGlobalCoordinator ** aGlobalCoordinator );

    static void         destroyGlobalCoordinator( dktGlobalCoordinator  *aGlobalCrd );

    /* Get information for performance view */
    static IDE_RC       getAllGlobalTransactonInfo( dktGlobalTxInfo ** aInfo, UInt * aGTxCnt );
    static IDE_RC       getAllRemoteTransactonInfo( dktRemoteTxInfo ** aInfo,
                                                    UInt             * aRTxCnt );
    static IDE_RC       getAllRemoteStmtInfo( dktRemoteStmtInfo * aInfo,
                                              UInt              * aRemoteStmtCnt );

    static void         getAllRemoteTransactionCount( UInt * aCount );

    static IDE_RC       getAllRemoteStmtCount( UInt  *aCount );

    /* 입력받은 global coordinator 를 관리대상으로 추가한다. */
    static void         addGlobalCoordinatorToList( dktGlobalCoordinator * aGlobalCrd );

    /* Global transaction id 를 입력받아 해당 global transaction 을 수행한
       global coordinator list node 를 list 에서 찾아 반환한다. */
    static IDE_RC       findGlobalCoordinator( 
                                        UInt                  aGlobalTxId,
                                        dktGlobalCoordinator **aGlobalCrd );

    /* Linker data session id 를 갖는 global coordinator 를 찾는다.*/
    static IDE_RC       findGlobalCoordinatorWithSessionId( 
                                        UInt                   aSessionId, 
                                        dktGlobalCoordinator **aGlobalCrd );

    static void         removeGlobalCoordinatorFromList( dktGlobalCoordinator  *aGlobalCoordinator );

    static UInt         generateGlobalTxId();

    static inline UInt  getActiveGlobalCoordinatorCnt();
    
    static IDE_RC  getDtxMinLSN( smLSN * aMinLSN );

    static inline dktNotifier *  getNotifier();
    static smLSN getDtxMinLSN( void );
    static UInt getNotifierBranchTxCnt();
    static IDE_RC getNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                              UInt                        * aInfoCount );
    static IDE_RC getShardNotifierTransactionInfo( dktNotifierTransactionInfo ** aInfo,
                                                   UInt                        * aInfoCount );
    static inline idBool isGT( const smLSN * aLSN1,
                               const smLSN * aLSN2 );
    
    static IDE_RC createGlobalCoordinatorAndSetSessionTxId( dksDataSession        * aSession,
                                                            UInt                    aLocalTxId,
                                                            dktGlobalCoordinator ** aGlobalCoordinator );

    static void   destroyGlobalCoordinatorAndUnSetSessionTxId(  dktGlobalCoordinator * aGlobalCoordinator,
                                                                dksDataSession       * aSession );
};

inline dktNotifier * dktGlobalTxMgr::getNotifier()
{
    return &mNotifier;
}

inline idBool dktGlobalTxMgr::isGT( const smLSN * aLSN1, 
                                    const smLSN * aLSN2 )
{
    if ( (UInt)(aLSN1->mFileNo) > (UInt)(aLSN2->mFileNo) )
    {
        return ID_TRUE;
    }
    else
    {
        if ( (UInt)(aLSN1->mFileNo) == (UInt)(aLSN2->mFileNo) )
        {
            if ( (UInt)(aLSN1->mOffset) > (UInt)(aLSN2->mOffset) )
            {
                return ID_TRUE;
            }
            else
            {
                /* do nothing */
            }
        }
        else
        {
            /* do nothing */
        }
    }

    return ID_FALSE;
}

#endif /* _O_DKT_GLOBAL_TX_MGR_H_ */


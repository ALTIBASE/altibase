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

#ifndef _O_SMX_LEGACY_TRANS_MGR_H_
#define _O_SMX_LEGACY_TRANS_MGR_H_ 1

#include <idu.h>
#include <smxDef.h>
#include <smxTrans.h>

class smxLegacyTransMgr
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static IDE_RC addLegacyTrans( smxTrans *  aSmxTrans,
                                  smLSN       aCommitEndLSN,
                                  void     ** aLegacyTrans,
                                  idBool      aIsCommit );

    static IDE_RC removeLegacyTrans( smuList * aLegacyTransNode,
                                     smTID     aTID );

    static IDE_RC allocAndSetLegacyStmt( void * aLegacyTrans,
                                         void * aStmtListHead );

    static IDE_RC removeLegacyStmt( smuList * aLegacyStmtNode,
                                    void    * aStmtListHead );

    static smuList* getLegacyTransNodeByTID( smTID aTID );

    static smSCN  getCommitSCN( smTID aTID );

    static inline UInt getLegacyTransCnt() {return mLegacyTransCnt; }

    static inline IDE_RC lockLTL() {return mLTLMutex.lock( NULL /* idvSQL* */ ); }
    static inline IDE_RC unlockLTL() {return mLTLMutex.unlock(); }

    static inline IDE_RC lockLSL() {return mLSLMutex.lock( NULL /* idvSQL* */ ); }
    static inline IDE_RC unlockLSL() {return mLSLMutex.unlock(); }

    /* BUG-42760 */
    static inline IDE_RC lockWaitForNoAccessAftDropTbl( smxLegacyTrans *aLegacyTX )
    {
        return aLegacyTX->mWaitForNoAccessAftDropTbl.lock( NULL );
    }

    static inline IDE_RC unlockWaitForNoAccessAftDropTbl( smxLegacyTrans *aLegacyTX )
    {
        return aLegacyTX->mWaitForNoAccessAftDropTbl.unlock();
    }

    static void waitForNoAccessAftDropTbl( void );
    /* BUG-42760 end */

private:
    static iduMutex       mLTLMutex;
    static iduMutex       mLSLMutex;
public:
    static smuList        mHeadLegacyTrans;
    static smuList        mHeadLegacyStmt;
    static UInt           mLegacyTransCnt;
    static UInt           mLegacyStmtCnt;
};


#endif /* _O_SMX_LEGACY_TRANS_MGR_H_ */

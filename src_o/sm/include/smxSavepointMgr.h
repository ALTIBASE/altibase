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
 * $Id: smxSavepointMgr.h 90824 2021-05-13 05:35:21Z minku.kang $
 **********************************************************************/

#ifndef _O_SMX_SAVEPOINT_MGR_H_
#define _O_SMX_SAVEPOINT_MGR_H_ 1

#include <smu.h>

# define SM_DDL_BEGIN_SAVEPOINT  "$$DDL_BEGIN_SAVEPOINT$$"
# define SM_DDL_INFO_SAVEPOINT   "$$DDL_INFO_SAVEPOINT$$"

class smxTrans;
class smxSavepointMgr
{
//For Operation
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    void   initialize(smxTrans *aTrans);
    IDE_RC destroy( void * aSmiTrans );

    /* For Explicit Savepoint */
    IDE_RC setExpSavepoint( smxTrans    * aTrans,
                            const SChar * aSavepointName,
                            smOID         aOldTableOID,
                            UInt          aOldPartOIDCount,
                            smOID       * aOldPartOIDArray,
                            smOID         aNewTableOID,
                            UInt          aNewPartOIDCount,
                            smOID       * aNewPartOIDArray,
                            smxOIDNode  * aOIDNode,
                            smLSN       * aLSN,
                            svrLSN        aVolLSN,
                            ULong         aLockSequence );

    IDE_RC abortToExpSavepoint(smxTrans *aTrans,
                               const SChar *aSavePointName);

    /* For Implicit Savepoint */
    IDE_RC setImpSavepoint( smxSavepoint ** aSavepoint,
                            UInt            aStmtDepth,
                            smxOIDNode    * aOIDNode,
                            smLSN         * aLSN,
                            svrLSN          aVolLSN,
                            ULong           aLockSequence );

    UInt getStmtDepthFromImpSvpList();

    IDE_RC abortToImpSavepoint(smxTrans*     aTrans,
                               smxSavepoint* aSavepoint);

    IDE_RC unsetImpSavepoint( smxSavepoint* aSavepoint );

    /* For Replication Implicit SVP */
    idBool checkAndSetImpSVP4Repl();

    // For BUG-12512
    idBool isPsmSvpReserved() { return mReservePsmSvp; };

    // TASK-7244 PSM partial rollback in Sharding
    idBool isShardPsmSvpReserved() { return mReserveShardPsmSvp; };

    void clearPsmSvp() { mReservePsmSvp = ID_FALSE; };

    void reservePsmSvp( smxOIDNode  *aOIDNode,
                        smLSN       *aLSN,
                        svrLSN       aVolLSN,
                        ULong        aLockSequence,
                        idBool       aIsShard );

    IDE_RC writePsmSvp(smxTrans* aTrans);
    IDE_RC abortToPsmSvp(smxTrans* aTrans);

    IDE_RC unlockSeveralLock(smxTrans *aTrans,
                            ULong     aLockSequence);

    void   rollbackDDLTargetTableInfo( const SChar * aSavepointName );

    inline UInt getLstReplStmtDepth();

    idBool isExistExpSavepoint(const SChar *aSavepointName);  /* BUG-48489 */

    smxSavepointMgr(){};
    ~smxSavepointMgr(){};

private:
    smxSavepoint* findInExpSvpList(const SChar *aSavepointName,
                                   idBool       aDoRemove);

    smxSavepoint* findInImpSvpList(UInt aStmtID);

    inline IDE_RC alloc(smxSavepoint **aNewSavepoint);
    inline IDE_RC freeMem(smxSavepoint *aSavepoint);

    inline void addToSvpListTail(smxSavepoint *aList,
                                 smxSavepoint* aSavepoint);
    inline void removeFromLst(smxSavepoint* aSavepoint);

    void updateOIDList(smxTrans *aTrans, smxSavepoint *aSavepoint);
    void setReplSavepoint( smxSavepoint *aSavepoint );

//For Member
private:
    static iduMemPool      mSvpPool;
    smxSavepoint           mImpSavepoint;
    smxSavepoint           mExpSavepoint;

    /* (BUG-45368) TPC-C 성능테스트. savepoint 한개일때는 alloc 하지 않고 이변수를 쓴다. */
    smxSavepoint           mPreparedSP;
    idBool                 mIsUsedPreparedSP;

    // For BUG-12512
    idBool                 mReservePsmSvp;
    smxSavepoint           mPsmSavepoint;

    // TASK-7244 PSM partial rollback in Sharding
    idBool                 mReserveShardPsmSvp;

    UInt                   mDDLInfoCount;
};

inline void smxSavepointMgr::addToSvpListTail(smxSavepoint *aList,
                                              smxSavepoint *aSavepoint)
{
    aSavepoint->mPrvSavepoint = aList->mPrvSavepoint;
    aSavepoint->mNxtSavepoint = aList;

    aList->mPrvSavepoint->mNxtSavepoint = aSavepoint;
    aList->mPrvSavepoint = aSavepoint;
}

inline IDE_RC smxSavepointMgr::alloc(smxSavepoint **aNewSavepoint)
{
    if ( mIsUsedPreparedSP == ID_FALSE )
    {
        *aNewSavepoint  = &mPreparedSP;
        mIsUsedPreparedSP = ID_TRUE;
        return IDE_SUCCESS;
    }
    else
    {
        return mSvpPool.alloc((void**)aNewSavepoint); 
    }
}

inline void smxSavepointMgr::removeFromLst(smxSavepoint* aSavepoint)
{
    aSavepoint->mPrvSavepoint->mNxtSavepoint = aSavepoint->mNxtSavepoint;
    aSavepoint->mNxtSavepoint->mPrvSavepoint = aSavepoint->mPrvSavepoint;
}

inline IDE_RC smxSavepointMgr::freeMem(smxSavepoint* aSavepoint)
{
    removeFromLst(aSavepoint);

    if ( aSavepoint == &mPreparedSP )
    {
        mIsUsedPreparedSP = ID_FALSE;
        return IDE_SUCCESS;
    }
    else
    {
        return mSvpPool.memfree((void*)aSavepoint);
    }
}

inline UInt smxSavepointMgr::getLstReplStmtDepth()
{
    smxSavepoint *sCurSavepoint;

    sCurSavepoint = mImpSavepoint.mPrvSavepoint;

    return sCurSavepoint->mReplImpSvpStmtDepth;

}

#endif

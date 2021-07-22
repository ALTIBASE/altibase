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
 * $Id: smxOIDList.h 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#ifndef _O_SMX_OIDLIST_H_
#define _O_SMX_OIDLIST_H_

#include <smDef.h>
#include <idu.h>
#include <iduOIDMemory.h>
#include <smxDef.h>

class smxTrans;

typedef enum smxProcessOIDListOpt
{
    SMX_OIDLIST_INIT = 1,
    SMX_OIDLIST_DEST = 0
} smxProcessOIDListOpt ;

# define SMX_UNIQUEOID_HASH_BUCKETCOUNT (16)

class smxOIDList
{
//For Operation
public:
    typedef IDE_RC (smxOIDList::*addOIDFunc)(
                       smOID               aTableOID,
                       smOID               aTargetOID,
                       scSpaceID           aSpaceID,
                       UInt                aFlag,
                       smSCN               aSCN );

    IDE_RC add( smOID           aTableOID,
                smOID           aTargetOID,
                scSpaceID       aSpaceID,
                UInt            aFlag,
                smSCN           aSCN = SM_SCN_MAX );

    IDE_RC processOIDList(SInt                 aAgingState,
                          smLSN*               aLSN,
                          smSCN                aSCN,
                          smxProcessOIDListOpt aProcessOIDOpt,
                          ULong               *aAgingCnt,
                          idBool               aIsLegacyTrans);

    IDE_RC processOIDList4LegacyTx( smLSN   * aLSN, 
                                    smSCN     aSCN ); /* BUG-42760 */

    inline void initOIDNode(smxOIDNode *aOIDNode);
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();
    IDE_RC cloneInsertCachedOID();

    IDE_RC allocAndLinkOIDNode();

    /* BUG-47367 SCN Check는 OIDFreeList에서만 필요하다. */
    IDE_RC addOID( smOID           aTableOID,
                   smOID           aTargetOID,
                   scSpaceID       aSpaceID,
                   UInt            aFlag,
                   smSCN           aSCN = SM_SCN_MAX );

    IDE_RC addOIDWithCheckFlag(smOID      aTableOID,
                               smOID      aTargetOID,
                               scSpaceID  aSpaceID,
                               UInt       aFlag,
                               smSCN      aSCN = SM_SCN_MAX );

    IDE_RC addOIDToVerify( smOID           aTableOID,
                           smOID           aTargetOID,
                           scSpaceID       aSpaceID,
                           UInt            aFlag,
                           smSCN           aSCN = SM_SCN_MAX );


    static inline IDE_RC alloc( smxOIDNode **aNewNode );
    static inline IDE_RC freeMem( smxOIDNode *aNode );

    idBool isEmpty()
    {
        if( mOIDNodeListHead.mNxtNode == &mOIDNodeListHead )
        {
            IDE_DASSERT( mOIDNodeListHead.mOIDCnt == 0 );
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }

    // To fix BUG-14126
    idBool needAging(){ return mNeedAging; }

    void   setAgingFlag( idBool aNeedAging ) { mNeedAging = aNeedAging; };
    // To fix BUG-14126

    IDE_RC initialize(smxTrans              * aTrans,
                      smxOIDNode            * aCacheOIDNode4Insert,
                      idBool                  aIsUnique,
                      smxOIDList::addOIDFunc  aAddOIDFunc);
    void   init();

    IDE_RC freeOIDList();

    IDE_RC destroy();

    void dump();

    IDE_RC setSlotNextToSCN(smxOIDInfo* aOIDInfo,
                            smSCN       aSCN);

    IDE_RC processDeleteOIDList( UInt   aAgingState,
                                 smSCN  aSCN,
                                 ULong *aAgingCnt );

    IDE_RC processOIDListToVerify( idvSQL * aStatistics );

    IDE_RC setSCNForInDoubt(smTID aTID);

    /* BUG-42724 */
    IDE_RC setOIDFlagForInDoubt();

    static inline idBool checkIsAgingTarget( UInt         aAgingState,
                                             smxOIDInfo  *aOIDInfo );


private:
    IDE_RC processOIDListAtCommit(smxOIDInfo* aOIDInfo,
                                  smSCN       aSCN);

    IDE_RC processDropTblPending(smxOIDInfo* aOIDInfo);

    IDE_RC processAllIndexDisablePending( smxOIDInfo  * aOIDInfo );

    IDE_RC processDeleteBackupFile(smLSN*      aLSN,
                                   smxOIDInfo *aOIDInfo);

public:

    idBool                   mIsUnique;
    UInt                     mItemCnt;
    smxOIDNode               mOIDNodeListHead;
    smxOIDNode              *mCacheOIDNode4Insert;

private:
    /* mMemPoolType == 0 이면 mOIDListSize를 사용.
     * 1이면 mMemPool을 사용.
     * XXX: 테스트용으로 추가 */
    static UInt              mMemPoolType;
    static UInt              mOIDListSize;
    static iduOIDMemory      mOIDMemory;
    static iduMemPool        mMemPool;

public:
    smxTrans                *mTrans;
    smxOIDList::addOIDFunc   mAddOIDFunc;
    // To fix BUG-14126
    idBool                   mNeedAging;

    smuHashBase             *mUniqueOIDHash; // OID 중복허용하지 않기 위한 Hash.
};

void smxOIDList::initOIDNode(smxOIDNode *aOIDNode)
{
    aOIDNode->mPrvNode = NULL;
    aOIDNode->mNxtNode = NULL;
    aOIDNode->mOIDCnt  = 0;
    aOIDNode->mAlign   = 0;
}

/**********************************************************************
 * Description: aOIDInfo가 Aging대상이면 ID_TRUE, 아니면 ID_FALSE
 *
 * aAgingState - [IN] Commit시에는 SM_OID_ACT_AGING_COMMIT
 *                    Rollback시에는 SM_OID_ACT_AGING_ROLLBACk
 * aOIDInfo    - [IN] OID Info
 *
 * Releated Issue - 1. BUG-17417 V$Ager정보의 Add OID갯수는 실제 Ager가
 *                     해야할 작업의 갯수가 아니다.
 **********************************************************************/
idBool smxOIDList::checkIsAgingTarget( UInt         aAgingState,
                                       smxOIDInfo  *aOIDInfo )
{
    if( (aOIDInfo->mFlag & aAgingState) == aAgingState )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

inline IDE_RC smxOIDList::alloc( smxOIDNode **aNewNode )
{
    if ( mMemPoolType == 0 )
    {
        IDE_TEST( mOIDMemory.alloc( (void**)aNewNode ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mMemPool.alloc( (void**)aNewNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

inline IDE_RC smxOIDList::freeMem( smxOIDNode *aNode )
{
    if ( mMemPoolType == 0 )
    {
        IDE_TEST( mOIDMemory.memfree( aNode ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mMemPool.memfree( aNode ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


#endif

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
 * $Id: smlFT.cpp 36756 2009-11-16 11:58:29Z et16 $
 **********************************************************************/

/* ------------------------------------------------
 *  X$LOCK Fixed Table
 * ----------------------------------------------*/

#include <smErrorCode.h>
#include <smDef.h>
#include <smr.h>
#include <smc.h>
#include <smp.h>
#include <sml.h>
#include <smlReq.h>
#include <smlFT.h>

class smlLockInfo
{
public:
    smiLockItemType  mLockItemType;
    scSpaceID        mSpaceID;      // 잠금을 획득한 테이블스페이스 ID
    ULong            mItemID;       // 테이블타입이라면 Table OID
                                    // 디스크 데이타파일인 경우 File ID
    smTID            mTransID;
    SInt             mSlotID;      // lock을 요청한 transaction의 slot id
    UInt             mLockCnt;
    idBool           mBeGrant;   // grant되었는지를 나타내는 flag, BeGranted
    SInt             mLockMode;
    idBool           mUseLockItem;
    
    void getInfo(const smlLockNode* aNode)
    {
        mLockItemType   = aNode->mLockItemType;
        mSpaceID        = aNode->mSpaceID;
        mItemID         = aNode->mItemID;
        mTransID        = aNode->mTransID;
        mSlotID         = aNode->mSlotID;
        mLockCnt        = aNode->mLockCnt;
        mBeGrant        = aNode->mBeGrant;
        mLockMode       = aNode->mLockMode;
    }
};

/* --------------------------------------------------------------------
 * BUG-47388
 * iddRPTree에서 사용할 compare 함수,
 * void pointer 대신 LockItemID를 사용한다.
 * ------------------------------------------------------------------*/
SInt smlFT::compLockItemID( const void* aLeft, const void* aRight )
{
    ULong sLeft  = *(ULong*)aLeft;
    ULong sRight = *(ULong*)aRight;

    if ( sLeft == sRight ) return 0;
    if ( sLeft >  sRight ) return 1;
    return -1;
}

IDE_RC smlFT::getLockItemNodes( void                *aHeader,
                                iduFixedTableMemory *aMemory,
                                smlLockItem         *aLockItem )
{
    SInt                i;
    smlLockNode*        sCurLockNode;
    smlLockInfo         sLockInfo;

    sLockInfo.mUseLockItem = ID_TRUE;

    // Grant Lock Node list
    sCurLockNode = aLockItem->mFstLockGrant;

    for(i = 0; i < aLockItem->mGrantCnt; i++)
    {
        sLockInfo.getInfo(sCurLockNode);
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) &sLockInfo)
             != IDE_SUCCESS);
        sCurLockNode = sCurLockNode->mNxtLockNode;
    }

    sCurLockNode = aLockItem->mFstLockRequest;

    // request lock node list
    for(i = 0; i < aLockItem->mRequestCnt; i++)
    {
        sLockInfo.getInfo(sCurLockNode);
        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                            aMemory,
                                            (void *) &sLockInfo)
                 != IDE_SUCCESS);
        // alloc new lock node for performance view.
        sCurLockNode = sCurLockNode->mNxtLockNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* --------------------------------------------------------------------
 * BUG-47388 
 * Transaction Lock Node Array를 순회 하며
 * light mode로 동작중인 lock node를 출력한다.
 * X$LOCK, X$LOCK_TABLESPACE 에서 사용
 *   
 * aLockTableType [IN] 출력을 원하는 Lock Item Type
 * aLockItemTree  [IN] 이미 출력한 LockItem의 List, 즉 제외대상
 * ------------------------------------------------------------------*/
IDE_RC smlFT::getLockNodesFromTrans( idvSQL              *aStatistics,
                                     void                *aHeader,
                                     iduFixedTableMemory *aMemory,
                                     smiLockItemType      aLockTableType,
                                     iddRBTree           *aLockItemTree )
{
    SInt               i;
    SInt               sCount;
    SInt               sState = 0;
    smlLockNode      * sCurLockNode;
    smlLockInfo        sLockInfo;
    smlTransLockList * sCurSlot;
    smlLockNode      * sLockNodeHeader ;

    sCount = smlLockMgr::getSlotCount();

    sLockInfo.mUseLockItem = ID_FALSE;
    
    for(i = 0 ; i < sCount ; i++ )
    {
        sCurSlot = smlLockMgr::getTransLockList( i );

        smlLockMgr::lockTransNodeList( aStatistics, i );
        sState = 1;

        if ( (void*)sCurSlot->mLockNodeHeader.mPrvTransLockNode != (void*)sCurSlot )
        {
            sLockNodeHeader = &(sCurSlot->mLockNodeHeader);
            sCurLockNode    = sLockNodeHeader->mPrvTransLockNode;

            while ( sCurLockNode != sLockNodeHeader )
            {
                if(( sCurLockNode->mBeGrant      == ID_TRUE ) &&
                   ( sCurLockNode->mLockItemType == aLockTableType ))
                {
                    // 이미 출력한 LockItem은 제외
                    if ( aLockItemTree->search( &sCurLockNode->mItemID, NULL ) == ID_FALSE )
                    {
                        sLockInfo.getInfo(sCurLockNode);
                        IDE_TEST(iduFixedTable::buildRecord(aHeader,
                                                            aMemory,
                                                            (void *) &sLockInfo)
                                 != IDE_SUCCESS);
                    }
                }
                sCurLockNode = sCurLockNode->mPrvTransLockNode;
            }
        }
        sState = 0;
        smlLockMgr::unlockTransNodeList( i );
        
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        smlLockMgr::unlockTransNodeList( i );
    }

    return IDE_FAILURE;
}


IDE_RC smlFT::buildRecordForLockTBL(idvSQL              *aStatistics,
                                    void                *aHeader,
                                    void                * /* aDumpObj */,
                                    iduFixedTableMemory *aMemory)

{
    smcTableHeader *sCatTblHdr;
    smcTableHeader *sTableHeader;
    smpSlotHeader  *sPtr;
    smlLockItem    *sLockItem;
    SChar          *sCurPtr;
    SChar          *sNxtPtr;
    UInt            sState = 0;
    iddRBTree       sLockItemTree;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr = NULL;

    sLockItemTree.initialize( IDU_MEM_SM_SML,
                              ID_SIZEOF(ULong),
                              smlFT::compLockItemID );
    sState = 1;

    // [1] 테이블 헤더를 순회하면서 현재 lock node list를 구한다.
    while(1)
    {
        IDE_TEST( smcRecord::nextOIDall( sCatTblHdr,
                                         sCurPtr,
                                         &sNxtPtr )
                  != IDE_SUCCESS );

        if ( sNxtPtr == NULL )
        {
            break;
        }

        sPtr = (smpSlotHeader *)sNxtPtr;

        // To fix BUG-14681
        if ( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }

        sTableHeader = (smcTableHeader *)( sPtr + 1 );

        // 1. temp table은 skip ( PROJ-2201 TempTable은 이제 여기 없음 */
        // 2. drop된 table은 skip
        // 3. meta  table은 skip

        if( ( SMI_TABLE_TYPE_IS_META( sTableHeader ) == ID_TRUE ) ||
            ( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) )
        {
            sCurPtr = sNxtPtr;
            continue;
        }
        sLockItem = (smlLockItem*)sTableHeader->mLock ;

        IDE_TEST( sLockItem->mMutex.lock( aStatistics ) != IDE_SUCCESS );
        sState = 2;

        if( sLockItem->mFlag != SML_FLAG_LIGHT_MODE )
        {
            IDE_TEST( getLockItemNodes( aHeader,
                                        aMemory,
                                        sLockItem )
                      != IDE_SUCCESS );
            sLockItemTree.insert( &sLockItem->mItemID, NULL );
        }

        sState = 1;
        IDE_TEST( sLockItem->mMutex.unlock() != IDE_SUCCESS );

        sCurPtr = sNxtPtr;
    }
    IDE_TEST( getLockNodesFromTrans( aStatistics,
                                     aHeader,
                                     aMemory,
                                     SMI_LOCK_ITEM_TABLE,
                                     &sLockItemTree ) != IDE_SUCCESS );
    sState = 0;
    sLockItemTree.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sLockItem->mMutex.unlock() == IDE_SUCCESS );
        case 1:
            sLockItemTree.destroy();
            break;
        default:
            break;
    }
    return IDE_FAILURE;
}

static iduFixedTableColDesc gLockTBLColDesc[]=
{
    {
        (SChar*)"LOCK_ITEM_TYPE",
        offsetof(smlLockInfo,mLockItemType),
        IDU_FT_SIZEOF(smlLockInfo,mLockItemType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"TBS_ID",
        offsetof(smlLockInfo,mSpaceID),
        IDU_FT_SIZEOF(smlLockInfo,mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"TABLE_OID",
        offsetof(smlLockInfo,mItemID),
        IDU_FT_SIZEOF(smlLockInfo,mItemID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"TRANS_ID",
        offsetof(smlLockInfo,mTransID),
        IDU_FT_SIZEOF(smlLockInfo,mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"LOCK_MODE",
        offsetof(smlLockInfo,mLockMode),
        IDU_FT_SIZEOF(smlLockInfo,mLockMode),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },


    {
        (SChar*)"LOCK_CNT",
        offsetof(smlLockInfo,mLockCnt),
        IDU_FT_SIZEOF(smlLockInfo,mLockCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"IS_GRANT",
        offsetof(smlLockInfo,mBeGrant),
        IDU_FT_SIZEOF(smlLockInfo,mBeGrant),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"USE_LOCK_ITEM",
        offsetof(smlLockInfo,mUseLockItem),
        IDU_FT_SIZEOF(smlLockInfo,mUseLockItem),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gLockTBLDesc =
{
    (SChar *)"X$LOCK",
    smlFT::buildRecordForLockTBL,
    gLockTBLColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *   X$LOCK_MODE Fixed Table
 * ----------------------------------------------*/

IDE_RC smlFT::buildRecordForLockMode(idvSQL              * /*aStatistics*/,
                                     void        *aHeader,
                                     void        * /* aDumpObj */,
                                     iduFixedTableMemory *aMemory)
{
    UInt i;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

// [2] build record.
    for(i=0; i < SML_NUMLOCKTYPES; i++ )
    {
        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                              aMemory,
                                              (void *)& smlLockMgr::mLockMode2StrTBL[i])
             != IDE_SUCCESS);
    }//for


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static iduFixedTableColDesc   gLockTableModeColDesc[]=
{

    {
        (SChar*)"LOCK_MODE",
        offsetof(smlLockMode2StrTBL,mLockMode),
        IDU_FT_SIZEOF(smlLockMode2StrTBL,mLockMode),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"LOCK_DESC",
        offsetof(smlLockMode2StrTBL,mLockModeStr),
        SML_LOCKMODE_STRLEN,
        IDU_FT_TYPE_VARCHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

// X$LOCK
iduFixedTableDesc  gLockModeTableDesc =
{
    (SChar *)"X$LOCK_MODE",
    smlFT::buildRecordForLockMode,
    gLockTableModeColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};



/* ------------------------------------------------
 *   X$LOCK_WAIT  Fixed Table
 *   ex)
 *     TxA(1) is waiting TxB(2)
 *     TxB(2) is waiting TxC(3)
 *     TxC(3) is waiting TxD(4)
 *   select * from x$lock_wait
 *    =>  TID         WAITFORTID
 *         1              2
 *         2              3
 *         3              4
 * ----------------------------------------------*/

typedef struct smlLockWaitStat
{
    smTID  mTID;         // Current Waiting Tx

    smTID  mWaitForTID;  // Wait For Target Tx

} smlLockWaitStat;


IDE_RC smlFT::buildRecordForLockWait(idvSQL              * /*aStatistics*/,
                                     void        *aHeader,
                                     void        * /* aDumpObj */,
                                     iduFixedTableMemory *aMemory)
{
    SInt            i;
    SInt            j;
    SInt            k; /* for preventing from infinite loop */
    smlLockWaitStat sStat;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    for ( j = 0; j < smLayerCallback::getCurTransCnt(); j++ )
    {
        if ( smLayerCallback::isActiveBySID(j) == ID_TRUE )
        {
            for ( k = 0, i = smlLockMgr::mArrOfLockList[j].mFstWaitTblTransItem;
                  (i != SML_END_ITEM) && 
                      (i != ID_USHORT_MAX) &&
                      (k < smLayerCallback::getCurTransCnt()) ;
                  i = smlLockMgr::mWaitForTable[j][i].mNxtWaitTransItem, k++)
            {
                if (smlLockMgr::mWaitForTable[j][i].mIndex == 1)
                {
                    sStat.mTID        = smLayerCallback::getTIDBySID( j );
                    sStat.mWaitForTID = smLayerCallback::getTIDBySID( i );

                    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                                         aMemory,
                                                         (void *)& sStat)
                             != IDE_SUCCESS);
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc   gLockWaitColDesc[]=
{

    {
        (SChar*)"TRANS_ID",
        offsetof(smlLockWaitStat,mTID),
        IDU_FT_SIZEOF(smlLockWaitStat,mTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"WAIT_FOR_TRANS_ID",
        offsetof(smlLockWaitStat,mWaitForTID),
        IDU_FT_SIZEOF(smlLockWaitStat,mWaitForTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

// X$LOCK
iduFixedTableDesc  gLockWaitTableDesc =
{
    (SChar *)"X$LOCK_WAIT",
    smlFT::buildRecordForLockWait,
    gLockWaitColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  X$LOCK_TABLESPACE Fixed Table
 * ----------------------------------------------*/

IDE_RC smlFT::buildRecordForLockTBS(idvSQL              *aStatistics,
                                    void                *aHeader,
                                    void                * /* aDumpObj */,
                                    iduFixedTableMemory *aMemory)

{
    UInt                sState = 0;
    sctTableSpaceNode * sCurrSpaceNode;
    smlLockItem       * sLockItem;
    iddRBTree           sLockItemTree;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    sLockItemTree.initialize( IDU_MEM_SM_SML,
                              ID_SIZEOF(ULong),
                              smlFT::compLockItemID );
    sState = 1;

    sCurrSpaceNode = sctTableSpaceMgr::getFirstSpaceNode();

    while( sCurrSpaceNode != NULL )
    {
        sctTableSpaceMgr::lockSpaceNode( aStatistics,
                                         sCurrSpaceNode );
        sState = 2;

        if ( (sCurrSpaceNode->mState & SMI_TBS_DROPPED)
             != SMI_TBS_DROPPED )
        {
            sLockItem = (smlLockItem*)sCurrSpaceNode->mLockItem4TBS;
            IDE_TEST( sLockItem->mMutex.lock( aStatistics ) != IDE_SUCCESS );
            sState = 3;

            if( sLockItem->mFlag != SML_FLAG_LIGHT_MODE )
            {
                IDE_TEST( getLockItemNodes( aHeader,
                                            aMemory,
                                            sLockItem )
                          != IDE_SUCCESS );
                sLockItemTree.insert( &sLockItem->mItemID, NULL );
            }
            
            sState = 2;
            IDE_TEST( sLockItem->mMutex.unlock() != IDE_SUCCESS );
        }
        sState = 1;
        sctTableSpaceMgr::unlockSpaceNode( sCurrSpaceNode );

        sCurrSpaceNode = sctTableSpaceMgr::getNextSpaceNode( sCurrSpaceNode->mID );
    }

    IDE_TEST( getLockNodesFromTrans( aStatistics,
                                     aHeader,
                                     aMemory,
                                     SMI_LOCK_ITEM_TABLESPACE,
                                     &sLockItemTree ) != IDE_SUCCESS );
    sState = 0;
    sLockItemTree.destroy();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            IDE_ASSERT( sLockItem->mMutex.unlock() == IDE_SUCCESS );
        case 2:
            sctTableSpaceMgr::unlockSpaceNode( sCurrSpaceNode );
        case 1:
            sLockItemTree.destroy();
            break;
        default:
            break;
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc gLockTBSColDesc[]=
{
    {
        (SChar*)"LOCK_ITEM_TYPE",
        offsetof(smlLockInfo,mLockItemType),
        IDU_FT_SIZEOF(smlLockInfo,mLockItemType),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"TBS_ID",
        offsetof(smlLockInfo,mSpaceID),
        IDU_FT_SIZEOF(smlLockInfo,mSpaceID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"DBF_ID",
        offsetof(smlLockInfo,mItemID),
        IDU_FT_SIZEOF(smlLockInfo,mItemID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"TRANS_ID",
        offsetof(smlLockInfo,mTransID),
        IDU_FT_SIZEOF(smlLockInfo,mTransID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"LOCK_MODE",
        offsetof(smlLockInfo,mLockMode),
        IDU_FT_SIZEOF(smlLockInfo,mLockMode),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },


    {
        (SChar*)"LOCK_CNT",
        offsetof(smlLockInfo,mLockCnt),
        IDU_FT_SIZEOF(smlLockInfo,mLockCnt),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"IS_GRANT",
        offsetof(smlLockInfo,mBeGrant),
        IDU_FT_SIZEOF(smlLockInfo,mBeGrant),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USE_LOCK_ITEM",
        offsetof(smlLockInfo,mUseLockItem),
        IDU_FT_SIZEOF(smlLockInfo,mUseLockItem),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};


iduFixedTableDesc  gLockTBSDesc =
{
    (SChar *)"X$LOCK_TABLESPACE",
    smlFT::buildRecordForLockTBS,
    gLockTBSColDesc,
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  X$LOCK_RECORD Fixed Table
 * ----------------------------------------------*/

IDE_RC smlFT::buildRecordForLockRecord( idvSQL              * /*aStatistics*/,
                                        void                * aHeader,
                                        void                * /* aDumpObj */,
                                        iduFixedTableMemory * aMemory )
{
    SInt            i;
    SInt            j;
    SInt            k; /* for preventing from infinite loop */
    smlLockWaitStat sStat;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    for ( j = 0; j < smLayerCallback::getCurTransCnt(); j++ )
    {
        if ( smLayerCallback::isActiveBySID(j) == ID_TRUE )
        {
            for ( k = 0, i = smlLockMgr::mArrOfLockList[j].mFstWaitRecTransItem;
                  (i != SML_END_ITEM) && 
                      (i != ID_USHORT_MAX) &&
                      (k < smLayerCallback::getCurTransCnt()) ;
                  i = smlLockMgr::mWaitForTable[i][j].mNxtWaitRecTransItem, k++)
            {
                if (smlLockMgr::mWaitForTable[i][j].mIndex == 1)
                {
                    sStat.mTID        = smLayerCallback::getTIDBySID( i );
                    sStat.mWaitForTID = smLayerCallback::getTIDBySID( j );

                    IDE_TEST(iduFixedTable::buildRecord( aHeader,
                                                         aMemory,
                                                         (void *)& sStat )
                             != IDE_SUCCESS);
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc gLockRecordColDesc[]=
{
    {
        (SChar*)"TRANS_ID",
        offsetof(smlLockWaitStat,mTID),
        IDU_FT_SIZEOF(smlLockWaitStat,mTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"WAIT_FOR_TRANS_ID",
        offsetof(smlLockWaitStat,mWaitForTID),
        IDU_FT_SIZEOF(smlLockWaitStat,mWaitForTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gLockRecordDesc =
{
    (SChar *)"X$LOCK_RECORD",
    smlFT::buildRecordForLockRecord,
    gLockRecordColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

/* ------------------------------------------------
 *  X$DIST_LOCK_WAIT Fixed Table
 * ----------------------------------------------*/

IDE_RC smlFT::buildRecordForDistLockWait( idvSQL              * /*aStatistics*/,
                                          void                * aHeader,
                                          void                * /* aDumpObj */,
                                          iduFixedTableMemory * aMemory )
{
    SInt            i;
    SInt            j;
    smlLockWaitStat sStat;

    smxTrans * sTrans = NULL;

    IDE_ERROR( aHeader != NULL );
    IDE_ERROR( aMemory != NULL );

    for ( i = 0; i < smLayerCallback::getCurTransCnt(); i++ )
    {
        if ( smLayerCallback::isActiveBySID(i) == ID_TRUE )
        {
            sTrans = (smxTrans *)(smLayerCallback::getTransBySID( i ));

            if ( sTrans->mDistDeadlock4FT.mDetection != SMX_DIST_DEADLOCK_DETECTION_NONE )
            {
                for ( j = 0; j < smLayerCallback::getCurTransCnt(); j++ )
                {
                    if ( smlLockMgr::mTxList4DistDeadlock[i][j].mDistTxType == SML_DIST_DEADLOCK_TX_FIRST_DIST_INFO )
                    {
                        sStat.mTID        = smLayerCallback::getTIDBySID( i );
                        sStat.mWaitForTID = smLayerCallback::getTIDBySID( j );

                        IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                                              aMemory,
                                                              (void *)& sStat )
                                != IDE_SUCCESS );
                    }
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc gDistLockWaitColDesc[]=
{
    {
        (SChar*)"TRANS_ID",
        offsetof(smlLockWaitStat,mTID),
        IDU_FT_SIZEOF(smlLockWaitStat,mTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        (SChar*)"WAIT_FOR_TRANS_ID",
        offsetof(smlLockWaitStat,mWaitForTID),
        IDU_FT_SIZEOF(smlLockWaitStat,mWaitForTID),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },

    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gDistLockWaitDesc =
{
    (SChar *)"X$DIST_LOCK_WAIT",
    smlFT::buildRecordForDistLockWait,
    gDistLockWaitColDesc,
    IDU_STARTUP_CONTROL,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};


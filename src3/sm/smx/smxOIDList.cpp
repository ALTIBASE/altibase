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
 * $Id: smxOIDList.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <smErrorCode.h>
#include <sml.h>
#include <smr.h>
#include <smm.h>
#include <svm.h>
#include <smc.h>
#include <smn.h>
#include <smp.h>
#include <sdn.h>
#include <sct.h>
#include <svcRecord.h>
#include <smxReq.h>

#if defined(SMALL_FOOTPRINT)
#define SMX_OID_LIST_NODE_POOL_SIZE (8)
#else
#define SMX_OID_LIST_NODE_POOL_SIZE (1024)
#endif

iduOIDMemory  smxOIDList::mOIDMemory;
iduMemPool    smxOIDList::mMemPool;
UInt          smxOIDList::mOIDListSize;
UInt          smxOIDList::mMemPoolType;

const smxOidSavePointMaskType  smxOidSavePointMask[SM_OID_OP_COUNT] = {
    {/* SM_OID_OP_OLD_FIXED_SLOT                         */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_COMMIT       |
           SM_OID_ACT_AGING_COMMIT |
           SM_OID_ACT_AGING_INDEX  ),
        0x00000000
    },
    {/* SM_OID_OP_CHANGED_FIXED_SLOT                     */
        ~( SM_OID_ACT_SAVEPOINT ),
        0x00000000
    },
    {/* SM_OID_OP_NEW_FIXED_SLOT                         */
        ~( SM_OID_ACT_SAVEPOINT |
           SM_OID_ACT_COMMIT    ),
        SM_OID_ACT_AGING_COMMIT
    },
    {/* SM_OID_OP_DELETED_SLOT                           */
        ~( SM_OID_ACT_SAVEPOINT |
           SM_OID_ACT_COMMIT    |
           SM_OID_ACT_AGING_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_LOCK_FIXED_SLOT                        */
        ~( SM_OID_ACT_SAVEPOINT|
           SM_OID_ACT_COMMIT   ),
        0x00000000
    },
    {/* SM_OID_OP_UNLOCK_FIXED_SLOT                      */
        ~( SM_OID_ACT_SAVEPOINT|
           SM_OID_ACT_COMMIT   ),
        0x00000000
    },
    {/* SM_OID_OP_OLD_VARIABLE_SLOT                      */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_AGING_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_NEW_VARIABLE_SLOT                      */
        ~( SM_OID_ACT_SAVEPOINT ),
        SM_OID_ACT_AGING_COMMIT
    },
    {/* SM_OID_OP_DROP_TABLE                             */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_COMMIT       |
           SM_OID_ACT_AGING_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_DROP_TABLE_BY_ABORT                    */
        ~( SM_OID_ACT_SAVEPOINT |
           SM_OID_ACT_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_DELETE_TABLE_BACKUP                    */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_COMMIT       |
           SM_OID_ACT_AGING_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_DROP_INDEX                             */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_COMMIT       |
           SM_OID_ACT_AGING_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_UPATE_FIXED_SLOT                       */
        ~( SM_OID_ACT_SAVEPOINT),
        0x00000000
    },
    {/* SM_OID_OP_DDL_TABLE                              */
        ~( SM_OID_ACT_SAVEPOINT),
        0x00000000
    },
    /* BUG-27742 Partial Rollback시 LPCH가 두번 Free되는 문제가 있습니다.
     * Partial Rollback시 Tx의 Commit, Rollback과 무관하게
     * Old LPCH는 보존 되어야 하고, New LPCH는 제거되어야 합니다. */
    {/* SM_OID_OP_FREE_OLD_LPCH                          */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_AGING_COMMIT ),
        0x00000000
    },
    {/* SM_OID_OP_FREE_NEW_LPCH                          */
        ~( SM_OID_ACT_SAVEPOINT ),
        SM_OID_ACT_AGING_COMMIT
    },
    {/* SM_OID_OP_ALL_INDEX_DISABLE                      */
        ~( SM_OID_ACT_SAVEPOINT    |
           SM_OID_ACT_COMMIT ),
        0x00000000
    }
};

IDE_RC smxOIDList::initializeStatic()
{
    mMemPoolType = smuProperty::getTxOIDListMemPoolType();

    IDE_ASSERT( (mMemPoolType == 0) || (mMemPoolType == 1) );

    mOIDListSize = ID_SIZEOF(smxOIDInfo) *
                   (smuProperty::getOIDListCount() - 1) +
                   ID_SIZEOF(smxOIDNode);

    if ( mMemPoolType == 0 )
    {
        IDE_TEST( mOIDMemory.initialize( IDU_MEM_SM_TRANSACTION_OID_LIST,
                                         mOIDListSize,
                                         SMX_OID_LIST_NODE_POOL_SIZE )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mMemPool.initialize( IDU_MEM_SM_SMX,
                                       (SChar*)"SMX_OIDLIST_POOL",
                                       ID_SCALABILITY_SYS,
                                       mOIDListSize,
                                       SMX_OID_LIST_NODE_POOL_SIZE,
                                       IDU_AUTOFREE_CHUNK_LIMIT,		/* ChunkLimit */
                                       ID_TRUE,							/* UseMutex */
                                       IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                       ID_FALSE,						/* ForcePooling */
                                       ID_TRUE,							/* GarbageCollection */
                                       ID_TRUE,                         /* HWCacheLine */
                                       IDU_MEMPOOL_TYPE_LEGACY          /* mempool type*/ ) 
                  != IDE_SUCCESS );			
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smxOIDList::destroyStatic()
{
    if ( mMemPoolType == 0 )
    {
        IDE_TEST( mOIDMemory.destroy() != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( mMemPool.destroy() != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smxOIDList::initialize( smxTrans              * aTrans,
                               smxOIDNode            * aCacheOIDNode4Insert,
                               idBool                  aIsUnique,
                               smxOIDList::addOIDFunc  aAddOIDFunc )
{
    UInt  sState = 0;

    mIsUnique            = aIsUnique;
    mTrans               = aTrans;
    mCacheOIDNode4Insert = aCacheOIDNode4Insert;

    init();

    mAddOIDFunc  = aAddOIDFunc;

    mUniqueOIDHash = NULL;

    if ( mIsUnique == ID_TRUE )
    {
        /* TC/FIT/Limit/sm/smx/smxOIDList_initialize_malloc.sql */
        IDU_FIT_POINT_RAISE( "smxOIDList::initialize::malloc",
                              insufficient_memory );

        IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMX,
                                     ID_SIZEOF(smuHashBase),
                                     (void**)&mUniqueOIDHash ) != IDE_SUCCESS,
                        insufficient_memory );
        sState = 1;

        IDE_TEST( smuHash::initialize(
                      mUniqueOIDHash,
                      1,                                // ConcurrentLevel
                      SMX_UNIQUEOID_HASH_BUCKETCOUNT,
                      ID_SIZEOF(smOID),                 // KeyLength
                      ID_FALSE,                         // UseLatch
                      smxTrans::genHashValueFunc,
                      smxTrans::compareFunc )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_InsufficientMemory));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sState )
        {
            case 1:
                IDE_ASSERT( iduMemMgr::free(mUniqueOIDHash)
                            == IDE_SUCCESS );
                mUniqueOIDHash = NULL;
                break;

            default:
                break;
        }
    }
    IDE_POP();

    return IDE_FAILURE;
}

void smxOIDList::init()
{
    mItemCnt                  = 0;
    mOIDNodeListHead.mPrvNode = &mOIDNodeListHead;
    mOIDNodeListHead.mNxtNode = &mOIDNodeListHead;
    mOIDNodeListHead.mOIDCnt  = 0;
    mNeedAging                = ID_FALSE;

    if ( mCacheOIDNode4Insert != NULL )
    {
        initOIDNode(mCacheOIDNode4Insert);
    }
}


IDE_RC smxOIDList::freeOIDList()
{
    smxOIDNode *sCurOIDNode;
    smxOIDNode *sNxtOIDNode;

    sCurOIDNode = mOIDNodeListHead.mNxtNode;

    while(sCurOIDNode != &mOIDNodeListHead)
    {
        /* fix BUG-26934 : [codeSonar] Null Pointer Dereference */
        IDE_ASSERT( sCurOIDNode != NULL );

        sNxtOIDNode = sCurOIDNode->mNxtNode;
        if(sCurOIDNode !=  mCacheOIDNode4Insert)
        {
            IDE_TEST(freeMem(sCurOIDNode) != IDE_SUCCESS);
        }

        sCurOIDNode = sNxtOIDNode;
    }
    mOIDNodeListHead.mPrvNode = &mOIDNodeListHead;
    mOIDNodeListHead.mNxtNode = &mOIDNodeListHead;

    if ( mIsUnique == ID_TRUE )
    {
        IDE_TEST( smuHash::reset(mUniqueOIDHash) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smxOIDList::destroy()
{
    IDE_ASSERT( isEmpty() == ID_TRUE );

    if ( mUniqueOIDHash != NULL )
    {
        IDE_ASSERT( mIsUnique == ID_TRUE );

        IDE_TEST( smuHash::destroy(mUniqueOIDHash) != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free(mUniqueOIDHash)  != IDE_SUCCESS );
        mUniqueOIDHash = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



IDE_RC smxOIDList::allocAndLinkOIDNode()
{

    smxOIDNode *sNewOIDNode;

    /* smxOIDList_allocAndLinkOIDNode_alloc_NewOIDNode.tc */
    IDU_FIT_POINT("smxOIDList::allocAndLinkOIDNode::alloc::NewOIDNode");
    IDE_TEST( alloc( &sNewOIDNode ) != IDE_SUCCESS );
    initOIDNode(sNewOIDNode);
    sNewOIDNode->mPrvNode   = mOIDNodeListHead.mPrvNode;
    sNewOIDNode->mNxtNode   = &(mOIDNodeListHead);

    mOIDNodeListHead.mPrvNode->mNxtNode = sNewOIDNode;
    mOIDNodeListHead.mPrvNode = sNewOIDNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC smxOIDList::add(smOID           aTableOID,
                       smOID           aTargetOID,
                       scSpaceID       aSpaceID,
                       UInt            aFlag,
                       smSCN           aSCN )
{
    return (this->*mAddOIDFunc)(aTableOID,aTargetOID,aSpaceID,aFlag,aSCN);
}

IDE_RC smxOIDList::addOID(smOID           aTableOID,
                          smOID           aTargetOID,
                          scSpaceID       aSpaceID,
                          UInt            aFlag,
                          smSCN           aSCN )
{
    smxOIDNode *sCurOIDNode;
    UInt        sItemCnt;

    sCurOIDNode = mOIDNodeListHead.mPrvNode;

    if(sCurOIDNode == &(mOIDNodeListHead) ||
       sCurOIDNode->mOIDCnt >= smuProperty::getOIDListCount())
    {
        IDE_TEST(allocAndLinkOIDNode() != IDE_SUCCESS);
        sCurOIDNode = mOIDNodeListHead.mPrvNode;
    }

    sItemCnt = sCurOIDNode->mOIDCnt;

    sCurOIDNode->mArrOIDInfo[sItemCnt].mTableOID  = aTableOID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mTargetOID = aTargetOID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mSpaceID   = aSpaceID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mFlag      = aFlag;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mSCN       = aSCN;
    sCurOIDNode->mOIDCnt++;
    mItemCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxOIDList::addOIDWithCheckFlag(smOID           aTableOID,
                                       smOID           aTargetOID,
                                       scSpaceID       aSpaceID,
                                       UInt            aFlag,
                                       smSCN           aSCN )
{
    smxOIDNode *sCurOIDNode;
    UInt        sItemCnt;


    if( aFlag != SM_OID_NEW_INSERT_FIXED_SLOT )
    {
        /* Insert 외의 연산에 의한 OID가 왔으므로 Ager에 달 필요가 있다. */
        mNeedAging = ID_TRUE;
    }//if aFlag

    sCurOIDNode =mOIDNodeListHead.mPrvNode;

    if(sCurOIDNode == &(mOIDNodeListHead))
    {
        /* use cached insert oid node. */
        mCacheOIDNode4Insert->mPrvNode = &(mOIDNodeListHead);
        mCacheOIDNode4Insert->mNxtNode = &(mOIDNodeListHead);

        mOIDNodeListHead.mPrvNode = mCacheOIDNode4Insert;
        mOIDNodeListHead.mNxtNode = mCacheOIDNode4Insert;
        sCurOIDNode = mOIDNodeListHead.mPrvNode;

    }//if
    else
    {
        if( sCurOIDNode->mOIDCnt >= smuProperty::getOIDListCount())
        {
            IDE_TEST(allocAndLinkOIDNode() != IDE_SUCCESS);
            sCurOIDNode = mOIDNodeListHead.mPrvNode;
        }
    }

    sItemCnt = sCurOIDNode->mOIDCnt;

    sCurOIDNode->mArrOIDInfo[sItemCnt].mTableOID  = aTableOID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mTargetOID = aTargetOID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mSpaceID   = aSpaceID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mFlag      = aFlag;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mSCN       = aSCN;
    sCurOIDNode->mOIDCnt++;
    mItemCnt++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 중복된 OID를 관리하지 않고, Verify할 OID를 추가한다.
 *
 * BUG-27122 Restart Recovery 시 Undo Trans가 접근하는 인덱스에 대한
 * Integrity 체크기능 추가 (__SM_CHECK_DISK_INDEX_INTEGRITY=2)
 *
 * aTableOID  - [IN] 테이블 OID
 * aTargetOID - [IN] Integrity 체크할 인덱스 OID
 * aSpaceID   - [IN] 테이블스페이스 ID
 * aFlag      - [IN] OID 처리에 대한 Flag
 *
 **********************************************************************/
IDE_RC smxOIDList::addOIDToVerify( smOID           aTableOID,
                                   smOID           aTargetOID,
                                   scSpaceID       aSpaceID,
                                   UInt            aFlag,
                                   smSCN           aSCN )
{
    smxOIDInfo  * sCurOIDInfo;
    smxOIDNode  * sCurOIDNode;
    UInt          sItemCnt;

    IDE_ASSERT( aFlag == SM_OID_NULL );

    IDE_ASSERT( smuHash::findNode( mUniqueOIDHash,
                                   &aTargetOID,
                                   (void**)&sCurOIDInfo )
                == IDE_SUCCESS );

    IDE_TEST_CONT( sCurOIDInfo != NULL, already_exist );

    sCurOIDNode = mOIDNodeListHead.mPrvNode;

    if(sCurOIDNode == &(mOIDNodeListHead) ||
       sCurOIDNode->mOIDCnt >= smuProperty::getOIDListCount())
    {
        IDE_TEST(allocAndLinkOIDNode() != IDE_SUCCESS);
        sCurOIDNode = mOIDNodeListHead.mPrvNode;
    }

    sItemCnt = sCurOIDNode->mOIDCnt;

    sCurOIDNode->mArrOIDInfo[sItemCnt].mTableOID  = aTableOID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mTargetOID = aTargetOID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mSpaceID   = aSpaceID;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mFlag      = aFlag;
    sCurOIDNode->mArrOIDInfo[sItemCnt].mSCN       = aSCN;

    IDE_ASSERT( smuHash::insertNode( mUniqueOIDHash,
                                     &aTargetOID,
                                     &(sCurOIDNode->mArrOIDInfo[ sItemCnt ]))
                == IDE_SUCCESS );

    sCurOIDNode->mOIDCnt++;
    mItemCnt++;

    IDE_EXCEPTION_CONT( already_exist );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxOIDList::cloneInsertCachedOID()
{
    smxOIDNode *sNewOIDNode;
    UInt     i;

    /* smxOIDList_cloneInsertCachedOID_alloc_NewOIDNode.tc */
    IDU_FIT_POINT("smxOIDList::cloneInsertCachedOID::alloc::NewOIDNode");
    IDE_TEST(alloc(&sNewOIDNode) != IDE_SUCCESS);

    initOIDNode(sNewOIDNode);

    /* 1.move from cached insert oid node to new oid node. */
    for(i = 0; i <mCacheOIDNode4Insert->mOIDCnt; i++)
    {
        sNewOIDNode->mArrOIDInfo[i].mTableOID  =
            mCacheOIDNode4Insert->mArrOIDInfo[i].mTableOID;

        sNewOIDNode->mArrOIDInfo[i].mTargetOID =
            mCacheOIDNode4Insert->mArrOIDInfo[i].mTargetOID;

        sNewOIDNode->mArrOIDInfo[i].mSpaceID =
            mCacheOIDNode4Insert->mArrOIDInfo[i].mSpaceID;

        sNewOIDNode->mArrOIDInfo[i].mFlag =
            mCacheOIDNode4Insert->mArrOIDInfo[i].mFlag;

        sNewOIDNode->mOIDCnt++;
    }//for
    /* 2. link update. */

    sNewOIDNode->mPrvNode =  mCacheOIDNode4Insert->mPrvNode;
    sNewOIDNode->mNxtNode =  mCacheOIDNode4Insert->mNxtNode;

    mCacheOIDNode4Insert->mPrvNode->mNxtNode = sNewOIDNode;
    mCacheOIDNode4Insert->mNxtNode->mPrvNode = sNewOIDNode;
    /* 3. init cached insert oid node. */
    initOIDNode(mCacheOIDNode4Insert);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Transaction이 End하기전에 OID List에 대해서 SetSCN이나 Drop
 *               Table Pending연산을 수행한다.
 *
 * aAgingState     - [IN] if Commit, SM_OID_ACT_AGING_COMMIT, else
 *                        SM_OID_ACT_AGING_ROLLBACK.
 * aLSN            - [IN] Commit Log나 Abort Log의 LSN
 * aSCN            - [IN] Row에 Setting할 SCN
 * aProcessOIDOpt  - [IN] if Ager가 OID List를 처리한다면, SMX_OIDLIST_INIT
 *                        else SMX_OIDLIST_DEST
 * aAgingCnt       - [OUT] Aging될 OID 갯수
 **********************************************************************/
IDE_RC smxOIDList::processOIDList(SInt                   aAgingState,
                                  smLSN                * aLSN,
                                  smSCN                  aSCN,
                                  smxProcessOIDListOpt   aProcessOIDOpt,
                                  ULong                * aAgingCnt,
                                  idBool                 aIsLegacyTrans)
{
    smxOIDNode      *sCurOIDNode;
    UInt             i;
    UInt             sItemCnt;
    smxOIDInfo      *sCurOIDInfo;
    SInt             sFlag;
    ULong            sAgingCnt = 0;
    scSpaceID        sSpaceID;
    smcTableHeader  *sTableHeader;

    IDU_FIT_POINT( "1.BUG-23795@smxOIDList::processOIDList" );

    sCurOIDNode = mOIDNodeListHead.mNxtNode;

    while ( (sCurOIDNode != &mOIDNodeListHead) &&
            (sCurOIDNode != NULL ) )
    {
        sItemCnt    = sCurOIDNode->mOIDCnt;
        sCurOIDInfo = sCurOIDNode->mArrOIDInfo;

        for(i = 0; i < sItemCnt; i++)
        {
            /* 현재 OID리스트의 OID가 Aging대상이면 Aging갯수를 더한다. */
            if( checkIsAgingTarget( aAgingState, sCurOIDInfo + i ) == ID_TRUE )
            {
                sAgingCnt++;
            }
            
            /* PROJ-1381 Fetch Across Commits
             * Legacy Trans를 종료할 때 OID List에 대한 pending job을 수행한다.
             * Commit할 때 ager에 aging 대상 OID 개수를 줘야하므로
             * aging 할 OID 개수(sAgingCnt)는 세도록 한다. */
            if( aIsLegacyTrans == ID_TRUE )
            {
                continue;
            }

            if( aAgingState == SM_OID_ACT_AGING_COMMIT )
            {
                IDE_TEST( processOIDListAtCommit( sCurOIDInfo + i,
                                                  aSCN )
                          != IDE_SUCCESS );
            }

            sFlag = sCurOIDInfo[i].mFlag;

            /* Aging해야 한다면 */
            if( (sFlag & aAgingState) == aAgingState )
            {
                if((sFlag & SM_OID_TYPE_MASK) == SM_OID_TYPE_DROP_TABLE)
                {
                    IDE_TEST(processDropTblPending(sCurOIDInfo + i)
                             != IDE_SUCCESS);
                }

                /* BUG-16161: Add Column이 실패한 후 다시 Add Column을 수행
                 * 하면 Session이 Hang상태에 빠집니다.: Transaction이 Commit이
                 * 나 Abort시Backup파일을 삭제한다. */
                if( (sFlag & SM_OID_TYPE_MASK) == SM_OID_TYPE_DELETE_TABLE_BACKUP )
                {
                    IDE_TEST( processDeleteBackupFile( aLSN,
                                                       sCurOIDInfo + i ) 
                              != IDE_SUCCESS);

                    /* BUG-31881 
                     * TableBackup에 관한 연산을 했을때, PageReservation
                     * 이 있으므로, 이를 해제해야 한다.*/
                    IDE_ASSERT( smcTable::getTableHeaderFromOID(
                                                    sCurOIDInfo[i].mTableOID,
                                                    (void**)&sTableHeader )
                                == IDE_SUCCESS );
                    sSpaceID = smcTable::getSpaceID( sTableHeader );
                    IDE_TEST( smmFPLManager::finalizePageReservation( 
                                                                mTrans,
                                                                sSpaceID )
                              != IDE_SUCCESS );
                    IDE_TEST( svmFPLManager::finalizePageReservation( 
                                                                mTrans,
                                                                sSpaceID )
                             != IDE_SUCCESS );
                }
            }
        }//For

        sCurOIDNode = sCurOIDNode->mNxtNode;
    }//While

    if( aIsLegacyTrans == ID_FALSE )
    {
        if( aProcessOIDOpt == SMX_OIDLIST_INIT )
        {
            /* OID List를 Ager가 처리하고 Free시키기 때문에
               OID LIst Header를 단순히 초기화한다.*/
            init();
        }
        else
        {
            /* Aging할 작업이 없어야 한다. */
            IDE_ASSERT( sAgingCnt == 0 );

            /* OID List를 Ager에게 넘기지 않았기 때문에 직접 Free한다.*/
            IDE_ASSERT(aProcessOIDOpt == SMX_OIDLIST_DEST);

            /* Transaction이 Insert만을 하고 Commit일 경우에만
               Ager가 OID List를 넘겨받지 않는다.*/
            IDE_ASSERT(aAgingState == SM_OID_ACT_AGING_COMMIT);
            IDE_TEST( freeOIDList() != IDE_SUCCESS);
        }
    }

    *aAgingCnt = sAgingCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   BUG-42760

   LEAGACY Transaction 이 종료될때 호출하며,
   smxOIDList::processOIDList() 함수를 수정하여
   drop된 table의 OID를 SKIP하는 코드를 추가하였습니다.
*/
IDE_RC smxOIDList::processOIDList4LegacyTx( smLSN   * aLSN,
                                            smSCN     aSCN )
{
    smxOIDNode      * sCurOIDNode = NULL;
    UInt              i;
    UInt              sItemCnt;
    smxOIDInfo      * sCurOIDInfo = NULL;
    SInt              sFlag;
    ULong             sAgingCnt = 0;
    scSpaceID         sSpaceID;
    smcTableHeader  * sTableHeader = NULL;
    SInt              sAgingState = SM_OID_ACT_AGING_COMMIT;

    sCurOIDNode = mOIDNodeListHead.mNxtNode;

    while ( ( sCurOIDNode != &mOIDNodeListHead ) &&
            ( sCurOIDNode != NULL ) )
    {
        sItemCnt = sCurOIDNode->mOIDCnt;
        sCurOIDInfo = sCurOIDNode->mArrOIDInfo;

        for ( i = 0; i < sItemCnt; i++ )
        {
            IDE_ASSERT( smcTable::getTableHeaderFromOID( sCurOIDInfo[i].mTableOID,
                                                         (void **)&sTableHeader )
                        == IDE_SUCCESS );

            if ( smcTable::isDropedTable(sTableHeader) == ID_TRUE )
            {
                continue;
            }

            IDU_FIT_POINT( "smxOIDList::processOIDList4LegacyTx::isDropedTable::sleep" );

            /* 현재 OID리스트의 OID가 Aging대상이면 Aging갯수를 더한다. */
            if ( checkIsAgingTarget( sAgingState, sCurOIDInfo + i ) == ID_TRUE )
            {
                sAgingCnt++;
            }

            IDE_TEST( processOIDListAtCommit( sCurOIDInfo + i, aSCN )
                      != IDE_SUCCESS );

            sFlag = sCurOIDInfo[i].mFlag;

            /* Aging해야 한다면 */
            if ( ( sFlag & sAgingState ) == sAgingState )
            {
                if ( ( sFlag & SM_OID_TYPE_MASK ) == SM_OID_TYPE_DROP_TABLE )
                {
                    IDE_TEST( processDropTblPending( sCurOIDInfo + i )
                              != IDE_SUCCESS);
                }

                /* BUG-16161: Add Column이 실패한 후 다시 Add Column을 수행
                 * 하면 Session이 Hang상태에 빠집니다.: Transaction이 Commit이
                 * 나 Abort시Backup파일을 삭제한다. */
                if ( ( sFlag & SM_OID_TYPE_MASK ) == SM_OID_TYPE_DELETE_TABLE_BACKUP )
                {
                    IDE_TEST( processDeleteBackupFile( aLSN, sCurOIDInfo + i )
                              != IDE_SUCCESS );

                    /* BUG-31881 
                     * TableBackup에 관한 연산을 했을때, PageReservation
                     * 이 있으므로, 이를 해제해야 한다.*/
                    sSpaceID = smcTable::getSpaceID( sTableHeader );
                    IDE_TEST( smmFPLManager::finalizePageReservation( mTrans, sSpaceID )
                              != IDE_SUCCESS );
                    IDE_TEST( svmFPLManager::finalizePageReservation( mTrans, sSpaceID )
                              != IDE_SUCCESS );
                }
                else
                {
                    /* Nothing to do */
                }

            }
            else
            {
                /* Nothing to do */
            }

        }//For

        sCurOIDNode = sCurOIDNode->mNxtNode;
    }//While

    /* OID List를 Ager가 처리하고 Free시키기 때문에
       OID LIst Header를 단순히 초기화한다.*/
    init();

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : row의 mNext를 SCN으로 변경한다.
 *               slotHeader를 변경하는 연산이지만 logging을 하지 않는다.
 *               왜냐하면 이함수는 commit이 된 이후에도 불릴수 있기 때문이다. commit
 *               이후에는 절대로 그 트랜잭션이 한 행위에 대해 로깅을 하지 않는다.
 *
 * aOIDInfo - [IN] Delete 연산이 수행된 row에대한 정보가 들어있는 smxOIDInfo
 * aSCN     - [IN] 현재 트랜잭션의 commitSCN | deleteBit
 *
 **********************************************************************/
IDE_RC smxOIDList::setSlotNextToSCN(smxOIDInfo* aOIDInfo,
                                    smSCN       aSCN)
{
    smpSlotHeader *sSlotHeader;

    IDE_ASSERT( smmManager::getOIDPtr( aOIDInfo->mSpaceID,
                                       aOIDInfo->mTargetOID,
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );
    if ( sctTableSpaceMgr::isVolatileTableSpace(aOIDInfo->mSpaceID) == ID_TRUE )
    {

        SM_SET_SCN( &(sSlotHeader->mLimitSCN), &aSCN);

    }
    else
    {
        IDE_ERROR ( sctTableSpaceMgr::isMemTableSpace(aOIDInfo->mSpaceID) == ID_TRUE );

        IDE_TEST(smcRecord::setRowNextToSCN( aOIDInfo->mSpaceID,
                                             (SChar*)sSlotHeader,
                                             aSCN)
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxOIDList::processOIDListAtCommit( smxOIDInfo* aOIDInfo,
                                           smSCN       aSCN )
{
    SInt             sFlag;
    UInt             sOPFlag;
    smpSlotHeader  * sSmpSlotHeader;
    SInt             sActFlag;
    SChar          * sRowPtr = NULL;
    smcTableHeader * sTableHeader;

    sFlag = aOIDInfo->mFlag;

    sActFlag = sFlag & SM_OID_ACT_COMMIT;
    sOPFlag  = sFlag & SM_OID_OP_MASK;

    if( sActFlag == SM_OID_ACT_COMMIT )
    {
        switch(sOPFlag)
        {
            case SM_OID_OP_DELETED_SLOT:
                SM_SET_SCN_DELETE_BIT(&aSCN);

                IDE_TEST( setSlotNextToSCN(aOIDInfo, aSCN) != IDE_SUCCESS );

                break;

            case SM_OID_OP_DDL_TABLE:
                /* Tablespace에 DDL이 발생했다는 알리기 위해 TableSpace의
                   DDL SCN을 변경한다.*/
                sctTableSpaceMgr::updateTblDDLCommitSCN( aOIDInfo->mSpaceID,
                                                         aSCN );

                break;

            case SM_OID_OP_NEW_FIXED_SLOT:
            case SM_OID_OP_CHANGED_FIXED_SLOT:
                IDE_ASSERT(aOIDInfo->mTargetOID != 0);

                /* PROJ-1594 Volatile TBS
                 * 처리할 OID가 volatile TBS에 속해 있으면 svc 모듈을 호출해야 한다.*/
                IDE_ERROR( smmManager::getOIDPtr( aOIDInfo->mSpaceID,
                                                  aOIDInfo->mTargetOID,
                                                  (void**)&sRowPtr )
                               == IDE_SUCCESS );
                if ( sctTableSpaceMgr::isVolatileTableSpace(aOIDInfo->mSpaceID) == ID_TRUE )
                {
                    IDE_TEST( svcRecord::setSCN( (SChar*)sRowPtr,
                                                 aSCN )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_ERROR ( sctTableSpaceMgr::isMemTableSpace(aOIDInfo->mSpaceID) == ID_TRUE );
                    
                    /* PROJ-2429 Dictionary based data compress for on-disk DB */
                    if ( (sFlag & SM_OID_ACT_COMPRESSION) != SM_OID_ACT_COMPRESSION )
                    {
                        IDE_TEST( smcRecord::setSCN( aOIDInfo->mSpaceID,
                                                     (SChar*)sRowPtr,
                                                     aSCN )
                                  != IDE_SUCCESS );
                    }
                    else
                    {
                        /* nothing to do */
                    }
                }
                break;

            case SM_OID_OP_UNLOCK_FIXED_SLOT:
                /* PROJ-1594 Volatile TBS
                   처리할 OID가 volatile TBS에 속해 있으면 svc 모듈을 호출해야 한다.*/
                IDE_ERROR( smmManager::getOIDPtr( aOIDInfo->mSpaceID,
                                                  aOIDInfo->mTargetOID,
                                                  (void**)&sRowPtr )
                           == IDE_SUCCESS );
                if ( sctTableSpaceMgr::isVolatileTableSpace(aOIDInfo->mSpaceID) == ID_TRUE )
                {
                    IDE_TEST( svcRecord::unlockRow( mTrans,
                                                    (SChar*)sRowPtr )
                              != IDE_SUCCESS );
                }
                else
                {
                    IDE_ERROR ( sctTableSpaceMgr::isMemTableSpace(aOIDInfo->mSpaceID) == ID_TRUE );

                    IDE_TEST( smcRecord::unlockRow( mTrans,
                                                    aOIDInfo->mSpaceID,
                                                    (SChar*)sRowPtr )
                              != IDE_SUCCESS );
                }
                break;

            case SM_OID_OP_DROP_TABLE:
                IDE_ERROR( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                  aOIDInfo->mTableOID,
                                                  (void**)&sSmpSlotHeader )
                           == IDE_SUCCESS );
                IDE_TEST( smcRecord::setDeleteBitOnHeader(
                                                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                  sSmpSlotHeader )
                          != IDE_SUCCESS );

                sTableHeader = (smcTableHeader *)( (smpSlotHeader*)sSmpSlotHeader + 1);

                IDE_TEST( smcTable::setTableCreateSCN( sTableHeader, SM_SCN_INFINITE )
                          != IDE_SUCCESS );

                break;

            case SM_OID_OP_DROP_INDEX:
                /* xxx */
                IDE_ERROR( smcTable::getTableHeaderFromOID( aOIDInfo->mTableOID,
                                                            (void**)&sTableHeader )
                            == IDE_SUCCESS );
                IDE_TEST( smcTable::dropIndexList( sTableHeader ) != IDE_SUCCESS );
                break;

            case SM_OID_OP_DROP_TABLE_BY_ABORT:
                IDE_ERROR( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                  aOIDInfo->mTargetOID,
                                                  (void**)&sRowPtr )
                           == IDE_SUCCESS );
                IDE_TEST( smcRecord::setSCN( aOIDInfo->mSpaceID,
                                             (SChar *)sRowPtr,
                                             aSCN )
                          != IDE_SUCCESS );

                sTableHeader = (smcTableHeader *)( (smpSlotHeader*)sRowPtr + 1);

                IDE_TEST( smcTable::setTableCreateSCN( sTableHeader, SM_SCN_INFINITE )
                          != IDE_SUCCESS );

                break;

            case SM_OID_OP_OLD_FIXED_SLOT:
                IDE_TEST( setSlotNextToSCN(aOIDInfo, aSCN) != IDE_SUCCESS );
                break;

            case SM_OID_OP_ALL_INDEX_DISABLE:
                IDE_TEST( processAllIndexDisablePending(aOIDInfo) != IDE_SUCCESS );
                break;

            case SM_OID_OP_CREATE_TABLE:
                IDE_ERROR( aOIDInfo->mTargetOID != 0 );

                IDE_ERROR( smcTable::getTableHeaderFromOID( aOIDInfo->mTableOID,
                                                            (void**)&sTableHeader )
                            == IDE_SUCCESS );

                IDE_TEST( smcTable::setTableCreateSCN( sTableHeader, aSCN )
                          != IDE_SUCCESS );
                break;

            default:
                break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Alter Table Add Column, Drop Column시 발생한 Backup파일
 *               을 삭제한다.
 *
 * aLSN     - [IN] Commit Log나 Abort로그의 LSN
 * aOIDInfo - [IN] OID Information
 *
 **********************************************************************/
IDE_RC smxOIDList::processDeleteBackupFile(smLSN*      aLSN,
                                           smxOIDInfo *aOIDInfo)
{
    SChar sStrFileName[SM_MAX_FILE_NAME];

    /* Abort나 Commit로그를 디스크에 완전히 내린후에
     * Backup파일 삭제를 수행하여야 한다. 왜냐하면 Backup File을
     * 삭제했는데 Commit로그가 기록되지 않아서 Transaction이 Abort해야하는
     * 경우가 생기기때문이다. */
    IDE_TEST( smrLogMgr::syncLFThread( SMR_LOG_SYNC_BY_TRX, aLSN )
              != IDE_SUCCESS );

    idlOS::snprintf(sStrFileName,
                    SM_MAX_FILE_NAME,
                    "%s%c%"ID_vULONG_FMT"%s",
                    smcTable::getBackupDir(),
                    IDL_FILE_SEPARATOR,
                    aOIDInfo->mTableOID,
                    SM_TABLE_BACKUP_EXT);

    if ( idf::unlink(sStrFileName) != 0 )
    {
        IDE_TEST_RAISE(idf::access(sStrFileName, F_OK) == 0,
                       err_file_unlink );
    }
    else
    {
        //==========================================================
        // To Fix BUG-13924
        //==========================================================
        ideLog::log(SM_TRC_LOG_LEVEL_MRECOV,
                    SM_TRC_MRECORD_FILE_UNLINK,
                    sStrFileName);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_unlink);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, sStrFileName));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smxOIDList::processDropTblPending(smxOIDInfo* aOIDInfo)
{
    smcTableHeader  *sTableHeader;
    smxTrans        *sTrans;
    SInt             sFlag = aOIDInfo->mFlag;

    IDE_ASSERT((sFlag & SM_OID_TYPE_MASK) == SM_OID_TYPE_DROP_TABLE);

    /* BUG-15047: Table이 Commit후에 Drop Table Pending연산을
       수행후 Return */
    /* Ager가 Drop된 Table에 대해 접근하는 것을 방지*/
    smLayerCallback::waitForNoAccessAftDropTbl();

    //added for A4
    // disk table에 대한 dropTable Pending연산은
    //  disk GC가 해야한다.
    IDE_ASSERT( smcTable::getTableHeaderFromOID( aOIDInfo->mTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    switch( sTableHeader->mFlag & SMI_TABLE_TYPE_MASK )
    {
        case SMI_TABLE_META: /* BUG-38698 */
        case SMI_TABLE_MEMORY:
        case SMI_TABLE_VOLATILE:
        case SMI_TABLE_DISK:
        {
            IDE_ASSERT( smxTransMgr::alloc(&sTrans) == IDE_SUCCESS );
            IDE_ASSERT( sTrans->begin( NULL,
                                       ( SMI_TRANSACTION_REPL_NONE |
                                         SMI_COMMIT_WRITE_NOWAIT ),
                                       SMX_NOT_REPL_TX_ID )
                        == IDE_SUCCESS);

            IDE_ASSERT( smcTable::dropTablePending( NULL,
                                                    sTrans,
                                                    sTableHeader)
                        == IDE_SUCCESS);
            IDE_ASSERT( sTrans->commit() == IDE_SUCCESS );
            IDE_ASSERT( smxTransMgr::freeTrans(sTrans) == IDE_SUCCESS);

            /* Drop Table연산이 수행되었기 때문에 OID Flag Off시키고
               Ager에서 이런 연산이 또 처리되는지를 check하기위해
               IDE_ASSERT로 확인한다.*/
            /* BUG-32237 [sm_transaction] Free lock node when dropping table.
             * DropTablePending 단계에서는 freeLockNode를 할 수 없습니다.
             * Commit단계이기 때문에, Lock을 풀어주는 작업을 해야 하기 때문입니다.
             * 따라서 Ager에 이 Flag를 켜두고, Ager에서 LockNode를 Free하게
             * 해줍니다. */
        }
    }

    return IDE_SUCCESS;
}

/*******************************************************************************
 * Description: ALL INDEX DISABLE 구문의 pending 연산을 처리하는 함수.
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 * aOIDInfo     - [IN] SM_OID_OP_ALL_INDEX_DISABLE 타입을 가진 smxOIDInfo
 *****************************************************************************/
IDE_RC smxOIDList::processAllIndexDisablePending( smxOIDInfo  * aOIDInfo )
{
    smcTableHeader  *sTableHeader;
    smxTrans        *sTrans;
    SInt             sFlag = aOIDInfo->mFlag;

    IDE_ASSERT( (sFlag & SM_OID_OP_MASK) == SM_OID_OP_ALL_INDEX_DISABLE );


    /* Ager가 Drop된 Table의 index에 대해 접근하는 것을 방지*/
    smLayerCallback::waitForNoAccessAftDropTbl();

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aOIDInfo->mTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    IDE_ASSERT( smxTransMgr::alloc(&sTrans) == IDE_SUCCESS );
    IDE_ASSERT( sTrans->begin( NULL,
                               ( SMI_TRANSACTION_REPL_NONE |
                                 SMI_COMMIT_WRITE_NOWAIT ),
                               SMX_NOT_REPL_TX_ID )
                == IDE_SUCCESS );

    IDE_ASSERT( smnManager::disableAllIndex( NULL,
                                             sTrans,
                                             sTableHeader )
                == IDE_SUCCESS );

    IDE_ASSERT( sTrans->commit() == IDE_SUCCESS );
    IDE_ASSERT( smxTransMgr::freeTrans(sTrans) == IDE_SUCCESS );

    return IDE_SUCCESS;
}

/***********************************************************************
 *
 * Description : OID에 해당하는 디스크 인덱스의 Integrity를 Verify 한다.
 *
 * BUG-27122 Restart Recovery 시 Undo Trans가 접근하는 인덱스에 대한
 * Integrity 체크기능 추가 (__SM_CHECK_DISK_INDEX_INTEGRITY=2)
 *
 * aStatistics  - [IN] 통계정보
 *
 **********************************************************************/
IDE_RC smxOIDList::processOIDListToVerify( idvSQL * aStatistics )
{
    SChar            sStrBuffer[128];
    smxOIDNode     * sCurOIDNode;
    smxOIDInfo     * sCurOIDInfo;
    UInt             i;
    UInt             sItemCnt;
    smnIndexHeader * sIndexHeader;
    idBool           sIsFail = ID_FALSE;

    sCurOIDNode = mOIDNodeListHead.mNxtNode;

    while (sCurOIDNode != &mOIDNodeListHead)
    {
        sItemCnt = sCurOIDNode->mOIDCnt;

        for(i = 0; i < sItemCnt; i++)
        {
            sCurOIDInfo  = &sCurOIDNode->mArrOIDInfo[i];

            IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                               sCurOIDInfo->mTargetOID,
                                               (void**)&sIndexHeader )
                        == IDE_SUCCESS );

            idlOS::snprintf( sStrBuffer,
                             128,
                             "       [TABLE OID: %"ID_vULONG_FMT", "
                             "INDEX ID: %"ID_UINT32_FMT"] ",
                             ((smnIndexHeader*)sIndexHeader)->mTableOID,
                             ((smnIndexHeader*)sIndexHeader)->mId );
            IDE_CALLBACK_SEND_SYM( sStrBuffer );

            sIsFail = ID_FALSE;

            if ( sIndexHeader->mTableOID != sCurOIDInfo->mTableOID )
            {
                idlOS::snprintf( sStrBuffer,
                                 128,
                                 " [FAIL : Invalid TableOID "
                                 "%"ID_vULONG_FMT",%"ID_vULONG_FMT"] ",
                                 ((smnIndexHeader*)sIndexHeader)->mTableOID,
                                 sCurOIDInfo->mTableOID );

                IDE_CALLBACK_SEND_SYM( sStrBuffer );
                sIsFail = ID_TRUE;
            }

            if ( sIndexHeader->mHeader == NULL )
            {
                IDE_CALLBACK_SEND_SYM( " [FAIL : Index Runtime Header is Null]" );
                sIsFail = ID_TRUE;
            }

            if ( sIsFail == ID_TRUE )
            {
                continue; // 위에 Warnning인 경우는 무시한다.
            }

            if ( sdnbBTree::verifyIndexIntegrity(
                                  aStatistics,
                                  sIndexHeader->mHeader )
                 != IDE_SUCCESS )
            {
                IDE_CALLBACK_SEND_SYM( " [FAIL]\n" );
            }
            else
            {
                IDE_CALLBACK_SEND_SYM( " [PASS]\n" );
            }
        }

        sCurOIDNode = sCurOIDNode->mNxtNode;
    }

    return IDE_SUCCESS;
}

/* BUG-42724 : XA 트랜잭션에 의해 insert/update된 레코드의 OID 플래그를
 * 수정한다.
 */ 
IDE_RC smxOIDList::setOIDFlagForInDoubt()
{

    smxOIDNode      *sCurOIDNode;
    UInt             i;
    UInt             sItemCnt;
    UInt             sFlag;
    SChar           *sRowPtr = NULL;
    smpSlotHeader   *sSlotHdr = NULL;

    sCurOIDNode = mOIDNodeListHead.mNxtNode;

    while (sCurOIDNode != &mOIDNodeListHead)
    {
        sItemCnt = sCurOIDNode->mOIDCnt;
        for (i = 0; i < sItemCnt; i++)
        {
            sFlag   = sCurOIDNode->mArrOIDInfo[i].mFlag;

            IDE_ASSERT( smmManager::getOIDPtr( sCurOIDNode->mArrOIDInfo[i].mSpaceID,
                                               sCurOIDNode->mArrOIDInfo[i].mTargetOID,
                                               (void**)&sRowPtr )               
                        == IDE_SUCCESS );

            sSlotHdr = (smpSlotHeader*)sRowPtr;
 
            if ( SM_SCN_IS_FREE_ROW(sSlotHdr->mCreateSCN) == ID_TRUE )
            {
                /* BUG-42724 : insert/update 도중 Unique Violation으로 실패한 경우 여기서 걸리면
                 * 이미 리파인된 new 버전이라는 뜻이다. 이미 리파인된 slot이므로 무시한다.
                 */
                continue;
            }
            else
            {
                /* Nothing to do */
            }

            switch (sFlag)
            {
                case SM_OID_OLD_UPDATE_FIXED_SLOT:
                    if( SM_SCN_IS_FREE_ROW( sSlotHdr->mLimitSCN ) )
                    {
                        /* BUG-42724 : update 도중 Unique Violation으로 실패한 경우 ( 여기서 걸리면
                         * old 버전이라는 뜻이다. ) old version을 Aging 하면 안된다. 따라서 Ager에서
                         * commit 시 무시하도록 플래그를 제거해 주어야 한다.
                         */
                        sCurOIDNode->mArrOIDInfo[i].mFlag &= ~SM_OID_OLD_UPDATE_FIXED_SLOT;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;
                
                case SM_OID_XA_INSERT_UPDATE_ROLLBACK:
                    /* BUG-42724 XA에서 insert/update되는 new version이 롤백되는 경우 인덱스
                     * aging을 위해서 다음의 SM_OID_ACT_AGING_INDEX 플래그를 추가해야 한다.
                     */
                    sCurOIDNode->mArrOIDInfo[i].mFlag |= SM_OID_ACT_AGING_INDEX;
                    break;
                    
                default:
                    break;
            }
        }
        sCurOIDNode = sCurOIDNode->mNxtNode;
    }

    return IDE_SUCCESS;
}

IDE_RC smxOIDList::setSCNForInDoubt(smTID aTID)
{

    smxOIDNode      *sCurOIDNode;
    smVCPieceHeader *sVCPieceHeader;
    UInt             i;
    UInt             sItemCnt;
    UInt             sFlag;
    SChar           *sRecord;
    smSCN            sDeletedSCN;
    smSCN            sInfiniteSCN;
    smpSlotHeader   *sSlotHdr;
    
    SM_SET_SCN_INFINITE_AND_TID( &sInfiniteSCN, aTID );
    SM_SET_SCN_INFINITE_AND_TID( &sDeletedSCN, aTID );
    SM_SET_SCN_DELETE_BIT( &sDeletedSCN );

    sCurOIDNode = mOIDNodeListHead.mNxtNode;
    sSlotHdr = NULL;  
    
    while (sCurOIDNode != &mOIDNodeListHead)
    {
        sItemCnt = sCurOIDNode->mOIDCnt;
        for(i = 0; i < sItemCnt; i++)
        {
            sFlag   = sCurOIDNode->mArrOIDInfo[i].mFlag;

            IDE_ASSERT( smmManager::getOIDPtr( sCurOIDNode->mArrOIDInfo[i].mSpaceID,
                                               sCurOIDNode->mArrOIDInfo[i].mTargetOID,
                                               (void**)&sRecord )
                        == IDE_SUCCESS );

            sSlotHdr = (smpSlotHeader*)sRecord;

            /* SCN을 무한대로 설정하여 record lock을 획득한 경우*/
            switch(sFlag)
            {
                case SM_OID_NEW_INSERT_FIXED_SLOT:
                case SM_OID_NEW_UPDATE_FIXED_SLOT:
                    /*
                     * [BUG-26415] XA 트랜잭션중 Partial Rollback(Unique Volation)된 Prepare
                     *             트랜잭션이 존재하는 경우 서버 재구동이 실패합니다.
                     * : Insert 도중 Unique Volation으로 실패한 경우는 기존의 Delete Bit를
                     *   유지해야 한다.
                     */
                    IDE_TEST(smcRecord::setSCN4InDoubtTrans(sCurOIDNode->mArrOIDInfo[i].mSpaceID,
                                                            aTID,
                                                            sRecord)
                             != IDE_SUCCESS);
                    
                    /* BUG-42724 : new 버전에 delete bit가 있다면 Partial Rollback하였기 때문에
                     * undoALL 끝나고 refine될 것이다. restart 완료후 commit/rollback될 때
                     * 이 플래그를 보고 SCN을 set하거나 다시 aging 하지 않도록 OID 플래그 세트를
                     *  수정한다.
                     */   
                    if ( SM_SCN_IS_DELETED( sSlotHdr->mCreateSCN ) )
                    {
                        sCurOIDNode->mArrOIDInfo[i].mFlag &= ~SM_OID_ACT_COMMIT;
                        sCurOIDNode->mArrOIDInfo[i].mFlag &= ~SM_OID_ACT_AGING_ROLLBACK;
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;
                case SM_OID_LOCK_FIXED_SLOT:
                    /*
                     * TO Fix BUG-14596
                     * XA Tx들에 한해 redo 이후에 Lock을 세팅한다.
                     */
                    SMP_SLOT_SET_LOCK( ((smpSlotHeader*)sRecord), aTID );
                    break;
                case SM_OID_OLD_VARIABLE_SLOT:
                   /*
                    * record delete의 경우 variable column에 대해
                    * delete flag를 FALSE로 설정
                    */
                    sVCPieceHeader = (smVCPieceHeader *)sRecord;
                    sVCPieceHeader->flag &= ~SM_VCPIECE_FREE_MASK;
                    sVCPieceHeader->flag |= SM_VCPIECE_FREE_NO;
                    break;
                case SM_OID_DELETE_FIXED_SLOT:
                    IDE_TEST( smcRecord::setRowNextToSCN(sCurOIDNode->mArrOIDInfo[i].mSpaceID,
                                                         sRecord,
                                                         sDeletedSCN)
                              != IDE_SUCCESS);
                    break;
                case SM_OID_OLD_UPDATE_FIXED_SLOT:
                    /* BUG-31062 일반적인 Update의 Old Version은 Refine 시 Free되지만
                     * XA Prepare Transaction이 Update한 경우는
                     * 아직 Commit되지 않았으므로 Free하면 안된다.
                     * refine시 Free하지 않도록 Slot Header Flag에 표시해둔다. */
                    SMP_SLOT_SET_SKIP_REFINE( (smpSlotHeader*)sRecord );
                    
                    /* BUG-42724 : free가 아닌 경우에만 무한대로 설정한다. XA에서는 old version의
                     * limit이 free면 이미 new가 partial rollback된 old이다.
                     */
                    if ( !(SM_SCN_IS_FREE_ROW( sSlotHdr->mLimitSCN )) )
                    {
                        IDE_TEST( smcRecord::setRowNextToSCN(sCurOIDNode->mArrOIDInfo[i].mSpaceID,
                                                             sRecord,
                                                             sInfiniteSCN)
                                  != IDE_SUCCESS);
                    }
                    else
                    {
                        /* Nothing to do */
                    }

                    break;
                default:
                    break;
            }
        }
        sCurOIDNode = sCurOIDNode->mNxtNode;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

void smxOIDList::dump()
{

    smxOIDNode     *sCurOIDNode;
    smxOIDNode     *sNxtOIDNode;
    UInt            sFlag;
    UInt            i;
    SInt            sOIDCnt = 0;

    SChar       sOIDFlag[][100] = {"SM_OID_OLD_FIXED_SLOT",
                                   "SM_OID_CHANGED_FIXED_SLOT",
                                   "SM_OID_NEW_FIXED_SLOT",
                                   "SM_OID_DELETED_SLOT",
                                   "SM_OID_LOCK_FIXED_SLOT",
                                   "SM_OID_OLD_VARIABLE_SLOT",
                                   "SM_OID_NEW_VARIABLE_SLOT",
                                   "SM_OID_DROP_TABLE",
                                   "SM_OID_DROP_TABLE_BY_ABORT",
                                   "SM_OID_DELETE_TABLE_BACKUP",
                                   "SM_OID_DROP_INDEX"};

    sCurOIDNode = mOIDNodeListHead.mNxtNode;
    idlOS::fprintf(stderr, " \n== OID Node Information ==\n");
    while(sCurOIDNode != &mOIDNodeListHead)
    {
        sNxtOIDNode = sCurOIDNode->mNxtNode;
        for (i=0; i<sCurOIDNode->mOIDCnt; i++)
        {
            sFlag = SM_OID_OP_MASK & sCurOIDNode->mArrOIDInfo[i].mFlag;
            idlOS::fprintf(stderr, "TableOid=%-10"ID_vULONG_FMT", recordOid=%-10"ID_vULONG_FMT", flag=%s\n",
                                    sCurOIDNode->mArrOIDInfo[i].mTableOID,
                                    sCurOIDNode->mArrOIDInfo[i].mTargetOID,
                                    sOIDFlag[sFlag]);

            sOIDCnt ++;
        }
        sCurOIDNode = sNxtOIDNode;
    }
    idlOS::fprintf(stderr, " Total Oid Count = %"ID_UINT32_FMT"\n", sOIDCnt);
    idlOS::fprintf(stderr, " ==========================\n");

}

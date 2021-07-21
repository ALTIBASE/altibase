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
 * $Id: sctTableSpaceMgr.cpp 17358 2006-07-31 03:12:47Z bskim $
 *
 * Description :
 *
 * 테이블스페이스 관리자 ( TableSpace Manager : sctTableSpaceMgr )
 *
 **********************************************************************/

#include <smDef.h>
#include <sctDef.h>
#include <smErrorCode.h>
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smu.h>
#include <sdd.h>
#include <smm.h>
#include <svm.h>
#include <sctReq.h>
#include <sctTableSpaceMgr.h>

iduMutex            sctTableSpaceMgr::mMtxCrtTBS;
iduMutex            sctTableSpaceMgr::mGlobalPageCountCheckMutex;
sctTableSpaceNode * sctTableSpaceMgr::mSpaceNodeArray[SC_MAX_SPACE_ARRAY_SIZE];
scSpaceID           sctTableSpaceMgr::mNewTableSpaceID;


// PRJ-1149 BACKUP & RECOVERY
// 미디어복구에 필요한 RedoLSN
smLSN          sctTableSpaceMgr::mDiskRedoLSN;
 // 미디어복구에 필요한 RedoLSN
smLSN          sctTableSpaceMgr::mMemRedoLSN;
extern smiGlobalCallBackList gSmiGlobalCallBackList;
/*
 * BUG-17285,17123
 * [PRJ-1548] offline된 TableSpace에 대해서 데이타파일을
 *            삭제하다가 Error 발생하여 diff
 *
 * 테이블스페이스의 Lock Validation Option 정의
 */
sctTBSLockValidOpt sctTableSpaceMgr::mTBSLockValidOpt[ SMI_TBSLV_OPER_MAXMAX ] =
{
        SCT_VAL_INVALID,
        SCT_VAL_DDL_DML,
        SCT_VAL_DROP_TBS

};


/***********************************************************************
 * Description : 테이블스페이스 관리자 초기화
 **********************************************************************/
IDE_RC sctTableSpaceMgr::initialize( )
{
    IDE_TEST( mGlobalPageCountCheckMutex.initialize(
                                  (SChar*)"SCT_GLOBAL_PAGE_COUNT_CHECK_MUTEX",
                                  IDU_MUTEX_KIND_POSIX,
                                  IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );

    IDE_TEST( mMtxCrtTBS.initialize( (SChar*)"CREATE_TABLESPACE_MUTEX",
                                     IDU_MUTEX_KIND_POSIX,
                                     IDV_WAIT_INDEX_LATCH_FREE_DRDB_TBS_CREATION )
              != IDE_SUCCESS );

    idlOS::memset( mSpaceNodeArray,
                   0x00,
                   ID_SIZEOF( mSpaceNodeArray ) );

    mNewTableSpaceID      = 0;

    // PRJ-1149 BACKUP & MEDIA RECOVERY
    SM_LSN_INIT( mDiskRedoLSN );

    SM_LSN_INIT( mMemRedoLSN );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :
 * initialize에서 생성했던 모든 자원을 파괴한다.
 **********************************************************************/
IDE_RC sctTableSpaceMgr::destroy()
{
    UInt               i;
    sctTableSpaceNode *sSpaceNode;

    /* ------------------------------------------------
     * tablespace 배열을 순회하면서 모든 tablespace 노드를
     * destroy한다. (SMI_ALL_NOTOUCH 모드)
     * ----------------------------------------------*/
    for( i = 0 ; i < mNewTableSpaceID ; i++ )
    {
        sSpaceNode = mSpaceNodeArray[i];

        if ( sSpaceNode == NULL )
        {
            continue;
        }

        switch( getTBSLocation( sSpaceNode ) )
        {
            case SMI_TBS_MEMORY:
            case SMI_TBS_VOLATILE:
                {
                    // do nothing
                    // Memory, Volatile Tablespace의 경우
                    // smm/svmTBSStartupShutdown::destroyAllTBSNode에서
                    // 모든 TBS의 자원을 파괴하고
                    // Tablespace Node메모리도 해제한다.
                    //
                    // sctTableSpaceMgr은 sctTableSpaceNode만 관리할 뿐,
                    // 그 안의 Resource는 smm/svmManager가 할당하였으므로,
                    // smm/svmManager가 해제하는 것이 원칙상 맞다.

                    IDE_ASSERT(0);
                }
            case SMI_TBS_DISK:
                {
                    IDE_TEST( sddTableSpace::removeAllDataFiles(
                                  NULL, /* idvSQL* */
                                  NULL,
                                  (sddTableSpaceNode*)sSpaceNode,
                                  SMI_ALL_NOTOUCH,
                                  ID_FALSE) /* Don't ghost mark */
                              != IDE_SUCCESS );

                    //bug-15653
                    IDE_TEST( sctTableSpaceMgr::destroyTBSNode( sSpaceNode )
                              != IDE_SUCCESS );

                    /* BUG-18236 DB생성시에 서버 비정상 종료합니다.
                     *
                     * mSpaceNodeArray의 Memory Tablespace Node는 이미 Free되었습니다.*/
                    IDE_TEST( iduMemMgr::free(sSpaceNode) != IDE_SUCCESS );
                }
                break;
            default:
                break;
        }
    }

    IDE_TEST( mMtxCrtTBS.destroy() != IDE_SUCCESS );

    IDE_TEST( mGlobalPageCountCheckMutex.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*
    ( Disk/Memory 공통 ) Tablespace Node를 초기화한다.

    [알고리즘]
    (010) Tablespace 이름을 복사
    (020) Tablespace Attribute들을 복사
    (030) Mutex 초기화
    (040) Lock Item 메모리 할당, 초기화
*/
IDE_RC sctTableSpaceMgr::initializeTBSNode( sctTableSpaceNode * aSpaceNode,
                                            smiTableSpaceAttr * aSpaceAttr )
{
    SChar sMutexName[128];
    UInt  sState = 0;

    IDE_DASSERT( aSpaceNode != NULL );
    IDE_DASSERT( aSpaceAttr != NULL );


    ///////////////////////////////////////////////////////////
    // (010) Tablespace 이름을 복사
    aSpaceNode->mName = NULL;

    /* sctTableSpaceMgr_initializeTBSNode_malloc_Name.tc */
    IDU_FIT_POINT("sctTableSpaceMgr::initializeTBSNode::malloc::Name");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SCT,
                                 1,
                                 aSpaceAttr->mNameLength + 1,
                                 (void**)&(aSpaceNode->mName),
                                 IDU_MEM_FORCE ) 
              != IDE_SUCCESS );
    sState = 1;

    idlOS::strcpy( aSpaceNode->mName, aSpaceAttr->mName );

    ///////////////////////////////////////////////////////////
    // (020) Tablespace Attribute들을 복사
    // tablespace의 ID
    aSpaceNode->mID  = aSpaceAttr->mID;
    // tablespace 타입
    aSpaceNode->mType =  aSpaceAttr->mType;
    // tablespace의 online 여부
    aSpaceNode->mState = aSpaceAttr->mTBSStateOnLA;
    // table create SCN을 0으로 초기화 한다.
    SM_INIT_SCN( &( aSpaceNode->mMaxTblDDLCommitSCN ) );

    ///////////////////////////////////////////////////////////
    // (030) Mutex 초기화
    idlOS::sprintf( sMutexName, "TABLESPACE_%"ID_UINT32_FMT"_SYNC_MUTEX",
                    aSpaceAttr->mID );

    IDE_TEST( aSpaceNode->mSyncMutex.initialize( sMutexName,
                                                 IDU_MUTEX_KIND_POSIX,
                                                 IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 2;

    ///////////////////////////////////////////////////////////
    // (040) Lock Item 초기화
    IDE_TEST( smLayerCallback::allocLockItem( &(aSpaceNode->mLockItem4TBS) )
              != IDE_SUCCESS );
    sState = 3;


    IDE_TEST( smLayerCallback::initLockItem( aSpaceAttr->mID, // TBS ID
                                             ID_ULONG_MAX,    // TABLE OID
                                             SMI_LOCK_ITEM_TABLESPACE,
                                             aSpaceNode->mLockItem4TBS )
              != IDE_SUCCESS );
    sState = 4;

    idlOS::sprintf( sMutexName, "TABLESPACE_NODE_MUTEX_%"ID_UINT32_FMT"",
                    aSpaceAttr->mID );

    IDE_TEST( aSpaceNode->mMutex.initialize( sMutexName,
                                             IDU_MUTEX_KIND_POSIX,
                                             IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sState = 5;

    idlOS::sprintf( sMutexName, "TABLESPACE_BACKUP_COND_%"ID_UINT32_FMT"",
                    aSpaceAttr->mID );

    IDE_TEST_RAISE( aSpaceNode->mBackupCV.initialize( sMutexName )
                    != IDE_SUCCESS, error_cond_init);
    sState = 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_cond_init);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_ThrCondInit ));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch( sState )
    {
        case 6:
            IDE_ASSERT( aSpaceNode->mBackupCV.destroy() == IDE_SUCCESS );
        case 5:
            IDE_ASSERT( aSpaceNode->mMutex.destroy() == IDE_SUCCESS );
        case 4:
            IDE_ASSERT( smLayerCallback::destroyLockItem( aSpaceNode->mLockItem4TBS )
                        == IDE_SUCCESS );
        case 3:
            IDE_ASSERT( smLayerCallback::freeLockItem( aSpaceNode->mLockItem4TBS )
                        == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( aSpaceNode->mSyncMutex.destroy() == IDE_SUCCESS );

        case 1:
            IDE_ASSERT( iduMemMgr::free( aSpaceNode->mName )
                        == IDE_SUCCESS );

            aSpaceNode->mName = NULL;

        default :
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
    ( Disk/Memory 공통 ) Tablespace Node를 파괴한다.

    initializeTBSNode의 초기화 순서의 역순으로 파괴

    [알고리즘]
    (010) Lock Item 파괴, 메모리 해제
    (020) Mutex 파괴
    (030) Tablespace 이름 메모리 공간 반납
*/
IDE_RC sctTableSpaceMgr::destroyTBSNode(sctTableSpaceNode * aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    /////////////////////////////////////////////////////////////
    // (010) Lock Item 파괴, 메모리 해제
    IDE_ASSERT( aSpaceNode->mLockItem4TBS != NULL );

    IDE_ASSERT( smLayerCallback::destroyLockItem( aSpaceNode->mLockItem4TBS )
                == IDE_SUCCESS );

    IDE_ASSERT( smLayerCallback::freeLockItem( aSpaceNode->mLockItem4TBS )
                == IDE_SUCCESS );
    aSpaceNode->mLockItem4TBS = NULL;

    /////////////////////////////////////////////////////////////
    // (020) Mutex 파괴
    IDE_ASSERT( aSpaceNode->mBackupCV.destroy() == IDE_SUCCESS );
    IDE_ASSERT( aSpaceNode->mMutex.destroy() == IDE_SUCCESS );
    IDE_ASSERT( aSpaceNode->mSyncMutex.destroy() == IDE_SUCCESS );

    /////////////////////////////////////////////////////////////
    // (030) Tablespace 이름 메모리 공간 반납
    IDE_ASSERT( aSpaceNode->mName != NULL );
    IDE_ASSERT( iduMemMgr::free( aSpaceNode->mName )
                == IDE_SUCCESS );
    aSpaceNode->mName = NULL;


    return IDE_SUCCESS;
}

/*
    Tablespace의 Sync Mutex를 획득한다.

    Checkpoint Thread가 해당 Tablespace에 Checkpointing을 하지 못하도록한다

    [IN] aSpaceNode - Sync Mutex를 획득할 Tablespace Node

 */
IDE_RC sctTableSpaceMgr::latchSyncMutex( sctTableSpaceNode * aSpaceNode )
{

    IDE_DASSERT( aSpaceNode != NULL );

    IDE_TEST( aSpaceNode->mSyncMutex.lock( NULL )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Tablespace의 Sync Mutex를 풀어준다.

    Checkpoint Thread가 해당 Tablesapce에 Checkpointing을 하도록 허용한다.

    [IN] aSpaceNode - Sync Mutex를 풀어줄 Tablespace Node
*/
IDE_RC sctTableSpaceMgr::unlatchSyncMutex( sctTableSpaceNode * aSpaceNode )
{
    IDE_ASSERT( aSpaceNode != NULL );

    IDE_TEST( aSpaceNode->mSyncMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 **********************************************************************/
void sctTableSpaceMgr::addTableSpaceNode( sctTableSpaceNode *aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    IDE_ASSERT( mSpaceNodeArray[aSpaceNode->mID] == NULL );

    mSpaceNodeArray[aSpaceNode->mID] = aSpaceNode;
}

void sctTableSpaceMgr::removeTableSpaceNode( sctTableSpaceNode *aSpaceNode )
{
    IDE_DASSERT( aSpaceNode != NULL );

    IDE_ASSERT( mSpaceNodeArray[aSpaceNode->mID] != NULL );

    mSpaceNodeArray[aSpaceNode->mID] = NULL;
}



/*
   기능 : 테이블의 테이블스페이스에 대하여 INTENTION
          Lock And Validation

          smiValidateAndLockTable(), smiTable::lockTable, 커서 open시 호출

   인자

   [IN]  aTrans        : 트랜잭션(smxTrans)의 void* 형
   [IN]  aSpaceID      : Lock을 획득할 TBS ID
   [IN]  aTBSLvType    : 테이블스페이스 Lock Validation 타입
   [IN]  aIsIntent     : 상위노드에 대한 INTENTION 여부
   [IN]  aIsExclusive  : 상위노드에 대한 Exclusive 여부
   [IN]  aLockWaitMicroSec : 잠금요청후 Wait 시간

*/
IDE_RC sctTableSpaceMgr::lockAndValidateTBS(
                        void                * aTrans,           /* [IN] */
                        scSpaceID             aSpaceID,         /* [IN] */
                        sctTBSLockValidOpt    aTBSLvOpt,        /* [IN] */
                        idBool                aIsIntent,        /* [IN] */
                        idBool                aIsExclusive,     /* [IN] */
                        ULong                 aLockWaitMicroSec )
{
    idBool                sLocked;

    IDE_DASSERT( aTrans != NULL );

    // PRJ-1548 User Memory Tablespace
    // 잠금계층에 대한 재조정
    //
    IDE_TEST( lockTBSNodeByID( aTrans,
                               aSpaceID,
                               aIsIntent,
                               aIsExclusive,
                               aLockWaitMicroSec,
                               aTBSLvOpt,
                               &sLocked,
                               NULL) != IDE_SUCCESS );

    // 만약, Trylock을 시도해보고 잠금을 획득하지 못한 경우 다음과
    // 같이 체크해서 Exception을 발생시킨다.
    IDE_TEST( sLocked != ID_TRUE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
   기능 : 테이블의 Index Lob 컬럼관련 테이블스페이스들에 대하여
          INTENTION Lock And Validation

          smiValidateAndLockTable(), smiTable::lockTable, 커서 open시 호출

          본 함수는 Table에 대한 LockAnd Validate를 수행한 후에
          수행되어야 한다

   인자

   [IN]  aTrans        : 트랜잭션(smxTrans)의 void* 형
   [IN]  aTable        : 테이블헤더(smcTableHeader)의  void* 형
   [IN]  aTBSLvOpt     : 테이블스페이스 Lock Validation Option
   [IN]  aIsIntent     : 상위노드에 대한 INTENTION 여부
   [IN]  aIsExclusive  : 상위노드에 대한 Exclusive 여부
   [IN]  aLockWaitMicroSec : 잠금요청후 Wait 시간

*/
IDE_RC sctTableSpaceMgr::lockAndValidateRelTBSs(
                       void                * aTrans,           /* [IN] */
                       void                * aTable,           /* [IN] */
                       sctTBSLockValidOpt    aTBSLvOpt,        /* [IN] */
                       idBool                aIsIntent,        /* [IN] */
                       idBool                aIsExclusive,     /* [IN] */
                       ULong                 aLockWaitMicroSec )
{
    UInt                  sIdx;
    UInt                  sIndexCount;
    UInt                  sLobColCount;
    UInt                  sColCount;
    scSpaceID             sIndexTBSID;
    scSpaceID             sLobTBSID;
    scSpaceID             sLockedTBSID;
    scGRID              * sGRID;
    void                * sIndexHeader;
    smiColumn           * sColumn;
    UInt                  sColumnType;
    scSpaceID             sTableTBSID;
    idBool                sLocked;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );

    sIndexHeader = NULL;
    sIndexCount  = 0;

    // 1. 테이블 관련 테이블스페이스 노드에 대한 잠금 요청
    sTableTBSID = smLayerCallback::getTableSpaceID( aTable );

    sLockedTBSID  = sTableTBSID; // 이미 잠금이 획득된 TableSpace

    if ( isDiskTableSpace( sTableTBSID ) == ID_TRUE )
    {
        // 2. 테이블의 인덱스 관련 테이블스페이스 노드에 대한 잠금 요청
        //    테이블에 관련된 인덱스가 저장된 모든 테이블스페이스 노드에
        //    대해 잠금을 획득한다.
        //    잠금을 획득한 TBS Node에 대해서 중복 잠금획득을 허용한다.        i
        //    (성능이슈)
        //
        //    단, 메모리 인덱스에 대한 테이블스페이스 노드는 존재하지 않으므로
        //    잠금이 필요하지 않다.

        sIndexCount  = smLayerCallback::getIndexCount( aTable );

        // 인덱스 TBS Node에 잠금을 획득한다.
        for ( sIdx = 0 ; sIdx < sIndexCount ; sIdx++ )
        {
            sIndexHeader = (void*)smLayerCallback::getTableIndex( aTable, sIdx );

            sGRID = smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );

            sIndexTBSID  = SC_MAKE_SPACE( *sGRID );

            // 일단, 연속적으로 중복된 경우에 대해서 잠금을 회피하도록
            // 간단히 처리하기한다. (성능이슈)

            if ( sLockedTBSID != sIndexTBSID )
            {
                IDE_TEST( lockTBSNodeByID( aTrans,
                                           sIndexTBSID,
                                           aIsIntent,
                                           aIsExclusive,
                                           aLockWaitMicroSec,
                                           aTBSLvOpt,
                                           &sLocked,
                                           NULL ) != IDE_SUCCESS );

                // 만약, Trylock을 시도해보고 잠금을 획득하지 못한 경우 다음과
                // 같이 체크해서 Exception을 발생시킨다.
                IDE_TEST( sLocked != ID_TRUE );

                sLockedTBSID = sIndexTBSID;
            }
        }

        // 3. 테이블의 LOB Column 관련 테이블스페이스 노드에 대한 잠금 요청
        //    테이블에 관련된 LOB Column 이 저장된 모든 테이블스페이스 노드에
        //    대해 잠금을 획득한다.
        //    잠금을 획득한 TBS Node에 대해서 중복 잠금획득을 허용한다.
        //    (성능이슈)
        //
        //    단, 메모리 LOB Column대한 테이블스페이스 노드는 테이블과 동일한
        //    테이블스페이스므로 잠금이 필요없다.
        //    테이블에 인덱스 개수를 구한다.

        sLockedTBSID = sTableTBSID;

        // 테이블에 Lob Column 개수를 구한다.
        sColCount    = smLayerCallback::getColumnCount( aTable );
        sLobColCount = smLayerCallback::getTableLobColumnCount( aTable );

        // Lob Column의 TBS Node에 잠금을 획득한다.
        for ( sIdx = 0 ; sIdx < sColCount ; sIdx++ )
        {
            if ( sLobColCount == 0 )
            {
                break;
            }

            sColumn =
                (smiColumn*)smLayerCallback::getTableColumn( aTable, sIdx );

            sColumnType =
                sColumn->flag & SMI_COLUMN_TYPE_MASK;

            if ( sColumnType == SMI_COLUMN_TYPE_LOB )
            {
                sLobColCount--;
                sLobTBSID  = sColumn->colSpace;
                IDE_ASSERT( sColumn->colSpace == SC_MAKE_SPACE(sColumn->colSeg) );

                // 일단, 연속적으로 중복된 경우에 대해서 잠금을 회피하도록
                // 간단히 처리하기한다. (성능이슈)

                if ( sLockedTBSID != sLobTBSID )
                {
                    IDE_TEST( lockTBSNodeByID( aTrans,
                                               sLobTBSID,
                                               aIsIntent,
                                               aIsExclusive,
                                               aLockWaitMicroSec,
                                               aTBSLvOpt,
                                               &sLocked,
                                               NULL ) != IDE_SUCCESS );

                    // 만약, Trylock을 시도해보고 잠금을 획득하지 못한 경우 다음과
                    // 같이 체크해서 Exception을 발생시킨다.
                    IDE_TEST( sLocked != ID_TRUE );

                    sLockedTBSID = sLobTBSID;
                }
                else  /* sLockedTBSID == sLobTBSID */
                {
                    /* nothing to do */
                }
            }
            else /* ColumnType != SMI_COLUMN_TYPE_LOB */ 
            {
                /* nothing to do */
            }
        }

        IDE_ASSERT( sLobColCount == 0 );
    }
    else
    {
        // Memory TableSpace의 경우는 Index와 Lob Column TableSpace가
        // 테이블과 동일하기 때문에 고려하지 않는다.
        IDE_DASSERT( ( isMemTableSpace( sTableTBSID ) == ID_TRUE ) ||
                     ( isVolatileTableSpace( sTableTBSID ) == ID_TRUE ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   기능 : 테이블스페이스 노드에 대한 잠금을 획득한다.
   인자주석 : lockTBSNode와 동일
*/
IDE_RC sctTableSpaceMgr::lockTBSNodeByID( void             * aTrans,
                                          scSpaceID          aSpaceID,
                                          idBool             aIsIntent,
                                          idBool             aIsExclusive,
                                          ULong              aLockWaitMicroSec,
                                          sctTBSLockValidOpt aTBSLvOpt,
                                          idBool           * aLocked,
                                          sctLockHier      * aLockHier )
{
    sctTableSpaceNode * sSpaceNode;

    IDE_DASSERT( aTrans != NULL );

    // 1. 시스템 테이블스페이스 노드는 잠금을 획득하지 않는다.
    //    단, DBF Node에 대해서만 잠금을 획득해주면
    //    CREATE DBF와 AUTOEXTEND MODE, 자동확장, RESIZE 연산간의
    //    동시성을 제어할 수 있다.
    if ( isSystemTableSpace( aSpaceID ) == ID_TRUE )
    {
        if ( aLocked != NULL )
        {
            *aLocked = ID_TRUE;
        }

        IDE_CONT( skip_lock_system_tbs );
    }

    // 2. 테이블 스페이스 노드 포인터를 획득
    // BUG-28748 과거에는 SpaceNode를 No Latch Hash로 관리하여
    // Space Node의 탐색을 위해서 Mutex가 필요하였으나,
    // 이제는 Array로 관리 하므로 Space Node를 가져오기 위해서
    // Mutex를 잡지 않아도 됩니다.
    IDE_TEST( findSpaceNodeBySpaceID( aSpaceID,
                                      (void**)&sSpaceNode )
              != IDE_SUCCESS );

    IDE_ASSERT( sSpaceNode->mID == aSpaceID );

    IDE_TEST( lockTBSNode( aTrans,
                           sSpaceNode,
                           aIsIntent,
                           aIsExclusive,
                           aLockWaitMicroSec,
                           aTBSLvOpt,
                           aLocked,
                           aLockHier ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_lock_system_tbs );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   기능 : 테이블스페이스 노드에 대한 잠금을 획득한다.
          TBS ID를 검색하여 테이블스페이스 노드를 반환하여야 한다.
          그러므로, TBS List Mutex를 획득할 필요가 있다.

   인자

   [IN]  aTrans             : 트랜잭션(smxTrans)의 void* 형
   [IN]  aSpaceID           : 테이블스페이스 ID
   [IN]  aIsIntent          : 상위노드에 대한 INTENTION 여부
   [IN]  aIsExclusive       : 상위노드에 대한 Exclusive 여부
   [IN]  aLockWaitMicroSec  : 잠금요청후 Wait 시간
   [IN]  aTBSLvOpt        : Lock획득후 Tablespace에 대해 체크할 사항들
   [OUT] aLocked            : Lock획득여부
   [OUT] aLockHier          : 잠금획득한 LockSlot 포인터

*/
IDE_RC sctTableSpaceMgr::lockTBSNode( void              * aTrans,
                                      sctTableSpaceNode * aSpaceNode,
                                      idBool              aIsIntent,
                                      idBool              aIsExclusive,
                                      ULong               aLockWaitMicroSec,
                                      sctTBSLockValidOpt  aTBSLvOpt,
                                      idBool           *  aLocked,
                                      sctLockHier      *  aLockHier )
{
    void              * sLockSlot;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aSpaceNode != NULL );

    sLockSlot  = NULL;

    // 2. 시스템 테이블스페이스 노드는 잠금을 획득하지 않는다.
    //    단, DBF Node에 대해서만 잠금을 획득해주면
    //    CREATE DBF와 AUTOEXTEND MODE, 자동확장, RESIZE 연산간의
    //    동시성을 제어할 수 있다.
    if ( isSystemTableSpace( aSpaceNode->mID ) == ID_TRUE )
    {
        if ( aLocked != NULL )
        {
            *aLocked = ID_TRUE;
        }
    }
    else
    {
        // 3. Lock 관리자에 잠금요청을 획득한다.
        IDE_TEST( smLayerCallback::lockItem( aTrans,
                                             aSpaceNode->mLockItem4TBS,
                                             aIsIntent,
                                             aIsExclusive,
                                             aLockWaitMicroSec,
                                             aLocked,
                                             &sLockSlot )
                  != IDE_SUCCESS );

        if ( aLocked != NULL )
        {
            // 만약, Trylock을 시도해보고 잠금을 획득하지 못한 경우
            // 다음과 같이 체크해서 Exception을 발생시킨다.
            IDE_TEST( *aLocked == ID_FALSE );
        }

        if ( aLockHier != NULL )
        {
            // Short-Duration 잠금을 획득하여 사용하기 원한다면
            // 잠금획득한 LockSlot 포인터를 LockHier에 출력한다.
            aLockHier->mTBSNodeSlot = sLockSlot;
        }

        IDE_TEST( validateTBSNode( aSpaceNode,
                                   aTBSLvOpt ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   기능 : 테이블스페이스 노드에 대한 잠금을 획득한다.
   인자주석 : 위의 lockTBSNode와 동일
*/

IDE_RC sctTableSpaceMgr::lockTBSNodeByID( void              * aTrans,
                                          scSpaceID           aSpaceID,
                                          idBool              aIsIntent,
                                          idBool              aIsExclusive,
                                          sctTBSLockValidOpt  aTBSLvOpt )
{
    return lockTBSNodeByID( aTrans,
                            aSpaceID,
                            aIsIntent,
                            aIsExclusive,
                            sctTableSpaceMgr::getDDLLockTimeOut( (smxTrans*)aTrans ),
                            aTBSLvOpt,
                            NULL,
                            NULL );
}

/*
   기능 : 테이블스페이스 노드에 대한 잠금을 획득한다.
   인자주석 : 위의 lockTBSNode와 동일
*/
IDE_RC sctTableSpaceMgr::lockTBSNode( void              * aTrans,
                                      sctTableSpaceNode * aSpaceNode,
                                      idBool              aIsIntent,
                                      idBool              aIsExclusive,
                                      sctTBSLockValidOpt  aTBSLvOpt )
{
    return lockTBSNode( aTrans,
                        aSpaceNode,
                        aIsIntent,
                        aIsExclusive,
                        sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aTrans),
                        aTBSLvOpt,
                        NULL,
                        NULL );
}

/***********************************************************************
 * Description : tablespace의 new ID 할당
 *
 * tablespace ID를 반환한다. 아이디는 1씩 증가하는 정수이고 tablespace ID는
 * 재사용되는 경우가 없다고 정한다. 로그를 보고
 * 이 tablespace가 재사용된것인지 판단할 방법이 없기 때문이다.
 * 따라서 Max Tablespace개가 creates되면 그 후에는 tablespace를 create할 수 없다.
 *
 * + 2nd. code design
 *   - mNewTableSpaceID를 +1 한다.
 **********************************************************************/
IDE_RC sctTableSpaceMgr::allocNewTableSpaceID( scSpaceID*   aNewID )
{

    IDE_DASSERT( aNewID != NULL );

    IDE_TEST_RAISE( mNewTableSpaceID == (SC_MAX_SPACE_COUNT -1),
                    error_not_enough_tablespace_id );

    *aNewID = mNewTableSpaceID++;

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_enough_tablespace_id );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotEnoughTableSpaceID,
                                  mNewTableSpaceID) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/**********************************************************************
 * Description : Tablespace ID로 SpaceNode를 찾는다.
 * - 해당 Tablespace가 DROP된 경우 에러를 발생시킨다.
 *
 * BUG-28748 과거에는 SpaceNode를 No Latch Hash로 관리하여
 * SpaceNode를 탐색하기 위해서는 반드시 Mutex를 잡아야 하였지만,
 * 이제는 Array로 관리하므로, Space Node를 찾는 것에 대해서는
 * Mutex를 잡지 않아도 됩니다.
 *
 * [IN]  aSpaceID   - Tablespace ID
 * [OUT] aSpaceNode - Tablespace Node
 **********************************************************************/
IDE_RC sctTableSpaceMgr::findSpaceNodeBySpaceID( idvSQL   * aStatistics,
                                                 scSpaceID  aSpaceID,
                                                 void**     aSpaceNode,
                                                 idBool     aLockSpace )
{
    sctTableSpaceNode * sSpaceNode ;
    idBool              sIsLocked = ID_FALSE;

    IDE_ERROR( aSpaceNode != NULL );

    IDE_TEST_RAISE( aSpaceID >= mNewTableSpaceID,
                    error_not_found_tablespace_node );

    sSpaceNode  = mSpaceNodeArray[aSpaceID];
    *aSpaceNode = sSpaceNode; 

    IDU_FIT_POINT_RAISE ( "BUG-45857@sctTableSpaceMgr::findSpaceNodeBySpaceID",
                           error_not_found_tablespace_node );
    IDE_TEST_RAISE( sSpaceNode == NULL,
                    error_not_found_tablespace_node );

    IDE_ASSERT_MSG( sSpaceNode->mID == aSpaceID,
                    "Node Space ID : %"ID_UINT32_FMT"\n"
                    "Req Space ID  : %"ID_UINT32_FMT"\n",
                    (*(sctTableSpaceNode**)aSpaceNode)->mID,
                    aSpaceID );

    if ( aLockSpace == ID_TRUE )
    {
        lockSpaceNode( aStatistics,
                       sSpaceNode );
        sIsLocked = ID_TRUE;
    }

    IDE_TEST_RAISE( SMI_TBS_IS_DROPPED( sSpaceNode->mState ),
                    error_not_found_tablespace_node );

    // Tablespace Drop Pending 수행도중 사망한 경우에도
    // Drop된것으로 간주
    IDE_TEST_RAISE( SMI_TBS_IS_DROP_PENDING( sSpaceNode->mState ),
                    error_not_found_tablespace_node );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNode,
                                  aSpaceID) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/*
   PRJ-1548 User Memory Tablespace
   테이블스페이스 노드 검색
   존재하지 않거나 DROPPED 상태의 경우는 NULL을 반환한다.

 [IN]  aSpaceID   - Tablespace ID
 [OUT] aSpaceNode - Tablespace Node
*/
sctTableSpaceNode* sctTableSpaceMgr::findSpaceNodeWithoutException( scSpaceID  aSpaceID,
                                                                    idBool     aUsingTBSAttr )
{
    sctTableSpaceNode *sSpaceNode;
    UInt               sTBSState;

    sSpaceNode = mSpaceNodeArray[aSpaceID];

    if ( sSpaceNode != NULL )
    {
        if ( (aUsingTBSAttr == ID_TRUE) &&
             (isMemTableSpace( sSpaceNode ) == ID_TRUE) )
        { 
            sTBSState = ((smmTBSNode*)sSpaceNode)->mTBSAttr.mTBSStateOnLA;
        }
        else
        {
            sTBSState = sSpaceNode->mState;
        }

        // Tablespace Drop Pending 수행도중 사망한 경우에도
        // Drop된것으로 간주
        if ( SMI_TBS_IS_DROPPED(sTBSState) ||
             SMI_TBS_IS_DROP_PENDING(sTBSState) )
        {
            sSpaceNode = NULL;
        }
    }

    return sSpaceNode;
}


/*
   Tablespace가 존재하는지 체크한다.

   다음의 경우 ID_FALSE를 반환한다.
     - 존재하지 않는 Tablespace
     - Drop된 Tablespace
   그 외에는 ID_TRUE를 반환한다.
     ( Offline, Discard된 Tablespace의 경우에도 ID_TRUE를 반환 )
 */
idBool sctTableSpaceMgr::isExistingTBS( scSpaceID aSpaceID )
{
    sctTableSpaceNode *sSpaceNode;
    idBool             sExist;

    sSpaceNode = findSpaceNodeWithoutException( aSpaceID );

    if ( sSpaceNode == NULL )
    {
        sExist = ID_FALSE;
    }
    else
    {
        sExist = ID_TRUE;
    }
    return sExist;
}




/*
   Tablespace가 Memory에 Load되었는지 체크한다.

   다음의 경우 ID_FALSE를 반환한다.
     - 존재하지 않는 Tablespace
     - Drop된 Tablespace
     - OFFLINE된 Tablespace
   그 외에는 ID_TRUE를 반환한다.
 */
idBool sctTableSpaceMgr::isOnlineTBS( scSpaceID aSpaceID )
{
    idBool sIsOnline;

    sctTableSpaceNode *sSpaceNode;
    sSpaceNode = findSpaceNodeWithoutException( aSpaceID );

    if ( sSpaceNode == NULL )
    {
        sIsOnline = ID_FALSE;
    }
    else
    {
        // findSpaceNodeWithoutException 은 Drop된 Tablespace의 경우
        // TBSNode로 NULL을 리턴한다.
        // TBSNode가 NULL이 아니므로, TBS의 상태가 DROPPED일 수 없다.
        IDE_ASSERT( ( sSpaceNode->mState & SMI_TBS_DROPPED )
                    != SMI_TBS_DROPPED );

        if ( SMI_TBS_IS_ONLINE(sSpaceNode->mState) )
        {
            sIsOnline = ID_TRUE;
        }
        else
        {
            // OFFLINE이거나 DISCARD된 TABLESPACE
            sIsOnline = ID_FALSE;
        }
    }

    return sIsOnline;
}

/* Tablespace가 여러 State중 하나의 State를 지니는지 체크한다.

   [IN] aSpaceID  - 상태를 체크할 Tablespace의 ID
   [IN] aStateSet - 하나이상의 Tablespace상태를 OR로 묶은 State Set
 */
idBool sctTableSpaceMgr::hasState( scSpaceID   aSpaceID,
                                   sctStateSet aStateSet,
                                   idBool      aUsingTBSAttr )
{
    idBool             sRet = ID_FALSE;
    sctTableSpaceNode *sSpaceNode;
    UInt               sTBSState;

    IDE_DASSERT( aStateSet != SCT_SS_INVALID );

    sSpaceNode = findSpaceNodeWithoutException( aSpaceID, aUsingTBSAttr );

    // DROP된 Tablespace
    if ( sSpaceNode == NULL )
    {
        // StateSet에 DROPPED가 있으면 ID_TRUE리턴
        if ( SMI_TBS_IS_DROPPED( aStateSet) )
        {
            sRet = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        if ( ( aUsingTBSAttr == ID_TRUE ) &&
             ( isMemTableSpace(aSpaceID) == ID_TRUE ) )
        { 
            sTBSState = ((smmTBSNode*)sSpaceNode)->mTBSAttr.mTBSStateOnLA;
        }
        else
        {
            sTBSState = sSpaceNode->mState;
        }

        // Tablespace의 State가 aStateSet이 지니는 여러
        // State중 하나를 지니는 경우 ID_TRUE리턴
        if ( ( sTBSState & aStateSet ) != 0 )
        {
            sRet = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }
    }

    return sRet;
}

/* Tablespace가 여러 State중 하나의 State를 지니는지 체크한다.

   [IN] aSpaceNode - 상태를 체크할 Tablespace의 Node
   [IN] aStateSet  - 하나이상의 Tablespace상태를 OR로 묶은 State Set
 */
idBool sctTableSpaceMgr::hasState( sctTableSpaceNode * aSpaceNode,
                                   sctStateSet         aStateSet )
{
    IDE_DASSERT( aSpaceNode != NULL );

    return isStateInSet( aSpaceNode->mState, aStateSet );
}




/* Tablespace의 State에 aStateSet안의 State가 하나라도 있는지 체크한다.

   [IN] aTBSState  - 상태를 체크할 Tablespace의 State
   [IN] aStateSet  - 하나이상의 Tablespace상태를 OR로 묶은 State Set
 */
idBool sctTableSpaceMgr::isStateInSet( UInt        aTBSState,
                                       sctStateSet aStateSet )
{
    idBool sHasState ;

    // Tablespace의 State가 aStateSet이 지니는 여러
    // State중 하나를 지니는 경우 ID_TRUE리턴
    if ( ( aTBSState & aStateSet ) != 0 )
    {
        sHasState = ID_TRUE;
    }
    else
    {
        sHasState = ID_FALSE;
    }

    return sHasState;
}


/*
   Tablespace안의 Table/Index를 Open하기 전에 Tablespace가
   사용 가능한지 체크한다.

   [IN] aSpaceNode - Tablespace의 Node
   [IN] aValidate  -

   [주의] 해당 Tablespace에 Lock이 잡힌 채로 이 함수가 불려야
           이 함수호출시점의 상황이 그대로 유지됨을 보장할 수 있다.

   다음의 경우 에러를 발생시킨다.
     - 존재하지 않는 Tablespace
     - Drop된 Tablespace
     - Discard된 Tablespace
     - Offline Tablespace
 */
IDE_RC sctTableSpaceMgr::validateTBSNode( sctTableSpaceNode * aSpaceNode,
                                          sctTBSLockValidOpt  aTBSLvOpt )
{
    IDE_DASSERT( aSpaceNode != NULL );

    if ( ( aTBSLvOpt & SCT_VAL_CHECK_DROPPED ) == SCT_VAL_CHECK_DROPPED )
    {
        // DROP된 테이블스페이스의 경우 테이블스페이스가
        // 존재하지 않는다는 Exception으로 처리한다.
        IDE_TEST_RAISE( SMI_TBS_IS_DROPPED(aSpaceNode->mState),
                        error_not_found_tablespace_node );
    }



    if ( ( aTBSLvOpt & SCT_VAL_CHECK_DISCARDED ) ==
         SCT_VAL_CHECK_DISCARDED )
    {
        // Discard된 Tablespace의 경우
        IDE_TEST_RAISE( SMI_TBS_IS_DISCARDED(aSpaceNode->mState),
                        error_unable_to_use_discarded_tbs );
    }


    if ( ( aTBSLvOpt & SCT_VAL_CHECK_OFFLINE ) ==
         SCT_VAL_CHECK_OFFLINE )
    {
        // Offline Tablespace인 경우
        IDE_TEST_RAISE( SMI_TBS_IS_OFFLINE(aSpaceNode->mState),
                        error_unable_to_use_offline_tbs );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_NotFoundTableSpaceNode,
                                 aSpaceNode->mID) );
    }
    IDE_EXCEPTION( error_unable_to_use_discarded_tbs );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_UNABLE_TO_USE_DISCARDED_TBS,
                                 aSpaceNode->mName ) );
    }
    IDE_EXCEPTION( error_unable_to_use_offline_tbs );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_UNABLE_TO_USE_OFFLINE_TBS,
                                 aSpaceNode->mName ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/**********************************************************************
 * Description : TBS명을 입력받아 테이블스페이스 Node를 반환한다.
 *               이전에는 TBS Node가 없을 경우 NULL을 반환 하였으나
 *               BUG-26695에 의하여 isExistTBSNodeByName()와 용도에 따라
 *               둘로 나누어지되면서 TBS Node가 없을 경우 오류를 반환하는 것으로
 *               수정되었다.
 *
 *   aName      - [IN]  TBS Node를 찾을 TBS의 이름
 *   aSpaceNode - [OUT] 찾은 TBS Node를 반환
 **********************************************************************/
IDE_RC sctTableSpaceMgr::findSpaceNodeByName( SChar* aName,
                                              void** aSpaceNode,
                                              idBool aLockSpace )
{
    idBool             sIsLocked = ID_FALSE;
    UInt               i;
    idBool             sIsExist = ID_FALSE;
    sctTableSpaceNode *sSpaceNode;

    IDE_DASSERT( aName != NULL );
    IDE_DASSERT( aSpaceNode != NULL );

    for ( i = 0 ; i < mNewTableSpaceID ; i++ )
    {
        sSpaceNode = mSpaceNodeArray[i];

        if ( sSpaceNode == NULL )
        {
            continue;
        }

        if ( aLockSpace == ID_TRUE )
        {
            lockSpaceNode( NULL, sSpaceNode );
            sIsLocked = ID_TRUE;
        }

        if ( !SMI_TBS_IS_DROPPED(sSpaceNode->mState) )
        {
            if ( idlOS::strcmp( sSpaceNode->mName, aName) == 0 )
            {
                *aSpaceNode = sSpaceNode;
                sIsExist = ID_TRUE;
                break;
            }
        }

        if ( aLockSpace == ID_TRUE )
        {
            sIsLocked = ID_FALSE;
            unlockSpaceNode( sSpaceNode );
        }
    }

    IDE_TEST_RAISE( sIsExist == ID_FALSE,
                    error_not_found_tablespace_node_by_name );

    // Tablespace Drop Pending 수행도중 사망한 경우에도
    // Drop된것으로 간주
    IDE_TEST_RAISE( SMI_TBS_IS_DROP_PENDING( sSpaceNode->mState ),
                    error_not_found_tablespace_node_by_name );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node_by_name );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNodeByName,
                                  aName) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/**********************************************************************
 * Description : TBS Node Name을 넘겨받아 존재하는지만 확인한다.
 *
 *   aName - [IN] TBS Node가 존재하는지 확인할 TBS의 Name
 **********************************************************************/
idBool sctTableSpaceMgr::checkExistSpaceNodeByName( SChar* aTableSpaceName )
{
    UInt               i;
    idBool             sIsExist = ID_FALSE;
    sctTableSpaceNode *sSpaceNode;

    IDE_DASSERT( aTableSpaceName != NULL );

    for ( i = 0 ; i < mNewTableSpaceID ; i++ )
    {
        sSpaceNode = mSpaceNodeArray[i];

        if ( sSpaceNode == NULL )
        {
            continue;
        }

        if ( SMI_TBS_IS_DROPPED(sSpaceNode->mState) )
        {
            continue;
        }

        if ( idlOS::strcmp(sSpaceNode->mName, aTableSpaceName) == 0 )
        {
            sIsExist = ID_TRUE;
            break;
        }
    }

    return sIsExist;
}


/**********************************************************************
 * Description : for creating table on Query Processing
 **********************************************************************/
IDE_RC sctTableSpaceMgr::getTBSAttrByName( SChar*              aName,
                                           smiTableSpaceAttr*  aSpaceAttr )
{

    UInt               sState = 0;
    sctTableSpaceNode* sSpaceNode = NULL;

    IDE_DASSERT( aName != NULL );
    IDE_DASSERT( aSpaceAttr != NULL );

    IDE_TEST( findSpaceNodeByName( aName,
                                   (void**)&sSpaceNode,
                                   ID_TRUE ) // Lock
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sSpaceNode == NULL );

    IDE_TEST_RAISE( SMI_TBS_IS_CREATING(sSpaceNode->mState),
                    error_not_found_tablespace_node_by_name );

    switch( getTBSLocation( sSpaceNode ) )
    {
        case SMI_TBS_DISK:
            sddTableSpace::getTableSpaceAttr( (sddTableSpaceNode*)sSpaceNode,
                                               aSpaceAttr );
            break;
        case SMI_TBS_VOLATILE:
            svmManager::getTableSpaceAttr( (svmTBSNode*)sSpaceNode,
                                          aSpaceAttr );
            break;
        case SMI_TBS_MEMORY:
            smmManager::getTableSpaceAttr( (smmTBSNode*)sSpaceNode,
                                           aSpaceAttr );
            break;
        default:
            break;
    }

    sState = 0;
    unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_found_tablespace_node_by_name );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotFoundTableSpaceNodeByName,
                                  aName) );
    }
    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/**********************************************************************
 * Description : 테이블스페이스 ID에 해당하는 테이블스페이스 속성을 반환
 **********************************************************************/
IDE_RC sctTableSpaceMgr::getTBSAttrByID( idvSQL           * aStatistics,
                                         scSpaceID          aID,
                                         smiTableSpaceAttr* aSpaceAttr )
{
    UInt               sState = 0;
    sctTableSpaceNode* sSpaceNode;

    IDE_DASSERT( aSpaceAttr != NULL );

    IDE_TEST( findAndLockSpaceNodeBySpaceID( aStatistics,
                                             aID,
                                             (void**)&sSpaceNode )
              != IDE_SUCCESS );
    sState = 1;

    switch( sctTableSpaceMgr::getTBSLocation( sSpaceNode ) )
    {
        case SMI_TBS_DISK:
            sddTableSpace::getTableSpaceAttr( (sddTableSpaceNode*)sSpaceNode,
                                               aSpaceAttr );
            break;
        case SMI_TBS_MEMORY:
            smmManager::getTableSpaceAttr( (smmTBSNode*)sSpaceNode,
                                           aSpaceAttr );
            break;
        case SMI_TBS_VOLATILE:
            svmManager::getTableSpaceAttr( (svmTBSNode*)sSpaceNode,
                                          aSpaceAttr );
            break;
        default:
            break;
    }

    sState = 0;
    unlockSpaceNode( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;

}

/*
    Tablespace Attribute의 flag 를 반환한다.
 */
IDE_RC sctTableSpaceMgr::getTBSAttrFlagByID( scSpaceID   aSpaceID,
                                             UInt      * aAttrFlagPtr )
{
    sctTableSpaceNode* sSpaceNode;

    IDE_DASSERT( aAttrFlagPtr != NULL );

    IDE_TEST( findSpaceNodeBySpaceID( aSpaceID,
                                      (void**)&sSpaceNode )
              != IDE_SUCCESS );

    *aAttrFlagPtr = getTBSAttrFlag( sSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt sctTableSpaceMgr::getTBSAttrFlag( sctTableSpaceNode* aSpaceNode )
{
    UInt sAttrFlag = SMI_TBS_NONE;

    switch( sctTableSpaceMgr::getTBSLocation( aSpaceNode ) )
    {
        case SMI_TBS_DISK:
            sAttrFlag = sddTableSpace::getTBSAttrFlag( (sddTableSpaceNode*)aSpaceNode );
            break;
        case SMI_TBS_MEMORY:
            sAttrFlag = smmManager::getTBSAttrFlag( (smmTBSNode*)aSpaceNode );
            break;
        case SMI_TBS_VOLATILE:
            sAttrFlag = svmManager::getTBSAttrFlag( (svmTBSNode*)aSpaceNode );
            break;
        default:
            break;
    }

    return sAttrFlag;
}

/*
    Tablespace Attribute의 flag 를 세팅한다.
 */
void sctTableSpaceMgr::setTBSAttrFlag( sctTableSpaceNode* aSpaceNode,
                                       UInt               aAttrFlag )
{
    switch( sctTableSpaceMgr::getTBSLocation( aSpaceNode ) )
    {
        case SMI_TBS_DISK:
            sddTableSpace::setTBSAttrFlag( (sddTableSpaceNode*)aSpaceNode,
                                           aAttrFlag );
            break;
        case SMI_TBS_MEMORY:
            smmManager::setTBSAttrFlag( (smmTBSNode*)aSpaceNode,
                                        aAttrFlag );
            break;
        case SMI_TBS_VOLATILE:
            svmManager::setTBSAttrFlag( (svmTBSNode*)aSpaceNode,
                                        aAttrFlag );
            break;
        default:
            break;
    }
}


/*
    Tablespace의 Attribute Flag로부터 로그 압축여부를 얻어온다

    [IN] aSpaceID - Tablespace의 ID
    [OUT] aDoComp - Log압축 여부
 */
IDE_RC sctTableSpaceMgr::getSpaceLogCompFlag( scSpaceID aSpaceID,
                                              idBool   *aDoComp )
{
    IDE_DASSERT( aDoComp != NULL );

    UInt        sAttrFlag;
    idBool      sDoComp;

    if ( sctTableSpaceMgr::getTBSAttrFlagByID( aSpaceID,
                                               &sAttrFlag )
         == IDE_SUCCESS )
    {
        if ( ( sAttrFlag & SMI_TBS_ATTR_LOG_COMPRESS_MASK )
             == SMI_TBS_ATTR_LOG_COMPRESS_TRUE )
        {
            sDoComp = ID_TRUE;
        }
        else
        {
            sDoComp = ID_FALSE;
        }
    }
    else
    {
        // 아직 Tablespace가 생기지도 않은 상태
        if ( ideGetErrorCode() == smERR_ABORT_NotFoundTableSpaceNode )
        {
            IDE_CLEAR();
            // 기본적으로 가능하면 로그 압축 실시
            sDoComp = ID_TRUE;
        }
        else
        {
            IDE_RAISE( err_get_tbs_attr );
        }
    }

    *aDoComp = sDoComp;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_get_tbs_attr)
    {
        // do nothing. continue
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : refine 단계에서 호출:모든 temp tablespace를 초기화시킨다.
 **********************************************************************/
IDE_RC sctTableSpaceMgr::resetAllTempTBS( void *aTrans )
{
    UInt                i;
    sctTableSpaceNode*  sSpaceNode;

    IDE_DASSERT( aTrans != NULL );

    for( i = 0 ; i < mNewTableSpaceID ; i++ )
    {
        sSpaceNode = mSpaceNodeArray[i];

        if ( sSpaceNode == NULL )
        {
            continue;
        }

        if ( (sSpaceNode->mType == SMI_DISK_SYSTEM_TEMP) ||
             (sSpaceNode->mType == SMI_DISK_USER_TEMP) )
        {
            if ( (sSpaceNode->mState & SMI_TBS_DROPPED) != SMI_TBS_DROPPED )
            {
                IDE_TEST( smLayerCallback::resetTBS( NULL,
                                                     sSpaceNode->mID,
                                                     aTrans )
                          != IDE_SUCCESS );
            }
            else
            {
                // fix BUG-17501
                // 서버구동시 user disk temp tablespace reset과정에서
                // DROOPED 상태의 TBS에 대해 Assert 걸면 안됨.
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : 데이타 파일 해더 갱신.
 * 데이타 file이름으로 데이타 파일 노드를 찾는다.
 * -> smiMediaRecovery class에 의하여 불린다.
 **********************************************************************/
void  sctTableSpaceMgr::getDataFileNodeByName( SChar            * aFileName,
                                               sddDataFileNode ** aFileNode,
                                               scSpaceID        * aSpaceID,
                                               scPageID         * aFstPageID,
                                               scPageID         * aLstPageID,
                                               SChar           ** aSpaceName )
{
    UInt                i;
    scPageID            sFstPageID;
    scPageID            sLstPageID;
    sctTableSpaceNode*  sSpaceNode;

    IDE_DASSERT( aFileName != NULL );
    IDE_DASSERT( aFileNode != NULL );
    IDE_DASSERT( aSpaceID  != NULL );

    *aFileNode = NULL;

    for ( i = 0 ; i < mNewTableSpaceID ; i++ )
    {
        sSpaceNode = mSpaceNodeArray[i];

        if ( sSpaceNode == NULL )
        {
            continue;
        }

        if ( isDiskTableSpace( sSpaceNode ) == ID_FALSE )
        {
            continue;
        }

        lockSpaceNode( NULL /* idvSQL* */,
                       sSpaceNode );

        if ( SMI_TBS_IS_DROPPED( sSpaceNode->mState ) )
        {
            unlockSpaceNode( sSpaceNode );
            continue;
        }

        if ( sddTableSpace::getPageRangeByName( (sddTableSpaceNode*)sSpaceNode,
                                                aFileName,
                                                aFileNode,
                                                &sFstPageID,
                                                &sLstPageID) == IDE_SUCCESS )
        {
            if ( aFstPageID != NULL )
            {
                *aFstPageID = sFstPageID;
            }

            if ( aLstPageID != NULL )
            {
                *aLstPageID = sLstPageID;
            }

            if ( aSpaceName != NULL )
            {
                *aSpaceName = sSpaceNode->mName;
            }

            *aSpaceID = sSpaceNode->mID;
            unlockSpaceNode( sSpaceNode );
            break;
        }

        unlockSpaceNode( sSpaceNode );
    }
}

/***********************************************************************
 * Description : 트랜잭션 커밋 직전에 수행하기 위한 연산을 등록
 *
 * Disk Tablespace, Memory Tablespace모두 이 루틴을 이용한다.
 *
 * [IN] aTrans     : Pending Operation을 수행하게 될 Transaction
 * [IN] aSpaceID   : Pending Operation수행 대상이 되는 Tablespace
 * [IN] aIsCommit  : Commit시에 동작하는 Pending Operation이라면 ID_TRUE
 * [IN] aPendingOpType : Pending Operation의 종류
 * [OUT] aPendingOp : 새로 등록한 Pending Operation
 *                    aPendingOp != NULL인 경우에만 설정된다.
 **********************************************************************/
IDE_RC sctTableSpaceMgr::addPendingOperation( void               * aTrans,
                                              scSpaceID            aSpaceID,
                                              idBool               aIsCommit,
                                              sctPendingOpType     aPendingOpType,
                                              sctPendingOp      ** aPendingOp ) /* = NULL*/
{

    UInt         sState = 0;
    smuList      *sPendingOpList;
    sctPendingOp *sPendingOp;

    IDE_DASSERT( aTrans   != NULL );

    /* sctTableSpaceMgr_addPendingOperation_malloc_PendingOpList.tc */
    IDU_FIT_POINT("sctTableSpaceMgr::addPendingOperation::malloc::PendingOpList");
    IDE_TEST(iduMemMgr::malloc( IDU_MEM_SM_SDD,
                                ID_SIZEOF(smuList) + 1,
                                (void**)&sPendingOpList,
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sState = 1;

    SMU_LIST_INIT_BASE(sPendingOpList);

    /* sctTableSpaceMgr_addPendingOperation_calloc_PendingOp.tc */
    IDU_FIT_POINT("sctTableSpaceMgr::addPendingOperation::calloc::PendingOp");
    IDE_TEST(iduMemMgr::calloc( IDU_MEM_SM_SDD,
                                1,
                                ID_SIZEOF(sctPendingOp),
                                (void**)&sPendingOp,
                                IDU_MEM_FORCE) != IDE_SUCCESS );
    sState = 2;

    sPendingOp->mIsCommit       = aIsCommit;
    sPendingOp->mPendingOpType  = aPendingOpType;
    sPendingOp->mTouchMode      = SMI_ALL_NOTOUCH; // meaningless
    sPendingOp->mSpaceID        = aSpaceID;
    sPendingOp->mResizePageSize = 0; // alter resize
    sPendingOp->mFileID         = 0; // meaningless

    SM_LSN_INIT( sPendingOp->mOnlineTBSLSN );

    sPendingOp->mPendingOpFunc  = NULL; // NULL이면 함수 호출 안함
    sPendingOp->mPendingOpParam = NULL; // NULL이면 전달인자 없음

    sPendingOpList->mData = sPendingOp;

    smLayerCallback::addPendingOperation( aTrans, sPendingOpList );

    if ( aPendingOp != NULL )
    {
        *aPendingOp = sPendingOp;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    switch ( sState )
    {
        case 2 :
            IDE_ASSERT( iduMemMgr::free(sPendingOp) == IDE_SUCCESS );
        case 1 :
            IDE_ASSERT( iduMemMgr::free(sPendingOpList) == IDE_SUCCESS );
        default:
            break;
    }

    IDE_POP();

    return IDE_FAILURE;
}


/*
   기능 : 트랜잭션의 테이블스페이스 관련 연산에 대한
   Commit/Rollback Pending Operation을 수행한다.

   [IN] aPendingOp : Pending 아이템
   [IN] aIsCommit  : Commit/Rollback 여부
*/
IDE_RC sctTableSpaceMgr::executePendingOperation( idvSQL  * aStatistics,
                                                   void   * aPendingOp,
                                                  idBool    aIsCommit )
{
    idBool              sDoPending = ID_TRUE;
    UInt                sState     = 0;
    sctTableSpaceNode * sSpaceNode;
    sddDataFileNode   * sFileNode;
    sctPendingOp      * sPendingOp;

    sFileNode  = NULL;
    sSpaceNode = NULL;
    sPendingOp = (sctPendingOp*)aPendingOp;

    IDE_ASSERT( sPendingOp != NULL );

    IDE_TEST_CONT( sPendingOp->mIsCommit != aIsCommit, CONT_SKIP_PENDING );

    // TBS Node가 존재하지 않거나 DROPPED 상태의 경우는 NULL이 반환된다.
    sSpaceNode = findSpaceNodeWithoutException( sPendingOp->mSpaceID );

    IDE_TEST_CONT( sSpaceNode == NULL, CONT_SKIP_PENDING );

    lockSpaceNode( aStatistics,
                   sSpaceNode );
    sState = 1;

    // Memory TBS의 Drop Pending처리는 Pending함수에서 수행한다.
    if ( (isMemTableSpace( sPendingOp->mSpaceID ) == ID_TRUE) ||
         (isVolatileTableSpace( sPendingOp->mSpaceID ) == ID_TRUE) )
    {
        sState = 0;
        unlockSpaceNode( sSpaceNode );

        IDE_CONT( CONT_RUN_PENDING );
    }
    else
    {
        IDE_ERROR( isDiskTableSpace(sPendingOp->mSpaceID )  == ID_TRUE );
    }

    // 불필요한 DIFF를 발생시키지 않게 하기 위해 indending하지 않음
    // 아래 코드는 모두 Disk Tablespace의 Pending함수로 옮겨갈 내용들임
    switch( sPendingOp->mPendingOpType )
    {
        case SCT_POP_CREATE_TBS:
            IDE_ASSERT( SMI_TBS_IS_ONLINE(sSpaceNode->mState) );
            IDE_ASSERT( SMI_TBS_IS_CREATING(sSpaceNode->mState) );

            sSpaceNode->mState &= ~SMI_TBS_CREATING;
            break;

        case SCT_POP_DROP_TBS:
            // lock -> sync lock 요청하는 과정
            // sync lock -> lock 하는 과정은 존재하지 않도록 하여
            // 교착상태가 발생하지 않도록 한다.

            // sync가 진행중일때까지 대기하다가 lock이 획득된다.
            // sync lock을 획득하면 어떠한 DBF Node의 파일도 sync가
            // 진행될 수 없고,
            // sync lock에 대기한다.
            // 파일을 제거하고 sync 하는 연산이 sync lock을
            // 획득하게 되면
            // DROPPED 상태이기때문에 접근하지 않는다.

            // ONLINE/OFFLINE/DISCARD 상태여야 한다.
            IDE_ASSERT( hasState( sSpaceNode,
                                  SCT_SS_HAS_DROP_TABLESPACE ) == ID_TRUE );
            IDE_ASSERT( SMI_TBS_IS_DROPPING(sSpaceNode->mState) );

            sSpaceNode->mState = SMI_TBS_DROPPED;

            break;

        case SCT_POP_ALTER_TBS_ONLINE:
            sddTableSpace::setOnlineTBSLSN4Idx( (sddTableSpaceNode*)sSpaceNode,
                                                &(sPendingOp->mOnlineTBSLSN) );
            break;

        case SCT_POP_ALTER_TBS_OFFLINE:
            IDE_ASSERT( (sSpaceNode->mState & SMI_TBS_DROPPING)
                        != SMI_TBS_DROPPING );
            // do nothing.  Pending 함수에서 모든 처리 실시.
            break;

        case SCT_POP_ALTER_DBF_RESIZE:
            sddTableSpace::getDataFileNodeByIDWithoutException(
                                                (sddTableSpaceNode*)sSpaceNode,
                                                sPendingOp->mFileID,
                                                &sFileNode );

            if ( sFileNode != NULL )
            {
                // Commit Pending 수행에서 제거해준다.
                sFileNode->mState &= ~SMI_FILE_RESIZING;

                // RUNTIME에 Commit 연산에 대해서만 개수를 감소시킨다
                if ( ( smLayerCallback::isRestart() == ID_FALSE ) &&
                     ( aIsCommit == ID_TRUE ) )
                {
                    ((sddTableSpaceNode*)sSpaceNode)->mTotalPageCount
                        += sPendingOp->mResizePageSize;
                }
            }
            else
            {
                sDoPending = ID_FALSE;
            }
            break;

        case SCT_POP_CREATE_DBF:
            sddTableSpace::getDataFileNodeByIDWithoutException(
                                                    (sddTableSpaceNode*)sSpaceNode,
                                                    sPendingOp->mFileID,
                                                    &sFileNode );

            if ( sFileNode != NULL )
            {
                IDE_ASSERT( SMI_FILE_STATE_IS_ONLINE( sFileNode->mState ) );
                IDE_ASSERT( SMI_FILE_STATE_IS_CREATING( sFileNode->mState ) );

                // create tablespace로 인해 생성된 dbf는 SMI_FILE_CREATING상태가 아니며,
                // add datafile로 인해 생성된 dbf는 SMI_FILE_CREATING이여야 한다.

                sFileNode->mState &= ~SMI_FILE_CREATING;

                // RUNTIME에 Commit 연산에 대해서만 개수를 감소시킨다
                if ( ( smLayerCallback::isRestart() == ID_FALSE ) &&
                     ( aIsCommit == ID_TRUE ) )
                {
                    ((sddTableSpaceNode*)sSpaceNode)->mDataFileCount++;
                    ((sddTableSpaceNode*)sSpaceNode)->mTotalPageCount += sFileNode->mCurrSize;
                }
            }
            else
            {
                sDoPending = ID_FALSE;
            }
            break;

        case SCT_POP_DROP_DBF:
            sddTableSpace::getDataFileNodeByIDWithoutException(
                                                    (sddTableSpaceNode*)sSpaceNode,
                                                    sPendingOp->mFileID,
                                                    &sFileNode );

            if ( sFileNode != NULL )
            {
                IDE_ASSERT( SMI_FILE_STATE_IS_ONLINE( sFileNode->mState ) ||
                            SMI_FILE_STATE_IS_OFFLINE( sFileNode->mState ) );

                IDE_ASSERT( SMI_FILE_STATE_IS_DROPPING( sFileNode->mState ) );

                // FIX BUG-13125 DROP TABLESPACE시에 관련 페이지들을
                // Invalid시켜야 한다.
                // DBF 상태변경은 removeFilePending에서 처리한다.

                IDE_ASSERT( sPendingOp->mPendingOpFunc != NULL );

                IDE_ASSERT( (UChar*)(sPendingOp->mPendingOpParam)
                            == (UChar*)sFileNode );

                // RUNTIME에 Commit 연산에 대해서만 개수를 감소시킨다
                if ( ( smLayerCallback::isRestart() == ID_FALSE ) &&
                     ( aIsCommit == ID_TRUE ) )
                {
                    ((sddTableSpaceNode*)sSpaceNode)->mDataFileCount--;
                    ((sddTableSpaceNode*)sSpaceNode)->mTotalPageCount -= sFileNode->mCurrSize;
                }
                else
                {
                    // Restart Recovery 수행중 Pending 수행함.
                }
            }
            else
            {
                sDoPending = ID_FALSE;
            }
            break;

        case SCT_POP_ALTER_DBF_ONLINE:
            sddTableSpace::getDataFileNodeByIDWithoutException(
                                                (sddTableSpaceNode*)sSpaceNode,
                                                sPendingOp->mFileID,
                                                &sFileNode );

            if ( sFileNode != NULL )
            {
                /* BUG-21056: Restart Redo시 DBF를 Online으로 수행시 FileNode가
                 * Online으로 되어있을 수 있습니다.
                 *
                 * 이유: Restart Redo Point가 DBF Online이전으로 잡혀있을 수 있기
                 * 때문. */
                if ( smLayerCallback::isRestart() == ID_FALSE )
                {
                    IDE_ASSERT( SMI_FILE_STATE_IS_OFFLINE( sFileNode->mState ) );
                }

                IDE_ASSERT( sPendingOp->mPendingOpFunc == NULL );

                sFileNode->mState = sPendingOp->mNewDBFState;
            }
            else
            {
                sDoPending = ID_FALSE;
            }
            break;

        case SCT_POP_ALTER_DBF_OFFLINE:

            sddTableSpace::getDataFileNodeByIDWithoutException(
                                                (sddTableSpaceNode*)sSpaceNode,
                                                sPendingOp->mFileID,
                                                &sFileNode );

            if ( sFileNode != NULL )
            {
                IDE_ASSERT( sPendingOp->mPendingOpFunc == NULL );

                sFileNode->mState = sPendingOp->mNewDBFState;
            }
            else
            {
                sDoPending = ID_FALSE;
            }
            break;

        case SCT_POP_UPDATE_SPACECACHE:
            IDE_ASSERT( sPendingOp->mPendingOpFunc  != NULL );
            IDE_ASSERT( sPendingOp->mPendingOpParam != NULL );
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    sState = 0;
    unlockSpaceNode( sSpaceNode );

    // Loganchor 갱신
    switch( sPendingOp->mPendingOpType )
    {
        case SCT_POP_CREATE_TBS:
        case SCT_POP_DROP_TBS:
            /* PROJ-2386 DR
             * DR standby도 active와 동일한 시점에 loganchor를 update한다. */
            if ( ((sddTableSpaceNode*)sSpaceNode)->mAnchorOffset
                 != SCT_UNSAVED_ATTRIBUTE_OFFSET )
            {
                IDE_ASSERT( smLayerCallback::updateTBSNodeAndFlush( sSpaceNode ) == IDE_SUCCESS );
            }
            else
            {
                // create TBS 생성하다 실패한 경우
                // 저장이 안된 경우가 있다.
                // 즉, Create TBS의 Undo 과정에서 등록된
                // Pending(DROP_TBS)에 의해서 저장하지도 않은
                // TBS Node 속성을 update하려다 Loganchor를
                // 깨먹을 수 있다.
                IDE_ASSERT( (sPendingOp->mPendingOpType == SCT_POP_DROP_TBS) &&
                            (aIsCommit == ID_FALSE) );
            }
            break;

        case SCT_POP_DROP_DBF:
            // pending 내에서 loganchor flush
            break;
        case SCT_POP_CREATE_DBF:
        case SCT_POP_ALTER_DBF_RESIZE:
        case SCT_POP_ALTER_DBF_OFFLINE:
        case SCT_POP_ALTER_DBF_ONLINE:
            if ( sFileNode != NULL )
            {
                /* BUG-24086: [SD] Restart시에도 File이나 TBS에 대한 상태가 바뀌었을 경우
                 * LogAnchor에 상태를 반영해야 한다.
                 *
                 * Restart Recovery시에는 updateDBFNodeAndFlush하지 않던것을 하도록 변경.
                 * */
                if ( sFileNode->mAnchorOffset != SCT_UNSAVED_ATTRIBUTE_OFFSET )
                {
                    IDE_ASSERT( smLayerCallback::updateDBFNodeAndFlush( sFileNode ) == IDE_SUCCESS );
                }
                else
                {
                    // create DBF 생성하다 실패한 경우
                    // 저장이 안된 경우가 있다.
                    // 즉, Create DBF의 Undo 과정에서 등록된
                    // Pending(DROP_DBF)에 의해서 저장하지도 않은
                    // DBF Node 속성을 update하려다 Loganchor를
                    // 깨먹을 수 있다.
                    IDE_ASSERT( aIsCommit == ID_FALSE );
                }
            }
            else
            {
                // FileNode가 검색이 안된 경우 Nothing To Do...
                IDE_ASSERT( sDoPending == ID_FALSE );
            }
            break;

        case SCT_POP_ALTER_TBS_ONLINE:
        case SCT_POP_ALTER_TBS_OFFLINE:
        case SCT_POP_UPDATE_SPACECACHE:
            // do nothing.  Pending 함수에서 모든 처리 실시.
            break;

        default:
            IDE_ASSERT(0);
            break;

    }

    // 불필요한 DIFF를 발생시키지 않게 하기 위해 indending하지 않음

    IDE_EXCEPTION_CONT( CONT_RUN_PENDING );

    IDU_FIT_POINT( "2.PROJ-1548@sctTableSpaceMgr::executePendingOperation" );

    if ( ( sDoPending == ID_TRUE ) &&
         ( sPendingOp->mPendingOpFunc != NULL ) )
    {
        // 등록되어 있는 Pending Operation을 수행한다.
        IDE_TEST( (*sPendingOp->mPendingOpFunc) ( aStatistics,
                                                  sSpaceNode,
                                                  sPendingOp )
                  != IDE_SUCCESS );
    }
    else
    {
        // 등록된 Pending Operation이 NULL 인경우
    }

    IDE_EXCEPTION_CONT( CONT_SKIP_PENDING );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        unlockSpaceNode( sSpaceNode );
    }

    return IDE_FAILURE;
}

/*
    각각의 Tablespace에 대해 특정 Action을 수행한다.

    aAction    [IN] 수행할 Action함수
    aActionArg [IN] Action함수에 보낼 Argument
    aFilter    [IN] Action수행 여부를 결정하는 Filter

    - 사용예제
      smmManager::restoreTBS
 */
IDE_RC sctTableSpaceMgr::doAction4EachTBS( idvSQL            * aStatistics,
                                           sctAction4TBS       aAction,
                                           void              * aActionArg,
                                           sctActionExecMode   aActionExecMode )
{
    UInt                sState = 0;
    UInt                sSpaceID;
    sctTableSpaceNode * sCurTBS;

    for( sSpaceID = 0 ; sSpaceID < mNewTableSpaceID ; sSpaceID++ )
    {
        sCurTBS = mSpaceNodeArray[sSpaceID];

        if ( sCurTBS != NULL )
        {
            if ( aActionExecMode == SCT_ACT_MODE_LATCH )
            {
                lockSpaceNode( aStatistics,
                               sCurTBS );
                sState = 1;
            }

            if ( SMI_TBS_IS_NOT_DROPPED( sCurTBS->mState ) )
            {
                IDE_TEST( (*aAction)( aStatistics,
                                      sCurTBS,
                                      aActionArg )
                          != IDE_SUCCESS );
            }
            
            if ( sState == 1 )
            {
                sState = 0;
                unlockSpaceNode( sCurTBS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        unlockSpaceNode( sCurTBS );
    }

    return IDE_FAILURE;
}

/*
 * BUG-34187 윈도우 환경에서 슬러시와 역슬러시를 혼용해서 사용 불가능 합니다.
 */
#if defined(VC_WIN32)
void sctTableSpaceMgr::adjustFileSeparator( SChar * aPath )

{
    UInt  i;
    for ( i = 0 ; (aPath[i] != '\0') && (i < SMI_MAX_CHKPT_PATH_NAME_LEN) ; i++ )
    {
        if ( aPath[i] == '/' )
        {
            aPath[i] = IDL_FILE_SEPARATOR;
        }
        else
        {
            /* nothing to do */
        }
    }
}
#endif

/*
 * 기능 datafile path 명 validataion 확인 및 절대경로 생성
 *
 * 상대경로 입력시 default db dir을 path에 추가하여 절대경로를 만든다.
 * 또한, datafile path가 유효한지 검사하고
 * datafile 명이 시스템예약어인 "system", "temp", "undo"로 시작하면 안된다.
 * sdsFile에 동일 기능의 함수가 존재 합니다.  수정내용발생시 적용 필요합니다.
 *
 * + 2nd. code design
 * - filename에 특수문자가 존재하는지 검사한다.
 * - 시스템 예약어를 datafile 이름으로 사용하는지 검사한다.
 *   for( system keyword 개수만큼 )
 *   {
 *      if( 화일 이름의 prefix가 정의된 것이다)
 *      {
 *          return falure;
 *      }
 *   }
 * - 상대경로라면 절대경로로 변경하여 저장한다.
 * - 절대경로에 대해서 file명을 제외한 dir이 존재하는지 확인한다.
 *
 * [IN]     aCheckPerm   : 파일 권한 검사 여부
 * [IN/OUT] aValidName   : 상대경로를 받아서 절대경로로 변경하여 반환
 * [OUT]    aNameLength  : 절대경로의 길이
 * [IN]     aTBSLocation : 테이블 스페이스의 종류[SMI_TBS_MEMORY | SMI_TBS_DISK]
 */
IDE_RC sctTableSpaceMgr::makeValidABSPath( idBool         aCheckPerm,
                                           SChar*         aValidName,
                                           UInt*          aNameLength,
                                           smiTBSLocation aTBSLocation )
{
#if !defined(VC_WINCE) && !defined(SYMBIAN)
    UInt    i;
    SChar*  sPtr;

    DIR*    sDir;
    UInt    sDirLength;

    SChar   sFilePath[SM_MAX_FILE_NAME + 1];
    SChar   sChkptPath[SMI_MAX_CHKPT_PATH_NAME_LEN + 1];

    SChar*  sPath = NULL;

    IDE_ASSERT( aValidName  != NULL );
    IDE_ASSERT( aNameLength != NULL );
    IDE_DASSERT( idlOS::strlen(aValidName) == *aNameLength );

    // BUG-29812
    // aTBSLocation은 SMI_TBS_MEMORY와 SMI_TBS_DISK이어야 한다.
    IDE_ASSERT( (aTBSLocation == SMI_TBS_MEMORY) ||
                (aTBSLocation == SMI_TBS_DISK) );

    // fix BUG-15502
    IDE_TEST_RAISE( idlOS::strlen(aValidName) == 0,
                    error_filename_is_null_string );

    if ( aTBSLocation == SMI_TBS_MEMORY )
    {
        sPath = sChkptPath;
    }
    else
    {
        sPath = sFilePath;
    }

    /* ------------------------------------------------
     * datafile 이름에 대한 시스템 예약어 검사
     * ----------------------------------------------*/
#if defined(VC_WIN32)
    SInt  sIterator;
    for ( sIterator = 0 ; aValidName[sIterator] != '\0' ; sIterator++ ) 
    {
        if ( aValidName[sIterator] == '/' ) 
        {
             aValidName[sIterator] = IDL_FILE_SEPARATOR;
        }
        else
        {
            /* nothing to do */
        }
    }
#endif

    sPtr = idlOS::strrchr(aValidName, IDL_FILE_SEPARATOR);
    if ( sPtr == NULL )
    {
        sPtr = aValidName; // datafile 명만 존재
    }
    else
    {
        // Do Nothing...
    }

    sPtr = idlOS::strchr(aValidName, IDL_FILE_SEPARATOR);
#ifndef VC_WIN32
    if ( sPtr != &aValidName[0] )
#else
    /* BUG-38278 invalid datafile path at windows server
     * 윈도우즈 환경에서 '/' 나 '\' 로 시작되는
     * 경로 입력은 오류로 처리한다. */
    IDE_TEST_RAISE( sPtr == &aValidName[0], error_invalid_filepath_abs );

    if ( ( (aValidName[1] == ':') && (sPtr != &aValidName[2]) ) ||
         ( (aValidName[1] != ':') && (sPtr != &aValidName[0]) ) )
#endif
    {
        /* ------------------------------------------------
         * 상대경로(relative-path)인 경우
         * Disk TBS이면 default disk db dir을
         * Memory TBS이면 home dir ($ALTIBASE_HOME)을
         * 붙여서 절대경로(absolute-path)로 만든다.
         * ----------------------------------------------*/
        if ( aTBSLocation == SMI_TBS_MEMORY )
        {
            sDirLength = idlOS::strlen(aValidName) +
                         idlOS::strlen(idp::getHomeDir());

            IDE_TEST_RAISE( (sDirLength + 1) > SMI_MAX_CHKPT_PATH_NAME_LEN,
                            error_too_long_chkptpath );

            idlOS::snprintf( sPath, 
                             SMI_MAX_CHKPT_PATH_NAME_LEN,
                             "%s%c%s",
                             idp::getHomeDir(),
                             IDL_FILE_SEPARATOR,
                             aValidName );
        }
        else if ( aTBSLocation == SMI_TBS_DISK )
        {
            sDirLength = idlOS::strlen(aValidName) +
                         idlOS::strlen(smuProperty::getDefaultDiskDBDir());

            IDE_TEST_RAISE( (sDirLength + 1) > SM_MAX_FILE_NAME,
                            error_too_long_filepath );

            idlOS::snprintf( sPath, 
                             SM_MAX_FILE_NAME,
                             "%s%c%s",
                             smuProperty::getDefaultDiskDBDir(),
                             IDL_FILE_SEPARATOR,
                             aValidName );
        }

#if defined(VC_WIN32)
    for ( sIterator = 0 ; sPath[sIterator] != '\0' ; sIterator++ ) 
    {
        if ( sPath[sIterator] == '/' ) 
        {
             sPath[sIterator] = IDL_FILE_SEPARATOR;
        }
        else
        {
            /* nothing to do */
        }
    }
#endif
        idlOS::strcpy(aValidName, sPath);
        *aNameLength = idlOS::strlen(aValidName);

        sPtr = idlOS::strchr(aValidName, IDL_FILE_SEPARATOR);
#ifndef VC_WIN32
        IDE_TEST_RAISE( sPtr != &aValidName[0], error_invalid_filepath_abs );
#else
        IDE_TEST_RAISE( ((aValidName[1] == ':') && (sPtr != &aValidName[2])) ||
                        ((aValidName[1] != ':') && (sPtr != &aValidName[0])), 
                        error_invalid_filepath_abs );
#endif
    }

    /* ------------------------------------------------
     * 영문자, 숫자 + '/'는 허용하고 그외 문자는 허용하지 않는다.
     * (절대경로임)
     * ----------------------------------------------*/
    for ( i = 0 ; i < *aNameLength ; i++ )
    {
        if ( smuUtility::isAlNum(aValidName[i]) != ID_TRUE )
        {
            /* BUG-16283: Windows에서 Altibase Home이 '(', ')' 가 들어갈
               경우 DB 생성시 오류가 발생합니다. */
            IDE_TEST_RAISE( (aValidName[i] != IDL_FILE_SEPARATOR) &&
                            (aValidName[i] != '-') &&
                            (aValidName[i] != '_') &&
                            (aValidName[i] != '.') &&
                            (aValidName[i] != ':') &&
                            (aValidName[i] != '(') &&
                            (aValidName[i] != ')') &&
                            (aValidName[i] != ' ')
                            ,
                            error_invalid_filepath_keyword );

            if ( aValidName[i] == '.' )
            {
                if ( (i + 1) != *aNameLength )
                {
                    IDE_TEST_RAISE( aValidName[i+1] == '.',
                                    error_invalid_filepath_keyword );
                    IDE_TEST_RAISE( aValidName[i+1] == IDL_FILE_SEPARATOR,
                                    error_invalid_filepath_keyword );
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
    } // end of for

    // [BUG-29812] IDL_FILE_SEPARATOR가 한개도 없다면 절대경로가 아니다.
   IDE_TEST_RAISE( (sPtr = idlOS::strrchr(aValidName, IDL_FILE_SEPARATOR))
                   == NULL,
                   error_invalid_filepath_abs );

    // [BUG-29812] dir이 존재하는 확인한다.
    if ( (aCheckPerm == ID_TRUE) && (aTBSLocation == SMI_TBS_DISK) )
    {
        idlOS::strncpy( sPath, aValidName, SM_MAX_FILE_NAME );

        sDirLength = sPtr - aValidName;

        sDirLength = ( sDirLength == 0 ) ? 1 : sDirLength;
        sPath[sDirLength] = '\0';

        // fix BUG-19977
        IDE_TEST_RAISE( idf::access(sPath, F_OK) != 0,
                        error_not_exist_path );

        IDE_TEST_RAISE( idf::access(sPath, R_OK) != 0,
                        error_no_read_perm_path );
        IDE_TEST_RAISE( idf::access(sPath, W_OK) != 0,
                        error_no_write_perm_path );
        IDE_TEST_RAISE( idf::access(sPath, X_OK) != 0,
                        error_no_execute_perm_path );

        sDir = idf::opendir(sPath);
        IDE_TEST_RAISE( sDir == NULL, error_open_dir ); /* BUGBUG - ERROR MSG */

        (void)idf::closedir(sDir);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_filename_is_null_string );
    {
        if ( aTBSLocation == SMI_TBS_DISK )
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_FileNameIsNullString));
        }
        else
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_CheckpointPathIsNullString));
        }
    }
    IDE_EXCEPTION( error_not_exist_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExistPath, sPath));
    }
    IDE_EXCEPTION( error_no_read_perm_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoReadPermFile, sPath));
    }
    IDE_EXCEPTION( error_no_write_perm_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoWritePermFile, sPath));
    }
    IDE_EXCEPTION( error_no_execute_perm_path );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NoExecutePermFile, sPath));
    }
    IDE_EXCEPTION( error_invalid_filepath_abs );
    {
        if ( aTBSLocation == SMI_TBS_DISK )
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathABS));
        }
        else
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidCheckpointPathABS));
        }
    }
    IDE_EXCEPTION( error_invalid_filepath_keyword );
    {
        if ( aTBSLocation == SMI_TBS_DISK )
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathKeyWord));
        }
        else
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidCheckpointPathKeyWord));
        }
    }
    IDE_EXCEPTION( error_open_dir );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION( error_too_long_chkptpath );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TooLongCheckpointPath,
                                sPath,
                                idp::getHomeDir()));
    }
    IDE_EXCEPTION( error_too_long_filepath );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TooLongFilePath,
                                sPath,
                                smuProperty::getDefaultDiskDBDir()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
#else
    // Windows CE에서는 파일의 절대경로가 C:로 시작하지 않는다.
    return IDE_SUCCESS;
#endif
}

/* BUG-38621 
 * - 절대경로를 상대경로로 변환 
 * - makeValidABSPath(4)를 참조해서 만듬.
 *
 * [IN/OUT] aName        : 절대경로를 받아서 상대경로로 변경하여 반환
 * [OUT]    aNameLength  : 상대경로의 길이
 * [IN]     aTBSLocation : 테이블 스페이스의 종류[SMI_TBS_MEMORY | SMI_TBS_DISK]
 */
IDE_RC sctTableSpaceMgr::makeRELPath( SChar         * aName,
                                      UInt          * aNameLength,
                                      smiTBSLocation  aTBSLocation )
{
#if !defined(VC_WINCE) && !defined(SYMBIAN)
    SChar   sDefaultPath[SM_MAX_FILE_NAME + 1]          = {0, };
    SChar   sChkptPath[SMI_MAX_CHKPT_PATH_NAME_LEN + 1] = {0, };
    SChar   sFilePath[SM_MAX_FILE_NAME]                 = {0, };
    SChar*  sPath;
    UInt    sPathSize;

    IDE_ASSERT( aName != NULL );
    IDE_ASSERT( (aTBSLocation == SMI_TBS_MEMORY) ||
                (aTBSLocation == SMI_TBS_DISK) );

    IDE_ASSERT( smuProperty::getRELPathInLog() == ID_TRUE );

    IDE_TEST_RAISE( idlOS::strlen( aName ) == 0,
                    error_filename_is_null_string );

    if ( aTBSLocation == SMI_TBS_MEMORY )
    {
        idlOS::strncpy( sDefaultPath,
                        idp::getHomeDir(),
                        ID_SIZEOF( sDefaultPath ) - 1 );
        sPath           = sChkptPath;
        sPathSize       = ID_SIZEOF( sChkptPath );
    }
    else
    {
        idlOS::strncpy( sDefaultPath,
                        (SChar *)smuProperty::getDefaultDiskDBDir(),
                        ID_SIZEOF( sDefaultPath ) - 1 );
        sPath           = sFilePath;
        sPathSize       = ID_SIZEOF( sFilePath );
    }

#if defined(VC_WIN32)
    SInt  sIterator;
    for ( sIterator = 0 ; aName[sIterator] != '\0' ; sIterator++ ) 
    {
        if ( aName[sIterator] == '/' ) 
        {
             aName[sIterator] = IDL_FILE_SEPARATOR;
        }
    }
    for ( sIterator = 0; sDefaultPath[sIterator] != '\0'; sIterator++ ) 
    {
        if ( sDefaultPath[sIterator] == '/' ) 
        {
             sDefaultPath[sIterator] = IDL_FILE_SEPARATOR;
        }
    }
#endif

    IDE_TEST_RAISE ( idlOS::strncmp( aName,
                                     sDefaultPath,
                                     idlOS::strlen( sDefaultPath ) )
                     != 0,
                     error_invalid_filepath_abs );

    if ( idlOS::strlen( aName ) > idlOS::strlen( sDefaultPath ) )
    {
        IDE_TEST_RAISE( *(aName + idlOS::strlen( sDefaultPath ))
                        != IDL_FILE_SEPARATOR,
                        error_invalid_filepath_abs );

        idlOS::snprintf( sPath,
                         sPathSize,
                         "%s",
                         ( aName + idlOS::strlen( sDefaultPath ) + 1 ) );
    }
    else
    {
        if ( idlOS::strlen( aName ) == idlOS::strlen( sDefaultPath ) )
        {
            *sPath = '\0';
        }
        else
        {
            /* cannot access here */
            IDE_ASSERT( 0 );
        }
    }

    idlOS::strcpy( aName, sPath );

    if ( aNameLength != NULL )
    {
        *aNameLength = idlOS::strlen( aName );
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_filename_is_null_string );
    {
        if ( aTBSLocation == SMI_TBS_DISK )
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_FileNameIsNullString ) );
        }
        else
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_CheckpointPathIsNullString ) );
        }
    }
    IDE_EXCEPTION( error_invalid_filepath_abs );
    {
        if ( aTBSLocation == SMI_TBS_DISK )
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidFilePathABS));
        }
        else
        {
            IDE_SET(ideSetErrorCode(smERR_ABORT_InvalidCheckpointPathABS));
        }
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
#else
    // Windows CE에서는 파일의 절대경로가 C:로 시작하지 않는다.
    return IDE_SUCCESS;
#endif
}

/*
 [  메모리/디스크 공통 ]

 백업 진행을 위해 테이블스페이스의 상태를 백업상태로 설정한다.

 [ 알고리즘 ]
 1. Mgr Latch를 잡는다.
 2. 주어진 테이블 스펭이스 ID로 table space node 를 찾는다.
 3. 테이블 스페이스 상태를  backup으로 변경한다.
 4. 테이블 스페이스 첫번째 데이타 파일의 상태를 backup begin으로 변경한다.
 5. 디스크 테이블스페이스의 경우 MIN PI 노드를 추가한다.
 5. Mgr Latch를 푼다.

 [ 인자 ]
 [IN]  aStatistics : 통계정보
 [IN]  aSpaceNode  : 백업을 진행할 테이블스페이스
*/
IDE_RC sctTableSpaceMgr::startTableSpaceBackup( idvSQL            * aStatistics,
                                                sctTableSpaceNode * aSpaceNode )
{
    idBool sIsLocked = ID_FALSE;

    IDE_DASSERT( aSpaceNode != NULL );

    // temp table space는 백업받을 필요가 없다.
    IDE_TEST_RAISE( (aSpaceNode->mType == SMI_DISK_SYSTEM_TEMP) ||
                    (aSpaceNode->mType == SMI_DISK_USER_TEMP),
                    error_dont_need_backup_tempTableSpace);
  retry:
    lockSpaceNode( aStatistics,
                   aSpaceNode );
    sIsLocked = ID_TRUE;

    IDE_TEST_RAISE( SMI_TBS_IS_BACKUP( aSpaceNode->mState ),
                    error_already_backup_begin );

    if ( ( aSpaceNode->mState & SMI_TBS_BLOCK_BACKUP ) ==
         SMI_TBS_BLOCK_BACKUP )
    {
        sIsLocked = ID_FALSE;
        unlockSpaceNode( aSpaceNode );

        idlOS::sleep(1);

        goto retry;
    }

    // BACKUP 상태 설정
    aSpaceNode->mState |= SMI_TBS_BACKUP;

    sIsLocked = ID_FALSE;
    unlockSpaceNode( aSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_dont_need_backup_tempTableSpace );
    {
        IDE_SET( ideSetErrorCode(smERR_ABORT_DontNeedBackupTempTBS) );
    }
    IDE_EXCEPTION( error_already_backup_begin );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_AlreadyBeginBackup,
                                  aSpaceNode->mID ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        unlockSpaceNode( aSpaceNode );
    }

    return IDE_FAILURE;
}


/*
  [ 메모리/디스크 공통 ]
  ALTER TABLESPACE END BACKUP.. 구문 수행시 테이블스페이스의 백업상태를
  해제한다.

  디스크 테이블스페이스의 경우 MIN PI 노드도 함께 제거한다.(참고 BUG-15003)
  [IN] aStatistics : 통계정보
  [IN] aSpaceNode  : 백업상태를 해제하고자하는 테이블스페이스

*/
IDE_RC sctTableSpaceMgr::endTableSpaceBackup( idvSQL            * aStatistics,
                                              sctTableSpaceNode * aSpaceNode )
{
    idBool sIsLocked = ID_FALSE;

    lockSpaceNode( aStatistics,
                   aSpaceNode );
    sIsLocked = ID_TRUE;

    // alter tablespace  backup begin A를 하고나서,
    // alter tablespace  backup end B를 하는 경우를 막기위함이다.
    IDE_TEST_RAISE( SMI_TBS_IS_NOT_BACKUP( aSpaceNode->mState ),
                    error_not_begin_backup );

    aSpaceNode->mState &= ~SMI_TBS_BACKUP;

    wakeup4Backup( aSpaceNode );

    sIsLocked = ID_FALSE;
    unlockSpaceNode( aSpaceNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( error_not_begin_backup);
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_NotBeginBackup,
                                  aSpaceNode->mID ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        unlockSpaceNode( aSpaceNode );
    }

    return IDE_FAILURE;
}


/*
   데이타파일 헤더에 기록할 체크포인트 정보를 설정한다.

   [IN] aDiskRedoLSN   : 체크포인트과정에서 결정된 디스크 Redo LSN
   [IN] aMemRedoLSN : 메모리 테이블스페이스의 Redo LSN 배열
*/
void sctTableSpaceMgr::setRedoLSN4DBFileMetaHdr( smLSN* aDiskRedoLSN,
                                                 smLSN* aMemRedoLSN )
{
    if ( aDiskRedoLSN != NULL  )
    {
        // Disk Redo LSN을 설정한다.
        SM_GET_LSN( mDiskRedoLSN, *aDiskRedoLSN);
    }

    if ( aMemRedoLSN != NULL  )
    {
        IDE_ASSERT( (aMemRedoLSN->mFileNo != ID_UINT_MAX) &&
                    (aMemRedoLSN->mOffset != ID_UINT_MAX) );

        //  메모리 테이블스페이스의 Redo LSN을 설정한다.
        SM_GET_LSN( mMemRedoLSN, *aMemRedoLSN );
    }
    else
    {
        /* nothing to do ... */
    }

    return;
}

/*
    Alter Tablespace Online/Offline에 대한 에러처리를 수행한다.

    [IN] aTBSNode  - Alter Online/Offline하려는 Tablespace
    [IN] aNewTBSState - 새로 전이하려는 상태 ( Online or Offline )

    [ 선결조건 ]
      Tablespace에 X Lock을 잡은 상태에서 호출되어야 한다.
      이유 : Tablespace에 X Lock을 잡은 상태에서
              다른 DDL이 함께 동작하지 않음을 보장해야
              상태 체크가 유효하다.

    [ 알고리즘 ]
      (e-010) system tablespace 이면 에러
      (e-020) Tablespace의 상태가 이미 새로운 상태이면 에러

    [ 참고 ]
      DROP/DISCARD/OFFLINE된 Tablespace의 경우 lockTBSNode에서
      에러처리된 상태이다.
 */
IDE_RC sctTableSpaceMgr::checkError4AlterStatus( sctTableSpaceNode  * aTBSNode,
                                                 smiTableSpaceState   aNewTBSState )
{
    IDE_DASSERT( aTBSNode != NULL );

    // 테이블스페이스에 잠금을 획득하고 상태를 보기 때문에
    // 다른 트랜잭션에 의해 테이블스페이스 상태가 변경되지 않는다.
    // latch 획득이 필요없음.

    ///////////////////////////////////////////////////////////////////////////
    // (e-010) system tablespace 이면 에러
    IDE_TEST_RAISE( aTBSNode->mID <= SMI_ID_TABLESPACE_SYSTEM_DISK_TEMP,
                    error_alter_status_of_system_tablespace);

    ///////////////////////////////////////////////////////////////////////////
    // (e-020) Tablespace의 상태가 이미 새로운 상태이면 에러
    //         Ex> 이미 ONLINE상태인데 ONLINE상태로 전이하려고 하면 에러
    switch( aNewTBSState )
    {
        case SMI_TBS_ONLINE :
            IDE_TEST_RAISE( SMI_TBS_IS_ONLINE(aTBSNode->mState),
                            error_tbs_is_already_online );
            break;

        case SMI_TBS_OFFLINE :
            IDE_TEST_RAISE( SMI_TBS_IS_OFFLINE(aTBSNode->mState),
                            error_tbs_is_already_offline );
            break;

        default:
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(error_alter_status_of_system_tablespace);
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_CANNOT_ALTER_STATUS_OF_SYSTEM_TABLESPACE));
    }
    IDE_EXCEPTION(error_tbs_is_already_online);
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_TABLESPACE_IS_ALREADY_ONLINE));
    }
    IDE_EXCEPTION(error_tbs_is_already_offline);
    {
        IDE_SET(ideSetErrorCode(
                    smERR_ABORT_TABLESPACE_IS_ALREADY_OFFLINE));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*  Tablespace에 대해 진행중인 Backup이 완료되기를 기다린 후,
    Tablespace를 백업 불가능한 상태로 변경한다.

    [IN] aTBSNode           - 상태를 변경할 Tablespace의 Node
    [IN] aTBSSwitchingState - 상태전이 플래그
                              (SMI_TBS_SWITCHING_TO_OFFLINE,
                               SMI_TBS_SWITCHING_TO_ONLINE )
 */
IDE_RC sctTableSpaceMgr::wait4BackupAndBlockBackup( idvSQL            * aStatistics,
                                                    sctTableSpaceNode * aTBSNode,
                                                    smiTableSpaceState  aTBSSwitchingState )
{
    idBool sIsLocked = ID_FALSE;

    IDE_DASSERT( aTBSNode != NULL );

    lockSpaceNode( aStatistics,
                   aTBSNode );
    sIsLocked = ID_TRUE;

    while ( SMI_TBS_IS_BACKUP( aTBSNode->mState ) )
    {
        wait4Backup( aTBSNode );

        IDE_TEST( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS );
    }

    // Backup이 진행중이지 않음
    // Backup이 들어올 수 없도록 Tablespace의 상태를 변경

    // 새로 Oring하여 Add할 상태는 Tablespace Backup을
    // Blocking하는 비트가 켜져있어야 함
    IDE_ASSERT( ( aTBSSwitchingState & SMI_TBS_BLOCK_BACKUP )
                == SMI_TBS_BLOCK_BACKUP );

    aTBSNode->mState |= aTBSSwitchingState ;

    sIsLocked = ID_FALSE;
    unlockSpaceNode( aTBSNode );

    // 상황 종료

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsLocked == ID_TRUE )
    {
        unlockSpaceNode( aTBSNode );
    }

    return IDE_FAILURE;
}

/*  Table DDL(Create, Drop, Alter )를 수행하는 Transaction이 Commit할때
    tablespace의 mMaxTblDDLCommitSCN으로 변경한다.

    [IN] aSpaceID      - 상태를 변경할 Tablespace의 Node
    [IN] aCommitSCN    - Table에 DDL을 수행하는 Transaction의
                         CommitSCN
 */
void sctTableSpaceMgr::updateTblDDLCommitSCN( scSpaceID aSpaceID,
                                              smSCN     aCommitSCN)
{
    sctTableSpaceNode *sSpaceNode;

    IDE_ASSERT( findAndLockSpaceNodeBySpaceID( NULL,
                                               aSpaceID,
                                               (void**)&sSpaceNode )
                == IDE_SUCCESS );

    /* 이전에 설정된 SCN이 크다면 그래로 둔다. 항상 CrtTblCommitSCN은
     * 증가만 한다. */
    if ( SM_SCN_IS_LT( &( sSpaceNode->mMaxTblDDLCommitSCN ),
                       &(aCommitSCN) ) )
    {
        SM_SET_SCN( &( sSpaceNode->mMaxTblDDLCommitSCN ),
                    &aCommitSCN );
    }

    unlockSpaceNode( sSpaceNode );
}

/*  TableSpace에 대해서 Drop Tablespace를 수행하는 Transaction은
    자신의 ViewSCN을 따고 LockTable을 하는 사이에 다른 Transaction이
    이 Tablespace에 대해서 CreateTable, Drop Table을 했는지를
    검사해야한다.

    [IN] aSpaceID - 상태를 변경할 Tablespace의 Node
    [IN] aViewSCN - Tablespace에 대해서 DDL을 수행하는 Transaction의
                    ViewSCN
 */
IDE_RC sctTableSpaceMgr::canDropByViewSCN( scSpaceID aSpaceID,
                                           smSCN     aViewSCN )
{
    sctTableSpaceNode *sSpaceNode;

    IDE_ASSERT( findSpaceNodeBySpaceID( aSpaceID ,
                                        (void**)&sSpaceNode )
                == IDE_SUCCESS );

    /* ViewSCN이 MaxTblDDLCommitSCN보다 작다는 것은
       ViewSCN을 따고 Lock Tablespace를 하는 사이에 이 Tablespace에
       대해서 Create Table, Drop Table, Alter Table과 같은 연산이
       발생한 것이다. */
    if ( SM_SCN_IS_LT( &( aViewSCN ),
                       &( sSpaceNode->mMaxTblDDLCommitSCN ) ) )
    {
        IDE_RAISE( err_modified );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_modified );
    {
        IDE_SET( ideSetErrorCode( smERR_REBUILD_smiTBSModified ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 모든 디스크 Tablesapce의 Tablespace에 속해
 *               있는 모든 Datafile의 Max Open FD Count를 aMaxFDCnt4File
 *               로 변경한다.
 *
 * aMaxFDCnt4File - [IN] Max FD Count
 **********************************************************************/
IDE_RC sctTableSpaceMgr::setMaxFDCntAllDFileOfAllDiskTBS( UInt aMaxFDCnt4File )
{
    UInt               i;
    sctTableSpaceNode *sSpaceNode;

    for ( i = 0 ; i < mNewTableSpaceID ; i++ )
    {
        sSpaceNode = mSpaceNodeArray[i];

        if ( sSpaceNode == NULL )
        {
            continue;
        }

        if ( isDiskTableSpace( sSpaceNode ) == ID_TRUE )
        {
            /* 각 Tablespace별로 Max Open FD Count를 변경한다. */
            IDE_TEST( sddDiskMgr::setMaxFDCnt4AllDFileOfTBS( sSpaceNode,
                                                             aMaxFDCnt4File )
                  != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   PROJ-1548
   DDL_LOCK_TIMEOUT 프로퍼티에 따라 대기시간을 반환한다.
*/
ULong sctTableSpaceMgr::getDDLLockTimeOut( smxTrans * aTrans )
{
    SInt sDDLLockTimeout = 0;
    if ( aTrans != NULL )
    {
        if ( aTrans->mStatistics != NULL )
        {
            IDE_DASSERT(aTrans->mStatistics->mSess != NULL);
            if( aTrans->mStatistics->mSess->mSession != NULL )
            {
                sDDLLockTimeout = gSmiGlobalCallBackList.getDDLLockTimeout(aTrans->mStatistics->mSess->mSession);
            }
        }
    }
    else 
    {
        sDDLLockTimeout = smuProperty::getDDLLockTimeOut();
    }
    return (((sDDLLockTimeout == -1) ?
             ID_ULONG_MAX :
             sDDLLockTimeout *1000000) );
}

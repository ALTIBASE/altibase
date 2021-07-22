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
 * $Id: smcTable.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smm.h>
#include <smr.h>
#include <sdr.h>
#include <sdb.h>
#include <smp.h>
#include <svp.h>
#include <sdd.h>
#include <sdp.h>
#include <sdc.h>
#include <smcDef.h>
#include <smcTable.h>
#include <smiTableBackup.h>
#include <smiVolTableBackup.h>
#include <smcReq.h>
#include <sct.h>
#include <smx.h>

#define MAX_CHECK_CNT_FOR_SESSION (10)

extern smiGlobalCallBackList gSmiGlobalCallBackList;

iduMemPool smcTable::mDropIdxPagePool;
SChar      smcTable::mBackupDir[SM_MAX_FILE_NAME];
UInt       smcTable::mTotalIndex;

scSpaceID smcTable::getSpaceID(const void *aTableHeader)
{
    return ((smcTableHeader*)aTableHeader)->mSpaceID;
}

/***********************************************************************
 * Description : smcTable 전역 초기화
 ***********************************************************************/
IDE_RC smcTable::initialize()
{
    mTotalIndex = 0;

    idlOS::snprintf(mBackupDir,
                    SM_MAX_FILE_NAME,
                    "%s%c",
                    smuProperty::getDBDir(0), IDL_FILE_SEPARATOR);

    IDE_TEST(smcCatalogTable::initialize() != IDE_SUCCESS);

    // Drop Index Pool 초기화
    IDE_TEST(mDropIdxPagePool.initialize(IDU_MEM_SM_SMC,
                                         (SChar*)"DROP_INDEX_MEMORY_POOL",
                                         1,
                                         SM_PAGE_SIZE,
                                         SMC_DROP_INDEX_LIST_POOL_SIZE,
                                         IDU_AUTOFREE_CHUNK_LIMIT,			/* ChunkLimit */
                                         ID_TRUE,							/* UseMutex */
                                         IDU_MEM_POOL_DEFAULT_ALIGN_SIZE,	/* AlignByte */
                                         ID_FALSE,							/* ForcePooling */
                                         ID_TRUE,							/* GarbageCollection */
                                         ID_TRUE,                           /* HWCacheLine */
                                         IDU_MEMPOOL_TYPE_LEGACY            /* mempool type*/) 
             != IDE_SUCCESS);			

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : smcTable 전역 해제
 ***********************************************************************/
IDE_RC smcTable::destroy()
{
    IDE_TEST( smcCatalogTable::destroy() != IDE_SUCCESS );
    IDE_TEST( mDropIdxPagePool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Table의 Lock Item을 초기화하고 Runtime Item들을 NULL로 세팅한다.

    Server Startup시 DISCARD/OFFLINE Tablespace에 속한
    Table들에 대해 수행된다.

    [IN] aHeader - 초기화할 Table의 Header

    [ Lock Item 초기화 ]
      DISCARD된 Tablespace에 속한 하나의 Table에
      두 개 이상의 Transaction이 동시에 DROP을 수행할 경우
      동시성 제어를 위해 Lock이 필요하다.
 */
IDE_RC smcTable::initLockAndSetRuntimeNull( smcTableHeader* aHeader )
{
    // lock item, mSync 초기화
    IDE_TEST( initLockItem( aHeader ) != IDE_SUCCESS );

    // Runtime Item들의 Pointer들을 NULL로 초기화
    IDE_TEST( setRuntimeNull( aHeader ) != IDE_SUCCESS );

    // drop된 index header의 list 초기화
    aHeader->mDropIndexLst = NULL;
    aHeader->mDropIndex    = 0;

    // for qcmTableInfo
    // PROJ-1597
    aHeader->mRuntimeInfo  = NULL;

    /* BUG-42928 No Partition Lock */
    aHeader->mReplacementLock = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Table Header의 Runtime Item들을 NULL로 초기화한다.

    Server Startup시 DISCARD/OFFLINE Tablespace에 속한
    Table들에 대해 수행된다.

    [IN] aHeader - Table의 Header
*/
IDE_RC smcTable::setRuntimeNull( smcTableHeader* aHeader )
{
    UInt  sTableType;

    sTableType = SMI_GET_TABLE_TYPE( aHeader );

    aHeader->mDropIndexLst = NULL;
    aHeader->mDropIndex    = 0;
    aHeader->mRuntimeInfo  = NULL;

    /* BUG-42928 No Partition Lock */
    aHeader->mReplacementLock = NULL;

    if (sTableType == SMI_TABLE_MEMORY || sTableType == SMI_TABLE_META)
    {
        // Memory Table의 초기화
        IDE_TEST( smpAllocPageList::setRuntimeNull(
                      SM_MAX_PAGELIST_COUNT,
                      aHeader->mFixedAllocList.mMRDB )
                  != IDE_SUCCESS );

        IDE_TEST( smpAllocPageList::setRuntimeNull(
                      SM_MAX_PAGELIST_COUNT,
                      aHeader->mVarAllocList.mMRDB )
                  != IDE_SUCCESS );

        IDE_TEST( smpFixedPageList::setRuntimeNull(
                      & aHeader->mFixed.mMRDB )
                  != IDE_SUCCESS );

        IDE_TEST( smpVarPageList::setRuntimeNull(
                      SM_VAR_PAGE_LIST_COUNT,
                      aHeader->mVar.mMRDB )
                  != IDE_SUCCESS );
    }
    else if ( sTableType == SMI_TABLE_DISK )
    {
        IDE_TEST( sdpPageList::setRuntimeNull( & aHeader->mFixed.mDRDB )
                                               != IDE_SUCCESS);
    }
    else if (sTableType == SMI_TABLE_VOLATILE)
    {
        // Volatile Table의 초기화
        IDE_TEST( svpAllocPageList::setRuntimeNull(
                      SM_MAX_PAGELIST_COUNT,
                      aHeader->mFixedAllocList.mVRDB )
                  != IDE_SUCCESS );

        IDE_TEST( svpAllocPageList::setRuntimeNull(
                      SM_MAX_PAGELIST_COUNT,
                      aHeader->mVarAllocList.mVRDB )
                  != IDE_SUCCESS );

        IDE_TEST( svpFixedPageList::setRuntimeNull(
                      & aHeader->mFixed.mVRDB )
                  != IDE_SUCCESS );

        IDE_TEST( svpVarPageList::setRuntimeNull(
                      SM_VAR_PAGE_LIST_COUNT,
                      aHeader->mVar.mVRDB )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT(0);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : table header의 lock item 및 runtime item 초기화
 * table header의 lock item의 할당 및 초기화를 처리하며,
 * page entry의 runtime item들을 초기화한다.
 * - 2nd. code design
 *   + smlLockItem 타입의 lockitem의 메모리 할당 (layer callback)
 *   + lockitem의 초기화(layer callback)
 *   + table header의 sync 초기화
 *   + drop된 index header의 list 초기화
 *   + QP의 table meta 정보(qcmTableInfo)를 위한 포인터 초기화
 *   + 테이블 타입에 따른 page list entry 초기화
 *
 * aStatistics - [IN]
 * aHeader     - [IN] Table의 Header
 ***********************************************************************/
IDE_RC smcTable::initLockAndRuntimeItem( smcTableHeader * aHeader )
{
    // lock item, mSync 초기화
    IDE_TEST( initLockItem( aHeader ) != IDE_SUCCESS );
    // runtime item 초기화
    IDE_TEST( initRuntimeItem( aHeader ) != IDE_SUCCESS );

    // drop된 index header의 list 초기화
    aHeader->mDropIndexLst = NULL;
    aHeader->mDropIndex    = 0;

    // for qcmTableInfo
    aHeader->mRuntimeInfo  = NULL;

    /* BUG-42928 No Partition Lock */
    aHeader->mReplacementLock = NULL;

    // BUG-43761 init row template
    aHeader->mRowTemplate = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table header의 lock item및 mSync초기화
 * table header의 lock item의 할당 및 초기화를 처리
 * - 2nd. code design
 *   + smlLockItem 타입의 lockitem의 메모리 할당 (layer callback)
 *   + lockitem의 초기화(layer callback)
 *   + table header의 sync 초기화
 *
 * aStatistics - [IN]
 * aHeader     - [IN] Table의 Header
 ***********************************************************************/
IDE_RC smcTable::initLockItem( smcTableHeader * aHeader )
{
    UInt  sTableType;

    sTableType = SMI_GET_TABLE_TYPE( aHeader );

    IDE_ASSERT( (sTableType == SMI_TABLE_MEMORY)   ||
                (sTableType == SMI_TABLE_META)     ||
                (sTableType == SMI_TABLE_VOLATILE) ||
                (sTableType == SMI_TABLE_DISK)     ||
                (sTableType == SMI_TABLE_REMOTE) );


    // table의 lockitem 초기화
    IDE_TEST( smLayerCallback::allocLockItem( &aHeader->mLock ) != IDE_SUCCESS );
    IDE_TEST( smLayerCallback::initLockItem( aHeader->mSpaceID,         // TBS ID
                                             (ULong)aHeader->mSelfOID,  // TABLE OID
                                             SMI_LOCK_ITEM_TABLE,
                                             aHeader->mLock )
              != IDE_SUCCESS );

    aHeader->mIndexLatch = NULL;
    IDE_TEST( allocAndInitIndexLatch( aHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : table header의 runtime item 초기화
 * table header의 page entry의 runtime item들을 초기화한다.
 * - 2nd. code design
 *   + drop된 index header의 list 초기화
 *   + QP의 table meta 정보(qcmTableInfo)를 위한 포인터 초기화
 *   + 테이블 타입에 따른 page list entry 초기화
 *     if (memory table)
 *        - smpFixedPageListEntry의 runtime item 초기화
 *        - smpVarPageListEntry의 runtime item 초기화
 *     else if (disk table )
 *        - sdpPageList의 runtime item 초기화
 *     endif
 *
 * aStatistics - [IN]
 * aHeader     - [IN] Table의 Header
 ***********************************************************************/
IDE_RC smcTable::initRuntimeItem( smcTableHeader * aHeader )
{
    UInt  sTableType;

    sTableType = SMI_GET_TABLE_TYPE( aHeader );

    IDE_ASSERT((sTableType == SMI_TABLE_MEMORY)   ||
               (sTableType == SMI_TABLE_META)     ||
               (sTableType == SMI_TABLE_DISK)     ||
               (sTableType == SMI_TABLE_VOLATILE) ||
               (sTableType == SMI_TABLE_REMOTE) );

    // runtime item 초기화
    switch (sTableType)
    {
        case SMI_TABLE_MEMORY :
        case SMI_TABLE_META :
        case SMI_TABLE_REMOTE:
            // FixedAllocPageList의 Mutex 초기화
            IDE_TEST( smpAllocPageList::initEntryAtRuntime(
                          aHeader->mFixedAllocList.mMRDB,
                          aHeader->mSelfOID,
                          SMP_PAGETYPE_FIX )
                      != IDE_SUCCESS );

            /* fixed smpPageListEntry runtime 정보 초기화 및 mutex 초기화 */
            IDE_TEST( smpFixedPageList::initEntryAtRuntime(aHeader->mSelfOID,
                                                           &(aHeader->mFixed.mMRDB),
                                                           aHeader->mFixedAllocList.mMRDB)
                  != IDE_SUCCESS);

            // VarAllocPageList의 Mutex 초기화
            IDE_TEST( smpAllocPageList::initEntryAtRuntime(
                          aHeader->mVarAllocList.mMRDB,
                          aHeader->mSelfOID,
                          SMP_PAGETYPE_VAR )
                      != IDE_SUCCESS );

            /* variable smpPageListEntry runtime 정보 초기화 및 mutex 초기화 */
            IDE_TEST( smpVarPageList::initEntryAtRuntime(aHeader->mSelfOID,
                                                         aHeader->mVar.mMRDB,
                                                         aHeader->mVarAllocList.mMRDB)
                      != IDE_SUCCESS);

            break;
        case SMI_TABLE_VOLATILE :
            // FixedAllocPageList의 Mutex 초기화
            IDE_TEST( svpAllocPageList::initEntryAtRuntime(
                          aHeader->mFixedAllocList.mVRDB,
                          aHeader->mSelfOID,
                          SMP_PAGETYPE_FIX )
                      != IDE_SUCCESS );

            /* fixed svpPageListEntry runtime 정보 초기화 및 mutex 초기화 */
            IDE_TEST( svpFixedPageList::initEntryAtRuntime(aHeader->mSelfOID,
                                                           &(aHeader->mFixed.mVRDB),
                                                           aHeader->mFixedAllocList.mVRDB)
                  != IDE_SUCCESS);

            // VarAllocPageList의 Mutex 초기화
            IDE_TEST( svpAllocPageList::initEntryAtRuntime(
                          aHeader->mVarAllocList.mVRDB,
                          aHeader->mSelfOID,
                          SMP_PAGETYPE_VAR )
                      != IDE_SUCCESS );

            /* variable smpPageListEntry runtime 정보 초기화 및 mutex 초기화 */
            IDE_TEST( svpVarPageList::initEntryAtRuntime(aHeader->mSelfOID,
                                                         aHeader->mVar.mVRDB,
                                                         aHeader->mVarAllocList.mVRDB)
                      != IDE_SUCCESS);

            break;

        case SMI_TABLE_DISK :
            /* sdpPageListEntry runtime 정보 초기화 및 mutex 초기화 */
            IDE_TEST( sdpPageList::initEntryAtRuntime( aHeader->mSpaceID,
                                                       aHeader->mSelfOID,
                                                       SM_NULL_INDEX_ID,
                                                       &(aHeader->mFixed.mDRDB),
                                                       (sTableType == SMI_TABLE_DISK ? ID_TRUE : ID_FALSE) )
                      != IDE_SUCCESS );

            break;
        case SMI_TABLE_TEMP_LEGACY :
        default :
            /* Table Type이 잘못된 값이 들어왔다. */
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : table header의 lock item 및 runtime item 초기화
 * table header의 lock item의 해제 및 page entry의 runtime item을 해제한다.
 * - 2nd. code design
 *   + lockitem의 해제(layer callback)
 *   + smlLockItem 타입의 lockitem의 메모리 해제 (layer callback)
 *   + 테이블 타입에 따른 page list entry 해제
 *
 * aHeader - [IN] 테이블 헤더
 ***********************************************************************/
IDE_RC smcTable::finLockAndRuntimeItem( smcTableHeader * aHeader )
{
    /* BUG-18133 : TC/Server/qp2/StandardBMT/DiskTPC-H/1.sql에서 서버 비정상 종료함
     *
     * Temp Table일경우 여기서 IDE_DASSERT로 죽게끔 되어 있었으나 Temp Table일 경우에도
     * 이 함수가 호출되도록 하여야 한다. 왜냐하면 Runtime정보와 TableHeader에 할당된
     * Index Latch를 Free해야 되기 때문이다. */
    IDE_TEST( finLockItem( aHeader ) != IDE_SUCCESS );

    IDE_TEST( finRuntimeItem( aHeader ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table header의 lock item 해제
 * table header의 lock item의 해제.
 * - 2nd. code design
 *   + lockitem의 해제(layer callback)
 *   + smlLockItem 타입의 lockitem의 메모리 해제 (layer callback)
 *
 * aHeader - [IN] 테이블 헤더
 ***********************************************************************/
IDE_RC smcTable::finLockItem( smcTableHeader * aHeader )
{
    // OFFLINE/DISCARD Tablespace의 경우라도 mLock이 초기화되어 있다.
    IDE_ASSERT( aHeader->mLock != NULL );

    IDE_TEST( smLayerCallback::destroyLockItem( aHeader->mLock )
              != IDE_SUCCESS );
    IDE_TEST( smLayerCallback::freeLockItem( aHeader->mLock )
              != IDE_SUCCESS );

    aHeader->mLock = NULL ;

    /* BUG-42928 No Partition Lock */
    aHeader->mReplacementLock = NULL;

    // Drop된 Table의 경우 mIndexLatch가 이미 해제되어서
    // NULL일 수 있다.
    if ( aHeader->mIndexLatch != NULL )
    {
        IDE_TEST( finiAndFreeIndexLatch( aHeader ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : table header의 runtime item 해제
 * table header의 page entry의 runtime item을 해제한다.
 * - 2nd. code design
 *   + 테이블 타입에 따른 page list entry 해제
 *     if (memory table)
 *        - smpFixedPageListEntry의 runtime item 해제
 *        - smpVarPageListEntry의 runtime item 해제
 *     else if (disk table)
 *        - sdpPageList의 runtime item 해제
 *     endif
 *
 * aHeader - [IN] Table의 Header
 ***********************************************************************/
IDE_RC smcTable::finRuntimeItem( smcTableHeader * aHeader )
{
    switch( SMI_GET_TABLE_TYPE( aHeader ) )
    {
        case SMI_TABLE_MEMORY :
        case SMI_TABLE_META :
        case SMI_TABLE_REMOTE:            
            // To Fix BUG-16752 Memory Table Drop시 Runtime Entry가 해제되지 않음

            // Drop된 Table에 대해서는 Runtime Item이 모두 해제되어 있다.
            // Runtime Item이 존재하는( Drop되지 않은 ) Table에 대해서만
            // 처리
            if ( aHeader->mFixed.mMRDB.mRuntimeEntry != NULL )
            {
                /* fixed smpPageListEntry runtime 정보및 mutex 해제 */
                IDE_TEST(smpFixedPageList::finEntryAtRuntime(
                             &(aHeader->mFixed.mMRDB))
                         != IDE_SUCCESS);
            }

            if ( aHeader->mVar.mMRDB[0].mRuntimeEntry != NULL )
            {
                /* variable smpPageListEntry runtime 정보및 mutex 해제 */
                IDE_TEST(smpVarPageList::finEntryAtRuntime(aHeader->mVar.mMRDB)
                         != IDE_SUCCESS);
            }
            break;

        case SMI_TABLE_VOLATILE :
            // To Fix BUG-16752 Memory Table Drop시 Runtime Entry가 해제되지 않음
            if ( aHeader->mFixed.mVRDB.mRuntimeEntry != NULL )
            {
                /* fixed svpPageListEntry runtime 정보및 mutex 해제 */
                IDE_TEST(svpFixedPageList::finEntryAtRuntime(
                             &(aHeader->mFixed.mVRDB))
                         != IDE_SUCCESS);

            }

            if ( aHeader->mVar.mVRDB[0].mRuntimeEntry != NULL )
            {
                /* variable svpPageListEntry runtime 정보및 mutex 해제 */
                IDE_TEST(svpVarPageList::finEntryAtRuntime(aHeader->mVar.mVRDB)
                         != IDE_SUCCESS);
            }
            break;

        case SMI_TABLE_DISK :
            /* sdpPageListEntry runtime 정보 초기화 및 mutex 초기화 */
            IDE_TEST( sdpPageList::finEntryAtRuntime(&(aHeader->mFixed.mDRDB))
                      != IDE_SUCCESS);

            break;
        case SMI_TABLE_TEMP_LEGACY :
        default :
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : TableHeader의 지정된 Type의 Stat정보를 증가시킨다.
 *
 * aHeader       - [IN] Table의 Header
 * aTblStatType  - [IN] Table Stat Type
 ***********************************************************************/
void smcTable::incStatAtABort( smcTableHeader * aHeader,
                               smcTblStatType   aTblStatType )
{
    smpRuntimeEntry * sTableRuntimeEntry;

    IDE_ASSERT( aHeader != NULL );

    if( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE )
    {
        sTableRuntimeEntry =
            aHeader->mFixed.mMRDB.mRuntimeEntry ;

        IDE_ASSERT( sTableRuntimeEntry != NULL );

        IDE_ASSERT(sTableRuntimeEntry->mMutex.lock(NULL /* idvSQL* */)
                   == IDE_SUCCESS );

        // BUG-17371  [MMDB] Aging이 밀릴경우 System에 과부하
        //           및 Aging이 밀리는 현상이 더 심화됨.
        switch( aTblStatType )
        {
            case SMC_INC_UNIQUE_VIOLATION_CNT:
                sTableRuntimeEntry->mUniqueViolationCount++;
                break;

            case SMC_INC_RETRY_CNT_OF_LOCKROW:
                sTableRuntimeEntry->mLockRowRetryCount++;
                break;

            case SMC_INC_RETRY_CNT_OF_UPDATE:
                sTableRuntimeEntry->mUpdateRetryCount++;
                break;

            case SMC_INC_RETRY_CNT_OF_DELETE:
                sTableRuntimeEntry->mDeleteRetryCount++;
                break;

            default:
                break;
        }

        IDE_ASSERT(sTableRuntimeEntry->mMutex.unlock()
                   == IDE_SUCCESS );
    }
    return;
}

/***********************************************************************
 * Description : memory table 생성시 page list entry를 초기화
 * - 2nd. code design
 *   + fixed page list entry 초기화
 *   + variable page list entry 초기화
 *   + table header의 variable page list entry 개수(11개) 설정
 ***********************************************************************/
void smcTable::initMRDBTablePageList( vULong            aMax,
                                      smcTableHeader*   aHeader )
{
    IDE_DASSERT( aHeader != NULL );

    // memory table의 fixed pagelist entry 초기화
    smpFixedPageList::initializePageListEntry(
                      &(aHeader->mFixed.mMRDB),
                      aHeader->mSelfOID,
                      idlOS::align((UInt)(aMax), ID_SIZEOF(SDouble)));

    // memory table의 variable pagelist entry 초기화
    smpVarPageList::initializePageListEntry(aHeader->mVar.mMRDB,
                                            aHeader->mSelfOID);

    return;
}

/***********************************************************************
 * Description : volatile table 생성시 page list entry를 초기화
 * - 2nd. code design
 *   + fixed page list entry 초기화
 *   + variable page list entry 초기화
 *   + table header의 variable page list entry 개수(11개) 설정
 ***********************************************************************/
void smcTable::initVRDBTablePageList( vULong            aMax,
                                      smcTableHeader*   aHeader )
{
    IDE_DASSERT( aHeader != NULL );

    // volatile table의 fixed pagelist entry 초기화
    svpFixedPageList::initializePageListEntry(
                      &(aHeader->mFixed.mVRDB),
                      aHeader->mSelfOID,
                      idlOS::align((UInt)(aMax), ID_SIZEOF(SDouble)));

    // volatile table의 variable pagelist entry 초기화
    svpVarPageList::initializePageListEntry(aHeader->mVar.mVRDB,
                                            aHeader->mSelfOID);

    return;
}

/***********************************************************************
 * Description : disk table 생성시 page list entry를 초기화
 * - 2nd. code design
 *   + sdpPageListEntry 초기화
 *   + table의 data page를 위한 segment 생성 및
 *     segment header 페이지 초기화
 ***********************************************************************/
void smcTable::initDRDBTablePageList( vULong             aMax,
                                      smcTableHeader*    aHeader,
                                      smiSegAttr         aSegmentAttr,
                                      smiSegStorageAttr  aSegmentStoAttr )
{
    IDE_DASSERT( aHeader != NULL );

    // disk table의 pagelist entry 초기화
    sdpPageList::initializePageListEntry(
                 &(aHeader->mFixed.mDRDB),
                 idlOS::align((UInt)(aMax), ID_SIZEOF(SDouble)),
                 aSegmentAttr,
                 aSegmentStoAttr );

    return;
}

/***********************************************************************
 * Description : table header 초기화(memory-disk 공통)
 * 테이블 생성시 catalog table에서 할당받은 slot에 table header를 초기화한다.
 *
 * - 2nd. code design
 *   + 새로운 table header의 index 정보 초기화
 *   + 테이블 타입에 따른 table header의 page list entry 초기화
 *     if (memory table)
 *       - smpFixedPageListEntry의 runtime item 해제
 *       - smpVarPageListEntry의 runtime item 해제
 *     else if (disk table)
 *       - sdpPageList의 runtime item 해제(logging)
 *     endif
 *   + table header의 초기화
 *   + table header 초기화에 대한 로깅처리
 *     : SMR_SMC_TABLEHEADER_INIT (REDO가능)
 ***********************************************************************/
IDE_RC smcTable::initTableHeader( void*                 aTrans,
                                  scSpaceID             aSpaceID,
                                  vULong                aMax,
                                  UInt                  aColumnSize,
                                  UInt                  aColumnCount,
                                  smOID                 aFixOID,
                                  UInt                  aFlag,
                                  smcSequenceInfo*      aSequence,
                                  smcTableType          aType,
                                  smiObjectType         aObjectType,
                                  ULong                 aMaxRow,
                                  smiSegAttr            aSegmentAttr,
                                  smiSegStorageAttr     aSegmentStoAttr,
                                  UInt                  aParallelDegree,
                                  smcTableHeader*       aHeader )
{
    UInt sTableType;
    UInt i;

    idlOS::memset((SChar*)aHeader, 0, sizeof(smcTableHeader));

    aHeader->mLock                = NULL;
    aHeader->mSpaceID             = aSpaceID;
    aHeader->mType                = aType;
    aHeader->mObjType             = aObjectType;
    aHeader->mSelfOID             = aFixOID;

    aHeader->mColumnSize          = aColumnSize;
    aHeader->mColumnCount         = aColumnCount;
    aHeader->mLobColumnCount      = 0;

    aHeader->mColumns.length      = 0;
    aHeader->mColumns.fstPieceOID = SM_NULL_OID;
    aHeader->mColumns.flag        = SM_VCDESC_MODE_OUT;

    aHeader->mInfo.length         = 0;
    aHeader->mInfo.fstPieceOID    = SM_NULL_OID;
    aHeader->mInfo.flag           = SM_VCDESC_MODE_OUT;

    aHeader->mDropIndexLst        = NULL;
    aHeader->mDropIndex           = 0;
    aHeader->mFlag                = aFlag;
    // PROJ-1665
    aHeader->mParallelDegree      = aParallelDegree;
    aHeader->mIsConsistent        = ID_TRUE;
    aHeader->mMaxRow              = aMaxRow;
    /* BUG-48588 */ 
    SM_SET_SCN_INFINITE( &(aHeader->mTableCreateSCN) );

    aHeader->mRowTemplate         = NULL;

    /* BUG-42928 No Partition Lock */
    aHeader->mReplacementLock     = NULL;


    if( aMax < ID_SIZEOF(smOID) )
    {
        aMax = ID_SIZEOF(smOID);
    }

    sTableType = aFlag & SMI_TABLE_TYPE_MASK;
    if ((sTableType == SMI_TABLE_MEMORY) ||
        (sTableType == SMI_TABLE_META)   ||
        (sTableType == SMI_TABLE_REMOTE)) 
    {
        /* ------------------------------------------------
         * memory table의 variable page list entry와 fixed
         * page list entry를 모두 초기화
         * ----------------------------------------------*/
        initMRDBTablePageList(aMax, aHeader);
    }
    else if ( sTableType == SMI_TABLE_DISK )
    {
        /* ------------------------------------------------
         * disk table의 page list entry를 초기화하며,
         * table 데이타 저장을 위한 segment 생성 (mtx logging 모드)
         * ----------------------------------------------*/
        initDRDBTablePageList(aMax,
                              aHeader,
                              aSegmentAttr,
                              aSegmentStoAttr );
    }
    /* PROJ-1594 Volatile TBS */
    else if (sTableType == SMI_TABLE_VOLATILE)
    {
        initVRDBTablePageList(aMax, aHeader);
    }
    else
    {
        IDE_ASSERT(0);
    }

    if( aSequence != NULL )
    {
        idlOS::memcpy( &(aHeader->mSequence),
                       aSequence,
                       ID_SIZEOF(smcSequenceInfo) );
    }
    else
    {
        idlOS::memset( &(aHeader->mSequence),
                       0xFF,
                       ID_SIZEOF(smcSequenceInfo) );
    }

    for(i = 0; i < SMC_MAX_INDEX_OID_CNT; i ++)
    {
        aHeader->mIndexes[i].length = 0;
        aHeader->mIndexes[i].fstPieceOID = SM_NULL_OID;
        aHeader->mIndexes[i].flag = SM_VCDESC_MODE_OUT;
    }//for i;

    /* TASK-4990 changing the method of collecting index statistics */
    idlOS::memset( &aHeader->mStat, 0, ID_SIZEOF( smiTableStat ) );

    /* ------------------------------------------------
     * Update Type - SMR_SMC_TABLEHEADER_INIT
     * redo func. :  redo_SMC_TABLEHEADER_INIT()
     * ----------------------------------------------*/
    IDE_TEST(smrUpdate::initTableHeader( NULL, /* idvSQL* */
                                         aTrans,
                                         SM_MAKE_PID(aFixOID),
                                         SM_MAKE_OFFSET(aFixOID)+SMP_SLOT_HEADER_SIZE,
                                         aHeader,
                                         ID_SIZEOF(smcTableHeader) )
             != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  memory/disk 타입의 normal table 생성 (공통)
 * =====================================================================
 * - 2nd. code design
 *   + catalog table에 대하여 IX_LOCK 요청
 *   + 새로운 table의 header를 저장하기 위해 catalog table에서
 *     fixed slot을 할당
 *   + record의 fixed 영역의 길이를 계산
 *   + table 타입에 따른 table header 초기화
 *   + table 타입에 따라 해당 pagelist entry의 mutex 및
 *     runtime 정보를 초기화
 *   + table 생성 연산에 대한 NTA 로깅
 *     :SMR_OP_CREATE_TABLE
 *   + DDL의 오류발생시 rollback과정에서 할당된 fixed slot에
 *     트랜잭션 commit시 commit scn을 설정할 수 있도록
 *     SM_OID_DROP_TABLE_BY_ABORT표시
 *   + table의 column의 정보를 Variable영역에 설정
 *     : alloc variable slot
 *     : column 정보 로깅 - SMR_LT_UPDATE의 SMR_PHYSICAL
 *     : 트랜잭션 rollback시에 할당된 new variable slot을
 *       ager가 remove 할 수 있도록 SM_OID_NEW_VARIABLE_SLOT을 표시
 *     : table header에 컬럼 정보 변경 및 로깅
 *       - SMR_SMC_TABLEHEADER_UPDATE_COLUMNS
 *   + 새로운 table의 info 정보를 위하여 값을 설정
 *     : alloc variable slot
 *     : 할당받은 variable slot에 val을 갱신 및 로깅
 *        - SMR_LT_UPDATE의 SMR_SMC_PERS_UPDATE_VAR_ROW
 *     : 트랜잭션 rollback시에 할당된 new variable slot을
 *       ager가 remove 할 수 있도록 SM_OID_NEW_VARIABLE_SLOT을 표시
 *   + table 타입에 따라 NullRow를 data page에 insert 및 로깅
 *     : s?cRecord를 통하여 nullrow에 대한 insert 처리
 *     : insert된 nullrow에 대하여 NULL_ROW_SCN 설정 및 로깅
 *       - SMR_LT_UPDATE의 SMR_SMC_PERS_UPDATE_FIXED_ROW
 *     : table header에 nullrow에 대한 설정 및 로깅
 *       - SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_SET_NULLROW
 *   + 새로운 table에 대하여 X_LOCK 요청
 * =====================================================================
 *
 * - RECOVERY
 *
 *  # table 생성시 (* disk-table생성)
 *
 *  (0) (1)    (2)          (3)         (4)      (5)
 *   |---|------|------------|-----------|--------|-----------> time
 *       |      |            |           |        |
 *     alloc    *update     *alloc       init     create
 *     f-slot   seg rID,    segment      tbl-hdr  tbl
 *     from     meta pageID [NTA->(1)]   (R)      [NTA->(0)]
 *     cat-tbl  in tbl-hdr
 *    [NTA->(0)]     (R/U)
 *
 *      (6)       (7)     (8)         (9)      (10)        (11)
 *   ----|---------|-------|-----------|---------|----------|---> time
 *     alloc      init     update      alloc     update     update
 *     v-slot     column   column      v-slot    var row    info
 *     from       info     info        from      (R/U)      on tbl-hdr
 *     cat-tbl    (R/U)    on tbl-hdr  cat-tbl              (R/U)
 *     [NTA->(5)]          (R/U)       [NTA->(8)]
 *
 *      (12)      (13)    (14)
 *   ----|---------|-------|----------------> time
 *      insert    update   set
 *      ..        insert   nullrow
 *                row hdr  on tbl-hdr
 *                (R/U)    (R)
 *
 *  # Disk-Table Recovery Issue
 *
 *  - (2)까지 로깅하였다면
 *    + (2) -> undo
 *      : tbl-hdr에 segment rID, table meta PID를 이전 image로 복구한다.
 *    + (1) -> logical undo
 *      : tbl-hdr slot을 freeslot한다.
 *
 *  - (3)까지 mtx commit하였다면
 *    + (3)-> logical undo
 *      : segment를 free한다. ((3)':SDR_OP_NULL) [NTA->(1)]
 *        만약 free하지 못하였다면, 다시 logical undo를 수행하면 된다.
 *        free segment를 mtx commit 하였다면, [NTA->(1)]따라 다음 undo과정
 *        으로 skip하고 넘어간다.
 *    + (1) -> logical undo
 *      : tbl-hdr slot을 freeslot한다.
 *
 *  - (5)까지 로깅 하였다면
 *    + (5) -> logical undo
 *      : setDropTable을 처리한다. (U/R) (a)
 *      : doNTADropTable을 수행하여 dropTablePending을 호출한다. (b)(c)
 *      =>
 *
 *      (5)(a)    (b)                 (c)
 *       |--|------|-------------------|-----------> time
 *                 |                   |
 *                 update             free
 *                 seg rID,           segment
 *                 meta pageID        [NTA->(a)]
 *                 in tbl-hdr         (NTA OP NULL)
 *                 (R/U)
 *      ㄱ. (b)까지만 로깅했다면 (b)로그에 대하여 undo가 수행되며 다음과
 *          같이 처리된다.
 *          => (5)->(a)->(b)->(b)'->(a)'->(a)->(b)->(c)->(5)'->...
 *             NTA            CLR   CLR                  DUMMY_CLR
 *
 *      ㄴ. (c)까지 mtx commit이 되었다면
 *          => (5)->(a)->(b)->(c)->(d)->..->(5)'->...
 *             NTA                          DUMMY_CLR
 *
 *    + (1) -> logical undo
 *      : tbl-hdr slot을 freeslot한다.
 ***********************************************************************/
IDE_RC smcTable::createTable( idvSQL              * aStatistics,
                              void*                 aTrans,
                              scSpaceID             aSpaceID,
                              const smiColumnList*  aColumns,
                              UInt                  aColumnSize,
                              const void*           aInfo,
                              UInt                  aInfoSize,
                              const smiValue*       aNullRow,
                              UInt                  aFlag,
                              ULong                 aMaxRow,
                              smiSegAttr            aSegmentAttr,
                              smiSegStorageAttr     aSegmentStoAttr,
                              UInt                  aParallelDegree,
                              const void**          aHeader )
{
    smpSlotHeader   * sSlotHeader;
    smcTableHeader  * sHeader;
    smOID             sFixOID = SM_NULL_OID;
    smOID             sLobColOID;
    smSCN             sInfiniteSCN;
    smiColumn       * sColumn;
    smcTableHeader    sHeaderArg;
    scPageID          sHeaderPageID = 0;
    UInt              sMax;
    UInt              sCount;
    UInt              sTableType;
    UInt              sState = 0;
    UInt              i;
    smLSN             sLsnNTA;
    svrLSN            sLsn4Vol;
    ULong             sNullRowID;
    smTID             sTID;

    IDU_FIT_POINT( "1.BUG-42154@smcTable::createTable" );

    if(aMaxRow == 0)
    {
        aMaxRow = ID_ULONG_MAX;
    }

    sTID = smxTrans::getTransID( aTrans );

    // BUG-37607 Transaction ID should be recorded in the slot containing table header.
    SM_SET_SCN_INFINITE_AND_TID( &sInfiniteSCN, sTID );

    sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

    /* ------------------------------------------------
     * [1] 새로운 Table을 위한 Table Header영역을
     * Catalog Table에서 할당받는다.
     * ----------------------------------------------*/
    if(( aFlag & SMI_TABLE_PRIVATE_VOLATILE_MASK ) == SMI_TABLE_PRIVATE_VOLATILE_FALSE )
    {
        IDE_TEST( smpFixedPageList::allocSlot(aTrans,
                                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                              NULL,
                                              SMC_CAT_TABLE->mSelfOID,
                                              &(SMC_CAT_TABLE->mFixed.mMRDB),
                                              (SChar**)&sSlotHeader,
                                              sInfiniteSCN,
                                              SMC_CAT_TABLE->mMaxRow,
                                              SMP_ALLOC_FIXEDSLOT_ADD_INSERTCNT |
                                              SMP_ALLOC_FIXEDSLOT_SET_SLOTHEADER,
                                              SMR_OP_SMC_TABLEHEADER_ALLOC)
                  != IDE_SUCCESS );
    }
    else
    {
        /* PROJ-1407 Temporary Table
         * User session temporary table의 경우 restart시 free되어야 하므로
         * temp catalog에서 slot을 할당받는다.*/
        IDE_TEST( smpFixedPageList::allocSlotForTempTableHdr(
                                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                          SMC_CAT_TEMPTABLE->mSelfOID,
                                          &(SMC_CAT_TEMPTABLE->mFixed.mMRDB),
                                          (SChar**)&sSlotHeader,
                                          sInfiniteSCN ) 
                  != IDE_SUCCESS );
    }


    IDU_FIT_POINT( "1.BUG-18279@smcTable::createTable" );

    IDU_FIT_POINT( "1.PROJ-1552@smcTable::createTable" );


    /* ------------------------------------------------
     * [2] 새로운 Table을 위한 Table Header영역을
     * Catalog Table에서 할당받았으므로,
     * New Version List에 추가한다.
     * 이 함수의 [2] 참조.
     * ----------------------------------------------*/
    sHeaderPageID = SMP_SLOT_GET_PID((SChar *)sSlotHeader);
    sFixOID       = SM_MAKE_OID( sHeaderPageID,
                                 SMP_SLOT_GET_OFFSET( sSlotHeader ) );
    sState        = 2;
    sHeader       = (smcTableHeader *)( sSlotHeader + 1 );

    IDU_FIT_POINT_RAISE( "1.PROJ-1407@smcTable::createTable", err_ART );

    /* ------------------------------------------------
     * [3] 이 Table의 Fixed 영역의 Row 길이를
     * 계산한다.
     * 각 Column 정보의 <Offset + Size>값의 최대값을
     * Fixed 영역의 Row 길이로 설정.
     *
     * bugbug : 이 코드는 중복으로 사용되기때문에,
     * 별도의 함수로 분리시켜 호출하도록 처리해야함
     * ----------------------------------------------*/
    sTableType = aFlag & SMI_TABLE_TYPE_MASK;

    // Create Table을 위한 에러처리
    IDE_TEST( checkError4CreateTable( aSpaceID,
                                      aFlag )
              != IDE_SUCCESS );

    /* Column List가 Valid한지 검사하고 Column의 갯수와 Fixed row의 크기
       를 구한다.*/
    IDE_TEST( validateAndGetColCntAndFixedRowSize( sTableType,
                                                   aColumns,
                                                   &sCount,
                                                   &sMax )
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * 공통 table header의 대부분의 멤버는 동일하게
     * 초기화되더라도, page list entry에 대해서는
     * 테이블타입에 따라 초기화가 다르다.
     * ----------------------------------------------*/
    /* stack 변수에 table header 값 초기화 */
    IDE_TEST( initTableHeader( aTrans,
                               aSpaceID,
                               sMax,
                               aColumnSize,
                               sCount,
                               sFixOID,
                               aFlag,
                               NULL,
                               SMC_TABLE_NORMAL,
                               SMI_OBJECT_NONE,
                               aMaxRow,
                               aSegmentAttr,
                               aSegmentStoAttr,
                               aParallelDegree,
                               &sHeaderArg )
              != IDE_SUCCESS );

    /* 실제 table header 값을 설정 */
    idlOS::memcpy(sHeader, &sHeaderArg, ID_SIZEOF(smcTableHeader));

    IDU_FIT_POINT( "1.BUG-31673@smcTable::createTable" );

    // table 타입에 따라 해당 pagelist entry의 mutex 및 runtime 정보를 초기화
    IDE_TEST( initLockAndRuntimeItem( sHeader) != IDE_SUCCESS );

    if ( sTableType == SMI_TABLE_DISK )
    {
        /* ------------------------------------------------
         * disk table의 page list entry를 초기화하며,
         * table 데이타 저장을 위한 segment 생성 (mtx logging 모드)
         * ----------------------------------------------*/
         // disk table의 table segment 할당 및 table segment header page 초기화
         IDE_TEST( sdpSegment::allocTableSeg4Entry(
                       aStatistics,
                       aTrans,
                       aSpaceID,
                       sHeader->mSelfOID,
                       (sdpPageListEntry*)getDiskPageListEntry(sHeader),
                       SDR_MTX_LOGGING )
                   != IDE_SUCCESS );
    }

    /* ------------------------------------------------
     * [4] 이 Table의 Fixed 영역의 Row 길이가 Persistent Page Size를 초과한다면
     * 에러 처리 한다.
     * ----------------------------------------------*/
    sTableType = SMI_GET_TABLE_TYPE( sHeader );

    IDE_DASSERT( sTableType == SMI_TABLE_MEMORY ||
                 sTableType == SMI_TABLE_META   ||
                 sTableType == SMI_TABLE_DISK   ||
                 sTableType == SMI_TABLE_VOLATILE );

    // !!] OP_NTA : table 생성 연산에 대한 로깅
    IDE_TEST( smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                        aTrans,
                                        &sLsnNTA,
                                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                        SMR_OP_CREATE_TABLE,
                                        sFixOID)
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * !! CHECK RECOVERY POINT
     * case) SMR_OP_CREATE_TABLE을 로깅하고 crash 발생
     * undoAll과정에서 NTA logical undo를 수행하여 table을 제거한다.
     * disk table은 추가적으로 table segment까지 free한다.
     * ----------------------------------------------*/
    IDU_FIT_POINT( "1.PROJ-1552@smcTable::createTable" );
    sState = 1;

    /* ------------------------------------------------
     * DDL의 오류발생시 rollback과정에서 할당된 fixed slot에
     * 대하여 commit scn을 설정할수 있도록 표시
     * ----------------------------------------------*/
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       sFixOID,
                                       sFixOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_DROP_TABLE_BY_ABORT )
              != IDE_SUCCESS );

    /* Table Space에 대해서 DDL이 발생했다는것을 표시*/
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       sFixOID,
                                       sFixOID,
                                       aSpaceID,
                                       SM_OID_DDL_TABLE )
              != IDE_SUCCESS );

    /* tableHeader.mCreateSCN 설정이 필요하다는것을 표시 */
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       sFixOID,
                                       sFixOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_CREATE_TABLE )
              != IDE_SUCCESS );

    /* [5] 이 Table의 Column의 정보를 Variable영역에 설정한다. */
    IDE_TEST( setColumnAtTableHeader( aTrans,
                                      sHeader,
                                      aColumns,
                                      sCount,
                                      aColumnSize )
              != IDE_SUCCESS );

    // BUG-28356 [QP]실시간 add column 제약 조건에서
    //           lob column에 대한 제약 조건 삭제해야 함
    // Table내의 모든 Column을 순회하면서 LOB Column이면 LOB Segment를 생성한다.
    if( ( sTableType == SMI_TABLE_DISK ) &&
        ( sHeader->mLobColumnCount != 0 ) )
    {
        for( i = 0 ; i < sHeader->mColumnCount ; i++ )
        {
            sColumn = getColumnAndOID( sHeader,
                                       i, // Column Idx
                                       &sLobColOID );

            if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
            {
                SC_MAKE_NULL_GRID( sColumn->colSeg );
                IDE_TEST(sdpSegment::allocLobSeg4Entry(
                             aStatistics,
                             aTrans,
                             sColumn,
                             sLobColOID,
                             sHeader->mSelfOID,
                             SDR_MTX_LOGGING )
                         != IDE_SUCCESS);
            }
        }
    }

    /* ------------------------------------------------
     * [6] 새로운 Table의 Info 정보를 위하여 값을 설정한다.
     * ----------------------------------------------*/
    IDE_TEST(setInfoAtTableHeader(aTrans, sHeader, aInfo, aInfoSize)
             != IDE_SUCCESS);

    /* ------------------------------------------------
     * [7] 새로운 Table에 NullRow를 삽입한다.
     *
     * FOR A4 : nullrow 처리
     *
     * nullrow를 table에 insert하는 방식으로 처리한다.
     *
     * + memory table nullrow (smcRecord::makeNullRow)
     *  _______________________________________________________
     * |slot header|record header| fixed|__var__|fixed|__var__|
     * |___________|_____________|______|2_|OID_|_____|2_|OID_|
     *                                     | ___________/
     *                                  ___|/______
     *                                  |0|var    |
     *                                  |_|selfOID|
     *                                    catalog table의
     *                                    nullOID
     *
     *  nullrow의 varcolumn을 매번 할당하지 않기위해서는
     *  일반 테이블의 nullrow의 column중 varcolumn은 할당되지 않으며
     *  단지 catalog table의 nullOID에 저장된 varcolumn을
     *  assign하도록 처리
     *
     * ----------------------------------------------*/

    /* BUG-32544 [sm-mem-collection] The MMDB alter table operation can
     * generate invalid null row.
     * CreateTable시에 생성된 NullRow는 Undo할 필요가 없습니다.
     * 어차피 Rollback되면 Table이 Drop되면서 반환될 것이기 때문입니다.*/
    sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );
    if (sTableType == SMI_TABLE_DISK)
    {
        // PROJ-1705
        // Disk Table과 Disk Temp Table은
        // Null Row를 insert하지 않도록 변경한다.

        IDE_TEST( initRowTemplate( NULL, sHeader, NULL ) != IDE_SUCCESS );
    }
    /* PROJ-1594 Volatile TBS */
    else
    {
        if( sTableType == SMI_TABLE_VOLATILE )
        {
            sLsn4Vol = svrLogMgr::getLastLSN(
                ((smxTrans*)aTrans)->getVolatileLogEnv());

            IDE_TEST( smLayerCallback::makeVolNullRow( aTrans,
                                                       sHeader,
                                                       sInfiniteSCN,
                                                       aNullRow,
                                                       SM_FLAG_MAKE_NULLROW,
                                                       (smOID*)&sNullRowID )
                      != IDE_SUCCESS );

            /* vol tbs의 경우 아래와 같이 UndoLog를 지워주는 것이
             * NTA Log이다. */
            IDE_TEST( svrLogMgr::removeLogHereafter( 
                    ((smxTrans*)aTrans)->getVolatileLogEnv(),
                    sLsn4Vol )
                != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST(smcRecord::makeNullRow(aTrans,
                                            sHeader,
                                            sInfiniteSCN,
                                            aNullRow,
                                            SM_FLAG_MAKE_NULLROW,
                                            (smOID*)&sNullRowID)
                     != IDE_SUCCESS);

            /* smcRecord::makeNullRow안에서 Null Row OID Setting관련해서
               Logging합니다.*/
        }

        IDE_TEST( setNullRow( aTrans,
                              sHeader,
                              sTableType,
                              &sNullRowID )
                  != IDE_SUCCESS );
    }

    /* BUG-32544 [sm-mem-collection] The MMDB alter table operation can
     * generate invalid null row.
     * NTA Log를 통해 NullRow에 대한 Undo를 막습니다*/
    IDE_TEST(smrLogMgr::writeNullNTALogRec( NULL, /* idvSQL* */
                                            aTrans,
                                            & sLsnNTA)
             != IDE_SUCCESS);

    sState = 0;
    // insert dirty page into dirty page list
    IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                             SM_MAKE_PID(sHeader->mSelfOID))
              != IDE_SUCCESS );

    // [8] 새로 생성한 Table에 대하여 X Lock 요청
    IDE_TEST( smLayerCallback::lockTableModeX( aTrans, SMC_TABLE_LOCK( sHeader ) )
              != IDE_SUCCESS );

    *aHeader = (const void*)sHeader;


    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( err_ART );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_ART));
    }
#endif
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch(sState)
    {
        case 2:

            if(( aFlag & SMI_TABLE_PRIVATE_VOLATILE_MASK )
               == SMI_TABLE_PRIVATE_VOLATILE_FALSE )
            {
                IDE_ASSERT( smLayerCallback::addOID( aTrans,
                                                     SMC_CAT_TABLE->mSelfOID,
                                                     sFixOID,
                                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                     SM_OID_NEW_INSERT_FIXED_SLOT )
                            == IDE_SUCCESS );
            }
            else
            {
                IDE_ASSERT( smLayerCallback::addOID( aTrans,
                                                     SMC_CAT_TEMPTABLE->mSelfOID,
                                                     sFixOID,
                                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                     SM_OID_NEW_INSERT_FIXED_SLOT )
                            == IDE_SUCCESS );
            }

        case 1:
            IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(
                            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,sHeaderPageID)
                        == IDE_SUCCESS );

        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table Header에 새로운 Info 정보를 Setting한다. 만약
 *               기존 Info 정보가 있다면 삭제하고 새로운 Info 로 대체한다.
 *               만약 새로운 INfo 또한 지정된것이 없다면 Table의 Info
 *               는 NULL로 Setting된다.
 *
 * aTrans     - [IN] : Transaction Pointer
 * aTable     - [IN] : Table Header
 * aInfo      - [IN] : Info Pointer : 지정할 Info가 없다면 NULL
 * aInfoSize  - [IN] : Info Size    : 지정할 Info가 없다면 0
 ***********************************************************************/
IDE_RC smcTable::setInfoAtTableHeader( void*            aTrans,
                                       smcTableHeader*  aTable,
                                       const void*      aInfo,
                                       UInt             aInfoSize)
{
    smiValue         sVal;
    smOID            sNextOid = SM_NULL_OID;
    scPageID         sHeaderPageID = SM_NULL_PID;
    UInt             sState = 0;
    smOID            sFstVCPieceOID = SM_NULL_OID;
    smOID            sVCPieceOID = SM_NULL_OID;
    smVCPieceHeader* sVCPieceHeader;
    UInt             sInfoPartialLen;
    UInt             sOffset = 0;
    UInt             sMaxVarLen;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTable != NULL );

    /* [1] 기존 info 정보가 있는 경우 그 old version을 versioning list에 추가 */
    if(aTable->mInfo.fstPieceOID != SM_NULL_OID)
    {
        sVCPieceOID = aTable->mInfo.fstPieceOID;

        /* VC를 구성하는 VC Piece의 Flag에 Delete Bit추가.*/
        if ( ( aTable->mInfo.flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT )
        {
            IDE_TEST( smcRecord::deleteVC( aTrans,
                                           SMC_CAT_TABLE,
                                           sVCPieceOID )
                    != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. inmode 는 fixed record에서 함께 처리된다. */
        }
    }

    if( aInfo != NULL )
    {
        sInfoPartialLen = aInfoSize;

        /* =================================================================
         * [2] info 정보를 variable page에 저장
         *     info 정보가 SM_PAGE_SIZE를 넘는 경우 여러 페이지에 걸쳐 저장
         * ================================================================ */
        sMaxVarLen = SMP_VC_PIECE_MAX_SIZE;

        while ( sInfoPartialLen > 0 )
        {
            /* [1-1] info 정보를 위해 variable column 정보 구성 */
            if ( sInfoPartialLen == aInfoSize )
            {
                sVal.length      = sInfoPartialLen  % sMaxVarLen;
                sOffset          = (sInfoPartialLen / sMaxVarLen) * sMaxVarLen;
                sInfoPartialLen -= sVal.length;
            }
            else
            {
                sVal.length      = sMaxVarLen;
                sOffset         -= sMaxVarLen;
                sInfoPartialLen -= sMaxVarLen;
            }
            sVal.value  = (void *)((char *)aInfo + sOffset);

            /* [1-2] info 정보를 저장하기 위한 variable slot 할당 */
            IDE_TEST( smpVarPageList::allocSlot(
                          aTrans,
                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                          SMC_CAT_TABLE->mSelfOID,
                          SMC_CAT_TABLE->mVar.mMRDB,
                          sVal.length,
                          sNextOid,
                          &sVCPieceOID,
                          (SChar**)& sVCPieceHeader) != IDE_SUCCESS );

            /* [1-3] 할당받은 슬롯에 info 정보 저장 및 로깅 *
             *       dirty page 등록은 함수 내부에서 수행됨 */
            IDE_TEST(smrUpdate::updateVarRow(
                         NULL, /* idvSQL* */
                         aTrans,
                         SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                         sVCPieceOID,
                         (SChar*)(sVal.value),
                         sVal.length) != IDE_SUCCESS);


            IDE_TEST( smpVarPageList::setValue(
                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                          sVCPieceOID,
                          sVal.value,
                          sVal.length) != IDE_SUCCESS );

            /* [1-4] 새로 할당받은 슬롯을 versioning list에 추가 */
            IDE_TEST( smLayerCallback::addOID( aTrans,
                                               SMC_CAT_TABLE->mSelfOID,
                                               sVCPieceOID,
                                               SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                               SM_OID_NEW_VARIABLE_SLOT )
                      != IDE_SUCCESS );

            /* [1-5] info 정보가 multiple page에 걸쳐서 저장되는 경우
             *       그 페이지들끼리의 리스트를 유지                  */
            sNextOid = sVCPieceOID;
        }

        sFstVCPieceOID = sVCPieceOID;
    }
    else
    {
        aInfoSize = 0;
        sFstVCPieceOID = SM_NULL_OID;
    }

    sHeaderPageID = SM_MAKE_PID(aTable->mSelfOID);
    sState = 1;

    /* [3] info 변경 및 로깅 */
    IDE_TEST(smrUpdate::updateInfoAtTableHead(
                 NULL, /* idvSQL* */
                 aTrans,
                 SM_MAKE_PID(aTable->mSelfOID),
                 SM_MAKE_OFFSET(aTable->mSelfOID) + SMP_SLOT_HEADER_SIZE,
                 &(aTable->mInfo),
                 sFstVCPieceOID,
                 aInfoSize,
                 SM_VCDESC_MODE_OUT)
             != IDE_SUCCESS);


    aTable->mInfo.length      = aInfoSize;
    aTable->mInfo.fstPieceOID = sFstVCPieceOID;
    aTable->mInfo.flag        = SM_VCDESC_MODE_OUT;

    /* [4] 해당 테이블이 속한 페이지를 변경 페이지로 등록 */
    IDE_TEST(smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           sHeaderPageID)
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (sState == 1)
    {
        IDE_PUSH();
        IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(
                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sHeaderPageID)
                   == IDE_SUCCESS);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table Header에 새로운 Column List정보를 Setting한다. 만약
 *               기존 Column 정보가 있다면 삭제하고 새로운 Column List로 대체한다.
 *               만약 새로운 Column List또한 지정된것이 없다면 Table의 Column
 *               List는 NULL로 Setting된다.
 *
 * aTrans       - [IN] Transaction Pointer
 * aTableHeader - [IN] Table Header Pointer
 * aColumnsList - [IN] Column List   : 지정할 Column이 없다면 NULL로
 * aColumnCnt   - [IN] Column Count  : 지정할 Column이 없다면 0
 * aColumnSize  - [IN] Column Size   : 지정할 Column이 없다면 0
 ***********************************************************************/
IDE_RC smcTable::setColumnAtTableHeader( void*                aTrans,
                                         smcTableHeader*      aTableHeader,
                                         const smiColumnList* aColumnsList,
                                         UInt                 aColumnCnt,
                                         UInt                 aColumnSize )
{
    smOID             sVCPieceOID;
    smOID             sTableOID = aTableHeader->mSelfOID;
    UInt              sLength;
    smiColumnList   * sColumnList;
    const smiColumn * sOrgColumn;
    const smiColumn * sNewColumn;
    UInt              i;
    UInt              sLobColCnt;
    UInt              sUnitedVarColCnt = 0;

    IDE_DASSERT( aTrans         != NULL );
    IDE_DASSERT( aTableHeader   != NULL );

    IDE_ASSERT( aColumnsList != NULL && aColumnSize != 0 );

    /* 기존의 Column List 을 삭제한다. */
    if(aTableHeader->mColumns.fstPieceOID != SM_NULL_OID)
    {
        sColumnList = (smiColumnList *)aColumnsList;

        for( i = 0;
             (( i < aTableHeader->mColumnCount ) && ( sColumnList != NULL ));
             i++, sColumnList = sColumnList->next )
        {
            sOrgColumn = smcTable::getColumn( aTableHeader, i );
            sNewColumn = sColumnList->column;

            if( (sOrgColumn->flag != sNewColumn->flag) &&
                (sOrgColumn->offset != sNewColumn->offset) &&
                (sOrgColumn->size != sNewColumn->size) &&
                ((sOrgColumn->id & ~SMI_COLUMN_ID_MASK) !=
                 (sNewColumn->id & ~SMI_COLUMN_ID_MASK)) )
            {
                break;
            }
            else
            {
                // BUG-44814 DDL 수행시 테이블이 재생성되는 경우 수집된 통계 정보를 DDL 수행전으로 복구해야 합니다.
                // 새로운 smiColumn 을 set 할때 기존의 통계정보를 유지한다.
                idlOS::memcpy( (void*)&sNewColumn->mStat, &sOrgColumn->mStat, ID_SIZEOF(smiColumnStat) );
            }
        }//for

        IDE_TEST_RAISE( ((i != aTableHeader->mColumnCount) && (sColumnList != NULL)),
                        not_match_error );

        if ( ( aTableHeader->mColumns.flag & SM_VCDESC_MODE_MASK ) == SM_VCDESC_MODE_OUT )
        {
            IDE_TEST( smcRecord::deleteVC( aTrans,
                                           SMC_CAT_TABLE,
                                           aTableHeader->mColumns.fstPieceOID )
                    != IDE_SUCCESS );
        }
        else
        {
            /* Nothing to do. inmode 는 fixed record에서 함께 처리된다. */
        }
    }

    IDE_TEST( storeTableColumns( aTrans,
                                 aColumnSize,
                                 aColumnsList,
                                 SMI_GET_TABLE_TYPE( aTableHeader ),
                                 &sVCPieceOID )
              != IDE_SUCCESS );

    // BUG-28356 [QP]실시간 add column 제약 조건에서
    //           lob column에 대한 제약 조건 삭제해야 함
    // Table의 Lob Column의 수를 계산한다.
    sLobColCnt          = 0;
    sUnitedVarColCnt    = 0;
    for( sColumnList = (smiColumnList *)aColumnsList;
         sColumnList != NULL ;
         sColumnList = sColumnList->next )
    {
        sNewColumn = sColumnList->column;
        if ( (sNewColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
        {
            sLobColCnt++;
        }
        else if ( (sNewColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE )
        {
            sUnitedVarColCnt++;
        }
        else
        {
            /* Nothing to do */
        }
    }

    sLength = aColumnCnt * aColumnSize;
    /* ------------------------------------------------
     * table header에 컬럼 정보 변경 및 로깅
     * - SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_UPDATE_COLUMNS
     * ----------------------------------------------*/
    IDE_TEST(smrUpdate::updateColumnsAtTableHead(NULL, /* idvSQL* */
                                                 aTrans,
                                                 SM_MAKE_PID(sTableOID),
                                                 SM_MAKE_OFFSET(sTableOID)
                                                 + SMP_SLOT_HEADER_SIZE,
                                                 &(aTableHeader->mColumns),
                                                 sVCPieceOID,
                                                 sLength,
                                                 SM_VCDESC_MODE_OUT,
                                                 aTableHeader->mLobColumnCount,
                                                 sLobColCnt,
                                                 aTableHeader->mColumnCount,
                                                 aColumnCnt)
             != IDE_SUCCESS);
    aTableHeader->mLobColumnCount = sLobColCnt;
    aTableHeader->mUnitedVarColumnCount = sUnitedVarColCnt;

    // PROJ-1911
    // ALTER TABLE ... ADD COLUMN ... 수행 시,
    // default value가 'NULL'인 칼럼은 column count만 변경할 수 있음
    aTableHeader->mColumnCount = aColumnCnt;


    aTableHeader->mColumns.length = sLength;
    aTableHeader->mColumns.fstPieceOID = sVCPieceOID;
    aTableHeader->mColumns.flag = SM_VCDESC_MODE_OUT;

    /* [4] 해당 테이블이 속한 페이지를 변경 페이지로 등록 */
    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                 SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, SM_MAKE_PID(sTableOID))
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_match_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Column_Mismatch));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(
                   SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, SM_MAKE_PID(sTableOID))
               == IDE_SUCCESS);
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Column List로 부터 Column의 갯수와 Fixed Row의 크기를 구한다.
 *
 * aTableType    - [IN] Table Type
 * aColList      - [IN] Column List
 * aColCnt       - [OUT] Column 갯수
 * aFixedRowSize - [OUT] Fixed Row의 크기
 ***********************************************************************/
IDE_RC smcTable::validateAndGetColCntAndFixedRowSize(const UInt           aTableType,
                                                     const smiColumnList *aColumnList,
                                                     UInt                *aColumnCnt,
                                                     UInt                *aFixedRowSize)
{
    const smiColumn * sColumn;
    UInt  sCount;
    UInt  sColumnEndOffset;
    UInt  sFixedRowSize;
    smiColumnList *sColumnList;

    IDE_DASSERT(aColumnList != NULL);
    IDE_DASSERT(aColumnCnt  != NULL);
    IDE_DASSERT(aFixedRowSize != NULL);

    sColumnList = (smiColumnList *)aColumnList;
    sFixedRowSize = 0;
    sCount = 0;

    while( sColumnList != NULL)
    {
        sColumn  = sColumnList->column;

        if( (sColumn->flag & SMI_COLUMN_COMPRESSION_MASK)
            == SMI_COLUMN_COMPRESSION_FALSE )
        {
            switch(sColumn->flag & SMI_COLUMN_TYPE_MASK)
            {
                case SMI_COLUMN_TYPE_LOB:
 
                    if (aTableType == SMI_TABLE_DISK)
                    {
                        sColumnEndOffset = sColumn->offset + smLayerCallback::getDiskLobDescSize();
                    }
                    else
                    {
                        sColumnEndOffset = sColumn->offset +
                            IDL_MAX( ID_SIZEOF(smVCDescInMode) + sColumn->vcInOutBaseSize,
                                     ID_SIZEOF(smcLobDesc) );
                    }
 
                    break;
 
                case SMI_COLUMN_TYPE_VARIABLE:
 
                    if (aTableType == SMI_TABLE_DISK)
                    {
                        IDE_ASSERT(0);
                    }
                    else
                    {
                        sColumnEndOffset = ID_SIZEOF(smpSlotHeader);
                    }

                    break;

                case SMI_COLUMN_TYPE_VARIABLE_LARGE:

                    if ( aTableType == SMI_TABLE_DISK )
                    {
                        IDE_ASSERT(0);
                    }
                    else
                    {
                        // PROJ-1557 varchar 32k
                        // column end offset = max( vc header_inmode+vcInOutBaseSize, vc header )
                            sColumnEndOffset = sColumn->offset +
                                               IDL_MAX( ID_SIZEOF(smVCDescInMode) + sColumn->vcInOutBaseSize,
                                                        ID_SIZEOF(smVCDesc) );
                            IDE_TEST_RAISE( sColumn->size > SMP_MAX_VAR_COLUMN_SIZE,
                                            err_var_column_size );
                    }
 
                    break;
 
                case SMI_COLUMN_TYPE_FIXED:
 
                    sColumnEndOffset = sColumn->offset + sColumn->size;
 
                    break;
 
                default:
                    IDE_ASSERT(0);
                    break;
            }
        }
        else
        {
            // PROJ-2264
            sColumnEndOffset = sColumn->offset + ID_SIZEOF(smOID);
        }

        if( sColumnEndOffset > sFixedRowSize )
        {
            sFixedRowSize = sColumnEndOffset;
        }

        sCount++;
        sColumnList = sColumnList->next;
    }

    sFixedRowSize = idlOS::align8((UInt)(sFixedRowSize));

    if (aTableType == SMI_TABLE_DISK)
    {
        //PROJ-1705
    }
    else
    {
        IDE_TEST_RAISE( sFixedRowSize > SMP_PERS_PAGE_BODY_SIZE,
                        err_fixed_row_size );

        IDE_TEST_RAISE( sFixedRowSize > SMP_MAX_FIXED_ROW_SIZE,
                        err_fixed_row_size);
    }

    *aColumnCnt = sCount;
    *aFixedRowSize = sFixedRowSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_var_column_size);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_smcVarColumnSizeError));
    }
    IDE_EXCEPTION(err_fixed_row_size);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_smcFixedPageSizeError));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description :  PROJ-1362 QP - Large Record & Internal LOB 지원.
 *                table의 컬럼개수와 인덱스 개수 제약 풀기.
 *  Table의 Column 정보를 Variable slot(들)을 할당하여 저장한다.
 *  - code design
 * 현재 컬럼 piece길이 :=0;
 * 이전 variable slot OID := SM_NULL_OID;
 * SMP_MAX_VAR_SIZE크기 만큼 컬럼 piece버퍼 할당.
 * 현재 column :=  aColumns;
 * while(true)
 * do
 *   if((현재 Column Piece+ mtcColumn) > 컬럼 piece 버퍼 길이 또는 현재 컬럼 == NULL)
 *   then
 *      현재 column piece길이에 해당하는 variable slot 할당;
 *      variable slot body에 대한 physical logging;
 *      현재 컬럼 piece길이 만큼  variable slot으로 memory copy;
 *      할당한 variable slot의 길이를 현재컬럼 piece길이로 설정;
 *      //이전에 할당한 variable slot의 next(mOID)설정작업.
 *      if (이전 variable slot OID != SM_NULL_OID)
 *      then
 *        이전 variable slot의 next(mOID)를 할당된 variable slot으로 변경에
 *                대한 physical logging;
 *        이전 variable slot의 next(mOID)를 할당된 variable slot으로 메모리 변경;
 *         Dirty Page List에 이전 variable slot페이지 추가;
 *      else
 *          table header의 mColumns OID에 등록하기 위하여,
 *             첫번째 variable slot을 기억한다;
 *      fi
 *      트랜잭션 rollback시에 할당된 new variable slot을
 *      memory ager가 remove할수 있도록 SM_OID_NEW_VARIABLE_SLOT을 표시;
 *      Dirty Page List에 현재 할당한 variable slot이 속한 페이지등록;
 *      if(현재 컬럼 == NULL)
 *      then
 *        break;
 *      fi
 *
 *      이전 variable slot OID 갱신;
 *      컬럼 piece buffer 초기화;
 *      현재 column piece 길이 :=0;
 *   fi
 *   mtcColumn크기인 aColumnSize만큼 컬럼 piece buffer에 복사를 한다;
 *   현재 컬럼 piece 버퍼 길이 갱신;
 *  다음 컬럼으로 이동;
 *done  //end while
 ***********************************************************************/
IDE_RC smcTable::storeTableColumns( void*                  aTrans,
                                    UInt                   aColumnSize,
                                    const smiColumnList*   aColumns,
                                    UInt                   aTableType,
                                    smOID*                 aHeadVarOID)
{
    ULong             sColumnsPiece[SMP_VC_PIECE_MAX_SIZE / ID_SIZEOF(ULong)];
    smiColumnList*    sColumns;
    const smiColumn*  sColumn;
    UInt              i;
    smVCPieceHeader*  sVCPieceHeader        = NULL;
    smOID             sVCPieceOID           = SM_NULL_OID;
    smVCPieceHeader*  sPreVCPieceHeader     = NULL;
    smOID             sPreVCPieceOID        = SM_NULL_OID;
    smVCPieceHeader   sAftVCPieceHeader;
    SChar*            sVarData;
    UInt              sCurColumnsPieceLen = 0;
    UInt              sPieceType;

    IDE_DASSERT( aTrans         != NULL );
    IDE_DASSERT( aColumns       != NULL );
    IDE_DASSERT( aHeadVarOID    != NULL );

    sColumns = (smiColumnList*) aColumns;
    *aHeadVarOID = SM_NULL_OID;

    //컬럼 piece buffer 초기화 .
    idlOS::memset((SChar*)sColumnsPiece,0x00,SMP_VC_PIECE_MAX_SIZE);
    for(i=0;  ;i++ )
    {
        // 테이블 컬럼갯수가 SMI_COLUMN_ID_MAXIMUM을 초과하면 에러.
        IDE_TEST_RAISE( i > SMI_COLUMN_ID_MAXIMUM,
                        maximum_column_count_error );
        // (현재 Column Piece+ mtcColumn) > 컬럼 piece 버퍼길이 또는 현재 컬럼 == NULL 이면
        // variable slot할당 및 부분 컬럼 정보 저장.
        if( (( sCurColumnsPieceLen + aColumnSize ) > SMP_VC_PIECE_MAX_SIZE )
            || ( sColumns == NULL ) )
        {
            if( aTableType == SMI_TABLE_DISK )
            {
                sPieceType = SM_VCPIECE_TYPE_DISK_COLUMN;
            }
            else
            {
                sPieceType = SM_VCPIECE_TYPE_MEMORY_COLUMN;
            }

            //현재 column piece길이에 해당하는 variable slot 할당.
            IDE_TEST( smpVarPageList::allocSlot(
                          aTrans,
                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                          SMC_CAT_TABLE->mSelfOID,
                          SMC_CAT_TABLE->mVar.mMRDB,
                          sCurColumnsPieceLen,
                          SM_NULL_OID,
                          &sVCPieceOID,
                          (SChar**)&sVCPieceHeader,
                          sPieceType ) != IDE_SUCCESS);

            //variable slot body에 대한 physical logging.
            IDE_TEST(smrUpdate::updateVarRow(
                         NULL, /* idvSQL* */
                         aTrans,
                         SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                         sVCPieceOID,
                         (SChar*)sColumnsPiece,
                         sCurColumnsPieceLen) != IDE_SUCCESS);


            //현재 컬럼 piece길이 만큼  variable slot으로 memory copy.
            sVarData = (SChar*)(sVCPieceHeader+1);

            idlOS::memcpy(sVarData,(SChar*)sColumnsPiece, sCurColumnsPieceLen);

            //할당한 variable slot의 길이를 현재컬럼 piece길이로 설정.
            sVCPieceHeader->length = sCurColumnsPieceLen;

            //이전에 할당한 variable slot의 next(mOID)설정작업.
            if(sPreVCPieceOID != SM_NULL_OID)
            {
                IDE_ASSERT( smmManager::getOIDPtr( 
                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                sPreVCPieceOID,
                                (void**)&sPreVCPieceHeader )
                            == IDE_SUCCESS );

                sAftVCPieceHeader = *(sPreVCPieceHeader);

                sAftVCPieceHeader.nxtPieceOID = sVCPieceOID;

                //이전 variable slot의 next(mOID)를 할당된 variable slot으로 변경에
                //대한 physical logging.
                IDE_TEST( smrUpdate::updateVarRowHead(NULL, /* idvSQL* */
                                                      aTrans,
                                                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                      sPreVCPieceOID,
                                                      sPreVCPieceHeader,
                                                      &sAftVCPieceHeader)
                          != IDE_SUCCESS);


                //이전 variable slot의 next(mOID)를 할당된 variable slot으로 메모리 변경.
                sPreVCPieceHeader->nxtPieceOID = sVCPieceOID;
                //Dirty Page List에 이전 variable slot페이지 추가.
                IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                              SM_MAKE_PID(sPreVCPieceOID))
                          != IDE_SUCCESS);
            }//if sPreVCPieceOID  != SM_NULL_OID
            else
            {
                //table header의 mColumns OID에 등록하기 위하여,
                //첫번째 variable slot을 기억한다.
                *aHeadVarOID = sVCPieceOID;
            }//else
            //트랜잭션 rollback시에 할당된 new variable slot을
            //memory ager가 remove할수 있도록 SM_OID_NEW_VARIABLE_SLOT을 표시.
            IDE_TEST( smLayerCallback::addOID(
                          aTrans,
                          SMC_CAT_TABLE->mSelfOID,
                          sVCPieceOID,
                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                          SM_OID_NEW_VARIABLE_SLOT)
                      != IDE_SUCCESS);

            //Dirty Page List에 현재 할당한 variable slot이 속한 페이지등록.
            IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                     SM_MAKE_PID(sVCPieceOID) )
                      != IDE_SUCCESS );
            // 컬럼 list를 끝까지 갔다.
            if(sColumns == NULL)
            {
                break;
            }//if sColumns == NULL

            //이전 variable slot OID 갱신.
            sPreVCPieceOID = sVCPieceOID;
            //컬럼 piece buffer 초기화 .
            idlOS::memset((SChar*)sColumnsPiece, 0x00, SMP_VC_PIECE_MAX_SIZE);
            sCurColumnsPieceLen = 0;
        }//if

        sColumn = sColumns->column;

        //mtcColumn크기인 aColumnSize만큼 column piece buffer에 복사를 한다.
        idlOS::memcpy((SChar*)sColumnsPiece+sCurColumnsPieceLen,sColumn,aColumnSize);

        if( (sColumn->id & SMI_COLUMN_ID_MASK) == 0)
        {
            ((smiColumn *)( (SChar*)sColumnsPiece+sCurColumnsPieceLen))->id = (sColumn->id)|i;
        }//if
        else
        {
            IDE_TEST_RAISE( ((sColumn->id & SMI_COLUMN_ID_MASK) != i),
                            id_value_error );

        }//else
        //현재 컬럼 piece 버퍼 길이 갱신.
        sCurColumnsPieceLen += aColumnSize;
        //다음 컬럼으로 이동.
        sColumns = sColumns->next;
    }//for

    return IDE_SUCCESS;

    IDE_EXCEPTION( id_value_error );
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Invalid_ID_Value));
    }

    IDE_EXCEPTION( maximum_column_count_error );
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Maximum_Column_count));
    }

    IDE_EXCEPTION_END;


    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  PROJ-1362 QP - Large Record & Internal LOB 지원.
 *                table의 컬럼개수와 인덱스 개수 제약 풀기.
 *  테이블의 컬럼정보를 담는 variable slot들을 해제하고,
 *  테이블 헤더의 mColumns.mOID를 null oid로 설정한다.
 *  - code design
 *  마지막 variable slot부터 처음 variable 까지 역순으로 slot을 해제하기
 *  위하여 OID stack을 구성;
 *  while( stack이 empty가 아닐때 까지)
 *  do
 *    transaction의  last undo next LSN을 기억;
 *    if(stack의 현재 item의 이전 item이 존재 하는가 ?)
 *    then
 *       현재 variable slot의 이전 slot next(mOID)를 null로 변경하는
 *       physical logging;
 *       현재 variable slot의 이전 slot next(mOID)를 null로 변경;
 *       Dirty Page List에 이전 variable slot페이지 추가;
 *    else
 *         table header의  mColumns OID를  SM_NULL_OID으로 변경하는
 *         physical logging;
 *         table header의  mColumns OID를  SM_NULL_OID으로 변경;
 *    fi
 *    catalog table에 현재 variable slot을 해제하여 반환함;
 *  done
 *  stack memory  해제;
 ***********************************************************************/
IDE_RC smcTable::freeColumns(idvSQL          *aStatistics,
                             void*           aTrans,
                             smcTableHeader* aTableHeader)
{
    SInt                i;
    smLSN               sLsnNTA2;
    smcOIDStack         sOIDStack;
    smOID               sCurOID;
    smVCPieceHeader   * sPreVCPieceHeader;
    smVCPieceHeader     sAftVCPieceHeader;
    smcFreeLobSegFunc   sLobSegFunc;
    SChar             * sCurPtr;
    UInt                sState =0;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aTableHeader != NULL );
    sLobSegFunc = ( SMI_TABLE_TYPE_IS_DISK( aTableHeader ) == ID_TRUE )
                  ? smcTable::freeLobSegFunc : smcTable::freeLobSegNullFunc;

    /* smcTable_freeColumns_malloc_Arr.tc */
    IDU_FIT_POINT("smcTable::freeColumns::malloc::Arr");
    IDE_TEST( iduMemMgr::malloc(
                  IDU_MEM_SM_SMC,
                  ID_SIZEOF(smOID) * getMaxColumnVarSlotCnt(aTableHeader->mColumnSize),
                  (void**) &(sOIDStack.mArr))
              != IDE_SUCCESS);
    sState =1;

    buildOIDStack(aTableHeader, &sOIDStack);

    for( i = (SInt)(sOIDStack.mLength -1); i  >=0 ; i--)
    {
        sCurOID =   sOIDStack.mArr[i];
        IDE_TEST( sLobSegFunc(aStatistics,
                              aTrans,
                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                              sCurOID,
                              aTableHeader->mColumnSize,
                              SDR_MTX_LOGGING) != IDE_SUCCESS);

        sLsnNTA2 = smLayerCallback::getLstUndoNxtLSN( aTrans );
        if( (i -1)  >= 0)
        {
            /* 현재 variable slot의 이전 slot next(mOID)를 null로
               변경하는 physical logging. */
            IDE_ASSERT( smmManager::getOIDPtr( 
                            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                            sOIDStack.mArr[i-1],
                            (void**)&sPreVCPieceHeader )
                        == IDE_SUCCESS );

            sAftVCPieceHeader = *(sPreVCPieceHeader);

            sAftVCPieceHeader.nxtPieceOID = SM_NULL_OID;

            IDE_TEST( smrUpdate::updateVarRowHead(NULL, /* idvSQL* */
                                                  aTrans,
                                                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                  sOIDStack.mArr[i-1],
                                                  sPreVCPieceHeader,
                                                  &sAftVCPieceHeader)
                      != IDE_SUCCESS);

            //현재 variable slot의 이전 slot next(mOID)를 null로 변경.
            sPreVCPieceHeader->nxtPieceOID = SM_NULL_OID;

            //Dirty Page List에 이전 variable slot페이지 추가.
            IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                          SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                          SM_MAKE_PID(sOIDStack.mArr[i-1]))
                      != IDE_SUCCESS);
        }
        else
        {
            // table header의 mColumns 의 시작 variable slot.
            /* ------------------------------------------------
             *  table header의  column 정보에 SM_NULL_OID를 설정
             * - SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_UPDATE_COLUMNS 로깅
             * ----------------------------------------------*/
            IDE_TEST( smrUpdate::updateColumnsAtTableHead(NULL, /* idvSQL* */
                                                          aTrans,
                                                          SM_MAKE_PID(aTableHeader->mSelfOID),
                                                          SM_MAKE_OFFSET(aTableHeader->mSelfOID)
                                                          +SMP_SLOT_HEADER_SIZE,
                                                          &(aTableHeader->mColumns),
                                                          SM_NULL_OID,
                                                          0,
                                                          SM_VCDESC_MODE_OUT,
                                                          aTableHeader->mLobColumnCount,
                                                          0, /* aAfterLobColumnCount */
                                                          aTableHeader->mColumnCount,
                                                          aTableHeader->mColumnCount)
                      != IDE_SUCCESS );


            aTableHeader->mColumns.fstPieceOID  = SM_NULL_OID;
            aTableHeader->mColumns.length = 0;
            aTableHeader->mColumns.flag = SM_VCDESC_MODE_OUT;
        }//else

        IDE_ASSERT( smmManager::getOIDPtr( 
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                        sCurOID,
                        (void**)&sCurPtr )
                    == IDE_SUCCESS );

        /* ------------------------------------------------
         * catalog table에 variable slot을 해제하여 반환함.
         * ----------------------------------------------*/
        IDE_TEST( smpVarPageList::freeSlot(aTrans,
                                           SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SMC_CAT_TABLE->mVar.mMRDB,
                                           sCurOID,
                                           sCurPtr,
                                           &sLsnNTA2,
                                           SMP_TABLE_NORMAL)
              != IDE_SUCCESS );



    }//for i

    IDE_TEST( iduMemMgr::free(sOIDStack.mArr) != IDE_SUCCESS);


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    {
        if( sState == 1)
        {
            IDE_ASSERT( iduMemMgr::free(sOIDStack.mArr) == IDE_SUCCESS);

        }//if
    }
    return IDE_FAILURE;

}

/***********************************************************************
 * Description :  PROJ-1362 QP - Large Record & Internal LOB 지원.
 *                table의 컬럼개수와 인덱스 개수 제약 풀기.
 *  마지막 variable slot부터 처음 variable 까지 역순으로 slot을 해제하기
 *  위하여 OID stack을 구성한다.
 ***********************************************************************/
void smcTable::buildOIDStack(smcTableHeader* aTableHeader,
                             smcOIDStack*    aOIDStack)
{

    smOID            sVCPieceOID;
    smVCPieceHeader *sVCPieceHdr;
    UInt             i;

    for( i=0, sVCPieceOID =  aTableHeader->mColumns.fstPieceOID;
         sVCPieceOID != SM_NULL_OID ; i++ )
    {
        ID_SERIAL_BEGIN(aOIDStack->mArr[i] = sVCPieceOID);

        // BUG-27735 : Peterson Algorithm
        ID_SERIAL_EXEC( IDE_ASSERT( smmManager::getOIDPtr( 
                            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                            sVCPieceOID,
                            (void**)&sVCPieceHdr )
                        == IDE_SUCCESS ), 
            1 );

        ID_SERIAL_END(sVCPieceOID =  sVCPieceHdr->nxtPieceOID);
    }//for

    aOIDStack->mLength = i;
}

IDE_RC smcTable::freeLobSegNullFunc(idvSQL*           /*aStatistics*/,
                                    void*             /*aTrans*/,
                                    scSpaceID         /*aSpaceID*/,
                                    smOID             /*aColSlotOID*/,
                                    UInt              /*aColumnSize */,
                                    sdrMtxLogMode     /*aLoggingMode*/)
{
    return IDE_SUCCESS;

}


IDE_RC smcTable::freeLobSegFunc(idvSQL*           aStatistics,
                                void*             aTrans,
                                scSpaceID         aColSlotSpaceID,
                                smOID             aColSlotOID,
                                UInt              aColumnSize,
                                sdrMtxLogMode     aLoggingMode)
{
    smOID            sColOID;
    smVCPieceHeader* sVCPieceHeader;
    smiColumn*       sLobCol;
    UChar*           sLobColumn;
    UInt             i;
    idBool           sInvalidTBS=ID_FALSE;

    IDE_DASSERT( aColSlotSpaceID == SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC );

    IDE_ASSERT( smmManager::getOIDPtr( 
                    aColSlotSpaceID,
                    aColSlotOID,
                    (void**)&sVCPieceHeader )
                == IDE_SUCCESS );

    sLobColumn = (UChar*)(sVCPieceHeader+1);
    sColOID = aColSlotOID+ ID_SIZEOF(smVCPieceHeader);

    for( i = 0 ;  i <  sVCPieceHeader->length; i+=  aColumnSize)
    {
        sLobCol = (smiColumn*)sLobColumn;

        //다른 TBS에 lob column이 있을경우 그 tbs가 valid한지 검사해야만함.
        sInvalidTBS = sctTableSpaceMgr::hasState( sLobCol->colSpace,
                                                  SCT_SS_INVALID_DISK_TBS );
        if( sInvalidTBS == ID_TRUE )
        {
            continue;
        }

        // XXX 언제 colSeg가 NULL이 되는가? refine 시 colSeg가 NULL이었지만
        // undo시 이 값이 원복된다.
        if( ( SMI_IS_LOB_COLUMN( sLobCol->flag ) == ID_TRUE )  &&
            ( SC_GRID_IS_NOT_NULL( sLobCol->colSeg ) ) )
        {
            IDE_TEST( sdpSegment::freeLobSeg(aStatistics,
                                             aTrans,
                                             sColOID,
                                             sLobCol,
                                             aLoggingMode)
                      != IDE_SUCCESS);
        }

        sColOID    += aColumnSize;
        sLobColumn += aColumnSize;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/***********************************************************************
 * Description : table에 Lock획득
 ***********************************************************************/
IDE_RC smcTable::lockTable( void                * aTrans,
                            smcTableHeader      * aHeader,
                            sctTBSLockValidOpt    aTBSLvOpt,
                            UInt                  aFlag )
{
    idBool           sLocked;
    UInt             sLockFlag;
    iduMutex       * sMutex;

    /* ------------------------------------------------
     * [1] Drop하고자 하는 Table에 대하여 X lock 요청
     * ----------------------------------------------*/
    sLockFlag = aFlag & SMC_LOCK_MASK;

    if(sLockFlag == SMC_LOCK_TABLE)
    {

        // PRJ-1548 User Memory Tablespace
        // 테이블의 테이블스페이스들에 대하여 INTENTION 잠금을 획득한다.
        IDE_TEST( sctTableSpaceMgr::lockAndValidateTBS(
                      (void*)aTrans,                  /* smxTrans * */
                      getTableSpaceID((void*)aHeader),/* smcTableHeader* */
                      aTBSLvOpt,          /* 테이블스페이스 Validation 옵션 */
                      ID_TRUE,            /* intent lock  여부 */
                      ID_TRUE,            /* exclusive lock 여부 */
                      sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aTrans))
                  != IDE_SUCCESS );

        IDE_TEST( smLayerCallback::lockTableModeXAndCheckLocked( aTrans,
                                                                 SMC_TABLE_LOCK( aHeader ),
                                                                 &sLocked )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( sLocked == ID_FALSE, already_locked_error );
    }
    else
    {
        if(sLockFlag == SMC_LOCK_MUTEX)
        {
            sMutex = smLayerCallback::getMutexOfLockItem( SMC_TABLE_LOCK( aHeader ) ) ;
            IDE_TEST_RAISE( sMutex->lock( NULL ) != IDE_SUCCESS,
                            mutex_lock_error );

            smLayerCallback::addMutexToTrans( aTrans,
                                              (void*)sMutex );
        }
        else
        {
            IDE_ASSERT(0);
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( mutex_lock_error );
    {
        IDE_SET(ideSetErrorCode (smERR_FATAL_ThrMutexLock));
    }
    IDE_EXCEPTION(already_locked_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Already_Locked));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table의 유효성 검사

   PRJ-1548 User Memory TableSpace 개념도입
   Validate Table 에서 다음과 같이 Lock을 획득한다.
   [1] Table의 TableSpace에 대해 IX
   [2] Table에 대해 X
   [3] Table의 Index, Lob Column TableSpace에 대해 IX
   Table의 상위는 Table의 TableSpace이며 그에 대해 IX를
   획득한다.

 ***********************************************************************/
IDE_RC smcTable::validateTable( void                * aTrans,
                                smcTableHeader      * aHeader,
                                sctTBSLockValidOpt    aTBSLvOpt,
                                UInt                  aFlag )
{
    /* ------------------------------------------------
     * [1] TableSpace에 대하여 IX lock 요청
     * [2] Table에 대하여 X lock 요청
     * ----------------------------------------------*/
    IDE_TEST( lockTable( aTrans,
                         aHeader,
                         aTBSLvOpt,
                         aFlag ) != IDE_SUCCESS );

    /* Table Space에 대해서 DDL이 발생했다는것을 표시*/
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       aHeader->mSelfOID,
                                       aHeader->mSelfOID,
                                       aHeader->mSpaceID,
                                       SM_OID_DDL_TABLE )
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * [3] validation
     * 제거된 table인지 확인한다.
     * ----------------------------------------------*/
    IDE_TEST_RAISE( SMP_SLOT_IS_DROP( ((smpSlotHeader *)aHeader-1) ),
                    table_not_found_error );

    if ( aFlag == SMC_LOCK_TABLE )
    {
        // fix BUG-17212
        // 테이블의 Index, Lob 컬럼 관련 테이블스페이스들에 대하여
        // INTENTION 잠금을 획득한다.
        /* ------------------------------------------------
         * [1] TableSpace에 대하여 IX lock 요청
         * ----------------------------------------------*/
        IDE_TEST( sctTableSpaceMgr::lockAndValidateRelTBSs(
                  (void*)aTrans,    /* smxTrans* */
                  (void*)aHeader,   /* smcTableHeader */
                  aTBSLvOpt,        /* TBS Lock Validation Option */
                  ID_TRUE,          /* intent lock  여부 */
                  ID_TRUE,          /* exclusive lock 여부 */
                  sctTableSpaceMgr::getDDLLockTimeOut((smxTrans*)aTrans)) /* wait lock timeout */
                != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(table_not_found_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Table_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table 제거 (공통)
 * 테이블 타입에 따라서 pending operation을 등록하여야 한다.
 * memory table의 경우에는 ager에 의해 aging 수행시 smcTable::dropTablePending이
 * 호출 되며, disk table의 경우에는 TSS에 pending operation에 대한 table OID
 * 설정하여 disk G.C에 의해 처리하도록 한다.
 *
 * - 2nd. code design
 *   + table validation 처리
 *   + table header의 drop flag를 설정 및 로깅
 *     : SMR_LT_UPDATE의 SMR_SMC_PERS_SET_FIX_ROW_DROP_FLAG
 *     : drop flag에 SMP_SLOT_DROP_TRUE 설정
 *     : dirtypage 등록
 *   + table 타입에 따라 drop pending 연산 등록(temporary table은 제외)
 *     if (memory table)
 *       : ager에 의해 처리
 *       - SM_OID_DROP_TABLE 표시하여 트랜잭션 commit시에
 *         slot header에 delete bit를 설정하고, ager에 의해
 *         실제 drop 하게 한다.
 *       - 만약 table이 unpin 상태라면, ager에 의해서 table
 *          backup file을 제거하도록 한다.
 *     else if (disk table)
 *       : disk G.C에 의해 처리
 *       - mtx 시작 (SMR_LT_DRDB, SMR_OP_NULL)
 *       - 트랜잭션으로부터 tss RID를 얻는다.
 *       - 해당 tss RID가 포함된 page를 X_LATCH 모드로 fix 시킨다.
 *       - tss에 pending flag(SDR_4BYTES)를 설정 및 로깅
 *       - 제거할 table OID를 설정 및 로깅 (SDR_8BYTES)
 *       - mtx commit
 *     endif
 *
 * - RECOVERY
 *
 *  # table 제거 (* disk-table 제거)
 *
 *  (0) (1)            (2)            (3)      (4)
 *   |---|--------------|--------------|--------|--------> time
 *       |              |              |        |
 *     *alloc/init      set           set      commit
 *     tss              drop-flag     pending
 *     (R)              on tbl-slot   flag
 *                      (R/U)         to tss
 *                                    (R)
 *
 *  # Disk-Table Recovery Issue
 *
 *  - (1)까지 로깅하였다면
 *    + smxTrans::abort 수행한다.
 *      : 이때 할당되었던 tss에 대한 free를 수행한다.
 *
 *  - (2)까지 로깅하였다면
 *    + (2)에 대한 undo를 처리한다.
 *    + smxTrans::abort 수행한다.
 *
 *  - (3)까지 로깅하였다면
 *    + (2)에 대한 undo를 처리한다.
 *    + smxTrans::abort 수행한다.
 *
 *  - commit되었다면 tx commit 과정에서
 *    tbl-hdr의 slot의 SCN에 delete bit를 설정한다.
 *    그 후, disk GC에 의해서 drop table pending 연산을
 *    수행한다. dropTablePending은 다음과 같다. (b)(c)
 *    : 더 자세한 사항은 dropTablePending을 참고
 *
 *      (a)    (b)                 (c)
 *       |------|-------------------|-----------> time
 *              |                   |
 *              update             free
 *              seg rID,           segment
 *              meta pageID        [NTA->(a)]
 *              in tbl-hdr         (NTA OP NULL)
 *              (R/U)
 *
 *      ㄱ. (b)까지만 로깅했다면 (b)로그에 대하여 undo가 수행되며 다음과
 *          같이 처리된다.
 *          => (a)->(b)->(b)'->(a)'->(a)->(b)->(c)->...
 *             NTA            CLR   CLR
 *      ㄴ. (c)까지 mtx commit이 되었다면
 *          => (5)->(a)->(b)->(c)->(d)->..->(5)'->...
 *             NTA                          DUMMY_CLR
 *
 ***********************************************************************/
IDE_RC smcTable::dropTable( void               * aTrans,
                            const void         * aHeader,
                            sctTBSLockValidOpt   aTBSLvOpt,
                            UInt                 aFlag )
{
#ifdef DEBUG
    UInt              sTableType;
#endif
    UInt              sState  = 0;
    scPageID          sPageID = 0;
    smcTableHeader*   sHeader;

    sHeader = (smcTableHeader*)aHeader;

#ifdef DEBUG
    sTableType = SMI_GET_TABLE_TYPE( sHeader );
#endif
    IDE_DASSERT( sTableType == SMI_TABLE_MEMORY ||
                 sTableType == SMI_TABLE_META   ||
                 sTableType == SMI_TABLE_DISK   ||
                 sTableType == SMI_TABLE_VOLATILE ||
                 sTableType == SMI_TABLE_REMOTE );

    // [1] 유효한 table인지 확인하고 drop할 수 있는 준비를 한다.
    IDE_TEST( validateTable(aTrans,
                            sHeader,
                            aTBSLvOpt,
                            aFlag) != IDE_SUCCESS );

    /* ------------------------------------------------
     * [2] table Header에서 m_drop_flag를 설정한다.
     * - Update Type: SMR_SMP_PERS_PRE_DELETE_ROW
     * - table header가 저장된 fixed slot에 delete flag 설정
     * ----------------------------------------------*/
    sPageID = SM_MAKE_PID(sHeader->mSelfOID);
    sState  = 1;

    IDE_TEST( smrUpdate::setDropFlagAtFixedRow(
                                    NULL, /* idvSQL* */
                                    aTrans,
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                    sHeader->mSelfOID,
                                    ID_TRUE)
              != IDE_SUCCESS);

    //IDU_FIT_POINT("smcTable::dropTable::wait1");
    IDU_FIT_POINT( "1.PROJ-1552@smcTable::dropTable" );

    SMP_SLOT_SET_DROP( (smpSlotHeader *)sHeader-1 );

    /* ------------------------------------------------
     * [3] Drop하고자 하는 Table을 Pending Operation List에 추가한다.
     * memory table은 ager에 등록하고, disk table은 tss에 등록한다.
     * ----------------------------------------------*/
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       sHeader->mSelfOID,
                                       ID_UINT_MAX,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_DROP_TABLE )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                             sPageID ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
            IDE_PUSH();
            (void)smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                                 sPageID );
            IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 
 * 1.alter table 과정에서 exception발생으로 인한 undo 하는 루틴이다
 *   disk table에서는 고려할 필요없다.
 *   정상수행 중에 발생한 abort에 대한 Logical undo를 수행한다.
 * 2.통계정보갱신후 Table의 Plan 재구축을 위해서 사용한다.
 ***********************************************************************/
void  smcTable::changeTableSCNForRebuild( smOID aTableOID )
{
    smSCN            sLstSystemSCN;
    smpSlotHeader  * sSlotHeader;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                       aTableOID,
                                       (void**)&sSlotHeader ) 
                == IDE_SUCCESS );

    sLstSystemSCN = smmDatabase::mLstSystemSCN;
    SM_SET_SCN( &(sSlotHeader->mCreateSCN), &sLstSystemSCN );

    return;
}

/***********************************************************************
 * Description : for PR-4360 ALTER TABLE
 *
 * - table의 할당된 fixed/var pagelist를 모두 free하여
 *   system에 메모리 반환한다.
 * - memory table의 alter table 기능처리에만 사용되는 함수로써
 *   disk table에 대하여 지원하지 않아도 되는 함수
 *
 * BUG-30457  memory volatile 테이블에 대한 alter시
 * 공간 부족으로 rollback이 부족한 상황이면 비정상
 * 종료합니다.
 *
 * 본 연산은 AlterTable 중 원본 Table Backup 후 호출된다.
 * 따라서 이 연산의 Undo시에는, 새 Table Restore시 사용한
 * Page들을 DB로 돌려줄 수 있도록, 새 Table(aDstHeader)를
 * Logging한다. ==> BUG-34438에의해 smiTable::backupTableForAlterTable()로
 * 이동함
 ***********************************************************************/

IDE_RC smcTable::dropTablePageListPending( void           * aTrans,
                                           smcTableHeader * aHeader,
                                           idBool           aUndo )
{
    smLSN               sLsnNTA;
    UInt                sState = 0;

    IDE_ASSERT( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE );

    // for NTA
    sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );
    sState = 1;

    /* PROJ-1594 Volatile TBS */
    /* volatile table인지 memory table인지 판단하여 로직을 분기한다. */
    if( SMI_TABLE_TYPE_IS_VOLATILE( aHeader ) == ID_TRUE )
    {
        IDE_TEST( svpFixedPageList::freePageListToDB( aTrans,
                                                      aHeader->mSpaceID,
                                                      aHeader->mSelfOID,
                                                      &(aHeader->mFixed.mVRDB) )
                  != IDE_SUCCESS );

        (aHeader->mFixed.mVRDB.mRuntimeEntry)->mInsRecCnt = 0;
        (aHeader->mFixed.mVRDB.mRuntimeEntry)->mDelRecCnt = 0;

        IDE_TEST( svpVarPageList::freePageListToDB( aTrans,
                                                    aHeader->mSpaceID,
                                                    aHeader->mSelfOID,
                                                    aHeader->mVar.mVRDB )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( smpFixedPageList::freePageListToDB( aTrans,
                                                      aHeader->mSpaceID,
                                                      aHeader->mSelfOID,
                                                      &(aHeader->mFixed.mMRDB) )
                  != IDE_SUCCESS );

        (aHeader->mFixed.mMRDB.mRuntimeEntry)->mInsRecCnt = 0;
        (aHeader->mFixed.mMRDB.mRuntimeEntry)->mDelRecCnt = 0;

        IDE_TEST( smpVarPageList::freePageListToDB( aTrans,
                                                    aHeader->mSpaceID,
                                                    aHeader->mSelfOID,
                                                    aHeader->mVar.mMRDB )
                  != IDE_SUCCESS );
    }
	
    IDU_FIT_POINT( "1.BUG-34438@smiTable::dropTablePageListPending" );

    IDE_TEST( smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                        aTrans,
                                        &sLsnNTA,
                                        SMR_OP_NULL)
              != IDE_SUCCESS );


    sState = 0;
    /* BUG-30871 When excuting ALTER TABLE in MRDB, the Private Page Lists of
       new and old table are registered twice. */
    /* AllocPageList 및 FreePageList를 때어낼때, Transactino의
     * PrivatePageList도 때어내야 합니다. */
    IDE_TEST( smLayerCallback::dropMemAndVolPrivatePageList( aTrans, aHeader )
              != IDE_SUCCESS );

    if ( aUndo == ID_FALSE )
    {
        IDE_TEST( smLayerCallback::dropIndexes( aHeader ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        if(ideIsAbort() == IDE_SUCCESS)
        {
            IDE_PUSH();
            (void)smrRecoveryMgr::undoTrans(NULL, /* idvSQL* */
                                            aTrans,
                                            &sLsnNTA);
            IDE_POP();
        }
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : max rows를 변경하거나, column의 default value를 변경
 ***********************************************************************/
IDE_RC smcTable::modifyTableInfo( void                 * aTrans,
                                  smcTableHeader       * aHeader,
                                  const smiColumnList  * aColumns,
                                  UInt                   aColumnSize,
                                  const void           * aInfo,
                                  UInt                   aInfoSize,
                                  UInt                   aFlag,
                                  ULong                  aMaxRow,
                                  UInt                   aParallelDegree,
                                  sctTBSLockValidOpt     aTBSLvOpt,
                                  idBool                 aIsInitRowTemplate )
{
    ULong             sRecordCount = 0;
    UInt              sState = 0;
    scPageID          sHeaderPageID = 0;
    UInt              sTableFlag;
    UInt              sColumnCount = 0;
    UInt              sFixedRowSize;
    UInt              sBfrColCnt;
    UInt              i;
    smiColumn       * sColumn;
    smOID             sLobColOID;
    smnIndexHeader  * sIndexHeader;
    smnIndexModule  * sIndexModule;

    if(aMaxRow != ID_ULONG_MAX && aMaxRow != 0)
    {
        IDE_TEST(getRecordCount(aHeader, &sRecordCount)
                 != IDE_SUCCESS);

        IDE_TEST_RAISE(sRecordCount > aMaxRow, err_invalid_maxrow);
    }

    // DDL이 이므로 Table Validation을 수행한다.
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt,
                             SMC_LOCK_TABLE )
              != IDE_SUCCESS );

    if( aColumns != NULL )
    {
        /* Column List가 Valid한지 검사하고 Column의 갯수와 Fixed row의 크기
           를 구한다.*/
        IDE_TEST( validateAndGetColCntAndFixedRowSize(
                      SMI_GET_TABLE_TYPE( aHeader ),
                      aColumns,
                      &sColumnCount,
                      &sFixedRowSize)
                  != IDE_SUCCESS );

        sBfrColCnt = aHeader->mColumnCount;

        /* 이 Table의 Column의 정보를 Variable영역에 설정한다. */
        IDE_TEST( setColumnAtTableHeader( aTrans,
                                          aHeader,
                                          aColumns,
                                          sColumnCount,
                                          aColumnSize )
                  != IDE_SUCCESS );

        if ( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_TRUE )
        {
            /* BUG-28356 [QP]실시간 add column 제약 조건에서
             *           lob column에 대한 제약 조건 삭제해야 함
             * 실시간 Add Disk LOB Column 시 Lob Segment도 생성해 주어야 합니다.
             * 기존 Column들 이후에 새로 추가된 Column들을 순회하면서
             * LOB Column이 존재하면 LOB Segment를 생성합니다. */
            if ( aHeader->mLobColumnCount != 0  )
            {
                for ( i = sBfrColCnt ; i < aHeader->mColumnCount ; i++ )
                {
                    sColumn = getColumnAndOID( aHeader,
                                               i, // Column Idx
                                               &sLobColOID );
 
                    if ( (sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_LOB )
                    {
                        SC_MAKE_NULL_GRID( sColumn->colSeg );
                        IDE_TEST( sdpSegment::allocLobSeg4Entry(
                                      smxTrans::getStatistics( aTrans ),
                                      aTrans,
                                      sColumn,
                                      sLobColOID,
                                      aHeader->mSelfOID,
                                      SDR_MTX_LOGGING ) != IDE_SUCCESS );
                    }
                }
            }

            /* BUG-31949 [qp-ddl-dcl-execute] failure of index visibility check
             * after abort real-time DDL in partitioned table. */
            for( i = 0 ; i < smcTable::getIndexCount( aHeader ) ; i ++ )
            {
                sIndexHeader =
                    (smnIndexHeader*) smcTable::getTableIndex( aHeader, i );

                sIndexModule = (smnIndexModule*)sIndexHeader->mModule;
                IDE_TEST( sIndexModule->mRebuildIndexColumn(
                            sIndexHeader,
                            aHeader,
                            sIndexHeader->mHeader )
                    != IDE_SUCCESS );
            }

            if ( aIsInitRowTemplate == ID_TRUE )
            {
                /* PROJ-2399 Row Template
                 * 업데이트된 column정보를 바탕으로 RowTemplate를 재구성 한다. */
                IDE_TEST( destroyRowTemplate( aHeader )!= IDE_SUCCESS );
                IDE_TEST( initRowTemplate( NULL,  /* aStatistics */
                                           aHeader,
                                           NULL ) /* aActionArg */
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        aColumnSize = aHeader->mColumnSize;
    }



    if( aInfo != NULL )
    {
        /* 새로운 Table의 Info 정보를 위하여 값을 설정한다 */
        IDE_TEST(setInfoAtTableHeader(aTrans, aHeader, aInfo, aInfoSize)
                 != IDE_SUCCESS);
    }

    sHeaderPageID = SM_MAKE_PID(aHeader->mSelfOID);
    sState = 1;

    if( aFlag == SMI_TABLE_FLAG_UNCHANGE )
    {
        sTableFlag = aHeader->mFlag;
    }
    else
    {
        sTableFlag = aFlag;
    }

    if(aMaxRow == 0)
    {
        aMaxRow = aHeader->mMaxRow;
    }

    if ( aParallelDegree == 0 )
    {
        // Parallel Degree 설정이 안됨
        // 기존의 Parallel Degree를 따르면 됨
        aParallelDegree = aHeader->mParallelDegree;
    }

    IDE_TEST( smrUpdate::updateAllAtTableHead(NULL, /* idvSQL* */
                                              aTrans,
                                              aHeader,
                                              aColumnSize,
                                              &(aHeader->mColumns),
                                              &(aHeader->mInfo),
                                              sTableFlag,
                                              aMaxRow,
                                              aParallelDegree)
              != IDE_SUCCESS );

    IDU_FIT_POINT( "1.PROJ-1552@smcTable::modifyTableInfo" );

    aHeader->mFlag           = sTableFlag;
    aHeader->mMaxRow         = aMaxRow;
    aHeader->mParallelDegree = aParallelDegree;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sHeaderPageID)
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       SMC_CAT_TABLE->mSelfOID,
                                       aHeader->mSelfOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_CHANGED_FIXED_SLOT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_maxrow);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Invalid_MaxRows));
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sState != 0 )
    {
        IDE_ASSERT( smmDirtyPageMgr::insDirtyPage(
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                        sHeaderPageID)
                    == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : index 생성(공통)
 *
 * index 생성과정은 disk/memory table에 관계없이 공통
 * index 관리자에 의해 처리되기 때문에 collection layer
 * 에서는 고려할 것이 없다.
 * - 2nd. code design
 *   + table validation 처리
 *   + index 타입에 따라 indexheader 초기화
 *     : 공통인덱스관리자의 initIndexHeader에서 segment를 할당
 *     : index segment가 생성되는 tablespace id 명시 가능
 *   + table header에 index 정보를 저장하기 위한 variable
 *     slot 할당
 *     : table에 기존 Index가 1개 이상 존재하는 경우
 *       기존 값을 할당받은 variable slot에 복사하고
 *       새로 추가하는 index 정보를 추가
 *     : 추가하려는 index가 primary key라면, slot의
 *       맨 처음에 저장하고, 기존 index는 그 뒤로 저장
 *       그렇지 않다면 기존 index를 맨 처음에 새로운 index를
 *       맨 뒤로 저장한다.
 *   + dirty page 등록
 *   + 기존 index 정보를 저장한 old variable slot이 존재한다면,
 *     SMP_DELETE_SLOT을 설정한 후, 트랜잭션 commit 후에
 *     ager가 제거할 수 있도록 SM_OID_OLD_VARIABLE_SLOT 표시
 *   + 새로운 index 정보를 저장한 new variable slot에 대하여
 *     트랜잭션 rollback 후에 ager가 제거할 수 있도록
 *     SM_OID_OLD_VARIABLE_SLOT 표시
 *   + 새로운 index 정보를 저장한 variable slot을
 *     table header에 설정 및 로깅
 *     : table header에 대하여 acquire sync
 *     : 새로운 index 생성
 *     : table header에 대하여 release sync
 *   + index 생성에 대한 NTA 로깅 - SMR_OP_CREATE_INDEX
 *   + table header 변경에 대한 SM_OID_CHANGED_FIXED_SLOT 표시
 *
 * - RECOVERY
 *
 *  # index 생성시 (* disk-index생성)
 *
 *  (0) (1)        (2)         {(3)       (4)}       (5)      (6)      (7)
 *   |---|----------|-----------|---------|-----------|--------|--------|--> time
 *       |          |           |         |           |        |        |
 *     alloc        update      update    *alloc      set     update    NTA
 *     v-slot       new index,  seg rID    segment    delete  index     OP_CREATE
 *     from         header +    on        [NTA->(2)]  flag    oID       INDEX
 *     cat-tbl      기존 index  tbl-hdr               old     on        [NTA->(0)]
 *    [NTA->(0)]    header      (R/U)                 index   tbl-hdr
 *                  on v-slot                         v-slot  (R/U)
 *                  oID                               (R/U)
 *                  (R/U)
 *
 *  # Disk-index Recovery Issue
 *
 *  - (1)까지 로깅하였다면
 *    + (1) -> logical undo
 *      : v-slot을 delete flag를 설정한다.
 *    + smxTrans::abort 수행한다.
 *
 *  - (2)까지 로깅하였다면
 *    + (2) -> undo
 *      : 이전 index OID로 복구한다.
 *    + (1) -> logical undo
 *    + smxTrans::abort
 *
 *  - (4)까지 mtx commit 하였다면 (!!!!!!!!!!)
 *    + (4) -> logical undo
 *      : segment를 free한다. ((3)':SDR_OP_NULL) [NTA->(2)]
 *        만약 free하지 못하였다면, 다시 logical undo를 수행하면 된다.
 *        free segment를 mtx commit 하였다면, [NTA->(2)]따라 다음 undo과정
 *        으로 skip하고 넘어간다.
 *    + (2) -> undo
 *    + ...
 *    + smxTrans::abort을 수행한다.
 *
 *  - (6)까지 로깅하였다면
 *    + (6) -> undo
 *    + (5) -> undo
 *    + (4) -> logical undo
 *    + ...
 *    + smxTrans::abort을 수행한다.
 *
 *  - (7)까지 로깅하였다면
 *    + (7) -> logical undo
 *      : dropIndexByAbortOID를 수행하며 다음과 같다.
 *
 *      (7)       {(a)         (b)}        (c)         (d)      (e)       (7)'
 *       |----------|-----------|-----------|-----------|--------|--------|--> time
 *                  |           |           |           |        |        |
 *                 update      free         set         update  set      DUMMY
 *                 seg rID     segment      delete flag index   delete   CLR
 *                 on          [NTA->(7)]   old index   oID     flag
 *                 tbl-hdr                  v-slot      on      new index
 *                 (R/U)                    (R/U)       tbl-hdr v-slot
 *                                                      (R/U)   [used]
 *                                                              (R/U)
 *    + smxTrans::abort을 수행한다.
 *
 ***********************************************************************/
IDE_RC smcTable::createIndex( idvSQL               *aStatistics,
                              void                * aTrans,
                              scSpaceID             aSpaceID,
                              smcTableHeader      * aHeader,
                              SChar               * aName,
                              UInt                  aID,
                              UInt                  aType,
                              const smiColumnList * aColumns,
                              UInt                  aFlag,
                              UInt                  aBuildFlag,
                              smiSegAttr          * aSegAttr,
                              smiSegStorageAttr   * aSegStorageAttr,
                              ULong                 aDirectKeyMaxSize,
                              void               ** aIndex )
{

    SChar           * sOldIndexRowPtr = NULL;
    SChar           * sNewIndexRowPtr = NULL;
    SChar           * sDest;
    smOID             sNewIndexOID;
    smOID             sOldIndexOID;
    smiValue          sVal;
    void            * sIndexHeader;
    UInt              sState  = 0;
    scPageID          sPageID = 0;
    smLSN             sLsnNTA;
    smOID             sIndexOID;
    scOffset          sOffset;
    UInt              sIndexHeaderSize;
    UInt              sMaxIndexCount;
    UInt              sTableLatchState = 0;
    UInt              sAllocState = 0;
    UInt              sTableType;
    UInt              sIdx;
    UInt              sPieceType;
    SChar           * sNewIndexPtr;

    /* ------------------------------------------------
     * [1] table validation
     * ----------------------------------------------*/
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             SCT_VAL_DDL_DML ) // 테이블스페이스 Validation 옵션
              != IDE_SUCCESS );

    sMaxIndexCount = smLayerCallback::getMaxIndexCnt();

    /* ----------------------------
     * [2] validation for maximum index count
     * ---------------------------*/
    //max index count test
    IDE_TEST_RAISE( (getIndexCount(aHeader) +1) > sMaxIndexCount,
                    maximum_index_count_error);

    // for NTA
    sLsnNTA = smLayerCallback::getLstUndoNxtLSN( aTrans );

    // 공통 index 관리자를 통해 indexheader의 크기를 얻음
    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    /* ------------------------------------------------
     * [3] 새로 생성시키는 Index의 정보를 설정
     * indexheader 초기화
     * disk index나 memory index는 모두 동일한
     * index 관리자를 사용하므로 아래와 같이 처리하여도
     * 타입에 따른 indexheader를 초기화할 수 있다.
     * ----------------------------------------------*/
    /* smcTable_createIndex_malloc_IndexHeader.tc */
    IDU_FIT_POINT("smcTable::createIndex::malloc::IndexHeader");
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SMC,
                                sIndexHeaderSize,
                                (void**)&sIndexHeader,
                                IDU_MEM_FORCE)
              != IDE_SUCCESS );
    sAllocState = 1;

    /* ------------------------------------------------
     * FOR A4 : disk index는 공통인덱스관리자의 initIndexHeader에서
     * segment를 할당받는다.
     * index segment는 table과 다른 tablespace에 생성될 수 있으므로
     * tablespace id를 명시할수 있다.
     * ----------------------------------------------*/
    smLayerCallback::initIndexHeader( sIndexHeader,
                                      aHeader->mSelfOID,
                                      aName,
                                      aID,
                                      aType,
                                      aFlag,
                                      aColumns,
                                      aSegAttr,
                                      aSegStorageAttr,
                                      aDirectKeyMaxSize );
    // PROJ-1362 QP-Large Record & Internal LOB지원.
    // index 갯수 제약 풀기 part.
    // 새로운 인덱스 정보를 저장할 대상 variable slot를 찾는다.
    IDE_TEST( findIndexVarSlot2Insert(aHeader,
                                      aFlag,
                                      sIndexHeaderSize,
                                      &sIdx)
              != IDE_SUCCESS);

    /* ------------------------------------------------
     * [4] Table에 기존 Index가 0개 존재하는 경우
     * ----------------------------------------------*/
    sVal.length = aHeader->mIndexes[sIdx].length + sIndexHeaderSize;

    /* ------------------------------------------------
     * [5] Table에 기존 Index가 1개 이상 존재하는 경우
     * 기존 값을 새로운 Version에 copy하고 새로 추가하는 Index 정보를 append!
     * ----------------------------------------------*/
    if(aHeader->mIndexes[sIdx].fstPieceOID != SM_NULL_OID)
    {
        IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                           aHeader->mIndexes[sIdx].fstPieceOID,
                                           (void**)&sOldIndexRowPtr )
                    == IDE_SUCCESS );
        sVal.value = sOldIndexRowPtr + ID_SIZEOF(smVCPieceHeader);
    }
    else
    {
        sVal.value = NULL;
    }

    sTableType = SMI_GET_TABLE_TYPE( aHeader );

    if( sTableType == SMI_TABLE_DISK )
    {
        sPieceType = SM_VCPIECE_TYPE_DISK_INDEX;
    }
    else
    {
        sPieceType = SM_VCPIECE_TYPE_MEMORY_INDEX;
    }

    IDE_TEST( smpVarPageList::allocSlot( aTrans,
                                         SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                         SMC_CAT_TABLE->mSelfOID,
                                         SMC_CAT_TABLE->mVar.mMRDB,
                                         sVal.length,
                                         SM_NULL_OID,
                                         &sNewIndexOID,
                                         (SChar**)&sNewIndexRowPtr,
                                         sPieceType )
              != IDE_SUCCESS );

    sDest = (SChar *)sNewIndexRowPtr + ID_SIZEOF(smVCPieceHeader);
    sNewIndexPtr = (SChar *)sNewIndexRowPtr + ID_SIZEOF(smVCPieceHeader);

    sPageID = SM_MAKE_PID(sNewIndexOID);
    sState  = 1;

    if( (aFlag & SMI_INDEX_TYPE_MASK) == SMI_INDEX_TYPE_PRIMARY_KEY )
    {

        IDE_ASSERT( sIdx == 0);

        // primary key는 table에서 한개만 존재하며,
        // qp에 primary key가 variable slot의 맨앞에 있다고 가정하고 있음.

        /* ------------------------------------------------
         * 새로운 index가 primary라면 variable slot의
         * 맨 처음부분에 index header를 저장한다.
         * - SMR_LT_UPDATE의 SMR_PHYSICAL 로깅
         * ----------------------------------------------*/
        sOffset   = SM_MAKE_OFFSET(sNewIndexOID) + ID_SIZEOF(smVCPieceHeader);
        sIndexOID = SM_MAKE_OID(sPageID, sOffset);
        ((smnIndexHeader*)sIndexHeader)->mSelfOID = sIndexOID;

        idlOS::memcpy( sDest, sIndexHeader, sIndexHeaderSize );
        *aIndex = sDest;

        sDest += sIndexHeaderSize;

        if( aHeader->mIndexes[sIdx].length != 0 )
        {
            idlOS::memcpy( sDest,
                           sVal.value,
                           aHeader->mIndexes[sIdx].length );
        }
    }
    else
    {
        /* ------------------------------------------------
         * 새로운 index가 primary가 아니라면 variable slot의
         * 기존 index정보를 먼저 저장하고 맨 뒤부분에  새로운
         * index의 header를 저장한다.
         * - SMR_LT_UPDATE의 SMR_PHYSICAL 로깅
         * ----------------------------------------------*/
        sOffset   = SM_MAKE_OFFSET(sNewIndexOID) +
                    ID_SIZEOF(smVCPieceHeader);
        sIndexOID = SM_MAKE_OID(sPageID,
                                sOffset + aHeader->mIndexes[sIdx].length);
        ((smnIndexHeader*)sIndexHeader)->mSelfOID = sIndexOID;

        if(aHeader->mIndexes[sIdx].length != 0)
        {
            idlOS::memcpy( sDest,
                           sVal.value,
                           aHeader->mIndexes[sIdx].length );
        }

        sDest += aHeader->mIndexes[sIdx].length;
        idlOS::memcpy(sDest, sIndexHeader, sIndexHeaderSize );

        *aIndex = sDest;
    }

    // BUG-25313 : adjustIndexSelfOID() 위치 변경.
    adjustIndexSelfOID( (UChar*)sNewIndexPtr,
                        sNewIndexOID + ID_SIZEOF(smVCPieceHeader),
                        sVal.length );

    // BUG-25313 : smrUpdate::physicalUpdate() 위치 변경.
    IDE_TEST(smrUpdate::physicalUpdate(NULL, /* idvSQL* */
                                       aTrans,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       sPageID,
                                       sOffset,
                                       NULL,
                                       0,
                                       sNewIndexPtr,
                                       sVal.length,
                                       NULL,
                                       0)
             != IDE_SUCCESS);


    // 새로운 variable slot이 포함된 page를 dirty page로 등록
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID) != IDE_SUCCESS );

    IDE_DASSERT( sTableType == SMI_TABLE_MEMORY ||
                 sTableType == SMI_TABLE_META   ||
                 sTableType == SMI_TABLE_DISK   ||
                 sTableType == SMI_TABLE_VOLATILE );

    /* BUG-33803 Disk index를 disable 상태로 create 할 경우 index segment를
     * 생성하지 않는다. 이 후 index를 enable 상태로 변경할 때 index segment도
     * 함께 생성한다. */
    if( (sTableType == SMI_TABLE_DISK) &&
        ((aFlag & SMI_INDEX_USE_MASK) == SMI_INDEX_USE_ENABLE) )
    {
        // create index segment
        IDE_TEST( sdpSegment::allocIndexSeg4Entry(
                      aStatistics,
                      aTrans,
                      aSpaceID,
                      aHeader->mSelfOID,
                      sIndexOID,
                      aID,
                      aType,
                      aBuildFlag,  // BUG-17848
                      SDR_MTX_LOGGING,
                      aSegAttr,
                      aSegStorageAttr ) != IDE_SUCCESS );
    }

    sPageID = SM_MAKE_PID(aHeader->mSelfOID);

    if( aHeader->mIndexes[sIdx].fstPieceOID != SM_NULL_OID )
    {
        /* ------------------------------------------------
         * 기존 index 정보를 저장한 old variable slot이 존재한다면,
         * SMP_DELETE_SLOT을 설정한 후, 트랜잭션 commit 후에
         * ager가 제거할 수 있도록 SM_OID_OLD_VARIABLE_SLOT 표시
         * ----------------------------------------------*/
        IDE_TEST(smcRecord::setFreeFlagAtVCPieceHdr(aTrans,
                                                    aHeader->mSelfOID,
                                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                    aHeader->mIndexes[sIdx].fstPieceOID,
                                                    sOldIndexRowPtr,
                                                    SM_VCPIECE_FREE_OK)
                 != IDE_SUCCESS);

        IDU_FIT_POINT( "4.PROJ-1552@smcTable::createIndex" );

        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           SMC_CAT_TABLE->mSelfOID,
                                           aHeader->mIndexes[sIdx].fstPieceOID,
                                           SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SM_OID_OLD_VARIABLE_SLOT )
                  != IDE_SUCCESS );
    }

    /* ------------------------------------------------
     * 새로운 index 정보를 저장한 new variable slot에 대하여
     * 트랜잭션 rollback 후에 ager가 제거할 수 있도록
     * SM_OID_NEW_VARIABLE_SLOT 표시
     * ----------------------------------------------*/
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       SMC_CAT_TABLE->mSelfOID,
                                       sNewIndexOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_NEW_VARIABLE_SLOT )
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * [6] 새로운 index 정보를 저장한 variable slot을
     * table header에 설정 및 로깅
     * - SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_UPDATE_INDEX
     * ----------------------------------------------*/
    IDE_TEST( smrUpdate::updateIndexAtTableHead(NULL, /* idvSQL* */
                                                aTrans,
                                                aHeader->mSelfOID,
                                                &(aHeader->mIndexes[sIdx]),
                                                sIdx,
                                                sNewIndexOID,
                                                sVal.length,
                                                SM_VCDESC_MODE_OUT)
              != IDE_SUCCESS );

    IDU_FIT_POINT( "5.PROJ-1552@smcTable::createIndex" );

    sOldIndexOID = aHeader->mIndexes[sIdx].fstPieceOID;

    IDE_TEST( smcTable::latchExclusive( aHeader ) != IDE_SUCCESS );
    sTableLatchState = 1;

    aHeader->mIndexes[sIdx].length = sVal.length;
    aHeader->mIndexes[sIdx].fstPieceOID = sNewIndexOID;
    aHeader->mIndexes[sIdx].flag = SM_VCDESC_MODE_OUT;

    // smnIndex 초기화 : index 생성(module의 create호출)
    if ( ( smLayerCallback::getFlagOfIndexHeader( *aIndex ) & SMI_INDEX_USE_MASK )
         == SMI_INDEX_USE_ENABLE )
    {
        IDE_TEST( smLayerCallback::initIndex( aStatistics,
                                              aHeader,
                                              *aIndex,
                                              aSegAttr,
                                              aSegStorageAttr,
                                              NULL,
                                              0 )
                  != IDE_SUCCESS );
    }

    sTableLatchState = 0;
    IDE_TEST( smcTable::unlatch( aHeader ) != IDE_SUCCESS );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID)
              != IDE_SUCCESS );

    IDE_TEST( smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                        aTrans,
                                        &sLsnNTA,
                                        aHeader->mSpaceID,
                                        SMR_OP_CREATE_INDEX,
                                        sOldIndexOID,
                                        sIndexOID)
              != IDE_SUCCESS );

    IDU_FIT_POINT( "6.PROJ-1552@smcTable::createIndex" );

    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       SMC_CAT_TABLE->mSelfOID,
                                       aHeader->mSelfOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_CHANGED_FIXED_SLOT )
              != IDE_SUCCESS );

    sAllocState = 0;
    IDE_TEST( iduMemMgr::free(sIndexHeader) != IDE_SUCCESS );

    IDU_FIT_POINT( "1.PROJ-1548@smcTable::createIndex" );

    IDU_FIT_POINT( "1.BUG-30612@smcTable::createIndex" );

    return IDE_SUCCESS;

    IDE_EXCEPTION(maximum_index_count_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Maximum_Index_Count,
                                 sMaxIndexCount));
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        (void)smmDirtyPageMgr::insDirtyPage(
            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID);
        IDE_POP();
    }

    if(sTableLatchState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( smcTable::unlatch( aHeader ) == IDE_SUCCESS );
        IDE_POP();
    }

    if(sAllocState != 0)
    {
        IDE_ASSERT( iduMemMgr::free(sIndexHeader) == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

/*
 * PROJ-1917 MVCC Renewal
 * Out-Place Update를 위한 Index Self OID 재조정
 */
void smcTable::adjustIndexSelfOID( UChar * aIndexHeaders,
                                   smOID   aStartIndexOID,
                                   UInt    aTotalIndexHeaderSize )
{
    UInt             i;
    smnIndexHeader * sIndexHeader;

    for( i = 0;
         i < aTotalIndexHeaderSize;
         i += ID_SIZEOF( smnIndexHeader ) )
    {
        sIndexHeader = (smnIndexHeader*)(aIndexHeaders + i);
        sIndexHeader->mSelfOID = aStartIndexOID + i;
    }
}

/***********************************************************************
 * Description :  PROJ-1362 QP - Large Record & Internal LOB 지원.
 *                table의 컬럼개수와 인덱스 개수 제약 풀기.
 * 새로운 인덱스 정보를 저장할 대상 variable slot를 찾는다.
 *  - code design
 *  if (새로 추가할 인덱스가  primary key )
 *  then
 *      //replication때문에 primay key를 맨앞에 저장해야 한다.
 *      i = 0;
 *  else
 *      for( i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++)
 *      do
 *          if ( table헤더의 mIndexes[i] 가 null OID 이거나
 *           table 헤더의 mIndexes[i]의 현재 variable slot길이  + 하나의 인덱스 헤더 크기
 *             <= SMP_VC_PIECE_MAX_SIZE)
 *          then
 *            break;
 *          fi
 *      done
 *  fi
 * if( i == SMC_MAX_INDEX_OID_CNT)
 * then
 *    retrun 최대 인덱스 개수 error;
 * fi
 * IDE_EXCEPTION_CONT(success);
 *
 * return i;
 ***********************************************************************/
IDE_RC smcTable::findIndexVarSlot2Insert(smcTableHeader      *aHeader,
                                         UInt                 aFlag,
                                         const UInt          aIndexHeaderSize,
                                         UInt                *aTargetVarSlotIdx)
{
    UInt i =0;


    if( (aFlag & SMI_INDEX_TYPE_MASK) == SMI_INDEX_TYPE_PRIMARY_KEY )
    {
        *aTargetVarSlotIdx =0;
        IDE_ASSERT(aHeader->mIndexes[0].length + aIndexHeaderSize <=
                   SMP_VC_PIECE_MAX_SIZE);
    }
    else
    {
        for( ; i < SMC_MAX_INDEX_OID_CNT ; i++)
        {
            if( (aHeader->mIndexes[i].fstPieceOID == SM_NULL_OID) ||
                (((aHeader->mIndexes[i].length + aIndexHeaderSize)
                  <= SMP_VC_PIECE_MAX_SIZE) && (i != 0)) ||
                // primary key를 위하여 0번째 variable slot은
                (((aHeader->mIndexes[i].length + 2 * aIndexHeaderSize)
                  <= SMP_VC_PIECE_MAX_SIZE) && ( i == 0)) )

            {
                *aTargetVarSlotIdx = i;
                break;
            }//if
        }//for i
    }//else

    IDE_TEST_RAISE( i == SMC_MAX_INDEX_OID_CNT, maximum_index_count_error );

    return IDE_SUCCESS;

    IDE_EXCEPTION(maximum_index_count_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Maximum_Index_Count));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 인덱스 제거(공통)
 *
 * disk/memory 타입의 index에 대한 dropindex를 수행한다.
 * - 이 함수에서는 transaction commit시까지 인덱스 제거를
 *   pending 연산으로 처리하도록 한다.
 * - 또한 drop index 작업은 IX lock을 획득하기때문에, 동시에
 *   여러 transaction에 의해 수행될 수 있다.
 *
 * - RECOVERY
 *
 *  # index 제거 (* disk-index 제거)
 *
 ***********************************************************************/
IDE_RC smcTable::dropIndex( void               * aTrans,
                            smcTableHeader     * aHeader,
                            void               * aIndex,
                            sctTBSLockValidOpt   aTBSLvOpt )
{
    SChar          * sDest;
    SChar          * sBase;
    SChar          * sNewIndexRowPtr;
    SChar          * sOldIndexRowPtr;
    UInt             sTableType;
    UInt             sSlotIdx;
    SInt             sOffset = 0;
    smOID            sNewIndexOID = 0;
    smiValue         sVal;
    scPageID         sPageID = 0;
    UInt             sState = 0;
    void           * sIndexListPtr;
    UInt             sTableLatchState = 0;
    UInt             sIndexHeaderSize;
    UInt             sPieceType;

    /* Added By Newdaily since dropIndex Memory must be free at rollback */
    IDE_TEST( smrLogMgr::writeNTALogRec( NULL, /* idvSQL* */
                                         aTrans,
                                         smLayerCallback::getLstUndoNxtLSNPtr( aTrans ),
                                         aHeader->mSpaceID,
                                         SMR_OP_DROP_INDEX,
                                         aHeader->mSelfOID ) 
              != IDE_SUCCESS );


    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    /* ----------------------------
     * [1] table validation
     * ---------------------------*/
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt )
              != IDE_SUCCESS );

    /* ----------------------------
     * [2] drop하고자 하는 Index를 찾고,
     * Table의 Index 정보를 갱신한다.
     * ---------------------------*/
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(aHeader, //BUG-23218
                                              aIndex,
                                              sIndexHeaderSize,
                                              &sSlotIdx,
                                              &sOffset)
                 == IDE_SUCCESS );

    //sValue는 하나의 인덱스가 삭제되고 남은 인덱스헤더 크기.

    sVal.length = aHeader->mIndexes[sSlotIdx].length - sIndexHeaderSize;
    sVal.value  = NULL;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                       aHeader->mIndexes[sSlotIdx].fstPieceOID,
                                       (void**)&sOldIndexRowPtr )
                == IDE_SUCCESS );

    sBase      = sOldIndexRowPtr + ID_SIZEOF(smVCPieceHeader);
    sTableType = SMI_GET_TABLE_TYPE( aHeader );

    if( sTableType == SMI_TABLE_DISK )
    {
        sPieceType = SM_VCPIECE_TYPE_DISK_INDEX;
    }
    else
    {
        sPieceType = SM_VCPIECE_TYPE_MEMORY_INDEX;
    }

    /* BUG-30612 index header self oid가 잘못 기록되는 경우가 있습니다.
     * 버그는 수정 되었지만 이전에 이미 잘못 기록되어 있는 경우를 대비하여
     * drop하는 slot의 Index Header의 SelfOID를 보정하도록 합니다.
     * index header의 SelfOID는 여기와 index검증 에서만 사용됩니다. */
    adjustIndexSelfOID( (UChar*)sBase,
                        aHeader->mIndexes[sSlotIdx].fstPieceOID
                            + ID_SIZEOF(smVCPieceHeader),
                        aHeader->mIndexes[sSlotIdx].length );

    /* ------------------------------------------------
     * drop할 index에 대하여 pending 연산으로 등록시킴
     * - memory index의 경우에는 해당 transaction의 commit과정중에
     *   smcTable::dropIndexPending이 호출하여 바로 처리하며,
     *   disk index의 경우에는 해당 tss에 pending operation으로
     *   등록하여 disk G.C에 의해 제거하도록 처리한다.
     * ----------------------------------------------*/
    // 인덱스가 한개만 있는 테이블에서 drop index가 된경우.
    if( sVal.length == 0 )
    {
        sOffset = 0;
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           aHeader->mSelfOID,
                                           aHeader->mIndexes[sSlotIdx].fstPieceOID
                                               + ID_SIZEOF(smVCPieceHeader),
                                           SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SM_OID_DROP_INDEX )
                  != IDE_SUCCESS );
    }
    else
    {
        if( sVal.length > 0 )
        {
            // 인덱스가 2개 이상인 경우, 인덱스헤더가 하나빠진
            // 새로운 인덱스 정보 variable slot을 할당받는다.
            IDE_TEST( smpVarPageList::allocSlot(aTrans,
                                            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            SMC_CAT_TABLE->mSelfOID,
                                            SMC_CAT_TABLE->mVar.mMRDB,
                                            sVal.length,
                                            SM_NULL_OID,
                                            &sNewIndexOID,
                                            &sNewIndexRowPtr,
                                            sPieceType)
                      != IDE_SUCCESS );

            sDest = sNewIndexRowPtr + ID_SIZEOF(smVCPieceHeader);

            IDE_DASSERT( sTableType == SMI_TABLE_MEMORY ||
                         sTableType == SMI_TABLE_META   ||
                         sTableType == SMI_TABLE_DISK   ||
                         sTableType == SMI_TABLE_VOLATILE );

            // index 제거에 대한 pending operation 수행
            IDE_TEST( smLayerCallback::addOID( aTrans,
                                               aHeader->mSelfOID,
                                               aHeader->mIndexes[sSlotIdx].fstPieceOID
                                                   + ID_SIZEOF(smVCPieceHeader) + sOffset,
                                               SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                               SM_OID_DROP_INDEX )
                      != IDE_SUCCESS );

            sPageID = SM_MAKE_PID(sNewIndexOID);
            sState  = 1;

            //삭제된 인덱스 헤더를 빼고, 나머지에 대한 copy작업.
            idlOS::memcpy( sDest, sBase, sOffset);

            idlOS::memcpy( sDest + sOffset,
                           sBase + sOffset + sIndexHeaderSize,
                           sVal.length - sOffset);

            adjustIndexSelfOID( (UChar*)sDest,
                                sNewIndexOID + ID_SIZEOF(smVCPieceHeader),
                                sVal.length );

            IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                      sPageID)
                      != IDE_SUCCESS );

            // BUG-25313 : smrUpdate::physicalUpdate() 위치 변경.
            IDE_TEST( smrUpdate::physicalUpdate(
                                    NULL, /* idvSQL* */
                                    aTrans,
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                    sPageID,
                                    SM_MAKE_OFFSET(sNewIndexOID)
                                        + ID_SIZEOF(smVCPieceHeader),
                                    NULL,
                                    0,
                                    sDest,
                                    sVal.length,
                                    NULL,
                                    0)
                      != IDE_SUCCESS);

        }
    }

    /* ------------------------------------------------
     * [3] 각 경우에 맞게 version list에 추가
     * ----------------------------------------------*/
    IDE_TEST(smcRecord::setIndexDropFlag(
                                aTrans,
                                aHeader->mSelfOID,
                                aHeader->mIndexes[sSlotIdx].fstPieceOID
                                    + ID_SIZEOF(smVCPieceHeader) + sOffset,
                                sBase + sOffset,
                                (UShort)SMN_INDEX_DROP_TRUE )
             != IDE_SUCCESS);

    IDE_TEST(smcRecord::setFreeFlagAtVCPieceHdr(
                                        aTrans,
                                        aHeader->mSelfOID,
                                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                        aHeader->mIndexes[sSlotIdx].fstPieceOID,
                                        sOldIndexRowPtr,
                                        SM_VCPIECE_FREE_OK)
             != IDE_SUCCESS);

    // old index information version
    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       SMC_CAT_TABLE->mSelfOID,
                                       aHeader->mIndexes[sSlotIdx].fstPieceOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_OLD_VARIABLE_SLOT )
              != IDE_SUCCESS );

    if( sVal.length == 0 )
    {
        sNewIndexOID = SM_NULL_OID;
    }
    else
    {
        // new index information version
        IDE_TEST( smLayerCallback::addOID( aTrans,
                                           SMC_CAT_TABLE->mSelfOID,
                                           sNewIndexOID,
                                           SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SM_OID_NEW_VARIABLE_SLOT )
                  != IDE_SUCCESS );
    }

    sPageID = SM_MAKE_PID(aHeader->mSelfOID);

    /* ------------------------------------------------
     * SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_UPDATE_INDEX 로깅
     * ----------------------------------------------*/
    IDE_TEST( smrUpdate::updateIndexAtTableHead( NULL, /* idvSQL* */
                                                 aTrans,
                                                 aHeader->mSelfOID,
                                                 &(aHeader->mIndexes[sSlotIdx]),
                                                 sSlotIdx,
                                                 sNewIndexOID,
                                                 sVal.length,
                                                 SM_VCDESC_MODE_OUT )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "3.PROJ-1552@smcTable::dropIndex" );

    IDE_TEST( smcTable::latchExclusive( aHeader ) != IDE_SUCCESS );
    sTableLatchState = 1;

    if(aHeader->mDropIndexLst == NULL)
    {
        /* smcTable_dropIndex_alloc_IndexListPtr.tc */
        IDU_FIT_POINT("smcTable::dropIndex::alloc::IndexListPtr");
        IDE_TEST(mDropIdxPagePool.alloc(&sIndexListPtr) != IDE_SUCCESS);
        aHeader->mDropIndex = 0;

        idlOS::memcpy(sIndexListPtr, sBase + sOffset, sIndexHeaderSize);
        aHeader->mDropIndexLst = sIndexListPtr;
    }
    else
    {
        idlOS::memcpy( (SChar *)(aHeader->mDropIndexLst) +
                       ( aHeader->mDropIndex * sIndexHeaderSize ),
                       sBase + sOffset,
                       sIndexHeaderSize);
    }

    aHeader->mIndexes[sSlotIdx].fstPieceOID    = sNewIndexOID;
    aHeader->mIndexes[sSlotIdx].length         = sVal.length;
    aHeader->mIndexes[sSlotIdx].flag           = SM_VCDESC_MODE_OUT;

    (aHeader->mDropIndex)++;

    sTableLatchState = 0;
    IDE_TEST( smcTable::unlatch( aHeader ) != IDE_SUCCESS );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                      SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                      sPageID) 
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::addOID( aTrans,
                                       SMC_CAT_TABLE->mSelfOID,
                                       aHeader->mSelfOID,
                                       SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                       SM_OID_CHANGED_FIXED_SLOT )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        (void)smmDirtyPageMgr::insDirtyPage(
            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID);
        IDE_POP();
    }

    if(sTableLatchState != 0)
    {
        IDE_PUSH();
        IDE_ASSERT( smcTable::unlatch( aHeader ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description :  PROJ-1362 QP - Large Record & Internal LOB 지원.
 *                table의 컬럼개수와 인덱스 개수 제약 풀기.
 *  drop 인덱스  대상 variable slot를 찾고, 해당 variable slot에서
 *  인덱스 헤더 offset을 구한다.
 *  - code design
 * for( i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++)
 * do
 *  if( table헤더의 mIndexes[i] == SM_NULL_OID)
 *  then
 *    continue;
 *  else
 *    if( table헤더의 mIndexes[i]의 variable slot안에
 *       drop될 index헤더가 있는가?)
 *    then
 *        해당 variable slot에서 drop될 index의 offset을 구한다;
 *         break;
 *    fi
 *  fi
 *  fi
 * done
 * if( i == SMC_MAX_INDEX_OID_CNT)
 * then
 *     인덱스를 찾을수 없다는 에러;
 * fi
 * BUG-23218 : DROP INDEX 외에 ALTER INDEX 구문에서도 사용할 수 있도록 수정함
 **************************************************************************/
IDE_RC smcTable::findIndexVarSlot2AlterAndDrop(smcTableHeader      *aHeader,
                                               void*                aIndex, // BUBUB CHAR pointer
                                               const UInt           aIndexHeaderSize,
                                               UInt*                aTargetIndexVarSlotIdx,
                                               SInt*                aOffset)
{
    UInt i;
    UInt j;
    SChar   * sSrc;


    for(i=0  ; i < SMC_MAX_INDEX_OID_CNT ; i++)
    {
        if( aHeader->mIndexes[i].fstPieceOID == SM_NULL_OID)
        {
            continue;
        }//if
        IDE_ASSERT( smmManager::getOIDPtr( 
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                        aHeader->mIndexes[i].fstPieceOID + ID_SIZEOF(smVCPieceHeader),
                        (void**)&sSrc )
                    == IDE_SUCCESS );
        for( j = 0;
             j < aHeader->mIndexes[i].length;
             j += aIndexHeaderSize,sSrc += aIndexHeaderSize )
        {
            if( sSrc == aIndex ) /* 찾고있는 smnIndexHeader 인 경우 */
            {
                *aTargetIndexVarSlotIdx = i;
                *aOffset =  j;
                IDE_CONT( success );
            }//if
        }// inner for
    }//outer for

    IDE_TEST_RAISE( i == SMC_MAX_INDEX_OID_CNT, not_found_error );

    IDE_EXCEPTION_CONT( success );

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Index_Not_Found ));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : persistent index 상태 설정
 * 본 함수는 table의 특정 index에 대하여 persistent index 여부를
 * 변경하는 함수로써 memory index에 한해서 제공되는
 * 기능이므로, disk index에서는 고려할 필요 없다.
 ***********************************************************************/
IDE_RC smcTable::alterIndexInfo(void           * aTrans,
                                smcTableHeader * aHeader,
                                void           * aIndex,
                                UInt             aFlag)
{
    smnIndexHeader    *sIndexHeader;
    UInt               sIndexHeaderSize;
    UInt               sIndexSlot;
    scPageID           sPageID = SC_NULL_PID;
    SInt               sOffset;

    sIndexHeader = ((smnIndexHeader*)aIndex);
    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    /* ----------------------------
     * [1] table validation
     * ---------------------------*/
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             SCT_VAL_DDL_DML )
              != IDE_SUCCESS );


    /* fix BUG-23218 : 인덱스 탐색 과정 모듈화*/
    /* ----------------------------
     * [2] 수정하고자 하는 Index를 찾는다.
     * ---------------------------*/
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(aHeader,
                                              sIndexHeader,
                                              sIndexHeaderSize,
                                              &sIndexSlot,
                                              &sOffset)
                 == IDE_SUCCESS );

    /* ------------------------------------------------
     * [3] 찾은 인덱스의 sIndexSlot을 바탕으로,
     *     인덱스의 위치를 계산한다.
     * ------------------------------------------------*/
    sPageID = SM_MAKE_PID( aHeader->mIndexes[sIndexSlot].fstPieceOID );



    /* ---------------------------------------------
     * [4] 로깅 후 적용한다.
     * ---------------------------------------------*/

    IDE_TEST( smrUpdate::setIndexHeaderFlag( NULL, /* idvSQL* */
                                             aTrans,
                                             aHeader->mIndexes[sIndexSlot].fstPieceOID,
                                             ID_SIZEOF(smVCPieceHeader) + sOffset , // BUG-23218 : 로깅 위치 오류 수정
                                             smLayerCallback::getFlagOfIndexHeader( aIndex ),
                                             aFlag)
              != IDE_SUCCESS);


    smLayerCallback::setFlagOfIndexHeader( aIndex, aFlag );

    (void)smmDirtyPageMgr::insDirtyPage(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sPageID);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : persistent index에 Inconsistent함을 설정
 *
 ***********************************************************************/
IDE_RC smcTable::setIndexInconsistency(smOID            aTableOID,
                                       smOID            aIndexID )
{
    smcTableHeader   * sTableHeader;
    smnIndexHeader   * sIndexHeader;

    UInt               sIndexHeaderSize;
    UInt               sIndexSlot;
    scPageID           sPageID = SC_NULL_PID;
    SInt               sOffset;

    IDE_TEST( smcTable::getTableHeaderFromOID( aTableOID,
                                               (void**)&sTableHeader )
              != IDE_SUCCESS );

    sIndexHeader = (smnIndexHeader*)smcTable::getTableIndexByID(
                                (void*)sTableHeader,
                                aIndexID );
    
    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    /* fix BUG-23218 : 인덱스 탐색 과정 모듈화
     *  수정하고자 하는 Index의 Offset등을 찾는다. */
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(sTableHeader,
                                              sIndexHeader,
                                              sIndexHeaderSize,
                                              &sIndexSlot,
                                              &sOffset)
                 == IDE_SUCCESS );

    /* ------------------------------------------------
     *  찾은 인덱스의 sIndexSlot을 바탕으로,
     *  인덱스의 위치를 계산한다.
     * ------------------------------------------------*/
    sPageID = SM_MAKE_PID( sTableHeader->mIndexes[sIndexSlot].fstPieceOID );

    // Inconsistent -> Consistent경우는 없다.
    smLayerCallback::setIsConsistentOfIndexHeader( sIndexHeader, ID_FALSE );

    (void)smmDirtyPageMgr::insDirtyPage(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sPageID);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : 대상 table의 모든 persistent index에 Inconsistent함을 설정
 *
 * Related Issues:
 *      PROJ-2184 RP Sync 성능 향상
 *
 *  aTableHeader   - [IN] 대상 table의 smcTableHeader
 ***********************************************************************/
IDE_RC smcTable::setAllIndexInconsistency( smcTableHeader   * aTableHeader )
                                         
{
    smnIndexHeader    * sIndexHeader;
    UInt                sIndexIdx = 0;
    UInt                sIndexCnt;
                      
    UInt                sIndexHeaderSize;
    UInt                sIndexSlot;
    smVCPieceHeader   * sVCPieceHdr;
    UInt                sOffset;

    sIndexCnt = smcTable::getIndexCount( aTableHeader );

    /* Index가 하나도 없으면 바로 success return */
    IDE_TEST_CONT( sIndexCnt == 0, no_indexes );

    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    for( sIndexSlot = 0; sIndexSlot < SMC_MAX_INDEX_OID_CNT; sIndexSlot++ )
    {
        if(aTableHeader->mIndexes[sIndexSlot].fstPieceOID == SM_NULL_OID)
        {
            continue;
        }

        IDE_ASSERT( smmManager::getOIDPtr( 
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                        aTableHeader->mIndexes[sIndexSlot].fstPieceOID,
                        (void**)&sVCPieceHdr )
                    == IDE_SUCCESS );

        sOffset = 0;
        while( (sOffset < aTableHeader->mIndexes[sIndexSlot].length) &&
               (sIndexIdx < sIndexCnt) )
        {
            sIndexHeader = (smnIndexHeader*)((UChar*)sVCPieceHdr +
                                             ID_SIZEOF(smVCPieceHeader) +
                                             sOffset);

            if ( smLayerCallback::getIsConsistentOfIndexHeader( sIndexHeader )
                 == ID_TRUE )
            {
                // Inconsistent -> Consistent경우는 없다.
                smLayerCallback::setIsConsistentOfIndexHeader( sIndexHeader,
                                                               ID_FALSE );
            }
            else
            {
                /* 이미 inconsistent 상태이다. */
            }

            sOffset += sIndexHeaderSize;
            sIndexIdx++;
        }

        (void)smmDirtyPageMgr::insDirtyPage(
                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                SM_MAKE_PID(aTableHeader->mIndexes[sIndexSlot].fstPieceOID) );

        if( sIndexIdx == sIndexCnt )
        {
            /* 모든 index에 대해서 consistent flag를 설정했으므로 loop 탈출 */
            break;
        }
        else
        {
            IDE_DASSERT( sIndexIdx <= sIndexCnt );
        }
    }

    IDE_EXCEPTION_CONT( no_indexes );

    return IDE_SUCCESS;
}


/***********************************************************************
 * Description : undo시에 create index에 대한 nta operation 수행
 ***********************************************************************/
IDE_RC smcTable::dropIndexByAbortHeader( idvSQL*            aStatistics,
                                         void             * aTrans,
                                         smcTableHeader   * aHeader,
                                         const  UInt        aIdx,
                                         void             * aIndexHeader,
                                         smOID              aOIDIndex )
{
    scPageID         sPageID = 0;
    UInt             sState = 0;
    UInt             sDstLength;
    UInt             sSrcLength;
    SChar           *sIndexRowPtr;
    smOID            sSrcIndexOID;

    /* drop하고자 하는 Index를 찾고 Table의 Index 정보를 갱신한다.*/
    IDE_TEST_RAISE( aHeader->mIndexes[aIdx].fstPieceOID == SM_NULL_OID,
                    not_found_error );

    sSrcLength = aHeader->mIndexes[aIdx].length;
    sDstLength = sSrcLength - smLayerCallback::getSizeOfIndexHeader();

    /* 각 경우에 맞게 version list에 추가 */
    sSrcIndexOID = aHeader->mIndexes[aIdx].fstPieceOID;
    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                       sSrcIndexOID,
                                       (void**)&sIndexRowPtr )
                == IDE_SUCCESS );

    /* header에 assign 되었던 new index OID 에 delete flag 설정 */
    IDE_TEST(smcRecord::setFreeFlagAtVCPieceHdr(aTrans,
                                                aHeader->mSelfOID,
                                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                sSrcIndexOID,
                                                sIndexRowPtr,
                                                SM_VCPIECE_FREE_OK)
             != IDE_SUCCESS);

    sPageID = SM_MAKE_PID(aHeader->mSelfOID);
    sState  = 1;

    if( aOIDIndex != SM_NULL_OID )
    {
        /* BUG-17955: Add/Drop Column 수행후 smiStatement End시, Unable to invoke
         * mutex_lock()로 서버 비정상 종료 */
        IDE_TEST( removeIdxHdrAndCopyIndexHdrArr(
                                          aTrans,
                                          aIndexHeader,
                                          sIndexRowPtr,
                                          aHeader->mIndexes[aIdx].length,
                                          aOIDIndex )
                  != IDE_SUCCESS );

        /* 이전 index oID에 delete flag를 used로 설정한다. */
        IDE_ASSERT( smmManager::getOIDPtr(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    aOIDIndex,
                                    (void**)&sIndexRowPtr )
                    == IDE_SUCCESS );
        IDE_TEST(smcRecord::setFreeFlagAtVCPieceHdr(aTrans,
                                                    aHeader->mSelfOID,
                                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                    aOIDIndex,
                                                    sIndexRowPtr,
                                                    SM_VCPIECE_FREE_NO)
                 != IDE_SUCCESS);
    }
    else
    {
        /* aOIDIndex 가 NULL인 경우 length는 0이어야 함 */
        IDE_ASSERT( sDstLength == 0 );
    }

    /* 이전 index oID(aOIDIndex)를 table header에 assign한다. */
    IDE_TEST( smrUpdate::updateIndexAtTableHead(NULL, /* idvSQL* */
                                                aTrans,
                                                aHeader->mSelfOID,
                                                &(aHeader->mIndexes[aIdx]),
                                                aIdx,
                                                aOIDIndex,
                                                sDstLength,
                                                SM_VCDESC_MODE_OUT)
              != IDE_SUCCESS );


    /* 이전 index oID로 복구한다. */
    aHeader->mIndexes[aIdx].fstPieceOID = aOIDIndex;
    aHeader->mIndexes[aIdx].length = sDstLength;
    aHeader->mIndexes[aIdx].flag   = SM_VCDESC_MODE_OUT;

    // PR-14912
    if ( ( smrRecoveryMgr::isRestart() == ID_TRUE ) &&
         ( aOIDIndex != SM_NULL_OID ))
    {
        // at restart, previous index header slot not create,
        // but, do not redo CLR
        if ( smrRecoveryMgr::isRefineDRDBIdx() == ID_TRUE )
        {
            /* BUG-16555: Restart시 Undo하다가 smcTable::dropIndexByAbortHeader
             * 에서 서버 사망: Disk Table에 대해서만 위 함수가 호출되어야 함. */
            if( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_TRUE )
            {
                IDE_TEST( rebuildRuntimeIndexHeaders(
                                                 aStatistics,
                                                 aHeader,
                                                 0 ) /* aMaxSmoNo */
                          != IDE_SUCCESS );
            }
        }
        else
        {
            // nothing to do..
        }
    }
    else
    {
        // at runtime, previous index header slot not free
        // nothing to do...
    }

    sState = 0;
    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                  sPageID) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(not_found_error);
    {
        IDE_SET(ideSetErrorCode (smERR_ABORT_Index_Not_Found));
    }
    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        (void)smmDirtyPageMgr::insDirtyPage(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sPageID);
        IDE_POP();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : drop index에 대한 pending 연산 처리 (공통)
 ***********************************************************************/
IDE_RC smcTable::dropIndexPending( void*  aIndexHeader )
{

    smcTableHeader *sPtr;

    // [1] 실제 Index 레코드 삭제
    IDE_ASSERT( smcTable::getTableHeaderFromOID( smLayerCallback::getTableOIDOfIndexHeader( aIndexHeader ),
                                                 (void**)&sPtr )
                == IDE_SUCCESS );

    if ( smLayerCallback::isNullModuleOfIndexHeader( aIndexHeader ) != ID_TRUE )
    {
        IDE_TEST( smLayerCallback::dropIndex( sPtr,
                                              aIndexHeader )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : drop table의 pending 연산 수행 (공통)
 * pending 연산으로 등록된 droptable에 대한 실제 제거 과정을 수행한다.
 * memory table의 경우 ager가 처리하며, disk table인 경우 disk G.C에
 * 의해 호출되어 처리된다.
 *
 *  [!!!!!!!]
 * 이 memory ager/disk GC/refine catalog table 수행시 호출되는데, 단,
 * refine catalog table시에는 disk table에 한하여 호출되지 않는다.
 * 호출되는 경우는 다음과 같다 .
 *
 * - create memory table을 수행한 tx가 commit/abort한 경우 (by ager or G.C)
 * - drop disk/memory table을 수행한 tx가 commit한 경우  tss가 존재함
 *   (by ager or G.C)
 * - refine catalog table 수행시 memory table에 한하여 호출됨
 *
 * : 이밖에 create disk table하다가 abort하는 경우 해당 undo를 통하여
 *   완전 처리함.
 * : 이밖에 drop disk table하다가 commit하는 경우 해당 tss를 G.C에 의해
 *   처리함
 *
 * - 2nd code design
 *   + table page list를 시스템에 반환함
 *     if (memory table)
 *        - fixed page list entry 해제
 *        - variable page list entry 해제
 *     else if (disk table)
 *        - data page segment 해제
 *     endif
 *   + table header의 index 정보를 저장한 variable
 *     slot을 free시킴
 *   + table header의 column 정보를 저장한 variable
 *     slot을 free시킴
 *   + table header의 info 정보를 저장한 variable
 *     slot을 free시킴
 *   + table 제거에 대한 nta 로깅 (SMR_OP_NULL)
 *   + table slot page의 dirty page 등록
 *
 * - RECOVERY
 *
 *  # drop table pending (* disk-table생성)
 *
 *  (0){(1)         (2)}          {(3)          (4)}
 *   |---|-----------|--------------|------------|--------> time
 *       |           |              |            |
 *      *update      *free          update       free
 *      seg rID,     table         index oID    v-slot
 *      meta PID     segment       on tbl-hdr   to cat-tbl
 *      on tbl-hdr   [NTA->(0)]    (R/U)        [NTA->(2)]
 *      (R/U)
 *
 *     {(5)         (6)}          {(7)          (8)}        (9)
 *    ---|-----------|--------------|------------|-----------|----> time
 *       |           |              |            |           |
 *      update       free          update       free        [NTA->(0)]
 *      column oID   v-slot        info oID     v-slot
 *      on tbl-hdr   to cat-tbl    on tbl-hdr   to cat-tbl
 *      (R/U)       [NTA->(4)]    (R/U)        [NTA->(6)]
 *
 *  # Disk-Table Recovery Issue
 *
 *  : 할당된 TSS의 pending table oID가 기록되어 있는 것은  pending 과정이
 *    완료되지 않았거나 수행되지 않은 경우이다. 그러므로 이 함수를
 *    반복하여 수행하여야 한다.
 *
 *  - (1)까지 로깅하였다면
 *    + (1) -> undo
 *      : tbl-hdr에 segment rID, table meta PID를 이전 image로 복구한다.
 *    + smxTrans::abort을 수행한다.
 *
 *  - (2)까지 mtx commit이 되었다면
 *    + (2) -> logical undo
 *      : doNTADropTable을 수행하여 free segment를 한다.
 *    + smxTrans::abort한다.
 *    + ...
 *    + smxTrans::abort한다.
 *
 *  - (7)까지 로깅되었다면
 *    + (7) -> undo
 *      : table header에 info oID의 이전 image를 복구한다.
 *    + (6)에 대한 처리를 한다.
 *    + ...
 *    + smxTrans::abort한다.
 *
 *  - (8)까지 로깅되었다면
 *    + (8) -> logical redo
 *      : var slot을 다시 free 설정한다
 *    + (6)에 대한 처리를 한다.
 *    + ...
 *    + smxTrans::abort한다.
 *
 *  - (9)까지 로깅되었다면
 *    + (9) -> (0)번까지 skip undo
 *    + smxTrans::abort한다.
 *
 ***********************************************************************/
IDE_RC smcTable::dropTablePending( idvSQL          *aStatistics,
                                   void            * aTrans,
                                   smcTableHeader  * aHeader,
                                   idBool            aFlag )
{

    smOID               sRowOID;
    scPageID            sPageID;
    smLSN               sLsnNTA1;
    smLSN               sLsnNTA2;
    smpSlotHeader     * sSlotHeader;
    UInt                sState = 0;
    UInt                sTableType;
    idBool              sSkipTbsAcc;
    SChar             * sRowPtr;

    // Begin NTA[-1-]
    sLsnNTA1 = smLayerCallback::getLstUndoNxtLSN( aTrans );

    /* ------------------------------------------------
     * [1] table page list 해제
     * - memory table의 경우, page list entry에 할당된
     *   페이지를 free하여 시스템에 반환한다.
     * - disk table의 경우, segment를 해제함으로써
     *   테이블에 할당된 페이지들을 모두 시스템에 반환한다.
     * ----------------------------------------------*/
    sSlotHeader = (smpSlotHeader*)aHeader - 1;

    sPageID = SMP_SLOT_GET_PID((SChar *)sSlotHeader);

    sState = 1;

    sTableType = SMI_GET_TABLE_TYPE( aHeader );
    IDE_DASSERT( sTableType == SMI_TABLE_MEMORY   ||
                 sTableType == SMI_TABLE_META     ||
                 sTableType == SMI_TABLE_VOLATILE ||
                 sTableType == SMI_TABLE_DISK     ||
                 sTableType == SMI_TABLE_REMOTE );

    // Tablespace로의 접근 가능성 여부 ( 가능하면 ID_TRUE )
    //   - DROP/DISCARD된 Tablespace의 경우 접근이 불가능함.
    //   - DROP Table수행시 Tablespace에 IX락이 잡힌 상태이다.
    sSkipTbsAcc =  sctTableSpaceMgr::hasState( aHeader->mSpaceID,
                                               SCT_SS_SKIP_DROP_TABLE_CONTENT );

    if (sSkipTbsAcc == ID_TRUE )
    {
        // Tablespace에 접근하지 않는다.
        // 단 disk table인 경우 tablespace의 상태에 관계없이
        // index를 해제한다.
        if ((sTableType == SMI_TABLE_DISK) && (aFlag == ID_TRUE))
        {
            IDE_TEST( smLayerCallback::dropIndexes( aHeader )
                      != IDE_SUCCESS );
        }
    }
    else
    {
        if (sTableType == SMI_TABLE_DISK)
        {
            if ( sdpPageList::getTableSegDescPID(&(aHeader->mFixed.mDRDB))
                 != SD_NULL_PID )
            {
                IDE_TEST( sdpSegment::freeTableSeg4Entry(
                                              aStatistics,
                                              aHeader->mSpaceID,
                                              aTrans,
                                              aHeader->mSelfOID,
                                              &(aHeader->mFixed.mDRDB),
                                              SDR_MTX_LOGGING)
                          != IDE_SUCCESS );

                /* BUG-18101
                   원래 drop 된 table에 대해 runtime 정보를 해제하는 곳은
                   smcCatalogTable::finAllocedTableSlots()에서 였다.
                   하지만 dropPending시 호출되는 본 함수에서
                   table header 정보를 담고 있는 slot이 free되어 버려서
                   finAllocedTableSlots()에서 slot을 탐색할 때 검색이 안되어
                   drop 된 disk table에 대해서는 runtime 정보를 해제하지 못하고 있는
                   문제가 발견되었다.
                   제대로 처리하기 위해서는 철저한 설계에 바탕한 소스 리팩토링이
                   필요하기 때문에 일단 메모리 테이블과 마찬가지로
                   여기서 런타임 정보를 해제하도록 한다. */
                IDE_TEST(finRuntimeItem(aHeader) != IDE_SUCCESS);

                IDE_TEST( destroyRowTemplate( aHeader )!= IDE_SUCCESS );
            }
        }
        else if ((sTableType == SMI_TABLE_MEMORY) ||
                 (sTableType == SMI_TABLE_META)   ||
                 (sTableType == SMI_TABLE_REMOTE))
        {
            IDE_TEST( smpFixedPageList::freePageListToDB(
                                                  aTrans,
                                                  aHeader->mSpaceID,
                                                  aHeader->mSelfOID,
                                                  &(aHeader->mFixed.mMRDB))
                          != IDE_SUCCESS );

            IDE_TEST( smpVarPageList::freePageListToDB(aTrans,
                                                       aHeader->mSpaceID,
                                                       aHeader->mSelfOID,
                                                       aHeader->mVar.mMRDB)
                      != IDE_SUCCESS );

            IDE_TEST(finRuntimeItem(aHeader) != IDE_SUCCESS);
        }
        else if (sTableType == SMI_TABLE_VOLATILE)
        {
            IDE_TEST( svpFixedPageList::freePageListToDB(
                                                  aTrans,
                                                  aHeader->mSpaceID,
                                                  aHeader->mSelfOID,
                                                  &(aHeader->mFixed.mVRDB))
                      != IDE_SUCCESS );

            IDE_TEST( svpVarPageList::freePageListToDB(aTrans,
                                                       aHeader->mSpaceID,
                                                       aHeader->mSelfOID,
                                                       aHeader->mVar.mVRDB)
                      != IDE_SUCCESS );

            IDE_TEST(finRuntimeItem(aHeader) != IDE_SUCCESS);
        }
        else
        {
            IDE_ASSERT(0);
        }

        /* ------------------------------------------------
         * table의 index들을 모두 drop 시킴
         * - 일반적으로 a_flag 는 ID_TRUE 값을 가지며,
         *   refineCatalogTable에서 skip 하기 위해
         *   ID_FALSE 값을 가진다.
         * ----------------------------------------------*/
        if( aFlag == ID_TRUE )
        {
            IDE_TEST( smLayerCallback::dropIndexes( aHeader )
                      != IDE_SUCCESS );
        }
    }

    /* ------------------------------------------------
     * [2] table header의 index 정보를 저장한 variable
     *  slot을 free시킴
     * ----------------------------------------------*/
    if( smcTable::getIndexCount(aHeader) != 0)
    {
    // Begin NTA[-2-]
        IDE_TEST(freeIndexes(aStatistics,
                             aTrans,
                             aHeader,
                             sTableType)
                 != IDE_SUCCESS);
    // END NTA[ -2-]
    }

    // Table Header의 Index Latch를 파괴하고 해제
    IDE_TEST( finiAndFreeIndexLatch( aHeader )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "1.BUG-21545@smcTable::dropTablePending" );

    /* ------------------------------------------------
     * [3] table header의 column 정보를 저장한 variable
     *  slot을 free시킴
     * ----------------------------------------------*/
    if(aHeader->mColumns.fstPieceOID != SM_NULL_OID)
    {
        // Begin NTA[-3-]
        IDE_TEST( smcTable::freeColumns(aStatistics,
                                        aTrans,
                                        aHeader)
                  != IDE_SUCCESS);

        // END NTA[-3-]
    }

    /* ------------------------------------------------
     * [5] table header의 info 정보를 저장한 variable
     *  slot을 free시킴
     * ----------------------------------------------*/
    if(aHeader->mInfo.fstPieceOID != SM_NULL_OID)
    {
        // Begin NTA[-4-]
        sLsnNTA2 = smLayerCallback::getLstUndoNxtLSN( aTrans );

        /* ------------------------------------------------
         * info 정보를 저장한 variable slot의 해제 및 table header의
         * info 정보에 SM_NULL_OID를 설정
         * - SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_UPDATE_INFO 로깅
         * ----------------------------------------------*/
        IDE_TEST( smrUpdate::updateInfoAtTableHead(
                      NULL, /* idvSQL* */
                      aTrans,
                      SM_MAKE_PID(aHeader->mSelfOID),
                      SM_MAKE_OFFSET(aHeader->mSelfOID) + SMP_SLOT_HEADER_SIZE,
                      &(aHeader->mInfo),
                      SM_NULL_OID,
                      0,
                      SM_VCDESC_MODE_OUT) != IDE_SUCCESS );


        sRowOID = aHeader->mInfo.fstPieceOID;

        aHeader->mInfo.fstPieceOID = SM_NULL_OID;
        aHeader->mInfo.length = 0;
        aHeader->mInfo.flag = SM_VCDESC_MODE_OUT;

        /* ------------------------------------------------
         * catalog table에 variable slot을 해제하여 반환함.
         * ----------------------------------------------*/
        IDE_ASSERT( smmManager::getOIDPtr( 
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                        sRowOID,
                        (void**)&sRowPtr )
                    == IDE_SUCCESS );

        IDE_TEST( smpVarPageList::freeSlot(aTrans,
                                           SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SMC_CAT_TABLE->mVar.mMRDB,
                                           sRowOID,
                                           sRowPtr,
                                           &sLsnNTA2,
                                           SMP_TABLE_NORMAL)
                  != IDE_SUCCESS );
        // END   NTA[-4-]

    }

    // To Fix BUG-16752 Memory Table Drop시 Runtime Entry가 해제되지 않음
    //
    // Refine시 이미 Drop된 Table에 대해 Lock & Runtime Item을 할당하지
    // 않도록 하기 위함.
    //  => smpSlotHeader.mUsedFlag 를 ID_FALSE로 바꾼다.
    //     -> refine시 nextOIDall안쪽에서 skip

    //  (주의) 새로 생성되는 Table에 의해 사용되지 않도록
    //          catalog table의 free slot list에는 달지 않는다.
    IDE_TEST( smpFixedPageList::setFreeSlot(
                  aTrans,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  (SChar*) sSlotHeader,
                  SMP_TABLE_NORMAL )
              != IDE_SUCCESS );

    /* ------------------------------------------------
     * [6] table 제거에 대한 NTA 로깅
     * END NTA[-1-]
     * ----------------------------------------------*/
    IDE_TEST( smrLogMgr::writeNTALogRec(NULL, /* idvSQL* */
                                        aTrans,
                                        &sLsnNTA1,
                                        SMR_OP_NULL)
             != IDE_SUCCESS );


    IDE_TEST( smmDirtyPageMgr::insDirtyPage( 
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sPageID ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        (void)smmDirtyPageMgr::insDirtyPage(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sPageID);
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  PROJ-1362 QP - Large Record & Internal LOB 지원.
 *                table의 컬럼개수와 인덱스 개수 제약 풀기.
 *  table drop pending시에 불리는 함수로, table의 인덱스 정보를 담은
 *  variable slot을 해제한다.
 *  - code design
 *  for( i =0 ; i  < i < SMC_MAX_INDEX_OID_CNT; i++)
 *  do
 *    if( table헤더의 mIndex[i]  == SM_NULL_OID)
 *    then
 *         continue;
 *    fi
 *    트랜잭션의 last undo LSN을 얻는다;
 *    table 헤더의 mIndexes[i]정보에 SM_NULL_OID를 설정하는
 *    physical logging;
 *    table 헤더의 mIndexes[i]를 기억;
 *    table 헤더의 mIndexes[i]를 SM_NULL_OID으로 설정;
 *    table 헤더의 mIndexes[i]가 가르키고 있었던 variable slot해제
 *    (smpVarPageList::freeSlot);
 * done
 ***********************************************************************/
IDE_RC smcTable::freeIndexes(idvSQL          * aStatistics,
                             void            * aTrans,
                             smcTableHeader  * aHeader,
                             UInt              aTableType)
{
    smLSN      sLsnNTA2;
    SChar    * sIndexRowPtr;
    SChar    * sSrc;
    scGRID   * sIndexSegGRID;
    smOID      sVarOID;
    SChar    * sRowPtr;
    UInt       sIndexHeaderSize;
    UInt       i;
    UInt       j;

    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    for(i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++)
    {
        if( aHeader->mIndexes[i].fstPieceOID == SM_NULL_OID)
        {
            continue;
        }//if

        // Begin NTA[-2-]
        sLsnNTA2 = smLayerCallback::getLstUndoNxtLSN( aTrans );

        // disk index의 free segment
        if (aTableType == SMI_TABLE_DISK)
        {
            IDE_ASSERT( smmManager::getOIDPtr( 
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    aHeader->mIndexes[i].fstPieceOID,
                                    (void**)&sIndexRowPtr )
                        == IDE_SUCCESS );

            sSrc  = sIndexRowPtr + ID_SIZEOF(smVCPieceHeader);

            for( j = 0; j < aHeader->mIndexes[i].length; j += sIndexHeaderSize,
                     sSrc += sIndexHeaderSize )
            {
                sIndexSegGRID = smLayerCallback::getIndexSegGRIDPtr( sSrc );

                if(SC_GRID_IS_NOT_NULL(*sIndexSegGRID))
                {
                    IDE_TEST( sdpSegment::freeIndexSeg4Entry(
                                          aStatistics,
                                          SC_MAKE_SPACE( *sIndexSegGRID ),
                                          aTrans,
                                          aHeader->mIndexes[i].fstPieceOID
                                          + j
                                          + ID_SIZEOF(smVCPieceHeader),
                                          SDR_MTX_LOGGING)
                              != IDE_SUCCESS );
                }//if
            }//for j
        }//if
        /* ------------------------------------------------
         * index 정보를 저장한 variable slot의 해제 및 table header의
         * index 정보에 SM_NULL_OID를 설정
         * - SMR_LT_UPDATE의 SMR_SMC_TABLEHEADER_UPDATE_INDEX 로깅
         * ----------------------------------------------*/
        IDE_TEST( smrUpdate::updateIndexAtTableHead( NULL, /* idvSQL* */
                                                     aTrans,
                                                     aHeader->mSelfOID,
                                                     &(aHeader->mIndexes[i]),
                                                     i,
                                                     SM_NULL_OID,
                                                     0,
                                                     SM_VCDESC_MODE_OUT )
                  != IDE_SUCCESS );


        sVarOID = aHeader->mIndexes[i].fstPieceOID;

        aHeader->mIndexes[i].fstPieceOID = SM_NULL_OID;
        aHeader->mIndexes[i].length      = 0;
        aHeader->mIndexes[i].flag        = SM_VCDESC_MODE_OUT;

        /* ------------------------------------------------
         * catalog table에 variable slot을 해제하여 반환함.
         * ----------------------------------------------*/
        IDE_ASSERT( smmManager::getOIDPtr( 
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sVarOID,
                                    (void**)&sRowPtr )
                    == IDE_SUCCESS );

        IDE_TEST( smpVarPageList::freeSlot(aTrans,
                                           SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SMC_CAT_TABLE->mVar.mMRDB,
                                           sVarOID,
                                           sRowPtr,
                                           &sLsnNTA2,
                                           SMP_TABLE_NORMAL)
                  != IDE_SUCCESS );


        // END   NTA[-2-]
    }//i
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description :
 * disk index에 대한 제거작업은 disk G.C에 의해
 * 처리되므로, 본 함수에서는 skip 하도록 처리한다.
 ***********************************************************************/
IDE_RC smcTable::dropIndexList( smcTableHeader * aHeader )
{

    UInt       i;
    UInt       sState = 0;
    UInt       sIndexHeaderSize;
    SChar    * sIndexHeader;
    smxTrans * sTrans;
    scGRID   * sIndexSegGRID;

    if(aHeader->mDropIndexLst != NULL)
    {
        sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

        IDE_TEST( smcTable::latchExclusive( aHeader ) != IDE_SUCCESS );
        sState = 1;

        for(i = 0, sIndexHeader = (SChar *)aHeader->mDropIndexLst;
            i < aHeader->mDropIndex;
            i++, sIndexHeader += sIndexHeaderSize)
        {
            IDE_TEST(smxTransMgr::alloc(&sTrans) != IDE_SUCCESS);
            sState = 2;

            IDE_ASSERT( sTrans->begin( NULL,
                                       ( SMI_TRANSACTION_REPL_NONE |
                                         SMI_COMMIT_WRITE_NOWAIT ),
                                       SMX_NOT_REPL_TX_ID )
                        == IDE_SUCCESS);
            sState = 3;

            if( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_TRUE )
            {
                sIndexSegGRID  =  smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );
                if (SC_GRID_IS_NOT_NULL(*sIndexSegGRID))
                {
                    IDE_TEST( sdpSegment::freeIndexSeg4Entry(
                                  NULL,
                                  SC_MAKE_SPACE(*sIndexSegGRID),
                                  sTrans,
                                  ((smnIndexHeader*)sIndexHeader)->mSelfOID,
                                  SDR_MTX_LOGGING)
                              != IDE_SUCCESS );
                }
            }

            IDE_TEST( smcTable::dropIndexPending( sIndexHeader )
                      != IDE_SUCCESS );

            sState = 2;
            IDE_TEST(sTrans->commit() != IDE_SUCCESS);

            sState = 1;
            IDE_TEST(smxTransMgr::freeTrans(sTrans) != IDE_SUCCESS);
        }

        IDE_TEST(mDropIdxPagePool.memfree(aHeader->mDropIndexLst)
                 != IDE_SUCCESS);

        aHeader->mDropIndexLst = NULL;
        aHeader->mDropIndex = 0;

        sState = 0;
        IDE_TEST( smcTable::unlatch( aHeader ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 3:
            IDE_ASSERT( sTrans->abort( ID_FALSE, /* aIsLegacyTrans */
                                       NULL      /* aLegacyTrans */ ) == IDE_SUCCESS );
        case 2:
            IDE_ASSERT( smxTransMgr::freeTrans(sTrans) == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smcTable::unlatch( aHeader ) == IDE_SUCCESS );
            break;
        default:
            break;
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : Table Header에서 aIndex 번째 Column의 Column Info와
 *               그 Column Info의 위치를 나타내는 OID를 가져온다.
 ***********************************************************************/
smiColumn* smcTable::getColumnAndOID(const void* aTableHeader,
                                     const UInt  aIndex,
                                     smOID*      aOID)
{
    UInt                   sCurSize;
    const smcTableHeader * sTableHeader;
    smVCPieceHeader      * sVCPieceHeader;
    UInt                   sOffset;
    smiColumn            * sColumn = NULL;

    IDE_DASSERT( aTableHeader != NULL );

    sTableHeader = (const smcTableHeader*) aTableHeader;

    //테이블의 컬럼정보를 담은 시작  variable slot.
    IDE_ASSERT( smmManager::getOIDPtr( 
                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                sTableHeader->mColumns.fstPieceOID,
                                (void**)&sVCPieceHeader )
                == IDE_SUCCESS );
    sOffset = sTableHeader->mColumnSize * aIndex;
    *aOID = sTableHeader->mColumns.fstPieceOID;

    /* BUG-22367: 많은 컬럼 갯수를 가진 테이블 생성시 비정상 종료
     *
     * sCurSize < sOffset => sCurSize <= sOffset , = 조건이 빠졌음
     * */
    for( sCurSize = sVCPieceHeader->length; sCurSize <= sOffset; )
    {
        sOffset -= sCurSize;

        // next variable slot으로 이동.
        if( sVCPieceHeader->nxtPieceOID == SM_NULL_OID )
        {
            ideLog::log( IDE_SM_0,
                         "Index:    %u\n"
                         "CurSize:  %u\n"
                         "Offset:   %u\n",
                         aIndex,
                         sCurSize,
                         sOffset );

            ideLog::log( IDE_SM_0, "[ Table header ]" );
            ideLog::logMem( IDE_SM_0,
                            (UChar*)aTableHeader,
                            ID_SIZEOF(smcTableHeader) );

            sColumn = NULL;

            IDE_CONT( SKIP );
        }
        *aOID = sVCPieceHeader->nxtPieceOID;
        IDE_ASSERT( smmManager::getOIDPtr( 
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    sVCPieceHeader->nxtPieceOID,
                                    (void**)&sVCPieceHeader )
                    == IDE_SUCCESS );

        sCurSize = sVCPieceHeader->length;
    }//for

    /* Column Info의 정확한 위치를 OID로 반환한다. */
    *aOID = *aOID + ID_SIZEOF(smVCPieceHeader) + sOffset;

    sColumn = (smiColumn*)
        ((UChar*) sVCPieceHeader + ID_SIZEOF(smVCPieceHeader)+ sOffset);

    IDE_EXCEPTION_CONT( SKIP );

    /*
     * BUG-26933 [5.3.3 release] CodeSonar::Null Pointer Dereference (2)
     */
    IDE_DASSERT( sColumn != NULL );

    if( aIndex !=
        ( sColumn->id & SMI_COLUMN_ID_MASK ) )
    {
        ideLog::log( IDE_DUMP_0,
                     "ColumnID( %u ) != Index( %u )\n",
                     sColumn->id & SMI_COLUMN_ID_MASK,
                     aIndex );

        ideLog::logMem( IDE_DUMP_0,
                        (UChar*)sColumn,
                        ID_SIZEOF( smiColumn ) );

        ideLog::logMem( IDE_DUMP_0,
                        (UChar*)sTableHeader,
                        ID_SIZEOF( smcTableHeader ) );

        IDE_ASSERT( 0 );
    }

    return sColumn;
}

UInt smcTable::getColumnIndex( UInt aColumnID )
{
    return ( aColumnID & SMI_COLUMN_ID_MASK );
}

UInt smcTable::getColumnSize(void *aTableHeader)
{
    IDE_DASSERT(aTableHeader!=NULL);
    return ((smcTableHeader *)aTableHeader)->mColumnSize;
}


UInt smcTable::getColumnCount(const void *aTableHeader)
{
    IDE_DASSERT(aTableHeader!=NULL);
    return ((smcTableHeader *)aTableHeader)->mColumnCount;
}


/***********************************************************************
 *
 * Description :
 *  lob column의 갯수를 반환한다.
 *
 *  aTableHeader - [IN] 테이블 헤더
 *
 **********************************************************************/
UInt smcTable::getLobColumnCount(const void *aTableHeader)
{
    IDE_DASSERT(aTableHeader!=NULL);

#ifdef DEBUG
    const smiColumn *sColumn;

    UInt       sColumnCnt = getColumnCount( aTableHeader );
    UInt       i;
    UInt       sLobColumnCnt = 0;

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn( aTableHeader, i );

        if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) ==
           SMI_COLUMN_TYPE_LOB )
        {
            sLobColumnCnt++;
        }
    }

    IDE_DASSERT( ((smcTableHeader *)aTableHeader)->mLobColumnCount
                 == sLobColumnCnt );
#endif

    return ((smcTableHeader *)aTableHeader)->mLobColumnCount;
}

/***********************************************************************
 *
 * Description :
 *  lob column을 반환한다.
 *
 *  aTableHeader - [IN] 테이블 헤더
 *  aSpaceID     - [IN] Lob Segment Space ID
 *  aSegPID      - [IN] Lob segment PID
 *
 **********************************************************************/
const smiColumn* smcTable::getLobColumn( const void *aTableHeader,
                                         scSpaceID   aSpaceID,
                                         scPageID    aSegPID )
{
    const smiColumn *sColumn = NULL;
    const smiColumn *sLobColumn = NULL;
    UInt             sColumnCnt;
    scGRID           sSegGRID;
    UInt             i;

    IDE_DASSERT(aTableHeader!=NULL);

    SC_MAKE_GRID( sSegGRID,
                  aSpaceID,
                  aSegPID,
                  0 );

    sColumnCnt = getColumnCount( aTableHeader );

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn( aTableHeader, i );

        if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) ==  SMI_COLUMN_TYPE_LOB )
        {
            if( SC_GRID_IS_EQUAL(sSegGRID, sColumn->colSeg) )
            {
                sLobColumn = sColumn;
                break;
            }
        }
    }

    return sLobColumn;
}

/* 디스크 Page List Entry를 반환한다. */
void* smcTable::getDiskPageListEntry(void *aTableHeader)
{
    IDE_ASSERT(aTableHeader!=NULL);

    return (void*)&(((smcTableHeader *)aTableHeader)->mFixed.mDRDB);
}

/* 디스크 Page List Entry를 반환한다. */
void* smcTable::getDiskPageListEntry( smOID  aTableOID )
{
    smcTableHeader *sTableHeader;

    IDE_ASSERT( aTableOID != SM_NULL_OID );

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aTableOID,
                                                 (void**)&sTableHeader )
                == IDE_SUCCESS );

    return getDiskPageListEntry( sTableHeader );
}


ULong smcTable::getMaxRows(void *aTableHeader)
{

    IDE_DASSERT(aTableHeader!=NULL);
    return ((smcTableHeader *)aTableHeader)->mMaxRow;

}

smOID smcTable::getTableOID(const void *aTableHeader)
{

    IDE_DASSERT(aTableHeader!=NULL);
    return ((smcTableHeader *)aTableHeader)->mSelfOID;

}

/***********************************************************************
 * Description :
 * db refine과정에서 table backup file들에 대한 제거를 처리하는 함수로
 * disk table에서는 지원할 필요없다.
 ***********************************************************************/
IDE_RC smcTable::deleteAllTableBackup()
{
    DIR                  *sDIR       = NULL;
    struct  dirent       *sDirEnt    = NULL;
    struct  dirent       *sResDirEnt = NULL;
    SInt                  sOffset;
    SChar                 sFullFileName[SM_MAX_FILE_NAME];
    SChar                 sFileName[SM_MAX_FILE_NAME];
    SInt                  sRC;
    idBool                sCheck1;
    idBool                sCheck2;

    /* BUG-16161: Add Column이 실행후 다시 다시 Add Column을 수행하면
     * Session이 Hang상태로 빠집니다.: Restart Recovery시 Backup파일을
     * 이용한 Transaction의 로그가 Disk에 Sync되지않은 상태에서 여기서
     * Backup파일을 지운다면, Failure발생시 해당 Abort될 Transaction을
     * 다시 Abort할수가 없다. 때문에 여기서 log Flush를 수행하여 Abort가
     * 완전히 Disk에 반영되는 것을 보장하여야 한다.
     * */
    IDE_TEST( smrLogMgr::syncToLstLSN( SMR_LOG_SYNC_BY_REF )
              != IDE_SUCCESS );

    /* smcTable_deleteAllTableBackup_malloc_DirEnt.tc */
    /* smcTable_deleteAllTableBackup.tc */
    IDU_FIT_POINT_RAISE( "smcTable::deleteAllTableBackup::malloc::DirEnt",
                          insufficient_memory );
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMC,
                                       ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                                       (void**)&(sDirEnt),
                                       IDU_MEM_FORCE ) != IDE_SUCCESS,
                    insufficient_memory );

    /* smcTable_deleteAllTableBackup.tc */
    IDU_FIT_POINT_RAISE( "smcTable::deleteAllTableBackup::opendir", err_open_dir );
    sDIR = idf::opendir(getBackupDir());
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);

    errno = 0;
    /* smcTable_deleteAllTableBackup.tc */
    IDU_FIT_POINT_RAISE( "smcTable::deleteAllTableBackup::readdir_r", err_read_dir );
    sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt) ;
    IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );

    while(sResDirEnt != NULL)
    {
        idlOS::strcpy(sFileName, (const char*)sResDirEnt->d_name);
        if (idlOS::strcmp(sFileName, ".") == 0 || idlOS::strcmp(sFileName, "..") == 0)
        {
            /*
             * BUG-31529  [sm] errno must be initialized before calling readdir_r.
             */
	    errno = 0;
            sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
            errno = 0;

            continue;
        }

        sOffset = (SInt)idlOS::strlen(sFileName) - 4;

        if(sOffset > 4)
        {
            if(idlOS::strncmp(sFileName + sOffset, SM_TABLE_BACKUP_EXT, 4) == 0)
            {
                idlOS::snprintf(sFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                                getBackupDir(), IDL_FILE_SEPARATOR,sFileName);

                // BUG-20048
                // 파일의 헤더를 읽고, 알티베이스 백업파일이 맞는지를 검사한다.
                IDE_TEST( IDE_SUCCESS !=
                          smiTableBackup::isBackupFile(sFullFileName, &sCheck1) );
                IDE_TEST( IDE_SUCCESS !=
                          smiVolTableBackup::isBackupFile(sFullFileName, &sCheck2) );

                // 알티베이스 백업파일이 맞으면 삭제한다.
                if( (ID_TRUE == sCheck1) || (ID_TRUE == sCheck2) )
                {
                    while(1)
                    {
                        IDE_TEST_RAISE(idf::unlink(sFullFileName) != 0,
                                       err_file_unlink);
                        //==========================================================
                        // To Fix BUG-13924
                        //==========================================================
                        ideLog::log(SM_TRC_LOG_LEVEL_MRECORD,
                                    SM_TRC_MRECORD_FILE_UNLINK,
                                    sFullFileName);
                        break;
                    }//while
                }
            }
        }

        errno = 0;
        sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }//while

    idf::closedir( sDIR );
    sDIR = NULL;

    IDE_TEST( iduMemMgr::free( sDirEnt ) != IDE_SUCCESS );
    sDirEnt = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_file_unlink);
    {
        IDE_SET(ideSetErrorCode(smERR_FATAL_FileDelete, sFullFileName));
    }
    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotOpenDir, getBackupDir() ) );
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, getBackupDir() ) );
    }
    IDE_EXCEPTION( insufficient_memory );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION_END;

    if ( sDIR != NULL )
    {
        idf::closedir( sDIR );
        sDIR = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    if ( sDirEnt != NULL )
    {
        (void)iduMemMgr::free( sDirEnt );
        sDirEnt = NULL;
    }
    else
    {
        /* Nothing to do */
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :  memory table의 onlinebackup시 table backup file에 대한
 * backup본 생성을 처리하는 함수로 disk table에 대하여 지원할 필요없음
 **********************************************************************/
IDE_RC smcTable::copyAllTableBackup(SChar      * aSrcDir, 
                                    SChar      * aTargetDir)
{

    iduFile            sFile;
    struct  dirent    *sDirEnt    = NULL;
    struct  dirent    *sResDirEnt = NULL;
    DIR               *sDIR       = NULL;
    SInt               sOffset;
    SChar              sStrFullFileName[SM_MAX_FILE_NAME];
    SChar              sFileName[SM_MAX_FILE_NAME];
    SInt               sRC;

    /* smcTable_copyAllTableBackup_malloc_DirEnt.tc */
    IDU_FIT_POINT("smcTable::copyAllTableBackup::malloc::DirEnt");
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_SM_SMC,
                               ID_SIZEOF(struct dirent) + SM_MAX_FILE_NAME,
                               (void**)&(sDirEnt),
                               IDU_MEM_FORCE)
             != IDE_SUCCESS);

    sDIR = idf::opendir(aSrcDir);
    IDE_TEST_RAISE(sDIR == NULL, err_open_dir);
    IDE_TEST(sFile.initialize( IDU_MEM_SM_SMC,
                               1, /* Max Open FD Count */
                               IDU_FIO_STAT_OFF,
                               IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    errno = 0;
    sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt) ;
    IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
    errno = 0;

    while(sResDirEnt != NULL)
    {
        idlOS::strcpy(sFileName, (const char*)sResDirEnt->d_name);
        if (idlOS::strcmp(sFileName, ".") == 0 ||
            idlOS::strcmp(sFileName, "..") == 0)
        {
            errno = 0;
            sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
            IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
            errno = 0;

            continue;
        }

        sOffset = (SInt)idlOS::strlen(sFileName) - 4;
        if(sOffset > 4)
        {
            if(idlOS::strncmp(sFileName + sOffset,
                              SM_TABLE_BACKUP_EXT, 4) == 0)
            {
                idlOS::snprintf(sStrFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                                aSrcDir, IDL_FILE_SEPARATOR,
                                sFileName);
                IDE_TEST(sFile.setFileName(sStrFullFileName)!= IDE_SUCCESS);
                IDE_TEST(sFile.open()!= IDE_SUCCESS);
                idlOS::snprintf(sStrFullFileName, SM_MAX_FILE_NAME, "%s%c%s",
                               aTargetDir, IDL_FILE_SEPARATOR,
                               sFileName);

                IDE_TEST(sFile.copy(NULL, /* idvSQL* */
                                    sStrFullFileName,
                                    ID_FALSE)!= IDE_SUCCESS);

                IDE_TEST(sFile.close()!= IDE_SUCCESS);
            }
        }
        errno = 0;
        sRC = idf::readdir_r(sDIR, sDirEnt, &sResDirEnt);
        IDE_TEST_RAISE( (sRC != 0) && (errno != 0), err_read_dir );
        errno = 0;
    }
    IDE_TEST(sFile.destroy() != IDE_SUCCESS);
    idf::closedir(sDIR);
    sDIR = NULL;

    if(sDirEnt != NULL)
    {
        IDE_TEST(iduMemMgr::free(sDirEnt) != IDE_SUCCESS);
        sDirEnt = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_open_dir);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_NotDir));
    }
    IDE_EXCEPTION(err_read_dir);
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_CannotReadDir, aSrcDir ) );
    }
    IDE_EXCEPTION_END;

    if(sDirEnt != NULL)
    {
        (void)iduMemMgr::free(sDirEnt);
        sDirEnt = NULL;
    }

    // fix BUG-25556 : [codeSonar] closedir 추가.
    if(sDIR != NULL)
    {
        idf::closedir(sDIR);
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : FOR A5
 * fix - BUG-25611
 * memory table에 대한 LogicalUndo시, 아직 Refine돼지 않은 상태이기 때문에
 * 임시로 테이블을 Refine해준다.
 *
 * 반드시 finalizeTable4LogicalUndo와 쌍으로 호출되어야 한다.
 ***********************************************************************/
IDE_RC smcTable::prepare4LogicalUndo( smcTableHeader * aTable )
{
    IDE_TEST( initLockAndRuntimeItem( aTable ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : FOR A5
 * fix - BUG-25611
 * memory table에 대한 LogicalUndo시, 임시로 Refine된 테이블의 페이지를
 * 정리한다.
 *
 * 정리 대상
 * 1) FreePage(Fixed & Variable) - 초기화됨
 * 2) AllocatePage들의 Scanlist - 초기화됨
 * 3) Private Page - Table에게 반환됨
 * 4) RuntimeEntry - 초기화됨
 *
 * 반드시 prepare4LogicalUndo와 쌍으로 호출되어야 한다.
 ***********************************************************************/
IDE_RC smcTable::finalizeTable4LogicalUndo(smcTableHeader * aTable,
                                           void           * aTrans )
{
    smxTrans       *sTrans;

    sTrans = (smxTrans*)aTrans;

    // 1) FreePage(Fixed & Variable) - 초기화됨
    initAllFreePageHeader(aTable);
    // 2) AllocatePage들의 Scanlist - 초기화됨
    IDE_TEST( smpFixedPageList::resetScanList( aTable->mSpaceID,
                                             &(aTable->mFixed.mMRDB))
                     != IDE_SUCCESS);
    // 3) Private Page - Table에게 반환됨
    IDE_TEST( sTrans->finAndInitPrivatePageList() != IDE_SUCCESS );
    // 4) RuntimeEntry - 초기화됨
    IDE_TEST (finLockAndRuntimeItem(aTable) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




/***********************************************************************
 * Table에 할당된 모든 FreePageHeader를 초기화
 *
 * aTableHeader - [IN] Table의 Header
 ***********************************************************************/
void smcTable::initAllFreePageHeader( smcTableHeader* aTableHeader )
{
    UInt i;

    switch( SMI_GET_TABLE_TYPE( aTableHeader ) )
    {
        case SMI_TABLE_MEMORY :
        case SMI_TABLE_META :
            smpFreePageList::initAllFreePageHeader(aTableHeader->mSpaceID,
                                                   &(aTableHeader->mFixed.mMRDB));

            for( i = 0;
                 i < SM_VAR_PAGE_LIST_COUNT ;
                 i++ )
            {
                smpFreePageList::initAllFreePageHeader(aTableHeader->mSpaceID,
                                                       &(aTableHeader->mVar.mMRDB[i]));
            }
            break;
        case SMI_TABLE_VOLATILE :
            svpFreePageList::initAllFreePageHeader(aTableHeader->mSpaceID,
                                                   &(aTableHeader->mFixed.mVRDB));

            for( i = 0;
                 i < SM_VAR_PAGE_LIST_COUNT ;
                 i++ )
            {
                svpFreePageList::initAllFreePageHeader(aTableHeader->mSpaceID,
                                                       &(aTableHeader->mVar.mVRDB[i]));
            }
            break;
        default :
            IDE_ASSERT(0);
            break;
    }

    return;
}

/***********************************************************************
 * Description : table의 record 개수를 반환한다.
 * table 타입에 따른 page list entry의 record 개수
 * 계산하는 함수를 각각 호출
 * - 2nd code design
 *   + table 타입에 따라 포함하는 record의 개수를 구한다.
 *     if (memory table)
 *        - fixed page list entry의 record 개수 얻음
 *     else if (disk table or temporary table)
 *        - disk page list entry의 record 개수 얻음
 *     endif
 ***********************************************************************/
IDE_RC smcTable::getRecordCount( smcTableHeader* aHeader,
                                 ULong         * aRecordCount )
{
    UInt sTableType;

    IDE_DASSERT( aHeader != NULL );
    IDE_DASSERT( aRecordCount != NULL );

    sTableType = SMI_GET_TABLE_TYPE( aHeader );

    switch (sTableType)
    {
        case SMI_TABLE_MEMORY :
        case SMI_TABLE_META :
        case SMI_TABLE_REMOTE:
            /* memory table의 fixed page list entry의 record 개수 계산 */
           *aRecordCount = smpFixedPageList::getRecordCount( &(aHeader->mFixed.mMRDB) );
            break;
        case SMI_TABLE_DISK :
            /* disk table의 page list entry의 record 개수 계산 */
            IDE_TEST( sdpPageList::getRecordCount(
                          NULL, /* idvSQL* */
                          &(aHeader->mFixed.mDRDB),
                          aRecordCount,
                          (sTableType == SMI_TABLE_DISK ? ID_TRUE : ID_FALSE))
                      != IDE_SUCCESS);
            break;
        case SMI_TABLE_VOLATILE :
            /* volatile table의 fixed page list entry의 record 개수 계산 */
            *aRecordCount = svpFixedPageList::getRecordCount( &(aHeader->mFixed.mVRDB) );
            break;
        case SMI_TABLE_TEMP_LEGACY :
        default :
            /* IDE_ASSERT(0); */
            break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table의 record 개수를 변경한다.
 *
 * aHeader        - [IN] Table Header
 * aRecordCount   - [IN] Record Count
 ***********************************************************************/
IDE_RC smcTable::setRecordCount(smcTableHeader * aHeader,
                                ULong            aRecordCount)
{
    IDE_DASSERT( aHeader != NULL );

    switch( SMI_GET_TABLE_TYPE( aHeader ) )
    {
        case SMI_TABLE_MEMORY :
        case SMI_TABLE_META :
            smpFixedPageList::setRecordCount(&(aHeader->mFixed.mMRDB),
                                             aRecordCount);
            break;
        case SMI_TABLE_VOLATILE :
            svpFixedPageList::setRecordCount(&(aHeader->mFixed.mVRDB),
                                             aRecordCount);
            break;

        case SMI_TABLE_DISK :
            /* 이 함수는 Memory Table의 Record 갯수의
             * 변경에만 쓰인다. */
            IDE_ASSERT(0 );

        case SMI_TABLE_TEMP_LEGACY :
        default :
            IDE_ASSERT(0);
            break;
    }

    return IDE_SUCCESS;
}

const void * smcTable::getTableIndexByID( void       * aHeader,
                                          const UInt   aId )
{
    smcTableHeader  *sHeader;
    smnIndexHeader  *sIndexHeader = NULL;
    smVCPieceHeader *sVCPieceHdr  = NULL;
    UInt             i = 0;
    UInt             sCurSize = 0;

    sHeader = (smcTableHeader*) aHeader;
    for(i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++)
    {
        if(sHeader->mIndexes[i].fstPieceOID == SM_NULL_OID)
        {
            continue;
        }

        IDE_TEST( sHeader->mIndexes[i].length <= 0 );

        IDE_ASSERT( smmManager::getOIDPtr(
                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                sHeader->mIndexes[i].fstPieceOID,
                                (void**)&sVCPieceHdr )
                    == IDE_SUCCESS );

        sCurSize = 0;
        while( sCurSize < sHeader->mIndexes[i].length )
        {
            sIndexHeader = (smnIndexHeader*)((UChar*)sVCPieceHdr +
                                             ID_SIZEOF(smVCPieceHeader) +
                                             sCurSize);
            if( sIndexHeader->mId == aId )
            {
                break;
            }
            sCurSize += smLayerCallback::getSizeOfIndexHeader();
        }

        /* BUG-34018 만약 한 테이블의 Index Header 개수가 47개를 넘어가면,
         * 서버는 Index Id를 바탕으로 Index Header를 찾을 수 없음. */
        if( sIndexHeader->mId == aId )
        {
            break;
        }
    }//for i;

    IDE_TEST( sIndexHeader      == NULL );
    IDE_TEST( sIndexHeader->mId != aId );

    return sIndexHeader;

    IDE_EXCEPTION_END;

    ideLog::log( IDE_SERVER_0,
                 "Index not found ("
                 "Table OID : %u, "
                 "Index ID : %u, "
                 "Index array ID : %u )\n",
                 sHeader->mSelfOID,
                 aId,
                 i );

    ideLog::log( IDE_SERVER_0,
                 "Index array\n" );

    for(i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++)
    {
        ideLog::log( IDE_SERVER_0,
                     "[%u] "
                     "fstPieceOID : %u "
                     "Length : %u\n",
                     i,
                     sHeader->mIndexes[i].fstPieceOID,
                     sHeader->mIndexes[i].length );

        if(sHeader->mIndexes[i].fstPieceOID == SM_NULL_OID)
        {
            continue;
        }

        IDE_ASSERT( smmManager::getOIDPtr(
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                        sHeader->mIndexes[i].fstPieceOID,
                        (void**)&sVCPieceHdr )
                    == IDE_SUCCESS );

        sCurSize = 0;
        while( sCurSize < sHeader->mIndexes[i].length )
        {
            sIndexHeader = (smnIndexHeader*)((UChar*)sVCPieceHdr +
                                             ID_SIZEOF(smVCPieceHeader) +
                                             sCurSize);
            ideLog::log( IDE_SERVER_0,
                         "Index ID : %u\n",
                         sIndexHeader->mId );

            sCurSize += smLayerCallback::getSizeOfIndexHeader();
        }
    }

    IDE_ASSERT( 0 );

    return NULL;
}

/***********************************************************************
 * Description : 해당 파일이 제거 될때까지 대기
 ***********************************************************************/
IDE_RC smcTable::waitForFileDelete( idvSQL *aStatistics, SChar* aStrFileName )
{
    UInt        sCheckSessionCnt = 0;
    idvTime     sInitTime;
    idvTime     sCurTime;
     
    IDV_TIME_GET( &sInitTime );
    while(1)
    {
        IDU_FIT_POINT( "1.BUG-38254@smcTable::waitForFileDelete" );

        if(idf::access(aStrFileName, F_OK) != 0 )
        {
            break;
        }

        idlOS::sleep(3);

        sCheckSessionCnt++;
        if( sCheckSessionCnt >= MAX_CHECK_CNT_FOR_SESSION )
        {
            IDE_TEST( iduCheckSessionEvent( aStatistics )
                      != IDE_SUCCESS );
            sCheckSessionCnt = 0;

            IDV_TIME_GET( &sCurTime );

            IDE_TEST( smuProperty::getTableBackupTimeOut() <
                      (IDV_TIME_DIFF_MICRO(&sInitTime, &sCurTime)/1000000 ));
        }
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table의 모든 index를 enable 또는 disable 상태로 변경
 ***********************************************************************/
IDE_RC smcTable::setUseFlagOfAllIndex(void            * aTrans,
                                      smcTableHeader  * aHeader,
                                      UInt              aFlag)
{
    UInt             i;
    UInt             j;
    UInt             sState = 0;
    UInt             sFlag  = 0;
    SChar          * sCurIndexHeader;
    scPageID         sPageID = SC_NULL_PID;
    UInt             sIndexHeaderSize;

    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    for(i = 0 ; i < SMC_MAX_INDEX_OID_CNT ; i++)
    {
        if( aHeader->mIndexes[i].fstPieceOID == SM_NULL_OID)
        {
            continue;
        }

        sPageID = SM_MAKE_PID( aHeader->mIndexes[i].fstPieceOID );

        IDE_ASSERT( smmManager::getOIDPtr( 
                        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                        aHeader->mIndexes[i].fstPieceOID + ID_SIZEOF(smVCPieceHeader),
                        (void**)&sCurIndexHeader )
                    == IDE_SUCCESS );

        for( j = 0;
             j < aHeader->mIndexes[i].length;
             j += sIndexHeaderSize,
                 sCurIndexHeader += sIndexHeaderSize )
        {
            sFlag = smLayerCallback::getFlagOfIndexHeader( sCurIndexHeader ) & ~SMI_INDEX_USE_MASK;

            sFlag |= aFlag;

            IDE_TEST( smrUpdate::setIndexHeaderFlag( NULL, /* idvSQL* */
                                                     aTrans,
                                                     aHeader->mIndexes[i].fstPieceOID,
                                                     j + ID_SIZEOF(smVCPieceHeader),
                                                     smLayerCallback::getFlagOfIndexHeader( sCurIndexHeader ),
                                                     sFlag )
                      != IDE_SUCCESS );


            smLayerCallback::setFlagOfIndexHeader( sCurIndexHeader, sFlag );
            sState = 1;
        }

        sState = 0;
        IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                              SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                              sPageID) 
                  != IDE_SUCCESS );
    }

    if(aFlag == SMI_INDEX_USE_DISABLE)
    {
       sFlag = aHeader->mFlag;
       sFlag &= ~(SMI_TABLE_DISABLE_ALL_INDEX_MASK);
       sFlag |= SMI_TABLE_DISABLE_ALL_INDEX;
    }
    else if(aFlag == SMI_INDEX_USE_ENABLE)
    {
       sFlag = aHeader->mFlag;
       sFlag &= ~(SMI_TABLE_DISABLE_ALL_INDEX_MASK);
       sFlag |= SMI_TABLE_ENABLE_ALL_INDEX;
    }

    IDE_TEST( smrUpdate::updateFlagAtTableHead(NULL, /* idvSQL* */
                                               aTrans,
                                               aHeader,
                                               sFlag)
              != IDE_SUCCESS );


    aHeader->mFlag = sFlag;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                  SM_MAKE_PID(aHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    // fix BUG-14252
    if (sState != 0)
    {
       (void)smmDirtyPageMgr::insDirtyPage(
                                   SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                   sPageID);
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : table 타입에 따라 table이 할당한 page 개수를 구한다.
 * - 2nd code design
 *   + table 타입에 따른 table 마다 할당된 page의 총개수 반환
 *     if (memory table)
 *        - fixed page list entry의 할당된 page 개수를 더함
 *        - variable page list entry들의 할당된 page 개수 더함
 *     else if (disk table or temporary table)
 *        - disk page list entry의 할당된 page 개수 더함
 *     endif
 ***********************************************************************/
IDE_RC smcTable::getTablePageCount( void * aHeader, ULong* aPageCnt )
{
    ULong           sNeedPage = 0;
    smcTableHeader* sHeader = (smcTableHeader*)aHeader;

    switch( SMI_GET_TABLE_TYPE( sHeader ) )
    {
        case SMI_TABLE_MEMORY :
        case SMI_TABLE_META :
            /* memory table의 할당된 page 개수 구하기 */
            sNeedPage = smpManager::getAllocPageCount(&(sHeader->mFixed.mMRDB))
                + smpManager::getAllocPageCount(sHeader->mVar.mMRDB);
            break;
        case SMI_TABLE_DISK :
            /* disk table의 할당된 page 개수 구하기 */
            /* BUGBUG 페이지 갯수를 Return하는 함수 구현. */
            IDE_TEST( sdpPageList::getAllocPageCnt(
                          NULL, /* idvSQL* */
                          sHeader->mSpaceID,
                          &( sHeader->mFixed.mDRDB.mSegDesc ),
                          &sNeedPage )
                      != IDE_SUCCESS );
            break;
        case SMI_TABLE_VOLATILE :
            /* memory table의 할당된 page 개수 구하기 */
            sNeedPage = svpManager::getAllocPageCount(&(sHeader->mFixed.mVRDB))
                + svpManager::getAllocPageCount(sHeader->mVar.mVRDB);
            break;
        case SMI_TABLE_TEMP_LEGACY :
        default :
            IDE_ASSERT(0);
            break;
    }

    *aPageCnt = sNeedPage;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 총 index 개수 증가
 ***********************************************************************/
void smcTable::addToTotalIndexCount( UInt aCount )
{
    mTotalIndex += aCount;

    return;
}

/***********************************************************************
 * Description : table header 정보 반환 (logging시 필요한 정보)
 ***********************************************************************/
void smcTable::getTableHeaderInfo(void         * aHeader,
                                  scPageID     * aPageID,
                                  scOffset     * aOffset,
                                  smVCDesc     * aColumnsVCDesc,
                                  smVCDesc     * aInfoVCDesc,
                                  UInt         * aHeaderColumnSize,
                                  UInt         * aHeaderFlag,
                                  ULong        * aHeaderMaxRow,
                                  UInt         * aHeaderParallelDegree)
{
    smcTableHeader * sTableHeader;

    sTableHeader = (smcTableHeader*)aHeader;

    *aPageID               = SM_MAKE_PID(sTableHeader->mSelfOID);
    *aOffset               = SM_MAKE_OFFSET(sTableHeader->mSelfOID)
                             + SMP_SLOT_HEADER_SIZE;
    *aColumnsVCDesc        = sTableHeader->mColumns;
    *aInfoVCDesc           = sTableHeader->mInfo;
    *aHeaderColumnSize     = sTableHeader->mColumnSize;
    *aHeaderFlag           = sTableHeader->mFlag;
    *aHeaderMaxRow         = sTableHeader->mMaxRow;
    *aHeaderParallelDegree = sTableHeader->mParallelDegree;

    return;
}

/***********************************************************************
 * Description : table header의 flag 반환 (logging시 필요한 정보)
 ***********************************************************************/
void smcTable::getTableHeaderFlag(void         * aHeader,
                                  scPageID     * aPageID,
                                  scOffset     * aOffset,
                                  UInt         * aHeaderFlag)
{

    smcTableHeader * sTableHeader;

    sTableHeader = (smcTableHeader*)aHeader;

    *aPageID     = SM_MAKE_PID(sTableHeader->mSelfOID);
    *aOffset     = SM_MAKE_OFFSET(sTableHeader->mSelfOID)
                   + SMP_SLOT_HEADER_SIZE;
    *aHeaderFlag = sTableHeader->mFlag;

    return;

}

/***********************************************************************
 * Description : table header의 flag 반환
 ***********************************************************************/
UInt smcTable::getTableHeaderFlagOnly(void         * aHeader)
{
    smcTableHeader * sTableHeader;

    sTableHeader = (smcTableHeader*)aHeader;

    return sTableHeader->mFlag;

}

/***********************************************************************
 * Description : table header의 flag 반환 (logging시 필요한 정보)
 ***********************************************************************/
void smcTable::getTableIsConsistent( void         * aHeader,
                                     scPageID     * aPageID,
                                     scOffset     * aOffset,
                                     idBool       * aFlag )
{
    smcTableHeader * sTableHeader;

    sTableHeader = (smcTableHeader*)aHeader;

    *aPageID     = SM_MAKE_PID(sTableHeader->mSelfOID);
    *aOffset     = SM_MAKE_OFFSET(sTableHeader->mSelfOID)
                   + SMP_SLOT_HEADER_SIZE;
    *aFlag       = sTableHeader->mIsConsistent;

    return;

}


/***********************************************************************
 * Description : create index 구문처리과정중 예외상황으로 인한
 * DDL rollback을 수행한다.
 *
 * - 단, temporary disk index의 경우는 제외된다.
 *   왜냐하면 QP단에서 create index 수행중 abort발생하면
 *   명시적으로 drop temporary table을 수행하기 때문이다.
 ***********************************************************************/
IDE_RC  smcTable::dropIndexByAbortOID( idvSQL *aStatistics,
                                       void*   aTrans,
                                       smOID   aOldIndexOID,  // next
                                       smOID   aNewIndexOID ) // previous
{

    smcTableHeader *  sHeader = NULL;
    void           *  sIndexHeader;
    UInt              sIndexHeaderSize;
    UInt              i;
    SInt              sOffset;
    UInt              sState = 0;
    scGRID*           sIndexSegGRID;

    IDE_ASSERT( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                       aOldIndexOID,
                                       (void**)&sIndexHeader )
                == IDE_SUCCESS );

    IDE_ASSERT( smcTable::getTableHeaderFromOID( smLayerCallback::getTableOIDOfIndexHeader( sIndexHeader ),
                                                 (void**)&sHeader )
                == IDE_SUCCESS );

    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    if(smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        IDE_TEST( smcTable::latchExclusive( sHeader ) != IDE_SUCCESS );
        sState = 1;
    }

    // index header의 seg rID 위치 계산
    if( SMI_TABLE_TYPE_IS_DISK( sHeader ) == ID_TRUE )
    {
        sIndexSegGRID  =  smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );
        if (SC_GRID_IS_NOT_NULL(*sIndexSegGRID))
        {
            IDE_TEST( sdpSegment::freeIndexSeg4Entry(
                                             aStatistics,
                                             SC_MAKE_SPACE(*sIndexSegGRID),
                                             aTrans,
                                             aOldIndexOID,
                                             SDR_MTX_LOGGING)
                      != IDE_SUCCESS );
        }
    }

    // BUG-23218 : 인덱스 탐색 함수 이름 수정
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(sHeader,
                                              sIndexHeader,
                                              sIndexHeaderSize,
                                              &i,
                                              &sOffset)
                 == IDE_SUCCESS );

    IDE_TEST( smcTable::dropIndexByAbortHeader(aStatistics,
                                               aTrans,
                                               sHeader,
                                               i,
                                               sIndexHeader,
                                               aNewIndexOID)
              != IDE_SUCCESS );

    /* Restart시에는 Index의 Temporal부분이 생성되어 있지 않다.
     * 하지만 Disk Table의 경우 Redo후에 Index의 Temporal 부분을
     * 생성한다. */

    if( ( smrRecoveryMgr::isRestart()       == ID_FALSE ) ||
        ( SMI_TABLE_TYPE_IS_DISK( sHeader ) == ID_TRUE  ) )
    {
        IDE_TEST( smcTable::dropIndexPending( sIndexHeader )
                  != IDE_SUCCESS );
    }

    if(sHeader->mDropIndexLst != NULL)
    {
        mDropIdxPagePool.memfree(sHeader->mDropIndexLst);

        sHeader->mDropIndexLst = NULL;
        sHeader->mDropIndex    = 0;
    }

    if(smrRecoveryMgr::isRestart() == ID_FALSE)
    {
        sState = 0;
        IDE_TEST( smcTable::unlatch( sHeader ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();

        IDE_ASSERT( smcTable::unlatch( sHeader ) == IDE_SUCCESS );

        IDE_POP();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : table header의 drop index list를 초기화한다.
 ***********************************************************************/
IDE_RC  smcTable::clearIndexList( smOID aTableOID )
{

    UInt             sState = 0;
    smcTableHeader*  sHeader = NULL;

    IDE_ASSERT( smcTable::getTableHeaderFromOID( aTableOID,
                                                 (void**)&sHeader )
                == IDE_SUCCESS );

    if(sHeader->mDropIndexLst != NULL)
    {
        if(smrRecoveryMgr::isRestart() == ID_FALSE)
        {
            IDE_TEST( smcTable::latchExclusive( sHeader ) != IDE_SUCCESS );
            sState = 1;
        }


        IDE_TEST(mDropIdxPagePool.memfree(sHeader->mDropIndexLst)
                 != IDE_SUCCESS);
        sHeader->mDropIndexLst = NULL;

        if(smrRecoveryMgr::isRestart() == ID_FALSE)
        {
            sState = 0;
            IDE_TEST( smcTable::unlatch( sHeader ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();

        IDE_ASSERT( smcTable::unlatch( sHeader ) == IDE_SUCCESS );

        IDE_POP();
    }

    return IDE_FAILURE;

}

/***********************************************************************
 * Description : create table에 대한 operation NTA의 logical undo시 수행시
 * disk table 인경우에 ager에 table을 제거를 부탁하지 않고 바로 해당 tx가
 * 제거하도록 한다.
 ***********************************************************************/
IDE_RC smcTable::doNTADropDiskTable( idvSQL *aStatistics,
                                     void*   aTrans,
                                     SChar*  aRow )
{

    smcTableHeader*   sHeader;

    IDE_DASSERT( aTrans != NULL );
    IDE_DASSERT( aRow != NULL );

    sHeader = (smcTableHeader *)((smpSlotHeader*)aRow + 1);

    if( SMI_TABLE_TYPE_IS_DISK( sHeader ) == ID_TRUE )
    {
        dropTablePending(aStatistics,
                         aTrans, sHeader, ID_FALSE);
    }

    return IDE_SUCCESS;

}

/* SegmentGRID를 가지는 Lob 컬럼을 반환한다. */
const smiColumn* smcTable::findLOBColumn( void  * aHeader,
                                          scGRID  aSegGRID )
{
    UInt             i;
    const smiColumn *sColumn;
    const smiColumn *sLobColumn;
    UInt             sColumnCnt = getColumnCount( aHeader );

    sLobColumn = NULL;

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn( aHeader, i );

        if( (SMI_IS_LOB_COLUMN( sColumn->flag) == ID_TRUE) &&
            (SC_GRID_IS_EQUAL( sColumn->colSeg, aSegGRID )))
        {
            sLobColumn = sColumn;
            break;
        }
    }

    return sLobColumn;
}

/* PROJ-1671 LOB Segment에 대한 Segment Handle을 생성하고, 초기화한다.*/
IDE_RC smcTable::createLOBSegmentDesc( smcTableHeader * aHeader )
{
    UInt        i;
    smiColumn * sColumn;
    UInt        sColumnCnt;
    idBool      sInvalidTBS = ID_FALSE;

    IDE_ASSERT( aHeader != NULL );
    sColumnCnt = getColumnCount( aHeader );

    /*
     * BUG-22677 DRDB에서create table중 죽을때 recovery가 안되는 경우가 있음.
     */
    if( SMC_IS_VALID_COLUMNINFO(aHeader->mColumns.fstPieceOID) )
    {
        for( i = 0; i < sColumnCnt; i++ )
        {
            sColumn = (smiColumn*)smcTable::getColumn( aHeader, i );

            /*
             * BUG-26848 [SD] Lob column이 있는 tbs가 discard되었을때
             *           refine단계에서 죽는 경우가 있음.
             */
            sInvalidTBS = sctTableSpaceMgr::hasState( sColumn->colSpace,
                                                      SCT_SS_INVALID_DISK_TBS );
            if( sInvalidTBS == ID_TRUE )
            {
                continue;
            }

            if( ( SMI_IS_LOB_COLUMN(sColumn->flag) == ID_TRUE ) &&
                ( SC_GRID_IS_NOT_NULL( sColumn->colSeg )) )
            {
                IDE_TEST( sdpSegment::allocLOBSegDesc(
                              sColumn,
                              aHeader->mSelfOID )
                          != IDE_SUCCESS );
                
                IDE_TEST( sdpSegment::initLobSegDesc( sColumn )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/* PROJ-1671 LOB Segment에 대한 Segment Handle 해제한다. */
IDE_RC smcTable::destroyLOBSegmentDesc( smcTableHeader * aHeader )
{
    UInt        i;
    smiColumn * sColumn;
    UInt        sColumnCnt;
    idBool      sInvalidTBS = ID_FALSE;

    IDE_ASSERT( aHeader != NULL );
    sColumnCnt = getColumnCount( aHeader );

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = (smiColumn*)smcTable::getColumn( aHeader, i );

        sInvalidTBS = sctTableSpaceMgr::hasState( sColumn->colSpace,
                                                  SCT_SS_INVALID_DISK_TBS );
        if( sInvalidTBS == ID_TRUE )
        {
            continue;
        }

        if( ( SMI_IS_LOB_COLUMN(sColumn->flag) == ID_TRUE ) &&
            ( SC_GRID_IS_NOT_NULL( sColumn->colSeg )) )
        {
            IDE_TEST( sdpSegment::freeLOBSegDesc( sColumn )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 디스크 인덱스의 Runtime Header 구성한다.
 *
 *   aStatistics - [IN] 통계정보
 *   aHeader     - [IN] 디스크 테이블 헤더
 *   aMaxSmoNo   - [IN] 디스크 인덱스 런타임 헤더 구축시 설정할 SmoNo
 *
 **********************************************************************/
IDE_RC smcTable::rebuildRuntimeIndexHeaders( idvSQL         * aStatistics,
                                             smcTableHeader * aHeader,
                                             ULong            aMaxSmoNo )
{
    UInt                 i;
    UInt                 sIndexCnt;
    idBool               sIsExist;
    void               * sIndexHeader;
    scGRID             * sIndexGRID;
    smiSegAttr         * sSegAttrPtr;
    smiSegStorageAttr  * sSegStoAttrPtr;
    smrRTOI              sRTOI;

    IDE_ASSERT( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_TRUE );

    sIndexCnt = smcTable::getIndexCount(aHeader);

    for ( i = 0; i < sIndexCnt; i++ )
    {
        sIndexHeader = (void*)smcTable::getTableIndex(aHeader,i);
        sIndexGRID   = smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );

        if ( SC_GRID_IS_NULL(*sIndexGRID) )
        {
             continue;
        }

        // fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline 수행시
        // 올바르게 Index Runtime Header 해제하지 않음
        // Refine DRDB 과정과 Online 과정에서 호출될 수 있는데,
        // INVALID_DISK_TBS( DROPPED/DISCARDED ) 인경우에는
        // Skip 하도록 한다.
        sIsExist = sddDiskMgr::isValidPageID( aStatistics, /* idvSQL* */
                                              SC_MAKE_SPACE(*sIndexGRID),
                                              SC_MAKE_PID(*sIndexGRID));

        if ( sIsExist == ID_FALSE )
        {
            /* 유효하지 않는 세그먼트 페이지를 가진 인덱스의 런타임 헤더는
             * NULL 초기화한다 */
            smLayerCallback::setInitIndexPtr( sIndexHeader );
            continue;
        }

        if ( ( smLayerCallback::getFlagOfIndexHeader( sIndexHeader )
               & SMI_INDEX_USE_MASK ) == SMI_INDEX_USE_ENABLE )
        {
            sSegStoAttrPtr = smLayerCallback::getIndexSegStoAttrPtr( sIndexHeader );
            sSegAttrPtr    = smLayerCallback::getIndexSegAttrPtr( sIndexHeader );

            /* PROJ-2162 RestartRiskReduction
             * Property에 의해 강제로 예외함 */
            smrRecoveryMgr::initRTOI( &sRTOI );
            sRTOI.mCause    = SMR_RTOI_CAUSE_PROPERTY;
            sRTOI.mType     = SMR_RTOI_TYPE_INDEX;
            sRTOI.mState    = SMR_RTOI_STATE_CHECKED;
            sRTOI.mTableOID = smLayerCallback::getTableOIDOfIndexHeader( sIndexHeader );
            sRTOI.mIndexID  = smLayerCallback::getIndexIDOfIndexHeader( sIndexHeader );
            if( smrRecoveryMgr::isIgnoreObjectByProperty( &sRTOI ) == ID_TRUE )
            {
                IDE_TEST( smrRecoveryMgr::startupFailure( &sRTOI,
                                                          ID_FALSE )/*isRedo*/
                          != IDE_SUCCESS);
            }
            else
            {
                if ( smLayerCallback::initIndex( aStatistics,
                                                 aHeader,
                                                 sIndexHeader,
                                                 sSegAttrPtr,
                                                 sSegStoAttrPtr,
                                                 NULL,
                                                 aMaxSmoNo )
                     != IDE_SUCCESS )
                {
                    /* 긴급복구모드가 아니면 서버 종료시킴. */
                    IDE_TEST( smuProperty::getEmergencyStartupPolicy()
                                == SMR_RECOVERY_NORMAL );
                    IDE_TEST( smrRecoveryMgr::refineFailureWithIndex(
                                                        sRTOI.mTableOID,
                                                        sRTOI.mIndexID )
                            != IDE_SUCCESS);
                }
                else
                {
                    smLayerCallback::setIndexCreated( sIndexHeader, ID_TRUE );
                }
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 *
 * Description : 디스크 테이블의 모든 RUNTIME 정보를 구축한다.
 *
 *   RUNTIME 정보에는 다음과 같은 종류가 있다.
 *   - 테이블의 Lock Node와 Mutex
 *   - 테이블의 인덱스 RUNTIME 헤더
 *   - LOB 컬럼의 세그먼트 핸들
 *
 *   aActionArg 에 따라 선별적으로 초기화되기도 한다.
 *
 *   aStatistics - [IN] 통계정보
 *   aHeader     - [IN] 디스크 테이블 헤더
 *   aActionArg  - [IN] Action함수에 보낼 Argument
 *
 **********************************************************************/
IDE_RC smcTable::initRuntimeInfos( idvSQL         * aStatistics,
                                   smcTableHeader * aHeader,
                                   void           * aActionArg )
{
    idBool sInitLockAndRuntimeItem;

    IDE_ASSERT( aActionArg != NULL );

    sInitLockAndRuntimeItem = *(idBool*)aActionArg;

    if ( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_TRUE )
    {
        /* Disk table의 경우 undoAll에서 tableheader를
         * 접근하기 때문에 초기화를 여기서 해주어야 한다.
         * MemoryDB의 refine시에는 memory table에 대해서만 처리하도록 한다. */

        if ( sInitLockAndRuntimeItem == ID_TRUE )
        {
            IDE_TEST( initLockAndRuntimeItem( aHeader )
                      != IDE_SUCCESS );
        }

        IDE_TEST( rebuildRuntimeIndexHeaders( aStatistics,
                                              aHeader,
                                              0 )
                  != IDE_SUCCESS );

        /* PROJ-1671 LOB Segment에 대한 Segment Handle을 생성하고, 초기화한다.*/
        IDE_TEST( createLOBSegmentDesc( aHeader ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcTable::initRowTemplate( idvSQL         * /*aStatistics*/,
                                  smcTableHeader * aHeader,
                                  void           * /*aActionArg*/ )
{
    const smiColumn      * sColumn;
    UInt                   sColumnCount;
    UShort                 sColumnSeq;
    smcRowTemplate       * sRowTemplate; 
    smcColTemplate       * sColumnTemplate;
    SShort                 sColStartOffset = 0;
    UInt                   sColumnLength;
    UShort               * sVariableColumnOffsetArr;
    UShort                 sVariableColumnCount = 0;
    UChar                * sAllocMemPtr;
    UInt                   sAllocState = 0;
    idBool                 sIsEnableRowTemplate;

    IDE_DASSERT( aHeader != NULL );

    aHeader->mRowTemplate = NULL;

    sIsEnableRowTemplate = smuProperty::getEnableRowTemplate();

    IDE_TEST_CONT( SMI_TABLE_TYPE_IS_DISK( aHeader ) != ID_TRUE,
                    skip_init_row_template );

    IDE_TEST_CONT( sctTableSpaceMgr::hasState( aHeader->mSpaceID,             
                                                SCT_SS_INVALID_DISK_TBS ) 
                    == ID_TRUE, skip_init_row_template );

    IDE_TEST_CONT( aHeader->mColumnCount == 0, skip_init_row_template );

    sColumnCount = aHeader->mColumnCount;

    /* smcTable_initRowTemplate_calloc_AllocMemPtr.tc */
    IDU_FIT_POINT("smcTable::initRowTemplate::calloc::AllocMemPtr");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMC,
                                 1,
                                 ID_SIZEOF(smcRowTemplate) 
                                 + (ID_SIZEOF(smcColTemplate) * sColumnCount)
                                 + ((ID_SIZEOF(UShort) * sColumnCount)),
                                 (void**)&sAllocMemPtr )
                != IDE_SUCCESS );
    sAllocState = 1;

    sRowTemplate = (smcRowTemplate*)sAllocMemPtr;

    sRowTemplate->mColTemplate = 
                    (smcColTemplate*)(sAllocMemPtr + ID_SIZEOF(smcRowTemplate));

    sVariableColumnOffsetArr = 
                    (UShort*)(sAllocMemPtr + ID_SIZEOF(smcRowTemplate)
                                     + (ID_SIZEOF(smcColTemplate) * sColumnCount));

    sRowTemplate->mTotalColCount = sColumnCount;

    for ( sColumnSeq = 0; sColumnSeq < sColumnCount; sColumnSeq++ )
    {
        sColumnTemplate = &sRowTemplate->mColTemplate[sColumnSeq];
        sColumn         = smcTable::getColumn( aHeader, sColumnSeq );

        /* BUG-39679 Row Template 정보를 사용할것인지 아닌지 판단 한다.
           사용하지 않을경우 sColumnLength을 무조건 ID_UINT_MAX값으로한다. */
        if( sIsEnableRowTemplate == ID_TRUE )
        {
            IDE_ASSERT( gSmiGlobalCallBackList.getColumnStoreLen( sColumn,
                                                                  &sColumnLength )
                        == IDE_SUCCESS );
        }
        else
        {
            sColumnLength = ID_UINT_MAX;
        }

        /* sColumnLength가 ID_UINT_MAX인 경우는 variable column 이거나 
         * not null column이 아닌 경우이다. */
        if ( sColumnLength == ID_UINT_MAX )
        {
            sColumnTemplate->mColStartOffset  = sColStartOffset;
            sColumnTemplate->mStoredSize      = SMC_UNDEFINED_COLUMN_LEN;
            sColumnTemplate->mColLenStoreSize = 0;
            sColumnTemplate->mVariableColIdx  = sVariableColumnCount;
            /* sColStartOffset은 non variable column일때만 증가 시킨다.
             * (sColumnLength == ID_UINT_MAX가 아닐때 )
             * 즉, 각 column의 mColStartOffset은 해당 column의 앞에 존재하는
             * non variable column들 길이의 합이라고 볼 수 있다.
             * variable column의 길이는 fetch를 수행하는 함수인
             * sdc::doFetch함수에서 길이를 구해 사용된다.*/
            sColStartOffset                  += 0;

            /* variable column앞에 존재하는 non variable column들의 길이를
             * 바로 알기 위해 현재 variable column의 sColStartOffset을
             * 저장한다. (mVariableColOffset 정보) */
            sVariableColumnOffsetArr[sVariableColumnCount] = sColStartOffset;
            sVariableColumnCount++;
        }
        else
        {
            IDE_ASSERT( sColumnLength < ID_UINT_MAX );

            sColumnTemplate->mColStartOffset  = sColStartOffset;
            sColumnTemplate->mStoredSize      = sColumnLength;
            sColumnTemplate->mColLenStoreSize = SDC_GET_COLLEN_STORE_SIZE(sColumnLength);
            sColumnTemplate->mVariableColIdx  = sVariableColumnCount;
            sColStartOffset                  += (sColumnLength 
                                                 + sColumnTemplate->mColLenStoreSize);
        }
    }

    sRowTemplate->mVariableColOffset = sVariableColumnOffsetArr;
    sRowTemplate->mVariableColCount  = sVariableColumnCount;
    aHeader->mRowTemplate            = sRowTemplate;

    IDE_EXCEPTION_CONT(skip_init_row_template);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sAllocState == 1 )
    {
        IDE_ASSERT( iduMemMgr::free( sAllocMemPtr ) == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
}

IDE_RC smcTable::destroyRowTemplate( smcTableHeader * aHeader )
{
    if ( aHeader->mRowTemplate != NULL )
    {
        IDE_TEST( iduMemMgr::free( aHeader->mRowTemplate ) != IDE_SUCCESS );

        aHeader->mRowTemplate = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smcTable::dumpRowTemplate( smcRowTemplate * aRowTemplate )
{
    smcColTemplate * sColTemplate;
    UShort           sVariableColOffset;
    UInt             sColumnCount;
    UInt             sColumnSeq;

    ideLogEntry      sLog( IDE_ERR_0 );

    IDE_DASSERT( aRowTemplate != NULL );

    sColumnCount = aRowTemplate->mTotalColCount;

    sLog.append( "==================================\n"
                 "Dump Column Template\n");

    for ( sColumnSeq = 0; sColumnSeq < sColumnCount; sColumnSeq++ )
    {
        sColTemplate = &aRowTemplate->mColTemplate[sColumnSeq];

        sLog.appendFormat( "==================================\n"
                           "Column Seq      : %"ID_UINT32_FMT"\n"
                           "StoredSize      : %"ID_UINT32_FMT"\n"
                           "ColLenStoreSize : %"ID_UINT32_FMT"\n"
                           "VariableColIdx  : %"ID_UINT32_FMT"\n"
                           "ColStartOffset  : %"ID_UINT32_FMT"\n",
                           sColumnSeq,
                           sColTemplate->mStoredSize,
                           sColTemplate->mColLenStoreSize,
                           sColTemplate->mVariableColIdx,
                           sColTemplate->mColStartOffset );
    }

    sLog.append( "==================================\n" );

    sLog.append( "==================================\n"
                 "Dump Variable Column Offset\n");

    for ( sColumnSeq = 0; sColumnSeq < sColumnCount; sColumnSeq++ )
    {
        sVariableColOffset = aRowTemplate->mVariableColOffset[sColumnSeq];

        sLog.appendFormat( "==================================\n"
                           "Seq            : %"ID_UINT32_FMT"\n"
                           "ColStartOffset : %"ID_UINT32_FMT"\n",
                           sColumnSeq,
                           sVariableColOffset );
    }

    sLog.append( "==================================\n" );

    sLog.appendFormat( "==================================\n"
                       "TotalColCount     : %"ID_UINT32_FMT"\n"
                       "VariableColCount  : %"ID_UINT32_FMT"\n"
                       "==================================\n", 
                       aRowTemplate->mTotalColCount,
                       aRowTemplate->mVariableColCount );

    sLog.write();
}

/***********************************************************************
 *
 * Description : 디스크 테이블의 모든 디스크 인덱스의 Integrity 체크를 수행한다.
 *
 *   aStatistics - [IN] 통계정보
 *   aHeader     - [IN] 디스크 테이블 헤더
 *   aActionArg  - [IN] Action함수에 보낼 Argument
 *
 **********************************************************************/
IDE_RC smcTable::verifyIndexIntegrity( idvSQL         * aStatistics,
                                       smcTableHeader * aHeader,
                                       void           * aActionArg )
{
    UInt               i;
    UInt               sIndexCnt;
    void*              sIndexHeader;
    scGRID*            sIndexGRID;
    SChar              sStrBuffer[128];

    IDE_ASSERT( aActionArg == NULL );

    IDE_TEST_CONT( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    skip_verify );

    sIndexCnt = smcTable::getIndexCount(aHeader);

    for( i = 0; i < sIndexCnt; i++ )
    {
        sIndexHeader = (void*)smcTable::getTableIndex(aHeader,i);
        sIndexGRID   = smLayerCallback::getIndexSegGRIDPtr( sIndexHeader );

        if (SC_GRID_IS_NULL(*sIndexGRID) )
        {
            continue;
        }
        else
        {
            /* 디스크 인덱스 헤더 초기화시에 이미 인덱스 세그먼트 페이지에 대한
             * 디스크상에서의 Verify가 완료되었기 때문에 믿고 진행한다. */
        }

        if ( smLayerCallback::isIndexToVerifyIntegrity( (const smnIndexHeader*)sIndexHeader )
             == ID_FALSE )
        {
            continue;
        }

        if ( ( smLayerCallback::getFlagOfIndexHeader( sIndexHeader )
               & SMI_INDEX_USE_MASK ) == SMI_INDEX_USE_ENABLE )
        {
            idlOS::snprintf(sStrBuffer,
                            128,
                            "       [TABLE OID: %"ID_vULONG_FMT", "
                            "INDEX ID: %"ID_UINT32_FMT" "
                            "NAME : %s] ",
                            ((smnIndexHeader*)sIndexHeader)->mTableOID,
                            ((smnIndexHeader*)sIndexHeader)->mId,
                            ((smnIndexHeader*)sIndexHeader)->mName );

            IDE_CALLBACK_SEND_SYM( sStrBuffer );

            if ( smLayerCallback::verifyIndexIntegrity( aStatistics,
                                                        ((smnIndexHeader*)sIndexHeader)->mHeader )
                 != IDE_SUCCESS )
            {
                IDE_CALLBACK_SEND_SYM( " [FAIL]\n" );
            }
            else
            {
                IDE_CALLBACK_SEND_SYM( " [PASS]\n" );
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_verify );

    return IDE_SUCCESS;
}


IDE_RC smcTable::setNullRow(void*             aTrans,
                            smcTableHeader*   aTable,
                            UInt              aTableType,
                            void*             aData)
{
    smpSlotHeader*   sNullSlotHeaderPtr;
    smSCN            sNullRowSCN;
    smOID            sNullRowOID = SM_NULL_OID;

    smcTableHeader*  sTableHeader = (smcTableHeader*)aTable;

    IDE_DASSERT( aTableType != SMI_TABLE_DISK );

    sNullRowOID = *(( smOID* )aData);

    if( aTableType == SMI_TABLE_MEMORY )
    {
        IDE_ASSERT( smmManager::getOIDPtr( 
                        sTableHeader->mSpaceID, 
                        sNullRowOID,
                        (void**)&sNullSlotHeaderPtr )
                    == IDE_SUCCESS );

        SM_SET_SCN_NULL_ROW( &sNullRowSCN );

        IDE_ASSERT( SM_SCN_IS_EQ(&sNullRowSCN,
                                 &(sNullSlotHeaderPtr->mCreateSCN)));
    }

    IDE_TEST( smrUpdate::setNullRow(NULL, /* idvSQL* */
                                    aTrans,
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                    SM_MAKE_PID(sTableHeader->mSelfOID),
                                    SM_MAKE_OFFSET(sTableHeader->mSelfOID)
                                    + SMP_SLOT_HEADER_SIZE,
                                    sNullRowOID)
              != IDE_SUCCESS);


    sTableHeader->mNullOID = sNullRowOID;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                            SM_MAKE_PID(sTableHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table의 Lob Column리스트를 구성한다.
 *
 * aArrLobColumn - [OUT] Lob Column Pointer Array를 넘겨준다.
 ***********************************************************************/
IDE_RC smcTable::makeLobColumnList(
    void                     * aTableHeader,
    smiColumn             ***  aArrLobColumn,
    UInt                     * aLobColumnCnt)
{
    UInt              sLobColumnCnt;
    smiColumn       **sLobColumn;
    smiColumn       **sFence;
    smiColumn        *sColumn;
    UInt              i;
    UInt              sColumnType;

    sLobColumnCnt = smcTable::getLobColumnCount(aTableHeader);

    *aLobColumnCnt = sLobColumnCnt;
    *aArrLobColumn = NULL;

    if( sLobColumnCnt != 0 )
    {
        /* smcTable_makeLobColumnList_malloc_ArrLobColumn.tc */
        IDU_FIT_POINT("smcTable::makeLobColumnList::malloc::ArrLobColumn");
        IDE_TEST(iduMemMgr::malloc(
                     IDU_MEM_SM_SMC,
                     (ULong)sLobColumnCnt *
                     ID_SIZEOF(smiColumn*),
                     (void**)aArrLobColumn,
                     IDU_MEM_FORCE)
                 != IDE_SUCCESS);

        sLobColumn =  *aArrLobColumn;
        sFence     =  *aArrLobColumn + sLobColumnCnt;

        for( i = 0; sLobColumn < sFence; i++)
        {
            sColumn = (smiColumn*)smcTable::getColumn( aTableHeader, i );
            sColumnType =
                sColumn->flag & SMI_COLUMN_TYPE_MASK;

            if( sColumnType == SMI_COLUMN_TYPE_LOB )
            {
                *sLobColumn = sColumn;
                sLobColumn++;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( *aArrLobColumn != NULL )
    {
        iduMemMgr::free(*aArrLobColumn);
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Column List에 할당된 메모리를 반납한다.
 *
 * aColumnList - [IN] Column List
 ***********************************************************************/
IDE_RC smcTable::destLobColumnList(void *aColumnList)
{
    if( aColumnList != NULL )
    {
        IDE_TEST( iduMemMgr::free(aColumnList)
                  != IDE_SUCCESS );
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;

}

/***********************************************************************
 * Description : QP로부터 null row를 얻어온다.
 *
 * Implementation :
 *    먼저 smiColumnList를 구성한다. aColListBuffer는 smiColumnList
 *    array이다. 이 공간에 next와 column을 세팅한다.
 *    마지막 column list의 next는 NULL로 세팅해야 한다.
 *    구성된 smiColumnList와 null row가 만들어질 버퍼를 인자로
 *    callback 함수를 호출한다.
 *
 *    - aTableHeader: 대상 테이블의 테이블 해더
 *    - aColListBuffer: 대상 테이블의 컬럼 리스트가 구성될 버퍼 공간
 *    - aNullRow: smiValue들이 저장될 공간
 *    - aValueBuffer: fixed null value들이 저장될 공간
 *                    aNullRow[i].value가 가리키는 공간이된다.
 ***********************************************************************/
IDE_RC smcTable::makeNullRowByCallback(smcTableHeader *aTableHeader,
                                       smiColumnList  *aColListBuffer,
                                       smiValue       *aNullRow,
                                       SChar          *aValueBuffer)
{
    smiColumnList *sCurList;
    UInt           i;

    /* 컬럼의 개수가 최소한 1개이상이어야 한다. */
    IDE_ASSERT(aTableHeader->mColumnCount > 0);

    /* STEP-1 : smiColumnList를 구성한다. */
    sCurList = aColListBuffer;

    for (i = 0; i < aTableHeader->mColumnCount; i++)
    {
        sCurList->next = sCurList + 1;
        sCurList->column = smcTable::getColumn( aTableHeader, i );
        sCurList++;
    }
    /* 컬럼의 개수가 최소한 1개 이상이므로 for loop를 한번도 안도는 일은 없다. */
    /* 마지막 smiColumnList의 next를 NULL로 세팅해야 한다. */
    sCurList--;
    sCurList->next = NULL;

    /* STEP-2 : QP로부터 null row를 얻어온다. */
    IDE_TEST(gSmiGlobalCallBackList.makeNullRow(NULL,
                                                aColListBuffer,
                                                aNullRow,
                                                aValueBuffer)
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/**********************************************************************
 * BUG-30378 비정상적으로 Drop되었지만 refine돼지 않는
 * 테이블이 존재합니다.
 * 범용적으로 사용할 수 있는 Dump함수로 변경
 *
 * BUG-31206    improve usability of DUMPCI and DUMPDDF
 * Util에서도 쓸 수 있는 버퍼용 함수
 *
 * Boot log에 Internal Server Error메세지를 찍는다.
 *
 * [IN]     aTableHeader     - 에러가 발생한 Table의 Header
 * [IN]     aDumpBinary      - aTableHeader의 값을 전부 a로 찍을지 여부
 * [IN]     aDisplayTable    - 값을 표 형식으로 출력할지 여부
 * [OUT]    aOutBuf          - 대상 버퍼
 * [OUT]    aOutSize         - 대상 버퍼의 크기
 *
 **********************************************************************/

void  smcTable::dumpTableHeaderByBuffer( smcTableHeader * aTable,
                                         idBool           aDumpBinary,
                                         idBool           aDisplayTable,
                                         SChar          * aOutBuf,
                                         UInt             aOutSize )
{
    /* smcDef.h:71 - smcTableType */
    SChar            sType[][ 9] ={ "NORMAL",
                                    "TEMP",
                                    "CATALOG",
                                    "SEQ",
                                    "OBJECT" };
    /* smiDef.h:743- smiObjectType */
    SChar            sObjectType[][ 9] ={ "NONE",
                                          "PROC",
                                          "PROC_GRP",
                                          "VIEW",
                                          "DBLINK" };
    SChar            sTableType[][ 9] ={ "META",
                                         "TEMP",
                                         "MEMORY",
                                         "DISK",
                                         "FIXED",
                                         "VOLATILE",
                                         "REMOTE" };
    UInt             sStrLen;
    UInt             sTableTypeID;
    UInt             i;

    IDE_ASSERT( aTable != NULL );

    sTableTypeID = SMI_TABLE_TYPE_TO_ID( SMI_GET_TABLE_TYPE( aTable ) );

    if( aDisplayTable == ID_FALSE )
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "Dump Table Header\n"
                             "Type            : %s(%"ID_UINT32_FMT")\n"
                             "Flag            : %"ID_UINT32_FMT"\n"
                             "TableType       : %s(%"ID_UINT32_FMT")\n"
                             "ObjType         : %s(%"ID_UINT32_FMT")\n"
                             "SelfOID         : %"ID_vULONG_FMT"\n"
                             "NullOID         : %"ID_vULONG_FMT"\n"
                             "ColumnSize      : %"ID_UINT32_FMT"\n"
                             "ColumnCount     : %"ID_UINT32_FMT"\n"
                             "SpaceID         : %"ID_UINT32_FMT"\n"
                             "MaxRow          : %"ID_UINT64_FMT"\n"
                             "TableCreateSCN  : %"ID_UINT64_FMT"\n",
                             sType[aTable->mType],
                             aTable->mType,
                             aTable->mFlag,
                             sTableType[ sTableTypeID ],
                             sTableTypeID,
                             sObjectType[aTable->mObjType],
                             aTable->mObjType,
                             aTable->mSelfOID,
                             aTable->mNullOID,
                             aTable->mColumnSize,
                             aTable->mColumnCount,
                             aTable->mSpaceID,
                             aTable->mMaxRow,
                             SM_SCN_TO_LONG( aTable->mTableCreateSCN ) );

        switch( SMI_GET_TABLE_TYPE( aTable ) )
        {
        case SMI_TABLE_DISK:
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "SlotSize        : %"ID_UINT32_FMT"\n",
                                 aTable->mFixed.mDRDB.mSlotSize );
            break;
        case SMI_TABLE_META:
        case SMI_TABLE_MEMORY:
            idlVA::appendFormat(
                            aOutBuf,
                            aOutSize,
                            "FixPage         : SlotSize %8"ID_UINT32_FMT"  "
                            "HeadPageID %8"ID_UINT32_FMT"\n",
                            aTable->mFixed.mMRDB.mSlotSize,
                            smpAllocPageList::getFirstAllocPageID(
                                &aTable->mFixedAllocList.mMRDB[0]) );
            for( i = 0 ; i < SM_VAR_PAGE_LIST_COUNT ; i ++ )
            {
                idlVA::appendFormat(
                                aOutBuf,
                                aOutSize,
                                "VarPage [%2"ID_UINT32_FMT"]    : "
                                "SlotSize %8"ID_UINT32_FMT"  "
                                "HeadPageID %8"ID_UINT32_FMT"\n",
                                i,
                                aTable->mVar.mMRDB[i].mSlotSize,
                                smpAllocPageList::getFirstAllocPageID(
                                    &aTable->mVarAllocList.mMRDB[i]) );
            }
            break;
        case SMI_TABLE_VOLATILE:
            idlVA::appendFormat(
                            aOutBuf,
                            aOutSize,
                            "FixPage         : SlotSize %8"ID_UINT32_FMT"  "
                            "HeadPageID %8"ID_UINT32_FMT"\n",
                            aTable->mFixed.mVRDB.mSlotSize,
                            svpAllocPageList::getFirstAllocPageID(
                                &aTable->mFixedAllocList.mVRDB[0]) );
            for( i = 0 ; i < SM_VAR_PAGE_LIST_COUNT ; i ++ )
            {
                idlVA::appendFormat(
                                aOutBuf,
                                aOutSize,
                                "VarPage [%2"ID_UINT32_FMT"]    : "
                                "SlotSize %8"ID_UINT32_FMT"  "
                                "HeadPageID %8"ID_UINT32_FMT"\n",
                                i,
                                aTable->mVar.mVRDB[i].mSlotSize,
                                svpAllocPageList::getFirstAllocPageID(
                                    &aTable->mVarAllocList.mVRDB[i]) );
            }
            break;
        default:
            break;
        }
    }
    else
    {
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "%9s(%2"ID_UINT32_FMT") "
                             "%8"ID_UINT32_FMT
                             "%9s(%2"ID_UINT32_FMT") "
                             "%9s(%2"ID_UINT32_FMT") "
                             "%-12"ID_vULONG_FMT
                             "%-12"ID_vULONG_FMT
                             "%-8"ID_UINT32_FMT
                             "%-8"ID_UINT32_FMT
                             "%-4"ID_UINT32_FMT
                             "%-8"ID_UINT64_FMT,
                             sType[aTable->mType],
                             aTable->mType,
                             aTable->mFlag,
                             sTableType[ sTableTypeID ],
                             sTableTypeID,
                             sObjectType[aTable->mObjType],
                             aTable->mObjType,
                             aTable->mSelfOID,
                             aTable->mNullOID,
                             aTable->mColumnSize,
                             aTable->mColumnCount,
                             aTable->mSpaceID,
                             aTable->mMaxRow );
    }

    sStrLen = idlOS::strnlen( aOutBuf, aOutSize );

    if( aDumpBinary == ID_TRUE )
    {
        ideLog::ideMemToHexStr( (UChar*) aTable,
                                ID_SIZEOF( smcTableHeader ),
                                IDE_DUMP_FORMAT_NORMAL,
                                aOutBuf + sStrLen,
                                aOutSize - sStrLen );
    }
}


void  smcTable::dumpTableHeader( smcTableHeader * aTable )
{
    SChar          * sTempBuffer;

    IDE_ASSERT( aTable != NULL );

    if( iduMemMgr::calloc( IDU_MEM_SM_SMC,
                           1,
                           ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                           (void**)&sTempBuffer )
        == IDE_SUCCESS )
    {
        sTempBuffer[0] = '\0';
        dumpTableHeaderByBuffer( aTable,
                                 ID_TRUE,  // dump binary
                                 ID_FALSE, // display table
                                 sTempBuffer,
                                 IDE_DUMP_DEST_LIMIT );

        ideLog::log( IDE_SERVER_0,
                     "%s\n",
                     sTempBuffer );

        (void) iduMemMgr::free( sTempBuffer );
    }
}


/*
    Table Header의 Index Latch를 할당하고 초기화

    [IN] aTableHeader - IndexLatch가 초기화될 Table의 Header
 */
IDE_RC smcTable::allocAndInitIndexLatch(smcTableHeader * aTableHeader )
{
    SChar sBuffer[IDU_MUTEX_NAME_LEN];

    IDE_ASSERT( aTableHeader != NULL );

    UInt sState = 0;

    if ( aTableHeader->mIndexLatch != NULL )
    {
        // ASSERT를 거는 것이 맞지만,
        // 동작에는 문제가 없으므로 ASSERT대신 부트로그에 찍도록 처리
        ideLog::log( IDE_SERVER_0,
                     "InternalError [%s:%u]\n"
                     "IndexLatch Of TableHeader is initialized twice.\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__ );
        ideLog::logCallStack( IDE_SERVER_0 );

        dumpTableHeader( aTableHeader );

        // 디버그 모드일때만 서버를 죽인다.
        IDE_DASSERT(0);
    }

    /* smcTable_allocAndInitIndexLatch_malloc_IndexLatch.tc */
    IDU_FIT_POINT("smcTable::allocAndInitIndexLatch::malloc::IndexLatch");
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMC,
                                 ID_SIZEOF(iduLatch),
                                 (void**) &(aTableHeader->mIndexLatch) )
              != IDE_SUCCESS );
    sState =1;

    idlOS::snprintf( sBuffer,
                     ID_SIZEOF(sBuffer),
                     "TABLE_HEADER_LATCH_%"ID_vULONG_FMT,
                     aTableHeader->mSelfOID );

    IDE_TEST( aTableHeader->mIndexLatch->initialize( sBuffer )
              != IDE_SUCCESS );
    sState =2;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    {
        switch( sState )
        {
            case 2:
                IDE_ASSERT( aTableHeader->mIndexLatch->destroy( )
                            == IDE_SUCCESS );
            case 1:
                IDE_ASSERT( iduMemMgr::free( aTableHeader->mIndexLatch )
                            == IDE_SUCCESS );
                break;

            default:
                break;
        }

        aTableHeader->mIndexLatch = NULL;
    }
    IDE_POP();

    return IDE_FAILURE;
}


/*
    Table Header의 Index Latch를 파괴하고 해제

    [IN] aTableHeader - IndexLatch가 파괴될 Table의 Header
 */
IDE_RC smcTable::finiAndFreeIndexLatch(smcTableHeader * aTableHeader )
{
    IDE_ASSERT( aTableHeader != NULL );

    if ( aTableHeader->mIndexLatch == NULL )
    {
        /*
         * nothing to do
         *
         * 1. Create Table의 smrRecoveryMgr::undoTrans과정에서 호출될수 있고,
         * 2. 다시 한번 smxOIDList::processDropTblPending에서 호출될수 있다.
         *
         */
    }
    else
    {
        IDE_TEST( aTableHeader->mIndexLatch->destroy( )
                  != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free( aTableHeader->mIndexLatch )
                  != IDE_SUCCESS );

        aTableHeader->mIndexLatch = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Table Header의 Index Latch에 Exclusive Latch를 잡는다.

    [IN] aTableHeader - Exclusive Latch를 잡을 Table의 Header
*/
IDE_RC smcTable::latchExclusive(smcTableHeader * aTableHeader )
{
    IDE_ASSERT( aTableHeader != NULL );

    if ( aTableHeader->mIndexLatch == NULL )
    {
        // ASSERT를 거는 것이 맞지만,
        // 동작에는 문제가 없으므로 ASSERT대신 부트로그에 찍도록 처리
        //
        // (Index DDL과 동시 수행시에만 문제가 된다.)
        ideLog::log( IDE_SERVER_0,
                     "InternalError [%s:%u]\n"
                     "IndexLatch is not initialized yet.(trial to execute latchExclusive).\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__ );

        ideLog::logCallStack( IDE_SERVER_0 );

        dumpTableHeader( aTableHeader );

        // DEBUG Mode일때는 ASSERT로 죽인다.
        IDE_DASSERT(0);
    }
    else
    {
        IDE_TEST( aTableHeader->mIndexLatch->lockWrite( NULL, /* idvSQL* */
                                       NULL /* idvWeArgs* */ )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Table Header의 Index Latch에 Shared Latch를 잡는다.

    [IN] aTableHeader - Shared Latch를 잡을 Table의 Header
 */
IDE_RC smcTable::latchShared(smcTableHeader * aTableHeader )
{
    IDE_ASSERT( aTableHeader != NULL );

    if ( aTableHeader->mIndexLatch == NULL )
    {
        // ASSERT를 거는 것이 맞지만,
        // 동작에는 문제가 없으므로 ASSERT대신 부트로그에 찍도록 처리
        //
        // (Index DDL과 동시 수행시에만 문제가 된다.)
        ideLog::log( IDE_SERVER_0,
                     "InternalError [%s:%u]\n"
                     "IndexLatch is not initialized yet.(trial to execute latchShared).\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__ );

        ideLog::logCallStack( IDE_SERVER_0 );

        dumpTableHeader( aTableHeader );

        // DEBUG Mode일때는 ASSERT로 죽인다.
        IDE_DASSERT(0);
    }
    else
    {
        IDE_TEST( aTableHeader->mIndexLatch->lockRead( NULL, /* idvSQL* */
                                      NULL /* idvWeArgs* */ )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Table Header의 Index Latch를 풀어준다.

    [IN] aTableHeader - Index Latch를 풀어줄 Table의 Header
 */
IDE_RC smcTable::unlatch(smcTableHeader * aTableHeader )
{
    IDE_ASSERT( aTableHeader != NULL );

    if ( aTableHeader->mIndexLatch == NULL )
    {
        // ASSERT를 거는 것이 맞지만,
        // 동작에는 문제가 없으므로 ASSERT대신 부트로그에 찍도록 처리
        //
        // (Index DDL과 동시 수행시에만 문제가 된다.)
        ideLog::log( IDE_SERVER_0,
                     "InternalError [%s:%u]\n"
                     "IndexLatch is not initialized yet.(trial to execute unlatch).\n",
                     (SChar *)idlVA::basename(__FILE__),
                     __LINE__ );

        ideLog::logCallStack( IDE_SERVER_0 );

        dumpTableHeader( aTableHeader );

        // DEBUG Mode일때는 ASSERT로 죽인다.
        IDE_DASSERT(0);
    }
    else
    {
        IDE_TEST( aTableHeader->mIndexLatch->unlock(  )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : DropIndexByAbort함수에 의해서 호출되는데 새로운 Index Header
 *               Array에 기존의 Index Header Array에 Create 가 취소된 Index를
 *               제외한 Index Header들을 복사해서 옮겨주는 함수이다.
 *
 * aTrans             - [IN] Transaction Pointer
 * aDropIndexHeader   - [IN] Drop될 Index Header
 * aIndexHeaderArr    - [IN] 기존의 Index Header Array
 * aIndexHeaderArrLen - [IN] Index Header Array의 길이
 * aOIDIndexHeaderArr - [IN] 새로운 Index Header Array를 가지게 될 Var Row의
 *                           OID
 *
 * Related Issue :
 *   - BUG-17955: Add/Drop Column 수행후 smiStatement End시,
 *                Unable to invoke mutex_lock()로 서버 비정상 종료
 *   - BUG-30612: index header self oid가 잘못 기록될 수가 있습니다.
 ***********************************************************************/
IDE_RC smcTable::removeIdxHdrAndCopyIndexHdrArr(
                                            void*     aTrans,
                                            void*     aDropIndexHeader,
                                            void*     aIndexHeaderArr,
                                            UInt      aIndexHeaderArrLen,
                                            smOID     aOIDIndexHeaderArr )
{
    scGRID            sGRID;
    SChar            *sDestIndexRowPtr;
    SChar            *sFstIndexHeader;
    smrUptLogImgInfo  sAftImgInfo;
    static UInt       sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();
    UInt              sIdxCntInIdxArr = aIndexHeaderArrLen / sIndexHeaderSize;
    UInt              sCopySize ;

    IDE_ASSERT( sIdxCntInIdxArr - 1 != 0 );

    /* BUG-17955: Add/Drop Column 수행후 smiStatement End시, Unable to invoke
     * mutex_lock()로 서버 비정상 종료 */
    /* After Image를 새로 설정될 Index Header List에 복사해 준다. */
    sFstIndexHeader  = (SChar*)aIndexHeaderArr + ID_SIZEOF( smVCPieceHeader );
    IDE_ASSERT( smmManager::getOIDPtr( 
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    aOIDIndexHeaderArr,
                                   (void**) &sDestIndexRowPtr )
                == IDE_SUCCESS );
    sDestIndexRowPtr += ID_SIZEOF( smVCPieceHeader );

    sCopySize = aIndexHeaderArrLen - sIndexHeaderSize;

    /* create 되는 Index는 처음, 혹은 마지막에 추가된다.*/
    if( sFstIndexHeader == aDropIndexHeader )
    {
         // 첫번째 IndexHeader를 제거하는 경우
        idlOS::memcpy( sDestIndexRowPtr,
                       sFstIndexHeader + sIndexHeaderSize,
                       sCopySize );
    }
    else
    {
        // 마지막 IndexHeader를 제거하는 경우
        IDE_ASSERT( (UInt)((SChar*)aDropIndexHeader - sFstIndexHeader) == sCopySize );

        idlOS::memcpy( sDestIndexRowPtr,
                       sFstIndexHeader,
                       sCopySize );
    }

    /* BUG-30612 index header self oid가 잘못 기록될 수 있습니다.
     * 다른 slot에서 복사해 온 것이므로 self oid가 잘못되어 있다.
     * self oid를 보정한다. */
    adjustIndexSelfOID( (UChar*)sDestIndexRowPtr,
                        aOIDIndexHeaderArr + ID_SIZEOF(smVCPieceHeader),
                        sCopySize );

    sAftImgInfo.mLogImg = sDestIndexRowPtr;
    sAftImgInfo.mSize   = sCopySize;

    SC_MAKE_GRID(
        sGRID,
        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
        SM_MAKE_PID( aOIDIndexHeaderArr ),
        SM_MAKE_OFFSET( aOIDIndexHeaderArr ) + ID_SIZEOF( smVCPieceHeader ));

    /* redo only log이므로 wal을 지키지 않아도 된다. */
    IDE_TEST( smrUpdate::writeUpdateLog( NULL, /* idvSQL* */
                                         aTrans,
                                         SMR_PHYSICAL,
                                         sGRID,
                                         0, /* smrUpdateLog::mData */
                                         0, /* Before Image Count */
                                         NULL, /* Before Image Info Ptr */
                                         1, /* After Image Count */
                                         &sAftImgInfo )
              != IDE_SUCCESS );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                     SM_MAKE_PID( aOIDIndexHeaderArr ) )
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Table의 Flag를 변경한다.
 *
 * aTrans       [IN] Flag를 변경할 트랜잭션
 * aTableHeader [IN] Flag를 변경할 Table의 Header
 * aFlagMask    [IN] 변경할 Table Flag의 Mask
 * aFlagValue   [IN] 변경할 Table Flag의 Value
 **********************************************************************/
IDE_RC smcTable::alterTableFlag( void           * aTrans,
                                 smcTableHeader * aTableHeader,
                                 UInt             aFlagMask,
                                 UInt             aFlagValue )
{
    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aTableHeader != NULL );
    IDE_DASSERT( aFlagMask != 0);
    // Mask외의 Bit를 Value가 사용해서는 안된다.
    IDE_DASSERT( (~aFlagMask & aFlagValue) == 0 );

    UInt sNewFlag;

    // Table Flag변경 관련 에러 체크 실시
    IDE_TEST( checkError4AlterTableFlag( aTableHeader,
                                         aFlagMask,
                                         aFlagValue )
              != IDE_SUCCESS );

    sNewFlag = aTableHeader->mFlag;

    // Mask Bit를 모두 Clear
    sNewFlag = sNewFlag & ~aFlagMask;
    // Value Bit를 Set
    sNewFlag = sNewFlag | aFlagValue;

    IDE_TEST(
        smrUpdate::updateFlagAtTableHead(
            NULL,
            aTrans,
            aTableHeader,
            sNewFlag)
        != IDE_SUCCESS );

    aTableHeader->mFlag = sNewFlag;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aTableHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Table에 Inconsistent하다고 설정한다.
 *
 * aTableHeader [IN/OUT] Flag를 변경할 Table의 Header
 **********************************************************************/
IDE_RC smcTable::setTableHeaderInconsistency( smOID            aTableOID )
{
    smcTableHeader   * sTableHeader;

    IDE_TEST( smcTable::getTableHeaderFromOID( aTableOID,
                                               (void**)&sTableHeader )
              != IDE_SUCCESS );

    IDE_ERROR_MSG( aTableOID == sTableHeader->mSelfOID,
                   "aTableOID : %"ID_UINT64_FMT
                   "sTableOID : %"ID_UINT64_FMT,
                   aTableOID,
                   sTableHeader->mSelfOID );

    /* Inconsistent해지는 table 정보 dump */
    dumpTableHeader( sTableHeader );

    /* 실제 이미지상에도 변경하는 경우는, RestartRecovery시에도
     * 설정해야한다. */
    IDE_TEST( smrUpdate::setInconsistencyAtTableHead(
            sTableHeader,
            ID_FALSE ) // aForMediaRecovery
        != IDE_SUCCESS );

    /* Inconsistent -> Consistent는 없음 */
    sTableHeader->mIsConsistent = ID_FALSE;

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(sTableHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smcTable::setTableHeaderDropFlag( scSpaceID    aSpaceID,
                                         scPageID     aPID,
                                         scOffset     aOffset,
                                         idBool       aIsDrop )
{
    smpSlotHeader *sSlotHeader;

    IDE_ASSERT( smmManager::getOIDPtr( aSpaceID,
                                       SM_MAKE_OID( aPID, aOffset ),
                                       (void**)&sSlotHeader )
                == IDE_SUCCESS );

    if( aIsDrop == ID_TRUE )
    {
        SMP_SLOT_SET_DROP( sSlotHeader );
    }
    else
    {
        SMP_SLOT_UNSET_DROP( sSlotHeader );
    }

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(aSpaceID, aPID) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Create Table을 위한 에러처리
    [IN] aSpaceID   - 테이블스페이스의 ID
    [IN] aTableFlag - 생성할 테이블의 Flag
*/
IDE_RC smcTable::checkError4CreateTable( scSpaceID aSpaceID,
                                         UInt      aTableFlag )
{
    // 1. Volatile Table에 대해 로그 압축하도록 설정된 경우 에러
    if ( sctTableSpaceMgr::isVolatileTableSpace( aSpaceID ) == ID_TRUE )
    {
        if ( (aTableFlag & SMI_TABLE_LOG_COMPRESS_MASK) ==
             SMI_TABLE_LOG_COMPRESS_TRUE )
        {
            IDE_RAISE( err_volatile_log_compress );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_volatile_log_compress );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Table의 Flag변경에 대한 에러처리

    [IN] aTableHeader - 변경할 테이블의 헤더
    [IN] aFlagMask    - 변경할 Table Flag의 Mask
    [IN] aFlagValue   - 변경할 Table Flag의 Value
 */
IDE_RC smcTable::checkError4AlterTableFlag( smcTableHeader * aTableHeader,
                                            UInt             aFlagMask,
                                            UInt             aFlagValue )
{
    IDE_DASSERT( aTableHeader != NULL );

    // 1. Volatile Table에 대해 로그 압축하도록 설정시 에러
    if ( sctTableSpaceMgr::isVolatileTableSpace(
                               aTableHeader->mSpaceID ) == ID_TRUE )
    {
        if ( (aFlagMask & SMI_TABLE_LOG_COMPRESS_MASK) != 0 )
        {
            if ( (aFlagValue & SMI_TABLE_LOG_COMPRESS_MASK )
                == SMI_TABLE_LOG_COMPRESS_TRUE )
            {

                IDE_RAISE( err_volatile_log_compress );
            }
        }
    }


    // 2. 이미 Table Header에 해당 Flag로 설정되어 있는 경우 에러
    IDE_TEST_RAISE( (aTableHeader->mFlag & aFlagMask) == aFlagValue,
                    err_table_flag_already_set );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_table_flag_already_set );
    {
        IDE_SET(ideSetErrorCode( smERR_ABORT_TABLE_ATTR_FLAG_ALREADY_SET));
    }
    IDE_EXCEPTION( err_volatile_log_compress );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_UNABLE_TO_COMPRESS_VOLATILE_TBS_LOG));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description : Replication을 위해서 Table Meta를 기록한다.
 *
 * [IN] aTrans      - Log Buffer를 포함한 Transaction
 * [IN] aTableMeta  - 기록할 Table Meta의 헤더
 * [IN] aLogBody    - 기록할 Log Body
 * [IN] aLogBodyLen - 기록할 Log Body의 길이
 ******************************************************************************/
IDE_RC smcTable::writeTableMetaLog(void         * aTrans,
                                   smrTableMeta * aTableMeta,
                                   const void   * aLogBody,
                                   UInt           aLogBodyLen)
{
    smrTableMetaLog sLogHeader;
    smTID           sTransID;
    smrLogType      sLogType = SMR_LT_TABLE_META;
    UInt            sOffset;

    IDE_ASSERT(aTrans != NULL);
    IDE_ASSERT(aTableMeta != NULL);

    smLayerCallback::initLogBuffer( aTrans );
    sOffset = 0;

    /* Log header를 구성한다. */
    idlOS::memset(&sLogHeader, 0, ID_SIZEOF(smrTableMetaLog));

    sTransID = smLayerCallback::getTransID( aTrans );

    smrLogHeadI::setType(&sLogHeader.mHead, sLogType);

    smrLogHeadI::setSize(&sLogHeader.mHead,
                         SMR_LOGREC_SIZE(smrTableMetaLog) +
                         aLogBodyLen +
                         ID_SIZEOF(smrLogTail));

    smrLogHeadI::setTransID(&sLogHeader.mHead, sTransID);

    smrLogHeadI::setPrevLSN( &sLogHeader.mHead,
                             smLayerCallback::getLstUndoNxtLSN( aTrans ) );

    smrLogHeadI::setFlag(&sLogHeader.mHead, SMR_LOG_TYPE_NORMAL);

    if( (smrLogHeadI::getFlag(&sLogHeader.mHead) & SMR_LOG_SAVEPOINT_MASK)
         == SMR_LOG_SAVEPOINT_OK)
    {
        smrLogHeadI::setReplStmtDepth( &sLogHeader.mHead,
                                       smLayerCallback::getLstReplStmtDepth( aTrans ) );
    }
    else
    {
        smrLogHeadI::setReplStmtDepth( &sLogHeader.mHead,
                                       SMI_STATEMENT_DEPTH_NULL );
    }

    sLogHeader.mTableMeta = *aTableMeta;

    /* Write log header */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 (const void *)&sLogHeader,
                                                 sOffset,
                                                 SMR_LOGREC_SIZE(smrTableMetaLog) )
              != IDE_SUCCESS );
    sOffset += SMR_LOGREC_SIZE(smrTableMetaLog);

    /* Write log body */
    if (aLogBody != NULL)
    {
        IDE_ASSERT(aLogBodyLen != 0);
        IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                     aLogBody,
                                                     sOffset,
                                                     aLogBodyLen )
                  != IDE_SUCCESS );
        sOffset += aLogBodyLen;
    }

    /* Write log tail */
    IDE_TEST( smLayerCallback::writeLogToBuffer( aTrans,
                                                 (const void *)&sLogType,
                                                 sOffset,
                                                 ID_SIZEOF(smrLogType) )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::writeTransLog( aTrans, sLogHeader.mTableMeta.mOldTableOID ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Disk Table의 Insert Limit을 변경한다.
 *
 * aTrans       [IN] Flag를 변경할 트랜잭션
 * aTableHeader [IN] Flag를 변경할 Table의 Header
 * aSegAttr     [IN] 변경할 Table Insert Limit
 **********************************************************************/
IDE_RC smcTable::alterTableSegAttr( void                  * aTrans,
                                    smcTableHeader        * aHeader,
                                    smiSegAttr              aSegAttr,
                                    sctTBSLockValidOpt      aTBSLvOpt )
{
    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    // Disk Table이 아니면 무의미하므로 무시한다.
    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    not_support_error );

    // DDL이 이므로 Table Validation을 수행한다.
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt,
                             SMC_LOCK_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( smrUpdate::updateInsLimitAtTableHead(
                          NULL,
                          aTrans,
                          aHeader,
                          aHeader->mFixed.mDRDB.mSegDesc.mSegHandle.mSegAttr,
                          aSegAttr ) != IDE_SUCCESS );

    sdpSegDescMgr::setSegAttr( &aHeader->mFixed.mDRDB.mSegDesc,
                               &aSegAttr );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Disk Table의 Storage Attribute를 변경한다.
 *
 * aTrans       [IN] Flag를 변경할 트랜잭션
 * aTableHeader [IN] Flag를 변경할 Table의 Header
 * aSegStoAttr  [IN] 변경할 Table Storage Attribute
 **********************************************************************/
IDE_RC smcTable::alterTableSegStoAttr( void               * aTrans,
                                       smcTableHeader     * aHeader,
                                       smiSegStorageAttr    aSegStoAttr,
                                       sctTBSLockValidOpt   aTBSLvOpt )
{
    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    // Disk Table이 아니면 무의미하므로 무시한다.
    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    not_support_error );

    // DDL이 이므로 Table Validation을 수행한다.
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt,
                             SMC_LOCK_TABLE )
              != IDE_SUCCESS );

    IDE_TEST( smrUpdate::updateSegStoAttrAtTableHead(
                  NULL,
                  aTrans,
                  aHeader,
                  aHeader->mFixed.mDRDB.mSegDesc.mSegHandle.mSegStoAttr,
                  aSegStoAttr )
              != IDE_SUCCESS );

    sdpSegDescMgr::setSegStoAttr( &aHeader->mFixed.mDRDB.mSegDesc,
                                  &aSegStoAttr );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Disk Index의 INIT/MAX TRANS를 변경한다.
 *
 * aTrans       [IN] Flag를 변경할 트랜잭션
 * aTableHeader [IN] Flag를 변경할 Table의 Header
 * aSegAttr     [IN] 변경할 INDEX INIT/MAX TRANS
 **********************************************************************/
IDE_RC smcTable::alterIndexSegAttr( void                  * aTrans,
                                    smcTableHeader        * aHeader,
                                    void                  * aIndex,
                                    smiSegAttr              aSegAttr,
                                    sctTBSLockValidOpt      aTBSLvOpt )
{
    UInt         sIndexSlot;
    SInt         sOffset;
    UInt         sIndexHeaderSize;
    smiSegAttr * sBfrSegAttr;

    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    // Disk Table이 아니면 무의미하므로 무시한다.
    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    not_support_error );

    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    // DDL이 이므로 Table Validation을 수행한다.
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt,
                             SMC_LOCK_TABLE )
              != IDE_SUCCESS );

    // BUG-23218 : 인덱스 탐색 모듈화
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(aHeader,
                                              aIndex,
                                              sIndexHeaderSize,
                                              &sIndexSlot,
                                              &sOffset)
                 == IDE_SUCCESS );

    sBfrSegAttr = smLayerCallback::getIndexSegAttrPtr( aIndex );

    IDE_TEST(smrUpdate::setIdxHdrSegAttr(
                 NULL, /* idvSQL* */
                 aTrans,
                 aHeader->mIndexes[sIndexSlot].fstPieceOID,
                 ID_SIZEOF(smVCPieceHeader) + sOffset , //BUG-23218 : 로깅 위치 버그 수정
                 *sBfrSegAttr,
                 aSegAttr ) != IDE_SUCCESS);

    smLayerCallback::setIndexSegAttrPtr( aIndex, aSegAttr );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  SM_MAKE_PID(aHeader->mSelfOID))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Disk Index의 Storage Attribute를 변경한다.
 *
 * aTrans       [IN] Flag를 변경할 트랜잭션
 * aHeader      [IN] Flag를 변경할 Table의 Header
 * aIndex       [IN] Flag를 변경할 Index의 Header
 * aSegStoAttr  [IN] 변경할 Storage Attribute
 **********************************************************************/
IDE_RC smcTable::alterIndexSegStoAttr( void                * aTrans,
                                       smcTableHeader      * aHeader,
                                       void                * aIndex,
                                       smiSegStorageAttr     aSegStoAttr,
                                       sctTBSLockValidOpt    aTBSLvOpt )
{
    smnIndexHeader    *sIndexHeader;
    UInt               sIndexHeaderSize;
    UInt               sIndexSlot;
    scPageID           sPageID = SC_NULL_PID;
    SInt               sOffset;
    smiSegStorageAttr* sBfrSegStoAttr;
    smiSegStorageAttr  sAftSegStoAttr;

    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    // Disk Table이 아니면 무의미하므로 무시한다.
    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    not_support_error );

    sIndexHeader = ((smnIndexHeader*)aIndex);
    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    /* ----------------------------
     * [1] table validation
     * ---------------------------*/
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt )
              != IDE_SUCCESS );

    // BUG-23218 : 인덱스 탐색  모듈화
    /* ----------------------------
     * [2] 수정하고자 하는 Index를 찾는다.
     * ---------------------------*/
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(aHeader,
                                              sIndexHeader,
                                              sIndexHeaderSize,
                                              &sIndexSlot,
                                              &sOffset)
                 == IDE_SUCCESS );

    /* ------------------------------------------------
     * [3] 찾은 인덱스의 sIndexSlot을 바탕으로,
     *     인덱스의 위치를 계산한다.
     * ------------------------------------------------*/
    sPageID = SM_MAKE_PID( aHeader->mIndexes[sIndexSlot].fstPieceOID );

    /* ---------------------------------------------
     * [4] 기존의 Segment Storage Attribute를 가져와
     * before/after 이미지를 만든다.
     * ---------------------------------------------*/

    sBfrSegStoAttr = smLayerCallback::getIndexSegStoAttrPtr( aIndex );
    sAftSegStoAttr = *sBfrSegStoAttr;

    if ( aSegStoAttr.mInitExtCnt != 0 )
    {
        sAftSegStoAttr.mInitExtCnt = aSegStoAttr.mInitExtCnt;
    }
    if ( aSegStoAttr.mNextExtCnt != 0 )
    {
        sAftSegStoAttr.mNextExtCnt = aSegStoAttr.mNextExtCnt;
    }
    if ( aSegStoAttr.mMinExtCnt != 0 )
    {
        sAftSegStoAttr.mMinExtCnt = aSegStoAttr.mMinExtCnt;
    }
    if ( aSegStoAttr.mMaxExtCnt != 0 )
    {
        sAftSegStoAttr.mMaxExtCnt = aSegStoAttr.mMaxExtCnt;
    }

    /* ---------------------------------------------
     * [5] 로깅 후 적용한다.
     * ---------------------------------------------*/
    IDE_TEST(smrUpdate::setIdxHdrSegStoAttr(
                 NULL, /* idvSQL* */
                 aTrans,
                 aHeader->mIndexes[sIndexSlot].fstPieceOID,
                 ID_SIZEOF(smVCPieceHeader) + sOffset, // BUG-23218 : 로깅 위치 버그 수정
                 *sBfrSegStoAttr,
                 sAftSegStoAttr ) != IDE_SUCCESS);


    smLayerCallback::setIndexSegStoAttrPtr( aIndex, sAftSegStoAttr );

    (void)smmDirtyPageMgr::insDirtyPage(
        SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, sPageID);

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcTable::alterTableAllocExts( idvSQL              * aStatistics,
                                      void                * aTrans,
                                      smcTableHeader      * aHeader,
                                      ULong                 aExtendSize,
                                      sctTBSLockValidOpt    aTBSLvOpt )
{
    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    // Disk Table이 아니면 무의미하므로 무시한다.
    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    not_support_error );

    // DDL이 이므로 Table Validation을 수행한다.
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt,
                             SMC_LOCK_TABLE ) != IDE_SUCCESS );

    IDE_TEST( sdpSegment::allocExts( aStatistics,
                                     aHeader->mSpaceID,
                                     aTrans,
                                     &aHeader->mFixed.mDRDB.mSegDesc,
                                     aExtendSize ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smcTable::alterIndexAllocExts( idvSQL              * aStatistics,
                                      void                * aTrans,
                                      smcTableHeader      * aHeader,
                                      void                * aIndex,
                                      ULong                 aExtendSize,
                                      sctTBSLockValidOpt    aTBSLvOpt )
{
    const scGRID   *sIndexGRID;

    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    // Disk Table이 아니면 무의미하므로 무시한다.
    IDE_TEST_RAISE( SMI_TABLE_TYPE_IS_DISK( aHeader ) == ID_FALSE,
                    not_support_error );

    // DDL이 이므로 Table Validation을 수행한다.
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             aTBSLvOpt,
                             SMC_LOCK_TABLE ) != IDE_SUCCESS );

    sIndexGRID   = smLayerCallback::getIndexSegGRIDPtr( aIndex );

    IDE_TEST( sdpSegment::allocExts( aStatistics,
                                     SC_MAKE_SPACE(*sIndexGRID),
                                     aTrans,
                                     (sdpSegmentDesc*)smLayerCallback::getSegDescByIdxPtr( aIndex ),
                                     aExtendSize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


//fix BUG-22395
IDE_RC smcTable::alterIndexName( idvSQL              * aStatistics,
                                 void                * aTrans,
                                 smcTableHeader      * aHeader,
                                 void                * aIndex,
                                 SChar               * aName )
{
    smnIndexHeader  *sIndexHeader;
    UInt             sTableType;
    UInt             sIndexHeaderSize;
    UInt             sIndexSlot;
    SInt             sIndexOffset;
    scPageID         sPageID = SC_NULL_PID;
    scOffset         sNameOffset = SC_NULL_OFFSET;

    IDE_DASSERT( aTrans != NULL);
    IDE_DASSERT( aHeader != NULL );

    sTableType = SMI_GET_TABLE_TYPE( aHeader );
    IDE_TEST_RAISE( !(sTableType == SMI_TABLE_MEMORY   ||
                      sTableType == SMI_TABLE_DISK     ||
                      sTableType == SMI_TABLE_VOLATILE ),
                    not_support_error );

    sIndexHeader = ((smnIndexHeader*)aIndex);
    sIndexHeaderSize = smLayerCallback::getSizeOfIndexHeader();

    /* ----------------------------
     * [1] table validation
     * ---------------------------*/
    IDE_TEST( validateTable( aTrans,
                             aHeader,
                             SCT_VAL_DDL_DML )
              != IDE_SUCCESS );

    /* ----------------------------
     * [2] 수정하고자 하는 Index를 찾는다.
     * ---------------------------*/
    IDE_ASSERT( findIndexVarSlot2AlterAndDrop(aHeader,
                                              sIndexHeader,
                                              sIndexHeaderSize,
                                              &sIndexSlot,
                                              &sIndexOffset)
                 == IDE_SUCCESS );

    /* ------------------------------------------------
     * [3] 찾은 인덱스의 sIndexSlot와 offset을 바탕으로,
     * ------------------------------------------------*/
    sPageID     = SM_MAKE_PID( aHeader->mIndexes[sIndexSlot].fstPieceOID );
    sNameOffset = SM_MAKE_OFFSET( aHeader->mIndexes[sIndexSlot].fstPieceOID )
                  + ID_SIZEOF(smVCPieceHeader) 
                  + sIndexOffset + smLayerCallback::getIndexNameOffset() ;

    /* ----------------------------
     * [4] 로깅 후 이름을 설정한다.
     * ----------------------------*/
    IDE_TEST( smrUpdate::physicalUpdate( aStatistics,
                                         aTrans,
                                         SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                         sPageID,
                                         sNameOffset,
                                         smLayerCallback::getNameOfIndexHeader( sIndexHeader ), //before image
                                         SMN_MAX_INDEX_NAME_SIZE,
                                         aName, //after image
                                         SMN_MAX_INDEX_NAME_SIZE,
                                         NULL,
                                         0)
              != IDE_SUCCESS );

    smLayerCallback::setNameOfIndexHeader( sIndexHeader, aName );

    (void)smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                         sPageID);

    return IDE_SUCCESS;

    IDE_EXCEPTION( not_support_error );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_Not_Support_function));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : MEMORY TABLE에 FREE PAGE들을 DB에 반납한다.
 *
 * aTable     [IN] COMPACT할 TABLE의 헤더
 * aTrans     [IN] Transaction Pointer
 * aPages	  [IN] Numer of Pages for compaction (0, UINT_MAX : all)
 **********************************************************************/
IDE_RC smcTable::compact( void           * aTrans,
                          smcTableHeader * aTable,
                          UInt             aPages )
{
    UInt               i;
    smpPageListEntry * sPageListEntry;
    smpPageListEntry * sVolPageListEntry;

    IDE_DASSERT(aTrans != NULL);
    IDE_DASSERT(aTable != NULL);

    // PRJ-1548 User Memory TableSpace 개념도입
    // Validate Table 에서 다음과 같이 Lock을 획득한다.
    // [1] Table의 TableSpace에 대해 IX
    // [2] Table에 대해 X
    // [3] Table의 Index, Lob Column TableSpace에 대해 IX
    // Table의 상위는 Table의 TableSpace이며 그에 대해 IX를 획득한다.
    // 단, 메모리 Table은 [3] 사항에 대해서 수행하지 않는다.
    // BUGBUG-내부에서 ValidateTable 을 수행하지 않음.

    /* PROJ-1594 Volatile TBS */
    /* compact될 테이블이 memory 테이블 또는 volatile 테이블이다. */
    if ( (SMI_TABLE_TYPE_IS_MEMORY( aTable ) == ID_TRUE) ||
         (SMI_TABLE_TYPE_IS_META( aTable ) == ID_TRUE) )
    {
        // Pool에 등록된 FreePage(EmptyPage)가 너무 많으면 DB로 반납한다.
        // for Fixed
        sPageListEntry = &(aTable->mFixed.mMRDB);

        if ( sPageListEntry->mRuntimeEntry->mFreePagePool.mPageCount
            > SMP_FREEPAGELIST_MINPAGECOUNT )
        {
            IDE_TEST( smpFreePageList::freePagesFromFreePagePoolToDB(
                                                              aTrans,
                                                              aTable->mSpaceID,
                                                              sPageListEntry,
                                                              aPages )
                      != IDE_SUCCESS );
        }


        // for Var
        for( i = 0; i < SM_VAR_PAGE_LIST_COUNT ; i++ )
        {
            sPageListEntry = aTable->mVar.mMRDB + i;

            if( sPageListEntry->mRuntimeEntry->mFreePagePool.mPageCount
                > SMP_FREEPAGELIST_MINPAGECOUNT )
            {
                IDE_TEST( smpFreePageList::freePagesFromFreePagePoolToDB(
                                                          aTrans,
                                                          aTable->mSpaceID,
                                                          sPageListEntry,
                                                          aPages )
                          != IDE_SUCCESS );
            }
        }
    }
    else
    {
        IDE_ASSERT( SMI_TABLE_TYPE_IS_VOLATILE( aTable ) == ID_TRUE );

        // Pool에 등록된 FreePage(EmptyPage)가 너무 많으면 DB로 반납한다.
        // for Fixed
        sVolPageListEntry = &(aTable->mFixed.mVRDB);

        if( sVolPageListEntry->mRuntimeEntry->mFreePagePool.mPageCount
            > SMP_FREEPAGELIST_MINPAGECOUNT )
        {
            IDE_TEST( svpFreePageList::freePagesFromFreePagePoolToDB(
                                                      aTrans,
                                                      aTable->mSpaceID,
                                                      sVolPageListEntry,
                                                      aPages )
                      != IDE_SUCCESS );
        }

        // for Var
        for( i = 0; i < SM_VAR_PAGE_LIST_COUNT ; i++ )
        {
            sVolPageListEntry = aTable->mVar.mVRDB + i;

            if( sVolPageListEntry->mRuntimeEntry->mFreePagePool.mPageCount
                > SMP_FREEPAGELIST_MINPAGECOUNT )
            {
                IDE_TEST( svpFreePageList::freePagesFromFreePagePoolToDB(
                                                      aTrans,
                                                      aTable->mSpaceID,
                                                      sVolPageListEntry,
                                                      aPages )
                          != IDE_SUCCESS );
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool smcTable::isSegPIDOfTbl( void* aTable, scSpaceID aSpaceID, scPageID aSegPID )
{
    smcTableHeader *sTblHdr = (smcTableHeader*)((smpSlotHeader*)aTable + 1);

    if( ( aSegPID  == sdpPageList::getTableSegDescPID( &sTblHdr->mFixed.mDRDB ) ) &&
        ( aSpaceID == sTblHdr->mSpaceID ) )
    {
        return ID_TRUE;
    }
    else
    {
        return ID_FALSE;
    }
}

scPageID smcTable::getSegPID( void* aTable )
{
    smcTableHeader *sTblHdr = (smcTableHeader*)((smpSlotHeader*)aTable + 1);

    return sdpPageList::getTableSegDescPID( &sTblHdr->mFixed.mDRDB );
}

idBool smcTable::isIdxSegPIDOfTbl( void* aTable, scSpaceID aSpaceID, scPageID aSegPID )
{
    UInt            sIdxCnt;
    UInt            i;
    const void     *sIndex;
    const scGRID   *sIndexGRID;

    sIdxCnt = getIndexCount( aTable );

    for( i = 0; i < sIdxCnt ; i++ )
    {
        sIndex = getTableIndex( aTable, i );
        sIndexGRID = smLayerCallback::getIndexSegGRIDPtr( (void*)sIndex );

        if( ( SC_MAKE_SPACE(*sIndexGRID) == aSpaceID ) &&
            ( SC_MAKE_PID(*sIndexGRID)  == aSegPID ) )
        {
            break;
        }
    }

    if( i == sIdxCnt )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

idBool smcTable::isLOBSegPIDOfTbl( void* aTable, scSpaceID aSpaceID, scPageID aSegPID )
{
    const smiColumn *sColumn;
    smcTableHeader  *sTblHdr = (smcTableHeader*)((smpSlotHeader*)aTable + 1);

    UInt       sColumnCnt = getColumnCount( sTblHdr );
    UInt       i;

    for( i = 0; i < sColumnCnt; i++ )
    {
        sColumn = smcTable::getColumn( sTblHdr, i );

        if( ( sColumn->flag & SMI_COLUMN_TYPE_MASK ) ==
            SMI_COLUMN_TYPE_LOB )
        {
            if( ( sColumn->colSeg.mSpaceID == aSpaceID ) &&
                ( sColumn->colSeg.mPageID  == aSegPID ) )
            {
                break;
            }
        }
    }

    if( i == sColumnCnt )
    {
        return ID_FALSE;
    }
    else
    {
        return ID_TRUE;
    }
}

/***********************************************************************
 * Description : Replicate Log를 기록해야 하는 Table인지 판단한다.
 *
 * Implementation :
 *
 *  aTableHeader - [IN] Replicatiohn 유무를 확인할 Table 의 Header
 *  aTrans       - [IN] Transactioin
 **********************************************************************/

idBool smcTable::needReplicate(const smcTableHeader* aTableHeader,
                               void*  aTrans)
{
    idBool sReplicate;
    UInt   sLogTypeFlag;

    IDE_DASSERT( aTableHeader != NULL );
    IDE_DASSERT( aTrans != NULL );

    sReplicate    = smcTable::isReplicationTable(aTableHeader);
    sLogTypeFlag  = smLayerCallback::getLogTypeFlagOfTrans( aTrans );

    /* 해당 Table이 Replication이 걸려 있고 현재 Transaction이 Normal
     * Transaction이라면 Replication Sender가 볼수 있는 형태로 로그를 남겨야함
     * PROJ-1608 recovery from replication
     * Replication을 이용한 복구를 위해서 Normal(SMR_LOG_TYPE_NORMAL)이 아닌
     * recovery를 지원하는 receiver가 수행한 트랜잭션(SMR_LOG_TYPE_REPL_RECOVERY)도
     * Recovery Sender가 읽을 수 있도록 기록해야 하므로 Replicate할 수 있는 형태로
     * 기록되어야 한다.
     */
    if(sReplicate == ID_TRUE)
    {
        if((sLogTypeFlag == SMR_LOG_TYPE_NORMAL) ||
           (sLogTypeFlag == SMR_LOG_TYPE_REPL_RECOVERY))
        {
            sReplicate = ID_TRUE;
        }
        else
        {
            sReplicate = ID_FALSE;
        }
    }
    else
    {
        sReplicate = ID_FALSE;
    }

    /* PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발

       Supplemental Logging 하는 경우 Replication Transaction인지
       관계없이 무조건 Replication 로그를 기록한다
    */
    if ( smcTable::isSupplementalTable(aTableHeader) == ID_TRUE )
    {
        sReplicate = ID_TRUE;
    }

    return sReplicate;
}

/***********************************************************************
 * Description : index oid로부터 index header를 얻는다.
 * index의 OID는 index header를 바로 가리킨다.
 ***********************************************************************/
IDE_RC smcTable::getIndexHeaderFromOID( smOID     aOID, 
                                        void   ** aHeader )
{
    IDE_TEST( smmManager::getOIDPtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                     aOID,
                                     (void**)aHeader ) 
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description :
 * create Table에서 commitSCN 을smcTableHeader.mCreateSCN 에 설정한다. 
 * drop Table 에서 smcTableHeader.mCreateSCN 초기화 (SM_SCN_INFINITE)
 * ager 를 위한 정보 (BUG-48588)
 ***********************************************************************/
IDE_RC smcTable::setTableCreateSCN( smcTableHeader * aHeader, smSCN  aSCN )
{
    aHeader->mTableCreateSCN = aSCN; 

    return smmDirtyPageMgr::insDirtyPage( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                          SM_MAKE_PID(aHeader->mSelfOID) );
}
 

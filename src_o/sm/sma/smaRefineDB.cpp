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
 * $Id: smaRefineDB.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <idl.h>
#include <smm.h>
#include <smc.h>
#include <smn.h>
#include <smp.h>
#include <smaDef.h>
#include <smaRefineDB.h>
#include <sdd.h>
#include <sct.h>
#include <svcRecord.h>

IDE_RC smaRefineDB::refineTempCatalogTable(smxTrans       * aTrans,
                                           smcTableHeader * aHeader )
{

    IDE_TEST( smcTable::initLockAndRuntimeItem( aHeader ) != IDE_SUCCESS );

    // temp catalog table에 할당된 모든 page를 시스템에 반환한다.
    IDE_TEST(smcTable::dropTablePageListPending(aTrans,
                                                aHeader,
                                                ID_TRUE)
             != IDE_SUCCESS);

    // 모든 temp tablespace를 초기화시킨다.
    IDE_TEST( sctTableSpaceMgr::resetAllTempTBS((void*)aTrans)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}
/*
    Catalog Table의  Slot을 Free하고 Free Slot List에 매단다.

    [IN] aTrans   - slot을 free하려는 transaction
    [IN] aSlotPtr - slot의 포인터
 */
IDE_RC smaRefineDB::freeCatalogSlot( smxTrans  * aTrans,
                                     SChar     * aSlotPtr )
{
    scPageID sPageID = SMP_SLOT_GET_PID(aSlotPtr);

    IDE_TEST( smpFixedPageList::setFreeSlot(
                  aTrans,
                  SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                  sPageID,
                  aSlotPtr,
                  SMP_TABLE_NORMAL) != IDE_SUCCESS );

    smpFixedPageList::addFreeSlotToFreeSlotList(
        smpFreePageList::getFreePageHeader(
            SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
            sPageID),
        aSlotPtr );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaRefineDB::refineCatalogTableVarPage(smxTrans       * aTrans,
                                              smcTableHeader * aHeader)
{
    SChar           * sCurPtr;
    SChar           * sNxtPtr;
    smOID             sCurPieceOID;
    smOID             sNxtPieceOID;
    smVCPieceHeader * sVCPieceHeaderPtr;
    scPageID          sPageID;
    UInt              sIdx;
    UInt              i;
    UInt              sIndexHeaderSize;
    SChar           * sSrc;
    scGRID          * sIndexSegGRID;
    void            * sPagePtr;


    /* ----------------------------
     * [1] Table의 LockItem과 각 Mutex를
     * 초기화한다.
     * ---------------------------*/
    IDE_TEST( smcTable::initLockAndRuntimeItem( aHeader )
              != IDE_SUCCESS );

    /* ----------------------------
     * [2] Table의 Variable 영역의 값을 읽어
     * 무용화(Obsolete)된 값을 정리한다.
     * ---------------------------*/

    sCurPtr = NULL;
    sCurPieceOID = SM_NULL_OID;

    sIndexHeaderSize = smnManager::getSizeOfIndexHeader();

    while(1)
    {
        IDE_TEST( smpVarPageList::nextOIDallForRefineDB( aHeader->mSpaceID,
                                                         aHeader->mVar.mMRDB,
                                                         sCurPieceOID,
                                                         sCurPtr,
                                                         &sNxtPieceOID,
                                                         &sNxtPtr,
                                                         &sIdx)
                  != IDE_SUCCESS );

        if( sNxtPtr == NULL )
        {
            break;
        }

        sVCPieceHeaderPtr = (smVCPieceHeader *)sNxtPtr;
        sSrc              = sNxtPtr + ID_SIZEOF(smVCPieceHeader);

        if( SM_VCPIECE_IS_DISK_INDEX( sVCPieceHeaderPtr->flag) )
        {
            for( i = 0; i < sVCPieceHeaderPtr->length; i += sIndexHeaderSize,
                     sSrc += sIndexHeaderSize )
            {
                /* PROJ-2433
                 * smnIndexHeader 구조체의 mDropFlag를 UInt->UShort로 변경 */
                if ( (smnManager::getIndexDropFlag( sSrc ) == (UShort)SMN_INDEX_DROP_FALSE ) &&
                     (smnManager::isIndexEnabled( sSrc ) == ID_TRUE ) )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                /* BUG-33803 ALL INDEX DISABLE pending 연산 도중 서버 비정상
                 * 종료시, 서버 재구동 후 바로 대상 table을 DROP 하면,
                 * mHeader에 쓰레기값이 들어있는 상태에서 index drop을 시도하여
                 * segmentation fault가 발생한다. 따라서 disable 상태의 index는
                 * refine 과정에서 mHeader를 NULL로 설정해 준다. */
                ((smnIndexHeader*)sSrc)->mHeader = NULL;
                sIndexSegGRID = smnManager::getIndexSegGRIDPtr(sSrc);

                if(SC_GRID_IS_NOT_NULL(*sIndexSegGRID))
                {
                    // xxxx 주석
                    IDE_TEST( sdpSegment::freeIndexSeg4Entry(
                                  NULL,
                                  SC_MAKE_SPACE( *sIndexSegGRID ),
                                  aTrans,
                                  sNxtPieceOID + i + ID_SIZEOF(smVCPieceHeader),
                                  SDR_MTX_LOGGING)
                              != IDE_SUCCESS );
                }
            }
        }

        sPageID = SM_MAKE_PID(sNxtPieceOID);

        /* ----------------------------
         * Variable Slot의 Delete Flag가
         * 설정된 경우 무용화된 Row이다.
         * ---------------------------*/
        IDE_ASSERT( ( sVCPieceHeaderPtr->flag & SM_VCPIECE_FREE_MASK )
                    == SM_VCPIECE_FREE_NO );

        IDE_ASSERT( smmManager::getPersPagePtr( SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                                sPageID,
                                                &sPagePtr )
                    == IDE_SUCCESS );
        sIdx = smpVarPageList::getVarIdx( sPagePtr );
        (aHeader->mVar.mMRDB[sIdx].mRuntimeEntry->mInsRecCnt)++;

        sCurPieceOID = sNxtPieceOID;
        sCurPtr      = sNxtPtr;
    }

    for( i = 0; i < SM_VAR_PAGE_LIST_COUNT ; i++ )
    {
        // FreePageList[0]에서 N개의 FreePageList에 FreePage들을 나눠주고
        smpFreePageList::distributePagesFromFreePageList0ToTheOthers( (&(aHeader->mVar.mMRDB[i])) );

        // EmptyPage(전혀사용하지않는 FreePage)가 필요이상이면
        // FreePagePool에 반납하고 FreePagePool에도 필요이상이면
        // DB에 반납한다.
        IDE_TEST(smpFreePageList::distributePagesFromFreePageList0ToFreePagePool(
                     aTrans,
                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                     &(aHeader->mVar.mMRDB[i]) )
                 != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smaRefineDB::refineCatalogTableFixedPage(smxTrans       * aTrans,
                                                smcTableHeader * aHeader)
{

    smpSlotHeader  * sSlotHeaderPtr;
    smcTableHeader * sHeader;
    SChar          * sCurPtr;
    SChar          * sNxtPtr;
    smSCN            sSCN;
    UInt             sIndexCount;
    UInt             sRecordCount = 0;
    scPageID         sCurPageID = SM_NULL_PID;
    scPageID         sPrvPageID = SM_NULL_PID;

    /* ----------------------------
     * [1] Table의 Fixed 영역의 값을 읽어
     * 무용화(Obsolete)된 값을 정리한다.
     * ---------------------------*/

    sCurPtr = NULL;

    while(1)
    {
        IDE_TEST( smpFixedPageList::nextOIDallForRefineDB(
                      aHeader->mSpaceID,
                      &(aHeader->mFixed.mMRDB),
                      sCurPtr,
                      & sNxtPtr,
                      & sCurPageID )
                  != IDE_SUCCESS );

        if( sNxtPtr == NULL )
        {
            break;
        }

        sSlotHeaderPtr = (smpSlotHeader *)sNxtPtr;

        sSCN = sSlotHeaderPtr->mCreateSCN;

        /* ----------------------------
         * Table의 Row의 상태가 최소한
         * 1) 무한대가 아니든가
         * 2) Drop flag가 설정되어 있어야 한다.
         * ---------------------------*/
        /* BUG-14975: Delete Row Alloc후 곧바로 Undo시 ASSERT발생.
        IDE_ASSERT( SM_SCN_IS_NOT_INFINITE( sSCN ) ||
                    (sSlotHeaderPtr->mDropFlag == SMP_SLOT_DROP_TRUE) );
        */

        /*
         * ----------------------------
         * 다음의 경우에 free Slot
         * 3) delete bit이 설정되어 있고 해제되지 않은 경우
         * (테이블 생성하다가 실패한 경우에는 DropFlag가 FALSE이고,
         *  DELETE BIT는 설정되어 있을수 있다.)
         * ---------------------------
         */
        if( SMP_SLOT_IS_NOT_DROP( sSlotHeaderPtr ) &&
            SM_SCN_IS_DELETED( sSCN ) )
        {
            IDE_TEST( freeCatalogSlot(aTrans,
                                      sNxtPtr) != IDE_SUCCESS );

            sCurPtr = sNxtPtr;
            continue;
        }

        sHeader = (smcTableHeader *)(sSlotHeaderPtr + 1);

        if( SMP_SLOT_IS_DROP( sSlotHeaderPtr ) ||
            SM_SCN_IS_DELETED( sSCN ) )
        {
            /* BUG-30378 비정상적으로 Drop되었지만 refine돼지 않는
             * 테이블이 존재합니다.
             * (CASE-26385)
             *
             * Ab-normal 서버 종료가 아닌 정상 종료였다면
             * Used : true, drop : true인 상태이며 Index가 있는
             * 테이블은 존재해서는 안된다.
             * (참고로 Sequence등은 used:true, drop:true인 상태로
             *  존재할 수 있다. ) */
            sIndexCount = smcTable::getIndexCount( sHeader );

            if( ( smrRecoveryMgr::isABShutDown() == ID_FALSE ) &&
                ( sIndexCount != 0 ) )
            {
                ideLog::log(
                    IDE_SERVER_0,
                    "InternalError [%s:%u]\n"
                    "Invalid table header.\n"
                    "IndexCount : %u\n",
                    (SChar *)idlVA::basename(__FILE__) ,
                    __LINE__ ,
                    sIndexCount );
                smpFixedPageList::dumpSlotHeader( sSlotHeaderPtr );
                smcTable::dumpTableHeader( sHeader );

                // 디버그 모드일때만 서버를 죽인다.
                IDE_DASSERT( 0 );
            }


            // memory ,또는  disk table drop pending operation을
            // 하고, table header slot을 free한다.

            IDE_ERROR( (sHeader->mFlag & SMI_TABLE_TYPE_MASK) !=
                       SMI_TABLE_TEMP_LEGACY );
            if( (sHeader->mFlag & SMI_TABLE_TYPE_MASK) != SMI_TABLE_FIXED )
            {
                IDE_TEST( freeTableHdr( aTrans,
                                        sHeader,
                                        SMP_SLOT_GET_FLAGS( sSlotHeaderPtr ),
                                        sNxtPtr )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            sRecordCount++;
            sIndexCount = smcTable::getIndexCount(aHeader);
            IDE_ASSERT( sIndexCount == 0);

            smcTable::addToTotalIndexCount( smcTable::getIndexCount(sHeader) );

            /*
            * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
            * 유효한 레코드가 있다면 Scan List에 추가한다.
            */
            if( sCurPageID != sPrvPageID )
            {
                IDE_TEST( smpFixedPageList::linkScanList(
                                                aHeader->mSpaceID,
                                                sCurPageID,
                                                &(aHeader->mFixed.mMRDB) )
                          != IDE_SUCCESS );
                sPrvPageID = sCurPageID;
            }
        }

        sCurPtr = sNxtPtr;
    }

    smpFreePageList::distributePagesFromFreePageList0ToTheOthers(
                                                  &(aHeader->mFixed.mMRDB) );

    IDE_TEST(smpFreePageList::distributePagesFromFreePageList0ToFreePagePool(
                                             aTrans,
                                             aHeader->mSpaceID,
                                             &(aHeader->mFixed.mMRDB) )
             != IDE_SUCCESS);


    if(sRecordCount != 0)
    {
        IDE_TEST(smmManager::initSCN() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaRefineDB::freeTableHdr( smxTrans       *aTrans,
                                  smcTableHeader *aTableHdr,
                                  ULong           aSlotFlag,
                                  SChar          *aNxtPtr )
{
    scPageID sPageID;

    IDE_DASSERT( aTableHdr != NULL );

    // Refine SKIP할 Tablespace인지 체크
    if ( sctTableSpaceMgr::hasState( aTableHdr->mSpaceID,
                                     SCT_SS_SKIP_REFINE ) == ID_TRUE )
    {
        // fix BUG-17784  아직 완전히 drop 되지않은 disk table header가
        // refine 중 free되어 재사용되는 경우 발생

        // DROP/OFFLINE/DISCARD된 Tablespace의 경우
        // Catalog Table Slot만 반납한다.
        IDE_TEST( freeCatalogSlot( aTrans,
                                   aNxtPtr ) != IDE_SUCCESS );
    }
    else
    {
        if( ( aSlotFlag & SMP_SLOT_DROP_MASK )
            == SMP_SLOT_DROP_TRUE )
        {
            IDE_TEST( smcTable::dropTablePending( NULL,
                                                  aTrans,
                                                  aTableHdr,
                                                  ID_FALSE )
                      != IDE_SUCCESS );

            IDE_TEST( smcTable::finLockAndRuntimeItem(aTableHdr)
                      != IDE_SUCCESS );
        }

        sPageID = SMP_SLOT_GET_PID(aNxtPtr);

        IDE_TEST(smpFixedPageList::setFreeSlot(aTrans,
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    sPageID,
                    aNxtPtr,
                    SMP_TABLE_NORMAL)
                != IDE_SUCCESS );

        smpFixedPageList::addFreeSlotToFreeSlotList(
                smpFreePageList::getFreePageHeader(
                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                    sPageID),
                aNxtPtr );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smaRefineDB::refineTable(smxTrans            * aTrans,
                                smapRebuildJobItem  * aJobItem)
{
    UInt    sTableType = SMN_GET_BASE_TABLE_TYPE(aJobItem->mTable->mFlag);
    UInt    sTableOID;
    smrRTOI sRTOI;

    sTableOID = aJobItem->mTable->mSelfOID;

    smrRecoveryMgr::initRTOI( &sRTOI );
    sRTOI.mCause    = SMR_RTOI_CAUSE_PROPERTY;
    sRTOI.mType     = SMR_RTOI_TYPE_TABLE;
    sRTOI.mTableOID = sTableOID;
    sRTOI.mState    = SMR_RTOI_STATE_CHECKED;

    if( smrRecoveryMgr::isIgnoreObjectByProperty( &sRTOI ) == ID_TRUE )
    {
        /* PROJ-2162 RestartRiskReduction 
         * Table이 Consistent할때에만 Refine을 수행함 */
        IDE_CALLBACK_SEND_SYM("S");
        IDE_TEST( smrRecoveryMgr::startupFailure( &sRTOI, 
                                                  ID_FALSE )  // is redo
                  != IDE_SUCCESS);
        IDE_CONT( SKIP );
    }
    else
    {
        /* nothing to do ... */
    }

    if( smcTable::isTableConsistent( (void*)aJobItem->mTable ) == ID_FALSE )
    {
        /* PROJ-2162 RestartRiskReduction 
         * Table이 Consistent할때에만 Refine을 수행함 */
        smrRecoveryMgr::initRTOI( &sRTOI );
        sRTOI.mCause    = SMR_RTOI_CAUSE_OBJECT;
        sRTOI.mType     = SMR_RTOI_TYPE_TABLE;
        sRTOI.mTableOID = sTableOID;
        sRTOI.mState    = SMR_RTOI_STATE_CHECKED;
        IDE_CALLBACK_SEND_SYM("F");
        IDE_TEST( smrRecoveryMgr::startupFailure( &sRTOI, 
                                                  ID_FALSE )  // is redo
                  != IDE_SUCCESS);
        IDE_CONT( SKIP );
    }
    else
    {
        /* nothing to do ... */
    }

    /* PROJ-2162 RestartRiskReduction
     * Refine 실패를 대비해, Refine하는 Table의 OID를 출력함 */
    ideLog::log( IDE_SM_0,
                 "====================================================\n"
                 " [MRDB_TBL_REFINE_BEGIN] TABLEOID : %llu\n"
                 "====================================================",
                 sTableOID );

    if( smpFixedPageList::refinePageList( aTrans,
                aJobItem->mTable->mSpaceID,
                sTableType,
                &(aJobItem->mTable->mFixed.mMRDB) )
            != IDE_SUCCESS)
    {
        IDE_TEST( smrRecoveryMgr::refineFailureWithTable( sTableOID ) 
                  != IDE_SUCCESS );
        IDE_CONT( SKIP );
    }
    else
    {
        /* nothing to do ... */
    }

    if(smpVarPageList::refinePageList( 
                aTrans,
                aJobItem->mTable->mSpaceID,
                aJobItem->mTable->mVar.mMRDB )
            != IDE_SUCCESS)
    {
        IDE_TEST( smrRecoveryMgr::refineFailureWithTable( sTableOID ) 
                  != IDE_SUCCESS );
        IDE_CONT( SKIP );
    }
    else
    {
        /* nothing to do ... */
    }

    ideLog::log( IDE_SM_0,
                 "====================================================\n"
                 " [MRDB_TBL_REFINE_END]   TABLEOID : %llu\n"
                 "====================================================",
                sTableOID );

    IDE_CALLBACK_SEND_SYM(".");

    IDE_EXCEPTION_CONT( SKIP );

    aJobItem->mFinished = ID_TRUE;

    aJobItem->mTable->mSequence.mCurSequence =
        aJobItem->mTable->mSequence.mLstSyncSequence;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description
 *    모든 volatile table들을 초기화한다.
 *    이 함수는 Server startup service 단계에서 호출되어야 한다.
 *    왜냐하면 makeNullRow callback 호출시 mt 모듈이 초기화되어 있어야
 *    하기 때문이다.
 ************************************************************************/
IDE_RC smaRefineDB::initAllVolatileTables()
{
    SChar          *sNxtTablePtr;
    SChar          *sCurTablePtr;
    smcTableHeader *sCurTable;
    smpSlotHeader  *sSlot;
    smSCN           sSCN;
    smxTrans       *sTrans;
    smSCN           sInfiniteSCN;
    SLong           sValueBuffer[SM_PAGE_SIZE/ID_SIZEOF(SLong)];
    smiColumnList   sColList[SMI_COLUMN_ID_MAXIMUM];
    smiValue        sNullRow[ID_SIZEOF(smiValue) * SMI_COLUMN_ID_MAXIMUM];
    smOID           sNullRowOID;
    smnIndexHeader *sRebuildIndexList[SMC_INDEX_TYPE_COUNT];


    SM_INIT_SCN( &sInfiniteSCN );

    IDE_TEST(smxTransMgr::alloc((smxTrans**)&sTrans) != IDE_SUCCESS);
    IDE_ASSERT( sTrans->begin(NULL,
                              ( SMI_TRANSACTION_REPL_NONE |
                                SMI_COMMIT_WRITE_NOWAIT ),
                              SMX_NOT_REPL_TX_ID )
             == IDE_SUCCESS);

    sCurTablePtr = NULL;

    while (ID_TRUE)
    {
        IDE_TEST(smcRecord::nextOIDall((smcTableHeader *)smmManager::m_catTableHeader,
                                       sCurTablePtr,
                                       &sNxtTablePtr)
                 != IDE_SUCCESS);

        if (sNxtTablePtr != NULL)
        {
            sSlot     = (smpSlotHeader *)sNxtTablePtr;
            sCurTable = (smcTableHeader *)(sSlot + 1);
            SM_GET_SCN( &sSCN, &(sSlot)->mCreateSCN );

            sCurTablePtr = sNxtTablePtr;

            if( SMI_TABLE_TYPE_IS_VOLATILE( sCurTable ) == ID_FALSE )
            {
                continue;
            }

            /* BUGBUG 아래 세 if 문들은 없어져야 한다.
               왜냐하면 이 함수가 호출되었을 시에는
               카탈로그 테이블이 refine 과정을 마쳤기때문에
               drop된 TBS나 Table들은 모두 처리가 되었다. */
            if ( sctTableSpaceMgr::hasState( sCurTable->mSpaceID,
                                             SCT_SS_SKIP_REFINE ) == ID_TRUE )
            {
                continue;
            }
            if( SMP_SLOT_IS_DROP( sSlot ) )
            {
                continue;
            }
            if( SM_SCN_IS_DELETED( sSCN ) )
            {
                continue;
            }

            /* Table header의 page list entry 초기화 */
            IDE_TEST( smcTable::initLockAndRuntimeItem( sCurTable )
                      != IDE_SUCCESS );

            /* NULL ROW 얻어오기 */
            IDE_TEST(smcTable::makeNullRowByCallback(sCurTable,
                                                     sColList,
                                                     sNullRow,
                                                     (SChar*)sValueBuffer)
                     != IDE_SUCCESS);

            /* 빈 테이블에 NULL ROW 삽입하기 */
            IDE_TEST(svcRecord::makeNullRow(sTrans,
                                            sCurTable,
                                            sInfiniteSCN,
                                            (const smiValue *)sNullRow,
                                            SM_FLAG_MAKE_NULLROW,
                                            &sNullRowOID)
                     != IDE_SUCCESS);

            IDE_TEST( smcTable::setNullRow( sTrans,
                                            sCurTable,
                                            SMI_TABLE_VOLATILE,
                                            &sNullRowOID )
                      != IDE_SUCCESS );

            /* Index 초기화 수행하기 */
            IDE_TEST(smnManager::prepareRebuildIndexs(sCurTable,
                                                      sRebuildIndexList)
                     != IDE_SUCCESS);
         }
         else
         {
             break;
         }
    }

    IDE_TEST(sTrans->commit() != IDE_SUCCESS);
    IDE_TEST(smxTransMgr::freeTrans(sTrans) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/************************************************************************
 * Description : BUG-24518 [MDB] Shutdown Phase에서 메모리 테이블
 *               Compaction이 필요합니다.
 *
 * 데이터베이스상의 모든 메모리/메타 테이블에 대해서 Compaction을 수행하여
 * 이후 server start시 VSZ가 증가하지 않도록 막는다.
 ************************************************************************/
IDE_RC smaRefineDB::compactTables( void )
{

    smpSlotHeader  * sSlotHeaderPtr;
    smcTableHeader * sTableHeader;
    smcTableHeader * sCatalogHeader;
    SChar          * sCurPtr;
    SChar          * sNxtPtr;
    smSCN            sSCN;
    smxTrans       * sTrans = NULL;


    IDE_TEST( smxTransMgr::alloc( (smxTrans**)&sTrans ) != IDE_SUCCESS );
    IDE_ASSERT( sTrans->begin( NULL,
                               ( SMI_TRANSACTION_REPL_NONE |
                                 SMI_COMMIT_WRITE_NOWAIT ),
                               SMX_NOT_REPL_TX_ID )
              == IDE_SUCCESS );

    sCatalogHeader = (smcTableHeader *)smmManager::m_catTableHeader;

    sCurPtr = NULL;

    /*
     * 모든 테이블에 대해서
     */
    while(1)
    {
        IDE_TEST( smcRecord::nextOIDall( sCatalogHeader,
                                         sCurPtr,
                                         & sNxtPtr )
                  != IDE_SUCCESS );

        if( sNxtPtr == NULL )
        {
            break;
        }

        sSlotHeaderPtr = (smpSlotHeader *)sNxtPtr;
        sSCN = sSlotHeaderPtr->mCreateSCN;
        sTableHeader = (smcTableHeader *)(sSlotHeaderPtr + 1);
        sCurPtr = sNxtPtr;

        /*
         * INCONSISTENT 테이블스페이스는 Skip
         */
        if ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                         SCT_SS_SKIP_REFINE ) == ID_TRUE )
        {
            continue;
        }

        /*
         * 사용자에 의해서 삭제된 테이블은 Skip
         */
        if( SMP_SLOT_IS_DROP( sSlotHeaderPtr ) )
        {
            continue;
        }

        /*
         * 테이블 생성하다가 롤백된 경우는 Skip
         */
        if( SM_SCN_IS_DELETED( sSCN ) )
        {
            continue;
        }

        /*
         * 메모리이거나 메타 테이블에 대해서만 Compaction을 수행한다.
         */
        if( (SMI_TABLE_TYPE_IS_MEMORY( sTableHeader ) == ID_TRUE) ||
            (SMI_TABLE_TYPE_IS_META( sTableHeader )   == ID_TRUE) )
        {
            IDE_TEST( smcTable::compact( sTrans, 
                                         sTableHeader, 
                                         0 )   /* # of page (0:ALL) */ 
                      != IDE_SUCCESS );
        }
    }

    IDE_TEST( sTrans->commit() != IDE_SUCCESS );
    IDE_TEST( smxTransMgr::freeTrans( sTrans )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sTrans != NULL)
    {
        sTrans->abort( ID_FALSE, /* aIsLegacyTrans */
                       NULL      /* aLegacyTrans */ );
        smxTransMgr::freeTrans( sTrans);
    }

    return IDE_FAILURE;
}

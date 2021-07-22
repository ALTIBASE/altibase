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
 * $Id: smcTableSpace.cpp 19201 2006-11-30 00:54:40Z kmkim $
 **********************************************************************/

#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smc.h>
#include <smu.h>
#include <smcReq.h>
#include <smcTableSpace.h>
#include <smpFixedPageList.h>
#include <smpVarPageList.h>

/*
    smcTableSpace.cpp

    Tablespace관련 코드중 catalog table에 접근이 필요한 코드
*/

smcTableSpace::smcTableSpace()
{

}

/*
    Tablespace안의 각각의 Table에 대해 특정 작업을 수행한다.

    [IN] aTBSID      - 검색할 Tablespace의 ID
                       SC_NULL_SPACEID를 넘기면 모든 Tablespace를 검색
    [IN] aActionFunc - 수행할 Action함수
    [IN] aActionArg  - Action함수에 전달할 인자
 */
IDE_RC smcTableSpace::run4TablesInTBS(
                                   idvSQL*           aStatistics,
                                   scSpaceID         aTBSID,
                                   smcAction4Table   aActionFunc,
                                   void            * aActionArg)
{

    IDE_DASSERT( aActionFunc     != NULL );

    /* Catalog Table에서 TBSID에 저장된 Table에 대해 Action 수행*/
    IDE_TEST( run4TablesInTBS( aStatistics,
                               SMC_CAT_TABLE,
                               aTBSID,
                               aActionFunc,
                               aActionArg ) != IDE_SUCCESS );

    /* Temp Catalog Table에서 TBSID에 저장된 Table에 대해 Action 수행*/
    IDE_TEST( run4TablesInTBS( aStatistics,
                               SMC_CAT_TEMPTABLE,
                               aTBSID,
                               aActionFunc,
                               aActionArg ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Catalog Table안의 특정 Tablespace에 저장된 Table에 대해
    Action함수를 수행한다.

    [IN] aCatTableHeader - Table을 검색할 Catalog Table 의 Header
    [IN] aTBSID      - 검색할 Tablespace의 ID
                       SC_NULL_SPACEID를 넘기면 모든 Tablespace를 검색
    [IN] aActionFunc - 수행할 Action함수
    [IN] aActionArg  - Action함수에 전달할 인자
 */
IDE_RC smcTableSpace::run4TablesInTBS(
                               idvSQL*           aStatistics,
                               smcTableHeader  * aCatTableHeader,
                               scSpaceID         aTBSID,
                               smcAction4Table   aActionFunc,
                               void            * aActionArg)
{
    smcTableHeader    * sCatTblHdr;
    smcTableHeader    * sTableHdr;
    smpSlotHeader     * sSlotHdr;
    SChar             * sCurPtr;
    SChar             * sNxtPtr;
    smSCN               sSlotSCN;

    IDE_DASSERT( aCatTableHeader != NULL);
    IDE_DASSERT( aActionFunc     != NULL );

    sCatTblHdr = (smcTableHeader*)aCatTableHeader;
    sCurPtr = NULL;

    while(1)
    {
        /* 다음 Record를 Fetch: if sCurPtr == NULL, fetch first record,
           else fetch next record.*/
        IDE_TEST( smcRecord::nextOIDall( sCatTblHdr, sCurPtr, &sNxtPtr )
                  != IDE_SUCCESS );

        if( sNxtPtr == NULL )
        {
            break;
        }

        sSlotHdr = (smpSlotHeader *)sNxtPtr;
        SM_GET_SCN( &sSlotSCN, &(sSlotHdr->mCreateSCN) );

        sTableHdr = (smcTableHeader *)( sSlotHdr + 1 );

        if ( aTBSID == SC_NULL_SPACEID ||  // 모든 Tablespace 검색
             aTBSID == sTableHdr->mSpaceID ) // 특정 Tablespace 검색
        {
            // Action함수 호출
            IDE_TEST( (*aActionFunc)( aStatistics,
                                      SMP_SLOT_GET_FLAGS( sSlotHdr ),
                                      sSlotSCN,
                                      sTableHdr,
                                      aActionArg )
                      != IDE_SUCCESS );
        }

        sCurPtr = sNxtPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
    Online->Offline으로 변하는 Tablespace에 속한 Table들에 대한 처리

    Refine시에 Offline된 Tablespace안의 Table에 대해 호출한다.

    [IN] aTBSID - Online->Online으로 변하는 Tablespace의 ID
 */
IDE_RC smcTableSpace::alterTBSOffline4Tables( idvSQL*     aStatistics,
                                              scSpaceID   aTBSID )
{
    sctTableSpaceNode * sSpaceNode;
    ULong               sMaxSmoNo = 0;

    // BUG-24403
    IDE_ASSERT( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTBSID,
                                                          (void**)&sSpaceNode )
                == IDE_SUCCESS );


    // BUG-24403
    /* BUG-27714 TC/Server/sm4/Project2/PRJ-1548/dynmem/../suites/conc/dt_dml.
     * sql에서 서버 비정상 종료
     * MaxSmoNoForOffline 변수가 있는 곳은 Disk TableSBS뿐이며, DiskTBS일
     * 경우에만 이 값을 사용합니다. */
    if( sctTableSpaceMgr::isDiskTableSpace( aTBSID ) == ID_TRUE )
    {
        IDE_TEST ( run4TablesInTBS( aStatistics,
                                    aTBSID,
                                    alterTBSOfflineAction,
                                    (void*)&sMaxSmoNo )
                   != IDE_SUCCESS );

        ((sddTableSpaceNode*)sSpaceNode)->mMaxSmoNoForOffline = sMaxSmoNo;
    }
    else
    {
        //Memory TBS
        IDE_ERROR( sctTableSpaceMgr::isMemTableSpace( aTBSID ) == ID_TRUE );

        IDE_TEST ( run4TablesInTBS( aStatistics,
                                    aTBSID,
                                    alterTBSOfflineAction,
                                    NULL )
                   != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Offline->Online으로 변하는 Tablespace에 속한 Table들에 대한 처리

    Refine시에 Offline된 Tablespace안의 Table에 대해 호출한다.

    [IN] aTrans - Refine을 수행할 Transaction
    [IN] aTBSID - Offline->Online으로 변하는 Tablespace의 ID
 */
IDE_RC smcTableSpace::alterTBSOnline4Tables(idvSQL*      aStatistics,
                                            void       * aTrans,
                                            scSpaceID    aTBSID )
{
    sddTableSpaceNode      * sSpaceNode;
    smcTBSOnlineActionArgs   sTBSOnlineActionArg;

    sTBSOnlineActionArg.mTrans = aTrans;
    /* BUG-27714 TC/Server/sm4/Project2/PRJ-1548/dynmem/../suites/conc/dt_dml.
     * sql에서 서버 비정상 종료
     * MaxSmoNoForOffline 변수가 있는 곳은 Disk TableSBS뿐이며, DiskTBS일
     * 경우에만 이 값을 사용합니다. */
    if( sctTableSpaceMgr::isDiskTableSpace( aTBSID ) == ID_TRUE )
    {
        // BUG-24403
        IDE_ASSERT( sctTableSpaceMgr::findSpaceNodeBySpaceID( aTBSID,
                                                              (void**)&sSpaceNode )
                    == IDE_SUCCESS );

        sTBSOnlineActionArg.mMaxSmoNo = sSpaceNode->mMaxSmoNoForOffline;
    }

    IDE_TEST ( run4TablesInTBS( aStatistics,
                                aTBSID,
                                alterTBSOnlineAction,
                                (void*)&sTBSOnlineActionArg )
               != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    Online->Offline으로 변하는 Tablespace에 속한 Table에 대한 Action함수

    [IN] aTableHeader - Offline으로 변하는 Tablespace에 속한 Table의 Header
    [IN] aActionArg   - Action함수 인자
 */
IDE_RC smcTableSpace::alterTBSOfflineAction(
                           idvSQL         * /* aStatistics */,
                           ULong            aSlotFlag,
                           smSCN            aSlotSCN,
                           smcTableHeader * aTableHeader,
                           void           * aActionArg )
{
    UInt             sTableType;

    IDE_DASSERT( aTableHeader != NULL );

    sTableType = SMI_GET_TABLE_TYPE( aTableHeader );

    if ( sTableType == SMI_TABLE_DISK )
    {
        IDE_TEST( alterDiskTBSOfflineAction( aSlotFlag,
                                             aSlotSCN,
                                             aTableHeader,
                                             aActionArg )
                  != IDE_SUCCESS );
    }
    else
    {
        if (sTableType == SMI_TABLE_MEMORY)
        {
            IDE_TEST( alterMemTBSOfflineAction( aTableHeader )
                      != IDE_SUCCESS );
        }

        // Temp Table의 경우 아무 처리 안함.
        // 이유 : Alter Tablespace Online/Offline 불가능
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    Offline->Online 으로 변하는 Tablespace에 속한 Table에 대한 Action함수

    [IN] aTableHeader - Online으로 변하는 Tablespace에 속한 Table의 Header
    [IN] aActionArg   - Action함수 인자
 */
IDE_RC smcTableSpace::alterTBSOnlineAction(
                           idvSQL*          aStatistics,
                           ULong            aSlotFlag,
                           smSCN            aSlotSCN,
                           smcTableHeader * aTableHeader,
                           void           * aActionArg )
{
    UInt             sTableType;

    IDE_DASSERT( aTableHeader != NULL );

    sTableType = SMI_GET_TABLE_TYPE( aTableHeader );

    if( sTableType == SMI_TABLE_DISK )
    {
        IDE_TEST( alterDiskTBSOnlineAction( aStatistics,
                                            aSlotFlag,
                                            aSlotSCN,
                                            aTableHeader,
                                            aActionArg )
                  != IDE_SUCCESS );
    }
    else
    {
        if (sTableType == SMI_TABLE_MEMORY)
        {
            IDE_TEST( alterMemTBSOnlineAction( aSlotFlag,
                                               aSlotSCN,
                                               aTableHeader,
                                               aActionArg )
                      != IDE_SUCCESS );
        }


        // Temp Table의 경우 아무 처리 안함.
        // 이유 : Alter Tablespace Online/Offline 불가능
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}



/*
    Offline->Online 으로 변하는 Disk Tablespace에 속한
    Table에 대한 Action함수

    [ 알고리즘 ]
    Alloc/Init Runtime Info At Table Header
    Rebuild IndexRuntime Header

    본 함수는 Alter Tablespace Online에서 호출된다.

    [IN] aTableHeader - Online으로 변하는 Tablespace에 속한 Table의 Header
    [IN] aActionArg   - Action함수 인자
 */
IDE_RC smcTableSpace::alterDiskTBSOnlineAction( idvSQL         * aStatistics,
                                                ULong            aSlotFlag,
                                                smSCN            aSlotSCN,
                                                smcTableHeader * aTableHeader,
                                                void           * aActionArg )
{
    ULong                    sMaxSmoNo;
    idBool                   sIsCheckIdxIntegrity = ID_FALSE;
    smcTBSOnlineActionArgs * sTBSOnlineActionArg;

    IDE_DASSERT( aTableHeader != NULL );
    IDE_DASSERT( aActionArg   != NULL );

    /* To fix CASE-6829 디스크 테이블스페이스가 온라인으로 전이할때에는
     * 인덱스 Integrity Level1이나 Level2가 차이가 없으므로
     * 프로퍼티가 비활성화만 아니면 Integrity 체크를 수행한다. */
    sTBSOnlineActionArg = (smcTBSOnlineActionArgs*)aActionArg;
    sMaxSmoNo = sTBSOnlineActionArg->mMaxSmoNo;

    // To fix CASE-6829
    if( smuProperty::getCheckDiskIndexIntegrity()
        != SMU_CHECK_DSKINDEX_INTEGRITY_DISABLED )
    {
        sIsCheckIdxIntegrity = ID_TRUE;
    }

    if( sIsCheckIdxIntegrity == ID_TRUE )
    {
        IDE_CALLBACK_SEND_SYM( "  [SM] [BEGIN : CHECK DISK INDEX INTEGRITY]\n" );
    }

    // BUGBUG-1548 Refine쪽 코드를 그대로 가져왔음.
    // Ager가 동작해도 문제없는지 검증필요

    /*
       1. normal table
       DROP_FALSE
       2. droped table
       DROP_TRUE
       3. create table by abort ( NTA로그까지 찍었지만 Abort하여 Logical Undo )
       DROP_TRUE deletebit
       4. create table by abort ( allocslot 까지 되고 NTA 로그 못찍고 Physical Undo)
       DROP_FALSE deletebit

       case 1,2,3  -> initLockAndRuntimeItem
       -  1번  => 사용중인 Table이므로 초기화해야함
       -  2번, => catalog table시에 drop Table pending이 호출됨
       3번    ( drop table pending수행을 위해서 초기화 필요)
       case 4      -> skip
       -  4번  => catalog table refine시 catalog table row만 지워짐
    */

    if (!(( ( aSlotFlag & SMP_SLOT_DROP_MASK )
            == SMP_SLOT_DROP_FALSE ) &&
          ( SM_SCN_IS_DELETED( aSlotSCN ) )))
    {
        // Offline Tablespace안의 Table에 대해
        // Table Header의 mLock은 초기화된 채로 유지된다.
        IDE_ASSERT( aTableHeader->mLock != NULL );

        ///////////////////////////////////////////////////////////
        // (010) Init Runtime Info At Table Header
        //        - Table의 Runtime정보 초기화 실시
        IDE_TEST( smcTable::initRuntimeItem( aTableHeader )
                  != IDE_SUCCESS );
    }

    if( ( aSlotFlag & SMP_SLOT_DROP_MASK )
        == SMP_SLOT_DROP_TRUE )
    {
        // 2번, 3번의 경우
        //  : 이미 Drop된 Table이거나 Create도중 Abort된 Table
        //  => 아무런 처리 안함
    }
    else
    {
        if( SM_SCN_IS_DELETED( aSlotSCN ) )
        {
            // 4번의 경우 ->  Create도중 Abort된 Table
            //  => 아무런 처리 안함
        }
        else
        {
            // fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline
            // 수행시 올바르게 Index Runtime Header 해제하지 않음
            // TBS에 X  잠금이 획득된 상태이며, X 잠금획득시에
            // Dropped/Discared 상태의 TBS에 대해서 Validation을 하기
            // 때문에 online을 진행할 수 없다.
            // 또한, TBS에 X 잠금을 획득하고 있기 때문에 online 하는
            // 중에 테이블스페이스가 dropped이 되는 경우가 없다.

            // Rebuild All Index Runtime Header
            // Online할 TableSpace를 기준으로 모든 Index Runtime Header를
            // rebuild 한다.
            // Offline시에는 반대로 하였다.

            /* fix BUG-17456
             * Disk Tablespace online이후 update 발생시 index 무한루프
             * Online시 rebuild된 DRDB Index Header의 SmoNo를 Buffer Pool에
             * 존재하는 Index Page들의 SmoNo중 가장 큰 값으로 rebuild 한다. */
            IDE_TEST( smcTable::rebuildRuntimeIndexHeaders(
                                       aStatistics,
                                       aTableHeader,
                                       sMaxSmoNo ) != IDE_SUCCESS );

            if ( sIsCheckIdxIntegrity == ID_TRUE )
            {
                IDE_TEST( smcTable::verifyIndexIntegrity(
                                    aStatistics,
                                    aTableHeader,
                                    NULL /* aActionArgs */ ) != IDE_SUCCESS );
            }

            /* PROJ-1671 LOB Segment에 대한 Segment Handle을 생성하고, 초기화한다.*/
            IDE_TEST( smcTable::createLOBSegmentDesc( aTableHeader )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Online->Offline 으로 변하는 Disk Tablespace에 속한
    Table에 대한 Action함수

    [IN] aTableHeader - Online으로 변하는 Tablespace에 속한 Table의 Header
    [IN] aActionArg   - Action함수 인자

    [ 알고리즘 ]
    Free All IndexRuntime Header Disk of TBS
    Free Runtime Item At Table Header

    [ 참고 ]
     1. 본 함수는 Alter Tablespace Offline의 Commit Pending에서 호출된다
     2. Table의 Lock Item을 해제할 수 없는 이유 :
        offline 된 TBS 를 Drop 할 수 있기 때문에 관련 Table의 X 잠금을
        획득하여야 하기 때문이다.
 */
IDE_RC smcTableSpace::alterDiskTBSOfflineAction(
                          ULong            aSlotFlag,
                          smSCN            aSlotSCN,
                          smcTableHeader * aTableHeader,
                          void           * aActionArg )
{
    IDE_DASSERT( aTableHeader != NULL );

    /*
       1. normal table
       DROP_FALSE
       2. droped table
       DROP_TRUE
       3. create table by abort ( NTA로그까지 찍었지만 Abort하여 Logical Undo )
       DROP_TRUE deletebit
       4. create table by abort ( allocslot 까지 되고 NTA 로그 못찍고 Physical Undo)
       DROP_FALSE deletebit

       case 1,2,3  -> initLockAndRuntimeItem
       -  1번  => 사용중인 Table이므로 초기화해야함
       -  2번, => catalog table시에 drop Table pending이 호출됨
       3번    ( drop table pending수행을 위해서 초기화 필요)
       case 4      -> skip
       -  4번  => catalog table refine시 catalog table row만 지워짐
    */

    if( ( aSlotFlag & SMP_SLOT_DROP_MASK )
        == SMP_SLOT_DROP_TRUE )
    {
        // 2번, 3번의 경우
        //  : 이미 Drop된 Table이거나 Create도중 Abort된 Table
        //  => 아무런 처리 안함
    }
    else
    {
        if( SM_SCN_IS_DELETED( aSlotSCN ) )
        {
            // 4번의 경우 ->  Create도중 Abort된 Table
            //  => 아무런 처리 안함
        }
        else
        {
            // fix BUG-17157 [PROJ-1548] Disk Tablespace Online/Offline
            // 수행시 올바르게 Index Runtime Header 해제하지 않음
            // TBS에 X  잠금이 획득된 상태이며, X 잠금획득시에
            // Dropped/Discared 상태의 TBS에 대해서 Validation을 하기
            // 때문에 offline을 진행할 수 없다.
            // 또한, TBS에 X 잠금을 획득하고 있기 때문에 offline 하는
            // 중에 테이블스페이스가 dropped이 되는 경우가 없다.

            // 1 번의 경우
            // (010) Free All IndexRuntime Header Disk of TBS
            // Offline TBS에 포함된 테이블을 기준으로
            // 모든 Index Runtime Header를 Free 한다.

            /* PROJ-1671 LOB Segment에 대한 Segment Handle을 해제한다. */
            IDE_TEST( smcTable::destroyLOBSegmentDesc( aTableHeader )
                      != IDE_SUCCESS );

            // BUG-24403
            smLayerCallback::getMaxSmoNoOfAllIndexes( aTableHeader,
                                                      (ULong*)aActionArg );

            IDE_TEST( smLayerCallback::dropIndexes( aTableHeader )
                      != IDE_SUCCESS );

            ///////////////////////////////////////////////////////
            // (020) Free Runtime Item  At Table Header
            /* Table의 Mutex와 Runtime영역을 해제 */
            IDE_TEST( smcTable::finRuntimeItem( aTableHeader )
                    != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*
    Offline->Online 으로 변하는 Memory Tablespace에 속한
    Table에 대한 Action함수

    [IN] aTableHeader - Online으로 변하는 Tablespace에 속한 Table의 Header
    [IN] aActionArg   - Action함수 인자

    [ 알고리즘 ]
     (010) Alloc/Init Runtime Info At Table Header
            - Mutex 객체, Free Page관련 정보 초기화 실시
     (020) Refine Table Pages
            - 모든 Page에 대해 Free Slot을 찾아서 각 Page에 달아준다.
            - 테이블 헤더의 Runtime정보에 Free Page들을 구축해준다.
     (030) Rebuild Indexes

    [ 참고 ]
     본 함수는 Alter Tablespace Offline에서 호출된다.

    [ 참고 ]
     Table의 Lock Item은 이미 초기화 되어 있는 상태이다.
        - Alter TBS Offline이후 Server 재기동 된 경우
          => Refine과정에서 Lock Item을 별도 초기화 되었다.
        - Alter TBS Offline이후 Server 재기동 되지 않은 경우
          => Alter TBS Offline시에는 Lock Item은 해제하지 않는다.

 */
IDE_RC smcTableSpace::alterMemTBSOnlineAction( ULong            aSlotFlag,
                                               smSCN            aSlotSCN,
                                               smcTableHeader * aTableHeader,
                                               void           * aActionArg )
{
    UInt                     sStage = 0;
    iduPtrList               sOIDList;
    void                   * sTrans;
    smcTBSOnlineActionArgs * sTBSOnlineActionArg;

    IDE_DASSERT( aTableHeader != NULL );

    sTBSOnlineActionArg = (smcTBSOnlineActionArgs*)aActionArg;
    sTrans = sTBSOnlineActionArg->mTrans;

    // BUGBUG-1548 Refine쪽 코드를 그대로 가져왔음.
    // Ager가 동작해도 문제없는지 검증필요

    /*
       1. normal table
       DROP_FALSE
       2. droped table
       DROP_TRUE
       3. create table by abort ( NTA로그까지 찍었지만 Abort하여 Logical Undo )
       DROP_TRUE deletebit
       4. create table by abort ( allocslot 까지 되고 NTA 로그 못찍고 Physical Undo)
       DROP_FALSE deletebit

       case 1,2,3  -> initLockAndRuntimeItem
       -  1번  => 사용중인 Table이므로 초기화해야함
       -  2번, => catalog table시에 drop Table pending이 호출됨
       3번    ( drop table pending수행을 위해서 초기화 필요)
       case 4      -> skip
       -  4번  => catalog table refine시 catalog table row만 지워짐
    */

    if (!(( ( aSlotFlag & SMP_SLOT_DROP_MASK )
            == SMP_SLOT_DROP_FALSE ) &&
          ( SM_SCN_IS_DELETED( aSlotSCN ) )))
    {
        // Offline Tablespace안의 Table에 대해
        // Table Header의 mLock은 초기화된 채로 유지된다.
        IDE_ASSERT( aTableHeader->mLock != NULL );

        ///////////////////////////////////////////////////////////
        // (010) Init Runtime Info At Table Header
        //        - Table의 Runtime정보 초기화 실시
        IDE_TEST( smcTable::initRuntimeItem( aTableHeader )
                  != IDE_SUCCESS );
    }

    if( ( aSlotFlag & SMP_SLOT_DROP_MASK )
        == SMP_SLOT_DROP_TRUE )
    {
        // 2번, 3번의 경우
        //  : 이미 Drop된 Table이거나 Create도중 Abort된 Table
        //  => 아무런 처리 안함
    }
    else
    {
        if( SM_SCN_IS_DELETED( aSlotSCN ) )
        {
            // 4번의 경우 ->  Create도중 Abort된 Table
            //  => 아무런 처리 안함
        }
        else
        {
            // BUGBUG-1548 이 부분 코드 상세 검증 필요. 많이 불안함

            /////////////////////////////////////////////////////////////
            // (020) Refine Table Pages
            //  - 모든 Page에 대해 Free Slot을 찾아서 각 Page에 달아준다.
            //  - 테이블 헤더의 Runtime정보에 Free Page들을 구축해준다.

            IDE_TEST( sOIDList.initialize(IDU_MEM_SM_SMM ) != IDE_SUCCESS );
            sStage = 1;


            // refinePageList에서 sOIDList를 필요로 하기때문에 넘긴다
            // 그러나, 여기에서 sOIDList를 사용하지는 않는다.
            // aTableType 인자를 0으로 넘기는데, 그 이유는
            // aTableType이 sOIDList에 OID를 추가할지의 여부 사용되기 때문
            IDE_TEST(smpFixedPageList::refinePageList(
                                           sTrans,
                                           aTableHeader->mSpaceID,
                                           0, /* aTableType */
                                           & (aTableHeader->mFixed.mMRDB) )
             != IDE_SUCCESS);

            sStage = 0;
            IDE_TEST( sOIDList.destroy() != IDE_SUCCESS );

            IDE_TEST(smpVarPageList::refinePageList( sTrans,
                                                     aTableHeader->mSpaceID,
                                                     aTableHeader->mVar.mMRDB )
                     != IDE_SUCCESS);


            ////////////////////////////////////////////////////////////
            // (030) Rebuild Indexes
            IDE_TEST( smLayerCallback::createIndexes( NULL,    /* idvSQL* */
                                                      sTrans,
                                                      aTableHeader,
                                                      ID_FALSE,/* aIsRestartRebuild */
                                                      ID_FALSE /* aIsNeedValidation */,
                                                      NULL,    /* Segment Attr */
                                                      NULL )   /* Storage Attr */
                      != IDE_SUCCESS );

        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sStage )
    {
        case 1:
            IDE_ASSERT( sOIDList.destroy() == IDE_SUCCESS );
            break;
        default:
            break;
    }
    IDE_POP();


    return IDE_FAILURE;
}


/*
    Online->Offline 으로 변하는 Memory Tablespace에 속한
    Table에 대한 Action함수

    [IN] aTableHeader - Online으로 변하는 Tablespace에 속한 Table의 Header
    [IN] aActionArg   - Action함수 인자

    [ 알고리즘 ]
     (010) Free All Index Memory of TBS ( 공유메모리인 경우 모두 제거 )
     (020) Free Runtime Item At Table Header

    [ 참고 ]
     1. 본 함수는 Alter Tablespace Offline의 Commit Pending에서 호출된다
     2. Table의 Lock Item을 해제할 수 있는 이유 :
          Tablespace에 X락을 잡은채로 이 함수가 호출되기 때문에,
          Table의 Lock Item에 접근중인 Transaction이 있을 수 없기 때문
          ( Tablespace에 먼저 IX, IS락을 잡고 Table에 X, S, IX, IS를 잡는다 )

 */
IDE_RC smcTableSpace::alterMemTBSOfflineAction( smcTableHeader * aTableHeader )
{
    IDE_DASSERT( aTableHeader != NULL );

    // BUGBUG-1548 drop된 Table, Create도중 실패한 Table에 대한 고려 필요

    ////////////////////////////////////////////////////////////////////////
    // (010) Free All Index Memory of TBS
    IDE_TEST( smLayerCallback::dropIndexes( aTableHeader )
              != IDE_SUCCESS );

    ////////////////////////////////////////////////////////////////////////
    // (020)  Free Runtime Item At Table Header
    /* Table의 Lock Item과 Runtime영역을 해제 */
    IDE_TEST( smcTable::finRuntimeItem( aTableHeader ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

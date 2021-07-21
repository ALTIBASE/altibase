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
 * $Id: smcCatalogTable.cpp 90259 2021-03-19 01:22:22Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smm.h>
#include <smp.h>
#include <smcDef.h>
#include <smcCatalogTable.h>
#include <smcRecord.h>
#include <smi.h>
#include <smcReq.h>

UInt  smcCatalogTable::getCatTempTableOffset()
{

    return SMC_CAT_TEMPTABLE_OFFSET;

}

/***********************************************************************
 * Description : DB 생성시에 Catalog Table과 Temp Catalog Table 생성
 *
 **********************************************************************/
IDE_RC smcCatalogTable::createCatalogTable()
{
    IDE_TEST( createCatalog(SMC_CAT_TABLE, SMM_CAT_TABLE_OFFSET)
              != IDE_SUCCESS );

    IDE_TEST( createCatalog(SMC_CAT_TEMPTABLE, SMC_CAT_TEMPTABLE_OFFSET)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Server shutdown시 Catalog Table에 대해 메모리 해제 수행
 *
 * Catalog Table과 Temp Catalog Table의 Record인 Temp Table
 * Header의 Lock Item과 RunTime 영역을 해제한다.
 **********************************************************************/
IDE_RC smcCatalogTable::finalizeCatalogTable()
{
    /* Catalog Table의 Record들의 Lock Item과 RunTime영역 해제*/
    IDE_TEST( finCatalog( SMC_CAT_TABLE ) != IDE_SUCCESS );
    /* Temp Catalog Table의 Record들의 Lock Item과 RunTime영역 해제*/
    IDE_TEST( finCatalog( SMC_CAT_TEMPTABLE ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 인터페이스를 맞추기 위한 Dummy함수
 **********************************************************************/
IDE_RC smcCatalogTable::initialize()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : 인터페이스를 맞추기 위한 Dummy함수
 **********************************************************************/
IDE_RC smcCatalogTable::destroy()
{
    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Shutdown시 Catalog Table및 그안의
 *               모든 Table의 Runtime정보 해제
 *
 * aCatTableHeader - [IN] Catalog Table Header (Normal or Temp)
 *
 **********************************************************************/
IDE_RC smcCatalogTable::finCatalog( void* aCatTableHeader )
{
    IDE_ASSERT(aCatTableHeader != NULL);

    smcTableHeader * sCatTblHdr;

    sCatTblHdr = (smcTableHeader*) aCatTableHeader;

    /* Catalog Table안의 Used Slot들에 대해 Runtime정보 해제 */
    IDE_TEST( finAllocedTableSlots( sCatTblHdr )
              != IDE_SUCCESS );

    /* aCatTableHeader의 Lock Item과 Runtime영역을 해제 */
    IDE_TEST( smcTable::finLockAndRuntimeItem( sCatTblHdr )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : Catalog Table을 생성하기 위해 Catalog Tabler Header를 초기
 *               화한다.
 *
 * aCatTableHeader - [IN] Catalog Table Header Ponter
 * aOffset         - [IN] Offset: Catalog Table Header는 0번째 Page에 위치하는
 *                        데 이 Offset은 그 0번 페이지의 시작위치에서의 offset이다.
 **********************************************************************/
IDE_RC smcCatalogTable::createCatalog( void*   aCatTableHeader,
                                       UShort  aOffset )
{
    smcTableHeader  * sCatTblHdr;
    smpSlotHeader   * sSlotHeader;
    UInt i;


    /* Catalog Table을 DB 생성시에 초기화한다. */
    sCatTblHdr = (smcTableHeader*)aCatTableHeader;

    /* 카탈로그 테이블이 Page경계를 벗어나지는 않는지 체크 */
    IDE_ASSERT( aOffset + ID_SIZEOF(smpSlotHeader) + ID_SIZEOF(smcTableHeader)
                <= SM_PAGE_SIZE );
    /* aCatTableHeader의 Slot Header를 초기화 한다.*/
    idlOS::memset((UChar *)sCatTblHdr - SMP_SLOT_HEADER_SIZE, 0,
                  ID_SIZEOF(smcTableHeader) + SMP_SLOT_HEADER_SIZE);

    /* m_self of record header contains offset from page header */
    sSlotHeader = (smpSlotHeader *)((UChar *)sCatTblHdr - SMP_SLOT_HEADER_SIZE);
    SMP_SLOT_SET_OFFSET( sSlotHeader, aOffset );

    for(i = 0 ; i < SMC_MAX_INDEX_OID_CNT; i++)
    {
        sCatTblHdr->mIndexes[i].length = 0;
        sCatTblHdr->mIndexes[i].fstPieceOID = SM_NULL_OID;
        sCatTblHdr->mIndexes[i].flag = SM_VCDESC_MODE_OUT;
    }

    /* etc members in catalog table header */
    sCatTblHdr->mType          = SMC_TABLE_CATALOG;
    sCatTblHdr->mSelfOID       = (SM_NULL_PID | (aOffset));
    sCatTblHdr->mFlag          = SMI_TABLE_REPLICATION_DISABLE
        | SMI_TABLE_LOCK_ESCALATION_DISABLE | SMI_TABLE_META ;

    /* Catalog Table에 대한 칼럼 정보를 variable slot영역에 할당한다.*/
    sCatTblHdr->mColumnSize      = 0;
    sCatTblHdr->mColumnCount     = 0;
    sCatTblHdr->mColumns.length  = 0;
    sCatTblHdr->mColumns.fstPieceOID = SM_NULL_OID;
    sCatTblHdr->mMaxRow              = ID_ULONG_MAX;

    sCatTblHdr->mNullOID = SM_NULL_OID;

    /* Catalog Table의 Page List를 초기화 한다. */
    smpFixedPageList::initializePageListEntry(
                       &sCatTblHdr->mFixed.mMRDB,
                       sCatTblHdr->mSelfOID,
                       idlOS::align8( (UInt)( ID_SIZEOF(smcTableHeader) + SMP_SLOT_HEADER_SIZE) ) );

    smpVarPageList::initializePageListEntry( sCatTblHdr->mVar.mMRDB,
                                             sCatTblHdr->mSelfOID);

    smpAllocPageList::initializePageListEntry( sCatTblHdr->mFixedAllocList.mMRDB );
    smpAllocPageList::initializePageListEntry( sCatTblHdr->mVarAllocList.mMRDB );

    IDE_TEST(smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                           SMM_MEMBASE_PAGEID)
             != IDE_SUCCESS);

    IDE_TEST( smcTable::initLockAndRuntimeItem( sCatTblHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    IDE_ASSERT(smmDirtyPageMgr::insDirtyPage(SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                             SMM_MEMBASE_PAGEID)
               == IDE_SUCCESS);
    IDE_POP();

    return IDE_FAILURE;
}

/**
 *  Catalog Table안의 Used Slot에 대해 Lock Item과 Runtime Item해제
 *
 *  대상 테이블 :
 *    Drop되지 않은 Table이거나, Create Table도중 Abort하여
 *    Drop Flag가 세팅된 테이블
 *
 *  aCatTableHeader가 가리키는 Table의 Record들은 Table Header들이다.
 *  이 Table Header에는 Server Start시 할당되는 Lock Item과 RunTime영
 *  역이 있다. 때문에 Server Stop시 이 영역을 Free하기 위해
 *  aCatTableHeader의 모든 Create된 Table에 대해서
 *  smcTable::finLockAndRuntimeItem을 수행한다.
 *
 * aCatTableHeader - [IN] Catalog Table Header (Normal or Temp)
 */
IDE_RC smcCatalogTable::finAllocedTableSlots( smcTableHeader * aCatTblHdr )
{
    IDE_ASSERT(aCatTblHdr != NULL);

    smcTableHeader * sHeader;
    smpSlotHeader  * sPtr;
    SChar          * sCurPtr;
    SChar          * sNxtPtr;
    smSCN            sSCN;

    sCurPtr = NULL;

    while(1)
    {
        /* 다음 Record를 Fetch: if sCurPtr == NULL, fetch first record,
           else fetch next record.*/
        IDE_TEST( smcRecord::nextOIDall( aCatTblHdr, sCurPtr, &sNxtPtr )
                  != IDE_SUCCESS );

        if( sNxtPtr == NULL )
        {
            break;
        }
        sPtr = (smpSlotHeader *)sNxtPtr;
        SM_GET_SCN( &sSCN, &(sPtr->mCreateSCN) );

        sHeader = (smcTableHeader *)( sPtr + 1 );

        /* BUG-15653: server stop시 mutex leak이 많이 발생함.
           메모리 테이블이 Drop Pending시에 finLockAndRuntimeItem을 호출하지 않았음.
           디스크와 동일하게 DB가 내려갈때 체크해서 Lock, Mutex를 Free해줌*/
        if( SM_SCN_IS_NOT_DELETED( sSCN ) ||
            SMP_SLOT_IS_DROP( sPtr ) )
        {
            /* Disk LOB Column에 대한 Segment Handle 해제한다. */
            if( SMI_TABLE_TYPE_IS_DISK( sHeader ) == ID_TRUE )
            {
                IDE_TEST( smcTable::destroyLOBSegmentDesc( sHeader )
                          != IDE_SUCCESS );

                IDE_TEST( smcTable::destroyRowTemplate( sHeader )!= IDE_SUCCESS );
            }

            /* Table의 Lock Item과 Runtime영역을 해제 */
            IDE_TEST( smcTable::finLockAndRuntimeItem( sHeader )
                      != IDE_SUCCESS );

        }

        sCurPtr = sNxtPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 * Description : DRDB(Disk Resident Database)는 Restart Recovery의 Undo시에
 *               Index Header에 접근한다. 때문에 Redo후 MMDB에 있는 Catalog
 *               Table의 Record들 중 Disk Table의 Header에 대해 Lock Item과
 *               Runtime영역을 초기화하고 Index들에 대해 연산을 수행할 수 있도록
 *               적절한 연산을 수행해야 한다.
 *
 *
 **********************************************************************/
IDE_RC smcCatalogTable::refineDRDBTables()
{
    idBool    sInitLockAndRuntimeItem = ID_TRUE;

    /* A. 디스크 인덱스의 런타임 헤더와 디스크 테이블의 LOB 세그먼트의
     *    기술자를 초기화한다. */
    IDE_TEST( doAction4EachTBL( NULL, /* aStatistics */
                                smcTable::initRuntimeInfos,
                                (void*)&sInitLockAndRuntimeItem )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/***********************************************************************
 *
 * Description : 각각의 Table에 대해 특정 Action을 수행한다.
 *
 *   aStatistics - [IN] 통계정보
 *   aAction     - [IN] 수행할 Action함수
 *   aActionArg  - [IN] Action함수에 보낼 Argument
 *
 **********************************************************************/
IDE_RC smcCatalogTable::doAction4EachTBL(idvSQL            * aStatistics,
                                         smcAction4TBL       aAction,
                                         void              * aActionArg )
{
    smcTableHeader * sCatTblHdr;
    smcTableHeader * sHeader;
    smpSlotHeader  * sPtr;
    SChar          * sCurPtr;
    SChar          * sNxtPtr;
    smSCN            sSCN;

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr    = NULL;

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
        sPtr = (smpSlotHeader *)sNxtPtr;
        SM_GET_SCN( &sSCN, &(sPtr->mCreateSCN) );

/*
        BUG-13936

        Table의 생성 도중에 실패한 경우 다음과 같은 처리를 통해 Slot이 해제된다.

        Create Table
          Catalog Table에서 Row할당
             ==> Drop=0(Not Dropped), Used='U'(Used), Delete=0
             (만약) Create Table도중 실패? Abort수행!
                 ==> Delete Bit=1       .... (가)
                     Ager가 Slot삭제
                          ==> Used='F'  .... (나)

        만약 Ager가 수행하여 (나)의 상태까지 갔다면 Used='F'이므로 nextOIDAll
        에 의해 걸러질 것이다.

        그러나, Ager가 아직 수행되지 않아서 (가)의 상태에 있다면,
        Table의 생성 도중에 실패한 경우이므로 refine대상에서 제외해야한다.

        참고로, Drop Table시의 Flag변화는 다음과 같다.

        Drop Table
          ==> Drop = 1(Dropped)
          (만약) Commit한다면?
               ==> Delete Bit=1
                   Ager는 이를 지우지 않는다.(Prepare된 Tx가 접근하고 있을 수 있으므로)
                   Shutdown후 Startup시 Refine도중에 Catalog Table 에서 row삭제
                   => Used='F'
*/
        // 1. Table의 생성 도중에 실패한 경우 refine대상에서 제외해야한다.
        // 2. sPtr->mDropFlag이 TRUE이더라도 Rollback으로 FALSE로 될수 있다.
        if ( SMP_SLOT_IS_NOT_DROP( sPtr ) &&
             SM_SCN_IS_DELETED( sSCN ) )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        sHeader = (smcTableHeader *)( sPtr + 1 );

        IDE_TEST( (*aAction)( aStatistics,
                              sHeader,
                              aActionArg ) != IDE_SUCCESS );

        sCurPtr = sNxtPtr;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}




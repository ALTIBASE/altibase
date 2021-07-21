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
* $Id:$
**********************************************************************/


#include <idl.h>
#include <idm.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smErrorCode.h>
#include <smDef.h>
#include <smu.h>
#include <smm.h>
#include <smmReq.h>

/* ------------------------------------------------
 * [] global variable
 * ----------------------------------------------*/
smmMemBase         smmDatabase::mMemBaseBackup;
smmMemBase        *smmDatabase::mDicMemBase        = NULL;
smSCN              smmDatabase::mLstSystemSCN;

iduMutex           smmDatabase::mMtxSCN;

IDE_RC smmDatabase::initialize()
{
    mDicMemBase               = NULL;

    SM_INIT_SCN(&mLstSystemSCN);

    IDE_TEST(mMtxSCN.initialize( (SChar*)"SMM_SCN_MUTEX",
                                 IDU_MUTEX_KIND_NATIVE,
                                 IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC smmDatabase::destroy()
{
    /* BUG-33675 mDicMemBase pointer should be freed after destroying Tablespace */
    mDicMemBase                 = NULL;
    IDE_TEST(mMtxSCN.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}


/* 데이터베이스 생성시 membase를 초기화한다.
 * aDBName           [IN] 데이터베이스 이름
 * aDbFilePageCount  [IN] 하나의 데이터베이스 파일이 가지는 Page수
 * aChunkPageCount   [IN] 하나의 Expand Chunk당 Page수
 * aDBCharSet        [IN] 데이터베이스 캐릭터 셋(PROJ-1579 NCHAR)
 * aNationalCharSet  [IN] 내셔널 캐릭터 셋(PROJ-1579 NCHAR)
 */
IDE_RC smmDatabase::initializeMembase( smmTBSNode * aTBSNode,
                                       SChar      * aDBName,
                                       vULong       aDbFilePageCount,
                                       vULong       aChunkPageCount,
                                       SChar      * aDBCharSet,
                                       SChar      * aNationalCharSet )
{
    UInt           i;
    PDL_Time_Value sTimeValue;
    
    IDE_ASSERT( aTBSNode->mMemBase != NULL );

    IDE_DASSERT( aDBName != NULL );
    IDE_DASSERT( aDBName[0] != '\0' );
    IDE_DASSERT( aDbFilePageCount > 0 );
    IDE_DASSERT( aChunkPageCount > 0 );

    // PROJ-1579 NCHAR
    IDE_DASSERT( aDBCharSet != NULL );
    IDE_DASSERT( aDBCharSet[0] != '\0' );
    IDE_DASSERT( aNationalCharSet != NULL );
    IDE_DASSERT( aNationalCharSet[0] != '\0' );

    IDE_ASSERT( idlOS::strlen( aDBName ) < SM_MAX_DB_NAME );
    IDE_ASSERT( idlOS::strlen( iduGetSystemInfoString() )
                < IDU_SYSTEM_INFO_LENGTH );

    idlOS::strncpy(aTBSNode->mMemBase->mDBname,
                   aDBName,
                   SM_MAX_DB_NAME - 1 );
    aTBSNode->mMemBase->mDBname[ SM_MAX_DB_NAME - 1 ] = '\0';

    idlOS::strncpy(aTBSNode->mMemBase->mProductSignature,
                   iduGetSystemInfoString(),
                   IDU_SYSTEM_INFO_LENGTH - 1 );
    aTBSNode->mMemBase->mProductSignature[ IDU_SYSTEM_INFO_LENGTH - 1 ] = '\0';

    smuMakeUniqueDBString(aTBSNode->mMemBase->mDBFileSignature);

    aTBSNode->mMemBase->mVersionID          = smVersionID;
    aTBSNode->mMemBase->mCompileBit         = iduCompileBit;
    aTBSNode->mMemBase->mBigEndian          = iduBigEndian;
    aTBSNode->mMemBase->mLogSize            = smuProperty::getLogFileSize();
    aTBSNode->mMemBase->mDBFilePageCount    = aDbFilePageCount;
    aTBSNode->mMemBase->mTxTBLSize          = smuProperty::getTransTblSize();

    SM_INIT_SCN( &(aTBSNode->mMemBase->mSystemSCN) );

    // PROJ-1579 NCHAR
    idlOS::strncpy(aTBSNode->mMemBase->mDBCharSet,
                   aDBCharSet,
                   IDN_MAX_CHAR_SET_LEN - 1 );
    aTBSNode->mMemBase->mDBCharSet[ IDN_MAX_CHAR_SET_LEN - 1 ] = '\0';

    // PROJ-1579 NCHAR
    idlOS::strncpy(aTBSNode->mMemBase->mNationalCharSet,
                   aNationalCharSet,
                   IDN_MAX_CHAR_SET_LEN - 1 );
    aTBSNode->mMemBase->mNationalCharSet[ IDN_MAX_CHAR_SET_LEN - 1 ] = '\0';


    // BUG-15197 sun 5.10 x86 에서 createdb시 SEGV 사망
    sTimeValue = idlOS::gettimeofday();
    aTBSNode->mMemBase->mTimestamp = (struct timeval)sTimeValue;

    for ( i = 0; i < SMM_PINGPONG_COUNT; i++ )
    {
        aTBSNode->mMemBase->mDBFileCount[i] = 0;
    }

    aTBSNode->mMemBase->mAllocPersPageCount = 0;


    // Expand Chunk관련 정보 설정
    aTBSNode->mMemBase->mExpandChunkPageCnt = aChunkPageCount ;
    aTBSNode->mMemBase->mCurrentExpandChunkCnt = 0;

    // Free Page List를 초기화 한다.
    for ( i = 0; i< SMM_MAX_FPL_COUNT; i++ )
    {
        aTBSNode->mMemBase->mFreePageLists[ i ].mFirstFreePageID = SM_NULL_PID ;
        aTBSNode->mMemBase->mFreePageLists[ i ].mFreePageCount = 0;
    }

    aTBSNode->mMemBase->mFreePageListCount = SMM_FREE_PAGE_LIST_COUNT;


    return IDE_SUCCESS;
}

/******************************************************************************
 * mDicMemBase->mSystemSCN 을 SCN_SYNC_INTERVAL 만큼 증가 시킨다.
 * 주의주의 lockSCNMtx 을 잡고 호출되어야 한다.
 *****************************************************************************/
IDE_RC smmDatabase::setSystemSCN( smSCN * aSystemSCN )
{
    smSCN     sAddedSCN;

    SM_SET_SCN( &sAddedSCN, aSystemSCN );
    SM_ADD_SCN( &sAddedSCN, smuProperty::getSCNSyncInterval()); // 80000

    // logging : SMR_SMM_MEMBASE_SET_SYSTEM_SCN
    IDE_TEST( smLayerCallback::setSystemSCN( sAddedSCN ) != IDE_SUCCESS );

    SM_SET_SCN( &(mDicMemBase->mSystemSCN), &sAddedSCN ); 
    // dead code로 의심된다 .... 근데 있으니까 일단 넣음
    SM_SET_SCN( &(mMemBaseBackup.mSystemSCN) , &sAddedSCN );

    IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC,
                                SMM_MEMBASE_PAGEID )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/******************************************************************************
 * 주어진 SCN 으로 LstSystemSCN을 설정한다.
 *****************************************************************************/
IDE_RC smmDatabase::setLstSystemSCN( smSCN * aLstSystemSCNPtr, smSCN * aNewLstSystemSCNPtr )
{
    smSCN     sLstSystemSCN;
    smSCN   * sPersSysSCNPtr;
    smSCN     sPrev;
    SInt      sState = 0;

    IDU_FIT_POINT_RAISE( "smmDatabase::setLstSystemSCN::invalidValueOfSystemSCN",
                         err_invaild_SCN );
    IDE_TEST_RAISE( SM_SCN_IS_SYSTEMSCN( (*aLstSystemSCNPtr) ) != ID_TRUE, err_invaild_SCN );
    
    while(1)
    {
        sLstSystemSCN  = getLstSystemSCN();
        
        /* 주어진 SCN이 SystemSCN 보다 작으면 무시 한다. */
        if ( SM_SCN_IS_GE( &sLstSystemSCN, aLstSystemSCNPtr ) )
        {
            if ( aNewLstSystemSCNPtr != NULL )
            {
                SM_SET_SCN( aNewLstSystemSCNPtr, &sLstSystemSCN );
            }
            break;           
        }

        sPersSysSCNPtr = getSystemSCN();

        if ( SM_SCN_IS_GT( aLstSystemSCNPtr, sPersSysSCNPtr ) )
        {
            IDE_ASSERT( lockSCNMtx() == IDE_SUCCESS);
            sState = 1;
            /* 그사이에 getSystemSCN() 값이 다른 쓰레드에 의해 증가됐을수 있다.
               LOCK 잡고 다시확인 */
            sPersSysSCNPtr = getSystemSCN();

            if ( SM_SCN_IS_GT( aLstSystemSCNPtr, sPersSysSCNPtr ) )
            {
                IDE_TEST( setSystemSCN( aLstSystemSCNPtr ) != IDE_SUCCESS );
            }
            sState = 0;
            IDE_ASSERT( unlockSCNMtx() == IDE_SUCCESS );
        }

        sPrev = acpAtomicCas64( &mLstSystemSCN, (*aLstSystemSCNPtr), sLstSystemSCN );

        if ( sPrev == sLstSystemSCN ) /* CAS SUCCESS */
        {
            if ( aNewLstSystemSCNPtr != NULL )
            {
                SM_SET_SCN( aNewLstSystemSCNPtr, aLstSystemSCNPtr );
            }
            break;
        }
#ifdef DEBUG 
        else /* CAS FAILURE */
        {
            /* Cas 실패의 이유가
               1.다른 트랜잭션에의해 SystemSCN을 증가시켜 aLstSystemSCNPtr보다 크다면 끝.
               2.다른 트랜잭션이 SystemSCN을 증가시켰지만 aLstSystemSCNPtr보다 작다면
                  aLstSystemSCNPtr 까지 증가시켜야 한다 
               3.sPrev 가 sLstSystemSCN 보다 작을수 없다.. */
            IDE_DASSERT_MSG ( sPrev > sLstSystemSCN,
                              "sPrevSCN : %"ID_UINT64_FMT","
                              "sLstSystemSCN : %"ID_UINT64_FMT","
                              "aLstSystemSCNPtr : 0x%"ID_xINT64_FMT"\n",
                              sPrev,
                              sLstSystemSCN,
                              aLstSystemSCNPtr );
        }
#endif
    } // while

    validateCommitSCN();

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invaild_SCN )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INVALID_SCN, (*aLstSystemSCNPtr) ) )
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        (void)unlockSCNMtx();
    }

    return IDE_FAILURE;
}

/*
 * Statement Begin시 View를 결정하기 위해 호출되는 함수
 * 주의 : 32비트에 대해서도 64비트 SCN을 Atomic하게 읽기 위해 양쪽 LSB,MSB를 parity bit로
 *        이용하기 위한 코드이다.
 *
 *       64비트로 컴파일 되었을 경우에는 아래의 코드가 사실상 의미가 없으나,
 *       32비트일 경우에는 대단히 큰 의미를 가진다.
 *
 *       if(SM_GET_SCN_HIGHVBIT(a_pSCN) == SM_GET_SCN_LOWVBIT(a_pSCN))
 *       {
 *           break;
 *       }
 */

void smmDatabase::getViewSCN(smSCN *a_pSCN)
{
    while(1)
    {
        SM_GET_SCN(a_pSCN, &mLstSystemSCN);

        if(SM_GET_SCN_HIGHVBIT(a_pSCN) == SM_GET_SCN_LOWVBIT(a_pSCN))
        {
            break;
        }

        idlOS::thr_yield();
    }

    SM_SET_SCN_VIEW_BIT(a_pSCN);
}


/*
 *  Tx의 commit과 관련한 SCN과의 주의 사항
 *
 *  - Non-Atomic tuple-set reading을 방지하기 위해 다음과 같은 순서로
 *    Tx commit작업을 수행한다.
 *
 *    1. Commit SCN을 할당 받는다.
 *
 *    2. Temporary SCN이 Persistent SCN에 도달하면, Persistent SCN + Gap을
 *       수행하고, 로깅한다.
 *
 *    3. 증가된 SCN(Commit-SCN)을 Temporary SCN으로 assign한다.
 *
 *    4. callback을 이용해서 SCN과 인자로 넘어온 Status 상위에서 할당하되,
 *       반드시 SCN, Status 순서대로 assign 한다. (smxTrans.cpp::setTransCommitSCN())
 *       왜냐하면, 특정 Tuple의 Validation 과정에서 해당 Tuple을 읽을 것인지
 *       말 것인지를 결정할 때, non-blocking 알고리즘을 이용하여
 *       Transaction 객체의 status와 SCN을 읽어서 이용하는데
 *       이 때는 Status -> SCN의 순서대로 읽기 때문이다.
 *       [ 참고 => smxTrans::getTransCommitSCN() ]
 *
 *         TX            SCN   Status
 *       Write 시 :   ------------------> (commit 시)
 *       Read  시 :  <------------------  (tuple Validation 시)
 *
 *       % Tx의  status가 commit이라도 tx의 commitSCN이 infinite일수 있다.
 *        Tx의 commit후 end를 하게되는데 이때  commitSCN이 inifinite로
 *        재초기화 되기때문이다.
 *
 *    5. 증가된 Commit-SCN을 Temporary SCN, System SCN에 반영한다.
 *
 */


IDE_RC smmDatabase::getCommitSCN( void    * aTrans,
                                  idBool    aIsLegacyTrans,
                                  void    * aStatus )
{

    smSCN     sCommitSCN;
    smSCN     spTempSysSCN;
    smSCN*    spPersSysSCN;

    smSCN     sCasNew;
    smSCN     sCasOld;

#ifdef ALTIBASE_FIT_CHECK
    SInt      sState = 0;
#endif

#if 0
    {
    IDE_ASSERT( lockSCNMtx() == IDE_SUCCESS);
    sState = 1;

    // 1. Get Last Temporary SYSTEM-SCN
    spTempSysSCN = getLstSystemSCN();
    sCommitSCN  = spTempSysSCN;

    // 2. Increase for getting Commit-SCN
    SM_INCREASE_SCN(&sCommitSCN);

    IDE_ASSERT( SM_SCN_IS_SYSTEMSCN(sCommitSCN) == ID_TRUE );

    // 3. Check whether the Temporary SCN overrun the Persistent SCN
    spPersSysSCN = getSystemSCN();

    if( SM_SCN_IS_GT(spTempSysSCN, spPersSysSCN) ||
        SM_SCN_IS_EQ(spTempSysSCN, spPersSysSCN) )
    {
        smSCN     sAddedSCN;

        SM_SET_SCN(&sAddedSCN, spPersSysSCN);

        if(smuProperty::getSCNSyncInterval() > 80000)
        {

            ideLog::log(SM_TRC_LOG_LEVEL_FATAL,
                        SM_TRC_MEMORY_SCN_SYNC_INTERVAL_FATAL,
                        smuProperty::getSCNSyncInterval());
            IDE_ASSERT(0);
        }

        SM_ADD_SCN(&sAddedSCN, smuProperty::getSCNSyncInterval())

        if(SM_SCN_IS_GT(getSystemSCN(), &sAddedSCN))
        {
            ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                        SM_TRC_MEMORY_INVALID_SCN,
                        SM_SCN_TO_LONG( *(getSystemSCN()) ),
                        SM_SCN_TO_LONG( sAddedSCN ) );
            IDE_ASSERT(0);
        }

        IDE_TEST( smLayerCallback::setSystemSCN( sAddedSCN ) != IDE_SUCCESS );


        SM_SET_SCN(spPersSysSCN, &sAddedSCN);

        IDE_TEST(smmDirtyPageMgr::insDirtyPage(
                                     SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                     SMM_MEMBASE_PAGEID)
                 != IDE_SUCCESS);
    }
    IDL_MEM_BARRIER;
    }
#endif

    /* BUG-47367 Tx이 commitSCN을 수행하기 전 상태를 Status에 표시해 준다.
     * 해당 표시가 되어 있는 Tx의 경우 checkSCN 과정에서 getTransCommitSCN 함수를 통해
     * 정상적인 commitSCN이 설정되기를 기다렸다 값을 읽어가게 된다. */
    if( aTrans != NULL )
    {
        smLayerCallback::setTransStatus( aTrans,
                                         SMX_TX_PRECOMMIT );
    }

    /* BUG-47367 항상 LstSystemSCN < PersSystemSCN 이어야한다. */
    while(1)
    {
        spTempSysSCN = getLstSystemSCN();
        spPersSysSCN = getSystemSCN();

        if ( SM_SCN_IS_GE( &spTempSysSCN, spPersSysSCN ) )
        {
            IDE_ASSERT( lockSCNMtx() == IDE_SUCCESS);
#ifdef ALTIBASE_FIT_CHECK
            sState = 1;
#endif
            /* 그사이에 getSystemSCN() 값이 다른 쓰레드에 의해 증가됐을수 있다.
               LOCK 잡고 다시확인 */

            spTempSysSCN = getLstSystemSCN();
            spPersSysSCN = getSystemSCN();

            if ( SM_SCN_IS_GE( &spTempSysSCN, spPersSysSCN ) )
            {
                setSystemSCN( spPersSysSCN );
            }
#ifdef ALTIBASE_FIT_CHECK
            sState = 0;
#endif
            IDE_ASSERT( unlockSCNMtx() == IDE_SUCCESS );
        }

        /* SM_INCREASE_SCN 와 같은 단위로 올라가야한다. */
        sCasNew = spTempSysSCN + 8;
        sCasOld = acpAtomicCas64( &mLstSystemSCN, sCasNew, spTempSysSCN );

        if ( sCasOld == spTempSysSCN )
        {
            /* CAS SUCCESS */
            sCommitSCN = sCasNew;
            break;
        }
    }

    /* CASE-6985 서버 종료후 startup하면 user와 테이블,
     * 데이터가 모두 사라짐*/
    validateCommitSCN();

    /* 
     * 4. Callback for strict ordered setting of Tx SCN & Status
     *
     * aTrans == NULL인 경우는 System SCN을 증가만 시킬경우이다.
     * Delete Thread에서 남아 있는 Aging대상 OID들을 처리하기위해
     * Commit SCN을 증가시킨다.
     *
     * BUG-30911 - 2 rows can be selected during executing index scan
     *             on unique index. 
     *
     * LstSystemSCN을 먼저 증가시키고 트랜잭션에 CommitSCN을 설정해야 한다.
     * 로 수정했었는데, BUG-31248 로 인해 다시 원복 합니다.
     */
    if( aTrans != NULL )
    {
        smLayerCallback::setTransSCNnStatus( aTrans, aIsLegacyTrans, &sCommitSCN, aStatus );
    }
    else
    {
        IDE_DASSERT( aTrans == NULL );
        IDE_DASSERT( aStatus == NULL ); 
        IDE_DASSERT( aIsLegacyTrans == ID_FALSE );
    }

    IDU_FIT_POINT( "1.BUG-30911@smmDatabase::getCommitSCN" );

    return IDE_SUCCESS;

#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION_END;

    IDE_PUSH();
    
    if( sState != 0 )
    {
        IDE_ASSERT( unlockSCNMtx() == IDE_SUCCESS );
    }

    IDE_POP();

    return IDE_FAILURE;
#endif
}

/*
 * 현재 System Commit Number가 Valid한지 검증한다.
 * 
 * BUG-47367 getCommitSCN에서 SCNMtx의 역할이 persSystemSCN을 증가시키는 것으로 한정됨에 따라
 * Lock을 잡지 않고 LstSystemSCN을 먼저 가져온뒤 persSystemSCN을 가져와 확인하는 방식으로 바꾼다.
 * 
 */
void smmDatabase::validateCommitSCN()
{
    smSCN sSCN;
    /* CASE-6985 서버 종료후 startup하면 user와 테이블,
     * 데이터가 모두 사라짐: 현재 Membase에 있는 SystemSCN은
     * 항상 Transaction에게 할당되는 m_lstSystemSCN보다 항상
     * 커야 한다. */
    sSCN = getLstSystemSCN();

    if( SM_SCN_IS_GT(&sSCN, getSystemSCN()) )
    {
        ideLog::log(SM_TRC_LOG_LEVEL_WARNNING,
                    "Invalid System SCN:%llu, \
                    Last SCN:%llu\n",
                    SM_SCN_TO_LONG( *(getSystemSCN()) ),
                    SM_SCN_TO_LONG( sSCN ));
        IDE_ASSERT(SM_SCN_IS_LT(&sSCN, getSystemSCN()));
    }
}

IDE_RC smmDatabase::checkVersion(smmMemBase *aMembase)
{

    UInt s_DbVersion;
    UInt s_SrcVersion;

    UInt sCheckVersion;
    UInt sCheckBit;
    UInt sCheckEndian;
    UInt sCheckLogFileSize;

    IDE_DASSERT( aMembase != NULL );

    /* ------------------------------------------------
     * [0] Read From Properties
     * ----------------------------------------------*/
    sCheckVersion = smuProperty::getCheckStartupVersion();
    sCheckBit = smuProperty::getCheckStartupBitMode();
    sCheckEndian = smuProperty::getCheckStartupEndian();
    sCheckLogFileSize = smuProperty::getCheckStartupLogSize();

    /* ------------------------------------------------
     * [1] Product Version Check
     * ----------------------------------------------*/

    if (sCheckVersion != 0)
    {
        s_DbVersion   = aMembase->mVersionID & SM_CHECK_VERSION_MASK;
        s_SrcVersion  = smVersionID & SM_CHECK_VERSION_MASK;

        IDE_TEST_RAISE (s_DbVersion != s_SrcVersion, version_mismatch_error);
    }

    /* ------------------------------------------------
     * [2] Bit Mode Check 32/64
     * ----------------------------------------------*/
    if (sCheckBit != 0)
    {
        IDE_TEST_RAISE(aMembase->mCompileBit != iduCompileBit,
                       version_mismatch_error);
    }

    /* ------------------------------------------------
     * [3] Endian Check
     * ----------------------------------------------*/
    if (sCheckEndian != 0)
    {
        IDE_TEST_RAISE(aMembase->mBigEndian != iduBigEndian,
                       version_mismatch_error);
    }

    /* ------------------------------------------------
     * [4] Log Size Check
     * ----------------------------------------------*/
    if (sCheckLogFileSize != 0)
    {
        IDE_TEST_RAISE(aMembase->mLogSize != smuProperty::getLogFileSize(),
                       version_mismatch_error);
    }

    /* ------------------------------------------------
     * [5] Transaction Table Size
     * ----------------------------------------------*/
    IDE_TEST_RAISE(aMembase->mTxTBLSize > smuProperty::getTransTblSize(),
                   version_mismatch_error);

    return IDE_SUCCESS;

    IDE_EXCEPTION(version_mismatch_error);
    {
        SChar s_diskVer[32];
        UInt  s_diskVersion = aMembase->mVersionID;

        idlOS::memset(s_diskVer, 0, 32);
        idlOS::snprintf(s_diskVer, 32,
                        "%"ID_xINT32_FMT".%"ID_xINT32_FMT".%"ID_xINT32_FMT,
                        ((s_diskVersion & SM_MAJOR_VERSION_MASK) >> 24),
                        ((s_diskVersion & SM_MINOR_VERSION_MASK) >> 16),
                        (s_diskVersion  & SM_PATCH_VERSION_MASK));

        IDE_SET(ideSetErrorCode(smERR_ABORT_BACKUP_DISK_INVALID,
                                s_diskVer,
                                (UInt)aMembase->mCompileBit,
                                (aMembase->mBigEndian == ID_TRUE) ? "BIG" : "LITTLE",
                                (ULong)aMembase->mLogSize,
                                (UInt)aMembase->mTxTBLSize,

                                smVersionString,
                                (UInt)iduCompileBit,
                                (iduBigEndian == ID_TRUE) ? "BIG" : "LITTLE",
                                (ULong)smuProperty::getLogFileSize(),
                                smuProperty::getTransTblSize()));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


IDE_RC smmDatabase::checkMembaseIsValid()
{

    IDE_TEST_RAISE(idlOS::strcmp(mMemBaseBackup.mDBname, mDicMemBase->mDBname) != 0,
                   err_invalid_membase);

    IDE_TEST_RAISE(idlOS::strcmp(mMemBaseBackup.mProductSignature,
                                 mDicMemBase->mProductSignature) != 0,
                   err_invalid_membase);
    IDE_TEST_RAISE(idlOS::strcmp(mMemBaseBackup.mDBFileSignature,
                                 mDicMemBase->mDBFileSignature) != 0,
                   err_invalid_membase);

    IDE_TEST_RAISE(mMemBaseBackup.mVersionID != mDicMemBase->mVersionID,
                   err_invalid_membase);
    IDE_TEST_RAISE(mMemBaseBackup.mCompileBit != mDicMemBase->mCompileBit,
                   err_invalid_membase);
    IDE_TEST_RAISE(mMemBaseBackup.mBigEndian != mDicMemBase->mBigEndian,
                   err_invalid_membase);
    IDE_TEST_RAISE(mMemBaseBackup.mLogSize != mDicMemBase->mLogSize,
                   err_invalid_membase);
    IDE_TEST_RAISE(mMemBaseBackup.mDBFilePageCount !=
                   mDicMemBase->mDBFilePageCount,
                   err_invalid_membase);
    IDE_TEST_RAISE(mMemBaseBackup.mTxTBLSize !=
                   mDicMemBase->mTxTBLSize,
                   err_invalid_membase);

    // PROJ-1579 NCHAR
    IDE_TEST_RAISE(idlOS::strcmp(mMemBaseBackup.mDBCharSet, 
                                 mDicMemBase->mDBCharSet) != 0,
                   err_invalid_membase);

    // PROJ-1579 NCHAR
    IDE_TEST_RAISE(idlOS::strcmp(mMemBaseBackup.mNationalCharSet, 
                                 mDicMemBase->mNationalCharSet) != 0,
                   err_invalid_membase);

    return IDE_SUCCESS;

    IDE_EXCEPTION(err_invalid_membase);
    {
        dumpMembase();
        IDE_SET(ideSetErrorCode(smERR_FATAL_MEMBASE_INVALID));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}


/*
 * Expand Chunk관련된 프로퍼티 값들이 제대로 된 값인지 체크한다.
 *
 * 1. 하나의 Chunk안의 데이터 페이지를 여러 Free Page List에 분배할 때,
 *    최소한 한번은 분배가 되는지 체크
 *
 *    만족조건 : Chunk당 데이터페이지수 >= 2 * List당 분배할 Page수 * List 수
 *
 * aChunkDataPageCount [IN] Expand Chunk안의 데이터페이지 수
 *                          ( FLI Page를 제외한 Page의 수 )
 */
IDE_RC smmDatabase::checkExpandChunkProps(smmMemBase * aMemBase)
{
    IDE_DASSERT( aMemBase != NULL );

    // 다중화된 Free Page List수가  createdb시점과 다른 경우
    IDE_TEST_RAISE(aMemBase->mFreePageListCount !=
                   SMM_FREE_PAGE_LIST_COUNT,
                   different_page_list_count );

    // Expand Chunk안의 Page수가 createdb시점과 다른 경우
    IDE_TEST_RAISE(aMemBase->mExpandChunkPageCnt !=
                   smuProperty::getExpandChunkPageCount() ,
                   different_expand_chunk_page_count );

    //  Expand Chunk가 추가될 때
    //  ( 데이터베이스 Chunk가 새로 할당될 때  )
    //  그 안의 Free Page들은 여러개의 다중화된 Free Page List로 분배된다.
    //
    //  이 때, 각각의 Free Page List에 최소한 하나의 Free Page가
    //  분배되어야 하도록 시스템의 아키텍쳐가 설계되어 있다.
    //
    //  만약 Expand Chunk안의 Free Page수가 충분하지 않아서,
    //  PER_LIST_DIST_PAGE_COUNT 개씩 모든 Free Page List에 분배할 수가
    //  없다면 에러를 발생시킨다.
    //
    //  Expand Chunk안의 Free List Info Page를 제외한
    //  데이터페이지만이 Free Page List에 분배되므로, 이 갯수를 체크해야 한다.
    //  그러나, 이러한 모든 내용을 일반 사용자가 이해하기에는 너무 난해하다.
    //
    //  Expand Chunk의 페이지 수는 Free Page List에 두번씩 분배할 수 있을만큼
    //  충분한 크기를 가지도록 강제한다.
    //
    //  조건식 : EXPAND_CHUNK_PAGE_COUNT <=
    //           2 * PER_LIST_DIST_PAGE_COUNT * PAGE_LIST_GROUP_COUNT
   IDE_TEST_RAISE(
        aMemBase->mExpandChunkPageCnt
        <
        2 * SMM_PER_LIST_DIST_PAGE_COUNT * aMemBase->mFreePageListCount,
        err_too_many_per_list_page_count );


    return IDE_SUCCESS;

    IDE_EXCEPTION(different_expand_chunk_page_count);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DIFFERENT_DB_EXPAND_CHUNK_PAGE_COUNT,
                                SMM_FREE_PAGE_LIST_COUNT,
                                aMemBase->mExpandChunkPageCnt ));
    }
    IDE_EXCEPTION(different_page_list_count);
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_DIFFERENT_DB_FREE_PAGE_LIST_COUNT,
                                SMM_FREE_PAGE_LIST_COUNT,
                                aMemBase->mFreePageListCount ));
    }
    IDE_EXCEPTION( err_too_many_per_list_page_count );
    {
        IDE_SET(ideSetErrorCode(smERR_ABORT_TOO_MANY_PER_LIST_PAGE_COUNT_ERROR,
                                (ULong) aMemBase->mExpandChunkPageCnt,
                                (ULong) SMM_PER_LIST_DIST_PAGE_COUNT,
                                (ULong) aMemBase->mFreePageListCount ));
    }
    IDE_EXCEPTION_END;


    return IDE_FAILURE;

}


// PROJ-1579 NCHAR
SChar* smmDatabase::getDBCharSet()
{
    SChar* sDBCharSet;

    if ( mDicMemBase != NULL )
    {
        sDBCharSet = mDicMemBase->mDBCharSet;
    }
    else
    {
        sDBCharSet = (SChar*)"";
    }

    return sDBCharSet;
}

SChar* smmDatabase::getNationalCharSet()
{
    SChar* sNationalCharSet;

    if ( mDicMemBase != NULL )
    {
        sNationalCharSet = mDicMemBase->mNationalCharSet;
    }
    else
    {
        sNationalCharSet = (SChar*)"";
    }

    return sNationalCharSet;
}


void smmDatabase::dumpMembase()
{
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE1);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE2, mDicMemBase->mDBname);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE3, mDicMemBase->mProductSignature);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE4, mDicMemBase->mDBFileSignature);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE5, mDicMemBase->mVersionID);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE6, mDicMemBase->mCompileBit);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE7, mDicMemBase->mBigEndian);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE8, mDicMemBase->mLogSize);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE9, mDicMemBase->mDBFilePageCount);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE10, mDicMemBase->mTxTBLSize);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE11, mDicMemBase->mDBCharSet);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE12, mDicMemBase->mNationalCharSet);

    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE13);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE2, mMemBaseBackup.mDBname);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE3, mMemBaseBackup.mProductSignature);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE4, mMemBaseBackup.mDBFileSignature);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE5, mMemBaseBackup.mVersionID);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE6, mMemBaseBackup.mCompileBit);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE7, mMemBaseBackup.mBigEndian);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE8, mMemBaseBackup.mLogSize);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE9, mMemBaseBackup.mDBFilePageCount);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE10, mMemBaseBackup.mTxTBLSize);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE11, mDicMemBase->mDBCharSet);
    ideLog::log(SM_TRC_LOG_LEVEL_MEMORY, SM_TRC_MEMORY_DUMP_MEMBASE12, mDicMemBase->mNationalCharSet);
}

#ifdef DEBUG
/***********************************************************************
 * Description : BUG-31862 resize transaction table without db migration
 *      MemBase와 프로퍼티의 트랜잭션 테이블 사이즈를 비교한다.
 *      프로퍼티의 값이 MemBase의 값보다 커야 하며, 2^N 값이어야 한다.
 *
 * Implementation :
 *
 * [IN] aMembase - 테이블스페이스의 멤베이스
 *
 **********************************************************************/
IDE_RC smmDatabase::checkTransTblSize(smmMemBase * aMemBase)
{
    UInt    sTransTblSize   = smuProperty::getTransTblSize();

    IDE_DASSERT( aMemBase != NULL );
    IDE_DASSERT( sTransTblSize >= aMemBase->mTxTBLSize );
    
    return IDE_SUCCESS;
}
#endif

/***********************************************************************
 * Description : BUG-31862 resize transaction table without db migration
 *      프로퍼티의 TRANSACTION_TABLE_SIZE 값으로
 *      mDicMemBase의 mTxTBLSize를 확장한다. 
 *      값은 2^N만 가능하다.
 *
 * Implementation :
 *
 **********************************************************************/
IDE_RC smmDatabase::refineTransTblSize()
{
    UInt    sTransTblSize   = smuProperty::getTransTblSize();

    IDE_DASSERT( checkTransTblSize(mDicMemBase) == IDE_SUCCESS );

    if ( sTransTblSize > mDicMemBase->mTxTBLSize )
    {
        setTxTBLSize(mDicMemBase, sTransTblSize);

        IDE_TEST( smmDirtyPageMgr::insDirtyPage(
                                    SMI_ID_TABLESPACE_SYSTEM_MEMORY_DIC, 
                                    SMM_MEMBASE_PAGEID)
                  != IDE_SUCCESS);

        makeMembaseBackup();
    }
    else
    {
        /* do nothing */
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

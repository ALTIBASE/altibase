/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/
#include <idl.h>
#include <idErrorCode.h>
#include <iduShmLatch.h>
#include <iduShmDef.h>
#include <iduShmKeyMgr.h>
#include <iduShmChunkMgr.h>
#include <idwPMMgr.h>
#include <iduVLogShmMgr.h>
#include <idrLogMgr.h>
#include <iduShmKeyFile.h>
#include <iduShmMgr.h>
#include <iduShmPersMgr.h>

/* MSB( Most Significant Bit ) */
SInt iduShmMgr::mMSBit4FF[] = {
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4,
    4, 4,
    4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5,
    5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6,
    6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6,
    6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7,
    7, 7, 7, 7, 7, 7, 7
};

iduShmSSegment * iduShmMgr::mSSegment;
ULong            iduShmMgr::mAttachSegCnt;
iduShmHeader  ** iduShmMgr::mArrSegment;
idBool           iduShmMgr::mIsShmAreaFreeAtShutdown;
SInt             iduShmMgr::mOffsetBitSize4ShmAddr;
struct timeval   iduShmMgr::mStartUpTime;
uid_t            iduShmMgr::mUID; /* BUG-40895 */

SChar* iduShmMgr::getPtrOfAddr( idShmAddr aAddr )
{
    SChar  * sPtr = NULL;

    if( aAddr != IDU_SHM_NULL_ADDR )
    {
        if((aAddr & IDU_SHM_PERS_ADDR) == 0)
        {
            ULong    sSegID;

            sSegID = IDU_SHM_GET_ADDR_SEGID( aAddr );

            if( sSegID >= mSSegment->mSegCount )
            {
                ideLog::log( IDE_SERVER_0,
                             "iduShmMgr::getPtrOfAddr\n"
                             "aAddr               : %"ID_vULONG_FMT"\n"
                             "sSegID & Offset     : "
                             "%"ID_UINT64_FMT",%"ID_UINT64_FMT"\n"
                             "mSSegment->mSegCount: %"ID_UINT64_FMT"\n"
                             "mSSegment->mMaxSegCount : %"ID_UINT64_FMT"\n",
                             aAddr,
                             sSegID,
                             IDU_SHM_GET_ADDR_OFFSET(aAddr),
                             mSSegment->mSegCount,
                             mSSegment->mMaxSegCount );

                IDE_ASSERT(0);
            }

            sPtr = (SChar*)mArrSegment[sSegID];

            if( sPtr == NULL )
            {
                IDE_ASSERT( attachSeg( sSegID, ID_FALSE /*aIsCleanPurpose*/  )
                            == IDE_SUCCESS );

                sPtr = (SChar*)mArrSegment[sSegID];
            }

            sPtr = sPtr + IDU_SHM_GET_ADDR_OFFSET( aAddr );
        }
        else
        {
            sPtr = (SChar*)iduShmPersMgr::getPtrOfAddr(aAddr);
        }
    }

    return sPtr;
}

/***********************************************************************
 * Description : Shared Memory Manager를 초기화 한다. DAEMON Process의
 *               경우에는 System Segment를 생성하고 SHM_STARTUP_SIZE 만큼
 *               Shared Memory를 생성한다. 다른 Type Process의 경우 System
 *               Segment가 존재하는 경우 Attach를 수행하고 System Segment에
 *               존재하는 Data Segment정보를 이용하여 Attach를 시도한다.
 *
 * aProcType - [IN] Process Type
 *               + IDU_PROC_TYPE_DAEMON
 *               + IDU_PROC_TYPE_WSERVER
 *               + IDU_PROC_TYPE_RSERVER
 *               + IDU_PROC_TYPE_USER
 *
 **********************************************************************/
IDE_RC iduShmMgr::initialize( iduShmProcType aProcType )
{
    UInt              sDataSegSize = iduProperty::getShmChunkSize();
    ULong             sMaxSegCnt;
    UInt              sStartUpShmSize;
    ULong             sMaxShmSize  = iduProperty::getShmMaxSize();
    iduShmTxInfo    * sTxInfo;
    idvSQL            sStatistics;
    UInt              sInitChunkCnt;
    UInt              sAllocChunkSize;
    UInt              i;
    idBool            sIsValid;

    idvManager::initSQL( &sStatistics, NULL /* Session */ );

    mOffsetBitSize4ShmAddr = getMSBit( sDataSegSize ) + 1; // Chunk overhead

    getMaxSegCnt( sMaxShmSize, sDataSegSize, &sMaxSegCnt );

    IDE_TEST_RAISE( sMaxSegCnt == 0, err_invalid_segcnt );

    IDE_TEST( initLtArea( sMaxSegCnt ) != IDE_SUCCESS );

    IDE_TEST( idrLogMgr::initialize() != IDE_SUCCESS );

    IDE_TEST( iduShmChunkMgr::initialize( sMaxSegCnt ) != IDE_SUCCESS );


    idrLogMgr::disableLogging();

    /* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
    mUID = idlOS::getuid();

    // Shared Memory영역을 검사하여 Valid하지 않으면 제거
    IDE_TEST( checkShmAndClean( aProcType, &sIsValid ) != IDE_SUCCESS );

    if ( sIsValid == ID_TRUE )
    {
        idrLogMgr::enableLogging();

        IDE_TEST( attach( aProcType ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST_RAISE( createSSegment( NULL /* aStatistics */, sDataSegSize, sMaxSegCnt )
                        != IDE_SUCCESS, err_create_ssegment );


        idrLogMgr::enableLogging();

        IDE_TEST( iduShmKeyMgr::initializeStatic( mSSegment )
                  != IDE_SUCCESS );

        IDE_TEST( idwPMMgr::initialize( aProcType ) != IDE_SUCCESS );

        sStartUpShmSize = iduProperty::getStartUpShmSize();

        IDV_WP_SET_MAIN_PROC_INFO( &sStatistics );

        sTxInfo = idwPMMgr::getMainShmThrInfo( &sStatistics );

        IDV_WP_SET_THR_INFO( &sStatistics, sTxInfo );

        sInitChunkCnt = sStartUpShmSize / iduProperty::getShmChunkSize();

        /* Alloc Thread Info하면서 Data Chunk 1개가 이미 할당되었다. */
        for( i = 0; i < sInitChunkCnt; i++ )
        {
            sAllocChunkSize = iduProperty::getShmChunkSize();

            if( ( getTotalSegSize() + sAllocChunkSize ) > iduProperty::getShmMaxSize() )
            {
                sAllocChunkSize = iduProperty::getShmMaxSize() - getTotalSegSize();
            }

            if( sAllocChunkSize == 0 )
            {
                break;
            }

            IDE_TEST( createDSegment( &sStatistics,
                                      sTxInfo,
                                      IDU_SHM_TYPE_DATA_SEGMENT,
                                      sAllocChunkSize )
                      != IDE_SUCCESS );

            idrLogMgr::commit( &sStatistics, sTxInfo );
        }
    }

    //Daemon 만 shared Memory 상태를 변경한다. 
    if ( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON )
    {
        // 비정상 종료를 대비해 SSegment에 정상 종료 상태를 FALSE로 세팅
        (void)setNormalShutdown( ID_FALSE );
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_segcnt );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_INVALID_SHM_SEGCOUNT ) );
    }
    IDE_EXCEPTION( err_create_ssegment );
    {
        /* BUG-35859 */
        IDE_SET( ideSetErrorCode( idERR_FATAL_CREATE_SYSTEM_SEGMENT ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Shared Memory Manager를 destroy한다.
 *               Daemon프로세스의 경우 SHM_POLICY에 따라 Segment들을
 *               destroy or detach한다.
 *               
 * aIsNormalShutdown - [IN] Normal or Abnormal Shutdown
 *                          모든 모듈이 정상적인 상태로 종료되어
 *                          Shared Memory 상태가 Vaild한지 여부를 나타낸다.
 *                          이후 startup시 SHM_POLICY=1일 때 바로 Attach하여
 *                          사용할 수 있는지 확인에 쓰인다.
 **********************************************************************/
IDE_RC iduShmMgr::destroy( idBool aIsNormalShutdown )
{
    iduShmProcInfo  * sDaemonProcInfo;
    sDaemonProcInfo = idwPMMgr::getProcInfo( IDU_PROC_TYPE_DAEMON );

    //Daemon 만 shared Memory 상태를 변경한다. 
    if ( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON ) 
    {
        (void)setNormalShutdown( aIsNormalShutdown );
    }

    // BUG-36132
    // Daemon이거나, Daemon이 없이 시작된(TSM, util) Process일 경우 Destroy한다.
    if ( ( idwPMMgr::getProcType() == IDU_PROC_TYPE_DAEMON ) ||
         ( sDaemonProcInfo->mType  == IDU_SHM_PROC_TYPE_NULL ) )
    {
        IDE_ASSERT( idwPMMgr::destroy() == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( idwPMMgr::detach() == IDE_SUCCESS );
    }

    // SHM_POLICY=1이라면 Segment들을 drop하지 않는다.
    // Daemon이 죽은 경우 or Daemon없이 initialize된 경우도 Free되야함.
    if ( ( isShmAreaFreeAtShutdown() == ID_TRUE ) &&
         ( (sDaemonProcInfo->mType == IDU_SHM_PROC_TYPE_NULL) ||
           (sDaemonProcInfo->mType == IDU_PROC_TYPE_DAEMON) ) )
    {
        IDE_ASSERT( dropAllDSegment() == IDE_SUCCESS );
        IDE_ASSERT( dropSSegment( ID_FALSE /* aIsCleanPurpose */ )
                    == IDE_SUCCESS );
        IDE_ASSERT( iduShmChunkMgr::destroy() == IDE_SUCCESS );

        IDE_ASSERT( destLtArea() == IDE_SUCCESS );

        IDE_ASSERT( iduShmKeyMgr::destroyStatic()
                    == IDE_SUCCESS );
    }
    else
    {
        IDE_ASSERT( detach() == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

/***********************************************************************
 * Description : Shared Memory Chunk가 유효한지 체크하고
 *               유효하지 않다면 Clean
 *
 * aProcType - [IN] Process Type(Daemon Working User)
 * aIsValid - [OUT] Shared Memory 공간이 유효함(Attach하여 사용가능)
 **********************************************************************/
IDE_RC iduShmMgr::checkShmAndClean( iduShmProcType aProcType,
                                    idBool       * aIsValid )
{
    iduShmSSegment  * sSSegHdr;
    idBool            sIsValid = ID_TRUE;
    UInt              sState = 0;

    if ( aIsValid != NULL )
    {
        *aIsValid = ID_FALSE;
    }

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_IDU,
                                 ID_SIZEOF(iduShmSSegment),
                                 (void**)&sSSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    // 공유 메모리가 존재하면 Validation체크함
    if ( iduShmChunkMgr::attachAndGetSSegHdr( sSSegHdr ) == IDE_SUCCESS )
    {
        IDE_TEST( validateShmChunk( &sSSegHdr->mHeader ) != IDE_SUCCESS );
    }

    // 정상종료되지 못한 ShmChunk는 재사용이 불가능하다.
    // ShmPolicy가 1이 아닌 경우 기존 공유메모리는 제거한다.
    if ( ( aProcType == IDU_PROC_TYPE_DAEMON ) &&
         ( ( iduProperty::getShmMemoryPolicy() == 0) ||
           ( sSSegHdr->mIsNormalShutdown != ID_TRUE ) ) )
    {
        (void)cleanAllShmArea( aProcType, iduProperty::getShmDBKey() );

        sIsValid = ID_FALSE;
    }

    if( aIsValid != NULL )
    {
        *aIsValid = sIsValid;
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sSSegHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sSSegHdr ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 기 생성된 Shared Memory에 Attach한다.
 *               [!] Clean을 위해 attach하는 경우 attach4Clean 사용해야함.
 *
 * aProcType - [IN] Process Type(Daemon, Working, User)
 **********************************************************************/
IDE_RC iduShmMgr::attach( iduShmProcType aProcType )
{
    iduShmSSegment  * sSSegHdr;
    ULong             i;
    iduSCH          * sNewSCH;
    ULong             sAllocSegCnt;
    iduShmTxInfo    * sTxInfo;
    idvSQL            sStatistics;
    idrSVP            sVSavepoint;

    idvManager::initSQL( &sStatistics, NULL /* Session */ );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_IDU,
                                 ID_SIZEOF(iduShmSSegment),
                                 (void**)&sSSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( iduShmChunkMgr::attachAndGetSSegHdr( sSSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( iduShmChunkMgr::attachChunk( sSSegHdr->mHeader.mShmKey,
                                           sSSegHdr->mHeader.mSize,
                                           &sNewSCH )
              != IDE_SUCCESS );

    mSSegment = (iduShmSSegment*)sNewSCH->mChunkPtr;
    registerOldSeg( sNewSCH, 0 );

    IDE_TEST( iduShmKeyMgr::initializeStatic( mSSegment )
              != IDE_SUCCESS );

    // Daemon의 경우 attach인 경우에도 PM Manager를 초기화한다.
    if ( aProcType == IDU_PROC_TYPE_DAEMON )
    {
        IDE_TEST( idwPMMgr::initialize( aProcType ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( idwPMMgr::attach( aProcType ) != IDE_SUCCESS );
    }

    IDV_WP_SET_MAIN_PROC_INFO( &sStatistics );

    IDE_TEST( idwPMMgr::allocThrInfo( &sStatistics,
                                      IDU_SHM_THR_DEFAULT_ATTR,
                                      &sTxInfo )
              != IDE_SUCCESS );

    /* 새로운 Segment를 할당하고 등록하다가 죽은 Process가 존재할 수 있다.
     * 때문에 할당된 Seg의 갯수는 AllocSeg Latch를 잡은 후에 가져온다. */
    idrLogMgr::setSavepoint( sTxInfo, &sVSavepoint );

    IDE_TEST( iduShmLatchAcquire( &sStatistics,
                                  sTxInfo,
                                  &mSSegment->mLatch4AllocSeg )
              != IDE_SUCCESS );

    sAllocSegCnt = mSSegment->mSegCount;

    IDE_TEST( idrLogMgr::commit2Svp( &sStatistics, sTxInfo, &sVSavepoint )
              != IDE_SUCCESS );

    for( i = 1; i < sAllocSegCnt; i++ )
    {
        IDE_TEST( attachSeg( i, ID_FALSE ) /* IsCleanPurpose */
                  != IDE_SUCCESS );
    }

    mAttachSegCnt = sAllocSegCnt;

    IDE_TEST( idwPMMgr::freeThrInfo( &sStatistics, sTxInfo )
              != IDE_SUCCESS );

    IDE_TEST( iduMemMgr::free( sSSegHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUGBUG 왜 필요한지 모르겠다.
    if( aIsCleanPurpose == ID_FALSE )
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_ATTACH_SHARED_MEMORY_MGR ) );
    }
    */

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Clean을 위해 Shared Memory에 Attach한다.
 *
 * aProcType - [IN] Process Type(Daemon, Working, User)
 * aShmDBKey - [IN] Clean하려는 SSysChunk의 DBKEY
 **********************************************************************/
IDE_RC iduShmMgr::attach4Clean( iduShmProcType aProcType, UInt aShmDBKey )
{
    iduShmSSegment  * sSSegHdr;
    ULong             i;
    iduSCH          * sNewSCH;
    ULong             sAllocSegCnt;
    UInt              sState = 0;
    UInt const        sDataSegSizeProp = iduProperty::getShmChunkSize();
    ULong const       sMaxShmSizeProp  = iduProperty::getShmMaxSize();
    ULong             sMaxSegCountProp = 0;

    IDE_ASSERT( aProcType == IDU_PROC_TYPE_DAEMON );

    getMaxSegCnt( sMaxShmSizeProp, sDataSegSizeProp, &sMaxSegCountProp );

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ID_IDU,
                                 ID_SIZEOF(iduShmSSegment),
                                 (void**)&sSSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduShmChunkMgr::attachAndGetSSegHdr( sSSegHdr, aShmDBKey )
              != IDE_SUCCESS );

    IDE_TEST( iduShmChunkMgr::attachChunk( sSSegHdr->mHeader.mShmKey,
                                           sSSegHdr->mHeader.mSize,
                                           &sNewSCH )
              != IDE_SUCCESS );

    mSSegment = (iduShmSSegment*)sNewSCH->mChunkPtr;
    registerOldSeg( sNewSCH, 0 );

    sAllocSegCnt = mSSegment->mSegCount;

    IDE_TEST_RAISE( mSSegment->mMaxSegCount > sMaxSegCountProp,
                    err_incompat_config );

    for ( i = 1; i < sAllocSegCnt; i++ )
    {
        IDE_TEST( attachSeg( i, ID_TRUE /*aIsCleanPurpose*/ ) != IDE_SUCCESS );
    }

    mAttachSegCnt = sAllocSegCnt;

    IDE_TEST( iduShmKeyMgr::initializeStatic( mSSegment )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sSSegHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_incompat_config )
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_IDU_SHM_INCOMPATIBLE_CONFIG,
                                  mSSegment->mSegSize,
                                  mSSegment->mMaxSegCount,
                                  sDataSegSizeProp,
                                  sMaxSegCountProp ) );
        IDE_ASSERT( ID_FALSE );
    }

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sSSegHdr ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : Shared Memory에 Attach하고 있던 자원들을 해제한다.
 *
 **********************************************************************/
IDE_RC iduShmMgr::detach()
{
    ULong     sIndex;
    ULong     sAllocSegCnt;

    IDE_ASSERT( mArrSegment    != NULL );
    IDE_ASSERT( mSSegment      != NULL );
    IDE_ASSERT( mArrSegment[0] != NULL );

    sAllocSegCnt = mSSegment->mSegCount;

    if( sAllocSegCnt != 0 )
    {
        sIndex = sAllocSegCnt;

        while(1)
        {
            sIndex--;

            if( mArrSegment[sIndex] != NULL )
            {
                IDE_ASSERT( mAttachSegCnt != 0 );

                mAttachSegCnt--;

                // fix BUG-41100
                if( mSSegment->mArrSegInfo[sIndex].mShmKey !=
                            IDU_SHM_SHM_KEY_NULL )
                {
                    IDE_ASSERT( iduShmChunkMgr::detachChunk( mArrSegment[sIndex]->mSegID )
                                == IDE_SUCCESS );
                }
                else
                {
                    IDE_ASSERT( iduGetShmProcType() == IDU_PROC_TYPE_USER );
                }
            }

            if( sIndex == 0 )
            {
                break;
            }
        }
    }

    IDE_ASSERT( iduShmChunkMgr::destroy() == IDE_SUCCESS );

    IDE_ASSERT( destLtArea() == IDE_SUCCESS );

    mSSegment = NULL;

    IDE_ASSERT( iduShmKeyMgr::destroyStatic()
                == IDE_SUCCESS );

    return IDE_SUCCESS;
}

IDE_RC iduShmMgr::attachSeg( UInt aSegID, idBool aIsCleanPurpose )
{
    iduSCH * sNewSCH;
    IDE_RC   sRC;

    while(1)
    {
        if( mArrSegment[aSegID] == NULL )
        {
            if( aIsCleanPurpose == ID_TRUE )
            {
                if( mSSegment->mArrSegInfo[aSegID].mShmKey
                    == IDU_SHM_SHM_KEY_NULL )
                {
                    break;
                }
            }
            else
            {
                IDE_ASSERT( mSSegment->mArrSegInfo[aSegID].mShmKey
                            != IDU_SHM_SHM_KEY_NULL );
            }

            sRC = iduShmChunkMgr::attachChunk( mSSegment->mArrSegInfo[aSegID].mShmKey,
                                               mSSegment->mArrSegInfo[aSegID].mSize,
                                               &sNewSCH );

            if( aIsCleanPurpose == ID_FALSE )
            {
                IDE_TEST( sRC == IDE_FAILURE );
            }
            else
            {
                if( sRC == IDE_FAILURE )
                {
                    mSSegment->mArrSegInfo[aSegID].mShmKey = IDU_SHM_SHM_KEY_NULL;
                    break;
                }
            }

            registerOldSeg( sNewSCH, aSegID );
        }

        break;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
** TLSF main interface. Right out of the white paper.
*/
IDE_RC iduShmMgr::createSSegment( iduShmTxInfo * aShmTxInfo,
                                  UInt           aSize4DataSeg,
                                  ULong          aMaxSegCnt )
{
    ULong     sSSegmentSize;
    iduSCH  * sNewSCH;
    ULong     sAllocSlotIdx;

    sSSegmentSize = ID_SIZEOF(iduShmSSegment) +
        (aMaxSegCnt - 1) * ID_SIZEOF(iduStShmSegInfo);

    IDE_TEST( iduShmChunkMgr::createSysChunk( sSSegmentSize,
                                              &sNewSCH )
              != IDE_SUCCESS );

    mSSegment = (iduShmSSegment*)sNewSCH->mChunkPtr;

    mStartUpTime = idlOS::gettimeofday();

    IDE_TEST( iduShmKeyFile::setStartupInfo( &mStartUpTime ) != IDE_SUCCESS );

    initSSegment( sNewSCH->mShmKey,
                  aSize4DataSeg,
                  sSSegmentSize,
                  IDU_SHM_TYPE_SYSTEM_SEGMENT,
                  aMaxSegCnt,
                  mSSegment );

    IDE_TEST( registerNewSeg( aShmTxInfo,
                              sNewSCH,
                              &sAllocSlotIdx ) != IDE_SUCCESS );

    mSSegment->mNxtShmKey = sNewSCH->mShmKey - 1;

    IDE_ASSERT( sAllocSlotIdx == 0 );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::dropSSegment( idBool aIsCleanPurpose )
{
    UInt i;
    UInt sSegIdx = mSSegment->mHeader.mSegID;

    if( aIsCleanPurpose == ID_FALSE )
    {
        destSSegment( mSSegment );
    }

    for( i = 0; i < IDU_SHM_REMOVE_SHM_SEG_RETRY_TIMES; i++ )
    {
        if( iduShmChunkMgr::freeShmChunk( sSegIdx ) == IDE_SUCCESS )
        {
            break;
        }

        idlOS::sleep(1);
    }

    IDE_TEST( i == IDU_SHM_REMOVE_SHM_SEG_RETRY_TIMES );

    mArrSegment[0] = NULL;

    mAttachSegCnt--;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::registerNewSeg( iduShmTxInfo * aShmTxInfo,
                                  iduSCH       * aNewSCH,
                                  ULong        * aNewFreeSlotIdx )
{
    ULong sFreeSlotIdx = mSSegment->mSegCount;

    IDE_ASSERT( mSSegment->mSegCount < mSSegment->mMaxSegCount );

    IDE_TEST( iduVLogShmMgr::writeRegisterNewSeg( aShmTxInfo,
                                                  mSSegment->mSegCount,
                                                  mSSegment->mFstFreeSegInfoIdx )
              != IDE_SUCCESS );

    mSSegment->mArrSegInfo[sFreeSlotIdx].mShmKey = aNewSCH->mShmKey;
    mSSegment->mArrSegInfo[sFreeSlotIdx].mSize   = aNewSCH->mSize;

    mSSegment->mSegCount           = sFreeSlotIdx + 1;
    mSSegment->mFstFreeSegInfoIdx  = mSSegment->mSegCount;

    mArrSegment[sFreeSlotIdx] = (iduShmHeader*)aNewSCH->mChunkPtr;
    mAttachSegCnt++;

    iduShmChunkMgr::registerShmChunk( sFreeSlotIdx, aNewSCH );

    *aNewFreeSlotIdx = sFreeSlotIdx;


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aNewFreeSlotIdx = ID_UINT_MAX;

    return IDE_FAILURE;
}

void iduShmMgr::registerOldSeg( iduSCH * aNewSCH, ULong aSegID )
{
    IDE_ASSERT( aSegID < mSSegment->mSegCount );

    mArrSegment[aSegID] = (iduShmHeader*)aNewSCH->mChunkPtr;
    mAttachSegCnt++;

    iduShmChunkMgr::registerShmChunk( aSegID, aNewSCH );
}

IDE_RC iduShmMgr::createDSegment( idvSQL       * aStatistics,
                                  iduShmTxInfo * aShmTxInfo,
                                  iduShmType     aShmType,
                                  UInt           aChunkSize )
{
    iduShmHeader       * sShmSegHeadPtr;
    iduSCH             * sNewSCH;
    ULong                sFreeSlotIdx;
    iduShmBlockHeader  * sFstFreeBlock;
    UInt                 sState = 0;
    idrSVP               sSavepoint;
    // SHM_MAXSIZE에서 SYS SEGMENT의 크기는 제외한다.
    ULong                sMaxShmSize = iduProperty::getShmMaxSize();

    // BUG-36530
    IDE_TEST_RAISE( (mSSegment->mSegCount >= mSSegment->mMaxSegCount) ||
                    (sMaxShmSize < getTotalSegSize() + aChunkSize),
                    err_exceed_max_shmsize );

    IDE_TEST_RAISE( ( aChunkSize < IDU_SHM_BLOCK_SIZE_MIN ) ||
                    ( aChunkSize > IDU_SHM_BLOCK_SIZE_MAX ),
                    err_invalid_arg );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );
    sState = 1;

    IDE_TEST( iduShmChunkMgr::allocSHMChunk( aChunkSize,
                                             IDU_SHM_TYPE_DATA_SEGMENT,
                                             &sNewSCH )
              != IDE_SUCCESS );

    /* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
    if (mSSegment->mUID != mUID)
    {
        IDE_TEST( iduShmChunkMgr::changeSHMChunkOwner( sNewSCH->mShmID,
                                                       mSSegment->mUID )
                  != IDE_SUCCESS );
    }
    else
    {
        /* same user */
    }

    sShmSegHeadPtr = (iduShmHeader*)sNewSCH->mChunkPtr;

    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                  aShmTxInfo,
                                  &mSSegment->mLatch4AllocSeg )
              != IDE_SUCCESS );

    IDE_TEST( registerNewSeg( aShmTxInfo, sNewSCH, &sFreeSlotIdx )
              != IDE_SUCCESS );

    initShmHeader( sNewSCH->mShmKey,
                   sFreeSlotIdx,
                   aChunkSize,
                   aShmType,
                   sShmSegHeadPtr );

    IDE_TEST( iduVLogShmMgr::writeAllocSeg( aShmTxInfo,
                                            sNewSCH )
              != IDE_SUCCESS );

    IDE_TEST( initDataAreaOfSeg( aShmTxInfo, sShmSegHeadPtr, &sFstFreeBlock )
              != IDE_SUCCESS );

    IDE_TEST( insertBlock( aShmTxInfo, mSSegment, sFstFreeBlock )
              != IDE_SUCCESS );

    /* 새로운 Segment의 생성 및 등록이 완료되었기 때문에, 추후 Undo되는것을
     * 방지해야 한다. */
    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     aShmTxInfo,
                                     &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_arg );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_INVALID_SHARED_MEMORY_BLOCK_SIZE,
                                  aChunkSize ) );
    }
    IDE_EXCEPTION( err_exceed_max_shmsize );
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_Shm_Max_Size));
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::dropDSegment( iduShmHeader * aSegHeader )
{
    UInt i;
    UInt sSegIdx = aSegHeader->mSegID;

    for( i = 0; i < IDU_SHM_REMOVE_SHM_SEG_RETRY_TIMES; i++ )
    {
        if( iduShmChunkMgr::freeShmChunk( sSegIdx ) == IDE_SUCCESS )
        {
            break;
        }

        idlOS::sleep(1);
    }

    IDE_TEST( i == IDU_SHM_REMOVE_SHM_SEG_RETRY_TIMES );

    mArrSegment[sSegIdx] = NULL;

    IDE_ASSERT(( mAttachSegCnt != 1 ) &&  ( mAttachSegCnt != 0 ));

    mAttachSegCnt--;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::dropAllDSegment()
{
    ULong    i;
    iduSCH * sNewSCH;

    for( i = mSSegment->mSegCount - 1; i > 0 ; i-- )
    {
        if( mSSegment->mArrSegInfo[i].mShmKey !=
            IDU_SHM_SHM_KEY_NULL )
        {
            if( mArrSegment[i] == NULL )
            {
                IDE_TEST( iduShmChunkMgr::attachChunk(
                              mSSegment->mArrSegInfo[i].mShmKey,
                              mSSegment->mArrSegInfo[i].mSize,
                              &sNewSCH )
                          != IDE_SUCCESS );

                registerOldSeg( sNewSCH, i );
            }

            // xdb BUG-40305
            if( mArrSegment[i]->mSize != 0 )
            {
                IDE_TEST( dropDSegment( mArrSegment[i] ) != IDE_SUCCESS );
            }
        }

        mArrSegment[i] = NULL;
        mSSegment->mArrSegInfo[i].mShmKey = IDU_SHM_SHM_KEY_NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::initDataAreaOfSeg( iduShmTxInfo       * aShmTxInfo,
                                     iduShmHeader       * aNewSeg,
                                     iduShmBlockHeader ** aInitBlock )
{
    iduShmBlockHeader     * sNxtBlock = NULL;
    iduShmBlockHeader     * sInitBlock;
    UInt                    sOffset;
    UInt                    sBlockSize;

    sInitBlock = (iduShmBlockHeader*)alignUpPtr(
        (SChar*)aNewSeg + ID_SIZEOF(iduShmHeader),
        IDU_SHM_ALIGN_SIZE );

    sOffset    = (SChar*)sInitBlock - (SChar*)aNewSeg;
    sBlockSize = aNewSeg->mSize - sOffset - IDU_SHM_BLOCK_START_OFFSET -
        IDU_SHM_BLOCK_OVERHEAD * 2 /* Fst Block and Last Sentinel Block */;

    IDE_ASSERT( sBlockSize >= IDU_SHM_BLOCK_SIZE_MIN );

    initFstBlockHeader(
        sInitBlock,
        IDU_SHM_MAKE_SHM_ADDR(
            aNewSeg->mSegID,
            sOffset + IDU_SHM_ALLOC_BLOCK_HEADER_SIZE ), /* Self Addr */
        sBlockSize );

    IDE_DASSERT( (SChar*)sInitBlock ==
                 IDU_SHM_GET_BLOCK_HEADER_PTR( sInitBlock->mAddrSelf ) );

    IDE_TEST( setBlockFree( aShmTxInfo, sInitBlock )
              != IDE_SUCCESS );

    IDE_TEST( setPrevBlockUsed( aShmTxInfo, sInitBlock )
              != IDE_SUCCESS );

    /* Split the block to create a zero-size pool sentinel block. */
    IDE_TEST( linkAndGetBlockNext( aShmTxInfo, sInitBlock, &sNxtBlock )
              != IDE_SUCCESS );

    IDU_SHM_SET_BLOCK_SIZE( sNxtBlock, 0 );

    sNxtBlock->mAddrSelf = sInitBlock->mAddrSelf + sBlockSize + IDU_SHM_BLOCK_OVERHEAD;

    IDE_DASSERT( (SChar*)sNxtBlock ==
                 IDU_SHM_GET_BLOCK_HEADER_PTR( sNxtBlock->mAddrSelf ) );

    IDE_TEST( setBlockUsed( aShmTxInfo, sNxtBlock )
              != IDE_SUCCESS );

    IDE_TEST( setPrevBlockFree( aShmTxInfo, sNxtBlock )
              != IDE_SUCCESS );

    *aInitBlock = sInitBlock;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aInitBlock = NULL;

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::allocMem( idvSQL               * aStatistics,
                            iduShmTxInfo         * aShmTxInfo,
                            iduMemoryClientIndex  /* aIndex */,
                            UInt                   aSize,
                            void                ** aAllocPtr )
{
    UInt                 sAdjustSize;
    iduShmBlockHeader  * sBlock;
    SInt                 sState = 0;
    UInt                 sChunkSize;
    idrSVP               sSavepoint;

    IDE_ASSERT( aSize != 0 );
    IDE_ASSERT( aSize <= IDU_SHM_ADDR_MAX_BLOCK_SIZE );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );
    sState = 1;

    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                  aShmTxInfo,
                                  &mSSegment->mLatch4AllocBlk )
              != IDE_SUCCESS );

    IDE_DASSERT( validateShmArea() == IDE_SUCCESS );


    sAdjustSize = adjustRequestSize( aSize, IDU_SHM_ALIGN_SIZE );

    IDE_TEST_RAISE( ( sAdjustSize < IDU_SHM_BLOCK_SIZE_MIN ) ||
                    ( sAdjustSize > IDU_SHM_BLOCK_SIZE_MAX ),
                    err_invalid_arg );

    IDE_TEST( findFree( aShmTxInfo, mSSegment, sAdjustSize, &sBlock )
                  != IDE_SUCCESS );

    if( sBlock == NULL )
    {
        sChunkSize = getChunkSize( sAdjustSize );

        IDE_TEST( createDSegment( aStatistics,
                                  aShmTxInfo,
                                  IDU_SHM_TYPE_DATA_SEGMENT,
                                  sChunkSize )
                  != IDE_SUCCESS );

        IDE_TEST( findFree( aShmTxInfo, mSSegment, sAdjustSize, &sBlock )
                  != IDE_SUCCESS );

        IDE_ASSERT( sBlock != NULL );
    }

    IDE_TEST( prepareUsedBlock( aShmTxInfo, mSSegment, sBlock, sAdjustSize, aAllocPtr )
              != IDE_SUCCESS );

    IDE_DASSERT( validateBlock( sBlock ) == ID_TRUE );

    IDE_DASSERT( (SChar*)(*aAllocPtr) == IDU_SHM_GET_ADDR_PTR( getAddr(*aAllocPtr ) ) );

    IDE_DASSERT( validateFBMatrix() == ID_TRUE );


    IDE_TEST( iduVLogShmMgr::writeAllocMem( aShmTxInfo,
                                            sSavepoint.mLSN,
                                            sBlock )
              != IDE_SUCCESS );


    IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

    IDE_TEST( idrLogMgr::releaseLatch2Svp( aStatistics,
                                           aShmTxInfo,
                                           &sSavepoint )
              != IDE_SUCCESS );

    IDU_FIT_POINT( "iduShmMgr::allocMem::return" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_arg );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_INVALID_SHARED_MEMORY_BLOCK_SIZE,
                                  sAdjustSize ) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : AllocMem과 동일하지만 Undo를 하지 않는다.
 *               따라서 본 함수를 사용하다가 죽은 경우 Memory Leak이 발생할 수 있다.
 *
 * aStatistics   - [IN] Statistics
 * aShmTxInfo    - [IN] Shared Memory Tx
 * aIndex        - [IN] Client Index.
 * aSize         - [IN] Alloc될 Size
 * aAllocPtr     - [OUT] Alloc된 Memory를 반환함.
 ***********************************************************************/
IDE_RC iduShmMgr::allocMemWithoutUndo( idvSQL               * aStatistics,
                                       iduShmTxInfo         * aShmTxInfo,
                                       iduMemoryClientIndex  /* aIndex */,
                                       UInt                   aSize,
                                       void                ** aAllocPtr )
{
    UInt                 sAdjustSize;
    iduShmBlockHeader  * sBlock;
    SInt                 sState = 0;
    UInt                 sChunkSize;
    idrSVP               sSavepoint;

    IDE_ASSERT( aSize != 0 );
    IDE_ASSERT( aSize <= IDU_SHM_ADDR_MAX_BLOCK_SIZE );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );
    sState = 1;

    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                  aShmTxInfo,
                                  &mSSegment->mLatch4AllocBlk )
              != IDE_SUCCESS );

    IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

    sAdjustSize = adjustRequestSize( aSize, IDU_SHM_ALIGN_SIZE );

    IDE_TEST( findFree( aShmTxInfo, mSSegment, sAdjustSize, &sBlock )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( ( sAdjustSize < IDU_SHM_BLOCK_SIZE_MIN ) ||
                    ( sAdjustSize > IDU_SHM_BLOCK_SIZE_MAX ),
                    err_invalid_arg );

    if( sBlock == NULL )
    {
        sChunkSize = getChunkSize( sAdjustSize );

        IDE_TEST( createDSegment( aStatistics,
                                  aShmTxInfo,
                                  IDU_SHM_TYPE_DATA_SEGMENT,
                                  sChunkSize )
                  != IDE_SUCCESS );

        IDE_TEST( findFree( aShmTxInfo, mSSegment, sAdjustSize, &sBlock )
                  != IDE_SUCCESS );

        IDE_ASSERT( sBlock != NULL );
    }

    IDE_TEST( prepareUsedBlock( aShmTxInfo, mSSegment, sBlock, sAdjustSize, aAllocPtr )
              != IDE_SUCCESS );

    IDE_DASSERT( validateBlock( sBlock ) == ID_TRUE );

    IDE_DASSERT( (SChar*)(*aAllocPtr) == IDU_SHM_GET_ADDR_PTR( getAddr(*aAllocPtr ) ) );

    IDE_DASSERT( validateFBMatrix() == ID_TRUE );

    IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics,
                                     aShmTxInfo,
                                     &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_arg );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_INVALID_SHARED_MEMORY_BLOCK_SIZE,
                                  sAdjustSize ) );
    }
    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::allocAlignMem( idvSQL               * aStatistics,
                                 iduShmTxInfo         * aShmTxInfo,
                                 iduMemoryClientIndex   /* aIndex */,
                                 UInt                   aSize,
                                 UInt                   aAlignSize,
                                 void                ** aAllocPtr )
{
    UInt                sState = 0;
    UInt                sAdjustSize;
    UInt                sMinimumGap;
    UInt                sSizeWithGap;

    UInt                sAlignedSize;
    UInt                sGap;
    iduShmBlockHeader * sBlock;
    SChar             * sDataPtr;
    SChar             * sAlignedDataPtr;

    UInt                sRemainGap;
    UInt                sOffset;
    void              * sNextAligned;
    UInt                sChunkSize;
    idrSVP              sSavepoint;

    IDE_ASSERT( aAllocPtr != NULL );

    IDE_ASSERT( aSize != 0 );
    IDE_ASSERT( aSize <= IDU_SHM_BLOCK_SIZE_MAX );

    sAdjustSize = adjustRequestSize( aSize, IDU_SHM_ALIGN_SIZE );

    /*
    ** We must allocate an additional minimum block size bytes so that if
    ** our free block will leave an alignment gap which is smaller, we can
    ** trim a leading free block and release it back to the heap. We must
    ** do this because the previous physical block is in use, therefore
    ** the prev_phys_block field is not valid, and we can't simply adjust
    ** the size of that block.
    */
    sMinimumGap  = ID_SIZEOF(iduShmBlockHeader);
    sSizeWithGap =
        adjustRequestSize( sAdjustSize + aAlignSize + sMinimumGap,
                           aAlignSize );

    /* If alignment is less than or equals base alignment, we're done. */
    sAlignedSize =
        ( aAlignSize <= IDU_SHM_ALIGN_SIZE ) ? sAdjustSize : sSizeWithGap;

    IDE_ASSERT( sAlignedSize <= IDU_SHM_ADDR_MAX_BLOCK_SIZE );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );
    sState = 1;

    IDE_TEST( iduShmLatchAcquire( aStatistics,
                                  aShmTxInfo,
                                  &mSSegment->mLatch4AllocBlk )
              != IDE_SUCCESS );

    IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

    IDE_TEST( findFree( aShmTxInfo, mSSegment, sAlignedSize, &sBlock )
              != IDE_SUCCESS );

    /* 현재 sAlignedSize만큼의 공간이 없다면 */
    if( sBlock == NULL )
    {
        sChunkSize = getChunkSize( sAlignedSize );

        IDE_TEST( createDSegment( aStatistics,
                                  aShmTxInfo,
                                  IDU_SHM_TYPE_DATA_SEGMENT,
                                  sChunkSize )
                  != IDE_SUCCESS );

        IDE_TEST( findFree( aShmTxInfo, mSSegment, sAlignedSize, &sBlock  )
                  != IDE_SUCCESS );

        IDE_ASSERT( sBlock != NULL );
    }

    IDE_DASSERT( ID_SIZEOF(iduShmBlockHeader) ==
                 ( IDU_SHM_BLOCK_SIZE_MIN + IDU_SHM_BLOCK_OVERHEAD ) );

    sDataPtr = (SChar*)getDataPtr4Block( sBlock );
    sAlignedDataPtr = alignUpPtr( sDataPtr, aAlignSize );

    sGap = (ULong)sAlignedDataPtr - (ULong)sDataPtr;

    /* If gap size is too small, offset to next aligned boundary. */
    if( ( sGap != 0 ) && ( sGap < sMinimumGap ) )
    {
        sRemainGap = sMinimumGap - sGap;
        sOffset    = IDL_MAX( sRemainGap, aAlignSize );

        sNextAligned    = sAlignedDataPtr + sOffset;
        sAlignedDataPtr = alignUpPtr( sNextAligned, aAlignSize );

        sGap = sAlignedDataPtr - sDataPtr;
    }

    if( sGap != 0 )
    {
        IDE_ASSERT( sGap >= sMinimumGap && "Gap size too small");
        IDE_TEST( trimFreeLeading( aShmTxInfo, mSSegment, sBlock, sGap, &sBlock )
                  != IDE_SUCCESS );
    }

    IDE_TEST( prepareUsedBlock( aShmTxInfo, mSSegment, sBlock, sAlignedSize, aAllocPtr )
              != IDE_SUCCESS );

    IDE_DASSERT( validateFBMatrix() == ID_TRUE );

    IDE_TEST( iduVLogShmMgr::writeAllocMem( aShmTxInfo,
                                            sSavepoint.mLSN,
                                            sBlock )
              != IDE_SUCCESS );

    IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

    IDE_TEST( idrLogMgr::releaseLatch2Svp( aStatistics,
                                           aShmTxInfo,
                                           &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                          aShmTxInfo,
                                          &sSavepoint )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

IDE_RC iduShmMgr::freeMem( idvSQL       * aStatistics,
                           iduShmTxInfo * aShmTxInfo,
                           idrSVP       * aSavepoint,
                           void         * aPtr )
{
    iduShmBlockHeader * sBlock;
    idrSVP              sSavepoint;
    SInt                sState = 0;

    /* Don't attempt to free a NULL pointer. */
    if( aPtr != NULL )
    {
        idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );
        sState = 1;

        if( aSavepoint == NULL )
        {
            aSavepoint = &sSavepoint;
        }

        IDE_TEST( iduShmLatchAcquire( aStatistics,
                                      aShmTxInfo,
                                      &mSSegment->mLatch4AllocBlk )
                  != IDE_SUCCESS );

        IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

        sBlock = getBlockFromDataPtr( aPtr );

#ifdef MEMORY_ASSERT
        {
            iduShmBlockFooter *sFooter =
                (iduShmBlockFooter*)getNextBlock( sBlock ) - 1;
            if ( sFooter->mFence != gFence )
            {
                ideLog::log( IDE_ERR_0,
                             "*** Buffer overrun detected in iduShmMgr::"
                             "freeMem().  (mAddrSelf: " IDU_SHM_ADDR_FMT
                             ", mSize: %u)\n",
                             IDU_SHM_ADDR_ARGS( sBlock->mAddrSelf ),
                             IDU_SHM_GET_BLOCK_SIZE( sBlock ) );
                IDE_ASSERT( 0 && "buffer overrun" );
            }
        }
#endif

        IDU_SHM_VALIDATE_ADDR_PTR( sBlock->mAddrSelf, aPtr );

        IDE_ASSERT( isBlockFree( sBlock ) == ID_FALSE );

        IDE_TEST( setBlkAndPrvOfNxtBlkFree( aShmTxInfo, sBlock )
                  != IDE_SUCCESS );


        IDE_TEST( mergePrev( aShmTxInfo, mSSegment, sBlock, &sBlock )
                  != IDE_SUCCESS );

        IDE_TEST( mergeNext( aShmTxInfo, mSSegment, sBlock, &sBlock )
                  != IDE_SUCCESS );

        IDE_TEST( insertBlock( aShmTxInfo, mSSegment, sBlock )
                  != IDE_SUCCESS );

        IDE_DASSERT( validateFBMatrix() == ID_TRUE );

        IDE_DASSERT( validateShmArea() == IDE_SUCCESS );

        IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, aSavepoint )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint ) == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

UInt iduShmMgr::getBlockSize( void* aDataPtr )
{
    UInt                 sBlockSize  = 0;
    iduShmBlockHeader  * sBlock      = NULL;

    if( aDataPtr != NULL )
    {
        sBlock      = getBlockFromDataPtr( aDataPtr );
        sBlockSize  = IDU_SHM_GET_BLOCK_SIZE( sBlock );
    }

    return sBlockSize;
}

iduShmBlockHeader* iduShmMgr::getBlockPtrByAddr( idShmAddr aBlockAddr )
{
    IDE_ASSERT( aBlockAddr != IDU_SHM_NULL_ADDR );

    return getBlockFromDataPtr( getPtrOfAddr( aBlockAddr ) );
}

/*
** Overhead of the TLSF structures in a given memory block passed to
** tlsf_create, equal to the size of a pool_t plus overhead of the initial
** free block and the sentinel block.
*/
UInt iduShmMgr::getOverhead()
{
    const UInt sOverhead = ID_SIZEOF(iduShmHeader) +
        2 * IDU_SHM_BLOCK_OVERHEAD;

    return sOverhead;
}

idShmAddr iduShmMgr::getAddr( void* aAllocPtr )
{
    iduShmBlockHeader *sBlock = getBlockFromDataPtr( aAllocPtr );
    return sBlock->mAddrSelf;
}

/*
** The TLSF block information provides us with enough information to
** provide a reasonably intelligent implementation of realloc, growing or
** shrinking the currently allocated block as required.
**
** This routine handles the somewhat esoteric edge cases of realloc:
** - a non-zero size with a null pointer will behave like malloc
** - a zero size with a non-null pointer will behave like free
** - a request that cannot be satisfied will leave the original buffer
**   untouched
** - an extended buffer size will leave the newly-allocated area with
**   contents undefined
*/
IDE_RC iduShmMgr::reallocMem( idvSQL              * aStatistics,
                              iduShmTxInfo        * aShmTxInfo,
                              iduMemoryClientIndex  aIndex,
                              void                * aPtr,
                              UInt                  aSize,
                              void               ** aAllocPtr )
{
    iduShmBlockHeader * sCurBlock;
    iduShmBlockHeader * sNxtBlock;
    UInt                sCurSize;
    UInt                sCombineSize;
    UInt                sAdjustSize;
    idrSVP              sSavepoint;
    UInt                sState = 0;

    void  * sAllocPtr = NULL;

    IDE_ASSERT( aSize != 0 );
    IDE_ASSERT( aSize <= IDU_SHM_BLOCK_SIZE_MAX );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );
    sState = 1;

    /* Zero-size requests are treated as free. */
    if( ( aPtr != NULL ) && ( aSize == 0 ) )
    {
        IDE_TEST( freeMem( aStatistics, aShmTxInfo, &sSavepoint, aPtr )
                  != IDE_SUCCESS );
    }
    else
    {
        /* Requests with NULL pointers are treated as malloc. */
        if( aPtr == NULL )
        {
            IDE_TEST( allocMem( aStatistics, aShmTxInfo, aIndex, aSize, &sAllocPtr )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( iduShmLatchAcquire( aStatistics, aShmTxInfo, &mSSegment->mLatch4AllocBlk )
                      != IDE_SUCCESS );
            sState = 1;

            sCurBlock = getBlockFromDataPtr( aPtr );
            sNxtBlock = getNextBlock( sCurBlock );

            sCurSize     = IDU_SHM_GET_BLOCK_SIZE( sCurBlock );
            sCombineSize = sCurSize + IDU_SHM_GET_BLOCK_SIZE( sNxtBlock )
                + IDU_SHM_BLOCK_OVERHEAD;
            sAdjustSize  = adjustRequestSize( aSize, IDU_SHM_ALIGN_SIZE );

            /*
            ** If the next block is used, or when combined with the current
            ** block, does not offer enough space, we must reallocate and copy.
            */
            if( ( sAdjustSize > sCurSize ) &&
                ( ( isBlockFree( sNxtBlock ) == ID_FALSE ) ||
                  ( sAdjustSize > sCombineSize ) ) )
            {
                sState = 0;
                IDE_TEST( iduShmLatchRelease( aShmTxInfo,
                                              &mSSegment->mLatch4AllocBlk )
                          != IDE_SUCCESS );

                IDE_TEST( allocMem( aStatistics, aShmTxInfo, aIndex, aSize, &sAllocPtr )
                          != IDE_SUCCESS );

                idlOS::memcpy( sAllocPtr, aPtr, IDL_MIN( sCurSize, aSize ));

                IDE_TEST( freeMem( aStatistics, aShmTxInfo, &sSavepoint, aPtr )
                          != IDE_SUCCESS );
            }
            else
            {
                /* Do we need to expand to the next block? */
                if( sAdjustSize > sCurSize )
                {
                    IDE_TEST( mergeNext( aShmTxInfo, mSSegment, sCurBlock, &sCurBlock )
                              != IDE_SUCCESS );

                    IDE_TEST( setBlkUsedAndPrvOfNxtBlkUsed( aShmTxInfo, sCurBlock )
                              != IDE_SUCCESS );
                }

                /* Trim the resulting block and return the original pointer. */
                IDE_TEST( trimUsedBlock( aShmTxInfo, mSSegment, sCurBlock, sAdjustSize )
                          != IDE_SUCCESS );
                sAllocPtr = aPtr;

                IDE_DASSERT( validateFBMatrix() == ID_TRUE );
            }
        }
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSavepoint )
              != IDE_SUCCESS );

    *aAllocPtr = sAllocPtr;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_PUSH();
        IDE_ASSERT( iduShmLatchRelease( aShmTxInfo, &mSSegment->mLatch4AllocBlk )
                    == IDE_SUCCESS );
        IDE_POP();
    }

    return IDE_FAILURE;
}

void iduShmMgr::initSSegment( key_t             aShmKey,
                              UInt              aSegSize,
                              UInt              aSysSegSize,
                              iduShmType        aShmType,
                              ULong             aMaxSegCnt,
                              iduShmSSegment  * aShmSSegment )
{
    UInt        i, j;
    idShmAddr   sAddrObj;

    initShmHeader( aShmKey,
                   0 /* SegID */,
                   aSysSegSize,
                   aShmType,
                   &aShmSSegment->mHeader );

    for( i = 0; i < IDU_SHM_META_MAX_COUNT; i++ )
    {
        aShmSSegment->mArrMetaBlockAddr[i] = IDU_SHM_NULL_ADDR;
    }

    aShmSSegment->mStatistics.mTotalSize    = 0;
    aShmSSegment->mStatistics.mAllocSize    = 0;
    aShmSSegment->mStatistics.mFreeSize     = 0;
    aShmSSegment->mStatistics.mFreeBlkCnt   = 0;
    aShmSSegment->mStatistics.mAllocBlkCnt  = 0;
    aShmSSegment->mStatistics.mAllocReqCnt  = 0;
    aShmSSegment->mStatistics.mFreeReqCnt   = 0;

    sAddrObj = IDU_SHM_GET_ADDR_SUBMEMBER( 0, /* Parent Addr */
                                           iduShmSSegment,
                                           mLatch4AllocBlk );

    iduShmLatchInit( sAddrObj,
                     IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                     &aShmSSegment->mLatch4AllocBlk );

    aShmSSegment->mFstLvlBitmap = 0;

    for( i = 0; i < IDU_SHM_FST_LVL_INDEX_COUNT; i++ )
    {
        aShmSSegment->mSndLvlBitmap[i] = 0;

        for( j = 0; j < IDU_SHM_SND_LVL_INDEX_COUNT; j++ )
        {
            aShmSSegment->mFBMatrix[i][j] = IDU_SHM_NULL_ADDR;
        }
    }

    sAddrObj = IDU_SHM_GET_ADDR_SUBMEMBER( 0, /* Parent Addr */
                                           iduShmSSegment,
                                           mLatch4AllocSeg );

    iduShmLatchInit( sAddrObj,
                     IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                     &aShmSSegment->mLatch4AllocSeg );

    aShmSSegment->mSegCount    = 0;
    aShmSSegment->mSegSize     = aSegSize;
    aShmSSegment->mMaxSegCount = aMaxSegCnt;
    aShmSSegment->mNxtShmKey   = IDU_SHM_SHM_KEY_NULL;
    aShmSSegment->mFstFreeSegInfoIdx = 0;

    /* BUG-40895 Cannot destroy the shared memory segments which is created as other user */
    aShmSSegment->mUID         = idlOS::getuid();

    aShmSSegment->mIsNormalShutdown = ID_FALSE;

    initBlockHeader(
        &aShmSSegment->mNullBlock,
        IDU_SHM_MAKE_SHM_ADDR( 0,
                               offsetof( iduShmSSegment, mNullBlock) ),
        0 /* Size of Block */ );

    sAddrObj = IDU_SHM_GET_ADDR_SUBMEMBER( 0, /* Parent Addr */
                                           iduShmSSegment,
                                           mProcMgrInfo );

    aShmSSegment->mProcMgrInfo.mAddrSelf   = sAddrObj;
    aShmSSegment->mProcMgrInfo.mCurProcCnt = 0;
    aShmSSegment->mProcMgrInfo.mFlag       = IDU_SHM_PROC_REGISTER_ON;

    idlOS::memset( aShmSSegment->mArrSegInfo,
                   0,
                   ID_SIZEOF(iduStShmSegInfo) * aMaxSegCnt );
}

void iduShmMgr::destSSegment( iduShmSSegment * aShmSSegment )
{
    iduShmLatchDest( &aShmSSegment->mLatch4AllocBlk );
    iduShmLatchDest( &aShmSSegment->mLatch4AllocSeg );
}

IDE_RC iduShmMgr::validateShmChunk( iduShmHeader * aShmHeader )
{
    UInt sPrevShmKey;
    UInt sVersion4Shm;
    UInt sVersion4ShmMgr;

    sVersion4Shm    = aShmHeader->mVersionID & IDU_CHECK_VERSION_MASK;

    sVersion4ShmMgr = iduShmVersionID & IDU_CHECK_VERSION_MASK;

    IDE_TEST_RAISE( sVersion4Shm != sVersion4ShmMgr, err_mismatch_version );

    IDE_TEST( iduShmKeyFile::getStartupInfo( &mStartUpTime, &sPrevShmKey )
              != IDE_SUCCESS );

    // 공유 메모리가 생성된 시간과 ShmKey가 같아야 함
    if ( ( sPrevShmKey != iduProperty::getShmDBKey() ) ||
         ( idlOS::memcmp( &aShmHeader->mStartUpTime,
                          &mStartUpTime,
                          ID_SIZEOF( struct timeval ) ) != 0 ) )
    {
        IDE_RAISE( err_not_shm_owner );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( err_mismatch_version );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_UNCOMPATIBLE_SHARED_MEMORY,
                                  aShmHeader->mVersion,
                                  iduShmVersionString ) );
    }
    IDE_EXCEPTION( err_not_shm_owner );
    {
        IDE_SET( ideSetErrorCode( idERR_FATAL_NOT_OWNER_OF_SHARED_MEMORY,
                                  aShmHeader->mShmKey ) );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void iduShmMgr::cleanAllShmArea( iduShmProcType aProcType, UInt aShmDBKey )
{
    if ( attach4Clean( aProcType, aShmDBKey ) == IDE_SUCCESS )
    {
        (void)dropAllDSegment();
        (void)dropSSegment( ID_TRUE /* aIsCleanPurpose */ );
    }
    mAttachSegCnt = 0;
}

IDE_RC iduShmMgr::validateShmArea()
{
    iduShmSSegment    * sSSegment;
    iduShmBlockHeader * sShmBlkHdr;
    idShmAddr           sLoopAddr;

    sSSegment = iduShmMgr::mSSegment;

    for( UInt i = 0 ; i < IDU_SHM_FST_LVL_INDEX_COUNT ; i++ )
    {
        for( UInt j = 0 ; j < IDU_SHM_SND_LVL_INDEX_COUNT ; j++ )
        {
            sLoopAddr = sSSegment->mFBMatrix[i][j];
            while( sLoopAddr != IDU_SHM_NULL_ADDR )
            {
                sShmBlkHdr = (iduShmBlockHeader*)
                    iduShmMgr::getBlockPtrByAddr( sLoopAddr );

                IDE_ASSERT( sShmBlkHdr->mAddrSelf == sLoopAddr );

                sLoopAddr = sShmBlkHdr->mFreeList.mAddrNext;
            }
        }
    }

    return IDE_SUCCESS;
}

IDE_RC iduShmMgr::validateShmMgr( idvSQL       * aStatistics,
                                  iduShmTxInfo * aShmTxInfo )
{
    idrSVP sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    IDE_TEST( iduShmLatchAcquire( aStatistics, aShmTxInfo, &mSSegment->mLatch4AllocBlk )
              != IDE_SUCCESS );

    IDE_TEST( validateShmArea() != IDE_SUCCESS );

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}


/*****************************************************************************
 * Description : 모든 Data Segment를 제거한다.
 *               attach에 실패할 경우 해당 shmKey는 NULL로 세팅되고 skip한다.
 * [IN] aSSegment   - Attach된 System Segment
 *****************************************************************************/
IDE_RC iduShmMgr::dropAllDSegmentAndPrint( iduShmSSegment  * aSSegment )
{
    ULong    i;
    iduSCH   sNewSCH;
    IDE_RC   sRC;

    idlOS::printf("Destroy Data Segment....\n");

    for( i = aSSegment->mSegCount - 1; i > 0 ; i-- )
    {
        if( aSSegment->mArrSegInfo[i].mShmKey != IDU_SHM_SHM_KEY_NULL )
        {
            sRC = iduShmChunkMgr::attachChunk( aSSegment->mArrSegInfo[i].mShmKey,
                                               aSSegment->mArrSegInfo[i].mSize,
                                               &sNewSCH );

            if( sRC != IDE_SUCCESS )
            {
                idlOS::printf( "Unable to attach the shared memory."
                               "\tShmKey  = %d\n",
                               aSSegment->mArrSegInfo[i].mShmKey );
            }
            else
            {
                idlOS::printf( "\tShmKey  = %d"
                               "\tShmSize = %u\n",
                               aSSegment->mArrSegInfo[i].mShmKey,
                               aSSegment->mArrSegInfo[i].mSize );

                IDE_TEST( iduShmChunkMgr::removeChunck( (SChar*)sNewSCH.mChunkPtr,
                                                        sNewSCH.mShmID )
                          != IDE_SUCCESS );
            }
        }

        aSSegment->mArrSegInfo[i].mShmKey = IDU_SHM_SHM_KEY_NULL;
    }

    idlOS::printf("Destroy Data Segment.... Complete!!!\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*****************************************************************************
 * Description : copy된 SSegment정보를 이용하여 SSegment에 Attach하고
 *               Data & System Segment를 제거한다.
 *
 * [IN] aSSegment  - System Segment의 Copy본이 전달된다.(Attach된 SSegment가 아님)
 *
 **************************************************************************/
IDE_RC iduShmMgr::dropAllSegmentAndPrint( iduShmSSegment * aSSegment )
{
    iduSCH           sNewSCH;
    iduShmSSegment * sSysSegment;

    IDE_ASSERT( aSSegment->mArrSegInfo[0].mShmKey == aSSegment->mHeader.mShmKey );
    IDE_ASSERT( aSSegment->mArrSegInfo[0].mSize == aSSegment->mHeader.mSize );

    // Attach System Segment
    IDE_TEST( iduShmChunkMgr::attachChunk(
                    aSSegment->mArrSegInfo[0].mShmKey,
                    aSSegment->mArrSegInfo[0].mSize,
                    &sNewSCH )
              != IDE_SUCCESS );

    sSysSegment = (iduShmSSegment *)sNewSCH.mChunkPtr;

    // Drop All Data Segment
    IDE_TEST( dropAllDSegmentAndPrint( sSysSegment )
              != IDE_SUCCESS );

    // Drop System Segment
    idlOS::printf("Destroy System Segment....\n");

    idlOS::printf( "\tShmKey     = %d\n"
                   "\tShmSize    = %u\n",
                   aSSegment->mHeader.mShmKey,
                   aSSegment->mHeader.mSize );

    IDE_TEST( iduShmChunkMgr::removeChunck( (SChar*)sSysSegment,
                                            sNewSCH.mShmID )
              != IDE_SUCCESS );

    idlOS::printf("Destroy System Segment.... Complete!!!\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*************************************************************************
 * Description : ShmUtil에서 Shared Memory를 제거하면 SSegment에 있던 PMMgr이
 *               free되므로 Destroy되어야 한다. (Detach 불가능)
 ************************************************************************/
IDE_RC iduShmMgr::eraseShmAreaAndPrint()
{
    iduShmSSegment * sSSegHdr;
    UInt             sState = 0;

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_MML_MAIN,
                                 ID_SIZEOF(iduShmSSegment),
                                 (void**)&sSSegHdr )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( iduShmChunkMgr::attachAndGetSSegHdr( sSSegHdr )
              != IDE_SUCCESS );

    IDE_TEST( dropAllSegmentAndPrint( sSSegHdr )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free( sSSegHdr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState != 0 )
    {
        IDE_ASSERT( iduMemMgr::free( sSSegHdr ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


UInt iduShmMgr::getMaxBlockSize()
{
    UInt sMaxBlockSize4Chunk;
    UInt sMaxBlockSize;
    UInt sAlignUpSize;
    UInt sFstLvlIdx;
    UInt sSndLvlIdx;

    UInt sChunkSize = iduProperty::getShmChunkSize();

    sMaxBlockSize4Chunk = getMaxBlockSize4Chunk( sChunkSize );

    getMatrixIdx4Ins( sMaxBlockSize4Chunk, &sFstLvlIdx, &sSndLvlIdx );

    IDE_ASSERT( sFstLvlIdx > 1 );

    if( sSndLvlIdx == 0 )
    {
        sFstLvlIdx   = sFstLvlIdx - 1;
        sSndLvlIdx   = sFstLvlIdx - IDU_SHM_SND_LVL_IDX_COUNT_LOG2;
    }

    sAlignUpSize = ( 1 << ( sFstLvlIdx -
                            IDU_SHM_SND_LVL_IDX_COUNT_LOG2 ) ) - 1;

    sMaxBlockSize = ( 1 << ( sFstLvlIdx + IDU_SHM_FST_LVL_INDEX_SHIFT - 1 ) )
        + ( sSndLvlIdx - 1 ) * sAlignUpSize;

    return sMaxBlockSize;
}

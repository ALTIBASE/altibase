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
 
/***********************************************************************
 * $Id$
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <idErrorCode.h>
#include <iduVLogShmMemPool.h>
#include <ideMsgLog.h>

/*
 * 해당 mem pool이 mem pool mgr에 의해서 관리되어질수 있는가?
 */
#define IDU_ADD_OR_DEL_MEMPOOL ( ( iduMemMgr::isServerMemType() == ID_TRUE )   && \
                                 ( mUseMutex == ID_TRUE) )

/* ------------------------------------------------

   MemPool 간략한 설명 ( 2005.3.30 by lons )


   개요
   mempool은 매번 malloc system call을 호출하지 않고
   memory를 할당하기 위한 목적을 가진 클래스이다.
   하나의 mempool에서 할당해 줄 수 있는 메모리의 크기는 항상 고정된다.
   여러 메모리 크기를 할당하고자 한다면 다른 크기로
   init된 mempool을 2개 이상 만들어야 한다.


   구성요소( class )
   iduShmMemPool : mem list를 관리한다. 하나의 mem pool에서 mem list는
   다수개일 수 있다. 이것은 system cpu 개수와 관계있음.
   iduShmMemList : 실제 memory를 할당하여 확보하고 있는 list
   memChunk의 리스트로 구성된다.
   iduStShmMemChunk : scuMemSlot list를 유지한다. mempool에서 할당해줄
   메모리가 모자랄때마다 하나씩 추가된다.
   scuMemSlot의 free count, max count를 유지한다.
   scuMemSlot : 실제 할당해줄 메모리

   -------      -------
   | chunk | -  | chunk | - ...
   -------      -------
   |
   ---------      ---------
   | element |  - | element | - ....
   ---------      ---------

   (eg.)

   iduShmMemPool::initialize( cpu개수, ID_SIZEOF(UInt), 5 )을 수행하면
   하나의 chunk와 5개의 element가 생긴다.
   element의 개수는 몇개 만큼의 메모리를 미리 할당하느냐를 결정한다.

   -------
   | chunk |
   -------
   |
   ---------      ---------
   | element |  - | element | - ...
   ---------      ---------

   iduShmMemPool::alloc을 수행하면
   element 하나가 return 된다.
   만약 element가 부족하면 chunk 하나와 element 5개를 더 할당한다.
   * ----------------------------------------------*/
/*-----------------------------------------------------------
 * Description: iduShmMemPool을 초기화 한다.
 *
 * aIndex            - [IN] 이 함수를 호출한 모듈
 *                        (어디서 호출하는지 정보를 유지하기 위해 필요)
 * aName             - [IN] iduShmMemPool의 식별 이름
 * aListCount
 * aElemSize         - [IN] 할당할 메모리의 단위크기
 * aElemCount        - [IN] 한 chunk내의 momory element의 개수,
 *                          즉, chunk내의 slot의 개수
 * aChunkLimit       - [IN] memPool에서 유지할 chunk개수의 최대치.
 *             (단 모든 chunk가 다 할당중인 경우에는 이 개수에 포함되지 않는다.)
 * aUseLatch         - [IN] mutex사용 여부
 * aAlignByte        - [IN] aline되어 있는 메모리를 할당받고 싶을때, 원하는 align
 *                          값을 넣어서 함수를 호출한다.
 * aForcePooling     - [IN] property에서 mem pool을 사용하지 않도록 설정했더라도
 *                          강제로 생성.(BUG-21547)
 * aGarbageCollection - [IN] Garbage collection에 기여하도록 할것인가 결정.
 *                           ID_TRUE일때 mem pool manager에 의해서 관리되어짐
 * ---------------------------------------------------------*/
IDE_RC iduShmMemPool::initialize( idvSQL              * aStatistics,
                                  iduShmTxInfo        * aShmTxInfo,
                                  iduMemoryClientIndex  aIndex,
                                  SChar               * aName,
                                  UInt                  aListCount,
                                  vULong                aElemSize,
                                  vULong                aElemCount,
                                  vULong                aChunkLimit,
                                  idBool                aUseLatch,
                                  UInt                  aAlignByte,
                                  idBool                aGarbageCollection,
                                  idShmAddr             aAddr4MemPool,
                                  iduStShmMemPool     * aMemPoolInfo )
{
    UInt              i = 0;
    idShmAddr         sAddr4MemLst;
    iduStShmMemList * sArrMemListInfo;
    idrSVP            sSavepoint;

    IDE_ASSERT( aName != NULL );

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    IDE_TEST_RAISE( aElemCount == 0 , err_invalid_chunksize );

    aMemPoolInfo->mAddrSelf = aAddr4MemPool;

    idlOS::snprintf( aMemPoolInfo->mName,
                     IDU_SHM_MEM_POOL_NAME_LEN,
                     "%s",
                     aName );

    IDU_SHM_VALIDATE_ADDR_PTR( aAddr4MemPool, aMemPoolInfo );

    iduShmLatchInit( IDU_SHM_GET_ADDR_SUBMEMBER( aAddr4MemPool,
                                                 iduStShmMemPool,
                                                 mLatch),
                     IDU_SHM_LATCH_SPIN_COUNT_DEFAULT,
                     &aMemPoolInfo->mLatch );

    IDU_SHM_VALIDATE_ADDR_PTR( aMemPoolInfo->mLatch.mAddrSelf,
                               &aMemPoolInfo->mLatch );

    aMemPoolInfo->mUseMutex  = aUseLatch;
    aMemPoolInfo->mListCount = aListCount;
    aMemPoolInfo->mIndex     = aIndex;
    aMemPoolInfo->mAllocIdx  = 0;

    aMemPoolInfo->mGarbageCollection = aGarbageCollection;
    aMemPoolInfo->mAddrNxtPool       = IDU_SHM_NULL_ADDR;

    IDE_TEST( iduShmMgr::allocMem( aStatistics,
                                   aShmTxInfo,
                                   aIndex,
                                   aMemPoolInfo->mListCount * ID_SIZEOF(iduStShmMemList),
                                   (void**)&sArrMemListInfo )
              != IDE_SUCCESS );


    aMemPoolInfo->mAddrArrMemList = iduShmMgr::getAddr( (void*)sArrMemListInfo );

    sAddr4MemLst = aMemPoolInfo->mAddrArrMemList;

    for( i = 0; i < aMemPoolInfo->mListCount; i++ )
    {
        IDE_TEST( iduShmMemList::initialize(
                      aStatistics,
                      aShmTxInfo,
                      aMemPoolInfo->mIndex,
                      i,
                      aName,
                      aElemSize,
                      aElemCount,
                      aChunkLimit,
                      aUseLatch,
                      aAlignByte,
                      sAddr4MemLst + i * ID_SIZEOF(iduStShmMemList),
                      sArrMemListInfo + i )
                  != IDE_SUCCESS );
    }

    IDE_TEST( iduVLogShmMemPool::writeInitialize( aShmTxInfo,
                                                  sSavepoint.mLSN,
                                                  aMemPoolInfo )
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION( err_invalid_chunksize );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_idaNullValue ) );
    }
    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC iduShmMemPool::destroy( idvSQL          * aStatistics,
                               iduShmTxInfo    * aShmTxInfo,
                               idrSVP          * aSavepoint,
                               iduStShmMemPool * aMemPoolInfo,
                               idBool            aCheck )
{
    iduStShmMemList * sArrMemListInfo;
    idrSVP            sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    if( aSavepoint == NULL )
    {
        aSavepoint = &sSavepoint;
    }

    sArrMemListInfo =
        (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR_CHECK( aMemPoolInfo->mAddrArrMemList );

    while( aMemPoolInfo->mListCount != 0 )
    {
        IDE_TEST( iduVLogShmMemPool::writeSetListCnt( aShmTxInfo,
                                                      aMemPoolInfo )
                  != IDE_SUCCESS );

        aMemPoolInfo->mListCount--;


        IDE_TEST( iduShmMemList::destroy( aStatistics,
                                          aShmTxInfo,
                                          aSavepoint,
                                          sArrMemListInfo + aMemPoolInfo->mListCount,
                                          aCheck )
                  != IDE_SUCCESS );
    }

    if( aMemPoolInfo->mAddrArrMemList != IDU_SHM_NULL_ADDR )
    {
        IDE_TEST( iduVLogShmMemPool::writeSetArrMemList( aShmTxInfo,
                                                         aMemPoolInfo )
                  != IDE_SUCCESS );

        aMemPoolInfo->mAddrArrMemList = IDU_SHM_NULL_ADDR;


        IDE_TEST( iduShmMgr::freeMem( aStatistics,
                                      aShmTxInfo,
                                      aSavepoint,
                                      sArrMemListInfo ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                      aShmTxInfo,
                                      &sSavepoint ) == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC iduShmMemPool::cralloc( idvSQL          * aStatistics,
                               iduShmTxInfo    * aShmTxInfo,
                               iduStShmMemPool * aMemPoolInfo,
                               idShmAddr       * aAddr4Mem,
                               void           ** aMem )
{
    SInt              i = 0;
    iduStShmMemList * sCurMemListInfo;
    idrSVP            sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    i  = idCore::acpAtomicInc32( &aMemPoolInfo->mAllocIdx )
         % aMemPoolInfo->mListCount;

    sCurMemListInfo = getArrMemList( aMemPoolInfo ) + i;

    if( aMemPoolInfo->mUseMutex == ID_TRUE )
    {
        iduShmMemList::lock( aStatistics, aShmTxInfo, sCurMemListInfo );

        IDE_TEST( iduShmMemList::cralloc( aStatistics,
                                          aShmTxInfo,
                                          sCurMemListInfo,
                                          aAddr4Mem,
                                          aMem )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( iduShmMemList::cralloc( aStatistics,
                                          aShmTxInfo,
                                          sCurMemListInfo,
                                          aAddr4Mem,
                                          aMem )
                  != IDE_SUCCESS );
    }

    IDE_TEST( idrLogMgr::releaseLatch2Svp( aStatistics,
                                           aShmTxInfo,
                                           &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics,
                                      aShmTxInfo,
                                      &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC iduShmMemPool::alloc( idvSQL          * aStatistics,
                             iduShmTxInfo    * aShmTxInfo,
                             iduStShmMemPool * aMemPoolInfo,
                             idShmAddr       * aAddr4Mem,
                             void           ** aMem )
{
    SInt              i;
    iduStShmMemList * sMemList;
    idrSVP            sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    // fix BUG-21547
    //i  = idlOS::getParallelIndex() % aMemPoolInfo->mListCount;
    i  = idCore::acpAtomicInc32( &aMemPoolInfo->mAllocIdx ) 
         % aMemPoolInfo->mListCount;

    sMemList = getArrMemList( aMemPoolInfo ) + i;

    if( aMemPoolInfo->mUseMutex == ID_TRUE )
    {
        iduShmMemList::lock( aStatistics, aShmTxInfo, sMemList );

        IDE_TEST( iduShmMemList::alloc( aStatistics, aShmTxInfo, sMemList, aAddr4Mem, aMem )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( iduShmMemList::alloc( aStatistics, aShmTxInfo, sMemList, aAddr4Mem, aMem )
                  != IDE_SUCCESS );
    }


    IDE_TEST( idrLogMgr::releaseLatch2Svp( aStatistics, aShmTxInfo, &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

IDE_RC iduShmMemPool::memfree( idvSQL          * aStatistics,
                               iduShmTxInfo    * aShmTxInfo,
                               idrSVP          * aSavepoint,
                               iduStShmMemPool * aMemPoolInfo,
                               idShmAddr         aAddr4Mem,
                               void            * aMem )
{
    iduStShmMemList    * sCurListInfo = NULL;
    iduStShmMemChunk   * sCurChunk    = NULL;
    idrSVP               sSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    if( aSavepoint == NULL )
    {
        aSavepoint = &sSavepoint;
    }

    sCurListInfo = getArrMemList( aMemPoolInfo );

    // fix BUG-21547
    // Memory Check Option에서는 이 과정을 생략해야 함.
    sCurChunk = (iduStShmMemChunk*)IDU_SHM_GET_ADDR_PTR_CHECK(
        *((idShmAddr *)((UChar *)aMem + sCurListInfo->mElemSize)) );

    sCurListInfo = (iduStShmMemList*)
        IDU_SHM_GET_ADDR_PTR_CHECK( sCurChunk->mAddrParent );

    if( aMemPoolInfo->mUseMutex == ID_TRUE )
    {
        iduShmMemList::lock( aStatistics, aShmTxInfo, sCurListInfo );

        IDE_TEST( iduShmMemList::memfree( aStatistics,
                                          aShmTxInfo,
                                          aSavepoint,
                                          sCurListInfo,
                                          aAddr4Mem,
                                          aMem )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( iduShmMemList::memfree( aStatistics,
                                          aShmTxInfo,
                                          aSavepoint,
                                          sCurListInfo,
                                          aAddr4Mem,
                                          aMem )
                  != IDE_SUCCESS );
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, aSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

/*
 * PROJ-2065 Garbage collection
 *
 * 사용하지 않는 free chunk들을 OS에 반납하여 메모리공간을 확보한다.
 */
IDE_RC iduShmMemPool::shrink( idvSQL          * aStatistics,
                              iduShmTxInfo    * aShmTxInfo,
                              iduStShmMemPool * aMemPoolInfo,
                              UInt            * aSize )
{
    UInt               i;
    UInt               sSize  = 0;
    iduStShmMemList  * sArrListInfo = NULL;
    iduStShmMemList  * sCurListInfo = NULL;
    idrSVP             sSavepoint;

    *aSize = 0;

    idrLogMgr::setSavepoint( aShmTxInfo, &sSavepoint );

    sArrListInfo = getArrMemList( aMemPoolInfo );

    if( ( aMemPoolInfo->mGarbageCollection == ID_TRUE ) &&
        ( iduMemMgr::isServerMemType() == ID_TRUE) )
    {
        for (i = 0; i < aMemPoolInfo->mListCount; i++)
        {
            sCurListInfo = &sArrListInfo[i];

            IDE_ASSERT( sCurListInfo != NULL );

            if( aMemPoolInfo->mUseMutex == ID_TRUE )
            {
                iduShmMemList::lock( aStatistics, aShmTxInfo, sCurListInfo );
            }

            IDE_TEST( iduShmMemList::shrink( aStatistics,
                                             aShmTxInfo,
                                             sCurListInfo,
                                             &sSize )
                      != IDE_SUCCESS );

            *aSize += sSize;
        }
    }

    IDE_TEST( idrLogMgr::commit2Svp( aStatistics, aShmTxInfo, &sSavepoint )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_ASSERT( idrLogMgr::abort2Svp( aStatistics, aShmTxInfo, &sSavepoint )
                == IDE_SUCCESS );

    return IDE_FAILURE;
}

void iduShmMemPool::dumpState( idvSQL          * aStatistics,
                               iduShmTxInfo    * aShmTxInfo,
                               iduStShmMemPool * aMemPoolInfo,
                               SChar           * aBuffer,
                               UInt              aBufferSize )
{
    UInt               sSize = 0;
    UInt               i;
    iduStShmMemList  * sArrMemListInfo = NULL;
    idrSVP             sVSavepoint;

    idrLogMgr::setSavepoint( aShmTxInfo, &sVSavepoint );

    sArrMemListInfo = getArrMemList( aMemPoolInfo );

    for( i = 0; i < aMemPoolInfo->mListCount; i++ )
    {
        iduShmMemList::lock( aStatistics, aShmTxInfo, sArrMemListInfo + i );

        sSize += iduShmMemList::getUsedMemory( sArrMemListInfo + i );


        IDE_ASSERT( idrLogMgr::commit2Svp( aStatistics,
                                           aShmTxInfo,
                                           &sVSavepoint )
                    == IDE_SUCCESS );
    }

    idlOS::snprintf( aBuffer,
                     aBufferSize,
                     "MemPool Used Memory:%"ID_UINT32_FMT"\n",
                     sSize );
}

void iduShmMemPool::status( iduStShmMemPool * aMemPoolInfo )
{
    UInt  i;
    SChar sBuffer[IDE_BUFFER_SIZE];

    iduStShmMemList  * sArrMemListInfo = NULL;

    sArrMemListInfo = getArrMemList( aMemPoolInfo );

    // fix BUG-21547
    idlOS::snprintf(sBuffer,
                    ID_SIZEOF(sBuffer),
                    "  - Memory List Count:%5"ID_UINT32_FMT"\n",
                    (UInt)aMemPoolInfo->mListCount);
    IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);

    idlOS::snprintf(sBuffer,
                    ID_SIZEOF(sBuffer),
                    "  - Slot   Size:%5"ID_UINT32_FMT"\n",
                    (UInt)(sArrMemListInfo[0].mSlotSize));
    IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);

    for(i = 0; i < aMemPoolInfo->mListCount; i++)
    {
        idlOS::snprintf(sBuffer,
                        ID_SIZEOF(sBuffer),
                        "  - Memory List *** [%"ID_UINT32_FMT"]\n",
                        (UInt)(i + 1));
        IDE_CALLBACK_SEND_SYM_NOLOG(sBuffer);

        iduShmMemList::status( sArrMemListInfo + i );
    }
}

/* added by mskim for check memory statement */
ULong iduShmMemPool::getMemorySize( iduStShmMemPool * aMemPoolInfo )
{
    UInt               i;
    UInt               sMemSize        = 0;
    iduStShmMemList  * sArrMemListInfo = NULL;

    sArrMemListInfo = getArrMemList( aMemPoolInfo );

    // fix BUG-21547
    for( i = 0; i < aMemPoolInfo->mListCount; i++ )
    {
        sMemSize += iduShmMemList::getUsedMemory( sArrMemListInfo + i );
    }

    return sMemSize;
}

void iduShmMemPool::fillMemPoolInfo( iduStShmMemPool     * aMemPoolInfo,
                                     iduShmMemPoolState  * aArrMemPoolInfo )
{
    UInt i;

    iduStShmMemList  * sArrMemListInfo = NULL;

    sArrMemListInfo = getArrMemList( aMemPoolInfo );

    for(i = 0; i < aMemPoolInfo->mListCount; i++)
    {
        iduShmMemList::fillMemPoolInfo( sArrMemListInfo + i,
                                        aArrMemPoolInfo + i );
    }
}

void iduShmMemPool::dumpFreeChunk4TSM( iduStShmMemPool * aMemPoolInfo )
{
    UInt               i;
    iduStShmMemList  * sArrMemListInfo = NULL;

    sArrMemListInfo = getArrMemList( aMemPoolInfo );

    for( i = 0; i < aMemPoolInfo->mListCount; i++ )
    {
        idlOS::printf( "##[%"ID_INT32_FMT"] Chunk Dump Begin\n", i );
        iduShmMemList::dumpPartialFreeChunk4TSM( sArrMemListInfo + i );
        idlOS::printf( "##[%"ID_INT32_FMT"] Chunk Dump End\n", i );
    }
}


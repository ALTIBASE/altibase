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

#ifndef _O_IDU_SHM_MEM_POOL_H_
#define _O_IDU_SHM_MEM_POOL_H_ 1

#include <idl.h>
#include <iduShmMemList.h>

#define IDU_AUTOFREE_CHUNK_LIMIT         5

#define IDU_SHM_MEM_POOL_DEFAULT_ALIGN_SIZE  8

#define IDU_SHM_MEM_POOL_MUTEX_POSTFIX       "_MUTEX_"

#define IDU_SHM_MEM_POOL_MUTEX_NAME_LEN (64)

#define IDU_SHM_MEM_POOL_NAME_LEN  ( IDU_SHM_MEM_POOL_MUTEX_NAME_LEN - \
                                 ID_SIZEOF(IDU_SHM_MEM_POOL_MUTEX_POSTFIX)-10 -1 )

typedef struct iduStShmMemPool
{
    idShmAddr            mAddrSelf;
    iduShmLatch          mLatch;
    idShmAddr            mAddrNxtPool;
    UInt                 mListCount;
    idShmAddr            mAddrArrMemList;
    iduMemoryClientIndex mIndex;

    /* iduShmMemPool uses mAllocIdx to allocate mem lists in an
     * (approximately) round-robin fashion. */
    UInt                 mAllocIdx;

    idBool               mUseMutex;

    idBool               mGarbageCollection;

    SChar                mName[IDU_SHM_MEM_POOL_NAME_LEN];
} iduStShmMemPool;

class iduShmMemPool
{
  public:
    static IDE_RC initialize( idvSQL             * aStatistics,
                              iduShmTxInfo       * aShmTxInfo,
                              iduMemoryClientIndex aIndex,
                              SChar              * aStrName,
                              UInt                 aListCount,
                              vULong               aElemSize,
                              vULong               aElemCount,
                              vULong               aChunkLimit,        /* IDU_AUTOFREE_CHUNK_LIMIT */
                              idBool               aUseLatch,          /* ID_TRUE */
                              UInt                 aAlignByte,         /* IDU_SHM_MEM_POOL_DEFAULT_ALIGN_SIZE */
                              idBool               aGarbageCollection, /* ID_TRUE */
                              idShmAddr            aAddr4MemPool,
                              iduStShmMemPool    * aMemPoolInfo );

    static IDE_RC destroy( idvSQL             * aStatistics,
                           iduShmTxInfo       * aShmTxInfo,
                           idrSVP             * aSavepoint,
                           iduStShmMemPool    * aMemPoolInfo,
                           idBool               aCheck /* ID_TRUE */ );

    static IDE_RC cralloc( idvSQL          * aStatistics,
                           iduShmTxInfo    * aShmTxInfo,
                           iduStShmMemPool * aMemPoolInfo,
                           idShmAddr       * aAddr4Mem,
                           void           ** aMem );

    static IDE_RC alloc( idvSQL          * aStatistics,
                         iduShmTxInfo    * aShmTxInfo,
                         iduStShmMemPool * aMemPoolInfo,
                         idShmAddr       * aAddr4Mem,
                         void           ** aMem );

    static IDE_RC memfree( idvSQL          * aStatistics,
                           iduShmTxInfo    * aShmTxInfo,
                           idrSVP          * aSavepoint,
                           iduStShmMemPool * aMemPoolInfo,
                           idShmAddr         aAddr4Mem,
                           void            * aMem );

    static IDE_RC shrink( idvSQL          * aStatistics,
                          iduShmTxInfo    * aShmTxInfo,
                          iduStShmMemPool * aMemPoolInfo,
                          UInt            * aSize );

    static void dumpState( idvSQL          * aStatistics,
                           iduShmTxInfo    * aShmTxInfo,
                           iduStShmMemPool * aMemPoolInfo,
                           SChar           * aBuffer,
                           UInt              aBufferSize );

    static void status( iduStShmMemPool * aMemPoolInfo );

    /* added by mskim for check memory statement */
    static ULong getMemorySize( iduStShmMemPool * aMemPoolInfo );

    static iduStShmMemList* getArrMemList( iduStShmMemPool * aMemPoolInfo );

    static void fillMemPoolInfo( iduStShmMemPool     * aMemPoolInfo,
                                 iduShmMemPoolState  * aArrMemPoolInfo );

    static void dumpFreeChunk4TSM( iduStShmMemPool * aMemPoolInfo );
};

inline iduStShmMemList* iduShmMemPool::getArrMemList( iduStShmMemPool * aMemPoolInfo )
{
    return (iduStShmMemList*)IDU_SHM_GET_ADDR_PTR_CHECK(
        aMemPoolInfo->mAddrArrMemList);
}


#endif  // _O_IDU_SHM_MEM_POOL_H_


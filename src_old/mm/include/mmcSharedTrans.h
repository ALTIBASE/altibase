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

#ifndef _O_MMC_SHARED_TRANS_H_
#define _O_MMC_SHARED_TRANS_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <iduLatch.h>
#include <iduList.h>

class mmcTransObj;
class mmcSession;
class mmcTransPrivateInfo;

typedef struct mmcTransBucket
{
    iduList     mChain;
    iduLatch    mBucketLatch;
} mmcTransBucket;

class mmcSharedTrans
{
public:
    static IDE_RC initialize();
    static IDE_RC finalize();

    static IDE_RC allocTrans( mmcTransObj ** aTrans, mmcSession * aSession );
    static IDE_RC freeTrans( mmcTransObj ** aTrans, mmcSession * aSession );

private:
    static IDE_RC initializeBucket( mmcTransBucket * aBucket);
    static IDE_RC finalizeBucket( mmcTransBucket * aBucket );
    static void finalizeFreeTrans();
    static void freeChain( iduList * aNode );
    static inline UInt getBucketIndex( void * aKey );
    static idBool findTrans( mmcTransBucket *  sBucket,
                             mmcSession     *  aSession,
                             mmcTransObj    ** aTransOut );
    static IDE_RC allocate( mmcTransObj ** aTrans,
                            mmcSession   * aSession );
    static IDE_RC allocTransFromMemPool( mmcTransObj ** aTrans );
    static void allocTransFromFreeList( mmcTransObj ** aTrans );
    static void free( mmcTransObj * aTrans );
    static void freeTransToMemPool( mmcTransObj * aTrans );
    static void freeTransToFreeList( mmcTransObj * aTrans );
    static IDE_RC initTransInfo( mmcTransObj * aTransObj );
    static void reuseTransInfo( mmcTransObj * aTransObj );
    static void finiTransInfo( mmcTransObj * aTransObj );

private:
    static iduMemPool       mPool;
    static iduList          mFreeTransChain;
    static iduLatch         mFreeTransLatch;
    static UInt             mFreeTransCount;
    static UInt             mFreeTransMaxCount;
    static UInt             mBucketCount;
    static mmcTransBucket * mHash;
};

#endif  // _O_MMC_SHARED_TRANS_H_

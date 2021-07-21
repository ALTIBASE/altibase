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

#ifndef _O_MMD_XID_MANAGER_H_
#define _O_MMD_XID_MANAGER_H_ 1

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <mmdDef.h>

class mmdXid;
struct mmcTransObj;

class mmdXidManager
{
private:
    static iduMemPool  mPool;
    static iduMemPool  mPool4IdXidNode;
    static mmdXidHashBucket *mHash;

public:
    // bug-35382: mutex optimization during alloc and dealloc
    static iduMemPool  mXidMutexPool;

    static IDE_RC initialize();
    static IDE_RC finalize();
    /* BUG-18981 */
    static IDE_RC alloc(mmdXid      **aXid,
                        ID_XID       *aUserXid,
                        mmcTransObj  *aTrans);
    //fix BUG-21794
    static IDE_RC alloc(mmdIdXidNode **aXidNode, ID_XID *aXid);
    
    static IDE_RC free(mmdXid *aXid, idBool aFreeTrans);
    //fix BUG-21794
    /* BUG-27968 XA Fix/Unfix Scalability를 향상시켜야 합니다.
     XA Unfix 시에 latch duaration을 줄이기위하여 xid fix-Count를 xid list latch release전에
     구한다.*/
    static IDE_RC free(mmdIdXidNode *aXidNode);
    /* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now.
       that is to say , chanage the granularity from global to bucket level.*/
    static IDE_RC add(mmdXid *aXid, UInt aBucket);

    static IDE_RC remove(mmdXid *aXid, UInt *aFixCount);
    //fix BUG-22033
    static IDE_RC find(mmdXid **aXid, ID_XID *aUserXid, UInt aBucket, mmdXaLogFlag  aXaLogFlag = MMD_XA_DO_LOG);

    static IDE_RC lockRead(UInt aBucket);
    static IDE_RC lockWrite(UInt aBucket);
    static IDE_RC unlock(UInt aBucket);
    static UInt   getBucketPos(ID_XID *aXid);
    static IDE_RC initBucket(mmdXidHashBucket* aBucket);
    static void   freeBucketChain(iduList*  aChain);
    //fix BUG-22669 XID list performance view need.
    static IDE_RC buildRecordForXID(idvSQL              * /*aStatistics*/,
                                    void                 *aHeader,
                                    void                 */*aDummyObj*/,
                                    iduFixedTableMemory  *aMemory);
};

/* fix BUG-35374 To improve scalability about XA, latch granularity of XID hash should be more better than now
  that is to say , chanage the granularity from global to bucket level.
 */
inline IDE_RC mmdXidManager::lockRead(UInt aBucket)
{
    // PROJ-2408
    return mHash[aBucket].mBucketLatch.lockRead(NULL,/* idvSQL* */  NULL/* idvWeArgs* */ );
}

inline IDE_RC mmdXidManager::lockWrite(UInt aBucket)
{
    // PROJ-2408
    return mHash[aBucket].mBucketLatch.lockWrite( NULL,/* idvSQL* */ NULL /* idvWeArgs* */);
}

inline IDE_RC mmdXidManager::unlock(UInt aBucket)
{
    // PROJ-2408
    return mHash[aBucket].mBucketLatch.unlock();
}


#endif

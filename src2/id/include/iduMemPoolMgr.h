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
 
/**********************************************************************
 * $Id: iduMemPoolMgr.h 80575 2017-07-21 07:06:35Z yoonhee.kim $
 **********************************************************************/

#ifndef _O_IDU_MEM_POOL_MGR_H_
#define _O_IDU_MEM_POOL_MGR_H_ 1

#include <idl.h>
#include <iduMemMgr.h>
#include <iduMemPool.h>
#include <iduFixedTable.h>

typedef struct iduMemPoolInfo
{
    iduMemoryClientIndex mWhere;
    UInt                 mMemListCnt;

    vULong               mFreeCnt;
    vULong               mFullCnt;
    vULong               mPartialCnt;
    vULong               mChunkLimit;
    vULong               mChunkSize;
    vULong               mElemCnt;
    vULong               mElemSize;
    SChar                mName[IDU_MEM_POOL_NAME_LEN];
}iduMemPoolInfo;

class idvSQL;

class iduMemPoolMgr
{
  public:
    static IDE_RC          initialize();
    static IDE_RC          destroy();

    static IDE_RC buildRecord4MemPool(
                        idvSQL              * aStatistics,
                        void                * aHeader,
                        void                * aDumpObj,
                        iduFixedTableMemory * aMemory );

    static IDE_RC addMemPool(iduMemPool * aMemPool);
    static IDE_RC delMemPool(iduMemPool * aMemPool);
    static IDE_RC shrinkMemPools(void);

  private:
    static void verifyPoolList(void);

  public: 
    static iduMemPool      * mListHead;
    static iduMutex          mMutex;
    static UInt              mMemPoolCnt;

};

#endif  


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
 * $Id:
 **********************************************************************/

#ifndef _O_SMU_MEMSTACK_H_
#define _O_SMU_MEMSTACK_H_ 1

#include <idl.h>
#include <smDef.h>
#include <iduLatch.h>

typedef struct smuMemNode
{
    ULong mDataSize;
    ULong mData[1];
}smuMemNode;

#define SMU_MEM_NODE_SIZE (ID_SIZEOF(smuMemNode) - ID_SIZEOF(ULong))

class smuMemStack
{
public :
    /* °´Ã¼ ÃÊ±âÈ­ */
    IDE_RC initialize( iduMemoryClientIndex   aMemoryIndex,
                       UInt        aDataSize,
                       UInt        aStackInitCount );
    IDE_RC destroy();
    IDE_RC resetStackInitCount( UInt aNewStackInitCount );
    IDE_RC resetDataSize( UInt aNewDataSize );
    IDE_RC pop( UChar   **aData );
    IDE_RC popWithLock( UChar   **aData );
    IDE_RC push( UChar  *aData );
    IDE_RC pushWithLock( UChar  *aData );
    UInt getCount()
    {
        return mNodeStack.getTotItemCnt();
    };
    UInt getNodeCount()
    {
        return mNodeCount;
    };
    UInt getInitCount()
    {
        return mInitCount;
    };
    UInt getDataSize()
    {
        return mDataSize;
    };
    UInt getNodeSize()
    {
        return mDataSize + SMU_MEM_NODE_SIZE;
    };
    iduMemoryClientIndex getMemoryIndex()
    {
        return mMemoryIndex;
    };

    UInt getNodeSize( UChar* aData )
    {
        smuMemNode * sNode = (smuMemNode*)(aData - SMU_MEM_NODE_SIZE);
        return sNode->mDataSize + SMU_MEM_NODE_SIZE;
    };

    void lock()
    {
        (void)mMutex.lock( NULL );
    }

    void unlock()
    {
        (void)mMutex.unlock();
    }

    void incNodeCount()
    {
        (void)idCore::acpAtomicInc32( &mNodeCount );
    }

    void decNodeCount()
    {
        (void)idCore::acpAtomicDec32( &mNodeCount );
    }

private:
    iduStackMgr  mNodeStack;
    UInt         mDataSize;
    UInt         mInitCount;
    UInt         mNodeCount;
    UInt         mNodeCountOtherSize;
    iduMemoryClientIndex   mMemoryIndex;
    iduMutex     mMutex;
};

#endif /* _O_SMU_MEMSTACK_H_ */

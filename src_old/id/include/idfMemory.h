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
 * $Id: iduMemory.h 17293 2006-07-25 03:04:24Z mhjeong $
 **********************************************************************/

#ifndef _O_IDFMEMORY_H_
# define _O_IDFMEMORY_H_  1

#include <idl.h>
#include <iduList.h>
#include <iduMemMgr.h>

class idfMemory {
    
public:
    idfMemory();
    ~idfMemory();

    IDE_RC               init( UInt                 aPageSize,
                               UInt                 aAlignSize,
                               idBool               aLock );
    IDE_RC               destroy(); 

    IDE_RC               alloc( void **aBuffer, size_t aSize );
    IDE_RC               cralloc( void **aBuffer, size_t aSize );
    IDE_RC               free( void *aBuffer );
    IDE_RC               freeAll();

    SInt                 lock();
    SInt                 unlock();

    SChar *              getBufferW() { return mBufferW; };
    SChar *              getBufferR() { return mBufferR; };

private:
    iduList              mNodeList;

    idBool               mLock;
    PDL_thread_mutex_t   mMutex;
    UInt                 mAlignSize;
    SChar              * mBufferW;
    SChar              * mBufferR;
};

#endif /* _O_IDF_MEMORY_H_ */

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
 * $id$
 **********************************************************************/

#ifndef _O_DKD_RECORD_BUFFER_MGR_H_
#define _O_DKD_RECORD_BUFFER_MGR_H_ 1


#include <dkd.h>

IDE_RC  fetchRowFromRecordBuffer( void  *aMgrHandle,  void    **aRow );
IDE_RC  insertRowIntoRecordBuffer( void *aMgrHandle, dkdRecord   *aRecord );
IDE_RC  restartRecordBuffer( void   *aMgrHandle );

class dkdRecordBufferMgr
{
private:
    idBool                  mIsEndOfFetch;
    SInt                    mRecordCnt;
    ULong                   mBufSize;       /* buffer's size *//* BUG-36895: UInt -> ULong */ 
    UInt                    mBufBlockCnt;   /* buffer block count */
    dkdRecord              *mCurRecord;     /* current record */
    iduMemAllocator        *mAllocator;     /* dynamic TLSF allocator *//* BUG-37215 */
    iduList                 mRecordList;    /* record list */

public:
    /* Initialize / Finalize */
    /* BUG-37215 */
    IDE_RC                  initialize( UInt             aBufBlockCnt, 
                                        iduMemAllocator *aAllocator );
    /* BUG-37487 */
    void                    finalize();

    /* Operations */
    IDE_RC                  fetchRow( void  **aRow );
    IDE_RC                  insertRow( dkdRecord  *aRecord );
    IDE_RC                  restart();

    IDE_RC                  moveNext();
    IDE_RC                  moveFirst();

    inline ULong            getRecordBufferSize();
    inline UInt             getBufferBlockCount();
    inline iduList*         getRecordList();
};

/* BUG-36895: UInt -> ULong */
inline ULong dkdRecordBufferMgr::getRecordBufferSize()
{
    return mBufSize;
}

inline UInt dkdRecordBufferMgr::getBufferBlockCount()
{
    return mBufBlockCnt;
}

inline iduList* dkdRecordBufferMgr::getRecordList()
{
    return &mRecordList;
}

#endif /* _O_DKD_RECORD_BUFFER_MGR_H_ */

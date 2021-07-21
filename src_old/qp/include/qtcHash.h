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
 * $Id$
 *
 *      PROJ-2452 Caching for DETERMINISTIC Function
 *
 **********************************************************************/

#ifndef _O_QTC_HASH_H_
#define _O_QTC_HASH_H_ 1

#include <iduList.h>
#include <qc.h>
#include <qmsParseTree.h>

typedef struct qtcHashRecord
{
    void          * mValue;          // data of return value
    void          * mKeyData;        // data of input parameters
    UInt          * mKeyLengthArr;   // key length array of input parameters
    UInt            mKey;            // key
    iduListNode     mRecordListNode; // list node for hash record
} qtcHashRecord;

typedef struct qtcHashBucket
{
    UInt            mRecordCnt;      // total record count of a bucket
    iduList         mRecordList;     // list of hash records
} qtcHashBucket;

typedef struct qtcHashTable
{
    qtcHashBucket * mBucketArr;      // bucket array
    UInt            mBucketCnt;      // bucket count
    UInt            mKeyCnt;         // key count
    UInt            mTotalRecordCnt; // total record count
} qtcHashTable;

class qtcHash
{
    /* For Execution */
public:

    static IDE_RC initialize( iduMemory     * aMemory,
                              UInt            aBucketCnt,
                              UInt            aKeyCnt,
                              UInt          * aRemainSize,
                              qtcHashTable ** aHashTable );

    static IDE_RC insert( qtcHashTable  * aHashTable,
                          qtcHashRecord * aNewRecord );

    static IDE_RC compareKeyData( mtcStack      * aStack,
                                  qtcHashRecord * aRecord,
                                  UInt            aKeyCnt,
                                  idBool        * aResult );

    static inline UInt hash( UInt aBucketCnt, UInt aKey );

private:

};

inline UInt qtcHash::hash( UInt aKey,
                           UInt aBucketCnt )
{
    IDE_DASSERT( aKey       > 0 );
    IDE_DASSERT( aBucketCnt > 0 );

    return aKey % aBucketCnt;
}

#endif /* _O_QTC_HASH_H_ */

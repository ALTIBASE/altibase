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
 
/*****************************************************************************
 * $Id:
 ****************************************************************************/

/* **********************************************************************
 *   $Id: 
 *   NAME
 *     iddRBHash.h - Hash table with Red-Black Tree
 *
 *   DESCRIPTION
 *     Red-Black Tree를 하부구조로 하는 Hash
 *
 *   Class
 *     iduRBHash : Key/Data pair를 저장할 수 있는 해시 테이블
 *
 *   MODIFIED   (MM/DD/YY)
 ********************************************************************** */

#ifndef O_IDD_RBHASH_H
#define O_IDD_RBHASH_H   1

#include <idl.h>
#include <ideErrorMgr.h>
#include <idu.h>
#include <iduLatch.h>
#include <iddDef.h>
#include <iddRBTree.h>

typedef struct iddRBHashLink
{
    iddRBHashLink* mPrev;
    iddRBHashLink* mNext;
} iddRBHashLink;

class iddRBHash : public iddRBHashLink
{
public:
    IDE_RC initialize(const SChar*,
                      const iduMemoryClientIndex,
                      const UInt,
                      const UInt,
                      const idBool,
                      const iddHashFunc,
                      const iddCompFunc);
    IDE_RC reset(void);
    IDE_RC destroy(void);

    IDE_RC insert(const void*, void* = NULL);
    IDE_RC search(const void*, void** = NULL);
    IDE_RC update(const void*, void*, void** = NULL);
    IDE_RC remove(const void*, void** = NULL);

    inline SInt hash(const void* aKey) {return mHashFunc(aKey) % mBucketCount;};
    inline UInt getBucketCount(void) {return mBucketCount;}

private:
    typedef iddRBLatchTree bucket;

private:
    SChar                       mName[32];
    iduMemoryClientIndex        mIndex;
    iddHashFunc                 mHashFunc;
    bucket*                     mBuckets;
    UInt                        mBucketCount;

public:
    inline IDE_RC  lock(void)   { return mMutex.lock(NULL); }
    inline IDE_RC  unlock(void) { return mMutex.unlock(); }

private:
    iduMutex    mMutex;

public:
    /**
     * iddRBHash를 순회할 수 있는 iterator 클래스
     * 해시함수 결과가 가장 작은 버킷의
     * 비교함수로 비교시 가장 작은 노드부터
     * 차례로 순회할 수 있다
     */
    class iterator
    {
    public:
        iterator(iddRBHash* = NULL, SInt = 0);
        iterator& moveNext(void);
        const iterator& operator=(const iterator&);
        bool operator==(const iterator& aIter);
        inline bool operator!=(const iterator& aIter)
        {
            return !(operator==(aIter));
        }
        inline iterator& operator++() { return moveNext(); }
        inline iterator& operator++(int) { return moveNext(); }
        inline void* getKey(void) { return mTreeIterator.getKey(); }
        inline void* getData(void) { return mTreeIterator.getData(); }

        iddRBHash*          mHash;
        UInt                mCurrent;
        bucket::iterator    mTreeIterator;
    };

    inline const iterator begin(void) {return iterator(this, 0);}
    inline const iterator end(void) {return iterator(NULL);}

    friend class iterator;

public:
    IDE_RC          open(void);
    IDE_RC          close(void);
    idBool          isEnd(void);
    void*           getCurNode(void);
    void*           getNxtNode(void);
    IDE_RC          cutCurNode(void** = NULL);
    IDE_RC          delCurNode(void** = NULL);

private:
    iterator        mIterator;
    idBool          mOpened;

public:
    static IDE_RC initializeStatic(void);
    static IDE_RC destroyStatic(void);
    void          fillHashStat(iddRBHashStat*);
    void          fillBucketStat(iddRBHashStat*, const UInt);
    void          clearStat(void);

public:
    static iddRBHashLink    mGlobalLink;
    static iduMutex         mGlobalMutex;
};

#endif /* O_IDD_RBHASH_H */


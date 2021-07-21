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
 * $Id:$
 **********************************************************************/

#ifndef _O_SDB_CPLIST_SET_H_
#define _O_SDB_CPLIST_SET_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <sdbBCB.h>

class sdbCPListSet
{
public:
    IDE_RC initialize( UInt aListCount, sdLayerState aType );

    IDE_RC destroy();

    void add( idvSQL *aStatistics, void *aBCB );

    sdBCB* getMin();

    sdBCB* getNextOf( void *aCurrBCB );

    void remove( idvSQL *aStatistics, void *aBCB );

    void getMinRecoveryLSN(idvSQL *aStatistics, smLSN *aMinLSN);

    void getMaxRecoveryLSN(idvSQL *aStatistics, smLSN *aMaxLSN);

    UInt getTotalBCBCnt();

    UInt getListCount();

    // sdbBCBListStat을 위한 함수들
    UInt getLength(UInt aListNo);
    void* getFirst(UInt aListNo);
    void* getLast(UInt aListNo);

private:
    /* 유지하는 list 갯수 */
    UInt        mListCount;

    /* BUG-47945 cp list 디버깅 정보 보강 */
    sdLayerState mLayerType;
    
    /* 리스트들의 base. 배열형태로 되어있다.*/
    smuList    *mBase;
    
    /* 리스트들의 길이. 배열형태로 되어있다.*/
    UInt       *mListLength;
    
    /* 리스트들의 뮤텍스. 배열형태로 되어있다.*/
    iduMutex   *mListMutex;
};  

inline UInt sdbCPListSet::getListCount()
{
    return mListCount;
}

inline UInt sdbCPListSet::getLength(UInt aListNo)
{
    IDE_DASSERT(aListNo < mListCount);
    return mListLength[aListNo];
}

inline void* sdbCPListSet::getFirst(UInt aListNo)
{
    IDE_DASSERT(aListNo < mListCount);
    return SMU_LIST_GET_FIRST(&mBase[aListNo])->mData;
}

inline void* sdbCPListSet::getLast(UInt aListNo)
{
    IDE_DASSERT(aListNo < mListCount);
    return SMU_LIST_GET_LAST(&mBase[aListNo])->mData;
}

#endif // _O_SDB_CPLIST_SET_H_


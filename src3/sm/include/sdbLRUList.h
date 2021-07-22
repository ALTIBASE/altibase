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
/***********************************************************************
 * PROJ-1568 BUFFER MANAGER RENEWAL
 ***********************************************************************/

#ifndef _O_SDB_LRU_LIST_H_
#define _O_SDB_LRU_LIST_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <sdbBCB.h>
#include <sdbBufferPoolStat.h>

class sdbLRUList
{
public:
    IDE_RC  initialize(UInt aListID, UInt aHotMax, sdbBufferPoolStat *aStat);

    IDE_RC  destroy();

    void    setHotMax(idvSQL *aStatistics, UInt aNewHotMax);

    sdbBCB* removeColdLast(idvSQL *aStatistics);
    idBool  removeBCB(idvSQL *aStatistics, sdbBCB *aBCB);

    void    addToHot(idvSQL *aStatistics, sdbBCB *aBCB);

    void    insertBCB2BehindMid(idvSQL *aStatistics, sdbBCB *aBCB);

    IDE_RC  checkValidation(idvSQL *aStatistics);
    
    UInt    moveMidPtrForward(idvSQL *aStatistics, UInt aMoveCnt);

    inline UInt    getHotLength();

    inline UInt    getColdLength();

    inline UInt    getID();

    void dump();

    // fixed table 구축을 위한 정보
    // 동시성이 전혀 고려되어 있지 않음
    inline sdbBCB* getFirst();
    inline sdbBCB* getMid();
    inline sdbBCB* getLast();

private:
    /* LRUList 식별자. LRU리스트를 여러개 유지할 수 있으므로, 각 리스트 마다
     * 식별자를 둔다.  이 식별자를 통해 BCB내에 유지함으로 해서 각 BCB가
     * 어떤 리스트에 속해있는지 정확히 알아낼 수 있다.*/
    UInt      mID;

    /* LRUList의 base */
    smuList   mBaseObj;
    smuList  *mBase;
    /* Cold first를 mMid가 가리키고 있다. cold로 삽입할때 mMid의 mPrev로 삽입하고
     * 삽입한 BCB를 mMid로 변경한다.*/
    smuList  *mMid;

    /* LRU List에서 현재 Hot BCB의 갯수 */
    UInt      mHotLength;
    /* LRU List가 가질수 있는 Hot BCB의 최대치 */
    UInt      mHotMax;

    iduMutex  mMutex;

    UInt      mColdLength;

    sdbBufferPoolStat *mStat;
};

UInt sdbLRUList::getID()
{
    return mID;
}

UInt sdbLRUList::getColdLength()
{
    return mColdLength;
}

UInt sdbLRUList::getHotLength()
{
    return mHotLength;
}

/***********************************************************************
 * Description :
 *  fixed table 구축을 위한 정보. 동시성이 전혀 고려되어 있지 않음.
 *  리스트의 가장 처음 BCB를 얻어온다.
 ***********************************************************************/
sdbBCB* sdbLRUList::getFirst()
{
    if (SMU_LIST_IS_EMPTY(mBase))
    {
        return NULL;
    }
    else
    {
        if (SMU_LIST_GET_FIRST(mBase) == mMid)
        {
            return NULL;
        }
        else
        {
            return (sdbBCB*)(SMU_LIST_GET_FIRST(mBase))->mData;
        }
    }
}

/***********************************************************************
 * Description :
 *  fixed table 구축을 위한 정보. 동시성이 전혀 고려되어 있지 않음.
 *  midPoint의 BCB를 얻어온다.
 ***********************************************************************/
sdbBCB* sdbLRUList::getMid()
{
    return mMid == NULL ? NULL : (sdbBCB*)mMid->mData;
}

/***********************************************************************
 * Description :
 *  fixed table 구축을 위한 정보. 동시성이 전혀 고려되어 있지 않음.
 *  리스트의 가장 마지막 BCB를 얻어온다.
 ***********************************************************************/
sdbBCB* sdbLRUList::getLast()
{
    if (SMU_LIST_IS_EMPTY(mBase))
    {
        return NULL;
    }
    else
    {
        return (sdbBCB*)(SMU_LIST_GET_LAST(mBase))->mData;
    }
}

#endif // _O_SDB_LRU_LIST_H_


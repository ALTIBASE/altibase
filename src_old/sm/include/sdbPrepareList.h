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

#ifndef _O_SDB_PREPARE_LIST_H_
#define _O_SDB_PREPARE_LIST_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <sdbBCB.h>

class sdbPrepareList
{
public:
    IDE_RC initialize(UInt aListID);

    IDE_RC destroy();

    sdbBCB* removeLast(idvSQL *aStatistics);

    IDE_RC addBCBList(idvSQL *aStatistics,
                      sdbBCB *aFirstBCB,
                      sdbBCB *aLastBCB,
                      UInt    aLength);

    inline UInt getLength();

    inline UInt getWaitingClientCount();

    IDE_RC timedWaitUntilBCBAdded(idvSQL *aStatistics,
                                  UInt    aTimeMilliSec,
                                  idBool *aBCBAdded);

    // sdbBCBListStat을 위한 함수들
    inline sdbBCB* getFirst();
    inline sdbBCB* getLast();
    inline UInt    getID();

private:
    IDE_RC checkValidation(idvSQL *aStatistics);

private:
    /* prepare list 식별자. prepare list를 여러개 유지할 수 있으므로, 각 리스트 마다
     * 식별자를 둔다.  이 식별자를 통해 BCB내에 유지함으로 해서 각 BCB가
     * 어떤 리스트에 속해있는지 정확히 알아낼 수 있다.*/
    UInt      mID;

    /* smuList에서 필요로 하는 base list 실제로 mData는 아무것도 가리키고
     * 있지 않는다.*/
    smuList     mBaseObj;
    smuList    *mBase;

    iduMutex    mMutex;
    /* prepare list가 비었을때, BCB가 삽입되기를  대기하기 위한 mutex */
    iduMutex    mMutexForWait;
    /* prepare list가 비었을때, BCB가 삽입되기를  대기하기 위한 cond variable */
    iduCond     mCondVar;

    UInt        mListLength;
    /* 현재 prepare list에 대해서 대기중인 쓰레드의 갯수 */
    UInt        mWaitingClientCount;
};

/***********************************************************************
 * Description :
 *  현재 list의 길이를 얻는다. 뮤텍스를 잡지 않기 때문에, 정확한 현재 상황을
 *  반영하지 않을 수 있다.
 ***********************************************************************/
UInt sdbPrepareList::getLength()
{
    return mListLength;
}

sdbBCB* sdbPrepareList::getFirst()
{
    if (SMU_LIST_IS_EMPTY(mBase))
    {
        return NULL;
    }
    else
    {
        return (sdbBCB*)SMU_LIST_GET_FIRST(mBase)->mData;
    }
}

sdbBCB* sdbPrepareList::getLast()
{
    if (SMU_LIST_IS_EMPTY(mBase))
    {
        return NULL;
    }
    else
    {
        return (sdbBCB*)SMU_LIST_GET_LAST(mBase)->mData;
    }
}

UInt sdbPrepareList::getID()
{
    return mID;
}

UInt sdbPrepareList::getWaitingClientCount()
{
    return mWaitingClientCount;
}

#endif // _O_SDB_PREPARE_LIST_H_

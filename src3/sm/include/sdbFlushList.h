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


#ifndef _O_SDB_FLUSH_LIST_H_
#define _O_SDB_FLUSH_LIST_H_ 1

#include <sdbDef.h>
#include <idu.h>
#include <idv.h>
#include <sdbBCB.h>

class sdbFlushList
{
public:
    IDE_RC initialize( UInt aListID, sdbBCBListType aListType );

    IDE_RC destroy();

    void beginExploring(idvSQL *aStatistics);

    void endExploring(idvSQL *aStatistics);

    sdbBCB* getNext(idvSQL *aStatistics);

    void remove(idvSQL *aStatistics, sdbBCB *aTarget);

    void add(idvSQL *aStatistics, sdbBCB *aBCB);

    IDE_RC checkValidation(idvSQL *aStatistics);

    inline UInt getPartialLength();

    // sdbBCBListStat을 위한 함수들
    inline sdbBCB*          getFirst();
    inline sdbBCB*          getLast();
    inline UInt             getID();
    inline sdbBCBListType   getListType();
    inline void             setMaxListLength( UInt aMaxListLength );
    inline idBool           isUnderMaxLength();

private:
    /* FlushList 식별자. Flush리스트를 여러개 유지할 수 있으므로, 각 리스트 마다
     * 식별자를 둔다.  이 식별자를 통해 BCB내에 유지함으로 해서 각 BCB가
     * 어떤 리스트에 속해있는지 정확히 알아낼 수 있다.*/
    UInt      mID;

    /* PROJ-2669
     * Normal/Delayed Flush List 구분을 위해 필요
     * SDB_BCB_FLUSH_LIST or SDB_BCB_FLUSH_HOT_LIST */
    sdbBCBListType mListType;
    
    /* smuList에서 필요로 하는 base list 실제로 mData는 아무것도 가리키고
     * 있지 않는다.*/
    smuList   mBaseObj;
    smuList  *mBase;
    /* getNext를 호출할때, 리턴해야 하는 BCB를 가지고 있는 변수 */
    smuList  *mCurrent;
    /* 현재 flush list에 접근하고 있는 flusher의 갯수, beginExploring
     * 시에 값이 증가 되고, endExploring시에 값이 감소한다.*/
    SInt      mExplorerCount;

    iduMutex  mMutex;

    UInt      mListLength;

    /* PROJ-2669 Flush List 최대 길이 */
    UInt      mMaxListLength;
};  

/* BUGBUG:
 * flusher들이 어떤 FLUSH List를 flush해야 할지 선택하는데, length를 통해
 * 선택하는 경우에, 문제가 생길 수 있다.
 * 왜냐면, flusher가 들어와서 
 * */
UInt sdbFlushList::getPartialLength()
{
    return mListLength;
}

sdbBCB* sdbFlushList::getFirst()
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

sdbBCB* sdbFlushList::getLast()
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

UInt sdbFlushList::getID()
{
    return mID;
}

sdbBCBListType sdbFlushList::getListType()
{
    return mListType;
}

void sdbFlushList::setMaxListLength( UInt aMaxListLength )
{
    mMaxListLength      = aMaxListLength;
}

idBool sdbFlushList::isUnderMaxLength()
{
    return (idBool)( mListLength < mMaxListLength );
}

#endif // _O_SDB_FLUSH_LIST_H_


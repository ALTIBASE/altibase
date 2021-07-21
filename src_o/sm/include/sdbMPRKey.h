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
 **********************************************************************/

#ifndef _O_SDB_MPR_KEY_H_
#define _O_SDB_MPR_KEY_H_ 1

#include <smDef.h>
#include <sdbDef.h>
#include <sdbBCB.h>

#define SDB_MAX_MPR_PAGE_COUNT   (128)

typedef enum
{
    SDB_MRPBCB_TYPE_BUFREG = 1,
    SDB_MRPBCB_TYPE_NEWFETCH
} sdbMPRBCBType;

/****************************************************************
 * MPR에서 여러번에 걸쳐서 읽는 경우에도 연속되는 페이지는 연속으로 읽는다.
 * 이때, 한번에 read하게 되는 단위를 sdbReadUnit이라고 한다.
 *
 * 여기엔 FirstPID와 연속된 페이지 갯수, 그리고 실제 BCB를 가리키는
 * 포인터 변수를 가지고 있다.
 ****************************************************************/
typedef struct sdbMPRBCB
{
    sdbBCB        *mBCB;
    sdbMPRBCBType  mType;
} sdbMPRBCB;

typedef struct sdbReadUnit
{
    scPageID     mFirstPID;
    UInt         mReadBCBCount;
    sdbMPRBCB  * mReadBCB;
} sdbReadUnit;

typedef enum
{
    SDB_MPR_KEY_CLEAN = 0,
    SDB_MPR_KEY_FETCHED
} sdbMPRKeyState;
/****************************************************************
 * PROJ-1568 설계문서의 3.3 참조.
 * Multi Page Read를 위해 유지하는 자료구조.
 * MPR에서 읽어야 하는 페이지와 읽지 말아야 하는 페이지에 관한 정보를 유지함.
 ****************************************************************/
class sdbMPRKey 
{ 
public:
    inline void    initFreeBCBList();

    inline sdbBCB* removeLastFreeBCB();

    inline void    addFreeBCB(sdbBCB *aBCB);

    inline void    removeAllBCB(sdbBCB **aFirstBCB,
                                sdbBCB **aLastBCB,
                                UInt    *aBCBCount);

    inline idBool  hasNextPage();

    inline void    moveToNext();
    inline void    moveToBeforeFst();

public:
    scSpaceID      mSpaceID;
    SInt           mIOBPageCount;
    SInt           mReadUnitCount;
    sdbReadUnit    mReadUnit[SDB_MAX_MPR_PAGE_COUNT/2];
   
    // MPR에 target이 된 BCB들은 실제 read를 했건 안했건 모두 버퍼에 있어야
    // 한다. 그리고 그들은 모두 mBCBs에서 포인팅하고 있다.
    // MPRKey를 이용하여 getPage를 할때는 반드시 순서대로 접근을 해야 한다.
    // 이때, 현재 접근 중엔 BCB에 대한 정보는 mCurrentIndex에서 유지한다.
    sdbMPRBCB      mBCBs[SDB_MAX_MPR_PAGE_COUNT];
    SInt           mCurrentIndex;
    
    /* MPR에서 연속적인 공간을 읽어 들이기 위해선, 연속적인 메모리가 필요하다.
     * 이것을 위해서 미리 공간을 할당한다. 이 공간의 크기는 한번에 읽어 들이는
     * 경우를 대비한 크기이다.
     * */
    UChar        * mReadIOB;
    
    /* MPR에서는 frame으로 사용되는 메모리를 따로 할당하지 않고, 기존에 존재하는
     * BCB를 이용한다. 이것은 getVictim을 통해서 얻어오기 때문에, 버퍼 매니저에
     * overhead를 줄 수 있다.  그렇기 때문에, 따로 mFreeList를 유지한다.
     * 기본적으로 MPR이 사용하고 난 BCB는 버퍼매니저에 반납하지 않고,
     * mFreeList에 반납한다. 버퍼매니저에 반납하는 경우는, 다른 누군가에 의해
     * hit(fix)가 된 경우이다. */
    smuList        mFreeListBase;
    SInt           mFreeBCBCount;
    sdbMPRKeyState mState;
};

void sdbMPRKey::initFreeBCBList()
{
    mFreeBCBCount = 0;
    SMU_LIST_INIT_BASE(&mFreeListBase);
}

/****************************************************************
 * Description:
 *  mFreeList에서 FreeBCB하나를 리턴한다.
 ****************************************************************/
sdbBCB* sdbMPRKey::removeLastFreeBCB()
{
    sdbBCB  *sRet;
    smuList *sNode;

    if (SMU_LIST_IS_EMPTY(&mFreeListBase))
    {
        sRet = NULL;
    }
    else
    {
        sNode = SMU_LIST_GET_LAST(&mFreeListBase);
        SMU_LIST_DELETE(sNode);

        sRet = (sdbBCB*)sNode->mData;
        SDB_INIT_BCB_LIST(sRet);
        IDE_DASSERT( mFreeBCBCount != 0);
        mFreeBCBCount--;
    }

    return sRet;
}

/****************************************************************
 * Description:
 *  mFreeList에 FreeBCB하나를 추가한다.
 ****************************************************************/
void sdbMPRKey::addFreeBCB(sdbBCB *aBCB)
{
    mFreeBCBCount++;
    SMU_LIST_ADD_FIRST(&mFreeListBase, &aBCB->mBCBListItem);
}

/****************************************************************
 * Description:
 *  mFreeList에 있는 모든 Free BCB 들을 List형태로 리턴한다.
 ****************************************************************/
void sdbMPRKey::removeAllBCB(sdbBCB **aFirstBCB,
                             sdbBCB **aLastBCB,
                             UInt    *aBCBCount)
{
    smuList *sFirst = SMU_LIST_GET_FIRST(&mFreeListBase);
    smuList *sLast  = SMU_LIST_GET_LAST(&mFreeListBase);

    SMU_LIST_CUT_BETWEEN(&mFreeListBase, sFirst);
    SMU_LIST_CUT_BETWEEN(sLast, &mFreeListBase);
    SMU_LIST_INIT_BASE(&mFreeListBase);

    *aFirstBCB = (sdbBCB*)sFirst->mData;
    *aLastBCB  = (sdbBCB*)sLast->mData;
    *aBCBCount = mFreeBCBCount;

    mFreeBCBCount = 0;
}


/****************************************************************
 * Description:
 *  더이상 MPR에서 getPage할 수 있는 페이지가 남아 있는지 확인한다.
 ****************************************************************/
idBool sdbMPRKey::hasNextPage()
{
    return ( ( mCurrentIndex + 1 ) < mIOBPageCount ? ID_TRUE : ID_FALSE);
}

/****************************************************************
 * Description:
 *  다음 페이지에 대해서 getPage하기 위해 호출한다.
 ****************************************************************/
void sdbMPRKey::moveToNext()
{
    IDE_ASSERT(mCurrentIndex <= mIOBPageCount);
    mCurrentIndex++;
}

void sdbMPRKey::moveToBeforeFst()
{
    mCurrentIndex = -1;
}

#endif // _O_SDB_MPR_KEY_H_

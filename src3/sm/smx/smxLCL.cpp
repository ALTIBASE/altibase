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
 **********************************************************************/
/**************************************************************
 * FILE DESCRIPTION : smxLCL.cpp                              *
 * -----------------------------------------------------------*
  본 파일은 각 transaction에서  메모리 또는 Disk LobCursor
  list를 관리하고, oldestSCN값을 반환하는 모듈이다.

  LobCursor 삽입 /삭제는 ,동시성 없이 순차적으로 수행되며,
  disk , memory garbage collector에서 읽는   oldest SCN값은
  LobCursor가 삽입/삭제가 발생하더라도  blocking없이 수행되도록
설계 하였다.

  그림과 같이 Lob Cursor List(LCL)는  circular double linked list
  로 관리된다.

  baseNode in LCL(Lob Cursor List)
                                    next
   |   ---------------------------------
   V   V                               |
   -------  next   -------         ----------
  |      | ---->  |       | ---->  |        |
  |mSCN  |  prev  |  40   |        |  80    |
  |(40)  | <---   |       | <---   |        |
  -------         ---------        ---------
                                       ^
    |       prev                        |
     -----------------------------------

                 ---------------------->
                   insert진행방향(SCN값 순서로 정렬)

 smxLSL의 object안에  smLobCusro인 base노드의 mSCN값이
 oldestSCN값이 되도록 list를 아래와 같이 관리한다.

  -  circular dobule linked list에
    node insert시 head node가 되는 경우에는 basenode의
     mSCN을 갱신시킨다.

  -  circular dobule linked list에 node를 삭제시,
    head node를 삭제하는 경우에는 head node의
    next  node의 mSCN으로 base node의 mSCN을 갱신시킨다.

 oldestSCN을 blocking없이 읽기 위하여 perterson's algorithm
 을 도입하였다.
**********************************************************************/

#include <smErrorCode.h>
#include <smm.h>
#include <smxTransMgr.h>

/*********************************************************************
  Name:  smxLCL::getOldestSCN
  Argument: smSCN*
  Description:
  baseNode의 mSCN을 assgin하여 준다.
  즉 baseNode의 mSCN이 oldestSCN을 유지 하도록 circular double linked
  list 삽입시 head node가 되거나 , list삭제시 head node를 삭제한
  경우에 base node의 mSCN을 갱신한다.

  baseNode in SMXLCL
                                    next
   |   ---------------------------------
   V   V                               |
   -------  next   -------         ----------
  |      | ---->  |       | ---->  |        |
  |mSCN  |  prev  |  40   |        |  80    |
  |(40)  | <---   |       | <---   |        |
  -------         ---------        ---------
                                       ^
    |       prev                        |
     -----------------------------------

  list  insert/delet는 serial하게 이루어지고 oldestSCN을
 읽는부분은 insert/delete시에 blocking없이 수행하기 위하여
  peterson algorithm을  아래와 같이 도입하였다.

  mSyncA                      mSyncB
    -----------------------------> reader(smxLCL::getOldestSCN)
    <----------------------------- writer(smxLCL::insert,remove)

  added for A4.
 **********************************************************************/
void smxLCL::getOldestSCN(smSCN* aSCN)
{

    UInt sSyncVa=0;
    UInt sSyncVb=0;

    IDE_DASSERT( aSCN != NULL );
    while(1)
    {
        ID_SERIAL_BEGIN(sSyncVa = mSyncVa);

        // BUG-27735 : Peterson Algorithm
        ID_SERIAL_EXEC( SM_GET_SCN(aSCN, &(mBaseNode.mLobViewEnv.mSCN)), 1 );

        ID_SERIAL_END(sSyncVb = mSyncVb);

        if(sSyncVa == sSyncVb)
        {
            break;
        }
        else
        {
            idlOS::thr_yield();
        }
    }//while
}

/*********************************************************************
  Name:  smxLCL::insert
  Description:
  circular dobule linked list insert시에
  node를   아래 그림과 같이 맨뒤에 append되도록 한다.

  insert시 head node가 되는 경우에는 base node의
  mSCN을 갱신시킨다.

                            next

     |--------------------------------
     V                               |
   -------  next   -------         ----------
  |      | ---->  |       | ---->  |        |
  |mSCN  |  prev  |  40   |        |  80    |
  |(40)  | <---   |       | <---   |        |
  -------         ---------        ---------
  base node
                                        ^
    |       prev                        |
     -----------------------------------

                    -------------------->
                    insert 진행방향(SCN값 순서로 정렬)

 **********************************************************************/
void  smxLCL::insert(smLobCursor* aNode)
{

    IDE_DASSERT( aNode != NULL );

    aNode->mPrev        =  mBaseNode.mPrev;
    aNode->mNext        =  &mBaseNode;
    mBaseNode.mPrev->mNext  =  aNode;
    mBaseNode.mPrev = aNode;

    mCursorCnt++;

    // head node일 경우에 oldest scn값을 갱신한다.
    if(aNode->mPrev == &mBaseNode)
    {
        ID_SERIAL_BEGIN(mSyncVb++);

        // BUG-27735 : Peterson Algorithm
        ID_SERIAL_EXEC( SM_SET_SCN(&(mBaseNode.mLobViewEnv.mSCN),
                                   &(aNode->mLobViewEnv.mSCN)), 1 );

        ID_SERIAL_END(mSyncVa++);
    }
}

/*********************************************************************
  Name:  smxLCL::remove
  Description:
  circular dobule linked list에 node를 삭제하고,
  head node를 삭제하는 경우에는 head node의
  next node의 mSCN으로 base node의 mSCN을 갱신시킨다.
 **********************************************************************/
void   smxLCL::remove(smLobCursor* aNode)
{

    IDE_DASSERT( aNode != NULL );

    // head node일 경우가 삭제된 경우이기때문에
    // oldestSCN값을 갱신해야 된다.
    if(aNode->mPrev == &mBaseNode)
    {
        ID_SERIAL_BEGIN(mSyncVb++);
        if(mBaseNode.mPrev == aNode)
        {
            //LoCursor가  하나도 없게되는 경우
            //oldestSCN을 무한대로 설정한다.
            // BUG-27735 : Peterson Algorithm
            ID_SERIAL_EXEC(
                SM_SET_SCN_INFINITE(&(mBaseNode.mLobViewEnv.mSCN)),
                1 );
        }//if
        else
        {
            // next node의 SCN으로 oldest scn값을 갱신한다.
            // BUG-27735 : Peterson Algorithm
            ID_SERIAL_EXEC( SM_SET_SCN(&(mBaseNode.mLobViewEnv.mSCN),
                                       &(aNode->mNext->mLobViewEnv.mSCN)),
                            1 );
        }//else
        ID_SERIAL_END(mSyncVa++);
    }//if aNode->mPrev

    aNode->mPrev->mNext = aNode->mNext;
    aNode->mNext->mPrev = aNode->mPrev;
    aNode->mNext = NULL;
    aNode->mPrev = NULL;

    mCursorCnt--;

    return;
}

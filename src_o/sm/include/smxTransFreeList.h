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
 * $Id: smxTransFreeList.h 88191 2020-07-27 03:08:54Z mason.lee $
 **********************************************************************/

#ifndef _O_SMX_TRANS_FREE_LIST_H_
#define _O_SMX_TRANS_FREE_LIST_H_ 1

#include <idu.h>

class smxTrans;

class smxTransFreeList
{
//For Operation
public:
    IDE_RC initialize(smxTrans *aTransArray,
                      UInt      aSeqNumber,
                      SInt      aFstItem,
                      SInt      aLstItem);

    IDE_RC rebuild(UInt aSeqNumber,
                   SInt aFstItem,
                   SInt aLstItem);

    IDE_RC destroy();

    IDE_RC allocTrans(smxTrans **aTrans);

    IDE_RC freeTrans(smxTrans *aTrans);
#if 0
    void dump();
#endif
    IDE_RC lock() { return mMutex.lock( NULL /* idvSQL* */); }
    IDE_RC unlock() { return mMutex.unlock(); }

    void   validateTransFreeList();

//For Member
public:
    SInt      mFreeTransCnt;
    SInt      mCurFreeTransCnt;
    smxTrans *mFstFreeTrans;

private:
    iduMutex  mMutex;
};

#endif

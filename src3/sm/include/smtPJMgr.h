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
 * $Id: smtPJMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/


#ifndef _O_SMT_PJ_MGR_H_  // Parallel Job Manager
#define _O_SMT_PJ_MGR_H_ 1

#include <idl.h>
#include <ideErrorMgr.h>
#include <iduMutex.h>
#include <idtBaseThread.h>

class smtPJChild;

class smtPJMgr : public idtBaseThread
{
    friend class smtPJChild;
    
    smtPJChild       **mChildArray;
protected:
    SInt               mChildCount;
    iduMutex           mMutex;
    idBool             mLocalSuccess;
    idBool            *mGlobalSuccess;
    idBool             mIgnoreError;
    UInt               mErrorNo;
    
public:
    smtPJMgr();
    virtual ~smtPJMgr(){};

    IDE_RC initialize(SInt         aChildCount,
                      smtPJChild **aChildArray,
                      idBool      *aSuccess,
                      idBool       aIgnoreError = ID_FALSE);
    IDE_RC destroy();

    IDE_RC lock() { return mMutex.lock( NULL ); }
    IDE_RC unlock() { return mMutex.unlock(); }
    
    iduMutex* getMutex() { return &mMutex; }
    
    virtual void run();
    
//     virtual IDE_RC restartClient(SInt    aChild, // Child 번호
//                                  idBool  isInit, // 초기화시 호출?
//                                  idBool *aJobAssigned  // 일이 할당되었나?
//                                  ) = 0;
    
    virtual IDE_RC assignJob(SInt    aChild, // Child 번호
                             idBool *aJobAssigned  // 일이 할당되었나?
                             ) = 0;
    idBool    getResult() { return mLocalSuccess; }
    UInt      getErrorNo() { return mErrorNo ; }
};

#endif

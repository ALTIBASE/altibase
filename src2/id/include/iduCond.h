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
 
/***********************************************************************
 * $Id: iduMemory.h 17293 2006-07-25 03:04:24Z mhjeong $
 **********************************************************************/

#ifndef _O_IDU_COND_H_
#define _O_IDU_COND_H_ 1

#include <idl.h>
#include <iduMutex.h>
#include <iduMemDefs.h>

#define IDU_COND_NAME_SIZE      (64)
#define IDU_COND_STATUS_SIZE    (64)
#define IDU_COND_PTR_SIZE       (16)

#define IDU_IGNORE_TIMEDOUT     (ID_TRUE)
#define IDU_CATCH_TIMEDOUT      (ID_FALSE)

typedef struct iduCondCore
{
    PDL_cond_t                  mCond;
    idBool                      mIsTimedOut;
    void*                       mMutex;
    SInt                        mCondStatus;
    struct iduCondCore*         mCondNext;
    struct iduCondCore*         mInfoNext;

    SChar                       mName[IDU_COND_NAME_SIZE];
    SChar*                      mCondStatusString;
    UChar                       mMutexPtr[IDU_COND_PTR_SIZE];
} iduCondCore;

class iduCond
{
    //public member function
public:
    static IDE_RC   initializeStatic(iduPeerType = IDU_SERVER_TYPE);
    static IDE_RC   destroyStatic(void); 
    static SInt     lockCond(void);
    static SInt     unlockCond(void);
    static SInt     lockInfo(void);
    static SInt     unlockInfo(void);

    IDE_RC          initialize(SChar* = NULL);
    IDE_RC          destroy(void);
    IDE_RC          signal(void);
    IDE_RC          broadcast(void);
    IDE_RC          wait(iduMutex*);
    IDE_RC          timedwait(iduMutex*, PDL_Time_Value*,
                              idBool = IDU_CATCH_TIMEDOUT);
    inline idBool   isTimedOut() {return mCore->mIsTimedOut;}

    static iduCondCore* getFirstCond();
    static iduCondCore* getNextCond(iduCondCore*);

private:
    /* List manipulation mutex and head of the list */
    static PDL_thread_mutex_t   mInfoMutex;
    static PDL_thread_mutex_t   mCondMutex;
    static iduCondCore          mHead;
    static iduPeerType          mCondMgrType;

    iduCondCore*                mCore;

    /*
     * Project 2408
     * Pool for condition variables
     */
    static iduMemSmall          mCondPool;
    static idBool               mUsePool;
    static IDE_RC               allocCond(iduCondCore**);
    static IDE_RC               freeCond(iduCondCore*);
};

#endif /* _O_COND_MGR_H_ */


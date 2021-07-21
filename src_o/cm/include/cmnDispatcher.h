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

#ifndef _O_CMN_DISPATCHER_H_
#define _O_CMN_DISPATCHER_H_ 1

/* PROJ-2108 Dedicated thread mode which uses less CPU
 * This timeout(765432) is a magic number to confirm
 * whether the server is in dedicated thread mode
 */
#define DEDICATED_THREAD_MODE_TIMEOUT_FLAG 765432

typedef struct cmnDispatcher
{
    cmnDispatcherImpl       mImpl;

    UInt                    mLinkCount;
    iduList                 mLinkList;

    struct cmnDispatcherOP *mOp;
} cmnDispatcher;


struct cmnDispatcherOP
{
    SChar   *mName;

    IDE_RC (*mInitialize)(cmnDispatcher *aDispatcher, UInt aMaxLink);
    IDE_RC (*mFinalize)(cmnDispatcher *aDispatcher);

    IDE_RC (*mAddLink)(cmnDispatcher *aDispatcher, cmnLink *aLink);
    IDE_RC (*mRemoveLink)(cmnDispatcher *aDispatcher, cmnLink *aLink);
    IDE_RC (*mRemoveAllLinks)(cmnDispatcher *aDispatcher);

    IDE_RC (*mSelect)(cmnDispatcher  *aDispatcher,
                      iduList        *aReadyList,
                      UInt           *aReadyCount,
                      PDL_Time_Value *aTimeout);
};


idBool cmnDispatcherIsSupportedImpl(cmnDispatcherImpl aImpl);

IDE_RC cmnDispatcherAlloc(cmnDispatcher **aDispatcher, cmnDispatcherImpl aImpl, UInt aMaxLink);
IDE_RC cmnDispatcherFree(cmnDispatcher *aDispatcher);

cmnDispatcherImpl cmnDispatcherImplForLinkImpl(cmnLinkImpl aLinkImpl);

IDE_RC cmnDispatcherWaitLink(cmnLink *aLink, cmnDirection aDirection, PDL_Time_Value *aTimeout);

SInt cmnDispatcherCheckHandle(PDL_SOCKET aHandle, PDL_Time_Value *aTimeout);  /* BUG-45240 */

/* BUG-38951 Support to choice a type of CM dispatcher on run-time */
IDE_RC cmnDispatcherInitialize();


#endif

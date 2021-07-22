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

#include <mmErrorCode.h>
#include <mmcTask.h>
#include <mmtDispatcher.h>
#include <mmtThreadManager.h>

#define MMT_DISPATCHER_MAX_LINK     10
#define MMT_DISPATCHER_POLL_TIMEOUT 1000000

mmtDispatcher::mmtDispatcher() : idtBaseThread()
{
}

IDE_RC mmtDispatcher::initialize(cmiDispatcherImpl aDispatcherImpl)
{
    mRun            = ID_FALSE;
    mDispatcherImpl = aDispatcherImpl;
    mDispatcher     = NULL;
    mLinkCount      = 0;

    IDE_TEST(cmiAllocDispatcher(&mDispatcher,
                                aDispatcherImpl,
                                MMT_DISPATCHER_MAX_LINK) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if (mDispatcher != NULL)
    {
        /* Non-reachable */
        cmiFreeDispatcher(mDispatcher);
        mDispatcher = NULL;
    }

    return IDE_FAILURE;
}

IDE_RC mmtDispatcher::finalize()
{
    IDE_TEST(cmiFreeDispatcher(mDispatcher) != IDE_SUCCESS);
    mDispatcher = NULL;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

void mmtDispatcher::run()
{
    PDL_Time_Value  sTimeout;
    iduList         sReadyList;
    iduListNode    *sIterator;
    cmiLink        *sLink;
    UInt            sAddLinkPos = 0;

    IDE_ASSERT(mmtThreadManager::setupDefaultThreadSignal() == IDE_SUCCESS);

    sTimeout.set(0, MMT_DISPATCHER_POLL_TIMEOUT);

    mRun = ID_TRUE;

    while (mRun == ID_TRUE)
    {
        IDE_CLEAR();

        for (; sAddLinkPos < mLinkCount; sAddLinkPos++)
        {
            IDE_TEST_RAISE(cmiAddLinkToDispatcher(mDispatcher, mLink[sAddLinkPos]) != IDE_SUCCESS,
                           AddLinkFail);
        }

        IDE_TEST_RAISE(cmiSelectDispatcher(mDispatcher,
                                           &sReadyList,
                                           NULL,
                                           &sTimeout) != IDE_SUCCESS,
                       SelectFail);

        IDU_LIST_ITERATE(&sReadyList, sIterator)
        {
            sLink = (cmiLink *)sIterator->mObj;

            if (mmtThreadManager::dispatchCallback(sLink, mDispatcherImpl) != IDE_SUCCESS)
            {
                mmtThreadManager::logError(MM_TRC_DISPATCHER_CALLBACK_FAILED);
            }
        }
    }

    return;

    IDE_EXCEPTION(AddLinkFail);
    {
        mmtThreadManager::logError(MM_TRC_DISPATCHER_ADD_LINK_FAILED);
    }
    IDE_EXCEPTION(SelectFail);
    {
        mmtThreadManager::logError(MM_TRC_DISPATCHER_SELECT_FAILED);
    }
    IDE_EXCEPTION_END;
    {
        IDE_CALLBACK_FATAL("error occurred in mmtDispatcher::run()");
    }
}

void mmtDispatcher::stop()
{
    mRun = ID_FALSE;
}

IDE_RC mmtDispatcher::addLink(cmiLink *aLink)
{
    IDE_TEST_RAISE( mLinkCount >= CMI_LINK_IMPL_MAX, TooManyLinks );

    mLink[mLinkCount++] = aLink;

    return IDE_SUCCESS;

    IDE_EXCEPTION( TooManyLinks )
    {
        mmtThreadManager::logError( MM_TRC_DISPATCHER_HAS_TOO_MANY_LINKS );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

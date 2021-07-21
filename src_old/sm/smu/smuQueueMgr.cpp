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
 * $Id: smuQueueMgr.h
 **********************************************************************/

#include <smuQueueMgr.h>

smuQueueMgr::smuQueueMgr()
{
#define IDE_FN "smuQueueMgr::smuQueueMgr()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#undef IDE_FN
}

smuQueueMgr::~smuQueueMgr()
{

#define IDE_FN "smuQueueMgr::~smuQueueMgr()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#undef IDE_FN
}

IDE_RC smuQueueMgr::initialize(iduMemoryClientIndex aIndex,
                               UInt a_nItemSize)
{
#define IDE_FN "smuQueueMgr::initialize()"

    smuQueuePage  * s_pNewPage  = NULL;
    UInt            sState      = 0;

    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mIndex = aIndex;
    IDE_TEST(m_mutex.initialize((SChar *)"QUEUE_MANAGER_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    m_nPageSize = idlOS::getpagesize();

    /* smuQueueMgr_initialize_malloc_NewPage.tc */
    IDU_FIT_POINT("smuQueueMgr::initialize::malloc::NewPage");
    IDE_TEST(iduMemMgr::malloc(mIndex,
                               m_nPageSize,
                               (void**)&s_pNewPage,
                               IDU_MEM_FORCE)
             != IDE_SUCCESS);
    sState = 1;
    
    m_head = s_pNewPage;
    m_head->m_pNext = NULL;

    m_pCurQueue = m_head;
    mFront = 0;
    mRear = 0;
    mQueueLength = 0;

    m_nItemSize = a_nItemSize;
    m_cItemInPage = (m_nPageSize - sizeof(smuQueuePage) + sizeof(SChar)) / m_nItemSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free( s_pNewPage ) == IDE_SUCCESS );
            s_pNewPage = NULL;
        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC smuQueueMgr::destroy()
{
#define IDE_FN "smuQueueMgr::destroy()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    smuQueuePage  *s_pCurQueuePage;
    smuQueuePage  *s_pNxtQueuePage;

    s_pCurQueuePage = m_head;

    while(s_pCurQueuePage != NULL)
    {
        s_pNxtQueuePage = s_pCurQueuePage->m_pNext;
        IDE_TEST(iduMemMgr::free(s_pCurQueuePage)
                 != IDE_SUCCESS);
        s_pCurQueuePage = s_pNxtQueuePage;
    }

    IDE_TEST(m_mutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC smuQueueMgr::dequeue(idBool a_bLock, void* a_pItem, idBool *a_pEmpty)
{
    SInt           s_state = 0;
    smuQueuePage  *s_freePage;

    *a_pEmpty = ID_FALSE;

    if(a_bLock == ID_TRUE)
    {
        IDE_TEST(lock() != IDE_SUCCESS);
        s_state = 1;
    }

    if(m_pCurQueue == m_head && mFront == mRear)
    {
        *a_pEmpty = ID_TRUE;
    }
    else
    {
        if(mFront == m_cItemInPage)
        {
            s_freePage = m_head;
            m_head = m_head->m_pNext;
            
            IDE_TEST(iduMemMgr::free(s_freePage)
                     != IDE_SUCCESS);            
            mFront = 0;
        }

        idlOS::memcpy(a_pItem, m_head->m_Data + (mFront * m_nItemSize), m_nItemSize);
        mFront++;
        mQueueLength--;        
    }

    if(a_bLock == ID_TRUE)
    {
        s_state = 0;
        IDE_TEST(unlock() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(s_state != 0)
    {
        IDE_PUSH();
        (void)unlock();
        IDE_POP();
    }

    return IDE_FAILURE;
}

IDE_RC smuQueueMgr::enqueue(idBool a_bLock, void* a_pItem)
{
    SInt            s_state     = 0;
    smuQueuePage  * s_pNewPage  = NULL;
    UInt            sState      = 0;

    if(a_bLock == ID_TRUE)
    {
        IDE_TEST(lock() != IDE_SUCCESS);
        s_state = 1;
    }

    if( mRear == m_cItemInPage )
    {
        /* smuQueueMgr_enqueue_malloc_NewPage.tc */
        IDU_FIT_POINT("smuQueueMgr::enqueue::malloc::NewPage");
        IDE_TEST(iduMemMgr::malloc(mIndex,
                                   m_nPageSize,
                                   (void**)&s_pNewPage,
                                   IDU_MEM_FORCE)
                 != IDE_SUCCESS);
        sState = 1;
        mRear  = 0;
        
        s_pNewPage->m_pNext = NULL;
        m_pCurQueue->m_pNext = s_pNewPage;

        m_pCurQueue = s_pNewPage;

        mRear = 0;
    }

    idlOS::memcpy(m_pCurQueue->m_Data + (mRear * m_nItemSize), a_pItem, m_nItemSize);
    mRear++;
    mQueueLength++;    

    if(a_bLock == ID_TRUE)
    {
        s_state = 0;
        IDE_TEST(unlock() != IDE_SUCCESS);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            IDE_ASSERT( iduMemMgr::free(s_pNewPage) == IDE_SUCCESS );
            s_pNewPage = NULL;
        default:
            break;
    }

    if(s_state != 0)
    {
        IDE_PUSH();
        (void)unlock();
        IDE_POP();
    }

    return IDE_FAILURE;
}

idBool smuQueueMgr::isQueueEmpty()
{
    idBool     sisEmpty = ID_FALSE;

    if(m_pCurQueue == m_head && mFront == mRear)
    {
        sisEmpty = ID_TRUE;
    }
    return sisEmpty;
}

UInt smuQueueMgr::getQueueLength()
{
    return mQueueLength;
}


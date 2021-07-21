/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduPtrList.cpp 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

#include <idl.h>
#include <ideErrorMgr.h>
#include <ideLog.h>
#include <idErrorCode.h>
#include <iduPtrList.h>

iduPtrList::iduPtrList()
{
#define IDE_FN "iduPtrList::iduPtrList()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#undef IDE_FN
}

iduPtrList::~iduPtrList()
{
#define IDE_FN "iduPtrList::~iduPtrList()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

#undef IDE_FN
}

IDE_RC iduPtrList::initialize(iduMemoryClientIndex aIndex)
{
#define IDE_FN "iduPtrList::initialize()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mIndex = aIndex;

    IDE_TEST(m_mutex.initialize((SChar *)"PTR_LIST_MUTEX",
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL ) != IDE_SUCCESS);

    m_nPageSize = idlOS::getpagesize();
    m_head.m_pPrev = &m_head;
    m_head.m_pNext = &m_head;

    m_pCurPage = &m_head;
    m_nOffset = 0;

    m_nItemSize = sizeof(SChar*);
    m_cItemInPage = (m_nPageSize - sizeof(iduPtrListPage) + sizeof(SChar*)) / m_nItemSize;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduPtrList::destroy()
{
#define IDE_FN "iduPtrList::destroy()"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    iduPtrListPage  *s_pCurListPage;
    iduPtrListPage  *s_pNxtListPage;

    s_pCurListPage = m_head.m_pNext;

    while(s_pCurListPage != &m_head)
    {
        s_pNxtListPage = s_pCurListPage->m_pNext;
        IDE_TEST(iduMemMgr::free(s_pCurListPage)
                 != IDE_SUCCESS);

        s_pCurListPage = s_pNxtListPage;
    }

    m_head.m_pNext = &m_head;
    m_head.m_pPrev = &m_head;

    IDE_TEST(m_mutex.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC iduPtrList::read(idBool    a_bLock,
                        UInt      a_nFlag,
                        UInt      a_cRead,
                        void     *a_pBuffer,
                        SInt     *a_pReadCount)
{
    SInt s_state = 0;
    UInt s_cRead = 0;
    UInt s_cLeftItem;
    UInt s_cReadItem;

    if(a_bLock == ID_TRUE)
    {
        IDE_TEST(lock() != IDE_SUCCESS);
        s_state = 1;
    }

    switch(a_nFlag)
    {
        case IDU_STACK_PTR_READ_FST:
            m_pCurPage = m_head.m_pNext;
            m_nOffset = 0;
            break;

        case IDU_STACK_PTR_READ_NXT:
            if(m_pCurPage != &m_head)
            {
                while(1)
                {
                    s_cLeftItem = m_pCurPage->m_cItem - m_nOffset;

                    if(s_cLeftItem < (a_cRead - s_cRead))
                    {
                        idlOS::memcpy((SChar**)a_pBuffer + s_cRead ,
                                      (SChar**)(m_pCurPage->m_Data) + m_nOffset,
                                      sizeof(SChar*) * s_cLeftItem);

                        s_cRead += s_cLeftItem;
                        m_nOffset += s_cLeftItem;
                        s_cLeftItem = 0;
                    }
                    else
                    {
                        s_cReadItem = a_cRead - s_cRead;

                        idlOS::memcpy((SChar**)a_pBuffer + s_cRead,
                                      (SChar**)(m_pCurPage->m_Data) + m_nOffset,
                                      sizeof(SChar*) * s_cReadItem);

                        s_cRead += s_cReadItem;

                        IDE_ASSERT(s_cRead == a_cRead);
                        m_nOffset += s_cReadItem;

                        break;
                    }

                    //set next page
                    m_nOffset = 0;
                    m_pCurPage = m_pCurPage->m_pNext;

                    if(m_pCurPage == &m_head)
                    {
                        break;
                    }
                }//while
            }
    }//switch

    if(a_pReadCount != NULL)
    {
        *a_pReadCount = s_cRead;
    }

    if(s_state != 0)
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

IDE_RC iduPtrList::add(idBool a_bLock, void* a_pItem)
{
    SInt          s_state = 0;
    iduPtrListPage *s_pNewPage = NULL;

    if(a_bLock == ID_TRUE)
    {
        IDE_TEST(lock() != IDE_SUCCESS);
        s_state = 1;
    }

    IDE_ASSERT(a_pItem != NULL);

    if(m_nOffset == m_cItemInPage || m_pCurPage == &m_head)
    {
        if(m_pCurPage->m_pNext == &m_head)
        {
            // refine시 객체사용 : immediate 모드 OK
            IDE_TEST(iduMemMgr::malloc(mIndex,
                                       m_nPageSize,
                                       (void**)&s_pNewPage)
                     != IDE_SUCCESS);

            s_pNewPage->m_pPrev = m_head.m_pPrev;
            s_pNewPage->m_pNext = &m_head;
            s_pNewPage->m_cItem = 0;
            m_head.m_pPrev->m_pNext = s_pNewPage;
            m_head.m_pPrev = s_pNewPage;

            m_pCurPage = s_pNewPage;
        }
        else
        {
            m_pCurPage = m_pCurPage->m_pNext;
        }

        m_nOffset = 0;
    }

    *((void**)((SChar*)(m_pCurPage->m_Data) + sizeof(void*) * m_nOffset)) = a_pItem;

    m_nOffset++;
    m_pCurPage->m_cItem++;

    assert(m_pCurPage->m_cItem == m_nOffset);

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

void iduPtrList::merge(iduPtrList* a_pPtrList)
{
    if(a_pPtrList->m_head.m_pNext != &(a_pPtrList->m_head))
    {
        m_head.m_pPrev->m_pNext = a_pPtrList->m_head.m_pNext;

        a_pPtrList->m_head.m_pNext->m_pPrev = m_head.m_pPrev;
        a_pPtrList->m_head.m_pPrev->m_pNext = &m_head;

        m_head.m_pPrev = a_pPtrList->m_head.m_pPrev;

        a_pPtrList->m_head.m_pPrev = &(a_pPtrList->m_head);
        a_pPtrList->m_head.m_pNext = &(a_pPtrList->m_head);
        a_pPtrList->m_pCurPage = &(a_pPtrList->m_head);
        a_pPtrList->m_nOffset = 0;
    }
}

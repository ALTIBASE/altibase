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
 * $Id: smmDirtyPageList.cpp 90083 2021-02-26 00:58:48Z et16 $
 **********************************************************************/

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <smm.h>
#include <smErrorCode.h>
#include <smmManager.h>
#include <smmDirtyPageList.h>

smmDirtyPageList::smmDirtyPageList()
{
}

smmDirtyPageList::~smmDirtyPageList()
{
}

IDE_RC smmDirtyPageList::initialize(scSpaceID aSpaceID, UInt aSeqNumber)
{

    UInt sPool;
    SChar sBuffer[128];

    mSpaceID = aSpaceID ;
    
    idlOS::memset(sBuffer, 0, 128);

    idlOS::snprintf(sBuffer, 128, "DP_LIST_MUTEX_%"ID_UINT32_FMT, aSeqNumber);

    sPool = smuProperty::getDirtyPagePool();
    
    IDE_TEST(m_mutex.initialize(sBuffer,
                                IDU_MUTEX_KIND_NATIVE,
                                IDV_WAIT_INDEX_NULL) != IDE_SUCCESS);
    
    IDE_TEST(m_memList.initialize(IDU_MEM_SM_SMM,
                                  aSeqNumber,
                                  (SChar *)"DP_MEMLIST",
                                  ID_SIZEOF(smmDirtyPage),
                                  sPool,
                                  IDU_AUTOFREE_CHUNK_LIMIT)
             != IDE_SUCCESS);
    
    m_count  = 0;
    m_head   = NULL;
    m_opened = ID_FALSE;

    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smmDirtyPageList::destroy()
{
    IDE_TEST(m_mutex.destroy() != IDE_SUCCESS); 
    IDE_TEST(m_memList.destroy() != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

// mutex 잡은 이후에 호출되어야 함.
// called by DirtyPageMgr directly
IDE_RC smmDirtyPageList::addDirtyPage(scPageID a_page_no)
{
    smmDirtyPage *sDirtyPage;

    /* smmDirtyPageList_addDirtyPage_alloc_DirtyPage.tc */
    IDU_FIT_POINT("smmDirtyPageList::addDirtyPage::alloc::DirtyPage");
    IDE_TEST(m_memList.alloc((void **)&sDirtyPage) != IDE_SUCCESS);    
    sDirtyPage->m_pch  = smmManager::getPCHSlot(mSpaceID, a_page_no);
    ((smmPCH*)(sDirtyPage->m_pch->mPCH))->m_dirty = ID_TRUE;

    // link to list 
    sDirtyPage->m_next = m_head;
    m_head             = sDirtyPage;
    m_count++;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

IDE_RC smmDirtyPageList::open()
{
    
    IDE_ASSERT(m_opened == ID_FALSE);
    IDE_TEST(lock() != IDE_SUCCESS);
    
    m_opened = ID_TRUE;
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}

IDE_RC smmDirtyPageList::read( smPCSlot **a_pch )
{
    
    IDE_ASSERT(m_opened == ID_TRUE);

    if (m_head == NULL)
    {
        *a_pch = NULL;
    }
    else
    {
        smmDirtyPage *sRead;
        
        sRead  = m_head;
        *a_pch = sRead->m_pch;
        
        m_head = m_head->m_next;
        IDE_TEST( m_memList.memfree(sRead) != IDE_SUCCESS);
        m_count--;
    }
    return IDE_SUCCESS;
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
}


IDE_RC smmDirtyPageList::close()
{

    IDE_ASSERT(m_opened == ID_TRUE);
    
    IDE_TEST(unlock() != IDE_SUCCESS);
    m_opened = ID_FALSE;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    return IDE_FAILURE;
    
}

IDE_RC smmDirtyPageList::clear()
{

    smPCSlot *sPCHSlotPtr;
    smmPCH      *sPCHPtr;

    while(1)
    {
        IDE_TEST( read(&sPCHSlotPtr) != IDE_SUCCESS );
        
        if (sPCHSlotPtr == NULL)
        {
            break;
        }
        sPCHPtr = (smmPCH*)sPCHSlotPtr->mPCH;
        IDE_ASSERT( sPCHPtr != NULL );

        sPCHPtr->m_dirty = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

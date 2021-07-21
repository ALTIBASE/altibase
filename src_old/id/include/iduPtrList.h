/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: iduPtrList.h 66405 2014-08-13 07:15:26Z djin $
 ****************************************************************************/

#ifndef _O_IDU_PTR_LIST_H_
#define _O_IDU_PTR_LIST_H_ 1

#include <idl.h>
#include <idu.h>
#include <ideErrorMgr.h>
#include <iduMutex.h>

#define IDU_STACK_PTR_READ_FST  (0x00000002)
#define IDU_STACK_PTR_READ_NXT  (0x00000004)

typedef struct iduPtrListPage
{
    struct iduPtrListPage* m_pPrev;
    struct iduPtrListPage* m_pNext;
    UInt                   m_cItem;
    UInt                   m_nAlign;
    SChar                  m_Data[1];
}iduPtrListPage;

class iduPtrList
{
private:
    iduMemoryClientIndex mIndex;
    
    iduPtrListPage   m_head;

    iduPtrListPage*  m_pCurPage;
    UInt             m_nOffset;

    UInt             m_nItemSize;
    
    iduMutex         m_mutex;
    UInt             m_nPageSize;
    UInt             m_cItemInPage;
    
public:
    iduPtrList();
    ~iduPtrList();

    IDE_RC initialize(iduMemoryClientIndex aIndex);
    IDE_RC destroy();


    IDE_RC lock() { return m_mutex.lock(NULL); }
    IDE_RC unlock() { return m_mutex.unlock(); }

    IDE_RC read(idBool    a_bLock,
                UInt      a_nFlag,
                UInt      a_cRead,
                void     *a_pBuffer,
                SInt     *a_pReadCount);
        
    IDE_RC add(idBool a_bLock, void* a_pItem);
    void merge(iduPtrList*);
};

#endif

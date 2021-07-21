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
 * $Id: smmDirtyPageList.h 90083 2021-02-26 00:58:48Z et16 $
 **********************************************************************/

#ifndef _O_SMM_DIRTY_PAGE_LIST_H_
#define _O_SMM_DIRTY_PAGE_LIST_H_ 1

#include <idl.h>
#include <iduMutex.h>
#include <smmDef.h>
#include <iduMemListOld.h>

class smmDirtyPageMgr;

class smmDirtyPageList
{
    friend class smmDirtyPageMgr;
    scSpaceID     mSpaceID;
    iduMemListOld m_memList;
    vULong        m_count; // dirty page count;
    smmDirtyPage *m_head;
    idBool        m_opened;
    
private:
    iduMutex      m_mutex;
    
protected:
    // directly called by DirtyPageMgr 
    IDE_RC addDirtyPage( scPageID a_page_no ); 
public:
    smmDirtyPageList();
    ~smmDirtyPageList();
    IDE_RC initialize(scSpaceID aSpaceID, UInt aSeqNumber);
    IDE_RC destroy();
    IDE_RC clear();

    // FOR CheckPoint API but, smmDirtyPageMgr는 아래 API를 사용하지 않음
    IDE_RC open();
    IDE_RC read(smPCSlot **a_pch); // auto memory release, in case NULL-end;
    IDE_RC close();
    vULong getCount() { return m_count; }
    
    IDE_RC lock() { return m_mutex.lock( NULL ); }
    IDE_RC unlock() { return m_mutex.unlock(); }
    
};

#endif

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
 * $Id: smmDirtyPageMgr.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef _O_SMM_DIRTY_PAGE_MGR_H_
#define _O_SMM_DIRTY_PAGE_MGR_H_ 1

#include <idl.h>
#include <idu.h>
#include <iduHash.h>
#include <smmDef.h>
#include <smmDirtyPageList.h>

class smmDirtyPageMgr
{
private:
    // 이 Dirty Page Mgr은  이 Tablespace에 속한 Dirty Page만을 관리한다.
    scSpaceID mSpaceID ; 
    
    SInt m_listCount; // list 갯수
    smmDirtyPageList *m_list;
    
    IDE_RC lockDirtyPageList(smmDirtyPageList **a_list);
    
public:
    IDE_RC initialize(scSpaceID aSpaceID, SInt a_listCount);
    IDE_RC destroy();

    // 모든 Dirty Page List들의 Dirty Page들을 제거한다.
    IDE_RC removeAllDirtyPages();
    
    // Dirty Page 리스트에 변경된 Page를 추가한다.
    IDE_RC insDirtyPage(scPageID aPageID ); 
    
    smmDirtyPageList* getDirtyPageList(SInt a_num)
    {
        IDE_ASSERT(a_num < m_listCount);
        return &m_list[a_num];
    }

    SInt  getDirtyPageListCount() { return m_listCount; }

    
   /***********************************************************************
      여러 Tablespace에 대한 Dirty Page연산을 관리하는 인터페이스
      Refactoring을 통해 추후 별도의 Class로 빼도록 한다.
    ***********************************************************************/
public :
    // Dirty Page 리스트에 변경된 Page를 추가한다.
    static IDE_RC insDirtyPage(scSpaceID aSpaceID, scPageID aPageID );
    static IDE_RC insDirtyPage(scSpaceID aSpaceID, void *   a_new_page);
    
    // Dirty Page관리자를 생성한다.
    static IDE_RC initializeStatic();

    // Dirty Page관리자를 파괴한다.
    static IDE_RC destroyStatic();

    // 특정 Tablespace를 위한 Dirty Page관리자를 생성한다.
    static IDE_RC createDPMgr(smmTBSNode * aTBSNode );
    
    // 특정 Tablespace를 위한 Dirty Page관리자를 찾아낸다.
    static IDE_RC findDPMgr( scSpaceID aSpaceID, smmDirtyPageMgr ** aDPMgr );
    
    // 특정 Tablespace의 Dirty Page관리자를 제거한다.
    static IDE_RC removeDPMgr(smmTBSNode * aTBSNode);
};

#endif

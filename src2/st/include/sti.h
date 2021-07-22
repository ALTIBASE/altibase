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
 

/******************************************************************************
 * $Id:  $
 *****************************************************************************/

#ifndef _O_STI_H_
#define _O_STI_H_ 1

#include <idl.h>
#include <stnmrFT.h>
#include <stnmrModule.h>
#include <stmFixedTable.h>

// PROJ-1726 performance view definition
// st/sti/sti.cpp 와 qp/qcm/qcmPerformanceView.cpp 두 군데에서
// 사용되므로 한 코드로 관리하기 위하여 #define 으로 정의함.
#define ST_PERFORMANCE_VIEWS \
    (SChar*)"CREATE VIEW V$MEM_RTREE_HEADER "\
               "(INDEX_NAME, INDEX_ID, TABLE_TBS_ID,"\
               "TREE_MBR_MIN_X, TREE_MBR_MIN_Y, TREE_MBR_MAX_X, TREE_MBR_MAX_Y, "\
               "USED_NODE_COUNT, PREPARE_NODE_COUNT) "\
            "AS SELECT "\
               "INDEX_NAME, INDEX_ID, TABLE_TBS_ID,"\
               "TREE_MBR_MIN_X, TREE_MBR_MIN_Y, TREE_MBR_MAX_X, TREE_MBR_MAX_Y, "\
               "USED_NODE_COUNT, PREPARE_NODE_COUNT "\
            "FROM X$MEM_RTREE_HEADER",\
\
    (SChar*)"CREATE VIEW V$MEM_RTREE_NODEPOOL "\
               "(TOTAL_PAGE_COUNT, TOTAL_NODE_COUNT, FREE_NODE_COUNT, USED_NODE_COUNT, "\
               "NODE_SIZE, TOTAL_ALLOC_REQ, TOTAL_FREE_REQ, FREE_REQ_COUNT ) "\
            "AS SELECT "\
               "TOTAL_PAGE_COUNT, TOTAL_NODE_COUNT, FREE_NODE_COUNT,"\
               "USED_NODE_COUNT, "\
               "NODE_SIZE, TOTAL_ALLOC_REQ, TOTAL_FREE_REQ, FREE_REQ_COUNT "\
            "FROM X$MEM_RTREE_NODEPOOL"
// 주의 : 마지막 performance view 에는 ',' 를 생략할 것!

class sti
{
private:

public:

    static IDE_RC addExtSM_Recovery( void );

    static IDE_RC addExtSM_Index( void );
        
    static IDE_RC addExtMT_Module( void );

    static IDE_RC addExtQP_Callback( void );

    static IDE_RC initSystemTables( void );

    // Proj-2059 DB Upgrade
    // Geometry타입 확인을 위해 Tool에서 Insert가능한 Text형태로 출력할
    // 수 있어야 합니다.
    static IDE_RC getTextFromGeometry(
                    void*               aObj,
                    UChar*              aBuf,
                    UInt                aMaxSize,
                    UInt*               aOffset,
                    IDE_RC*             aReturn);

    // PROJ-2166 Spatial Validation
    static IDE_RC initialize( void );
};

#endif /* _O_STI_H_ */


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
 * $Id: smnModules.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h> // hjohn add for win32 porting
#include <smnModules.h>
#include <smnnModule.h>
#include <smnbModule.h>
#include <sdnnModule.h>
#include <sdnpModule.h>
#include <sdnbModule.h>
#include <smnfModule.h>
#include <svnModules.h>

/*    
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  |       | 이름  | Meta | Temp | Memory | Disk  | Fixed | Volatile | Remote |
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  | SEQ   |       |      |      |        |       |       |          |        |
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  | BTree |       |      |      |        |       |       |          |        |
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  | RTree | <=============== 외부 등록 ====================================> |
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  | Hash  |       |      |      |        |       |       |          |        |
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  | BTree2|       |      |      |        |       |       |          |        |
  +-------+-------+------+------+--------+-------+-------+----------+--------+
  | 3DRTr | <=============== 외부 등록 ====================================> |
  +-------+-------+------+------+--------+-------+-------+----------+--------+ */

smnIndexType gSmnSequentialScan =
   { SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID /* 0 */,
     SMN_BUILTIN_INDEX,
     SMN_INDEX_UNIQUE_DISABLE          |
     SMN_INDEX_COMPOSITE_DISABLE       |
     SMN_INDEX_NUMERIC_COMPARE_DISABLE |
     SMN_INDEX_AGING_ENABLE,
     (SChar*)"SEQUENTIAL",
     {&smnnMetaModule, NULL, &smnnModule, &sdnnModule, &smnfModule, &svnnModule, NULL}
   };

smnIndexType gSmnBTreeIndex =
   { SMI_BUILTIN_B_TREE_INDEXTYPE_ID /* 1 */,
     SMN_BUILTIN_INDEX,
     SMN_INDEX_UNIQUE_ENABLE          |
     SMN_INDEX_COMPOSITE_ENABLE       |
     SMN_INDEX_NUMERIC_COMPARE_ENABLE |
     SMN_INDEX_AGING_ENABLE,
     (SChar*)"BTREE",
     {&smnbMetaModule, NULL, &smnbModule, &sdnbModule, NULL, &svnbModule, NULL}
   };

smnIndexType gSmnGRIDScan =
   { SMI_BUILTIN_GRID_INDEXTYPE_ID /* 4 */,
     SMN_BUILTIN_INDEX,
     SMN_INDEX_UNIQUE_DISABLE          |
     SMN_INDEX_COMPOSITE_DISABLE       |
     SMN_INDEX_NUMERIC_COMPARE_DISABLE |
     SMN_INDEX_AGING_DISABLE,
     (SChar*)"_PROWID",
     {NULL, NULL, &smnpModule, &sdnpModule, NULL, &svnpModule, NULL}
   };

smnIndexType * gSmnAllIndex[SMI_INDEXTYPE_COUNT];


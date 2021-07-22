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
 
#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smDef.h>
#include <svcRecord.h>
#include <svnModules.h>
#include <smnManager.h>
#include <smnbModule.h>

static IDE_RC svnbIndexMemoryNA(const smnIndexModule*);

smnIndexModule svnbModule =
{
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_VOLATILE,
                              SMI_BUILTIN_B_TREE_INDEXTYPE_ID ),
    SMN_RANGE_ENABLE        |
      SMN_DIMENSION_DISABLE |
      SMN_DEFAULT_ENABLE    |
      SMN_BOTTOMUP_BUILD_ENABLE,
    SMP_VC_PIECE_MAX_SIZE - 1,  // BUG-23113

    /* 아래 함수 포인터 멤버에 smnbBTree::prepareXXX, releaseXXX 함수들을
       사용해서는 안된다. 그 함수들은 한 모듈 인스턴스내에서만
       사용되어야 한다. 즉,
          smnbBTree::prepareIteratorMem
          smnbBTree::releaseIteratorMem
          smnbBTree::prepareFreeNodeMem
          smnbBTree::releaseFreeNodeMem
       들을 사용하면 안된다. 왜냐하면 smnbModule과 svnbModule이
       위 함수들을 같이 사용하면 위 함수들이 접근하는
       mIteratorPool, gSmnbNodePool, gSmnbFreeNodeList 전역 변수 또는 멤버들을
       공유하게 되는데, 이들에 대한 초기화 및 해제 작업을 중복으로 하게 된다.
       따라서 volatile B-tree index module에서는 이 함수가 불려질 경우
       아무런 작업도 하지 않도록 하기 위해 svnbIndexMemoryNA
       함수 포인터를 달도록 한다. */

    (smnMemoryFunc)       svnbIndexMemoryNA,
    (smnMemoryFunc)       svnbIndexMemoryNA,
    (smnMemoryFunc)       svnbIndexMemoryNA,
    (smnMemoryFunc)       svnbIndexMemoryNA,
    (smnCreateFunc)       smnbBTree::create,
    (smnDropFunc)         smnbBTree::drop,

    (smTableCursorLockRowFunc) smnManager::lockRow,
    (smnDeleteFunc)            smnbBTree::deleteNA,
    (smnFreeFunc)              smnbBTree::freeSlot,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,

    (smInitFunc)         smnbBTree::init,
    (smDestFunc)         smnbBTree::dest,
    (smnFreeNodeListFunc) smnbBTree::freeAllNodeList,
    (smnGetPositionFunc)  smnbBTree::getPosition,
    (smnSetPositionFunc)  smnbBTree::setPosition,

    (smnAllocIteratorFunc) smnbBTree::allocIterator,
    (smnFreeIteratorFunc)  smnbBTree::freeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,
    (smnMakeDiskImageFunc) smnbBTree::makeDiskImage,

    (smnBuildIndexFunc)     smnbBTree::buildIndex,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         smnbBTree::gatherStat
};

IDE_RC svnbIndexMemoryNA(const smnIndexModule*)
{
    return IDE_SUCCESS;
}


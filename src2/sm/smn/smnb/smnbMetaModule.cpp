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
 

/*******************************************************************************
 * $Id: smnbMetaModule.cpp 86324 2019-11-01 09:11:29Z et16 $
 ******************************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smDef.h>
#include <smnManager.h>
#include <smnbModule.h>

static IDE_RC smnbMetaAA( const smnIndexModule* );

smnIndexModule smnbMetaModule =
{
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_META,
                              SMI_BUILTIN_B_TREE_INDEXTYPE_ID ),
    SMN_RANGE_ENABLE | SMN_DIMENSION_DISABLE | SMN_DEFAULT_ENABLE |
        SMN_BOTTOMUP_BUILD_ENABLE,
    SMP_VC_PIECE_MAX_SIZE - 1,  // BUG-23113
    (smnMemoryFunc)       smnbMetaAA,
    (smnMemoryFunc)       smnbMetaAA,
    (smnMemoryFunc)       NULL,
    (smnMemoryFunc)       NULL,
    (smnCreateFunc)       smnbBTree::create,
    (smnDropFunc)         smnbBTree::drop,

    (smTableCursorLockRowFunc)  smnManager::lockRow,
    (smnDeleteFunc)             smnbBTree::deleteNA,
    (smnFreeFunc)               smnbBTree::freeSlot,
    (smnInsertRollbackFunc)     NULL,
    (smnDeleteRollbackFunc)     NULL,
    (smnAgingFunc)              NULL,

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

IDE_RC smnbMetaAA( const smnIndexModule* )
{
    return IDE_SUCCESS;
}


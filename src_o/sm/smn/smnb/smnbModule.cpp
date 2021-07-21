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
 * $Id: smnbModule.cpp 91256 2021-07-19 01:28:30Z emlee $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <idCore.h>
#include <smc.h>
#include <smm.h>
#include <smu.h>
#include <smDef.h>
#include <smErrorCode.h>
#include <smnReq.h>
#include <smnManager.h>
#include <smnbModule.h>
#include <sgmManager.h>
#include <smiMisc.h>

extern smiGlobalCallBackList gSmiGlobalCallBackList;

smmSlotList  smnbBTree::mSmnbNodePool;
void*        smnbBTree::mSmnbFreeNodeList; /* AGER 가 정리하기 위해 들고있는 NODE LIST */

UInt         smnbBTree::mNodeSize;
UInt         smnbBTree::mIteratorSize;
UInt         smnbBTree::mNodeSplitRate;     /* BUG-40509 */


//fix BUG-23007
/* Index Node Pool */

smnIndexModule smnbModule =
{
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_MEMORY,
                              SMI_BUILTIN_B_TREE_INDEXTYPE_ID ),
    SMN_RANGE_ENABLE | SMN_DIMENSION_DISABLE | SMN_DEFAULT_ENABLE |
        SMN_BOTTOMUP_BUILD_ENABLE | SMN_FREE_NODE_LIST_ENABLE,
    SMP_VC_PIECE_MAX_SIZE - 1,  // BUG-23113
    (smnMemoryFunc)       smnbBTree::prepareIteratorMem,
    (smnMemoryFunc)       smnbBTree::releaseIteratorMem,
    (smnMemoryFunc)       smnbBTree::prepareFreeNodeMem,
    (smnMemoryFunc)       smnbBTree::releaseFreeNodeMem,
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

static const  smSeekFunc smnbSeekFunctions[32][12] =
{
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirstU,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirstR,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirstU,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)smnbBTree::fetchNextU,
        (smSeekFunc)smnbBTree::fetchPrevU,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::beforeFirstR,
        (smSeekFunc)smnbBTree::afterLastR,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnbBTree::fetchNextU,
        (smSeekFunc)smnbBTree::fetchPrevU,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::beforeFirst
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLastU,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLastR,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLastU,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)smnbBTree::fetchPrevU,
        (smSeekFunc)smnbBTree::fetchNextU,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::afterLastR,
        (smSeekFunc)smnbBTree::beforeFirstR,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnbBTree::fetchPrev,
        (smSeekFunc)smnbBTree::fetchNext,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnbBTree::fetchPrevU,
        (smSeekFunc)smnbBTree::fetchNextU,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::beforeFirst,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::retraverse,
        (smSeekFunc)smnbBTree::afterLast
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
        (smSeekFunc)smnbBTree::NA,
    }
};

void smnbBTree::setIndexProperties()
{
    /* PROJ-2433
     * NODE SIZE는 property __MEM_BTREE_INDEX_NODE_SIZE 값이다. */

    /* BUG-46402
     * PAGE 단위가 아닌 NODE 단위로 mempool alloc 받도록 수정했기 때문에
     * PAGE의 낭비되는 공간이 없도록 node size를 보정하는 작업은 필요없게 되었다. */
    mNodeSize     = idlOS::align( smuProperty::getMemBtreeNodeSize() );

    /* PROJ-2433
     * mIteratorSize는 btree index 공통으로 사용하는 smnbIterator에 할당할 메모리 크기이다.
     * smnbIterator는 헤더 + rows[] 로 구성되며,
     * rows[]에는 fetch될 node의 row pointer들이 저장된다.
     * 따라서 row[]의 최대크기는 (node 크기 - smnbLNode 크기) 이다.
     * highfence를 사용하기때문에 row poniter + 1개 추가용량을 확보한다 */
    mIteratorSize = ( ID_SIZEOF( smnbIterator ) +
                      mNodeSize - ID_SIZEOF( smnbLNode ) );

    smnbBTree::setNodeSplitRate(); /* BUG-40509 */
}

IDE_RC smnbBTree::prepareIteratorMem( const smnIndexModule* )
{
    setIndexProperties();

    return IDE_SUCCESS;
}

IDE_RC smnbBTree::releaseIteratorMem(const smnIndexModule* )
{
    return IDE_SUCCESS;
}


IDE_RC smnbBTree::prepareFreeNodeMem( const smnIndexModule* )
{
    IDE_TEST( mSmnbNodePool.initialize( IDU_MEM_SM_INDEX,
                                        "SMNB_INDEX_NODE_POOL",
                                        mNodeSize,
                                        SMNB_NODE_POOL_MAXIMUM,
                                        SMNB_NODE_POOL_CACHE )
              != IDE_SUCCESS );

    IDE_TEST( smnManager::allocFreeNodeList(
                                        (smnFreeNodeFunc)smnbBTree::freeNode,
                                        &mSmnbFreeNodeList)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::releaseFreeNodeMem(const smnIndexModule* )
{
    smnManager::destroyFreeNodeList(mSmnbFreeNodeList);

    IDE_TEST( mSmnbNodePool.release() != IDE_SUCCESS );
    IDE_TEST( mSmnbNodePool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::freeAllNodeList(idvSQL         * /*aStatistics*/,
                                  smnIndexHeader * aIndex,
                                  void           * /*aTrans*/)
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    volatile smnbLNode  * sCurLNode = NULL;
    volatile smnbINode  * sFstChildINode;
    volatile smnbINode  * sCurINode;
    smnbHeader          * sHeader;
    smmSlot               sSlots = { &sSlots, &sSlots };
    smmSlot             * sFreeNode;
    SInt                  sSlotCount = 0;
    ULong                 sDestroyFailCnt = 0;

    IDE_DASSERT( aIndex != NULL );

    sHeader = (smnbHeader*)(aIndex->mHeader);

    if ( sHeader != NULL )
    {
        sFstChildINode = (smnbINode*)(sHeader->root);

        while(sFstChildINode != NULL)
        {
            sCurINode = sFstChildINode;
            sFstChildINode = (smnbINode*)(sFstChildINode->mChildPtrs[0]);

            if ( SMNB_IS_LEAF_NODE ( sCurINode ) )
            {
                sCurLNode = (smnbLNode*)sCurINode;

                while(sCurLNode != NULL)
                {
                    sSlotCount++;

                    if ( destroyNode( (smnbNode*)sCurLNode ) != IDE_SUCCESS )
                    {
                        sDestroyFailCnt++;
                    }

                    sFreeNode = (smmSlot*)sCurLNode;

                    sFreeNode->prev = sSlots.prev;
                    sFreeNode->next = &sSlots;

                    sSlots.prev->next = sFreeNode;
                    sSlots.prev = sFreeNode;

                    sCurLNode = sCurLNode->nextSPtr;
                }

                sFreeNode = sSlots.next;

                while(sFreeNode != &sSlots)
                {
                    sFreeNode = sFreeNode->next;
                }

                sSlots.next->prev = sSlots.prev;
                sSlots.prev->next = sSlots.next;

                IDE_TEST( mSmnbNodePool.releaseSlots( sSlotCount, sSlots.next )
                          != IDE_SUCCESS );

                break;
            }
            else
            {
                while( sCurINode != NULL )
                {
                    sSlotCount++;

                    if ( destroyNode( (smnbNode*)sCurINode ) != IDE_SUCCESS )
                    {
                        sDestroyFailCnt++;
                    }

                    sFreeNode = (smmSlot*)sCurINode;

                    sFreeNode->prev = sSlots.prev;
                    sFreeNode->next = &sSlots;

                    sSlots.prev->next = sFreeNode;
                    sSlots.prev = sFreeNode;

                    sCurINode = sCurINode->nextSPtr;
                }
            }
        }

        sHeader->pFstNode = NULL;
        sHeader->pLstNode = NULL;
        sHeader->root     = NULL;
    }

    if ( sDestroyFailCnt != 0 )
    {
        ideLog::log( IDE_ERR_0,
                     "fail to destroy node - smnbBTree::freeAllNodeList, Count[%"ID_UINT64_FMT"]",
                     sDestroyFailCnt );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDestroyFailCnt != 0 )
    {
        ideLog::log( IDE_ERR_0,
                     "fail to destroy node - smnbBTree::freeAllNodeList, Count[%"ID_UINT64_FMT"]",
                     sDestroyFailCnt );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::freeNode( smnbNode* aNodes )
{
    smmSlot* sSlots;
    smmSlot* sSlot;
    UInt     sCount;
    ULong    sDestroyFailCnt = 0;

    if ( aNodes != NULL )
    {
        if ( destroyNode( aNodes ) != IDE_SUCCESS )
        {
            sDestroyFailCnt++;
        }

        sSlots       = (smmSlot*)aNodes;
        aNodes       = (smnbNode*)(aNodes->freeNodeList);
        sSlots->prev = sSlots;
        sSlots->next = sSlots;
        sCount       = 1;

        while( aNodes != NULL )
        {
            if ( destroyNode( aNodes ) != IDE_SUCCESS )
            {
                sDestroyFailCnt++;
            }

            sSlot             = (smmSlot*)aNodes;
            aNodes            = (smnbNode*)(aNodes->freeNodeList);
            sSlot->prev       = sSlots;
            sSlot->next       = sSlots->next;
            sSlots->next      = sSlot;
            sSlot->next->prev = sSlot;
            sCount++;
        }

        IDE_TEST( mSmnbNodePool.releaseSlots( sCount, sSlots )
                      != IDE_SUCCESS );
    }

    if ( sDestroyFailCnt != 0 )
    {
        ideLog::log( IDE_ERR_0,
                     "fail to destroy node - smnbBTree::freeNode, Count[%"ID_UINT64_FMT"]",
                     sDestroyFailCnt );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDestroyFailCnt != 0 )
    {
        ideLog::log( IDE_ERR_0,
                     "fail to destroy node - smnbBTree::freeNode, Count[%"ID_UINT64_FMT"]",
                     sDestroyFailCnt );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::create( idvSQL               */*aStatistics*/,
                          smcTableHeader*       aTable,
                          smnIndexHeader*       aIndex,
                          smiSegAttr   *        /*aSegAttr*/,
                          smiSegStorageAttr*    /*aSegStoAttr */,
                          smnInsertFunc*        aInsert,
                          smnIndexHeader**      aRebuildIndexHeader,
                          ULong                 /*aSmoNo*/ )
{
    smnbHeader       * sHeader;
    smnbHeader       * sSrcIndexHeader;
    UInt               sStage = 0;
    UInt               sIndexCount;
    const smiColumn  * sColumn;
    smnbColumn       * sIndexColumn;
    smnbColumn       * sIndexColumn4Build;
    UInt               sAlignVal;
    UInt               sMaxAlignVal = 0;
    UInt               sOffset = 0;
    SChar              sBuffer[IDU_MUTEX_NAME_LEN];
    UInt               sNonStoringSize = 0;
    UInt               sCompareFlags   = 0;

    //TC/FIT/Limit/sm/smn/smnb/smnbBTree_create_malloc1.sql
    IDU_FIT_POINT_RAISE( "smnbBTree::create::malloc1",
                          ERR_INSUFFICIENT_MEMORY );

    // fix BUG-22898
    // 메모리 b-tree Run Time Header 생성
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                       ID_SIZEOF(smnbHeader),
                                       (void**)&sHeader ) != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );
    sStage = 1;

    // BUG-28856 logging 병목 제거 (NATIVE -> POSIX)
    // TASK-4102에서 POSIX가 더 효율적인것으로 판단됨
    idlOS::snprintf(sBuffer, 
                    IDU_MUTEX_NAME_LEN, 
                    "SMNB_MUTEX_%"ID_UINT64_FMT,
                    (ULong)(aIndex->mId));
    IDE_TEST( sHeader->mTreeMutex.initialize(
                                sBuffer,
                                IDU_MUTEX_KIND_POSIX,
                                IDV_WAIT_INDEX_NULL )
                != IDE_SUCCESS );

    mSmnbNodePool.makeChild( SMM_SLOT_LIST_MAXIMUM_DEFAULT,
                             SMM_SLOT_LIST_CACHE_DEFAULT,
                             &sHeader->mNodePool );

    sStage = 2;

    //TC/TC/Limit/sm/smn/smnb/smnbBTree_create_malloc2.sql
    IDU_FIT_POINT_RAISE( "smnbBTree::create::malloc2",
                          ERR_INSUFFICIENT_MEMORY );

    // fix BUG-22898
    // Index Header의 Columns 생성
    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 ID_SIZEOF(smnbColumn) * (aIndex->mColumnCount),
                                 (void**)&(sHeader->columns) ) != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );

    //TC/FIT/Limit/sm/smn/smnb/smnbBTree_create_malloc3.sql
    IDU_FIT_POINT_RAISE( "smnbBTree::create::malloc3",
                          ERR_INSUFFICIENT_MEMORY );

    IDE_TEST_RAISE( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 ID_SIZEOF(smnbColumn) * (aIndex->mColumnCount),
                                 (void**)&(sHeader->columns4Build) ) != IDE_SUCCESS,
                    ERR_INSUFFICIENT_MEMORY );

    sHeader->fence = sHeader->columns + aIndex->mColumnCount;
    sHeader->fence4Build = sHeader->columns4Build + aIndex->mColumnCount;

    sStage = 3;

    sHeader->mIsCreated    = ID_FALSE;
    sHeader->mIsConsistent = ID_TRUE;
    sHeader->root          = NULL;
    sHeader->pFstNode      = NULL;
    sHeader->pLstNode      = NULL;
    sHeader->nodeCount     = 0;
    sHeader->tempPtr       = NULL;
    sHeader->cRef          = 0;
    sHeader->mSpaceID      = aTable->mSpaceID; /* FIXED TABLE "X$MEM_BTREE_HEADER" 에서만 사용됨. */
    sHeader->mIsMemTBS     = smLayerCallback::isMemTableSpace( aTable->mSpaceID );

    // To fix BUG-17726
    if ( ( aIndex->mFlag & SMI_INDEX_TYPE_MASK ) == SMI_INDEX_TYPE_PRIMARY_KEY )
    {
        sHeader->mIsNotNull = ID_TRUE;
    }
    else
    {
        sHeader->mIsNotNull = ID_FALSE;
    }

    sHeader->mIndexHeader = aIndex;

    sOffset = 0;
    for( sIndexCount = 0; sIndexCount < aIndex->mColumnCount; sIndexCount++ )
    {
        sIndexColumn = &sHeader->columns[sIndexCount];
        sIndexColumn4Build = &sHeader->columns4Build[sIndexCount];

        IDE_TEST_RAISE( (aIndex->mColumns[sIndexCount] & SMI_COLUMN_ID_MASK )
                        >= aTable->mColumnCount,
                        ERR_COLUMN_NOT_FOUND );
        sColumn = smcTable::getColumn(aTable,
                                      ( aIndex->mColumns[sIndexCount] & SMI_COLUMN_ID_MASK));

        IDE_TEST_RAISE( sColumn->id != aIndex->mColumns[sIndexCount],
                        ERR_COLUMN_NOT_FOUND );

        idlOS::memcpy( &sIndexColumn->column, sColumn, ID_SIZEOF(smiColumn) );

        idlOS::memcpy( &sIndexColumn4Build->column, sColumn, ID_SIZEOF(smiColumn) );

        idlOS::memcpy( &sIndexColumn4Build->keyColumn,
                       &sIndexColumn4Build->column,
                       ID_SIZEOF(smiColumn) );

        if ( ( ( sIndexColumn4Build->keyColumn.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE ) ||
             ( ( sIndexColumn4Build->keyColumn.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
        {
            sIndexColumn4Build->keyColumn.flag &= ~SMI_COLUMN_TYPE_MASK;
            sIndexColumn4Build->keyColumn.flag |= SMI_COLUMN_TYPE_FIXED;
        }

        IDE_TEST(gSmiGlobalCallBackList.getAlignValue(sColumn,
                                                      &sAlignVal)
                 != IDE_SUCCESS );

        sOffset = idlOS::align( sOffset, sAlignVal );

        sIndexColumn4Build->keyColumn.offset = sOffset;
        sOffset += sIndexColumn4Build->column.size;

        /* PROJ-2433
         * direct key index 인경우
         * 첫번째 컬럼을 direct key compare로 세팅한다. */
        if ( ( sIndexCount == 0 ) &&
             ( ( aIndex->mFlag & SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE ) )
        {
            sCompareFlags = ( aIndex->mColumnFlags[sIndexCount] | SMI_COLUMN_COMPARE_DIRECT_KEY ) ;
        }
        else
        {
            sCompareFlags = aIndex->mColumnFlags[sIndexCount];
        }

        IDE_TEST( gSmiGlobalCallBackList.findCompare( sColumn,
                                                      sCompareFlags,
                                                      &sIndexColumn->compare )
                  != IDE_SUCCESS );

        /* PROJ-2433 
         * bottom-build시 사용되는 compare 함수는
         * direct key를 사용하지 않는 compare 함수로 세팅한다. */
        IDE_TEST( gSmiGlobalCallBackList.findCompare( sColumn,
                                                      aIndex->mColumnFlags[sIndexCount],
                                                      &sIndexColumn4Build->compare )
                  != IDE_SUCCESS );

        IDE_TEST( gSmiGlobalCallBackList.findKey2String( sColumn,
                                                         aIndex->mColumnFlags[sIndexCount],
                                                         &sIndexColumn->key2String)
                 != IDE_SUCCESS );
        sIndexColumn4Build->key2String = sIndexColumn->key2String;

        IDE_TEST( gSmiGlobalCallBackList.findIsNull( sColumn,
                                                     aIndex->mColumnFlags[sIndexCount],
                                                     &sIndexColumn->isNull )
                  != IDE_SUCCESS );
        sIndexColumn4Build->isNull = sIndexColumn->isNull;

        IDE_TEST( gSmiGlobalCallBackList.findNull( sColumn,
                                                   aIndex->mColumnFlags[sIndexCount],
                                                   &sIndexColumn->null )
                  != IDE_SUCCESS );
        sIndexColumn4Build->null = sIndexColumn->null;

        /* BUG-24449
         * 키의 헤더 크기는 타입에 따라 다르다. 따라서 MT 함수로 길이를 획득하고,
         * 길이를 편집하고, 길이를 저장해야 한다.
         * ActualSize함수를 통해 길이를 알고, makeMtdValue 함수를 통해 MT Type으
         * 로 복원한다.
         * 그리고 헤더 길이도 MT 함수로부터 얻는다
         */
        IDE_TEST( gSmiGlobalCallBackList.findActualSize(
                      sColumn,
                      &sIndexColumn->actualSize )
                  != IDE_SUCCESS );
        sIndexColumn4Build->actualSize = sIndexColumn->actualSize;

        /* PROJ-2429 Dictionary based data compress for on-disk DB
         * memory index 의 min/max값은 smiGetCompressionColumn를 이용해
         * 구한다음 연산한다. 따라서 dictionary compression과 관계없이
         * 해당 data type의 함수를 사용해야 한다. */
        IDE_TEST( gSmiGlobalCallBackList.findCopyDiskColumnValue4DataType(
                      sColumn,
                      &sIndexColumn->makeMtdValue)
                  != IDE_SUCCESS );
        sIndexColumn4Build->makeMtdValue = sIndexColumn->makeMtdValue;
        
        IDE_TEST( gSmiGlobalCallBackList.getNonStoringSize( sColumn,
                                                            &sNonStoringSize )
                  != IDE_SUCCESS );
        sIndexColumn->headerSize       = sNonStoringSize;
        sIndexColumn4Build->headerSize = sIndexColumn->headerSize ;

        if ( sAlignVal > sMaxAlignVal )
        {
            sMaxAlignVal = sAlignVal;
        }

        if ( sIndexCount == 0 )
        {
            /* PROJ-2433
             * 여기서 direct key index 관련된 값들을 세팅한다. */
            IDE_TEST_RAISE ( setDirectKeyIndex( sHeader,
                                                aIndex,
                                                sColumn,
                                                sAlignVal ) != IDE_SUCCESS,
                             ERR_TOO_SMALL_DIRECT_KEY_SIZE )
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( sMaxAlignVal < 8 )
    {
        sHeader->mSlotAlignValue = 4;
    }
    else
    {
        sHeader->mSlotAlignValue = 8;
    }

    sHeader->mFixedKeySize = sOffset;

    /* index statistics */
    IDE_TEST( smnManager::initIndexStatistics( aIndex,
                                               (smnRuntimeHeader*)sHeader,
                                               aIndex->mId )
              != IDE_SUCCESS );

    aIndex->mHeader = (smnRuntimeHeader*)sHeader;

    if ( aRebuildIndexHeader != NULL )
    {
        if ( *aRebuildIndexHeader != NULL )
        {
            sHeader->tempPtr = *aRebuildIndexHeader;
            sSrcIndexHeader = (smnbHeader*)((*aRebuildIndexHeader)->mHeader);
            sSrcIndexHeader->cRef++;
        }
        else
        {
            *aRebuildIndexHeader = aIndex;
        }
    }
    else
    {
        sHeader->tempPtr = NULL;
    }
    
    /* PROJ-2334 PMT */
    if ( ( ( aIndex->mFlag & SMI_INDEX_UNIQUE_MASK ) == SMI_INDEX_UNIQUE_ENABLE ) ||
         ( ( aIndex->mFlag & SMI_INDEX_LOCAL_UNIQUE_MASK ) == SMI_INDEX_LOCAL_UNIQUE_ENABLE ) )
    {
        *aInsert = smnbBTree::insertRowUnique;
    }
    else
    {
        *aInsert = smnbBTree::insertRow;
    }

    if ( smuProperty::getDBMSStatMethod() == SMU_DBMS_STAT_METHOD_AUTO )
    {
        aIndex->mStat.mKeyCount = 
            ((smcTableHeader *)aTable)->mFixed.mMRDB.mRuntimeEntry->mInsRecCnt;
    }

    idlOS::memset( &(sHeader->mStmtStat), 0x00, ID_SIZEOF(smnbStatistic) );
    idlOS::memset( &(sHeader->mAgerStat), 0x00, ID_SIZEOF(smnbStatistic) );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND );
    {
        IDE_SET( ideSetErrorCode( smERR_FATAL_smnColumnNotFound ) );
    }
    IDE_EXCEPTION( ERR_INSUFFICIENT_MEMORY );
    {
        IDE_SET( ideSetErrorCode( idERR_ABORT_InsufficientMemory ) );
    }
    IDE_EXCEPTION( ERR_TOO_SMALL_DIRECT_KEY_SIZE );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_TooSmallDirectKeySize ) );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sStage )
    {
            //fix BUG-22898
        case 3:
            (void) iduMemMgr::free( sHeader->columns );
            (void) iduMemMgr::free( sHeader->columns4Build );
        case 2:
            (void) sHeader->mNodePool.release();
            (void) sHeader->mNodePool.destroy();
        case 1:
            //fix BUG-22898
            (void) iduMemMgr::free( sHeader );
            break;
        default:
            break;
    }
    IDE_POP();

    aIndex->mHeader = NULL;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::buildIndex( idvSQL*               aStatistics,
                              void*                 aTrans,
                              smcTableHeader*       aTable,
                              smnIndexHeader*       aIndex,
                              smnGetPageFunc        aGetPageFunc,
                              smnGetRowFunc         aGetRowFunc,
                              SChar*                aNullRow,
                              idBool                aIsNeedValidation,
                              idBool                aIsEnableTable,
                              UInt                  aParallelDegree,
                              UInt                /*aBuildFlag*/,
                              UInt                /*aTotalRecCnt*/ )
{
    smnIndexModule*     sIndexModules;
    UInt                sThreadCnt;
    UInt                sKeyValueSize;
    UInt                sKeySize;
    UInt                sRunSize;
    ULong               sMemoryIndexBuildValueThreshold;
    idBool              sBuildWithKeyValue = ID_TRUE;
    UInt                sPageCnt;

    IDE_DASSERT( aGetPageFunc != NULL );
    IDE_DASSERT( aGetRowFunc  != NULL ); 
    IDE_DASSERT( aNullRow     != NULL );

    sIndexModules = aIndex->mModule;

    sPageCnt = aTable->mFixed.mMRDB.mRuntimeEntry->mAllocPageList->mPageCount;

    if ( aParallelDegree == 0 )
    {
        sThreadCnt = smuProperty::getIndexBuildThreadCount();
    }
    else
    {
        sThreadCnt = aParallelDegree;
    }

    if ( sPageCnt < sThreadCnt )
    {
        sThreadCnt = sPageCnt;
    }

    if ( sThreadCnt == 0 )
    {
        sThreadCnt = 1;
    }

    sKeyValueSize = getMaxKeyValueSize( aIndex );

    // BUG-19249 : sKeySize = key value 크기 + row ptr 크기
    sKeySize      = getKeySize( sKeyValueSize );

    sMemoryIndexBuildValueThreshold =
        smuProperty::getMemoryIndexBuildValueLengthThreshold();

    sRunSize        = smuProperty::getMemoryIndexBuildRunSize();

    // PROJ-1629 : Memory Index Build
    // 기존의 row pointer를 이용하거나 key value를 이용한 index build를
    // 선택적으로 사용가능.
    // row pointer를 이용하는 경우 :
    // C-1. key value lenght가 property에 정의된 값보다 클 때
    // C-2. 한개의 key size가 run의 크기보다 클 때
    // C-3. persistent index일 때
    // BUG-19249 : C-2 추가
    if ( ( sKeyValueSize > sMemoryIndexBuildValueThreshold ) ||   // C-1
         ( sKeySize > (sRunSize - ID_SIZEOF(UShort)) )      ||   // C-2
         ( (aIndex->mFlag & SMI_INDEX_PERSISTENT_MASK) ==        // C-3
           SMI_INDEX_PERSISTENT_ENABLE ) )
    {
        sBuildWithKeyValue = ID_FALSE;
    }

    if ( sBuildWithKeyValue == ID_TRUE )
    {
        IDE_TEST( smnbValuebaseBuild::buildIndex( aTrans,
                                                  aStatistics,
                                                  aTable,
                                                  aIndex,
                                                  sThreadCnt,
                                                  aIsNeedValidation,
                                                  sKeySize,
                                                  sRunSize,
                                                  aGetPageFunc,
                                                  aGetRowFunc )
                  != IDE_SUCCESS);

        // BUG-19249
        ((smnbHeader*)aIndex->mHeader)->mBuiltType = 'V';
    }
    else
    {
        IDE_TEST( smnbPointerbaseBuild::buildIndex( aTrans,
                                                    aStatistics,
                                                    aTable,
                                                    aIndex,
                                                    sThreadCnt,
                                                    aIsEnableTable,
                                                    aIsNeedValidation,
                                                    aGetPageFunc,
                                                    aGetRowFunc,
                                                    aNullRow )
                  != IDE_SUCCESS);

        // BUG-19249
        ((smnbHeader*)aIndex->mHeader)->mBuiltType = 'P';
    }

    IDE_TEST_RAISE( sIndexModules == NULL, ERR_NOT_FOUND );

    (void)smnManager::setIndexCreated( aIndex, ID_TRUE );

    return IDE_SUCCESS;

    /* 해당 type의 인덱스를 찾을 수 없습니다. */
    IDE_EXCEPTION( ERR_NOT_FOUND );
    IDE_SET( ideSetErrorCode( smERR_FATAL_smnNotSupportedIndex ) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::drop( smnIndexHeader * aIndex )
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    volatile smnbLNode  *s_pCurLNode;
    volatile smnbINode  *s_pFstChildINode;
    volatile smnbINode  *s_pCurINode;
    smnbHeader          *s_pHeader;
    smmSlot              s_slots = { &s_slots, &s_slots };
    smmSlot             *s_pFreeNode;
    SInt                 s_slotCount = 0;
    SInt                 s_cSlot;
    ULong                sDestroyFailCnt = 0;

    s_pHeader = (smnbHeader*)(aIndex->mHeader);

    if ( s_pHeader != NULL )
    {
        s_pFstChildINode = (smnbINode*)(s_pHeader->root);

        while(s_pFstChildINode != NULL)
        {
            s_pCurINode = s_pFstChildINode;
            s_pFstChildINode = (smnbINode*)(s_pFstChildINode->mChildPtrs[0]);

            if ( SMNB_IS_LEAF_NODE( s_pCurINode ) )
            {
                s_pCurLNode = (smnbLNode*)s_pCurINode;

                while(s_pCurLNode != NULL)
                {
                    s_slotCount++;
                    
                    if ( destroyNode( (smnbNode*)s_pCurLNode ) != IDE_SUCCESS )
                    {
                        sDestroyFailCnt++;
                    }

                    s_pFreeNode = (smmSlot*)s_pCurLNode;

                    s_pFreeNode->prev = s_slots.prev;
                    s_pFreeNode->next = &s_slots;

                    s_slots.prev->next = s_pFreeNode;
                    s_slots.prev = s_pFreeNode;

                    s_pCurLNode = s_pCurLNode->nextSPtr;
                }

                s_pFreeNode = s_slots.next;
                s_cSlot = s_slotCount;

                while(s_pFreeNode != &s_slots)
                {
                    s_pFreeNode = s_pFreeNode->next;
                    s_cSlot--;
                }

                s_slots.next->prev = s_slots.prev;
                s_slots.prev->next = s_slots.next;

                IDE_TEST( s_pHeader->mNodePool.releaseSlots(
                                                s_slotCount,
                                                s_slots.next,
                                                SMM_SLOT_LIST_MUTEX_NEEDLESS )
                          != IDE_SUCCESS );

                break;
            }
            else
            {
                while(s_pCurINode != NULL)
                {
                    s_slotCount++;

                    if ( destroyNode( (smnbNode*)s_pCurINode ) != IDE_SUCCESS )
                    {
                        sDestroyFailCnt++;
                    }

                    s_pFreeNode = (smmSlot*)s_pCurINode;

                    s_pFreeNode->prev = s_slots.prev;
                    s_pFreeNode->next = &s_slots;

                    s_slots.prev->next = s_pFreeNode;
                    s_slots.prev = s_pFreeNode;

                    s_pCurINode = s_pCurINode->nextSPtr;
                }
            }
        }

        IDE_TEST( s_pHeader->mNodePool.release() != IDE_SUCCESS );
        IDE_TEST( s_pHeader->mNodePool.destroy() != IDE_SUCCESS );

        IDE_TEST( s_pHeader->mTreeMutex.destroy() != IDE_SUCCESS );

        IDE_TEST( smnManager::destIndexStatistics( 
                        aIndex,
                        (smnRuntimeHeader*)s_pHeader )
                  != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free( s_pHeader->columns) != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free( s_pHeader->columns4Build ) != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free( s_pHeader ) != IDE_SUCCESS );

        aIndex->mHeader = NULL;
    }

    if ( sDestroyFailCnt != 0 )
    {
        ideLog::log( IDE_ERR_0,
                     "fail to destroy node - smnbBTree::drop, Count[%"ID_UINT64_FMT"]",
                     sDestroyFailCnt );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sDestroyFailCnt != 0 )
    {
        ideLog::log( IDE_ERR_0,
                     "fail to destroy node - smnbBTree::drop, Count[%"ID_UINT64_FMT"]",
                     sDestroyFailCnt );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::init( smnbIterator        * aIterator,
                        void                * aTrans,
                        smcTableHeader      * aTable,
                        smnIndexHeader      * aIndex,
                        void                * /* aDumpObject */,
                        const smiRange      * aKeyRange,
                        const smiRange      * /* aKeyFilter */,
                        const smiCallBack   * aRowFilter,
                        UInt                  aFlag,
                        smSCN                 aSCN,
                        smSCN                 aInfinite,
                        idBool                aUntouchable,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc,
                        smiStatement        * aStatement )
{
    idvSQL                    *sSQLStat;

    aIterator->SCN             = aSCN;
    aIterator->infinite        = aInfinite;
    aIterator->trans           = aTrans;
    aIterator->table           = aTable;
    aIterator->curRecPtr       = NULL;
    aIterator->lstFetchRecPtr  = NULL;
    aIterator->lstReturnRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->tid             = smLayerCallback::getTransID( aTrans );
    aIterator->flag            = ( aUntouchable == ID_TRUE ) ? SMI_ITERATOR_READ : SMI_ITERATOR_WRITE;
    aIterator->mProperties     = aProperties;
    aIterator->index           = (smnbHeader*)aIndex->mHeader;
    aIterator->mKeyRange       = aKeyRange;
    aIterator->mRowFilter      = aRowFilter;
    aIterator->mStatement      = aStatement;
    

    IDL_MEM_BARRIER;

    *aSeekFunc = smnbSeekFunctions[aFlag&(SMI_TRAVERSE_MASK|
                                          SMI_PREVIOUS_MASK|
                                          SMI_LOCK_MASK)];

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mMemCursorIndexScan, 1);

    if ( sSQLStat != NULL )
    {
        IDV_SESS_ADD(sSQLStat->mSess, IDV_STAT_INDEX_MEM_CURSOR_IDX_SCAN, 1);
    }

    return IDE_SUCCESS;
}

IDE_RC smnbBTree::dest( smnbIterator * /*aIterator*/ )
{
    return IDE_SUCCESS;
}

IDE_RC smnbBTree::isRowUnique( void             * aTrans,
                               smnbStatistic    * aIndexStat,
                               smSCN              aStmtViewSCN,
                               void             * aRow,
                               smTID            * aTid,
                               SChar           ** aExistUniqueRow,
                               idBool             aForbiddenToRetry )
{
    smSCN sCreateSCN;
    smSCN sRowSCN;
    smTID sRowTID;
    smTID sMyTID;

    smSCN sLimitSCN;
    smSCN sNxtSCN;
    smTID sNxtTID;

    /* aTrans가 NULL이면 uniqueness를 검사하지 않는다. */
    IDE_TEST_CONT( aTrans == NULL, SKIP_UNIQUE_CHECK );

    sMyTID = smLayerCallback::getTransID( aTrans );


    SM_SET_SCN( &sCreateSCN, &(((smpSlotHeader*)aRow)->mCreateSCN) );
    SM_SET_SCN( &sLimitSCN,  &(((smpSlotHeader*)aRow)->mLimitSCN)  );

    /* PROJ-2733 */
    if ( ((smxTrans *)aTrans)->mIsGCTx == ID_TRUE )
    {
        if ( SM_SCN_IS_INFINITE( sCreateSCN ) &&
             /* BUG-48244 : 내(TX)가 2PC한 TX는 Pending 완료시까지 대기할 필요없다. */
             ( SMP_GET_TID( sCreateSCN ) != smxTrans::getTransID( aTrans ) ) )
        {
            IDE_TEST( smxTrans::waitPendingTx( (smxTrans *)aTrans,
                                               sCreateSCN,
                                               aStmtViewSCN )
                      != IDE_SUCCESS );
        }

        if ( SM_SCN_IS_INFINITE( sLimitSCN ) &&
             /* BUG-48244 : 내(TX)가 2PC한 TX는 Pending 완료시까지 대기할 필요없다. */
             ( SMP_GET_TID( sLimitSCN ) != smxTrans::getTransID( aTrans ) ) )
        {
            IDE_TEST( smxTrans::waitPendingTx( (smxTrans *)aTrans,
                                               sLimitSCN,
                                               aStmtViewSCN )
                      != IDE_SUCCESS );
        }

        /* BUG-48244 */
        SM_SET_SCN( &sCreateSCN, &(((smpSlotHeader*)aRow)->mCreateSCN) );
        SM_SET_SCN( &sLimitSCN,  &(((smpSlotHeader*)aRow)->mLimitSCN)  );
    }

    IDU_FIT_POINT( "PROJ-2733@smnbBTree::isRowUnique" );

    SMX_GET_SCN_AND_TID( sCreateSCN, sRowSCN, sRowTID );
    SMX_GET_SCN_AND_TID( sLimitSCN,  sNxtSCN, sNxtTID );

    aIndexStat->mKeyValidationCount++;

    /* BUG-32655
     * SCN에 DeleteBit가 설정된 경우. 이 경우는 Rollback등으로
     * 아예 유효하지 않은 Row가 된 경우이기에
     * 무시해야 된다. */
    IDE_TEST_CONT( SM_SCN_IS_DELETED( sRowSCN ) , SKIP_UNIQUE_CHECK );

    if ( SM_SCN_IS_INFINITE( sRowSCN ) )
    {
        if ( sRowTID != sMyTID )
        {
            *aTid = sRowTID;
            IDE_ERROR_RAISE( *aTid != 0, ERR_CORRUPTED_INDEX );
            IDE_ERROR_RAISE( *aTid != SM_NULL_TID, ERR_CORRUPTED_INDEX );
        }
        else
        {
            IDE_TEST_RAISE( SM_SCN_IS_FREE_ROW( sNxtSCN ),
                            ERR_UNIQUE_VIOLATION );
        }
    }
    else
    {
        /* BUG-14953: PK가 두개 둘어감: 여기서 Next OID는 다른
           Transaction읽고 있는 중에 Update가 될수 있기 때문에 값을
           복사해서 Test해야한다.*/

        IDE_TEST_RAISE( SM_SCN_IS_FREE_ROW( sNxtSCN ),
                        ERR_UNIQUE_VIOLATION );

        if ( SM_SCN_IS_LOCK_ROW( sNxtSCN ) )
        {
            //자기 자신이 이미 lock을 잡고 있는 row라면 unique에러
            IDE_TEST_RAISE( sNxtTID == sMyTID, ERR_UNIQUE_VIOLATION );

            //남이 row를 잡고 있다면 그 트랜잭션이 끝나길 기다린다.
            *aTid = sNxtTID;
            IDE_ERROR_RAISE( *aTid != 0, ERR_CORRUPTED_INDEX );
            IDE_ERROR_RAISE( *aTid != SM_NULL_TID, ERR_CORRUPTED_INDEX );
        }
        else
        {
            if ( SM_SCN_IS_INFINITE( sNxtSCN ) )
            {
                if ( sNxtTID != sMyTID )
                {
                    *aTid = sNxtTID;
                    IDE_ERROR_RAISE( *aTid != 0, ERR_CORRUPTED_INDEX );
                    IDE_ERROR_RAISE( *aTid != SM_NULL_TID, ERR_CORRUPTED_INDEX );
                }
            }

            if ( ( SM_SCN_IS_NOT_INFINITE( sNxtSCN ) ) &&
                 ( SM_SCN_IS_NOT_INIT( aStmtViewSCN ) && SM_SCN_IS_LT( &aStmtViewSCN, &sNxtSCN ) ) )
            {
                // BUG-15097
                // unique check시 먼저 들어간 row의 commit scn이
                // 현재 transaction의 begin scn보다 크면
                // 현재 transaction은 그 row를 볼 수 없다.
                // 이땐 retry error를 발생시켜야 한다.
                IDE_RAISE( ERR_ALREADY_MODIFIED );
            }
        }
    }

    IDE_EXCEPTION_CONT( SKIP_UNIQUE_CHECK );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_UNIQUE_VIOLATION );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smnUniqueViolation ) );

        // PROJ-2264
        if ( aExistUniqueRow != NULL )
        {
            *aExistUniqueRow = (SChar*)aRow;
        }
    }
    IDE_EXCEPTION( ERR_ALREADY_MODIFIED );
    {
        if( aForbiddenToRetry == ID_TRUE )
        {
            IDE_DASSERT( ((smxTrans*)aTrans)->mIsGCTx == ID_TRUE );

            SChar sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[INDEX VALIDATION] "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "RowSCN:%"ID_UINT64_FMT", "
                             "NxtSCN:%"ID_UINT64_FMT,
                             SM_SCN_TO_LONG(aStmtViewSCN),
                             SM_SCN_TO_LONG(sRowSCN),
                             SM_SCN_TO_LONG(sNxtSCN) );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode(smERR_RETRY_Already_Modified) );
        }
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong RID / SCN" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
   TASK-6743 
   기존 compareRows() 함수의 호출횟수가 많아서,
   용도에 맞게 사용하도록 3개의 함수로 나눔.

    compareRows()    : 값만 비교.
    compareRowsAndPtr()  : 값이 동일하면 포인터로 비교.
    compareRowsAndPtr2() : compareRowsAndPtr()과 동일하나, 값이 동일한지를 매개변수로 알려줌.
 */
SInt smnbBTree::compareRows( smnbStatistic    * aIndexStat,
                             const smnbColumn * aColumns,
                             const smnbColumn * aFence,
                             const void       * aRow1,
                             const void       * aRow2 )
{
    SInt          sResult;
    smiColumn   * sColumn;
    smiValueInfo  sValueInfo1;
    smiValueInfo  sValueInfo2;

    if ( aRow1 == NULL )
    {
        if ( aRow2 == NULL )
        {
            return 0;
        }

        return 1;
    }
    else
    {
        if ( aRow2 == NULL )
        {
            return -1;
        }
    }

    SMI_SET_VALUEINFO(
        &sValueInfo1, NULL, aRow1, 0, SMI_OFFSET_USE,
        &sValueInfo2, NULL, aRow2, 0, SMI_OFFSET_USE );

    for ( ; aColumns < aFence; aColumns++ )
    {
        aIndexStat->mKeyCompareCount++;
        sColumn = (smiColumn*)&(aColumns->column);

        if ( ( aColumns->column.flag & SMI_COLUMN_TYPE_MASK ) ==
               SMI_COLUMN_TYPE_FIXED )
        {
            sValueInfo1.column = sColumn;
            sValueInfo2.column = sColumn;

            sResult = aColumns->compare( &sValueInfo1,
                                         &sValueInfo2 );
        }
        else
        {
            sResult = compareVarColumn( aColumns,
                                        aRow1,
                                        aRow2 );
        }

        if ( sResult != 0 )
        {
            return sResult;
        }
    }

    return 0;
}

SInt smnbBTree::compareRowsAndPtr( smnbStatistic    * aIndexStat,
                                   const smnbColumn * aColumns,
                                   const smnbColumn * aFence,
                                   const void       * aRow1,
                                   const void       * aRow2 )
{
    SInt          sResult;
    smiColumn   * sColumn;
    smiValueInfo  sValueInfo1;
    smiValueInfo  sValueInfo2;

    if ( aRow1 == NULL )
    {
        if ( aRow2 == NULL )
        {
            return 0;
        }

        return 1;
    }
    else
    {
        if ( aRow2 == NULL )
        {
            return -1;
        }
    }

    SMI_SET_VALUEINFO(
        &sValueInfo1, NULL, aRow1, 0, SMI_OFFSET_USE,
        &sValueInfo2, NULL, aRow2, 0, SMI_OFFSET_USE );

    for ( ; aColumns < aFence; aColumns++ )
    {
        aIndexStat->mKeyCompareCount++;
        sColumn = (smiColumn*)&(aColumns->column);

        if ( ( aColumns->column.flag & SMI_COLUMN_TYPE_MASK ) ==
             SMI_COLUMN_TYPE_FIXED )
        {
            sValueInfo1.column = sColumn;
            sValueInfo2.column = sColumn;

            sResult = aColumns->compare( &sValueInfo1,
                                         &sValueInfo2 );
        }
        else
        {
            sResult = compareVarColumn( (smnbColumn*)aColumns,
                                        aRow1,
                                        aRow2 );
        }

        if ( sResult != 0 )
        {
            return sResult;
        }
    }

    /* BUG-39043 키 값이 동일 할 경우 pointer 위치로 순서를 정하도록 할 경우
     * 서버를 새로 올린 경우 위치가 변경되기 때문에 persistent index를 사용하는 경우
     * 저장된 index와 실제 순서가 맞지 않아 문제가 생길 수 있다.
     * 이를 해결하기 위해 키 값이 동일 한 경우 OID로 순서를 정하도록 한다. */
    if( SMP_SLOT_GET_OID( aRow1 ) > SMP_SLOT_GET_OID( aRow2 ) )
    {
        return 1;
    }
    if( SMP_SLOT_GET_OID( aRow1 ) < SMP_SLOT_GET_OID( aRow2 ) )
    {
        return -1;
    }

    return 0;
}

SInt smnbBTree::compareRowsAndPtr2( smnbStatistic    * aIndexStat,
                                    const smnbColumn * aColumns,
                                    const smnbColumn * aFence,
                                    SChar            * aRow1,
                                    SChar            * aRow2,
                                    idBool           * aIsEqual )
{
    SInt          sResult;
    smiColumn   * sColumn;
    smiValueInfo  sValueInfo1;
    smiValueInfo  sValueInfo2;

    *aIsEqual = ID_FALSE;

    if(aRow1 == NULL)
    {
        if(aRow2 == NULL)
        {
            *aIsEqual = ID_TRUE;

            return 0;
        }

        return 1;
    }
    else
    {
        if(aRow2 == NULL)
        {
            return -1;
        }
    }

    SMI_SET_VALUEINFO(
        &sValueInfo1, NULL, aRow1, 0, SMI_OFFSET_USE,
        &sValueInfo2, NULL, aRow2, 0, SMI_OFFSET_USE );

    for( ; aColumns < aFence; aColumns++ )
    {
        aIndexStat->mKeyCompareCount++;
        sColumn = (smiColumn*)&(aColumns->column);

        if( (aColumns->column.flag & SMI_COLUMN_TYPE_MASK) ==
            SMI_COLUMN_TYPE_FIXED )
        {
            sValueInfo1.column = sColumn;
            sValueInfo2.column = sColumn;

            sResult = aColumns->compare( &sValueInfo1,
                                         &sValueInfo2 );
        }
        else
        {
            sResult = compareVarColumn( (smnbColumn*)aColumns,
                                        aRow1,
                                        aRow2 );
        }

        if( sResult != 0 )
        {
            return sResult;
        }
    }

    *aIsEqual = ID_TRUE;

    /* BUG-39043 키 값이 동일 할 경우 pointer 위치로 순서를 정하도록 할 경우
     * 서버를 새로 올린 경우 위치가 변경되기 때문에 persistent index를 사용하는 경우
     * 저장된 index와 실제 순서가 맞지 않아 문제가 생길 수 있다.
     * 이를 해결하기 위해 키 값이 동일 한 경우 OID로 순서를 정하도록 한다. */
    if( SMP_SLOT_GET_OID( aRow1 ) > SMP_SLOT_GET_OID( aRow2 ) )
    {
        return 1;
    }
    if( SMP_SLOT_GET_OID( aRow1 ) < SMP_SLOT_GET_OID( aRow2 ) )
    {
        return -1;
    }

    return 0;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::compareKeys                     *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * row를 direct key와 비교하는 함수.
 *
 *  - row와 row를 비교하는 compareRowsAndPtr() 함수와 대응됨
 *  - direct key를 포함하는 slot이 비었는지 확인하기위해 aRow1이 필요함.
 *
 * aIndexStat      - [IN]  통계정보
 * aColumns        - [IN]  컬럼정보
 * aFence          - [IN]  컬럼정보 Fence (마지막 컬럼정보 위치)
 * aKey1           - [IN]  direct key pointer
 * aRow1           - [IN]  direct key와 동일 slot의 row pointer
 * aPartialKeySize - [IN]  direct key가 partial key일 경우의 길이
 * aRow2           - [IN]  비교할 row pointer
 *********************************************************************/
SInt smnbBTree::compareKeys( smnbStatistic      * aIndexStat,
                             const smnbColumn   * aColumns,
                             const smnbColumn   * aFence,
                             void               * aKey1,
                             SChar              * aRow1,
                             UInt                 aPartialKeySize,
                             SChar              * aRow2,
                             idBool             * aIsSuccess )
{
    SInt          sResult = 0;
    smiColumn     sDummyColumn;
    smiColumn   * sColumn = NULL;
    smiValueInfo  sValueInfo1;
    smiValueInfo  sValueInfo2;
    UInt          sFlag = 0;

    if ( aRow1 == NULL )
    {
        if ( aRow2 == NULL )
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if ( aRow2 == NULL )
        {
            return -1;
        }
        else
        {
            /* nothing to do */
        }
    }

    IDE_ERROR_RAISE( aColumns < aFence, ERR_CORRUPTED_INDEX );

    aIndexStat->mKeyCompareCount++;

    if ( ( aColumns->column.flag & SMI_COLUMN_TYPE_MASK ) ==
         SMI_COLUMN_TYPE_FIXED )
    {
        sDummyColumn.offset = 0;
        sDummyColumn.flag   = 0;

        sColumn = (smiColumn*)&(aColumns->column);

        sFlag = SMI_OFFSET_USELESS;

        if ( aPartialKeySize != 0 )
        {
            sFlag &= ~SMI_PARTIAL_KEY_MASK;
            sFlag |= SMI_PARTIAL_KEY_ON;
        }
        else
        {
            sFlag &= ~SMI_PARTIAL_KEY_MASK;
            sFlag |= SMI_PARTIAL_KEY_OFF;
        }

        SMI_SET_VALUEINFO( &sValueInfo1,
                           &sDummyColumn,
                           aKey1,
                           aPartialKeySize,
                           sFlag,
                           &sValueInfo2,
                           sColumn,
                           aRow2,
                           0,
                           SMI_OFFSET_USE );

        sResult = aColumns->compare( &sValueInfo1,
                                     &sValueInfo2 );
    }
    else
    {
        sResult = compareKeyVarColumn( (smnbColumn*)aColumns,
                                       aKey1,
                                       aPartialKeySize,
                                       aRow2 );
    }

    *aIsSuccess = ID_TRUE;

    return sResult;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Order" );
    }
    IDE_EXCEPTION_END;

    *aIsSuccess = ID_FALSE;

    return sResult;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::compareKeyVarColumn             *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * variable row를 direct key와 비교하는 함수.
 *
 *  - variable row와 row를 비교하는 compareVarColumn() 함수와 대응됨
 *
 * aColumns        - [IN]  컬럼정보
 * aKey            - [IN]  direct key pointer
 * aPartialKeySize - [IN]  direct key가 partial key일 경우의 길이
 * aRow            - [IN]  비교할 row pointer
 *********************************************************************/
SInt smnbBTree::compareKeyVarColumn( smnbColumn * aColumn,
                                     void       * aKey,
                                     UInt         aPartialKeySize,
                                     SChar      * aRow )
{
    SInt              sResult = 0;
    smiColumn         sDummyColumn;
    smiColumn         sColumn;
#ifdef DEBUG
    smVCDesc        * sColumnVCDesc = NULL;
#endif
    ULong             sInPageBuffer = 0;
    smiValueInfo      sValueInfo1;
    smiValueInfo      sValueInfo2;
    UInt              sFlag = 0;

    sColumn = aColumn->column;

#ifdef DEBUG
    // BUG-37533
    if ( ( sColumn.flag & SMI_COLUMN_COMPRESSION_MASK )
         == SMI_COLUMN_COMPRESSION_FALSE )
    {
        if ( (sColumn.flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
        {
            // BUG-30068 : malloc 실패시 비정상 종료합니다.
            sColumnVCDesc = (smVCDesc*)(aRow + sColumn.offset);
            /* VARIABLE_LARGE 는 구형 VARIABLE 타입이다. 간단히 DASSERT로만 확인한다. */
            IDE_DASSERT( sColumnVCDesc->length <= SMP_VC_PIECE_MAX_SIZE );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // compressed column이면 smVCDesc의 length를 검사하지 않는다.
    }
#endif

    sDummyColumn.offset = 0;
    sDummyColumn.flag   = 0;

    sColumn.value = (void*)&sInPageBuffer;
    *(ULong *)(sColumn.value) = 0;

    sFlag = SMI_OFFSET_USELESS;

    if ( aPartialKeySize != 0 )
    {
        sFlag &= ~SMI_PARTIAL_KEY_MASK;
        sFlag |= SMI_PARTIAL_KEY_ON;
    }
    else
    {
        sFlag &= ~SMI_PARTIAL_KEY_MASK;
        sFlag |= SMI_PARTIAL_KEY_OFF;
    }

    SMI_SET_VALUEINFO( &sValueInfo1,
                       (const smiColumn*)&sDummyColumn,
                       aKey,
                       aPartialKeySize,
                       sFlag,
                       &sValueInfo2,
                       (const smiColumn*)&sColumn,
                       aRow,
                       0,
                       SMI_OFFSET_USE );

    sResult = aColumn->compare( &sValueInfo1,
                                &sValueInfo2 );
    return sResult;
}

/********************************************************
 * To fix BUG-21235
 * OUT-VARCHAR를 위한 Compare Function
 ********************************************************/
SInt smnbBTree::compareVarColumn( const smnbColumn * aColumn,
                                  const void       * aRow1,
                                  const void       * aRow2 )
{
    SInt        sResult;
    smiColumn   sColumn1;
    smiColumn   sColumn2;
#ifdef DEBUG
    smVCDesc  * sColumnVCDesc1;
    smVCDesc  * sColumnVCDesc2;
#endif
    ULong       sInPageBuffer1;
    ULong       sInPageBuffer2;
    smiValueInfo  sValueInfo1;
    smiValueInfo  sValueInfo2;

    sColumn1 = aColumn->column;
    sColumn2 = aColumn->column;

#ifdef DEBUG
    // BUG-37533
    if ( ( sColumn1.flag & SMI_COLUMN_COMPRESSION_MASK )
         == SMI_COLUMN_COMPRESSION_FALSE )
    {
        if ( ( sColumn1.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
        {
            // BUG-30068 : malloc 실패시 비정상 종료합니다.
            sColumnVCDesc1 = (smVCDesc*)((SChar *)aRow1 + sColumn1.offset);
            /* VARIABLE_LARGE 는 구형 VARIABLE 타입이다. 간단히 DASSERT로만 확인한다. */
            IDE_DASSERT( sColumnVCDesc1->length <= SMP_VC_PIECE_MAX_SIZE );

            sColumnVCDesc2 = (smVCDesc*)((SChar *)aRow2 + sColumn2.offset);
            /* VARIABLE_LARGE 는 구형 VARIABLE 타입이다. 간단히 DASSERT로만 확인한다. */
            IDE_DASSERT( sColumnVCDesc2->length <= SMP_VC_PIECE_MAX_SIZE );
        }
        else
        {
            /* Nothing to do */
        }
    }
    else
    {
        // compressed column이면 smVCDesc의 length를 검사하지 않는다.
    }
#endif

    sColumn1.value = (void*)&sInPageBuffer1;
    *(ULong*)(sColumn1.value) = 0;
    sColumn2.value = (void*)&sInPageBuffer2;
    *(ULong*)(sColumn2.value) = 0;

    SMI_SET_VALUEINFO( &sValueInfo1, (const smiColumn*)&sColumn1, aRow1, 0, SMI_OFFSET_USE,
                       &sValueInfo2, (const smiColumn*)&sColumn2, aRow2, 0, SMI_OFFSET_USE );

    sResult = aColumn->compare( &sValueInfo1,
                                &sValueInfo2 );

    return sResult;
}


idBool smnbBTree::isNullColumn( smnbColumn * aColumn,
                                smiColumn  * aSmiColumn,
                                SChar      * aRow )
{
    idBool  sResult;
    UInt    sLength = 0;
    SChar * sKeyPtr = NULL;

    if ( ( aSmiColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
           != SMI_COLUMN_COMPRESSION_TRUE )
    {
        if ( (aSmiColumn->flag & SMI_COLUMN_TYPE_MASK) ==
              SMI_COLUMN_TYPE_FIXED )
        {
            sResult = aColumn->isNull( NULL,
                                       aRow + aSmiColumn->offset );
        }
        else
        {
            sResult = isVarNull( aColumn, aSmiColumn, aRow );
        }
    }
    else
    {
        // BUG-37464 
        // compressed column에 대해서 index 생성시 null 검사를 올바로 하지 못함
        sKeyPtr = (SChar*)smiGetCompressionColumn( aRow,
                                                   aSmiColumn,
                                                   ID_TRUE, // aUseColumnOffset
                                                   &sLength );

        if ( sKeyPtr == NULL )
        {
            sResult = ID_TRUE;
        }
        else
        {
            sResult = aColumn->isNull( NULL, sKeyPtr );
        }
    }

    return sResult;
}

/********************************************************
 * To fix BUG-21235
 * OUT-VARCHAR를 위한 Is Null Function
 ********************************************************/
idBool smnbBTree::isVarNull( smnbColumn * aColumn,
                             smiColumn  * aSmiColumn,
                             SChar      * aRow )
{
    idBool      sResult;
    smiColumn   sColumn;
#ifdef DEBUG
    smVCDesc  * sColumnVCDesc;
#endif
    ULong       sInPageBuffer;
    UChar     * sOrgValue;
    SChar     * sValue;
    UInt        sLength = 0;

    sColumn = *aSmiColumn;

    sOrgValue = (UChar*)sColumn.value;
#ifdef DEBUG
    if ( ( sColumn.flag & SMI_COLUMN_TYPE_MASK ) == SMI_COLUMN_TYPE_VARIABLE_LARGE )
    {
        /* BUG-30068 */
        sColumnVCDesc = (smVCDesc*)(aRow + sColumn.offset);
        /* VARIABLE_LARGE 는 구형 VARIABLE 타입이다. 간단히 DASSERT로만 확인한다. */
        IDE_DASSERT( sColumnVCDesc->length <= SMP_VC_PIECE_MAX_SIZE );
    }
#endif

    sColumn.value = (void*)&sInPageBuffer;
    *(ULong*)(sColumn.value) = 0;

    sValue = sgmManager::getVarColumn(aRow, &sColumn, &sLength);

    if ( sValue == NULL )
    {
        sResult = ID_TRUE;
    }
    else
    {
        sResult = aColumn->isNull( &sColumn, sValue );
    }

    sColumn.value = sOrgValue;

    return sResult;
}

/* BUG-30074 disk table의 unique index에서 NULL key 삭제 시/
 *           non-unique index에서 deleted key 추가 시
 *           Cardinality의 정확성이 많이 떨어집니다.
 * Key 전체가 Null인지 확인한다. 모두 Null이어야 한다. */

idBool smnbBTree::isNullKey( smnbHeader * aIndex,
                             SChar      * aRow )
{

    smnbColumn * sColumn;
    idBool       sIsNullKey   = ID_TRUE;

    for( sColumn = &aIndex->columns[0];
         sColumn < aIndex->fence;
         sColumn++)
    {
        if ( smnbBTree::isNullColumn( sColumn,
                                      &(sColumn->column),
                                      aRow ) == ID_FALSE )
        {
            sIsNullKey = ID_FALSE;
            break;
        }
    }

    return sIsNullKey;
}

UInt smnbBTree::getMaxKeyValueSize( smnIndexHeader *aIndexHeader )
{
    UInt             sTotalSize;

    IDE_DASSERT( aIndexHeader != NULL );

    sTotalSize = idlOS::align8(((smnbHeader*)aIndexHeader->mHeader)->mFixedKeySize);

    return sTotalSize;
}

IDE_RC smnbBTree::updateStat4Insert( smnIndexHeader   * aPersIndex,
                                     smnbHeader       * aIndex,
                                     smnbLNode        * aLeafNode,
                                     SInt               aSlotPos,
                                     SChar            * aRow,
                                     idBool             aIsUniqueIndex )
{
    SChar            * sPrevRowPtr;
    SChar            * sNextRowPtr;
    smnbColumn       * sColumn;
    SLong            * sNumDistPtr;
    SInt               sCompareResult;

    // get next slot
    if ( aSlotPos == ( aLeafNode->mSlotCount - 1 ) )
    {
        // BUG-47554 memory index 에서 latch 관리를 IDL_MEM_BARRIER 가 아닌 atomic 함수로 변경합니다.
        // 갱신 할 Key의 위치가 Leaf Node의 마지막 Slot일 때,
        // Next Node의 첫번째 Key의 Row Pointer를 가져온다.
        // 자주 호출 되지 않고, 최적화로 인한 문제가 있어서
        // 별도의 함수로 추출한다.
        sNextRowPtr = getNxtLNodeFstRow( aLeafNode );
    }
    else
    {
        sNextRowPtr = aLeafNode->mRowPtrs[aSlotPos + 1];
    }
    // get prev slot
    if ( aSlotPos == 0 )
    {
        // BUG-47554 memory index 에서 latch 관리를 IDL_MEM_BARRIER 가 아닌 atomic 함수로 변경합니다.
        // 갱신 할 Key의 위치가 Leaf Node의 첫번째 Slot일 때,
        // Prev Node의 마지막 Key의 Row Pointer를 가져온다.
        // 자주 호출 되지 않고, 최적화로 인한 문제가 있어서
        // 별도의 함수로 추출한다.
        sPrevRowPtr = getPrvLNodeLstRow( aLeafNode );
    }
    else
    {
        IDE_DASSERT( ( aSlotPos - 1 ) >= 0 );
        sPrevRowPtr = aLeafNode->mRowPtrs[aSlotPos - 1];
    }

    sColumn = &aIndex->columns[0];
    // update index statistics

    if ( isNullColumn( sColumn,
                       &(sColumn->column),
                       aRow ) != ID_TRUE )
    {
        /* MinValue */
        if ( sPrevRowPtr == NULL )
        {
            IDE_TEST( setMinMaxStat( aPersIndex,
                                     aRow,
                                     sColumn,
                                     ID_TRUE ) != IDE_SUCCESS ); // aIsMinStat
        }

        /* MaxValue */
        if ( ( sNextRowPtr == NULL) ||
             ( isNullColumn( sColumn,
                             &(sColumn->column),
                             sNextRowPtr ) == ID_TRUE) )
        {
            IDE_TEST( setMinMaxStat( aPersIndex,
                                     aRow,
                                     sColumn,
                                     ID_FALSE ) != IDE_SUCCESS ); // aIsMinStat
        }
    }

    /* NumDist */
    sNumDistPtr = &aPersIndex->mStat.mNumDist;

    while(1)
    {
        if ( ( aIsUniqueIndex == ID_TRUE ) && ( aIndex->mIsNotNull == ID_TRUE ) )
        {
            idCore::acpAtomicInc64( (void*) sNumDistPtr );
            break;
        }

        if ( sPrevRowPtr != NULL )
        {
            sCompareResult = compareRows( &(aIndex->mStmtStat),
                                          aIndex->columns,
                                          aIndex->fence,
                                          aRow,
                                          sPrevRowPtr ); 

            if ( sCompareResult == 0 )
            {
                break;
            }

            IDE_ERROR_RAISE( sCompareResult > 0, ERR_CORRUPTED_INDEX );
        }

        if ( sNextRowPtr != NULL )
        {
            sCompareResult = compareRows( &(aIndex->mStmtStat),
                                          aIndex->columns,
                                          aIndex->fence,
                                          aRow,
                                          sNextRowPtr );

            if ( sCompareResult == 0 )
            {
                break;
            }

            IDE_ERROR_RAISE( sCompareResult < 0, ERR_CORRUPTED_INDEX );
        }

        /* BUG-30074 - disk table의 unique index에서 NULL key 삭제 시/
         *             non-unique index에서 deleted key 추가 시 NumDist의
         *             정확성이 많이 떨어집니다.
         *
         * Null Key의 경우 NumDist를 갱신하지 않도록 한다.
         * Memory index의 NumDist도 동일한 정책으로 변경한다. */
        if ( smnbBTree::isNullKey( aIndex,
                                   aRow ) != ID_TRUE )
        {
            idCore::acpAtomicInc64( (void*) sNumDistPtr );
        }

        break;
    }

    /* BUG-32943 [sm-disk-index] After executing DELETE ROW, the KeyCount
     * of DRDB Index is not decreased  */
    idCore::acpAtomicInc64( (void*)&aPersIndex->mStat.mKeyCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        smnbBTree::logIndexHeader( aIndex->mIndexHeader );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Order" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * 갱신 할 Key의 위치가 Leaf Node의 마지막 Slot일 때,
 * Next Node의 첫번째 Key의 Row Pointer를 가져온다.
 *
 * 동시성에서 dirty read를 활용한 non block 설계 되었는데,
 * 최적화로 인하여 다시 가져오지 않는 문제가 있고,
 * 자주 호출되지 않아서, 별도의 함수로 추출한다.
 * BUG-47554 memory index 에서 latch 관리를
 * IDL_MEM_BARRIER 가 아닌 atomic 함수로 변경합니다.
 *
 * a_pLeafNode - [IN] Leaf Node
 *********************************************************************/
SChar* smnbBTree::getNxtLNodeFstRow( smnbLNode * a_pLeafNode )
{
    smnbLNode *sNextNode;
    IDU_LATCH  sVersion;
    SChar     *sNextRowPtr;

    // cur node 에 lock이 잡혀 있어서 next는 변경되지 않는다.
    sNextNode = (smnbLNode*)a_pLeafNode->nextSPtr;

    if (( sNextNode == NULL ) ||
        ( smuProperty::getSkipIdxStatNodeBound() == ID_TRUE ))
    {
        sNextRowPtr = NULL;
    }
    else
    {
        do
        {
            sVersion    = getLatchValueOfLNode( sNextNode ) & IDU_LATCH_UNMASK;
            sNextRowPtr = (SChar*)acpAtomicGet64( &sNextNode->mRowPtrs[0] );

        } while( sVersion != getLatchValueOfLNode(sNextNode) );
    }
    return sNextRowPtr;
}

/*********************************************************************
 * 갱신 할 Key의 위치가 Leaf Node의 첫번째 Slot일 때,
 * Prev Node의 마지막 Key의 Row Pointer를 가져온다.
 *
 * 동시성에서 dirty read를 활용한 non block 설계 되었는데,
 * 최적화로 인하여 다시 가져오지 않는 문제가 있고,
 * 자주 호출되지 않아서, 별도의 함수로 추출한다.
 * BUG-47554 memory index 에서 latch 관리를
 * IDL_MEM_BARRIER 가 아닌 atomic 함수로 변경합니다.
 *
 * a_pLeafNode - [IN] Leaf Node
 *********************************************************************/
SChar* smnbBTree::getPrvLNodeLstRow( smnbLNode * a_pLeafNode )
{
    smnbLNode * sPrevNode;
    IDU_LATCH   sVersion;
    SChar     * sPrevRowPtr;

    // cur node 에 lock이 잡혀 있어서 prev는 변경되지 않는다.
    sPrevNode = (smnbLNode*)a_pLeafNode->prevSPtr;

    if (( sPrevNode == NULL ) ||
        ( smuProperty::getSkipIdxStatNodeBound() == ID_TRUE ))
    {
        sPrevRowPtr = NULL;
    }
    else
    {
        do
        {
            sVersion    = getLatchValueOfLNode( sPrevNode ) & IDU_LATCH_UNMASK;

            // slot count가 변경 될 수 있으므로 atomic을 사용하지 못하고 mem barrier 로 갱신해서 가져옴
            IDL_MEM_BARRIER;
            sPrevRowPtr = sPrevNode->mRowPtrs[sPrevNode->mSlotCount - 1];

        }while( sVersion != getLatchValueOfLNode(sPrevNode) );
    }

    return sPrevRowPtr;
}

IDE_RC smnbBTree::insertIntoLeafNode( smnbHeader      * aHeader,
                                      smnbLNode       * aLeafNode,
                                      SChar           * aRow,
                                      SInt              aSlotPos )
{
    /* PROJ-2433 */
    if ( aSlotPos <= ( aLeafNode->mSlotCount - 1 ) )
    {
        shiftLeafSlots( aLeafNode,
                        (SShort)aSlotPos,
                        (SShort)( aLeafNode->mSlotCount - 1 ),
                        1 );
    }
    else
    {
        /* nothing to do */
    }

    setLeafSlot( aLeafNode,
                 (SShort)aSlotPos,
                 aRow,
                 NULL ); /* direct key는 아래서 세팅 */

    if ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE )
    {
        IDE_TEST( smnbBTree::makeKeyFromRow( aHeader, 
                                             aRow,
                                             SMNB_GET_KEY_PTR( aLeafNode, aSlotPos ) ) 
                  != IDE_SUCCESS );
    }
    else
    {
        /* nothing to do */
    }

    IDL_MEM_BARRIER;

    aLeafNode->mSlotCount++;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::insertIntoInternalNode( smnbHeader  * aHeader,
                                          smnbINode   * a_pINode,
                                          void        * a_pRow,
                                          void        * aKey,
                                          void        * a_pChild )
{
    SInt s_nSlotPos;

    IDE_TEST( findSlotInNode( aHeader,
                              (smnbNode *)a_pINode,
                              a_pRow,
                              &s_nSlotPos ) != IDE_SUCCESS );

    /* PROJ-2433 */
    if ( s_nSlotPos <= ( a_pINode->mSlotCount - 1 ) )
    {
        shiftInternalSlots( a_pINode,
                            (SShort)s_nSlotPos,
                            (SShort)( a_pINode->mSlotCount - 1 ),
                            1 );
    }
    else
    {
        /* nothing to do */
    }

    setInternalSlot( a_pINode,
                     (SShort)s_nSlotPos,
                     (smnbNode *)a_pChild,
                     (SChar *)a_pRow,
                     aKey );

    IDL_MEM_BARRIER;

    a_pINode->mSlotCount++;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::findSlotInNode                  *
 * ------------------------------------------------------------------*
 * NODE 내에서 row의 위치를 찾는다.
 *
 * - 일반 INDEX인 경우
 *   함수 findRowInLeaf() 를 사용해 검색한다.
 *
 * - (PROJ-2433) Direct Key Index를 사용중이라면
 *   함수 findKeyInNode() 를 사용해 direct key 기반으로 검색후
 *   함수 compareRowsAndPtr()와 findRowInNode() 를 사용해 row 기반으로 재검색한다.
 *
 *  => direct key로 비교후 row 기반으로 또다시 비교해야 하는 이유.
 *    1. direct key가 partail key 인경우 정확한 위치를 찾을수 없기때문이다
 *    2. 동일한 key 값이 여러개이면 row pointer까지 비교해 정확한 slot을 찾아야하기 때문이다.
 *
 * aHeader   - [IN]  INDEX 헤더
 * aNode     - [IN]  INDEX NODE
 * aRow      - [IN]  검색할 row pointer
 * aSlot     - [OUT] 찾아진 slot 위치
 *********************************************************************/
IDE_RC smnbBTree::findSlotInNode( smnbHeader  * aHeader,
                                  smnbNode    * aNode,
                                  void        * aRow,
                                  SInt        * aSlot )
{
    SInt    sMinimum = 0;
    SInt    sMaximum = 0;

    sMaximum = aNode->mSlotCount - 1;

    if ( sMaximum == -1 )
    {
        *aSlot = 0;

        return IDE_SUCCESS;
    }

    if ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE )
    {
        IDE_TEST( findKeyInNode( aHeader,
                                 aNode,
                                 sMinimum,
                                 sMaximum,
                                 (SChar *)aRow,
                                 aSlot ) != IDE_SUCCESS );

        if ( *aSlot > sMaximum )
        {
            /* 범위내에없다. */
            return IDE_SUCCESS;
        }
 
        if ( compareRowsAndPtr( &aHeader->mAgerStat,
                                aHeader->columns,
                                aHeader->fence,
                                aNode->mRowPtrs[*aSlot],
                                aRow )
             >= 0 )
        {
            return IDE_SUCCESS;
        }
        else
        {
            sMinimum = *aSlot + 1; /* *aSlot은 찾는 slot이 아닌것을 확인했다. skip 한다. */
        }
    }

    if ( sMinimum <= sMaximum )
    {
        findRowInNode( aHeader,
                       aNode,
                       sMinimum,
                       sMaximum,
                       (SChar *)aRow,
                       aSlot );
    }
    else
    {
        /* PROJ-2433
         * node 범위내에 만족하는값이없다. */
        *aSlot = sMaximum + 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::findKeyInNode( smnbHeader   * aHeader,
                                 smnbNode     * aNode,
                                 SInt           aMinimum,
                                 SInt           aMaximum,
                                 SChar        * aRow,
                                 SInt         * aSlot )
{
    idBool  sIsSuccess      = ID_TRUE;
    SInt    sMedium         = 0;
    UInt    sPartialKeySize = 0;

    if ( aHeader->mIsPartialKey == ID_TRUE )
    {
        sPartialKeySize = aNode->mKeySize;
    }

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;

        if ( compareKeys( &aHeader->mAgerStat,
                          aHeader->columns,
                          aHeader->fence,
                          SMNB_GET_KEY_PTR( aNode, sMedium ),
                          aNode->mRowPtrs[sMedium],
                          sPartialKeySize,
                          aRow,
                          &sIsSuccess )
             >= 0 )
        {
            aMaximum = sMedium - 1;
            *aSlot   = sMedium;
        }
        else
        {
            aMinimum = sMedium + 1;
            *aSlot   = aMinimum;
        }

        IDE_TEST( sIsSuccess != ID_TRUE );
    }
    while ( aMinimum <= aMaximum );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smnbBTree::findRowInNode( smnbHeader   * aHeader,
                               smnbNode     * aNode,
                               SInt           aMinimum,
                               SInt           aMaximum,
                               SChar        * aRow,
                               SInt         * aSlot )
{
    SInt    sMedium = 0;

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;

        if ( compareRowsAndPtr( &aHeader->mAgerStat,
                                aHeader->columns,
                                aHeader->fence,
                                aNode->mRowPtrs[sMedium],
                                aRow )
             >= 0 )
        {
            aMaximum = sMedium - 1;
            *aSlot   = sMedium;
        }
        else
        {
            aMinimum = sMedium + 1;
            *aSlot   = aMinimum;
        }
    }
    while ( aMinimum <= aMaximum );
}

IDE_RC smnbBTree::splitInternalNode(smnbHeader      * a_pIndexHeader,
                                    void            * a_pRow,
                                    void            * aKey,
                                    void            * a_pChild,
                                    SInt              a_nSlotPos,
                                    smnbINode       * a_pINode,
                                    smnbINode       * a_pNewINode,
                                    void           ** a_pSepKeyRow,
                                    void           ** aSepKey )
{
    SInt i = 0;

    IDL_MEM_BARRIER;

    // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
    a_pIndexHeader->nodeCount++;

    // To fix BUG-18671
    initInternalNode( a_pNewINode,
                      a_pIndexHeader,
                      IDU_LATCH_UNLOCKED );

    IDL_MEM_BARRIER; /* 노드의 초기화후에 link를 수행해야 됨*/

    a_pNewINode->nextSPtr  = a_pINode->nextSPtr;
    a_pNewINode->prevSPtr  = a_pINode;

    if ( a_pINode->nextSPtr != NULL )
    {
        SMNB_SCAN_LATCH(a_pINode->nextSPtr);
        a_pINode->nextSPtr->prevSPtr = a_pNewINode;
        SMNB_SCAN_UNLATCH(a_pINode->nextSPtr);
    }

    a_pINode->nextSPtr = a_pNewINode;

    if ( a_nSlotPos <= SMNB_INTERNAL_SLOT_SPLIT_COUNT( a_pIndexHeader ) )
    {
        /* PROJ-2433 */
        if ( SMNB_INTERNAL_SLOT_SPLIT_COUNT(a_pIndexHeader) <= ( a_pINode->mSlotCount - 1 ) )
        {
            copyInternalSlots( a_pNewINode,
                               0,
                               a_pINode,
                               (SShort)( SMNB_INTERNAL_SLOT_SPLIT_COUNT(a_pIndexHeader) ),
                               (SShort)( a_pINode->mSlotCount - 1 ) );
        }
        else
        {
            /* 여기로 올수 없다. */
            IDE_ERROR_RAISE( 0, ERR_CORRUPTED_INDEX );
        }

        a_pNewINode->mSlotCount = a_pINode->mSlotCount - SMNB_INTERNAL_SLOT_SPLIT_COUNT(a_pIndexHeader);
        a_pINode   ->mSlotCount = SMNB_INTERNAL_SLOT_SPLIT_COUNT(a_pIndexHeader);

        if ( a_nSlotPos <= ( a_pINode->mSlotCount - 1 ) )
        {
            shiftInternalSlots( a_pINode,
                                (SShort)a_nSlotPos,
                                (SShort)( a_pINode->mSlotCount - 1 ),
                                1 );
        }
        else
        {
            /* nothing to do */
        }

        setInternalSlot( a_pINode,
                         (SShort)a_nSlotPos,
                         (smnbNode *)a_pChild,
                         (SChar *)a_pRow,
                         aKey );

        (a_pINode->mSlotCount)++;
    }
    else
    {
        /* PROJ-2433 */
        if ( SMNB_INTERNAL_SLOT_SPLIT_COUNT(a_pIndexHeader) <= ( a_nSlotPos - 1 ) )
        {
            copyInternalSlots( a_pNewINode,
                               0,
                               a_pINode,
                               (SShort)( SMNB_INTERNAL_SLOT_SPLIT_COUNT( a_pIndexHeader ) ),
                               (SShort)( a_nSlotPos - 1 ) );

            i = a_nSlotPos - SMNB_INTERNAL_SLOT_SPLIT_COUNT( a_pIndexHeader );
        }
        else
        {
            /* 여기로 올수 없다. */
            IDE_ERROR_RAISE( 0, ERR_CORRUPTED_INDEX );
        }

        setInternalSlot( a_pNewINode,
                         (SShort)i,
                         (smnbNode *)a_pChild,
                         (SChar *)a_pRow,
                         aKey );

        i++;

        copyInternalSlots( a_pNewINode,
                           (SShort)i,
                           a_pINode,
                           (SShort)a_nSlotPos,
                           (SShort)( a_pINode->mSlotCount - 1 ) );

        a_pNewINode->mSlotCount = a_pINode->mSlotCount - SMNB_INTERNAL_SLOT_SPLIT_COUNT( a_pIndexHeader ) + 1;
        a_pINode   ->mSlotCount = SMNB_INTERNAL_SLOT_SPLIT_COUNT( a_pIndexHeader );
    }

    *a_pSepKeyRow = NULL;
    *aSepKey      = NULL;

    getInternalSlot( NULL,
                     (SChar **)a_pSepKeyRow,
                     aSepKey,
                     a_pINode,
                     (SShort)( a_pINode->mSlotCount - 1 ) );

    IDL_MEM_BARRIER;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index SlotCount" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::splitLeafNode( smnbHeader    * a_pIndexHeader,
                                 SInt            a_nSlotPos,
                                 smnbLNode     * a_pLeafNode,
                                 smnbLNode     * a_pNewLeafNode,
                                 smnbLNode    ** aTargetLeaf,
                                 SInt          * aTargetSeq )
{
    IDL_MEM_BARRIER;

    // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
    a_pIndexHeader->nodeCount++;

#ifdef ALTIBASE_FIT_CHECK
    a_pNewLeafNode->latch = IDU_LATCH_LOCKED;
#endif
    IDU_FIT_POINT_RAISE( "BUG-46504@smnbBTree::splitLeafNode::initLeafNode", ERR_FAIL_TO_INITIALIZE_LEAF_NODE );

    // To fix BUG-18671
    IDE_ERROR_RAISE( initLeafNode( a_pNewLeafNode,
                                   a_pIndexHeader,
                                   IDU_LATCH_LOCKED ) == IDE_SUCCESS,
                     ERR_FAIL_TO_INITIALIZE_LEAF_NODE );
    
    SMNB_SCAN_UNLATCH( a_pNewLeafNode );

    IDL_MEM_BARRIER; /* 노드의 초기화후에 link를 수행해야 됨*/

    /* new node에 key 분배가 완료될때까지 잡아놔야한다.
     * 그렇지 않으면, no latch로 이전 노드를 보려할때 아직 key 분배가 안된
     * 상태에서 볼 수 있다.
     * 예를 들면, 통계 갱신시 이전 노드를 보려할때..
     * 따라서 new node를 tree에 연결하기 전에 latch bit를 설정해야 한다. */
    SMNB_SCAN_LATCH( a_pNewLeafNode );

    a_pNewLeafNode->nextSPtr  = a_pLeafNode->nextSPtr;
    a_pNewLeafNode->prevSPtr  = a_pLeafNode;

    if ( a_pLeafNode->nextSPtr != NULL)
    {
        /* split을 하기 위해 tree latch 잡은 상황이라 Tree 구조가 변경되지
         * 않는다. backward scan시에도 tree latch를 잡기 때문에 문제 없다. */
        a_pLeafNode->nextSPtr->prevSPtr = a_pNewLeafNode;
    }

    a_pLeafNode->nextSPtr = a_pNewLeafNode;

    if ( a_nSlotPos <= SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader ) )
    {
        /* PROJ-2433 */
        if ( SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader ) <= ( a_pLeafNode->mSlotCount - 1 ) )
        {
            copyLeafSlots( a_pNewLeafNode,
                           0,
                           a_pLeafNode,
                           (SShort)( SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader ) ),
                           (SShort)( a_pLeafNode->mSlotCount - 1 ) );
        }
        else
        {
            /* nothing to do */
        }

        a_pNewLeafNode->mSlotCount = a_pLeafNode->mSlotCount - SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader );
        a_pLeafNode   ->mSlotCount = SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader );

        *aTargetLeaf = a_pLeafNode;
        *aTargetSeq = a_nSlotPos;
    }
    else
    {
        /* PROJ-2433 */
        if ( SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader ) <= ( a_pLeafNode->mSlotCount - 1 ) )
        {
            copyLeafSlots( a_pNewLeafNode,
                           0,
                           a_pLeafNode,
                           (SShort)( SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader ) ),
                           (SShort)( a_pLeafNode->mSlotCount - 1 ) );
        }
        else
        {
            /* nothing to do */
        }

        a_pLeafNode   ->mSlotCount = SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader );
        a_pNewLeafNode->mSlotCount = ( SMNB_LEAF_SLOT_MAX_COUNT( a_pIndexHeader ) -
                                       SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader ) );

        *aTargetLeaf = a_pNewLeafNode;
        *aTargetSeq  = a_nSlotPos - SMNB_LEAF_SLOT_SPLIT_COUNT( a_pIndexHeader );
    }

    SMNB_SCAN_UNLATCH( a_pNewLeafNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_INITIALIZE_LEAF_NODE )
    {
        SMNB_SCAN_UNLATCH( a_pNewLeafNode ); /* LATCH 풀어줘야함 */

        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] fail to initialize leaf node" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::checkUniqueness( smnIndexHeader        * aIndexHeader,
                                   void                  * a_pTrans,
                                   smnbStatistic         * aIndexStat,
                                   const smnbColumn      * a_pColumns,
                                   const smnbColumn      * a_pFence,
                                   smnbLNode             * a_pLeafNode,
                                   SInt                    a_nSlotPos,
                                   SChar                 * a_pRow,
                                   smSCN                   aStmtViewSCN,
                                   idBool                  aIsTreeLatched,
                                   smTID                 * a_pTransID,
                                   idBool                * aIsRetraverse,
                                   SChar                ** aExistUniqueRow,
                                   SChar                ** a_diffRow,
                                   SInt                  * a_diffSlotPos,
                                   idBool                  aForbiddenToRetry )
{
    SInt          i;
    SInt          j;
    SInt          s_nSlotPos;
    smnbLNode   * s_pCurLeafNode;
    UInt          sState = 0;
    SInt          sSlotCount = 0;
    SInt          sCompareResult;
    idBool        sIsReplTx = ID_FALSE;
    smnbLNode   * sTempNode = NULL;
    idBool        sCheckEnd = ID_FALSE;
    UInt          sTempLockState = 0;

    *a_pTransID    = SM_NULL_TID;
    *aIsRetraverse = ID_FALSE;

    sIsReplTx = smxTransMgr::isReplTrans( a_pTrans );

    /* 이웃 노드를 봐야할 필요가 있는 경우 Tree latch를 잡아야 한다.
     * 현재 삽입하고자 하는 노드에만 latch가 잡혀있는 상태이므로
     * 해당 노드만 보고 좌우 이웃노드를 봐야하는지 판단해야 한다.
     * 삽입할 위치가 처음 또는 마지막 이거나 삽입할 row와 첫번째, 마지막 row의
     * 값이 동일하면 좌우 이웃 노드를 봐야한다고 판단한다. */
    if ( aIsTreeLatched == ID_FALSE )
    {
        if ( ( a_nSlotPos == a_pLeafNode->mSlotCount ) ||
             ( compareRows( aIndexStat,
                            a_pColumns,
                            a_pFence,
                            a_pRow,
                            a_pLeafNode->mRowPtrs[a_pLeafNode->mSlotCount - 1] ) == 0 ) )
        {
            *aIsRetraverse = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        if ( ( a_nSlotPos == 0 ) ||
             ( compareRows( aIndexStat,
                            a_pColumns,
                            a_pFence,
                            a_pRow,
                            a_pLeafNode->mRowPtrs[0] ) == 0 ) )
        {
            *aIsRetraverse = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        /* Tree latch 잡고 다시 시도해야 한다. */
        if ( *aIsRetraverse == ID_TRUE )
        {
            IDE_CONT( need_to_retraverse );
        }
    }

    /*
     * uniqueness 체크를 시작한다.
     */

    s_pCurLeafNode = a_pLeafNode;

    /* backward 탐색
     * backward로 lock를 잡는데, Tree latch를 획득한 상태이기 때문에
     * 데드락 문제는 없다.
     *
     * lock을 잡는 이유는 이전 맨 처음 노드에는 동일 값을 가진 key와 다른 값을
     * 가진 key가 섞여있기 때문에 Tree latch없이 insert/freeSlot이 수행될 수
     * 있기고, 이때문에 slot 위치가 변경될 수 있기 때문이다. */
    while ( s_pCurLeafNode != NULL )
    {
        /* 삽입할 노드는 이미 latch가 잡혀있으므로, 또 잡으면 안된다. */
        if ( a_pLeafNode != s_pCurLeafNode )
        {
            /* 이웃 노드를 봐야한다면 Tree latch가 잡혀있어야 한다. */
            IDE_ERROR_RAISE( ( aIsTreeLatched == ID_TRUE ), ERR_NOT_TREE_LATCH );

            lockNode( s_pCurLeafNode );
            sState = 1;

            /* 마지막 slot부터 시작. */
            s_nSlotPos = s_pCurLeafNode->mSlotCount;
        }
        else
        {
            /* 삽입할 노드는 삽입할 위치 부터 시작 */
            s_nSlotPos = a_nSlotPos;
        }

        for (i = s_nSlotPos - 1; i >= 0; i--)
        {
            sCompareResult = compareRows( aIndexStat,
                                          a_pColumns,
                                          a_pFence,
                                          a_pRow,
                                          s_pCurLeafNode->mRowPtrs[i] );
            if ( sCompareResult > 0 )
            {
                break;
            }

            if ( sCompareResult != 0 )
            {
                smnbBTree::logIndexHeader( aIndexHeader );
                logIndexNode( aIndexHeader, (smnbNode*)s_pCurLeafNode );
                IDE_ERROR_RAISE( 0, ERR_CORRUPTED_INDEX );
            }

            IDE_TEST( isRowUnique( a_pTrans,
                                   aIndexStat,
                                   aStmtViewSCN,
                                   s_pCurLeafNode->mRowPtrs[i],
                                   a_pTransID,
                                   aExistUniqueRow,
                                   aForbiddenToRetry )
                      != IDE_SUCCESS );

            if ( *a_pTransID != SM_NULL_TID )
            {
                *a_diffSlotPos = i;
                *a_diffRow = (SChar*)s_pCurLeafNode->mRowPtrs[i];

                /* BUG-45524 replication 사용시 reciever쪽에서는 Tx 대기를 하지 않기 때문에 
                 *           일시적으로 동일한 키가 다수 존재할 수 있습니다.
                 *           중복된 키는 sender쪽의 rollback로그에 의해 자연적으로 제거 되긴하지만
                 *           제거될 예정인 키를 unique violation 체크시 비교 대상으로 삼은 상태에서
                 *           local query나 log 유실이 발생할 경우 문제가 발생할 수 있습니다.
                 *           이를 막기 위해 동일한 키가 다수 존재할 경우에 이들 중
                 *           이들 모두를 대상으로 비교 연산을 수행해야 합니다. */
                if( sIsReplTx != ID_TRUE )
                {
                    /* replication이 아니라면 타 slot을 탐색할 필요가 없다. */
                    break;
                }
                else
                {
                    /* 동일한 키 값이 있는 row가 더 있는지 이전 slot을 확인한다.
                     * 동일한 키 값을 가진 row중 commit된 row가 있다면 해당 row를 상태로 unique check를 해야 >한다. */
                    j = i - 1;

                    while( j >= 0 )
                    {
                        sCompareResult = compareRows( aIndexStat,
                                                      a_pColumns,
                                                      a_pFence,
                                                      a_pRow,
                                                      s_pCurLeafNode->mRowPtrs[j] );

                        if( sCompareResult == 0 )
                        {
                            /* 동일한 키 값을 가진 row가 있다면 isRowUnique에서 unique 체크를 한다. */
                            IDE_TEST(isRowUnique(a_pTrans,
                                                 aIndexStat,
                                                 aStmtViewSCN,
                                                 s_pCurLeafNode->mRowPtrs[j],
                                                 a_pTransID,
                                                 aExistUniqueRow,
                                                 aForbiddenToRetry)
                                     != IDE_SUCCESS);

                            /* unique check를 통과 했다면 이전 slot에 대해서도 반복한다. */
                            j--;
                        }
                        else
                        {
                            /* 동일한 키값을 가진 row가 이전 slot에 없음 */
                            break;
                        }
                    }

                    if( ( j != -1 ) || ( s_pCurLeafNode->prevSPtr == NULL) )
                    {
                        /* 노드 내에서 동일하지 않은 키를 찾았거나 이전 노드가 없다면 
                         * 이전 노드의 탐색을 수행 하지 않는다. */
                        break;
                    }
                    else
                    {
                        /* 키 값이 탐색된 노드 전체를 탐색한 후에도 동일 키 값이 계속 발견된다면
                         * 이전 노드에 대해서도 탐색을 수행해야 한다. */
                    }

                    /* 노드에 lock을 걸어야 하므로 tree latch를 잡고 있지 않다면 tree latch를 잡고 재수행한다. */
                    if( aIsTreeLatched == ID_FALSE )
                    {
                        if( sState == 1)
                        {
                            sState = 0;
                            unlockNode( s_pCurLeafNode );
                        }

                        *aIsRetraverse = ID_TRUE;
                        IDE_CONT( need_to_retraverse );
                    }
                    else
                    {
                        sTempNode = s_pCurLeafNode->prevSPtr;

                        while( ( sCheckEnd == ID_FALSE ) && ( sTempNode != NULL ) )
                        {
                            lockNode( sTempNode );
                            sTempLockState = 1;

                            for( j = ( sTempNode->mSlotCount - 1) ; j>= 0; j-- )
                            {
                                sCompareResult = compareRows( aIndexStat,
                                                              a_pColumns,
                                                              a_pFence,
                                                              a_pRow,
                                                              sTempNode->mRowPtrs[j] );

                                if( sCompareResult != 0 )
                                {
                                    /* 동일하지 않은 키가 발견될 경우 더 이상 탐색할 필요 없다. */
                                    sTempLockState = 0;
                                    unlockNode( sTempNode );

                                    sCheckEnd = ID_TRUE;

                                    break;
                                }
                                else
                                {
                                    /* 동일한 키 값이 발견될 경우 unique check를 수행한다. */
                                    IDE_TEST(isRowUnique(a_pTrans,
                                                         aIndexStat,
                                                         aStmtViewSCN,
                                                         sTempNode->mRowPtrs[j],
                                                         a_pTransID,
                                                         aExistUniqueRow,
                                                         aForbiddenToRetry)
                                             != IDE_SUCCESS);
                                }
                            }

                            if( sCheckEnd == ID_TRUE )
                            {
                                break;
                            }
                            /* 이전 노드 하나를 다 탐색했음에도 동일 키 값이 계속 발견된다면
                             * 다시 그 앞 노드에 대해서도 탐색을 수행한다.*/

                            sTempLockState = 0;
                            unlockNode( sTempNode );

                            sTempNode = sTempNode->prevSPtr;
                        }
                    } 
                }
                break;
            }
        }

        if ( sState == 1 )
        {
            sState = 0;
            unlockNode( s_pCurLeafNode );
        }

        if ( *a_pTransID != SM_NULL_TID )
        {
            IDE_CONT( need_to_wait );
        }

        if ( i != -1 )
        {
            break;
        }

        s_pCurLeafNode = s_pCurLeafNode->prevSPtr;
    }

    /* forward */
    s_pCurLeafNode = a_pLeafNode;

    while (s_pCurLeafNode != NULL)
    {
        if ( a_pLeafNode != s_pCurLeafNode )
        {
            /* 이웃 노드를 봐야한다면 Tree latch가 잡혀있어야 한다. */
            IDE_ERROR_RAISE( ( aIsTreeLatched == ID_TRUE ), ERR_NOT_TREE_LATCH );

            lockNode( s_pCurLeafNode );
            sState = 1;

            /* 이웃 노드인 경우 처음부터 시작 */
            s_nSlotPos = -1;
        }
        else
        {
            /* 삽입한 노드는 삽입할 위치부터 시작 */
            s_nSlotPos = a_nSlotPos - 1;
        }

        sSlotCount = s_pCurLeafNode->mSlotCount;

        for (i = s_nSlotPos + 1; i < sSlotCount; i++)
        {
            sCompareResult = compareRows( aIndexStat,
                                          a_pColumns,
                                          a_pFence,
                                          a_pRow,
                                          s_pCurLeafNode->mRowPtrs[i] );
            if ( sCompareResult < 0 )
            {
                break;
            }

            if ( sCompareResult != 0 )
            {
                smnbBTree::logIndexHeader( aIndexHeader );
                logIndexNode( aIndexHeader, (smnbNode*)s_pCurLeafNode );
                IDE_ERROR_RAISE( 0, ERR_CORRUPTED_INDEX );
            }

            IDE_TEST( isRowUnique( a_pTrans,
                                   aIndexStat,
                                   aStmtViewSCN,
                                   s_pCurLeafNode->mRowPtrs[i],
                                   a_pTransID,
                                   aExistUniqueRow,
                                   aForbiddenToRetry )
                      != IDE_SUCCESS );

            if ( *a_pTransID != SM_NULL_TID )
            {
                *a_diffSlotPos = i;
                *a_diffRow = s_pCurLeafNode->mRowPtrs[i];

                /* BUG-45524 replication 사용시 reciever쪽에서는 Tx대기를 하지 않기 때문에 
                 *           일시적으로 동일한 키가 다수 존재할 수 있습니다.
                 *           중복된 키는 sender쪽의 rollback로그에 의해 자연적으로 제거 되긴하지만
                 *           제거될 예정인 키를 unique violation 체크시 비교 대상으로 삼은 상태에서
                 *           local query나 log 유실이 발생할 경우 문제가 발생할 수 있습니다.
                 *           이를 막기 위해 동일한 키가 다수 존재할 경우에 
                 *           이들 모두를 대상으로 비교 연산을 수행해야 합니다. */
                if( sIsReplTx != ID_TRUE )
                {    
                    /* replication이 아니라면 타 slot을 탐색할 필요가 없다. */
                    break;
                }    
                else 
                {    
                    /* 동일한 키 값이 있는 row가 더 있는지 다음 slot을 확인한다.
                     * 동일한 키 값을 가진 row중 commit된 row가 있다면 해당 row를 상태로 unique check를 해야 한다. */
                    j = i + 1;

                    while( j < sSlotCount )
                    {    
                        sCompareResult = compareRows( aIndexStat,
                                                      a_pColumns,
                                                      a_pFence,
                                                      a_pRow,
                                                      s_pCurLeafNode->mRowPtrs[j] );

                        if( sCompareResult == 0 )
                        {   
                            /* 동일한 키 값을 가진 row가 있다면 isRowUnique에서 unique 체크를 한다. */
                            IDE_TEST(isRowUnique(a_pTrans,
                                                 aIndexStat,
                                                 aStmtViewSCN,
                                                 s_pCurLeafNode->mRowPtrs[j],
                                                 a_pTransID,
                                                 aExistUniqueRow,
                                                 aForbiddenToRetry)
                                     != IDE_SUCCESS);

                            /* unique check를 통과 했다면 다음 slot에 대해서도 반복한다. */
                            j++;
                        }    
                        else 
                        {    
                            /* 동일한 키값을 가진 row가 다음 slot에 없음 */
                            break;
                        }
                    }

                    if( ( j != sSlotCount ) || ( s_pCurLeafNode->nextSPtr == NULL) )
                    {
                        /* 노드 내에서 동일하지 않은 키를 찾았거나 다음 노드가 없다면 
                         * 다음 노드의 탐색을 수행하지 않는다. */
                        break;
                    }
                    else
                    {
                        /* 키 값이 탐색된 노드 전체를 탐색한 후에도 동일 키 값이 계속 발견된다면
                         * 다음 노드에 대해서도 탐색을 수행해야 한다. */
                    }

                    /* 노드에 lock을 걸어야 하므로 tree latch를 잡고 있지 않다면 tree latch를 잡고 재수행한다. */
                    if( aIsTreeLatched == ID_FALSE )
                    {
                        if( sState == 1 )
                        {
                            sState = 0;
                            unlockNode( s_pCurLeafNode );
                        }

                        *aIsRetraverse = ID_TRUE;
                        IDE_CONT( need_to_retraverse );
                    }
                    else
                    {
                        sTempNode = s_pCurLeafNode->nextSPtr;

                        while( ( sCheckEnd == ID_FALSE ) && ( sTempNode != NULL ) )
                        {
                            lockNode( sTempNode );
                            sTempLockState = 1;

                            for( j = 0 ; j < sTempNode->mSlotCount ; j++ )
                            {
                                sCompareResult =
                                    compareRows( aIndexStat,
                                                 a_pColumns,
                                                 a_pFence,
                                                 a_pRow,
                                                 sTempNode->mRowPtrs[j]);

                                if( sCompareResult != 0)
                                {
                                    /* 동일하지 않은 키가 발견될 경우 더 이상 탐색할 필요 없다. */
                                    sTempLockState = 0;
                                    unlockNode( sTempNode );

                                    sCheckEnd = ID_TRUE;

                                    break;
                                }
                                else
                                {
                                    /* 동일한 키 값이 발견될 경우 unique check를 수행한다. */
                                    IDE_TEST(isRowUnique(a_pTrans,
                                                         aIndexStat,
                                                         aStmtViewSCN,
                                                         sTempNode->mRowPtrs[j],
                                                         a_pTransID,
                                                         aExistUniqueRow,
                                                         aForbiddenToRetry)
                                             != IDE_SUCCESS);
                                }
                            }

                            if( sCheckEnd == ID_TRUE )
                            {
                                break;
                            }
                            /* 다음 노드 하나를 다 탐색했음에도 동일 키 값이 계속 발견된다면
                             * 다시 그 다음 노드에 대해서도 탐색을 수행한다.*/

                            sTempLockState = 0;
                            unlockNode( sTempNode );

                            sTempNode = sTempNode->nextSPtr;
                        }
                    }
                }
                break;
            }
        }

        if ( sState == 1 )
        {
            sState = 0;
            unlockNode( s_pCurLeafNode );
        }

        if ( *a_pTransID != SM_NULL_TID )
        {
            IDE_CONT( need_to_wait );
        }

        if ( i != sSlotCount )
        {
            break;
        }

        IDE_ERROR_RAISE( ( aIsTreeLatched == ID_TRUE ), ERR_NOT_TREE_LATCH );

        s_pCurLeafNode = s_pCurLeafNode->nextSPtr;
    }

    IDE_EXCEPTION_CONT( need_to_wait );
    IDE_EXCEPTION_CONT( need_to_retraverse );

    IDE_ERROR_RAISE( ( sState == 0 ), ERR_NOT_STATE_ZERO );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Order" );
    }
    IDE_EXCEPTION( ERR_NOT_TREE_LATCH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] Tree Latch is not latched." );
    }
    IDE_EXCEPTION( ERR_NOT_STATE_ZERO )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] State is not zero." );
    }
    IDE_EXCEPTION_END;
    
    if( sTempLockState == 1 )
    {
        unlockNode( sTempNode );
    }

    if ( sState == 1 )
    {
        unlockNode( s_pCurLeafNode );
    }

    return IDE_FAILURE;
}

void smnbBTree::getPreparedNode( smnbNode    ** aNodeList,
                                 UInt         * aNodeCount,
                                 void        ** aNode )
{
    smnbNode    *sNode;
    smnbNode    *sPrev;
    smnbNode    *sNext;

    IDE_DASSERT( aNodeList  != NULL );
    IDE_DASSERT( aNodeCount != NULL );
    IDE_DASSERT( aNode      != NULL );
    IDE_DASSERT( *aNodeCount > 0 );

    sNode = *aNodeList;
    sPrev = (smnbNode*)((*aNodeList)->prev);
    sNext = (smnbNode*)((*aNodeList)->next);

    sPrev->next = sNext;
    sNext->prev = sPrev;

    sNode->prev = NULL;
    sNode->next = NULL;

    *aNode = sNode;
    (*aNodeCount)--;

    if ( *aNodeCount == 0 )
    {
        *aNodeList = NULL;
    }
    else
    {
        *aNodeList = sNext;
    }
}


IDE_RC smnbBTree::prepareNodes( smnbHeader    * aIndexHeader,
                                smnbStack     * aStack,
                                SInt            aDepth,
                                smnbNode     ** aNewNodeList,
                                UInt          * aNewNodeCount )
{
    smnbNode        * sNode;
    smnbNode        * sNewNodeList;
    smnbStack       * sStack;
    SInt              sCurDepth;
    SInt              sNewNodeCount;

    IDE_DASSERT( aIndexHeader != NULL );
    IDE_DASSERT( aStack       != NULL );
    IDE_DASSERT( aNewNodeList != NULL );
    IDE_DASSERT( aNewNodeCount != NULL );

    sStack    = aStack;

    *aNewNodeList  = NULL;
    *aNewNodeCount = 0;

    /* Leaf node가 full 되어 split 하는 것이기 때문에 Leaf node를 위한 새로운
     * 노드는 꼭 필요하게 된다. 그래서 필요한 노드 갯수는 1부터 시작 */
    sNewNodeCount = 1;

    for ( sCurDepth = aDepth; sCurDepth >= 0; sCurDepth-- )
    {
        sNode = (smnbNode*)sStack[sCurDepth].node;

        if ( SMNB_IS_INTERNAL_NODE( sNode ) )
        {
            /* slot count가 max에 도달했다면 split 과정에서 새로운 node를
             * 필요로하게 된다. */
            if ( sStack[sCurDepth].slotCount == SMNB_INTERNAL_SLOT_MAX_COUNT( aIndexHeader ) )
            {
                sNewNodeCount++;
            }
            else
            {
                IDE_ERROR_RAISE( sStack[sCurDepth].slotCount < SMNB_INTERNAL_SLOT_MAX_COUNT( aIndexHeader ), ERR_CORRUPTED_INDEX );
                break;
            }
        }
    }

    IDE_ERROR_RAISE( sNewNodeCount >= 1, ERR_CORRUPTED_INDEX );

    /* Root Node까지 split 되어야 한다면, 새로운 Root Node도 생성되어야 한다. */
    if ( sCurDepth < 0 )
    {
        sNewNodeCount++;
    }

    IDE_TEST( aIndexHeader->mNodePool.allocateSlots(
                                            sNewNodeCount,
                                            (smmSlot**)&sNewNodeList,
                                            SMM_SLOT_LIST_MUTEX_NEEDLESS )
              != IDE_SUCCESS );

    *aNewNodeList  = sNewNodeList;
    *aNewNodeCount = sNewNodeCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index SlotCount" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::makeRootNode                    *
 * ------------------------------------------------------------------*
 * BUG-47303
 *
 * Root Node를 생성한다.
 *
 * aIndex         - [IN] INDEX 정보
 * aRow           - [IN] row pointer
 * aIsUniqueIndex - [IN] UNIQUE INDEX 여부
 *********************************************************************/
IDE_RC smnbBTree::makeRootNode( smnIndexHeader * aIndex,
                                SChar          * aRow,
                                idBool           aIsUniqueIndex )
{
    smnbHeader * sIndexHeader = (smnbHeader *)(aIndex->mHeader);
    smnbLNode  * sRootLNode   = NULL;

    IDE_TEST( sIndexHeader->mNodePool.allocateSlots( 1,
                                                     (smmSlot**)&sRootLNode,
                                                     SMM_SLOT_LIST_MUTEX_NEEDLESS )
              != IDE_SUCCESS );

    /* BUG-18292 : V$MEM_BTREE_HEADER 정보 추가 */
    sIndexHeader->nodeCount++;

    IDE_ERROR_RAISE( initLeafNode( sRootLNode,
                                   sIndexHeader,
                                   IDU_LATCH_UNLOCKED ) == IDE_SUCCESS,
                     ERR_FAIL_TO_INITIALIZE_ROOT_NODE );

    IDE_TEST( insertIntoLeafNode( sIndexHeader,
                                  sRootLNode,
                                  aRow,
                                  0 ) != IDE_SUCCESS );

    if ( needToUpdateStat( sIndexHeader->mIsMemTBS ) == ID_TRUE )
    {
        IDE_TEST( updateStat4Insert( aIndex,
                                     sIndexHeader,
                                     sRootLNode,
                                     0,
                                     aRow,
                                     aIsUniqueIndex ) != IDE_SUCCESS );
    }

    IDL_MEM_BARRIER;

    sIndexHeader->root = sRootLNode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_INITIALIZE_ROOT_NODE )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] fail to initialize root node" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::processKeyRedistribution        *
 * ------------------------------------------------------------------*
 * BUG-47303
 *
 * 키 재분배를 수행한다.
 *
 * aIndex         - [IN]     INDEX 정보
 * aCurINode      - [IN]     키 재분배를 수행할 LEAF NODE의 상위 INTERNAL NODE
 * aCurLNode      - [IN]     키 재분배를 수행할 LEAF NODE
 * aIsTreeLatched - [IN/OUT] TREE LATCH를 잡았는지 여부
 * aIsNodeLatched - [IN/OUT] NODE LATCH를 잡았는지 여부
 *********************************************************************/
IDE_RC smnbBTree::processKeyRedistribution( smnIndexHeader * aIndex, 
                                            smnbINode      * aCurINode,
                                            smnbLNode      * aCurLNode,
                                            idBool         * aIsTreeLatched,
                                            idBool         * aIsNodeLatched )
{
    smnbHeader * sIndexHeader          = NULL;
    smnbLNode  * sNxtLNode             = NULL;
    idBool       sIsNxtNodeLatched     = ID_FALSE;
    SInt         sKeyRedistributeCount = 0;
    SChar      * sOldKeyRow            = NULL;

    sIndexHeader = (smnbHeader*)(aIndex->mHeader);
    sNxtLNode    = aCurLNode->nextSPtr;

    lockNode( sNxtLNode );
    sIsNxtNodeLatched = ID_TRUE;

    IDU_FIT_POINT( "BUG-45283@smnbBTree::insertRow::lockNode" ); 

    while(1) /* no loop */
    {
        if ( sNxtLNode->mSlotCount >= 
             ( ( SMNB_LEAF_SLOT_MAX_COUNT( sIndexHeader ) *
                 smuProperty::getMemIndexKeyRedistributionStandardRate() / 100 ) ) )
        {
            /* checkEnableKeyRedistribution 수행 후 이웃 노드에 lock을 잡는 사이에 이웃 노드에
               삽입연산이 발생하여 기준을 초과할 경우에는 키 재분배를 수행하지 않는다. */
            sIsNxtNodeLatched = ID_FALSE;
            unlockNode( sNxtLNode );

            *aIsNodeLatched = ID_FALSE;
            unlockNode( aCurLNode );

            *aIsTreeLatched = ID_FALSE;
            unlockTree( sIndexHeader );

            break;
        }

        /* 키 재분배로 이동 시킬 slot의 수를 구한다. */
        sKeyRedistributeCount = calcKeyRedistributionPosition( sIndexHeader,
                                                               aCurLNode->mSlotCount,
                                                               sNxtLNode->mSlotCount );

        sOldKeyRow = aCurLNode->mRowPtrs[aCurLNode->mSlotCount - 1 ];

        /* 키 재분배를 수행해 slot의 일부를 이웃 노드로 이동한다. */
        if ( keyRedistribute( sIndexHeader,
                              aCurLNode, 
                              sNxtLNode, 
                              sKeyRedistributeCount ) != IDE_SUCCESS )
        {
            /* 해당 노드 진입전 다른 Tx에 의해 split/aging이 발생한 경우이므로 재수행한다. */
            sIsNxtNodeLatched = ID_FALSE;
            unlockNode( sNxtLNode );

            *aIsNodeLatched = ID_FALSE;
            unlockNode( aCurLNode );

            *aIsTreeLatched = ID_FALSE;
            unlockTree( sIndexHeader );

            break;
        }

        sIsNxtNodeLatched = ID_FALSE;
        unlockNode( sNxtLNode );

        IDE_ERROR_RAISE( *aIsNodeLatched == ID_TRUE, ERR_NOT_NODE_LATCH );

        /* 부모 노드를 갱신하다. */
        IDE_TEST( keyRedistributionPropagate( sIndexHeader,
                                              aCurINode,
                                              aCurLNode,
                                              sOldKeyRow) != IDE_SUCCESS );

        /* 삽입 연산을 수행하기 위해 latch를 풀고 재시작한다. */
        *aIsNodeLatched = ID_FALSE;
        unlockNode( aCurLNode );

        *aIsTreeLatched = ID_FALSE;
        unlockTree( sIndexHeader );

        break;
    }

    if ( sIsNxtNodeLatched == ID_TRUE )
    {
        sIsNxtNodeLatched = ID_FALSE;
        unlockNode( sNxtLNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_NODE_LATCH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] Node Latch is not latched." );
    }
    IDE_EXCEPTION_END;

    if ( sIsNxtNodeLatched == ID_TRUE )
    {
        sIsNxtNodeLatched = ID_FALSE;
        unlockNode( sNxtLNode );
    }

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::processNodeSplit                *
 * ------------------------------------------------------------------*
 * BUG-47303
 *
 * NODE SPLIT 을 수행한다.
 *
 * aIndex         - [IN] INDEX 정보
 * aStack         - [IN] 인덱스 트리 탐색정보를 저장한 스택
 * aDepth         - [IN] 탐색한 인덱스 트리의 깊이 (aStack의 깊이)
 * aCurLNode      - [IN] SPLIT 할 LEAF NODE
 * aRow           - [IN] row pointer
 * aSlotPos       - [IN] row pointer를 입력할 LEAF NODE내 위치
 * aIsUniqueIndex - [IN] UNIQUE INDEX 여부
 *********************************************************************/
IDE_RC smnbBTree::processNodeSplit( smnIndexHeader * aIndex,
                                    smnbStack      * aStack,
                                    SInt             aDepth,
                                    smnbLNode      * aCurLNode,
                                    SChar          * aRow,
                                    SInt             aSlotPos,
                                    idBool           aIsUniqueIndex )
{
    smnbHeader * sIndexHeader  = (smnbHeader *)(aIndex->mHeader);
    SInt         sSlotPos      = -1;
    void*        sLeftNode     = NULL;
    smnbNode*    sNewNodeList  = NULL;
    UInt         sNewNodeCount = 0;
    void       * sNewNode      = NULL;
    smnbINode  * sCurINode     = NULL;
    smnbINode  * sRootINode    = NULL;
    smnbLNode  * sTargetNode   = NULL;
    SInt         sTargetSeq    = -1;
    void       * sSepKeyRow1   = NULL;
    void       * sSepKey1      = NULL;
    idBool       sIsLeafScanLatched     = ID_FALSE;
    idBool       sIsInternalScanLatched = ID_FALSE;

    /* split시 필요한 노드 갯수만큼 미리 할당해둔다. */
    IDE_TEST( prepareNodes( sIndexHeader,
                            aStack,
                            aDepth,
                            &sNewNodeList,
                            &sNewNodeCount )
              != IDE_SUCCESS );

    IDE_ERROR_RAISE( sNewNodeList != NULL, ERR_CORRUPTED_INDEX_NEWNODE );
    IDE_ERROR_RAISE( sNewNodeCount > 0, ERR_CORRUPTED_INDEX_NEWNODE );


    /* PROJ-1617 */
    sIndexHeader->mStmtStat.mNodeSplitCount++;

    getPreparedNode( &sNewNodeList,
                     &sNewNodeCount,
                     &sNewNode );

    IDE_ERROR_RAISE( sNewNode != NULL, ERR_CORRUPTED_INDEX_NEWNODE );

    SMNB_SCAN_LATCH(aCurLNode);
    sIsLeafScanLatched = ID_TRUE;

    IDE_TEST( splitLeafNode( sIndexHeader,
                             aSlotPos,
                             aCurLNode,
                             (smnbLNode*)sNewNode,
                             &sTargetNode,
                             &sTargetSeq )
              != IDE_SUCCESS );

    IDE_TEST( insertIntoLeafNode( sIndexHeader,
                                  sTargetNode,
                                  aRow,
                                  sTargetSeq ) != IDE_SUCCESS );

    getLeafSlot( (SChar **)&sSepKeyRow1,
                 &sSepKey1,
                 aCurLNode,
                 (SShort)( aCurLNode->mSlotCount - 1 ) );

    sIsLeafScanLatched = ID_FALSE;
    SMNB_SCAN_UNLATCH(aCurLNode);

    if ( needToUpdateStat( sIndexHeader->mIsMemTBS ) == ID_TRUE )
    {
        IDE_TEST( updateStat4Insert( aIndex,
                                     sIndexHeader,
                                     sTargetNode,
                                     sTargetSeq,
                                     aRow,
                                     aIsUniqueIndex ) != IDE_SUCCESS );
    }

    sLeftNode = aCurLNode;

    aDepth--;

    while (aDepth >= 0)
    {
        sCurINode = (smnbINode*)(aStack[aDepth].node);

        SMNB_SCAN_LATCH(sCurINode);
        sIsInternalScanLatched = ID_TRUE;

        /* PROJ-2433
         * 새로운 new split으로 child pointer 변경함 */
        sCurINode->mChildPtrs[aStack[aDepth].lstReadPos] = (smnbNode *)sNewNode;

        IDE_TEST( findSlotInNode( sIndexHeader,
                                  (smnbNode *)sCurINode,
                                  sSepKeyRow1,
                                  &sSlotPos ) != IDE_SUCCESS );
        IDE_ERROR_RAISE(sSlotPos == aStack[aDepth].lstReadPos, ERR_CORRUPTED_INDEX_SLOTCOUNT);

        if ( sCurINode->mSlotCount < SMNB_INTERNAL_SLOT_MAX_COUNT( sIndexHeader ) )
        {
            IDE_TEST( insertIntoInternalNode( sIndexHeader,
                                              sCurINode,
                                              sSepKeyRow1,
                                              sSepKey1,
                                              sLeftNode ) != IDE_SUCCESS );

            sIsInternalScanLatched = ID_FALSE;
            SMNB_SCAN_UNLATCH(sCurINode);

            break;
        }

        IDE_ERROR_RAISE( sNewNodeCount > 0, ERR_CORRUPTED_INDEX_NEWNODE );

        getPreparedNode( &sNewNodeList,
                         &sNewNodeCount,
                         &sNewNode );

        IDE_ERROR_RAISE( sNewNode != NULL, ERR_CORRUPTED_INDEX_NEWNODE );

        IDE_TEST( splitInternalNode( sIndexHeader,
                                     sSepKeyRow1,
                                     sSepKey1,
                                     sLeftNode,
                                     sSlotPos,
                                     sCurINode,
                                     (smnbINode*)sNewNode,
                                     &sSepKeyRow1, 
                                     &sSepKey1 ) != IDE_SUCCESS );

        sIsInternalScanLatched = ID_FALSE;
        SMNB_SCAN_UNLATCH(sCurINode);

        sLeftNode = sCurINode;
        aDepth--;
    }

    if ( aDepth < 0 )
    {
        IDE_ERROR_RAISE( sNewNodeCount > 0, ERR_CORRUPTED_INDEX_NEWNODE );

        getPreparedNode( &sNewNodeList,
                         &sNewNodeCount,
                         (void**)&sRootINode );

        IDE_ERROR_RAISE( sRootINode != NULL, ERR_CORRUPTED_INDEX_NEWNODE );

        /* BUG-18292 : V$MEM_BTREE_HEADER 정보 추가 */
        sIndexHeader->nodeCount++;

        initInternalNode( sRootINode,
                          sIndexHeader,
                          IDU_LATCH_UNLOCKED );

        sRootINode->mSlotCount = 2;

        setInternalSlot( sRootINode,
                         0,
                         (smnbNode *)sLeftNode,
                         (SChar *)sSepKeyRow1,
                         sSepKey1 );

        setInternalSlot( sRootINode,
                         1,
                         (smnbNode *)sNewNode,
                         (SChar *)NULL,
                         (void *)NULL );

        IDL_MEM_BARRIER;
        sIndexHeader->root = sRootINode;
    }

    if ( sNewNodeCount > 0 )
    {
        IDE_DASSERT( 0 );
        IDE_TEST( sIndexHeader->mNodePool.releaseSlots( sNewNodeCount,
                                                        (smmSlot*)sNewNodeList,
                                                        SMM_SLOT_LIST_MUTEX_NEEDLESS )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_SLOTCOUNT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Node SlotCount" );
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_NEWNODE )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Cannot Alloc New Node" );
    }
    IDE_EXCEPTION_END;

    if ( sIsLeafScanLatched == ID_TRUE )
    {
        sIsLeafScanLatched = ID_FALSE;
        SMNB_SCAN_UNLATCH( aCurLNode );
    }

    if ( sIsInternalScanLatched == ID_TRUE )
    {
        sIsInternalScanLatched = ID_FALSE;
        SMNB_SCAN_UNLATCH( sCurINode );
    }

    if ( sNewNodeCount > 0 )
    {
        if ( sIndexHeader->mNodePool.releaseSlots( sNewNodeCount,
                                                   (smmSlot*)sNewNodeList,
                                                   SMM_SLOT_LIST_MUTEX_NEEDLESS )
             != IDE_SUCCESS )
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
            ideLog::log( IDE_ERR_0, "[SM] fail to release index slots" );
        }
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::insertRowUnique( idvSQL          * aStatistics,
                                   void            * aTrans,
                                   void            * aTable,
                                   void            * aIndex,
                                   smSCN             aInfiniteSCN,
                                   SChar           * aRow,
                                   SChar           * aNull,
                                   idBool            aUniqueCheck,
                                   smSCN             aStmtViewSCN,
                                   void            * aRowSID,
                                   SChar          ** aExistUniqueRow,
                                   ULong             aInsertWaitTime,
                                   idBool            aForbiddenToRetry )
{
    smnbHeader* sHeader;

    sHeader  = (smnbHeader*)((smnIndexHeader*)aIndex)->mHeader;

    if ( compareRows( &(sHeader->mStmtStat),
                      sHeader->columns,
                      sHeader->fence,
                      aRow,
                      aNull )
         != 0 )
    {
        aNull = NULL;
    }

    IDE_TEST( insertRow( aStatistics,
                         aTrans, 
                         aTable, 
                         aIndex, 
                         aInfiniteSCN,
                         aRow, 
                         aNull, 
                         aUniqueCheck, 
                         aStmtViewSCN, 
                         aRowSID,
                         aExistUniqueRow, 
                         aInsertWaitTime,
                         aForbiddenToRetry )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::insertRow( idvSQL*           aStatistics,
                             void*             a_pTrans,
                             void *            /* aTable */,
                             void*             a_pIndex,
                             smSCN             /* aInfiniteSCN */,
                             SChar*            a_pRow,
                             SChar*            a_pNull,
                             idBool            aUniqueCheck,
                             smSCN             aStmtViewSCN,
                             void*             /*aRowSID*/,
                             SChar**           aExistUniqueRow,
                             ULong             aInsertWaitTime, 
                             idBool            aForbiddenToRetry )

{
    smnbHeader     * s_pIndexHeader;
    SInt             s_nDepth;
    SInt             s_nSlotPos;
    smnbStack        s_stack[SMNB_STACK_DEPTH];
    smnbLNode      * s_pCurLNode        = NULL;
    smnbINode      * s_pCurINode        = NULL;
    smTID            s_transID          = SM_NULL_TID;
    idBool           sIsUniqueIndex;
    idBool           sIsTreeLatched     = ID_FALSE;
    idBool           sIsNodeLatched     = ID_FALSE;
    idBool           sIsRetraverse      = ID_FALSE;
    idBool           sNeedTreeLatch     = ID_FALSE;
    idBool           sIsLeafScanLatched = ID_FALSE;
    smnIndexHeader * sIndex             = ((smnIndexHeader*)a_pIndex);
    SChar          * s_diffRow          = NULL;
    SInt             s_diffSlotPos      = 0;
    smpSlotHeader  * s_diffRowSlot      = NULL;

    s_pIndexHeader = (smnbHeader*)sIndex->mHeader;
    sIsUniqueIndex = (((smnIndexHeader*)a_pIndex)->mFlag & SMI_INDEX_UNIQUE_MASK)
                         == SMI_INDEX_UNIQUE_ENABLE ? ID_TRUE : ID_FALSE;

restart:
    /* BUG-32742 [sm] Create DB fail in Altibase server
     *           Solaris x86에서 DB 생성시 서버 hang 걸림.
     *           원인은 컴파일러 최적화 문제로 보이고, 이를 회피하기 위해
     *           아래와 같이 수정함 */
    if ( sNeedTreeLatch == ID_TRUE )
    {
        lockTree( s_pIndexHeader );
        sIsTreeLatched = ID_TRUE;
        sNeedTreeLatch = ID_FALSE;
    }

    s_nDepth = -1;

    //check version and retraverse.
    IDE_TEST( findPosition( s_pIndexHeader,
                            NULL, /* smiCallBack */
                            a_pRow,
                            &s_nDepth,
                            s_stack,
                            &s_pCurLNode ) != IDE_SUCCESS );

    if ( s_nDepth < 0 )
    {
        /* root node가 없는 경우. Tree latch잡고, 루트 노드 생성한다. */
        if ( sIsTreeLatched == ID_FALSE )
        {
            lockTree( s_pIndexHeader );
            sIsTreeLatched = ID_TRUE;
        }

        if ( s_pIndexHeader->root == NULL )
        {
            // Tree is empty...
            IDE_TEST( makeRootNode( sIndex,
                                    a_pRow,
                                    sIsUniqueIndex )
                      != IDE_SUCCESS );
        }
        else
        {
            sIsTreeLatched = ID_FALSE;
            unlockTree( s_pIndexHeader );

            goto restart;
        }
    }
    else
    {
        IDE_ERROR_RAISE( SMNB_IS_LEAF_NODE( s_pCurLNode ), 
                         ERR_CORRUPTED_INDEX_FLAG );

        IDU_FIT_POINT( "smnbBTree::insertRow::beforeLock" );

        lockNode( s_pCurLNode );
        sIsNodeLatched = ID_TRUE;

        IDU_FIT_POINT( "smnbBTree::insertRow::afterLock" );

        /* 빈 노드에 들어왔다.
         * lock 잡기 전에 이미 모두 freeSlot되었을 수 있다. */
        if ( ( s_pCurLNode->mSlotCount == 0 ) || 
             ( ( s_pCurLNode->flag & SMNB_NODE_VALID_MASK ) == SMNB_NODE_INVALID ) )
        {
            IDE_ERROR_RAISE( sIsTreeLatched == ID_FALSE, ERR_NOT_TREE_UNLATCH );

            sIsNodeLatched = ID_FALSE;
            unlockNode( s_pCurLNode );

            goto restart;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( findSlotInNode( s_pIndexHeader,
                                  (smnbNode *)s_pCurLNode,
                                  a_pRow,
                                  &s_nSlotPos ) != IDE_SUCCESS );

        if ( s_pCurLNode->mSlotCount > s_nSlotPos )
        {
            if ( s_pCurLNode->mRowPtrs[s_nSlotPos] == a_pRow )
            {
                IDE_CONT( skip_insert_row );
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* 삽입할 leaf node를 찾고 leaf node에서 삽입 위치를 찾기 전에
             * split이 발생하게 되면, next node에 삽입해야 하지만
             * 제일 마지막 slot에 삽입될 수 있다.
             * 현 상태로는 next node 어디에 삽입해야 하는지 알 수 없으므로
             * 다시 시도 한다. */
            if ( s_pCurLNode->nextSPtr != NULL )
            {
                IDE_ERROR_RAISE( sIsTreeLatched == ID_FALSE, ERR_NOT_TREE_UNLATCH );

                sIsNodeLatched = ID_FALSE;
                unlockNode( s_pCurLNode );

                goto restart;
            }
        }

        if ( ( a_pNull == NULL ) && ( aUniqueCheck == ID_TRUE ) )
        {
            IDE_TEST( checkUniqueness( sIndex,
                                       a_pTrans,
                                       &(s_pIndexHeader->mStmtStat),
                                       s_pIndexHeader->columns,
                                       s_pIndexHeader->fence,
                                       s_pCurLNode,
                                       s_nSlotPos,
                                       a_pRow,
                                       aStmtViewSCN,
                                       sIsTreeLatched,
                                       &s_transID,
                                       &sIsRetraverse,
                                       aExistUniqueRow,
                                       &s_diffRow,
                                       &s_diffSlotPos,
                                       aForbiddenToRetry )
                      != IDE_SUCCESS );

            if ( sIsRetraverse == ID_TRUE )
            {
                IDE_ERROR_RAISE( sIsTreeLatched == ID_FALSE, ERR_NOT_TREE_UNLATCH );

                sNeedTreeLatch = ID_TRUE;

                sIsNodeLatched = ID_FALSE;
                unlockNode( s_pCurLNode );

                goto restart;
            }

            // PROJ-1553 Replication self-deadlock
            // s_transID가 0이 아니면 wait해야 하지만
            // 현재 transaction과 sTransID의 transaction이
            // 모두 replication tx이고, 같은 ID이면
            // 대기하지 않고 그냥 pass한다.
            if ( s_transID != SM_NULL_TID )
            {
                /* TASK-6548 duplicated unique key */
                IDE_TEST_RAISE( ( smxTransMgr::isReplTrans( a_pTrans ) == ID_TRUE ) &&
                                ( smxTransMgr::isSameReplID( a_pTrans, s_transID ) == ID_TRUE ) &&
                                ( smxTransMgr::isTransForReplConflictResolution( s_transID ) == ID_TRUE ),
                                ERR_UNIQUE_VIOLATION_REPL );

                if ( smLayerCallback::isWaitForTransCase( a_pTrans,
                                                          s_transID )
                     == ID_TRUE )
                {
                    IDE_RAISE( ERR_WAIT );
                }
                else
                {
                    /* Nothing to do */
                }
            }
            else
            {
                /* Nothing to do */
            }
        }

        if ( s_pCurLNode->mSlotCount < SMNB_LEAF_SLOT_MAX_COUNT( s_pIndexHeader ) )
        {
            SMNB_SCAN_LATCH(s_pCurLNode);
            sIsLeafScanLatched = ID_TRUE;

            IDE_ERROR_RAISE( s_pCurLNode->mSlotCount != 0, ERR_CORRUPTED_INDEX_SLOTCOUNT );

            IDE_TEST( insertIntoLeafNode( s_pIndexHeader,
                                s_pCurLNode,
                                a_pRow,
                                s_nSlotPos ) != IDE_SUCCESS );

            sIsLeafScanLatched = ID_FALSE;
            SMNB_SCAN_UNLATCH(s_pCurLNode);

            if ( needToUpdateStat( s_pIndexHeader->mIsMemTBS ) == ID_TRUE )
            {
                IDE_TEST( updateStat4Insert( sIndex,
                                             s_pIndexHeader,
                                             s_pCurLNode,
                                             s_nSlotPos,
                                             a_pRow,
                                             sIsUniqueIndex) != IDE_SUCCESS );
            }
            else
            {
                /* nothing to do ... */
            }

        }
        else
        {
            /* split/key redistribution이 필요.. Tree latch를 안잡았으면 잡고 다시 시도. */
            if ( sIsTreeLatched == ID_FALSE )
            {
                sNeedTreeLatch = ID_TRUE;

                sIsNodeLatched = ID_FALSE;
                unlockNode( s_pCurLNode );

                goto restart;
            }

            /* PROJ-2613 Key Redistibution in MRDB Index */
            /* checkEnableKeyRedistribution의 마지막 인자로 s_pCurINode를 넣을 경우
               compiler의 optimizing에 의해 code reordering이 발생하여
               s_pCurINode가 값을 받기전 checkEnableKeyRedistribution을 호출하여 죽는 경우가 발생한다.
               이를 막기 위해 checkEnableKeyRedistribution에는 직접 stack에서 뽑은 값을 넘긴다. */

            /* BUG-42899 split 대상 노드가 최상위 노드일 경우 stack에서 부모노드를 가져올 수 없다. */
            if ( s_nDepth > 0 )
            {
                s_pCurINode = (smnbINode*)(s_stack[s_nDepth - 1].node);
            }
            else
            {
                /* 해당 노드가 최상위 노드일 경우 키 재분배를 수행할 수 없다. */
                s_pCurINode = NULL;
            }

            if ( ( s_pCurINode != NULL ) && 
                 ( checkEnableKeyRedistribution( s_pIndexHeader, 
                                                 s_pCurLNode,
                                                 (smnbINode*)(s_stack[s_nDepth - 1].node) ) 
                   == ID_TRUE ) )
            {
                /* 키 재분배를 수행한다. */
                IDE_TEST( processKeyRedistribution( sIndex, 
                                                    s_pCurINode,
                                                    s_pCurLNode,
                                                    &sIsTreeLatched,
                                                    &sIsNodeLatched )
                          != IDE_SUCCESS );
                
                goto restart;
            }
            else
            {
                /* split을 수행한다. */
                IDE_TEST( processNodeSplit( sIndex,
                                            s_stack,
                                            s_nDepth,
                                            s_pCurLNode,
                                            a_pRow,
                                            s_nSlotPos,
                                            sIsUniqueIndex )
                          != IDE_SUCCESS );
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_insert_row );

    if ( sIsNodeLatched == ID_TRUE )
    {
        sIsNodeLatched = ID_FALSE;
        unlockNode( s_pCurLNode );
    }
    if ( sIsTreeLatched == ID_TRUE )
    {
        sIsTreeLatched = ID_FALSE;
        unlockTree( s_pIndexHeader );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_WAIT );

    if ( sIsNodeLatched == ID_TRUE )
    {
        sIsNodeLatched = ID_FALSE;
        unlockNode( s_pCurLNode );
    }
    if ( sIsTreeLatched == ID_TRUE )
    {
        sIsTreeLatched = ID_FALSE;
        unlockTree( s_pIndexHeader );
    }

    /* BUG-38198 Session이 아직 살아있는지 체크하여
     * timeout등으로 사라졌다면 해당 정보를 dump후 fail 처리한다. */
    IDE_TEST_RAISE( iduCheckSessionEvent( aStatistics ) != IDE_SUCCESS, 
                    err_SESSIONEND );

    IDE_TEST( smLayerCallback::waitForTrans( a_pTrans,    /* aTrans        */ 
                                             s_transID,   /* aWaitTransID  */  
                                             s_pIndexHeader->mSpaceID,
                                             aInsertWaitTime /* aLockWaitTime */)
              != IDE_SUCCESS);

    goto restart;

    IDE_EXCEPTION( err_SESSIONEND );
    {
        s_diffRowSlot = (smpSlotHeader*)s_diffRow;

        ideLog::logMem( IDE_DUMP_0,
                        (UChar*)SMP_GET_PERS_PAGE_HEADER( s_diffRow ),
                        SM_PAGE_SIZE,
                        "[ Index Insert Timeout Infomation]\n"
                        "PageID     :%"ID_UINT32_FMT"\n"
                        "TID        :%"ID_UINT32_FMT"\n"
                        "Slot Number:%"ID_UINT32_FMT"\n"
                        "Slot Offset:%"ID_UINT64_FMT"\n"
                        "CreateSCN  :%"ID_UINT64_FMT"\n",
                        SMP_SLOT_GET_PID( s_diffRowSlot ),
                        s_transID, 
                        s_diffSlotPos,
                        (ULong)SMP_SLOT_GET_OFFSET( s_diffRowSlot ),
                        SM_SCN_TO_LONG( s_diffRowSlot->mCreateSCN ) ); 
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_FLAG )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Flag" );
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_SLOTCOUNT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Index Node SlotCount is Zero" );
    }
    IDE_EXCEPTION( ERR_NOT_TREE_UNLATCH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] Tree Latch is not unlatched." );
    }
    IDE_EXCEPTION( ERR_UNIQUE_VIOLATION_REPL )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_smnUniqueViolationInReplTrans ) );
    }
    IDE_EXCEPTION_END;

    if ( sIsLeafScanLatched == ID_TRUE )
    {
        sIsLeafScanLatched = ID_FALSE;
        SMNB_SCAN_UNLATCH( s_pCurLNode );
    }

    if (sIsNodeLatched == ID_TRUE)
    {
        sIsNodeLatched = ID_FALSE;
        unlockNode( s_pCurLNode );
    }
    if ( sIsTreeLatched == ID_TRUE )
    {
        sIsTreeLatched = ID_FALSE;
        unlockTree( s_pIndexHeader );
    }

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( sIndex );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::findFirstSlotInNode             *
 * ------------------------------------------------------------------*
 * NODE 내에서 가장 첫번째나오는 row의 위치를 찾는다.
 * ( 동일한 key가 여러인경우 가장 왼쪽(첫번째)의 slot 위치를 찾음 )
 *
 * case1. 일반 index이거나, partial key index가 아닌경우
 *   case1-1. direct key index인 경우
 *            : 함수 findFirstKeyInNode() 실행해 direct key 기반으로 검색
 *   case1-2. 일반 index인 경우
 *            : 함수 findFirstRowInNode() 실행해 row 기반으로 검색
 *
 * case2. partial key index인경우
 *        : 함수 findFirstKeyInNode() 실행해 direct key 기반으로 검색후
 *          함수 callback() 또는 findFirstRowInNode() 실행해 재검색
 *
 *  => partial key 에서 재검색이 필요한 이유
 *    1. partial key로 검색한 위치는 정확한 위치가 아닐수있기때문에
 *       full key로 재검색을 하여 확인하여야 한다
 *
 * aHeader   - [IN]  INDEX 헤더
 * aCallBack - [IN]  Range Callback
 * aNode     - [IN]  INDEX NODE
 * aMinimum  - [IN]  검색할 slot범위 최소값
 * aMaximum  - [IN]  검색할 slot범위 최대값
 * aSlot     - [OUT] 찾아진 slot 위치
 *********************************************************************/
inline void smnbBTree::findFirstSlotInNode( smnbHeader          * aHeader,
                                            const smiCallBack   * aCallBack,
                                            const smnbNode      * aNode,
                                            SInt                * aSlot )
{
    idBool    sResult;
    SChar   * sRow      = NULL;
    SInt      sMinimum  = 0;
    SInt      sMaximum  = 0;

    sMaximum = aNode->mSlotCount - 1;

    if ( aHeader->mIsPartialKey == ID_FALSE )
    {
        /* full direct key */
        if ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE )
        {
            /* 복합키인경우 첫컬럼은key로 뒷컬럼들은 row로처리한다 */
            findFirstKeyInNode( aHeader,
                                aCallBack,
                                aNode,
                                sMinimum,
                                sMaximum,
                                aSlot );
            IDE_CONT( end );
        }
        /* direct key 없음 */
        else
        {
            findFirstRowInNode( aCallBack,
                                aNode,
                                sMinimum,
                                sMaximum,
                                aSlot );
            IDE_CONT( end );
        }
    }
    /* partial key */
    else
    {
        findFirstKeyInNode( aHeader,
                            aCallBack,
                            aNode,
                            sMinimum,
                            sMaximum,
                            aSlot );

        if ( *aSlot > sMaximum )
        {
            /* 범위내에없다. */
            IDE_CONT( end );
        }
        else
        {
            sRow = aNode->mRowPtrs[*aSlot];

            if ( sRow == NULL )
            {
                /* 범위내에없다. */
                IDE_CONT( end );
            }
        }

        if ( isFullKey( aHeader,
                        aNode,
                        (SShort)(*aSlot) ) == ID_TRUE )
        {
            aCallBack->callback( &sResult,
                                 sRow,
                                 SMNB_GET_KEY_PTR( aNode, *aSlot ),
                                 0,
                                 SC_NULL_GRID,
                                 aCallBack->data );
        }
        else
        {
            aCallBack->callback( &sResult,
                                 sRow,
                                 NULL,
                                 0,
                                 SC_NULL_GRID,
                                 aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            IDE_CONT( end );
        }
        else
        {
            sMinimum = *aSlot + 1; /* *aSlot은 찾는 slot이 아닌것을 확인했다. skip 한다. */

            if ( sMinimum <= sMaximum )
            {
                findFirstRowInNode( aCallBack,
                                    aNode,
                                    sMinimum,
                                    sMaximum,
                                    aSlot );
            }
            else
            {
                /* PROJ-2433
                 * node 범위내에 만족하는값이없다. */
                *aSlot = sMaximum + 1;
            }

            IDE_CONT( end );
        }
    }

    IDE_EXCEPTION_CONT( end );

    return;
}

void smnbBTree::findFirstRowInNode( const smiCallBack   * aCallBack,
                                    const smnbNode      * aNode,
                                    SInt                  aMinimum,
                                    SInt                  aMaximum,
                                    SInt                * aSlot )
{
    SInt      sMedium   = 0;
    idBool    sResult;
    SChar   * sRow      = NULL;

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;
        sRow    = aNode->mRowPtrs[sMedium];

        if ( sRow == NULL )
        {
            sResult = ID_TRUE;
        }
        else
        {
            // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
            IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sRow)->mCreateSCN) );

            (void)aCallBack->callback( &sResult,
                                       sRow,
                                       NULL,
                                       0,
                                       SC_NULL_GRID,
                                       aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            aMaximum = sMedium - 1;
            *aSlot   = (UInt)sMedium;
        }
        else
        {
            aMinimum = sMedium + 1;
            *aSlot   = (UInt)aMinimum;
        }
    }
    while (aMinimum <= aMaximum);
}

void smnbBTree::findFirstKeyInNode( smnbHeader          * aHeader,
                                    const smiCallBack   * aCallBack,
                                    const smnbNode      * aNode,
                                    SInt                  aMinimum,
                                    SInt                  aMaximum,
                                    SInt                * aSlot )
{
    SInt      sMedium           = 0;
    idBool    sResult;
    SChar   * sRow              = NULL;
    UInt      sPartialKeySize   = 0;

    if ( aHeader->mIsPartialKey == ID_TRUE )
    {
        sPartialKeySize = aNode->mKeySize;
    }

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;

        sRow = aNode->mRowPtrs[sMedium];

        if ( sRow == NULL )
        {
            sResult = ID_TRUE;
        }
        else
        {
            (void)aCallBack->callback( &sResult,
                                       sRow, /* composite key index의 첫번째이후 컬럼을 비교하기위해서*/
                                       SMNB_GET_KEY_PTR( aNode, sMedium ),
                                       sPartialKeySize,
                                       SC_NULL_GRID,
                                       aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            aMaximum = sMedium - 1;
            *aSlot   = (UInt)sMedium;
        }
        else
        {
            aMinimum = sMedium + 1;
            *aSlot   = (UInt)aMinimum;
        }
    }
    while (aMinimum <= aMaximum);
}

/* BUG-26625 [SN] 메모리 인덱스 스캔시 SMO를 검출하는 로직에 문제가 있습니다.
 *
 * findPosition 경우의 수 정리
 *
 * 1) slotCount == 0
 *      해당 노드를 탐색하기 직전 노드가 삭제됨
 *      1.1) Stack up, 상위 노드 재탐색
 *          (Stack이 비었으면 Root부터 다시)
 *      1.2) 1.1의 경우이지만 재탐색할 노드가 없음
 *          탐색할 대상이 삭제된 경우이기 때문에 결과가 사라진 상태.
 *          즉 탐색할 필요가 없음 -> 못 찾음, LNode = NULL
 *
 *  2) slotPos == slotCount
 *      해당 노드를 탐색하기 직전 SMO가 일어나 목표 키가 이웃 노드에 있음
 *      2.1) 이웃 노드가 있음
 *           정상적으로 이웃 노드를 또 찾아들어감. -> 이웃노드 재탐색
 *      2.2) 이웃 노드가 없음
 *           상위 노드부터 재탐색. (1.1과 같음)
 */
IDE_RC smnbBTree::findPosition( smnbHeader         * aIndexHeader,
                                const smiCallBack  * aCallBack,
                                SChar              * aRow,
                                SInt               * aDepth,
                                smnbStack          * aStack,
                                smnbLNode         ** aLNode )
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    SInt                 sSlotCount;
    SInt                 sDepth      = -1;
    SInt                 sSlotPos;
    volatile smnbINode * sCurINode   = (smnbINode*)aIndexHeader->root;
    volatile smnbINode * sChildINode;
    volatile smnbINode * sCurSINode;
    IDU_LATCH            sVersion;
    IDU_LATCH            sNewVersion = IDU_LATCH_UNLOCKED;

    if ( sCurINode != NULL )
    {
        while ( SMNB_IS_INTERNAL_NODE( sCurINode ) )
        {
            sVersion     = getLatchValueOfINode(sCurINode) & (~SMNB_SCAN_LATCH_BIT);
            IDL_MEM_BARRIER;

            while(1)
            {
                sSlotCount  = sCurINode->mSlotCount;
                sCurSINode  = sCurINode->nextSPtr;

                if ( sSlotCount != 0 )
                {

                    /* BUG-47206 : aCallBack, aRow 중 하나만 사용함.
                                   beforFirstInternal() 에서 호출시는 aCallBack 을 이용해 range minimum 값을 찾고,
                                   insertKey(), freeSlot() 등 에서 호출시는 aRow 값의 위치를 찾는다. */
                    if ( aCallBack != NULL )
                    {
                        IDE_DASSERT( aRow == NULL );

                        findFirstSlotInNode( aIndexHeader,
                                             aCallBack,
                                             (smnbNode *)sCurINode,
                                             &sSlotPos );
                    }
                    else
                    {
                        IDE_DASSERT( aRow != NULL );

                        IDE_TEST( findSlotInNode( (smnbHeader *)aIndexHeader,
                                                  (smnbNode *)sCurINode,
                                                  aRow,
                                                  &sSlotPos ) != IDE_SUCCESS );
                    }

                    sChildINode = (smnbINode*)(sCurINode->mChildPtrs[sSlotPos]);
                }
                else
                {
                    sChildINode = NULL;
                }

                sNewVersion = getLatchValueOfINode(sCurINode);

                if ( sVersion == sNewVersion )
                {
                    break;
                }
                else
                {
                    sVersion = sNewVersion & (~SMNB_SCAN_LATCH_BIT);
                }

                IDL_MEM_BARRIER;
            }

            if ( sSlotCount == 0 ) /* Case 1 */
            {
                if ( sDepth == -1 ) /* Case 1.1 */
                {
                    sCurINode = (smnbINode*)aIndexHeader->root;
                }
                else
                {
                    sDepth--;
                    aStack--;
                    sCurINode = (smnbINode*)(aStack->node);
                }

                if ( sCurINode == NULL ) /* Case 1.2 */
                {
                    sDepth = -1;
                    break;
                }

                continue;
            }

            if ( sSlotCount == sSlotPos ) /* Case 2 */
            {
                sCurINode = sCurSINode;

                if ( sCurINode == NULL ) /* Case 2.2 */
                {
                    if ( sDepth == -1 )
                    {
                        sCurINode = (smnbINode*)aIndexHeader->root;
                    }
                    else
                    {
                        sDepth--;
                        aStack--;
                        sCurINode = (smnbINode*)(aStack->node);
                    }
                }
                else
                {
                    /* Case 2. 1 */
                }

                continue;
            }

            aStack->node       = (smnbINode*)sCurINode;
            aStack->lstReadPos = sSlotPos;
            aStack->slotCount  = sSlotCount;

            sDepth++;
            aStack++;

            sCurINode   = sChildINode;

        } /* INTERNAL NODE LOOP */

        IDL_MEM_BARRIER;

        if ( sCurINode != NULL )
        {
            aStack->node = (smnbINode*)sCurINode;
            sDepth++;
        }
    }

    *aDepth = sDepth;
    *aLNode = (smnbLNode*)sCurINode;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::updateStat4Delete( smnIndexHeader  * aPersIndex,
                                     smnbHeader      * aIndex,
                                     smnbLNode       * a_pLeafNode,
                                     SChar           * a_pRow,
                                     SInt            * a_pSlotPos,
                                     idBool            aIsUniqueIndex )
{
    SInt               s_nSlotPos;
    SChar            * sPrevRowPtr;
    SChar            * sNextRowPtr;
    smnbColumn       * sFirstColumn = &aIndex->columns[0];  // To fix BUG-26469
    SInt               sCompareResult;
    SChar            * sStatRow;
    SLong            * sNumDistPtr;

    s_nSlotPos = *a_pSlotPos;

    IDE_TEST_CONT( ( ( s_nSlotPos == a_pLeafNode->mSlotCount ) ||
                     ( a_pLeafNode->mRowPtrs[s_nSlotPos] != a_pRow ) ),
                   skip_update_stat );

    IDE_TEST_CONT( ( smnbBTree::isNullKey( aIndex, a_pRow ) == ID_TRUE ),
                   skip_update_stat );

    // get next slot
    if ( s_nSlotPos == ( a_pLeafNode->mSlotCount - 1 ) )
    {
        // BUG-47554 memory index 에서 latch 관리를 IDL_MEM_BARRIER 가 아닌 atomic 함수로 변경합니다.
        // 갱신 할 Key의 위치가 Leaf Node의 마지막 Slot일 때,
        // Next Node의 첫번째 Key의 Row Pointer를 가져온다.
        // 자주 호출 되지 않고, 최적화로 인한 문제가 있어서
        // 별도의 함수로 추출한다.
        sNextRowPtr = getNxtLNodeFstRow( a_pLeafNode );
    }
    else
    {
        IDE_DASSERT( ( s_nSlotPos + 1 ) >= 0 );
        sNextRowPtr = a_pLeafNode->mRowPtrs[s_nSlotPos + 1];
    }

    // get prev slot
    if ( s_nSlotPos == 0 )
    {
        // BUG-47554 memory index 에서 latch 관리를 IDL_MEM_BARRIER 가 아닌 atomic 함수로 변경합니다.
        // 갱신 할 Key의 위치가 Leaf Node의 첫번째 Slot일 때,
        // Prev Node의 마지막 Key의 Row Pointer를 가져온다.
        // 자주 호출 되지 않고, 최적화로 인한 문제가 있어서
        // 별도의 함수로 추출한다.
        sPrevRowPtr = getPrvLNodeLstRow( a_pLeafNode );
    }
    else
    {
        sPrevRowPtr = a_pLeafNode->mRowPtrs[s_nSlotPos - 1];
    }

    // update index statistics

    // To fix BUG-26469
    // sColumn[0] 을 사용하면 안된다.
    // sColumn 은 전체 키컬럼이 NULL 인지 체크하는 로직에서 position 이 옮겨졌다.
    // 그래서 sColumn[0] 은 실제 0번째가 아닌 옮겨진 position 이다.
    if ( isNullColumn( sFirstColumn,
                       &(sFirstColumn->column),
                       a_pRow ) != ID_TRUE )
    {
        if ( (a_pLeafNode->prevSPtr == NULL) && (s_nSlotPos == 0) )
        {
            if ( ( sNextRowPtr == NULL ) ||
                 ( isNullColumn( sFirstColumn,
                                 &(sFirstColumn->column),
                                 sNextRowPtr ) == ID_TRUE ) )
            {
                /* setMinMaxStat 에서 Row가 NULL이면 통계정보 초기화함*/
                sStatRow = NULL;
            }
            else
            {
                sStatRow = sNextRowPtr;
            }

            IDE_TEST( setMinMaxStat( aPersIndex,
                                     sStatRow,
                                     sFirstColumn,
                                     ID_TRUE ) != IDE_SUCCESS ); // aIsMinStat
        }

        if ( ( sNextRowPtr == NULL ) ||
             ( isNullColumn( sFirstColumn,
                             &(sFirstColumn->column),
                             sNextRowPtr ) == ID_TRUE ) )
        {
            if ( sPrevRowPtr == NULL )
            {
                /* setMinMaxStat 에서 Row가 NULL이면 통계정보 초기화함*/
                sStatRow = NULL;
            }
            else
            {
                sStatRow = sPrevRowPtr;
            }

            IDE_TEST( setMinMaxStat( aPersIndex,
                                     sStatRow,
                                     sFirstColumn,
                                     ID_FALSE ) != IDE_SUCCESS ); // aIsMinStat
        }
    }

    /* NumDist */
    sNumDistPtr = &aPersIndex->mStat.mNumDist;

    while(1)
    {
        if ( ( aIsUniqueIndex == ID_TRUE ) && ( aIndex->mIsNotNull == ID_TRUE ) )
        {
            idCore::acpAtomicDec64( (void*) sNumDistPtr );
            break;
        }
        else
        {
            /* nothing to do */
        }

        if ( sPrevRowPtr != NULL )
        {
            sCompareResult = compareRows( &(aIndex->mAgerStat),
                                          aIndex->columns,
                                          aIndex->fence,
                                          a_pRow,
                                          sPrevRowPtr ) ;
            if ( sCompareResult == 0 )
            {
                break;
            }
            else
            {
                IDE_ERROR_RAISE( sCompareResult > 0, ERR_CORRUPTED_INDEX );
            }
        }
        else
        {
            /* nothing to do */
        }

        if ( sNextRowPtr != NULL )
        {
            sCompareResult = compareRows( &(aIndex->mAgerStat),
                                          aIndex->columns,
                                          aIndex->fence,
                                          a_pRow,
                                          sNextRowPtr );

            if ( sCompareResult == 0 )
            {
                break;
            }
            else
            {
                IDE_ERROR_RAISE( sCompareResult < 0, ERR_CORRUPTED_INDEX );
            }
        }
        else
        {
            /* nothing to do */
        }

        /* BUG-30074 - disk table의 unique index에서 NULL key 삭제 시/
         *             non-unique index에서 deleted key 추가 시 Cardinality의
         *             정확성이 많이 떨어집니다.
         *
         * Null Key의 경우 NumDist를 갱신하지 않도록 한다.
         * Memory index의 NumDist도 동일한 정책으로 변경한다. */
        if ( smnbBTree::isNullColumn( &(aIndex->columns[0]),
                                      &(aIndex->columns[0].column),
                                      a_pRow ) != ID_TRUE )
        {
            idCore::acpAtomicDec64( (void*) sNumDistPtr );
        }
        else
        {
            /* nothing to do */
        }
        break;
    }

    IDE_EXCEPTION_CONT( skip_update_stat );

    /* BUG-32943 [sm-disk-index] After executing DELETE ROW, the KeyCount
     * of DRDB Index is not decreased  */
    idCore::acpAtomicDec64( (void*)&aPersIndex->mStat.mKeyCount );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        smnbBTree::logIndexHeader( aIndex->mIndexHeader );

        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Order" );
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

void smnbBTree::deleteSlotInLeafNode( smnbLNode     * a_pLeafNode,
                                      SChar         * a_pRow,
                                      SInt          * a_pSlotPos )
{
    SInt  s_nSlotPos;

    s_nSlotPos  = *a_pSlotPos;
    *a_pSlotPos = -1;

    // To fix PR-14107
    if ( ( s_nSlotPos != a_pLeafNode->mSlotCount ) &&
         ( a_pLeafNode->mRowPtrs[s_nSlotPos] == a_pRow ) )
    {
        //delete slot
        /* PROJ-2433 */
        if ( ( s_nSlotPos + 1 ) <= ( a_pLeafNode->mSlotCount - 1 ) )
        {
            shiftLeafSlots( a_pLeafNode,
                            (SShort)( s_nSlotPos + 1 ),
                            (SShort)( a_pLeafNode->mSlotCount - 1 ),
                            (SShort)(-1) );
        }
        else
        {
            /* nothing to do */
        }

        // BUG-29582: leaf node에서 마지막 slot이 delete될때, 지워진 slot의
        // row pointer를 NULL로 초기화한다.
        setLeafSlot( a_pLeafNode,
                     (SShort)( a_pLeafNode->mSlotCount - 1 ),
                     NULL,
                     NULL );

        (a_pLeafNode->mSlotCount)--;

        *a_pSlotPos = s_nSlotPos;
    }
    else
    {
        /* nothing to do */
    }
}

IDE_RC smnbBTree::deleteSlotInInternalNode(smnbINode*        a_pInternalNode,
                                           SInt              a_nSlotPos)
{
    // BUG-26941 CodeSonar::Type Underrun
    IDE_ERROR_RAISE( a_nSlotPos >= 0, ERR_CORRUPTED_INDEX );

    if ( a_pInternalNode->mSlotCount > 1 )
    {
        if ( a_pInternalNode->mRowPtrs[a_nSlotPos] == NULL )
        {
            // BUG-27456 Klocwork SM(4)
            IDE_ERROR_RAISE( a_nSlotPos != 0, ERR_CORRUPTED_INDEX );

            /*
             * PROJ-2433
             * child pointer는 그대로 유지하고
             * row pointer, direct key 삭제
             */
            setInternalSlot( a_pInternalNode,
                             (SShort)( a_nSlotPos - 1 ),
                             a_pInternalNode->mChildPtrs[a_nSlotPos - 1],
                             NULL,
                             NULL );
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    /* PROJ-2433 */
    if ( ( a_nSlotPos + 1 ) <= ( a_pInternalNode->mSlotCount - 1 ) )
    {
        shiftInternalSlots( a_pInternalNode,
                            (SShort)( a_nSlotPos + 1 ),
                            (SShort)( a_pInternalNode->mSlotCount - 1 ),
                            (SShort)(-1) );
    }
    else
    {
        /* nothing to do */
    }

    // BUG-29582: internal node에서 마지막 slot이 delete될때, 지워진 slot의
    // row pointer를 NULL로 초기화한다.
    setInternalSlot( a_pInternalNode,
                     (SShort)( a_pInternalNode->mSlotCount - 1 ),
                     NULL,
                     NULL,
                     NULL );

    (a_pInternalNode->mSlotCount)--;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Slot Position" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

void smnbBTree::merge(smnbINode* a_pLeftNode,
                      smnbINode* a_pRightNode)
{
    copyInternalSlots( a_pLeftNode,
                       a_pLeftNode->mSlotCount,
                       a_pRightNode,
                       0,
                       (SShort)( a_pRightNode->mSlotCount - 1 ) );
}

IDE_RC smnbBTree::deleteNA( idvSQL          * /* aStatistics */,
                            void            * /* a_pTrans */,
                            void            * /* a_pIndex */,
                            SChar           * /* a_pRow */,
                            smiIterator     * /* aIterator */,
                            sdSID             /* aRowSID */ )
{
    // memory b tree는 row의 delete시에 index key에 delete mark를
    // 하지 않는다.

    return IDE_SUCCESS;
}

IDE_RC smnbBTree::freeSlot( void            * a_pIndex,
                            SChar           * a_pRow,
                            idBool            aIgnoreNotFoundKey,
                            idBool          * aIsExistFreeKey )

{
    smnbHeader*       s_pIndexHeader;
    SInt              s_nDepth;
    smnbLNode*        s_pCurLeafNode = NULL;
    smnbINode*        s_pCurInternalNode = NULL;
    smnbNode*         s_pFreeNode = NULL;
    SInt              s_nSlotPos;
    smnbStack         s_stack[SMNB_STACK_DEPTH];
    void*             s_pDeleteRow = NULL;
    SChar           * s_pInsertRow = NULL;
    void            * sInsertKey   = NULL;
    idBool            sIsUniqueIndex;
    idBool            sIsTreeLatched = ID_FALSE;
    idBool            sNeedTreeLatch = ID_FALSE;
    smnbLNode       * sLatchedNode[3];   /* prev/curr/next 최대 3개 잡힐수있다 */
    UInt              sLatchedNodeCount = 0;
    UInt              sDeleteNodeCount = 0;
    smOID             sRowOID;

    IDE_DASSERT( a_pIndex        != NULL );
    IDE_DASSERT( a_pRow          != NULL );
    IDE_DASSERT( aIsExistFreeKey != NULL );

    *aIsExistFreeKey = ID_TRUE;

    s_pIndexHeader  = (smnbHeader*)((smnIndexHeader*)a_pIndex)->mHeader;
    sIsUniqueIndex = (((smnIndexHeader*)a_pIndex)->mFlag&SMI_INDEX_UNIQUE_MASK)
        == SMI_INDEX_UNIQUE_ENABLE ? ID_TRUE : ID_FALSE;

/* freeSlotByLeafFullScan() 함수 테스트용 코드, 발견하면 free slot하고
 * 발견하지 못하거나 혹은 node count 1 혹은 last key이면 기존 코드를 탐색한다. */
#if 0
    if ( aIgnoreNotFoundKey == ID_FALSE )
    {
        if( freeSlotByLeafFullScan( s_pIndexHeader,
                                    a_pRow ) == IDE_SUCCESS )
        {
             IDE_CONT( SKIP_FREE_SLOT );
        }
    }
#endif

restart:
    /* BUG-32742 [sm] Create DB fail in Altibase server
     *           Solaris x86에서 DB 생성시 서버 hang 걸림.
     *           원인은 컴파일러 최적화 문제로 보이고, 이를 회피하기 위해
     *           아래와 같이 수정함 */
    if ( sNeedTreeLatch == ID_TRUE )
    {
        IDU_FIT_POINT( "smnbBTree::freeSlot::beforeLockTree" );

        lockTree( s_pIndexHeader );
        sIsTreeLatched = ID_TRUE;
        sNeedTreeLatch = ID_FALSE;

        IDU_FIT_POINT( "smnbBTree::freeSlot::afterLockTree" );
    }

    s_nDepth = -1;

    IDE_TEST( findPosition( s_pIndexHeader,
                            NULL, /* smiCallBack */
                            a_pRow,
                            &s_nDepth,
                            s_stack,
                            &s_pCurLeafNode ) != IDE_SUCCESS );

    /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
     * index aging.
     * BUG-32984 [SM] during rollback after updateInplace in memory index,
     *                it attempts to delete a key that does not exist.
     * updateInplace rollback인 경우 키가 존재하지 않을 수 있다. */
    if ( s_nDepth == -1 )
    {
        *aIsExistFreeKey = ID_FALSE;

        if ( aIgnoreNotFoundKey == ID_FALSE )
        {
            if ( sIsTreeLatched == ID_TRUE )
            {
                sIsTreeLatched = ID_FALSE;
                unlockTree(s_pIndexHeader) ;
            }
            /* BUG-47618 key를 반드시 발견해야 하는 상황에서 발견하지 못하였을 때
             * leaf node를 fullscan 해서 row pointer를 찾습니다.
             * 예외 상황에서만 호출되는 함수로, 발견하면 통과하고,
             * row를 찾아서 delete 하지 못할 경우 추가 예외가 발생합니다.*/
            IDE_TEST( freeSlotByLeafFullScan( s_pIndexHeader,
                                              a_pRow ) != IDE_SUCCESS );
            *aIsExistFreeKey = ID_TRUE;
        }

        IDE_CONT( SKIP_FREE_SLOT );
    }

    while(s_nDepth >= 0) /* no loop */
    {
        s_pDeleteRow   = a_pRow;

        /* BUG-32781 during update MMDB Index statistics, a reading slot 
         *           can be removed.
         *
         * Tree latch를 잡는 경우 prev/next/curr node를 잡아 updateStat 중에
         * row/node가 free되는 경우를 없앤다. */
        if ( sIsTreeLatched == ID_TRUE )
        {
            /* prev node */
            if ( s_pCurLeafNode->prevSPtr != NULL )
            {
                lockNode( s_pCurLeafNode->prevSPtr );
                sLatchedNode[sLatchedNodeCount] = s_pCurLeafNode->prevSPtr;
                sLatchedNodeCount++;
            }
            else
            {
                /* nothing to do */
            }

            /* curr node */
            lockNode( s_pCurLeafNode );
            sLatchedNode[sLatchedNodeCount] = s_pCurLeafNode;
            sLatchedNodeCount++;

            /* next node */
            if ( s_pCurLeafNode->nextSPtr != NULL )
            {
                lockNode( s_pCurLeafNode->nextSPtr );
                sLatchedNode[sLatchedNodeCount] = s_pCurLeafNode->nextSPtr;
                sLatchedNodeCount++;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            lockNode( s_pCurLeafNode );
            sLatchedNode[sLatchedNodeCount] = s_pCurLeafNode;
            sLatchedNodeCount++;
        }

        IDE_ERROR_RAISE( sLatchedNodeCount <= 3, ERR_INVALID_LATCH_NODE_COUNT );

        /* node latch를 잡기전 split 되어 삭제할 키가 다른 노드로 이동했을
         * 수도 있다. 그렇게 되면 현 노드의 모든 Key가 삭제될 수 있다. */
        if ( ( s_pCurLeafNode->mSlotCount == 0 ) ||
             ( ( s_pCurLeafNode->flag & SMNB_NODE_VALID_MASK ) == SMNB_NODE_INVALID ) )
        {
            IDE_ERROR_RAISE( sIsTreeLatched == ID_FALSE, ERR_NOT_TREE_UNLATCH );
            IDE_ERROR_RAISE( sLatchedNodeCount == 1, ERR_INVALID_LATCH_NODE_COUNT );

            sLatchedNodeCount = 0;
            unlockNode( s_pCurLeafNode );

            goto restart;
        }
        else
        {
            /* nothing to do */
        }

        /* 노드 삭제까지 필요한 경우 Tree latch를 잡아야 한다. */
        if ( sIsTreeLatched == ID_FALSE )
        {
            if ( ( s_pCurLeafNode->mSlotCount - 1 ) == 0 )
            {
                IDE_ERROR_RAISE( sLatchedNodeCount == 1, ERR_INVALID_LATCH_NODE_COUNT );
                sNeedTreeLatch = ID_TRUE;

                sLatchedNodeCount = 0;
                unlockNode( s_pCurLeafNode );

                goto restart;
            }
            else
            {
                /* nothing to do */
            }
        }

        /* 키 위치를 찾고 */
        IDE_TEST( findSlotInNode( s_pIndexHeader,
                                  (smnbNode *)s_pCurLeafNode,
                                  s_pDeleteRow,
                                  &s_nSlotPos ) != IDE_SUCCESS );

        /* 삭제할 키를 찾는 도중 split 된 경우.. */
        if ( s_nSlotPos == s_pCurLeafNode->mSlotCount )
        {
            if ( s_pCurLeafNode->nextSPtr != NULL )
            {
                IDE_ERROR_RAISE( sIsTreeLatched == ID_FALSE, ERR_NOT_TREE_UNLATCH );
                IDE_ERROR_RAISE( sLatchedNodeCount == 1, ERR_INVALID_LATCH_NODE_COUNT );

                sLatchedNodeCount = 0;
                unlockNode( s_pCurLeafNode );

                goto restart;
            }
        }
        else
        {
            /* nothing to do */
        }

        /* 삭제할 키 위치가 마지막 slot이면 상위 노드의 slot이 변경되기 때문에
         * Tree latch를 잡는다. */
        if ( sIsTreeLatched == ID_FALSE )
        {
            if ( s_nSlotPos == ( s_pCurLeafNode->mSlotCount - 1 ) )
            {
                IDE_ERROR_RAISE( sLatchedNodeCount == 1, ERR_INVALID_LATCH_NODE_COUNT );

                sNeedTreeLatch = ID_TRUE;

                sLatchedNodeCount = 0;
                unlockNode( s_pCurLeafNode );

                goto restart;
            }
            else
            {
                /* nothing to do */
            }
        }

        if ( needToUpdateStat( s_pIndexHeader->mIsMemTBS ) == ID_TRUE )
        {
            IDE_TEST( updateStat4Delete( (smnIndexHeader*)a_pIndex,
                                         s_pIndexHeader,
                                         s_pCurLeafNode,
                                         (SChar*)s_pDeleteRow,
                                         &s_nSlotPos,
                                         sIsUniqueIndex ) != IDE_SUCCESS );
        }
        else
        {
            /* nothing to do ... */
        }

        SMNB_SCAN_LATCH(s_pCurLeafNode);

        deleteSlotInLeafNode( s_pCurLeafNode,
                              (SChar*)s_pDeleteRow,
                              &s_nSlotPos );

        SMNB_SCAN_UNLATCH(s_pCurLeafNode);

        /* BUG-32655 [sm-mem-index] The MMDB Ager must not ignore the failure of
         * index aging.
         * BUG-32984 [SM] during rollback after updateInplace in memory index,
         *                it attempts to delete a key that does not exist.
         * updateInplace rollback인 경우 키가 존재하지 않을 수 있다. */
        if ( s_nSlotPos == -1 )
        {
            *aIsExistFreeKey = ID_FALSE;

            if ( aIgnoreNotFoundKey == ID_FALSE )
            {
                while ( sLatchedNodeCount > 0 )
                {
                    sLatchedNodeCount--;
                    unlockNode(sLatchedNode[sLatchedNodeCount]);
                }

                if ( sIsTreeLatched == ID_TRUE )
                {
                    sIsTreeLatched = ID_FALSE;
                    unlockTree(s_pIndexHeader);
                }
                /* BUG-47618 key를 반드시 발견해야 하는 상황에서 발견하지 못하였을 때
                 * leaf node를 fullscan 해서 row pointer를 찾습니다.
                 * 예외 상황에서만 호출되는 함수로, 발견하면 통과하고,
                 * row를 찾아서 delete 하지 못할 경우 추가 예외가 발생합니다.*/
                IDE_TEST( freeSlotByLeafFullScan( s_pIndexHeader,
                                                  a_pRow ) != IDE_SUCCESS );
                *aIsExistFreeKey = ID_TRUE;
            }

            IDE_CONT( SKIP_FREE_SLOT );
        }

        if ( s_pCurLeafNode->mSlotCount == 0 )
        {
            IDE_ERROR_RAISE( sIsTreeLatched == ID_TRUE, ERR_NOT_TREE_LATCH );

            //remove from leaf node list
            if ( s_pCurLeafNode->prevSPtr != NULL )
            {
                ((smnbLNode*)(s_pCurLeafNode->prevSPtr))->nextSPtr = s_pCurLeafNode->nextSPtr;
            }

            if ( s_pCurLeafNode->nextSPtr != NULL )
            {
                ((smnbLNode*)(s_pCurLeafNode->nextSPtr))->prevSPtr = s_pCurLeafNode->prevSPtr;
            }

            s_pCurLeafNode->freeNodeList = s_pFreeNode;
            s_pFreeNode = (smnbNode*)s_pCurLeafNode;

            // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
            sDeleteNodeCount++;

            if ( s_pCurLeafNode == s_pIndexHeader->root )
            {
                IDL_MEM_BARRIER;
                s_pIndexHeader->root = NULL;
            }
        }
        else
        {
            if ( s_nSlotPos == s_pCurLeafNode->mSlotCount )
            {
                /* PROJ-2433 */
                s_pInsertRow = NULL;
                sInsertKey   = NULL;

                getLeafSlot( &s_pInsertRow,
                             &sInsertKey, 
                             s_pCurLeafNode,
                             (SShort)( s_nSlotPos - 1 ) );
            }
            else
            {
                break;
            }
        }

        for(s_nDepth--; s_nDepth >= 0; s_nDepth--)
        {
            s_pCurInternalNode = (smnbINode*)(s_stack[s_nDepth].node);

            SMNB_SCAN_LATCH(s_pCurInternalNode);

            s_nSlotPos = s_stack[s_nDepth].lstReadPos;

            if ( s_pInsertRow != NULL )
            {
                if ( s_pCurInternalNode->mRowPtrs[s_nSlotPos] != NULL )
                {
                    /* PROJ-2433
                     * child pointer는 유지하고
                     * row pointer와 direct key를변경 */
                    setInternalSlot( s_pCurInternalNode,
                                     (SShort)s_nSlotPos,
                                     s_pCurInternalNode->mChildPtrs[s_nSlotPos],
                                     s_pInsertRow,
                                     sInsertKey );
                }
                else
                {
                    SMNB_SCAN_UNLATCH(s_pCurInternalNode);

                    break;
                }
            }
            else
            {
                IDE_TEST( deleteSlotInInternalNode(s_pCurInternalNode, s_nSlotPos)
                          != IDE_SUCCESS );
                s_nSlotPos--;
            }

            SMNB_SCAN_UNLATCH(s_pCurInternalNode);

            // BUG-26625 메모리 인덱스 스캔시 SMO를 검출하는 로직에 문제가 있습니다.

            if ( s_pCurInternalNode->mSlotCount == 0 )
            {
                //remove from sibling node list
                if ( s_pCurInternalNode->prevSPtr != NULL )
                {
                    ((smnbINode*)(s_pCurInternalNode->prevSPtr))->nextSPtr = s_pCurInternalNode->nextSPtr;
                }

                if ( s_pCurInternalNode->nextSPtr != NULL )
                {
                    ((smnbINode*)(s_pCurInternalNode->nextSPtr))->prevSPtr = s_pCurInternalNode->prevSPtr;
                }

                if ( s_pCurInternalNode == s_pIndexHeader->root )
                {
                    IDL_MEM_BARRIER;
                    s_pIndexHeader->root = NULL;
                }

                s_pCurInternalNode->freeNodeList = s_pFreeNode;
                s_pFreeNode  = (smnbNode*)s_pCurInternalNode;
                s_pInsertRow = NULL;
                sInsertKey   = NULL;

                s_pIndexHeader->mAgerStat.mNodeDeleteCount++;

                // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
                sDeleteNodeCount++;
            }
            else
            {
                if ( s_nSlotPos == ( s_pCurInternalNode->mSlotCount - 1 ) )
                {
                    if ( s_pInsertRow == NULL )
                    {
                        /* PROJ-2433 */
                        s_pInsertRow = NULL;
                        sInsertKey   = NULL;

                        getInternalSlot( NULL,
                                         &s_pInsertRow,
                                         &sInsertKey,
                                         s_pCurInternalNode,
                                         (SShort)s_nSlotPos );
                    }

                    if ( s_pInsertRow == NULL )
                    {
                        s_pCurInternalNode =
                            (smnbINode*)(s_pCurInternalNode->mChildPtrs[s_pCurInternalNode->mSlotCount - 1]);

                        while ( SMNB_IS_INTERNAL_NODE( s_pCurInternalNode ) )
                        {
                            SMNB_SCAN_LATCH(s_pCurInternalNode);

                            IDE_TEST( s_pCurInternalNode->mSlotCount < 1 );

                            /*
                             * PROJ-2433
                             * child pointer는 유지
                             */
                            setInternalSlot( s_pCurInternalNode,
                                             (SShort)( s_pCurInternalNode->mSlotCount - 1 ),
                                             s_pCurInternalNode->mChildPtrs[s_pCurInternalNode->mSlotCount - 1],
                                             NULL,
                                             NULL );

                            SMNB_SCAN_UNLATCH(s_pCurInternalNode);

                            s_pCurInternalNode =
                                (smnbINode*)(s_pCurInternalNode->mChildPtrs[s_pCurInternalNode->mSlotCount - 1]);
                        }

                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }

        break;
    }//while

    IDE_EXCEPTION_CONT( SKIP_FREE_SLOT );

    while ( sLatchedNodeCount > 0 )
    {
        sLatchedNodeCount--;
        unlockNode( sLatchedNode[sLatchedNodeCount] );
    }

    if ( sIsTreeLatched == ID_TRUE )
    {
        sIsTreeLatched = ID_FALSE;
        unlockTree( s_pIndexHeader );
    }

    if ( s_pFreeNode != NULL )
    {
        IDE_TEST( s_pFreeNode == s_pFreeNode->freeNodeList );

        // BUG-18292 : V$MEM_BTREE_HEADER 정보 추가
        s_pIndexHeader->nodeCount -= sDeleteNodeCount;

        smLayerCallback::addNodes2LogicalAger( mSmnbFreeNodeList,
                                               (smnNode*)s_pFreeNode );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_NOT_TREE_UNLATCH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] Tree Latch is not unlatched." );
    }
    IDE_EXCEPTION( ERR_INVALID_LATCH_NODE_COUNT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] invalid latch node count [%"ID_UINT32_FMT"]",
                     sLatchedNodeCount );
    }
    IDE_EXCEPTION( ERR_NOT_TREE_LATCH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] Tree Latch is not latched." );
    }
    IDE_EXCEPTION_END;

    smnbBTree::logIndexHeader( (smnIndexHeader*)a_pIndex );

    if ( s_pCurInternalNode != NULL )
    {
        logIndexNode( (smnIndexHeader*)a_pIndex,
                      (smnbNode*)s_pCurInternalNode );
    }

    if ( s_pCurLeafNode != NULL )
    {
        logIndexNode( (smnIndexHeader*)a_pIndex,
                      (smnbNode*)s_pCurLeafNode );
    }

    sRowOID = SMP_SLOT_GET_OID( a_pRow );

    ideLog::log( IDE_ERR_0,
                 "RowPtr:%16"ID_vULONG_FMT" %16"ID_vULONG_FMT,
                 a_pRow,
                 sRowOID );

    ideLog::logMem( IDE_ERR_0,
                    (UChar*)a_pRow,
                    s_pIndexHeader->mFixedKeySize
                    + SMP_SLOT_HEADER_SIZE );

    IDE_DASSERT( 0 );

    while ( sLatchedNodeCount > 0 )
    {
        sLatchedNodeCount--;
        unlockNode( sLatchedNode[sLatchedNodeCount] );
    }

    if ( sIsTreeLatched == ID_TRUE )
    {
        sIsTreeLatched = ID_FALSE;
        unlockTree( s_pIndexHeader );
    }

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( (smnIndexHeader *)a_pIndex );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::checkExistKey( void   * aIndex,
                                 SChar  * aRow )

{
    smnbHeader*  sIndexHeader;
    SInt         sDepth;
    smnbLNode*   sCurLeafNode = NULL;
    SInt         sSlotPos;
    smnbStack    sStack[SMNB_STACK_DEPTH];
    smOID        sRowOID;
    UInt         sState = 0;

    sIndexHeader  = (smnbHeader*)((smnIndexHeader*)aIndex)->mHeader;
    sDepth = -1;

    lockTree( sIndexHeader );
    sState = 1;

    IDE_TEST( findPosition( sIndexHeader,
                            NULL, /* smiCallBack */
                            aRow,
                            &sDepth,
                            sStack,
                            &sCurLeafNode ) != IDE_SUCCESS );

    if ( sDepth != -1 )
    {
        /* 해당 Node 찾았음 */

        IDE_TEST( findSlotInNode( sIndexHeader,
                                  (smnbNode *)sCurLeafNode,
                                  aRow,
                                  &sSlotPos ) != IDE_SUCCESS );

        if ( ( sSlotPos != sCurLeafNode->mSlotCount ) &&
             ( sCurLeafNode->mRowPtrs[ sSlotPos ] == aRow ) )
        {
            /* Delete되었는지 확인하러 들어왔는데, 존재하면 이상함 */
            smnbBTree::logIndexHeader( (smnIndexHeader*)aIndex );
            logIndexNode( (smnIndexHeader*)aIndex,
                          (smnbNode*)sCurLeafNode );
            sRowOID = SMP_SLOT_GET_OID( aRow );
            ideLog::log( IDE_SM_0, 
                         "RowPtr:%16lu %16lu",
                         aRow,
                         sRowOID );
            ideLog::logMem( IDE_SM_0,
                            (UChar*)aRow,
                            sIndexHeader->mFixedKeySize 
                            + SMP_SLOT_HEADER_SIZE );

            /* BUG-47526 Assert 위치를 옮깁니다.
             * 상위 단 보다는 Index의 key를 확인한 시점에 Assert되어야
             * 비교 검사가 가능한듯 하여 아래로 내려보냅니다. */
            IDE_ASSERT( 0 );
        }
    }

    sState = 0;
    unlockTree( sIndexHeader );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smnbBTree::logIndexHeader( (smnIndexHeader*)aIndex );

    if ( sCurLeafNode != NULL )
    {
        logIndexNode( (smnIndexHeader*)aIndex,
                      (smnbNode*)sCurLeafNode );
        sRowOID = SMP_SLOT_GET_OID( aRow );
        ideLog::log( IDE_SM_0, 
                     "RowPtr:%16lu %16lu",
                     aRow,
                     sRowOID );
        ideLog::logMem( IDE_SM_0,
                        (UChar*)aRow,
                        sIndexHeader->mFixedKeySize + SMP_SLOT_HEADER_SIZE );

        IDE_DASSERT( 0 );
    }

    if ( sState != 0)
    {
        IDE_PUSH();
        unlockTree( sIndexHeader );
        IDE_POP();
    }

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( (smnIndexHeader *)aIndex );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::gatherStat                      *
 * ------------------------------------------------------------------*
 * TASK-4990 changing the method of collecting index statistics
 * 수동 통계정보 수집 기능
 * index의 통계정보를 구축한다.
 *
 * Statistics    - [IN]  IDLayer 통계정보
 * Trans         - [IN]  이 작업을 요청한 Transaction
 * Percentage    - [IN]  Sampling Percentage
 * Degree        - [IN]  Parallel Degree
 * Header        - [IN]  대상 TableHeader
 * Index         - [IN]  대상 index
 *********************************************************************/
IDE_RC smnbBTree::gatherStat( idvSQL         * /* aStatistics */, 
                              void           * aTrans,
                              SFloat           aPercentage,
                              SInt             /* aDegree */,
                              smcTableHeader * aHeader,
#ifdef DEBUG
                              void           * aTotalTableArg,
#else
                              void           * /* aTotalTableArg */,
#endif
                              smnIndexHeader * aIndex,
                              void           * aStats,
                              idBool           aDynamicMode )
{
    smnbHeader       * sIdxHdr = NULL;
    smiIndexStat       sIndexStat;
    UInt               sAnalyzedNodeCount = 0;
    UInt               sTotalNodeCount    = 0;
    SLong              sKeyCount          = 0;
    volatile SLong     sNumDist           = 0; /* BUG-40691 */
    volatile SLong     sClusteringFactor  = 0; /* BUG-40691 */
    UInt               sIndexHeight       = 0;
    SLong              sMetaSpace         = 0;
    SLong              sUsedSpace         = 0;
    SLong              sAgableSpace       = 0;
    SLong              sFreeSpace         = 0;
    SLong              sAvgSlotCnt        = 0;
    idBool             sIsNodeLatched     = ID_FALSE;
    idBool             sIsFreeNodeLocked  = ID_FALSE;
    smnbLNode        * sCurLeafNode;
    smxTrans         * sSmxTrans          = (smxTrans *)aTrans;
    UInt               sStatFlag          = SMI_INDEX_BUILD_RT_STAT_UPDATE;

    IDE_DASSERT( aIndex != NULL );
    IDE_DASSERT( aTotalTableArg == NULL ); 

    /* BUG-44794 인덱스 빌드시 인덱스 통계 정보를 수집하지 않는 히든 프로퍼티 추가 */
    SMI_INDEX_BUILD_NEED_RT_STAT( sStatFlag, sSmxTrans );

    sIdxHdr       = (smnbHeader*)(aIndex->mHeader);
    idlOS::memset( &sIndexStat, 0, ID_SIZEOF( smiIndexStat ) );

    IDE_TEST( smLayerCallback::beginIndexStat( aHeader,
                                               aIndex,
                                               aDynamicMode )
              != IDE_SUCCESS );

    /* FreeNode를 막는다. Node재활용을 막아서 TreeLatch 없이
     * Node를 FullScan 하기 위함이다. */
    smLayerCallback::blockFreeNode();
    sIsFreeNodeLocked = ID_TRUE;

    /************************************************************
     * Node 분석을 통한 통계정보 획득 시작
     ************************************************************/
    /* BeforeFirst겸 첫 페이지를 얻어옴 */
    sCurLeafNode = NULL;
    getNextNode4Stat( sIdxHdr,
                      ID_FALSE, /* BackTraverse */
                      &sIsNodeLatched,
                      &sIndexHeight,
                      &sCurLeafNode );

    while( sCurLeafNode != NULL )
    {
        /*Sampling Percentage가 P라 했을때, 누적되는 값 C를 두고 C에 P를
         * 더했을때 1을 넘어가는 경우에 Sampling 하는 것으로 한다.
         *
         * 예)
         * P=1, C=0
         * 첫번째 페이지 C+=P  C=>0.25
         * 두번째 페이지 C+=P  C=>0.50
         * 세번째 페이지 C+=P  C=>0.75
         * 네번째 페이지 C+=P  C=>1(Sampling!)  C--; C=>0
         * 다섯번째 페이지 C+=P  C=>0.25
         * 여섯번째 페이지 C+=P  C=>0.50
         * 일곱번째 페이지 C+=P  C=>0.75
         * 여덟번째 페이지 C+=P  C=>1(Sampling!)  C--; C=>0 */
        if ( ( (UInt)( aPercentage * sTotalNodeCount
                       + SMI_STAT_SAMPLING_INITVAL ) ) !=
             ( (UInt)( aPercentage * (sTotalNodeCount+1)
                       + SMI_STAT_SAMPLING_INITVAL ) ) )
        {
            sAnalyzedNodeCount ++;
            IDE_TEST( analyzeNode( aIndex,
                                   sIdxHdr,
                                   sCurLeafNode,
                                   (SLong *)&sClusteringFactor,
                                   (SLong *)&sNumDist,
                                   &sKeyCount,
                                   &sMetaSpace,
                                   &sUsedSpace,
                                   &sAgableSpace,
                                   &sFreeSpace )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Skip함 */
        }
        sTotalNodeCount ++;

        getNextNode4Stat( sIdxHdr,
                          ID_FALSE, /* BackTraverse */
                          &sIsNodeLatched,
                          NULL, /* sIndexHeight */
                          &sCurLeafNode );
    }

    /************************************************************
     * 통계정보 보정함
     ************************************************************/
    if ( sAnalyzedNodeCount == 0 )
    {
        /* 조회대상이 없으니 보정 필요 없음 */
    }
    else
    {
        if ( sKeyCount == sAnalyzedNodeCount )
        {
            /* Node당 Key가 하나씩 들어간 경우. 크기보정만 함 */
            sNumDist = sNumDist * sTotalNodeCount / sAnalyzedNodeCount; 
            sClusteringFactor = 
                sClusteringFactor * sTotalNodeCount / sAnalyzedNodeCount; 
        }
        else
        {
            /* NumDist를 보정한다. 현재 값은 페이지와 페이지간에 걸친 키
             * 관계는 계산되지 않기 때문에, 그 값을 추정한다.
             *
             * C     = 현재Cardinaliy
             * K     = 분석한 Key개수
             * P     = 분석한 Node개수
             * (K-P) = 분석된 Key관계 수
             * (K-1) = 실제 Key관계 수
             *
             * C / ( K - P ) * ( K - 1 ) */

            sNumDist = (SLong)
                ( ( ( ( (SDouble) sNumDist ) / ( sKeyCount - sAnalyzedNodeCount ) )
                    * ( sKeyCount - 1 ) * sTotalNodeCount / sAnalyzedNodeCount )
                  + 1 + SMNB_COST_EPS );

            sClusteringFactor = (SLong)
                ( ( ( ( (SDouble) sClusteringFactor ) / ( sKeyCount - sAnalyzedNodeCount ) )
                  * ( sKeyCount - 1 ) * sTotalNodeCount / sAnalyzedNodeCount )
                  + SMNB_COST_EPS );
        }
        sAvgSlotCnt  = sKeyCount / sAnalyzedNodeCount;
        sUsedSpace   = sUsedSpace * sTotalNodeCount / sAnalyzedNodeCount;
        sAgableSpace = sAgableSpace * sTotalNodeCount / sAnalyzedNodeCount;
        sFreeSpace   = sFreeSpace * sTotalNodeCount / sAnalyzedNodeCount;
        sKeyCount    = sKeyCount * sTotalNodeCount / sAnalyzedNodeCount;
    }

    sIndexStat.mSampleSize        = aPercentage;
    sIndexStat.mNumPage           = sIdxHdr->nodeCount;
    sIndexStat.mAvgSlotCnt        = sAvgSlotCnt;
    sIndexStat.mIndexHeight       = sIndexHeight;
    sIndexStat.mClusteringFactor  = sClusteringFactor;
    sIndexStat.mNumDist           = sNumDist; 
    sIndexStat.mKeyCount          = sKeyCount;
    sIndexStat.mMetaSpace         = sMetaSpace;
    sIndexStat.mUsedSpace         = sUsedSpace;
    sIndexStat.mAgableSpace       = sAgableSpace;
    sIndexStat.mFreeSpace         = sFreeSpace;

    /************************************************************
     * MinMax 통계정보 구축
     ************************************************************/
    /* PROJ-2492 Dynamic sample selection */
    // 다이나믹 모드일때 index 의 min,max 는 수집하지 않는다.
    if ( aDynamicMode == ID_FALSE )
    {
        /* BUG-44794 인덱스 빌드시 인덱스 통계 정보를 수집하지 않는 히든 프로퍼티 추가 */
        if ( ( sStatFlag & SMI_INDEX_BUILD_RT_STAT_MASK )
                == SMI_INDEX_BUILD_RT_STAT_UPDATE )
        {
            IDE_TEST( rebuildMinMaxStat( aIndex,
                                         sIdxHdr,
                                         ID_TRUE ) /* aMinStat */
                      != IDE_SUCCESS );
            IDE_TEST( rebuildMinMaxStat( aIndex,
                                         sIdxHdr,
                                         ID_FALSE ) /* aMaxStat */
                      != IDE_SUCCESS );
        }
    }
    else
    {
        // Nothing To Do
    }

    IDE_TEST( smLayerCallback::setIndexStatWithoutMinMax( aIndex,
                                                          aTrans,
                                                          (smiIndexStat*)aStats,
                                                          &sIndexStat,
                                                          aDynamicMode,
                                                          sStatFlag )
              != IDE_SUCCESS );

    sIsFreeNodeLocked = ID_FALSE;
    smLayerCallback::unblockFreeNode();

    IDE_TEST( smLayerCallback::endIndexStat( aHeader,
                                             aIndex,
                                             aDynamicMode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    smnbBTree::logIndexHeader( aIndex );

    ideLog::log( IDE_SM_0,
                 "========================================\n"
                 " REBUILD INDEX STATISTICS: END(FAIL)    \n"
                 "========================================\n"
                 " Index Name: %s\n"
                 " Index ID:   %u\n"
                 "========================================\n",
                 aIndex->mName,
                 aIndex->mId );

    if ( sIsNodeLatched == ID_TRUE )
    {
        sIsNodeLatched = ID_FALSE;
        unlockNode( sCurLeafNode );
    }

    if ( sIsFreeNodeLocked == ID_TRUE )
    {
        sIsFreeNodeLocked = ID_FALSE;
        smLayerCallback::unblockFreeNode();
    }

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( aIndex );
    }

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::rebuildMinMaxstat               *
 * ------------------------------------------------------------------*
 * TASK-4990 changing the method of collecting index statistics
 * MinMax 통계정보를 구축한다.
 *
 * aPersistentIndexHeader - [IN]  대상 Index의 PersistentHeader
 * aRuntimeIndexHeader    - [IN]  대상 Index의 RuntimeHeader
 * aMinStat               - [IN]  MinStat인가?
 *********************************************************************/
IDE_RC smnbBTree::rebuildMinMaxStat( smnIndexHeader * aPersistentIndexHeader,
                                     smnbHeader     * aRuntimeIndexHeader,
                                     idBool           aMinStat )
{
    smnbLNode        * sCurLeafNode;
    idBool             sIsNodeLatched    = ID_FALSE;
    idBool             sBacktraverse;
    idBool             sSet = ID_FALSE;
    SInt               i;

    if ( aMinStat == ID_TRUE )
    {
        /* 정방향 탐색하면서 가장 작은 키를 찾음 */
        sBacktraverse = ID_FALSE;
    }
    else
    {
        /* 정방향 탐색하면서 가장 큰 키를 찾음 */
        sBacktraverse = ID_TRUE;
    }

    /* MinValue 찾아 구축함 */
    sCurLeafNode = NULL;
    getNextNode4Stat( aRuntimeIndexHeader,
                      sBacktraverse, 
                      &sIsNodeLatched,
                      NULL, /* sIndexHeight */
                      &sCurLeafNode );

    while( sCurLeafNode != NULL )
    {
        if ( sCurLeafNode->mSlotCount > 0 )
        {
            /* Key가 있는 Node를 찾았음. */
            if ( isNullColumn( aRuntimeIndexHeader->columns,
                               &(aRuntimeIndexHeader->columns->column),
                               sCurLeafNode->mRowPtrs[ 0 ] )
                 == ID_TRUE )
            {
                /* 첫 Key가 NULL이면 전부 NULL */
                if ( aMinStat == ID_TRUE )
                {
                    /* Min값을 찾기 위해 정방향 탐색했는데 다 Null이란건
                     * Min값이 없다는 의미 */
                    sIsNodeLatched = ID_FALSE;
                    unlockNode( sCurLeafNode );
                    break;
                }
                else
                {
                    /*MaxStat을 찾는 경우는 다음 Node를 찾으면 됨 */
                }
                
            }
            else
            {
                /* Null 아닌 키가 있음 */
                if ( aMinStat == ID_TRUE )
                {
                    /* Min이면 0번째 Key를 바로 설정 */
                    IDE_TEST( setMinMaxStat( aPersistentIndexHeader,
                                             sCurLeafNode->mRowPtrs[ 0 ],
                                             aRuntimeIndexHeader->columns,
                                             ID_TRUE ) != IDE_SUCCESS ); // aIsMinStat
                    sSet = ID_TRUE;
                }
                else
                {
                    /* Max면 Null이 아닌 키를 탐색 */
                    for ( i = sCurLeafNode->mSlotCount - 1 ; i >= 0 ; i-- )
                    {
                        if ( isNullColumn( 
                                aRuntimeIndexHeader->columns,
                                &(aRuntimeIndexHeader->columns->column),
                                sCurLeafNode->mRowPtrs[ i ] )
                            == ID_FALSE )
                        {
                            IDE_TEST( setMinMaxStat( aPersistentIndexHeader,
                                                     sCurLeafNode->mRowPtrs[ i ],
                                                     aRuntimeIndexHeader->columns,
                                                     ID_FALSE ) != IDE_SUCCESS ); // aIsMinStat
                            sSet = ID_TRUE;
                            break;
                        }
                        else
                        {
                            /* Null임. 다음 Row로 */
                        }
                    }
                    /* 순회 중 NULL이 아닌 값을 찾아서 i가 0과 같거나 커야함*/
                    IDE_ERROR( i >= 0 );
                }
                sIsNodeLatched = ID_FALSE;
                unlockNode( sCurLeafNode );
                break;
            }
        }
        else
        {
            /* Node에 Key가 없음*/
        }
        getNextNode4Stat( aRuntimeIndexHeader,
                          sBacktraverse, 
                          &sIsNodeLatched,
                          NULL, /* sIndexHeight */
                          &sCurLeafNode );
    }

    if ( sSet == ID_FALSE )
    {
        IDE_TEST( setMinMaxStat( aPersistentIndexHeader,
                       NULL, /*RowPointer */
                       aRuntimeIndexHeader->columns,
                       aMinStat ) != IDE_SUCCESS ); // aIsMinStat
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    logIndexNode( aPersistentIndexHeader,
                  (smnbNode*)sCurLeafNode );

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::analyzeNode                     *
 * ------------------------------------------------------------------*
 * TASK-4990 changing the method of collecting index statistics
 * 해당 Node를 분석한다.
 *
 * aPersistentIndexHeader - [IN]  대상 Index의 PersistentHeader
 * aRuntimeIndexHeader    - [IN]  대상 Index의 RuntimeHeader
 * aTargetNode            - [IN]  대상 Node
 * aClusteringFactor      - [OUT] 획득한 ClusteringFactor
 * aNumDist               - [OUT] 획득한 NumDist
 * aKeyCount              - [OUT] 획득한 KeyCount
 * aMetaSpace             - [OUT] PageHeader, ExtDir등 Meta 공간
 * aUsedSpace             - [OUT] 현재 사용중인 공간
 * aAgableSpace           - [OUT] 나중에 Aging가능한 공간
 * aFreeSpace             - [OUT] Data삽입이 가능한 빈 공간
 *********************************************************************/
IDE_RC smnbBTree::analyzeNode( smnIndexHeader * aPersistentIndexHeader,
                               smnbHeader     * aRuntimeIndexHeader,
                               smnbLNode      * aTargetNode,
                               SLong          * aClusteringFactor,
                               SLong          * aNumDist,
                               SLong          * aKeyCount,
                               SLong          * aMetaSpace,
                               SLong          * aUsedSpace,
                               SLong          * aAgableSpace, 
                               SLong          * aFreeSpace )
{
    SInt                sCompareResult;
    SChar             * sCurRow          = NULL;
    SChar             * sPrevRow         = NULL;
    SInt                sDeletedKeyCount = 0;
    SLong               sNumDist         = 0;
    SLong               sClusteringFactor = 0;
    scPageID            sPrevPID = SC_NULL_PID;
    scPageID            sCurPID  = SC_NULL_PID;
    UInt                i;

    for ( i = 0 ; i < (UInt)aTargetNode->mSlotCount ; i++ )
    {
        sPrevRow = sCurRow;
        sCurRow  = aTargetNode->mRowPtrs[ i ];

        /* KeyCount 계산 */
        if ( ( !( SM_SCN_IS_FREE_ROW( ((smpSlotHeader*)sCurRow)->mLimitSCN ) ) ) ||
             ( SM_SCN_IS_LOCK_ROW( ((smpSlotHeader*)sCurRow)->mLimitSCN ) ) )
        {
            /* Next가 설정된 지워질 Row */
            sDeletedKeyCount ++;
        }

        /* NumDist 계산 */
        if ( sPrevRow != NULL )
        {
            if ( isNullColumn( aRuntimeIndexHeader->columns,
                              &(aRuntimeIndexHeader->columns->column),
                              sCurRow ) == ID_FALSE )
            {
                /* Null이 아닌경우에만 NumDist 계산함 */
                sCompareResult = compareRows( &aRuntimeIndexHeader->mStmtStat,
                                              aRuntimeIndexHeader->columns,
                                              aRuntimeIndexHeader->fence,
                                              sCurRow,
                                              sPrevRow );
                IDE_ERROR( sCompareResult >= 0 );
                if ( sCompareResult > 0 )
                {
                    sNumDist ++;
                }
                else
                {
                    /* 동일함 */
                }
            }
            else
            {
                /* NullKey*/
            }
            
            sPrevPID = sCurPID;
            sCurPID = SMP_SLOT_GET_PID( sCurRow );
            if ( sPrevPID != sCurPID )
            {
                sClusteringFactor ++;
            }
        }
    }

    /* PROJ-2433 */
    *aNumDist          += sNumDist;
    *aKeyCount         += aTargetNode->mSlotCount;
    *aClusteringFactor += sClusteringFactor;
    *aMetaSpace         = 0;
    *aUsedSpace        += ( ( aTargetNode->mSlotCount - sDeletedKeyCount ) *
                            ( ID_SIZEOF( aTargetNode->mRowPtrs[0] ) + aTargetNode->mKeySize ) ); 
    *aAgableSpace      += ( sDeletedKeyCount *
                            ( ID_SIZEOF( aTargetNode->mRowPtrs[0] ) + aTargetNode->mKeySize ) );
    *aFreeSpace        += ( ( SMNB_LEAF_SLOT_MAX_COUNT(aRuntimeIndexHeader) - aTargetNode->mSlotCount ) *
                            ( ID_SIZEOF( aTargetNode->mRowPtrs[0] ) + aTargetNode->mKeySize ) );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    logIndexNode( aPersistentIndexHeader,
                  (smnbNode*)aTargetNode );

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::getNextNode4Stat                *
 * ------------------------------------------------------------------*
 * TASK-4990 changing the method of collecting index statistics
 * 다음 Node를 가져온다.
 *
 * aIdxHdr       - [IN]     대상 Index
 * aBackTraverse - [IN]     역탐색할 경우.
 * aNodeLatched  - [IN/OUT] NodeLatch를 잡았는가?
 * aIndexHeight  - [OUT]    NULL이 아닐 경우, Index 높이 반환함.
 * aCurNode      - [OUT]    다음 Node
 *********************************************************************/
void smnbBTree::getNextNode4Stat( smnbHeader     * aIdxHdr,
                                  idBool           aBackTraverse,
                                  idBool         * aNodeLatched,
                                  UInt           * aIndexHeight,
                                  smnbLNode     ** aCurNode )
{
    UInt          sIndexHeight      = 0;
    smnbINode   * sCurInternalNode;
    smnbLNode   * sCurLeafNode;

    sCurLeafNode = *aCurNode;

    if ( sCurLeafNode == NULL )
    {
        /* 최초 탐색, BeforeFirst *
         * TreeLatch를 걸어 SMO를 막고 Leftmost를 타며 내려감 */
        lockTree( aIdxHdr );

        if ( aIdxHdr->root == NULL )
        {
            sCurLeafNode = NULL; /* 비었다 ! */
        }
        else
        {
            sCurInternalNode = (smnbINode*)aIdxHdr->root;
            while ( SMNB_IS_INTERNAL_NODE( sCurInternalNode ) )
            {
                sIndexHeight ++;
                if ( aBackTraverse == ID_FALSE )
                {
                    sCurInternalNode = 
                        (smnbINode*)sCurInternalNode->mChildPtrs[0];
                }
                else
                {
                    sCurInternalNode = 
                        (smnbINode*)sCurInternalNode->mChildPtrs[
                            sCurInternalNode->mSlotCount - 1 ];
                }
            }

            sCurLeafNode = (smnbLNode*)sCurInternalNode;
            lockNode( sCurLeafNode );
            *aNodeLatched = ID_TRUE;
        }

        unlockTree( aIdxHdr );
    }
    else
    {
        /* 다음 Node 탐색.
         * 그냥 링크를 타고 좇아가면 됨 */
        *aNodeLatched = ID_FALSE;
        unlockNode( sCurLeafNode );

        if ( aBackTraverse == ID_FALSE )
        {
            sCurLeafNode = sCurLeafNode->nextSPtr;
        }
        else
        {
            sCurLeafNode = sCurLeafNode->prevSPtr;
        }
        if ( sCurLeafNode == NULL )
        {
            /* 다왔다 ! */
        }
        else
        {
            lockNode( sCurLeafNode );
            *aNodeLatched = ID_TRUE;
        }
    }

    /* NodeLatch를 안잡았으면 반환할 LeafNode가 없어야 하고
     * NodeLatch를 잡았으면 LeafNode가 있어야 한다. */
    IDE_DASSERT( 
        ( ( *aNodeLatched == ID_FALSE ) && ( sCurLeafNode == NULL ) ) ||
        ( ( *aNodeLatched == ID_TRUE  ) && ( sCurLeafNode != NULL ) ) );
    *aCurNode = sCurLeafNode;

    if ( aIndexHeight != NULL )
    {
         ( *aIndexHeight ) = sIndexHeight;
    }
}

IDE_RC smnbBTree::NA( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;
}

IDE_RC smnbBTree::beforeFirst( smnbIterator* a_pIterator,
                               const smSeekFunc** )
{
    for( a_pIterator->mKeyRange      = a_pIterator->mKeyRange;
         a_pIterator->mKeyRange->prev != NULL;
         a_pIterator->mKeyRange        = a_pIterator->mKeyRange->prev ) ;

    IDE_TEST( beforeFirstInternal( a_pIterator ) != IDE_SUCCESS );

    a_pIterator->flag  = a_pIterator->flag & (~SMI_RETRAVERSE_MASK);
    a_pIterator->flag |= SMI_RETRAVERSE_BEFORE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::beforeFirstInternal( smnbIterator* a_pIterator )
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    const smiRange* s_pRange;
    volatile SChar* s_pRow;
    idBool          s_bResult;
    SInt            s_nSlot = -1;
    SInt            s_cSlot;
    SInt            i = 0;
    SInt            s_nMin;
    SInt            s_nMax;
    IDU_LATCH       s_version;
    smnbStack       sStack[SMNB_STACK_DEPTH];
    SInt            sDepth = -1;
    smnbLNode*      s_pCurLNode = NULL;
    smnbLNode*      s_pNxtLNode = NULL;
    smnbLNode*      s_pPrvLNode = NULL;
    void          * sKey        = NULL;
    SChar         * sRowPtr     = NULL;
    idBool          sIsVisible  = ID_FALSE;

    s_pRange = a_pIterator->mKeyRange;

    if ( a_pIterator->mProperties->mReadRecordCount > 0 )
    {
        IDE_TEST( findPosition( a_pIterator->index,
                                &s_pRange->minimum,
                                NULL, /* aRow */
                                &sDepth,
                                sStack,
                                &s_pCurLNode ) != IDE_SUCCESS );

        while( (sDepth < 0) && (s_pRange->next != NULL) )
        {
            s_pRange = s_pRange->next;
            IDE_TEST( findPosition( a_pIterator->index,
                                    &s_pRange->minimum,
                                    NULL, /* aRow */
                                    &sDepth,
                                    sStack,
                                    &s_pCurLNode ) != IDE_SUCCESS );

            a_pIterator->mKeyRange = s_pRange;
        }
    }

    if ( ( sDepth >= 0 ) && ( a_pIterator->mProperties->mReadRecordCount > 0 ) )
    {
        while(1)
        {
            s_version   = getLatchValueOfLNode(s_pCurLNode) & (~SMNB_SCAN_LATCH_BIT);
            IDL_MEM_BARRIER;

            s_cSlot      = s_pCurLNode->mSlotCount;
            s_pPrvLNode  = s_pCurLNode->prevSPtr;
            s_pNxtLNode  = s_pCurLNode->nextSPtr;

            if ( s_cSlot == 0 )
            {
                if ( s_version == getLatchValueOfLNode(s_pCurLNode) )
                {
                    s_pCurLNode = s_pNxtLNode;

                    if ( s_pCurLNode == NULL )
                    {
                        break;
                    }
                }

                s_nSlot = 0;
                continue;
            }
            else
            {
                findFirstSlotInNode( a_pIterator->index,
                                     &s_pRange->minimum,
                                     (smnbNode *)s_pCurLNode,
                                     &s_nSlot );
            }

            a_pIterator->version  = s_version;
            a_pIterator->node     = s_pCurLNode;

            a_pIterator->prvNode  = s_pPrvLNode;
            a_pIterator->nxtNode  = s_pNxtLNode;

            a_pIterator->least    = ID_TRUE;
            a_pIterator->highest  = ID_FALSE;

            s_nMin = s_nSlot;
            s_nMax = s_cSlot - 1;

            if ( ( s_nSlot + 1 ) < s_cSlot )
            {
                s_pRow  = a_pIterator->node->mRowPtrs[s_nSlot];

                if ( s_pRow == NULL )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                /*
                 * PROJ-2433
                 * 지금값이 최대값 조건도 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.
                 */
                if ( isFullKey( a_pIterator->index,
                                (smnbNode *)(a_pIterator->node),
                                (SShort)s_nSlot ) == ID_TRUE )
                {
                    sKey = SMNB_GET_KEY_PTR( a_pIterator->node, s_nSlot );
                }
                else
                {
                    sKey = NULL;
                }

#ifdef DEBUG
                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                // BUG-29106 debugging code
                if ( SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN) )
                {
                    int  sSlotIdx;

                    ideLog::log( IDE_SERVER_0, "s_pRow : %lu\n", s_pRow );
                    ideLog::log( IDE_SERVER_0,
                                 "s_nSlot : %d, s_cSlot : %d\n",
                                 s_nSlot, s_cSlot );
                    for ( sSlotIdx = 0 ; sSlotIdx < s_cSlot ; sSlotIdx++ )
                    {
                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar *)(a_pIterator->node->mRowPtrs[sSlotIdx]),
                                        64 );
                    }
                    ideLog::log( IDE_SERVER_0, "s_pCurLNode : %lu\n", s_pCurLNode);
                    ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator->node,
                                    mNodeSize );
                    ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator,
                                    mIteratorSize );
                    IDE_DASSERT( 0 );
                }
#endif

                s_pRange->maximum.callback( &s_bResult,
                                            (SChar*)s_pRow,
                                            sKey,
                                            0,
                                            SC_NULL_GRID,
                                            s_pRange->maximum.data );

                if ( s_bResult == ID_TRUE )
                {
                    if ( ( s_nSlot + 2 ) < s_cSlot )
                    {
                        s_pRow = s_pCurLNode->mRowPtrs[s_nSlot + 1];

                        if ( s_pRow == NULL )
                        {
                            continue;
                        }

                        /* PROJ-2433
                         * 최대값 조건에 만족하는지 확인하는부분이다.
                         * direct key가 full key여야만 제대로된 비교가 가능하다.
                         */
                        if ( isFullKey( a_pIterator->index,
                                        (smnbNode *)s_pCurLNode,
                                        (SShort)( s_nSlot + 1 ) ) == ID_TRUE )
                        {
                            sKey = SMNB_GET_KEY_PTR( s_pCurLNode, s_nSlot + 1 );
                        }
                        else
                        {
                            sKey = NULL;
                        }

#ifdef DEBUG
                        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                        // BUG-29106 debugging code
                        if ( SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN) )
                        {
                            int sSlotIdx;

                            ideLog::log( IDE_SERVER_0, "s_pRow : %lu\n", s_pRow );
                            ideLog::log( IDE_SERVER_0,
                                         "s_nSlot : %d, s_cSlot : %d\n",
                                         s_nSlot, s_cSlot );
                            for ( sSlotIdx = 0 ; sSlotIdx < s_cSlot ; sSlotIdx++ )
                            {
                                ideLog::logMem( IDE_SERVER_0,
                                                (UChar *)(s_pCurLNode->mRowPtrs[sSlotIdx]),
                                                64 );
                            }
                            ideLog::log( IDE_SERVER_0, "s_pCurLNode : %lu\n", s_pCurLNode);
                            ideLog::logMem( IDE_SERVER_0, (UChar *)s_pCurLNode,
                                            mNodeSize );
                            ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator,
                                            mIteratorSize );
                            IDE_DASSERT( 0 );
                        }
                        else
                        {
                            /* nothing to do */
                        }
#endif

                        s_pRange->maximum.callback( &s_bResult,
                                                    (SChar*)s_pRow,
                                                    sKey,
                                                    0,
                                                    SC_NULL_GRID,
                                                    s_pRange->maximum.data);

                        if ( s_bResult == ID_FALSE )
                        {
                            a_pIterator->highest   = ID_TRUE;
                            s_nMax                 = s_nSlot;
                        }
                        else
                        {
                            /* nothing to do */
                        }
                    }
                }
                else
                {
                    a_pIterator->highest  = ID_TRUE;
                    s_nMax                = -1;
                }
            }

            if ( a_pIterator->highest == ID_FALSE )
            {
                s_pRow = s_pCurLNode->mRowPtrs[s_cSlot - 1];

                if ( s_pRow == NULL )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                /*
                 * PROJ-2433
                 * 최대값 조건에 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.
                 */
                if ( isFullKey( a_pIterator->index,
                                (smnbNode *)s_pCurLNode,
                                SShort( s_cSlot - 1 ) ) == ID_TRUE )
                {
                    sKey = SMNB_GET_KEY_PTR( s_pCurLNode, s_cSlot - 1 );
                }
                else
                {
                    sKey = NULL;
                }

#ifdef DEBUG
                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                // BUG-29106 debugging code
                if ( SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN) )
                {
                    int sSlotIdx;

                    ideLog::log( IDE_SERVER_0, "s_pRow : %lu\n", s_pRow );
                    ideLog::log( IDE_SERVER_0,
                                 "s_nSlot : %d, s_cSlot : %d\n",
                                 s_nSlot, s_cSlot );
                    for( sSlotIdx = 0; sSlotIdx < s_cSlot; sSlotIdx ++ )
                    {
                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar *)(s_pCurLNode->mRowPtrs[sSlotIdx]),
                                        64 );
                    }
                    ideLog::log( IDE_SERVER_0, "s_pCurLNode : %lu\n", s_pCurLNode);
                    ideLog::logMem( IDE_SERVER_0, (UChar *)s_pCurLNode,
                                    mNodeSize );
                    ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator,
                                    mIteratorSize );
                    IDE_DASSERT( 0 );
                }
#endif

                s_pRange->maximum.callback( &s_bResult,
                                            (SChar*)s_pRow,
                                            sKey,
                                            0,
                                            SC_NULL_GRID,
                                            s_pRange->maximum.data );
                if ( s_bResult == ID_TRUE )
                {
                    s_nMax = s_cSlot - 1;
                }
                else
                {
                    smnbBTree::findLastSlotInLeaf( a_pIterator->index,
                                                   &(s_pRange->maximum),
                                                   s_pCurLNode,
                                                   0,
                                                   s_cSlot - 1,
                                                   &s_nSlot );

                    a_pIterator->highest   = ID_TRUE;
                    s_nMax = s_nSlot;
                }
            }

            /* BUG-44043
               fetchNext()가 최초로 호출될때 scn을 만족하는 row가 하나라도 있어야
               aIterator->lstReturnRecPtr 값으로 세팅되어서,
               Key Redistribution이 발생해도 다음노드들을 탐색해서 정확한 다음 row를 찾아낼수있다. */

            /* BUG-48353
               fetchNext() 에서 어차피 checkSCN() 수행한다.
               여기서는 위의 BUG-44043 을 감안하여 최소 1개의 visible한 값만 찾는다.
               checkSCN() 수행시 발생할수있는 Pending Wait 를 최소화하기 위함이다.
               => 캐쉬노드를 만드는 여기서 대량의 Pending Wait가 발생하면,
                  Node Latch Loop 를 빠져나가지못해 결과적으로 FETCH 성능에 악영향을 줄수있다.*/
            sIsVisible = ID_FALSE;

            for ( i = s_nMin; i <= s_nMax; i++ )
            {
                sRowPtr = s_pCurLNode->mRowPtrs[i];

                if ( sRowPtr == NULL )
                {
                    break;
                }

                IDE_TEST( smnManager::checkSCN( (smiIterator *)a_pIterator,
                                                sRowPtr,
                                                NULL,
                                                &sIsVisible )
                          != IDE_SUCCESS );

                if ( sIsVisible == ID_TRUE )
                {
                    s_nMin = i; /* checSCN을 통과한 slot을 Min 으로 설정한다. */
                    break;
                }
            }

            /* checkSCN을 통과한 첫번째 slot 부터 s_nMax slot 까지
               캐쉬노드로 복사한다. */
            if ( sIsVisible == ID_TRUE )
            {
                idlOS::memcpy( &(a_pIterator->rows[0]),
                               &(s_pCurLNode->mRowPtrs[s_nMin]),
                               ( s_nMax - s_nMin + 1 ) * ID_SIZEOF(s_pCurLNode->mRowPtrs[0]) );
            }

            if ( s_version == getLatchValueOfLNode(s_pCurLNode) )
            {
                if ( sIsVisible == ID_TRUE )
                {
                    break;
                }

                s_pCurLNode = s_pNxtLNode;

                if ( ( s_pCurLNode == NULL ) || ( a_pIterator->highest == ID_TRUE ) )
                {
                    s_pCurLNode = NULL;
                    break;
                }
            }

            s_nSlot = 0;
        }

        /* PROJ-2433 */
        a_pIterator->lowFence  = a_pIterator->rows;
        a_pIterator->highFence = ( ( sIsVisible == ID_TRUE ) ? 
                                   ( a_pIterator->rows + ( s_nMax - s_nMin ) ) :
                                   ( a_pIterator->rows - 1 ) );
        a_pIterator->slot      = a_pIterator->rows - 1;
    }
    else
    {
        s_pCurLNode = NULL;
    }

    if ( s_pCurLNode == NULL )
    {
        a_pIterator->least     =
        a_pIterator->highest   = ID_TRUE;
        a_pIterator->slot      = (SChar**)&(a_pIterator->slot);
        a_pIterator->lowFence  = a_pIterator->slot + 1;
        a_pIterator->highFence = a_pIterator->slot - 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( a_pIterator->index->mIndexHeader );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::findLast( smnbHeader       * aIndexHeader,
                            const smiCallBack* a_pCallBack,
                            SInt*              a_pDepth,
                            smnbStack*         a_pStack,
                            SInt*              /*a_pSlot*/ )
{
    SInt        s_nDepth;
    smnbLNode*  s_pCurLNode;

    s_nDepth = -1;

    if ( aIndexHeader->root != NULL )
    {
        IDE_TEST( findLstLeaf( aIndexHeader,
                     a_pCallBack,
                     &s_nDepth,
                     a_pStack,
                     &s_pCurLNode) != IDE_SUCCESS );

        // backward scan(findLast)는 Tree 전체를 Lock 잡기 때문
        IDE_ERROR_RAISE( -1 <= s_nDepth, ERR_CORRUPTED_INDEX_NODE_DEPTH );

        s_nDepth++;
        a_pStack[s_nDepth].node = s_pCurLNode;
    }
    else
    {
        /* nothing to do */
    }

    // BUG-26625 메모리 인덱스 스캔시 SMO를 검출하는 로직에 문제가 있습니다.
    IDE_ERROR_RAISE( (s_nDepth < 0) ||
                     ( SMNB_IS_LEAF_NODE( s_pCurLNode ) ),
                     ERR_CORRUPTED_INDEX_FLAG );

    *a_pDepth = s_nDepth;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_NODE_DEPTH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Node Depth" );
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_FLAG )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Flag" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::findLastSlotInLeaf              *
 * ------------------------------------------------------------------*
 * LEAF NODE 내에서 가장 마지막에 나오는 row의 위치를 찾는다.
 * ( 동일한 key가 여러인경우 가장 오른쪽(마지막)의 slot 위치를 찾음 )
 *
 * case1. 일반 index이거나, partial key index가 아닌경우
 *   case1-1. direct key index인 경우
 *            : 함수 findLastKeyInLeaf() 실행해 direct key 기반으로 검색
 *   case1-2. 일반 index인 경우
 *            : 함수 findLastRowInLeaf() 실행해 row 기반으로 검색
 *
 * case2. partial key index인경우
 *        : 함수 findLastKeyInLeaf() 실행해 direct key 기반으로 검색후
 *          함수 callback() 또는 findLastRowInLeaf() 실행해 재검색
 *
 *  => partial key 에서 재검색이 필요한 이유
 *    1. partial key로 검색한 위치는 정확한 위치가 아닐수있기때문에
 *       full key로 재검색을 하여 확인하여야 한다
 *
 * aHeader   - [IN]  INDEX 헤더
 * aCallBack - [IN]  Range Callback
 * aNode     - [IN]  LEAF NODE
 * aMinimum  - [IN]  검색할 slot범위 최소값
 * aMaximum  - [IN]  검색할 slot범위 최대값
 * aSlot     - [OUT] 찾아진 slot 위치
 *********************************************************************/
inline void smnbBTree::findLastSlotInLeaf( smnbHeader          * aHeader,
                                           const smiCallBack   * aCallBack,
                                           const smnbLNode     * aNode,
                                           SInt                  aMinimum,
                                           SInt                  aMaximum,
                                           SInt                * aSlot )
{
    idBool  sResult;

    if ( aHeader->mIsPartialKey == ID_FALSE )
    {
        /* full direct key */
        if ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE )
        {
            /* 복합키인경우 첫컬럼은key로 뒷컬럼들은 row로처리한다 */
            findLastKeyInLeaf( aHeader,
                               aCallBack,
                               aNode,
                               aMinimum,
                               aMaximum,
                               aSlot );
            IDE_CONT( end );
        }
        /* direct key 없음 */
        else
        {
            findLastRowInLeaf( aCallBack,
                               aNode,
                               aMinimum,
                               aMaximum,
                               aSlot );
            IDE_CONT( end );
        }
    }
    /* partial key */
    else
    {
        findLastKeyInLeaf( aHeader,
                           aCallBack,
                           aNode,
                           aMinimum,
                           aMaximum,
                           aSlot );

        if ( *aSlot < aMinimum )
        {
            /* 범위내에없다. */
            IDE_CONT( end );
        }
        else
        {
            if ( aNode->mRowPtrs[*aSlot] == NULL )
            {
                /* 범위내에없다. */
                IDE_CONT( end );
            }
            else
            {
                /* nothing to do */
            }
        }

        /* partial key라도, 해당 node의 key가 모두 포함된 경우라면, key로 비교한다  */
        if ( isFullKey( aHeader,
                        (smnbNode *)aNode,
                        (SShort)(*aSlot) ) == ID_TRUE )
        {
            aCallBack->callback( &sResult,
                                 aNode->mRowPtrs[*aSlot],
                                 SMNB_GET_KEY_PTR( aNode, *aSlot ),
                                 0,
                                 SC_NULL_GRID,
                                 aCallBack->data );
        }
        else
        {
            aCallBack->callback( &sResult,
                                 aNode->mRowPtrs[*aSlot],
                                 NULL,
                                 0,
                                 SC_NULL_GRID,
                                 aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            /* 찾은 slot과 바로뒤 slot 중 맞는것을 찾기위해 full key로 다시 검색한다 */

            aMinimum = *aSlot;

            if ( aMaximum >= ( *aSlot + 1 ) )
            {
                aMaximum = *aSlot + 1;
            }
            else
            {
                /* 찾은 slot 뒤로 다른 slot이 없다. 찾은 slot이 맞다. */
                IDE_CONT( end );
            }
        }
        else
        {
            aMaximum = *aSlot;
        }

        if ( aMinimum <= aMaximum )
        {
            findLastRowInLeaf( aCallBack,
                               aNode,
                               aMinimum,
                               aMaximum,
                               aSlot );
        }
        else
        {
            /* PROJ-2433
             * node 범위내에 만족하는값이없다. */
            *aSlot = aMinimum - 1;
        }
    }

    IDE_EXCEPTION_CONT( end );

    return;
}

void smnbBTree::findLastRowInLeaf( const smiCallBack   * aCallBack,
                                   const smnbLNode     * aNode,
                                   SInt                  aMinimum,
                                   SInt                  aMaximum,
                                   SInt                * aSlot )
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    SInt              sMedium   = 0;
    idBool            sResult;
    volatile SChar  * sRow      = NULL;

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;
        sRow    = aNode->mRowPtrs[sMedium];

        if ( sRow == NULL )
        {
            sResult = ID_FALSE;
        }
        else
        {
            // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
            IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sRow)->mCreateSCN) );

            (void)aCallBack->callback( &sResult,
                                       (SChar *)sRow,
                                       NULL,
                                       0,
                                       SC_NULL_GRID,
                                       aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            aMinimum = sMedium + 1;
            *aSlot   = (UInt)sMedium;
        }
        else
        {
            aMaximum = sMedium - 1;
            *aSlot   = (UInt)aMaximum;
        }
    }
    while ( aMinimum <= aMaximum );
}

void smnbBTree::findLastKeyInLeaf( smnbHeader          * aHeader,
                                   const smiCallBack   * aCallBack,
                                   const smnbLNode     * aNode,
                                   SInt                  aMinimum,
                                   SInt                  aMaximum,
                                   SInt                * aSlot )
{
    SInt      sMedium           = 0;
    idBool    sResult;
    SChar   * sRow              = NULL;
    void    * sKey              = NULL;
    UInt      sPartialKeySize   = ( aHeader->mIsPartialKey == ID_TRUE ) ? (aNode->mKeySize) : 0;

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;
        sKey    = SMNB_GET_KEY_PTR( aNode, sMedium );
        sRow    = aNode->mRowPtrs[sMedium];
                         
        if ( sRow == NULL )
        {
            sResult = ID_FALSE;
        }
        else
        {
            (void)aCallBack->callback( &sResult,
                                       sRow,
                                       sKey,
                                       sPartialKeySize,
                                       SC_NULL_GRID,
                                       aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            aMinimum = sMedium + 1;
            *aSlot   = (UInt)sMedium;
        }
        else
        {
            aMaximum = sMedium - 1;
            *aSlot   = (UInt)aMaximum;
        }
    }
    while ( aMinimum <= aMaximum );
}

IDE_RC smnbBTree::findLstLeaf(smnbHeader       * aIndexHeader,
                              const smiCallBack* a_pCallBack,
                              SInt*              a_pDepth,
                              smnbStack*         a_pStack,
                              smnbLNode**        a_pLNode)
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    SInt                 s_nSlotCount   = 0;
    SInt                 s_nLstReadPos  = 0;
    SInt                 s_nDepth       = 0;
    SInt                 s_nSlotPos     = 0;
    volatile smnbINode * s_pCurINode    = NULL;
    volatile smnbINode * s_pChildINode  = NULL;

    s_nDepth = *a_pDepth;

    if ( s_nDepth == -1 )
    {
        s_pCurINode   = (smnbINode*)aIndexHeader->root;
        s_nLstReadPos = -1;
    }
    else
    {
        s_pCurINode   = (smnbINode*)(a_pStack->node);
        s_nLstReadPos = a_pStack->lstReadPos;

        a_pStack--;
        s_nDepth--;
    }

    if ( s_pCurINode != NULL )
    {
        while ( SMNB_IS_INTERNAL_NODE( s_pCurINode ) )
        {
            s_nLstReadPos++;

            s_nSlotCount = s_pCurINode->mSlotCount;
            IDE_ERROR_RAISE( s_nSlotCount != 0, ERR_CORRUPTED_INDEX );

            findLastSlotInInternal( aIndexHeader,
                                    a_pCallBack,
                                    (smnbINode*)s_pCurINode,
                                    s_nLstReadPos,
                                    s_nSlotCount - 1,
                                    &s_nSlotPos );

            IDE_DASSERT( s_nSlotPos >= 0 );
            s_pChildINode = (smnbINode*)(s_pCurINode->mChildPtrs[s_nSlotPos]);

            s_nLstReadPos = -1;

            IDE_ERROR_RAISE( ( s_nSlotCount != 0 ) ||
                             ( s_nSlotCount != s_nSlotPos ), ERR_CORRUPTED_INDEX ); /* backward scan(findLast)는 Tree 전체를 Lock 잡기 때문 */

            a_pStack->node       = (smnbINode*)s_pCurINode;
            a_pStack->lstReadPos = s_nSlotPos;

            s_nDepth++;
            a_pStack++;

            s_pCurINode   = s_pChildINode;
        }
    }
    else
    {
        /* nothing to do */
    }

    *a_pDepth = s_nDepth;
    *a_pLNode = (smnbLNode*)s_pCurINode;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index SlotCount" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::findLastSlotInInternal          *
 * ------------------------------------------------------------------*
 * INTERNAL NODE 내에서 가장 마지막에 나오는 row의 위치를 찾는다.
 * ( 동일한 key가 여러인경우 가장 오른쪽(마지막)의 slot 위치를 찾음 )
 *
 * case1. 일반 index이거나, partial key index가 아닌경우
 *   case1-1. direct key index인 경우
 *            : 함수 findLastInInternal() 실행해 direct key 기반으로 검색
 *   case1-2. 일반 index인 경우
 *            : 함수 findLastRowInInternal() 실행해 row 기반으로 검색
 *
 * case2. partial key index인경우
 *        : 함수 findLastInInternal() 실행해 direct key 기반으로 검색후
 *          함수 callback() 또는 findLastRowInInternal() 실행해 재검색
 *
 *  => partial key 에서 재검색이 필요한 이유
 *    1. partial key로 검색한 위치는 정확한 위치가 아닐수있기때문에
 *       full key로 재검색을 하여 확인하여야 한다
 *
 * aHeader   - [IN]  INDEX 헤더
 * aCallBack - [IN]  Range Callback
 * aNode     - [IN]  INTERNAL NODE
 * aMinimum  - [IN]  검색할 slot범위 최소값
 * aMaximum  - [IN]  검색할 slot범위 최대값
 * aSlot     - [OUT] 찾아진 slot 위치
 *********************************************************************/
inline void smnbBTree::findLastSlotInInternal( smnbHeader          * aHeader,
                                               const smiCallBack   * aCallBack,
                                               const smnbINode     * aNode,
                                               SInt                  aMinimum,
                                               SInt                  aMaximum,
                                               SInt                * aSlot )
{
    idBool  sResult;

    if ( aHeader->mIsPartialKey == ID_FALSE )
    {
        /* full direct key */
        if ( SMNB_IS_DIRECTKEY_INDEX( aHeader ) == ID_TRUE )
        {
            /* 복합키인경우 첫컬럼은key로 뒷컬럼들은row로처리한다 */
            findLastKeyInInternal( aHeader,
                                   aCallBack,
                                   aNode,
                                   aMinimum,
                                   aMaximum,
                                   aSlot );
            IDE_CONT( end );
        }
        /* direct key 없음 */
        else
        {
            findLastRowInInternal( aCallBack,
                                   aNode,
                                   aMinimum,
                                   aMaximum,
                                   aSlot );
            IDE_CONT( end );
        }
    }
    else
    {
        /* partial key */
        findLastKeyInInternal( aHeader,
                               aCallBack,
                               aNode,
                               aMinimum,
                               aMaximum,
                               aSlot );

        if ( *aSlot < aMinimum )
        {
            /* 범위내에없다. */
            IDE_CONT( end );
        }
        else
        {
            if ( aNode->mRowPtrs[*aSlot] == NULL )
            {
                /* 범위내에없다. */
                IDE_CONT( end );
            }
            else
            {
                /* nothing to do */
            }
        }

        /* partial key라도, 해당 node의 key가 모두 포함된 경우라면, key로 비교한다  */
        if ( isFullKey( aHeader,
                        (smnbNode *)aNode,
                        (SShort)(*aSlot) ) == ID_TRUE )
        {
            aCallBack->callback( &sResult,
                                 aNode->mRowPtrs[*aSlot],
                                 SMNB_GET_KEY_PTR( aNode, *aSlot ),
                                 0,
                                 SC_NULL_GRID,
                                 aCallBack->data );
        }
        else
        {
            /* 나머지경우는 indirect key로 비교한다 */
            aCallBack->callback( &sResult,
                                 aNode->mRowPtrs[*aSlot],
                                 NULL,
                                 0,
                                 SC_NULL_GRID,
                                 aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            /* 찾은 slot과 바로뒤 slot 중 맞는것을 찾기위해 full key로 다시 검색한다 */

            aMinimum = *aSlot;

            if ( aMaximum >= ( *aSlot + 1 ) )
            {
                aMaximum = *aSlot + 1;
            }
            else
            {
                /* 찾은 slot 뒤로 다른 slot이 없다. 찾은 slot이 맞다. */
                IDE_CONT( end );
            }
        }
        else
        {
            aMaximum = *aSlot;
        }

        if ( aMinimum <= aMaximum )
        {
            findLastRowInInternal( aCallBack,
                                   aNode,
                                   aMinimum,
                                   aMaximum,
                                   aSlot );
        }
        else
        {
            /* PROJ-2433
             * node 범위내에 만족하는값이없다. */
            *aSlot = aMinimum - 1;
        }
    }

    IDE_EXCEPTION_CONT( end );

    return;
}

//BUG-48230
IDE_RC smnbBTree::beforeFirstQ( void* aIterator )
{
    smnbIterator* a_pIterator = (smnbIterator*)aIterator;

    for( a_pIterator->mKeyRange      = a_pIterator->mKeyRange;
         a_pIterator->mKeyRange->prev != NULL;
         a_pIterator->mKeyRange        = a_pIterator->mKeyRange->prev ) ;

    IDE_TEST( beforeFirstInternalQ( a_pIterator ) != IDE_SUCCESS );

    a_pIterator->flag  = a_pIterator->flag & (~SMI_RETRAVERSE_MASK);
    a_pIterator->flag |= SMI_RETRAVERSE_BEFORE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smnbBTree::beforeFirstInternalQ( smnbIterator* a_pIterator )
{
   /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    const smiRange* s_pRange;
    volatile SChar* s_pRow;
    idBool          s_bResult;
    SInt            s_nSlot = -1;
    SInt            s_cSlot;
    SInt            i = 0;
    SInt            s_nMin;
    SInt            s_nMax;
    SInt            sLoop;
    IDU_LATCH       s_version;
    smnbStack       sStack[SMNB_STACK_DEPTH];
    SInt            sDepth = -1;
    smnbLNode*      s_pCurLNode = NULL;
    smnbLNode*      s_pNxtLNode = NULL;
    smnbLNode*      s_pPrvLNode = NULL;
    void          * sKey        = NULL;
    SChar         * sRowPtr     = NULL;

    smSCN           sLockRowSCN = SM_MAKE_SCN_LOCK_ROW( a_pIterator->tid );
    smSCN           sBfrNextSCN;
    smSCN           sRowSCN;
    smpSlotHeader * sSlotHdr;

    s_pRange = a_pIterator->mKeyRange;

    if ( a_pIterator->mProperties->mReadRecordCount > 0 )
    {
        IDE_TEST( findPosition( a_pIterator->index,
                                &s_pRange->minimum,
                                NULL, /* aRow */
                                &sDepth,
                                sStack,
                                &s_pCurLNode ) != IDE_SUCCESS );

        while( (sDepth < 0) && (s_pRange->next != NULL) )
        {
            s_pRange = s_pRange->next;
            IDE_TEST( findPosition( a_pIterator->index,
                                    &s_pRange->minimum,
                                    NULL, /* aRow */
                                    &sDepth,
                                    sStack,
                                    &s_pCurLNode ) != IDE_SUCCESS );

            a_pIterator->mKeyRange = s_pRange;
        }
    }

    if ( ( sDepth >= 0 ) && ( a_pIterator->mProperties->mReadRecordCount > 0 ) )
    {
        while(1)
        {
            s_version   = getLatchValueOfLNode(s_pCurLNode) & (~SMNB_SCAN_LATCH_BIT);
            IDL_MEM_BARRIER;

            s_cSlot      = s_pCurLNode->mSlotCount;
            s_pPrvLNode  = s_pCurLNode->prevSPtr;
            s_pNxtLNode  = s_pCurLNode->nextSPtr;

            if ( s_cSlot == 0 )
            {
                if ( s_version == getLatchValueOfLNode(s_pCurLNode) )
                {
                    s_pCurLNode = s_pNxtLNode;

                    if ( s_pCurLNode == NULL )
                    {
                        break;
                    }
                }

                s_nSlot = 0;
                continue;
            }
            else
            {
                findFirstSlotInNode( a_pIterator->index,
                                     &s_pRange->minimum,
                                     (smnbNode *)s_pCurLNode,
                                     &s_nSlot );
            }

            a_pIterator->version  = s_version;
            a_pIterator->node     = s_pCurLNode;

            a_pIterator->prvNode  = s_pPrvLNode;
            a_pIterator->nxtNode  = s_pNxtLNode;

            a_pIterator->least    = ID_TRUE;
            a_pIterator->highest  = ID_FALSE;

            s_nMin = s_nSlot;
            s_nMax = s_cSlot - 1;

            if ( ( s_nSlot + 1 ) < s_cSlot )
            {
                s_pRow  = a_pIterator->node->mRowPtrs[s_nSlot];

                if ( s_pRow == NULL )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                /*
                 * PROJ-2433
                 * 지금값이 최대값 조건도 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.
                 */
                if ( isFullKey( a_pIterator->index,
                                (smnbNode *)(a_pIterator->node),
                                (SShort)s_nSlot ) == ID_TRUE )
                {
                    sKey = SMNB_GET_KEY_PTR( a_pIterator->node, s_nSlot );
                }
                else
                {
                    sKey = NULL;
                }

#ifdef DEBUG
                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                // BUG-29106 debugging code
                if ( SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN) )
                {
                    int  sSlotIdx;

                    ideLog::log( IDE_SERVER_0, "s_pRow : %lu\n", s_pRow );
                    ideLog::log( IDE_SERVER_0,
                                 "s_nSlot : %d, s_cSlot : %d\n",
                                 s_nSlot, s_cSlot );
                    for ( sSlotIdx = 0 ; sSlotIdx < s_cSlot ; sSlotIdx++ )
                    {
                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar *)(a_pIterator->node->mRowPtrs[sSlotIdx]),
                                        64 );
                    }
                    ideLog::log( IDE_SERVER_0, "s_pCurLNode : %lu\n", s_pCurLNode);
                    ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator->node,
                                    mNodeSize );
                    ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator,
                                    mIteratorSize );
                    IDE_DASSERT( 0 );
                }
#endif

                s_pRange->maximum.callback( &s_bResult,
                                            (SChar*)s_pRow,
                                            sKey,
                                            0,
                                            SC_NULL_GRID,
                                            s_pRange->maximum.data );

                if ( s_bResult == ID_TRUE )
                {
                    if ( ( s_nSlot + 2 ) < s_cSlot )
                    {
                        s_pRow = s_pCurLNode->mRowPtrs[s_nSlot + 1];

                        if ( s_pRow == NULL )
                        {
                            continue;
                        }

                        /* PROJ-2433
                         * 최대값 조건에 만족하는지 확인하는부분이다.
                         * direct key가 full key여야만 제대로된 비교가 가능하다.
                         */
                        if ( isFullKey( a_pIterator->index,
                                        (smnbNode *)s_pCurLNode,
                                        (SShort)( s_nSlot + 1 ) ) == ID_TRUE )
                        {
                            sKey = SMNB_GET_KEY_PTR( s_pCurLNode, s_nSlot + 1 );
                        }
                        else
                        {
                            sKey = NULL;
                        }

#ifdef DEBUG
                        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                        // BUG-29106 debugging code
                        if ( SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN) )
                        {
                            int sSlotIdx;

                            ideLog::log( IDE_SERVER_0, "s_pRow : %lu\n", s_pRow );
                            ideLog::log( IDE_SERVER_0,
                                         "s_nSlot : %d, s_cSlot : %d\n",
                                         s_nSlot, s_cSlot );
                            for ( sSlotIdx = 0 ; sSlotIdx < s_cSlot ; sSlotIdx++ )
                            {
                                ideLog::logMem( IDE_SERVER_0,
                                                (UChar *)(s_pCurLNode->mRowPtrs[sSlotIdx]),
                                                64 );
                            }
                            ideLog::log( IDE_SERVER_0, "s_pCurLNode : %lu\n", s_pCurLNode);
                            ideLog::logMem( IDE_SERVER_0, (UChar *)s_pCurLNode,
                                            mNodeSize );
                            ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator,
                                            mIteratorSize );
                            IDE_DASSERT( 0 );
                        }
                        else
                        {
                            /* nothing to do */
                        }
#endif

                        s_pRange->maximum.callback( &s_bResult,
                                                    (SChar*)s_pRow,
                                                    sKey,
                                                    0,
                                                    SC_NULL_GRID,
                                                    s_pRange->maximum.data);

                        if ( s_bResult == ID_FALSE )
                        {
                            a_pIterator->highest   = ID_TRUE;
                            s_nMax                 = s_nSlot;
                        }
                        else
                        {
                            /* nothing to do */
                        }
                    }
                }
                else
                {
                    a_pIterator->highest  = ID_TRUE;
                    s_nMax                = -1;
                }
            }

            if ( a_pIterator->highest == ID_FALSE )
            {
                s_pRow = s_pCurLNode->mRowPtrs[s_cSlot - 1];

                if ( s_pRow == NULL )
                {
                    continue;
                }
                else
                {
                    /* nothing to do */
                }

                /*
                 * PROJ-2433
                 * 최대값 조건에 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.
                 */
                if ( isFullKey( a_pIterator->index,
                                (smnbNode *)s_pCurLNode,
                                SShort( s_cSlot - 1 ) ) == ID_TRUE )
                {
                    sKey = SMNB_GET_KEY_PTR( s_pCurLNode, s_cSlot - 1 );
                }
                else
                {
                    sKey = NULL;
                }

#ifdef DEBUG
                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                // BUG-29106 debugging code
                if ( SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN) )
                {
                    int sSlotIdx;

                    ideLog::log( IDE_SERVER_0, "s_pRow : %lu\n", s_pRow );
                    ideLog::log( IDE_SERVER_0,
                                 "s_nSlot : %d, s_cSlot : %d\n",
                                 s_nSlot, s_cSlot );
                    for( sSlotIdx = 0; sSlotIdx < s_cSlot; sSlotIdx ++ )
                    {
                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar *)(s_pCurLNode->mRowPtrs[sSlotIdx]),
                                        64 );
                    }
                    ideLog::log( IDE_SERVER_0, "s_pCurLNode : %lu\n", s_pCurLNode);
                    ideLog::logMem( IDE_SERVER_0, (UChar *)s_pCurLNode,
                                    mNodeSize );
                    ideLog::logMem( IDE_SERVER_0, (UChar *)a_pIterator,
                                    mIteratorSize );
                    IDE_DASSERT( 0 );
                }
#endif

                s_pRange->maximum.callback( &s_bResult,
                                            (SChar*)s_pRow,
                                            sKey,
                                            0,
                                            SC_NULL_GRID,
                                            s_pRange->maximum.data );
                if ( s_bResult == ID_TRUE )
                {
                    s_nMax = s_cSlot - 1;
                }
                else
                {
                    smnbBTree::findLastSlotInLeaf( a_pIterator->index,
                                                   &(s_pRange->maximum),
                                                   s_pCurLNode,
                                                   0,
                                                   s_cSlot - 1,
                                                   &s_nSlot );

                    a_pIterator->highest   = ID_TRUE;
                    s_nMax = s_nSlot;
                }
            }

            /* BUG-44043
               fetchNext()가 최초로 호출될때 scn을 만족하는 row가 하나라도 있어야
               aIterator->lstReturnRecPtr 값으로 세팅되어서,
               Key Redistribution이 발생해도 다음노드들을 탐색해서 정확한 다음 row를 찾아낼수있다. */
            i = -1;
            for ( sLoop = s_nMin; sLoop <= s_nMax; sLoop++ )
            {
                sRowPtr = s_pCurLNode->mRowPtrs[sLoop];

                if ( sRowPtr == NULL )
                {
                    break;
                }
                
                sRowSCN = ((smpSlotHeader*)sRowPtr)->mCreateSCN;
                if ( SM_SCN_IS_INFINITE( sRowSCN ) )
                {
                    if ( SMP_GET_TID( sRowSCN ) == a_pIterator->tid )
                    {
                        i++;
                        a_pIterator->rows[i] = sRowPtr; 
                    }
                }
                else{
                    if ( SM_SCN_IS_FREE_ROW( ((smpSlotHeader*)sRowPtr)->mLimitSCN ) )
                    {
                        i++;
                        a_pIterator->rows[i] = sRowPtr;           
                    }    
                }
            }

            if ( s_version == getLatchValueOfLNode(s_pCurLNode) )
            {
                if ( i != -1 )
                {
                    sBfrNextSCN = SM_SCN_INIT;
                    for ( sLoop = 0 ; sLoop <= i ; sLoop++ )
                    {
                        sSlotHdr = (smpSlotHeader*) a_pIterator->rows[sLoop];

                        SC_MAKE_GRID( a_pIterator->mRowGRID,
                                      a_pIterator->table->mSpaceID,
                                      SMP_SLOT_GET_PID( sSlotHdr ),
                                      SMP_SLOT_GET_OFFSET( sSlotHdr ) );

                        IDE_TEST( a_pIterator->mRowFilter->callback( &s_bResult,
                                                                     sSlotHdr,
                                                                     NULL,
                                                                     0,
                                                                     a_pIterator->mRowGRID,
                                                                     a_pIterator->mRowFilter->data )
                                  != IDE_SUCCESS );
                        if ( s_bResult == ID_TRUE )
                        {
                            sBfrNextSCN = idCore::acpAtomicCas64( &(sSlotHdr->mLimitSCN),
                                                                  sLockRowSCN,
                                                                  SM_SCN_FREE_ROW );
                            if ( sBfrNextSCN == SM_SCN_FREE_ROW )
                            {
                                IDE_TEST( smLayerCallback::addOID( (smxTrans*)(a_pIterator->trans),
                                                                   a_pIterator->table->mSelfOID,
                                                                   SMI_CHANGE_GRID_TO_OID( a_pIterator->mRowGRID ),
                                                                   a_pIterator->table->mSpaceID,
                                                                   SM_OID_TYPE_UNLOCK_FIXED_SLOT )
                                          != IDE_SUCCESS);

                                a_pIterator->rows[0] = a_pIterator->rows[sLoop] ;
                                a_pIterator->lowFence  = a_pIterator->rows;
                                a_pIterator->highFence = a_pIterator->rows;
                                a_pIterator->slot      = a_pIterator->rows - 1;
                                return IDE_SUCCESS;
                            }
                        }
                    }
                }
                else
                {
                    /* nothing to do */
                }

                s_pCurLNode = s_pNxtLNode;

                if ( ( s_pCurLNode == NULL ) || ( a_pIterator->highest == ID_TRUE ) )
                {
                    s_pCurLNode = NULL;
                    break;
                }
            }

            s_nSlot = 0;
        }

        /* PROJ-2433 */
        a_pIterator->lowFence  = a_pIterator->rows;
        a_pIterator->highFence = a_pIterator->rows + i;
        a_pIterator->slot      = a_pIterator->rows - 1;
    }
    else
    {
        s_pCurLNode = NULL;
    }

    if ( s_pCurLNode == NULL )
    {
        a_pIterator->least     =
        a_pIterator->highest   = ID_TRUE;
        a_pIterator->slot      = (SChar**)&(a_pIterator->slot);
        a_pIterator->lowFence  = a_pIterator->slot + 1;
        a_pIterator->highFence = a_pIterator->slot - 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( a_pIterator->index->mIndexHeader );
    }

    return IDE_FAILURE;

}

void smnbBTree::findLastRowInInternal( const smiCallBack   * aCallBack,
                                       const smnbINode     * aNode,
                                       SInt                  aMinimum,
                                       SInt                  aMaximum,
                                       SInt                * aSlot )
{
    SInt      sMedium   = 0;
    idBool    sResult;
    SChar   * sRow      = NULL;

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;
        sRow    = aNode->mRowPtrs[sMedium];

        if ( sRow == NULL )
        {
            sResult = ID_FALSE;
        }
        else
        {
            // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
            IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sRow)->mCreateSCN) );

            (void)aCallBack->callback( &sResult,
                                       sRow,
                                       NULL,
                                       0,
                                       SC_NULL_GRID,
                                       aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            aMinimum = sMedium + 1;
            *aSlot   = (UInt)aMinimum;
        }
        else
        {
            aMaximum = sMedium - 1;
            *aSlot   = (UInt)sMedium;
        }
    }
    while ( aMinimum <= aMaximum );
}

void smnbBTree::findLastKeyInInternal( smnbHeader          * aHeader,
                                       const smiCallBack   * aCallBack,
                                       const smnbINode     * aNode,
                                       SInt                  aMinimum,
                                       SInt                  aMaximum,
                                       SInt                * aSlot )
{
    SInt      sMedium           = 0;
    idBool    sResult;
    SChar   * sRow              = NULL;
    void    * sKey              = NULL;
    UInt      sPartialKeySize   = ( aHeader->mIsPartialKey == ID_TRUE ) ? (aNode->mKeySize) : 0;

    do
    {
        sMedium = (aMinimum + aMaximum) >> 1;
        sKey    = SMNB_GET_KEY_PTR( aNode, sMedium );
        sRow    = aNode->mRowPtrs[sMedium];
                         
        if ( sRow == NULL )
        {
            sResult = ID_FALSE;
        }
        else
        {
            (void)aCallBack->callback( &sResult,
                                       sRow, /* composite key index의 첫번째이후 컬럼을 비교하기위해서*/
                                       sKey,
                                       sPartialKeySize,
                                       SC_NULL_GRID,
                                       aCallBack->data );
        }

        if ( sResult == ID_TRUE )
        {
            aMinimum = sMedium + 1;
            *aSlot   = (UInt)aMinimum;
        }
        else
        {
            aMaximum = sMedium - 1;
            *aSlot   = (UInt)sMedium;
        }
    }
    while ( aMinimum <= aMaximum );
}

IDE_RC smnbBTree::afterLast( smnbIterator* a_pIterator,
                             const smSeekFunc** )
{
    for( a_pIterator->mKeyRange        = a_pIterator->mKeyRange;
         a_pIterator->mKeyRange->next != NULL;
         a_pIterator->mKeyRange        = a_pIterator->mKeyRange->next ) ;

    IDE_TEST( afterLastInternal( a_pIterator ) != IDE_SUCCESS );

    a_pIterator->flag  = a_pIterator->flag & (~SMI_RETRAVERSE_MASK);
    a_pIterator->flag |= SMI_RETRAVERSE_AFTER;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*BUG-48230: afterLast는 인덱스 전체에 Lock을 잡아야 하므로
 *           beforeFirstInternalQ 처럼 최적화하는 것이 의미
 *           없다.
 */
IDE_RC smnbBTree::afterLastQ( void* aIterator )
{
    smnbIterator* a_pIterator = (smnbIterator*)aIterator;

    for( a_pIterator->mKeyRange        = a_pIterator->mKeyRange;
         a_pIterator->mKeyRange->next != NULL;
         a_pIterator->mKeyRange        = a_pIterator->mKeyRange->next ) ;

    IDE_TEST( afterLastInternal( a_pIterator ) != IDE_SUCCESS );

    a_pIterator->flag  = a_pIterator->flag & (~SMI_RETRAVERSE_MASK);
    a_pIterator->flag |= SMI_RETRAVERSE_AFTER;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::afterLastInternal( smnbIterator* a_pIterator )
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    const smiRange* s_pRange;
    idBool          s_bResult;
    SInt            s_nSlot;
    SInt            i = 0;
    SInt            s_nMin;
    SInt            s_nMax;
    SInt            s_cSlot;
    volatile SChar* s_pRow;
    idBool          s_bNext;
    IDU_LATCH       s_version;
    smnbStack       sStack[SMNB_STACK_DEPTH];
    SInt            sDepth = -1;
    smnbLNode*      s_pCurLNode = NULL;
    smnbLNode*      s_pNxtLNode = NULL;
    smnbLNode*      s_pPrvLNode = NULL;
    void          * sKey        = NULL;

    lockTree( a_pIterator->index );

    s_pRange = a_pIterator->mKeyRange;

    if ( a_pIterator->mProperties->mReadRecordCount > 0 )
    {
        IDE_TEST( smnbBTree::findLast( a_pIterator->index,
                                       &(s_pRange->maximum),
                                       &sDepth,
                                       sStack,
                                       &s_nSlot ) != IDE_SUCCESS );

        while( (sDepth < 0) && (s_pRange->prev != NULL) )
        {
            s_pRange = s_pRange->prev;
            IDE_TEST( smnbBTree::findLast( a_pIterator->index,
                                           &(s_pRange->maximum),
                                           &sDepth,
                                           sStack,
                                           &s_nSlot ) != IDE_SUCCESS );
        }
    }

    a_pIterator->mKeyRange = s_pRange;

    if ( ( sDepth >= 0 ) && ( a_pIterator->mProperties->mReadRecordCount >  0 ) )
    {
        s_pCurLNode = (smnbLNode*)(sStack[sDepth].node);
        s_bNext     = ID_TRUE;

        while(1)
        {
            s_version = getLatchValueOfLNode(s_pCurLNode) & (~SMNB_SCAN_LATCH_BIT);
            IDL_MEM_BARRIER;

            s_cSlot      = s_pCurLNode->mSlotCount;
            s_pNxtLNode  = s_pCurLNode->nextSPtr;
            s_pPrvLNode  = s_pCurLNode->prevSPtr;

            IDE_ERROR_RAISE( s_cSlot > 0, ERR_CORRUPTED_INDEX );

            findLastSlotInLeaf( a_pIterator->index,
                                &s_pRange->maximum,
                                s_pCurLNode,
                                0,
                                s_cSlot - 1,
                                &s_nSlot);

            a_pIterator->version   = s_version;
            a_pIterator->node      = s_pCurLNode;
            a_pIterator->prvNode   = s_pCurLNode->prevSPtr;
            a_pIterator->nxtNode   = s_pCurLNode->nextSPtr;
            a_pIterator->least     = ID_FALSE;
            a_pIterator->highest   = ID_TRUE;

            s_nMin = 0;
            s_nMax = s_nSlot;

            if ( s_nSlot > 0 )
            {
                s_pRow = a_pIterator->node->mRowPtrs[s_nSlot];
                
                if ( s_pRow == NULL )
                {
                    continue;
                }

                /*
                 * PROJ-2433
                 * 지금값이 최소값 조건도 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.
                 */ 
                if ( isFullKey( a_pIterator->index,
                                (smnbNode *)(a_pIterator->node),
                                (SShort)s_nSlot ) == ID_TRUE )
                {
                    sKey = SMNB_GET_KEY_PTR( a_pIterator->node, s_nSlot );
                }
                else
                {
                    sKey = NULL;
                }

                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN ));

                /* PROJ-2433 */
                s_pRange->minimum.callback( &s_bResult,
                                            (SChar*)s_pRow,
                                            sKey,
                                            0,
                                            SC_NULL_GRID,
                                            s_pRange->minimum.data );
                if ( s_bResult == ID_TRUE )
                {
                    if ( s_nSlot > 1 )
                    {
                        s_pRow = a_pIterator->node->mRowPtrs[s_nSlot - 1];

                        if ( s_pRow == NULL )
                        {
                            continue;
                        }

                        /* PROJ-2433
                         * 지금값이 최소값 조건도 만족하는지 확인하는부분이다.
                         * direct key가 full key여야만 제대로된 비교가 가능하다. */
                        if ( isFullKey( a_pIterator->index,
                                        (smnbNode *)a_pIterator->node,
                                        (SShort)( s_nSlot - 1 ) ) == ID_TRUE )
                        {
                            sKey = SMNB_GET_KEY_PTR( a_pIterator->node, s_nSlot - 1 );
                        }
                        else
                        {
                            sKey = NULL;
                        }

                        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                        IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN ));

                        /* PROJ-2433 */
                        s_pRange->minimum.callback( &s_bResult,
                                                    (SChar*)s_pRow,
                                                    sKey,
                                                    0,
                                                    SC_NULL_GRID,
                                                    s_pRange->minimum.data );
                        if ( s_bResult == ID_FALSE )
                        {
                            a_pIterator->least    = ID_TRUE;
                            s_nMin                = s_nSlot;
                        }
                    }
                }
                else
                {
                    a_pIterator->least = ID_TRUE;
                    s_nMin = s_nSlot + 1;
                }
            }

            if ( a_pIterator->least == ID_FALSE )
            {
                s_pRow = a_pIterator->node->mRowPtrs[0];

                if ( s_pRow == NULL )
                {
                    continue;
                }

                /* PROJ-2433
                 * 지금값이 최소값 조건도 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다. */
                if ( isFullKey( a_pIterator->index,
                                (smnbNode *)(a_pIterator->node),
                                0 ) == ID_TRUE )
                {
                    sKey = SMNB_GET_KEY_PTR( a_pIterator->node, 0 );
                }
                else
                {
                    sKey = NULL;
                }

                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)s_pRow)->mCreateSCN ));

                /* PROJ-2433 */
                s_pRange->minimum.callback( &s_bResult,
                                            (SChar*)s_pRow,
                                            sKey,
                                            0,
                                            SC_NULL_GRID,
                                            s_pRange->minimum.data );
                if ( s_bResult == ID_TRUE )
                {
                    a_pIterator->least    = ID_FALSE;
                    s_nMin                = 0;
                }
                else
                {
                    smnbBTree::findFirstSlotInNode( a_pIterator->index,
                                                    &(s_pRange->minimum),
                                                    (smnbNode *)(a_pIterator->node),
                                                    &s_nSlot );

                    a_pIterator->least  = ID_TRUE;
                    s_nMin              = s_nSlot;
                }
            }

            IDU_FIT_POINT( "1.BUG-31890@smnbBTree::afterLastInternal" );

            /* PROJ-2433 */
            if ( ( s_nMin >= 0 ) &&
                 ( s_nMax - s_nMin >= 0 ) )
            {
                idlOS::memcpy( &(a_pIterator->rows[0]),
                               &(a_pIterator->node->mRowPtrs[s_nMin]),
                               (s_nMax - s_nMin + 1) * ID_SIZEOF(a_pIterator->node->mRowPtrs[0]) );
                i = s_nMax - s_nMin;
            }
            else
            {
                i = -1;
            }

            a_pIterator->lowFence  = a_pIterator->rows;
            a_pIterator->highFence = a_pIterator->rows + i;
            a_pIterator->slot      = a_pIterator->highFence + 1;

            if ( s_version == getLatchValueOfLNode(s_pCurLNode) )
            {
                if ( i != -1 )
                {
                    break;
                }
                else
                {
                    /* nothing to do */
                }

                IDE_ERROR_RAISE( s_cSlot != 0, ERR_CORRUPTED_INDEX );

                if ( s_nSlot < s_cSlot - 1)
                {
                    s_bNext = ID_FALSE;
                }
                else
                {
                    /* nothing to do */
                }

                if ( ( s_bNext == ID_TRUE ) && ( s_pNxtLNode != NULL ) )
                {
                    s_pCurLNode = s_pNxtLNode;
                }
                else
                {
                    s_bNext = ID_FALSE;
                    s_pCurLNode = s_pPrvLNode;
                }

                if ( ( s_pCurLNode == NULL ) || ( a_pIterator->least == ID_TRUE ) )
                {
                    s_pCurLNode = NULL;
                    break;
                }
                else
                {
                    /* nothing to do */
                }
            }
            else
            {
                /* nothing to do */
            }

            s_nSlot   = 0;
        }//while
    }

    if ( s_pCurLNode == NULL )
    {
        a_pIterator->least     =
        a_pIterator->highest   = ID_TRUE;
        a_pIterator->slot      = (SChar**)&a_pIterator->slot;
        a_pIterator->lowFence  = a_pIterator->slot + 1;
        a_pIterator->highFence = a_pIterator->slot - 1;
    }

    unlockTree( a_pIterator->index );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Slot Position" );
    }
    IDE_EXCEPTION_END;

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( a_pIterator->index->mIndexHeader );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::beforeFirstU( smnbIterator*       aIterator,
                                const smSeekFunc** aSeekFunc )
{
    IDE_TEST( smnbBTree::beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::afterLastU( smnbIterator*       aIterator,
                              const smSeekFunc** aSeekFunc )
{
    IDE_TEST( smnbBTree::afterLast( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::beforeFirstR( smnbIterator*       aIterator,
                                const smSeekFunc** aSeekFunc )
{
    smnbIterator        *sIterator;
    smiCursorProperties sCursorProperty;
    UInt                sState = 0;
    ULong               sIteratorBuffer[ SMI_ITERATOR_SIZE / ID_SIZEOF(ULong) ];

    sIterator = (smnbIterator *)sIteratorBuffer;
    IDE_TEST( allocIterator( (void**)&sIterator ) != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smnbBTree::beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );
    sIterator->curRecPtr       = aIterator->curRecPtr;
    sIterator->lstFetchRecPtr  = aIterator->lstFetchRecPtr;
    sIterator->lstReturnRecPtr = aIterator->lstReturnRecPtr;
    sIterator->least           = aIterator->least;
    sIterator->highest         = aIterator->highest;
    sIterator->mKeyRange       = aIterator->mKeyRange;
    sIterator->node            = aIterator->node;
    sIterator->nxtNode         = aIterator->nxtNode;
    sIterator->prvNode         = aIterator->prvNode;
    sIterator->slot           = aIterator->slot;
    sIterator->lowFence       = aIterator->lowFence;
    sIterator->highFence      = aIterator->highFence;
    sIterator->flag           = aIterator->flag;

    /* (BUG-45368) 전체 복사하지않고 변경되는 값만 저장후 복원. */
    sCursorProperty.mReadRecordCount    = aIterator->mProperties->mReadRecordCount;
    sCursorProperty.mFirstReadRecordPos = aIterator->mProperties->mFirstReadRecordPos;

    /* (BUG-45368) node cache 전체를 복사하지 않고 사용된 만큼만 저장후 복원. */
    if ( aIterator->highFence >= aIterator->lowFence )
    {
        idlOS::memcpy( sIterator->rows,
                       aIterator->rows,
                       ( ( (void **)aIterator->highFence - (void **)aIterator->lowFence + 1 ) *
                         ID_SIZEOF(aIterator->rows) ) );
    }
    else
    {
        /* nothing to do */
    }

    IDE_TEST( smnbBTree::fetchNextR( aIterator ) != IDE_SUCCESS );

    aIterator->curRecPtr       = sIterator->curRecPtr;
    aIterator->lstFetchRecPtr  = sIterator->lstFetchRecPtr;
    aIterator->lstReturnRecPtr = sIterator->lstReturnRecPtr;
    aIterator->least           = sIterator->least;
    aIterator->highest         = sIterator->highest;
    aIterator->mKeyRange       = sIterator->mKeyRange;
    aIterator->node            = sIterator->node;
    aIterator->nxtNode         = sIterator->nxtNode;
    aIterator->prvNode         = sIterator->prvNode;
    aIterator->slot           = sIterator->slot;
    aIterator->lowFence       = sIterator->lowFence;
    aIterator->highFence      = sIterator->highFence;
    aIterator->flag           = sIterator->flag;

    aIterator->mProperties->mReadRecordCount    = sCursorProperty.mReadRecordCount;
    aIterator->mProperties->mFirstReadRecordPos = sCursorProperty.mFirstReadRecordPos;

    if ( sIterator->highFence >= sIterator->lowFence )
    {
        idlOS::memcpy( aIterator->rows,
                       sIterator->rows,
                       ( ( (void **)sIterator->highFence - (void **)sIterator->lowFence + 1 ) *
                         ID_SIZEOF(sIterator->rows) ) );
    }
    else
    {
        /* nothing to do */
    }

    *aSeekFunc += 6;

    sState = 0;
    (void)freeIterator( sIterator ); /* always IDE_SUCCESS return */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        (void)freeIterator( sIterator ); /* always IDE_SUCCESS return */
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::afterLastR( smnbIterator*       aIterator,
                              const smSeekFunc** aSeekFunc )
{
    IDE_TEST( smnbBTree::beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    IDE_TEST( smnbBTree::fetchNextR( aIterator ) != IDE_SUCCESS );

    IDE_TEST( smnbBTree::afterLast( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    aIterator->flag  = aIterator->flag & (~SMI_RETRAVERSE_MASK);
    aIterator->flag |= SMI_RETRAVERSE_AFTER;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::fetchNext( smnbIterator    * a_pIterator,
                             const void     ** aRow )
{
    idBool     sIsRestart;
    idBool     sResult;
    idBool     sCanReusableRollback;
    smxTrans * sTrans = (smxTrans*)a_pIterator->trans;
    idBool     sIsVisible;

restart:

    for( a_pIterator->slot++;
         a_pIterator->slot <= a_pIterator->highFence;
         a_pIterator->slot++ )
    {
        sCanReusableRollback = ID_TRUE;

        a_pIterator->curRecPtr      = *a_pIterator->slot;
        a_pIterator->lstFetchRecPtr = a_pIterator->curRecPtr;

        /* PROJ-2433 
         * NULL체크 checkSCN 위치변경*/
        if ( a_pIterator->curRecPtr == NULL )
        {
            a_pIterator->highFence = a_pIterator->slot - 1;
            break;
        }
        else
        {
            /* nothing to do */
        }
        IDE_TEST( smnManager::checkSCN( (smiIterator*)a_pIterator,
                                        a_pIterator->curRecPtr,
                                        &sCanReusableRollback,
                                        &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW
                     (((smpSlotHeader *)(a_pIterator->curRecPtr))->mCreateSCN) );

        *aRow = a_pIterator->curRecPtr;
        a_pIterator->lstReturnRecPtr = a_pIterator->curRecPtr;

        SC_MAKE_GRID( a_pIterator->mRowGRID,
                      a_pIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST( a_pIterator->mRowFilter->callback( &sResult,
                                                     *aRow,
                                                     NULL,
                                                     0,
                                                     a_pIterator->mRowGRID,
                                                     a_pIterator->mRowFilter->data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            if( sCanReusableRollback == ID_FALSE )
            {
                sTrans->mIsReusableRollback = ID_FALSE;
            }

            if ( a_pIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if ( a_pIterator->mProperties->mReadRecordCount != 1 )
                {
                    a_pIterator->mProperties->mReadRecordCount--;

                    return IDE_SUCCESS;
                }
                else
                {
                    a_pIterator->mProperties->mReadRecordCount--;

                    a_pIterator->highFence = a_pIterator->slot;
                    a_pIterator->highest   = ID_TRUE;

                    return IDE_SUCCESS;
                }
            }
            else
            {
                a_pIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    if ( a_pIterator->highest == ID_FALSE )
    {
        sIsRestart = ID_FALSE;

        IDE_TEST( getNextNode( a_pIterator,
                               &sIsRestart ) != IDE_SUCCESS );

        if ( sIsRestart == ID_TRUE )
        {
            goto restart;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    a_pIterator->highest = ID_TRUE;

    if ( a_pIterator->mKeyRange->next != NULL )
    {
        a_pIterator->mKeyRange = a_pIterator->mKeyRange->next;
        (void)beforeFirstInternal( a_pIterator );

        IDE_TEST( iduCheckSessionEvent(a_pIterator->mProperties->mStatistics) != IDE_SUCCESS );

        goto restart;
    }
    else
    {
        /* nothing to do */
    }

    a_pIterator->slot            = a_pIterator->highFence + 1;
    a_pIterator->curRecPtr       = NULL;
    a_pIterator->lstFetchRecPtr  = NULL;
    a_pIterator->lstReturnRecPtr = NULL;
    SC_MAKE_NULL_GRID( a_pIterator->mRowGRID );
    *aRow                        = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::fetchNextQ( smnbIterator    * a_pIterator,
                              const void     ** aRow )
{
    idBool     sIsRestart;
    idBool     sResult;
    idBool     sCanReusableRollback;
    smxTrans * sTrans = (smxTrans*)a_pIterator->trans;
    smpSlotHeader     * sSlotHeader;
    ULong               sNxtSCN;
    smTID               sNxtTID;
    UInt sState = 0;

restart:

    for( a_pIterator->slot++;
         a_pIterator->slot <= a_pIterator->highFence;
         a_pIterator->slot++ )
    {
        sCanReusableRollback = ID_TRUE;

        a_pIterator->curRecPtr      = *a_pIterator->slot;
        a_pIterator->lstFetchRecPtr = a_pIterator->curRecPtr;

        /* PROJ-2433
         * NULL체크 checkSCN 위치변경*/
        if ( a_pIterator->curRecPtr == NULL )
        {
            a_pIterator->highFence = a_pIterator->slot - 1;
            sState |= 1;
            break;
        }
        else
        {
            /* nothing to do */
        }

        sSlotHeader                 = (smpSlotHeader*)a_pIterator->curRecPtr;

        /* PROJ-2433
         * NULL체크 checkSCN 위치변경*/
        IDE_ASSERT( sSlotHeader != NULL );

        SMX_GET_SCN_AND_TID( sSlotHeader->mLimitSCN, sNxtSCN, sNxtTID );
        if(( SM_SCN_IS_NOT_LOCK_ROW( sNxtSCN ) ) ||
           ( a_pIterator->tid != sNxtTID ))
        {
            sState |= 2;
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW
                     (((smpSlotHeader *)(a_pIterator->curRecPtr))->mCreateSCN) );

        *aRow = a_pIterator->curRecPtr;
        a_pIterator->lstReturnRecPtr = a_pIterator->curRecPtr;

        SC_MAKE_GRID( a_pIterator->mRowGRID,
                      a_pIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST( a_pIterator->mRowFilter->callback( &sResult,
                                                     *aRow,
                                                     NULL,
                                                     0,
                                                     a_pIterator->mRowGRID,
                                                     a_pIterator->mRowFilter->data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            if( sCanReusableRollback == ID_FALSE )
            {
                sTrans->mIsReusableRollback = ID_FALSE;
            }

            if ( a_pIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if ( a_pIterator->mProperties->mReadRecordCount != 1 )
                {
                    a_pIterator->mProperties->mReadRecordCount--;

                    return IDE_SUCCESS;
                }
                else
                {
                    a_pIterator->mProperties->mReadRecordCount--;

                    a_pIterator->highFence = a_pIterator->slot;
                    a_pIterator->highest   = ID_TRUE;

                    return IDE_SUCCESS;
                }
            }
            else
            {
                a_pIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    if ( a_pIterator->highest == ID_FALSE )
    {
        sIsRestart = ID_FALSE;

        IDE_TEST( getNextNode( a_pIterator,
                               &sIsRestart ) != IDE_SUCCESS );

        if ( sIsRestart == ID_TRUE )
        {
            goto restart;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    a_pIterator->highest = ID_TRUE;

    if ( a_pIterator->mKeyRange->next != NULL )
    {
        a_pIterator->mKeyRange = a_pIterator->mKeyRange->next;
        (void)beforeFirstInternal( a_pIterator );

        IDE_TEST( iduCheckSessionEvent(a_pIterator->mProperties->mStatistics) != IDE_SUCCESS );

        goto restart;
    }
    else
    {
        /* nothing to do */
    }

    a_pIterator->slot            = a_pIterator->highFence + 1;
    a_pIterator->curRecPtr       = NULL;
    a_pIterator->lstFetchRecPtr  = NULL;
    a_pIterator->lstReturnRecPtr = NULL;
    SC_MAKE_NULL_GRID( a_pIterator->mRowGRID );
    *aRow                        = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::getNextNode( smnbIterator   * aIterator,
                               idBool         * aIsRestart )
{
    idBool        sResult;
    SInt          sSlotCount  = 0;
    SInt          i           = -1;
    IDU_LATCH     sVersion    = IDU_LATCH_UNLOCKED;
    smnbLNode   * sCurLNode   = NULL;
    smnbLNode   * sNxtLNode   = NULL;
    SChar       * sRowPtr     = NULL;
    void        * sKey        = NULL;
    SChar       * sLastReadRow  = NULL;     /* 이전 노드에서 마지막으로 읽은 row pointer */
    idBool        isSetRowCache = ID_FALSE;
    SInt          sMin;
    SInt          sMax;
    smnbLNode   * sNode         = NULL;

    IDE_DASSERT( aIterator->highest == ID_FALSE );
    *aIsRestart = ID_FALSE;

    sCurLNode = aIterator->nxtNode;

    sLastReadRow = aIterator->lstReturnRecPtr;

    IDU_FIT_POINT( "smnbBTree::getNextNode::wait" );

    /* BUG-44043 

       Key redistribution의 영향으로
       이전 노드에서 마지막 읽었던 row ( = aIterator->lstReturnRecPtr ) 가
       현재노드 또는 다음노드에 존재할수있다.

       aIterator->lstReturnRecPtr 보다 큰 첫번째 row 위치를 발견할때까지
       next node를 탐색한다.
     */ 

    while ( sCurLNode != NULL )
    {
        while ( 1 )
        {
            sVersion = getLatchValueOfLNode(sCurLNode) & IDU_LATCH_UNMASK;
            IDL_MEM_BARRIER;

            isSetRowCache = ID_FALSE;
            sSlotCount    = sCurLNode->mSlotCount;
            sNxtLNode     = sCurLNode->nextSPtr;

            if ( sSlotCount == 0 )
            {
                IDL_MEM_BARRIER;
                if ( sVersion == getLatchValueOfLNode( sCurLNode ) )
                {
                    sCurLNode = sNxtLNode;
                    break;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                /* nothing to do */
            }

            findNextSlotInLeaf( aIterator->index,
                                sCurLNode,
                                0,
                                sSlotCount - 1,
                                sLastReadRow,
                                &sMin );

            if ( sMin >= sSlotCount )
            { 
                /* 현재노드에서 sLastReadRow를 찾지못한경우,
                   다음노드에서 다시 찾도록 한다. */
                if ( sVersion == getLatchValueOfLNode( sCurLNode ) )
                {
                    sCurLNode = sNxtLNode;
                    break;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                /* nothing to do */
            }

            i = -1;

            aIterator->highest = ID_FALSE;
            aIterator->least   = ID_FALSE;
            aIterator->node    = sCurLNode;
            aIterator->prvNode = sCurLNode->prevSPtr;
            aIterator->nxtNode = sCurLNode->nextSPtr;

            sRowPtr = sCurLNode->mRowPtrs[sSlotCount - 1];

            if ( sRowPtr == NULL )
            {
                continue;
            }
            else
            {
                /* nothing to do */
            }

            /* PROJ-2433
             * 최대값 조건을 만족하는지 확인하는부분이다.
             * direct key가 full key여야만 제대로된 비교가 가능하다.
             */
            if ( isFullKey( aIterator->index,
                            (smnbNode *)sCurLNode,
                            (SShort)( sSlotCount - 1 ) ) == ID_TRUE )
            {
                sKey = SMNB_GET_KEY_PTR( sCurLNode, sSlotCount - 1 );
            }
            else
            {
                sKey = NULL;
            }

            /* BUG-27948 이미 삭제된 Row를 보고 있는지 확인 */
            IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sRowPtr)->mCreateSCN) );

            aIterator->mKeyRange->maximum.callback( &sResult,
                                                    sRowPtr,
                                                    sKey,
                                                    0,
                                                    SC_NULL_GRID,
                                                    aIterator->mKeyRange->maximum.data );

            sMax = sSlotCount - 1;

            if ( sResult == ID_FALSE )
            {
                smnbBTree::findLastSlotInLeaf( aIterator->index,
                                               &aIterator->mKeyRange->maximum,
                                               sCurLNode,
                                               sMin,
                                               sSlotCount - 1,
                                               &sMax );
                aIterator->highest   = ID_TRUE;
            }
            else
            {
                /* nothing to do */
            }

            /* PROJ-2433 */
            if ( ( sMin >= 0 ) &&
                 ( sMax - sMin >= 0 ) )
            {
                idlOS::memcpy( &(aIterator->rows[0]),
                               &(sCurLNode->mRowPtrs[sMin]),
                               ( sMax - sMin + 1 ) * ID_SIZEOF(sCurLNode->mRowPtrs[0]) );
                i = sMax - sMin;
            }
            else
            {
                i = -1;
            }

            isSetRowCache = ID_TRUE;

            if ( sVersion == getLatchValueOfLNode( sCurLNode ) )
            {
                break;
            }
        } /* loop for node latch */

        if ( isSetRowCache == ID_TRUE )
        {
            /* row cache가 세팅되었으면, 나머지 iterator값들도 세팅한다. */

            aIterator->highFence = aIterator->rows + i;
            aIterator->lowFence  = aIterator->rows;
            aIterator->slot      = aIterator->lowFence - 1;

            aIterator->version   = sVersion;
            aIterator->least     = ID_FALSE;

            IDE_TEST( iduCheckSessionEvent( aIterator->mProperties->mStatistics ) != IDE_SUCCESS );

            *aIsRestart = ID_TRUE;

            break;
        }
        else
        {
            /* nothing to do */
        }

    } /* loop for next node */

    IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-47385 fetchNext에서 timeout이 발생할 경우 인덱스 링크가 깨져 무한루프를 돌 가능성이 있다.
     *           이 경우 차후 분석을 위해  현재 탐색중인 노드와
     이전 노드 2개/이후 노드 2개의 링크 정보를 trc로그로 남기도록 한다. */
    if( ( ideGetErrorCode() == idERR_ABORT_Query_Timeout ) ||
        ( ideGetErrorCode() == idERR_ABORT_Session_Disconnected ) )
    {
        ideLog::log(IDE_SM_0,"A timeout occurred during index fetch. Index [%s] link error is suspected.\n",
                    aIterator->index->mIndexHeader->mName );

        sNode = aIterator->node;

        lockTree( aIterator->index );

        if( sNode != NULL )
        {
            if( sNode->prevSPtr != NULL )
            {
                if( sNode->prevSPtr->prevSPtr != NULL )
                {
                    dumpNodeLink( sNode->prevSPtr->prevSPtr );
                }

                dumpNodeLink( sNode->prevSPtr );
            }

            dumpNodeLink( sNode );

            if( sNode->nextSPtr != NULL )
            {
                dumpNodeLink( sNode->nextSPtr );

                if( sNode->nextSPtr->nextSPtr != NULL )
                {
                    dumpNodeLink( sNode->nextSPtr->nextSPtr );
                }
            }
        }

        unlockTree( aIterator->index );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::fetchPrev( smnbIterator* aIterator,
                             const void**  aRow )
{
    idBool     sIsRestart;
    idBool     sResult;
    idBool     sCanReusableRollback = ID_TRUE;
    smxTrans * sTrans               = (smxTrans*)aIterator->trans;
    idBool     sIsVisible;

restart:

    for ( aIterator->slot-- ;
          aIterator->slot >= aIterator->lowFence ;
          aIterator->slot-- )
    {
        sCanReusableRollback = ID_TRUE;

        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        /* PROJ-2433 */
        if ( aIterator->curRecPtr == NULL )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                        aIterator->curRecPtr,
                                        &sCanReusableRollback,
                                        &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW
                     (((smpSlotHeader *)(aIterator->curRecPtr))->mCreateSCN) );

        *aRow = aIterator->curRecPtr;

        SC_MAKE_GRID( aIterator->mRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST( aIterator->mRowFilter->callback( &sResult,
                                                   *aRow,
                                                   NULL,
                                                   0,
                                                   aIterator->mRowGRID,
                                                   aIterator->mRowFilter->data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            if( sCanReusableRollback == ID_FALSE )
            {     
                sTrans->mIsReusableRollback = ID_FALSE;
            }

            if ( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if ( aIterator->mProperties->mReadRecordCount != 1 )
                {
                    aIterator->mProperties->mReadRecordCount--;

                    return IDE_SUCCESS;
                }
                else
                {
                    aIterator->mProperties->mReadRecordCount--;

                    aIterator->lowFence = aIterator->slot;
                    aIterator->least   = ID_TRUE;
                    return IDE_SUCCESS;
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( aIterator->least == ID_FALSE )
    {
        sIsRestart = ID_FALSE;

        IDE_TEST( getPrevNode( aIterator,
                               &sIsRestart ) != IDE_SUCCESS );

        if ( sIsRestart == ID_TRUE )
        {
            goto restart;
        }
        else
        {
            /* nothing to do */
        }
    }
    else
    {
        /* nothing to do */
    }

    aIterator->least = ID_TRUE;

    if ( aIterator->mKeyRange->prev != NULL )
    {
        aIterator->mKeyRange = aIterator->mKeyRange->prev;
        (void)afterLastInternal( aIterator );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        goto restart;
    }
    else
    {
        /* nothing to do */
    }

    aIterator->slot           = aIterator->lowFence - 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow                     = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::getPrevNode( smnbIterator   * aIterator,
                               idBool         * aIsRestart )
{

    idBool        sResult;
    SInt          sSlot      = 0;
    SInt          sLastSlot  = 0;
    SInt          sSlotCount = 0;
    SInt          i          = -1;
    IDU_LATCH     sVersion   = IDU_LATCH_UNLOCKED;
    smnbLNode   * sCurLNode  = NULL;
    SChar       * sRowPtr    = NULL;
    void        * sKey       = NULL;
    SInt          sState     = 0;
    smnbLNode   * sNode      = NULL;

    IDE_DASSERT( aIterator->least == ID_FALSE )
    *aIsRestart = ID_FALSE;

    lockTree( aIterator->index );
    sState = 1;

    sCurLNode = aIterator->node->prevSPtr;

    while ( sCurLNode != NULL )
    {
        i = -1;

        sVersion = getLatchValueOfLNode(sCurLNode) & IDU_LATCH_UNMASK;
        IDL_MEM_BARRIER;

        sSlotCount = sCurLNode->mSlotCount;

        aIterator->version = sVersion;
        aIterator->node    = sCurLNode;
        aIterator->least   = ID_FALSE;
        aIterator->prvNode = sCurLNode->prevSPtr;
        aIterator->nxtNode = sCurLNode->nextSPtr;

        if ( sSlotCount == 0 )
        {
            sCurLNode = sCurLNode->prevSPtr;
            continue;
        }
        else
        {
            /* nothing to do */
        }

        sRowPtr = aIterator->node->mRowPtrs[0];
        if ( sRowPtr == NULL )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        /* PROJ-2433
         * 최소값 조건을 만족하는지 확인하는부분이다.
         * direct key가 full key여야만 제대로된 비교가 가능하다. */
        if ( isFullKey( aIterator->index,
                        (smnbNode *)(aIterator->node),
                        0 ) == ID_TRUE )
        {
            sKey = SMNB_GET_KEY_PTR( aIterator->node, 0 );
        }
        else
        {
            sKey = NULL;
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)aIterator->node->mRowPtrs[0])->mCreateSCN) );

        /* PROJ-2433 */
        aIterator->mKeyRange->minimum.callback( &sResult,
                                                sRowPtr,
                                                sKey,
                                                0,
                                                SC_NULL_GRID,
                                                aIterator->mKeyRange->minimum.data );
        sSlot = 0;

        if ( sResult == ID_FALSE )
        {
            smnbBTree::findFirstSlotInNode( aIterator->index,
                                            &aIterator->mKeyRange->minimum,
                                            (smnbNode *)(aIterator->node),
                                            &sSlot );
            aIterator->least = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        /* PROJ-2433 */
        sLastSlot = sSlotCount - 1;

        if ( ( sSlot >= 0 ) &&
             ( sLastSlot - sSlot >= 0 ) )
        {
            idlOS::memcpy( &(aIterator->rows[0]),
                           &(aIterator->node->mRowPtrs[sSlot]),
                           (sLastSlot - sSlot + 1) * ID_SIZEOF(aIterator->node->mRowPtrs[0]) );
            i = sLastSlot - sSlot;
        }
        else
        {
            i = -1;
        }

        if ( sVersion == getLatchValueOfLNode( sCurLNode ) )
        {
            break;
        }
        else
        {
            /* nothing to do */
        }
    }

    /*
     * BUG-31558  It's possible to miss some rows when fetching 
     *            with the descending index(back traverse).
     *
     * if we didn't find any available slot, go to the previous node with holding the tree latch.
     */

    /* PROJ-2433
     * 위의 말 생각하지않아도됨
     * 빈 node가 아니고, 위에서 checkSCN 하지않고 모두 복사했으므로 꼭하나이상있다 */
    if ( i >= 0 )
    {
        aIterator->highFence = aIterator->rows + i;
        aIterator->lowFence  = aIterator->rows;
        aIterator->slot      = aIterator->highFence + 1;

        aIterator->highest = ID_FALSE;

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        *aIsRestart = ID_TRUE;
    }
    else
    {
        /* nothing to do */
    }

    sState = 0;
    unlockTree( aIterator->index );

    IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    /* BUG-47385 fetchPrev에서 timeout이 발생할 경우 인덱스 링크가 깨져 무한루프를 돌 가능성이 있다.
     *           이 경우 차후 분석을 위해  현재 탐색중인 노드와
     이전 노드 2개/이후 노드 2개의 링크 정보를 trc로그로 남기도록 한다. */
    if( ( ideGetErrorCode() == idERR_ABORT_Query_Timeout ) ||
        ( ideGetErrorCode() == idERR_ABORT_Session_Disconnected ) )
    {
        ideLog::log(IDE_SM_0,"A timeout occurred during index fetch. Index [%s] link error is suspected.\n",
                    aIterator->index->mIndexHeader->mName );

        sNode = aIterator->node;

        if( sState == 0 )
        {
            lockTree( aIterator->index );
            sState = 1; 
        }

        if( sNode != NULL )
        {
            if( sNode->prevSPtr != NULL )
            {
                if( sNode->prevSPtr->prevSPtr != NULL )
                {
                    dumpNodeLink( sNode->prevSPtr->prevSPtr );
                }

                dumpNodeLink( sNode->prevSPtr );
            }

            dumpNodeLink( sNode );

            if( sNode->nextSPtr != NULL )
            {
                dumpNodeLink( sNode->nextSPtr );

                if( sNode->nextSPtr->nextSPtr != NULL )
                {
                    dumpNodeLink( sNode->nextSPtr->nextSPtr );
                }
            }
        }

        sState = 0;
        unlockTree( aIterator->index );
    }

    if ( sState != 0 )
    {
        IDE_PUSH();
        unlockTree( aIterator->index );
        IDE_POP();
    }
    else
    {
        /* nothing to do */
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::fetchNextU( smnbIterator* aIterator,
                              const void**  aRow )
{
    idBool     sIsRestart;
    idBool     sResult;
    idBool     sCanReusableRollback = ID_TRUE;
    smxTrans * sTrans               = (smxTrans*)aIterator->trans;
    idBool     sIsVisible;

  restart:

    for( aIterator->slot++;
         aIterator->slot <= aIterator->highFence;
         aIterator->slot++ )
    {
        sCanReusableRollback = ID_TRUE;

        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        /* PROJ-2433 
         * NULL체크 checkSCN 위치변경*/
        if ( aIterator->curRecPtr == NULL )
        {
            aIterator->highFence = aIterator->slot - 1;
            break;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                        aIterator->curRecPtr,
                                        &sCanReusableRollback,
                                        &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW
                     (((smpSlotHeader *)(aIterator->curRecPtr))->mCreateSCN) );

        *aRow = aIterator->curRecPtr;
        aIterator->lstReturnRecPtr = aIterator->curRecPtr;

        SC_MAKE_GRID( aIterator->mRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST( aIterator->mRowFilter->callback( &sResult,
                                                   *aRow,
                                                   NULL,
                                                   0,
                                                   aIterator->mRowGRID,
                                                   aIterator->mRowFilter->data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            if( sCanReusableRollback == ID_FALSE )
            {     
                sTrans->mIsReusableRollback = ID_FALSE;
            }

            smnManager::updatedRow( (smiIterator*)aIterator );

            *aRow = aIterator->curRecPtr;
            SC_MAKE_GRID( aIterator->mRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                          SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );


            if ( SM_SCN_IS_NOT_DELETED( ((smpSlotHeader*)aRow)->mCreateSCN ) )
            {
                if ( aIterator->mProperties->mFirstReadRecordPos == 0 )
                {
                    if ( aIterator->mProperties->mReadRecordCount != 1 )
                    {
                        aIterator->mProperties->mReadRecordCount--;

                        return IDE_SUCCESS;
                    }
                    else
                    {
                        aIterator->mProperties->mReadRecordCount--;

                        aIterator->highFence = aIterator->slot;
                        aIterator->highest   = ID_TRUE;
                        return IDE_SUCCESS;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }

            }
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( aIterator->highest == ID_FALSE )
    {
        sIsRestart = ID_FALSE;

        IDE_TEST( getNextNode( aIterator, &sIsRestart ) != IDE_SUCCESS );

        if ( sIsRestart == ID_TRUE )
        {
            goto restart;
        }
        else
        {
            /* nothing to do */
        }
    }

    aIterator->highest = ID_TRUE;

    if ( aIterator->mKeyRange->next != NULL )
    {
        aIterator->mKeyRange = aIterator->mKeyRange->next;
        (void)beforeFirstInternal( aIterator );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        goto restart;
    }

    aIterator->slot            = aIterator->highFence + 1;
    aIterator->curRecPtr       = NULL;
    aIterator->lstFetchRecPtr  = NULL;
    aIterator->lstReturnRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow                      = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::fetchPrevU( smnbIterator* aIterator,
                              const void**  aRow )
{
    idBool     sIsRestart;
    idBool     sResult;
    idBool     sCanReusableRollback = ID_TRUE;
    smxTrans * sTrans               = (smxTrans*)aIterator->trans;
    idBool     sIsVisible;

restart:

    for( aIterator->slot--;
         aIterator->slot >= aIterator->lowFence;
         aIterator->slot-- )
    {
        sCanReusableRollback = ID_TRUE;

        aIterator->curRecPtr = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        /* PROJ-2433 */
        if ( aIterator->curRecPtr == NULL )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                        aIterator->curRecPtr,
                                        &sCanReusableRollback,
                                        &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW
                     (((smpSlotHeader *)(aIterator->curRecPtr))->mCreateSCN) );

        *aRow = aIterator->curRecPtr;

        SC_MAKE_GRID( aIterator->mRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST( aIterator->mRowFilter->callback( &sResult,
                                                   *aRow,
                                                   NULL,
                                                   0,
                                                   aIterator->mRowGRID,
                                                   aIterator->mRowFilter->data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            if( sCanReusableRollback == ID_FALSE )
            {     
                sTrans->mIsReusableRollback = ID_FALSE;
            }

            smnManager::updatedRow( (smiIterator*)aIterator );

            *aRow = aIterator->curRecPtr;
            SC_MAKE_GRID( aIterator->mRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                          SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );


            if ( SM_SCN_IS_NOT_DELETED( ((smpSlotHeader*)aRow)->mCreateSCN ) )
            {
                if ( aIterator->mProperties->mFirstReadRecordPos == 0 )
                {
                    if ( aIterator->mProperties->mReadRecordCount != 1 )
                    {
                        aIterator->mProperties->mReadRecordCount--;

                        return IDE_SUCCESS;
                    }
                    else
                    {
                        aIterator->mProperties->mReadRecordCount--;

                        aIterator->lowFence = aIterator->slot;
                        aIterator->least    = ID_TRUE;
                        return IDE_SUCCESS;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }

            }
        }
        else
        {
            /* nothing to do */
        }
    }

    if ( aIterator->least == ID_FALSE )
    {
        sIsRestart = ID_FALSE;

        IDE_TEST( getPrevNode( aIterator, &sIsRestart ) != IDE_SUCCESS );

        if ( sIsRestart == ID_TRUE )
        {
            goto restart;
        }
        else
        {
            /* nothing to do */
        }
    }

    aIterator->least = ID_TRUE;

    if ( aIterator->mKeyRange->prev != NULL )
    {
        aIterator->mKeyRange = aIterator->mKeyRange->prev;
        (void)afterLastInternal( aIterator );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        goto restart;
    }

    aIterator->slot           = aIterator->lowFence - 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow                     = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::fetchNextR( smnbIterator* aIterator )
{
    SChar     * sRow = NULL;
    scGRID     sRowGRID;
    idBool     sIsRestart;
    idBool     sResult;
    idBool     sCanReusableRollback = ID_TRUE;
    smxTrans * sTrans               = (smxTrans*)aIterator->trans;
    idBool     sIsVisible;

  restart:

    for( aIterator->slot++;
         aIterator->slot <= aIterator->highFence;
         aIterator->slot++ )
    {
        sCanReusableRollback = ID_TRUE;

        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        /* PROJ-2433 
         * NULL체크 checkSCN 위치변경*/
        if ( aIterator->curRecPtr == NULL )
        {
            aIterator->highFence = aIterator->slot - 1;
            break;
        }
        else
        {
            /* nothing to do */
        }

        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                        aIterator->curRecPtr,
                                        &sCanReusableRollback,
                                        &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* nothing to do */
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW
                     (((smpSlotHeader *)(aIterator->curRecPtr))->mCreateSCN) );

        sRow = aIterator->curRecPtr;
        aIterator->lstReturnRecPtr = aIterator->curRecPtr;

        SC_MAKE_GRID( sRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)sRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sRow ) );

        IDE_TEST( aIterator->mRowFilter->callback( &sResult,
                                                   sRow,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->mRowFilter->data )
                  != IDE_SUCCESS );

        if ( sResult == ID_TRUE )
        {
            if( sCanReusableRollback == ID_FALSE )
            {     
                sTrans->mIsReusableRollback = ID_FALSE;
            } 

            if ( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if ( aIterator->mProperties->mReadRecordCount != 1 )
                {
                    aIterator->mProperties->mReadRecordCount--;

                    IDE_TEST( smnManager::lockRow( (smiIterator*)aIterator)
                              != IDE_SUCCESS );
                }
                else
                {
                    aIterator->mProperties->mReadRecordCount--;

                    aIterator->highFence = aIterator->slot;
                    aIterator->highest   = ID_TRUE;

                    IDE_TEST( smnManager::lockRow( (smiIterator*)aIterator)
                              != IDE_SUCCESS );
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    if ( aIterator->highest == ID_FALSE )
    {
        sIsRestart = ID_FALSE;

        IDE_TEST( getNextNode( aIterator, &sIsRestart ) != IDE_SUCCESS );

        if ( sIsRestart == ID_TRUE )
        {
            goto restart;
        }
        else
        {
            /* nothing to do */
        }
    }

    aIterator->highest = ID_TRUE;

    if ( aIterator->mKeyRange->next != NULL )
    {
        aIterator->mKeyRange = aIterator->mKeyRange->next;
        (void)beforeFirstInternal( aIterator );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        goto restart;
    }

    aIterator->slot            = aIterator->highFence + 1;
    aIterator->curRecPtr       = NULL;
    aIterator->lstReturnRecPtr = NULL;
    aIterator->lstFetchRecPtr  = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::retraverse( idvSQL        * /* aStatistics */,
                              smnbIterator  * aIterator,
                              const void   ** /* aRow */ )
{
    idBool            sResult;
    smnbHeader*       sHeader;
    SChar*            sRowPtr;
    UInt              sFlag;
    SInt              sMin;
    SInt              sMax;
    SInt              i, j;
    IDU_LATCH         sVersion;
    smnbLNode*        sCurLNode;
    smnbLNode*        sNxtLNode;
    SInt              sSlotCount;
    SChar*            sTmpRowPtr;
    void            * sTmpKey = NULL;
    smnbStack         sStack[SMNB_STACK_DEPTH];
    SInt              sDepth;
    idBool            sIsVisible;

    sRowPtr  = aIterator->lstFetchRecPtr;
    sFlag    = aIterator->flag & SMI_RETRAVERSE_MASK;

    if ( sRowPtr == NULL )
    {
        IDE_ERROR_RAISE( sFlag != SMI_RETRAVERSE_NEXT, ERR_INVALID_FLAG );

        if ( sFlag == SMI_RETRAVERSE_BEFORE )
        {
            IDE_TEST(beforeFirst(aIterator, NULL) != IDE_SUCCESS);
        }
        else
        {
            if ( sFlag == SMI_RETRAVERSE_AFTER )
            {
                IDE_TEST(afterLast(aIterator, NULL) != IDE_SUCCESS);
            }
            else
            {
                IDE_ERROR_RAISE( 0 , ERR_INVALID_FLAG );
            }
        }
    }
    else
    {
        while(1)
        {
            if ( (aIterator->flag & SMI_ITERATOR_TYPE_MASK) == SMI_ITERATOR_WRITE )
            {
                if ( sFlag == SMI_RETRAVERSE_BEFORE )
                {
                    if ( aIterator->slot == aIterator->highFence )
                    {
                        break;
                    }

                    sRowPtr = *(aIterator->slot + 1);
                }

                if ( sFlag == SMI_RETRAVERSE_AFTER )
                {
                    if ( aIterator->slot == aIterator->lowFence )
                    {
                        break;
                    }

                    sRowPtr = *(aIterator->slot - 1);
                }
            }

            sHeader  = (smnbHeader*)aIterator->index;

            aIterator->highest = ID_FALSE;
            aIterator->least   = ID_FALSE;

            sDepth = -1;

            IDE_TEST( findPosition( sHeader,
                                    NULL, /* smiCallBack */
                                    sRowPtr,
                                    &sDepth,
                                    sStack,
                                    &sCurLNode ) != IDE_SUCCESS );

            IDE_ERROR_RAISE( sDepth >= 0, ERR_CORRUPTED_INDEX_NODE_DEPTH );

            while(1)
            {
                sVersion   = getLatchValueOfLNode(sCurLNode) & IDU_LATCH_UNMASK;
                IDL_MEM_BARRIER;

                sNxtLNode  = sCurLNode->nextSPtr;
                sSlotCount = sCurLNode->mSlotCount;

                if ( sSlotCount == 0)
                {
                    if ( sVersion == getLatchValueOfLNode(sCurLNode))
                    {
                        sCurLNode = sNxtLNode;
                    }

                    IDE_ERROR_RAISE( sCurLNode != NULL, ERR_CORRUPTED_INDEX_SLOTCOUNT );

                    continue;
                }

                aIterator->node = sCurLNode;
                aIterator->prvNode = sCurLNode->prevSPtr;
                aIterator->nxtNode = sCurLNode->nextSPtr;

                sTmpRowPtr = sCurLNode->mRowPtrs[sSlotCount - 1];

                if ( sTmpRowPtr == NULL)
                {
                    continue;
                }

                /* PROJ-2433
                 * 지금값이 최대값 조건도 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.  */
                if ( isFullKey( aIterator->index,
                                (smnbNode *)(aIterator->node),
                                (SShort)( sSlotCount - 1 ) ) == ID_TRUE )
                {
                    sTmpKey = SMNB_GET_KEY_PTR( sCurLNode, sSlotCount - 1 );
                }
                else
                {
                    sTmpKey = NULL;
                }

                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sTmpRowPtr)->mCreateSCN) );

                /* PROJ-2433 */
                aIterator->mKeyRange->maximum.callback( &sResult,
                                                        sTmpRowPtr,
                                                        sTmpKey,
                                                        0,
                                                        SC_NULL_GRID,
                                                        aIterator->mKeyRange->maximum.data );

                sMax = sSlotCount - 1;

                if ( sResult == ID_FALSE )
                {
                    smnbBTree::findLastSlotInLeaf( aIterator->index,
                                                   &aIterator->mKeyRange->maximum,
                                                   sCurLNode,
                                                   0,
                                                   sSlotCount - 1,
                                                   &sMax );

                    aIterator->highest   = ID_TRUE;
                }
                else
                {
                    /* nothing to do */
                }

                sTmpRowPtr = aIterator->node->mRowPtrs[0];

                if ( sTmpRowPtr == NULL)
                {
                    continue;
                }

                /* PROJ-2433
                 * 지금값이 최소값 조건도 만족하는지 확인하는부분이다.
                 * direct key가 full key여야만 제대로된 비교가 가능하다.  */
                if ( isFullKey( aIterator->index,
                                (smnbNode *)(aIterator->node),
                                0 ) == ID_TRUE )
                {
                    sTmpKey = SMNB_GET_KEY_PTR( aIterator->node, 0 );
                }
                else
                {
                    sTmpKey = NULL;
                }

                // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
                IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sTmpRowPtr)->mCreateSCN));

                /* PROJ-2433 */
                aIterator->mKeyRange->minimum.callback( &sResult,
                                                        sTmpRowPtr,
                                                        sTmpKey,
                                                        0,
                                                        SC_NULL_GRID,
                                                        aIterator->mKeyRange->minimum.data );

                sMin = 0;

                if ( sResult == ID_FALSE )
                {
                    smnbBTree::findFirstSlotInNode( aIterator->index,
                                                    &aIterator->mKeyRange->minimum,
                                                    (smnbNode *)(aIterator->node),
                                                    &sMin );

                    aIterator->least     = ID_TRUE;
                }
                else
                {
                    /* nothing to do */
                }

                aIterator->slot = NULL;

                for(i = sMin, j = -1; i <= sMax; i++)
                {
                    sTmpRowPtr = aIterator->node->mRowPtrs[i];

                    if ( sTmpRowPtr == NULL)
                    {
                        break;
                    }

                    IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator, sTmpRowPtr, NULL, &sIsVisible )
                              != IDE_SUCCESS );
                    if( sIsVisible == ID_TRUE ) 
                    {
                        j++;
                        aIterator->rows[j] = sTmpRowPtr;

                        if ( sRowPtr == aIterator->rows[j])
                        {
                            aIterator->slot = aIterator->rows + j;
                        }
                    }
                }

                if ( aIterator->slot != NULL)
                {
                    if ( (aIterator->flag & SMI_ITERATOR_TYPE_MASK) == SMI_ITERATOR_WRITE)
                    {
                        if ( sFlag == SMI_RETRAVERSE_BEFORE)
                        {
                            (aIterator->slot)--;
                        }

                        if ( sFlag == SMI_RETRAVERSE_AFTER)
                        {
                            (aIterator->slot)++;
                        }
                    }

                    aIterator->lowFence  = aIterator->rows;
                    aIterator->highFence = aIterator->rows + j;

                    if ( sVersion == getLatchValueOfLNode(sCurLNode))
                    {
                        aIterator->version = sVersion;
                        break;
                    }
                }
                else
                {
                    if ( sVersion == getLatchValueOfLNode(sCurLNode))
                    {
                        sCurLNode = sNxtLNode;
                    }
                }
            }

            break;
        }//while
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_NODE_DEPTH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Node Depth" );
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_SLOTCOUNT )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index SlotCount" );
    }
    IDE_EXCEPTION( ERR_INVALID_FLAG )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] flag is RETRAVERSE_NEXT [%"ID_UINT32_FMT"]",
                     sFlag );
    }
    IDE_EXCEPTION_END;

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( aIterator->index->mIndexHeader );
    }

    return IDE_FAILURE;
}

IDE_RC smnbBTree::makeDiskImage(smnIndexHeader* a_pIndex,
                                smnIndexFile*   a_pIndexFile)
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    SInt                 i;
    smnbHeader         * s_pIndexHeader;
    volatile smnbLNode * s_pCurLNode;
    volatile smnbINode * s_pCurINode;

    s_pIndexHeader = (smnbHeader*)(a_pIndex->mHeader);

    s_pCurINode = (smnbINode*)(s_pIndexHeader->root);

    if ( s_pCurINode != NULL)
    {
        while ( SMNB_IS_INTERNAL_NODE( s_pCurINode ) )
        {
            s_pCurINode = (smnbINode*)(s_pCurINode->mChildPtrs[0]);
        }

        s_pCurLNode = (smnbLNode*)s_pCurINode;

        while(s_pCurLNode != NULL)
        {
            for ( i = 0 ; i < s_pCurLNode->mSlotCount ; i++ )
            {
                IDE_TEST( a_pIndexFile->write( (SChar*)(s_pCurLNode->mRowPtrs[i]),
                                               ID_FALSE ) != IDE_SUCCESS );
            }

            s_pCurLNode = s_pCurLNode->nextSPtr;
        }
    }

    IDE_TEST(a_pIndexFile->write(NULL, ID_TRUE) != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::makeNodeListFromDiskImage(smcTableHeader *a_pTable, smnIndexHeader *a_pIndex)
{
    /* BUG-37643 Node의 array를 지역변수에 저장하는데
     * compiler 최적화에 의해서 지역변수가 제거될 수 있다.
     * 따라서 이러한 변수는 volatile로 선언해야 한다. */
    smnIndexFile          s_indexFile;
    smnbHeader          * s_pIndexHeader = NULL;
    volatile smnbLNode  * s_pCurLNode    = NULL;
    SInt                  sState         = 0;
    SInt                  i              = 0;
    UInt                  sReadCnt       = 0;

    s_pIndexHeader = (smnbHeader*)(a_pIndex->mHeader);

    IDE_TEST(s_indexFile.initialize(a_pTable->mSelfOID, a_pIndex->mId)
             != IDE_SUCCESS);
    sState = 1;

    IDE_TEST(s_indexFile.open() != IDE_SUCCESS);
    sState = 2;

    while(1)
    {
        IDE_TEST(s_pIndexHeader->mNodePool.allocateSlots(1,
                                                      (smmSlot**)&(s_pCurLNode))
                 != IDE_SUCCESS);

        IDE_TEST_RAISE( initLeafNode( (smnbLNode*)s_pCurLNode,
                                       s_pIndexHeader,
                                       IDU_LATCH_UNLOCKED ) != IDE_SUCCESS,
                        ERR_FAIL_TO_INITIALIZE_LEAF_NODE );

        IDE_TEST( s_indexFile.read( a_pTable->mSpaceID,
                                    (SChar **)(((smnbLNode *)s_pCurLNode)->mRowPtrs),
                                    (UInt)SMNB_LEAF_SLOT_MAX_COUNT( s_pIndexHeader ),
                                    &sReadCnt )
                  != IDE_SUCCESS );
 
        s_pCurLNode->mSlotCount = (SShort)sReadCnt;

        if ( s_pCurLNode->mSlotCount == 0 )
        {
            //free Node
            IDE_TEST(freeNode((smnbNode*)s_pCurLNode) != IDE_SUCCESS);

            break;
        }
        else
        {
            /* nothing to do */
        }

        if ( SMNB_IS_DIRECTKEY_INDEX( s_pIndexHeader ) == ID_TRUE )
        {
            /* PROJ-2433
             * 로딩된 row에 대한 direct key를 생성한다 */
            for ( i = 0 ; i < s_pCurLNode->mSlotCount ; i++ )
            {
                IDE_TEST( makeKeyFromRow( s_pIndexHeader,
                                          s_pCurLNode->mRowPtrs[i],
                                          SMNB_GET_KEY_PTR( s_pCurLNode, i ) )
                          != IDE_SUCCESS );
            }
        }
        else
        {
            /* nothing to do */
        }

        s_pIndexHeader->nodeCount++;

        if ( s_pIndexHeader->pFstNode == NULL )
        {
            s_pIndexHeader->pFstNode = (smnbLNode*)s_pCurLNode;
            s_pIndexHeader->pLstNode = (smnbLNode*)s_pCurLNode;

            s_pCurLNode->sequence  = 0;
        }
        else
        {
            s_pCurLNode->sequence = s_pIndexHeader->pLstNode->sequence + 1;

            s_pIndexHeader->pLstNode->nextSPtr = (smnbLNode*)s_pCurLNode;
            s_pCurLNode->prevSPtr = s_pIndexHeader->pLstNode;

            s_pIndexHeader->pLstNode = (smnbLNode*)s_pCurLNode;
        }

        if ( s_pCurLNode->mSlotCount != SMNB_LEAF_SLOT_MAX_COUNT( s_pIndexHeader ) )
        {
            break;
        }
        else
        {
            /* nothing to do */
        }
    }//while
    sState = 1;
    IDE_TEST(s_indexFile.close() != IDE_SUCCESS);
    sState = 0;
    IDE_TEST(s_indexFile.destroy() != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_FAIL_TO_INITIALIZE_LEAF_NODE )
    {
        /* inconsistent 상태로 변경하지 않고, trace log 만 남기고 fail return 한다. 
           이 함수는 index build 시에 호출되므로 index build 실패로 끝난다. */
        ideLog::log( IDE_SM_0, "fail to initialize leaf node" );
    }
    IDE_EXCEPTION_END;

    // To fix BUG-20631
    IDE_PUSH();
    switch (sState)
    {
        case 2:
            (void)s_indexFile.close();
        case 1:
            (void)s_indexFile.destroy();
            break;
        case 0:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDU_LATCH smnbBTree::getLatchValueOfINode(volatile smnbINode* aNodePtr)
{
    // latch 를 가져온다
    return idCore::acpAtomicGet32( &(aNodePtr->latch ));
}

IDU_LATCH  smnbBTree::getLatchValueOfLNode(volatile smnbLNode* aNodePtr)
{
    // latch 를 가져온다
    return idCore::acpAtomicGet32( &(aNodePtr->latch ));
}

IDE_RC smnbBTree::getPosition( smnbIterator *     aIterator,
                               smiCursorPosInfo * aPosInfo )
{
    IDU_FIT_POINT_RAISE( "BUG-46504@smnbBTree::getPositione::invalidFence", ERR_INVALID_FENCE );
    IDE_ERROR_RAISE( ( aIterator->slot >= aIterator->lowFence ) ||
                     ( aIterator->slot <= aIterator->highFence ),
                     ERR_INVALID_FENCE );

    aPosInfo->mCursor.mMRPos.mLeafNode = aIterator->node;

    /* BUG-39983 iterator에 node 정보를 저장한 이후 getPosition이 발생하기 전에 SMO가
       발생한 경우에도 setPosition에서 SMO가 일어난 것을 확인 할 수 있어야 한다. */
    aPosInfo->mCursor.mMRPos.mPrevNode = aIterator->prvNode;
    aPosInfo->mCursor.mMRPos.mNextNode = aIterator->nxtNode;
    aPosInfo->mCursor.mMRPos.mVersion  = aIterator->version;
    aPosInfo->mCursor.mMRPos.mRowPtr   = aIterator->curRecPtr;
    aPosInfo->mCursor.mMRPos.mKeyRange = (smiRange *)aIterator->mKeyRange; /* BUG-43913 */

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_FENCE )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] invalid fence. slot, lowfence, highfence "
                                "[%"ID_XPOINTER_FMT" ,%"ID_XPOINTER_FMT", %"ID_XPOINTER_FMT"]",
                                aIterator->slot, aIterator->lowFence, aIterator->highFence );
    }
    IDE_EXCEPTION_END;

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( aIterator->index->mIndexHeader );
    }

    return IDE_FAILURE;
}


IDE_RC smnbBTree::setPosition( smnbIterator     * aIterator,
                               smiCursorPosInfo * aPosInfo )
{
    IDU_LATCH    sVersion;
    SInt         sSlotCount;
    idBool       sResult;
    SInt         sLastSlot;
    SInt         sFirstSlot;
    SInt         i;
    SInt         j;
    smnbLNode  * sNode      = (smnbLNode*)aPosInfo->mCursor.mMRPos.mLeafNode;
    SChar      * sRowPtr    = (SChar*)aPosInfo->mCursor.mMRPos.mRowPtr;
    SChar     ** sStartPtr  = NULL;
    idBool       sFound     = ID_FALSE;
    UInt         sFlag      = ( aIterator->flag & SMI_RETRAVERSE_MASK ) ;
    void       * sKey       = NULL;
    idBool       sIsVisible;

    aIterator->mKeyRange = aPosInfo->mCursor.mMRPos.mKeyRange; /* BUG-43913 */

    while( 1 )
    {
        sVersion   = getLatchValueOfLNode(sNode) & IDU_LATCH_UNMASK;
        IDL_MEM_BARRIER;

        if ( (sVersion != aPosInfo->mCursor.mMRPos.mVersion) &&
            ( (sNode->prevSPtr != aPosInfo->mCursor.mMRPos.mPrevNode) ||
              (sNode->nextSPtr != aPosInfo->mCursor.mMRPos.mNextNode) ) )
        {
            /* iterator에 node 정보가 저장된 이후 SMO가 발생했을 경우
               목표 slot이 해당 node에 없을 가능성이 있으므로 getPosition에서 저장한 정보를 버리고
               retraverse로 slot의 위치를 다시 찾는다. */
            break; 
        }

        // set iterator
        sSlotCount = sNode->mSlotCount;
        aIterator->highest = ID_FALSE;
        aIterator->least   = ID_FALSE;
        aIterator->node    = sNode;
        aIterator->prvNode = sNode->prevSPtr;
        aIterator->nxtNode = sNode->nextSPtr;

        // find higher boundary
        sRowPtr = aIterator->node->mRowPtrs[sSlotCount - 1];
        if ( sRowPtr == NULL )
        {
            continue;
        }

        /* PROJ-2433
         * 지금값이 최대값 조건도 만족하는지 확인하는부분이다.
         * direct key가 full key여야만 제대로된 비교가 가능하다.  */
        if ( isFullKey( aIterator->index,
                        (smnbNode *)(aIterator->node),
                        (SShort)( sSlotCount - 1 ) ) == ID_TRUE )
        {
            sKey = SMNB_GET_KEY_PTR( aIterator->node, sSlotCount - 1 );
        }
        else
        {
            sKey = NULL;
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sRowPtr)->mCreateSCN) );

        aIterator->mKeyRange->maximum.callback( &sResult,
                                                sRowPtr,
                                                sKey,
                                                0,
                                                SC_NULL_GRID,
                                                aIterator->mKeyRange->maximum.data );
        sLastSlot = sSlotCount - 1;
        if ( sResult == ID_FALSE )
        {
            smnbBTree::findLastSlotInLeaf( aIterator->index,
                                           &aIterator->mKeyRange->maximum,
                                           aIterator->node,
                                           0,
                                           sSlotCount - 1,
                                           &sLastSlot );
            aIterator->highest = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        // find lower boundary
        sRowPtr = aIterator->node->mRowPtrs[0];
        if ( sRowPtr == NULL )
        {
            continue;
        }

        /* PROJ-2433
         * 지금값이 최소값 조건도 만족하는지 확인하는부분이다.
         * direct key가 full key여야만 제대로된 비교가 가능하다.  */
        if ( isFullKey( aIterator->index,
                        (smnbNode *)(aIterator->node),
                        0 ) == ID_TRUE )
        {
            sKey = SMNB_GET_KEY_PTR( aIterator->node, 0 );
        }
        else
        {
            sKey = NULL;
        }

        // BUG-27948 이미 삭제된 Row를 보고 있는지 확인
        IDE_DASSERT( !SM_SCN_IS_FREE_ROW(((smpSlotHeader *)sRowPtr)->mCreateSCN) );

        /*PROJ-2433 */
        aIterator->mKeyRange->minimum.callback( &sResult,
                                                sRowPtr,
                                                sKey,
                                                0,
                                                SC_NULL_GRID,
                                                aIterator->mKeyRange->minimum.data );
        sFirstSlot = 0;
        if ( sResult == ID_FALSE )
        {
            smnbBTree::findFirstSlotInNode( aIterator->index,
                                            &aIterator->mKeyRange->minimum,
                                            (smnbNode *)(aIterator->node),
                                            &sFirstSlot );
            aIterator->least = ID_TRUE;
        }
        else
        {
            /* nothing to do */
        }

        // make row cache
        j = -1;
        aIterator->lowFence = &aIterator->rows[0];
   
        /* 탐색 범위 밖부터 탐색의 기준점을 잡는다.
           이 기준점은 차후 정상적으로 값을 읽었는지 체크할때 사용된다. */
        sStartPtr = aIterator->rows - 2;
        aIterator->slot = sStartPtr;
        for( i = sFirstSlot; i <= sLastSlot; i++ )
        {
            sRowPtr = aIterator->node->mRowPtrs[i];
            if ( sRowPtr == NULL ) // delete op is running concurrently
            {
                break;
            }

            IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                            sRowPtr,
                                            NULL,
                                            &sIsVisible )
                      != IDE_SUCCESS );
            if( sIsVisible == ID_TRUE )
            {
                j++;
                aIterator->rows[j] = sRowPtr;
                if ( sRowPtr == aPosInfo->mCursor.mMRPos.mRowPtr )
                {
                    if ( sFlag == SMI_RETRAVERSE_BEFORE )
                    {
                        aIterator->slot = &aIterator->rows[j-1];
                    }
                    else
                    {
                        aIterator->slot = &aIterator->rows[j+1];
                    }
                    sFound = ID_TRUE;
                }
            }
        }
        aIterator->highFence = &aIterator->rows[j];

        if ( sVersion == getLatchValueOfLNode( sNode ) )
        {
            /* BUG-39983 탐색이 성공했을 경우 루프를 종료한다.
               탐색이 실패했을 경우에도 retraverse로 가기 위해 루프를 종료한다. */
            break;
        }
        else
        {
            /* BUG-39983 탐색 중 다른 Tx가 해당 node를 수정했을 경우
              위치가 변경될 가능성이 있으므로 재탐색한다. */
            sFound = ID_FALSE;
            continue;
        }
    }

    if ( sFound != ID_TRUE )
    {
        /* BUG-39983 lstFetchRecPtr을 NULL로 세팅해 노드를 처음부터 탐색하도록 한다.*/
        aIterator->lstFetchRecPtr = NULL;
        IDE_TEST( retraverse( NULL,
                              aIterator,
                              NULL )
                  != IDE_SUCCESS );
    }

    /* 탐색 결과가 시작점일 경우 탐색에 실패한 것이므로 assert 처리한다. */
    IDE_ERROR_RAISE( aIterator->slot != sStartPtr, ERR_CORRUPTED_INDEX );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Iterator" );
    }
    IDE_EXCEPTION_END;

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( aIterator->index->mIndexHeader );
    }

    return IDE_FAILURE;
}


IDE_RC smnbBTree::allocIterator( void ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}


IDE_RC smnbBTree::freeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

/*******************************************************************
 * Definition
 *
 *    To Fix BUG-15670
 *
 *    Row Pointer 정보로부터 Fixed/Variable에 관계없이
 *    Key Value와 Key Size를 획득한다.
 *
 * Implementation
 *
 *    Null 이 아님이 보장되는 상황에서 호출되어야 한다.
 *
 *******************************************************************/
IDE_RC smnbBTree::getKeyValueAndSize(  SChar         * aRowPtr,
                                       smnbColumn    * aIndexColumn,
                                       void          * aKeyValue,
                                       UShort        * aKeySize )
{
    SChar     * sColumnPtr;
    SChar     * sKeyPtr;
    UShort      sKeySize;
    UShort      sMinMaxKeySize;
    smiColumn * sColumn;
    UInt        sLength = 0;

    //---------------------------------------
    // Parameter Validation
    //---------------------------------------

    IDE_DASSERT( aRowPtr      != NULL );
    IDE_DASSERT( aIndexColumn != NULL );
    IDE_DASSERT( aKeySize     != NULL );

    //---------------------------------------
    // Column Pointer 위치 획득
    //---------------------------------------

    sColumn    = &aIndexColumn->column;
    sColumnPtr =  aRowPtr + sColumn->offset;

    //---------------------------------------
    // Key Value 획득
    //---------------------------------------
    if ( ( sColumn->flag & SMI_COLUMN_COMPRESSION_MASK )
         != SMI_COLUMN_COMPRESSION_TRUE )
    {
        switch( sColumn->flag & SMI_COLUMN_TYPE_MASK )
        {
            case SMI_COLUMN_TYPE_FIXED:
                if ( sColumn->size > MAX_MINMAX_VALUE_SIZE )
                {
                    // CHAR 혹은 VARCHAR type column이면서 column의 길이가
                    // MAX_MINMAX_VALUE_SIZE보다 길 경우,
                    // key value를 조정하고 length도 수정해야 함.
    
                    // BUG-24449 Key크기는 MT 함수를 통해 획득해야 함.
                    sKeySize = aIndexColumn->actualSize( sColumn,
                                                         sColumnPtr );
                    if ( sKeySize > MAX_MINMAX_VALUE_SIZE )
                    {
                        sMinMaxKeySize = MAX_MINMAX_VALUE_SIZE;
                    }
                    else
                    {
                        sMinMaxKeySize = sKeySize;
                    }
                    sKeySize = sMinMaxKeySize - aIndexColumn->headerSize;
                    aIndexColumn->makeMtdValue( sColumn->size,
                                                aKeyValue,
                                                0, //aCopyOffset
                                                sKeySize,
                                                sColumnPtr + aIndexColumn->headerSize);
                }
                else
                {
                    // column의 type이 무엇이든 MAX_MINMAX_VALUE_SIZE보다
                    // 작을 경우 : key value를 조작할 필요 없음
                    sMinMaxKeySize = sColumn->size;
                    idlOS::memcpy( (UChar*)aKeyValue,
                                (UChar*)sColumnPtr,
                                sMinMaxKeySize  );
                }
                break;
            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
    
                sKeyPtr = sgmManager::getVarColumn( aRowPtr, sColumn , &sLength );
    
                // Null이 아님이 보장되어야 함.
                IDE_ERROR_RAISE( sLength != 0, ERR_CORRUPTED_INDEX_LENGTH );
    
                //-------------------------------
                // copy from smiGetVarColumn()
                //-------------------------------
    
    
                if ( sLength > MAX_MINMAX_VALUE_SIZE )
                {
                    // VARCHAR type column key의 길이가 MAX_MINMAX_VALUE_SIZE보다
                    // 길 경우, key value를 조정하고 length도 수정해야 함.
                    sMinMaxKeySize = MAX_MINMAX_VALUE_SIZE;
    
                    sKeySize = sMinMaxKeySize - aIndexColumn->headerSize;
                    aIndexColumn->makeMtdValue( sColumn->size,
                                                aKeyValue,
                                                0, //aCopyOffset
                                                sKeySize,
                                                sKeyPtr + aIndexColumn->headerSize );
                }
                else
                {
                    sMinMaxKeySize = sLength;
                    idlOS::memcpy( (UChar*)aKeyValue,
                                (UChar*)sKeyPtr,
                                sMinMaxKeySize );
                }
    
                break;
            default:
                sMinMaxKeySize = 0;
                IDE_ERROR_RAISE( 0, ERR_CORRUPTED_INDEX_FLAG );
                break;
        }
    }
    else
    {
        // BUG-37489 compress column 에 대해 index 를 만들때 서버 FATAL
        sColumnPtr = (SChar*)smiGetCompressionColumn( aRowPtr,
                                                      sColumn,
                                                      ID_TRUE, // aUseColumnOffset
                                                      &sLength );
        IDE_DASSERT( sColumnPtr != NULL );

        if ( sColumn->size > MAX_MINMAX_VALUE_SIZE )
        {
            // CHAR 혹은 VARCHAR type column이면서 column의 길이가
            // MAX_MINMAX_VALUE_SIZE보다 길 경우,
            // key value를 조정하고 length도 수정해야 함.

            sKeySize = aIndexColumn->actualSize( sColumn,
                                                 sColumnPtr );

            if ( sKeySize > MAX_MINMAX_VALUE_SIZE )
            {
                sMinMaxKeySize = MAX_MINMAX_VALUE_SIZE;
            }
            else
            {
                sMinMaxKeySize = sKeySize;
            }
            sKeySize = sMinMaxKeySize - aIndexColumn->headerSize;
            aIndexColumn->makeMtdValue( sColumn->size,
                                        aKeyValue,
                                        0, //aCopyOffset
                                        sKeySize,
                                        sColumnPtr + aIndexColumn->headerSize);
        }
        else
        {
            // column의 type이 무엇이든 MAX_MINMAX_VALUE_SIZE보다
            // 작을 경우 : key value를 조작할 필요 없음
            sMinMaxKeySize = sColumn->size;
            idlOS::memcpy( (UChar*)aKeyValue,
                           (UChar*)sColumnPtr,
                           sMinMaxKeySize  );
        }
    }

    *aKeySize = sMinMaxKeySize;

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_LENGTH )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Row Length" );
    }
    IDE_EXCEPTION( ERR_CORRUPTED_INDEX_FLAG )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Flag" );
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnbBTree::setMinMaxStat( smnIndexHeader * aCommonHeader,
                                 SChar          * aRowPtr,
                                 smnbColumn     * aIndexColumn,
                                 idBool           aIsMinStat )
{
    smnRuntimeHeader    * sRunHeader;
    UShort                sDummyKeySize;
    void                * sTarget;

    sRunHeader = (smnRuntimeHeader*)aCommonHeader->mHeader;

    if ( aIsMinStat == ID_TRUE )
    {
        sTarget = aCommonHeader->mStat.mMinValue;
    }
    else
    {
        sTarget = aCommonHeader->mStat.mMaxValue;
    }

    (void)( sRunHeader->mStatMutex.lock(NULL) ); /* always ID_SUCCESS return */
    ID_SERIAL_BEGIN( sRunHeader->mAtomicB++; );

    if ( aRowPtr == NULL )
    {
        ID_SERIAL_EXEC(
            idlOS::memset( sTarget,
                           0x00,
                           MAX_MINMAX_VALUE_SIZE),
            1 );
    }
    else
    {
        ID_SERIAL_EXEC(
            IDE_TEST( getKeyValueAndSize( aRowPtr,
                                          aIndexColumn,
                                          sTarget,
                                          &sDummyKeySize ) != IDE_SUCCESS ),
            1 );
    }
    ID_SERIAL_END( sRunHeader->mAtomicA++; );
    (void)( sRunHeader->mStatMutex.unlock() ); /* always ID_SUCCESS return */

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 
 * BUG-35163 - [sm_index] [SM] add some exception properties for __DBMS_STAT_METHOD
 *
 * __DBMS_STAT_METHOD가 MANUAL일때 아래 두 프로퍼티로 특정 타입의 DB에 통계 갱신
 * 방법을 변경할 수 있다.
 *
 * __DBMS_STAT_METHOD_FOR_MRDB
 * __DBMS_STAT_METHOD_FOR_VRDB
 */
inline idBool smnbBTree::needToUpdateStat( idBool aIsMemTBS )
{
    idBool  sRet;
    SInt    sStatMethod;

    if ( smuProperty::getDBMSStatMethod() == SMU_DBMS_STAT_METHOD_AUTO )
    {
        sRet = ID_TRUE;
    }
    else
    {
        if ( aIsMemTBS == ID_TRUE )
        {
            sStatMethod = smuProperty::getDBMSStatMethod4MRDB();
        }
        else
        {
            /* volatile tablespace */
            sStatMethod = smuProperty::getDBMSStatMethod4VRDB();
        }

        if ( sStatMethod == SMU_DBMS_STAT_METHOD_AUTO )
        {
            sRet = ID_TRUE;
        }
        else
        {
            sRet = ID_FALSE;
        }
    }

    return sRet;
}


IDE_RC smnbBTree::dumpIndexNode( smnIndexHeader * aIndex,
                                 smnbNode       * aNode,
                                 SChar          * aOutBuf,
                                 UInt             aOutSize )
{
    smnbLNode      * sLNode;
    smnbINode      * sINode;
    UInt             sFixedKeySize;
    smOID            sOID;
    SChar          * sValueBuf;
    UInt             sState = 0;
    SInt             i;

    sFixedKeySize = ((smnbHeader*)aIndex->mHeader)->mFixedKeySize;

    IDE_TEST( iduMemMgr::calloc( IDU_MEM_ID, 1,
                                 ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                 (void**)&sValueBuf )
              != IDE_SUCCESS );
    sState = 1;


    if ( SMNB_IS_LEAF_NODE( aNode ) )
    {
        /* Leaf Node */
        sLNode = (smnbLNode*)aNode;
        
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "LeafNode:\n"
                             "flag         : %"ID_INT32_FMT"\n"
                             "sequence     : %"ID_UINT32_FMT"\n"
                             "prevSPtr     : %"ID_vULONG_FMT"\n"
                             "nextSPtr     : %"ID_vULONG_FMT"\n"
                             "keySize      : %"ID_UINT32_FMT"\n"
                             "maxSlotCount : %"ID_INT32_FMT"\n"
                             "slotCount    : %"ID_INT32_FMT"\n"
                             "[Num]           RowPtr           RowOID\n",
                             sLNode->flag,
                             sLNode->sequence,
                             sLNode->prevSPtr,
                             sLNode->nextSPtr,
                             sLNode->mKeySize,
                             sLNode->mMaxSlotCount,
                             sLNode->mSlotCount );

        for ( i = 0 ; i < sLNode->mSlotCount ; i++ )
        {
            if ( sLNode->mRowPtrs[ i ] != NULL )
            {
                sOID = SMP_SLOT_GET_OID( sLNode->mRowPtrs[ i ] );
            }
            else
            {
                sOID = SM_NULL_OID;
            }
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "[%3"ID_UINT32_FMT"]"
                                 " %16"ID_vULONG_FMT
                                 " %16"ID_vULONG_FMT,
                                 i,
                                 sLNode->mRowPtrs[ i ],
                                 sOID );
            ideLog::ideMemToHexStr( (UChar*)sLNode->mRowPtrs[ i ],
                                    sFixedKeySize + SMP_SLOT_HEADER_SIZE,
                                    IDE_DUMP_FORMAT_PIECE_SINGLE |
                                    IDE_DUMP_FORMAT_ADDR_NONE |
                                    IDE_DUMP_FORMAT_BODY_HEX |
                                    IDE_DUMP_FORMAT_CHAR_ASCII ,
                                    sValueBuf,
                                    IDE_DUMP_DEST_LIMIT );
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "   %s\n",
                                 sValueBuf);

        }
    }
    else
    {
        /* Internal Node */
        sINode = (smnbINode*)aNode;
        
        idlVA::appendFormat( aOutBuf,
                             aOutSize,
                             "InternalNode:\n"
                             "flag         : %"ID_INT32_FMT"\n"
                             "sequence     : %"ID_UINT32_FMT"\n"
                             "prevSPtr     : %"ID_vULONG_FMT"\n"
                             "nextSPtr     : %"ID_vULONG_FMT"\n"
                             "keySize      : %"ID_UINT32_FMT"\n"
                             "maxSlotCount : %"ID_INT32_FMT"\n"
                             "slotCount    : %"ID_INT32_FMT"\n"
                             "[Num]           RowPtr           RowOID\n",
                             sINode->flag,
                             sINode->sequence,
                             sINode->prevSPtr,
                             sINode->nextSPtr,
                             sINode->mKeySize,
                             sINode->mMaxSlotCount,
                             sINode->mSlotCount );

        for ( i = 0 ; i < sINode->mSlotCount ; i++ )
        {
            if ( sINode->mRowPtrs[ i ] != NULL )
            {
                sOID = SMP_SLOT_GET_OID( sINode->mRowPtrs[ i ] );
            }
            else
            {
                sOID = SM_NULL_OID;
            }
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "[%3"ID_UINT32_FMT"]"
                                 " %16"ID_vULONG_FMT
                                 " %16"ID_vULONG_FMT
                                 " %16"ID_vULONG_FMT"\n",
                                 i,
                                 sINode->mRowPtrs[ i ],
                                 sOID,
                                 sINode->mChildPtrs[ i ] );
            ideLog::ideMemToHexStr( (UChar*)sINode->mRowPtrs[ i ],
                                    sFixedKeySize,
                                    IDE_DUMP_FORMAT_NORMAL,
                                    sValueBuf,
                                    IDE_DUMP_DEST_LIMIT );
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 sValueBuf);
        }
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free( sValueBuf ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState == 1 )
    {
        sState = 0;
        if ( iduMemMgr::free( sValueBuf ) != IDE_SUCCESS )
        {
            ideLog::log( IDE_ERR_0, "[SM] fail to free buffer" );
        }
    }

    return IDE_FAILURE;
}

void smnbBTree::logIndexNode( smnIndexHeader * aIndex,
                              smnbNode       * aNode )
{
    SChar           * sOutBuffer4Dump;

    if( aNode != NULL )
    {
        if ( iduMemMgr::calloc( IDU_MEM_SM_SMN,
                                1,
                                ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                (void**)&sOutBuffer4Dump )
             == IDE_SUCCESS )
        {
            if ( dumpIndexNode( aIndex, 
                                aNode, 
                                sOutBuffer4Dump,
                                IDE_DUMP_DEST_LIMIT )
                 == IDE_SUCCESS )
            {
                ideLog::log( IDE_SM_0, "%s\n", sOutBuffer4Dump );
            }
            else
            {
                /* dump fail */
            }

            (void) iduMemMgr::free( sOutBuffer4Dump );
        }
        else
        {
            /* alloc fail */
        }
    }
    else
    {
        ideLog::log( IDE_SM_0, "Can Not Dump Null Node" );
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::makeKeyFromRow                  *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * DIRECT KEY INDEX에서 사용할 DIRECT KEY를 row에서 추출하는함수.
 *
 * - COMPOSITE KEY INDEX의 경우는 첫번째컬럼만 추출한다.
 * - partial key가 가능한 자료형이면 aIndex->mKeySize 만큼 partial 되어들어간다.
 *
 * aIndex    - [IN]  INDEX 헤더
 * aRow      - [IN]  row pointer
 * aKey      - [OUT] 추출된 Direct Key
 *********************************************************************/
IDE_RC smnbBTree::makeKeyFromRow( smnbHeader   * aIndex,
                                  SChar        * aRow,
                                  void         * aKey )
{
    UInt          sLength       = 0;
    SChar       * sVarValuePtr  = NULL;
    smnbColumn  * sFirstCol     = &(aIndex->columns[0]);
    void        * sKeyBuf       = NULL;

    if ( ( sFirstCol->column.flag & SMI_COLUMN_COMPRESSION_MASK )
          == SMI_COLUMN_COMPRESSION_TRUE )
    {
        /* PROJ-2433
         * compression table에 생성되는 index는
         * 일반 index를 사용한다. */
        ideLog::log( IDE_SERVER_0,
                     "ERROR! cannot make Direct Key Index in compression table [Index name:%s]",
                     aIndex->mIndexHeader->mName );
        IDE_ERROR_RAISE( 0, ERR_CORRUPTED_INDEX );
    }
    else
    {
        switch ( sFirstCol->column.flag & SMI_COLUMN_TYPE_MASK )
        {
            case SMI_COLUMN_TYPE_FIXED:

                if ( sFirstCol->column.size <= aIndex->mKeySize )
                {
                    idlOS::memcpy( aKey,
                                   aRow + sFirstCol->column.offset,
                                   sFirstCol->column.size );
                }
                else
                {
                    idlOS::memcpy( aKey,
                                   aRow + sFirstCol->column.offset,
                                   aIndex->mKeySize );
                }
                break;

            case SMI_COLUMN_TYPE_VARIABLE:
            case SMI_COLUMN_TYPE_VARIABLE_LARGE:
               
                /* PROJ-2419 */
                if ( aIndex->mIsPartialKey == ID_TRUE )
                {
                    /* partial key 인경우 buff를 통해 key를 받은후,
                     * partial 길이만큼잘라 반환한다 */
                    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SMN, 1,
                                                 sFirstCol->column.size,
                                                 (void **)&sKeyBuf )
                              != IDE_SUCCESS );

                    sVarValuePtr = sgmManager::getVarColumn( aRow,
                                                             &(sFirstCol->column),
                                                             (SChar *)sKeyBuf );

                    idlOS::memcpy( aKey,
                                   sKeyBuf,
                                   aIndex->mKeySize );
                }
                else
                {
                    sVarValuePtr = sgmManager::getVarColumn( aRow,
                                                             &(sFirstCol->column),
                                                             &sLength);

                    if ( sLength <= aIndex->mKeySize )
                    {
                        idlOS::memcpy( aKey, sVarValuePtr, sLength );
                    }
                    else
                    {
                        idlOS::memcpy( aKey, sVarValuePtr, aIndex->mKeySize );
                    }
                }

                /* variable column의 value가 NULL일 경우, NULL 값을 채움 */
                if ( sVarValuePtr == NULL )
                {
                    /* PROJ-2433 
                     * null 값을세팅한다
                     * mtd모듈의 setNull을 실행한다. */
                    sFirstCol->null( &(sFirstCol->column), aKey );
                }
                else
                {
                    /* nothing to do */
                }

                break;
            default:
                ideLog::log( IDE_SM_0,
                             "Invalid first column flag : %"ID_UINT32_FMT" [Index name:%s]",
                             sFirstCol->column.flag,
                             aIndex->mIndexHeader->mName );
                break;
        }
    }

    if ( sKeyBuf != NULL )
    {
        IDE_ERROR_RAISE( iduMemMgr::free( sKeyBuf ) == IDE_SUCCESS,
                         ERR_FAIL_TO_FREE_BUFFER );
        sKeyBuf = NULL;
    }
    else
    {
        /* nothing to do */
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_CORRUPTED_INDEX )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );

        ideLog::log( IDE_SM_0, "Index Corrupted Detected : Wrong Index Flag" );
    }
    IDE_EXCEPTION( ERR_FAIL_TO_FREE_BUFFER )
    {
        sKeyBuf = NULL; /* 다시 free 시도하지 않게 하기 위해서 */
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] fail to free buffer - case 1" );
    }
    IDE_EXCEPTION_END;

    if ( sKeyBuf != NULL )
    {
        if ( iduMemMgr::free( sKeyBuf ) != IDE_SUCCESS )
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
            ideLog::log( IDE_ERR_0, "[SM] fail to free buffer - case 2" );
        }
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::setDirectKeyIndex               *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * BTREE INDEX 동적헤더에 direct key index 관련된 값들을 세팅한다.
 *
 * - composite index은 첫번째 컬럼만 direct key로 저장된다.
 * - CHAR,VARCHAR,NCHAR,NVARCHAR 데이터형은 partial key index를 허용한다.
 *
 * aHeader   - [IN]  BTREE INDEX 동적헤더
 * aIndex    - [IN]  INDEX 헤더
 * aColumn   - [IN]  컬럼정보
 * aMtdAlign - [IN]  데이터형별 MTD구조체 Align
 *********************************************************************/
IDE_RC smnbBTree::setDirectKeyIndex( smnbHeader         * aHeader,
                                     smnIndexHeader     * aIndex,
                                     const smiColumn    * aColumn,
                                     UInt                 aMtdAlign )
{
    UInt sColumnSize = 0;

    if ( ( aIndex->mFlag & SMI_INDEX_DIRECTKEY_MASK ) == SMI_INDEX_DIRECTKEY_TRUE ) 
    {
        /* 컬럼크기를 구조체 align으로 조정 */
        sColumnSize = idlOS::align( aColumn->size, aMtdAlign );

        if ( gSmiGlobalCallBackList.isUsablePartialDirectKey( (void *)aColumn ) )
        {
            if ( aIndex->mMaxKeySize >= sColumnSize )
            {
                /* full-key */
                aHeader->mIsPartialKey  = ID_FALSE;
                aHeader->mKeySize       = sColumnSize;
            }
            else
            {
                /* partial-key */
                aHeader->mIsPartialKey  = ID_TRUE;
                /* mMaxKeySize보다 크지않은 값중에 align 맞는 가장 큰값 찾기 */
                aHeader->mKeySize       = idlOS::align( aIndex->mMaxKeySize - aMtdAlign + 1, aMtdAlign );
            }
        }
        /* 기타자료형 */
        else
        {
            /* 문자형 이외의 자료형에 대해서는 partial key를 인정하지않는다. */
            IDE_TEST( aIndex->mMaxKeySize < sColumnSize );

            /* full-key */
            aHeader->mIsPartialKey  = ID_FALSE;
            aHeader->mKeySize       = sColumnSize;
        }

        /* DIRECT KEY INDEX의 INTERNAL NODE 구조 : smnbINode + child_pointers + row_pointers + direct_keys */
        aHeader->mINodeMaxSlotCount = ( ( mNodeSize - ID_SIZEOF(smnbINode) + ID_SIZEOF(smnbNode *) ) /
                                        ( ID_SIZEOF(smnbNode *) + ID_SIZEOF(SChar *) + aHeader->mKeySize ) );
        /* DIRECT KEY INDEX의 LEAF NODE 구조 : smnbLNode + row_pointers + direct_keys */
        aHeader->mLNodeMaxSlotCount = ( ( mNodeSize - ID_SIZEOF(smnbLNode) ) /
                                        ( ID_SIZEOF(SChar *) + aHeader->mKeySize ) );

    }
    else /* normal index (=no direct key index) */
    {
        aHeader->mIsPartialKey      = ID_FALSE;
        aHeader->mKeySize           = 0;
        aHeader->mINodeMaxSlotCount = ( ( mNodeSize - ID_SIZEOF(smnbINode) + ID_SIZEOF(smnbNode *) ) /
                                        ( ID_SIZEOF(smnbNode *) + ID_SIZEOF(SChar *) ) );
        aHeader->mLNodeMaxSlotCount = ( ( mNodeSize - ID_SIZEOF(smnbLNode) ) /
                                        ( ID_SIZEOF(SChar *) ) );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::isFullKey                       *
 * ------------------------------------------------------------------*
 * PROJ-2433 Direct Key Index
 * NODE의 aIdx번째 slot의 direct key가 full key 인지여부를 리턴한다.
 *
 * aIndex    - [IN]  INDEX 헤더
 * aNode     - [IN]  INDEX NODE
 * aIdx      - [IN]  slot index
 *********************************************************************/
inline idBool smnbBTree::isFullKey( smnbHeader      * aIndex,
                                    const smnbNode  * aNode,
                                    SShort            aIdx )
{
    if ( SMNB_IS_DIRECTKEY_INDEX( aIndex ) == ID_TRUE )
    {
        if ( aIndex->mIsPartialKey == ID_FALSE )
        {
            /* direct key index 이고, partial key index가 아님 */
            return ID_TRUE;
        }
        else
        {
            if ( aIndex->columns[0].actualSize( &(aIndex->columns[0].column),
                                                SMNB_GET_KEY_PTR( aNode, aIdx ) )
                 <= aNode->mKeySize )
            {
                /* partial key index이지만,
                   이 slot은 모든 direct key를 포함하고있음 */
                return ID_TRUE;
            }
        }
    }

    return ID_FALSE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::keyRedistribute                 *
 * ------------------------------------------------------------------*
 * PROJ-2613 Key redistribution in MRDB index
 * 키 재분배를 수행한다.
 * 입력 받은 인자 만큼의 slot을 기존 노드에서 다음 노드로 이동시킨다.
 *
 * aIndex                 - [IN]  INDEX 헤더
 * aCurNode               - [IN]  Src LEAF NODE
 * aNxtNode               - [IN]  Dest LEAF NODE
 * aKeyRedistributeCount  - [IN]  키 재분배로 이동시킬 slot의 수
 *********************************************************************/
IDE_RC smnbBTree::keyRedistribute( smnbHeader * aIndex,
                                   smnbLNode  * aCurNode,
                                   smnbLNode  * aNxtNode,
                                   SInt         aKeyRedistributeCount )
{
    SInt i = 0;

    IDE_ERROR( aKeyRedistributeCount > 0 );

    /* 재분배 되는 키는 최대 slot 수의 1/2를 넘을 수 없다. */
    IDE_ERROR( aKeyRedistributeCount <= ( SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) / 2 ) );

    /* 이웃 노드의 slot수를 확인해 해당 노드의 slot수가 0일 경우
     * 이웃 노드가 제거 되었을 수 있으므로 키 재분배를 중지한다. */
    IDE_TEST( aNxtNode->mSlotCount == 0 );

    /* 현재 노드가 다른 Tx에 의해 split되었을 수 있으므로 키 재분배를 중지한다. */
    IDE_TEST( aCurNode->mSlotCount != SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) );

    /* 키를 재분배 하기전 두 리프노드에게 SCAN LATCH를 건다. */
    SMNB_SCAN_LATCH( aCurNode );
    SMNB_SCAN_LATCH( aNxtNode );


    /* 이웃 노드의 slot을 뒤로 이동시켜 재분배될 키가 들어갈 자리를 만든다. */
    shiftLeafSlots( aNxtNode,
                    0,
                    (SShort)( aNxtNode->mSlotCount - 1 ),
                    (SShort)( aKeyRedistributeCount ) ); 

    /* 현재 노드의 slot을 이웃 노드에 복사하여 재분배를 수행한다. */
    copyLeafSlots( aNxtNode,
                   0,
                   aCurNode,
                   (SShort)( SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) - aKeyRedistributeCount ),
                   (SShort)( aCurNode->mSlotCount - 1 ) );

    /* 현재 노드에 남아있는 slot을 제거한다. */
    for ( i = 0; i < aKeyRedistributeCount; i++ )
    {
        setLeafSlot( aCurNode, 
                     (SShort)( ( aCurNode->mSlotCount - 1 ) - i ), 
                     NULL, 
                     NULL );
    }
    
    /* 현재 노드와 이웃 노드의 slotcount를 갱신한다.*/
    aNxtNode->mSlotCount = aNxtNode->mSlotCount + aKeyRedistributeCount;
    aCurNode->mSlotCount = aCurNode->mSlotCount - aKeyRedistributeCount;

    /* 두 리프노드에 걸린 SCAN LATCH를 푼다. */
    SMNB_SCAN_UNLATCH( aNxtNode );
    SMNB_SCAN_UNLATCH( aCurNode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::keyRedistributionPropagate      *
 * ------------------------------------------------------------------*
 * PROJ-2613 Key redistribution in MRDB index
 * 리프노드의 키 재분배가 수행된 후 그에 맞춰 부모 노드도 갱신한다.
 *
 * aIndex      - [IN]  INDEX 헤더
 * aINode      - [IN]  갱신할 부모 노드
 * aLNode      - [IN]  키 재분배가 수행된 리프 노드
 * aOldRowPtr  - [IN]  키 재분배가 수행되기전 리프 노드의 대표 값
 *********************************************************************/
IDE_RC smnbBTree::keyRedistributionPropagate( smnbHeader * aIndex,
                                              smnbINode  * aINode,
                                              smnbLNode  * aLNode,
                                              SChar      * aOldRowPtr )
{
    SInt    s_pOldPos = 0;
    void    * sNewKey = NULL;
    void    * sNewRowPtr = NULL;
#ifdef DEBUG
    void    * sOldKeyInInternal = NULL;
    void    * sOldKeyRowPtrInInternal = NULL;
#endif

    IDE_ERROR( aOldRowPtr != NULL );

    /* 리프노드에서 인터널 노드로 전파할 값을 가져온다. */
    getLeafSlot( (SChar **)&sNewRowPtr,
                 &sNewKey,
                 aLNode,
                 (SShort)( aLNode->mSlotCount - 1 ) );

    /* 인터널 노드내 갱신할 위치를 가져온다. */
    IDE_TEST( findSlotInNode( aIndex, 
                              (smnbNode *)aINode, 
                              aOldRowPtr, 
                              &s_pOldPos ) != IDE_SUCCESS );

#ifdef DEBUG 
    /* 인터널 노드에서 가져온 키 값이 정상적인지 확인하는 부분.
       매번 slot을 탐색하는 것은 비용이 크기때문에 debug mode에서만 작동하도록 한다. */
    IDE_ERROR( aLNode == (smnbLNode*)(aINode->mChildPtrs[s_pOldPos]) );

    getInternalSlot( NULL,
                     (SChar **)&sOldKeyInInternal,
                     &sOldKeyRowPtrInInternal,
                     aINode,
                     (SShort)s_pOldPos );

    IDE_ERROR( sOldKeyInInternal == (void*)aOldRowPtr );
#endif

    SMNB_SCAN_LATCH( aINode );

    /* 인터널 노드를 갱신한다. */
    setInternalSlot( aINode,
                     (SShort)s_pOldPos,
                     (smnbNode *)aLNode,
                     (SChar *)sNewRowPtr,
                     sNewKey );

    SMNB_SCAN_UNLATCH( aINode );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::keyReorganization               *
 * ------------------------------------------------------------------*
 * PROJ-2614 Memory Index Reorganization
 * 리프노드를 순회하면서 이웃 노드와 통합이 가능하다면 통합한다.
 *
 * aIndex      - [IN]  INDEX 헤더
 *********************************************************************/
IDE_RC smnbBTree::keyReorganization( smnbHeader    * aIndex )
{
    smnbLNode   * sCurLNode = NULL;
    smnbLNode   * sNxtLNode = NULL;
    smnbINode   * sCurINode = NULL;
    smnbNode    * sFreeNodeList = NULL;
    UInt          sDeleteNodeCount = 0;
    UInt          sLockState = 0;
    UInt          sNodeState = 0;
    void        * sOldKeyRow = NULL;
    void        * sNewKeyRow = NULL;
    void        * sNewKey = NULL;
    SInt          sInternalSlotPos = 0;
    SInt          sNodeOldCount = 0;
    SInt          sNodeNewCount = 0;
    SInt          i = 0;

    sCurINode = ( smnbINode * )aIndex->root;

    /* 1. 첫번째 leaf node와 그 부모가 되는 internal node를 찾는다. */

    /* 해당 인덱스 안에 노드가 없거나 1개만 남아있을 경우에는
     * 통합을 수행할수 없으므로 reorganization을 종료한다. */
    IDE_TEST_CONT( ( sCurINode == NULL ) || 
                   ( SMNB_IS_LEAF_NODE( sCurINode ) ), 
                   SKIP_KEY_REORGANIZATION );

    IDU_FIT_POINT( "smnbBTree::keyReorganization::findFirstLeafNode" );

    lockTree( aIndex );
    sLockState = 1;

    while ( SMNB_IS_INTERNAL_NODE( sCurINode->mChildPtrs[0] ) )
    {
        sCurINode = (smnbINode*)sCurINode->mChildPtrs[0];
    }
    sCurLNode = (smnbLNode*)sCurINode->mChildPtrs[0];

    sLockState = 0;
    unlockTree( aIndex );

    while ( sCurLNode != NULL )
    {
        /* 통합 작업은 lockTable 작업을 수행하지 않고 수행되기 때문에
         * 통합 작업 수행 중 해당 인덱스가 제거될 수 있다.
         * 수행 중 인덱스 제거가 확인될 경우 통합 작업을 종료한다. */
        IDE_TEST_CONT( aIndex->mIndexHeader->mDropFlag == SMN_INDEX_DROP_TRUE,
                       SKIP_KEY_REORGANIZATION );

        /* 2. 대상 리프 노드가 다음 노드와 통합이 가능한지 확인한다. */
        if ( checkEnableReorgInNode( sCurLNode,
                                     sCurINode,
                                     SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) ) == ID_FALSE )
        {
            /* 통합이 불가능할 경우 다음 노드로 이동한다. */
            sCurLNode = sCurLNode->nextSPtr;
            if ( sCurLNode == NULL )
            {
                /* 리프노드의 끝까지 왔을 경우 */
                continue;
            }
            else
            {
                if ( sCurLNode->mSlotCount == 0 )
                {
                    /* 포인터를 타고 들어온 리프노드가 제거되었을 경우 */
                    continue;
                }
                else
                {
                    /* do nothing... */
                }
            }

            sOldKeyRow = sCurLNode->mRowPtrs[( sCurLNode->mSlotCount ) - 1];

            IDE_TEST( findSlotInNode( aIndex, 
                                      (smnbNode *)sCurINode, 
                                      sOldKeyRow, 
                                      &sInternalSlotPos ) != IDE_SUCCESS );

            /* 이웃 노드로 이동하면서 부모 노드가 변경된 경우 */
            if ( sCurINode->mSlotCount <= sInternalSlotPos )
            {
                /* 다음 인터널 노드가 없는 경우 */
                IDE_TEST( sCurINode->nextSPtr == NULL );

                sCurINode = sCurINode->nextSPtr;

                /* 다음 노드에도 없을 경우 인터널 노드에 문제가 발생한 것이므로 통합을 중지 */
                IDE_TEST( findSlotInNode( aIndex, 
                                          (smnbNode *)sCurINode, 
                                          sOldKeyRow,
                                          &sInternalSlotPos ) != IDE_SUCCESS );

                IDE_TEST( sCurLNode != ( smnbLNode* )( sCurINode->mChildPtrs[sInternalSlotPos] ) );
            }
            else
            {
                /* do nothing... */
            }
            
            continue;    
        }
        else
        {
            sNodeState = 0;

            sNxtLNode = sCurLNode->nextSPtr;
            IDU_FIT_POINT( "smnbBTree::keyReorganization::beforeLock" );

            /* 3. lock을 잡는다. */
            lockTree( aIndex );
            sLockState = 1;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint1" );

            lockNode( sCurLNode );
            sLockState = 2;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint2" );

            lockNode( sNxtLNode );
            sLockState = 3;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::afterLock" );

            /* 4. 2번 작업 후 treelock을 잡기 전 이웃 노드가 변경될 수 있으니 다시 확인한다. */
            if ( checkEnableReorgInNode( sCurLNode,
                                         sCurINode,
                                         SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) ) == ID_FALSE )       
            {
                sLockState = 2;
                unlockNode( sNxtLNode );

                sLockState = 1;
                unlockNode( sCurLNode );

                sLockState = 0;
                unlockTree( aIndex );

                continue;
            }
            else
            {
                /* do nothing... */
            }

            /* 5. 현재 노드의 값을 저장해둔다.
             *    이 값은 인터널 노드를 갱신하기 위한 위치를 찾는데 사용된다. */

            sOldKeyRow = sCurLNode->mRowPtrs[( sCurLNode->mSlotCount ) - 1];

            IDE_TEST( findSlotInNode( aIndex, 
                                      (smnbNode *)sCurINode, 
                                      sOldKeyRow, 
                                      &sInternalSlotPos ) != IDE_SUCCESS );

            /* 6. 현재 노드와 다음 노드간 통합 작업을 수행한다. */
            SMNB_SCAN_LATCH( sCurLNode );
            SMNB_SCAN_LATCH( sNxtLNode );

            sLockState = 4;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint3" );

            sNodeOldCount = sCurLNode->mSlotCount;

            IDE_ERROR( sCurLNode->mSlotCount + sNxtLNode->mSlotCount 
                       <= SMNB_LEAF_SLOT_MAX_COUNT( aIndex ) );

            /* 6.1 다음 노드의 slot들을 현재 노드에 복사한다. */
            copyLeafSlots( sCurLNode, 
                           sCurLNode->mSlotCount,
                           sNxtLNode,
                           0, /* aSrcStartIdx */
                           sNxtLNode->mSlotCount );

            sNodeState = 1;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint4" );

            /* 6.2 현재 노드의 메타 정보를 갱신한다. */

            /* 트리의 마지막 노드에 대해서는 수행하지 않으므로
             * 항상 다음 노드의 next노드의 prev pointer를 조절해야 한다. */
            IDE_ERROR( sNxtLNode->nextSPtr != NULL );

            lockNode( sNxtLNode->nextSPtr );
            sLockState = 5;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint10" );

            SMNB_SCAN_LATCH( sNxtLNode->nextSPtr );
            sLockState = 6;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint5" );

            sNxtLNode->nextSPtr->prevSPtr = sNxtLNode->prevSPtr;

            SMNB_SCAN_UNLATCH( sNxtLNode->nextSPtr );
            sLockState = 5;

            unlockNode( sNxtLNode->nextSPtr );
            sLockState = 4;

            sCurLNode->mSlotCount += sNxtLNode->mSlotCount;
            sNodeNewCount = sCurLNode->mSlotCount;
            sCurLNode->nextSPtr = sNxtLNode->nextSPtr;

            sNxtLNode->flag |= SMNB_NODE_INVALID;

            sNodeState = 2;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint6" );

            SMNB_SCAN_UNLATCH( sNxtLNode );
            SMNB_SCAN_UNLATCH( sCurLNode );

            sLockState = 3;

            /* 7. 제거될 이웃 노드를 free node list에 연결한다. */
            sNxtLNode->freeNodeList = sFreeNodeList;
            sFreeNodeList = (smnbNode*)sNxtLNode;

            sNodeState = 3;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint7" );

            sDeleteNodeCount++;

            /* 8. 합쳐진 노드의 키 값을 가져온다. */
            sNewKeyRow = NULL;
            sNewKey = NULL;

            getLeafSlot( (SChar **)&sNewKeyRow,
                         &sNewKey,
                         sCurLNode,
                         (SShort)( sCurLNode->mSlotCount - 1 ) );

            /* 9. 리프 노드의 lock을 해제한다. */
            sLockState = 2;
            unlockNode( sNxtLNode );

            sLockState = 1;
            unlockNode( sCurLNode );

            /* 10. 인터널 노드를 갱신한다. */
            SMNB_SCAN_LATCH( sCurINode );

            shiftInternalSlots( sCurINode, 
                                ( SShort )( sInternalSlotPos + 1 ), 
                                ( SShort )( sCurINode->mSlotCount - 1 ),
                                ( SShort )(-1) );
            
            setInternalSlot( sCurINode,
                             ( SShort )sInternalSlotPos,
                             (smnbNode *)sCurLNode,
                             (SChar *)sNewKeyRow,
                             sNewKey );

            setInternalSlot( sCurINode,
                             ( SShort )( sCurINode->mSlotCount - 1 ),
                             NULL,
                             NULL,
                             NULL );

            sNodeState = 4;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint8" );

            sCurINode->mSlotCount--;

            sNodeState = 5;

            IDU_FIT_POINT( "smnbBTree::keyReorganization::exceptionPoint9" );

            SMNB_SCAN_UNLATCH( sCurINode );

            /* 11. tree latch를 해제한다. */
            sLockState = 0;
            unlockTree( aIndex );
        }
    }

    /* 작업이 완료 된 후 free list의 노드들을 정리한다. */

    if ( sFreeNodeList != NULL )
    {
        aIndex->nodeCount -= sDeleteNodeCount;

        smLayerCallback::addNodes2LogicalAger( mSmnbFreeNodeList,
                                               ( smnNode* )sFreeNodeList );
    }
    else
    {
        /* do nothing... */
    }

    IDE_EXCEPTION_CONT( SKIP_KEY_REORGANIZATION );

    IDU_FIT_POINT_RAISE( "BUG-46504@smnbBTree::keyReorganization::errNotStateZero", ERR_NOT_STATE_ZERO );
    IDE_ERROR_RAISE( sLockState == 0, ERR_NOT_STATE_ZERO );

    return IDE_SUCCESS;
    
    IDE_EXCEPTION( ERR_NOT_STATE_ZERO )
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_INDEX ) );
        ideLog::log( IDE_ERR_0, "[SM] lock state is not zero. [%"ID_UINT32_FMT"]", sLockState );
    }
    IDE_EXCEPTION_END;

    switch ( sNodeState )
    {
        case 5:     /* 인터널 노드의 slotcounter 정보 변경 복원 */
            sCurINode->mSlotCount++;
        case 4:     /* 인터널 노드의 slot 변경 복원 */
            /* 인터널 노드의 slot shift 복원 */
            shiftInternalSlots( sCurINode,
                                ( SShort )( sInternalSlotPos + 1 ),
                                ( SShort )( sCurINode->mSlotCount - 1 ),
                                ( SShort )(1) );

            /* 통합으로 사라질 노드와의 연결을 복원 */
            getLeafSlot( (SChar **)&sNewKeyRow,
                         &sNewKey,
                         sNxtLNode,
                         (SShort)( sNxtLNode->mSlotCount - 1 ) );
            setInternalSlot( sCurINode,
                             ( SShort )( sInternalSlotPos + 1 ),
                             (smnbNode *)sNxtLNode,
                             (SChar *)sNewKeyRow,
                             sNewKey );

            /* 통합대상인 리프노드의 키 값을 복원 */
            getLeafSlot( (SChar **)&sNewKeyRow,
                         &sNewKey,
                         sCurLNode,
                         (SShort)( sNodeOldCount - 1 ) ); 
            setInternalSlot( sCurINode,
                             ( SShort )sInternalSlotPos,
                             (smnbNode *)sCurLNode,
                             (SChar *)sNewKeyRow,
                             sNewKey );
            SMNB_SCAN_UNLATCH( sCurINode );

            lockNode( sCurLNode );
            sLockState = 2;
            lockNode( sNxtLNode );
            sLockState = 3;

            sDeleteNodeCount--;
        case 3:     /* 다음 리프 노드를 freelist에 연결한 변경을 복원 */
            sFreeNodeList = (smnbNode*)sNxtLNode->freeNodeList;
            sNxtLNode->freeNodeList = NULL;

            SMNB_SCAN_LATCH( sCurLNode );
            SMNB_SCAN_LATCH( sNxtLNode );
            sLockState = 4;
        case 2:     /* 통합이 수행된 노드의 메타정보 갱신을 복원 */
            sNxtLNode->flag |= SMNB_NODE_VALID;

            SMNB_SCAN_LATCH( sNxtLNode->nextSPtr );
            sNxtLNode->nextSPtr->prevSPtr = sNxtLNode;
            SMNB_SCAN_UNLATCH( sNxtLNode->nextSPtr );

            sCurLNode->mSlotCount = sNodeOldCount;
            sCurLNode->nextSPtr = sNxtLNode;

        case 1:     /* 다음 리프 노드의 slot을 현재 리프노드에 통합한 수정을 복원 */
            for ( i = 0; i < sNodeNewCount - sNodeOldCount; i++ )
            {
                setLeafSlot( sCurLNode,
                             (SShort)( sNodeNewCount - 1 - i ),
                             NULL,
                             NULL );
            }
        case 0:
        default:
            break;
    }

    switch ( sLockState )
    {
        case 6:
            SMNB_SCAN_UNLATCH( sNxtLNode->nextSPtr );
        case 5:
            unlockNode( sNxtLNode->nextSPtr );
        case 4:
            SMNB_SCAN_UNLATCH( sNxtLNode );
            SMNB_SCAN_UNLATCH( sCurLNode );
        case 3:
            unlockNode( sNxtLNode );
        case 2:
            unlockNode( sCurLNode );
        case 1:
            unlockTree( aIndex );
        case 0:
        default:
            break;
    }

    /* 작업이 완료 된 후 free list의 노드들을 정리한다. */
    if ( sFreeNodeList != NULL )
    {
        aIndex->nodeCount -= sDeleteNodeCount;

        smLayerCallback::addNodes2LogicalAger( mSmnbFreeNodeList,
                                               ( smnNode* )sFreeNodeList );
    }

    if ( ideGetErrorCode() == smERR_ABORT_INCONSISTENT_INDEX )
    {
        setInconsistentIndex( aIndex->mIndexHeader );
    }

    IDE_SET( ideSetErrorCode( smERR_ABORT_ReorgFail ) );

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::findNextSlotInLeaf              *
 * ------------------------------------------------------------------*
 * BUG-44043, BUG-46428
 *
 * 리프노드에서 aSearchRow 값보다 큰 첫번째 슬롯 위치를 찾아서
 * aSlot 으로 리턴한다.
 * ( 위치를 찾지 못하면 노드의 mSlotCount 값을 리턴한다. )
 *
 * aIndex     - [IN]  INDEX 정보
 * aNode      - [IN]  탐색할 리프노드
 * aMinimum   - [IN]  탐색할 노드범위의 mimumum slot 
 * aMaximum   - [IN]  탐색할 노드범위의 maximum slot
 * aSearchRow - [IN]  탐색할 키
 * aSlot      - [OUT] 탐색된 슬롯 위치
 *********************************************************************/
inline void smnbBTree::findNextSlotInLeaf( smnbHeader       * index,
                                           const smnbLNode  * aNode,
                                           SInt               aMinimum,
                                           SInt               aMaximum,
                                           SChar            * aSearchRow,
                                           SInt             * aSlot )
{
    smnbStatistic    * sIndexStat   = &index->mAgerStat;
    const smnbColumn * sColumns     = index->columns;
    const smnbColumn * sFence       = index->fence;

    SInt sMinimum = aMinimum;
    SInt sMaximum = aMaximum;
    SInt sMedium  = 0;
    SChar  * sRow = NULL;

    do
    {
        sMedium = ( sMinimum + sMaximum ) >> 1;

        sRow    = aNode->mRowPtrs[sMedium];

        if ( smnbBTree::compareRowsAndPtr( sIndexStat,
                                           sColumns,
                                           sFence,
                                           sRow,
                                           aSearchRow ) > 0 )
        {
            sMaximum = sMedium - 1;
            *aSlot   = sMedium;
        }
        else
        {
            sMinimum = sMedium + 1;
            *aSlot   = sMinimum;
        }
    }
    while ( sMinimum <= sMaximum );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnbBTree::setInconsistentIndex              *
 * ------------------------------------------------------------------*
 * BUG-46504
 *
 * 인덱스를 inconsistent 상태로 변경한다.
 * (inconsistent 상태가 된 인덱스는 rebuild 할때까지 사용할수없다.)
 *
 * aIndex     - [IN]  INDEX 헤더
 *********************************************************************/
void smnbBTree::setInconsistentIndex( smnIndexHeader * aIndex )
{
    IDE_DASSERT( aIndex != NULL );

    /* inconsistent 되지 않은 index만 inconsistent 상태로 만든다. */
    /* inconsistent로 변경중에 다른 TX가 끼어들수 있으나,
       TRACE LOG를 중복으로 찍게 되는것 이외에는 문제되지 않는다. */

    if ( smnManager::getIsConsistentOfIndexHeader( aIndex ) == ID_TRUE )
    {
        smnManager::setIsConsistentOfIndexHeader( aIndex, ID_FALSE );
    }
}

void smnbBTree::dumpNodeLink( smnbLNode * aLNode )
{
    if( aLNode != NULL )
    {
        if( aLNode->prevSPtr != NULL )
        {
            if( aLNode->nextSPtr != NULL )
            {
                /* 대상 노드의 이전/이후 링크가 모두 존재할 경우 */
                ideLog::log(IDE_SM_0,"NodePtr = %"ID_XPOINTER_FMT
                                     ", PrevPtr = %"ID_XPOINTER_FMT
                                     ", NextPtr = %"ID_XPOINTER_FMT"\n",
                                     aLNode,
                                     aLNode->prevSPtr,
                                     aLNode->nextSPtr );
            }
            else
            {
                /* 대상 노드의 이전 링크만 존재하고 이후 링크가 존재하지 않을 경우 */
                ideLog::log(IDE_SM_0,"NodePtr = %"ID_XPOINTER_FMT
                                     ", PrevPtr = %"ID_XPOINTER_FMT"\n",
                                     aLNode,
                                     aLNode->prevSPtr );
            }
        }
        else
        {
            if( aLNode->nextSPtr != NULL )
            {
                /* 대상 노드의 이후 링크만 존재하고 이전 링크가 존재하지 않을 경우 */
                ideLog::log(IDE_SM_0,"NodePtr = %"ID_XPOINTER_FMT
                                     ", nextPtr = %"ID_XPOINTER_FMT"\n",
                                     aLNode,
                                     aLNode->nextSPtr );
            }
            else
            {
                /* 대상 노드의 이전/이후 링크가 모두 존재하지 않을 경우 */
                ideLog::log(IDE_SM_0,"NodePtr = %"ID_XPOINTER_FMT"\n",
                                     aLNode );
            }
        }
    }

    return;
}
 
/*********************************************************************
 * Common Index Header 및 smnbModule Index Header를 출력한다.(BUG-47619)
 *********************************************************************/
void smnbBTree::logIndexHeader( smnIndexHeader * aCommonHeader )
{
    SChar      * sOutBuffer4Dump;

    /* Common Index Header 출력 */
    smnManager::logCommonHeader( aCommonHeader );

    if ( aCommonHeader->mHeader != NULL )
    {
        if ( iduMemMgr::calloc( IDU_MEM_SM_SMN, 1,
                                ID_SIZEOF( SChar ) * IDE_DUMP_DEST_LIMIT,
                                (void**)&sOutBuffer4Dump )
             == IDE_SUCCESS )
        {
            /* smnbModule Index Header 출력 */
            dumpIndexHeader( (smnbHeader*)(aCommonHeader->mHeader),
                             sOutBuffer4Dump,
                             IDE_DUMP_DEST_LIMIT );

            ideLog::log( IDE_ERR_0, "%s\n", sOutBuffer4Dump );

            (void) iduMemMgr::free( sOutBuffer4Dump );
        }
        else
        {
            /* alloc fail */
        }
    }
    else
    {
        ideLog::log( IDE_ERR_0,
                     "smnbIndexHeader* is NULL(smnbBTree::logIndexHeader())\n" );
    }
}

void smnbBTree::dumpIndexHeader( smnbHeader * aHeader,
                                 SChar      * aOutBuf,
                                 UInt         aOutSize )
{
    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "Memory Index Header :\n"
                     "Pointer             : 0x%"ID_xPOINTER_FMT"\n"
                     "Root node ptr       : 0x%"ID_xPOINTER_FMT"\n"
                     "First node ptr      : 0x%"ID_xPOINTER_FMT"\n"
                     "Last node ptr       : 0x%"ID_xPOINTER_FMT"\n"
                     "Node count          : %"ID_UINT32_FMT"\n"
                     "Is not null         : %"ID_UINT32_FMT"\n"
                     "Common header ptr   : 0x%"ID_xPOINTER_FMT"\n\n"
                     "cRef                : %"ID_UINT32_FMT"\n"
                     "Key size            : %"ID_UINT32_FMT"\n"
                     "Is partial key      : %"ID_UINT32_FMT"\n"
                     "I node max slot cnt : %"ID_INT32_FMT"\n"
                     "L node max slot cnt : %"ID_INT32_FMT"\n"
                     "Space ID            : %"ID_UINT32_FMT"\n"
                     "Fixed key Size      : %"ID_UINT32_FMT"\n"
                     "Slot align value    : %"ID_UINT32_FMT"\n"
                     "Built type          : %"ID_UINT32_FMT"\n",
                     aHeader,
                     aHeader->root,
                     aHeader->pFstNode,
                     aHeader->pLstNode,
                     aHeader->nodeCount,
                     aHeader->mIsNotNull,
                     aHeader->mIndexHeader,
                     aHeader->cRef,
                     aHeader->mKeySize,
                     aHeader->mIsPartialKey,
                     aHeader->mINodeMaxSlotCount,
                     aHeader->mLNodeMaxSlotCount,
                     aHeader->mSpaceID,
                     aHeader->mFixedKeySize,
                     aHeader->mSlotAlignValue,
                     aHeader->mBuiltType );
}

/*********************************************************************
 * index 를 탑색하여 key를 찾지 못한 예외 발생 상황에서 호출되어
 * leaf node를 fullscan 해서 key를 찾아서 정리한다.(BUG-47618)
 *
 * 반드시 찾아야만 하는데 찾지 못한 경우에 호출되는 함수이다.
 * 탐색에 문제가 있거나 찾지 못하면 추가 예외가 발생된다.
 * 
 * 찾아으면 Internal node 문제이다.
 * 못찾으면 key가 아예 존재하지 않는 문제이다.
 *
 * internal node를 탐색하지 않아서 iNode Stack이 없다.
 * 그러므로 internal node가 변경되지 않는 선에서 수정한다.
 * (node에 key가 1개 뿐이거나, last key이면 정리하지 못한다.)
 *********************************************************************/
IDE_RC smnbBTree::freeSlotByLeafFullScan( smnbHeader   * aIndexHeader,
                                          SChar        * aRow )
{
    smnbINode * sCurINode;
    smnbLNode * sLeafNode;
    SInt        i;
    SInt        sKeyPos   = -1;
    idBool      sIsTreeLocked = ID_FALSE;
    idBool      sIsNodeLocked = ID_FALSE;
    IDU_LATCH   sVersion   = IDU_LATCH_UNLOCKED;

    /* 1. Tree Lock을 잡는다.
     * 2. internal node의 mChildPtrs[0] 으로 내려간다.
     * 3. leaf node를 발견하면 첫번째 ptr부터 row ptr 과 비교,
     * 4.  발견하면 key를 free한다. */
    lockTree( aIndexHeader );
    sIsTreeLocked = ID_TRUE;

    sCurINode = (smnbINode*)(aIndexHeader->root);

    IDE_ERROR( sCurINode != NULL );

    /* Internal node scan, 0번 slot을 따라가서 Index의 첫번째 leaf node를 찾는다.*/
    while((sCurINode->flag & SMNB_NODE_TYPE_MASK)== SMNB_NODE_TYPE_INTERNAL)
    {
        sCurINode = (smnbINode*)(sCurINode->mChildPtrs[0]);
        IDE_ERROR( sCurINode != NULL );
    }

    IDE_ERROR((sCurINode->flag & SMNB_NODE_TYPE_MASK) == SMNB_NODE_TYPE_LEAF);

    sLeafNode = (smnbLNode*)sCurINode;

    do
    {
        IDE_ERROR((sCurINode->flag & SMNB_NODE_TYPE_MASK) == SMNB_NODE_TYPE_LEAF);

        /* leaf node full scan,
         * 탐색 단계에서 node lock을 잡지 않아도 된다.
         * tree lock 이 잡혀 있어서 node 간 key 이동이 되지 않는다.
         * leaf node간 key 이동 하는 경우 last key가 변경되므로
         * internal node의 변경이 일어난다. tree lock을 잡아야 변경 가능하다.*/
         while(1)
         {
             sVersion  = getLatchValueOfLNode( sLeafNode ) & IDU_LATCH_UNMASK;

             IDL_MEM_BARRIER;

             for( i = 0 ; i< sLeafNode->mSlotCount ; i++ )
             {
                 if( sLeafNode->mRowPtrs[i] == aRow )
                 {
                     /* 찾았다. */
                     sKeyPos = i;
                     break;
                 }
             }

             if( sVersion == getLatchValueOfLNode( sLeafNode ) )
             {
                 break;
             }

             sKeyPos = -1;
         }

        if ( sKeyPos != -1 )
        {
            break;
        }

        sLeafNode = sLeafNode->nextSPtr;

    }while( sLeafNode != NULL );

    /* 1. Key를 찾지 못하면 예외 처리
     * 2. key가 하나뿐인 Leaf node는 처리 할 수 없다..
     * 3. 내가 마지막 key 여서도 처리 할 수 없다.
     * 2,3 internal을 탐색하지 않아서 internal node stack 이 없다.
     *     leaf node만 수정 가능하다면 처리하고 아니면 예외 발생 한다.*/
    IDE_ERROR( sLeafNode != NULL );
    IDE_ERROR( sKeyPos != -1 );

    lockNode( sLeafNode );
    sIsNodeLocked = ID_TRUE;

    /* lock을 잡기 전에 위치가 변경 될 수 있다. */
    if( sLeafNode->mRowPtrs[sKeyPos] != aRow )
    {
        sKeyPos = -1 ;
        for( i = 0 ; i< sLeafNode->mSlotCount ; i++ )
        {
            if( sLeafNode->mRowPtrs[i] == aRow )
            {
                /* 찾았다.*/
                sKeyPos = i;
                break;
            }
        }
        IDE_ERROR( sKeyPos != -1 );
    }

    IDE_TEST( sLeafNode->mSlotCount <= 1 );
    IDE_TEST(( sLeafNode->mSlotCount - 1 ) <= sKeyPos );

    SMNB_SCAN_LATCH( sLeafNode );

    deleteSlotInLeafNode( sLeafNode,
                          aRow,
                          &sKeyPos );

    SMNB_SCAN_UNLATCH( sLeafNode );

    IDE_ERROR( sKeyPos != -1 );

    sIsNodeLocked = ID_FALSE;
    unlockNode( sLeafNode );

    sIsTreeLocked = ID_FALSE;
    unlockTree( aIndexHeader );

    ideLog::log(IDE_ERR_0,"remove key by leaf node fullscan\n");

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sIsNodeLocked == ID_TRUE )
    {
        sIsNodeLocked = ID_FALSE;
        unlockNode( sLeafNode );
    }

    if ( sIsTreeLocked == ID_TRUE )
    {
        sIsTreeLocked = ID_FALSE;
        unlockTree( aIndexHeader );
    }

    ideLog::log(IDE_ERR_0,"not found key by leaf node fullscan\n");

    return IDE_FAILURE;
}

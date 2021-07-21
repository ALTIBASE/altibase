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
 * $Id: stnmrModule.cpp 17358 2006-07-31 03:12:47Z bskim $
 *
 * Description :
 *
 * 시공간 인덱스( Spatio-Temporal Index : stnmrModule.cpp )
 *
 * 1. 설계개요
 *
 * 2. 설계구조
 *
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <idu.h>
#include <smp.h>
#include <smc.h>
#include <smm.h>
#include <smu.h>
#include <smDef.h>

#include <smnReq.h>
#include <stnmrDef.h>
#include <stErrorCode.h>
#include <smnManager.h>
#include <sgmManager.h>
#include <stnmrModule.h>
#include <stix.h>
#include <stdUtils.h>
#include <smiDef.h>

extern smiGlobalCallBackList gSmiGlobalCallBackList;

smmSlotList         gSmnrNodePool;
static void*        gSmnrFreeNodeList;

smnIndexModule stnmrModule =
{
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_MEMORY,
                              SMI_ADDITIONAL_RTREE_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE | SMN_DIMENSION_ENABLE | SMN_DEFAULT_DISABLE |
        SMN_BOTTOMUP_BUILD_DISABLE | SMN_FREE_NODE_LIST_ENABLE,
    ID_UINT_MAX,  // BUG-23113
    (smnMemoryFunc)       stnmrRTree::prepareIteratorMem,
    (smnMemoryFunc)       stnmrRTree::releaseIteratorMem,
    (smnMemoryFunc)       stnmrRTree::prepareFreeNodeMem,
    (smnMemoryFunc)       stnmrRTree::releaseFreeNodeMem,
    (smnCreateFunc)       stnmrRTree::create,
    (smnDropFunc)         stnmrRTree::drop,

    (smTableCursorLockRowFunc)  smnManager::lockRow,
    (smnDeleteFunc)             stnmrRTree::deleteRowNA,
    (smnFreeFunc)               stnmrRTree::freeSlot,
    (smnInsertRollbackFunc)     NULL,
    (smnDeleteRollbackFunc)     NULL,
    (smnAgingFunc)              NULL,

    (smInitFunc)          stnmrRTree::init,
    (smDestFunc)          stnmrRTree::dest,
    (smnFreeNodeListFunc) stnmrRTree::freeAllNodeList,
    (smnGetPositionFunc)  stnmrRTree::getPositionNA,
    (smnSetPositionFunc)  stnmrRTree::setPositionNA,

    (smnAllocIteratorFunc) stnmrRTree::allocIterator,
    (smnFreeIteratorFunc)  stnmrRTree::freeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,

    (smnMakeDiskImageFunc)stnmrRTree::makeDiskImage,

    (smnBuildIndexFunc)     stnmrRTree::buildIndex,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         NULL
};

static const  smSeekFunc stnmrSeekFunctions[32][12] =
{
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)stnmrRTree::fetchNext,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirstU,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::fetchNext,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirstR,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::fetchNext,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)stnmrRTree::fetchNext,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirstU,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst,
        (smSeekFunc)stnmrRTree::fetchNext,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::beforeFirst
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA        
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
        (smSeekFunc)stnmrRTree::NA,
    }
};

IDE_RC stnmrRTree::prepareIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

IDE_RC stnmrRTree::releaseIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}


IDE_RC stnmrRTree::prepareFreeNodeMem( const smnIndexModule* )
{
    static idBool sIsInit = ID_FALSE;
    
    if(sIsInit == ID_FALSE)
    {
        sIsInit = ID_TRUE;
        IDE_TEST( gSmnrNodePool.initialize( IDU_MEM_SM_INDEX,
                                            "STNMR_RTREE_NODE_POOL",
                                            ID_SIZEOF(stnmrNode), 
                                            STNMR_NODE_POOL_MAXIMUM,
                                            STNMR_NODE_POOL_CACHE )
                  != IDE_SUCCESS );

        smnManager::allocFreeNodeList( (smnFreeNodeFunc)stnmrRTree::freeNode,
                                       &gSmnrFreeNodeList);
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

IDE_RC stnmrRTree::releaseFreeNodeMem( const smnIndexModule* )
{
    static idBool sIsRelease = ID_FALSE;

    if(sIsRelease == ID_FALSE)
    {
        sIsRelease = ID_TRUE;
        
        smnManager::destroyFreeNodeList(gSmnrFreeNodeList);
        
        IDE_TEST( gSmnrNodePool.release() != IDE_SUCCESS );
        IDE_TEST( gSmnrNodePool.destroy() != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}


IDE_RC stnmrRTree::freeAllNodeList( idvSQL*         /*aStatistics*/, 
                                    smnIndexHeader* /*aIndex*/,
                                    void*           /*aTrans*/ )
{
    return IDE_SUCCESS;
}


IDE_RC stnmrRTree::freeNode( void* aNodes )
{

    smmSlot*   sSlots;
    smmSlot*   sSlot;
    stnmrNode* sNodes;
    UInt       sCount;

    sNodes = (stnmrNode*)aNodes;
    
    if( sNodes != NULL )
    {
        sSlots        = (smmSlot*)sNodes;
        sNodes        = sNodes->mFreeNodeList;
        sSlots->prev  = sSlots;
        sSlots->next  = sSlots;
        sCount        = 1;

        while( sNodes != NULL )
        {
            sSlot             = (smmSlot*)sNodes;
            sNodes            = sNodes->mFreeNodeList;
            sSlot->prev       = sSlots;
            sSlot->next       = sSlots->next;
            sSlots->next      = sSlot;            
            sSlot->next->prev = sSlot;
            sCount++;
        }

        IDE_TEST( gSmnrNodePool.releaseSlots( sCount, sSlots )
                  != IDE_SUCCESS );
    }

    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

IDE_RC stnmrRTree::create( idvSQL*               /*aStatistics*/,
                           smcTableHeader*       aTable,
                           smnIndexHeader*       aIndex,
                           smiSegAttr*           /*aSegAttr*/,
                           smiSegStorageAttr*    /*aSegStoAttr*/,
                           smnInsertFunc*        aInsert,
                           smnIndexHeader**      aRebuildIndexHeader,
                           ULong                  /*aSmoNo*/ )
{

    stnmrHeader* sHeader;
    UInt         sStage = 0;
    UInt         sIndexCount;
    const smiColumn*   sColumn;
    SChar sBuffer[128];
    
    idlOS::memset(sBuffer, 0, 128);
    
    idlOS::snprintf(sBuffer, 128, "STNMR_MUTEX_%"ID_UINT64_FMT,
                   (ULong)(aIndex->mId));


    //fix BUG-23023 런타임 헤더 생성
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 ID_SIZEOF(stnmrHeader),
                                 (void**)&sHeader )
              != IDE_SUCCESS );
    sStage = 1;

    //fix BUG-23023 칼럼 생성
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_SM_SMN,
                                 ID_SIZEOF(stnmrColumn) * (aIndex->mColumnCount),
                                 (void**)&(sHeader->mColumns) )
              != IDE_SUCCESS );
    sStage = 2;
    
    IDE_TEST( sHeader->mMutex.initialize(sBuffer,
                                         IDU_MUTEX_KIND_NATIVE,
                                         IDV_WAIT_INDEX_NULL )
              != IDE_SUCCESS );
    sStage = 3;

    IDE_TEST( gSmnrNodePool.makeChild( SMM_SLOT_LIST_MAXIMUM_DEFAULT, 
                                       SMM_SLOT_LIST_CACHE_DEFAULT,
                                       &sHeader->mNodePool )
              != IDE_SUCCESS );
        
    sHeader->mRoot         = NULL;
    sHeader->mIsCreated    = ID_FALSE;
    sHeader->mIsConsistent = ID_TRUE;
    sHeader->mFstNode      = NULL;
    sHeader->mLstNode      = NULL;
    sHeader->mNodeCount    = 0;
    sHeader->mVersion      = 0;
    sHeader->mRootVersion  = ID_ULONG_MAX;
    sHeader->mLatch        = IDU_LATCH_UNLOCKED;
    
    sHeader->mFence = sHeader->mColumns + aIndex->mColumnCount;
    
    for( sIndexCount = 0; sIndexCount < aIndex->mColumnCount; sIndexCount++ )
    {
        IDE_TEST_RAISE( (aIndex->mColumns[sIndexCount] & SMI_COLUMN_ID_MASK )
                        >= aTable->mColumnCount, ERR_COLUMN_NOT_FOUND );
        sColumn = smcTable::getColumn( aTable,
                                       (aIndex->mColumns[sIndexCount] 
                                       & SMI_COLUMN_ID_MASK) );
        
        IDE_TEST_RAISE( sColumn->id != aIndex->mColumns[sIndexCount],
                        ERR_COLUMN_NOT_FOUND );
        idlOS::memcpy( &sHeader->mColumns[sIndexCount].mColumn,
                       sColumn, ID_SIZEOF(smiColumn) );

        IDE_TEST( gSmiGlobalCallBackList.findCompare( sColumn,
                    aIndex->mColumnFlags[sIndexCount],
                    &sHeader->mColumns[sIndexCount].mCompare) != IDE_SUCCESS );
    }

    IDE_TEST( smnManager::initIndexStatistics( aIndex,
                                               (smnRuntimeHeader*)sHeader ,
                                               aIndex->mId )
              != IDE_SUCCESS );
    
    aIndex->mHeader = (smnRuntimeHeader*) sHeader;
    
    if(aRebuildIndexHeader != NULL)
    {
        if(*aRebuildIndexHeader != NULL)
        {
            /* nothing to do */
        }
        else
        {
            *aRebuildIndexHeader = aIndex;
        }
    }
    
    if( ( aIndex->mFlag & SMI_INDEX_UNIQUE_MASK ) == SMI_INDEX_UNIQUE_ENABLE )
    {
        *aInsert = stnmrRTree::insertRowUnique;
    }
    else
    {
        *aInsert = stnmrRTree::insertRow;
    }
   
    sHeader->mKeyCount = ((smcTableHeader *)aTable)->mFixed.mMRDB.mRuntimeEntry->mInsRecCnt;
    
    idlOS::memset( &(sHeader->mStmtStat), 0x00, ID_SIZEOF(stnmrStatistic) );
    idlOS::memset( &(sHeader->mAgerStat), 0x00, ID_SIZEOF(stnmrStatistic) );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND );
    IDE_SET( ideSetErrorCode( stERR_FATAL_COLUMN_NOT_FOUND ) );    

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sStage )
    {
        case 3:
            (void) sHeader->mMutex.destroy();
        case 2:
            (void) iduMemMgr::free( sHeader->mColumns ) ;
        case 1:
            (void) iduMemMgr::free( sHeader ) ;
        default :
            break;
    }
    IDE_POP();

    aIndex->mHeader = NULL;

    return IDE_FAILURE;
    
}

IDE_RC stnmrRTree::buildIndex( idvSQL*               aStatistics,
                               void*                 /* aTrans */,
                               smcTableHeader*       aTable,
                               smnIndexHeader*       aIndex,
                               smnGetPageFunc        aGetPageFunc,
                               smnGetRowFunc         aGetRowFunc,
                               SChar*                /* aNullPtr */,
                               idBool                aIsNeedValidation,
                               idBool                /* aIsEnableTable */,
                               UInt                  /* aParallelDegree */,
                               UInt                  /*aBuildFlag*/,
                               UInt                  /*aTotalRecCnt*/ )
{
    smnIndexModule    * sIndexModules;
    scPageID            sPageID;
    SChar             * sFence;
    SChar             * sRow;
    idBool              sLocked = ID_FALSE;

    sIndexModules = aIndex->mModule;

    ideLog::log( IDE_SM_0, "\
=========================================\n\
 [MEM_IDX_CRE] 0. BUILD INDEX            \n\
                  NAME : %s              \n\
                  ID   : %u              \n\
=========================================\n\
          BUILD_THREAD_COUNT     : %d    \n\
=========================================\n",
                 aIndex->mName,
                 aIndex->mId,
                 1 );
    
    sPageID = SM_NULL_OID;

    while( 1 )
    {
        IDE_TEST( aGetPageFunc( aTable, &sPageID, &sLocked ) != IDE_SUCCESS );

        if( sPageID == SM_NULL_OID )
        {
            break;
        }

        sRow = NULL;

        while( 1 )
        {
            // BUG-29134: stnmrHeader를 smnbHeader로 사용하는 문제
            IDE_TEST( aGetRowFunc( aTable,
                                   sPageID,
                                   &sFence,
                                   &sRow,
                                   aIsNeedValidation )
                      != IDE_SUCCESS );

            if( sRow == NULL )
            {
                break;
            }

            IDE_TEST( insertRow( NULL,         /*idvSQL* */
                                 NULL,         /*aTrans*/
                                 NULL,         /*aTable*/ 
                                 aIndex, 
                                 SM_SCN_INIT,  /*aInfiniteSCN*/
                                 sRow, 
                                 NULL,         /*aNull*/
                                 ID_TRUE,      /*aUniqueCheck*/
                                 SM_SCN_INIT,  /*aStmtViewSCN*/
                                 NULL,         /*aRowSID*/
                                 NULL,         /*aExistUniqueRow*/
                                 ID_ULONG_MAX, /*aInsertWaitTime*/
                                 ID_FALSE )    /*aForbiddenToRetry*/
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    (void)(sIndexModules)->mFreeAllNodeList( aStatistics,
                                             aIndex,
                                             NULL );
    if( sLocked == ID_TRUE )
    {
        (void)smnManager::releasePageLatch( aTable, sPageID );
        sLocked = ID_FALSE;
    }
    IDE_POP();

    return IDE_FAILURE;
}

IDE_RC stnmrRTree::drop( smnIndexHeader * aIndex )
{
    stnmrHeader *sHeader;
    smmSlot      sSlots = { &sSlots, &sSlots  };
    UInt         sSlotCount;

    sHeader  = (stnmrHeader*)aIndex->mHeader;

    if( sHeader != NULL)
    {
        if(sHeader->mRoot != NULL)
        {
            sSlotCount = getSlots( sHeader->mRoot, &sSlots );
            if( sSlotCount != 0 )
            {
                sSlots.prev->next = sSlots.next;
                sSlots.next->prev = sSlots.prev;
                IDE_TEST( sHeader->mNodePool.releaseSlots( sSlotCount, 
                    sSlots.next, SMM_SLOT_LIST_MUTEX_NEEDLESS ) 
                    != IDE_SUCCESS );
            }
        }
        
        IDE_TEST( sHeader->mNodePool.release() != IDE_SUCCESS );
        IDE_TEST( sHeader->mNodePool.destroy() != IDE_SUCCESS );

        IDE_TEST( smnManager::destIndexStatistics( aIndex,
                                                   (smnRuntimeHeader*)sHeader )
                  != IDE_SUCCESS );
        
        IDE_TEST( sHeader->mMutex.destroy() != IDE_SUCCESS );
        IDE_TEST( iduMemMgr::free( sHeader->mColumns ) !=  IDE_SUCCESS );  //fix BUG-23023
        IDE_TEST( iduMemMgr::free( sHeader ) !=  IDE_SUCCESS );
        aIndex->mHeader = NULL;
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

IDE_RC stnmrRTree::init( stnmrIterator       * aIterator,
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
    idvSQL*    sSQLStat;

    aIterator->mSCN            = aSCN;
    aIterator->mInfinite       = aInfinite;
    aIterator->mTrans          = aTrans;
    aIterator->mTable          = aTable;
    aIterator->mCurRecPtr      = NULL;
    aIterator->mLstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->mTid            = smLayerCallback::getTransID( aTrans );
    aIterator->mIndex          = (stnmrHeader*)aIndex->mHeader;
    aIterator->mKeyRange       = aKeyRange;
    aIterator->mRowFilter      = aRowFilter;
    aIterator->mFlag           = aUntouchable == ID_TRUE 
                                 ? SMI_ITERATOR_READ : SMI_ITERATOR_WRITE;
    aIterator->mProperties     = aProperties;
    aIterator->mStatement      = aStatement;
    

    IDL_MEM_BARRIER;
    
    *aSeekFunc = stnmrSeekFunctions[aFlag&(SMI_TRAVERSE_MASK|SMI_PREVIOUS_MASK|
                                               SMI_LOCK_MASK)];
    
    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mMemCursorIndexScan, 1);
    
    if (sSQLStat != NULL)
    {
        IDV_SESS_ADD(sSQLStat->mSess, IDV_STAT_INDEX_MEM_CURSOR_IDX_SCAN, 1);
    }
   
    return IDE_SUCCESS;

}

IDE_RC stnmrRTree::dest( stnmrIterator* /*aIterator*/ )
{
    return IDE_SUCCESS;
}


UInt stnmrRTree::getSlots( stnmrNode* aNode,
                           smmSlot*   aSlots )
{
    
    smmSlot* sSlot;
    SInt     sSlotCount;
    SInt     i;
    
    if( aNode->mFlag != STNMR_NODE_TYPE_LEAF )
    {
        sSlotCount        = 1;
        for( i = 0; i < aNode->mSlotCount; i++)
        {
            sSlotCount += getSlots( (stnmrNode*)aNode->mSlots[i].mPtr, aSlots );
        }
    }
    else
    {
        sSlotCount        = 1;
    }

    sSlot = (smmSlot*)aNode;
    sSlot->prev       = aSlots;
    sSlot->next       = aSlots->next;
    sSlot->prev->next = sSlot;
    sSlot->next->prev = sSlot;
    
    return sSlotCount;
    
}

void stnmrRTree::insertNode( stnmrNode*  aNewNode,
                             stdMBR*     aMbr,
                             void*       aPtr,
                             ULong       aVersion )
{

    aNewNode->mSlots[aNewNode->mSlotCount].mMbr = *aMbr;
    aNewNode->mSlots[aNewNode->mSlotCount].mPtr =  aPtr;
    aNewNode->mSlots[aNewNode->mSlotCount].mVersion = aVersion;
    
    aNewNode->mSlotCount++;
}

SInt stnmrRTree::bestentry( stnmrNode* aNode,
                           stdMBR*     aMbr)
                               
{

    SDouble    sMinArea;
    SDouble    sMinDelta;
    SDouble    sDelta;
    SInt       i;
    UInt       sMini;
    stnmrNode* sCurNode;
   
    sCurNode = aNode;
    sMini    = 0;
    
    sMinArea  = stdUtils::getMBRArea(&sCurNode->mSlots[0].mMbr);
    sMinDelta = stdUtils::getMBRDelta(&sCurNode->mSlots[0].mMbr, aMbr);

    for(i = 1; i < sCurNode->mSlotCount; i++)
    {
        sDelta = stdUtils::getMBRDelta(&sCurNode->mSlots[i].mMbr, aMbr);

        if( (sDelta < sMinDelta) ||((sDelta == sMinDelta) && (sMinArea > 
            stdUtils::getMBRArea(&sCurNode->mSlots[i].mMbr))) )
        {
            sMinDelta = sDelta;
            sMini     = i;
            sMinArea = stdUtils::getMBRArea(&sCurNode->mSlots[i].mMbr);
        }
    }
	
    return sMini;
    
}

stnmrNode* stnmrRTree::chooseLeaf( stnmrHeader*  aHeader,
                                   stdMBR*       aMbr,
                                   stnmrStack*   aStack,
                                   SInt*         aDepth )
{

    stnmrNode*  sCurNode;
    SInt        sDepth = -1;
    
    sCurNode = aHeader->mRoot;

    SInt s_nReadPos;

    if(sCurNode != NULL)
    {
        while(1)
        {
            if((sCurNode->mFlag & STNMR_NODE_TYPE_MASK ) == 
                STNMR_NODE_TYPE_LEAF)
            {
                break;
            }
            
            s_nReadPos = bestentry(sCurNode, aMbr);

            aStack->mNodePtr    = sCurNode;
            aStack->mLstReadPos = s_nReadPos;

            aStack++;
            sDepth++;

            IDE_ASSERT(sCurNode->mSlots[s_nReadPos].mVersion == 
                ((stnmrNode*)sCurNode->mSlots[s_nReadPos].mPtr)->mVersion);
            
            sCurNode = (stnmrNode*)sCurNode->mSlots[s_nReadPos].mPtr;
        }
    }

    *aDepth = sDepth;
    
    return sCurNode;

}

IDE_RC stnmrRTree::insertRowUnique( idvSQL          * aStatistics,
                                    void            * aTrans,
                                    void            * aTable,
                                    void            * aIndex,
                                    smSCN             aInfiniteSCN,
                                    SChar           * aRow,
                                    SChar           * aNull,
                                    idBool            aUniqueCheck,
                                    smSCN             /*aStmtViewSCN*/,
                                    void            * aRowSID,
                                    SChar          ** /*aExistUniqueRow*/,
                                    ULong             /* aInsertWaitTime */,
                                    idBool            aForbiddenToRetry )
{

    stnmrHeader    * sHeader;
    
    sHeader  = (stnmrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    if(compareRows(sHeader->mColumns, sHeader->mFence, aRow, aNull ) != 0)
    {
        aNull = NULL;
    }
    
    IDE_TEST(insertRow( aStatistics,
                        aTrans, 
                        aTable, 
                        aIndex, 
                        aInfiniteSCN,
                        aRow, 
                        aNull, 
                        aUniqueCheck, 
                        SM_SCN_INIT, /*aStmtViewSCN*/
                        aRowSID, 
                        NULL,         /*aExistUniqueRow*/
                        ID_ULONG_MAX, /*aInsertWaitTime*/
                        aForbiddenToRetry )
             != IDE_SUCCESS);
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

void stnmrRTree::propagate( stnmrHeader*    aHeader,
                            stnmrStatistic* aIndexStat,
                            stnmrNode*      aNode,
                            UInt            aUpdatePos,
                            stnmrStack*     aStack )
{

    stnmrStack* sStack;
    stnmrNode*  sCurNode;
    stnmrNode*  sParent;
    SInt        sCurPos;
    SInt        sParPos;    
    SInt        sCount = 0;
   
    sStack    = aStack;
    sCurNode  = aNode;

    sCurPos   = sCurNode->mSlotCount-1;
    sParPos   = sStack->mLstReadPos;
    sParent   = NULL;

    while(1)
    {
        sCount++;

        //Latch Parent Node
        sParent = sStack->mNodePtr;

        IDE_ASSERT(sParent != aNode);
        
        STNMR_NODE_LATCH(sParent);

        aIndexStat->mKeyCompareCount++;
        if(stdUtils::isMBRContains(&sParent->mSlots[sParPos].mMbr, 
                                   &sCurNode->mSlots[sCurPos].mMbr)
           == ID_TRUE)
        {
            if(aUpdatePos != ID_UINT_MAX)
            {
                aIndexStat->mKeyCompareCount++;
                if(stdUtils::isMBRContains(
                        &sParent->mSlots[sParPos].mMbr, 
                        &sCurNode->mSlots[aUpdatePos].mMbr) == ID_TRUE)
                {
                    sParent->mSlots[sParPos].mPtr = sCurNode;
                    sParent->mSlots[sParPos].mVersion = sCurNode->mVersion;

                    //UnLatch Parent Node
                    STNMR_NODE_UNLATCH(sParent);

                    break;
                }
            }
            else
            {
                sParent->mSlots[sParPos].mPtr = sCurNode;
                sParent->mSlots[sParPos].mVersion = sCurNode->mVersion;

                //UnLatch Parent Node
                STNMR_NODE_UNLATCH(sParent);

                break;
            }
        }

        if(aUpdatePos != ID_UINT_MAX)
        {
            stdUtils::getMBRExtent(
                &sParent->mSlots[sParPos].mMbr, 
                &sCurNode->mSlots[aUpdatePos].mMbr);
            aUpdatePos = ID_UINT_MAX;
        }

        stdUtils::getMBRExtent( &sParent->mSlots[sParPos].mMbr,
                                &sCurNode->mSlots[sCurPos].mMbr);

        sParent->mSlots[sParPos].mPtr = sCurNode;
        sParent->mSlots[sParPos].mVersion = sCurNode->mVersion;
        
        STNMR_NODE_UNLATCH(sParent);
        
        sCurNode = sParent;
        
        if(sParent != aHeader->mRoot)
        {
            sStack--;
            sCurPos = sParPos;
            sParPos = sStack->mLstReadPos;
            
            sParent = sStack->mNodePtr;
        }
        else
        {
            break;
        }
    }

}

void stnmrRTree::split( stnmrHeader*    aHeader,
                        stnmrNode*      aLNode,
                        stnmrNode*      aRNode,
                        UInt            aPos,
                        stdMBR*         aChildMBR,
                        void*           aChildPtr,
                        ULong           aChildVersion,
                        stdMBR*         aInsertMBR,
                        void*           aInsertPtr,
                        ULong           aInsertVersion,
                        stdMBR*         aMbr1,
                        stdMBR*         aMbr2)
{

    SInt       sMax;
    SInt       sMin;
    SInt       sChooseNode;
    stnmrSlot  sTmpSlotList[STNMR_SLOT_MAX+1];

    IDL_MEM_BARRIER;
    
    initNode(aRNode, aLNode->mFlag, IDU_LATCH_UNLOCKED);

    STNMR_NODE_LATCH(aRNode);
    
    aRNode->mVersion = aLNode->mVersion;
    
    //Pick Seed And Split and Propagate
    idlOS::memcpy(sTmpSlotList, 
                  aLNode->mSlots, 
                  STNMR_SLOT_MAX * ID_SIZEOF(stnmrSlot));

    //aLNode를 초기화하면 next가 NULL이 되기 때문에 여기서 먼저 
    //RNode의 Link를 연결한다.
    aRNode->mNextSPtr = aLNode->mNextSPtr;

    initNode(aLNode, aLNode->mFlag, IDU_LATCH_LOCKED | aLNode->mLatch);

    aLNode->mVersion = ++(aHeader->mVersion);
    
    if(aPos != ID_UINT_MAX)
    {
        sTmpSlotList[aPos].mMbr     = *aChildMBR;
        sTmpSlotList[aPos].mPtr     =  aChildPtr;
        sTmpSlotList[aPos].mVersion = aChildVersion;
    }
    
    sTmpSlotList[STNMR_SLOT_MAX].mMbr     = *aInsertMBR;
    sTmpSlotList[STNMR_SLOT_MAX].mPtr     =  aInsertPtr;
    sTmpSlotList[STNMR_SLOT_MAX].mVersion = aInsertVersion;
    
    pickSeed(sTmpSlotList, STNMR_SLOT_MAX + 1);

    *aMbr1 = sTmpSlotList[STNMR_SLOT_MAX - 1].mMbr;
    *aMbr2 = sTmpSlotList[STNMR_SLOT_MAX].mMbr;

    insertSlot(aLNode, &sTmpSlotList[STNMR_SLOT_MAX-1]);
    insertSlot(aRNode, &sTmpSlotList[STNMR_SLOT_MAX]);

    sMax = STNMR_SLOT_MAX - 1;
    sMin = STNMR_SLOT_MAX / 2;

    while(sMax != 0)
    {
        if(aLNode->mSlotCount == sMin)
        {
            sChooseNode = 1;
        }
        else
        {
            if(aRNode->mSlotCount == sMin)
            {
                sChooseNode = 0;
            }
            else
            {
                sChooseNode = pickNext(sTmpSlotList, sMax, aMbr1, aMbr2);
            }
        }

        if(sChooseNode == 0)
        {
            insertSlot(aLNode, &sTmpSlotList[sMax - 1]);
            stdUtils::getMBRExtent(aMbr1, &sTmpSlotList[sMax - 1].mMbr);
        }
        else
        {
            insertSlot(aRNode, &sTmpSlotList[sMax - 1]);
            stdUtils::getMBRExtent(aMbr2, &sTmpSlotList[sMax - 1].mMbr);
        }
        sMax--;
    }
    
    IDL_MEM_BARRIER;
    
    //Link Sibling Node
    IDL_MEM_BARRIER;
    aLNode->mNextSPtr = aRNode;

    STNMR_NODE_UNLATCH(aRNode);
}

void stnmrRTree::pickSeed( stnmrSlot* aSlots,
                           SInt       aCount )
{

    SDouble   sMaxArea;
    SDouble   sTmpArea;
    SInt      i;
    SInt      j;
    SInt      sSeed1 = 0;
    SInt      sSeed2 = 1;
    stdMBR    sMbrTemp;
    stnmrSlot sTmpSlot;

    stdUtils::copyMBR(&sMbrTemp, &aSlots[0].mMbr);
    sMaxArea = 0;
    
    for(i = 0; i < aCount - 1 ; i++)
    {
        for(j = i + 1; j < aCount; j++)
        {
            sMbrTemp = aSlots[i].mMbr;
            
            sTmpArea = stdUtils::getMBRArea(
                                stdUtils::getMBRExtent(&sMbrTemp, 
                                                       &aSlots[j].mMbr));
            sTmpArea -= (stdUtils::getMBRArea(&aSlots[i].mMbr) +
                         stdUtils::getMBRArea(&aSlots[j].mMbr));
            
            if(sTmpArea > sMaxArea)
            {
                sMaxArea = sTmpArea;
                sSeed1 = i;
                sSeed2 = j;
            }
        }
    }

    sTmpSlot              = aSlots[sSeed1];
    aSlots[sSeed1]        = aSlots[aCount - 2];
    aSlots[aCount - 2]    = sTmpSlot;
    
    sTmpSlot              = aSlots[sSeed2];
    aSlots[sSeed2]        = aSlots[aCount-1];
    aSlots[aCount-1]      = sTmpSlot;
    
}

SInt stnmrRTree::pickNext( stnmrSlot*  aSlots,
                           SInt        aCount,
                           stdMBR*     aMbr1,
                           stdMBR*     aMbr2 )
{

    SDouble   sDeltaA;
    SDouble   sDeltaB;
    SDouble   sMinDelta;
    stnmrSlot sTmpSlot;
    
    SInt      i;
    SInt      sPos;
    SInt      sChooseNode;
    
    sDeltaA = stdUtils::getMBRDelta( &aSlots[0].mMbr, aMbr1 );
    sDeltaB = stdUtils::getMBRDelta( &aSlots[0].mMbr, aMbr2 );

    if(sDeltaA >  sDeltaB)
    {
        sMinDelta = sDeltaB;
        sChooseNode = 1;
        sPos = 0;        
    }
    else
    {
        sMinDelta = sDeltaA;
        sChooseNode = 0;
        sPos = 0;
    }
    
    for(i = 1; i < aCount ; i++)
    {
        sDeltaA = stdUtils::getMBRDelta(&aSlots[i].mMbr, aMbr1);
        sDeltaB = stdUtils::getMBRDelta(&aSlots[i].mMbr, aMbr2);
        
        if(STNMR_MIN(sDeltaA, sDeltaB) < sMinDelta)
        {
            sPos = i;
            
            if(sDeltaA < sDeltaB)
            {
                sMinDelta   = sDeltaA;
                sChooseNode = 0;
            }
            else
            {
                sMinDelta = sDeltaB;
                sChooseNode = 1;
            }
        }
    }

    sTmpSlot = aSlots[aCount - 1];
    
    aSlots[aCount-1] = aSlots[sPos];
    aSlots[sPos] = sTmpSlot;
    
    return sChooseNode;
    
}

IDE_RC stnmrRTree::insertRow( idvSQL          * /*aStatistics */,
                              void            * /*aTrans*/,
                              void            * /*aTable*/,
                              void            * aIndex,
                              smSCN             /*aInfiniteSCN*/,
                              SChar           * aRow,
                              SChar           * /*aNull*/,
                              idBool            /*aUniqueCheck*/,
                              smSCN             /*aStmtViewSCN*/,
                              void            * /*aRowSID*/,
                              SChar          ** /*aExistUniqueRow*/,
                              ULong             /*aInsertWaitTime*/,
                              idBool            /*aForbiddenToRetry*/ )
{

    stnmrHeader*       sHeader;    
    stnmrNode*         sLeafNode;
    UInt               sState = 0;
    SInt               sDepth = -1;
    stnmrNode*         sNewRootNode  = NULL;
    stnmrNode*         sNewRNode     = NULL;
    stnmrNode*         sChildNode    = NULL;
    stnmrNode*         sCurNode;
    UInt               sUpdatePos    = ID_UINT_MAX;
    stdMBR             sMbr1;
    stdMBR             sMbr2;
    stnmrStack*        sCurStackPtr;
    stdMBR             sChildMbr;
    stdMBR             sInsertMbr;
    void*              sInsertPtr;
    stnmrStack         sStack[STNMR_STACK_DEPTH];
    ULong              sInsertVersion = 0;
    ULong              sChildVersion = 0;
    SInt               sPos;	
    idBool             sIsUpdate = ID_FALSE;
    idBool             sIsIndexable;
    
    stdGeometryHeader  *sGeoHeader;

    sHeader  = (stnmrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    // Fix BUG-15844 :  Geometry가 NULL또는 Empty인 경우는 삽입하지
    //                  않는다.
    IDE_TEST( getGeometryHeaderFromRow( sHeader, aRow, MTD_OFFSET_USE, &sGeoHeader )
              != IDE_SUCCESS );
    // BUG-27518
    stdUtils::isIndexableObject(sGeoHeader, &sIsIndexable);

    IDE_TEST_RAISE(sIsIndexable == ID_FALSE, no_action_insert);
    
    sInsertMbr =  sGeoHeader->mMbr;
     
    IDE_TEST(sHeader->mMutex.lock(NULL) != IDE_SUCCESS);
    sState = 1;
    
    if(sHeader->mRoot == NULL)
    {
        //Latch Tree Header
        IDL_MEM_BARRIER;
        sHeader->mLatch |= STNMR_NODE_LATCH_BIT;
        IDL_MEM_BARRIER;
        
        IDE_TEST(sHeader->mNodePool.allocateSlots(1,
                                                (smmSlot**)&sNewRootNode,
                                                SMM_SLOT_LIST_MUTEX_NEEDLESS)
                 != IDE_SUCCESS);
        sHeader->mNodeCount++;

        initNode(sNewRootNode, STNMR_NODE_TYPE_LEAF, IDU_LATCH_UNLOCKED);
        
        insertNode(sNewRootNode, &sInsertMbr, aRow, sInsertVersion);

        sNewRootNode->mVersion = ++(sHeader->mVersion);
        sHeader->mRoot = sNewRootNode;
        sHeader->mRootVersion = sNewRootNode->mVersion;
        sHeader->mTreeMBR = sInsertMbr;

        //UnLatch Tree Header
        IDL_MEM_BARRIER;
        sHeader->mLatch++;
    }
    else
    {
        sLeafNode = chooseLeaf(sHeader, &sInsertMbr, sStack, &sDepth);

        sPos = locateInNode(sLeafNode, aRow);

        if(sPos == -1)
        {
            sCurNode     = sLeafNode;
            sCurStackPtr = sStack + sDepth;
            sInsertPtr   = aRow;
            
            while(1)
            {
                if(sCurNode->mSlotCount < STNMR_SLOT_MAX)
                {
                    STNMR_NODE_LATCH(sCurNode);
                
                    insertNode(sCurNode,&sInsertMbr,sInsertPtr,sInsertVersion);
                    stdUtils::getMBRExtent(
                        &sHeader->mTreeMBR, 
                        &sInsertMbr);

                    if(sIsUpdate == ID_TRUE)
                    {
                        sCurNode->mSlots[sUpdatePos].mMbr = sChildMbr;
                        IDE_ASSERT(sCurNode->mSlots[sUpdatePos].mPtr 
                                   == sChildNode);
                        sCurNode->mSlots[sUpdatePos].mVersion = 
                            sChildNode->mVersion;
                        stdUtils::getMBRExtent(
                            &sHeader->mTreeMBR, 
                            &sChildMbr);
                    }
                    else
                    {
                        sUpdatePos = ID_UINT_MAX;
                    }

                    STNMR_NODE_UNLATCH(sCurNode);
                
                    if(sHeader->mRoot == sCurNode)
                    {
                        IDE_ASSERT(sHeader->mRootVersion == sCurNode->mVersion);
                        break;
                    }

                    //propagate 를 하면 되겠지...
                    propagate(sHeader,
                              &(sHeader->mStmtStat),
                              sCurNode,
                              sUpdatePos,
                              sCurStackPtr);
                
                    break;
                }
            
                IDE_TEST(sHeader->mNodePool.allocateSlots(1, 
                                          (smmSlot**)&sNewRNode, 
                                          SMM_SLOT_LIST_MUTEX_NEEDLESS)
                         != IDE_SUCCESS);
                sHeader->mNodeCount++;

                STNMR_NODE_LATCH(sCurNode);
            
                sHeader->mStmtStat.mNodeSplitCount++;
                
                split(sHeader, sCurNode, sNewRNode, sUpdatePos, &sChildMbr, 
                      sChildNode, sChildVersion,&sInsertMbr, sInsertPtr,
                      sInsertVersion, &sMbr1, &sMbr2);
            
                STNMR_NODE_UNLATCH(sCurNode);
                
                if(sCurNode == sHeader->mRoot)
                {
                    IDE_TEST(sHeader->mNodePool.allocateSlots(1,
                                              (smmSlot**)&sNewRootNode,
                                               SMM_SLOT_LIST_MUTEX_NEEDLESS)
                             != IDE_SUCCESS);
                    sHeader->mNodeCount++;

                    initNode(sNewRootNode, STNMR_NODE_TYPE_INTERNAL, 
                             IDU_LATCH_UNLOCKED);

                    sNewRootNode->mVersion = ++(sHeader->mVersion);
                    
                    insertNode(sNewRootNode, &sMbr1,
                               sCurNode, sCurNode->mVersion);
                    insertNode(sNewRootNode, &sMbr2,
                               sNewRNode, sNewRNode->mVersion);
                    
                    //Latch Tree Header
                    IDL_MEM_BARRIER;
                    sHeader->mLatch |= STNMR_NODE_LATCH_BIT;
                    IDL_MEM_BARRIER;

                    sHeader->mRoot = sNewRootNode;
                    sHeader->mRootVersion = sNewRootNode->mVersion;
                
                    //UnLatch Tree Header
                    IDL_MEM_BARRIER;
                    sHeader->mLatch++;
                
                    break;
                }

                sChildMbr   = sMbr1;
                sChildNode = sCurNode;
                sChildVersion = sChildNode->mVersion;
            
                sInsertMbr  = sMbr2;
                sInsertPtr = sNewRNode;
                sInsertVersion = sNewRNode->mVersion;
            
                sUpdatePos = sCurStackPtr->mLstReadPos;
                sCurNode   = sCurStackPtr->mNodePtr;
                sIsUpdate  = ID_TRUE;
            
                sCurStackPtr--;
                sDepth--;
            }//while
        }
    }

    sHeader->mKeyCount++;

    sState = 0;
    IDE_TEST(sHeader->mMutex.unlock() != IDE_SUCCESS);
    
    IDE_EXCEPTION_CONT( no_action_insert );
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sState != 0)
    {
        IDE_PUSH();
        (void)sHeader->mMutex.unlock();
        IDE_POP();
    }
    
    return IDE_FAILURE;

}

IDE_RC stnmrRTree::deleteRowNA( idvSQL*        /* aStatistics */,
                                void*          /*aTrans*/,
                                void*          /* aIndex */,
                                SChar*         /* aRow */,
                                smiIterator*   /*aIterator*/,
                                sdSID          /* aRowSID */ )
{

    return IDE_SUCCESS;

}
    
IDE_RC stnmrRTree::freeSlot( void*             aIndex,
                             SChar*            aRow,
                             idBool            /*aIgnoreNotFoundKey*/,
                             idBool          * aIsExistFreeKey )
{

    stnmrHeader*       sHeader;    
    stnmrNode*         sCurNode;
    stdMBR             sDeleteMBR;
    UInt               sState = 0;
    SInt               sPos;	
    stnmrNode*         sVersioned = NULL;
    stnmrStack*        sCurStackPtr;
    SInt               sDepth;
    stdGeometryHeader  *sGeoHeader;

    stnmrStack         sStack[STNMR_STACK_DEPTH];
    idBool             sIsIndexable;

    IDE_ASSERT( aIndex          != NULL );
    IDE_ASSERT( aRow            != NULL );
    IDE_ASSERT( aIsExistFreeKey != NULL );

    *aIsExistFreeKey = ID_TRUE;
    
    sHeader  = (stnmrHeader*)((smnIndexHeader*)aIndex)->mHeader;
    sCurNode = sHeader->mRoot;

    // Fix BUG-15844 :  Geometry가 NULL또는 Empty인 경우는 삽입하지
    //                  않았으므로 삭제하지 않는다.
    sHeader  = (stnmrHeader*)((smnIndexHeader*)aIndex)->mHeader;
    
    IDE_TEST( getGeometryHeaderFromRow( sHeader, aRow, MTD_OFFSET_USE, &sGeoHeader )
              != IDE_SUCCESS );

    // BUG-27518
    stdUtils::isIndexableObject(sGeoHeader, &sIsIndexable);
    
    IDE_TEST_RAISE( sIsIndexable == ID_FALSE, no_action_delete );
    
    sDeleteMBR = sGeoHeader->mMbr;
        
    IDE_TEST(sHeader->mMutex.lock(NULL) != IDE_SUCCESS);
    sState = 1;

    sCurNode = findLeaf(sHeader,
                        &(sHeader->mAgerStat),
                        sStack,
                        &sDeleteMBR,
                        aRow,
                        &sDepth);
    // BUG-23878
    // in MVCC, The row can be null(no rows for delete) in some special case.
    if ( sCurNode == NULL )
    {
        *aIsExistFreeKey = ID_FALSE;
        IDE_RAISE( NO_ROW_FOR_DELETE );
    }
    else
    {
        *aIsExistFreeKey = ID_TRUE;
    }
    
    sCurStackPtr = sStack + sDepth;

    sPos = locateInNode(sCurNode, aRow);

    if(sPos >= 0)
    {
        while(1)
        {
            if(sCurNode->mSlotCount == 1)
            {
                STNMR_NODE_LATCH(sCurNode);
                
                sCurNode->mFreeNodeList  = sVersioned;
                sVersioned         = sCurNode;
                
                sCurNode->mSlotCount = 0;
                
                STNMR_NODE_UNLATCH(sCurNode);
                
                sHeader->mAgerStat.mNodeDeleteCount++;
                
                if(sHeader->mRoot == sCurNode)
                {
                    //Latch Tree Header
                    IDL_MEM_BARRIER;
                    sHeader->mLatch |= STNMR_NODE_LATCH_BIT;
                    IDL_MEM_BARRIER;
                    
                    sHeader->mRootVersion = ID_ULONG_MAX;
                    
                    sHeader->mRoot = NULL;

                    //UnLatch Tree Header
                    IDL_MEM_BARRIER;
                    sHeader->mLatch++;
                    
                    break;
                }
            }
            else
            {
                STNMR_NODE_LATCH(sCurNode);
                
                deleteNode(sCurNode, sPos);

                STNMR_NODE_UNLATCH(sCurNode);

                break;
            }

            sCurStackPtr--;
            
            sPos = sCurStackPtr->mLstReadPos;

            IDE_ASSERT(sCurStackPtr->mNodePtr->mSlots[sPos].mPtr 
                       == sCurNode);

            IDE_ASSERT(sCurNode != sCurStackPtr->mNodePtr);
                
            sCurNode   = sCurStackPtr->mNodePtr;
        }
    }
    
    if(sVersioned != NULL)
    {
        smLayerCallback::addNodes2LogicalAger(gSmnrFreeNodeList,
                                              (smnNode*)sVersioned);
    }

    sHeader->mKeyCount--;
    // BUG-23878
    IDE_EXCEPTION_CONT(NO_ROW_FOR_DELETE);
    
    sState = 0;
    IDE_TEST(sHeader->mMutex.unlock() != IDE_SUCCESS);
    

    IDE_EXCEPTION_CONT( no_action_delete );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    IDE_PUSH();
    if (sState != 0)
    {
        (void)sHeader->mMutex.unlock();
    }
    IDE_POP();

    return IDE_FAILURE;

}

void stnmrRTree::deleteNode( stnmrNode* aNode,
                             SInt       aPos )
{

    SInt i;

    IDE_ASSERT( (aNode->mSlotCount != 0) && (aNode->mSlotCount > aPos) );
    
    // fix BUG-15442 : 동시에 Delete할경우 R-Tree에서 서버 다운
    for( i=aPos; i<aNode->mSlotCount-1; i++ )
    {
        // mPtr은 반드시 원자성을 유지해야 하기때문에
        // 구조체에 대한 Assign전에 포인터의 Assign한다.
        aNode->mSlots[ i ].mPtr = aNode->mSlots[ i + 1 ].mPtr;
        aNode->mSlots[ i ]      = aNode->mSlots[ i + 1 ];
    }
    aNode->mSlotCount--;
}

SInt stnmrRTree::locateInNode( stnmrNode* aNode,
                               void*      aPtr)
{

    SInt i;
    SInt sPos;

    sPos = -1;

    for(i = 0; i < aNode->mSlotCount; i++)
    {
        if(aPtr == aNode->mSlots[i].mPtr)
        {
            sPos = i;
            break;
        }
    }

    return sPos;
}

void stnmrRTree::findNextLeaf( const stnmrHeader*  aHeader,
                               const smiCallBack*  aCallBack,
                               stnmrStack*         aStack,
                               SInt*               aDepth )
{
  
    SInt          i, j;
    SInt          sSlotCount;
    SInt          sDepth;
    stnmrStack*   sStack;
    stnmrNode*    sRootNodePtr = NULL;
    stnmrNode*    sCurNodePtr  = NULL;
    stnmrNode*    sNxtNodePtr  = NULL;
    stnmrSlot     sSlots[STNMR_SLOT_MAX];
    IDU_LATCH     sLatch;
    ULong         sOldVersion;
    ULong         sCurVersion;
    ULong         sRootVersion;
    idBool        sResult;
    
    sDepth = *aDepth;

    while(1)
    {
        sStack = aStack + sDepth;
        
        if(sDepth == -1)
        {
            while(1)
            {
                IDL_MEM_BARRIER;
                sLatch = getLatchValueOfHeader((stnmrHeader*)aHeader) & 
                         IDU_LATCH_UNMASK;
                IDL_MEM_BARRIER;
                
                if(aHeader->mRoot == NULL)
                {
                    break;
                }

                sRootNodePtr = aHeader->mRoot;
                sRootVersion  = aHeader->mRootVersion;

                IDL_MEM_BARRIER;
                if(aHeader->mLatch == sLatch)
                {
                    sDepth++;
                    sStack++;
                
                    sStack->mNodePtr = sRootNodePtr;
                    sStack->mVersion = sRootVersion;
                    
                    break;
                }
            }
        }

        sCurVersion  = sOldVersion = 0;
        
        while(sDepth >= 0)
        {
            if(sOldVersion == sCurVersion)
            {
                sCurNodePtr = sStack->mNodePtr;
                sOldVersion  = sStack->mVersion;
               
                if( (sCurNodePtr->mFlag & STNMR_NODE_TYPE_MASK) 
                    == STNMR_NODE_TYPE_LEAF )
                {
                    break;
                }

                sDepth--;
                sStack--;
            }
            else
            {
                IDE_ASSERT(sNxtNodePtr != NULL);
            
                sCurNodePtr = sNxtNodePtr;
            }
            
            IDE_ASSERT( (sCurNodePtr->mFlag & STNMR_NODE_TYPE_MASK) 
                        == STNMR_NODE_TYPE_INTERNAL );
            
            while(1)
            {
                IDL_MEM_BARRIER;
                sLatch = getLatchValueOfNode(sCurNodePtr) & IDU_LATCH_UNMASK;
                IDL_MEM_BARRIER;
                
                sCurVersion  = sCurNodePtr->mVersion;
                sNxtNodePtr = sCurNodePtr->mNextSPtr;
                sSlotCount  = sCurNodePtr->mSlotCount;
                
                for(i = 0, j = -1; i < sSlotCount; i++)
                {
                    (void) aCallBack->callback( &sResult,
                                                &(sCurNodePtr->mSlots[i].mMbr),
                                                NULL,
                                                0,
                                                SC_NULL_GRID,
                                                aCallBack->data);
                    
                    //Check Overlaps Rectangle
                    if(sResult == ID_TRUE)
                    {
                        j++;
                        sSlots[j] = sCurNodePtr->mSlots[i];
                    }
                }

                IDL_MEM_BARRIER;
                if(sLatch == sCurNodePtr->mLatch)
                {
                    if(sOldVersion != sCurVersion)
                    {
                        IDE_ASSERT(sNxtNodePtr != NULL);
                    }
                    
                    break;
                }
            }

            for(i = 0; i <= j; i++)
            {
                sDepth++;
                sStack++;
                        
                sStack->mNodePtr = sSlots[i].mNodePtr;
                sStack->mVersion = sSlots[i].mVersion;
            }
        }

        break;
    }
    
    *aDepth = sDepth;
    
}

stnmrNode* stnmrRTree::findLeaf( const stnmrHeader* aHeader,
                                 stnmrStatistic*    aIndexStat,
                                 stnmrStack*        aStack, 
                                 stdMBR*            aMbr,
                                 void*              aPtr,
                                 SInt*              aDepth)
{
  
    stnmrNode  *sCurNode = aHeader->mRoot;
    stnmrStack *sStack   = aStack;
    SInt        sPos;
    SInt        sDepth   = 0;
    
    sStack->mNodePtr    = aHeader->mRoot;
    sStack->mLstReadPos = 0;

    sPos = -1;

    if(sCurNode != NULL)
    {
        while(1)
        {
            while((sStack->mLstReadPos) < (sCurNode->mSlotCount))
            {
                if(sCurNode->mFlag == STNMR_NODE_TYPE_LEAF)
                {
                    sPos = locateInNode(sCurNode, aPtr);
                    break;
                }
                else
                {
                    // Fix Bug-15839
                    aIndexStat->mKeyCompareCount++;
                    if(stdUtils::isMBRContains( 
                           &sCurNode->mSlots[sStack->mLstReadPos].mMbr,
                           aMbr) 
                       == ID_TRUE)
                    {
                        sCurNode = (stnmrNode*)
                            sCurNode->mSlots[sStack->mLstReadPos].mPtr;
                        sStack++;
                        sStack->mNodePtr = sCurNode;
                        sDepth++;
                        sStack->mLstReadPos = 0;
                    }
                    else
                    {
                        sStack->mLstReadPos++;
                    }
                }
            }
            
            if(sCurNode == aHeader->mRoot)
            {
                if( sPos == -1 )
                {
                    sDepth = -1;
                    return NULL;
                }
                else
                {
                    break;
                }
            }
            else
            {
                if( sPos >= 0 )
                {
                    break;
                }
                else
                {
                    sStack--;
                    sDepth--;
                    sCurNode = sStack->mNodePtr;
                    sStack->mLstReadPos++;                
                }
            }
        }
    }
    else
    {
        sDepth = -1;
    }
    
    *aDepth = sDepth;
    
    return sCurNode;

}

IDE_RC stnmrRTree::NA( void )
{

    IDE_SET( ideSetErrorCode( stERR_ABORT_TRAVERSE_NOT_APPLICABLE ) );

    return IDE_FAILURE;

}

IDE_RC stnmrRTree::beforeFirstU( stnmrIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc)
{
    
    (void)stnmrRTree::beforeFirst(aIterator, aSeekFunc);
    
    *aSeekFunc += 6;

    return IDE_SUCCESS;
    
}

IDE_RC stnmrRTree::beforeFirstR( stnmrIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc)
{

    stnmrIterator sIterator;
    smiCursorProperties sCursorProperty;

    IDE_TEST(stnmrRTree::beforeFirst(aIterator, aSeekFunc) != IDE_SUCCESS);
    sIterator.mCurRecPtr       = aIterator->mCurRecPtr;
    sIterator.mLstFetchRecPtr  = aIterator->mLstFetchRecPtr;
    sIterator.mLeast           = aIterator->mLeast;
    sIterator.mHighest         = aIterator->mHighest;
    sIterator.mKeyRange        = aIterator->mKeyRange;
    sIterator.mNode            = aIterator->mNode;
    sIterator.mSlot            = aIterator->mSlot;
    sIterator.mLowFence        = aIterator->mLowFence;
    sIterator.mHighFence       = aIterator->mHighFence;
    sIterator.mDepth           = aIterator->mDepth;
    sIterator.mFlag            = aIterator->mFlag;
    
    sIterator.mProperties = &sCursorProperty;
    idlOS::memcpy(sIterator.mProperties,
                  aIterator->mProperties,
                  ID_SIZEOF(smiCursorProperties));
    idlOS::memcpy(sIterator.mStack, aIterator->mStack,
                  ID_SIZEOF(stnmrStack) * (aIterator->mDepth + 1));
    
    IDE_TEST(stnmrRTree::fetchNextR(aIterator) != IDE_SUCCESS);

    aIterator->mCurRecPtr      = sIterator.mCurRecPtr;
    aIterator->mLstFetchRecPtr = sIterator.mLstFetchRecPtr;
    aIterator->mLeast          = sIterator.mLeast;
    aIterator->mHighest        = sIterator.mHighest;
    aIterator->mKeyRange       = sIterator.mKeyRange;
    aIterator->mNode           = sIterator.mNode;
    aIterator->mSlot           = sIterator.mSlot;
    aIterator->mLowFence       = sIterator.mLowFence;
    aIterator->mHighFence      = sIterator.mHighFence;
    aIterator->mDepth          = sIterator.mDepth;
    aIterator->mFlag           = sIterator.mFlag;
    
    idlOS::memcpy(aIterator->mProperties,
                  sIterator.mProperties,
                  ID_SIZEOF(smiCursorProperties));
    idlOS::memcpy(aIterator->mStack, sIterator.mStack,
                  ID_SIZEOF(stnmrStack) * (sIterator.mDepth + 1));

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC stnmrRTree::afterLastR( stnmrIterator*       aIterator,
                               const smSeekFunc**  aSeekFunc )
{

    IDE_TEST(stnmrRTree::beforeFirst(aIterator, aSeekFunc) != IDE_SUCCESS);

    IDE_TEST(stnmrRTree::fetchNextR(aIterator) != IDE_SUCCESS);
    *aSeekFunc += 6;
    
    aIterator->mFlag = SMI_RETRAVERSE_AFTER;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;

}

IDE_RC stnmrRTree::fetchNextU( stnmrIterator* aIterator,
                               const void**   aRow )
{
    
    idBool      sResult;
    stnmrNode*  sCurNode;
    SInt        sSlotCount;
    SInt        i, j;
    void*       sPtr;
    stnmrNode*  sNxtNode;
    IDU_LATCH   sLatch;
    ULong       sVersion;
    ULong       sCurNodeVersion;
    const smiCallBack* sCallBack;
    idBool     sCanReusableRollback = ID_TRUE;
    smxTrans * sTrans               = (smxTrans*)aIterator->mTrans;
    idBool     sIsVisible;
    
    sCallBack = &(aIterator->mKeyRange->maximum);
        
  restart:

    for( aIterator->mSlot++;
         aIterator->mSlot <= aIterator->mHighFence;
         aIterator->mSlot++ )
    {
        sCanReusableRollback = ID_TRUE;

        aIterator->mCurRecPtr      = (SChar*)(aIterator->mSlot);
        aIterator->mLstFetchRecPtr = aIterator->mCurRecPtr;
        *aRow = aIterator->mCurRecPtr;

        SC_MAKE_GRID( aIterator->mRowGRID,
                      ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                        (const smpSlotHeader*)*aRow,
                                        &sCanReusableRollback,
                                        &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_TRUE )
        {
            IDE_TEST(aIterator->mRowFilter->callback( &sResult,
                                                      *aRow,
                                                      NULL,
                                                      0,
                                                      aIterator->mRowGRID,
                                                      aIterator->mRowFilter->data)
                     != IDE_SUCCESS );
            if(sResult == ID_TRUE)
            {
                if( sCanReusableRollback == ID_FALSE )
                {     
                    sTrans->mIsReusableRollback = ID_FALSE;
                }

                smnManager::updatedRow((smiIterator*)aIterator);
                
                *aRow = aIterator->mCurRecPtr;

                SC_MAKE_GRID( aIterator->mRowGRID,
                              ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                              SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                              SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );
                
                if(SM_SCN_IS_NOT_DELETED(((smpSlotHeader*)(*aRow))->mCreateSCN))
                {
                    return IDE_SUCCESS;
                }
            }
        }
    }

    sCurNode = NULL;
    if(aIterator->mHighest == ID_FALSE || aIterator->mVersion != ID_ULONG_MAX)
    {
        if(aIterator->mVersion == ID_ULONG_MAX)
        {
            findNextLeaf(aIterator->mIndex,
                         &(aIterator->mKeyRange->maximum),
                         aIterator->mStack,
                         &(aIterator->mDepth));

            // BUG-23344 
            IDE_TEST_RAISE(aIterator->mDepth == -1, NO_RESULT);
            
            aIterator->mNode = aIterator->mStack[aIterator->mDepth].mNodePtr;
            sCurNode = aIterator->mNode;
            sVersion = aIterator->mStack[aIterator->mDepth].mVersion;
            aIterator->mVersion = sVersion;
            aIterator->mDepth = aIterator->mDepth - 1;
            aIterator->mNxtNode = NULL;
        }
        else
        {
            IDE_ASSERT(aIterator->mNxtNode != NULL);
               
            aIterator->mNode = aIterator->mNxtNode;
            sCurNode = aIterator->mNxtNode;
            sVersion  = aIterator->mVersion;

            aIterator->mNxtNode = NULL;
        }
        
        if(aIterator->mDepth >= -1)
        {
            while(1)
            {
                IDL_MEM_BARRIER;
                sLatch   = getLatchValueOfNode(sCurNode) & IDU_LATCH_UNMASK;
                IDL_MEM_BARRIER;
                
                sCurNodeVersion = sCurNode->mVersion;
                sSlotCount      = sCurNode->mSlotCount;
                sNxtNode        = sCurNode->mNextSPtr;

                for(i = 0, j = -1; i < sSlotCount; i++)
                {
                    sCanReusableRollback = ID_TRUE;

                    sPtr = aIterator->mNode->mSlots[i].mPtr;

                    if(sPtr == NULL)
                    {
                        break;
                    }

                    // Fix  BUG-16072, Add Check KeyRange
                    sCallBack->callback( &sResult,
                                         &(aIterator->mNode->mSlots[i].mMbr),
                                         NULL,
                                         0,
                                         SC_NULL_GRID,
                                         sCallBack->data );
                    
                    if( sResult==ID_TRUE )
                    {
                        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                                        (const smpSlotHeader*)sPtr,
                                                        &sCanReusableRollback,
                                                        &sIsVisible )
                                  != IDE_SUCCESS );
                        if( sIsVisible == ID_TRUE )
                        {
                            if( sCanReusableRollback == ID_FALSE )
                            {     
                                sTrans->mIsReusableRollback = ID_FALSE;
                            }

                            j++;
                            aIterator->mRows[j] = (SChar*)sPtr;
                        }
                    }
                    
                }

                aIterator->mHighFence = aIterator->mRows + j;
                aIterator->mLowFence  = aIterator->mRows;
                aIterator->mSlot      = aIterator->mLowFence - 1;
                aIterator->mNxtNode   = sNxtNode;
                
                IDL_MEM_BARRIER;
                if(sLatch == sCurNode->mLatch)
                {
                    if(sVersion == sCurNodeVersion)
                    {
                        aIterator->mVersion = ID_ULONG_MAX;
                    }
                    
                    break;
                }
            }
            
            if(aIterator->mVersion == ID_ULONG_MAX && aIterator->mDepth < 0)
            {
                aIterator->mHighest = ID_TRUE;
            }

            aIterator->mLeast     = ID_FALSE;

            IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                      != IDE_SUCCESS );
            
            goto restart;
        }
    }
    // BUG-23344
    IDE_EXCEPTION_CONT(NO_RESULT);
    
    aIterator->mHighest = ID_TRUE;
    aIterator->mDepth   = -1;
    aIterator->mVersion = ID_ULONG_MAX;
    
    if(aIterator->mKeyRange->next != NULL)
    {
        aIterator->mKeyRange = aIterator->mKeyRange->next;

        (void)beforeFirstInternal(aIterator);

        IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                  != IDE_SUCCESS );
        
        goto restart;
    }
    
    aIterator->mSlot           = aIterator->mHighFence + 1;
    aIterator->mCurRecPtr      = NULL;
    aIterator->mLstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow                      = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC stnmrRTree::fetchNextR( stnmrIterator* aIterator )
{

    smpSlotHeader* sRow;
    scGRID         sRowGRID;
    stnmrNode*     sCurNode;
    SInt           sSlotCount;
    SInt           i, j;
    void*          sPtr;
    stnmrNode*     sNxtNode;
    IDU_LATCH      sLatch;
    ULong          sVersion;
    ULong          sCurNodeVersion;
    idBool         sResult;
    const smiCallBack* sCallBack;
    idBool         sCanReusableRollback = ID_TRUE;
    smxTrans     * sTrans = (smxTrans*)aIterator->mTrans;
    idBool         sIsVisible; 
    
    sCallBack = &(aIterator->mKeyRange->maximum);

  restart:

    for(aIterator->mSlot++;
        aIterator->mSlot <= aIterator->mHighFence;
        aIterator->mSlot++)
    {
        sCanReusableRollback = ID_TRUE;

        aIterator->mCurRecPtr      = *aIterator->mSlot;
        aIterator->mLstFetchRecPtr =  aIterator->mCurRecPtr;
        
        sRow = (smpSlotHeader*)(aIterator->mCurRecPtr);

        SC_MAKE_GRID( sRowGRID,
                      ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)sRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)sRow ) );
        
        IDE_TEST( smnManager::checkSCN((smiIterator*)aIterator, sRow, &sCanReusableRollback, &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_TRUE )
        {
            IDE_TEST(aIterator->mRowFilter->callback( &sResult,
                                                      sRow,
                                                      NULL,
                                                      0,
                                                      sRowGRID,
                                                      aIterator->mRowFilter->data)
                     != IDE_SUCCESS);
            if(sResult == ID_TRUE)
            {
                if( sCanReusableRollback == ID_FALSE )
                {     
                    sTrans->mIsReusableRollback = ID_FALSE;
                }

                IDE_TEST(smnManager::lockRow((smiIterator*)aIterator)
                         != IDE_SUCCESS );
            }
        }
    }

    sCurNode = NULL;
    
    if(aIterator->mHighest == ID_FALSE || aIterator->mVersion != ID_ULONG_MAX)
    {
        if(aIterator->mVersion == ID_ULONG_MAX)
        {
            findNextLeaf(aIterator->mIndex,
                         &(aIterator->mKeyRange->maximum),
                         aIterator->mStack,
                         &(aIterator->mDepth));

            // BUG-23344
            IDE_TEST_RAISE(aIterator->mDepth == -1, NO_RESULT);
            
            aIterator->mNode = aIterator->mStack[aIterator->mDepth].mNodePtr;
            sCurNode = aIterator->mNode;
            sVersion  = aIterator->mStack[aIterator->mDepth].mVersion;
            aIterator->mVersion = sVersion;
            aIterator->mDepth = aIterator->mDepth - 1;
            aIterator->mNxtNode = NULL;
        }
        else
        {
            IDE_ASSERT(aIterator->mNxtNode != NULL);
               
            aIterator->mNode = aIterator->mNxtNode;
            sCurNode = aIterator->mNxtNode;
            sVersion  = aIterator->mVersion;

            aIterator->mNxtNode = NULL;
        }
        
        if(aIterator->mDepth >= -1)
        {
            while(1)
            {
                IDL_MEM_BARRIER;
                sLatch   = getLatchValueOfNode(sCurNode) & IDU_LATCH_UNMASK;
                IDL_MEM_BARRIER;

                sCurNodeVersion = sCurNode->mVersion;
                sSlotCount   = sCurNode->mSlotCount;
                sNxtNode = sCurNode->mNextSPtr;
                
                for(i = 0, j = -1; i < sSlotCount; i++)
                {
                    sCanReusableRollback = ID_TRUE;

                    sPtr = aIterator->mNode->mSlots[i].mPtr;

                    if(sPtr == NULL)
                    {
                        break;
                    }

                    // Fix  BUG-16072, Add Check KeyRange
                    sCallBack->callback( &sResult,
                                         &(aIterator->mNode->mSlots[i].mMbr),
                                         NULL,
                                         0,
                                         SC_NULL_GRID,
                                         sCallBack->data );

                    if( sResult==ID_TRUE )
                    {
                        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                                        (const smpSlotHeader*)sPtr,
                                                        &sCanReusableRollback,
                                                        &sIsVisible )
                                  != IDE_SUCCESS );
                        if( sIsVisible == ID_TRUE )
                        {
                            if( sCanReusableRollback == ID_FALSE )
                            {     
                                sTrans->mIsReusableRollback = ID_FALSE;
                            }

                            j++;
                            aIterator->mRows[j] = (SChar*)sPtr;
                        }
                    }
                    
                }

                aIterator->mHighFence = aIterator->mRows + j;
                aIterator->mLowFence  = aIterator->mRows;
                aIterator->mSlot      = aIterator->mLowFence - 1;
                aIterator->mNxtNode   = sNxtNode;
                
                IDL_MEM_BARRIER;
                if(sLatch == sCurNode->mLatch)
                {
                    if(sVersion == sCurNodeVersion)
                    {
                        aIterator->mVersion = ID_ULONG_MAX;
                    }
                    
                    break;
                }
            }

            if(aIterator->mVersion == ID_ULONG_MAX && aIterator->mDepth < 0)
            {
                aIterator->mHighest = ID_TRUE;
            }
            
            aIterator->mLeast     = ID_FALSE;

            IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                      != IDE_SUCCESS );
            
            goto restart;
        }
    }

    // BUG-23344
    IDE_EXCEPTION_CONT(NO_RESULT);
    
    aIterator->mHighest = ID_TRUE;
    aIterator->mDepth   = -1;
    aIterator->mVersion = ID_ULONG_MAX;
    
    if( aIterator->mKeyRange->next != NULL )
    {
        aIterator->mKeyRange = aIterator->mKeyRange->next;

        (void)beforeFirstInternal( aIterator );

        IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                  != IDE_SUCCESS );
        
        goto restart;
    }

    aIterator->mSlot           = aIterator->mHighFence + 1;
    aIterator->mCurRecPtr      = NULL;
    aIterator->mLstFetchRecPtr = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC stnmrRTree::beforeFirst( stnmrIterator* aIterator,
                                const smSeekFunc** )
{

    for( aIterator->mKeyRange        = aIterator->mKeyRange;
         aIterator->mKeyRange->prev != NULL;
         aIterator->mKeyRange        = aIterator->mKeyRange->prev ) ;
    
    IDE_TEST( beforeFirstInternal(aIterator) != IDE_SUCCESS );

    aIterator->mFlag = SMI_RETRAVERSE_BEFORE;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
    
}

IDE_RC stnmrRTree::beforeFirstInternal( stnmrIterator* aIterator )
{

    SInt             i, j;
    SInt             sSlotCount;
    IDU_LATCH        sLatch;
    stnmrNode*       sCurNode;
    stnmrStack*      sStack;
    ULong            sVersion;
    ULong            sCurNodeVersion;
    const smiRange*  sRange;
    void*            sPtr;
    idBool           sResult;
    idBool           sCanReusableRollback = ID_TRUE;
    smxTrans       * sTrans = (smxTrans*)aIterator->mTrans;
    idBool           sIsVisible;

    aIterator->mVersion   = ID_ULONG_MAX;
    
    sRange = aIterator->mKeyRange;

    aIterator->mDepth = -1;
    
    stnmrRTree::findNextLeaf(aIterator->mIndex,
                             &(sRange->maximum),
                             aIterator->mStack,
                             &aIterator->mDepth);
    
    while(aIterator->mDepth < 0 && sRange->next != NULL)
    {
        sRange = sRange->next;

        aIterator->mDepth = -1;
            
        stnmrRTree::findNextLeaf(aIterator->mIndex,
                                 &(sRange->maximum),
                                 aIterator->mStack,
                                 &aIterator->mDepth);
    }

    aIterator->mKeyRange = sRange;

    if(aIterator->mDepth >= 0)
    {
        //Check SCN
        sStack = aIterator->mStack + aIterator->mDepth;
        (aIterator->mDepth)--;
        
        sCurNode = sStack->mNodePtr;
        aIterator->mVersion = sStack->mVersion;
        sVersion = sStack->mVersion;
        
        while(1)
        {
            IDL_MEM_BARRIER;
            sLatch   = getLatchValueOfNode(sCurNode) & IDU_LATCH_UNMASK;
            IDL_MEM_BARRIER;
            
            sSlotCount         = sCurNode->mSlotCount;
            sCurNodeVersion    = sCurNode->mVersion;
            aIterator->mNxtNode = sCurNode->mNextSPtr;
            
            for(i = 0, j = -1; i < sSlotCount; i++)
            {
                sCanReusableRollback = ID_TRUE;

                sPtr = sCurNode->mSlots[i].mPtr;

                if(sPtr == NULL)
                {
                    break;
                }
                
                IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                                (const smpSlotHeader*)sPtr,
                                                NULL,
                                                &sIsVisible )
                          != IDE_SUCCESS );
                if( sIsVisible == ID_TRUE )
                {
                    // Fix BUG-15293 to set key range. 
                    sRange->maximum.callback( &sResult,
                                              &sCurNode->mSlots[i].mMbr,
                                              NULL,
                                              0,
                                              SC_NULL_GRID,
                                              sRange->maximum.data); 
                    
                    if(sResult == ID_TRUE)
                    {
                        if( sCanReusableRollback == ID_FALSE )
                        {     
                            sTrans->mIsReusableRollback = ID_FALSE;
                        }

                        j++;
                        aIterator->mRows[j] = (SChar*)sPtr;
                    }
                }
            }

            IDL_MEM_BARRIER;
            if(sLatch == sCurNode->mLatch)
            {
                if(sVersion == sCurNodeVersion)
                {
                    aIterator->mVersion = ID_ULONG_MAX;
                }
                else
                {
                    aIterator->mVersion = sVersion;
                    IDE_ASSERT(aIterator->mNxtNode != NULL);
                } 
                break;
            }
        }

        aIterator->mHighFence = aIterator->mRows + j;
        aIterator->mLowFence  = aIterator->mRows;
        aIterator->mSlot      = aIterator->mLowFence - 1;
        aIterator->mNode      = sCurNode;
        aIterator->mLeast     = ID_TRUE;

        if(aIterator->mDepth < 0 && aIterator->mVersion == ID_ULONG_MAX)
        {
            aIterator->mVersion   = ID_ULONG_MAX;
            aIterator->mHighest   = ID_TRUE;
        }
        else
        {
            aIterator->mHighest   = ID_FALSE;
        }
    }
    else
    {
        aIterator->mLeast     =  
            aIterator->mHighest   = ID_TRUE;
        aIterator->mSlot      = (SChar**)&aIterator->mSlot;
        aIterator->mLowFence  = aIterator->mSlot + 1;
        aIterator->mHighFence = aIterator->mSlot - 1;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

IDE_RC stnmrRTree::fetchNext( stnmrIterator* aIterator,
                              const void**   aRow )
{
    
    stnmrNode*  sCurNode;
    SInt        sSlotCount;
    SInt        i, j;
    void*       sPtr;
    stnmrNode*  sNxtNode;
    IDU_LATCH   sLatch;
    ULong       sVersion;
    ULong       sCurNodeVersion;
    idBool      sResult;
    const smiCallBack* sCallBack;
    idBool      sCanReusableRollback = ID_TRUE;
    smxTrans  * sTrans = (smxTrans*)aIterator->mTrans;
    idBool      sIsVisible;
    
    sCallBack = &(aIterator->mKeyRange->maximum);
    
  restart:

    for(aIterator->mSlot++;
        aIterator->mSlot <= aIterator->mHighFence;
        aIterator->mSlot++ )
    {
        aIterator->mCurRecPtr = *aIterator->mSlot;
        aIterator->mLstFetchRecPtr = aIterator->mCurRecPtr;
        
        *aRow = aIterator->mCurRecPtr;

        SC_MAKE_GRID( aIterator->mRowGRID,
                      ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        IDE_TEST(aIterator->mRowFilter->callback( &sResult,
                                                  *aRow,
                                                  NULL,
                                                  0,
                                                  aIterator->mRowGRID,
                                                  aIterator->mRowFilter->data)
                 != IDE_SUCCESS);
        
        if(sResult == ID_TRUE)
        {
            return IDE_SUCCESS;
        }
    }

    sCurNode = NULL;
    
    if((aIterator->mHighest == ID_FALSE)||(aIterator->mVersion != ID_ULONG_MAX))
    {
        if(aIterator->mVersion == ID_ULONG_MAX)
        {
            findNextLeaf(aIterator->mIndex,
                         &(aIterator->mKeyRange->maximum),
                         aIterator->mStack,
                         &(aIterator->mDepth));

            // BUG-23344
            IDE_TEST_RAISE(aIterator->mDepth == -1, NO_RESULT);
            
            aIterator->mNode = aIterator->mStack[aIterator->mDepth].mNodePtr;
            sCurNode = aIterator->mNode;
            
            //BUG-4927            
            //IDE_ASSERT((sCurNode->flag & STNMR_NODE_TYPE_MASK) 
            //== STNMR_NODE_TYPE_LEAF);
            
            sVersion  = aIterator->mStack[aIterator->mDepth].mVersion;
            aIterator->mVersion = sVersion;
            aIterator->mDepth = aIterator->mDepth - 1;
            aIterator->mNxtNode = NULL;
        }
        else
        {
            IDE_ASSERT(aIterator->mNxtNode != NULL);
               
            aIterator->mNode = aIterator->mNxtNode;
            sCurNode = aIterator->mNxtNode;
            sVersion  = aIterator->mVersion;

            aIterator->mNxtNode = NULL;
        }
        
        if(aIterator->mDepth >= -1)
        {
            while(1)
            {
                IDL_MEM_BARRIER;
                sLatch   = getLatchValueOfNode(sCurNode) & IDU_LATCH_UNMASK;
                IDL_MEM_BARRIER;
                            
                sCurNodeVersion = sCurNode->mVersion;
                sSlotCount   = sCurNode->mSlotCount;
                sNxtNode = sCurNode->mNextSPtr;
                
                for(i = 0, j = -1; i < sSlotCount; i++)
                {
                    sPtr = sCurNode->mSlots[i].mPtr;

                    if(sPtr == NULL)
                    {
                        break;
                    }

                    // Fix  BUG-16072, Add Check KeyRange
                    sCallBack->callback( &sResult,
                                         &(sCurNode->mSlots[i].mMbr),
                                         NULL,
                                         0,
                                         SC_NULL_GRID,
                                         sCallBack->data );

                    if( sResult==ID_TRUE )
                    {
                        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator,
                                                        (const smpSlotHeader*)sPtr,
                                                        &sCanReusableRollback,
                                                        &sIsVisible )
                                  != IDE_SUCCESS );
                        if( sIsVisible  == ID_TRUE )
                        {
                            if( sCanReusableRollback == ID_FALSE )
                            {     
                                sTrans->mIsReusableRollback = ID_FALSE;
                            }

                            j++;
                            aIterator->mRows[j] = (SChar*)sPtr;
                        }
                    }
                }

                aIterator->mHighFence = aIterator->mRows + j;
                aIterator->mLowFence  = aIterator->mRows;
                aIterator->mSlot      = aIterator->mLowFence - 1;
                aIterator->mNxtNode   = sNxtNode;
                
                IDL_MEM_BARRIER;
                if(sLatch == sCurNode->mLatch)
                {
                    if(sVersion == sCurNodeVersion)
                    {
                        aIterator->mVersion = ID_ULONG_MAX;
                    }
                    else
                    {
                        IDE_ASSERT(aIterator->mNxtNode != NULL);
                    }
                    break;
                }
            }

            if((aIterator->mVersion == ID_ULONG_MAX) && (aIterator->mDepth < 0))
            {
                aIterator->mHighest = ID_TRUE;
            }

            aIterator->mLeast     = ID_FALSE;

            IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                      != IDE_SUCCESS );
            
            goto restart;
        }
    }

    // BUG-23344
    IDE_EXCEPTION_CONT(NO_RESULT);
    
    aIterator->mHighest = ID_TRUE;
    aIterator->mDepth   = -1;
    aIterator->mVersion = ID_ULONG_MAX;

    if(aIterator->mKeyRange->next != NULL)
    {
        aIterator->mKeyRange = aIterator->mKeyRange->next;
        IDE_TEST( beforeFirstInternal(aIterator) != IDE_SUCCESS );

        IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                  != IDE_SUCCESS );
        
        goto restart;
    }
    
    aIterator->mSlot           = aIterator->mHighFence + 1;
    aIterator->mCurRecPtr      = NULL;
    aIterator->mLstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow                     = NULL;
/*
    aIterator->mFlag           = SMI_RETRAVERSE_AFTER;
*/
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

SInt stnmrRTree::compareRows( const stnmrColumn* aColumns,
                              const stnmrColumn* aFence,
                              const void*        aRow1,
                              const void*        aRow2 )
{
    
    SInt         sResult;
    smiValueInfo sValueInfo1;
    smiValueInfo sValueInfo2;
    
    SMI_SET_VALUEINFO( &sValueInfo1, NULL, aRow1, 0, SMI_OFFSET_USE,
                       &sValueInfo2, NULL, aRow2, 0, SMI_OFFSET_USE );
    for( ; aColumns < aFence; aColumns++ )
    {
        sValueInfo1.column = &aColumns->mColumn;
        sValueInfo2.column = &aColumns->mColumn;
        
        sResult = aColumns->mCompare( &sValueInfo1, &sValueInfo2 );
        if(sResult != 0)
        {
            return sResult;
        }
    }
    
    return 0;
    
}

SInt stnmrRTree::compareRowsAndOID( const stnmrColumn* aColumns,
                                    const stnmrColumn* aFence,
                                    SChar*             aRow1,
                                    SChar*             aRow2 )
{    
    SInt         sResult;
    smiValueInfo sValueInfo1;
    smiValueInfo sValueInfo2;
    
    SMI_SET_VALUEINFO( &sValueInfo1, NULL, aRow1, 0, SMI_OFFSET_USE,
                       &sValueInfo2, NULL, aRow2, 0, SMI_OFFSET_USE );
    for( ; aColumns < aFence; aColumns++ )
    {
        sValueInfo1.column = &aColumns->mColumn;
        sValueInfo2.column = &aColumns->mColumn;

        sResult = aColumns->mCompare( &sValueInfo1, &sValueInfo2 );
        if(sResult != 0)
        {
            return sResult;
        }
    }

    if(aRow1 > aRow2)
    {
        return 1;
    }
    if(aRow1 < aRow2)
    {
        return -1;
    }
    return 0;
    
}

SInt stnmrRTree::compareRows( const stnmrColumn* aColumns,
                              const stnmrColumn* aFence,
                              SChar*             aRow1,
                              SChar*             aRow2,
                              idBool*            aEqual )
{
    
    SInt         sResult;
    smiValueInfo sValueInfo1;
    smiValueInfo sValueInfo2;

    *aEqual = ID_FALSE; 
    
    SMI_SET_VALUEINFO( &sValueInfo1, NULL, aRow1, 0, SMI_OFFSET_USE,
                       &sValueInfo2, NULL, aRow2, 0, SMI_OFFSET_USE );
    for( ; aColumns < aFence; aColumns++ )
    {
        sValueInfo1.column = &aColumns->mColumn;
        sValueInfo2.column = &aColumns->mColumn;

        sResult = aColumns->mCompare( &sValueInfo1, &sValueInfo2 );
        if(sResult != 0)
        {
            return sResult;
        }
    }

    *aEqual = ID_TRUE;

    if(aRow1 > aRow2)
    {
        return 1;
    }
    if(aRow1 < aRow2)
    {
        return -1;
    }
    return 0;
    
}

IDE_RC stnmrRTree::rebuild( void*             /*aTrans*/,
                            smnIndexHeader*   /*aIndex*/,
                            SChar*            /*aNull*/,
                            smnSortStack*     /*aStack*/ )
{

    return IDE_SUCCESS;
    
}

void stnmrRTree::insertSlot( stnmrNode* aNode,
                             stnmrSlot* aSlot )

{

    idlOS::memcpy(&(aNode->mSlots[aNode->mSlotCount]),
                  aSlot, 
                  ID_SIZEOF(stnmrSlot));
    aNode->mSlotCount++;

}

IDE_RC stnmrRTree::makeDiskImage( smnIndexHeader* /*_ aIndex _*/,
                                  smnIndexFile*   /*_ aIndexFile _*/ )
{
    
    return IDE_SUCCESS;
}

IDU_LATCH stnmrRTree::getLatchValueOfNode( volatile stnmrNode* aNodePtr )
{
    return aNodePtr->mLatch;
}

IDU_LATCH stnmrRTree::getLatchValueOfHeader( volatile stnmrHeader* aHeaderPtr )
{
    return aHeaderPtr->mLatch;
}

IDE_RC stnmrRTree::getPositionNA( stnmrIterator *     /* aIterator */,
                                  smiCursorPosInfo * /* aPosInfo */ )
{
/*
       Do nothing...
*/
    return IDE_SUCCESS;

}

IDE_RC stnmrRTree::setPositionNA( stnmrIterator *    /* aIterator */,
                                  smiCursorPosInfo * /* aPosInfo */ )
{

/*
       Do nothing...
*/
    return IDE_SUCCESS;

}

IDE_RC stnmrRTree::allocIterator( void ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}


IDE_RC stnmrRTree::freeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

// BUG-22210
// Get geometry header from row data.
IDE_RC stnmrRTree::getGeometryHeaderFromRow( stnmrHeader*         aHeader,
                                       const void *         aRow,
                                       UInt                 aFlag,
                                       stdGeometryHeader**  aGeoHeader )
{
    smiColumn       * sColumn   = NULL;  
    UInt              sLen      = 0;
    
    sColumn = &aHeader->mColumns->mColumn;

    if ( ( (sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE ) ||
         ( (sColumn->flag & SMI_COLUMN_TYPE_MASK) == SMI_COLUMN_TYPE_VARIABLE_LARGE ) )
    {
        // should be memory!!
        IDE_ASSERT((sColumn->flag & SMI_COLUMN_STORAGE_MASK) == SMI_COLUMN_STORAGE_MEMORY);
        
        *aGeoHeader = (stdGeometryHeader*) sgmManager::getVarColumn( (SChar*)aRow, sColumn, &sLen );
        IDE_TEST_RAISE(*aGeoHeader == NULL, null_return);

    }
    else
    {
        // call original code 
        *aGeoHeader = 
            (stdGeometryHeader*) mtd::valueForModule( &aHeader->mColumns->mColumn,
                                                      aRow,
                                                      aFlag,
                                                      &stdGeometryNull );    
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(null_return);
    {
        *aGeoHeader = &stdGeometryNull;
    }
    IDE_EXCEPTION_END;
    // anyway return success!!
    return IDE_SUCCESS;
}


//======================================================================
//  X$MEM_RTREE_HEADER
//  memory rtree index의 run-time header를 보여주는 peformance view
//======================================================================

IDE_RC stnmrRTree::buildRecordForMemRTreeHeader(idvSQL              * /*aStatistics*/,
                                                void                * aHeader,
                                                void                * /* aDumpObj */,
                                                iduFixedTableMemory * aMemory)    
{
    smcTableHeader    * sCatTblHdr;
    smcTableHeader    * sTableHeader;
    smpSlotHeader     * sPtr;
    SChar             * sCurPtr;
    SChar             * sNxtPtr;
    UInt               sTableType;
    smnIndexHeader    * sIndexCursor;
    stnmrHeader       * sIndexHeader;
    stnmrHeader4PerfV   sIndexHeader4PerfV;
    void              * sTrans;
    UInt                sIndexCnt;
    UInt                i;
    void              * sISavepoint = NULL;
    UInt                sDummy = 0;
    UInt                sState = 0;

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr = NULL;

    if ( aMemory->useExternalMemory() == ID_TRUE )
    {
        /* BUG-43006 FixedTable Indexing Filter */
        sTrans = ((smiFixedTableProperties *)aMemory->getContext())->mTrans;
    }
    else
    {
        /* BUG-32292 [sm-util] Self deadlock occur since fixed-table building
         * operation uses another transaction. 
         * NestedTransaction을 사용하면 Self-deadlock 우려가 있다.
         * 따라서 id Memory 영역으로부터 Iterator를 얻어 Transaction을 얻어낸다. */
        sTrans = ((smiIterator*)aMemory->getContext())->trans;
    }

    while(1)
    {
        IDE_TEST(smcRecord::nextOIDall(sCatTblHdr,sCurPtr,&sNxtPtr)
                  != IDE_SUCCESS);
        if( sNxtPtr == NULL )
        {
            break;
        }
        sPtr = (smpSlotHeader *)sNxtPtr;
        
        if( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) == ID_TRUE )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }
        sTableHeader = (smcTableHeader *)( sPtr + 1 );
        
        sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

        // memory & meta table only
        if( (sTableType != SMI_TABLE_MEMORY) &&
            (sTableType != SMI_TABLE_META) )
        {
            sCurPtr = sNxtPtr;
            continue;
        }
        
        // BUG-30867 Discard 된 Tablespace에 속한 Table의 Index도 Skip되어야 함
        if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
           ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                         SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
        {
            sCurPtr = sNxtPtr;
            continue;
        }
        
        sIndexCnt =  smcTable::getIndexCount(sTableHeader);

        if( sIndexCnt != 0  )
        {
            //DDL 을 방지.
            sState = 1;
            IDE_TEST( smLayerCallback::setImpSavepoint( sTrans, 
                                                        &sISavepoint,
                                                        sDummy )
                      != IDE_SUCCESS );

            /* BUG-48160 lock 잡힌 table 은 제외하고 출력하는 기능 추가.
             * Property : __SKIP_LOCKED_TABLE_AT_FIXED_TABLE */
            if ( smLayerCallback::lockTableModeIS4FixedTable(sTrans,
                                                             SMC_TABLE_LOCK( sTableHeader ))
                 == IDE_SUCCESS )
            {
                sState = 2;
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                // BUG-30867 Discard 된 Tablespace에 속한 Table의 Index도 Skip되어야 함
                if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
                   ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                                 SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
                {
                    sState = 1;
                    IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                    sISavepoint )
                              != IDE_SUCCESS );
                    sState = 0;
                    IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                                  sISavepoint )
                              != IDE_SUCCESS );
                    sCurPtr = sNxtPtr;
                    continue;
                }//if

                // lock을 대기하는 동안 index가 drop되었거나, 새로운 index가
                // 생성되었을 수 있으므로 정확한 index 수를 다시 구한다.
                // 뿐만 아니라, index cnt를 증가시킨 후 index를 생성하므로
                // index가 완료되지 못하면 index cnt가 감소하므로 다시 구해야 함.
                sIndexCnt = smcTable::getIndexCount(sTableHeader);

                for( i = 0; i < sIndexCnt; i++ )
                {
                    sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader, i );
                    if( sIndexCursor->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
                    {
                        continue;
                    }
                    sIndexHeader = (stnmrHeader*)(sIndexCursor->mHeader);

                    if( sIndexHeader == NULL )
                    {
                        /* BUG-32417 [sm-mem-index] The fixed table 
                         * 'X$MEM_BTREE_HEADER'
                         * doesn't consider that indices is disabled. 
                         * IndexRuntimeHeader가 없는 경우는 제외한다. */
                        idlOS::memset( &sIndexHeader4PerfV, 
                                       0x00, 
                                       ID_SIZEOF(stnmrHeader4PerfV) );
                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;
                    }
                    else
                    {
                        idlOS::memset( &sIndexHeader4PerfV, 
                                       0x00, 
                                       ID_SIZEOF(stnmrHeader4PerfV) );
                        idlOS::memcpy( &sIndexHeader4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);
                        sIndexHeader4PerfV.mIndexID = sIndexCursor->mId;
                        sIndexHeader4PerfV.mTableTSID = sTableHeader->mSpaceID;
                        sIndexHeader4PerfV.mUsedNodeCount = sIndexHeader->mNodeCount;
                        sIndexHeader4PerfV.mTreeMBR = sIndexHeader->mTreeMBR;
                        sIndexHeader4PerfV.mPrepareNodeCount =
                            sIndexHeader->mNodePool.getFreeSlotCount();
                    }

                    IDE_TEST( iduFixedTable::buildRecord( 
                                  aHeader,
                                  aMemory,
                                  (void *)&sIndexHeader4PerfV )
                              != IDE_SUCCESS);
                }//for
                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                sISavepoint )
                          != IDE_SUCCESS );
            }
            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                          sISavepoint )
                      != IDE_SUCCESS );
        }// if 인덱스가 있으면         
        sCurPtr = sNxtPtr;
    }// while

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( smLayerCallback::abortToImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smLayerCallback::unsetImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gMemRTreeHeaderColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(stnmrHeader4PerfV, mName ),
        SMN_MAX_INDEX_NAME_SIZE,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(stnmrHeader4PerfV, mIndexID ),
        IDU_FT_SIZEOF(stnmrHeader4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TABLE_TBS_ID",
        offsetof(stnmrHeader4PerfV, mTableTSID ),
        IDU_FT_SIZEOF(stnmrHeader4PerfV, mTableTSID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MIN_X",
        offsetof(stnmrHeader4PerfV, mTreeMBR) + offsetof(stdMBR, mMinX),
        IDU_FT_SIZEOF(stdMBR, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MIN_Y",
        offsetof(stnmrHeader4PerfV, mTreeMBR) + offsetof(stdMBR, mMinY),
        IDU_FT_SIZEOF(stdMBR, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MAX_X",
        offsetof(stnmrHeader4PerfV, mTreeMBR) + offsetof(stdMBR, mMaxX),
        IDU_FT_SIZEOF(stdMBR, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MAX_Y",
        offsetof(stnmrHeader4PerfV, mTreeMBR) + offsetof(stdMBR, mMaxY),
        IDU_FT_SIZEOF(stdMBR, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_NODE_COUNT",
        offsetof(stnmrHeader4PerfV, mUsedNodeCount ),
        IDU_FT_SIZEOF(stnmrHeader4PerfV, mUsedNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"PREPARE_NODE_COUNT",
        offsetof(stnmrHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_SIZEOF(stnmrHeader4PerfV, mPrepareNodeCount ),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gMemRTreeHeaderDesc=
{
    (SChar *)"X$MEM_RTREE_HEADER",
    stnmrRTree::buildRecordForMemRTreeHeader,
    gMemRTreeHeaderColDesc,    
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

//======================================================================
//  X$MEM_RTREE_STAT
//  memory index의 run-time statistic information을 위한 peformance view
//======================================================================

IDE_RC stnmrRTree::buildRecordForMemRTreeStat(idvSQL              * /*aStatistics*/,
                                              void                * aHeader,
                                              void                * /* aDumpObj */,
                                              iduFixedTableMemory * aMemory)    
{
    smcTableHeader   * sCatTblHdr;
    smcTableHeader   * sTableHeader;
    smpSlotHeader    * sPtr;
    SChar            * sCurPtr;
    SChar            * sNxtPtr;
    UInt               sTableType;
    smnIndexHeader   * sIndexCursor;
    stnmrHeader      * sIndexHeader;
    stnmrStat4PerfV    sIndexStat4PerfV;
    void             * sTrans;
    UInt               sIndexCnt;
    UInt               i;
    void             * sISavepoint = NULL;
    UInt               sDummy = 0;
    UInt               sState = 0;

    sCatTblHdr = (smcTableHeader*)SMC_CAT_TABLE;
    sCurPtr = NULL;

    if ( aMemory->useExternalMemory() == ID_TRUE )
    {
        /* BUG-43006 FixedTable Indexing Filter */
        sTrans = ((smiFixedTableProperties *)aMemory->getContext())->mTrans;
    }
    else
    {
        /* BUG-32292 [sm-util] Self deadlock occur since fixed-table building
         * operation uses another transaction. 
         * NestedTransaction을 사용하면 Self-deadlock 우려가 있다.
         * 따라서 id Memory 영역으로부터 Iterator를 얻어 Transaction을 얻어낸다. */
        sTrans = ((smiIterator*)aMemory->getContext())->trans;
    }

    while(1)
    {
        IDE_TEST(smcRecord::nextOIDall(sCatTblHdr,sCurPtr,&sNxtPtr)
                  != IDE_SUCCESS);
        if( sNxtPtr == NULL )
        {
            break;
        }
        sPtr = (smpSlotHeader *)sNxtPtr;
        
        if( SM_SCN_IS_INFINITE(sPtr->mCreateSCN) == ID_TRUE )
        {
            /* BUG-14974: 무한 Loop발생.*/
            sCurPtr = sNxtPtr;
            continue;
        }
        sTableHeader = (smcTableHeader *)( sPtr + 1 );
        
        sTableType = sTableHeader->mFlag & SMI_TABLE_TYPE_MASK;

        // memory & meta table only
        if( (sTableType != SMI_TABLE_MEMORY) &&
            (sTableType != SMI_TABLE_META) )
        {
            sCurPtr = sNxtPtr;
            continue;
        }

        // BUG-30867 Discard 된 Tablespace에 속한 Table도 Skip되어야 함
        if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
           ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                             SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
        {
            sCurPtr = sNxtPtr;
            continue;
        }
        
        sIndexCnt =  smcTable::getIndexCount(sTableHeader);

        if( sIndexCnt != 0  )
        {
            //DDL 을 방지.
            sState = 1;
            IDE_TEST( smLayerCallback::setImpSavepoint( sTrans, 
                                                        &sISavepoint,
                                                        sDummy )
                      != IDE_SUCCESS );

            /* BUG-48160 lock 잡힌 table 은 제외하고 출력하는 기능 추가.
             * Property : __SKIP_LOCKED_TABLE_AT_FIXED_TABLE */
            if ( smLayerCallback::lockTableModeIS4FixedTable( sTrans,
                                                              SMC_TABLE_LOCK( sTableHeader ))
                 == IDE_SUCCESS )
            {
                sState = 2;
            
                //lock을 잡았지만 table이 drop된 경우에는 skip;
                // BUG-30867 Discard 된 Tablespace에 속한 Table도 Skip되어야 함
                if(( smcTable::isDropedTable(sTableHeader) == ID_TRUE ) ||
                   ( sctTableSpaceMgr::hasState( sTableHeader->mSpaceID,
                                                 SCT_SS_INVALID_DISK_TBS ) == ID_TRUE ))
                {
                    sState = 1;
                    IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                    sISavepoint )
                              != IDE_SUCCESS );
                    sState = 0;
                    IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                                  sISavepoint )
                              != IDE_SUCCESS );
                    sCurPtr = sNxtPtr;
                    continue;
                }//if

                // lock을 대기하는 동안 index가 drop되었거나, 새로운 index가
                // 생성되었을 수 있으므로 정확한 index 수를 다시 구한다.
                // 뿐만 아니라, index cnt를 증가시킨 후 index를 생성하므로
                // index가 완료되지 못하면 index cnt가 감소하므로 다시 구해야 함.
                sIndexCnt = smcTable::getIndexCount(sTableHeader);

                for( i = 0; i < sIndexCnt; i++ )
                {
                    sIndexCursor = (smnIndexHeader*)smcTable::getTableIndex( sTableHeader, i );
                    if( sIndexCursor->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
                    {
                        continue;
                    }

                    sIndexHeader = (stnmrHeader*)(sIndexCursor->mHeader);
                    if( sIndexHeader == NULL )
                    {
                        /* BUG-32417 [sm-mem-index] The fixed table 
                         * 'X$MEM_BTREE_HEADER'
                         * doesn't consider that indices is disabled. 
                         * IndexRuntimeHeader가 없는 경우는 제외한다. */
                        idlOS::memset( &sIndexStat4PerfV, 
                                       0x00, 
                                       ID_SIZEOF(stnmrStat4PerfV) );

                        idlOS::memcpy( &sIndexStat4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);

                        sIndexStat4PerfV.mIndexID = 
                            sIndexCursor->mId;
                    }
                    else
                    {
                        idlOS::memset( &sIndexStat4PerfV, 
                                       0x00, 
                                       ID_SIZEOF(stnmrStat4PerfV) );

                        idlOS::memcpy( &sIndexStat4PerfV.mName,
                                       &sIndexCursor->mName,
                                       SMN_MAX_INDEX_NAME_SIZE+8);

                        sIndexStat4PerfV.mIndexID = 
                            sIndexCursor->mId;

                        sIndexStat4PerfV.mTreeLatchStat = 
                            *(sIndexHeader->mMutex.getMutexStat());

                        sIndexStat4PerfV.mKeyCount = sIndexHeader->mKeyCount;
                        sIndexStat4PerfV.mStmtStat = sIndexHeader->mStmtStat;
                        sIndexStat4PerfV.mAgerStat = sIndexHeader->mAgerStat;
                        sIndexStat4PerfV.mTreeMBR  = sIndexHeader->mTreeMBR;
                    }

                    IDE_TEST( iduFixedTable::buildRecord( 
                                  aHeader,
                                  aMemory,
                                  (void *)&sIndexStat4PerfV )
                              != IDE_SUCCESS);
                }//for
                sState = 1;
                IDE_TEST( smLayerCallback::abortToImpSavepoint( sTrans, 
                                                                sISavepoint )
                          != IDE_SUCCESS );
            }
            sState = 0;
            IDE_TEST( smLayerCallback::unsetImpSavepoint( sTrans, 
                                                          sISavepoint )
                      != IDE_SUCCESS );
        }// if 인덱스가 있으면         
        sCurPtr = sNxtPtr;
    }// while

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch ( sState )
    {
        case 2:
            IDE_ASSERT( smLayerCallback::abortToImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( smLayerCallback::unsetImpSavepoint( sTrans, sISavepoint )
                        == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gMemRTreeStatColDesc[]=
{
    {
        (SChar*)"INDEX_NAME",
        offsetof(stnmrStat4PerfV, mName ),
        SMN_MAX_INDEX_NAME_SIZE,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"INDEX_ID",
        offsetof(stnmrStat4PerfV, mIndexID ),
        IDU_FT_SIZEOF(stnmrStat4PerfV, mIndexID),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_TRY_COUNT",
        offsetof(stnmrStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mTryCount),
        IDU_FT_SIZEOF(iduMutexStat, mTryCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_LOCK_COUNT",
        offsetof(stnmrStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mLockCount),
        IDU_FT_SIZEOF(iduMutexStat, mLockCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar *)"TREE_LATCH_MISS_COUNT",
        offsetof(stnmrStat4PerfV, mTreeLatchStat) + offsetof(iduMutexStat, mMissCount),
        IDU_FT_SIZEOF(iduMutexStat, mMissCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"KEY_COUNT",
        offsetof(stnmrStat4PerfV, mKeyCount),
        IDU_FT_SIZEOF(stnmrStat4PerfV, mKeyCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_STMT",
        offsetof(stnmrStat4PerfV, mStmtStat) + offsetof(stnmrStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(stnmrStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_STMT",
        offsetof(stnmrStat4PerfV, mStmtStat) + offsetof(stnmrStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(stnmrStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_STMT",
        offsetof(stnmrStat4PerfV, mStmtStat) + offsetof(stnmrStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(stnmrStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_STMT",
        offsetof(stnmrStat4PerfV, mStmtStat) + offsetof(stnmrStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(stnmrStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_COMPARE_COUNT_BY_AGER",
        offsetof(stnmrStat4PerfV, mAgerStat) + offsetof(stnmrStatistic, mKeyCompareCount),
        IDU_FT_SIZEOF(stnmrStatistic, mKeyCompareCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"KEY_VALIDATION_COUNT_BY_AGER",
        offsetof(stnmrStat4PerfV, mAgerStat) + offsetof(stnmrStatistic, mKeyValidationCount),
        IDU_FT_SIZEOF(stnmrStatistic, mKeyValidationCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_SPLIT_COUNT_BY_AGER",
        offsetof(stnmrStat4PerfV, mAgerStat) + offsetof(stnmrStatistic, mNodeSplitCount),
        IDU_FT_SIZEOF(stnmrStatistic, mNodeSplitCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"NODE_DELETE_COUNT_BY_AGER",
        offsetof(stnmrStat4PerfV, mAgerStat) + offsetof(stnmrStatistic, mNodeDeleteCount),
        IDU_FT_SIZEOF(stnmrStatistic, mNodeDeleteCount),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL
    },
    {
        (SChar*)"TREE_MBR_MIN_X",
        offsetof(stnmrStat4PerfV, mTreeMBR) + offsetof(stdMBR, mMinX),
        IDU_FT_SIZEOF(stdMBR, mMinX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MIN_Y",
        offsetof(stnmrStat4PerfV, mTreeMBR) + offsetof(stdMBR, mMinY),
        IDU_FT_SIZEOF(stdMBR, mMinY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MAX_X",
        offsetof(stnmrStat4PerfV, mTreeMBR) + offsetof(stdMBR, mMaxX),
        IDU_FT_SIZEOF(stdMBR, mMaxX ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TREE_MBR_MAX_Y",
        offsetof(stnmrStat4PerfV, mTreeMBR) + offsetof(stdMBR, mMaxY),
        IDU_FT_SIZEOF(stdMBR, mMaxY ),
        IDU_FT_TYPE_DOUBLE,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gMemRTreeStatDesc=
{
    (SChar *)"X$MEM_RTREE_STAT",
    stnmrRTree::buildRecordForMemRTreeStat,
    gMemRTreeStatColDesc,    
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_USE,
    NULL
};

//======================================================================
//  X$MEM_RTREE_NODEPOOL
//  memory index의 node pool을 보여주는 peformance view
//======================================================================
IDE_RC stnmrRTree::buildRecordForMemRTreeNodePool(idvSQL              * /*aStatistics*/,
                                                  void                * aHeader,
                                                  void                * /* aDumpObj */,
                                                  iduFixedTableMemory * aMemory)    
{
    stnmrNodePool4PerfV   sIndexNodePool4PerfV;
    smnFreeNodeList     * sFreeNodeList = (smnFreeNodeList*)gSmnrFreeNodeList;

    UInt sSlotPerPage    = gSmnrNodePool.getSlotPerPage();
    UInt sAllocSlotCount = gSmnrNodePool.getAllocSlotCount();

    /* BUG-46402 : 페이지 개념이 없어졌으나, 호환성을 위해서 계산함 */
    sIndexNodePool4PerfV.mTotalPageCount = ( sSlotPerPage == 0 ) ? 0 
                                           : ( ( sAllocSlotCount + sSlotPerPage - 1 ) / sSlotPerPage );

    sIndexNodePool4PerfV.mTotalNodeCount = sAllocSlotCount;
    sIndexNodePool4PerfV.mFreeNodeCount  = gSmnrNodePool.getFreeSlotCount();
    sIndexNodePool4PerfV.mUsedNodeCount  =
        sIndexNodePool4PerfV.mTotalNodeCount - sIndexNodePool4PerfV.mFreeNodeCount;
    sIndexNodePool4PerfV.mNodeSize       = gSmnrNodePool.getSlotSize();
    sIndexNodePool4PerfV.mTotalAllocReq  = gSmnrNodePool.getTotalAllocReq();
    sIndexNodePool4PerfV.mTotalFreeReq   = gSmnrNodePool.getTotalFreeReq();

    sIndexNodePool4PerfV.mFreeReqCount   =
        sFreeNodeList->mAddCnt - sFreeNodeList->mHandledCnt;

    IDE_TEST( iduFixedTable::buildRecord( aHeader,
                                          aMemory,
                                          (void *)&sIndexNodePool4PerfV )
              != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static iduFixedTableColDesc  gMemRTreeNodePoolColDesc[]=
{
    {
        (SChar*)"TOTAL_PAGE_COUNT",
        offsetof(stnmrNodePool4PerfV, mTotalPageCount ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mTotalPageCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_NODE_COUNT",
        offsetof(stnmrNodePool4PerfV, mTotalNodeCount ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mTotalNodeCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_NODE_COUNT",
        offsetof(stnmrNodePool4PerfV, mFreeNodeCount ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mFreeNodeCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"USED_NODE_COUNT",
        offsetof(stnmrNodePool4PerfV, mUsedNodeCount ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mUsedNodeCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"NODE_SIZE",
        offsetof(stnmrNodePool4PerfV, mNodeSize ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mNodeSize),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_ALLOC_REQ",
        offsetof(stnmrNodePool4PerfV, mTotalAllocReq ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mTotalAllocReq),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"TOTAL_FREE_REQ",
        offsetof(stnmrNodePool4PerfV, mTotalFreeReq ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mTotalFreeReq),
        IDU_FT_TYPE_UBIGINT,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        (SChar*)"FREE_REQ_COUNT",
        offsetof(stnmrNodePool4PerfV, mFreeReqCount ),
        IDU_FT_SIZEOF(stnmrNodePool4PerfV, mFreeReqCount),
        IDU_FT_TYPE_UINTEGER,
        NULL,
        0, 0,NULL // for internal use
    },
    {
        NULL,
        0,
        0,
        IDU_FT_TYPE_CHAR,
        NULL,
        0, 0,NULL // for internal use
    }
};

iduFixedTableDesc  gMemRTreeNodePoolDesc=
{
    (SChar *)"X$MEM_RTREE_NODEPOOL",
    stnmrRTree::buildRecordForMemRTreeNodePool,
    gMemRTreeNodePoolColDesc,    
    IDU_STARTUP_META,
    0,
    0,
    IDU_FT_DESC_TRANS_NOT_USE,
    NULL
};

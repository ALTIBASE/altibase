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
 * $Id: stndrModule.cpp 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

/*********************************************************************
 * FILE DESCRIPTION : stndrModule.cpp
 * ------------------------------------------------------------------*
 *
 *********************************************************************/

#include <idl.h>
#include <ide.h>
#include <mtd.h>

#include <smDef.h>
#include <smnDef.h>
#include <smnManager.h>

#include <sdp.h>
#include <sdc.h>
#include <sdnReq.h>

#include <smxTrans.h>
#include <sdrMiniTrans.h>
#include <sdnManager.h>
#include <sdnIndexCTL.h>
#include <sdbMPRMgr.h>

#include <stdUtils.h>

#include <stndrDef.h>
#include <stndrStackMgr.h>
#include <stndrModule.h>
#include <stndrTDBuild.h>
#include <stndrBUBuild.h>

extern smiGlobalCallBackList gSmiGlobalCallBackList;

extern mtdModule stdGeometry;

static UInt gMtxDLogType = SM_DLOG_ATTR_DEFAULT;


static sdnCallbackFuncs gCallbackFuncs4CTL =
{
    (sdnSoftKeyStamping)stndrRTree::softKeyStamping,
    (sdnHardKeyStamping)stndrRTree::hardKeyStamping
};


smnIndexModule stndrModule =
{
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_DISK,
                              SMI_ADDITIONAL_RTREE_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE | SMN_DIMENSION_ENABLE | SMN_DEFAULT_DISABLE |
    SMN_BOTTOMUP_BUILD_ENABLE,
    ID_UINT_MAX,                // BUG-23113: RTree Key Size에 제한을 두지 않는다.
    (smnMemoryFunc)             stndrRTree::prepareIteratorMem,
    (smnMemoryFunc)             stndrRTree::releaseIteratorMem,
    (smnMemoryFunc)             NULL, // prepareFreeNodeMem
    (smnMemoryFunc)             NULL, // releaseFreeNodeMem
    (smnCreateFunc)             stndrRTree::create,
    (smnDropFunc)               stndrRTree::drop,

    (smTableCursorLockRowFunc)  stndrRTree::lockRow,
    (smnDeleteFunc)             stndrRTree::deleteKey,
    (smnFreeFunc)               NULL,
    (smnInsertRollbackFunc)     stndrRTree::insertKeyRollback,
    (smnDeleteRollbackFunc)     stndrRTree::deleteKeyRollback,
    (smnAgingFunc)              stndrRTree::aging,

    (smInitFunc)                stndrRTree::init,
    (smDestFunc)                stndrRTree::dest,
    (smnFreeNodeListFunc)       stndrRTree::freeAllNodeList,
    (smnGetPositionFunc)        stndrRTree::getPositionNA,
    (smnSetPositionFunc)        stndrRTree::setPositionNA,

    (smnAllocIteratorFunc)      stndrRTree::allocIterator,
    (smnFreeIteratorFunc)       stndrRTree::freeIterator,
    (smnReInitFunc)             NULL,
    (smnInitMetaFunc)           stndrRTree::initMeta,

    (smnMakeDiskImageFunc)      NULL,
    (smnBuildIndexFunc)         stndrRTree::buildIndex,
    (smnGetSmoNoFunc)           stndrRTree::getSmoNo,
    (smnMakeKeyFromRow)         stndrRTree::makeKeyValueFromRow,
    (smnMakeKeyFromSmiValue)    stndrRTree::makeKeyValueFromSmiValueList,
    (smnRebuildIndexColumn)     stndrRTree::rebuildIndexColumn,
    (smnSetIndexConsistency)    stndrRTree::setConsistent,
    (smnGatherStat)             NULL
};


static const  smSeekFunc stndrSeekFunctions[32][12] =
{
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)stndrRTree::fetchNext,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirstW,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::fetchNext,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirstRR,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::fetchNext,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)stndrRTree::fetchNext,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirstW,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst,
        (smSeekFunc)stndrRTree::fetchNext,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::beforeFirst
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
        (smSeekFunc)stndrRTree::NA,
    }
};

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * MBR의 MinX 값으로 정렬하기 위한 compare 함수
 *********************************************************************/
extern "C" SInt gCompareKeyArrayByAxisX( const void * aLhs,
                                         const void * aRhs )
{
    const stndrKeyArray * sLhs = (const stndrKeyArray*)aLhs;
    const stndrKeyArray * sRhs = (const stndrKeyArray*)aRhs;

    IDE_ASSERT( aLhs != NULL );
    IDE_ASSERT( aRhs != NULL );

    if( sLhs->mMBR.mMinX == sRhs->mMBR.mMinX )
    {
        return 0;
    }
    else
    {
        if( sLhs->mMBR.mMinX > sRhs->mMBR.mMinX )
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * MBR의 MinY 값으로 정렬하기 위한 compare 함수
 *********************************************************************/
extern "C" SInt gCompareKeyArrayByAxisY( const void * aLhs,
                                         const void * aRhs )
{
    const stndrKeyArray * sLhs = (const stndrKeyArray*)aLhs;
    const stndrKeyArray * sRhs = (const stndrKeyArray*)aRhs;

    IDE_ASSERT( aLhs != NULL );
    IDE_ASSERT( aRhs != NULL );

    if( sLhs->mMBR.mMinY == sRhs->mMBR.mMinY )
    {
        return 0;
    }
    else
    {
        if( sLhs->mMBR.mMinY > sRhs->mMBR.mMinY )
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * quick sort를 위한 swap 함수
 *********************************************************************/
void stndrRTree::swap( stndrKeyArray * aArray,
                       SInt            i,
                       SInt            j )
{
    stndrKeyArray   sTmp;

    sTmp      = aArray[i];
    aArray[i] = aArray[j];
    aArray[j] = sTmp;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * KeyArray를 quick sort로 정렬하는 함수이다. compare 함수에 따라서 MBR의
 * MinX 또는 MinY 값으로 정렬한다.
 *********************************************************************/
void stndrRTree::quickSort( stndrKeyArray * aArray,
                            SInt            aArraySize,
                            SInt         (* compare)( const void* aLhs,
                                                      const void* aRhs ) )
{
    stndrSortStackSlot  sStack[STNDR_MAX_SORT_STACK_SIZE];
    stndrSortStackSlot  sSlot;
    SInt                sDepth = -1;
    SInt                sLast;
    SInt                i;

    
    sDepth++;
    sStack[sDepth].mArray     = aArray;
    sStack[sDepth].mArraySize = aArraySize;

    while( sDepth >= 0 )
    {
        IDE_ASSERT( sDepth < (SInt)STNDR_MAX_SORT_STACK_SIZE );
        
        sSlot = sStack[sDepth];
        sDepth--;
        
        if( sSlot.mArraySize <= 1 )
            continue;

        swap( sSlot.mArray, 0, sSlot.mArraySize/2 );
        sLast = 0;

        for( i = 1; i < sSlot.mArraySize; i++ )
        {
            if( compare( (const void *)&sSlot.mArray[i],
                         (const void *)&sSlot.mArray[0] ) < 0 )
            {
                swap( sSlot.mArray, ++sLast, i );
            }
        }

        swap( sSlot.mArray, 0, sLast );

        sDepth++;
        sStack[sDepth].mArray = sSlot.mArray;
        sStack[sDepth].mArraySize = sLast;

        sDepth++;
        sStack[sDepth].mArray = sSlot.mArray + sLast + 1;
        sStack[sDepth].mArraySize = sSlot.mArraySize - sLast - 1;
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 본 함수는 Iterator를 할당해 줄 memory pool을 초기화
 *********************************************************************/
IDE_RC stndrRTree::prepareIteratorMem( smnIndexModule * /* aIndexModule */ )
{
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 본 함수는 Iterator memory pool을 해제
 *********************************************************************/
IDE_RC stndrRTree::releaseIteratorMem( const smnIndexModule * )
{
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 인덱스를 새로 build하다가 에러가 발생하면 지금까지 만들어온 인덱스
 * 노드들을 해제해야 한다. 본 함수는 이럴때 호출된다. 세그먼트에서
 * 할당된 모든 Page를 해제한다.
 *********************************************************************/
IDE_RC stndrRTree::freeAllNodeList( idvSQL          * aStatistics,
                                    smnIndexHeader  * aIndex,
                                    void            * aTrans )
{
    sdrMtx            sMtx;
    idBool            sMtxStart = ID_FALSE;
    stndrHeader     * sIndex;
    sdpSegmentDesc  * sSegmentDesc;
    

    sIndex = (stndrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    // FOR A4 : index build시에 에러 발생하면 호출됨
    // 인덱스가 생성한 모든 노드들을 해제함

    // Index Header에서 Segment Descriptor가 소유한 모든 page를 해제한다.
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    
    sMtxStart = ID_TRUE;
    
    sSegmentDesc = &(sIndex->mSdnHeader.mSegmentDesc);
    
    IDE_TEST( sdpSegDescMgr::getSegMgmtOp(
                  &(sIndex->mSdnHeader.mSegmentDesc))->mResetSegment(
                      aStatistics,
                      &sMtx,
                      SC_MAKE_SPACE(aIndex->mIndexSegDesc),
                      &(sSegmentDesc->mSegHandle),
                      SDP_SEG_TYPE_INDEX )
              != IDE_SUCCESS );

    sMtxStart = ID_FALSE;
    
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMtxStart == ID_TRUE )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 키를 생성하기 위해서는 Row를 Fetch해야 한다. 이때 모든 칼럼이
 * 아닌, Index가 걸린 칼럼만 Fetch해야 하기 때문에 따로 FetchColumn-
 * List4Key를 구성한다.
 *********************************************************************/
IDE_RC stndrRTree::makeFetchColumnList4Index( void          * aTableHeader,
                                              stndrHeader   * aIndexHeader )
{
    const smiColumn     * sTableColumn;
    smiColumn           * sSmiColumnInFetchColumn;
    smiFetchColumnList  * sFetchColumnList;
    stndrColumn         * sIndexColumn;
    UShort                sColumnSeq;

    
    IDE_DASSERT( aTableHeader != NULL );
    IDE_DASSERT( aIndexHeader != NULL );

    sIndexColumn = &(aIndexHeader->mColumn);
    sColumnSeq   = sIndexColumn->mKeyColumn.id % SMI_COLUMN_ID_MAXIMUM;
    sTableColumn = smcTable::getColumn(aTableHeader, sColumnSeq);

    sFetchColumnList        = &(aIndexHeader->mFetchColumnListToMakeKey);
    sSmiColumnInFetchColumn = (smiColumn *)sFetchColumnList->column;

    /* set fetch column */
    idlOS::memcpy( sSmiColumnInFetchColumn,
                   sTableColumn,
                   ID_SIZEOF(smiColumn) );

    /* Proj-1872 Disk Index 저장구조 최적화
     * Index에 달린 FetchColumnList는 VRow를 만들지 않는다. 따라서 Offset은
     * 의미가 없으므로 0으로 설정한다. */
    sSmiColumnInFetchColumn->offset = 0;

    sFetchColumnList->columnSeq = sColumnSeq;
    /* set fetch column list */
    sFetchColumnList->copyDiskColumn =
        (void *)sIndexColumn->mCopyDiskColumnFunc;

    sFetchColumnList->next = NULL;
    
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * create함수를 통해 인덱스 칼럼 정보를 설정하기 위해 호출된다. 이후
 * 추가적으로 실시간 Alter DDL에 의해 칼럼정보를 변경할 필요가 있을때도
 * 호출되게 된다.
 *********************************************************************/
IDE_RC stndrRTree::rebuildIndexColumn( smnIndexHeader   * aIndex,
                                       smcTableHeader   * aTable,
                                       void             * aHeader )
{
    const smiColumn * sTableColumn = NULL;
    stndrColumn     * sIndexColumn = NULL;
    stndrHeader     * sHeader = NULL;
    UInt              sColID;
    SInt              i;
    UInt              sNonStoringSize = 0;

    sHeader = (stndrHeader *)aHeader;

    // R-Tree는 컬럼 갯수가 1개이다.
    IDE_ASSERT( aIndex->mColumnCount == 1 );

    for( i = 0; i < 1/* aIndex->mColumnCount */; i++)
    {
        sIndexColumn = &sHeader->mColumn;

        // 컬럼 정보(KeyColumn, mt callback functions,...) 초기화
        sColID = aIndex->mColumns[i] & SMI_COLUMN_ID_MASK;
        IDE_TEST_RAISE( sColID >= aTable->mColumnCount, ERR_COLUMN_NOT_FOUND );

        sTableColumn = smcTable::getColumn( aTable,sColID );

        IDE_TEST_RAISE( sTableColumn->id != aIndex->mColumns[i],
                        ERR_COLUMN_NOT_FOUND );

        // make mVRowColumn (for cursor level visibility )
        idlOS::memcpy( &sIndexColumn->mVRowColumn,
                       sTableColumn,
                       ID_SIZEOF(smiColumn) );

        // make mKeyColumn
        idlOS::memcpy( &sIndexColumn->mKeyColumn,
                       &sIndexColumn->mVRowColumn,
                       ID_SIZEOF(smiColumn) );

        sIndexColumn->mKeyColumn.flag = aIndex->mColumnFlags[i];
        sIndexColumn->mKeyColumn.flag &= ~SMI_COLUMN_USAGE_MASK;
        sIndexColumn->mKeyColumn.flag |= SMI_COLUMN_USAGE_INDEX;

        // PROJ-1872 Disk Index 저장구조 최적화
        IDE_TEST( gSmiGlobalCallBackList.findCopyDiskColumnValue( 
                      sTableColumn,
                      &sIndexColumn->mCopyDiskColumnFunc )
                  != IDE_SUCCESS );
        
        IDE_TEST( gSmiGlobalCallBackList.findKey2String(
                      sTableColumn,
                      aIndex->mColumnFlags[i],
                      &sIndexColumn->mKey2String )
                  != IDE_SUCCESS );
        
        IDE_TEST( gSmiGlobalCallBackList.findIsNull( 
                      sTableColumn,
                      aIndex->mColumnFlags[i],
                      &sIndexColumn->mIsNull )
                  != IDE_SUCCESS );
        
        /* PROJ-1872 Disk Index 저장구조 최적화
         * MakeKeyValueFromRow시, Row는 Length-Known타입의 Null을 1Byte로 압축
         * 하여 표현하기 때문에 NullValue를 알지 못한다. 따라서 이 Null을 가져
         * 오기 위해 mNull 함수를 설정한다. */
        IDE_TEST( gSmiGlobalCallBackList.findNull( 
                      sTableColumn,
                      aIndex->mColumnFlags[i],
                      &sIndexColumn->mNull )
                  != IDE_SUCCESS );

        /* BUG-24449 
         * 키의 헤더 크기는 타입에 따라 다르다. */
         IDE_TEST( gSmiGlobalCallBackList.getNonStoringSize( sTableColumn, 
                                                             &sNonStoringSize )
                   != IDE_SUCCESS );
        sIndexColumn->mMtdHeaderLength  = sNonStoringSize;
            
    } // for
    
    // BUG-22946 
    IDE_TEST( makeFetchColumnList4Index(aTable, sHeader) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_COLUMN_NOT_FOUND );

    IDE_SET( ideSetErrorCode(smERR_FATAL_smnColumnNotFound) );

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * To Fix BUG-21925                     
 * Meta Page의 mIsConsistent를 설정한다.
 *
 * 주의 : Transaction을 사용하지 않기 때문에 online 상태에서는
 *       사용되어서는 안된다.
 *********************************************************************/
IDE_RC stndrRTree::setConsistent( smnIndexHeader * aIndex,
                                  idBool           aIsConsistent )
{
    idvSQL      * sStatistics;
    idvSession    sDummySession;
    idvSQL        sDummySQL;
    sdrMtx        sMtx;
    stndrMeta   * sMeta;
    idBool        sTrySuccess;
    UInt          sState = 0;
    stndrHeader * sHeader;
    scSpaceID     sSpaceID;
    sdRID         sMetaRID;

    sHeader  = (stndrHeader*)aIndex->mHeader;
    sSpaceID = sHeader->mSdnHeader.mIndexTSID;
    sMetaRID = sHeader->mSdnHeader.mMetaRID;

    
    idlOS::memset( &sDummySession, 0, ID_SIZEOF(sDummySession) );

    idvManager::initSQL( &sDummySQL,
                         &sDummySession,
                         NULL,
                         NULL,
                         NULL,
                         NULL,
                         IDV_OWNER_UNKNOWN );
    
    sStatistics = &sDummySQL;
    
    IDE_TEST( sdrMiniTrans::begin( sStatistics,
                                   &sMtx,
                                   (void*)NULL,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_DUMMY )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( sdbBufferMgr::getPageByRID( sStatistics,
                                          sSpaceID,
                                          sMetaRID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          &sMtx,
                                          (UChar**)&sMeta,
                                          &sTrySuccess )
              != IDE_SUCCESS );

    
    IDE_TEST( sdrMiniTrans::writeNBytes( &sMtx,
                                         (UChar*)&sMeta->mIsConsistent,
                                         (void*)&aIsConsistent,
                                         ID_SIZEOF(aIsConsistent) )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 본 함수는 시스템 스타트때나 혹은 인덱스를 새로 create할 때 run-time
 * index header를 생성하여 다는 역할을 한다. 메모리 인덱스와 동일하게
 * 하기 위해 smmManager로 부터 temp Page를 할당받아 사용한다. 나중에
 * memmgr에서 받도록 변경 요망.(모든 인덱스에 대해서)
 *********************************************************************/
IDE_RC stndrRTree::create( idvSQL               * aStatistics,
                           smcTableHeader       * aTable,
                           smnIndexHeader       * aIndex,
                           smiSegAttr           * aSegAttr,
                           smiSegStorageAttr    * aSegStorageAttr,
                           smnInsertFunc        * aInsert,
                           smnIndexHeader      ** /*aRebuildIndexHeader*/,
                           ULong                  aSmoNo )
{
    UChar         * sPage;
    UChar         * sMetaPagePtr;
    stndrMeta     * sMeta;
    stndrHeader   * sHeader = NULL;
    idBool          sTrySuccess;
    sdpSegState     sIndexSegState;
    smLSN           sRecRedoLSN;
    sdpSegMgmtOp  * sSegMgmtOp;
    scPageID        sSegPID;
    scPageID        sMetaPID;
    UInt            sState = 0;
    SChar           sBuffer[IDU_MUTEX_NAME_LEN];
    stndrNodeHdr  * sNodeHdr = NULL;


    // Disk R-Tree의 컬럼 수는 1개이다.
    IDE_ASSERT( aIndex->mColumnCount == 1 );

    // 디스크 R-Tree의 Run Time Header 및 멤버 동적 할당
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_ST_STN,
                                ID_SIZEOF(stndrHeader),
                                (void**)&sHeader)
              != IDE_SUCCESS );

    /* BUG-40964 runtime index header 연결 위치를 메모리 할당 이후로 변경 */  
    aIndex->mHeader = (smnRuntimeHeader*) sHeader;
    
    sState = 1;

    IDE_TEST( iduMemMgr::malloc(
                  IDU_MEM_ST_STN,
                  ID_SIZEOF(smiColumn),
                  (void**) &(sHeader->mFetchColumnListToMakeKey.column) )
              != IDE_SUCCESS);
    sState = 2;

    // Run Time Header 멤버 변수 초기화
    idlOS::snprintf( sBuffer, 
                     ID_SIZEOF(sBuffer), 
                     "INDEX_HEADER_LATCH_%"ID_UINT32_FMT, 
                     aIndex->mId );

    IDE_TEST( sHeader->mSdnHeader.mLatch.initialize(sBuffer) != IDE_SUCCESS );

    /* statistics */
    IDE_TEST( smnManager::initIndexStatistics( aIndex,
                                               (smnRuntimeHeader*)sHeader,
                                               aIndex->mId )
              != IDE_SUCCESS );

    idlOS::snprintf( sBuffer,
                     ID_SIZEOF(sBuffer),
                     "%s""%"ID_UINT32_FMT,
                     "INDEX_SMONO_MUTEX_",
                     aIndex->mId );

    IDE_TEST( sHeader->mSmoNoMutex.initialize(
                  sBuffer,
                  IDU_MUTEX_KIND_POSIX,
                  IDV_WAIT_INDEX_NULL) != IDE_SUCCESS );

    sHeader->mSdnHeader.mIsCreated = ID_FALSE;
    sHeader->mSdnHeader.mLogging   = ID_TRUE;

    sHeader->mSdnHeader.mTableTSID = ((smcTableHeader*)aTable)->mSpaceID;
    sHeader->mSdnHeader.mIndexTSID = SC_MAKE_SPACE(aIndex->mIndexSegDesc);
    sHeader->mSdnHeader.mSmoNo     = aSmoNo;

    sHeader->mSdnHeader.mTableOID  = aIndex->mTableOID;
    sHeader->mSdnHeader.mIndexID   = aIndex->mId;

    sHeader->mSmoNoAtomicA = 0;
    sHeader->mSmoNoAtomicB = 0;

    sHeader->mVirtualRootNodeAtomicA = 0;
    sHeader->mVirtualRootNodeAtomicB = 0;

    sHeader->mKeyCount = ((smcTableHeader *)aTable)->mFixed.mDRDB.mRecCnt;

    idlOS::memset( &(sHeader->mDMLStat), 0x00, ID_SIZEOF(stndrStatistic) );
    idlOS::memset( &(sHeader->mQueryStat), 0x00, ID_SIZEOF(stndrStatistic) );


    // Segment 설정 (PROJ-1671)
    // insert high limit과 insert low limit은 사용하지 않지만 설정한다.
    sdpSegDescMgr::setDefaultSegAttr(
        &(sHeader->mSdnHeader.mSegmentDesc.mSegHandle.mSegAttr),
        SDP_SEG_TYPE_INDEX );

    sdpSegDescMgr::setSegAttr(
        &sHeader->mSdnHeader.mSegmentDesc, aSegAttr );

    IDE_DASSERT( aSegAttr->mInitTrans <= aSegAttr->mMaxTrans );
    IDE_DASSERT( aSegAttr->mInitTrans <= SMI_MAXIMUM_INDEX_CTL_SIZE );
    IDE_DASSERT( aSegAttr->mMaxTrans <= SMI_MAXIMUM_INDEX_CTL_SIZE );

    // Storage 속성을 설정한다.
    sdpSegDescMgr::setSegStoAttr( &sHeader->mSdnHeader.mSegmentDesc,
                                  aSegStorageAttr );

    /* BUG-37955 index segment에 table OID와 Index ID를 기록하도록 수정 */
    IDE_TEST( sdpSegDescMgr::initSegDesc(
                  &sHeader->mSdnHeader.mSegmentDesc,
                  SC_MAKE_SPACE(aIndex->mIndexSegDesc),
                  SC_MAKE_PID(aIndex->mIndexSegDesc),
                  SDP_SEG_TYPE_INDEX,
                  aIndex->mTableOID,
                  aIndex->mId ) != IDE_SUCCESS );

    if( sHeader->mSdnHeader.mSegmentDesc.mSegMgmtType !=
        sdpTableSpace::getSegMgmtType(SC_MAKE_SPACE(aIndex->mIndexSegDesc)) )
    {
        ideLog::log( IDE_SERVER_0, "\
===================================================\n\
               IDX Name : %s                       \n\
               ID       : %u                       \n\
                                                   \n\
sHeader->mSegmentDesc.mSegMgmtType : %u\n\
sdpTableSpace::getSegMgmtType(SC_MAKE_SPACE(aIndex->mIndexSegDesc)) : %u\n",
                     aIndex->mName,
                     aIndex->mId,
                     sHeader->mSdnHeader.mSegmentDesc.mSegMgmtType,
                     sdpTableSpace::getSegMgmtType(SC_MAKE_SPACE(aIndex->mIndexSegDesc)) );

        ideLog::log( IDE_SERVER_0, "\
-<stndrHeader>--------------------------------------\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)sHeader, ID_SIZEOF(stndrHeader) );
        
        ideLog::log( IDE_SERVER_0, "\
-<smnIndexHeader>----------------------------------\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)aIndex, ID_SIZEOF(smnIndexHeader) );
        IDE_ASSERT( 0 );
    }

    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( &sHeader->mSdnHeader.mSegmentDesc );

    IDE_TEST( sSegMgmtOp->mGetSegState(
                  aStatistics,
                  SC_MAKE_SPACE(aIndex->mIndexSegDesc),
                  sHeader->mSdnHeader.mSegmentDesc.mSegHandle.mSegPID,
                  &sIndexSegState)
              != IDE_SUCCESS );

    if( sIndexSegState == SDP_SEG_FREE )
    {
        // for PBT
        // restart disk index rebuild시 문제가 있어도 죽이지 않음.
        ideLog::log( SM_TRC_LOG_LEVEL_DINDEX,
                     SM_TRC_DINDEX_INDEX_SEG_FREE,
                     aTable->mFixed.mDRDB.mSegDesc.mSegHandle.mSegPID,
                     aIndex->mIndexSegDesc );
        
        return IDE_SUCCESS;
    }
    
    // set meta page to runtime header
    sSegPID  = sdpSegDescMgr::getSegPID(
        &(sHeader->mSdnHeader.mSegmentDesc) );

    IDE_TEST( sSegMgmtOp->mGetMetaPID(aStatistics,
                                      sHeader->mSdnHeader.mIndexTSID,
                                      sSegPID,
                                      0, /* Seg Meta PID Array Index */
                                      &sMetaPID)
              != IDE_SUCCESS );

    sHeader->mSdnHeader.mMetaRID =
        SD_MAKE_RID( sMetaPID, SMN_INDEX_META_OFFSET );

    IDE_TEST( sdbBufferMgr::fixPageByRID(
                  aStatistics,
                  sHeader->mSdnHeader.mIndexTSID,
                  sHeader->mSdnHeader.mMetaRID,
                  (UChar**)&sMeta,
                  &sTrySuccess)
              != IDE_SUCCESS );

    sMetaPagePtr = sdpPhyPage::getPageStartPtr( sMeta );
    
    sHeader->mRootNode              = sMeta->mRootNode;
    sHeader->mEmptyNodeHead         = sMeta->mEmptyNodeHead;
    sHeader->mEmptyNodeTail         = sMeta->mEmptyNodeTail;
    sHeader->mFreeNodeCnt           = sMeta->mFreeNodeCnt;
    sHeader->mFreeNodeHead          = sMeta->mFreeNodeHead;
    sHeader->mFreeNodeSCN           = sMeta->mFreeNodeSCN;
    sHeader->mConvexhullPointNum    = sMeta->mConvexhullPointNum;

    /* RTree는 NumDist가 의미 없음 */
    sHeader->mSdnHeader.mIsConsistent  = sMeta->mIsConsistent;

    sHeader->mMaxKeyCount = smuProperty::getRTreeMaxKeyCount();
    
    // 로깅 최소화 (PROJ-1469)
    // mIsConsistent = ID_FALSE : index access 불가
    sHeader->mSdnHeader.mIsCreatedWithLogging
        = sMeta->mIsCreatedWithLogging;
    
    sHeader->mSdnHeader.mIsCreatedWithForce
        = sMeta->mIsCreatedWithForce;
    
    sHeader->mSdnHeader.mCompletionLSN
        = sMeta->mNologgingCompletionLSN;

    IDE_TEST( sdbBufferMgr::unfixPage(aStatistics, sMetaPagePtr)
              != IDE_SUCCESS );

    // mIsConsistent = ID_TRUE 이고 NOLOGGING/NOFORCE로 생성되었을 경우
    // index build시 index page들이 disk에 force되었는지 check
    if( (sHeader->mSdnHeader.mIsConsistent         == ID_TRUE ) &&
        (sHeader->mSdnHeader.mIsCreatedWithLogging == ID_FALSE) &&
        (sHeader->mSdnHeader.mIsCreatedWithForce   == ID_FALSE) )
    {
        // index build후 index page들이 disk에 force되지 않았으면
        // sHeader->mCompletionLSN과 sRecRedoLSN을 비교해서
        // sHeader->mCompletionLSN이 sRecRedoLSN보다 크면
        // sHeader->mIsConsistent = FALSE
        (void)smrRecoveryMgr::getDiskRedoLSNFromLogAnchor( &sRecRedoLSN );

        if( smrCompareLSN::isGTE( &(sHeader->mSdnHeader.mCompletionLSN),
                                  &sRecRedoLSN ) == ID_TRUE )
        {
            sHeader->mSdnHeader.mIsConsistent = ID_FALSE;

            // To fix BUG-21925
            IDE_TEST( setConsistent( aIndex,
                                     ID_FALSE) /* isConsistent */
                      != IDE_SUCCESS );
        }
    }

    // Tree MBR 설정
    if( sMeta->mRootNode != SD_NULL_PID )
    {
        IDE_TEST( sdbBufferMgr::fixPageByPID(
                      aStatistics,
                      sHeader->mSdnHeader.mIndexTSID,
                      sMeta->mRootNode,
                      (UChar**)&sPage,
                      &sTrySuccess)
                  != IDE_SUCCESS );
        
        sNodeHdr = (stndrNodeHdr *)sdpPhyPage::getLogicalHdrStartPtr( sPage );
        sHeader->mTreeMBR = sNodeHdr->mMBR;
        sHeader->mInitTreeMBR = ID_TRUE;
        
        IDE_TEST( sdbBufferMgr::unfixPage(aStatistics, sPage)
                  != IDE_SUCCESS );
    }
    else
    {
        STNDR_INIT_MBR( sHeader->mTreeMBR );
        sHeader->mInitTreeMBR = ID_FALSE;
    }
    
    // column 설정
    IDE_TEST( rebuildIndexColumn( aIndex, aTable, sHeader ) != IDE_SUCCESS );

    // Insert, Delete 함수 설정
    *aInsert = stndrRTree::insertKey;

    // Virtual Root Node 설정
    setVirtualRootNode( sHeader,
                        sHeader->mRootNode,
                        sHeader->mSdnHeader.mSmoNo );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            (void)iduMemMgr::free( (void*)sHeader->mFetchColumnListToMakeKey.column );
            // no break
        case 1:
            (void)iduMemMgr::free( sHeader );
            break;
        default:
            break;
    }

    aIndex->mHeader = NULL;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 인덱스를 빌드한다.
 *********************************************************************/
IDE_RC stndrRTree::buildIndex( idvSQL           * aStatistics,
                               void             * aTrans,
                               smcTableHeader   * aTable,
                               smnIndexHeader   * aIndex,
                               smnGetPageFunc     /*aGetPageFunc */,
                               smnGetRowFunc      /*aGetRowFunc  */,
                               SChar            * /*aNullRow     */,
                               idBool             aIsNeedValidation,
                               idBool             /* aIsEnableTable */,
                               UInt               aParallelDegree,
                               UInt               aBuildFlag,
                               UInt               aTotalRecCnt )

{
    stndrHeader * sHeader;

    if ( aTotalRecCnt > 0 )
    {
        if ( (aBuildFlag & SMI_INDEX_BUILD_FASHION_MASK) ==
             SMI_INDEX_BUILD_BOTTOMUP )
        {
            IDE_TEST( buildDRBottomUp(aStatistics,
                                      aTrans,
                                      aTable,
                                      aIndex,
                                      aIsNeedValidation,
                                      aBuildFlag,
                                      aParallelDegree)
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( buildDRTopDown(aStatistics,
                                     aTrans,
                                     aTable,
                                     aIndex,
                                     aIsNeedValidation,
                                     aBuildFlag,
                                     aParallelDegree)
                      != IDE_SUCCESS );
        }
    }
    else
    {
        sHeader = (stndrHeader*)aIndex->mHeader;
        sHeader->mSdnHeader.mIsCreated = ID_TRUE;

        IDE_TEST( stndrBUBuild::updateStatistics(
                      aStatistics,
                      NULL,
                      aIndex,
                      0,
                      0 )
                  != IDE_SUCCESS );

        IDE_TEST( buildMeta(aStatistics,
                            aTrans,
                            sHeader)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Top-Down 방식의 인덱스 빌드를 수행한다.
 *********************************************************************/
IDE_RC stndrRTree::buildDRTopDown(idvSQL            * aStatistics,
                                  void              * aTrans,
                                  smcTableHeader    * aTable,
                                  smnIndexHeader    * aIndex,
                                  idBool              aIsNeedValidation,
                                  UInt                aBuildFlag,
                                  UInt                aParallelDegree )
{
    SInt              i;
    SInt              sThreadCnt     = 0;
    idBool            sCreateSuccess = ID_TRUE;
    ULong             sAllocPageCnt;
    stndrTDBuild    * sThreads       = NULL;
    stndrHeader     * sHeader;
    SInt              sInitThreadCnt = 0;
    UInt              sState         = 0;

    
    // disk temp table은 cluster index이기때문에
    // build index를 하지 않는다.
    // 즉 create cluster index후 , key를 insert하는 형태임.
    IDE_DASSERT( (aTable->mFlag & SMI_TABLE_TYPE_MASK) == SMI_TABLE_DISK );
    IDE_DASSERT( aIndex->mType == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID );

    sHeader = (stndrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    // create index 시에는 meta page를 잡지 않기위해 ID_FALSE로 해야한다.
    // No-logging 시에는 index runtime header에 세팅한다.
    sHeader->mSdnHeader.mIsCreated = ID_FALSE;

    IDE_TEST( buildMeta( aStatistics,
                         aTrans,
                         sHeader ) != IDE_SUCCESS );

    if( aParallelDegree == 0 )
    {
        sThreadCnt = smuProperty::getIndexBuildThreadCount();
    }
    else
    {
        sThreadCnt = aParallelDegree;
    }

    IDE_TEST( sdpPageList::getAllocPageCnt(
                  aStatistics,
                  aTable->mSpaceID,
                  &( aTable->mFixed.mDRDB.mSegDesc ),
                  &sAllocPageCnt )
              != IDE_SUCCESS );

    if( sAllocPageCnt  < (UInt)sThreadCnt )
    {
        sThreadCnt = 1;
    }

    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ST_STN,
                                 ID_SIZEOF(stndrTDBuild)*sThreadCnt,
                                 (void**)&sThreads )
              != IDE_SUCCESS );
    sState = 1;

    for( i = 0; i < sThreadCnt; i++)
    {
        new (sThreads + i) stndrTDBuild;
        IDE_TEST(sThreads[i].initialize(
                              sThreadCnt,
                              i,
                              aTable,
                              aIndex,
                              smLayerCallback::getFstDskViewSCN( aTrans ),
                              aIsNeedValidation,
                              aBuildFlag,
                              aStatistics,
                              &sCreateSuccess ) != IDE_SUCCESS);
        sInitThreadCnt++;
    }

    for( i = 0; i < sThreadCnt; i++)
    {
        IDE_TEST(sThreads[i].start( ) != IDE_SUCCESS);
        IDE_TEST(sThreads[i].waitToStart(0) != IDE_SUCCESS);
    }

    for( i = 0; i < sThreadCnt; i++)
    {
        IDE_TEST(sThreads[i].join() != IDE_SUCCESS);
    }

    if( sCreateSuccess != ID_TRUE )
    {
        IDE_TEST(iduCheckSessionEvent(aStatistics) != IDE_SUCCESS);
        for( i = 0; i < sThreadCnt; i++)
        {
            if( sThreads[i].getErrorCode() != 0 )
            {
                ideCopyErrorInfo( ideGetErrorMgr(),
                                  sThreads[i].getErrorMgr() );
                break;
            }
        }
        IDE_TEST( 1 );
    }

    for( i = ( sThreadCnt - 1 ) ; i >= 0 ; i-- )
    {
        sInitThreadCnt--;
        IDE_TEST( sThreads[i].destroy( ) != IDE_SUCCESS );
    }

    sState = 0;
    IDE_TEST( iduMemMgr::free(sThreads) != IDE_SUCCESS );

    sHeader->mSdnHeader.mIsCreated = ID_TRUE;
    sHeader->mSdnHeader.mIsConsistent = ID_TRUE;
    smrLogMgr::getLstLSN( &(sHeader->mSdnHeader.mCompletionLSN) );

    IDE_TEST( buildMeta( aStatistics,
                         aTrans,
                         sHeader )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();

    // BUG-27612 CodeSonar::Use After Free (4)
    if( sState == 1 )
    {
        for( i = 0 ; i < sInitThreadCnt; i++ )
        {
            (void)sThreads[i].destroy();
        }
        (void)iduMemMgr::free(sThreads);
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Bottom-UP 방식의 인덱스 빌드를 수행한다.
 *********************************************************************/
IDE_RC stndrRTree::buildDRBottomUp( idvSQL          * aStatistics,
                                    void            * aTrans,
                                    smcTableHeader  * aTable,
                                    smnIndexHeader  * aIndex,
                                    idBool            aIsNeedValidation,
                                    UInt              aBuildFlag,
                                    UInt              aParallelDegree )
{
    SInt          sThreadCnt;
    ULong         sTotalExtentCnt;
    ULong         sTotalSortAreaSize;
    UInt          sTotalMergePageCnt;
    sdpSegInfo    sSegInfo;
    stndrHeader * sHeader;

    // disk temp table은 cluster index이기때문에
    // build index를 하지 않는다.
    // 즉 create cluster index후 , key를 insert하는 형태임.
    IDE_DASSERT ((aTable->mFlag & SMI_TABLE_TYPE_MASK) == SMI_TABLE_DISK);
    IDE_DASSERT( aIndex->mType == SMI_ADDITIONAL_RTREE_INDEXTYPE_ID );

    sHeader = (stndrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    // create index 시에는 meta page를 잡지 않기위해 ID_FALSE로 해야한다.
    // No-logging 시에는 index runtime header에 세팅한다.
    sHeader->mSdnHeader.mIsCreated = ID_FALSE;

    if( aParallelDegree == 0 )
    {
        sThreadCnt = smuProperty::getIndexBuildThreadCount();
    }
    else
    {
        sThreadCnt = aParallelDegree;
    }

    IDE_TEST( sdpSegment::getSegInfo(
                  aStatistics,
                  aTable->mSpaceID,
                  sdpSegDescMgr::getSegPID( &(aTable->mFixed.mDRDB) ),
                  &sSegInfo )
              != IDE_SUCCESS );

    sTotalExtentCnt = sSegInfo.mExtCnt;

    if( sTotalExtentCnt < (UInt)sThreadCnt )
    {
        sThreadCnt = 1;
    }

    sTotalSortAreaSize = smuProperty::getSortAreaSize();

    // 쓰레드당 SORT_AREA_SIZE는 4페이지보다 커야 한다.
    while( (sTotalSortAreaSize / sThreadCnt) < SD_PAGE_SIZE*4 )
    {
        sThreadCnt = sThreadCnt / 2;
        if( sThreadCnt == 0 )
        {
            sThreadCnt = 1;
            break;
        }
    }

    sTotalMergePageCnt = smuProperty::getMergePageCount();

    // 쓰레드당 MERGE_PAGE_COUNT는 2페이지보다 커야 한다.
    while( (sTotalMergePageCnt / sThreadCnt) < 2 )
    {
        sThreadCnt = sThreadCnt / 2;
        if( sThreadCnt == 0 )
        {
            sThreadCnt = 1;
            break;
        }
    }

    // Parallel Index Build
    IDE_TEST( stndrBUBuild::main( aStatistics,
                                  aTrans,
                                  aTable,
                                  aIndex,
                                  sThreadCnt,
                                  sTotalSortAreaSize,
                                  sTotalMergePageCnt,
                                  aIsNeedValidation,
                                  aBuildFlag )
              != IDE_SUCCESS );

    setVirtualRootNode( sHeader,
                        sHeader->mRootNode,
                        sHeader->mSdnHeader.mSmoNo );

    sHeader->mSdnHeader.mIsCreated = ID_TRUE;

    IDE_TEST( buildMeta( aStatistics,
                         aTrans,
                         sHeader )
              != IDE_SUCCESS );

    // nologging & force인 경우 modify된 페이지들을 강제로 flush한다.
    if( (aBuildFlag & SMI_INDEX_BUILD_FORCE_MASK) == SMI_INDEX_BUILD_FORCE )
    {
        IDE_DASSERT( (aBuildFlag & SMI_INDEX_BUILD_LOGGING_MASK) ==
                     SMI_INDEX_BUILD_NOLOGGING );

        ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] Buffer Flush                 \n\
========================================\n" );

        IDE_TEST( sdbBufferMgr::flushPagesInRange(
                      NULL,
                      SC_MAKE_SPACE(aIndex->mIndexSegDesc),
                      0,
                      ID_UINT_MAX ) != IDE_SUCCESS );

        ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] Flushed Page Count           \n\
========================================\n");
    }

    ideLog::log( IDE_SM_0, "\
========================================\n\
 [IDX_CRE] BUILD INDEX COMPLETED        \n\
========================================\n" );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::drop
 * ------------------------------------------------------------------*
 * 본 함수는 index를 drop하거나 system을 shutdown할 때 호출된다.
 * drop시에는 run-time header만 헤제한다.
 * 이 함수는 commit 로그를 찍은 후, 혹은 shutdown시에만 들어오며,
 * index segment는 TSS에 이미 RID가 달린 상태이다.
 *********************************************************************/
IDE_RC stndrRTree::drop( smnIndexHeader * aIndex )
{
    stndrHeader * sHeader;

    
    sHeader = (stndrHeader*)(aIndex->mHeader);

    if( sHeader != NULL )
    {
        sHeader->mSdnHeader.mSegmentDesc.mSegHandle.mSegPID = SD_NULL_PID;

        IDE_TEST(
            sdpSegDescMgr::getSegMgmtOp(
                &(sHeader->mSdnHeader.mSegmentDesc))->mDestroy(
                &(sHeader->mSdnHeader.mSegmentDesc.mSegHandle) )
            != IDE_SUCCESS );

        sHeader->mSdnHeader.mSegmentDesc.mSegHandle.mCache = NULL;

        IDE_TEST( sHeader->mSdnHeader.mLatch.destroy()
                  != IDE_SUCCESS );

        IDE_TEST( smnManager::destIndexStatistics( aIndex,
                                                   (smnRuntimeHeader*)sHeader )
                  != IDE_SUCCESS );
        
        IDE_TEST( sHeader->mSmoNoMutex.destroy()
                  != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free( 
                      (void*)sHeader->mFetchColumnListToMakeKey.column)
                  != IDE_SUCCESS );

        IDE_TEST( iduMemMgr::free(sHeader) != IDE_SUCCESS );
        
        aIndex->mHeader = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::init
 * ------------------------------------------------------------------*
 * 본 함수는 인덱스를 traverse하기 위한 iterator를 초기화한다.
 *********************************************************************/
IDE_RC stndrRTree::init( stndrIterator          * aIterator,
                         void                   * aTrans,
                         smcTableHeader         * aTable,
                         smnIndexHeader         * aIndex,
                         void                   * /*aDumpObject*/,
                         const smiRange         * aKeyRange,
                         const smiRange         * /*aKeyFilter*/,
                         const smiCallBack      * aRowFilter,
                         UInt                     aFlag,
                         smSCN                    aSCN,
                         smSCN                    aInfinite,
                         idBool                   aUntouchable,
                         smiCursorProperties    * aProperties,
                         const smSeekFunc      ** aSeekFunc,
                         smiStatement           * aStatement )
{
    idvSQL  * sSQLStat = NULL;
    idBool    sStackInit = ID_FALSE;

    
    aIterator->mSCN             = aSCN;
    aIterator->mInfinite        = aInfinite;
    aIterator->mTrans           = aTrans;
    aIterator->mTable           = aTable;
    aIterator->mCurRecPtr       = NULL;
    aIterator->mLstFetchRecPtr  = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->mTID             = smLayerCallback::getTransID( aTrans );
    aIterator->mFlag            = aUntouchable == ID_TRUE ? SMI_ITERATOR_READ : SMI_ITERATOR_WRITE;
    aIterator->mProperties      = aProperties;
    aIterator->mStatement       = aStatement;

    aIterator->mIndex               = aIndex;
    aIterator->mKeyRange            = aKeyRange;
    aIterator->mRowFilter           = aRowFilter;
    aIterator->mIsLastNodeInRange   = ID_TRUE;
    aIterator->mCurRowPtr           = &aIterator->mRowCache[0];
    aIterator->mCacheFence          = &aIterator->mRowCache[0];
    aIterator->mPage                = (UChar*)aIterator->mAlignedPage;

    IDE_TEST( stndrStackMgr::initialize( &aIterator->mStack ) );
    sStackInit = ID_TRUE;

    idlOS::memset( aIterator->mPage, 0x00, SD_PAGE_SIZE );
    
    *aSeekFunc = stndrSeekFunctions[ aFlag & (SMI_TRAVERSE_MASK |
                                              SMI_PREVIOUS_MASK |
                                              SMI_LOCK_MASK) ];

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD( sSQLStat, mDiskCursorIndexScan, 1 );

    if( sSQLStat != NULL )
    {
        IDV_SESS_ADD( sSQLStat->mSess,
                      IDV_STAT_INDEX_DISK_CURSOR_IDX_SCAN, 1 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStackInit == ID_TRUE )
    {
        (void)stndrStackMgr::destroy( &aIterator->mStack );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 인덱스를 traverse하기 위한 iterator를 해제한다. stack traverse를 위해
 * 동적으로 생성한 메모리를 반환한다.
 *********************************************************************/
IDE_RC stndrRTree::dest( stndrIterator * aIterator )
{
    stndrStackMgr::destroy( &aIterator->mStack );
    
    return IDE_SUCCESS;
}
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Unchained Key(Normal Key)를 Chained Key로 변경한다.               
 * Chained Key라는 것은 Key가 가리키는 트랜잭션 정보가 CTS Chain상에
 * 있음을 의미한다.                                                 
 * 이미 Chained Key는 또 다시 Chained Key가 될수 없으며, Chained    
 * Key에 대한 정보는 UNDO에 기록되고, 향후 Visibility 검사시에      
 * 이용된다.
 *
 * !!CAUTION!! : Disk R-Tree의 경우 Chained Key를 만들 때 당시 CTS의
 * CommitSCN를 키의 mCreateSCN, mLimitSCN에 설정한다. findChainedKey 시에
 * 키의 mCreateSCN 또는 mLimitSCN를 Undo 레코드에 저장된 CTS의 CommitSCN과
 * 비교하여 해당 CTS의 Chained Key 여부를 판단한다.
 *********************************************************************/
IDE_RC stndrRTree::logAndMakeChainedKeys( sdrMtx          * aMtx,
                                          sdpPhyPageHdr   * aNode,
                                          UChar             aCTSlotNum,
                                          UChar           * aContext,
                                          UChar           * aKeyList,
                                          UShort          * aKeyListSize,
                                          UShort          * aChainedKeyCount )
{
    IDE_TEST( writeChainedKeysLog( aMtx,
                                   aNode,
                                   aCTSlotNum )
              != IDE_SUCCESS );

    IDE_TEST( makeChainedKeys( aNode,
                               aCTSlotNum,
                               aContext,
                               aKeyList,
                               aKeyListSize,
                               aChainedKeyCount )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Unchained Key(Normal Key)를 Chained Key로 변경한다.               
 * Chained Key라는 것은 Key가 가리키는 트랜잭션 정보가 CTS Chain상에
 * 있음을 의미한다.                                                 
 * 이미 Chained Key는 또 다시 Chained Key가 될수 없으며, Chained    
 * Key에 대한 정보는 UNDO에 기록되고, 향후 Visibility 검사시에      
 * 이용된다.
 *
 * !!CAUTION!! : Disk R-Tree의 경우 Chained Key를 만들 때 당시 CTS의
 * CommitSCN를 키의 mCreateSCN, mLimitSCN에 설정한다. findChainedKey 시에
 * 키의 mCreateSCN 또는 mLimitSCN를 Undo 레코드에 저장된 CTS의 CommitSCN과
 * 비교하여 해당 CTS의 Chained Key 여부를 판단한다.
 *********************************************************************/
IDE_RC stndrRTree::writeChainedKeysLog( sdrMtx          * aMtx,
                                        sdpPhyPageHdr   * aNode,
                                        UChar             aCTSlotNum )
{
    smSCN                     sCommitSCN;
    sdnCTL                  * sCTL;
    sdnCTS                  * sCTS;

    sCTL = sdnIndexCTL::getCTL( aNode );
    sCTS = sdnIndexCTL::getCTS( sCTL, aCTSlotNum );
    SM_SET_SCN( &sCommitSCN, &sCTS->mCommitSCN );

    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aNode,
                                         NULL,
                                         ID_SIZEOF(UChar) + ID_SIZEOF(smSCN),
                                         SDR_STNDR_MAKE_CHAINED_KEYS )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&aCTSlotNum,
                                   ID_SIZEOF(UChar))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&sCommitSCN,
                                   ID_SIZEOF(smSCN))
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Unchained Key(Normal Key)를 Chained Key로 변경한다.               
 * Chained Key라는 것은 Key가 가리키는 트랜잭션 정보가 CTS Chain상에
 * 있음을 의미한다.                                                 
 * 이미 Chained Key는 또 다시 Chained Key가 될수 없으며, Chained    
 * Key에 대한 정보는 UNDO에 기록되고, 향후 Visibility 검사시에      
 * 이용된다.
 *
 * !!CAUTION!! : Disk R-Tree의 경우 Chained Key를 만들 때 당시 CTS의
 * CommitSCN를 키의 mCreateSCN, mLimitSCN에 설정한다. findChainedKey 시에
 * 키의 mCreateSCN 또는 mLimitSCN를 Undo 레코드에 저장된 CTS의 CommitSCN과
 * 비교하여 해당 CTS의 Chained Key 여부를 판단한다.
 *********************************************************************/
IDE_RC stndrRTree::makeChainedKeys( sdpPhyPageHdr   * aNode,
                                    UChar             aCTSlotNum,
                                    UChar           * /* aContext */,
                                    UChar           * /* aKeyList */,
                                    UShort          * aKeyListSize,
                                    UShort          * aChainedKeyCount )
{
    UInt                      i;
    UShort                    sKeyCount;
    UChar                   * sSlotDirPtr;
    smSCN                     sCommitSCN;
    sdnCTL                  * sCTL;
    sdnCTS                  * sCTS;
    stndrLKey               * sLeafKey;

    sCTL = sdnIndexCTL::getCTL( aNode );
    sCTS = sdnIndexCTL::getCTS( sCTL, aCTSlotNum );
    SM_SET_SCN( &sCommitSCN, &sCTS->mCommitSCN );

    *aKeyListSize = 0;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aNode);
    sKeyCount   = sdpSlotDirectory::getCount(sSlotDirPtr);

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST ( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                            i,
                                                            (UChar**)&sLeafKey )
                   != IDE_SUCCESS );
        /*
         * DEAD KEY나 STABLE KEY는 Chained Key가 될수 없다.
         */
        if( (STNDR_GET_STATE( sLeafKey  ) == STNDR_KEY_DEAD) ||
            (STNDR_GET_STATE( sLeafKey  ) == STNDR_KEY_STABLE) )
        {
            continue;
        }

        /*
         * 이미 Chained Key는 다시 Chained Key가 될수 없다.
         */
        if( (STNDR_GET_CCTS_NO( sLeafKey ) == aCTSlotNum) &&
            (STNDR_GET_CHAINED_CCTS( sLeafKey ) == SDN_CHAINED_NO) )
        {
            STNDR_SET_CHAINED_CCTS( sLeafKey , SDN_CHAINED_YES );
            STNDR_SET_CSCN( sLeafKey, &sCommitSCN );
            (*aChainedKeyCount)++;
        }
        if( (STNDR_GET_LCTS_NO( sLeafKey ) == aCTSlotNum) &&
            (STNDR_GET_CHAINED_LCTS( sLeafKey ) == SDN_CHAINED_NO) )
        {
            STNDR_SET_CHAINED_LCTS( sLeafKey , SDN_CHAINED_YES );
            STNDR_SET_LSCN( sLeafKey, &sCommitSCN );
            (*aChainedKeyCount)++;
        }
    }

    /*
     * Key의 상태가 DEAD인 경우( LimitCTS만 Stamping이 된 경우) 라면
     * CTS.mRefCnt보다 작을수 있다
     */
    if( (*aChainedKeyCount > sdnIndexCTL::getRefKeyCount(aNode, aCTSlotNum))
        ||
        (ID_SIZEOF(sdnCTS) + ID_SIZEOF(UShort) + *aKeyListSize >= SD_PAGE_SIZE) )
    {
        ideLog::log( IDE_SERVER_0,
                     "CTS slot number : %u"
                     ", Chained key count : %u"
                     ", Ref Key count : %u"
                     ", Key List size : %u\n",
                     aCTSlotNum, *aChainedKeyCount,
                     sdnIndexCTL::getRefKeyCount( aNode, aCTSlotNum ),
                     *aKeyListSize );
        dumpIndexNode( aNode );
        IDE_ASSERT( 0 );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Chained Key를 Unchained Key(Normal Key)로 변경한다.               
 * Unchained Key라는 것은 Key가 가리키는 트랜잭션 정보가 Key.CTS#가
 * 가리키는 CTS에 있음을 의미한다.
 *********************************************************************/
IDE_RC stndrRTree::logAndMakeUnchainedKeys( idvSQL        * aStatistics,
                                            sdrMtx        * aMtx,
                                            sdpPhyPageHdr * aNode,
                                            sdnCTS        * aCTS,
                                            UChar           aCTSlotNum,
                                            UChar         * aChainedKeyList,
                                            UShort          aChainedKeySize,
                                            UShort        * aUnchainedKeyCount,
                                            UChar         * aContext )
{
    UChar       * sSlotDirPtr;
    UChar       * sUnchainedKey = NULL;
    UInt          sUnchainedKeySize = 0;
    UInt          sState = 0;
    UShort        sSlotCount;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aNode);
    sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

    /* SlotNum(UShort) + CreateCTS(UChar) + LimitCTS(UChar) */
    IDE_TEST( iduMemMgr::malloc(IDU_MEM_SM_SDN,
                ID_SIZEOF(UChar) * 4 * sSlotCount,
                (void **)&sUnchainedKey,
                IDU_MEM_IMMEDIATE )
            != IDE_SUCCESS);
    sState = 1;

    IDE_TEST( makeUnchainedKeys( aStatistics,
                                 aNode,
                                 aCTS,
                                 aCTSlotNum,
                                 aChainedKeyList,
                                 aChainedKeySize,
                                 aContext,
                                 sUnchainedKey,
                                 &sUnchainedKeySize,
                                 aUnchainedKeyCount )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( writeUnchainedKeysLog( aMtx,
                                     aNode,
                                     *aUnchainedKeyCount,
                                     sUnchainedKey,
                                     sUnchainedKeySize )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( iduMemMgr::free(sUnchainedKey) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void)iduMemMgr::free( sUnchainedKey );
    }

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Chained Key를 Unchained Key(Normal Key)로 변경한다.               
 * Unchained Key라는 것은 Key가 가리키는 트랜잭션 정보가 Key.CTS#가
 * 가리키는 CTS에 있음을 의미한다.
 *********************************************************************/
IDE_RC stndrRTree::writeUnchainedKeysLog( sdrMtx        * aMtx,
                                          sdpPhyPageHdr * aNode,
                                          UShort          aUnchainedKeyCount,
                                          UChar         * aUnchainedKey,
                                          UInt            aUnchainedKeySize )
{
    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)aNode,
                                         NULL,
                                         ID_SIZEOF(UShort) + aUnchainedKeySize,
                                         SDR_STNDR_MAKE_UNCHAINED_KEYS )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&aUnchainedKeyCount,
                                   ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)aUnchainedKey,
                                   aUnchainedKeySize )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Chained Key를 Unchained Key(Normal Key)로 변경한다.               
 * Unchained Key라는 것은 Key가 가리키는 트랜잭션 정보가 Key.CTS#가
 * 가리키는 CTS에 있음을 의미한다.
 *********************************************************************/
IDE_RC stndrRTree::makeUnchainedKeys( idvSQL        * aStatistics,
                                      sdpPhyPageHdr * aNode,
                                      sdnCTS        * aCTS,
                                      UChar           aCTSlotNum,
                                      UChar         * aChainedKeyList,
                                      UShort          aChainedKeySize,
                                      UChar         * aContext,
                                      UChar         * aUnchainedKey,
                                      UInt          * aUnchainedKeySize,
                                      UShort        * aUnchainedKeyCount )

{
    UShort                    sKeyCount;
    UShort                    i;
    UChar                     sChainedCCTS;
    UChar                     sChainedLCTS;
    UChar                   * sSlotDirPtr;
    UChar                   * sUnchainedKey;
    UShort                    sUnchainedKeyCount = 0;
    UInt                      sCursor = 0;
    stndrLKey               * sLeafKey;
    stndrCallbackContext    * sContext;
    scOffset                  sSlotOffset;

    IDE_ASSERT( aContext      != NULL );
    IDE_ASSERT( aUnchainedKey != NULL );

    *aUnchainedKeyCount = 0;
    *aUnchainedKeySize  = 0;

    sSlotDirPtr   = sdpPhyPage::getSlotDirStartPtr((UChar*)aNode);
    sKeyCount     = sdpSlotDirectory::getCount(sSlotDirPtr);
    sUnchainedKey = aUnchainedKey;

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           i, 
                                                           (UChar**)&sLeafKey )
                  != IDE_SUCCESS );

        if( (STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DEAD) ||
            (STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_STABLE) )
        {
            continue;
        }
        
        sChainedCCTS = SDN_CHAINED_NO;
        sChainedLCTS = SDN_CHAINED_NO;

        sContext = (stndrCallbackContext*)aContext;
        sContext->mLeafKey  = sLeafKey;

        /*
         * Chained Key라면
         */
        if( (STNDR_GET_CCTS_NO( sLeafKey ) == aCTSlotNum) &&
            (STNDR_GET_CHAINED_CCTS( sLeafKey ) == SDN_CHAINED_YES) )
        {
            /*
             * Chain될 당시에는 있었지만, Chaind Key가 해당 페이지내에
             * 존재하지 않을수도 있다.
             * 1. SMO에 의해서 Chaind Key가 이동한 경우
             * 2. Chaind Key가 DEAD상태 일때
             *    (LIMIT CTS만 Soft Key Stamping이 된 경우)
             */
            if( findChainedKey( aStatistics,
                                aCTS,
                                aChainedKeyList,
                                aChainedKeySize,
                                &sChainedCCTS,
                                &sChainedLCTS,
                                (UChar*)sContext ) == ID_TRUE )
            {
                idlOS::memcpy( sUnchainedKey + sCursor, &i, ID_SIZEOF(UShort) );
                sCursor += 2;
                *(sUnchainedKey + sCursor) = sChainedCCTS;
                sCursor += 1;
                *(sUnchainedKey + sCursor) = sChainedLCTS;
                sCursor += 1;

                if( sChainedCCTS == SDN_CHAINED_YES )
                {
                    sUnchainedKeyCount++;
                    STNDR_SET_CHAINED_CCTS( sLeafKey , SDN_CHAINED_NO );
                    IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr,
                                                          i,
                                                          &sSlotOffset )
                              != IDE_SUCCESS );
                    sdnIndexCTL::addRefKey( aNode,
                                            aCTSlotNum,
                                            sSlotOffset);
                }
                if( sChainedLCTS == SDN_CHAINED_YES )
                {
                    sUnchainedKeyCount++;
                    STNDR_SET_CHAINED_LCTS( sLeafKey , SDN_CHAINED_NO );
                    IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr,
                                                          i,
                                                          &sSlotOffset )
                              != IDE_SUCCESS );
                    sdnIndexCTL::addRefKey( aNode,
                                            aCTSlotNum,
                                            sSlotOffset );
                }
            }
            else
            {
                if( sdnIndexCTL::hasChainedCTS( aNode, aCTSlotNum ) != ID_TRUE )
                {
                    ideLog::log( IDE_SERVER_0,
                                 "CTS slot number : %u"
                                 ", Key slot number : %u\n",
                                 aCTSlotNum, i );
                    dumpIndexNode( aNode );
                    IDE_ASSERT( 0 );
                }
            }
        }
        else
        {
            if( (STNDR_GET_LCTS_NO( sLeafKey ) == aCTSlotNum) &&
                (STNDR_GET_CHAINED_LCTS( sLeafKey ) == SDN_CHAINED_YES) )
            {
                if( findChainedKey( aStatistics,
                                    aCTS,
                                    aChainedKeyList,
                                    aChainedKeySize,
                                    &sChainedCCTS,
                                    &sChainedLCTS,
                                    (UChar*)sContext ) == ID_TRUE )
                {
                    idlOS::memcpy( sUnchainedKey + sCursor, &i, ID_SIZEOF(UShort) );
                    sCursor += 2;
                    *(sUnchainedKey + sCursor) = SDN_CHAINED_NO;
                    sCursor += 1;
                    *(sUnchainedKey + sCursor) = sChainedLCTS;
                    sCursor += 1;

                    if( sChainedLCTS == SDN_CHAINED_YES )
                    {
                        sUnchainedKeyCount++;
                        STNDR_SET_CHAINED_LCTS( sLeafKey , SDN_CHAINED_NO );
                        IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr,
                                                              i,
                                                              &sSlotOffset )
                                 != IDE_SUCCESS );
                        sdnIndexCTL::addRefKey( aNode,
                                                aCTSlotNum,
                                                sSlotOffset );
                    }
                }
                else
                {
                    if( sdnIndexCTL::hasChainedCTS( aNode, aCTSlotNum ) != ID_TRUE )
                    {
                        ideLog::log( IDE_SERVER_0,
                                     "CTS slot number : %u"
                                     ", Key slot number : %u\n",
                                     aCTSlotNum, i );
                        dumpIndexNode( aNode );
                        IDE_ASSERT( 0 );
                    }
                }
            }
        }
    }

    *aUnchainedKeyCount = sUnchainedKeyCount;
    *aUnchainedKeySize  = sCursor;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}
#endif
#if 0
/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 주어진 Leaf Key가 sCTS의 Chained Key 인지를 확인한다.
 * sCTS의 mCommitSCN이 Leaf Key의 mCreateSCN 또는 mLimitSCN 과 동일하면
 * sCTS의 Chained Key 이다.
 *********************************************************************/
idBool stndrRTree::findChainedKey( idvSQL   * /* aStatistics */,
                                   sdnCTS   * sCTS,
                                   UChar    * /* aChainedKeyList */,
                                   UShort     /* aChainedKeySize */,
                                   UChar    * aChainedCCTS,
                                   UChar    * aChainedLCTS,
                                   UChar    * aContext )
{
    stndrCallbackContext    * sContext;
    stndrLKey               * sLeafKey;
    smSCN                     sCommitSCN;
    smSCN                     sKeyCSCN;
    smSCN                     sKeyLSCN;
    idBool                    sFound = ID_FALSE;
    
    
    IDE_DASSERT( aContext != NULL );
    
    sContext = (stndrCallbackContext*)aContext;

    IDE_DASSERT( sContext->mIndex != NULL );
    IDE_DASSERT( sContext->mLeafKey  != NULL );
    IDE_DASSERT( sContext->mStatistics != NULL );
    
    sLeafKey = sContext->mLeafKey;

    sCommitSCN = sCTS->mCommitSCN;
    STNDR_GET_CSCN( sLeafKey, &sKeyCSCN );
    STNDR_GET_LSCN( sLeafKey, &sKeyLSCN );

    if( SM_SCN_IS_EQ( &sKeyCSCN, &sCommitSCN ) )
    {
        if( aChainedCCTS != NULL )
        {
            *aChainedCCTS = SDN_CHAINED_YES;
            sFound = ID_TRUE;
        }
    }
    
    if( SM_SCN_IS_EQ( &sKeyLSCN, &sCommitSCN ) )
    {
        if( aChainedLCTS != NULL )
        {
            *aChainedLCTS = SDN_CHAINED_YES;
            sFound = ID_TRUE;
        }
    }

    return sFound;
}
#endif
/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * aIterator가 현재 가리키고 있는 Row에 대해서 XLock을 획득합니다.
 *
 * aProperties - [IN] Index Iterator
 *
 * Related Issue:
 *   BUG-19068: smiTableCursor가 현재가리키고 있는 Row에 대해서
 *              Lock을 잡을수 잇는 Interface가 필요합니다.
 *********************************************************************/
IDE_RC stndrRTree::lockRow( stndrIterator * aIterator )
{
    scSpaceID sTableTSID;

    sTableTSID = ((smcTableHeader*)aIterator->mTable)->mSpaceID;

    if( aIterator->mCurRowPtr == aIterator->mCacheFence )
    {
        ideLog::log( IDE_SERVER_0, "Index iterator info:\n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar *)aIterator,
                        ID_SIZEOF(stndrIterator) );
        IDE_ASSERT( 0 );
    }

    return sdnManager::lockRow( aIterator->mProperties,
                                aIterator->mTrans,
                                &(aIterator->mSCN),
                                &(aIterator->mInfinite),
                                sTableTSID,
                                SD_MAKE_SID_FROM_GRID(aIterator->mRowGRID),
                                aIterator->mStatement->isForbiddenToRetry() );
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 삭제할 키를 찾고, 찾은 노드에 키가 하나가 존재한다면 Empty Node에
 * 연결한다. 연결된 Node는 정말로 삭제 가능할때 Insert Transaction에 
 * 의해서 재사용된다. 
 * 삭제연산도 트랜잭션정보를 기록할 공간(CTS)가 필요하며, 이를 할당
 * 받을수 없는 경우에는 할당받을수 있을때까지 기다려야 한다.
 *********************************************************************/
IDE_RC stndrRTree::deleteKey( idvSQL        * aStatistics,
                              void          * aTrans,
                              void          * aIndex,
                              SChar         * aKeyValue,
                              smiIterator   * aIterator,
                              sdSID           aRowSID )
{
    sdrMtx            sMtx;
    stndrPathStack    sStack;
    sdpPhyPageHdr   * sLeafNode;
    SShort            sLeafKeySeq;
    sdpPhyPageHdr   * sTargetNode;
    SShort            sTargetKeySeq;
    scPageID          sEmptyNodePID[2] = { SD_NULL_PID, SD_NULL_PID };
    UInt              sEmptyNodeCount = 0;
    stndrKeyInfo      sKeyInfo;
    idBool            sMtxStart = ID_FALSE;
    smnIndexHeader  * sIndex;
    stndrHeader     * sHeader;
    stndrNodeHdr    * sNodeHdr;
    smLSN             sNTA;
    idvWeArgs         sWeArgs;
    idBool            sIsSuccess;
    idBool            sIsRetry = ID_FALSE;
    UInt              sAllocPageCount = 0;
    stndrStatistic    sIndexStat;
    UShort            sKeyValueLength;
    idBool            sIsIndexable;
    idBool            sIsPessimistic = ID_FALSE;


    isIndexableRow( aIndex, aKeyValue, &sIsIndexable );
    
    IDE_TEST_RAISE( sIsIndexable == ID_FALSE, RETURN_NO_DELETE );
    
    sIndex  = (smnIndexHeader*)aIndex;
    sHeader = (stndrHeader*)sIndex->mHeader;

    // To fix BUG-17939
    IDE_DASSERT( sHeader->mSdnHeader.mIsConsistent == ID_TRUE );

    sKeyInfo.mKeyValue   = (UChar*)&(((stdGeometryHeader*)aKeyValue)->mMbr);
    sKeyInfo.mRowPID     = SD_MAKE_PID(aRowSID);
    sKeyInfo.mRowSlotNum = SD_MAKE_SLOTNUM(aRowSID);

    IDL_MEM_BARRIER;
    
    sNTA = smxTrans::getTransLstUndoNxtLSN( aTrans );

    idlOS::memset( &sIndexStat, 0x00, sizeof(sIndexStat) );

  retry:

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType ) != IDE_SUCCESS );
    sMtxStart = ID_TRUE;

    sIsRetry = ID_FALSE;

    // init stack
    // BUG-29039 codesonar ( Uninitialized Variable )
    sStack.mDepth = -1;

    sIsPessimistic = ID_FALSE;

  retraverse:
    
    // traverse
    sIsSuccess = ID_TRUE;
    
    IDE_TEST( traverse( aStatistics,
                        sHeader,
                        &sMtx,
                        &aIterator->infinite,
                        NULL, /* aFstDiskViewSCN */
                        &sKeyInfo,
                        SD_NULL_PID,
                        STNDR_TRAVERSE_DELETE_KEY,
                        &sStack,
                        &sIndexStat,
                        &sLeafNode,
                        &sLeafKeySeq ) != IDE_SUCCESS );

    if( sLeafNode == NULL )
    {
        ideLog::log( IDE_SERVER_0, "\
=============================================\n\
              NAME : %s                      \n\
              ID   : %u                      \n\
                                             \n\
stndrRTree::deleteKey, Traverse error        \n\
ROW_PID:%u, ROW_SLOTNUM: %u                  \n",
                 sIndex->mName,
                 sIndex->mId,
                 sKeyInfo.mRowPID,
                 sKeyInfo.mRowSlotNum );
        
        ideLog::log( IDE_SERVER_0,
                     "Leaf key sequence : %d\n",
                     sLeafKeySeq );
        
        ideLog::log( IDE_SERVER_0, "Index stack dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sStack, ID_SIZEOF(stndrPathStack) );
        
        ideLog::log( IDE_SERVER_0, "Index header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sHeader, ID_SIZEOF(stndrHeader) );
        
        ideLog::log( IDE_SERVER_0, "Index run-time header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)sIndex, ID_SIZEOF(smnIndexHeader) );
        
        ideLog::log( IDE_SERVER_0, "Key info dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sKeyInfo, ID_SIZEOF(stndrKeyInfo) );
        
        IDE_ASSERT( 0 );
    }

    IDE_TEST( deleteKeyFromLeafNode( aStatistics,
                                     &sIndexStat,
                                     &sMtx,
                                     sHeader,
                                     &aIterator->infinite,
                                     sLeafNode,
                                     &sLeafKeySeq,
                                     &sIsSuccess )
              != IDE_SUCCESS );
    
    if( sIsSuccess == ID_FALSE )
    {
        if( sIsPessimistic == ID_FALSE )
        {
            sIsPessimistic = ID_TRUE;

            sMtxStart = ID_FALSE;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

            IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                           &sMtx,
                                           aTrans,
                                           SDR_MTX_LOGGING,
                                           ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                           gMtxDLogType )
                      != IDE_SUCCESS );
            sMtxStart = ID_TRUE;
            
            IDV_WEARGS_SET(
                &sWeArgs,
                IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
                0,   // WaitParam1
                0,   // WaitParam2
                0 ); // WaitParam3

            IDE_TEST( sHeader->mSdnHeader.mLatch.lockWrite( aStatistics,
                                                            &sWeArgs )
                      != IDE_SUCCESS);

            IDE_TEST( sdrMiniTrans::push(
                          &sMtx,
                          (void*)&sHeader->mSdnHeader.mLatch,
                          SDR_MTX_LATCH_X)
                      != IDE_SUCCESS );

            sStack.mDepth = -1;
            goto retraverse;
        }
        else
        {
            sKeyValueLength = getKeyValueLength();

            IDE_TEST( splitLeafNode( aStatistics,
                                     &sIndexStat,
                                     &sMtx,
                                     &sMtxStart,
                                     sHeader,
                                     &aIterator->infinite,
                                     &sStack,
                                     sLeafNode,
                                     &sKeyInfo,
                                     sKeyValueLength,
                                     sLeafKeySeq,
                                     ID_FALSE,
                                     smuProperty::getRTreeSplitMode(),
                                     &sTargetNode,
                                     &sTargetKeySeq,
                                     &sAllocPageCount,
                                     &sIsRetry )
                      != IDE_SUCCESS );

            if( sIsRetry == ID_TRUE )
            {
                // key propagation을 위해 latch를 잡는 중에 root node가 변경된 경우
                sMtxStart = ID_FALSE;
                IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
                sIndexStat.mOpRetryCount++;
                goto retry;
            }

            IDE_TEST( deleteKeyFromLeafNode( aStatistics,
                                             &sIndexStat,
                                             &sMtx,
                                             sHeader,
                                             &aIterator->infinite,
                                             sTargetNode,
                                             &sTargetKeySeq,
                                             &sIsSuccess )
                      != IDE_SUCCESS );

            if( sIsSuccess != ID_TRUE )
            {
                ideLog::log( IDE_SERVER_0,
                             "Target Key sequence : %d\n",
                             sTargetKeySeq );

                ideLog::log( IDE_SERVER_0, "Index header dump:\n" );
                ideLog::logMem( IDE_SERVER_0, (UChar *)sHeader, ID_SIZEOF(stndrHeader) );
                
                dumpIndexNode( sTargetNode );
                IDE_ASSERT( 0 );
            }

            findEmptyNodes( &sMtx,
                            sHeader,
                            sLeafNode,
                            sEmptyNodePID,
                            &sEmptyNodeCount );
                
            sIndexStat.mNodeSplitCount++;
        }
    }
    else
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sLeafNode );

        if( sNodeHdr->mUnlimitedKeyCount == 0 )
        {
            sEmptyNodePID[0] = sLeafNode->mPageID;
            sEmptyNodeCount = 1;
        }
    }

    sdrMiniTrans::setRefNTA( &sMtx,
                             sHeader->mSdnHeader.mIndexTSID,
                             SDR_OP_STNDR_DELETE_KEY_WITH_NTA,
                             &sNTA );

    sMtxStart = ID_FALSE;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    
    if( sAllocPageCount > 0 )
    {
        IDE_TEST( nodeAging( aStatistics,
                             aTrans,
                             &sIndexStat,
                             sHeader,
                             sAllocPageCount )
                  != IDE_SUCCESS );
    }
    
    if( sEmptyNodeCount > 0 )
    {
        IDE_TEST( linkEmptyNodes( aStatistics,
                                  aTrans,
                                  sHeader,
                                  &sIndexStat,
                                  sEmptyNodePID )
                  != IDE_SUCCESS );
    }

    STNDR_ADD_STATISTIC( &sHeader->mDMLStat, &sIndexStat );

    IDE_EXCEPTION_CONT( RETURN_NO_DELETE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMtxStart == ID_TRUE )
    {
        sMtxStart = ID_FALSE;
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * InsertKey가 Rollback되는 경우에 로그에 기반하여 호출된다.         
 * 롤백할 키를 찾고, 찾은 노드에 키가 하나가 존재한다면 Empty Node에 
 * 연결한다. 연결된 Node는 정말로 삭제가능할때 Insert Transaction에  
 * 의해서 재사용된다.                                                
 * 해당연산은 트랜잭션정보를 할당할 필요가 없다. 즉, Rollback전에   
 * 이미 트랜잭션이 할당 받은 공간이 사용한다.                        
 *********************************************************************/
IDE_RC stndrRTree::insertKeyRollback( idvSQL    * aStatistics,
                                      void      * aMtx,
                                      void      * aIndex,
                                      SChar     * aKeyValue,
                                      sdSID       aRowSID,
                                      UChar     * /*aRollbackContext*/,
                                      idBool      /*aIsDupKey*/ )
{
    sdrMtx                  * sMtx;
    stndrPathStack            sStack;
    stndrHeader             * sHeader;
    stndrKeyInfo              sKeyInfo;
    stndrLKey               * sLeafKey;
    stndrStatistic            sIndexStat;
    sdpPhyPageHdr           * sLeafNode;
    stndrNodeHdr            * sNodeHdr;
    UShort                    sUnlimitedKeyCount;
    SShort                    sLeafKeySeq;
    UShort                    sTotalDeadKeySize = 0;
    UShort                    sKeyOffset;
    UChar                   * sSlotDirPtr;
    UChar                     sCurCreateCTS;
    idBool                    sIsSuccess = ID_TRUE;
    UShort                    sTotalTBKCount = 0;

    
    idlOS::memset( &sIndexStat, 0x00, sizeof(sIndexStat) );

    sMtx = (sdrMtx*)aMtx;
    sHeader = (stndrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    IDE_TEST_RAISE( sHeader->mSdnHeader.mIsConsistent == ID_FALSE,
                    SKIP_UNDO );
    
    sKeyInfo.mKeyValue = (UChar*)aKeyValue;
    sKeyInfo.mRowPID = SD_MAKE_PID( aRowSID );
    sKeyInfo.mRowSlotNum = SD_MAKE_SLOTNUM( aRowSID );

  retraverse:

    // init stack
    sStack.mDepth = -1;

    IDE_TEST( traverse( aStatistics,
                        sHeader,
                        sMtx,
                        NULL, /* aCursorSCN */
                        NULL, /* aFstDiskViewSCN */
                        &sKeyInfo,
                        SD_NULL_PID,
                        STNDR_TRAVERSE_INSERTKEY_ROLLBACK,
                        &sStack,
                        &sIndexStat,
                        &sLeafNode,
                        &sLeafKeySeq )
              != IDE_SUCCESS );

    if( sLeafNode == NULL )
    {
        ideLog::log( IDE_SERVER_0, "\
============================================= \n\
              NAME : %s                       \n\
              ID   : %u                       \n\
                                              \n\
The root node is not exist.                   \n\
stndrRTree::insertKeyRollback, Traverse error \n\
ROW_PID:%u, ROW_SLOTNUM: %u                   \n",
                 ((smnIndexHeader*)aIndex)->mName,
                 ((smnIndexHeader*)aIndex)->mId,
                 sKeyInfo.mRowPID,
                 sKeyInfo.mRowSlotNum );
        
        ideLog::log( IDE_SERVER_0,
                     "Leaf Key sequence : %d\n",
                     sLeafKeySeq );
        
        ideLog::log( IDE_SERVER_0, "Index stack dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sStack, ID_SIZEOF(stndrPathStack) );
        
        ideLog::log( IDE_SERVER_0, "Index header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sHeader, ID_SIZEOF(stndrHeader) );
        
        ideLog::log( IDE_SERVER_0, "Index run-time header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)aIndex, ID_SIZEOF(smnIndexHeader) );
        
        ideLog::log( IDE_SERVER_0, "Key info dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sKeyInfo, ID_SIZEOF(stndrKeyInfo) );
        IDE_ASSERT( 0 );
    }

    sNodeHdr    = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)sLeafNode);
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sLeafNode);
    IDE_TEST ( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                        sLeafKeySeq,
                                                        (UChar**)&sLeafKey )
               != IDE_SUCCESS );
    IDE_TEST ( sdpSlotDirectory::getValue( sSlotDirPtr, 
                                           sLeafKeySeq,
                                           &sKeyOffset )
               != IDE_SUCCESS );

    if( sNodeHdr->mUnlimitedKeyCount == 1 )
    {
        IDE_TEST( linkEmptyNode( aStatistics,
                                 &sIndexStat,
                                 sHeader,
                                 sMtx,
                                 sLeafNode,
                                 &sIsSuccess )
                  != IDE_SUCCESS );
        
        if( sIsSuccess == ID_FALSE )
        {
            IDE_TEST( sdrMiniTrans::commit( sMtx ) != IDE_SUCCESS );
            
            IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                           sMtx,
                                           sdrMiniTrans::getTrans(sMtx),
                                           SDR_MTX_LOGGING,
                                           ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                           gMtxDLogType ) != IDE_SUCCESS );
            
            goto retraverse;
        }
    }
    
    sCurCreateCTS = STNDR_GET_CCTS_NO( sLeafKey );
    
    sTotalDeadKeySize = sNodeHdr->mTotalDeadKeySize;
    sTotalDeadKeySize += getKeyLength( (UChar*)sLeafKey ,
                                       ID_TRUE /* aIsLeaf */ );
    sTotalDeadKeySize += ID_SIZEOF( sdpSlotEntry );

    STNDR_SET_CCTS_NO( sLeafKey , SDN_CTS_INFINITE );
    STNDR_SET_STATE( sLeafKey , STNDR_KEY_DEAD );

    IDE_TEST(sdrMiniTrans::writeNBytes( sMtx,
                                        (UChar*)&sNodeHdr->mTotalDeadKeySize,
                                        (void*)&sTotalDeadKeySize,
                                        ID_SIZEOF(sNodeHdr->mTotalDeadKeySize) )
             != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)sLeafKey->mTxInfo,
                                         (void*)sLeafKey->mTxInfo,
                                         ID_SIZEOF(UChar)*2 )
              != IDE_SUCCESS );

    sUnlimitedKeyCount = sNodeHdr->mUnlimitedKeyCount - 1;
    IDE_DASSERT( sUnlimitedKeyCount < sdpSlotDirectory::getCount( sSlotDirPtr ) );
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&(sNodeHdr->mUnlimitedKeyCount),
                                         (void*)&sUnlimitedKeyCount,
                                         ID_SIZEOF(UShort) )
              != IDE_SUCCESS );
    
    if( (STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DEAD) &&
        (STNDR_GET_TB_TYPE( sLeafKey ) == STNDR_KEY_TB_KEY) )
    {
        sTotalTBKCount = sNodeHdr->mTBKCount - 1;
        IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                             (UChar*)&(sNodeHdr->mTBKCount),
                                             (void*)&sTotalTBKCount,
                                             ID_SIZEOF(UShort) )
                  != IDE_SUCCESS );
    }
    
    if( SDN_IS_VALID_CTS( sCurCreateCTS ) )
    {
        IDE_TEST( sdnIndexCTL::unbindCTS( sMtx,
                                          sLeafNode,
                                          sCurCreateCTS,
                                          sKeyOffset )
                  != IDE_SUCCESS );
    }
    
    STNDR_ADD_STATISTIC( &sHeader->mDMLStat, &sIndexStat );
   
    IDE_EXCEPTION_CONT( SKIP_UNDO );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * DeleteKey가 Rollback되는 경우에 로그에 기반하여 호출된다.         
 * 롤백할 키를 찾고, 찾은 키의 상태를 STABLE이나 UNSTABLE로 변경한다.
 * 해당연산은 트랜잭션정보를 할당할 필요가 없다. 즉, Rollback전에    
 * 이미 트랜잭션이 할당 받은 공간이 사용한다. 
 *********************************************************************/
IDE_RC stndrRTree::deleteKeyRollback( idvSQL    * aStatistics,
                                      void      * aMtx,
                                      void      * aIndex,
                                      SChar     * aKeyValue,
                                      sdSID       aRowSID,
                                      UChar     * aRollbackContext )
{
    stndrPathStack            sStack;
    stndrHeader             * sHeader;
    stndrKeyInfo              sKeyInfo;
    stndrLKey               * sLeafKey;
    stndrStatistic            sIndexStat;
    sdpPhyPageHdr           * sLeafNode;
    SShort                    sLeafKeySeq;
    stndrNodeHdr            * sNodeHdr;
    UShort                    sUnlimitedKeyCount;
    UChar                     sLimitCTS;
    smSCN                     sCommitSCN;
    smSCN                     sCursorSCN;
    smSCN                     sFstDiskViewSCN;
    UShort                    sKeyOffset;
    UChar                   * sSlotDirPtr;
    sdrMtx                  * sMtx;

    idlOS::memset( &sIndexStat, 0x00, sizeof(sIndexStat) );

    sMtx = (sdrMtx*)aMtx;
    sHeader = (stndrHeader*)((smnIndexHeader*)aIndex)->mHeader;

    IDE_TEST_RAISE( sHeader->mSdnHeader.mIsConsistent == ID_FALSE,
                    SKIP_UNDO );

    SM_SET_SCN( &sCursorSCN,
                &(((stndrRollbackContext*)aRollbackContext)->mLimitSCN) );

    SM_SET_SCN( &sFstDiskViewSCN,
                &(((stndrRollbackContext*)aRollbackContext)->mFstDiskViewSCN) );
    
    sKeyInfo.mKeyValue   = (UChar*)aKeyValue;
    sKeyInfo.mRowPID     = SD_MAKE_PID( aRowSID );
    sKeyInfo.mRowSlotNum = SD_MAKE_SLOTNUM( aRowSID );

    // init stack
    sStack.mDepth = -1;

    IDE_TEST( traverse( aStatistics,
                        sHeader,
                        sMtx,
                        &sCursorSCN,
                        &sFstDiskViewSCN,
                        &sKeyInfo,
                        SD_NULL_PID,
                        STNDR_TRAVERSE_DELETEKEY_ROLLBACK,
                        &sStack,
                        &sIndexStat,
                        &sLeafNode,
                        &sLeafKeySeq ) != IDE_SUCCESS );

    if( sLeafNode == NULL )
    {
        ideLog::log( IDE_SERVER_0, "\
============================================= \n\
              NAME : %s                       \n\
              ID   : %u                       \n\
                                              \n\
stndrRTree::deleteKeyRollback, Traverse error \n\
ROW_PID:%u, ROW_SLOTNUM: %u                   \n",
                 ((smnIndexHeader*)aIndex)->mName,
                 ((smnIndexHeader*)aIndex)->mId,
                 sKeyInfo.mRowPID,
                 sKeyInfo.mRowSlotNum );
        
        ideLog::log( IDE_SERVER_0,
                     "Leaf Key sequence : %d\n",
                     sLeafKeySeq );
        
        ideLog::log( IDE_SERVER_0, "Index stack dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sStack, ID_SIZEOF(stndrStack) );
        
        ideLog::log( IDE_SERVER_0, "Index header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sHeader, ID_SIZEOF(stndrHeader) );
        
        ideLog::log( IDE_SERVER_0, "Index run-time header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)aIndex, ID_SIZEOF(smnIndexHeader) );
        
        ideLog::log( IDE_SERVER_0, "Key info dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)&sKeyInfo, ID_SIZEOF(stndrKeyInfo) );
        
        dumpIndexNode( sLeafNode );
        IDE_ASSERT( 0 );
    }

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sLeafNode );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)sLeafNode);
    IDE_TEST ( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                        sLeafKeySeq,
                                                        (UChar**)&sLeafKey )
               != IDE_SUCCESS );
    IDE_TEST ( sdpSlotDirectory::getValue( sSlotDirPtr, 
                                           sLeafKeySeq,
                                           &sKeyOffset )
               != IDE_SUCCESS );
    sLimitCTS = STNDR_GET_LCTS_NO( sLeafKey  );

    sUnlimitedKeyCount = sNodeHdr->mUnlimitedKeyCount + 1;
    IDE_DASSERT( sUnlimitedKeyCount <= sdpSlotDirectory::getCount( sSlotDirPtr ) );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&(sNodeHdr->mUnlimitedKeyCount),
                                         (void*)&sUnlimitedKeyCount,
                                         ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    if( SDN_IS_VALID_CTS( sLimitCTS ) )
    {
        IDE_TEST( sdnIndexCTL::unbindCTS( sMtx,
                                          sLeafNode,
                                          sLimitCTS,
                                          sKeyOffset )
                  != IDE_SUCCESS );
    }

    STNDR_SET_LCTS_NO( sLeafKey , SDN_CTS_INFINITE );
    
    if( STNDR_GET_CCTS_NO( sLeafKey ) == SDN_CTS_INFINITE )
    {
        STNDR_SET_STATE( sLeafKey , STNDR_KEY_STABLE );
    }
    else
    {
        if( STNDR_GET_CCTS_NO( sLeafKey ) == SDN_CTS_IN_KEY )
        {
            IDE_TEST( getCommitSCN( aStatistics,
                                    NULL,        /* aTrans */
                                    sLeafNode,
                                    (stndrLKeyEx*)sLeafKey ,
                                    ID_FALSE,    /* aIsLimitSCN */
                                    SM_SCN_INIT, /* aStmtViewSCN */
                                    &sCommitSCN )
                      != IDE_SUCCESS );

            if( isAgableTBK( sCommitSCN ) == ID_TRUE )
            {
                STNDR_SET_STATE( sLeafKey , STNDR_KEY_STABLE );
            }
            else
            {
                STNDR_SET_STATE( sLeafKey , STNDR_KEY_UNSTABLE );
            }
        }
        else
        {
            STNDR_SET_STATE( sLeafKey , STNDR_KEY_UNSTABLE );
        }
    }

    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)sLeafKey->mTxInfo,
                                         (void*)sLeafKey->mTxInfo,
                                         ID_SIZEOF(UChar)*2 )
              != IDE_SUCCESS );

    STNDR_ADD_STATISTIC( &sHeader->mDMLStat, &sIndexStat );

    IDE_EXCEPTION_CONT( SKIP_UNDO );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 사용자가 강제적으로 aging시킬때 호출되는 함수이다.                
 * 많은 로그를 남기지 않기 위해서 Compaction은 하지 않는다.          
 * 1. Leaf Node를 찾는다.
 * 2. 모든 Leaf Node를 탐색하면서 DelayedStamping을 수행한다.
 * 3. Node Aging을 수행한다.                                         
 *********************************************************************/
IDE_RC stndrRTree::aging( idvSQL            * aStatistics,
                          void              * aTrans,
                          smcTableHeader    * /* aHeader */,
                          smnIndexHeader    * aIndex )
{
    stndrHeader          * sIdxHdr = NULL;
    idBool                 sIsSuccess;
    stndrNodeHdr         * sNodeHdr = NULL;
    sdnCTS               * sCTS;
    sdnCTL               * sCTL;
    UInt                   i;
    smSCN                  sCommitSCN;
    UInt                   sState = 0;
    sdpSegCCache         * sSegCache;
    sdpSegHandle         * sSegHandle;
    sdpSegMgmtOp         * sSegMgmtOp;
    sdbMPRMgr              sMPRMgr;
    scPageID               sCurPID;
    sdpPhyPageHdr        * sPage;
    ULong                  sUsedSegSizeByBytes = 0;
    UInt                   sMetaSize = 0;
    UInt                   sFreeSize = 0;
    UInt                   sUsedSize = 0;
    sdpSegInfo             sSegInfo;

    
    IDE_DASSERT( aIndex  != NULL );
    
    sIdxHdr = (stndrHeader*)(aIndex->mHeader);

    IDE_TEST( smLayerCallback::rebuildMinViewSCN( aStatistics ) 
              != IDE_SUCCESS );

    sSegHandle = sdpSegDescMgr::getSegHandle(&(sIdxHdr->mSdnHeader.mSegmentDesc));
    sSegCache  = (sdpSegCCache*)sSegHandle->mCache;
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp(&(sIdxHdr->mSdnHeader.mSegmentDesc));

    IDE_TEST( sSegMgmtOp->mGetSegInfo(
                             NULL,
                             sIdxHdr->mSdnHeader.mIndexTSID,
                             sdpSegDescMgr::getSegPID( &(sIdxHdr->mSdnHeader.mSegmentDesc) ),
                             NULL, /* aTableHeader */
                             &sSegInfo )
               != IDE_SUCCESS );

    IDE_TEST( sMPRMgr.initialize(
                  aStatistics,
                  sIdxHdr->mSdnHeader.mIndexTSID,
                  sdpSegDescMgr::getSegHandle(&(sIdxHdr->mSdnHeader.mSegmentDesc)),
                  NULL ) /*aFilter*/
              != IDE_SUCCESS );
    sState = 1;

    sCurPID = SM_NULL_PID;

    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    while( 1 )
    {
        IDE_TEST( sMPRMgr.getNxtPageID(NULL, /*aData*/
                                       &sCurPID)
                  != IDE_SUCCESS );

        if( sCurPID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                         sIdxHdr->mSdnHeader.mIndexTSID,
                                         sCurPID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_MULTI_PAGE_READ,
                                         (UChar**)&sPage,
                                         &sIsSuccess )
                  != IDE_SUCCESS );
        sState = 2;

        IDE_DASSERT( (sPage->mPageType == SDP_PAGE_FORMAT) ||
                     (sPage->mPageType == SDP_PAGE_INDEX_RTREE) ||
                     (sPage->mPageType == SDP_PAGE_INDEX_META_RTREE) );

        if( (sPage->mPageType != SDP_PAGE_INDEX_RTREE) ||
            (sSegMgmtOp->mIsFreePage((UChar*)sPage) == ID_TRUE) )
        {
            sState = 1;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                (UChar*)sPage )
                      != IDE_SUCCESS );
            continue;
        } 
        else 
        {   
            /* nothing to do */
        }

        sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr(
                                                                (UChar*)sPage);
        
        if( sNodeHdr->mState != STNDR_IN_FREE_LIST )
        {
            /*
             * 1. Delayed CTL Stamping
             */
            if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
            {
                sCTL = sdnIndexCTL::getCTL( sPage );

                for( i = 0; i < sdnIndexCTL::getCount( sCTL ); i++ )
                {
                    sCTS = sdnIndexCTL::getCTS( sCTL, i );

                    if( sCTS->mState == SDN_CTS_UNCOMMITTED )
                    {
                        IDE_TEST( sdnIndexCTL::delayedStamping( aStatistics,
                                                                aTrans, 
                                                                sCTS,
                                                                SDB_MULTI_PAGE_READ,
                                                                SM_SCN_INIT, /* aStmtViewSCN */ 
                                                                &sCommitSCN,
                                                                &sIsSuccess )
                            
                                  != IDE_SUCCESS );
                    }
                }
            }
            else
            {
                /* nothing to do */
            }
            sMetaSize = ID_SIZEOF( sdpPhyPageHdr )
                        + idlOS::align8(ID_SIZEOF(stndrNodeHdr))
                        + sdpPhyPage::getSizeOfCTL( sPage )
                        + ID_SIZEOF( sdpPageFooter )
                        + ID_SIZEOF( sdpSlotDirHdr );

            sFreeSize = sNodeHdr->mTotalDeadKeySize + sPage->mTotalFreeSize;
            sUsedSize = SD_PAGE_SIZE - (sMetaSize + sFreeSize );
            sUsedSegSizeByBytes += sUsedSize;
            IDE_DASSERT ( sMetaSize + sFreeSize + sUsedSize == SD_PAGE_SIZE );
        }
        else
        {
            /* nothing to do */
        }

        sState = 1;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             (UChar*)sPage )
                  != IDE_SUCCESS );
    }

    /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
    sSegCache->mFreeSegSizeByBytes = (sSegInfo.mFmtPageCnt * SD_PAGE_SIZE) - sUsedSegSizeByBytes;

    sState = 0;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );

    // 2. Node Aging
    IDE_TEST( nodeAging( aStatistics,
                         aTrans,
                         &(sIdxHdr->mDMLStat),
                         sIdxHdr,
                         ID_UINT_MAX )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   (UChar*)sPage )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sMPRMgr.destroy() == IDE_SUCCESS );
        default:
            break;
    }
    
    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::NA
 * ------------------------------------------------------------------*
 * default function for invalid traversing protocol
 *********************************************************************/
IDE_RC stndrRTree::NA( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;
}
/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::getPosition
 * ------------------------------------------------------------------*
 * 현재 Iterator의 위치를 저장한다.
 *********************************************************************/
IDE_RC stndrRTree::getPositionNA( stndrIterator     * /*aIterator*/,
                                  smiCursorPosInfo  * /*aPosInfo*/ )
{
    /* Do nothing */
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::setPosition
 * ------------------------------------------------------------------*
 * 이전에 저장된 Iterator의 위치로 다시 복귀시킨다.
 *********************************************************************/
IDE_RC stndrRTree::setPositionNA( stndrIterator     * /*aIterator*/,
                                  smiCursorPosInfo  * /*aPosInfo*/ )
{
    /* Do nothing */
    return IDE_SUCCESS;
}

IDE_RC stndrRTree::allocIterator( void ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

IDE_RC stndrRTree::freeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 인덱스 Runtime Header의 SmoNo를 얻는다. SmoNo는 ULong 타입이기 때문에
 * 32비트 머신의 경우 partial read의 가능성이 있다. 따라서 peterson 알고리즘
 * 으로 SmoNo을 얻어 가도록한다.
 *
 * !!CAUTION!! : Runtime Header에 직접 접근하여 SmoNo를 획득해서는 안된다.
 * 반드시 본 함수를 이용하여 peterson 알고리즘 방식으로 SmoNo를 획득하도록
 * 한다.
 *********************************************************************/
void stndrRTree::getSmoNo( void * aIndex, ULong * aSmoNo )
{
    stndrHeader * sIndex = (stndrHeader*)aIndex;
#ifndef COMPILE_64BIT
    UInt          sAtomicA;
    UInt          sAtomicB;
#endif

    IDE_ASSERT( aIndex != NULL );
    
#ifndef COMPILE_64BIT
    while( 1 )
    {
        ID_SERIAL_BEGIN( sAtomicA = sIndex->mSmoNoAtomicA );
        
        ID_SERIAL_EXEC( *aSmoNo = sIndex->mSdnHeader.mSmoNo, 1 );

        ID_SERIAL_END( sAtomicB = sIndex->mSmoNoAtomicB );

        if( sAtomicA == sAtomicB )
        {
            break;
        }

        idlOS::thr_yield();
    }
#else
    *aSmoNo = sIndex->mSdnHeader.mSmoNo;
#endif

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 인덱스 Runtime Header의 SmoNo를 1 증가 시킨다. 본 함수는 동시에 호출될 수
 * 있으므로 Mutex를 이용하여 동기화하며, peterson 알고리즘 이용하여 32비트
 * 머신에서 partial read 하지 않도록 막는다.
 *********************************************************************/
IDE_RC stndrRTree::increaseSmoNo( idvSQL        * aStatistics,
                                  stndrHeader   * aIndex,
                                  ULong         * aSmoNo )
{
    UInt sSerialValue = 0;
    
    IDE_TEST( aIndex->mSmoNoMutex.lock(aStatistics) != IDE_SUCCESS );
    ID_SERIAL_BEGIN( aIndex->mSmoNoAtomicB++ );

    ID_SERIAL_EXEC( aIndex->mSdnHeader.mSmoNo++, ++sSerialValue );
    ID_SERIAL_EXEC( *aSmoNo = aIndex->mSdnHeader.mSmoNo, ++sSerialValue );
    
    ID_SERIAL_END( aIndex->mSmoNoAtomicA++ );
    IDE_TEST( aIndex->mSmoNoMutex.unlock() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Virtual Root Node의 얻는다. Virtual Root Node는 ChildPID, ChildSmoNo
 * 를 가지고 있으며, Virtual Root Node의 Child는 실제 Root Node를 가리킨다.
 *********************************************************************/
void stndrRTree::getVirtualRootNode( stndrHeader            * aIndex,
                                     stndrVirtualRootNode   * aVRootNode )
{
    UInt    sSerialValue = 0;
    UInt    sAtomicA = 0;
    UInt    sAtomicB = 0;

    IDE_ASSERT( aIndex != NULL );
    IDE_ASSERT( aVRootNode != NULL );
    
    while( 1 )
    {
        sSerialValue = 0;
        
        ID_SERIAL_BEGIN( sAtomicA = aIndex->mVirtualRootNodeAtomicA );
        
        ID_SERIAL_EXEC( aVRootNode->mChildSmoNo = aIndex->mVirtualRootNode.mChildSmoNo,
                        ++sSerialValue );


        ID_SERIAL_EXEC( aVRootNode->mChildPID = aIndex->mVirtualRootNode.mChildPID,
                        ++sSerialValue );

        ID_SERIAL_END( sAtomicB = aIndex->mVirtualRootNodeAtomicB );

        if( sAtomicA == sAtomicB )
        {
            break;
        }

        idlOS::thr_yield();
    }

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Virtual Root Node의 설정한다.
 * !!!: 본 함수는 동시에 한 쓰레드만이 호출해야 한다. 만일 그렇지 않다면
 *      Mutex로 보호되어야 한다.
 *********************************************************************/
void stndrRTree::setVirtualRootNode( stndrHeader  * aIndex,
                                     scPageID       aRootNode,
                                     ULong          aSmoNo )
{
    UInt    sSerialValue = 0;

    ID_SERIAL_BEGIN( aIndex->mVirtualRootNodeAtomicB++ );

    ID_SERIAL_EXEC( aIndex->mVirtualRootNode.mChildPID = aRootNode,
                    ++sSerialValue );
    
    ID_SERIAL_EXEC( aIndex->mVirtualRootNode.mChildSmoNo = aSmoNo,
                    ++sSerialValue );

    ID_SERIAL_END( aIndex->mVirtualRootNodeAtomicA++ );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * row를 fetch 하면서 Index의 KeyValue를 생성한다.
 * Disk R-Tree의 경우 row의 stdGeometryHeader 만을 읽어서 구성한다.
 * insertKey, deleteKey에서 stdGeometryHeader의 mMbr 멤버를 KeyValue로
 * 사용한다.
 *********************************************************************/
IDE_RC stndrRTree::makeKeyValueFromRow(
                            idvSQL                  * aStatistics,
                            sdrMtx                  * aMtx,
                            sdrSavePoint            * aSP,
                            void                    * aTrans,
                            void                    * aTableHeader,
                            const smnIndexHeader    * aIndex,
                            const UChar             * aRow,
                            sdbPageReadMode           aPageReadMode,
                            scSpaceID                 aTableSpaceID,
                            smFetchVersion            aFetchVersion,
                            sdSID                     aTSSlotSID,
                            const smSCN             * aSCN,
                            const smSCN             * aInfiniteSCN,
                            UChar                   * aDestBuf,
                            idBool                  * aIsRowDeleted,
                            idBool                  * aIsPageLatchReleased)
{
    stndrHeader         * sIndexHeader;
    sdcIndexInfo4Fetch    sIndexInfo4Fetch;
    ULong                 sValueBuffer[STNDR_MAX_KEY_BUFFER_SIZE/ID_SIZEOF(ULong)];

    IDE_DASSERT( aTrans           != NULL );
    IDE_DASSERT( aTableHeader     != NULL );
    IDE_DASSERT( aIndex           != NULL );
    IDE_DASSERT( aRow             != NULL );
    IDE_DASSERT( aDestBuf         != NULL );

    if( aIndex->mType != SMI_ADDITIONAL_RTREE_INDEXTYPE_ID )
    {
        ideLog::log( IDE_SERVER_0, "Index Header :\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)aIndex, ID_SIZEOF(smnIndexHeader) );
        IDE_ASSERT( 0 );
    }

    sIndexHeader = ((stndrHeader*)aIndex->mHeader);

    /* sdcRow::fetch() 함수로 넘겨줄 row info 및 callback 함수 설정 */
    sIndexInfo4Fetch.mTableHeader           = aTableHeader;
    sIndexInfo4Fetch.mCallbackFunc4Index    = makeSmiValueListInFetch;
    sIndexInfo4Fetch.mBuffer                = (UChar*)sValueBuffer;
    sIndexInfo4Fetch.mBufferCursor          = (UChar*)sValueBuffer;
    sIndexInfo4Fetch.mFetchSize             = ID_SIZEOF(stdGeometryHeader);

    IDE_TEST( sdcRow::fetch(
                          aStatistics,
                          aMtx,
                          aSP,
                          aTrans,
                          aTableSpaceID,
                          (UChar*)aRow,
                          ID_TRUE, /* aIsPersSlot */
                          aPageReadMode,
                          &sIndexHeader->mFetchColumnListToMakeKey,
                          aFetchVersion,
                          aTSSlotSID,
                          aSCN,
                          aInfiniteSCN,
                          &sIndexInfo4Fetch,
                          NULL, /* aLobInfo4Fetch */
                          ((smcTableHeader*)aTableHeader)->mRowTemplate,
                          (UChar*)sValueBuffer,
                          aIsRowDeleted,
                          aIsPageLatchReleased)
                  != IDE_SUCCESS );

    if( *aIsRowDeleted == ID_FALSE )
    {
        makeKeyValueFromSmiValueList( aIndex, 
                                      sIndexInfo4Fetch.mValueList, 
                                      aDestBuf );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * sdcRow가 넘겨준 정보를 바탕으로 smiValue형태를 다시 구축한다.
 * 인덱스 키는 다시 구축된 smiValue를 바탕으로 키를 생성하게 된다.
 * Disk R-Tree의 경우 mFetchSize( = sizeof(stdGeometryHeader) ) 만을
 * 읽어서 구성한다.
 *********************************************************************/
IDE_RC stndrRTree::makeSmiValueListInFetch(
                       const smiColumn              * aIndexColumn,
                       UInt                           aCopyOffset,
                       const smiValue               * aColumnValue,
                       void                         * aIndexInfo )
{
    smiValue            * sValue;
    UShort                sColumnSeq;
    UInt                  sLength;
    sdcIndexInfo4Fetch  * sIndexInfo;

    IDE_DASSERT( aIndexColumn     != NULL );
    IDE_DASSERT( aColumnValue     != NULL );
    IDE_DASSERT( aIndexInfo       != NULL );

    sIndexInfo = (sdcIndexInfo4Fetch*)aIndexInfo;
    sColumnSeq = aIndexColumn->id & SMI_COLUMN_ID_MASK;
    sValue     = &sIndexInfo->mValueList[ sColumnSeq ];

    /* Proj-1872 Disk Index 저장구조 최적화 
     * 이 함수는 키 생성시 불리며, 키 생성시 mFetchColumnListToMakeKey를
     * 이용한다. mFetchColumnListToMakeKey의 column(aIndexColumn)은 VRow를
     * 생성할때는 이용되지 않기 때문에, 항상 Offset이 0이다. */
    IDE_DASSERT( aIndexColumn->offset == 0 );
    if( sIndexInfo->mFetchSize <= 0 ) //FetchSize는 Rtree만을 위함
    {
        ideLog::log( IDE_SERVER_0, "Index info:\n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar *)sIndexInfo,
                        ID_SIZEOF(sdcIndexInfo4Fetch) );
        IDE_ASSERT( 0 );
    }

    if( aCopyOffset == 0 ) //first col-piece
    {
        sValue->length = aColumnValue->length;
        if( sValue->length > sIndexInfo->mFetchSize )
        {
            sValue->length = sIndexInfo->mFetchSize;
        }
        
        sValue->value = sIndexInfo->mBufferCursor;
        
        sLength = sValue->length;
    }
    else //first col-piece가 아닌 경우
    {
        if( (sValue->length + aColumnValue->length) > sIndexInfo->mFetchSize )
        {
            sLength = sIndexInfo->mFetchSize - sValue->length;
        }
        else
        {
            sLength = aColumnValue->length;
        }
        
        sValue->length += sLength;
    }

    if( 0 < sLength ) //NULL일 경우 length는 0
    {
        ID_WRITE_AND_MOVE_DEST( sIndexInfo->mBufferCursor, 
                                aColumnValue->value, 
                                sLength );
    }

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * smiValue list를 가지고 Key Value(index에서 사용하는 값)를 만든다.
 * insert DML시에 이 함수를 호출한다.
 * Disk R-Tree의 경우 stdGeometryHeader를 KeyValue로 구성한다.
 *********************************************************************/
IDE_RC stndrRTree::makeKeyValueFromSmiValueList( const smnIndexHeader   * aIndex,
                                                 const smiValue         * aValueList,
                                                 UChar                  * aDestBuf )
{
    smiValue            * sIndexValue;
    UShort                sColumnSeq;
    UShort                sColCount = 0;
    UChar               * sBufferCursor;
    smiColumn           * sKeyColumn;
    stndrColumn         * sIndexColumn;
    stdGeometryHeader   * sGeometryHeader;


    IDE_DASSERT( aIndex     != NULL );
    IDE_DASSERT( aValueList != NULL );
    IDE_DASSERT( aDestBuf   != NULL );

    sBufferCursor = aDestBuf;
    sColCount     = aIndex->mColumnCount;

    IDE_ASSERT( sColCount == 1 );

    sColumnSeq   = aIndex->mColumns[0] & SMI_COLUMN_ID_MASK;
    sIndexValue  = (smiValue*)(aValueList + sColumnSeq);
        
    sIndexColumn = &((stndrHeader*)aIndex->mHeader)->mColumn;
    sKeyColumn   = &sIndexColumn->mKeyColumn;

    if( sColumnSeq != (sKeyColumn->id % SMI_COLUMN_ID_MAXIMUM) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Column sequence : %u"
                     ", Column id : %u"
                     "\nColumn information :\n",
                     sColumnSeq, sKeyColumn->id );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar *)sKeyColumn,
                        ID_SIZEOF(smiColumn) );
        IDE_ASSERT( 0 );
    }

    if( sIndexValue->length == 0 ) // NULL
    {
        (void)stdUtils::nullify( (stdGeometryHeader*)sBufferCursor );
    }
    else
    {
        IDE_ASSERT( sIndexValue->length >= ID_SIZEOF(stdGeometryHeader) );

        sGeometryHeader = (stdGeometryHeader*)sIndexValue->value;

        ID_WRITE_AND_MOVE_DEST( sBufferCursor,
                                sGeometryHeader,
                                ID_SIZEOF(stdGeometryHeader) );
    }
    
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::allocCTS
 * ------------------------------------------------------------------*
 * CTS를 할당한다. 
 * 공간이 부족한 경우에는 Compaction이후에 확장도 고려한다. 
 *********************************************************************/
IDE_RC stndrRTree::allocCTS( idvSQL             * aStatistics,
                             stndrHeader        * aIndex,
                             sdrMtx             * aMtx,
                             sdpPhyPageHdr      * aNode,
                             UChar              * aCTSlotNum,
                             sdnCallbackFuncs   * aCallbackFunc,
                             UChar              * aContext,
                             SShort             * aKeySeq )
{
    UShort            sNeedSize;
    UShort            sFreeSize;
    stndrNodeHdr    * sNodeHdr;
    idBool            sSuccess;

    
    IDE_TEST( sdnIndexCTL::allocCTS(aStatistics,
                                    aMtx,
                                    &aIndex->mSdnHeader.mSegmentDesc.mSegHandle,
                                    aNode,
                                    aCTSlotNum,
                                    aCallbackFunc,
                                    aContext)
              != IDE_SUCCESS );

    if( *aCTSlotNum == SDN_CTS_INFINITE )
    {
        sNodeHdr  = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

        sNeedSize = ID_SIZEOF( sdnCTS );
        
        sFreeSize = getTotalFreeSize( aIndex, aNode );
        sFreeSize += sNodeHdr->mTotalDeadKeySize;

        if( sNeedSize <= sFreeSize )
        {
            // Non-Fragment Free Space가 필요한 사이즈보다 작은 경우는
            // Compaction하지 않는다.
            if( sNeedSize > getNonFragFreeSize(aIndex, aNode) )
            {
                if( aKeySeq != NULL )
                {
                    adjustKeyPosition( aNode, aKeySeq );
                }
            
                IDE_TEST( compactPage( aMtx,
                                       aNode,
                                       ID_TRUE ) != IDE_SUCCESS );
            }

            *aCTSlotNum = sdnIndexCTL::getCount( aNode );

            IDE_TEST( sdnIndexCTL::extend(
                          aMtx,
                          &aIndex->mSdnHeader.mSegmentDesc.mSegHandle,
                          aNode,
                          ID_TRUE, //aLogging
                          &sSuccess)
                      != IDE_SUCCESS );

            if( sSuccess == ID_FALSE )
            {
                *aCTSlotNum = SDN_CTS_INFINITE;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::allocPage
 * ------------------------------------------------------------------*
 * Free Node List에 Free페이지가 존재한다면 Free Node List로 부터 빈
 * 페이지를 할당 받고, 만약 그렇지 않다면 Physical Layer로 부터 할당
 * 받는다.
 *********************************************************************/
IDE_RC stndrRTree::allocPage( idvSQL            * aStatistics,
                              stndrStatistic    * aIndexStat,
                              stndrHeader       * aIndex,
                              sdrMtx            * aMtx,
                              UChar            ** aNewNode )
{
    idBool            sIsSuccess;
    UChar           * sMetaPage;
    stndrMeta       * sMeta;
    stndrNodeHdr    * sNodeHdr;
    smSCN             sSysMinDskViewSCN;
    sdrSavePoint      sSP;

    if( aIndex->mFreeNodeCnt > 0 )
    {
        sMetaPage = sdrMiniTrans::getPagePtrFromPageID(
            aMtx,
            aIndex->mSdnHeader.mIndexTSID,
            SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ));
    
        if( sMetaPage == NULL )
        {
            IDE_TEST( stndrRTree::getPage( aStatistics,
                                           &(aIndexStat->mMetaPage),
                                           aIndex->mSdnHeader.mIndexTSID,
                                           SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ),
                                           SDB_X_LATCH,
                                           SDB_WAIT_NORMAL,
                                           aMtx,
                                           (UChar**)&sMetaPage,
                                           &sIsSuccess )
                      != IDE_SUCCESS );
        }

        sMeta = (stndrMeta*)( sMetaPage + SMN_INDEX_META_OFFSET );

        IDE_ASSERT( sMeta->mFreeNodeHead != SD_NULL_PID );
        IDE_ASSERT( sMeta->mFreeNodeCnt == aIndex->mFreeNodeCnt );
        
        sdrMiniTrans::setSavePoint( aMtx, &sSP );
        
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mIndexPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       sMeta->mFreeNodeHead,
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       aMtx,
                                       (UChar**)aNewNode,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
        
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*)*aNewNode);

        /*
         * Leaf Node의 경우는 반드시 FREE LIST에 연결되어 있는
         * 상태여야 한다.
         */
        if( ( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE ) &&
            ( sNodeHdr->mState != STNDR_IN_FREE_LIST ) )
        {
            ideLog::log( IDE_SERVER_0,
                         "Index TSID : %u"
                         ", Get page ID : %u\n",
                         aIndex->mSdnHeader.mIndexTSID,
                         sMeta->mFreeNodeHead );
            dumpIndexNode( (sdpPhyPageHdr *)*aNewNode );
            IDE_ASSERT( 0 );
        }
        
        // SCN 비교
        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );
        
        if( SM_SCN_IS_LT(&sNodeHdr->mFreeNodeSCN, &sSysMinDskViewSCN) )
        {
            aIndex->mFreeNodeCnt--;
            aIndex->mFreeNodeHead = sNodeHdr->mNextFreeNode;
            aIndex->mFreeNodeSCN  = sNodeHdr->mNextFreeNodeSCN;
        
            IDE_TEST( setFreeNodeInfo(aStatistics,
                                      aIndex,
                                      aIndexStat,
                                      aMtx,
                                      sNodeHdr->mNextFreeNode,
                                      aIndex->mFreeNodeCnt,
                                      &aIndex->mFreeNodeSCN)
                      != IDE_SUCCESS );

            IDE_TEST( sdpPhyPage::reset((sdpPhyPageHdr*)*aNewNode,
                                        ID_SIZEOF( stndrNodeHdr ),
                                        aMtx)
                      != IDE_SUCCESS );

            IDE_TEST( sdpSlotDirectory::logAndInit((sdpPhyPageHdr*)*aNewNode,
                                                   aMtx)
                      != IDE_SUCCESS );

            IDE_RAISE( SKIP_ALLOC_NEW_PAGE );
        }
        else
        {
            sdrMiniTrans::releaseLatchToSP( aMtx, &sSP );
        }
    }

    // alloc page
    IDE_ASSERT( sdpSegDescMgr::getSegMgmtOp(
                    &(aIndex->mSdnHeader.mSegmentDesc) )->mAllocNewPage(
                        aStatistics,
                        aMtx,
                        aIndex->mSdnHeader.mIndexTSID,
                        &(aIndex->mSdnHeader.mSegmentDesc.mSegHandle),
                        SDP_PAGE_INDEX_RTREE,
                        (UChar**)aNewNode)
                == IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::logAndInitLogicalHdr(
                  (sdpPhyPageHdr*)*aNewNode,
                  ID_SIZEOF(stndrNodeHdr),
                  aMtx,
                  (UChar**)&sNodeHdr ) != IDE_SUCCESS );

    IDE_TEST( sdpSlotDirectory::logAndInit((sdpPhyPageHdr*)*aNewNode,
                                           aMtx)
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_ALLOC_NEW_PAGE );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::getPage
 * ------------------------------------------------------------------*
 * To fix BUG-18252
 * 인덱스 페이지및 메타페이지의 접근 빈도에 대한 통계정보 구축
 *********************************************************************/
IDE_RC stndrRTree::getPage( idvSQL          * aStatistics,
                            stndrPageStat   * aPageStat,
                            scSpaceID         aSpaceID,
                            scPageID          aPageID,
                            sdbLatchMode      aLatchMode,
                            sdbWaitMode       aWaitMode,
                            void            * aMtx,
                            UChar          ** aRetPagePtr,
                            idBool          * aTrySuccess )
{
    idvSQL      * sStatistics;
    idvSession    sDummySession;
    idvSQL        sDummySQL;
    ULong         sGetPageCount;
    ULong         sReadPageCount;
    

    sDummySQL.mGetPageCount = 0;
    sDummySQL.mReadPageCount = 0;

    if( aStatistics == NULL )
    {
        //fix for UMR
        idvManager::initSession( &sDummySession, 0, NULL );

        idvManager::initSQL( &sDummySQL,
                             &sDummySession,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             IDV_OWNER_UNKNOWN );
        sStatistics = &sDummySQL;
    }
    else
    {
        sStatistics = aStatistics;
    }

    sGetPageCount = sStatistics->mGetPageCount;
    sReadPageCount = sStatistics->mReadPageCount;

    IDE_TEST( sdbBufferMgr::getPageByPID( sStatistics,
                                          aSpaceID,
                                          aPageID,
                                          aLatchMode,
                                          aWaitMode,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          aRetPagePtr,
                                          aTrySuccess,
                                          NULL /*IsCorruptPage*/ )
              != IDE_SUCCESS );

    aPageStat->mGetPageCount += sStatistics->mGetPageCount - sGetPageCount;
    aPageStat->mReadPageCount += sStatistics->mReadPageCount - sReadPageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::freePage
 * ------------------------------------------------------------------*
 * Free된 노드를 Free Node List에 연결한다.
 *********************************************************************/
IDE_RC stndrRTree::freePage( idvSQL         * aStatistics,
                             stndrStatistic * aIndexStat,
                             stndrHeader    * aIndex,
                             sdrMtx         * aMtx,
                             UChar          * aFreePage )
{
    idBool            sIsSuccess;
    UChar           * sMetaPage;
    stndrMeta       * sMeta;
    stndrNodeHdr    * sNodeHdr;
    smSCN             sNextFreeNodeSCN;
    UChar             sNodeState = STNDR_IN_FREE_LIST;
    
    sMetaPage = sdrMiniTrans::getPagePtrFromPageID(
                                aMtx,
                                aIndex->mSdnHeader.mIndexTSID,
                                SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ));
                            
    if( sMetaPage == NULL )
    {
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mMetaPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       SD_MAKE_PID(aIndex->mSdnHeader.mMetaRID),
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       aMtx,
                                       (UChar**)&sMetaPage,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
    }

    sMeta = (stndrMeta*)( sMetaPage + SMN_INDEX_META_OFFSET );

    IDE_ASSERT( sMeta->mFreeNodeCnt == aIndex->mFreeNodeCnt );

    sNextFreeNodeSCN = aIndex->mFreeNodeSCN;
    
    aIndex->mFreeNodeCnt++;
    aIndex->mFreeNodeHead = sdpPhyPage::getPageID( aFreePage );
    SMX_GET_SYSTEM_VIEW_SCN( &aIndex->mFreeNodeSCN );
        
    sNodeHdr = (stndrNodeHdr*)
                    sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aFreePage );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sNodeHdr->mFreeNodeSCN,
                                         (void*)&aIndex->mFreeNodeSCN,
                                         ID_SIZEOF(aIndex->mFreeNodeSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sNodeHdr->mNextFreeNodeSCN,
                                         (void*)&sNextFreeNodeSCN,
                                         ID_SIZEOF(sNextFreeNodeSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sNodeHdr->mNextFreeNode,
                                         (void*)&sMeta->mFreeNodeHead,
                                         ID_SIZEOF(sMeta->mFreeNodeHead) )
              != IDE_SUCCESS );

    // Internal Node가 Free List에 달릴 수도 있으므로 freePage 시에
    // 상태를 STNDR_FREE_LIST로 변경한다.
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sNodeHdr->mState,
                                         (void*)&sNodeState,
                                         ID_SIZEOF(sNodeState) )
              != IDE_SUCCESS );

    IDE_TEST( setFreeNodeInfo( aStatistics,
                               aIndex,
                               aIndexStat,
                               aMtx,
                               aIndex->mFreeNodeHead,
                               aIndex->mFreeNodeCnt,
                               &aIndex->mFreeNodeSCN )
              != IDE_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Index node의 Header(stndrNodeHdr)를 초기화한다. 새 Index node를 
 * 할당받게 되면, 반드시 이 함수를 통하여 Node header를 초기화하게
 * 되며, Logging option에 따라서, logging/no-logging을 결정한다.
 *********************************************************************/
IDE_RC stndrRTree::initializeNodeHdr( sdrMtx        * aMtx,
                                      sdpPhyPageHdr * aNode,
                                      UShort          aHeight,
                                      idBool          aIsLogging )
{
    stndrNodeHdr    * sNodeHdr;
    UShort            sKeyCount  = 0;
    UChar             sNodeState = STNDR_IN_USED;
    UShort            sTotalDeadKeySize = 0;
    scPageID          sEmptyPID = SD_NULL_PID;
    UShort            sTBKCount = 0;
    UShort            sRefTBK[STNDR_TBK_MAX_CACHE];
    UInt              i;
    smSCN             sSCN;

    
    SM_INIT_SCN( &sSCN );

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)aNode);

    for( i = 0; i < STNDR_TBK_MAX_CACHE; i++ )
    {
        sRefTBK[i] = STNDR_TBK_CACHE_NULL;
    }
    
    if(aIsLogging == ID_TRUE)
    {

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mNextEmptyNode,
                                             (void*)&sEmptyPID,
                                             ID_SIZEOF(sNodeHdr->mNextEmptyNode) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mNextFreeNode,
                                             (void*)&sEmptyPID,
                                             ID_SIZEOF(sNodeHdr->mNextFreeNode) )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mFreeNodeSCN,
                                             (void*)&sSCN,
                                             ID_SIZEOF(sNodeHdr->mFreeNodeSCN) )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sNodeHdr->mNextFreeNodeSCN,
                                             (void*)&sSCN,
                                             ID_SIZEOF(sNodeHdr->mNextFreeNodeSCN) )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mHeight,
                                            (void*)&aHeight,
                                            ID_SIZEOF(aHeight))
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mUnlimitedKeyCount,
                                            (void*)&sKeyCount,
                                            ID_SIZEOF(sKeyCount))
                  != IDE_SUCCESS );
        
        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mTotalDeadKeySize,
                                            (void*)&sTotalDeadKeySize,
                                            ID_SIZEOF(sTotalDeadKeySize))
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mTBKCount,
                                            (void*)&sTBKCount,
                                            ID_SIZEOF(sTBKCount))
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mRefTBK,
                                            (void*)&sRefTBK,
                                            ID_SIZEOF(UShort)*STNDR_TBK_MAX_CACHE)
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::writeNBytes(aMtx,
                                            (UChar*)&sNodeHdr->mState,
                                            (void*)&sNodeState,
                                            ID_SIZEOF(sNodeState))
                  != IDE_SUCCESS );
    }
    else
    {
        sNodeHdr->mNextEmptyNode     = sEmptyPID;
        sNodeHdr->mNextEmptyNode     = sEmptyPID;
        sNodeHdr->mFreeNodeSCN       = sSCN;
        sNodeHdr->mNextFreeNodeSCN   = sSCN;
        sNodeHdr->mHeight            = aHeight;
        sNodeHdr->mUnlimitedKeyCount = sKeyCount;
        sNodeHdr->mTotalDeadKeySize  = sTotalDeadKeySize;
        sNodeHdr->mTBKCount          = sTBKCount;
        sNodeHdr->mState             = sNodeState;
        
        idlOS::memcpy( sNodeHdr->mRefTBK,
                       sRefTBK,
                       ID_SIZEOF(UShort)*STNDR_TBK_MAX_CACHE );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::makeNewootNode
 * ------------------------------------------------------------------*
 * 새로운 node를 할당하여 root 노드로 만든다.
 * 새로 생성된 Root가 Leaf Node가 아닌 경우 Split된 두 노드를 가리키는
 * 두 개의 키가 올라온다.
 * !!CAUTION!!: Runtime Header의 Root Node 번호를 변경 후에
 * SmoNo를 증가 시킨다. Traverse 쪽은 SmoNo를 먼저 얻고 Root Node를 얻어가기
 * 때문이다.
 *********************************************************************/
IDE_RC stndrRTree::makeNewRootNode( idvSQL          * aStatistics,
                                    stndrStatistic  * aIndexStat,
                                    sdrMtx          * aMtx,
                                    idBool          * aMtxStart,
                                    stndrHeader     * aIndex,
                                    smSCN           * aInfiniteSCN,
                                    stndrKeyInfo    * aLeftKeyInfo,
                                    sdpPhyPageHdr   * aLeftNode,
                                    stndrKeyInfo    * aRightKeyInfo,
                                    UShort            aKeyValueLen,
                                    sdpPhyPageHdr  ** aNewChildPage,
                                    UInt            * aAllocPageCount )
{
    sdpPhyPageHdr         * sNewRootPage;
    stndrNodeHdr          * sNodeHdr;
    UShort                  sHeight;
    scPageID                sNewRootPID;
    scPageID                sChildPID;
    UShort                  sKeyLen;
    UShort                  sAllowedSize;
    scOffset                sSlotOffset;
    stndrIKey             * sIKey;
    SShort                  sKeySeq = 0;
    idBool                  sIsSuccess = ID_FALSE;
    ULong                   sSmoNo;
    stdMBR                  sNodeMBR;
    IDE_RC                  sRc;
    UChar                   sCTSlotNum = SDN_CTS_INFINITE;
    stndrCallbackContext    sContext;

    
    // set height
    if( aLeftNode == NULL )
    {
        sHeight = 0;
    }
    else
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aLeftNode );
        sHeight = sNodeHdr->mHeight + 1;
    }
    
    // SMO의 최상단 -- allocPages
    // allocate new pages, stack tmp depth + 2 만큼
    IDE_TEST( preparePages( aStatistics,
                            aIndex,
                            aMtx,
                            aMtxStart,
                            sHeight + 1 )
              != IDE_SUCCESS );

    // init root node
    IDE_ASSERT( allocPage( aStatistics,
                           aIndexStat,
                           aIndex,
                           aMtx,
                           (UChar**)&sNewRootPage )
                == IDE_SUCCESS );
    (*aAllocPageCount)++;

    IDE_ASSERT( ((vULong)sNewRootPage % SD_PAGE_SIZE) == 0 );

    sNewRootPID = sdpPhyPage::getPageID( (UChar*)sNewRootPage );

    IDE_TEST( initializeNodeHdr(aMtx,
                                sNewRootPage,
                                sHeight,
                                ID_TRUE)
              != IDE_SUCCESS );
                                 
    if( sHeight == 0 ) // 새 root가 leaf node
    {
        IDE_ASSERT( aLeftKeyInfo != NULL && aRightKeyInfo == NULL );

        if( aIndex->mRootNode != SD_NULL_PID )
        {
            ideLog::log( IDE_SERVER_0, "Index header:\n" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar *)aIndex,
                            ID_SIZEOF(stndrHeader) );
            IDE_ASSERT( 0 );
        }
        
        IDE_TEST( sdnIndexCTL::init(
                      aMtx,
                      &aIndex->mSdnHeader.mSegmentDesc.mSegHandle,
                      sNewRootPage,
                      0)
                  != IDE_SUCCESS );
        
        *aNewChildPage = sNewRootPage;
        sKeySeq = 0;

        IDE_TEST( canInsertKey( aStatistics,
                                aMtx,
                                aIndex,
                                aIndexStat,
                                aLeftKeyInfo,
                                sNewRootPage,
                                &sKeySeq,
                                &sCTSlotNum,
                                &sContext )
                  != IDE_SUCCESS );

        IDE_TEST( insertKeyIntoLeafNode( aMtx,
                                         aIndex,
                                         aInfiniteSCN,
                                         sNewRootPage,
                                         &sKeySeq,
                                         aLeftKeyInfo,
                                         sCTSlotNum,
                                         &sIsSuccess )
                  != IDE_SUCCESS );
        
        IDE_ASSERT( sIsSuccess == ID_TRUE );
    }
    else
    {
        IDE_ASSERT( aLeftKeyInfo != NULL && aRightKeyInfo != NULL );
        
        // 새 child node의 PID는 새 root의 새 slot의 child PID
        IDE_ASSERT( allocPage(aStatistics,
                              aIndexStat,
                              aIndex,
                              aMtx,
                              (UChar**)aNewChildPage) == IDE_SUCCESS );
        (*aAllocPageCount)++;

        // insert left key
        sChildPID = sdpPhyPage::getPageID( (UChar*)aLeftNode );
        sKeyLen   = STNDR_IKEY_LEN( aKeyValueLen );
        sKeySeq   = 0;

        IDE_TEST( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)sNewRootPage,
                                         sKeySeq, // slot number
                                         sKeyLen,
                                         ID_TRUE,
                                         &sAllowedSize,
                                         (UChar**)&sIKey,
                                         &sSlotOffset,
                                         1 )
                  != IDE_SUCCESS );

        STNDR_KEYINFO_TO_IKEY( (*aLeftKeyInfo),
                               sChildPID,
                               aKeyValueLen,
                               sIKey );

        if( aIndex->mSdnHeader.mLogging == ID_TRUE )
        {
            IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                                 (UChar*)sIKey,
                                                 (void*)NULL,
                                                 ID_SIZEOF(sKeySeq)+sKeyLen,
                                                 SDR_STNDR_INSERT_INDEX_KEY )
                      != IDE_SUCCESS );

            IDE_TEST( sdrMiniTrans::write( aMtx,
                                           (void*)&sKeySeq,
                                           ID_SIZEOF(sKeySeq) )
                      != IDE_SUCCESS );

            IDE_TEST( sdrMiniTrans::write( aMtx,
                                           (void*)sIKey,
                                           sKeyLen )
                      != IDE_SUCCESS );
        }

        // insert right key
        sChildPID = sdpPhyPage::getPageID( (UChar*)*aNewChildPage );
        sKeyLen   = STNDR_IKEY_LEN( aKeyValueLen );
        sKeySeq   = 1;

        IDE_TEST( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)sNewRootPage,
                                         sKeySeq, // slot number
                                         sKeyLen,
                                         ID_TRUE,
                                         &sAllowedSize,
                                         (UChar**)&sIKey,
                                         &sSlotOffset,
                                         1 )
                  != IDE_SUCCESS );

        STNDR_KEYINFO_TO_IKEY( (*aRightKeyInfo),
                               sChildPID,
                               aKeyValueLen,
                               sIKey );

        if( aIndex->mSdnHeader.mLogging == ID_TRUE )
        {
            IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                                (UChar*)sIKey,
                                                (void*)NULL,
                                                ID_SIZEOF(sKeySeq)+sKeyLen,
                                                SDR_STNDR_INSERT_INDEX_KEY )
                      != IDE_SUCCESS );

            IDE_TEST( sdrMiniTrans::write(aMtx,
                                          (void*)&sKeySeq,
                                          ID_SIZEOF(sKeySeq) )
                      != IDE_SUCCESS );

            IDE_TEST( sdrMiniTrans::write(aMtx,
                                          (void*)sIKey,
                                          sKeyLen)
                      != IDE_SUCCESS );
        }
        
    } // if( sHeight == 0 )

    // update node MBR
    IDE_ASSERT( adjustNodeMBR( aIndex,
                               sNewRootPage,
                               NULL,                  /* aInsertMBR */
                               STNDR_INVALID_KEY_SEQ, /* aUpdateKeySeq */
                               NULL,                  /* aUpdateMBR */
                               STNDR_INVALID_KEY_SEQ, /* aDeleteKeySeq */
                               &sNodeMBR )
                == IDE_SUCCESS );
        
    sRc = setNodeMBR( aMtx,
                      sNewRootPage,
                      &sNodeMBR );

    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0, "update MBR : \n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sNodeMBR,
                        ID_SIZEOF(stdMBR) );
        dumpIndexNode( sNewRootPage );
        IDE_ASSERT( 0 );
    }

    IDE_TEST( backupRuntimeHeader( aMtx, aIndex ) != IDE_SUCCESS );

    // update Tree MBR
    // : Tree MBR은 동시성 문제로,Root Node가 Latch가
    // 잡힌 상태에서 업데이트 하도록 한다.
    if( aIndex->mInitTreeMBR == ID_TRUE )
    {
        aIndex->mTreeMBR = sNodeMBR;
    }
    else
    {
        aIndex->mTreeMBR = sNodeMBR;
        aIndex->mInitTreeMBR = ID_TRUE;
    }

    // RootNode와 RootNode의 SmoNo를 설정한다.
    aIndex->mRootNode = sNewRootPID;
    
    IDE_TEST( increaseSmoNo( aStatistics,
                             aIndex,
                             &sSmoNo )
              != IDE_SUCCESS );

    sdpPhyPage::setIndexSMONo( (UChar*)sNewRootPage, sSmoNo );

    // Virtual Root Node를 설정한다.
    setVirtualRootNode( aIndex, sNewRootPID, sSmoNo );

    IDE_TEST( setIndexMetaInfo( aStatistics,
                                aIndex,
                                aIndexStat,
                                aMtx,
                                &sNewRootPID,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * index에 Key를 insert 한다.
 *
 * 먼저 키를 삽입하기 위한 Leaf Node를 탐색한다. 탐색 방법은 Key 삽입으로
 * 인한 영억 확장이 최소화 되는 노드를 산택하는 것이다. Leaf Node를 찾기 위한
 * 탐색은 하나의 노드에 대해서만 S-Latch를 잡고 Optimistic 하게 수행되며,
 * Leaf Node에 대해서만 X-Latch를 잡는다.
 *
 * Leaf Node에 Key를 삽입할 공간이 있으면 키를 삽입한다. 키 삽입으로 인해 노드
 * MBR이 변경될 경우 이를 상위 부모노드에 Propagation한다. 이때 Leaf Node로
 * 부터 변경될 상위 부모 노드에까지 X-Latch가 잡한다.
 *
 * Leaf Node에 Key를 삽입할 공간이 없으면 Split을 수행한다. Split 수행 후
 * 반환된 노드에 Key를 삽입한다. Split 후 Empty Node가 생성될 경우 이를
 * Empty List에 연결한다.
 *
 * insertKey가 새로운 노드를 할당 받았을 경우에 대해서만 Node Aging을 수행
 * 한다.
 *********************************************************************/
IDE_RC stndrRTree::insertKey( idvSQL    * aStatistics,
                              void      * aTrans,
                              void      * /* aTable */,
                              void      * aIndex,
                              smSCN       aInfiniteSCN,
                              SChar     * aKeyValue,
                              SChar     * /* aNullRow */,
                              idBool      /* aUniqueCheck */,
                              smSCN       aStmtViewSCN,
                              void      * aRowSID,
                              SChar    ** /* aExistUniqueRow */,
                              ULong       /* aInsertWaitTime */,
                              idBool      /* aForbiddenToRetry */ )
{
    sdrMtx                  sMtx;
    stndrPathStack          sStack;
    sdpPhyPageHdr         * sNewChildPage;
    UShort                  sKeyValueLength;
    sdpPhyPageHdr         * sLeafNode;
    SShort                  sLeafKeySeq;
    stndrNodeHdr          * sNodeHdr;
    idBool                  sIsSuccess;
    stndrKeyInfo            sKeyInfo;
    idBool                  sMtxStart = ID_FALSE;
    idBool                  sIsRetry = ID_FALSE;
    stndrHeader           * sHeader = (stndrHeader*)((smnIndexHeader*)aIndex)->mHeader;
    smLSN                   sNTA;
    idvWeArgs               sWeArgs;
    UInt                    sAllocPageCount = 0;
    sdSID                   sRowSID;
    smSCN                   sStmtSCN;
    sdpPhyPageHdr         * sTargetNode;
    SShort                  sTargetKeySeq;
    scPageID                sEmptyNodePID[2] = { SD_NULL_PID, SD_NULL_PID };
    UInt                    sEmptyNodeCount = 0;
    stndrStatistic          sIndexStat;
    stdMBR                  sKeyInfoMBR;
    stdMBR                  sLeafNodeMBR;
    idBool                  sIsIndexable;
    idBool                  sIsPessimistic = ID_FALSE;
    UChar                   sCTSlotNum = SDN_CTS_INFINITE;
    stndrCallbackContext    sContext;

    
    isIndexableRow( aIndex, aKeyValue, &sIsIndexable );
    
    IDE_TEST_RAISE(sIsIndexable == ID_FALSE, RETURN_NO_INSERT);

    sRowSID = *((sdSID*)aRowSID);

    SM_SET_SCN( &sStmtSCN, &aStmtViewSCN );
    SM_CLEAR_SCN_VIEW_BIT( &sStmtSCN );

    sKeyInfo.mKeyValue   = (UChar*)&(((stdGeometryHeader*)aKeyValue)->mMbr);
    sKeyInfo.mRowPID     = SD_MAKE_PID( sRowSID );
    sKeyInfo.mRowSlotNum = SD_MAKE_SLOTNUM( sRowSID );
    sKeyInfo.mKeyState   = SM_SCN_IS_MAX( sStmtSCN )
        ? STNDR_KEY_STABLE : STNDR_KEY_UNSTABLE;

    STNDR_GET_MBR_FROM_KEYINFO( sKeyInfoMBR, &sKeyInfo );

    sNTA = smxTrans::getTransLstUndoNxtLSN( aTrans );

    idlOS::memset( &sIndexStat, 0x00, ID_SIZEOF(sIndexStat) );

  retry:
        
    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType ) != IDE_SUCCESS );

    sMtxStart = ID_TRUE;

    sIsRetry = ID_FALSE;
        
    sStack.mDepth = -1;
    
    if( sHeader->mRootNode == SD_NULL_PID )
    {
        IDV_WEARGS_SET(
            &sWeArgs,
            IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
            0,   // WaitParam1
            0,   // WaitParam2
            0 ); // WaitParam3

        IDE_TEST( sHeader->mSdnHeader.mLatch.lockWrite(aStatistics,
                                                       &sWeArgs )
                  != IDE_SUCCESS);

        IDE_TEST( sdrMiniTrans::push(
                      &sMtx,
                      (void*)&sHeader->mSdnHeader.mLatch,
                      SDR_MTX_LATCH_X)
                  != IDE_SUCCESS );

        if( sHeader->mRootNode != SD_NULL_PID )
        {
            // latch 잡는 사이에 Root Node가 생긴 경우
            sMtxStart = ID_FALSE;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
            sIndexStat.mOpRetryCount++;
            goto retry;
        }

        sKeyValueLength = getKeyValueLength();

        IDE_TEST( makeNewRootNode( aStatistics,
                                   &sIndexStat,
                                   &sMtx,
                                   &sMtxStart,
                                   sHeader,
                                   &aInfiniteSCN,
                                   &sKeyInfo,   // aLeftKeyInfo
                                   NULL,        // aLeftNode
                                   NULL,        // aRightKeyInfo
                                   sKeyValueLength,
                                   &sNewChildPage,
                                   &sAllocPageCount )
                  != IDE_SUCCESS );

        sLeafNode = sNewChildPage;
    }
    else
    {
        sIsPessimistic = ID_FALSE;
        
      retraverse:
        
        IDE_TEST( chooseLeafNode( aStatistics,
                                  &sIndexStat,
                                  &sMtx,
                                  &sStack,
                                  sHeader,
                                  &sKeyInfo,
                                  &sLeafNode,
                                  &sLeafKeySeq ) != IDE_SUCCESS );
        
        // 탐색중 Root Node가 사라진 경우 retry한다.
        if( sLeafNode == NULL )
        {
            sMtxStart = ID_FALSE;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            sIndexStat.mOpRetryCount++;
            goto retry;
        }


        // BUG-29596: LeafNode에 대한 키 삽입 공간 체크시 CTL의 확장을 고려하지 않아 FATAL 발생합니다.
        // 키 삽입시 CTL의 확장을 고려하지 않아서 Free 공간을 잘못 계산하는 문제
        if( canInsertKey( aStatistics,
                          &sMtx,
                          sHeader,
                          &sIndexStat,
                          &sKeyInfo,
                          sLeafNode,
                          &sLeafKeySeq,
                          &sCTSlotNum,
                          &sContext ) == IDE_SUCCESS )
        {
            sNodeHdr = (stndrNodeHdr*)
                sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sLeafNode );

            sLeafNodeMBR = sNodeHdr->mMBR;

            if( stdUtils::isMBRContains( &sLeafNodeMBR, &sKeyInfoMBR )
                != ID_TRUE )
            {
                stdUtils::getMBRExtent( &sLeafNodeMBR, &sKeyInfoMBR );
                
                sStack.mDepth--;
                IDE_TEST( propagateKeyValue( aStatistics,
                                             &sIndexStat,
                                             sHeader,
                                             &sMtx,
                                             &sStack,
                                             sLeafNode,
                                             &sLeafNodeMBR,
                                             &sIsRetry ) != IDE_SUCCESS );
            
                if( sIsRetry == ID_TRUE )
                {
                    // propagation중에 root node가 변경된 경우
                    sMtxStart = ID_FALSE;
                    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
                    sIndexStat.mOpRetryCount++;
                    goto retry;
                }
            }

            IDE_TEST( insertKeyIntoLeafNode( &sMtx,
                                             sHeader,
                                             &aInfiniteSCN,
                                             sLeafNode,
                                             &sLeafKeySeq,
                                             &sKeyInfo,
                                             sCTSlotNum,
                                             &sIsSuccess )
                      != IDE_SUCCESS );
        }
        else
        {
            if( sIsPessimistic == ID_FALSE )
            {
                sIsPessimistic = ID_TRUE;
                
                sMtxStart = ID_FALSE;
                IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

                
                IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                               &sMtx,
                                               aTrans,
                                               SDR_MTX_LOGGING,
                                               ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                               gMtxDLogType )
                          != IDE_SUCCESS );
                sMtxStart = ID_TRUE;

                IDV_WEARGS_SET(
                    &sWeArgs,
                    IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
                    0,   // WaitParam1
                    0,   // WaitParam2
                    0 ); // WaitParam3

                IDE_TEST( sHeader->mSdnHeader.mLatch.lockWrite( aStatistics,
                                                                &sWeArgs )
                          != IDE_SUCCESS );

                IDE_TEST( sdrMiniTrans::push(
                              &sMtx,
                              (void*)&sHeader->mSdnHeader.mLatch,
                              SDR_MTX_LATCH_X )
                          != IDE_SUCCESS );

                IDE_TEST( findValidStackDepth( aStatistics,
                                               &sIndexStat,
                                               sHeader,
                                               &sStack,
                                               NULL /* aSmoNo */ )
                          != IDE_SUCCESS );

                if( sStack.mDepth < 0 )
                {
                    // BUG-29596: LeafNode에 대한 키 삽입 공간 체크시 CTL의 확장을 고려하지 않아 FATAL 발생합니다.
                    // Stress Test(InsertDelete) 중 Hang 걸리는 문제
                    sMtxStart = ID_FALSE;
                    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
                    sIndexStat.mOpRetryCount++;
                    goto retry;
                }
                else
                {
                    sStack.mDepth--;
                }
                
                goto retraverse;
            }
            else
            {
                sKeyValueLength = getKeyValueLength();

                IDE_TEST( splitLeafNode( aStatistics,
                                         &sIndexStat,
                                         &sMtx,
                                         &sMtxStart,
                                         sHeader,
                                         &aInfiniteSCN,
                                         &sStack,
                                         sLeafNode,
                                         &sKeyInfo,
                                         sKeyValueLength,
                                         sLeafKeySeq,
                                         ID_TRUE,
                                         smuProperty::getRTreeSplitMode(),
                                         &sTargetNode,
                                         &sTargetKeySeq,
                                         &sAllocPageCount,
                                         &sIsRetry )
                          != IDE_SUCCESS );

                if( sIsRetry == ID_TRUE )
                {
                    // key propagation을 위해 latch를 잡는 중에 root node가 변경된 경우
                    sMtxStart = ID_FALSE;
                    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
                    sIndexStat.mOpRetryCount++;
                    goto retry;
                }

                IDE_TEST( canInsertKey( aStatistics,
                                        &sMtx,
                                        sHeader,
                                        &sIndexStat,
                                        &sKeyInfo,
                                        sTargetNode,
                                        &sTargetKeySeq,
                                        &sCTSlotNum,
                                        &sContext )
                          != IDE_SUCCESS );
            
                IDE_TEST( insertKeyIntoLeafNode( &sMtx,
                                                 sHeader,
                                                 &aInfiniteSCN,
                                                 sTargetNode,
                                                 &sTargetKeySeq,
                                                 &sKeyInfo,
                                                 sCTSlotNum,
                                                 &sIsSuccess )
                          != IDE_SUCCESS );

                findEmptyNodes( &sMtx,
                                sHeader,
                                sLeafNode,
                                sEmptyNodePID,
                                &sEmptyNodeCount );
                
                sIndexStat.mNodeSplitCount++;
            }
        }
    } // if( sHeader->mRootNode == SD_NULL_PID )

    sHeader->mKeyCount++;

    sdrMiniTrans::setRefNTA( &sMtx,
                             sHeader->mSdnHeader.mIndexTSID,
                             SDR_OP_STNDR_INSERT_KEY_WITH_NTA,
                             &sNTA );

    sMtxStart = ID_FALSE;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    if( sAllocPageCount > 0 )
    {
        IDE_TEST( nodeAging( aStatistics,
                             aTrans,
                             &sIndexStat,
                             sHeader,
                             sAllocPageCount )
                  != IDE_SUCCESS );
    }

    if( sEmptyNodeCount > 0 )
    {
        IDE_TEST( linkEmptyNodes( aStatistics,
                                  aTrans,
                                  sHeader,
                                  &sIndexStat,
                                  sEmptyNodePID )
                  != IDE_SUCCESS );
    }

    STNDR_ADD_STATISTIC( &sHeader->mDMLStat, &sIndexStat );

    IDE_EXCEPTION_CONT( RETURN_NO_INSERT );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    if( sMtxStart == ID_TRUE )
    {
        sMtxStart = ID_FALSE;
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 만약 EMPTY LIST에 link된 이후에, 다른 트랜잭션에 의해서 키가 삽입
 * 된 경우에는 EMPTY LIST에서 해당 노드를 제거한다.                  
 * Node삭제는 더이상 해당 노드에 키가 없음을 보장해야 한다. 즉, 모든 
 * CTS가 DEAD상태를 보장해야 한다. 따라서, Hard Key Stamping을 통해  
 * 모든 CTS가 DEAD가 될수 있도록 시도한다.
 *********************************************************************/
IDE_RC stndrRTree::nodeAging( idvSQL            * aStatistics,
                              void              * aTrans,
                              stndrStatistic    * aIndexStat,
                              stndrHeader       * aIndex,
                              UInt                aFreePageCount )
{
    sdnCTL          * sCTL;
    sdnCTS          * sCTS;
    UInt              i;
    UChar             sAgedCount = 0;
    scPageID          sFreeNode;
    sdpPhyPageHdr   * sPage;
    UChar             sDeadCTSlotCount = 0;
    sdrMtx            sMtx;
    stndrKeyInfo      sKeyInfo;
    ULong             sTempKeyBuf[STNDR_MAX_KEY_BUFFER_SIZE/ID_SIZEOF(ULong)];
    stndrLKey       * sLeafKey;
    UShort            sKeyLength;
    UInt              sMtxStart = 0;
    UInt              sFreePageCount = 0;
    idBool            sSuccess = ID_FALSE;
    stndrNodeHdr    * sNodeHdr;
    idvWeArgs         sWeArgs;
    UChar           * sSlotDirPtr;
    
    /*
     * Link를 획득한 이후에 다른 트랜잭션에 의해서 삭제된 경우는
     * NodeAging을 하지 않는다.
     */
    IDE_TEST_RAISE( aIndex->mEmptyNodeHead == SD_NULL_PID, SKIP_AGING );

    while( 1 )
    {
        sDeadCTSlotCount = 0;
        
        sFreeNode = aIndex->mEmptyNodeHead;

        if( sFreeNode == SD_NULL_PID )
        {
            break;
        }
        
        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aTrans,
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType )
                  != IDE_SUCCESS );
        sMtxStart = 1;

        IDV_WEARGS_SET( &sWeArgs,
                        IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
                        0,   // WaitParam1
                        0,   // WaitParam2
                        0 ); // WaitParam3
        
        IDE_TEST( aIndex->mSdnHeader.mLatch.lockWrite( aStatistics,
                                                       &sWeArgs )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::push( &sMtx,
                                      (void*)&aIndex->mSdnHeader.mLatch,
                                      SDR_MTX_LATCH_X )
                  != IDE_SUCCESS );

        /*
         * 이전에 설정된 sFreeNode를 그대로 사용하면 안된다.
         * X Latch를 획득한 이후에 FreeNode를 다시 확인해야 함.
         */
        sFreeNode = aIndex->mEmptyNodeHead;

        if( sFreeNode == SD_NULL_PID )
        {
            sMtxStart = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
            break;
        }
        
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mIndexPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       sFreeNode,
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       &sMtx,
                                       (UChar**)&sPage,
                                       &sSuccess )
                  != IDE_SUCCESS );

        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sPage );

        /*
         * EMPTY LIST에 있는 시간중에 다른 트랜잭션에 의해서 키가 삽입된 경우에는
         * EMPTY LIST에서 unlink한다.
         */
        if( sNodeHdr->mUnlimitedKeyCount > 0 )
        {
            IDE_TEST( unlinkEmptyNode( aStatistics,
                                       aIndexStat,
                                       aIndex,
                                       &sMtx,
                                       sPage,
                                       STNDR_IN_USED )
                      != IDE_SUCCESS );

            sMtxStart = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            continue;
        }

        /*
         * TBK KEY들에 대해 Aging을 수행한다.
         */
        if( sNodeHdr->mTBKCount > 0 )
        {
            IDE_TEST( agingAllTBK( aStatistics,
                                   &sMtx,
                                   sPage,
                                   &sSuccess )
                      != IDE_SUCCESS );
        }

        if( sSuccess == ID_FALSE )
        {
            sMtxStart = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            IDE_RAISE( SKIP_AGING );
        }
        
        /*
         * HardKeyStamping을 해서 모든 CTS가 DEAD상태를 만든다.
         */
        sCTL = sdnIndexCTL::getCTL( sPage );

        for( i = 0; i < sdnIndexCTL::getCount(sCTL); i++ )
        {
            sCTS = sdnIndexCTL::getCTS( sCTL, i );

            if( sCTS->mState != SDN_CTS_DEAD )
            {
                IDE_TEST( hardKeyStamping( aStatistics,
                                           aIndex,
                                           &sMtx,
                                           sPage,
                                           i,
                                           &sSuccess )
                          != IDE_SUCCESS );

                if( sSuccess == ID_FALSE )
                {
                    sMtxStart = 0;
                    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

                    IDE_RAISE( SKIP_AGING );
                }
            }

            sDeadCTSlotCount++;
        }

        /*
         * 모든 CTS가 DEAD상태일 경우는 노드를 삭제할수 있는 상태이다.
         */
        if( sDeadCTSlotCount == sdnIndexCTL::getCount(sCTL) )
        {
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPage );
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                        sSlotDirPtr,
                                                        0,
                                                        (UChar**)&sLeafKey)
                      != IDE_SUCCESS );
            sKeyLength = getKeyLength( (UChar *)sLeafKey,
                                       ID_TRUE /* aIsLeaf */ );

            idlOS::memcpy( (UChar*)sTempKeyBuf, sLeafKey, sKeyLength );
            
            STNDR_LKEY_TO_KEYINFO( sLeafKey , sKeyInfo );
            
            sKeyInfo.mKeyValue = STNDR_LKEY_KEYVALUE_PTR( (UChar*)sTempKeyBuf );

            sMtxStart = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

            /*
             * 실제적으로 Node를 삭제한다.
             */
            IDE_TEST( freeNode( aStatistics,
                                aTrans,
                                aIndex,
                                sFreeNode,
                                &sKeyInfo,
                                aIndexStat,
                                &sFreePageCount ) != IDE_SUCCESS );
            
            sAgedCount += sFreePageCount;
        }
        else
        {
            sMtxStart = 0;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
        }

        if( sAgedCount >= aFreePageCount )
        {
            break;
        }
    }

    IDE_EXCEPTION_CONT( SKIP_AGING );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMtxStart == 1 )
    {
        sMtxStart = 0;
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 해당 노드에 있는 모든 TBK 키들을 Aging 한다.                      
 * 해당 함수는 Node Aging에서만 수행되기 때문에 Create한 트랜잭션에 
 * 대해서 Agable 여부를 검사하지 않는다.                      
 *********************************************************************/
IDE_RC stndrRTree::agingAllTBK( idvSQL          * aStatistics,
                                sdrMtx          * aMtx,
                                sdpPhyPageHdr   * aNode,
                                idBool          * aIsSuccess )
{
    UChar           * sSlotDirPtr;
    UShort            sKeyCount;
    stndrLKey       * sLeafKey;
    stndrLKeyEx     * sLeafKeyEx;
    smSCN             sLimitSCN;
    idBool            sIsSuccess = ID_TRUE;
    UInt              i;
    stndrNodeHdr    * sNodeHdr;
    UShort            sDeadKeySize = 0;
    UShort            sDeadTBKCount = 0;
    UShort            sTotalTBKCount = 0;
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    
    sKeyCount = sdpSlotDirectory::getCount( sSlotDirPtr );

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           i,
                                                           (UChar**)&sLeafKey )
                  != IDE_SUCCESS );
        sLeafKeyEx = (stndrLKeyEx*)sLeafKey;
        
        if( STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DEAD )
        {
            continue;
        }
        
        if( STNDR_GET_TB_TYPE( sLeafKey ) == STNDR_KEY_TB_CTS )
        {
            continue;
        }
        
        IDE_DASSERT( STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DELETED );

        /*
         * Create한 트랜잭션의 Agable검사는 하지 않는다.
         * Limit 트랜잭션이 Agable하다고 판단되면 Create도 Agable하다고
         * 판단할수 있다.
         */
        if( STNDR_GET_LCTS_NO( sLeafKey  ) == SDN_CTS_IN_KEY )
        {
            IDE_TEST( getCommitSCN( aStatistics,
                                    NULL,        /* aTrans */
                                    aNode,
                                    sLeafKeyEx,
                                    ID_TRUE,     /* aIsLimitSCN */
                                    SM_SCN_INIT, /* aStmtViewSCN */
                                    &sLimitSCN )
                      != IDE_SUCCESS );

            if( isAgableTBK( sLimitSCN ) == ID_FALSE )
            {
                sIsSuccess = ID_FALSE;
                break;
            }
            else
            {
                STNDR_SET_STATE( sLeafKey , STNDR_KEY_DEAD );
                STNDR_SET_CCTS_NO( sLeafKey , SDN_CTS_INFINITE );
                STNDR_SET_LCTS_NO( sLeafKey , SDN_CTS_INFINITE );

                sDeadTBKCount++;

                IDE_TEST( sdrMiniTrans::writeNBytes(
                              aMtx,
                              (UChar*)sLeafKey ->mTxInfo,
                              (void*)sLeafKey ->mTxInfo,
                              ID_SIZEOF(UChar)*2 )
                          != IDE_SUCCESS );
                
                sDeadKeySize += getKeyLength( (UChar*)sLeafKey ,
                                              ID_TRUE );
                
                sDeadKeySize += ID_SIZEOF( sdpSlotEntry );
            }
        }
    }

    if( sDeadKeySize > 0 )
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );
        
        sDeadKeySize += sNodeHdr->mTotalDeadKeySize;
        
        IDE_TEST(sdrMiniTrans::writeNBytes(
                     aMtx,
                     (UChar*)&sNodeHdr->mTotalDeadKeySize,
                     (void*)&sDeadKeySize,
                     ID_SIZEOF(UShort) )
                 != IDE_SUCCESS );
    }

    if( sDeadTBKCount > 0 )
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );
        
        sTotalTBKCount = sNodeHdr->mTBKCount - sDeadTBKCount;
        
        IDE_TEST(sdrMiniTrans::writeNBytes(
                     aMtx,
                     (UChar*)&sNodeHdr->mTBKCount,
                     (void*)&sTotalTBKCount,
                     ID_SIZEOF(UShort) )
                 != IDE_SUCCESS );
    }

    *aIsSuccess = sIsSuccess;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * TBK 키에 대해서 주어진 CommitSCN이 Agable한지 검사한다.               
 *********************************************************************/
idBool stndrRTree::isAgableTBK( smSCN   aCommitSCN )
{
    smSCN  sSysMinDskViewSCN;
    idBool sIsSuccess = ID_TRUE;

    if( SM_SCN_IS_INFINITE(aCommitSCN) == ID_TRUE )
    {
        sIsSuccess = ID_FALSE;
    }
    else
    {
        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

        /*
         * Restart Undo시에 호출되었거나, Service 상태에서 MinDiskViewSCN
         * 보다 작은 경우는 Aging이 가능하다.
         */
        if( SM_SCN_IS_INIT( sSysMinDskViewSCN ) ||
            SM_SCN_IS_LT( &aCommitSCN, &sSysMinDskViewSCN ) )
        {
            sIsSuccess = ID_TRUE;
        }
        else
        {
            sIsSuccess = ID_FALSE;
        }
    }

    return sIsSuccess;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * DEAD Key중에 하나를 선택해서 삭제할 노드를 찾는다.               
 * 찾은 노드에 키가 하나라도 존재한다면 EMPTY LIST에 연결이후에 다른 
 * 트랜잭션에 의해서 키가 삽입된 경우이기 때문에, 이러한 경우에는    
 * 해당 노드를 EMTPY LIST에서 제거한다.                              
 * 그렇지 않은 경우에는 노드 삭제를 하며 상위 부모 노드에 해당 노드에 대한
 * 키를 삭제한다.
 * Free Node 이후 EMPTY NODE는 FREE LIST로 이동되어 재사용된다.      
 *********************************************************************/
IDE_RC stndrRTree::freeNode( idvSQL         * aStatistics,
                             void           * aTrans,
                             stndrHeader    * aIndex,
                             scPageID         aFreeNodeID,
                             stndrKeyInfo   * aKeyInfo,
                             stndrStatistic * aIndexStat,
                             UInt           * aFreePageCount )
{
    UChar           * sLeafNode;
    SShort            sLeafKeySeq;
    sdrMtx            sMtx;
    idBool            sMtxStart = ID_FALSE;
    idvWeArgs         sWeArgs;
    scPageID          sSegPID;
    stndrPathStack    sStack;
    stndrNodeHdr    * sNodeHdr;
    scPageID          sPID;
    idBool            sIsRetry = ID_FALSE;


    sSegPID = aIndex->mSdnHeader.mSegmentDesc.mSegHandle.mSegPID;

    // init stack
    sStack.mDepth = -1;

    IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                   &sMtx,
                                   aTrans,
                                   SDR_MTX_LOGGING,
                                   ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                   gMtxDLogType )
              != IDE_SUCCESS );
    
    sMtxStart = ID_TRUE;

    IDV_WEARGS_SET( &sWeArgs,
                    IDV_WAIT_INDEX_LATCH_DRDB_RTREE_INDEX_SMO,
                    0,   // WaitParam1
                    0,   // WaitParam2
                    0 ); // WaitParam3

    IDE_TEST( aIndex->mSdnHeader.mLatch.lockWrite( aStatistics,
                                                   &sWeArgs )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::push( &sMtx,
                                  (void*)&aIndex->mSdnHeader.mLatch,
                                  SDR_MTX_LATCH_X )
              != IDE_SUCCESS );

    /*
     * TREE LATCH를 획득하기 이전에 설정된 aFreeNode가 유효한지 검사한다.
     */
    IDE_TEST_RAISE( aIndex->mEmptyNodeHead != aFreeNodeID, SKIP_UNLINK_NODE );
        
    IDE_TEST( traverse( aStatistics,
                        aIndex,
                        &sMtx,
                        NULL, /* aCursorSCN */
                        NULL, /* aFstDiskViewSCN */
                        aKeyInfo,
                        aFreeNodeID,
                        STNDR_TRAVERSE_FREE_NODE,
                        &sStack,
                        aIndexStat,
                        (sdpPhyPageHdr**)&sLeafNode,
                        &sLeafKeySeq )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( sLeafNode == NULL, SKIP_UNLINK_NODE );

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)sLeafNode);

    if( sNodeHdr->mUnlimitedKeyCount == 0 )
    {
        if( sStack.mDepth > 0 )
        {
            sPID = sdpPhyPage::getPageID( (UChar*)sLeafNode );
            sStack.mDepth--;
            IDE_TEST( deleteInternalKey( aStatistics,
                                         aIndex,
                                         aIndexStat,
                                         sSegPID,
                                         sPID,
                                         &sMtx,
                                         &sStack,
                                         aFreePageCount,
                                         &sIsRetry )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( sIsRetry == ID_TRUE, SKIP_UNLINK_NODE );
        }
        else
        {
            IDE_TEST( unsetRootNode( aStatistics,
                                     &sMtx,
                                     aIndex,
                                     aIndexStat )
                      != IDE_SUCCESS );
        }

        IDE_TEST( unlinkEmptyNode( aStatistics,
                                   aIndexStat,
                                   aIndex,
                                   &sMtx,
                                   (sdpPhyPageHdr*)sLeafNode,
                                   STNDR_IN_FREE_LIST )
                  != IDE_SUCCESS );

        IDE_TEST( freePage( aStatistics,
                            aIndexStat,
                            aIndex,
                            &sMtx,
                            sLeafNode )
                  != IDE_SUCCESS );
        
        aIndexStat->mNodeDeleteCount++;
        (*aFreePageCount)++;
    }
    else
    {
        /*
         * EMPTY LIST에 있는 시간중에 다른 트랜잭션에 의해서 키가 삽입된
         * 경우에는 EMPTY LIST에서 unlink한다.
         */
        IDE_TEST( unlinkEmptyNode( aStatistics,
                                   aIndexStat,
                                   aIndex,
                                   &sMtx,
                                   (sdpPhyPageHdr*)sLeafNode,
                                   STNDR_IN_USED )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_UNLINK_NODE );
    
    sMtxStart = ID_FALSE;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMtxStart == ID_TRUE )
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Index에서 Row의 RID와 Key Value를 이용하여 특정 키의 위치를 찾느 함수이다.
 * Traverse가 호출되는 경우는 다음과 같다.
 *
 * traverse가 필요한 경우:
 *      1. freeNode
 *      2. deleteKey
 *      3. insertKeyRollback
 *      4. deleteKeyRollback
 *********************************************************************/
IDE_RC stndrRTree::traverse( idvSQL         * aStatistics,
                             stndrHeader    * aIndex,
                             sdrMtx         * aMtx,
                             smSCN          * aCursorSCN,
                             smSCN          * aFstDiskViewSCN,
                             stndrKeyInfo   * aKeyInfo,
                             scPageID         aFreeNodePID,
                             UInt             aTraverseMode,
                             stndrPathStack * aStack,
                             stndrStatistic * aIndexStat,
                             sdpPhyPageHdr ** aLeafNode,
                             SShort         * aLeafKeySeq )
{
    stndrNodeHdr        * sNodeHdr;
    stndrStack            sTraverseStack;
    stndrStackSlot        sStackSlot;
    stndrIKey           * sIKey;
    stndrVirtualRootNode  sVRootNode;
    sdpPhyPageHdr       * sNode;
    scPageID              sChildPID;
    UChar               * sSlotDirPtr;
    UShort                sKeyCount;
    SShort                sKeySeq;
    ULong                 sIndexSmoNo;
    ULong                 sNodeSmoNo;
    idBool                sIsSuccess;
    idBool                sTraverseStackInit = ID_FALSE;
    stdMBR                sMBR;
    stdMBR                sKeyMBR;
    sdrSavePoint          sSP;
    SInt                  i;
    UInt                  sState = 0;


    IDE_TEST( stndrStackMgr::initialize( &sTraverseStack )
              != IDE_SUCCESS );
    sTraverseStackInit = ID_TRUE;
    
    *aLeafNode = NULL;
    *aLeafKeySeq = STNDR_INVALID_KEY_SEQ; // BUG-29515: Codesonar (Uninitialized Value)

    stndrStackMgr::clear( &sTraverseStack );

    // init stack
    aStack->mDepth = -1;

    getVirtualRootNode( aIndex, &sVRootNode );

    if( sVRootNode.mChildPID != SD_NULL_PID )
    {
        IDE_TEST( stndrStackMgr::push( &sTraverseStack,
                                       sVRootNode.mChildPID,
                                       sVRootNode.mChildSmoNo,
                                       (SShort)STNDR_INVALID_KEY_SEQ )
                  != IDE_SUCCESS );
    }

    IDU_FIT_POINT( "1.PROJ-1591@stndrRTree::traverse" );

    while( stndrStackMgr::getDepth(&sTraverseStack) >= 0 )
    {
        sStackSlot = stndrStackMgr::pop( &sTraverseStack );

        if( sStackSlot.mNodePID == SD_NULL_PID )
        {
            ideLog::log( IDE_SERVER_0,
                         "Index Runtime Header:" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)aIndex,
                            ID_SIZEOF(stndrHeader) );

            ideLog::log( IDE_SERVER_0,
                         "Stack Slot:" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&sStackSlot,
                            ID_SIZEOF(stndrStackSlot) );

            ideLog::log( IDE_SERVER_0,
                         "Traverse Stack Depth %d"
                         ", Traverse Stack Size %d"
                         ", Traverse Stack:",
                         sTraverseStack.mDepth,
                         sTraverseStack.mStackSize );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)sTraverseStack.mStack,
                            ID_SIZEOF(stndrStackSlot) * sTraverseStack.mStackSize );
            IDE_ASSERT( 0 );
        }

        sState = 1;
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndex->mQueryStat.mIndexPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       sStackSlot.mNodePID,
                                       SDB_S_LATCH,
                                       SDB_WAIT_NORMAL,
                                       NULL,
                                       (UChar**)&sNode,
                                       &sIsSuccess )
                  != IDE_SUCCESS );

        sNodeHdr =
            (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sNode );

        if( sdpPhyPage::getIndexSMONo(sNode) > sStackSlot.mSmoNo )
        {
            if( sdpPhyPage::getNxtPIDOfDblList(sNode) == SD_NULL_PID )
            {
                ideLog::log( IDE_SERVER_0,
                             "Index Runtime Header:" );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)aIndex,
                                ID_SIZEOF(stndrHeader) );

                ideLog::log( IDE_SERVER_0,
                             "Stack Slot:" );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)&sStackSlot,
                                ID_SIZEOF(stndrStackSlot) );

                ideLog::log( IDE_SERVER_0,
                             "Traverse Stack Depth %d"
                             ", Traverse Stack Size %d"
                             ", Traverse Stack:",
                             sTraverseStack.mDepth,
                             sTraverseStack.mStackSize );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)sTraverseStack.mStack,
                                ID_SIZEOF(stndrStackSlot) * sTraverseStack.mStackSize );

                dumpIndexNode( sNode );
                IDE_ASSERT( 0 );
            }
            
            IDE_TEST( stndrStackMgr::push( &sTraverseStack,
                                           sdpPhyPage::getNxtPIDOfDblList(sNode),
                                           sStackSlot.mSmoNo,
                                           (SShort)STNDR_INVALID_KEY_SEQ )
                      != IDE_SUCCESS );
            
            aIndexStat->mFollowRightLinkCount++;
        }

        // push path
        while( aStack->mDepth >= 0 )
        {
            if( aStack->mStack[aStack->mDepth].mHeight >
                sNodeHdr->mHeight )
            {
                break;
            }

            aStack->mDepth--;
        }

        aStack->mDepth++;
        aStack->mStack[aStack->mDepth].mNodePID = sStackSlot.mNodePID;
        aStack->mStack[aStack->mDepth].mHeight  = sNodeHdr->mHeight;
        aStack->mStack[aStack->mDepth].mSmoNo   = sdpPhyPage::getIndexSMONo(sNode);
        aStack->mStack[aStack->mDepth].mKeySeq  = sStackSlot.mKeySeq;
        if( aStack->mDepth >= 1 )
        {
            aStack->mStack[aStack->mDepth - 1].mKeySeq = sStackSlot.mKeySeq;
        }

        if( sNodeHdr->mHeight > 0 ) // internal
        {
            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sNode );
            sKeyCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

            getSmoNo( aIndex, &sIndexSmoNo );
            IDL_MEM_BARRIER;
            
            for( i = 0; i < sKeyCount; i++ )
            {
                aIndexStat->mKeyCompareCount++;
                
                IDE_TEST ( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                               sSlotDirPtr,
                                                               i,
                                                               (UChar**)&sIKey )
                           != IDE_SUCCESS );

                STNDR_GET_MBR_FROM_IKEY( sMBR, sIKey );
                STNDR_GET_CHILD_PID( &sChildPID, sIKey );
                STNDR_GET_MBR_FROM_KEYINFO( sKeyMBR, aKeyInfo );

                if( stdUtils::isMBRContains(&sMBR, &sKeyMBR) == ID_TRUE )
                {
                    IDE_TEST( stndrStackMgr::push( &sTraverseStack,
                                                   sChildPID,
                                                   sIndexSmoNo,
                                                   i ) != IDE_SUCCESS );
                }
            }

            sState = 0;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sNode )
                      != IDE_SUCCESS );
        }
        else // leaf
        {
            sNodeSmoNo = sdpPhyPage::getIndexSMONo( sNode );

            sState = 0;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sNode )
                      != IDE_SUCCESS );
            
            
            sdrMiniTrans::setSavePoint( aMtx, &sSP );

            IDE_TEST( stndrRTree::getPage( aStatistics,
                                           &(aIndexStat->mIndexPage),
                                           aIndex->mSdnHeader.mIndexTSID,
                                           sStackSlot.mNodePID,
                                           SDB_X_LATCH,
                                           SDB_WAIT_NORMAL,
                                           aMtx,
                                           (UChar**)&sNode,
                                           &sIsSuccess )
                      != IDE_SUCCESS );
            
            if( sdpPhyPage::getIndexSMONo(sNode) > sNodeSmoNo )
            {
                if( sdpPhyPage::getNxtPIDOfDblList(sNode) == SD_NULL_PID )
                {
                    ideLog::log( IDE_SERVER_0,
                                 "Index Runtime Header:" );
                    ideLog::logMem( IDE_SERVER_0,
                                    (UChar*)aIndex,
                                    ID_SIZEOF(stndrHeader) );

                    ideLog::log( IDE_SERVER_0,
                                 "Stack Slot:" );
                    ideLog::logMem( IDE_SERVER_0,
                                    (UChar*)&sStackSlot,
                                    ID_SIZEOF(stndrStackSlot) );

                    ideLog::log( IDE_SERVER_0,
                                 "Traverse Stack Depth %d"
                                 ", Traverse Stack Size %d"
                                 ", Traverse Stack:",
                                 sTraverseStack.mDepth,
                                 sTraverseStack.mStackSize );
                    ideLog::logMem( IDE_SERVER_0,
                                    (UChar*)sTraverseStack.mStack,
                                    ID_SIZEOF(stndrStackSlot) * sTraverseStack.mStackSize );

                    dumpIndexNode( sNode );
                    IDE_ASSERT( 0 );
                }
            
                IDE_TEST( stndrStackMgr::push( &sTraverseStack,
                                               sdpPhyPage::getNxtPIDOfDblList(sNode),
                                               sNodeSmoNo,
                                               sStackSlot.mKeySeq )
                          != IDE_SUCCESS );
                
                aIndexStat->mFollowRightLinkCount++;
            }

            if( aTraverseMode == STNDR_TRAVERSE_FREE_NODE )
            {
                IDE_ASSERT(aFreeNodePID != SD_NULL_PID);
                
                if( sdpPhyPage::getPageID((UChar*)sNode) == aFreeNodePID )
                {
                    *aLeafNode   = sNode;
                    *aLeafKeySeq = STNDR_INVALID_KEY_SEQ;

                    aStack->mStack[aStack->mDepth].mKeySeq = STNDR_INVALID_KEY_SEQ;
                    break;
                }
            }
            else
            {
                IDE_TEST( findLeafKey( aIndex,
                                       aMtx,
                                       aCursorSCN,
                                       aFstDiskViewSCN,
                                       aTraverseMode,
                                       sNode,
                                       aIndexStat,
                                       aKeyInfo,
                                       &sKeySeq,
                                       &sIsSuccess ) != IDE_SUCCESS );
            
                if( sIsSuccess == ID_TRUE )
                {
                    *aLeafNode   = sNode;
                    *aLeafKeySeq = sKeySeq;

                    aStack->mStack[aStack->mDepth].mKeySeq = sKeySeq;
                    break;
                }
            } // if( aTraverseMode == STNDR_TRAVERSE_FREE_NODE )

            sdrMiniTrans::releaseLatchToSP( aMtx, &sSP );
        }
    } // while

    sTraverseStackInit = ID_FALSE;
    IDE_TEST( stndrStackMgr::destroy( &sTraverseStack )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        if( sdbBufferMgr::releasePage( aStatistics, (UChar*)sNode )
            != IDE_SUCCESS )
        {
            dumpIndexNode( sNode );
            IDE_ASSERT( 0 );
        }
    }

    if( sTraverseStackInit == ID_TRUE )
    {
        sTraverseStackInit = ID_FALSE;
        if( stndrStackMgr::destroy( &sTraverseStack )
            != IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0,
                         "Index Runtime Header:" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)aIndex,
                            ID_SIZEOF(stndrHeader) );

            ideLog::log( IDE_SERVER_0,
                         "Stack Slot:" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&sStackSlot,
                            ID_SIZEOF(stndrStackSlot) );

            ideLog::log( IDE_SERVER_0,
                         "Traverse Stack Depth %d"
                         ", Traverse Stack Size %d"
                         ", Traverse Stack:",
                         sTraverseStack.mDepth,
                         sTraverseStack.mStackSize );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)sTraverseStack.mStack,
                            ID_SIZEOF(stndrStackSlot) * sTraverseStack.mStackSize );
            IDE_ASSERT( 0 );
        }
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 주어진 Leaf Node에서 Row의 RID와 Key Value를 이용하여 대상 키를 찾는다.
 * 기본적으로 RID와 Key Value를 비교하며, 탐색 대상에 따라 다음과 같은 비교가
 * 추가로 수행된다.
 *  1. delete key
 *     키 상태가 STABLE 또는 UNSTABLE 상태
 *  2. insert key rollback
 *     키 상태가 UNSTABLE
 *  3. delete key rollback
 *     키 상태가 DELETED
 *     키의 LimitSCN이 CursorSCN과 동일
 *     MyTransaction
 *********************************************************************/
IDE_RC stndrRTree::findLeafKey( stndrHeader     * /*aIndex*/,
                                sdrMtx          * aMtx,
                                smSCN           * aCursorSCN,
                                smSCN           * aFstDiskViewSCN,
                                UInt              aTraverseMode,
                                sdpPhyPageHdr   * aNode,
                                stndrStatistic  * aIndexStat,
                                stndrKeyInfo    * aKeyInfo,
                                SShort          * aKeySeq,
                                idBool          * aIsSuccess )
{
    UChar           * sSlotDirPtr;
    UShort            sKeyCount;
    stndrLKey       * sLKey;
    stndrKeyInfo      sLKeyInfo;
    smSCN             sCreateSCN;
    smSCN             sLimitSCN;
    stdMBR            sMBR;
    stdMBR            sKeyMBR;
    UInt              i;
    smSCN             sBeginSCN;
    sdSID             sTSSlotSID;

    
    *aIsSuccess = ID_FALSE;
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    for( i = 0; i < sKeyCount; i++ )
    {
        aIndexStat->mKeyCompareCount++;
        
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           (UChar**)&sLKey )
                  != IDE_SUCCESS );

        STNDR_LKEY_TO_KEYINFO( sLKey, sLKeyInfo );

        if( (sLKeyInfo.mRowPID     == aKeyInfo->mRowPID) &&
            (sLKeyInfo.mRowSlotNum == aKeyInfo->mRowSlotNum) )
        {
            STNDR_GET_MBR_FROM_LKEY( sMBR, sLKey );
            STNDR_GET_MBR_FROM_KEYINFO( sKeyMBR, aKeyInfo );

            STNDR_GET_CSCN( sLKey, &sCreateSCN );
            STNDR_GET_LSCN( sLKey, &sLimitSCN );

            aIndexStat->mKeyCompareCount++;
            
            if( stdUtils::isMBRContains(&sMBR, &sKeyMBR) != ID_TRUE )
            {
                continue;
            }

            switch( aTraverseMode )
            {
                case STNDR_TRAVERSE_DELETE_KEY:
                {
                    if( STNDR_GET_STATE(sLKey) == STNDR_KEY_STABLE ||
                        STNDR_GET_STATE(sLKey) == STNDR_KEY_UNSTABLE )
                    {
                        *aKeySeq = i;
                        *aIsSuccess = ID_TRUE;
                    }
                }
                break;
                
                case STNDR_TRAVERSE_INSERTKEY_ROLLBACK:
                {
                    if( STNDR_GET_STATE(sLKey) == STNDR_KEY_UNSTABLE )
                    {
                        *aKeySeq = i;
                        *aIsSuccess = ID_TRUE;
                    }
                }
                break;
                
                case STNDR_TRAVERSE_DELETEKEY_ROLLBACK:
                {
                    IDE_ASSERT( aCursorSCN != NULL );
                
                    if( STNDR_GET_STATE(sLKey) == STNDR_KEY_DELETED &&
                        SM_SCN_IS_EQ(&sLimitSCN, aCursorSCN) )
                    {
                        if( STNDR_GET_LCTS_NO( sLKey ) == SDN_CTS_IN_KEY )
                        {
                            STNDR_GET_TBK_LSCN( ((stndrLKeyEx*)sLKey ), &sBeginSCN );
                            STNDR_GET_TBK_LTSS( ((stndrLKeyEx*)sLKey ), &sTSSlotSID );
                            
                            if( isMyTransaction( aMtx->mTrans,
                                                 sBeginSCN,
                                                 sTSSlotSID,
                                                 aFstDiskViewSCN ) )
                            {
                                *aKeySeq = i;
                                *aIsSuccess = ID_TRUE;
                            }
                        }
                        else
                        {
                            if( sdnIndexCTL::isMyTransaction( aMtx->mTrans,
                                                              aNode,
                                                              STNDR_GET_LCTS_NO(sLKey),
                                                              aFstDiskViewSCN ) )
                            {
                                *aKeySeq = i;
                                *aIsSuccess = ID_TRUE;
                            }
                        }
                    }
                }
                break;
                
                default:
                    ideLog::log( IDE_SERVER_0,
                                 "Traverse Mode: %u\n",
                                 aTraverseMode );
                    IDE_ASSERT( 0 );
                    break;
            }

            if( *aIsSuccess == ID_TRUE )
            {
                break;
            }
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * Transaction Level에서 Transaction의 Snapshot에 속한 키인지        
 * 확인한다. 해당 함수는 Key에 있는 트랜잭션 정보만으로 Visibility를
 * 확인할수 있는 경우인지를 검사한다. 만약 판단이 서지 않는다면      
 * Cursor Level visibility를 검사해야 한다.                         
 *                                                                   
 * 아래의 경우가 Visibility를 확인할수 없는 경우이다.               
 *                                                                   
 * 1. CreateSCN이 Shnapshot.SCN보다 작은 Duplicate Key를 만났을 경우 
 * 2. 자신이 삽입/삭제한 키를 만났을 경우                            
 *                                                                   
 * 위의 경우를 제외한 모든 키는 아래와 같이 4가지 경우로 분류된다.   
 *                                                                   
 * 1. LimitSCN < CreateSCN < StmtSCN                                 
 *    : LimitSCN이 Upper Bound SCN인 경우가 CreateSCN보다 LimitSCN이 
 *      더 작을 수 있다. Upper Bound SCN은 "0"이다.                  
 *      해당 경우는 "Visible = TRUE"이다.                            
 *                                                                   
 * 2. CreateSCN < StmtSCN < LimitSCN                                 
 *    : 아직 삭제되지 않았기 때문에 "Visible = TRUE"이다.            
 *                                                                   
 * 3. CreateSCN < LimitSCN < StmtSCN                                
 *    : 이미 삭제된 키이기 때문에 "Visible = FALSE"이다.             
 *                                                                   
 * 4. StmtSCN < CreateSCN < LimitSCN                                 
 *    : Select가 시작할 당시 삽입이 되지도 않았었던 키이기 때문에    
 *      "Visible = FALSE"이다.                                       
 *********************************************************************/
IDE_RC stndrRTree::tranLevelVisibility( idvSQL          * aStatistics,
                                        void            * aTrans,
                                        UChar           * aNode,
                                        UChar           * aLeafKey ,
                                        smSCN           * aStmtViewSCN,
                                        idBool          * aIsVisible,
                                        idBool          * aIsUnknown )
{
    stndrLKey               * sLeafKey;
    stndrKeyInfo              sKeyInfo;
    smSCN                     sCreateSCN;
    smSCN                     sLimitSCN;
    smSCN                     sBeginSCN;
    sdSID                     sTSSlotSID;
    
    sLeafKey  = (stndrLKey*)aLeafKey;
    STNDR_LKEY_TO_KEYINFO( sLeafKey, sKeyInfo );

    if( STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DEAD )
    {
        *aIsVisible = ID_FALSE;
        *aIsUnknown = ID_FALSE;
        IDE_RAISE( RETURN_SUCCESS );
    }

    if( STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_STABLE )
    {
        *aIsVisible = ID_TRUE;
        *aIsUnknown = ID_FALSE;
        IDE_RAISE( RETURN_SUCCESS );
    }
    
    if( STNDR_GET_CCTS_NO( sLeafKey ) == SDN_CTS_INFINITE )
    {
        SM_INIT_SCN( &sCreateSCN );
        if( STNDR_GET_LCTS_NO( sLeafKey ) == SDN_CTS_INFINITE )
        {
            ideLog::log( IDE_SERVER_0, "Leaf Key:\n" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar *)sLeafKey,
                            ID_SIZEOF( stndrLKey ) );
            dumpIndexNode( (sdpPhyPageHdr *)aNode );
            IDE_ASSERT( 0 );
        }
    }
    else
    {
        if( STNDR_GET_CCTS_NO( sLeafKey ) == SDN_CTS_IN_KEY )
        {
            IDE_TEST( getCommitSCN( aStatistics,
                                    aTrans,
                                    (sdpPhyPageHdr*)aNode,
                                    (stndrLKeyEx*)sLeafKey ,
                                    ID_FALSE, /* aIsLimitSCN */
                                    *aStmtViewSCN,
                                    &sCreateSCN )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sdnIndexCTL::getCommitSCN( aStatistics,
                                                 aTrans,
                                                 (sdpPhyPageHdr*)aNode,
                                                 SDB_SINGLE_PAGE_READ,
                                                 STNDR_GET_CCTS_NO( sLeafKey ),
                                                 *aStmtViewSCN,
                                                 &sCreateSCN )
                      != IDE_SUCCESS );
        }
    }

    if( STNDR_GET_LCTS_NO( sLeafKey ) == SDN_CTS_INFINITE )
    {
        SM_MAX_SCN( &sLimitSCN );
    }
    else
    {
        if( STNDR_GET_LCTS_NO( sLeafKey ) == SDN_CTS_IN_KEY )
        {
            IDE_TEST( getCommitSCN( aStatistics,
                                    aTrans,
                                    (sdpPhyPageHdr*)aNode,
                                    (stndrLKeyEx*)sLeafKey ,
                                    ID_TRUE, /* aIsLimitSCN */
                                    *aStmtViewSCN,
                                    &sLimitSCN )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( sdnIndexCTL::getCommitSCN( aStatistics,
                                                 aTrans,
                                                 (sdpPhyPageHdr*)aNode,
                                                 SDB_SINGLE_PAGE_READ,
                                                 STNDR_GET_LCTS_NO( sLeafKey ),
                                                 *aStmtViewSCN,
                                                 &sLimitSCN )
                      != IDE_SUCCESS );
        }
    }

    if( SM_SCN_IS_INFINITE( sCreateSCN ) == ID_TRUE )
    {
        if( STNDR_GET_CCTS_NO( sLeafKey ) == SDN_CTS_IN_KEY )
        {
            STNDR_GET_TBK_CSCN( ((stndrLKeyEx*)sLeafKey ), &sBeginSCN );
            STNDR_GET_TBK_CTSS( ((stndrLKeyEx*)sLeafKey ), &sTSSlotSID );
            
            if( isMyTransaction( aTrans, sBeginSCN, sTSSlotSID ) )
            {
                *aIsVisible = ID_FALSE;
                *aIsUnknown = ID_TRUE;
                IDE_RAISE( RETURN_SUCCESS );
            }
        }
        else
        {
            if( sdnIndexCTL::isMyTransaction( aTrans,
                                              (sdpPhyPageHdr*)aNode,
                                              STNDR_GET_CCTS_NO( sLeafKey ) )
                == ID_TRUE )
            {
                *aIsVisible = ID_FALSE;
                *aIsUnknown = ID_TRUE;
                IDE_RAISE( RETURN_SUCCESS );
            }
        }
    }
    else
    {
        /******************************************************************
         * PROJ-1381 - FAC로 커밋한 key에 대해서는 visibility를 unkown으로
         * 설정해서 table에 조회해 봄으로써 visibility를 결정해야 한다.
         *
         * FAC fetch cursor가 물고 있는 Trans 객체는 FAC로 커밋할 당시의
         * 트랜잭션이 아니라, FAC 커밋 이후에 새로 begin한 트랜잭션이다.
         * 또한 index에서는 key를 insert할 당시의 infinite SCN을 남겨두지도
         * 않는다. 따라서 FAC fetch cursor는 FAC로 커밋한 key를 발견하더라도,
         * 자신이 insert한 key인지 여부를 확인할 수 없다.
         *
         * 때문에 FAC로 커밋한 key를 발견하면 infinite SCN을 유지하는
         * table로부터 자신이 볼 수 있는 key가 맞는지 확인해 보아야 한다.
         *****************************************************************/
        if( SDC_CTS_SCN_IS_LEGACY(sCreateSCN) == ID_TRUE )
        {
            *aIsVisible = ID_FALSE;
            *aIsUnknown = ID_TRUE;
            IDE_RAISE( RETURN_SUCCESS );
        }

        if( SM_SCN_IS_INFINITE( sLimitSCN ) == ID_TRUE )
        {
            if( STNDR_GET_LCTS_NO( sLeafKey ) == SDN_CTS_IN_KEY )
            {
                STNDR_GET_TBK_LSCN( ((stndrLKeyEx*)sLeafKey ), &sBeginSCN );
                STNDR_GET_TBK_LTSS( ((stndrLKeyEx*)sLeafKey ), &sTSSlotSID );
                
                if( isMyTransaction( aTrans, sBeginSCN, sTSSlotSID ) )
                {
                    *aIsVisible = ID_FALSE;
                    *aIsUnknown = ID_TRUE;
                    IDE_RAISE( RETURN_SUCCESS );
                }
            }
            else
            {
                if( sdnIndexCTL::isMyTransaction( aTrans,
                                                  (sdpPhyPageHdr*)aNode,
                                                  STNDR_GET_LCTS_NO( sLeafKey ) )
                    == ID_TRUE )
                {
                    *aIsVisible = ID_FALSE;
                    *aIsUnknown = ID_TRUE;
                    IDE_RAISE( RETURN_SUCCESS );
                }
            }

            if( SM_SCN_IS_LT( &sCreateSCN, aStmtViewSCN ) )
            {
                /*
                 *      CreateSCN < StmtSCN
                 * 
                 *             -----------
                 *             |              
                 *  ----------------------------
                 *          Create   ^
                 *                  Stmt
                 */
                *aIsVisible = ID_TRUE;
                *aIsUnknown = ID_FALSE;
                IDE_RAISE( RETURN_SUCCESS );
            }
        }
        else
        {
            /* PROJ-1381 - Fetch Across Commits */
            if( SDC_CTS_SCN_IS_LEGACY(sLimitSCN) == ID_TRUE )
            {
                *aIsVisible = ID_FALSE;
                *aIsUnknown = ID_TRUE;
                IDE_RAISE( RETURN_SUCCESS );
            }

            if( SM_SCN_IS_LT( &sCreateSCN, aStmtViewSCN ) )
            {
                if( SM_SCN_IS_LT( aStmtViewSCN, &sLimitSCN ) )
                {
                    /*
                     * CreateSCN < StmtSCN < LimitSCN
                     * 
                     *          ----------------
                     *          |              |
                     *  ----------------------------
                     *        Create    ^     Limit
                     *                 Stmt  
                     */
                    *aIsVisible = ID_TRUE;
                    *aIsUnknown = ID_FALSE;
                    IDE_RAISE( RETURN_SUCCESS );
                }
                else
                {
                    /*
                     *     LimitSCN < StmtSCN
                     *     
                     *          ---------
                     *          |       |
                     *  ----------------------------
                     *        Create  Limit    ^ 
                     *                        Stmt  
                     */
                    *aIsVisible = ID_FALSE;
                    *aIsUnknown = ID_FALSE;
                    IDE_RAISE( RETURN_SUCCESS );
                }
            }
        }

    }

    /*
     *     StmtSCN < CreateSCN
     *     
     *              ---------
     *              |       |
     *  ----------------------------
     *      ^     Create  Limit
     *     Stmt  
     */
    *aIsVisible = ID_FALSE;
    *aIsUnknown = ID_FALSE;

    
    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 커서 레벨에서 Transaction의 Snapshot에 속한 키인지 확인한다.
 * 다음의 경우에 볼 수 있는 키이다.
 *
 * sCreateSCN < aInfiniteSCN < sLimitSCN
 *********************************************************************/
IDE_RC stndrRTree::cursorLevelVisibility( stndrLKey * aLeafKey,
                                          smSCN     * aInfiniteSCN,
                                          idBool    * aIsVisible )
{
    smSCN   sCreateSCN;
    smSCN   sLimitSCN;


    STNDR_GET_CSCN( aLeafKey, &sCreateSCN );

    if( SM_SCN_IS_CI_INFINITE( sCreateSCN ) == ID_TRUE )
    {
        SM_INIT_CI_SCN( &sCreateSCN );
    }

    STNDR_GET_LSCN( aLeafKey, &sLimitSCN );

    if( SM_SCN_IS_CI_INFINITE( sLimitSCN ) == ID_TRUE )
    {
        SM_MAX_CI_SCN( &sLimitSCN );
    }

    if( SM_SCN_IS_LT( &sCreateSCN, aInfiniteSCN ) )
    {
        if( SM_SCN_IS_LT( aInfiniteSCN, &sLimitSCN ) )
        {
            *aIsVisible = ID_TRUE;
            IDE_RAISE( RETURN_SUCCESS );
        }
    }

    *aIsVisible = ID_FALSE;

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Internal Node에서 해당 Child를 가리키는 키를 삭제한다.
 * 본 함수는 Child로부터 키가 삭제될 Internal Node까지 X-Latch가 잡힌 상태
 * 에서 수행된다.
 * 본 함수에 의해 Internal Node의 모든 키가 삭제될 경우 상위 부모 노드의 키
 * 삭제를 재귀적으로 요청한다.
 *********************************************************************/
IDE_RC stndrRTree::deleteInternalKey( idvSQL            * aStatistics,
                                      stndrHeader       * aIndex,
                                      stndrStatistic    * aIndexStat,
                                      scPageID            aSegPID,
                                      scPageID            aChildPID,
                                      sdrMtx            * aMtx,
                                      stndrPathStack    * aStack,
                                      UInt              * aFreePageCount,
                                      idBool            * aIsRetry )
{
    UShort            sKeyCount;
    sdpPhyPageHdr   * sNode;
    SShort            sSeq;
    UChar           * sSlotDirPtr;
    UChar           * sFreeKey;
    UShort            sKeyLen;
    scPageID          sPID;
    stdMBR            sDummy;
    stdMBR            sNodeMBR;
    IDE_RC            sRc;

    
    // x latch current internal node
    IDE_TEST( getParentNode( aStatistics,
                             aIndexStat,
                             aMtx,
                             aIndex,
                             aStack,
                             aChildPID,
                             &sPID,
                             &sNode,
                             &sSeq,
                             &sDummy,
                             aIsRetry )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( *aIsRetry == ID_TRUE, RETURN_SUCCESS );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    if( sKeyCount == 1 )
    {
        if( aStack->mDepth == 0 )
        {
            IDE_TEST( unsetRootNode( aStatistics,
                                     aMtx,
                                     aIndex,
                                     aIndexStat )
                      != IDE_SUCCESS );
        }
        else
        {
            aStack->mDepth--;
            IDE_TEST( deleteInternalKey( aStatistics,
                                         aIndex,
                                         aIndexStat,
                                         aSegPID,
                                         sPID,
                                         aMtx,
                                         aStack,
                                         aFreePageCount,
                                         aIsRetry )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( *aIsRetry == ID_TRUE, RETURN_SUCCESS );
        }

        IDE_TEST( freePage( aStatistics,
                            aIndexStat,
                            aIndex,
                            aMtx,
                            (UChar*)sNode )
                  != IDE_SUCCESS );
        
        aIndexStat->mNodeDeleteCount++;

        (*aFreePageCount)++;
    }
    else
    {
        // propagate key value
        IDE_ASSERT( adjustNodeMBR( aIndex,
                                   sNode,
                                   NULL,                   /* aInsertMBR */
                                   STNDR_INVALID_KEY_SEQ,  /* aUpdateKeySeq */
                                   NULL,                   /* aUpdateMBR */
                                   sSeq,                   /* aDeleteKeySeq */
                                   &sNodeMBR )
                    == IDE_SUCCESS );

        aStack->mDepth--;
        IDE_TEST( propagateKeyValue( aStatistics,
                                     aIndexStat,
                                     aIndex,
                                     aMtx,
                                     aStack,
                                     sNode,
                                     &sNodeMBR,
                                     aIsRetry )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( *aIsRetry == ID_TRUE, RETURN_SUCCESS );

        sRc = setNodeMBR( aMtx, sNode, &sNodeMBR );

        if( sRc != IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "update MBR : \n" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&sNodeMBR,
                            ID_SIZEOF(stdMBR) );
            dumpIndexNode( sNode );
            IDE_ASSERT( 0 );
        }

        // free slot
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           sSeq,
                                                           &sFreeKey )
                  != IDE_SUCCESS );
        
        sKeyLen = getKeyLength( (UChar*)sFreeKey,
                                ID_FALSE /* aIsLeaf */);

        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sFreeKey,
                                             (void*)&sKeyLen,
                                             ID_SIZEOF(UShort),
                                             SDR_STNDR_FREE_INDEX_KEY )
                  != IDE_SUCCESS );

        sdpPhyPage::freeSlot( sNode,
                              sSeq,
                              sKeyLen,
                              1  /* aSlotAlignValue */ );
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 메타페이지에서 Root Node로의 link를 삭제한다.
 * Runtime Header의 Root Node를 SD_NULL_PID로 설정한다.
 *********************************************************************/
IDE_RC stndrRTree::unsetRootNode( idvSQL            * aStatistics,
                                  sdrMtx            * aMtx,
                                  stndrHeader       * aIndex,
                                  stndrStatistic    * aIndexStat )
{
    IDE_TEST( backupRuntimeHeader( aMtx, aIndex ) != IDE_SUCCESS );

    aIndex->mRootNode = SD_NULL_PID;

    IDE_TEST( setIndexMetaInfo( aStatistics,
                                aIndex,
                                aIndexStat,
                                aMtx,
                                &aIndex->mRootNode,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL,
                                NULL )
              != IDE_SUCCESS );

    aIndex->mInitTreeMBR = ID_FALSE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * 주어진 Empty Node들을 List에 연결한다.                            
 * 동시성 문제로 페이지 래치 획득이 실패했다면 조금 기다렸다가      
 * 다시 시도한다. 
 *********************************************************************/
IDE_RC stndrRTree::linkEmptyNodes( idvSQL           * aStatistics,
                                   void             * aTrans,
                                   stndrHeader      * aIndex,
                                   stndrStatistic   * aIndexStat,
                                   scPageID         * aEmptyNodePID )
{
    sdrMtx            sMtx;
    idBool            sMtxStart = ID_FALSE;
    stndrNodeHdr    * sNodeHdr;
    UInt              sEmptyNodeSeq;
    sdpPhyPageHdr   * sPage;
    idBool            sIsSuccess;

    for( sEmptyNodeSeq = 0; sEmptyNodeSeq < 2; )
    {
        if( aEmptyNodePID[sEmptyNodeSeq] == SD_NULL_PID )
        {
            sEmptyNodeSeq++;
            continue;
        }

        IDE_TEST( sdrMiniTrans::begin( aStatistics,
                                       &sMtx,
                                       aTrans,
                                       SDR_MTX_LOGGING,
                                       ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                       gMtxDLogType ) != IDE_SUCCESS );
        sMtxStart = ID_TRUE;
    
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mIndexPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       aEmptyNodePID[sEmptyNodeSeq],
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       &sMtx,
                                       (UChar**)&sPage,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
        
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sPage );
        
        if( sNodeHdr->mUnlimitedKeyCount != 0 )
        {
            sMtxStart = ID_FALSE;
            IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );
            
            sEmptyNodeSeq++;
            continue;
        }
        
        IDE_TEST( linkEmptyNode( aStatistics,
                                 aIndexStat,
                                 aIndex,
                                 &sMtx,
                                 sPage,
                                 &sIsSuccess )
                  != IDE_SUCCESS );

        sMtxStart = ID_FALSE;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        if( sIsSuccess == ID_TRUE )
        {
            sEmptyNodeSeq++;
        }
        else
        {
            /*
             * 동시성 문제로 페이지 래치 획득이 실패했다면 조금 기다렸다가
             * 다시 시도한다.
             */
            idlOS::thr_yield();
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sMtxStart == ID_TRUE )
    {
        sMtxStart = ID_FALSE;
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * EMPTY LIST에 해당 노드를 연결한다.                                
 * 이미 EMPTY LIST나 FREE LIST에 연결되어 있는 노드라면 SKIP하고,    
 * 그렇지 않은 경우는 link에 연결한다.
 *********************************************************************/
IDE_RC stndrRTree::linkEmptyNode( idvSQL            * aStatistics,
                                  stndrStatistic    * aIndexStat,
                                  stndrHeader       * aIndex,
                                  sdrMtx            * aMtx,
                                  sdpPhyPageHdr     * aNode,
                                  idBool            * aIsSuccess )
{
    sdpPhyPageHdr   * sTailPage;
    scPageID          sPageID;
    stndrNodeHdr    * sCurNodeHdr;
    stndrNodeHdr    * sTailNodeHdr;
    idBool            sIsSuccess;
    UChar             sNodeState;
    scPageID          sNullPID = SD_NULL_PID;
    UChar           * sMetaPage;
    

    *aIsSuccess = ID_TRUE;

    sPageID  = sdpPhyPage::getPageID( (UChar*)aNode );
    
    sCurNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

    IDE_TEST_RAISE( (sCurNodeHdr->mState == STNDR_IN_EMPTY_LIST) ||
                    (sCurNodeHdr->mState == STNDR_IN_FREE_LIST),
                    SKIP_LINKING );

    /*
     * Index runtime Header의 empty node list정보는
     * Meta Page의 latch에 의해서 보호된다.
     */
    sMetaPage = sdrMiniTrans::getPagePtrFromPageID(
        aMtx,
        aIndex->mSdnHeader.mIndexTSID,
        SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ));
    
    if( sMetaPage == NULL )
    {
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mMetaPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ),
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       aMtx,
                                       (UChar**)&sMetaPage,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
    }
    
    if( aIndex->mEmptyNodeHead == SD_NULL_PID )
    {
        IDE_DASSERT( aIndex->mEmptyNodeTail == SD_NULL_PID );
        aIndex->mEmptyNodeHead =
            aIndex->mEmptyNodeTail = sPageID;

        IDE_TEST( setIndexEmptyNodeInfo( aStatistics,
                                         aIndex,
                                         aIndexStat,
                                         aMtx,
                                         &aIndex->mEmptyNodeHead,
                                         &aIndex->mEmptyNodeTail )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_DASSERT( aIndex->mEmptyNodeTail != SD_NULL_PID );

        /*
         * Deadlock을 피하기 위해서 tail page에 getPage를 try한다.
         * 만약 실패한다면 모든 연산을 다시 수행해야 한다.
         */
        sTailPage = (sdpPhyPageHdr*)
            sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                aIndex->mSdnHeader.mIndexTSID,
                                                aIndex->mEmptyNodeTail);
        if( sTailPage == NULL )
        {
            IDE_TEST( stndrRTree::getPage( aStatistics,
                                           &(aIndexStat->mIndexPage),
                                           aIndex->mSdnHeader.mIndexTSID,
                                           aIndex->mEmptyNodeTail,
                                           SDB_X_LATCH,
                                           SDB_WAIT_NO,
                                           aMtx,
                                           (UChar**)&sTailPage,
                                           aIsSuccess) != IDE_SUCCESS );

            IDE_TEST_RAISE( *aIsSuccess == ID_FALSE,
                            SKIP_LINKING );
        }
        
        sTailNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*)sTailPage);

        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&sTailNodeHdr->mNextEmptyNode,
                      (void*)&sPageID,
                      ID_SIZEOF(sPageID) ) != IDE_SUCCESS );
        aIndex->mEmptyNodeTail = sPageID;

        IDE_TEST( setIndexEmptyNodeInfo( aStatistics,
                                         aIndex,
                                         aIndexStat,
                                         aMtx,
                                         NULL,
                                         &aIndex->mEmptyNodeTail )
                  != IDE_SUCCESS );
    }

    sNodeState = STNDR_IN_EMPTY_LIST;
    
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(sCurNodeHdr->mState),
                  (void*)&sNodeState,
                  ID_SIZEOF(sNodeState) ) != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(sCurNodeHdr->mNextEmptyNode),
                  (void*)&sNullPID,
                  ID_SIZEOF(sNullPID) ) != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_LINKING );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * EMPTY LIST에서 해당 노드를 삭제한다. 
 *********************************************************************/
IDE_RC stndrRTree::unlinkEmptyNode( idvSQL          * aStatistics,
                                    stndrStatistic  * aIndexStat,
                                    stndrHeader     * aIndex,
                                    sdrMtx          * aMtx,
                                    sdpPhyPageHdr   * aNode,
                                    UChar             aNodeState )
{
    stndrNodeHdr    * sNodeHdr;
    UChar           * sMetaPage;
    idBool            sIsSuccess = ID_TRUE;
    

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)aNode);

    /*
     * FREE LIST에 연결해야 하는 경우에는 반드시 현재 상태가
     * EMPTY LIST에 연결되어 있는 상태여야 한다.
     */
    if( (aNodeState != STNDR_IN_USED) &&
        (sNodeHdr->mState != STNDR_IN_EMPTY_LIST) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Node state : %u"
                     "\nNode header state : %u\n",
                     aNodeState, sNodeHdr->mState );
        dumpIndexNode( aNode );
        IDE_ASSERT( 0 );
    }
    
    IDE_TEST_RAISE( sNodeHdr->mState == STNDR_IN_USED,
                    SKIP_UNLINKING );
    
    if( aIndex->mEmptyNodeHead == SD_NULL_PID )
    {
        ideLog::log( IDE_SERVER_0, "Index header dump:\n" );
        ideLog::logMem( IDE_SERVER_0, (UChar *)aIndex, ID_SIZEOF(stndrHeader) );
        dumpIndexNode( aNode );
        IDE_ASSERT( 0 );
    }

    /*
     * Index runtime Header의 empty node list정보는
     * Meta Page의 latch에 의해서 보호된다.
     */
    sMetaPage = sdrMiniTrans::getPagePtrFromPageID(
        aMtx,
        aIndex->mSdnHeader.mIndexTSID,
        SD_MAKE_PID(aIndex->mSdnHeader.mMetaRID) );
    
    if( sMetaPage == NULL )
    {
        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mMetaPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ),
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       aMtx,
                                       (UChar**)&sMetaPage,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
    }
    
    aIndex->mEmptyNodeHead = sNodeHdr->mNextEmptyNode;

    if( sNodeHdr->mNextEmptyNode == SD_NULL_PID )
    {
        aIndex->mEmptyNodeHead = SD_NULL_PID;
        aIndex->mEmptyNodeTail = SD_NULL_PID;

        IDE_TEST( setIndexEmptyNodeInfo( aStatistics,
                                         aIndex,
                                         aIndexStat,
                                         aMtx,
                                         &aIndex->mEmptyNodeHead,
                                         &aIndex->mEmptyNodeTail )
                  != IDE_SUCCESS );
    }
    else
    {
        aIndex->mEmptyNodeHead = sNodeHdr->mNextEmptyNode;

        IDE_TEST( setIndexEmptyNodeInfo( aStatistics,
                                         aIndex,
                                         aIndexStat,
                                         aMtx,
                                         &aIndex->mEmptyNodeHead,
                                         NULL )
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sNodeHdr->mState),
                                         (void*)&aNodeState,
                                         ID_SIZEOF(aNodeState) )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( SKIP_UNLINKING );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            *
 * ------------------------------------------------------------------*
 * index segment의 첫번째 페이지(META PAGE)에 변경된 Empty Node List를
 * 실제로 로깅한다. 
 *********************************************************************/
IDE_RC stndrRTree::setIndexEmptyNodeInfo( idvSQL            * aStatistics,
                                          stndrHeader       * aIndex,
                                          stndrStatistic    * aIndexStat,
                                          sdrMtx            * aMtx,
                                          scPageID          * aEmptyNodeHead,
                                          scPageID          * aEmptyNodeTail )
{
    UChar       * sPage;
    stndrMeta   * sMeta;
    idBool        sIsSuccess;
    

    sPage = sdrMiniTrans::getPagePtrFromPageID(
        aMtx,
        aIndex->mSdnHeader.mIndexTSID,
        SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ));
    
    if( sPage == NULL )
    {
        // SegHdr 페이지 포인터를 구함
        IDE_TEST( stndrRTree::getPage(
                      aStatistics,
                      &(aIndexStat->mMetaPage),
                      aIndex->mSdnHeader.mIndexTSID,
                      SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ),
                      SDB_X_LATCH,
                      SDB_WAIT_NORMAL,
                      aMtx,
                      (UChar**)&sPage,
                      &sIsSuccess ) != IDE_SUCCESS );
    }

    sMeta = (stndrMeta*)( sPage + SMN_INDEX_META_OFFSET );

    if( aEmptyNodeHead != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&sMeta->mEmptyNodeHead,
                      (void*)aEmptyNodeHead,
                      ID_SIZEOF(*aEmptyNodeHead) )
                  != IDE_SUCCESS );
    }
    
    if( aEmptyNodeTail != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&sMeta->mEmptyNodeTail,
                      (void*)aEmptyNodeTail,
                      ID_SIZEOF(*aEmptyNodeTail) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Unlimited Key가 0인 노드들을 찾는다. 
 * Split 이후에 호출되기 때문에 현재과 다음 노드만을 검사한다. 
 *********************************************************************/
void stndrRTree::findEmptyNodes( sdrMtx         * aMtx,
                                 stndrHeader    * aIndex,
                                 sdpPhyPageHdr  * aStartPage,
                                 scPageID       * aEmptyNodePID,
                                 UInt           * aEmptyNodeCount )
{
    sdpPhyPageHdr   * sCurPage;
    sdpPhyPageHdr   * sPrevPage;
    stndrNodeHdr    * sNodeHdr;
    UInt              sEmptyNodeSeq = 0;
    UInt              i;
    scPageID          sNextPID;
    

    aEmptyNodePID[0] = SD_NULL_PID;
    aEmptyNodePID[1] = SD_NULL_PID;

    sCurPage = aStartPage;

    for( i = 0; i < 2; i++ )
    {
        if( sCurPage == NULL )
        {
            if( i == 1 )
            {
                ideLog::log( IDE_SERVER_0,
                             "Index TSID : %u"
                             ", Get page ID : %u\n",
                             aIndex->mSdnHeader.mIndexTSID, sNextPID );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar *)aIndex, ID_SIZEOF(stndrHeader) );
                dumpIndexNode( sPrevPage );
            }
            IDE_ASSERT( 0 );
        }
        
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr((UChar*)sCurPage);

        if( sNodeHdr->mUnlimitedKeyCount == 0 )
        {
            aEmptyNodePID[sEmptyNodeSeq] = sCurPage->mPageID;
            sEmptyNodeSeq++;
        }
        
        sNextPID = sdpPhyPage::getNxtPIDOfDblList( sCurPage );
        
        if( sNextPID != SD_NULL_PID )
        {
            sPrevPage = sCurPage;
            sCurPage = (sdpPhyPageHdr*)
                sdrMiniTrans::getPagePtrFromPageID( aMtx,
                                                    aIndex->mSdnHeader.mIndexTSID,
                                                    sNextPID );
        }
    }

    *aEmptyNodeCount = sEmptyNodeSeq;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 새로운 Key를 Insert하기 위해 Leaf Node를 Split하고 키가 Insert될 Node를
 * 반환한다.
 *
 * Split 이 수행되는 경우는 다음과 같다.
 *  o insertKey 시에 Leaf Node에 새로운 키를 삽입할 공간이 없을 경우
 *  o deleteKey 시에 Leaf Node에 TBK 타입으로 delete를 수행할때 공간이
 *    없을 경우
 *
 * Split 방식(aSplitMode)은 아래 두가지를 지원하며 디폴트로는 RStart 방식의
 * Split을 수행 한다.
 *********************************************************************/
IDE_RC stndrRTree::splitLeafNode( idvSQL            * aStatistics,
                                  stndrStatistic    * aIndexStat,
                                  sdrMtx            * aMtx,
                                  idBool            * aMtxStart,
                                  stndrHeader       * aIndex,
                                  smSCN             * aInfiniteSCN,
                                  stndrPathStack    * aStack,
                                  sdpPhyPageHdr     * aNode,
                                  stndrKeyInfo      * aKeyInfo,
                                  UShort              aKeyValueLen,
                                  SShort              aKeySeq,
                                  idBool              aIsInsert,
                                  UInt                aSplitMode,
                                  sdpPhyPageHdr    ** aTargetNode,
                                  SShort            * aTargetKeySeq,
                                  UInt              * aAllocPageCount,
                                  idBool            * aIsRetry )
{
    stndrKeyInfo      sOldNodeKeyInfo;
    stndrKeyInfo      sNewNodeKeyInfo;
    stndrKeyInfo    * sInsertKeyInfo;
    stndrKeyArray   * sKeyArray = NULL;
    UShort            sKeyArrayCnt;
    sdpPhyPageHdr   * sNewNode;
    idBool            sInsertKeyOnNewPage;
    idBool            sDeleteKeyOnNewPage;
    SShort            sInsertKeySeq;
    SShort            sDeleteKeySeq;
    SShort            sInsertKeyIdx;
    UShort            sSplitPoint;
    stdMBR            sOldNodeMBR;
    stdMBR            sNewNodeMBR;
    stdMBR            sNodeMBR;
    ULong             sPrevSmoNo;
    ULong             sNewSmoNo;
    IDE_RC            sRc;


    if( aIsInsert == ID_TRUE )
    {
        sInsertKeyInfo = aKeyInfo;
        sInsertKeySeq  = sdpSlotDirectory::getCount(
            sdpPhyPage::getSlotDirStartPtr((UChar*)aNode) );
        sDeleteKeySeq  = STNDR_INVALID_KEY_SEQ;
    }
    else
    {
        sInsertKeyInfo = NULL;
        sInsertKeySeq  = STNDR_INVALID_KEY_SEQ;
        sDeleteKeySeq  = aKeySeq;
    }
    
    // make split group
    IDE_TEST( makeKeyArray( NULL,                  /* aUpdateKeyInfo */
                            STNDR_INVALID_KEY_SEQ  /* aUpdateKeySeq */,
                            sInsertKeyInfo,
                            sInsertKeySeq,
                            aNode,
                            &sKeyArray,
                            &sKeyArrayCnt )
              != IDE_SUCCESS );

    makeSplitGroup( aIndex,
                    aSplitMode,
                    sKeyArray,
                    sKeyArrayCnt,
                    &sSplitPoint,
                    &sOldNodeMBR,
                    &sNewNodeMBR );

    adjustKeySeq( sKeyArray,
                  sKeyArrayCnt,
                  sSplitPoint,
                  &sInsertKeySeq,
                  NULL, /* aUpdateKeySeq */
                  &sDeleteKeySeq,
                  &sInsertKeyOnNewPage,
                  NULL, /* aUpdateKeyOnNewPage */
                  &sDeleteKeyOnNewPage );

    // move slot 시에 제외
    if( sInsertKeySeq != STNDR_INVALID_KEY_SEQ )
    {
        if( sInsertKeyOnNewPage == ID_TRUE )
        {
            sInsertKeyIdx = sInsertKeySeq + (sSplitPoint + 1);
        }
        else
        {
            sInsertKeyIdx = sInsertKeySeq;
        }

        sKeyArray[sInsertKeyIdx].mKeySeq = STNDR_INVALID_KEY_SEQ;
    }

    //
    sOldNodeKeyInfo.mKeyValue = (UChar *)&sOldNodeMBR;
    sNewNodeKeyInfo.mKeyValue = (UChar *)&sNewNodeMBR;

    // propagation
    aStack->mDepth--;
    IDE_TEST( propagateKeyInternalNode( aStatistics,
                                        aIndexStat,
                                        aMtx,
                                        aMtxStart,
                                        aIndex,
                                        aInfiniteSCN,
                                        aStack,
                                        aKeyValueLen,
                                        &sOldNodeKeyInfo,
                                        aNode,
                                        &sNewNodeKeyInfo,
                                        aSplitMode,
                                        &sNewNode,
                                        aAllocPageCount,
                                        aIsRetry )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( *aIsRetry == ID_TRUE, SKIP_SPLIT_LEAF );

    // initialize new node
    sPrevSmoNo = sdpPhyPage::getIndexSMONo( aNode );

    IDE_TEST( increaseSmoNo( aStatistics, aIndex, &sNewSmoNo )
              != IDE_SUCCESS );

    sdpPhyPage::setIndexSMONo( (UChar*)aNode, sNewSmoNo );
    sdpPhyPage::setIndexSMONo( (UChar*)sNewNode, sPrevSmoNo );

    IDE_TEST( initializeNodeHdr( aMtx,
                                 sNewNode,
                                 0,
                                 aIndex->mSdnHeader.mLogging )
              != IDE_SUCCESS );

    IDE_TEST( sdnIndexCTL::init( aMtx,
                                 &aIndex->mSdnHeader.mSegmentDesc.mSegHandle,
                                 sNewNode,
                                 sdnIndexCTL::getUsedCount(aNode) )
              != IDE_SUCCESS );

    // distribute keys between old node and new node
    IDE_ASSERT( moveSlots( aMtx,
                           aIndex,
                           aNode,
                           sKeyArray,
                           sSplitPoint + 1,
                           sKeyArrayCnt - 1,
                           sNewNode ) == IDE_SUCCESS );

    // adjust node mbr
    if( adjustNodeMBR( aIndex,
                       aNode,
                       NULL,                   /* aInsertMBR */
                       STNDR_INVALID_KEY_SEQ,  /* aUpdateKeySeq */
                       NULL,                   /* aUpdateMBR */
                       STNDR_INVALID_KEY_SEQ,  /* aDeleteKeySeq */
                       &sNodeMBR ) == IDE_SUCCESS )
    {
        sRc = setNodeMBR( aMtx, aNode, &sNodeMBR );
        if( sRc != IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "update MBR : \n" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&sNodeMBR,
                            ID_SIZEOF(stdMBR) );
            dumpIndexNode( aNode );
            IDE_ASSERT( 0 );
        }
    }

    if( adjustNodeMBR( aIndex,
                       sNewNode,
                       NULL,                   /* aInsertMBR */
                       STNDR_INVALID_KEY_SEQ,  /* aUpdateKeySeq */
                       NULL,                   /* aUpdateMBR */
                       STNDR_INVALID_KEY_SEQ,  /* aDeleteKeySeq */
                       &sNodeMBR ) == IDE_SUCCESS )
    {
        sRc = setNodeMBR( aMtx, sNewNode, &sNodeMBR );
        if( sRc != IDE_SUCCESS )
        {
            ideLog::log( IDE_SERVER_0, "update MBR : \n" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar*)&sNodeMBR,
                            ID_SIZEOF(stdMBR) );
            dumpIndexNode( sNewNode );
            IDE_ASSERT( 0 );
        }
    }

    // BUG-29560: 한 노드에 대해 두번 Split 발생시 이전 링크를 연결하지 않는 문제
    sdpDblPIDList::setNxtOfNode( &sNewNode->mListNode,
                                 aNode->mListNode.mNext,
                                 aMtx );
    
    sdpDblPIDList::setNxtOfNode( &aNode->mListNode,
                                 sNewNode->mPageID,
                                 aMtx );

    // set target node
    if( aIsInsert == ID_TRUE )
    {
        *aTargetNode =
            ( sInsertKeyOnNewPage == ID_TRUE ) ? sNewNode : aNode;
        
        *aTargetKeySeq = sInsertKeySeq;
    }
    else
    {
        *aTargetNode =
            ( sDeleteKeyOnNewPage == ID_TRUE ) ? sNewNode : aNode;
        
        *aTargetKeySeq = sDeleteKeySeq;
    }

    IDE_EXCEPTION_CONT( SKIP_SPLIT_LEAF );

    if(sKeyArray != NULL)
    {
        (void)iduMemMgr::free( sKeyArray );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sKeyArray != NULL)
    {
        (void)iduMemMgr::free( sKeyArray );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Node MBR을 갱신한다.
 *********************************************************************/
IDE_RC stndrRTree::setNodeMBR( sdrMtx        * aMtx,
                               sdpPhyPageHdr * aNode,
                               stdMBR        * aMBR )
{
    stndrNodeHdr    * sNodeHdr;

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );
    
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(sNodeHdr->mMBR.mMinX),
                  (void*)&aMBR->mMinX,
                  ID_SIZEOF(aMBR->mMinX))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(sNodeHdr->mMBR.mMinY),
                  (void*)&aMBR->mMinY,
                  ID_SIZEOF(aMBR->mMinY))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(sNodeHdr->mMBR.mMaxX),
                  (void*)&aMBR->mMaxX,
                  ID_SIZEOF(aMBR->mMaxX))
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&(sNodeHdr->mMBR.mMaxY),
                  (void*)&aMBR->mMaxY,
                  ID_SIZEOF(aMBR->mMaxY))
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 노드의 Split으로 인한 변경 사항을 상위 노드에 전파한다. 노드가 Split 되면
 * 두 개의 노드가 생성되고 이로 인한 변경 사항이 KeyInfo로 전달되어 호출된다.
 * aUpdateKeyInfo 에는 분할대상 노드 정보가, aInsertKeyInfo 에는 분할되어 
 * 새로 생성된 노드 정보가 전달된다.
 *
 * 최상위 노드에까지 propgation 된다면 Stack의 Depth가 -1이다. 이 때 Root
 * Node가 변경되었는지 확인하고, 변경되었다면, 다시 Insert를 수행하도록 한다.
 * 키 삽입을 위하여 경로 탐색 후, Root Node가 새로 생성되었다면, Stack에
 * 해당 Root 정보가 없으므로 키 변경이 반영되지 않을수 있는 문제가 있기
 * 때문이다.
 *
 * 현재 Stack Depth가 -1이 아니라면 분할 대상 노드를 가리키는 부모 노드를
 * 찾아 X-Latch를 잡고, 부모 노드에 기존 노드를 가리키는 키를 업데이트하고
 * 새로 생성된 노드에 대한 키를 삽입한다.
 * 
 * 부모 노드에 키를 삽입할 공간이 없으면 Split을 수행한다.
 * 
 * propagation은 변경 가능한 최상위 부모 노드까지 상위로 올라가며 X-Latch가
 * 잡힌 상태에서 최상위 노드로부터 하위 노드 방향으로 변경 작업이 수행된다.
 * Split의 경우 상위에서 새로운 자식 노드를 할당하여 내려주고 하위에서 내려준 
 * 자식 노드를 사용한다.
 *
 * 부모 노드에 키가 업데이트 되거나, 새로운 키가 삽입되면 노드의 MBR이 변경될
 * 수 있으므로, 노드 MBR의 변경 사항을 반영한다.
 *********************************************************************/
IDE_RC stndrRTree::propagateKeyInternalNode( idvSQL         * aStatistics,
                                             stndrStatistic * aIndexStat,
                                             sdrMtx         * aMtx,
                                             idBool         * aMtxStart,
                                             stndrHeader    * aIndex,
                                             smSCN          * aInfiniteSCN,
                                             stndrPathStack * aStack,
                                             UShort           aKeyValueLen,
                                             stndrKeyInfo   * aUpdateKeyInfo,
                                             sdpPhyPageHdr  * aUpdateChildNode,
                                             stndrKeyInfo   * aInsertKeyInfo,
                                             UInt             aSplitMode,
                                             sdpPhyPageHdr ** aNewChildNode,
                                             UInt           * aAllocPageCount,
                                             idBool         * aIsRetry )
{
    sdpPhyPageHdr   * sNode = NULL;
    scPageID          sPID;
    scPageID          sNewChildPID;
    stndrNodeHdr    * sNodeHdr = NULL;
    SShort            sKeySeq;
    UShort            sKeyLength;
    idBool            sInsertable = ID_TRUE;
    stdMBR            sUpdateMBR;
    stdMBR            sInsertMBR;
    stdMBR            sNodeMBR;
    stdMBR            sDummy;
    IDE_RC            sRc;
    
    
    if( aStack->mDepth == -1 )
    {
        if( aStack->mStack[0].mNodePID != aIndex->mRootNode )
        {
            *aIsRetry = ID_TRUE;
            IDE_RAISE( RETURN_SUCCESS );
        }
        
        IDE_TEST( makeNewRootNode( aStatistics,
                                   aIndexStat,
                                   aMtx,
                                   aMtxStart,
                                   aIndex,
                                   aInfiniteSCN,
                                   aUpdateKeyInfo,
                                   aUpdateChildNode,
                                   aInsertKeyInfo,
                                   aKeyValueLen,
                                   aNewChildNode,
                                   aAllocPageCount )
                  != IDE_SUCCESS );
    }
    else
    {
        sKeyLength = STNDR_IKEY_LEN( aKeyValueLen );
                
        IDE_TEST( getParentNode( aStatistics,
                                 aIndexStat,
                                 aMtx,
                                 aIndex,
                                 aStack,
                                 sdpPhyPage::getPageID((UChar*)aUpdateChildNode),
                                 &sPID,
                                 &sNode,
                                 &sKeySeq,
                                 &sDummy,
                                 aIsRetry )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( *aIsRetry == ID_TRUE, RETURN_SUCCESS );

        // node로 부터 slot 영역을 할당받는다(sSlot)
        if( canAllocInternalKey( aMtx,
                                 aIndex,
                                 sNode,
                                 sKeyLength,
                                 ID_FALSE,
                                 ID_FALSE )
            != IDE_SUCCESS )
        {
            sInsertable = ID_FALSE;
        }

        if( sInsertable != ID_TRUE )
        {
            IDE_TEST( splitInternalNode( aStatistics,
                                         aIndexStat,
                                         aMtx,
                                         aMtxStart,
                                         aIndex,
                                         aInfiniteSCN,
                                         aStack,
                                         aKeyValueLen,
                                         aUpdateKeyInfo,
                                         sKeySeq,
                                         aInsertKeyInfo,
                                         aSplitMode,
                                         sNode,
                                         aNewChildNode,
                                         aAllocPageCount,
                                         aIsRetry )
                      != IDE_SUCCESS );
        }
        else
        {
            sNodeHdr = (stndrNodeHdr*)
                sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sNode );
            
            IDE_TEST( preparePages( aStatistics,
                                    aIndex,
                                    aMtx,
                                    aMtxStart,
                                    sNodeHdr->mHeight )
                      != IDE_SUCCESS );
            
            IDE_ASSERT( allocPage( aStatistics,
                                   aIndexStat,
                                   aIndex,
                                   aMtx,
                                   (UChar**)aNewChildNode )
                        == IDE_SUCCESS );
            
            (*aAllocPageCount)++;

            sNewChildPID = sdpPhyPage::getPageID((UChar*)*aNewChildNode);

            STNDR_GET_MBR_FROM_KEYINFO( sUpdateMBR, aUpdateKeyInfo );
            STNDR_GET_MBR_FROM_KEYINFO( sInsertMBR, aInsertKeyInfo );

            IDE_ASSERT( adjustNodeMBR( aIndex,
                                       sNode,
                                       &sInsertMBR,
                                       sKeySeq,               /* aUpdateKeySeq */
                                       &sUpdateMBR,
                                       STNDR_INVALID_KEY_SEQ, /* aDeleteKeySeq */
                                       &sNodeMBR )
                        == IDE_SUCCESS );

            aStack->mDepth--;
            IDE_TEST( propagateKeyValue( aStatistics,
                                         aIndexStat,
                                         aIndex,
                                         aMtx,
                                         aStack,
                                         sNode,
                                         &sNodeMBR,
                                         aIsRetry )
                      != IDE_SUCCESS );

            IDE_TEST_RAISE( *aIsRetry == ID_TRUE, RETURN_SUCCESS );

            sRc = updateIKey( aMtx,
                              sNode,
                              sKeySeq,
                              &sUpdateMBR,
                              aIndex->mSdnHeader.mLogging );

            if( sRc != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0,
                             "Key sequence : %d"
                             ", Key value length : %u"
                             ", Update MBR: \n",
                             sKeySeq, aKeyValueLen );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)&sUpdateMBR,
                                ID_SIZEOF(sUpdateMBR) );
                dumpIndexNode( sNode );
                IDE_ASSERT( 0 );
            }

            sKeySeq = sdpSlotDirectory::getCount(
                sdpPhyPage::getSlotDirStartPtr((UChar*)sNode) );

            sRc = insertIKey( aMtx,
                              aIndex,
                              sNode,
                              sKeySeq,
                              aInsertKeyInfo,
                              aKeyValueLen,
                              sNewChildPID,
                              aIndex->mSdnHeader.mLogging );
            
            if( sRc != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0,
                             "Key sequence : %d"
                             ", Key value length : %u"
                             ", New child PID : %u"
                             ", Key Info : \n",
                             sKeySeq, aKeyValueLen, sNewChildPID );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)aInsertKeyInfo,
                                ID_SIZEOF(stndrKeyInfo) );
                
                dumpIndexNode( sNode );
                
                IDE_ASSERT( 0 );
            }

            sRc = setNodeMBR( aMtx, sNode, &sNodeMBR );
            if( sRc != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0, "update MBR : \n" );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)&sNodeMBR,
                                ID_SIZEOF(stdMBR) );
                dumpIndexNode( sNode );
                IDE_ASSERT( 0 );
            }
        }        
    }
    
    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Internal Node에 대한 Split을 수행한다. Leaf Node와 마찬가지로 Split
 * Mode에 따라 RTree, RStar Tree 방식으로 분할된다.
 *
 * Split 시에 업데이트 되는 키와 삽입되는 키 정보가 aUpdateKeyInfo,
 * aInsertKeyInfo를 통해 전달된다. 이 키들을 모두 고려하여 노드를 2개의
 * Split 그룹으로 분할한다. 분할된 노드에 대한 정보를 KeyInfo로 구성하여
 * 상위 노드에 전파한다. propagateKeyInternalNode에서 분할된 자식 노드를
 * 실제로 내려 주고, 원본 노드에서 Split 그룹에 따라서 키를 새로운 자식 노드로
 * 이동 시킨다.
 *
 * 노드가 분할되어 2개의 노드가 되는데, splitInternalNode 호출시에 전달된
 * 키를 Split 그룹에 따라 변경된 위치에 각각 반영한다. 이때 노드 MBR의 변경도
 * 같이 반영한다.
 *
 * R-Link를 위해 SmoNo를 증가 시키고, 새로운 자식 노드에는 분활대상 노드의
 * SmoNo를 설정하고, 분할 대상 노드에는 증가시킨 SmoNo를 설정한 후 분할
 * 대상 노드의 오른쪽 링크를 새로 생성된 자식 노드를 가리키도록 한다.
 *********************************************************************/
IDE_RC stndrRTree::splitInternalNode( idvSQL            * aStatistics,
                                      stndrStatistic    * aIndexStat,
                                      sdrMtx            * aMtx,
                                      idBool            * aMtxStart,
                                      stndrHeader       * aIndex,
                                      smSCN             * aInfiniteSCN,
                                      stndrPathStack    * aStack,
                                      UShort              aKeyValueLen,
                                      stndrKeyInfo      * aUpdateKeyInfo,
                                      SShort              aUpdateKeySeq,
                                      stndrKeyInfo      * aInsertKeyInfo,
                                      UInt                aSplitMode,
                                      sdpPhyPageHdr     * aNode,
                                      sdpPhyPageHdr    ** aNewChildNode,
                                      UInt              * aAllocPageCount,
                                      idBool            * aIsRetry )
{
    stndrKeyArray   * sKeyArray = NULL;
    sdpPhyPageHdr   * sNewNode;
    sdpPhyPageHdr   * sTargetNode;
    UShort            sKeyArrayCnt;
    stndrKeyInfo      sOldNodeKeyInfo;
    stndrKeyInfo      sNewNodeKeyInfo;
    idBool            sInsertKeyOnNewPage;
    idBool            sUpdateKeyOnNewPage;
    SShort            sUpdateKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort            sInsertKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort            sInsertKeyIdx;
    UShort            sSplitPoint;
    stdMBR            sOldNodeMBR;
    stdMBR            sNewNodeMBR;
    stdMBR            sUpdateMBR;
    stdMBR            sNodeMBR;
    ULong             sPrevSmoNo;
    ULong             sNewSmoNo;
    stndrNodeHdr*     sNodeHdr;
    IDE_RC            sRc;


    sInsertKeySeq = sdpSlotDirectory::getCount(
        sdpPhyPage::getSlotDirStartPtr((UChar*)aNode) );
    
    sUpdateKeySeq = aUpdateKeySeq;

    // make split group
    IDE_TEST( makeKeyArray( aUpdateKeyInfo,
                            sUpdateKeySeq,
                            aInsertKeyInfo,
                            sInsertKeySeq,
                            aNode,
                            &sKeyArray,
                            &sKeyArrayCnt )
              != IDE_SUCCESS );

    makeSplitGroup( aIndex,
                    aSplitMode,
                    sKeyArray,
                    sKeyArrayCnt,
                    &sSplitPoint,
                    &sOldNodeMBR,
                    &sNewNodeMBR );
    
    adjustKeySeq( sKeyArray,
                  sKeyArrayCnt,
                  sSplitPoint,
                  &sInsertKeySeq,
                  &sUpdateKeySeq,
                  NULL, /* aDeleteKeySeq */
                  &sInsertKeyOnNewPage,
                  &sUpdateKeyOnNewPage,
                  NULL /* aDeteteKeyOnNewPage */ );

    // move slot 시에 제외
    if( sInsertKeyOnNewPage == ID_TRUE )
    {
        sInsertKeyIdx = sInsertKeySeq + (sSplitPoint + 1);
    }
    else
    {
        sInsertKeyIdx = sInsertKeySeq;
    }
    
    sKeyArray[sInsertKeyIdx].mKeySeq = STNDR_INVALID_KEY_SEQ;

    //
    sOldNodeKeyInfo.mKeyValue = (UChar *)&sOldNodeMBR;
    sNewNodeKeyInfo.mKeyValue = (UChar *)&sNewNodeMBR;
    
    // propagation
    aStack->mDepth--;
    IDE_TEST( propagateKeyInternalNode( aStatistics,
                                        aIndexStat,
                                        aMtx,
                                        aMtxStart,
                                        aIndex,
                                        aInfiniteSCN,
                                        aStack,
                                        aKeyValueLen,
                                        &sOldNodeKeyInfo,
                                        aNode,
                                        &sNewNodeKeyInfo,
                                        aSplitMode,
                                        &sNewNode,
                                        aAllocPageCount,
                                        aIsRetry )
              != IDE_SUCCESS );

    IDE_TEST_RAISE( *aIsRetry == ID_TRUE, SKIP_SPLIT_INTERNAL );

    // 새 Child 노드를 할당받는다.
    IDE_ASSERT( allocPage( aStatistics,
                           aIndexStat,
                           aIndex,
                           aMtx,
                           (UChar**)aNewChildNode )
                == IDE_SUCCESS );
    
    (*aAllocPageCount)++;

    // initialize new node
    sPrevSmoNo = sdpPhyPage::getIndexSMONo( aNode );

    IDE_TEST( increaseSmoNo( aStatistics, aIndex, &sNewSmoNo )
              != IDE_SUCCESS );

    sdpPhyPage::setIndexSMONo( (UChar*)aNode, sNewSmoNo );
    sdpPhyPage::setIndexSMONo( (UChar*)sNewNode, sPrevSmoNo );

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

    IDE_TEST( initializeNodeHdr( aMtx,
                                 sNewNode,
                                 sNodeHdr->mHeight,
                                 aIndex->mSdnHeader.mLogging )
              != IDE_SUCCESS );
    
    // distribute keys between old node and new node
    sRc = moveSlots( aMtx,
                     aIndex,
                     aNode,
                     sKeyArray,
                     sSplitPoint + 1,
                     sKeyArrayCnt - 1,
                     sNewNode );
    
    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0,
                     "move from <%d> to <%u>"
                     "\nIndex depth : %d"
                     ", key array count : %d"
                     ", key array : \n",
                     sSplitPoint + 1, sKeyArrayCnt - 1,
                     aStack->mDepth, sKeyArrayCnt );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)sKeyArray,
                        ID_SIZEOF(stndrKeyArray) * sKeyArrayCnt );
        dumpIndexNode( aNode );
        dumpIndexNode( sNewNode );
        IDE_ASSERT( 0 );
    }

    // insert key
    sTargetNode = ( sInsertKeyOnNewPage == ID_TRUE ) ? sNewNode : aNode;
        
    sRc = insertIKey( aMtx,
                      aIndex,
                      sTargetNode,
                      sInsertKeySeq,
                      aInsertKeyInfo,
                      aKeyValueLen,
                      sdpPhyPage::getPageID((UChar*)*aNewChildNode),
                      aIndex->mSdnHeader.mLogging );

    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0,
                     "Key sequence number : %d"
                     ", Key value length : %u"
                     ", New child PID : %u"
                     ", Key Info: \n",
                     sInsertKeySeq,
                     aKeyValueLen,
                     sdpPhyPage::getPageID((UChar*)*aNewChildNode) );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)aInsertKeyInfo,
                        ID_SIZEOF(stndrKeyInfo) );
        dumpIndexNode( sTargetNode );
        IDE_ASSERT( 0 );
    }

    // Update Internal Key
    // 반드시 키 삽입 후 업데이트를 하도록 한다.
    // 그렇지 않으면 잘못된 Key Seq에 업데이트 될 수 있다.
    sTargetNode = ( sUpdateKeyOnNewPage == ID_TRUE ) ? sNewNode : aNode;
    STNDR_GET_MBR_FROM_KEYINFO( sUpdateMBR, aUpdateKeyInfo );
    sRc = updateIKey( aMtx,
                      sTargetNode,
                      sUpdateKeySeq,
                      &sUpdateMBR,
                      aIndex->mSdnHeader.mLogging );
    
    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0,
                     "Key sequence number: %d"
                     ", Key value length : %u"
                     ", update MBR : \n",
                     sUpdateKeySeq, aKeyValueLen );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sUpdateMBR,
                        ID_SIZEOF(sUpdateMBR) );
                
        dumpIndexNode( sTargetNode );
        IDE_ASSERT( 0 );
    }
    
    // adjust node mbr
    IDE_ASSERT( adjustNodeMBR( aIndex,
                               aNode,
                               NULL,                   /* aInsertMBR */
                               STNDR_INVALID_KEY_SEQ,  /* aUpdateKeySeq */
                               NULL,                   /* aUpdateMBR */
                               STNDR_INVALID_KEY_SEQ,  /* aDeleteKeySeq */
                               &sNodeMBR ) == IDE_SUCCESS );
    
    sRc = setNodeMBR( aMtx, aNode, &sNodeMBR );
    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0, "update MBR : \n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sNodeMBR,
                        ID_SIZEOF(stdMBR) );
        dumpIndexNode( aNode );
        IDE_ASSERT( 0 );
    }
    
    IDE_ASSERT( adjustNodeMBR( aIndex,
                               sNewNode,
                               NULL,                   /* aInsertMBR */
                               STNDR_INVALID_KEY_SEQ,  /* aUpdateKeySeq */
                               NULL,                   /* aUpdateMBR */
                               STNDR_INVALID_KEY_SEQ,  /* aDeleteKeySeq */
                               &sNodeMBR ) == IDE_SUCCESS );
    
    sRc = setNodeMBR( aMtx, sNewNode, &sNodeMBR );
    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0, "update MBR : \n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sNodeMBR,
                        ID_SIZEOF(stdMBR) );
        dumpIndexNode( sNewNode );
        IDE_ASSERT( 0 );
    }

    // BUG-29560: 한 노드에 대해 두번 Split 발생시 이전 링크를 연결하지 않는 문제
    sdpDblPIDList::setNxtOfNode( &sNewNode->mListNode,
                                 aNode->mListNode.mNext,
                                 aMtx );

    sdpDblPIDList::setNxtOfNode( &aNode->mListNode,
                                 sNewNode->mPageID,
                                 aMtx );

    IDE_EXCEPTION_CONT( SKIP_SPLIT_INTERNAL );
    
    if( sKeyArray != NULL )
    {
        IDE_TEST( iduMemMgr::free( sKeyArray ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sKeyArray != NULL )
    {
        (void)iduMemMgr::free( sKeyArray );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 분할 되는 노드에는 삽일될 키, 삭제될 키, 업데이트 될 키가 존재할 수 있다.
 * Split이 수행되면 이들의 KeySeq가 변경될 수 있으면, Split 후의 Key Seq를
 * 조정해준다.
 *********************************************************************/
void stndrRTree::adjustKeySeq( stndrKeyArray    * aKeyArray,
                               UShort             aKeyArrayCnt,
                               UShort             aSplitPoint,
                               SShort           * aInsertKeySeq,
                               SShort           * aUpdateKeySeq,
                               SShort           * aDeleteKeySeq,
                               idBool           * aInsertKeyOnNewPage,
                               idBool           * aUpdateKeyOnNewPage,
                               idBool           * aDeleteKeyOnNewPage )
{
    SShort  sNewInsertKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort  sNewUpdateKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort  sNewDeleteKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort  sInsertKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort  sUpdateKeySeq = STNDR_INVALID_KEY_SEQ;
    SShort  sDeleteKeySeq = STNDR_INVALID_KEY_SEQ;
    idBool  sInsertKeyOnNewPage = ID_TRUE;
    idBool  sUpdateKeyOnNewPage = ID_TRUE;
    idBool  sDeleteKeyOnNewPage = ID_TRUE;
    UInt    i;

    if( aInsertKeySeq != NULL)
    {
        sInsertKeySeq = *aInsertKeySeq;
    }

    if( aUpdateKeySeq != NULL)
    {
        sUpdateKeySeq = *aUpdateKeySeq;
    }

    if( aDeleteKeySeq != NULL)
    {
        sDeleteKeySeq = *aDeleteKeySeq;
    }
    
    sUpdateKeyOnNewPage = ID_TRUE;
    sInsertKeyOnNewPage = ID_TRUE;
    sDeleteKeyOnNewPage = ID_TRUE;
    
    for( i = 0; i <= aSplitPoint; i++ )
    {
        if( aKeyArray[i].mKeySeq == sInsertKeySeq )
        {
            sNewInsertKeySeq = i;
            sInsertKeyOnNewPage = ID_FALSE;
        }

        if( aKeyArray[i].mKeySeq == sUpdateKeySeq )
        {
            sNewUpdateKeySeq = i;
            sUpdateKeyOnNewPage = ID_FALSE;
        }

        if( aKeyArray[i].mKeySeq == sDeleteKeySeq )
        {
            sNewDeleteKeySeq = i;
            sDeleteKeyOnNewPage = ID_FALSE;
        }
    }

    for( i = aSplitPoint + 1; i < aKeyArrayCnt; i++ )
    {
        if( aKeyArray[i].mKeySeq == sInsertKeySeq )
        {
            sNewInsertKeySeq = i - (aSplitPoint + 1);
        }

        if( aKeyArray[i].mKeySeq == sUpdateKeySeq )
        {
            sNewUpdateKeySeq = i - (aSplitPoint + 1);
        }

        if( aKeyArray[i].mKeySeq == sDeleteKeySeq )
        {
            sNewDeleteKeySeq = i - (aSplitPoint + 1 );
        }
    }

    // set value
    if( aInsertKeySeq != NULL )
    {
        *aInsertKeySeq = sNewInsertKeySeq;
    }

    if( aUpdateKeySeq != NULL )
    {
        *aUpdateKeySeq = sNewUpdateKeySeq;
    }

    if( aDeleteKeySeq != NULL )
    {
        *aDeleteKeySeq = sNewDeleteKeySeq;
    }

    if( aInsertKeyOnNewPage != NULL )
    {
        *aInsertKeyOnNewPage = sInsertKeyOnNewPage;
    }

    if( aUpdateKeyOnNewPage != NULL )
    {
        *aUpdateKeyOnNewPage = sUpdateKeyOnNewPage;
    }
    
    if( aDeleteKeyOnNewPage != NULL )
    {
        *aDeleteKeyOnNewPage = sDeleteKeyOnNewPage;
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 노드를 분할하기 위한 Key Array를 할당한다. 할당된 Key Array를 통하여
 * Split 그룹이 생성된다. 
 *********************************************************************/
IDE_RC stndrRTree::makeKeyArray( stndrKeyInfo   * aUpdateKeyInfo,
                                 SShort           aUpdateKeySeq,
                                 stndrKeyInfo   * aInsertKeyInfo,
                                 SShort           aInsertKeySeq,
                                 sdpPhyPageHdr  * aNode,
                                 stndrKeyArray ** aKeyArray,
                                 UShort         * aKeyArrayCnt )
{
    UChar           * sSlotDirPtr;
    UShort            sKeyCount;
    UShort            sIdx;
    UShort            sKeyArrayCnt;
    stndrKeyArray   * sKeyArray = NULL;
    stndrNodeHdr    * sNodeHdr;
    stndrIKey       * sIKey;
    stndrLKey       * sLKey;
    stdMBR            sMBR;
    SInt              i;

    
    *aKeyArray  = NULL;

    sNodeHdr    = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    if( aInsertKeyInfo != NULL )
    {
        sKeyArrayCnt = sKeyCount + 1;
    }
    else
    {
        sKeyArrayCnt = sKeyCount;
    }
    
    IDE_TEST( iduMemMgr::malloc( IDU_MEM_ST_STN,
                                 ID_SIZEOF(stndrKeyArray) * sKeyArrayCnt,
                                 (void **)&sKeyArray,
                                 IDU_MEM_IMMEDIATE ) != IDE_SUCCESS );

    sIdx = 0;
    if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        for( i = 0; i < sKeyCount; i++ )
        {
            if( (aUpdateKeySeq != STNDR_INVALID_KEY_SEQ) && (aUpdateKeySeq == i) )
            {
                IDE_ASSERT( aUpdateKeyInfo != NULL );
                STNDR_GET_MBR_FROM_KEYINFO( sMBR, aUpdateKeyInfo );
            }
            else
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                                sSlotDirPtr, 
                                                                i,
                                                                (UChar**)&sLKey)
                          != IDE_SUCCESS );
                STNDR_GET_MBR_FROM_LKEY( sMBR, sLKey );
            }
            
            sKeyArray[sIdx].mKeySeq = i;
            sKeyArray[sIdx].mMBR = sMBR;
            sIdx++;
        }
    }
    else
    {
        for( i = 0; i < sKeyCount; i++ )
        {
            if( (aUpdateKeySeq != STNDR_INVALID_KEY_SEQ) && (aUpdateKeySeq == i) )
            {
                IDE_ASSERT( aUpdateKeyInfo != NULL );
                STNDR_GET_MBR_FROM_KEYINFO( sMBR, aUpdateKeyInfo );
            }
            else
            {
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                            sSlotDirPtr, 
                                                            i,
                                                            (UChar**)&sIKey )
                          != IDE_SUCCESS );
                STNDR_GET_MBR_FROM_IKEY( sMBR, sIKey );
            }
            
            sKeyArray[sIdx].mKeySeq = i;
            sKeyArray[sIdx].mMBR = sMBR;
            sIdx++;
        }
    }

    if( aInsertKeyInfo != NULL )
    {
        sKeyArray[sIdx].mKeySeq = aInsertKeySeq;
        STNDR_GET_MBR_FROM_KEYINFO( sKeyArray[sIdx].mMBR, aInsertKeyInfo );
    }

    *aKeyArrayCnt = sKeyArrayCnt;
    *aKeyArray = sKeyArray;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sKeyArray != NULL )
    {
        (void)iduMemMgr::free( (void*)sKeyArray );
    }

    *aKeyArrayCnt = 0;
    *aKeyArray = NULL;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Split Mode에 따라서 Split 그룹을 생성한다.
 *********************************************************************/
void stndrRTree::makeSplitGroup( stndrHeader    * aIndex,
                                 UInt             aSplitMode,
                                 stndrKeyArray  * aKeyArray,
                                 UShort           aKeyArrayCnt,
                                 UShort         * aSplitPoint,
                                 stdMBR         * aOldGroupMBR,
                                 stdMBR         * aNewGroupMBR )
{
    stdMBR  sMBR;
    UInt    i;


    IDE_ASSERT( aKeyArrayCnt > 2 );
    
    if( aSplitMode == STNDR_SPLIT_MODE_RTREE )
    {
        splitByRTreeWay( aIndex,
                         aKeyArray,
                         aKeyArrayCnt,
                         aSplitPoint );
    }
    else
    {
        splitByRStarWay( aIndex,
                         aKeyArray,
                         aKeyArrayCnt,
                         aSplitPoint );
    }

    // calculate group mbr
    stdUtils::copyMBR( &sMBR, &aKeyArray[0].mMBR );
    
    for( i = 0; i <= *aSplitPoint; i++ )
    {
        stdUtils::getMBRExtent(
            &sMBR,
            &aKeyArray[i].mMBR );
    }

    *aOldGroupMBR = sMBR;

    stdUtils::copyMBR( &sMBR, &aKeyArray[*aSplitPoint +1 ].mMBR );
    
    for( i = *aSplitPoint + 1; i < aKeyArrayCnt; i++ )
    {
        stdUtils::getMBRExtent(
            &sMBR,
            &aKeyArray[i].mMBR );
    }

    *aNewGroupMBR = sMBR;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * RStar 방식으로 Split 그룹을 생성한다.
 *********************************************************************/
void stndrRTree::splitByRStarWay( stndrHeader   * aIndex,
                                  stndrKeyArray * aKeyArray,
                                  UShort          aKeyArrayCnt,
                                  UShort        * aSplitPoint )
{
    stndrKeyArray   sKeyArrayX[STNDR_MAX_KEY_COUNT];
    stndrKeyArray   sKeyArrayY[STNDR_MAX_KEY_COUNT];
    SDouble         sSumPerimeterX;
    SDouble         sSumPerimeterY;
    UShort          sSplitPointX;
    UShort          sSplitPointY;


    IDE_ASSERT( aKeyArrayCnt <= STNDR_MAX_KEY_COUNT );

    idlOS::memcpy( sKeyArrayX, aKeyArray,
                   ID_SIZEOF(stndrKeyArray) * aKeyArrayCnt );
    
    idlOS::memcpy( sKeyArrayY, aKeyArray,
                   ID_SIZEOF(stndrKeyArray) * aKeyArrayCnt );

    // sort
    quickSort( sKeyArrayX,
               aKeyArrayCnt,
               gCompareKeyArrayByAxisX );

    quickSort( sKeyArrayY,
               aKeyArrayCnt,
               gCompareKeyArrayByAxisY );

    // get split infomation from each axis
    getSplitInfo( aIndex,
                  sKeyArrayX,
                  aKeyArrayCnt,
                  &sSplitPointX,
                  &sSumPerimeterX );
    
    getSplitInfo( aIndex,
                  sKeyArrayY,
                  aKeyArrayCnt,
                  &sSplitPointY,
                  &sSumPerimeterY );

    if( sSumPerimeterX < sSumPerimeterY )
    {
        idlOS::memcpy( aKeyArray, sKeyArrayX,
                       ID_SIZEOF(stndrKeyArray) * aKeyArrayCnt );

        *aSplitPoint = sSplitPointX;
    }
    else
    {
        idlOS::memcpy( aKeyArray, sKeyArrayY,
                       ID_SIZEOF(stndrKeyArray) * aKeyArrayCnt );

        *aSplitPoint = sSplitPointY;
    }

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 기본적인 RTree 방식으로 Split 그룹을 생성한다.
 *********************************************************************/
void stndrRTree::splitByRTreeWay( stndrHeader   * aIndex,
                                  stndrKeyArray * aKeyArray,
                                  UShort          aKeyArrayCnt,
                                  UShort        * aSplitPoint )
{
    stndrKeyArray   sTmpKeyArray[STNDR_MAX_KEY_COUNT];
    stndrKeyArray   sKeyArraySeed0[STNDR_MAX_KEY_COUNT];
    stndrKeyArray   sKeyArraySeed1[STNDR_MAX_KEY_COUNT];
    stndrKeyArray   sKey;
    stndrKeyArray   sKeySeed0;
    stndrKeyArray   sKeySeed1;
    UShort          sKeyCntSeed0 = 0;
    UShort          sKeyCntSeed1 = 0;
    stdMBR          sMBRSeed0;
    stdMBR          sMBRSeed1;
    UShort          sMinCnt;
    UShort          sSeed;
    SShort          sPos;
    SInt            i;
    UInt            j;

    IDE_ASSERT( aKeyArrayCnt <= STNDR_MAX_KEY_COUNT );

    idlOS::memcpy( sTmpKeyArray, aKeyArray,
                   ID_SIZEOF(stndrKeyArray) * aKeyArrayCnt );

    pickSeed( aIndex,
              sTmpKeyArray,
              aKeyArrayCnt,
              &sKeySeed0,
              &sKeySeed1,
              &sPos );

    sKeyArraySeed0[sKeyCntSeed0++] = sKeySeed0;
    sKeyArraySeed1[sKeyCntSeed1++] = sKeySeed1;

    sMBRSeed0 = sKeySeed0.mMBR;
    sMBRSeed1 = sKeySeed1.mMBR;

    sMinCnt = aKeyArrayCnt / 2;

    while( sPos >= 0 )
    {
        pickNext( aIndex,
                  sTmpKeyArray,
                  &sMBRSeed0,
                  &sMBRSeed1,
                  &sPos,
                  &sKey,
                  &sSeed );

        if( sKeyCntSeed0 > sMinCnt )
        {
            sSeed = 1;
        }
        
        if( sSeed == 0 )
        {
            sKeyArraySeed0[sKeyCntSeed0++] = sKey;
            
            stdUtils::getMBRExtent(
                &sMBRSeed0,
                &sKey.mMBR );
        }
        else
        {
            sKeyArraySeed1[sKeyCntSeed1++] = sKey;
            
            stdUtils::getMBRExtent(
                &sMBRSeed1,
                &sKey.mMBR);
        }
    }

    IDE_ASSERT( aKeyArrayCnt == (sKeyCntSeed0 + sKeyCntSeed1) );

    for( i = 0, j = 0; i < sKeyCntSeed0; i++ )
    {
        aKeyArray[j++] = sKeyArraySeed0[i];
    }
    
    *aSplitPoint = j - 1;
    
    for( i = 0; i < sKeyCntSeed1; i++ )
    {
        aKeyArray[j++] = sKeyArraySeed1[i];
    }

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * RTree 방식의 Split에서 사용된다. Dead Space가 가장 큰 키 쌍을 선택한다.
 * 선택돤 2개의 키를 Seed로 선택하고 Key Array의 맨 끝으로 이동시킨다.
 *********************************************************************/
void stndrRTree::pickSeed( stndrHeader      * /*aIndex*/,
                           stndrKeyArray    * aArray,
                           UShort             aArrayCount,
                           stndrKeyArray    * aKeySeed0,
                           stndrKeyArray    * aKeySeed1,
                           SShort           * aPos )
{
    stndrKeyArray   sTmpKey;
    SDouble         sMaxArea;
    SDouble         sTmpArea;
    stdMBR          sTmpMBR;
    UShort          sSeed0;
    UShort          sSeed1;
    UShort          i;
    UShort          j;


    IDE_ASSERT( aArrayCount >= 2 );

    // init seed
    sSeed0 = 0;
    sSeed1 = 1;
    
    sTmpMBR = aArray[sSeed0].mMBR;
            
    sTmpArea = 
        stdUtils::getMBRArea(
            stdUtils::getMBRExtent(
                &sTmpMBR,
                &aArray[sSeed1].mMBR ) );

    sTmpArea -= (stdUtils::getMBRArea(&aArray[sSeed0].mMBR) +
                 stdUtils::getMBRArea(&aArray[sSeed1].mMBR));

    sMaxArea = sTmpArea;

    // find best seed
    for( i = 0; i < aArrayCount - 1 ; i++ )
    {
        for( j = i + 1; j < aArrayCount; j++ )
        {
            sTmpMBR = aArray[i].mMBR;
            
            sTmpArea = 
                stdUtils::getMBRArea(
                    stdUtils::getMBRExtent( &sTmpMBR,
                                            &aArray[j].mMBR ) );

            sTmpArea -= (stdUtils::getMBRArea(&aArray[i].mMBR) +
                         stdUtils::getMBRArea(&aArray[j].mMBR));
            
            if( sTmpArea > sMaxArea )
            {
                sMaxArea = sTmpArea;
                sSeed0 = i;
                sSeed1 = j;
            }
        }
    }

    sTmpKey                 = aArray[sSeed0];
    aArray[sSeed0]          = aArray[aArrayCount - 2];
    aArray[aArrayCount - 2] = sTmpKey;

    sTmpKey                 = aArray[sSeed1];
    aArray[sSeed1]          = aArray[aArrayCount - 1];
    aArray[aArrayCount - 1] = sTmpKey;

    *aKeySeed0 = aArray[aArrayCount - 1];
    *aKeySeed1 = aArray[aArrayCount - 2];

    *aPos = aArrayCount - 3;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * RTree 방식의 Split에서 사용된다. Key Array에서 각 Seed에 포함될때 가장
 * 영역 확장이 적은 키를 선택한다.
 *********************************************************************/
void stndrRTree::pickNext( stndrHeader      * /*aIndex*/,
                           stndrKeyArray    * aArray,
                           stdMBR           * aMBRSeed0,
                           stdMBR           * aMBRSeed1,
                           SShort           * aPos,
                           stndrKeyArray    * aKey,
                           UShort           * aSeed )
{
    SDouble         sDeltaA;
    SDouble         sDeltaB;
    SDouble         sMinDelta;
    stndrKeyArray   sTmpKey;
    SInt            sMin;
    SInt            i;

    
    sDeltaA = stdUtils::getMBRDelta( &aArray[0].mMBR, aMBRSeed0 );
    sDeltaB = stdUtils::getMBRDelta( &aArray[0].mMBR, aMBRSeed1 );

    if( sDeltaA >  sDeltaB )
    {
        sMinDelta = sDeltaB;
        *aSeed = 1;
        sMin = 0;        
    }
    else
    {
        sMinDelta = sDeltaA;
        *aSeed = 0;
        sMin = 0;
    }
    
    for( i = 0; i < (*aPos); i++ )
    {
        sDeltaA = stdUtils::getMBRDelta( &aArray[i].mMBR, aMBRSeed0 );
        sDeltaB = stdUtils::getMBRDelta( &aArray[i].mMBR, aMBRSeed1 );
        
        if( STNDR_MIN(sDeltaA, sDeltaB) < sMinDelta )
        {
            sMin = i;
            
            if( sDeltaA < sDeltaB )
            {
                sMinDelta = sDeltaA;
                *aSeed = 0;
            }
            else
            {
                sMinDelta = sDeltaB;
                *aSeed = 1;
            }
        }
    }

    *aKey        = aArray[sMin];
    
    sTmpKey      = aArray[*aPos];
    aArray[sMin] = sTmpKey;

    (*aPos)--;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * aSrcNode로부터 aDstNode로 slot들을 옮기고 로깅한다.
 *  1. Copy Source Key to Destination Node
 *  2. Unbind Source Key
 *  3. Free Source Key and Adjust UnlimitedKeyCount
 *********************************************************************/
IDE_RC stndrRTree::moveSlots( sdrMtx            * aMtx,
                              stndrHeader       * aIndex,
                              sdpPhyPageHdr     * aSrcNode,
                              stndrKeyArray     * aKeyArray,
                              UShort              aFromIdx,
                              UShort              aToIdx,
                              sdpPhyPageHdr     * aDstNode )
{
    SInt                  i;
    SShort                sSeq;
    SShort                sDstSeq;
    UShort                sAllowedSize;
    UChar               * sSlotDirPtr;
    stndrLKey           * sSrcKey;
    stndrLKey           * sDstKey;
    UChar                 sSrcCreateCTS;
    UChar                 sSrcLimitCTS;
    UChar                 sDstCreateCTS;
    UChar                 sDstLimitCTS;
    UInt                  sKeyLen  = 0;
    UShort                sUnlimitedKeyCount = 0;
    UShort                sKeyOffset;
    stndrNodeHdr        * sSrcNodeHdr;
    stndrNodeHdr        * sDstNodeHdr;
    UShort                sDstTBKCount = 0;
    IDE_RC                sRc;

    sSrcNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aSrcNode );
    
    sDstNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aDstNode );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aSrcNode );

    // Leaf Node는 항상 Compaction이 완료된 상태여야 한다.
    IDE_DASSERT( ( STNDR_IS_LEAF_NODE(sSrcNodeHdr) == ID_FALSE )  ||
                 ( getNonFragFreeSize(aIndex, aSrcNode) ==
                   getTotalFreeSize(aIndex, aSrcNode) ) );

    // Copy Source Key to Destination Node
    sUnlimitedKeyCount = sDstNodeHdr->mUnlimitedKeyCount;
    sDstSeq = -1;

    for( i = aFromIdx; i <= aToIdx; i++ )
    {
        sSeq = aKeyArray[i].mKeySeq;

        // 새로 삽입되는 키는 넘어감
        if( sSeq == STNDR_INVALID_KEY_SEQ )
        {
            continue;
        }
        
        IDE_TEST(sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                          sSeq,
                                                          (UChar**)&sSrcKey )
                 != IDE_SUCCESS );

        sDstSeq++;
        
        if( STNDR_IS_LEAF_NODE(sSrcNodeHdr) == ID_TRUE )
        {
            sKeyLen = getKeyLength( (UChar*)sSrcKey,  ID_TRUE );

            sRc = canAllocLeafKey( aMtx,
                                   aIndex,
                                   aDstNode,
                                   sKeyLen,
                                   NULL );
            
            if( sRc != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0,
                             "From <%d> to <%d> : "
                             "Current sequence number : %d"
                             ", KeyLen %u\n",
                             aFromIdx, aToIdx, i, sKeyLen );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar *)&aKeyArray[aFromIdx],
                                ID_SIZEOF(stndrKeyArray) * (aToIdx - aFromIdx + 1) );
                dumpIndexNode( aSrcNode );
                dumpIndexNode( aDstNode );
                IDE_ASSERT( 0 );
            }
        }
        else
        {
            sKeyLen = getKeyLength( (UChar*)sSrcKey,  ID_FALSE );
                        
            sRc = canAllocInternalKey( aMtx,
                                       aIndex,
                                       aDstNode,
                                       sKeyLen,
                                       ID_TRUE,  /* aExecCompact */
                                       ID_TRUE ); /* aIsLogging */

            if( sRc != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0,
                             "From <%d> to <%d> : "
                             "Current sequence number : %d"
                             ", KeyLen %u\n",
                             aFromIdx, aToIdx, i, sKeyLen );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar *)&aKeyArray[aFromIdx],
                                ID_SIZEOF(stndrKeyArray) * (aToIdx - aFromIdx + 1) );
                dumpIndexNode( aSrcNode );
                dumpIndexNode( aDstNode );
                IDE_ASSERT( 0 );
            }
        }

        IDE_TEST( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aDstNode,
                                         sDstSeq,
                                         sKeyLen,
                                         ID_TRUE,
                                         &sAllowedSize,
                                         (UChar**)&sDstKey,
                                         &sKeyOffset,
                                         1 )
                  != IDE_SUCCESS );
        
        IDE_ASSERT( sAllowedSize >= sKeyLen );

        if( STNDR_IS_LEAF_NODE(sSrcNodeHdr) == ID_TRUE )
        {
            sSrcCreateCTS = STNDR_GET_CCTS_NO( sSrcKey );
            sSrcLimitCTS  = STNDR_GET_LCTS_NO( sSrcKey );

            sDstCreateCTS = sSrcCreateCTS;
            sDstLimitCTS  = sSrcLimitCTS;

            // Copy Source CTS to Destination Node
            if( SDN_IS_VALID_CTS(sSrcCreateCTS) )
            {
                if( STNDR_GET_STATE(sSrcKey) == STNDR_KEY_DEAD )
                {
                    sDstCreateCTS = SDN_CTS_INFINITE;
                }
                else
                {
                    sRc = sdnIndexCTL::allocCTS( aSrcNode,
                                                 sSrcCreateCTS,
                                                 aDstNode,
                                                 &sDstCreateCTS );
                    
                    if( sRc != IDE_SUCCESS )
                    {
                        ideLog::log( IDE_SERVER_0,
                                     "From <%d> to <%d> : "
                                     "Current sequence number : %d"
                                     "\nSource create CTS : %u"
                                     ", Dest create CTS : %u\n",
                                     aFromIdx, aToIdx, i,
                                     sSrcCreateCTS, sDstCreateCTS );
                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar *)&aKeyArray[aFromIdx],
                                        ID_SIZEOF(stndrKeyArray) * (aToIdx - aFromIdx + 1) );
                        dumpIndexNode( aSrcNode );
                        dumpIndexNode( aDstNode );
                        IDE_ASSERT( 0 );
                    }

                    sRc = sdnIndexCTL::bindCTS( aMtx,
                                                aIndex->mSdnHeader.mIndexTSID,
                                                sKeyOffset,
                                                aSrcNode,
                                                sSrcCreateCTS,
                                                aDstNode,
                                                sDstCreateCTS );
                    if ( sRc != IDE_SUCCESS )
                    {
                        ideLog::log( IDE_SERVER_0,
                                     "From <%d> to <%d> : "
                                     "Current sequence number : %d"
                                     "\nSource create CTS : %u"
                                     ", Dest create CTS : %u"
                                     "\nKey Offset : %u\n",
                                     aFromIdx, aToIdx, i,
                                     sSrcCreateCTS, sDstCreateCTS, sKeyOffset );
                        ideLog::logMem( IDE_SERVER_0,
                                        (UChar *)&aKeyArray[aFromIdx],
                                        ID_SIZEOF(stndrKeyArray) * (aToIdx - aFromIdx + 1) );
                        dumpIndexNode( aSrcNode );
                        dumpIndexNode( aDstNode );
                        IDE_ASSERT( 0 );
                    }
                }
            }

            if( SDN_IS_VALID_CTS(sSrcLimitCTS) )
            {
                sRc = sdnIndexCTL::allocCTS( aSrcNode,
                                             sSrcLimitCTS,
                                             aDstNode,
                                             &sDstLimitCTS );
                
                if( sRc != IDE_SUCCESS )
                {
                    ideLog::log( IDE_SERVER_0,
                                 "From <%d> to <%d> : "
                                 "Current sequence number : %d"
                                 "\nSource create CTS : %u"
                                 ", Dest create CTS : %u"
                                 "\nKey Offset : %u\n",
                                 aFromIdx, aToIdx, i,
                                 sSrcCreateCTS, sDstCreateCTS, sKeyOffset );
                    dumpIndexNode( aSrcNode );
                    dumpIndexNode( aDstNode );
                    IDE_ASSERT( 0 );
                }

                sRc = sdnIndexCTL::bindCTS( aMtx,
                                            aIndex->mSdnHeader.mIndexTSID,
                                            sKeyOffset,
                                            aSrcNode,
                                            sSrcLimitCTS,
                                            aDstNode,
                                            sDstLimitCTS );
                if ( sRc != IDE_SUCCESS )
                {
                    ideLog::log( IDE_SERVER_0,
                                 "From <%d> to <%d> : "
                                 "Current sequence number : %d"
                                 "\nSource create CTS : %u"
                                 ", Dest create CTS : %u"
                                 "\nKey Offset : %u\n",
                                 aFromIdx, aToIdx, i,
                                 sSrcCreateCTS, sDstCreateCTS, sKeyOffset );
                    dumpIndexNode( aSrcNode );
                    dumpIndexNode( aDstNode );
                    IDE_ASSERT( 0 );
                }
            }

            idlOS::memcpy( (UChar*)sDstKey, (UChar*)sSrcKey, sKeyLen );

            STNDR_SET_CCTS_NO( sDstKey, sDstCreateCTS );
            STNDR_SET_LCTS_NO( sDstKey, sDstLimitCTS );

            if( ( STNDR_GET_STATE(sDstKey) == STNDR_KEY_UNSTABLE ) ||
                ( STNDR_GET_STATE(sDstKey) == STNDR_KEY_STABLE ) )
            {
                sUnlimitedKeyCount++;
            }

            // BUG-29538 split시 TBK count를 조정하지 않고 있습니다.
            if( STNDR_GET_TB_TYPE( sSrcKey ) == STNDR_KEY_TB_KEY )
            {
                sDstTBKCount++;
                IDE_ASSERT( sDstTBKCount <= sSrcNodeHdr->mTBKCount );
            }
        }
        else
        {
            idlOS::memcpy( (UChar*)sDstKey, (UChar*)sSrcKey, sKeyLen );
        }
        
        // write log
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sDstKey,
                                             (void*)NULL,
                                             ID_SIZEOF(SShort)+sKeyLen,
                                             SDR_STNDR_INSERT_INDEX_KEY )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       (void*)&sDstSeq,
                                       ID_SIZEOF(SShort) )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       (void*)sDstKey,
                                       sKeyLen)
                  != IDE_SUCCESS );
    }

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sDstNodeHdr->mUnlimitedKeyCount,
                  (void*)&sUnlimitedKeyCount,
                  ID_SIZEOF(sUnlimitedKeyCount) )
              != IDE_SUCCESS);

    // BUG-29538 split시 TBK count를 조정하지 않고 있습니다.
    // Destination leaf node의 header에 TBK count를 저장하고 로깅
    IDE_ASSERT( sSrcNodeHdr->mTBKCount >= sDstTBKCount );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sDstNodeHdr->mTBKCount,
                  (void*)&sDstTBKCount,
                  ID_SIZEOF(sDstTBKCount))
              != IDE_SUCCESS);

    // Unbind Source Key
    for( i = aToIdx; i >= aFromIdx; i-- )
    {
        sSeq = aKeyArray[i].mKeySeq;

        if( (sSeq == STNDR_INVALID_KEY_SEQ) )
        {
            continue;
        }

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           sSeq,
                                                           (UChar**)&sSrcKey )
                  != IDE_SUCCESS );
        
        IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr, 
                                              sSeq,
                                              &sKeyOffset )
                  != IDE_SUCCESS );
        
        if( STNDR_IS_LEAF_NODE(sSrcNodeHdr) == ID_TRUE )
        {
            if ( SDN_IS_VALID_CTS( STNDR_GET_CCTS_NO(sSrcKey) ) )
            {
                IDE_TEST( sdnIndexCTL::unbindCTS( aMtx,
                                                  aSrcNode,
                                                  STNDR_GET_CCTS_NO( sSrcKey ),
                                                  sKeyOffset )
                          != IDE_SUCCESS );
            }

            if ( SDN_IS_VALID_CTS( STNDR_GET_LCTS_NO( sSrcKey ) ) )
            {
                IDE_TEST( sdnIndexCTL::unbindCTS( aMtx,
                                                  aSrcNode,
                                                  STNDR_GET_LCTS_NO( sSrcKey ),
                                                  sKeyOffset )
                          != IDE_SUCCESS );
            }
        }
    }

    // Free Source Key and Adjust UnlimitedKeyCount
    IDE_TEST( freeKeys( aMtx,
                        aSrcNode,
                        aKeyArray,
                        0,
                        aFromIdx - 1 )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * aFromIdx에서부터 aToIdx를 제외한 모든 키를 Free 시킨다.
 *********************************************************************/
IDE_RC stndrRTree::freeKeys( sdrMtx         * aMtx,
                             sdpPhyPageHdr  * aNode,
                             stndrKeyArray  * aKeyArray,
                             UShort           aFromIdx,
                             UShort           aToIdx )
{
    stndrNodeHdr    * sNodeHdr;
    UShort            sUnlimitedKeyCount = 0;
    UShort            sTBKCount          = 0;

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

    IDE_TEST( writeLogFreeKeys(
                  aMtx,
                  (UChar*)aNode,
                  aKeyArray,
                  aFromIdx,
                  aToIdx ) != IDE_SUCCESS );

    if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        freeKeysLeaf( aNode,
                      aKeyArray,
                      aFromIdx,
                      aToIdx,
                      &sUnlimitedKeyCount,
                      &sTBKCount );
    }
    else
    {
        freeKeysInternal( aNode,
                          aKeyArray,
                          aFromIdx,
                          aToIdx );
    }

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sNodeHdr->mUnlimitedKeyCount,
                  (void*)&sUnlimitedKeyCount,
                  ID_SIZEOF(sUnlimitedKeyCount) )
              != IDE_SUCCESS );

    // BUG-29538 split시 TBK count를 조정하지 않고 있습니다.
    // Source leaf node의 header에 TBK count를 저장하고 로깅
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sNodeHdr->mTBKCount,
                  (void*)&sTBKCount,
                  ID_SIZEOF(sTBKCount) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Internal Key를 Free 시킨다.
 *********************************************************************/
IDE_RC stndrRTree::freeKeysInternal( sdpPhyPageHdr    * aNode,
                                   stndrKeyArray    * aKeyArray,
                                   UShort             aFromIdx,
                                   UShort             aToIdx )
{

    SInt          i;
    UShort        sTmpBuf[SD_PAGE_SIZE]; // 2 * Page size -> align 문제...
    UChar       * sTmpPage;
    UChar       * sSlotDirPtr;
    UShort        sKeyLength;
    UShort        sAllowedSize;
    scOffset      sSlotOffset;
    UChar       * sSrcKey;
    UChar       * sDstKey;
    SShort        sSeq;
    SShort        sKeySeq = 0;
    

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );

    sTmpPage = ((vULong)sTmpBuf) % SD_PAGE_SIZE == 0
        ? (UChar*)sTmpBuf
        : (UChar*)sTmpBuf + SD_PAGE_SIZE - (((vULong)sTmpBuf) % SD_PAGE_SIZE);

    idlOS::memcpy( sTmpPage, aNode, SD_PAGE_SIZE );
    
    IDE_ASSERT( aNode->mPageID == ((sdpPhyPageHdr*)sTmpPage)->mPageID );

    (void)sdpPhyPage::reset( aNode,
                             ID_SIZEOF(stndrNodeHdr),
                             NULL );

    sdpSlotDirectory::init( aNode );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sTmpPage);

    for( i = aFromIdx; i <= aToIdx; i++ )
    {
        sSeq = aKeyArray[i].mKeySeq;

        if( sSeq == STNDR_INVALID_KEY_SEQ )
        {
            continue;
        }
        
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           sSeq,
                                                           &sSrcKey )
                  != IDE_SUCCESS );


        sKeyLength = getKeyLength( sSrcKey, ID_FALSE ); //aIsLeaf

        IDE_ASSERT( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aNode,
                                           sKeySeq,
                                           sKeyLength,
                                           ID_TRUE,
                                           &sAllowedSize,
                                           &sDstKey,
                                           &sSlotOffset,
                                           1 )
                    == IDE_SUCCESS );
        
        idlOS::memcpy( sDstKey, sSrcKey, sKeyLength );
        // Insert Logging할 필요 없음.

        sKeySeq++;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Leaf Key를 Free 시킨다.
 *********************************************************************/
IDE_RC stndrRTree::freeKeysLeaf( sdpPhyPageHdr    * aNode,
                                 stndrKeyArray    * aKeyArray,
                                 UShort             aFromIdx,
                                 UShort             aToIdx,
                                 UShort           * aUnlimitedKeyCount,
                                 UShort           * aTBKCount )
{
    SInt              i;
    UShort            sTmpBuf[SD_PAGE_SIZE]; // 2 * Page size -> align 문제...
    UChar           * sTmpPage;
    UChar           * sSlotDirPtr;
    UShort            sKeyLength;
    UShort            sAllowedSize;
    scOffset          sSlotOffset;
    UChar           * sSrcKey;
    UChar           * sDstKey;
    stndrLKey       * sLeafKey;
    UChar             sCreateCTS;
    UChar             sLimitCTS;
    stndrNodeHdr    * sNodeHdr;
    SShort            sKeySeq;
    UChar           * sDummy;
    UShort            sUnlimitedKeyCount = 0;
    SShort            sSeq;
    UShort            sTBKCount = 0;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aNode);
    sNodeHdr    = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)aNode);

    sTmpPage = ((vULong)sTmpBuf) % SD_PAGE_SIZE == 0
        ? (UChar*)sTmpBuf
        : (UChar*)sTmpBuf + SD_PAGE_SIZE - (((vULong)sTmpBuf) % SD_PAGE_SIZE);

    idlOS::memcpy(sTmpPage, aNode, SD_PAGE_SIZE);
    IDE_ASSERT( aNode->mPageID == ((sdpPhyPageHdr*)sTmpPage)->mPageID );

    (void)sdpPhyPage::reset( aNode,
                             ID_SIZEOF(stndrNodeHdr),
                             NULL );

    // sdpPhyPage::reset에서는 CTL을 초기화 해주지는 않는다.
    sdpPhyPage::initCTL( aNode,
                         (UInt)sdnIndexCTL::getCTLayerSize(sTmpPage),
                         &sDummy );

    sdpSlotDirectory::init( aNode );

    sdnIndexCTL::cleanAllRefInfo( aNode );

    sNodeHdr->mTotalDeadKeySize = 0;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sTmpPage);
    
    for( i = aFromIdx, sKeySeq = 0; i <= aToIdx; i++ )
    {
        sSeq = aKeyArray[i].mKeySeq;

        if( sSeq == STNDR_INVALID_KEY_SEQ )
        {
            continue;
        }

        
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(sSlotDirPtr, 
                                                          sSeq, 
                                                          &sSrcKey)
                  != IDE_SUCCESS );
        
        sLeafKey = (stndrLKey*)sSrcKey;
            
        sKeyLength = getKeyLength( sSrcKey, ID_TRUE ); //aIsLeaf

        IDE_ASSERT( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aNode,
                                           sKeySeq,
                                           sKeyLength,
                                           ID_TRUE,
                                           &sAllowedSize,
                                           &sDstKey,
                                           &sSlotOffset,
                                           1 )
                    == IDE_SUCCESS );

        sKeySeq++;
       
        idlOS::memcpy( sDstKey, sSrcKey, sKeyLength );

        sCreateCTS = STNDR_GET_CCTS_NO( sLeafKey );
        sLimitCTS  = STNDR_GET_LCTS_NO( sLeafKey );

        if ( SDN_IS_VALID_CTS(sCreateCTS) )
        {
            sdnIndexCTL::addRefKey( aNode, sCreateCTS, sSlotOffset );
        }

        if ( SDN_IS_VALID_CTS(sLimitCTS) )
        {
            sdnIndexCTL::addRefKey( aNode, sLimitCTS, sSlotOffset );
        }

        if( (STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_UNSTABLE) ||
            (STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_STABLE) )
        {
            sUnlimitedKeyCount++;
        }

        // BUG-29538 split시 TBK count를 조정하지 않고 있습니다.
        if( STNDR_GET_TB_TYPE( sLeafKey ) == STNDR_KEY_TB_KEY )
        {
            sTBKCount++;
            IDE_ASSERT( sTBKCount <= sNodeHdr->mTBKCount );
        }
    }

    *aUnlimitedKeyCount = sUnlimitedKeyCount;
    *aTBKCount          = sTBKCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * aKeyArray에서 최적의 Split Point와 분할 가능한 모든 경우의 누적
 * Perimeter를 구한다.
 *********************************************************************/
void stndrRTree::getSplitInfo( stndrHeader      * aIndex,
                               stndrKeyArray    * aKeyArray,
                               SShort             aKeyArrayCnt,
                               UShort           * aSplitPoint,
                               SDouble          * aSumPerimeter )
{
    SDouble sSumPerimeter;
    SDouble sMinPerimeter;
    SDouble sMinOverlap;
    SDouble sMinArea;
    SDouble sPerimeter1;
    SDouble sPerimeter2;
    SDouble sOverlap;
    SDouble sArea;
    stdMBR  sMBR1;
    stdMBR  sMBR2;
    UInt    sSplitRate = 40;
    SShort  sInitSplitPoint = 0;
    SShort  sMaxSplitPoint = 0;
    UShort  i; // BUG-30950 컴파일 버그로 인하여 SShort -> UShort로 변경

    IDE_ASSERT( aKeyArrayCnt >= 3 );

    sSplitRate = smuProperty::getRTreeSplitRate();
    if( sSplitRate > 50 )
    {
        sSplitRate = 50;
    }

    sInitSplitPoint = ( aKeyArrayCnt * sSplitRate ) / 100 ;
    if( sInitSplitPoint <  0)
    {
        sInitSplitPoint = 0;
    }
    
    sMaxSplitPoint  = sInitSplitPoint +
        ( aKeyArrayCnt - (sInitSplitPoint * 2) );
    if( sMaxSplitPoint >= (aKeyArrayCnt - 1) )
    {
        // zero base 이기 때문에 -2를 설정한다.
        sMaxSplitPoint = (aKeyArrayCnt - 2);
    }
    
    getArrayPerimeter( aIndex,
                       aKeyArray,
                       0,
                       aKeyArrayCnt - 1,
                       &sMinPerimeter,
                       &sMBR1 );
    
    sMinOverlap = stdUtils::getMBRArea( &sMBR1 );
    sMinArea    = sMinOverlap;

    sSumPerimeter = 0;
    *aSplitPoint = sInitSplitPoint;
    
    for( i = sInitSplitPoint; i <= sMaxSplitPoint; i++ )
    {
        getArrayPerimeter( aIndex,
                           aKeyArray,
                           0,
                           i,
                           &sPerimeter1,
                           &sMBR1 );
        
        getArrayPerimeter( aIndex,
                           aKeyArray,
                           i + 1,
                           (aKeyArrayCnt - 1),
                           &sPerimeter2,
                           &sMBR2 );

        sOverlap = stdUtils::getMBROverlap( &sMBR1, &sMBR2 );

        sArea = stdUtils::getMBRArea( &sMBR1 ) + stdUtils::getMBRArea( &sMBR2 );

        if( sOverlap < sMinOverlap )
        {
            *aSplitPoint = i;
            sMinOverlap = sOverlap;
        }
        else
        {
            if( (sOverlap == sMinOverlap) && (sArea < sMinArea) )
            {
                *aSplitPoint = i;
                sMinArea = sArea;
            }
        }

        sSumPerimeter += (sPerimeter1 + sPerimeter2);
    }

    *aSumPerimeter  = sSumPerimeter;

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Kery Array의 aStartPos부터 aEndPos의 까지의 Key를 포함하는 MBR의
 * Perimeter를 구한다.
 *********************************************************************/
void stndrRTree::getArrayPerimeter( stndrHeader     * /*aIndex*/,
                                    stndrKeyArray   * aKeyArray,
                                    UShort            aStartPos,
                                    UShort            aEndPos,
                                    SDouble         * aPerimeter,
                                    stdMBR          * aMBR )
{
    UInt i;

    *aMBR = aKeyArray[aStartPos].mMBR;

    for( i = aStartPos + 1; i <= aEndPos; i++ )
    {
        stdUtils::getMBRExtent( aMBR, &aKeyArray[i].mMBR );
    }

    *aPerimeter =
        (aMBR->mMaxX - aMBR->mMinX)*2 +
        (aMBR->mMaxY - aMBR->mMinY)*2;
    
    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 자식노드의 KeyValue(MBR) 변경을 상위 노드에 전파한다. 하위에서부터
 * 상위로 변경 가능한 모든 상위 노드에 대해 X-Latch를 잡은 후 최상위에서 
 * 아래로 내려 오면서 KeyValue 변경작업을 수행한다.
 * 이 때 Stack의 최상위 노드가 Root Node가 아니면 Root가 변경된 경우
 * 이므로 Retry 한다.
 *********************************************************************/
IDE_RC stndrRTree::propagateKeyValue( idvSQL            * aStatistics,
                                      stndrStatistic    * aIndexStat,
                                      stndrHeader       * aIndex,
                                      sdrMtx            * aMtx,
                                      stndrPathStack    * aStack,
                                      sdpPhyPageHdr     * aChildNode,
                                      stdMBR            * aChildNodeMBR,
                                      idBool            * aIsRetry )
{
    scPageID          sChildPID;
    scPageID          sParentPID;
    sdpPhyPageHdr   * sParentNode;
    SShort            sParentKeySeq;
    stdMBR            sParentKeyMBR;
    stdMBR            sParentNodeMBR;
    IDE_RC            sRc;
    

    if( aStack->mDepth < 0 )
    {
        IDE_RAISE( RETURN_SUCCESS );
    }

    sChildPID = sdpPhyPage::getPageID( (UChar*)aChildNode );

    IDE_TEST( getParentNode( aStatistics,
                             aIndexStat,
                             aMtx,
                             aIndex,
                             aStack,
                             sChildPID,
                             &sParentPID,
                             &sParentNode,
                             &sParentKeySeq,
                             &sParentKeyMBR,
                             aIsRetry )
              != IDE_SUCCESS );

    // root node 변경 체크하기
    IDE_TEST_RAISE( *aIsRetry == ID_TRUE, RETURN_SUCCESS );

    if( stdUtils::isMBREquals( &sParentKeyMBR,
                               aChildNodeMBR ) == ID_TRUE )
    {
        IDE_RAISE( RETURN_SUCCESS );
    }

    IDE_ASSERT( adjustNodeMBR( aIndex,
                               sParentNode,
                               NULL,                   /* aInsertKeySeq, */
                               sParentKeySeq,
                               aChildNodeMBR,
                               STNDR_INVALID_KEY_SEQ,  /* aDeleteKeySeq */
                               &sParentNodeMBR )
                == IDE_SUCCESS );
    
    aStack->mDepth--;
    IDE_TEST( propagateKeyValue( aStatistics,
                                 aIndexStat,
                                 aIndex,
                                 aMtx,
                                 aStack,
                                 sParentNode,
                                 &sParentNodeMBR,
                                 aIsRetry )
              != IDE_SUCCESS );

    if( *aIsRetry == ID_TRUE )
    {
        IDE_RAISE( RETURN_SUCCESS );
    }

    sRc = updateIKey( aMtx,
                      sParentNode,
                      sParentKeySeq,
                      aChildNodeMBR,
                      aIndex->mSdnHeader.mLogging );

    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0,
                     "Key sequence : %d"
                     ", update MBR : \n",
                     sParentKeySeq  );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)aChildNodeMBR,
                        ID_SIZEOF(stdMBR) );
        dumpIndexNode( sParentNode );
        IDE_ASSERT( 0 );
    }

    sRc = setNodeMBR( aMtx,
                      sParentNode,
                      &sParentNodeMBR );

    if( sRc != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0, "update MBR : \n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sParentNodeMBR,
                        ID_SIZEOF(stdMBR) );
        dumpIndexNode( sParentNode );
        IDE_ASSERT( 0 );
    }

    // update Tree MBR
    if( sParentPID == aIndex->mRootNode )
    {
        if( aIndex->mInitTreeMBR == ID_TRUE )
        {
            aIndex->mTreeMBR = sParentNodeMBR;
        }
        else
        {
            aIndex->mTreeMBR = sParentNodeMBR;
            aIndex->mInitTreeMBR = ID_TRUE;
        }
    }
    
    aIndexStat->mKeyPropagateCount++;

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Internal node인 aNode에 새 키를 insert한다. 
 *********************************************************************/
IDE_RC stndrRTree::insertIKey( sdrMtx           * aMtx,
                               stndrHeader      * aIndex,
                               sdpPhyPageHdr    * aNode,
                               SShort             aKeySeq,
                               stndrKeyInfo     * aKeyInfo,
                               UShort             aKeyValueLen,
                               scPageID           aRightChildPID,
                               idBool             aIsNeedLogging )
{
    UShort        sAllowedSize;
    scOffset      sSlotOffset;
    stndrIKey   * sIKey;
    UShort        sKeyLength = STNDR_IKEY_LEN( aKeyValueLen );


    IDE_TEST( canAllocInternalKey( aMtx,
                                   aIndex,
                                   aNode,
                                   sKeyLength,
                                   ID_TRUE, //aExecCompact
                                   aIsNeedLogging )
              != IDE_SUCCESS );

    IDE_TEST( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aNode,
                                     aKeySeq,
                                     sKeyLength,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     (UChar**)&sIKey,
                                     &sSlotOffset,
                                     1 )
              != IDE_SUCCESS );
    
    IDE_DASSERT( sAllowedSize >= sKeyLength );

    STNDR_KEYINFO_TO_IKEY( *aKeyInfo,
                           aRightChildPID,
                           aKeyValueLen,
                           sIKey );

    if( aIsNeedLogging == ID_TRUE )
    {
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sIKey,
                                             (void*)NULL,
                                             ID_SIZEOF(SShort)+sKeyLength,
                                             SDR_STNDR_INSERT_INDEX_KEY )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       (void*)&aKeySeq,
                                       ID_SIZEOF(SShort) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       (void*)sIKey,
                                       sKeyLength )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Internal node인 aNode에 기존 키를 업데이트 한다.
 *********************************************************************/
IDE_RC stndrRTree::updateIKey( sdrMtx           * aMtx,
                               sdpPhyPageHdr    * aNode,
                               SShort             aKeySeq,
                               stdMBR           * aKeyValue,
                               idBool             aLogging )
{
    UChar       * sSlotDirPtr;
    stndrIKey   * sIKey;


    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                       aKeySeq,
                                                       (UChar**)&sIKey )
              != IDE_SUCCESS );

    STNDR_SET_KEYVALUE_TO_IKEY( *aKeyValue, ID_SIZEOF(*aKeyValue), sIKey );

    if( aLogging == ID_TRUE )
    {
        IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                             (UChar*)sIKey,
                                             (void*)aKeyValue,
                                             ID_SIZEOF(*aKeyValue),
                                             SDR_STNDR_UPDATE_INDEX_KEY )
                  != IDE_SUCCESS );
    }
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * aChildPID를 가리키는 부모 노드를 찾아서 X-Latch를 잡는다.
 * 부모 노드가 Split이 발생한 경우 오른쪽 링크를 따라가면서 aChildPID를
 * 가리키는 부모 노드를 찾아서 X-Latch를 잡는다.
 *********************************************************************/
IDE_RC stndrRTree::getParentNode( idvSQL            * aStatistics,
                                  stndrStatistic    * aIndexStat,
                                  sdrMtx            * aMtx,
                                  stndrHeader       * aIndex,
                                  stndrPathStack    * aStack,
                                  scPageID            aChildPID,
                                  scPageID          * aParentPID,
                                  sdpPhyPageHdr    ** aParentNode,
                                  SShort            * aParentKeySeq,
                                  stdMBR            * aParentKeyMBR,
                                  idBool            * aIsRetry )
{
    stndrStackSlot    sStackSlot;
    sdpPhyPageHdr   * sNode;
    scPageID          sPID;
    scPageID          sIKeyChildPID;
    UShort            sKeyCount;
    stndrIKey       * sIKey;
    ULong             sNodeSmoNo;
    ULong             sIndexSmoNo;
    SShort            sKeySeq;
    UChar           * sSlotDirPtr;
    idBool            sIsSuccess;
    UShort            i;
    
    
    *aParentPID    = SD_NULL_PID;
    *aParentNode   = NULL;
    *aParentKeySeq = STNDR_INVALID_KEY_SEQ;

    sStackSlot = aStack->mStack[aStack->mDepth];

    sPID       = sStackSlot.mNodePID;
    sIndexSmoNo= sStackSlot.mSmoNo;
    sKeySeq    = sStackSlot.mKeySeq;

    IDE_TEST( stndrRTree::getPage( aStatistics,
                                   &(aIndexStat->mIndexPage),
                                   aIndex->mSdnHeader.mIndexTSID,
                                   sPID,
                                   SDB_X_LATCH,
                                   SDB_WAIT_NORMAL,
                                   aMtx,
                                   (UChar**)&sNode,
                                   &sIsSuccess ) != IDE_SUCCESS );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    if( (sIndexSmoNo == sdpPhyPage::getIndexSMONo(sNode)) &&
        (sKeySeq != STNDR_INVALID_KEY_SEQ ) &&
        (sKeyCount > sKeySeq) )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           sKeySeq,
                                                           (UChar**)&sIKey )
                  != IDE_SUCCESS );

        if( aChildPID == sIKey->mChildPID )
        {
            *aParentPID    = sPID;
            *aParentNode   = sNode;
            *aParentKeySeq = sKeySeq;

            STNDR_GET_MBR_FROM_IKEY( *aParentKeyMBR, sIKey );

            IDE_RAISE( RETURN_SUCCESS );
        }
    }

    while( (sNodeSmoNo = sdpPhyPage::getIndexSMONo(sNode))
           >= sIndexSmoNo )
    {
        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sNode );
        sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

        for( i = 0; i < sKeyCount; i++)
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                               i,
                                                               (UChar**)&sIKey )
                      != IDE_SUCCESS );

            STNDR_GET_CHILD_PID( &sIKeyChildPID, sIKey );

            if( aChildPID == sIKeyChildPID )
            {
                *aParentPID     = sPID;
                *aParentNode    = sNode;
                *aParentKeySeq  = i;

                STNDR_GET_MBR_FROM_IKEY( *aParentKeyMBR, sIKey );
                
                break;
            }
        }

        if( i != sKeyCount )
        {
            break;
        }

        if( sNodeSmoNo == sIndexSmoNo )
        {
            break;
        }

        sPID = sdpPhyPage::getNxtPIDOfDblList(sNode);
        if( sPID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndexStat->mIndexPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       sPID,
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       aMtx,
                                       (UChar**)&sNode,
                                       &sIsSuccess ) != IDE_SUCCESS );

        aIndexStat->mFollowRightLinkCount++;
    }

    if( (aStack->mDepth == 0) && (*aParentPID != aIndex->mRootNode) )
    {
        *aIsRetry = ID_TRUE;
    }

    if( (*aParentPID    == SD_NULL_PID) ||
        (*aParentNode   == NULL) ||
        (*aParentKeySeq == STNDR_INVALID_KEY_SEQ) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Child PID : %u"
                     ", sPID : %u"
                     ", Node Smo NO: %llu\n",
                     aChildPID, sPID, sNodeSmoNo );

        ideLog::log( IDE_SERVER_0,
                     "Stack Slot:\n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)&sStackSlot,
                        ID_SIZEOF(stndrStackSlot) );

        ideLog::log( IDE_SERVER_0,
                     "Path Stack:\n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar*)aStack,
                        ID_SIZEOF(stndrPathStack) );

        IDE_ASSERT( 0 );
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Leaf Node 키를 삽입할 공간이 있는지 확인한다.
 *********************************************************************/
IDE_RC stndrRTree::canInsertKey( idvSQL               * aStatistics,
                                 sdrMtx               * aMtx,
                                 stndrHeader          * aIndex,
                                 stndrStatistic       * aIndexStat,
                                 stndrKeyInfo         * aKeyInfo,
                                 sdpPhyPageHdr        * aLeafNode,
                                 SShort               * aLeafKeySeq,
                                 UChar                * aCTSlotNum,
                                 stndrCallbackContext * aContext )
{
    UChar   sAgedCount = 0;
    smSCN   sSysMinDskViewSCN;
    UShort  sKeyValueLen;
    UShort  sKeyLen;

    aContext->mIndex      = aIndex;
    aContext->mStatistics = aIndexStat;

    sKeyValueLen = getKeyValueLength();

    // BUG-30020: Top-Down 빌드시에 Stable한 키는 allocCTS를 스킵해야 합니다.
    if( aKeyInfo->mKeyState == STNDR_KEY_STABLE )
    {
        sKeyLen = STNDR_LKEY_LEN( sKeyValueLen, STNDR_KEY_TB_CTS );
        IDE_RAISE( SKIP_ALLOC_CTS );
    }

    IDE_TEST( stndrRTree::allocCTS( aStatistics,
                                    aIndex,
                                    aMtx,
                                    aLeafNode,
                                    aCTSlotNum,
                                    &gCallbackFuncs4CTL,
                                    (UChar*)aContext,
                                    aLeafKeySeq )
              != IDE_SUCCESS );

    if( *aCTSlotNum == SDN_CTS_INFINITE )
    {
        sKeyLen = STNDR_LKEY_LEN( sKeyValueLen, STNDR_KEY_TB_KEY );
    }
    else
    {
        sKeyLen = STNDR_LKEY_LEN( sKeyValueLen, STNDR_KEY_TB_CTS );
    }

    IDE_EXCEPTION_CONT( SKIP_ALLOC_CTS );

    if( canAllocLeafKey( aMtx,
                         aIndex,
                         aLeafNode,
                         sKeyLen,
                         aLeafKeySeq ) != IDE_SUCCESS )
    {
        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );
            
        // 적극적으로 공간 할당을 위해서 Self Aging을 한다.
        IDE_TEST( selfAging( aIndex,
                             aMtx,
                             aLeafNode,
                             &sSysMinDskViewSCN,
                             &sAgedCount ) != IDE_SUCCESS );

        if( sAgedCount > 0 )
        {
            IDE_TEST( canAllocLeafKey( aMtx,
                                       aIndex,
                                       aLeafNode,
                                       (UInt)sKeyLen,
                                       aLeafKeySeq )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( 1 ); // return IDE_FAILURE;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 키를 삽입할 최적의 Leaf Node를 찾는다. 키가 삽입될 때 MBR의 확장이
 * 최소화되는 방향으로 탐색한다.
 * 키 삽입 중 Split이 발생하면 Split이 발생하지 않은 상위 노드를 찾아서
 * 해당 위치로부터 재탐색한다.
 *********************************************************************/
IDE_RC stndrRTree::chooseLeafNode( idvSQL           * aStatistics,
                                   stndrStatistic   * aIndexStat,
                                   sdrMtx           * aMtx,
                                   stndrPathStack   * aStack,
                                   stndrHeader      * aIndex,
                                   stndrKeyInfo     * aKeyInfo,
                                   sdpPhyPageHdr   ** aLeafNode,
                                   SShort           * aLeafKeySeq )
{
    ULong                  sIndexSmoNo;
    SShort                 sKeySeq = 0;
    scPageID               sPID = SD_NULL_PID;
    scPageID               sChildPID = SD_NULL_PID;
    SDouble                sDelta;
    sdpPhyPageHdr        * sPage;
    stndrNodeHdr         * sNodeHdr;
    UInt                   sHeight;
    stndrStackSlot         sSlot;
    idBool                 sIsSuccess;
    idBool                 sIsRetry;
    idBool                 sFixState = ID_FALSE;
    sdrSavePoint           sSP;
    stndrVirtualRootNode   sVRootNode;

    
  retry:
    
    if( aStack->mDepth == -1 )
    {
        getVirtualRootNode( aIndex, &sVRootNode );

        sPID        = sVRootNode.mChildPID;
        sIndexSmoNo = sVRootNode.mChildSmoNo;
        
        if( sPID == SD_NULL_PID )
        {
            *aLeafNode = NULL;
            *aLeafKeySeq = 0;

            IDE_RAISE( RETURN_SUCCESS );
        }
    }
    else
    {
        sPID        = aStack->mStack[aStack->mDepth + 1].mNodePID;
        sIndexSmoNo = aStack->mStack[aStack->mDepth + 1].mSmoNo;
    }

    IDU_FIT_POINT( "1.PROJ-1591@stndrRTree::chooseLeafNode" );
    
    IDE_TEST( stndrRTree::getPage( aStatistics,
                                   &(aIndexStat->mIndexPage),
                                   aIndex->mSdnHeader.mIndexTSID,
                                   sPID,
                                   SDB_S_LATCH,
                                   SDB_WAIT_NORMAL,
                                   NULL,
                                   (UChar**)&sPage,
                                   &sIsSuccess ) != IDE_SUCCESS );
    sFixState = ID_TRUE;

    while( 1 )
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sPage );
        
        sHeight = sNodeHdr->mHeight;
        
        if( sHeight > 0 ) // Internal Node
        {
            findBestInternalKey( aKeyInfo,
                                 aIndex,
                                 sPage,
                                 &sKeySeq,
                                 &sChildPID,
                                 &sDelta,
                                 sIndexSmoNo,
                                 &sIsRetry );

            if( sIsRetry == ID_TRUE )
            {
                sFixState = ID_FALSE;
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                     (UChar*)sPage )
                          != IDE_SUCCESS );

                IDE_TEST( findValidStackDepth( aStatistics,
                                               aIndexStat,
                                               aIndex,
                                               aStack,
                                               &sIndexSmoNo )
                          != IDE_SUCCESS );

                // if( stndrStackMgr::getDepth(aStack) < 0 ) // SMO가 Root까지 일어남.
                if( aStack->mDepth < 0 ) // SMO가 Root까지 일어남.
                {
                    // init stack
                    aStack->mDepth = -1;
                    goto retry;
                }
                else
                {
                    sSlot = aStack->mStack[aStack->mDepth];
                    aStack->mDepth--;
                    
                    sPID = sSlot.mNodePID;
                    
                    IDE_TEST( stndrRTree::getPage( aStatistics,
                                                   &(aIndexStat->mIndexPage),
                                                   aIndex->mSdnHeader.mIndexTSID,
                                                   sPID,
                                                   SDB_S_LATCH,
                                                   SDB_WAIT_NORMAL,
                                                   NULL,
                                                   (UChar**)&sPage,
                                                   &sIsSuccess)
                              != IDE_SUCCESS );
                    sFixState = ID_TRUE;
                    
                    continue;
                }
            }
            else
            {
                aStack->mDepth++;
                aStack->mStack[aStack->mDepth].mNodePID = sPID;
                aStack->mStack[aStack->mDepth].mSmoNo  = sdpPhyPage::getIndexSMONo(sPage);
                aStack->mStack[aStack->mDepth].mKeySeq = sKeySeq;

                sPID = sChildPID;

                // !!! 반드시 Latch를 풀기 전에 IndexNSN를 딴다. 그렇지 않으면
                // 자식 노드의 Split를 감지할 수 없다.
                getSmoNo( aIndex, &sIndexSmoNo );
                IDL_MEM_BARRIER;

                sFixState = ID_FALSE;
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sPage )
                          != IDE_SUCCESS );
            
                // fix sChildPID to sPage
                IDE_TEST( stndrRTree::getPage( aStatistics,
                                               &(aIndexStat->mIndexPage),
                                               aIndex->mSdnHeader.mIndexTSID,
                                               sPID,
                                               SDB_S_LATCH,
                                               SDB_WAIT_NORMAL,
                                               NULL,
                                               (UChar**)&sPage,
                                               &sIsSuccess )
                          != IDE_SUCCESS );
                
                sFixState = ID_TRUE;
            }
        }
        else // Leaf Node
        {
            // sNode를 unfix한다.
            sFixState = ID_FALSE;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 (UChar*)sPage )
                      != IDE_SUCCESS );

            sdrMiniTrans::setSavePoint( aMtx, &sSP );
            
            IDE_TEST( stndrRTree::getPage( aStatistics,
                                           &(aIndexStat->mIndexPage),
                                           aIndex->mSdnHeader.mIndexTSID,
                                           sPID,
                                           SDB_X_LATCH,
                                           SDB_WAIT_NORMAL,
                                           aMtx,
                                           (UChar**)&sPage,
                                           &sIsSuccess )
                      != IDE_SUCCESS );

            findBestLeafKey( sPage, &sKeySeq, sIndexSmoNo, &sIsRetry );

            if( sIsRetry == ID_TRUE )
            {
                sdrMiniTrans::releaseLatchToSP( aMtx, &sSP );

                IDE_TEST( findValidStackDepth( aStatistics,
                                               aIndexStat,
                                               aIndex,
                                               aStack,
                                               &sIndexSmoNo )
                          != IDE_SUCCESS );

                if( aStack->mDepth < 0 ) // SMO가 Root까지 일어남.
                {
                    // init Stack
                    aStack->mDepth = -1;
                    goto retry;
                }
                else
                {
                    sSlot = aStack->mStack[aStack->mDepth];
                    aStack->mDepth--;
                    
                    sPID = sSlot.mNodePID;
                    
                    IDE_TEST( stndrRTree::getPage( aStatistics,
                                                   &(aIndexStat->mIndexPage),
                                                   aIndex->mSdnHeader.mIndexTSID,
                                                   sPID,
                                                   SDB_S_LATCH,
                                                   SDB_WAIT_NORMAL,
                                                   NULL,
                                                   (UChar**)&sPage,
                                                   &sIsSuccess)
                              != IDE_SUCCESS );
                    sFixState = ID_TRUE;
                    
                    continue;
                }
            }
            else
            {
                aStack->mDepth++;
                aStack->mStack[aStack->mDepth].mNodePID = sPID;
                aStack->mStack[aStack->mDepth].mSmoNo  = sdpPhyPage::getIndexSMONo(sPage);
                aStack->mStack[aStack->mDepth].mKeySeq = sKeySeq;

                *aLeafNode   = sPage;
                *aLeafKeySeq = sKeySeq;
            }
            
            break;
        }
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sFixState == ID_TRUE )
    {
        if( sdbBufferMgr::releasePage( aStatistics, (UChar*)sPage )
            != IDE_SUCCESS )
        {
            dumpIndexNode( sPage );
            IDE_ASSERT( 0 );
        }
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 키 삽입을 위해 Tree 탐색 중에 Split 된 노드를 만났을 경우 호출된다.
 * Stack를 따라 올가가며 Split이 발생하지 않은 노드를 찾는다.
 *********************************************************************/
IDE_RC stndrRTree::findValidStackDepth( idvSQL          * aStatistics,
                                        stndrStatistic  * aIndexStat,
                                        stndrHeader     * aIndex,
                                        stndrPathStack  * aStack,
                                        ULong           * aSmoNo )
{
    sdpPhyPageHdr   * sNode;
    stndrStackSlot    sSlot;
    ULong             sNodeSmoNo;
    idBool            sTrySuccess;

    while( 1 ) // 스택을 따라 올라가 본다
    {
        if( aStack->mDepth < 0 ) // root까지 SMO 발생
        {
            break;
        }
        else
        {
            sSlot = aStack->mStack[aStack->mDepth];
            
            IDE_TEST( stndrRTree::getPage( aStatistics,
                                           &(aIndexStat->mIndexPage),
                                           aIndex->mSdnHeader.mIndexTSID,
                                           sSlot.mNodePID,
                                           SDB_S_LATCH,
                                           SDB_WAIT_NORMAL,
                                           NULL,
                                           (UChar**)&sNode,
                                           &sTrySuccess )
                      != IDE_SUCCESS );
            
            sNodeSmoNo = sdpPhyPage::getIndexSMONo( sNode );

            IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sNode )
                      != IDE_SUCCESS );

            if( sNodeSmoNo <= sSlot.mSmoNo )
            {
                if( aSmoNo != NULL )
                {
                    *aSmoNo = sNodeSmoNo;
                }
                break; // 이 노드 하위부터 다시 traverse
            }

            aStack->mDepth--;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Internal Node에서 키를 삽입하기 위한 최적의 Key를 선택한다.
 *********************************************************************/
IDE_RC stndrRTree::findBestInternalKey( stndrKeyInfo  * aKeyInfo,
                                        stndrHeader   * /*aIndex*/,
                                        sdpPhyPageHdr * aNode,
                                        SShort        * aKeySeq,
                                        scPageID      * aChildPID,
                                        SDouble       * aDelta,
                                        ULong           aIndexSmoNo,
                                        idBool        * aIsRetry )
{
    stndrNodeHdr    * sNodeHdr;
    SDouble           sMinArea;
    SDouble           sMinDelta;
    SDouble           sDelta;
    UChar           * sSlotDirPtr;
    stndrIKey       * sIKey;
    UShort            sKeyCount;
    stdMBR            sKeyInfoMBR;
    stdMBR            sKeyMBR;
    scPageID          sMinChildPID;
    UInt              sMinSeq;
    UInt              i;

    
    *aIsRetry = ID_FALSE;

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );
    
    if( (sdpPhyPage::getIndexSMONo(aNode) > aIndexSmoNo) ||
        (sNodeHdr->mState == STNDR_IN_FREE_LIST) )
    {
        *aIsRetry = ID_TRUE;
        return IDE_SUCCESS; 
    }

    STNDR_GET_MBR_FROM_KEYINFO( sKeyInfoMBR, aKeyInfo );
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    sMinSeq = 0;
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                       sMinSeq,
                                                       (UChar**)&sIKey )
              != IDE_SUCCESS );

    STNDR_GET_MBR_FROM_IKEY( sKeyMBR, sIKey );

    sMinArea  = stdUtils::getMBRArea( &sKeyMBR );
    sMinDelta = stdUtils::getMBRDelta( &sKeyMBR, &sKeyInfoMBR );
    
    STNDR_GET_CHILD_PID( &sMinChildPID, sIKey );
    STNDR_GET_MBR_FROM_IKEY( sKeyMBR, sIKey );

    for( i = 1; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           (UChar**)&sIKey )
                  != IDE_SUCCESS );
        
        STNDR_GET_MBR_FROM_IKEY( sKeyMBR, sIKey );

        sDelta = stdUtils::getMBRDelta( &sKeyMBR, &sKeyInfoMBR );

        if( (sDelta < sMinDelta) ||
            ((sDelta == sMinDelta) &&
             (sMinArea > stdUtils::getMBRArea(&sKeyMBR))) )
        {
            sMinDelta = sDelta;
            sMinArea  = stdUtils::getMBRArea( &sKeyMBR );
            sMinSeq   = i;
            
            STNDR_GET_CHILD_PID( &sMinChildPID, sIKey );
        }
    }

    *aKeySeq    = sMinSeq;
    *aChildPID  = sMinChildPID;
    *aDelta     = sMinDelta;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Leaf Node에서 키를 삽입하기 위한 최적의 Key를 선택한다.
 *********************************************************************/
void stndrRTree::findBestLeafKey( sdpPhyPageHdr * aNode,
                                  SShort        * aKeySeq,
                                  ULong           aIndexSmoNo,
                                  idBool        * aIsRetry )
{
    stndrNodeHdr    * sNodeHdr;

    
    *aIsRetry = ID_FALSE;

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

    if( (sdpPhyPage::getIndexSMONo(aNode) > aIndexSmoNo) ||
        (sNodeHdr->mState == STNDR_IN_FREE_LIST) )
    {
        *aIsRetry = ID_TRUE;
        return;
    }

    *aKeySeq = sdpSlotDirectory::getCount(
        sdpPhyPage::getSlotDirStartPtr((UChar*)aNode) );

    return;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Node MBR를 구한다.
 *********************************************************************/
IDE_RC stndrRTree::adjustNodeMBR( stndrHeader   * aIndex,
                                  sdpPhyPageHdr * aNode,
                                  stdMBR        * aInsertMBR,
                                  SShort          aUpdateKeySeq,
                                  stdMBR        * aUpdateMBR,
                                  SShort          aDeleteKeySeq,
                                  stdMBR        * aNodeMBR )
{
    stndrNodeHdr    * sNodeHdr;

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

    if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        return adjustLNodeMBR( aIndex,
                               aNode,
                               aInsertMBR,
                               aUpdateKeySeq,
                               aUpdateMBR,
                               aDeleteKeySeq,
                               aNodeMBR );
    }
    else
    {
        return adjustINodeMBR( aIndex,
                               aNode,
                               aInsertMBR,
                               aUpdateKeySeq,
                               aUpdateMBR,
                               aDeleteKeySeq,
                               aNodeMBR );
    }
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Internal Node의 MBR을 구한다.
 *********************************************************************/
IDE_RC stndrRTree::adjustINodeMBR( stndrHeader   * /*aIndex*/,
                                   sdpPhyPageHdr * aNode,
                                   stdMBR        * aInsertMBR,
                                   SShort          aUpdateKeySeq,
                                   stdMBR        * aUpdateMBR,
                                   SShort          aDeleteKeySeq,
                                   stdMBR        * aNodeMBR )
{
    stndrIKey   * sIKey;
    stdMBR        sNodeMBR;
    stdMBR        sMBR;
    UChar       * sSlotDirPtr;
    UShort        sKeyCount;
    idBool        sIsFirst;
    SInt          i;


    sIsFirst = ID_TRUE;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    IDE_TEST( sKeyCount <= 0 );

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           (UChar**)&sIKey )
                  != IDE_SUCCESS );

        if( i == aDeleteKeySeq )
        {
            continue;
        }

        if( i == aUpdateKeySeq )
        {
            IDE_ASSERT( aUpdateMBR != NULL );
            sMBR = *aUpdateMBR;
        }
        else
        {
            STNDR_GET_MBR_FROM_IKEY( sMBR, sIKey );
        }

        if( sIsFirst == ID_TRUE )
        {
            sNodeMBR = sMBR;
            sIsFirst = ID_FALSE;
        }
        else
        {
            stdUtils::getMBRExtent( &sNodeMBR, &sMBR );
        }
    }

    // BUG-29039 codesonar ( Uninitialized Variable )
    // 발견하지 못한경우
    IDE_TEST( sIsFirst == ID_TRUE );

    if( aInsertMBR != NULL )
    {
        stdUtils::getMBRExtent( &sNodeMBR, aInsertMBR );
    }

    *aNodeMBR = sNodeMBR;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Leaf Node의 MBR을 구한다.
 *********************************************************************/
IDE_RC stndrRTree::adjustLNodeMBR( stndrHeader   * /*aIndex*/,
                                   sdpPhyPageHdr * aNode,
                                   stdMBR        * aInsertMBR,
                                   SShort          aUpdateKeySeq,
                                   stdMBR        * aUpdateMBR,
                                   SShort          aDeleteKeySeq,
                                   stdMBR        * aNodeMBR )
{
    stndrLKey   * sLKey;
    stdMBR        sNodeMBR;
    stdMBR        sMBR;
    UChar       * sSlotDirPtr;
    UShort        sKeyCount;
    idBool        sIsFirst;
    SInt          i;

    
    sIsFirst = ID_TRUE;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount   = sdpSlotDirectory::getCount( sSlotDirPtr );

    IDE_TEST( sKeyCount <= 0 );

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           (UChar**)&sLKey )
                  != IDE_SUCCESS );

        if( i == aDeleteKeySeq )
        {
            continue;
        }

        if( i == aUpdateKeySeq )
        {
            IDE_ASSERT( aUpdateMBR != NULL );
            sMBR = *aUpdateMBR;
        }
        else
        {
            STNDR_GET_MBR_FROM_LKEY( sMBR, sLKey );
        }

        if( sIsFirst == ID_TRUE )
        {
            sNodeMBR = sMBR;
            sIsFirst = ID_FALSE;
        }
        else
        {
            stdUtils::getMBRExtent( &sNodeMBR, &sMBR );
        }
    }

    // BUG-29039 codesonar ( Uninitialized Variable )
    // 발견하지 못한경우
    IDE_TEST( sIsFirst == ID_TRUE );

    if( aInsertMBR != NULL )
    {
        stdUtils::getMBRExtent( &sNodeMBR, aInsertMBR );
    }
    
    *aNodeMBR = sNodeMBR;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::softKeyStamping 
 * ------------------------------------------------------------------*
 * Internal SoftKeyStamping을 위한 Wrapper Function 
 *********************************************************************/
IDE_RC stndrRTree::softKeyStamping( sdrMtx          * aMtx,
                                    sdpPhyPageHdr   * aNode,
                                    UChar             aCTSlotNum,
                                    UChar           * aContext )
{
    stndrCallbackContext * sContext;

    sContext = (stndrCallbackContext*)aContext;
    return softKeyStamping( sContext->mIndex,
                            aMtx,
                            aNode,
                            aCTSlotNum );
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::softKeyStamping 
 * ------------------------------------------------------------------*
 * Soft Key Stamping은 CTS가 STAMPED상태에서 수행된다. 
 * CTS#를 갖는 모든 KEY들에 대해서 CTS#를 무한대로 변경한다. 
 * 만약 CreateCTS가 무한대로 변경되면 Key의 상태는 STABLE상태로 
 * 변경되고, LimitCTS가 무한대인 경우는 DEAD상태로 변경시킨다. 
 *********************************************************************/
IDE_RC stndrRTree::softKeyStamping( stndrHeader     * /* aIndex */,
                                    sdrMtx          * aMtx,
                                    sdpPhyPageHdr   * aNode,
                                    UChar             aCTSlotNum )
{
    UShort            sKeyCount;
    stndrLKey       * sLeafKey;
    UInt              i;
    stndrNodeHdr    * sNodeHdr;
    UShort            sKeyLen;
    UShort            sTotalDeadKeySize = 0;
    UShort          * sArrRefKey;
    UShort            sRefKeyCount;
    UShort            sAffectedKeyCount = 0;
    UChar           * sSlotDirPtr;
    smSCN             sCSSCNInfinite;

    
    SM_SET_SCN_CI_INFINITE( &sCSSCNInfinite );

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );
    
    sTotalDeadKeySize = sNodeHdr->mTotalDeadKeySize;
    
    IDE_TEST( sdrMiniTrans::writeLogRec(
                  aMtx,
                  (UChar*)aNode,
                  NULL,
                  ID_SIZEOF(aCTSlotNum),
                  SDR_STNDR_KEY_STAMPING)
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write(aMtx,
                                  (void*)&aCTSlotNum,
                                  ID_SIZEOF(aCTSlotNum))
              != IDE_SUCCESS );

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

    sdnIndexCTL::getRefKey( aNode,
                            aCTSlotNum,
                            &sRefKeyCount,
                            &sArrRefKey );

    for ( i = 0; i < SDN_CTS_MAX_KEY_CACHE; i++ )
    {
        if( sArrRefKey[i] == SDN_CTS_KEY_CACHE_NULL )
        {
            continue;
        }

        sAffectedKeyCount++;
        sLeafKey = (stndrLKey*)(((UChar*)aNode) + sArrRefKey[i]);

        if( STNDR_GET_CCTS_NO(sLeafKey) == aCTSlotNum )
        {
            STNDR_SET_CCTS_NO( sLeafKey , SDN_CTS_INFINITE );

            // Create CTS는 Stamping이 되지 않고 Limit CTS만 Stamping된
            // 경우는 DEAD상태일수 있기 때문에 SKIP한다. 또한 
            // STNDR_KEY_DELETED 상태는 변경하지 않는다.
            if( STNDR_GET_STATE(sLeafKey) == STNDR_KEY_UNSTABLE )
            {
                STNDR_SET_STATE( sLeafKey , STNDR_KEY_STABLE );

                STNDR_SET_CSCN( sLeafKey , &sCSSCNInfinite );
            }
        }

        if( STNDR_GET_LCTS_NO(sLeafKey) == aCTSlotNum )
        {
            if( STNDR_GET_STATE( sLeafKey ) != STNDR_KEY_DELETED )
            {
                ideLog::log( IDE_SERVER_0,
                             "CTS slot number : %u"
                             "\nCTS key cache idx : %u\n",
                             aCTSlotNum, i );
                dumpIndexNode( aNode );
                IDE_ASSERT( 0 );
            }

            sKeyLen = getKeyLength( (UChar*)sLeafKey, ID_TRUE /* aIsLeaf */);
            sTotalDeadKeySize += sKeyLen + ID_SIZEOF( sdpSlotEntry );

            STNDR_SET_CCTS_NO( sLeafKey, SDN_CTS_INFINITE );
            STNDR_SET_LCTS_NO( sLeafKey , SDN_CTS_INFINITE );
            STNDR_SET_STATE( sLeafKey , STNDR_KEY_DEAD );

            STNDR_SET_LSCN( sLeafKey, &sCSSCNInfinite );
        }
    }

    if( sAffectedKeyCount < sRefKeyCount )
    {
        // full scan해서 Key Stamping을 한다.
        for( i = 0; i < sKeyCount; i++ )
        {
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                        (UChar*)sSlotDirPtr, 
                                                        i,
                                                        (UChar**)&sLeafKey )
                      != IDE_SUCCESS );

            if( STNDR_GET_CCTS_NO(sLeafKey) == aCTSlotNum )
            {
                STNDR_SET_CCTS_NO( sLeafKey , SDN_CTS_INFINITE );

                // Create CTS는 Stamping이 되지 않고 Limit CTS만 Stamping된
                // 경우는 DEAD상태일수 있기 때문에 SKIP한다. 또한 
                // STNDR_KEY_DELETED 상태는 변경하지 않는다.
                if( STNDR_GET_STATE(sLeafKey) == STNDR_KEY_UNSTABLE )
                {
                    STNDR_SET_STATE( sLeafKey , STNDR_KEY_STABLE );
                    
                    STNDR_SET_CSCN( sLeafKey , &sCSSCNInfinite );
                    
                }
            }

            if( STNDR_GET_LCTS_NO(sLeafKey) == aCTSlotNum )
            {
                if( STNDR_GET_STATE( sLeafKey ) != STNDR_KEY_DELETED )
                {
                    ideLog::log( IDE_SERVER_0,
                                 "CTS slot number : %u"
                                 "\nKey sequeunce : %u\n",
                                 aCTSlotNum, i );
                    dumpIndexNode( aNode );
                    IDE_ASSERT( 0 );
                }
                
                sKeyLen = getKeyLength( (UChar*)sLeafKey , ID_TRUE /*aIsLeaf*/);
                sTotalDeadKeySize += sKeyLen + ID_SIZEOF( sdpSlotEntry );

                STNDR_SET_CCTS_NO( sLeafKey, SDN_CTS_INFINITE );
                STNDR_SET_LCTS_NO( sLeafKey , SDN_CTS_INFINITE );
                STNDR_SET_STATE( sLeafKey , STNDR_KEY_DEAD );

                STNDR_SET_LSCN( sLeafKey, &sCSSCNInfinite );
            }
        }
    }

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  aMtx,
                  (UChar*)&sNodeHdr->mTotalDeadKeySize,
                  (void*)&sTotalDeadKeySize,
                  ID_SIZEOF(sNodeHdr->mTotalDeadKeySize))
              != IDE_SUCCESS );

    IDE_TEST( sdnIndexCTL::freeCTS(aMtx,
                                   aNode,
                                   aCTSlotNum,
                                   ID_TRUE)
              != IDE_SUCCESS );
    
#if DEBUG
    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                        (UChar*)sSlotDirPtr, 
                                                        i,
                                                        (UChar**)&sLeafKey )
                  != IDE_SUCCESS );

        if( STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DEAD )
        {
            continue;
        }

        // SoftKeyStamping을 했는데도 CTS#가 변경되지 않은 Key는 있을수
        // 없다.
        IDE_ASSERT( STNDR_GET_CCTS_NO(sLeafKey) != aCTSlotNum );
        IDE_ASSERT( STNDR_GET_LCTS_NO(sLeafKey) != aCTSlotNum );
        if( (STNDR_GET_CCTS_NO( sLeafKey  ) == aCTSlotNum)
            ||
            (STNDR_GET_LCTS_NO( sLeafKey  ) == aCTSlotNum) )
        {
            ideLog::log( IDE_SERVER_0,
                         "CTS slot number : %u"
                         "\nKey sequence : %u\n",
                         aCTSlotNum, i );
            dumpIndexNode( aNode );
            IDE_ASSERT( 0 );
        }
    }
#endif
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::hardKeyStamping 
 * ------------------------------------------------------------------*
 * Internal HardKeyStamping을 위한 Wrapper Function 
 *********************************************************************/
IDE_RC stndrRTree::hardKeyStamping( idvSQL        * aStatistics,
                                    sdrMtx        * aMtx,
                                    sdpPhyPageHdr * aNode,
                                    UChar           aCTSlotNum,
                                    UChar         * aContext,
                                    idBool        * aSuccess )
{
    stndrCallbackContext * sContext = (stndrCallbackContext*)aContext;

    IDE_TEST( hardKeyStamping( aStatistics,
                               sContext->mIndex,
                               aMtx,
                               aNode,
                               aCTSlotNum,
                               aSuccess )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::hardKeyStamping 
 * ------------------------------------------------------------------*
 * 필요하다면 TSS로 부터 CommitSCN을 구해와서, SoftKeyStamping을 
 * 시도 한다. 
 *********************************************************************/
IDE_RC stndrRTree::hardKeyStamping( idvSQL          * aStatistics,
                                    stndrHeader     * aIndex,
                                    sdrMtx          * aMtx,
                                    sdpPhyPageHdr   * aNode,
                                    UChar             aCTSlotNum,
                                    idBool          * aIsSuccess )
{
    sdnCTL  * sCTL;
    sdnCTS  * sCTS;
    idBool    sSuccess = ID_TRUE;
    smSCN     sSysMinDskViewSCN;
    smSCN     sCommitSCN;
    
    sCTL = sdnIndexCTL::getCTL( aNode );
    sCTS = sdnIndexCTL::getCTS( sCTL, aCTSlotNum );

    if( sCTS->mState == SDN_CTS_UNCOMMITTED )
    {
        IDE_TEST( sdnIndexCTL::delayedStamping( aStatistics,
                                                NULL,        /* aTrans */
                                                sCTS,
                                                SDB_SINGLE_PAGE_READ,
                                                SM_SCN_INIT, /* aStmtViewSCN */ 
                                                &sCommitSCN,
                                                &sSuccess )
                  != IDE_SUCCESS );
    }

    if( sSuccess == ID_TRUE )
    {
        IDE_DASSERT( sCTS->mState == SDN_CTS_STAMPED );

        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

        sCommitSCN = sdnIndexCTL::getCommitSCN( sCTS );

        if( SM_SCN_IS_LT( &sCommitSCN, &sSysMinDskViewSCN ) )
        {
            IDE_TEST( softKeyStamping( aIndex,
                                       aMtx,
                                       aNode,
                                       aCTSlotNum )
                      != IDE_SUCCESS );
            *aIsSuccess = ID_TRUE;
        }
        else
        {
            *aIsSuccess = ID_FALSE;
        }
    }
    else
    {
        *aIsSuccess = ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Meta Page의 정보를 갱신한다.
 *********************************************************************/
IDE_RC stndrRTree::buildMeta( idvSQL    * aStatistics,
                              void      * aTrans,
                              void      * aIndex )
{
    stndrHeader     * sIndex = (stndrHeader*)aIndex;
    sdrMtx            sMtx;
    scPageID        * sRootNode;
    scPageID        * sFreeNodeHead;
    ULong           * sFreeNodeCnt;
    idBool          * sIsConsistent;
    smLSN           * sCompletionLSN;
    idBool          * sIsCreatedWithLogging;
    idBool          * sIsCreatedWithForce;
    UShort          * sConvexhullPointNum;
    stndrStatistic    sDummyStat;
    idBool            sMtxStart = ID_FALSE;
    sdrMtxLogMode     sLogMode;
    

    // BUG-27328 CodeSonar::Uninitialized Variable
    idlOS::memset( &sDummyStat, 0, ID_SIZEOF(sDummyStat) );

    // index runtime header의 mLogging은 DML에서 사용되는 것이므로
    // index build 후 항상 ID_TRUE로 초기화시킴
    sIndex->mSdnHeader.mLogging = ID_TRUE;

    sIsConsistent         = &(sIndex->mSdnHeader.mIsConsistent);
    sCompletionLSN        = &(sIndex->mSdnHeader.mCompletionLSN);
    sIsCreatedWithLogging = &(sIndex->mSdnHeader.mIsCreatedWithLogging);
    sIsCreatedWithForce   = &(sIndex->mSdnHeader.mIsCreatedWithForce);
    
    sRootNode             = &(sIndex->mRootNode);
    sFreeNodeHead         = &(sIndex->mFreeNodeHead);
    sFreeNodeCnt          = &(sIndex->mFreeNodeCnt);
    sConvexhullPointNum   = &(sIndex->mConvexhullPointNum);

    sLogMode  = (*sIsCreatedWithLogging == ID_TRUE) ?
        SDR_MTX_LOGGING : SDR_MTX_NOLOGGING;

    IDE_TEST( sdrMiniTrans::begin(aStatistics,
                                  &sMtx,
                                  aTrans,
                                  sLogMode,
                                  ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                  gMtxDLogType ) != IDE_SUCCESS );
    sMtxStart = ID_TRUE;

    sdrMiniTrans::setNologgingPersistent( &sMtx );

    IDE_TEST( setIndexMetaInfo( aStatistics,
                                sIndex,
                                &sDummyStat,
                                &sMtx,
                                sRootNode,
                                sFreeNodeHead,
                                sFreeNodeCnt,
                                sIsCreatedWithLogging,
                                sIsConsistent,
                                sIsCreatedWithForce,
                                sCompletionLSN,
                                sConvexhullPointNum ) != IDE_SUCCESS );

    sMtxStart = ID_FALSE;
    IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if(sMtxStart == ID_TRUE)
    {
        (void)sdrMiniTrans::rollback( &sMtx );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrBTree::backupRuntimeHeader             *
 * ------------------------------------------------------------------*
 * MtxRollback으로 인한 RuntimeHeader 복구를 위해, 값들을 백업해둔다.
 *
 * aMtx      - [In] 대상 Mtx
 * aIndex    - [In] 백업할 RuntimeHeader
 *********************************************************************/
IDE_RC stndrRTree::backupRuntimeHeader( sdrMtx      * aMtx,
                                        stndrHeader * aIndex )
{
    /* Mtx가 Abort되면, PageImage만 Rollback되지 RuntimeValud는
     * 복구되지 않습니다. 
     * 따라서 Rollback시 이전 값으로 복구하도록 합니다.
     * 어차피 대상 Page에 XLatch를 잡기 때문에 동시에 한 Mtx만
     * 변경합니다. 따라서 백업본은 하나만 있으면 됩니다.*/
    sdrMiniTrans::addPendingJob( aMtx,
                                 ID_FALSE, // isCommitJob
                                 stndrRTree::restoreRuntimeHeader,
                                 (void*)aIndex );

    aIndex->mRootNode4MtxRollback        = aIndex->mRootNode;
    aIndex->mEmptyNodeHead4MtxRollback   = aIndex->mEmptyNodeHead;
    aIndex->mEmptyNodeTail4MtxRollback   = aIndex->mEmptyNodeTail;
    aIndex->mFreeNodeCnt4MtxRollback     = aIndex->mFreeNodeCnt;
    aIndex->mFreeNodeHead4MtxRollback    = aIndex->mFreeNodeHead;
    aIndex->mFreeNodeSCN4MtxRollback     = aIndex->mFreeNodeSCN;
    aIndex->mKeyCount4MtxRollback        = aIndex->mKeyCount;
    aIndex->mTreeMBR4MtxRollback         = aIndex->mTreeMBR;
    aIndex->mInitTreeMBR4MtxRollback     = aIndex->mInitTreeMBR;
    aIndex->mVirtualRootNode4MtxRollback = aIndex->mVirtualRootNode;

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::restoreRuntimeHeader            *
 * ------------------------------------------------------------------*
 * MtxRollback으로 인한 RuntimeHeader의 Meta값들을 복구함
 *
 * aMtx      - [In] 대상 Mtx
 * aIndex    - [In] 백업할 RuntimeHeader
 *********************************************************************/
IDE_RC stndrRTree::restoreRuntimeHeader( void      * aIndex )
{
    stndrHeader  * sIndex;

    IDE_ASSERT( aIndex != NULL );

    sIndex = (stndrHeader*) aIndex;

    sIndex->mRootNode        = sIndex->mRootNode4MtxRollback;
    sIndex->mEmptyNodeHead   = sIndex->mEmptyNodeHead4MtxRollback;
    sIndex->mEmptyNodeTail   = sIndex->mEmptyNodeTail4MtxRollback;
    sIndex->mFreeNodeCnt     = sIndex->mFreeNodeCnt4MtxRollback;
    sIndex->mFreeNodeHead    = sIndex->mFreeNodeHead4MtxRollback;
    sIndex->mFreeNodeSCN     = sIndex->mFreeNodeSCN4MtxRollback;
    sIndex->mKeyCount        = sIndex->mKeyCount4MtxRollback;
    sIndex->mTreeMBR         = sIndex->mTreeMBR4MtxRollback;
    sIndex->mInitTreeMBR     = sIndex->mInitTreeMBR4MtxRollback;
    sIndex->mVirtualRootNode = sIndex->mVirtualRootNode4MtxRollback;

    return IDE_SUCCESS;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::setIndexMetaInfo
 * ------------------------------------------------------------------*
 * Meta Page에 변경된 통계정보를 실제로 로깅한다.
 *********************************************************************/
IDE_RC stndrRTree::setIndexMetaInfo( idvSQL         * aStatistics,
                                     stndrHeader    * aIndex,
                                     stndrStatistic * aIndexStat,
                                     sdrMtx         * aMtx,
                                     scPageID       * aRootPID,
                                     scPageID       * aFreeNodeHead,
                                     ULong          * aFreeNodeCnt,
                                     idBool         * aIsCreatedWithLogging,
                                     idBool         * aIsConsistent,
                                     idBool         * aIsCreatedWithForce,
                                     smLSN          * aNologgingCompletionLSN,
                                     UShort         * aConvexhullPointNum )
{
    UChar       * sPage = NULL;
    stndrMeta   * sMeta = NULL;
    idBool        sIsSuccess = ID_FALSE;
    

    IDE_ASSERT( (aRootPID                != NULL) ||
                (aFreeNodeHead           != NULL) ||
                (aFreeNodeCnt            != NULL) ||
                (aIsConsistent           != NULL) ||
                (aIsCreatedWithLogging   != NULL) ||
                (aIsCreatedWithForce     != NULL) ||
                (aNologgingCompletionLSN != NULL) ||
                (aConvexhullPointNum     != NULL) );

    sPage = sdrMiniTrans::getPagePtrFromPageID(
        aMtx,
        aIndex->mSdnHeader.mIndexTSID,
        SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ));
    
    if( sPage == NULL )
    {
        IDE_TEST( stndrRTree::getPage(
                         aStatistics,
                         &(aIndexStat->mMetaPage),
                         aIndex->mSdnHeader.mIndexTSID,
                         SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ),
                         SDB_X_LATCH,
                         SDB_WAIT_NORMAL,
                         aMtx,
                         (UChar**)&sPage,
                         &sIsSuccess ) != IDE_SUCCESS );
    }

    sMeta = (stndrMeta*)( sPage + SMN_INDEX_META_OFFSET );

    if( aRootPID != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mRootNode,
                                             (void*)aRootPID,
                                             ID_SIZEOF(*aRootPID) )
                  != IDE_SUCCESS );
    }

    if( aFreeNodeHead != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mFreeNodeHead,
                                             (void*)aFreeNodeHead,
                                             ID_SIZEOF(*aFreeNodeHead) )
                  != IDE_SUCCESS );
    }
    
    if( aFreeNodeCnt != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mFreeNodeCnt,
                                             (void*)aFreeNodeCnt,
                                             ID_SIZEOF(*aFreeNodeCnt) )
                  != IDE_SUCCESS );
    }
    
    if( aIsConsistent != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mIsConsistent,
                                             (void*)aIsConsistent,
                                             ID_SIZEOF(*aIsConsistent) )
                  != IDE_SUCCESS );
    }
    
    if( aIsCreatedWithLogging != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mIsCreatedWithLogging,
                                             (void*)aIsCreatedWithLogging,
                                             ID_SIZEOF(*aIsCreatedWithLogging) )
                  != IDE_SUCCESS );
    }
    
    if( aIsCreatedWithForce != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mIsCreatedWithForce,
                                             (void*)aIsCreatedWithForce,
                                             ID_SIZEOF(*aIsCreatedWithForce) )
                  != IDE_SUCCESS );
    }
    
    if( aNologgingCompletionLSN != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&(sMeta->mNologgingCompletionLSN.mFileNo),
                      (void*)&(aNologgingCompletionLSN->mFileNo),
                      ID_SIZEOF(aNologgingCompletionLSN->mFileNo) )
                  != IDE_SUCCESS );
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&(sMeta->mNologgingCompletionLSN.mOffset),
                      (void*)&(aNologgingCompletionLSN->mOffset),
                      ID_SIZEOF(aNologgingCompletionLSN->mOffset) )
                  != IDE_SUCCESS );
    }

    if( aConvexhullPointNum != NULL )
    {
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&sMeta->mConvexhullPointNum,
                                             (void*)aConvexhullPointNum,
                                             ID_SIZEOF(*aConvexhullPointNum) )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::preparePages
 * ------------------------------------------------------------------*
 * 주어진 개수만큼의 페이지를 할당받을수 있을지 검사한다.
 *********************************************************************/
IDE_RC stndrRTree::preparePages( idvSQL         * aStatistics,
                                 stndrHeader    * aIndex,
                                 sdrMtx         * aMtx,
                                 idBool         * aMtxStart,
                                 UInt             aNeedPageCnt )
{
    sdcUndoSegment  * sUDSegPtr;
    smSCN             sSysMinDskViewSCN;
    

    SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

    if( (aIndex->mFreeNodeCnt < aNeedPageCnt) ||
        SM_SCN_IS_GE(&aIndex->mFreeNodeSCN, &sSysMinDskViewSCN) )
    {
        if( sdpSegDescMgr::getSegMgmtOp(
                      &(aIndex->mSdnHeader.mSegmentDesc))->mPrepareNewPages(
                          aStatistics,
                          aMtx,
                          aIndex->mSdnHeader.mIndexTSID,
                          &(aIndex->mSdnHeader.mSegmentDesc.mSegHandle),
                          aNeedPageCnt)
                  != IDE_SUCCESS )
        {
            *aMtxStart = ID_FALSE;
            IDE_TEST( sdrMiniTrans::commit(aMtx) != IDE_SUCCESS );
            IDE_TEST( 1 );
        }
    }

    /* BUG-24400 디스크 인덱스 SMO중에 Undo 공간부족으로 Rollback 해서는 안됩니다.
     *           SMO 연산 수행하기 전에 Undo 세그먼트에 Undo 페이지 하나를 확보한 후에
     *           수행하여야 한다. 확보하지 못하면, SpaceNotEnough 에러를 반환한다. */
    if ( ((smxTrans*)aMtx->mTrans)->getTXSegEntry() != NULL )
    {
        sUDSegPtr = smxTrans::getUDSegPtr( (smxTrans*)aMtx->mTrans );
        if( sUDSegPtr == NULL )
        {
            ideLog::log( IDE_SERVER_0, "Transaction TX Segment entry info:\n" );
            ideLog::logMem( IDE_SERVER_0,
                            (UChar *)(((smxTrans *)aMtx->mTrans)->getTXSegEntry()),
                            ID_SIZEOF(*((smxTrans *)aMtx->mTrans)->getTXSegEntry()) );
            IDE_ASSERT( 0 );
        }

        if( sUDSegPtr->prepareNewPage( aStatistics, aMtx )
            != IDE_SUCCESS )
        {
            *aMtxStart = ID_FALSE;
            IDE_TEST( sdrMiniTrans::commit(aMtx) != IDE_SUCCESS );
            IDE_TEST( 1 );
        }
    }
    else
    {
        // Top-Down Build Thread시에는 TXSegEntry를 할당하지 않으며,
        // UndoPage를 Prepare할 필요도 없다.
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::fixPage
 * ------------------------------------------------------------------*
 * To fix BUG-18252
 * 인덱스 페이지및 메타페이지의 접근 빈도에 대한 통계정보 구축
 *********************************************************************/
IDE_RC stndrRTree::fixPage( idvSQL          * aStatistics,
                            stndrPageStat   * aPageStat,
                            scSpaceID         aSpaceID,
                            scPageID          aPageID,
                            UChar          ** aRetPagePtr,
                            idBool          * aTrySuccess )
{
    idvSQL      * sStatistics;
    idvSession    sDummySession;
    idvSQL        sDummySQL;
    ULong         sGetPageCount;
    ULong         sReadPageCount;

    
    sDummySQL.mGetPageCount = 0;
    sDummySQL.mReadPageCount = 0;

    if( aStatistics == NULL )
    {
        //fix for UMR
        idvManager::initSession( &sDummySession, 0, NULL );

        idvManager::initSQL( &sDummySQL,
                             &sDummySession,
                             NULL,
                             NULL,
                             NULL,
                             NULL,
                             IDV_OWNER_UNKNOWN );
        sStatistics = &sDummySQL;
    }
    else
    {
        sStatistics = aStatistics;
    }

    sGetPageCount = sStatistics->mGetPageCount;
    sReadPageCount = sStatistics->mReadPageCount;

    IDE_TEST( sdbBufferMgr::fixPageByPID( sStatistics,
                                          aSpaceID,
                                          aPageID,
                                          aRetPagePtr,
                                          aTrySuccess )
              != IDE_SUCCESS );

    aPageStat->mGetPageCount += sStatistics->mGetPageCount - sGetPageCount;
    aPageStat->mReadPageCount += sStatistics->mReadPageCount - sReadPageCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::unfixPage
 * ------------------------------------------------------------------*
 * To fix BUG-18252
 * 인덱스 페이지및 메타페이지의 접근 빈도에 대한 통계정보 구축
 *********************************************************************/
IDE_RC stndrRTree::unfixPage( idvSQL * aStatistics, UChar * aPagePtr )
{
    IDE_TEST( sdbBufferMgr::unfixPage( aStatistics, aPagePtr ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : stndrRTree::setFreeNodeInfo
 * ------------------------------------------------------------------*
 * To fix BUG-23287
 * Free Node 정보를 Meta 페이지에 설정한다.
 * 1. Free Node Head를 설정
 * 2. Free Node Count를 설정
 * 3. Free Node SCN을 설정
 *********************************************************************/
IDE_RC stndrRTree::setFreeNodeInfo( idvSQL          * aStatistics,
                                    stndrHeader     * aIndex,
                                    stndrStatistic  * aIndexStat,
                                    sdrMtx          * aMtx,
                                    scPageID          aFreeNodeHead,
                                    ULong             aFreeNodeCnt,
                                    smSCN           * aFreeNodeSCN )
{
    UChar       * sPage;
    stndrMeta   * sMeta;
    idBool        sIsSuccess;

    IDE_DASSERT( aMtx != NULL );

    sPage = sdrMiniTrans::getPagePtrFromPageID(
        aMtx,
        aIndex->mSdnHeader.mIndexTSID,
        SD_MAKE_PID(aIndex->mSdnHeader.mMetaRID) );
    
    if( sPage == NULL )
    {
        // SegHdr 페이지 포인터를 구함
        IDE_TEST( stndrRTree::getPage(
                      aStatistics,
                      &(aIndexStat->mMetaPage),
                      aIndex->mSdnHeader.mIndexTSID,
                      SD_MAKE_PID( aIndex->mSdnHeader.mMetaRID ),
                      SDB_X_LATCH,
                      SDB_WAIT_NORMAL,
                      aMtx,
                      (UChar**)&sPage,
                      &sIsSuccess ) != IDE_SUCCESS );
    }

    sMeta = (stndrMeta*)( sPage + SMN_INDEX_META_OFFSET );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sMeta->mFreeNodeHead,
                                         (void*)&aFreeNodeHead,
                                         ID_SIZEOF(aFreeNodeHead) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sMeta->mFreeNodeCnt,
                                         (void*)&aFreeNodeCnt,
                                         ID_SIZEOF(aFreeNodeCnt) )
              != IDE_SUCCESS );

    /* BUG-32764 [st-disk-index] The RTree module writes the invalid log of
     * FreeNodeSCN */
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&sMeta->mFreeNodeSCN,
                                         (void*)aFreeNodeSCN,
                                         ID_SIZEOF(*aFreeNodeSCN) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Key Value의 Length를 구한다.
 *********************************************************************/
UShort stndrRTree::getKeyValueLength()
{
    UShort sTotalKeySize;

    sTotalKeySize = ID_SIZEOF( stdMBR );

    return sTotalKeySize;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Leaf Node에 Key를 삽입한다. 본 함수는 Leaf Node에 X-Latch가 잡힌
 * 상태에서 호출된다.
 *********************************************************************/
IDE_RC stndrRTree::insertKeyIntoLeafNode( sdrMtx                * aMtx,
                                          stndrHeader           * aIndex,
                                          smSCN                 * aInfiniteSCN,
                                          sdpPhyPageHdr         * aLeafNode,
                                          SShort                * aLeafKeySeq,
                                          stndrKeyInfo          * aKeyInfo,
                                          UChar                   aCTSlotNum,
                                          idBool                * aIsSuccess )
{
    UShort sKeyValueLen;
    UShort sKeyLength;
    

    *aIsSuccess = ID_TRUE;

    sKeyValueLen = getKeyValueLength();

    // BUG-26060 [SN] BTree Top-Down Build시 잘못된 CTS#가
    // 설정되고 있습니다.
    if( aKeyInfo->mKeyState == STNDR_KEY_STABLE )
    {
        sKeyLength = STNDR_LKEY_LEN( sKeyValueLen, STNDR_KEY_TB_CTS );
    }
    else
    {
        if( aCTSlotNum == SDN_CTS_INFINITE )
        {
            sKeyLength = STNDR_LKEY_LEN( sKeyValueLen, STNDR_KEY_TB_KEY );
        }
        else
        {
            sKeyLength = STNDR_LKEY_LEN( sKeyValueLen, STNDR_KEY_TB_CTS );
        }
    }
    
    if( canAllocLeafKey( aMtx,
                         aIndex,
                         aLeafNode,
                         (UInt)sKeyLength,
                         aLeafKeySeq ) != IDE_SUCCESS )
    {
        ideLog::log( IDE_SERVER_0,
                     "Leaf Key sequence : %u"
                     ", Leaf Key length : %u"
                     ", CTS Slot Num : %u\n",
                     *aLeafKeySeq, sKeyLength, aCTSlotNum );

        ideLog::log( IDE_SERVER_0, "Key info dump:\n" );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar *)aKeyInfo,
                        ID_SIZEOF(stndrKeyInfo) );

        dumpIndexNode( aLeafNode );
        IDE_ASSERT(0);
    }
    
    // Top-Down Build가 아닌 경우중에 CTS 할당을 실패한 경우는
    // TBK로 키를 생성한다.
    if( (aKeyInfo->mKeyState != STNDR_KEY_STABLE) &&
        (aCTSlotNum == SDN_CTS_INFINITE) )
    {
        IDE_TEST( insertLeafKeyWithTBK( aMtx,
                                        aIndex,
                                        aInfiniteSCN,
                                        aLeafNode,
                                        aKeyInfo,
                                        sKeyValueLen,
                                        *aLeafKeySeq )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( insertLeafKeyWithTBT( aMtx,
                                        aIndex,
                                        aCTSlotNum,
                                        aInfiniteSCN,
                                        aLeafNode,
                                        aKeyInfo,
                                        sKeyValueLen,
                                        *aLeafKeySeq )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * sdpPhyPage::getNonFragFreeSize에 대한 wrapper 함수이다.
 * 노드의 Key Count가 aIndex->mMaxKeyCount 이상일 경우 0을 반환한다.
 *********************************************************************/
UShort stndrRTree::getNonFragFreeSize( stndrHeader   * aIndex,
                                       sdpPhyPageHdr * aNode )
{
    UChar  * sSlotDirPtr;
    UShort   sKeyCount;
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount = sdpSlotDirectory::getCount( sSlotDirPtr );

    if( sKeyCount >= aIndex->mMaxKeyCount )
    {
        return 0;
    }

    return sdpPhyPage::getNonFragFreeSize(aNode);
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * sdpPhyPage::getTotalFreeSize에 대한 wrapper 함수이다.
 * 노드의 Key Count가 aIndex->mMaxKeyCount 이상일 경우 0을 반환한다.
 *********************************************************************/
UShort stndrRTree::getTotalFreeSize( stndrHeader   * aIndex,
                                     sdpPhyPageHdr * aNode )
{
    UChar  * sSlotDirPtr;
    UShort   sKeyCount;
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );
    sKeyCount = sdpSlotDirectory::getCount( sSlotDirPtr );

    if( sKeyCount >= aIndex->mMaxKeyCount )
    {
        return 0;
    }

    return sdpPhyPage::getTotalFreeSize(aNode);
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Page에 주어진 크기의 slot을 할당할 수 있는지 검사하고, 필요하면
 * compactPage까지 수행한다. 
 *********************************************************************/
IDE_RC stndrRTree::canAllocInternalKey( sdrMtx          * aMtx,
                                        stndrHeader     * aIndex,
                                        sdpPhyPageHdr   * aNode,
                                        UInt              aSaveSize,
                                        idBool            aExecCompact,
                                        idBool            aIsLogging )
{
    UShort            sNeededFreeSize;
    UShort            sBeforeFreeSize;
    stndrNodeHdr    * sNodeHdr;

    sNeededFreeSize = aSaveSize + ID_SIZEOF(sdpSlotEntry);
    
    if( getNonFragFreeSize(aIndex, aNode) < sNeededFreeSize )
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

        sBeforeFreeSize = getTotalFreeSize( aIndex, aNode );

        // compact page를 해도 slot을 할당받지 못하는 경우
        IDE_TEST( (UInt)(sBeforeFreeSize + sNodeHdr->mTotalDeadKeySize) <
                  (UInt)sNeededFreeSize );

        if( aExecCompact == ID_TRUE )
        {
            IDE_TEST( compactPage( aMtx,
                                   aNode,
                                   aIsLogging ) != IDE_SUCCESS);
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Page에 주어진 크기의 slot을 할당할 수 있는지 검사하고, 필요하면
 * compactPage까지 수행한다. 
 *********************************************************************/
IDE_RC stndrRTree::canAllocLeafKey( sdrMtx          * aMtx,
                                    stndrHeader     * aIndex,
                                    sdpPhyPageHdr   * aNode,
                                    UInt              aSaveSize,
                                    SShort          * aKeySeq )
{
    UShort            sNeedFreeSize;
    UShort            sBeforeFreeSize;
    stndrNodeHdr    * sNodeHdr;

    
    sNeedFreeSize = aSaveSize + ID_SIZEOF(sdpSlotEntry);
    
    if( getNonFragFreeSize( aIndex, aNode ) < sNeedFreeSize )
    {
        sNodeHdr = (stndrNodeHdr*)
            sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

        sBeforeFreeSize = getTotalFreeSize( aIndex, aNode );;

        if( sBeforeFreeSize + sNodeHdr->mTotalDeadKeySize < sNeedFreeSize )
        {
            if( aKeySeq != NULL )
            {
                adjustKeyPosition( aNode, aKeySeq );
            }
                
            IDE_TEST( compactPage( aMtx,
                                   aNode,
                                   ID_TRUE ) != IDE_SUCCESS );

            // 이 경우는 할당할 수 없는 경우 이므로 FAILURE 처리한다.
            IDE_TEST( 1 );
        }
        else
        {
            if( aKeySeq != NULL )
            {
                adjustKeyPosition( aNode, aKeySeq );
            }
            
            IDE_TEST( compactPage( aMtx,
                                   aNode,
                                   ID_TRUE ) != IDE_SUCCESS);
            
            IDE_ASSERT( getNonFragFreeSize(aIndex, aNode) >= sNeedFreeSize );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 키 삽입 중 발생된 page split에 의해 기존 노드를 compact한다.
 * Logging을 한 후 실제 수행함수를 호출한다.
 *********************************************************************/
IDE_RC stndrRTree::compactPage( sdrMtx          * aMtx,
                                sdpPhyPageHdr   * aPage,
                                idBool            aIsLogging )
{
    stndrNodeHdr    * sNodeHdr;
    

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aPage );

    if( aIsLogging == ID_TRUE )
    {
        IDE_TEST( sdrMiniTrans::writeLogRec(
                      aMtx,
                      (UChar*)aPage,
                      NULL, // value
                      0,    // valueSize
                      SDR_STNDR_COMPACT_INDEX_PAGE )
                  != IDE_SUCCESS );
    }

    if( STNDR_IS_LEAF_NODE(sNodeHdr) == ID_TRUE )
    {
        compactPageLeaf( aPage );
    }
    else
    {
        compactPageInternal( aPage );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * 키 삽입 중 발생된 page split에 의해 기존 노드를 compact한다.
 *********************************************************************/
IDE_RC stndrRTree::compactPageInternal( sdpPhyPageHdr * aPage )
{

    SInt          i;
    UShort        sTmpBuf[SD_PAGE_SIZE]; // 2 * Page size -> align 문제...
    UChar       * sTmpPage;
    UChar       * sSlotDirPtr;
    UShort        sKeyLength;
    UShort        sAllowedSize;
    scOffset      sSlotOffset;
    UChar       * sSrcKey;
    UChar       * sDstKey;
    UShort        sKeyCount;
    

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aPage );
    sKeyCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

    sTmpPage = ((vULong)sTmpBuf) % SD_PAGE_SIZE == 0
        ? (UChar*)sTmpBuf
        : (UChar*)sTmpBuf + SD_PAGE_SIZE - (((vULong)sTmpBuf) % SD_PAGE_SIZE);

    idlOS::memcpy( sTmpPage, aPage, SD_PAGE_SIZE );
    
    if( aPage->mPageID != ((sdpPhyPageHdr*)sTmpPage)->mPageID )
    {
        dumpIndexNode( aPage );
        dumpIndexNode( (sdpPhyPageHdr *)sTmpPage );
        IDE_ASSERT( 0 );
    }

    (void)sdpPhyPage::reset( aPage,
                             ID_SIZEOF(stndrNodeHdr),
                             NULL );

    sdpSlotDirectory::init( aPage );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sTmpPage);

    for( i = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           &sSrcKey )
                  != IDE_SUCCESS );

        sKeyLength = getKeyLength( sSrcKey, ID_FALSE ); //aIsLeaf

        IDE_ASSERT( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aPage,
                                           i,
                                           sKeyLength,
                                           ID_TRUE,
                                           &sAllowedSize,
                                           &sDstKey,
                                           &sSlotOffset,
                                           1 )
                    == IDE_SUCCESS );
        
        idlOS::memcpy( sDstKey, sSrcKey, sKeyLength );
        // Insert Logging할 필요 없음.
    }
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 키 삽입 중 발생된 page split에 의해 기존 노드를 compact한다.
 * Compaction이후에 기존 CTS.refKey 정보가 invalid하기 때문에 이를
 * 보정해줄 필요가 있다. 
 *********************************************************************/
IDE_RC stndrRTree::compactPageLeaf( sdpPhyPageHdr * aPage )
{
    SInt              i;
    UShort            sTmpBuf[SD_PAGE_SIZE]; // 2 * Page size -> align 문제...
    UChar           * sTmpPage;
    UChar           * sSlotDirPtr;
    UShort            sKeyLength;
    UShort            sAllowedSize;
    scOffset          sSlotOffset;
    UChar           * sSrcKey;
    UChar           * sDstKey;
    UShort            sKeyCount;
    stndrLKey       * sLeafKey;
    UChar             sCreateCTS;
    UChar             sLimitCTS;
    stndrNodeHdr    * sNodeHdr;
    SShort            sKeySeq;
    UChar           * sDummy;
    

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr((UChar*)aPage);
    sKeyCount  = sdpSlotDirectory::getCount(sSlotDirPtr);
    sNodeHdr    = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr((UChar*)aPage);

    sTmpPage = ((vULong)sTmpBuf) % SD_PAGE_SIZE == 0
        ? (UChar*)sTmpBuf
        : (UChar*)sTmpBuf + SD_PAGE_SIZE - (((vULong)sTmpBuf) % SD_PAGE_SIZE);

    idlOS::memcpy(sTmpPage, aPage, SD_PAGE_SIZE);
    if( aPage->mPageID != ((sdpPhyPageHdr*)sTmpPage)->mPageID )
    {
        dumpIndexNode( aPage );
        dumpIndexNode( (sdpPhyPageHdr *)sTmpPage );
        IDE_ASSERT( 0 );
    }

    (void)sdpPhyPage::reset( aPage,
                             ID_SIZEOF(stndrNodeHdr),
                             NULL );

    // sdpPhyPage::reset에서는 CTL을 초기화 해주지는 않는다.
    sdpPhyPage::initCTL( aPage,
                         (UInt)sdnIndexCTL::getCTLayerSize(sTmpPage),
                         &sDummy );

    sdpSlotDirectory::init( aPage );

    sdnIndexCTL::cleanAllRefInfo( aPage );

    sNodeHdr->mTotalDeadKeySize = 0;

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sTmpPage);
    
    for( i = 0, sKeySeq = 0; i < sKeyCount; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           i,
                                                           &sSrcKey)
                  != IDE_SUCCESS );
        
        sLeafKey = (stndrLKey*)sSrcKey;
            
        if( STNDR_GET_STATE( sLeafKey ) == STNDR_KEY_DEAD )
        {
            continue;
        }
        
        sKeyLength = getKeyLength( sSrcKey, ID_TRUE ); //aIsLeaf

        IDE_ASSERT( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aPage,
                                           sKeySeq,
                                           sKeyLength,
                                           ID_TRUE,
                                           &sAllowedSize,
                                           &sDstKey,
                                           &sSlotOffset,
                                           1 )
                    == IDE_SUCCESS );

        sKeySeq++;
       
        idlOS::memcpy( sDstKey, sSrcKey, sKeyLength );

        sCreateCTS = STNDR_GET_CCTS_NO( sLeafKey );
        sLimitCTS  = STNDR_GET_LCTS_NO( sLeafKey );

        if ( SDN_IS_VALID_CTS(sCreateCTS) )
        {
            sdnIndexCTL::addRefKey( aPage,
                                    sCreateCTS,
                                    sSlotOffset );
        }

        if ( SDN_IS_VALID_CTS(sLimitCTS) )
        {
            sdnIndexCTL::addRefKey( aPage,
                                    sLimitCTS,
                                    sSlotOffset );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Key의 Length를 구한다.
 *********************************************************************/
UShort stndrRTree::getKeyLength( UChar * aKey, idBool aIsLeaf )
{
    UShort sTotalKeySize ;

    
    IDE_ASSERT( aKey != NULL );

    if( aIsLeaf == ID_TRUE )
    {
        sTotalKeySize = getKeyValueLength();
        
        sTotalKeySize =
            STNDR_LKEY_LEN( sTotalKeySize,
                            STNDR_GET_TB_TYPE( (stndrLKey*)aKey ) );
    }
    else
    {
        sTotalKeySize = getKeyValueLength();
        
        sTotalKeySize = STNDR_IKEY_LEN( sTotalKeySize );
    }

    IDE_ASSERT( sTotalKeySize < STNDR_MAX_KEY_BUFFER_SIZE );

    return sTotalKeySize;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Transaction의 OldestSCN보다 작은 CommitSCN을 갖는 CTS에 대해서
 * Soft Key Stamping(Aging)을 수행한다.
 *********************************************************************/
IDE_RC stndrRTree::selfAging( stndrHeader   * aIndex,
                              sdrMtx        * aMtx,
                              sdpPhyPageHdr * aNode,
                              smSCN         * aOldestSCN,
                              UChar         * aAgedCount )
{
    sdnCTL  * sCTL;
    sdnCTS  * sCTS;
    UInt      i;
    smSCN     sCommitSCN;
    UChar     sAgedCount = 0;
    

    sCTL = sdnIndexCTL::getCTL( aNode );

    for( i = 0; i < sdnIndexCTL::getCount(sCTL); i++ )
    {
        sCTS = sdnIndexCTL::getCTS( sCTL, i );

        if( sCTS->mState == SDN_CTS_STAMPED )
        {
            sCommitSCN = sdnIndexCTL::getCommitSCN( sCTS );
            if( SM_SCN_IS_LT(&sCommitSCN, aOldestSCN) )
            {
                IDE_TEST( softKeyStamping(aIndex,
                                          aMtx,
                                          aNode,
                                          i)
                          != IDE_SUCCESS );
                
                sAgedCount++;
            }
        }
    }

    *aAgedCount = sAgedCount;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * TBT 형태의 키를 삽입한다. 
 * 트랜잭션의 정보를 CTS에 Binding 한다. 
 *********************************************************************/
IDE_RC stndrRTree::insertLeafKeyWithTBT( sdrMtx                 * aMtx,
                                         stndrHeader            * aIndex,
                                         UChar                    aCTSlotNum,
                                         smSCN                  * aInfiniteSCN,
                                         sdpPhyPageHdr          * aLeafNode,
                                         stndrKeyInfo           * aKeyInfo,
                                         UShort                   aKeyValueLen,
                                         SShort                   aKeySeq )
{
    UShort  sKeyOffset;
    UShort  sAllocatedKeyOffset;
    UShort  sKeyLength;

    
    sKeyLength = STNDR_LKEY_LEN( aKeyValueLen, STNDR_KEY_TB_CTS );
    sKeyOffset = sdpPhyPage::getAllocSlotOffset( aLeafNode, sKeyLength );
        
    if( aKeyInfo->mKeyState != STNDR_KEY_STABLE )
    {
        IDE_ASSERT( aCTSlotNum != SDN_CTS_INFINITE );
            
        IDE_TEST( sdnIndexCTL::bindCTS( aMtx,
                                        aIndex->mSdnHeader.mIndexTSID,
                                        aLeafNode,
                                        aCTSlotNum,
                                        sKeyOffset)
                  != IDE_SUCCESS );
    }

    IDE_TEST( insertLKey(aMtx,
                         aIndex,
                         (sdpPhyPageHdr*)aLeafNode,
                         aKeySeq,
                         aCTSlotNum,
                         aInfiniteSCN,
                         STNDR_KEY_TB_CTS,
                         aKeyInfo,
                         aKeyValueLen,
                         ID_TRUE, //aIsLogging
                         &sAllocatedKeyOffset)
              != IDE_SUCCESS );

    if( sKeyOffset != sAllocatedKeyOffset )
    {
        ideLog::log( IDE_SERVER_0,
                     "Key Offset : %u"
                     ", Allocated Key Offset : %u"
                     "\nKey sequence : %d"
                     ", CT slot number : %u"
                     ", Key Value size : %u"
                     "\nKey Info :\n",
                     sKeyOffset, sAllocatedKeyOffset,
                     aKeySeq, aCTSlotNum, aKeyValueLen );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar *)aKeyInfo, ID_SIZEOF(stndrKeyInfo) );
        dumpIndexNode( (sdpPhyPageHdr *)aLeafNode );
        IDE_ASSERT( 0 );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * TBK 형태의 키를 삽입한다. 
 * 해당 함수는 CTS를 할당할수 없는 경우에 호출되며, 트랜잭션의 
 * 정보를 KEY 자체에 Binding 한다. 
 *********************************************************************/
IDE_RC stndrRTree::insertLeafKeyWithTBK( sdrMtx         * aMtx,
                                         stndrHeader    * aIndex,
                                         smSCN*           aInfiniteSCN,
                                         sdpPhyPageHdr  * aLeafNode,
                                         stndrKeyInfo   * aKeyInfo,
                                         UShort           aKeyValueLen,
                                         SShort           aKeySeq )
{
    UShort sKeyLength;
    UShort sKeyOffset;
    UShort sAllocatedKeyOffset = 0;

    
    sKeyLength = STNDR_LKEY_LEN( aKeyValueLen, STNDR_KEY_TB_KEY );
    sKeyOffset = sdpPhyPage::getAllocSlotOffset( aLeafNode, sKeyLength );
        
    IDE_TEST( insertLKey( aMtx,
                          aIndex,
                          (sdpPhyPageHdr*)aLeafNode,
                          aKeySeq,
                          SDN_CTS_IN_KEY,
                          aInfiniteSCN,
                          STNDR_KEY_TB_KEY,
                          aKeyInfo,
                          aKeyValueLen,
                          ID_TRUE, //aIsLogging
                          &sAllocatedKeyOffset )
              != IDE_SUCCESS );
    
    if( sKeyOffset != sAllocatedKeyOffset )
    {
        ideLog::log( IDE_SERVER_0,
                     "Key Offset : %u"
                     ", Allocated Key Offset : %u"
                     "\nKey sequence : %d"
                     ", CTS slot number : %u"
                     ", Key Value size : %u"
                     "\nKey Info :\n",
                     sKeyOffset, sAllocatedKeyOffset,
                     aKeySeq, SDN_CTS_IN_KEY, aKeyValueLen );
        ideLog::logMem( IDE_SERVER_0,
                        (UChar *)aKeyInfo, ID_SIZEOF(stndrKeyInfo) );
        dumpIndexNode( (sdpPhyPageHdr *)aLeafNode );
        IDE_ASSERT( 0 );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Insert Position을 획득한 이후, Compaction으로 인하여 Insertable   
 * Position이 변경될수 있으며, 해당 함수는 이를 보정해주는 역할을
 * 한다. 
 *********************************************************************/
IDE_RC stndrRTree::adjustKeyPosition( sdpPhyPageHdr   * aNode,
                                    SShort          * aKeyPosition )
{
    UChar       * sSlotDirPtr;
    SInt          i;
    UChar       * sSlot;
    stndrLKey   * sLeafKey;
    SShort        sOrgKeyPosition;
    SShort        sAdjustedKeyPosition;
    
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aNode );

    sOrgKeyPosition = *aKeyPosition;
    sAdjustedKeyPosition = *aKeyPosition;
    
    for( i = 0; i < sOrgKeyPosition; i++ )
    {
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           &sSlot )
                  != IDE_SUCCESS );
        sLeafKey  = (stndrLKey*)sSlot;
            
        if( STNDR_GET_STATE(sLeafKey) == STNDR_KEY_DEAD )
        {
            sAdjustedKeyPosition--;
        }
    }

    *aKeyPosition = sAdjustedKeyPosition;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Leaf Node인 aNode 새 키를 insert한다. 
 *********************************************************************/
IDE_RC stndrRTree::insertLKey( sdrMtx           * aMtx,
                               stndrHeader      * aIndex,
                               sdpPhyPageHdr    * aNode,
                               SShort             aKeySeq,
                               UChar              aCTSlotNum,
                               smSCN            * aInfiniteSCN,
                               UChar              aTxBoundType,
                               stndrKeyInfo     * aKeyInfo,
                               UShort             aKeyValueLen,
                               idBool             aIsLoggableSlot,
                               UShort           * aKeyOffset )
{
    UShort                    sAllowedSize;
    stndrLKey               * sLKey;
    UShort                    sKeyLength;
    UShort                    sKeyCount;
    stndrRollbackContext      sRollbackContext;
    stndrNodeHdr            * sNodeHdr;
    scOffset                  sKeyOffset;
    UShort                    sTotalTBKCount = 0;
    smSCN                     sFstDskViewSCN;
    smSCN                     sCreateSCN;
    smSCN                     sLimitSCN;
    sdSID                     sTSSlotSID;
    stdMBR                    sKeyInfoMBR;
    stdMBR                    sNodeMBR;
    idBool                    sIsChangedMBR = ID_FALSE;
    IDE_RC                    sRc;

    
    sFstDskViewSCN = smLayerCallback::getFstDskViewSCN( aMtx->mTrans );
    sTSSlotSID     = smLayerCallback::getTSSlotSID( aMtx->mTrans );
    sKeyLength     = STNDR_LKEY_LEN( aKeyValueLen, aTxBoundType );
    
    SM_SET_SCN( &sCreateSCN, aInfiniteSCN );
    SM_SET_SCN_CI_INFINITE( &sLimitSCN );
    
    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aNode );

    sKeyCount = sdpSlotDirectory::getCount(
        sdpPhyPage::getSlotDirStartPtr((UChar*)aNode) );

    IDE_TEST( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aNode,
                                     aKeySeq,
                                     sKeyLength,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     (UChar**)&sLKey,
                                     &sKeyOffset,
                                     1 )
              != IDE_SUCCESS );

    // aKeyOffset이 NULL이 아닐 경우, Return 해달라는 뜻
    if( aKeyOffset != NULL )
    {
        *aKeyOffset = sKeyOffset;
    }
    
    IDE_ASSERT( aKeyInfo->mKeyState == STNDR_KEY_UNSTABLE ||
                aKeyInfo->mKeyState == STNDR_KEY_STABLE );
    
    IDE_ASSERT( sAllowedSize >= sKeyLength );
    
    idlOS::memset(sLKey, 0x00, sKeyLength );

    STNDR_GET_MBR_FROM_KEYINFO( sKeyInfoMBR, aKeyInfo );
    
    STNDR_KEYINFO_TO_LKEY( (*aKeyInfo), aKeyValueLen, sLKey,
                           aCTSlotNum,           //CCTS_NO
                           sCreateSCN,
                           SDN_CTS_INFINITE,     //LCTS_NO
                           sLimitSCN,
                           aKeyInfo->mKeyState,  //STATE
                           aTxBoundType );       //TB_TYPE

    IDE_ASSERT( (STNDR_GET_STATE(sLKey) == STNDR_KEY_UNSTABLE) ||
                (STNDR_GET_STATE(sLKey) == STNDR_KEY_STABLE) );
    
    if( aTxBoundType == STNDR_KEY_TB_KEY )
    {
        STNDR_SET_TBK_CSCN( ((stndrLKeyEx*)sLKey), &sFstDskViewSCN );
        STNDR_SET_TBK_CTSS( ((stndrLKeyEx*)sLKey), &sTSSlotSID );
    }
    
    sRollbackContext.mTableOID = aIndex->mSdnHeader.mTableOID;
    sRollbackContext.mIndexID  = aIndex->mSdnHeader.mIndexID;

    sNodeHdr->mUnlimitedKeyCount++;

    if( sKeyCount == 0 )
    {
        sIsChangedMBR = ID_TRUE;
        sNodeMBR = sKeyInfoMBR;
    }
    else
    {
        if( stdUtils::isMBRContains( &sNodeHdr->mMBR, &sKeyInfoMBR )
            != ID_TRUE )
        {
            sIsChangedMBR = ID_TRUE;
            sNodeMBR = sNodeHdr->mMBR;
            stdUtils::getMBRExtent( &sNodeMBR, &sKeyInfoMBR );
        }
    }

    if( (aIsLoggableSlot == ID_TRUE) &&
        (aIndex->mSdnHeader.mLogging == ID_TRUE) )
    {
        if( sIsChangedMBR == ID_TRUE )
        {
            sRc = setNodeMBR( aMtx, aNode, &sNodeMBR );
            if( sRc != IDE_SUCCESS )
            {
                ideLog::log( IDE_SERVER_0, "update MBR : \n" );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar*)&sNodeMBR,
                                ID_SIZEOF(stdMBR) );
                dumpIndexNode( aNode );
                IDE_ASSERT( 0 );
            }
        }
        
        if( aTxBoundType == STNDR_KEY_TB_KEY )
        {
            sTotalTBKCount = sNodeHdr->mTBKCount + 1;
            IDE_TEST( sdrMiniTrans::writeNBytes(
                          aMtx,
                          (UChar*)&(sNodeHdr->mTBKCount),
                          (void*)&sTotalTBKCount,
                          ID_SIZEOF(UShort))
                      != IDE_SUCCESS );
        }
        
        IDE_TEST( sdrMiniTrans::writeNBytes(
                      aMtx,
                      (UChar*)&(sNodeHdr->mUnlimitedKeyCount),
                      (void*)&sNodeHdr->mUnlimitedKeyCount,
                      ID_SIZEOF(UShort))
                  != IDE_SUCCESS );

        sdrMiniTrans::setRefOffset( aMtx, SM_OID_NULL );
        
        IDE_TEST( sdrMiniTrans::writeLogRec(aMtx,
                                            (UChar*)sLKey,
                                            (void*)NULL,
                                            ID_SIZEOF(aKeySeq) + sKeyLength +
                                            ID_SIZEOF(sRollbackContext),
                                            SDR_STNDR_INSERT_KEY)
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write(aMtx,
                                      (void*)&aKeySeq,
                                      ID_SIZEOF(SShort))
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write(aMtx,
                                      (void*)&sRollbackContext,
                                      ID_SIZEOF(stndrRollbackContext))
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write(aMtx,
                                      (void*)sLKey,
                                      sKeyLength)
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

UInt stndrRTree::getMinimumKeyValueLength( smnIndexHeader * aIndexHeader )
{
    UInt sTotalSize = 0;

    IDE_DASSERT( aIndexHeader != NULL );

    sTotalSize = ID_SIZEOF( stdMBR );

    
    return sTotalSize;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * TBK 형태의 키에서 Commit SCN을 얻어가는 함수 
 * 해당 트랜잭션이 Commit되었다면 Delayed Stamping을 시도해 본다. 
 *********************************************************************/
IDE_RC stndrRTree::getCommitSCN( idvSQL         * aStatistics,
                                 void           * aTrans,
                                 sdpPhyPageHdr  * aNode,
                                 stndrLKeyEx    * aLeafKeyEx,
                                 idBool           aIsLimit,
                                 smSCN            aStmtViewSCN,
                                 smSCN          * aCommitSCN )
{
    smSCN       sBeginSCN;
    smSCN       sCommitSCN;
    sdSID       sTSSlotSID;
    idBool      sTrySuccess = ID_FALSE;
    smTID       sTransID;

    if( aIsLimit == ID_FALSE )
    {
        STNDR_GET_TBK_CSCN( aLeafKeyEx, &sBeginSCN );
        STNDR_GET_TBK_CTSS( aLeafKeyEx, &sTSSlotSID );
    }
    else
    {
        STNDR_GET_TBK_LSCN( aLeafKeyEx, &sBeginSCN );
        STNDR_GET_TBK_LTSS( aLeafKeyEx, &sTSSlotSID );
    }
    
    if( SM_SCN_IS_VIEWSCN( sBeginSCN ) )
    {
        IDE_TEST( sdcTSSegment::getCommitSCN( aStatistics,
                                              aTrans,
                                              sTSSlotSID,
                                              &sBeginSCN,
                                              aStmtViewSCN,
                                              &sTransID,
                                              &sCommitSCN )
                  != IDE_SUCCESS );

        /*
         * 해당 트랜잭션이 Commit되었다면 Delayed Stamping을 시도해 본다.
         */
        if( SM_SCN_IS_INFINITE( sCommitSCN ) == ID_FALSE )
        {
            sdbBufferMgr::tryEscalateLatchPage( aStatistics,
                                                (UChar*)aNode,
                                                SDB_SINGLE_PAGE_READ,
                                                &sTrySuccess );

            if( sTrySuccess == ID_TRUE )
            {
                sdbBufferMgr::setDirtyPageToBCB( aStatistics,
                                                 (UChar*)aNode );
                if( aIsLimit == ID_FALSE )
                {
                    STNDR_SET_TBK_CSCN( aLeafKeyEx, &sCommitSCN );
                }
                else
                {
                    STNDR_SET_TBK_LSCN( aLeafKeyEx, &sCommitSCN );
                }
            }
        }
    }
    else
    {
        sCommitSCN = sBeginSCN;
    }

    SM_SET_SCN(aCommitSCN, &sCommitSCN);
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * TBK Key가 자신의 트랜잭션이 수정한 키인지 검사한다.
 *********************************************************************/
idBool stndrRTree::isMyTransaction( void*   aTrans,
                                    smSCN   aBeginSCN,
                                    sdSID   aTSSlotSID )
{
    smSCN sSCN;
    
    if ( aTSSlotSID == smLayerCallback::getTSSlotSID( aTrans ) )
    {
        sSCN = smLayerCallback::getFstDskViewSCN( aTrans );

        if( SM_SCN_IS_EQ( &aBeginSCN, &sSCN ) )
        {
            return ID_TRUE;
        }
    }
    
    return ID_FALSE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Rollback시 자신의 트랜잭션이 수정한 키인지 검사한다. aSCN으로
 * FstDskViewSCN 값을 받는다.
 *********************************************************************/
idBool stndrRTree::isMyTransaction( void   * aTrans,
                                    smSCN    aBeginSCN,
                                    sdSID    aTSSlotSID,
                                    smSCN  * aSCN )
{
    if ( aTSSlotSID == smLayerCallback::getTSSlotSID( aTrans ) )
    {
        if( SM_SCN_IS_EQ( &aBeginSCN, aSCN ) )
        {
            return ID_TRUE;
        }
    }
    
    return ID_FALSE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * FREE KEY들의 logical log를 기록한다.
 *********************************************************************/
IDE_RC stndrRTree::writeLogFreeKeys( sdrMtx         * aMtx,
                                     UChar          * aNode,
                                     stndrKeyArray  * aKeyArray,
                                     UShort           aFromSeq,
                                     UShort           aToSeq )
{
    SShort  sCount;
    
    if(aFromSeq <= aToSeq)
    {
        sCount = aToSeq - aFromSeq + 1;
        IDE_TEST( sdrMiniTrans::writeLogRec(
                      aMtx,
                      (UChar*)aNode,
                      NULL,
                      (ID_SIZEOF(sCount) + (ID_SIZEOF(stndrKeyArray) * sCount)),
                      SDR_STNDR_FREE_KEYS )
                  != IDE_SUCCESS);

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       (void*)&sCount,
                                       ID_SIZEOF(sCount) )
                  != IDE_SUCCESS );

        IDE_TEST( sdrMiniTrans::write( aMtx,
                                       (void*)&aKeyArray[aFromSeq],
                                       ID_SIZEOF(stndrKeyArray) * sCount )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Leaf Node에서 주어진 키를 삭제한다.
 * CTS가 할당 가능하다면 TBT(Transactin info Bound in CTS)로 키를
 * 만들어야 하고, 반대라면 TBK(Transaction info Bound in Key)로
 * 만들어야 한다.  
 *********************************************************************/
IDE_RC stndrRTree::deleteKeyFromLeafNode( idvSQL            * aStatistics,
                                          stndrStatistic    * aIndexStat,
                                          sdrMtx            * aMtx,
                                          stndrHeader       * aIndex,
                                          smSCN             * aInfiniteSCN,
                                          sdpPhyPageHdr     * aLeafNode,
                                          SShort            * aLeafKeySeq ,
                                          idBool            * aIsSuccess )
{
    stndrCallbackContext      sCallbackContext;
    UChar                     sCTSlotNum;
    UShort                    sKeyOffset;
    UChar                   * sSlotDirPtr;
    stndrLKey               * sLeafKey;

    
    *aIsSuccess = ID_TRUE;
    
    sCallbackContext.mIndex = (stndrHeader*)aIndex;
    sCallbackContext.mStatistics = aIndexStat;
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aLeafNode );
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                       *aLeafKeySeq,
                                                       (UChar**)&sLeafKey )
              != IDE_SUCCESS );
    IDE_TEST ( sdpSlotDirectory::getValue( sSlotDirPtr, 
                                           *aLeafKeySeq,
                                           &sKeyOffset )
               != IDE_SUCCESS );

    if( (STNDR_GET_STATE( sLeafKey ) != STNDR_KEY_UNSTABLE) &&
        (STNDR_GET_STATE( sLeafKey ) != STNDR_KEY_STABLE) )
    {
        ideLog::log( IDE_SERVER_0,
                     "Leaf key sequence number : %d\n",
                     *aLeafKeySeq );
        dumpIndexNode( aLeafNode );
        IDE_ASSERT( 0 );
    }
    
    IDE_TEST( sdnIndexCTL::allocCTS( aStatistics,
                                     aMtx,
                                     &aIndex->mSdnHeader.mSegmentDesc.mSegHandle,
                                     aLeafNode,
                                     &sCTSlotNum,
                                     &gCallbackFuncs4CTL,
                                     (UChar*)&sCallbackContext )
              != IDE_SUCCESS );

    /*
     * CTS 할당을 실패한 경우는 TBK로 키를 생성한다.
     */
    if( sCTSlotNum == SDN_CTS_INFINITE )
    {
        IDE_TEST( deleteLeafKeyWithTBK( aMtx,
                                        aIndex,
                                        aInfiniteSCN,
                                        aLeafNode,
                                        aLeafKeySeq ,
                                        aIsSuccess )
                  != IDE_SUCCESS );

        IDE_TEST_RAISE( *aIsSuccess == ID_FALSE, ALLOC_FAILURE );
    }
    else
    {
        IDE_TEST( deleteLeafKeyWithTBT( aMtx,
                                        aIndex,
                                        aInfiniteSCN,
                                        sCTSlotNum,
                                        aLeafNode,
                                        *aLeafKeySeq  )
                  != IDE_SUCCESS );
    }
    
    IDE_EXCEPTION_CONT( ALLOC_FAILURE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * TBT 형태의 키를 삭제한다. 
 * CTS에 삭제하는 트랜잭션의 정보를 Binding한다. 
 *********************************************************************/
IDE_RC stndrRTree::deleteLeafKeyWithTBT( sdrMtx         * aMtx,
                                         stndrHeader    * aIndex,
                                         smSCN          * aInfiniteSCN,
                                         UChar            aCTSlotNum,
                                         sdpPhyPageHdr  * aLeafNode,
                                         SShort           aLeafKeySeq )
{
    stndrRollbackContext      sRollbackContext;
    UChar                   * sSlotDirPtr;
    stndrLKey               * sLeafKey;
    UShort                    sKeyOffset;
    UShort                    sKeyLength;
    stndrNodeHdr            * sNodeHdr;
    SShort                    sDummyKeySeq;
    idBool                    sRemoveInsert;
    UShort                    sUnlimitedKeyCount;
    smSCN                     sFstDiskViewSCN;

    
    sFstDiskViewSCN = smLayerCallback::getFstDskViewSCN( aMtx->mTrans );
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aLeafNode );
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                       aLeafKeySeq,
                                                       (UChar**)&sLeafKey )
              != IDE_SUCCESS );
    IDE_TEST( sdpSlotDirectory::getValue( sSlotDirPtr, 
                                          aLeafKeySeq,
                                          &sKeyOffset )
              != IDE_SUCCESS );
    sKeyLength = getKeyLength( (UChar*)sLeafKey ,
                               ID_TRUE /* aIsLeaf */ );

    IDE_TEST( sdnIndexCTL::bindCTS( aMtx,
                                    aIndex->mSdnHeader.mIndexTSID,
                                    aLeafNode,
                                    aCTSlotNum,
                                    sKeyOffset )
              != IDE_SUCCESS );
    
    sRollbackContext.mTableOID = aIndex->mSdnHeader.mTableOID;
    sRollbackContext.mIndexID  = aIndex->mSdnHeader.mIndexID;
    sRollbackContext.mFstDiskViewSCN = sFstDiskViewSCN;

    SM_SET_SCN( &sRollbackContext.mLimitSCN, aInfiniteSCN );

    STNDR_SET_LCTS_NO( sLeafKey , aCTSlotNum );
    STNDR_SET_LSCN( sLeafKey, aInfiniteSCN );
    STNDR_SET_STATE( sLeafKey , STNDR_KEY_DELETED );

    sNodeHdr = (stndrNodeHdr*)
        sdpPhyPage::getLogicalHdrStartPtr((UChar*)aLeafNode);
    sUnlimitedKeyCount = sNodeHdr->mUnlimitedKeyCount - 1;
    IDE_DASSERT( sUnlimitedKeyCount < sdpSlotDirectory::getCount(sSlotDirPtr) );
                 
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sNodeHdr->mUnlimitedKeyCount),
                                         (void*)&sUnlimitedKeyCount,
                                         ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    sdrMiniTrans::setRefOffset( aMtx, SM_OID_NULL );
    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)sLeafKey ,
                                         (void*)NULL,
                                         sKeyLength +
                                         ID_SIZEOF(stndrRollbackContext) +
                                         ID_SIZEOF(SShort) +
                                         ID_SIZEOF(UChar),
                                         SDR_STNDR_DELETE_KEY_WITH_NTA )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&sRollbackContext,
                                   ID_SIZEOF(stndrRollbackContext) )
              != IDE_SUCCESS );

    sRemoveInsert = ID_FALSE;
    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&sRemoveInsert,
                                   ID_SIZEOF(UChar) )
              != IDE_SUCCESS );

    sDummyKeySeq = 0; /* 의미없는 값 */
    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&sDummyKeySeq,
                                   ID_SIZEOF(SShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)sLeafKey ,
                                   sKeyLength )
              != IDE_SUCCESS );
        
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * TBK 형태의 키를 삭제한다. 
 * Key 자체에 삭제하는 트랜잭션의 정보를 Binding 한다. 
 * 기존 키가 TBK라면 새로운 공간을 할당할 필요가 없지만 반대의 
 * 경우는 키를 위한 공간을 할당해야 한다. 
 *********************************************************************/
IDE_RC stndrRTree::deleteLeafKeyWithTBK( sdrMtx         * aMtx,
                                         stndrHeader    * aIndex,
                                         smSCN          * aInfiniteSCN,
                                         sdpPhyPageHdr  * aLeafNode,
                                         SShort         * aLeafKeySeq ,
                                         idBool         * aIsSuccess )
{
    smSCN                     sSysMinDskViewSCN;
    UChar                     sAgedCount = 0;
    UShort                    sKeyLength;
    UShort                    sAllowedSize;
    UChar                   * sSlotDirPtr;
    stndrLKey               * sLeafKey;
    UChar                     sTxBoundType;
    stndrLKey               * sRemoveKey;
    stndrRollbackContext      sRollbackContext;
    UShort                    sUnlimitedKeyCount;
    stndrNodeHdr            * sNodeHdr;
    UShort                    sTotalTBKCount = 0;
    UChar                     sRemoveInsert = ID_FALSE;
    smSCN                     sFstDiskViewSCN;
    sdSID                     sTSSlotSID;
    scOffset                  sOldKeyOffset;
    scOffset                  sNewKeyOffset;

    *aIsSuccess = ID_TRUE;
    
    sFstDiskViewSCN = smLayerCallback::getFstDskViewSCN( aMtx->mTrans );
    sTSSlotSID      = smLayerCallback::getTSSlotSID( aMtx->mTrans );
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aLeafNode );
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                       *aLeafKeySeq,
                                                       (UChar**)&sRemoveKey )
              != IDE_SUCCESS );

    sTxBoundType = STNDR_GET_TB_TYPE( sRemoveKey );

    sKeyLength = STNDR_LKEY_LEN( getKeyValueLength(),
                                 STNDR_KEY_TB_KEY );
    
    if( sTxBoundType == STNDR_KEY_TB_KEY )
    {
        sRemoveInsert = ID_FALSE;
        sLeafKey  = sRemoveKey;
        IDE_RAISE( SKIP_ALLOC_SLOT );
    }
    
    /*
     * canAllocLeafKey 에서의 Compaction으로 인하여
     * KeySeq가 변경될수 있다.
     */
    if( canAllocLeafKey ( aMtx,
                          aIndex,
                          aLeafNode,
                          (UInt)sKeyLength,
                          aLeafKeySeq  ) != IDE_SUCCESS )
    {
        SMX_GET_MIN_DISK_VIEW( &sSysMinDskViewSCN );

        /*
         * 적극적으로 공간 할당을 위해서 Self Aging을 한다.
         */
        IDE_TEST( selfAging( aIndex,
                             aMtx,
                             aLeafNode,
                             &sSysMinDskViewSCN,
                             &sAgedCount )
                  != IDE_SUCCESS );

        if( sAgedCount > 0 )
        {
            if( canAllocLeafKey ( aMtx,
                                  aIndex,
                                  aLeafNode,
                                  (UInt)sKeyLength,
                                  aLeafKeySeq  )
                != IDE_SUCCESS )
            {
                *aIsSuccess = ID_FALSE;
            }
        }
        else
        {
            *aIsSuccess = ID_FALSE;
        }
    }
    
    IDE_TEST_RAISE( *aIsSuccess == ID_FALSE, ALLOC_FAILURE );

    sRemoveInsert = ID_TRUE;
    IDE_TEST( sdpPhyPage::allocSlot( (sdpPhyPageHdr*)aLeafNode,
                                     *aLeafKeySeq,
                                     sKeyLength,
                                     ID_TRUE,
                                     &sAllowedSize,
                                     (UChar**)&sLeafKey,
                                     &sNewKeyOffset,
                                     1 )
              != IDE_SUCCESS );
    
    idlOS::memset( sLeafKey , 0x00, sKeyLength );

    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                       *aLeafKeySeq  + 1,
                                                       (UChar**)&sRemoveKey )
              != IDE_SUCCESS );

    idlOS::memcpy( sLeafKey , sRemoveKey, ID_SIZEOF( stndrLKey ) );
    STNDR_SET_TB_TYPE( sLeafKey , STNDR_KEY_TB_KEY );
    idlOS::memcpy( STNDR_LKEY_KEYVALUE_PTR( sLeafKey  ),
                   STNDR_LKEY_KEYVALUE_PTR( sRemoveKey ),
                   getKeyValueLength() );

    // BUG-29506 TBT가 TBK로 전환시 offset을 CTS에 반영하지 않습니다.
    // 이전 offset값을 TBK로 삭제되는 key의 offset으로 수정
    IDE_TEST(sdpSlotDirectory::getValue( sSlotDirPtr,
                                         *aLeafKeySeq + 1,   
                                         &sOldKeyOffset )
             != IDE_SUCCESS );

    if( SDN_IS_VALID_CTS(STNDR_GET_CCTS_NO(sLeafKey)) )
    {
        IDE_TEST( sdnIndexCTL::updateRefKey( aMtx,
                                             aLeafNode,
                                             STNDR_GET_CCTS_NO( sLeafKey ),
                                             sOldKeyOffset,
                                             sNewKeyOffset )
                  != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( SKIP_ALLOC_SLOT );

    STNDR_SET_TBK_LSCN( ((stndrLKeyEx*)sLeafKey ), &sFstDiskViewSCN );
    STNDR_SET_TBK_LTSS( ((stndrLKeyEx*)sLeafKey ), &sTSSlotSID );

    STNDR_SET_LCTS_NO( sLeafKey , SDN_CTS_IN_KEY );
    STNDR_SET_LSCN( sLeafKey, aInfiniteSCN );;
    
    STNDR_SET_STATE( sLeafKey , STNDR_KEY_DELETED );

    sRollbackContext.mTableOID = aIndex->mSdnHeader.mTableOID;
    sRollbackContext.mIndexID  = aIndex->mSdnHeader.mIndexID;
    sRollbackContext.mFstDiskViewSCN = sFstDiskViewSCN;
    SM_SET_SCN( &sRollbackContext.mLimitSCN, aInfiniteSCN );
   
    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)aLeafNode );
    sUnlimitedKeyCount = sNodeHdr->mUnlimitedKeyCount - 1;
   
    IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                         (UChar*)&(sNodeHdr->mUnlimitedKeyCount),
                                         (void*)&sUnlimitedKeyCount,
                                         ID_SIZEOF(UShort) )
              != IDE_SUCCESS );

    if( sRemoveInsert == ID_TRUE )
    {
        sTotalTBKCount = sNodeHdr->mTBKCount + 1;
        IDE_TEST( sdrMiniTrans::writeNBytes( aMtx,
                                             (UChar*)&(sNodeHdr->mTBKCount),
                                             (void*)&sTotalTBKCount,
                                             ID_SIZEOF(UShort) )
                  != IDE_SUCCESS );
    }
   
    sdrMiniTrans::setRefOffset( aMtx, SM_OID_NULL );
    IDE_TEST( sdrMiniTrans::writeLogRec( aMtx,
                                         (UChar*)sLeafKey ,
                                         (void*)NULL,
                                         sKeyLength +
                                         ID_SIZEOF(stndrRollbackContext) +
                                         ID_SIZEOF(SShort) +
                                         ID_SIZEOF(UChar),
                                         SDR_STNDR_DELETE_KEY_WITH_NTA )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&sRollbackContext,
                                   ID_SIZEOF(stndrRollbackContext) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)&sRemoveInsert,
                                   ID_SIZEOF(UChar) )
              != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)aLeafKeySeq ,
                                   ID_SIZEOF(SShort) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::write( aMtx,
                                   (void*)sLeafKey ,
                                   sKeyLength )
              != IDE_SUCCESS );

    /*
     * 새로운 KEY가 할당되었다면 기존 KEY를 삭제한다.
     */
    if( sRemoveInsert == ID_TRUE )
    {
        IDE_DASSERT( sTxBoundType != STNDR_KEY_TB_KEY );

        sKeyLength = getKeyLength( (UChar *)sRemoveKey,
                                   ID_TRUE /* aIsLeaf */ );
            
        IDE_TEST(sdrMiniTrans::writeLogRec(aMtx,
                                           (UChar*)sRemoveKey,
                                           (void*)&sKeyLength,
                                           ID_SIZEOF(UShort),
                                           SDR_STNDR_FREE_INDEX_KEY )
                 != IDE_SUCCESS );
            
        sdpPhyPage::freeSlot(aLeafNode,
                             *aLeafKeySeq  + 1,
                             sKeyLength,
                             1 );
    }
    
    IDE_EXCEPTION_CONT( ALLOC_FAILURE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 callback에 의거하여 keyrange에 해당하는 leaf slot의 바로
 * 앞으로 커서를 이동시킨다. 
 * 주로 read lock으로 traversing할때 호출된다.
 *********************************************************************/
IDE_RC stndrRTree::beforeFirst( stndrIterator       *  aIterator,
                                const smSeekFunc   **  /**/)
{
    for( aIterator->mKeyRange       = aIterator->mKeyRange;
         aIterator->mKeyRange->prev != NULL;
         aIterator->mKeyRange       = aIterator->mKeyRange->prev ) ;

    IDE_TEST( beforeFirstInternal( aIterator ) != IDE_SUCCESS );

    aIterator->mFlag  = aIterator->mFlag & (~SMI_RETRAVERSE_MASK);
    aIterator->mFlag |= SMI_RETRAVERSE_BEFORE;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 callback에 의거하여 keyrange에 해당하는 모든 leaf slot의
 * 바로 앞으로 커서를 이동시킨다. 
 * key Range는 리스트로 존재할 수 있는데, 해당하는 Key가 존재하지
 * 않는 key range는 skip한다. 
 *********************************************************************/
IDE_RC stndrRTree::beforeFirstInternal( stndrIterator * aIterator )
{
    if(aIterator->mProperties->mReadRecordCount > 0)
    {
        IDE_TEST( stndrRTree::findFirst(aIterator) != IDE_SUCCESS );

        while( (aIterator->mCacheFence        == aIterator->mRowCache) &&
               (aIterator->mIsLastNodeInRange == ID_TRUE) &&
               (aIterator->mKeyRange->next    != NULL) )
        {
            aIterator->mKeyRange = aIterator->mKeyRange->next;
            
            IDE_TEST( stndrRTree::findFirst(aIterator) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 maximum callback에 의거하여 해당 callback을 만족하는 Key를
 * 찾아 row cache를 구성한다.
 *********************************************************************/
IDE_RC stndrRTree::findFirst( stndrIterator * aIterator )
{
    idvSQL              * sStatistics = NULL;
    sdpPhyPageHdr       * sLeafNode = NULL;
    const smiRange      * sRange = NULL;
    stndrStack          * sStack = NULL;
    stndrHeader         * sIndex = NULL;
    idBool                sIsRetry;
    UInt                  sState = 0;
    stndrVirtualRootNode  sVRootNode;


    sIndex      = (stndrHeader*)((smnIndexHeader*)(aIterator->mIndex))->mHeader;
    sStatistics = aIterator->mProperties->mStatistics;
    sStack      = &aIterator->mStack;
    sRange      = aIterator->mKeyRange;

  retry:
    
    stndrStackMgr::clear(sStack);

    getVirtualRootNode( sIndex, &sVRootNode );
    
    if( sVRootNode.mChildPID != SD_NULL_PID )
    {
        IDE_TEST( stndrStackMgr::push( sStack,
                                       sVRootNode.mChildPID,
                                       sVRootNode.mChildSmoNo,
                                       STNDR_INVALID_KEY_SEQ )
                  != IDE_SUCCESS );

        IDE_TEST( findNextLeaf( sStatistics,
                                &(sIndex->mQueryStat),
                                sIndex,
                                &(sRange->maximum),
                                sStack,
                                &sLeafNode,
                                &sIsRetry )
                  != IDE_SUCCESS );

        if( sIsRetry == ID_TRUE )
        {
            goto retry;
        }

        if( sLeafNode != NULL )
        {
            sState = 1;
        }

        IDE_TEST( makeRowCache( aIterator, (UChar*)sLeafNode )
                  != IDE_SUCCESS );

        if( sLeafNode != NULL )
        {
            sState = 0;
            
            IDE_TEST( sdbBufferMgr::releasePage( sStatistics,
                                                 (UChar*)sLeafNode )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 && sLeafNode != NULL )
    {
        (void)sdbBufferMgr::releasePage( sStatistics, (UChar*)sLeafNode );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 maximum callback를 만족하는 Leaf Node를 탐색한다. aCallBack이
 * NULL일 경우 모든 Leaf Node를 탐색한다.
 *********************************************************************/
IDE_RC stndrRTree::findNextLeaf( idvSQL             * aStatistics,
                                 stndrStatistic     * aIndexStat,
                                 stndrHeader        * aIndex,
                                 const smiCallBack  * aCallBack,
                                 stndrStack         * aStack,
                                 sdpPhyPageHdr     ** aLeafNode,
                                 idBool             * aIsRetry )
{
    sdpPhyPageHdr   * sNode;
    stndrNodeHdr    * sNodeHdr;
    stndrStackSlot    sSlot;
    stndrIKey       * sIKey;
    stdMBR            sMBR;
    scPageID          sChildPID;
    UChar           * sSlotDirPtr;
    UShort            sKeyCount;
    ULong             sNodeSmoNo;
    ULong             sIndexSmoNo;
    idBool            sResult;
    idBool            sIsSuccess;
    UInt              sState = 0;
    UInt              i;

    
    *aIsRetry = ID_FALSE;
    *aLeafNode = NULL;

    while( stndrStackMgr::getDepth(aStack) >= 0 )
    {
        sSlot = stndrStackMgr::pop( aStack );

        IDU_FIT_POINT( "1.PROJ-1591@stndrRTree::findNextLeaf" );

        IDE_TEST( stndrRTree::getPage( aStatistics,
                                       &(aIndex->mQueryStat.mIndexPage),
                                       aIndex->mSdnHeader.mIndexTSID,
                                       sSlot.mNodePID,
                                       SDB_S_LATCH,
                                       SDB_WAIT_NORMAL,
                                       NULL,
                                       (UChar**)&sNode,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
        sState = 1;

        sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( (UChar*)sNode );
        sNodeSmoNo = sdpPhyPage::getIndexSMONo( sNode );

        if( sNodeHdr->mState == STNDR_IN_FREE_LIST )
        {
            // BUG-29629: Disk R-Tree의 Scan시 FREE LIST 페이지에 대해 S-Latch를
            //            않아서 Hang이 발생합니다.
            sState = 0;
            IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                 (UChar*)sNode )
                      != IDE_SUCCESS );
            continue;
        }

        if( sNodeSmoNo > sSlot.mSmoNo )
        {
            if( sdpPhyPage::getNxtPIDOfDblList(sNode) == SD_NULL_PID )
            {
                ideLog::log( IDE_SERVER_0,
                             "Index Header:\n" );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar *)aIndex,
                                ID_SIZEOF(stndrHeader) );

                ideLog::log( IDE_SERVER_0,
                             "Stack Slot:\n" );
                ideLog::logMem( IDE_SERVER_0,
                                (UChar *)&sSlot,
                                ID_SIZEOF(stndrStackSlot) );

                dumpIndexNode( sNode );
                IDE_ASSERT( 0 );
            }
            
            IDE_TEST( stndrStackMgr::push(
                          aStack,
                          sdpPhyPage::getNxtPIDOfDblList(sNode),
                          sSlot.mSmoNo,
                          0 )
                      != IDE_SUCCESS );

            aIndexStat->mFollowRightLinkCount++;
        }

        if( STNDR_IS_LEAF_NODE( sNodeHdr ) == ID_TRUE )
        {
            *aLeafNode = sNode;
            break;
        }

        sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sNode );
        sKeyCount = sdpSlotDirectory::getCount( sSlotDirPtr );

        getSmoNo( aIndex, &sIndexSmoNo );
        IDL_MEM_BARRIER;

        IDE_TEST( iduCheckSessionEvent(aStatistics) != IDE_SUCCESS );
        for( i = 0; i < sKeyCount; i++ )
        {
            sResult = ID_TRUE;
            
            IDE_TEST ( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                                i,
                                                                (UChar**)&sIKey )
                       != IDE_SUCCESS );

            STNDR_GET_MBR_FROM_IKEY( sMBR, sIKey );
            STNDR_GET_CHILD_PID( &sChildPID, sIKey );

            if( aCallBack != NULL )
            {
                (void)aCallBack->callback( &sResult,
                                           &sMBR,
                                           NULL,
                                           0,
                                           SC_NULL_GRID,
                                           aCallBack->data );
                
                aIndexStat->mKeyRangeCount++;
            }

            if( sResult == ID_TRUE )
            {
                IDE_TEST( stndrStackMgr::push( aStack,
                                               sChildPID,
                                               sIndexSmoNo,
                                               0 )
                          != IDE_SUCCESS );
            }
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics, (UChar*)sNode )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        if( sdbBufferMgr::releasePage( aStatistics, (UChar*)sNode )
            != IDE_SUCCESS )
        {
            dumpIndexNode( sNode );
            IDE_ASSERT( 0 );
        }
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * aNode에서 aCallBack을 만족하는 마지막 Key 까지 mRowRID와 Key Value를
 * Row Cache로 구성한다. Row Cache의 대상은 maximum KeyRange를 통과한
 * Key 들 중에서 Transaction Level Visibility와 Cursor Level Visibility
 * 를 통과하는 Key 들이다.               
 *********************************************************************/
IDE_RC stndrRTree::makeRowCache( stndrIterator * aIterator, UChar * aNode )
{
    stndrStack      * sStack;
    stndrLKey       * sLKey;
    stndrKeyInfo      sKeyInfo;
    stdMBR            sMBR;
    const smiRange  * sRange;
    UChar           * sSlotDirPtr;
    UShort            sKeyCount;
    idBool            sIsVisible;
    idBool            sIsUnknown;
    idBool            sResult;
    UInt              i;

    sStack = &aIterator->mStack;

    // aIterator의 Row Cache를 초기화 한다.
    aIterator->mCurRowPtr = aIterator->mRowCache - 1;
    aIterator->mCacheFence = &aIterator->mRowCache[0];

    if( stndrStackMgr::getDepth( sStack ) == -1 )
    {
        aIterator->mIsLastNodeInRange = ID_TRUE;
    }
    else
    {
        aIterator->mIsLastNodeInRange = ID_FALSE;
    }

    sRange = aIterator->mKeyRange;

    // 읽어들일 키들을 캐슁한다.

    // TEST CASE: makeRowCacheForward에서 Leaf Node가 없을 경우를 테스트 하기
    if( aNode == NULL )
    {
        IDE_RAISE( RETURN_SUCCESS );
    }
    
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( aNode );
    sKeyCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

    for( i = 0; i < sKeyCount; i++ )
    {
        sIsVisible = ID_FALSE;
        sIsUnknown = ID_FALSE;

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr, 
                                                           i,
                                                           (UChar**)&sLKey )
                  != IDE_SUCCESS );

        STNDR_GET_MBR_FROM_LKEY( sMBR, sLKey );
        
        sRange->maximum.callback( &sResult,
                                  &sMBR,
                                  NULL,
                                  0,
                                  SC_NULL_GRID,
                                  sRange->maximum.data ); 
        if( sResult == ID_FALSE )
        {
            continue;
        }

        IDE_TEST( tranLevelVisibility( aIterator->mProperties->mStatistics,
                                       aIterator->mTrans,
                                       aNode,
                                       (UChar*)sLKey,
                                       &aIterator->mSCN,
                                       &sIsVisible,
                                       &sIsUnknown ) != IDE_SUCCESS );

        if( (sIsVisible == ID_FALSE) && (sIsUnknown == ID_FALSE) )
        {
            continue;
        }

        if( sIsUnknown == ID_TRUE )
        {
            IDE_TEST( cursorLevelVisibility( sLKey,
                                             &aIterator->mInfinite,
                                             &sIsVisible ) != IDE_SUCCESS );

            if( sIsVisible == ID_FALSE )
            {
                continue;
            }
        }

        // save slot info
        STNDR_LKEY_TO_KEYINFO( sLKey, sKeyInfo );
        
        aIterator->mCacheFence->mRowPID     = sKeyInfo.mRowPID;
        aIterator->mCacheFence->mRowSlotNum = sKeyInfo.mRowSlotNum;
        aIterator->mCacheFence->mOffset     =
            sdpPhyPage::getOffsetFromPagePtr( (UChar*)sLKey );

        idlOS::memcpy( &(aIterator->mPage[aIterator->mCacheFence->mOffset]),
                       (UChar*)sLKey,
                       getKeyLength( (UChar*)sLKey, ID_TRUE ) );

        // increase cache fence
        aIterator->mCacheFence++;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 캐시되어 있는 Row가 있다면 캐시에서 주고, 만약 없다면 Next Node를
 * 캐시한다. 
 *********************************************************************/
IDE_RC stndrRTree::fetchNext( stndrIterator * aIterator,
                              const void   ** aRow )
{
    stndrHeader * sIndex;
    idvSQL      * sStatistics;
    idBool        sNeedMoreCache;

    
    sIndex      = (stndrHeader*)((smnIndexHeader*)(aIterator->mIndex))->mHeader;
    sStatistics = aIterator->mProperties->mStatistics;

  read_from_cache:
    IDE_TEST( fetchRowCache( aIterator,
                             aRow,
                             &sNeedMoreCache ) != IDE_SUCCESS );

    IDE_TEST_RAISE( sNeedMoreCache == ID_FALSE, SKIP_CACHE );

    if( aIterator->mIsLastNodeInRange == ID_TRUE )  // Range의 끝에 도달함.
    {
        if( aIterator->mKeyRange->next != NULL ) // next key range가 존재하면
        {
            aIterator->mKeyRange = aIterator->mKeyRange->next;

            IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics)
                      != IDE_SUCCESS);
            
            (void)beforeFirstInternal( aIterator );

            goto read_from_cache;
        }
        else
        {
            // 커서의 상태를 after last상태로 한다.
            aIterator->mCurRowPtr = aIterator->mCacheFence;
            
            *aRow = NULL;
            SC_MAKE_NULL_GRID( aIterator->mRowGRID );
            
            return IDE_SUCCESS;
        }
    }
    else
    {
        IDE_TEST( iduCheckSessionEvent( sStatistics )
                  != IDE_SUCCESS);

        IDE_TEST( makeNextRowCache( aIterator, sIndex )
                  != IDE_SUCCESS);

        goto read_from_cache;
    }

    IDE_EXCEPTION_CONT( SKIP_CACHE );
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 테이블로 부터 자신에게 맞는 Stable Version을 얻어와 Row Filter 적용
 * 한다.
 *********************************************************************/
IDE_RC stndrRTree::fetchRowCache( stndrIterator * aIterator,
                                  const void   ** aRow,
                                  idBool        * aNeedMoreCache )
{
    UChar           * sDataSlot;
    UChar           * sDataPage;
    idBool            sIsSuccess;
    idBool            sResult;
    sdSID             sRowSID;
    smSCN             sMyFstDskViewSCN;
    stndrHeader     * sIndex;
    scSpaceID         sTableTSID;
    sdcRowHdrInfo     sRowHdrInfo;
    idBool            sIsRowDeleted;
    idvSQL          * sStatistics;
    sdSID             sTSSlotSID;
    idBool            sIsMyTrans;
    UChar           * sDataSlotDir;
    idBool            sIsPageLatchReleased = ID_TRUE;
    smSCN             sFstDskViewSCN;
    

    sIndex           = (stndrHeader*)((smnIndexHeader*)(aIterator->mIndex))->mHeader;
    sTableTSID       = sIndex->mSdnHeader.mTableTSID;
    sStatistics      = aIterator->mProperties->mStatistics;
    sTSSlotSID       = smLayerCallback::getTSSlotSID(aIterator->mTrans);
    sMyFstDskViewSCN = smLayerCallback::getFstDskViewSCN(aIterator->mTrans);

    *aNeedMoreCache = ID_TRUE;
    
    while( aIterator->mCurRowPtr + 1 < aIterator->mCacheFence )
    {
        IDE_TEST(iduCheckSessionEvent(sStatistics) != IDE_SUCCESS);
        aIterator->mCurRowPtr++;

        sRowSID = SD_MAKE_SID( aIterator->mCurRowPtr->mRowPID,
                               aIterator->mCurRowPtr->mRowSlotNum);

        IDE_TEST( getPage( sStatistics,
                           &(sIndex->mQueryStat.mIndexPage),
                           sTableTSID,
                           SD_MAKE_PID( sRowSID ),
                           SDB_S_LATCH,
                           SDB_WAIT_NORMAL,
                           NULL, /* aMtx */
                           (UChar**)&sDataPage,
                           &sIsSuccess )
                  != IDE_SUCCESS );
        
        sIsPageLatchReleased = ID_FALSE;
        
        sDataSlotDir = sdpPhyPage::getSlotDirStartPtr(sDataPage);
        
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                    sDataSlotDir,
                                                    SD_MAKE_SLOTNUM( sRowSID ),
                                                    &sDataSlot )
                  != IDE_SUCCESS );

        IDE_TEST( makeVRowFromRow( sIndex,
                                   sStatistics,
                                   NULL, /* aMtx */
                                   NULL, /* aSP */
                                   aIterator->mTrans,
                                   aIterator->mTable,
                                   sTableTSID,
                                   sDataSlot,
                                   SDB_SINGLE_PAGE_READ,
                                   aIterator->mProperties->mFetchColumnList,
                                   SMI_FETCH_VERSION_CONSISTENT,
                                   sTSSlotSID,
                                   &(aIterator->mSCN),
                                   &(aIterator->mInfinite),
                                   (UChar*)*aRow,
                                   &sIsRowDeleted,
                                   &sIsPageLatchReleased )
                  != IDE_SUCCESS );

        if ( sIsRowDeleted == ID_TRUE )
        {
            if( sIsPageLatchReleased == ID_FALSE )
            {
                sIsPageLatchReleased = ID_TRUE;
                IDE_TEST( sdbBufferMgr::releasePage( sStatistics,
                                                     (UChar*)sDataPage )
                          != IDE_SUCCESS );
            }
            
            continue;
        }

        if( (aIterator->mFlag & SMI_ITERATOR_TYPE_MASK) 
            == SMI_ITERATOR_WRITE )
        {
            /* BUG-23319
             * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
            /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
             * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
             * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
             * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
             * 상황에 따라 적절한 처리를 해야 한다. */
            if( sIsPageLatchReleased == ID_TRUE )
            {
                IDE_TEST( getPage( sStatistics,
                                   &(sIndex->mQueryStat.mIndexPage),
                                   sTableTSID,
                                   SD_MAKE_PID( sRowSID ),
                                   SDB_S_LATCH,
                                   SDB_WAIT_NORMAL,
                                   NULL, /* aMtx */
                                   (UChar**)&sDataPage,
                                   &sIsSuccess )
                          != IDE_SUCCESS );
                
                sIsPageLatchReleased = ID_FALSE;

                sDataSlotDir = sdpPhyPage::getSlotDirStartPtr(sDataPage);
                IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                                    sDataSlotDir,
                                                    SD_MAKE_SLOTNUM(sRowSID),
                                                    &sDataSlot )
                          != IDE_SUCCESS );
            }

            sIsMyTrans = sdcRow::isMyTransUpdating( sDataPage,
                                                    sDataSlot,
                                                    sMyFstDskViewSCN,
                                                    sTSSlotSID,
                                                    &sFstDskViewSCN );

            if ( sIsMyTrans == ID_TRUE )
            {
                sdcRow::getRowHdrInfo( sDataSlot, &sRowHdrInfo );

                if( SM_SCN_IS_GE( &sRowHdrInfo.mInfiniteSCN,
                                  &aIterator->mInfinite ) )
                {
                    IDE_ASSERT( 0 );
                }
            }
        }

        if( sIsPageLatchReleased == ID_FALSE )
        {
            sIsPageLatchReleased = ID_TRUE;
            IDE_TEST( sdbBufferMgr::releasePage( sStatistics,
                                                 (UChar*)sDataPage )
                      != IDE_SUCCESS );
        }

        SC_MAKE_GRID_WITH_SLOTNUM( aIterator->mRowGRID,
                                   sTableTSID,
                                   SD_MAKE_PID(sRowSID),
                                   SD_MAKE_SLOTNUM(sRowSID) );

        sIndex->mQueryStat.mRowFilterCount++;

        IDE_TEST( aIterator->mRowFilter->callback( &sResult,
                                                   *aRow,
                                                   NULL,
                                                   0,
                                                   aIterator->mRowGRID,
                                                   aIterator->mRowFilter->data) != IDE_SUCCESS );

        if( sResult == ID_FALSE )
        {
            continue;
        }

        // skip count 만큼 row 건너뜀
        if( aIterator->mProperties->mFirstReadRecordPos > 0 )
        {
            aIterator->mProperties->mFirstReadRecordPos--;
        }
        else if( aIterator->mProperties->mReadRecordCount > 0 )
        {
            aIterator->mProperties->mReadRecordCount--;
            *aNeedMoreCache = ID_FALSE;

            break;
        }
        else
        {
            // 커서의 상태를 after last상태로 한다.
            aIterator->mCurRowPtr = aIterator->mCacheFence;
            SC_MAKE_NULL_GRID( aIterator->mRowGRID );
            *aRow = NULL;

            *aNeedMoreCache = ID_FALSE;
            break;
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sIsPageLatchReleased == ID_FALSE )
    {
        IDE_PUSH();
        if( sdbBufferMgr::releasePage( sStatistics, (UChar*)sDataPage )
            != IDE_SUCCESS )
        {
            dumpIndexNode( (sdpPhyPageHdr *)sDataPage );
            IDE_ASSERT( 0 );
        }
        IDE_POP();
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * row로부터 VRow를 생성한다. 오로지 Fetch과정에서 호출되며, RowFilter,
 * Qp에 올려주는 용도로만 사용한다.
 *********************************************************************/
IDE_RC stndrRTree::makeVRowFromRow( stndrHeader         * aIndex,
                                    idvSQL              * aStatistics,
                                    sdrMtx              * aMtx,
                                    sdrSavePoint        * aSP,
                                    void                * aTrans,
                                    void                * aTable,
                                    scSpaceID             aTableSpaceID,
                                    const UChar         * aRow,
                                    sdbPageReadMode       aPageReadMode,
                                    smiFetchColumnList  * aFetchColumnList,
                                    smFetchVersion        aFetchVersion,
                                    sdSID                 aTSSlotSID,
                                    const smSCN         * aSCN,
                                    const smSCN         * aInfiniteSCN,
                                    UChar               * aDestBuf,
                                    idBool              * aIsRowDeleted,
                                    idBool              * aIsPageLatchReleased )
{
    IDE_DASSERT( aTrans     != NULL );
    IDE_DASSERT( aRow       != NULL );
    IDE_DASSERT( aDestBuf   != NULL );

    IDE_DASSERT( checkFetchColumnList( aIndex, aFetchColumnList ) 
                 == IDE_SUCCESS );

    IDE_TEST( sdcRow::fetch( aStatistics,
                             aMtx,
                             aSP,
                             aTrans,
                             aTableSpaceID,
                             (UChar*)aRow,
                             ID_TRUE, /* aIsPersSlot */
                             aPageReadMode,
                             aFetchColumnList,
                             aFetchVersion,
                             aTSSlotSID,
                             aSCN,
                             aInfiniteSCN,
                             NULL,
                             NULL, /* aLobInfo4Fetch */
                             ((smcTableHeader*)aTable)->mRowTemplate,
                             aDestBuf,
                             aIsRowDeleted,
                             aIsPageLatchReleased)
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Row를 바탕으로 VRow생성시 fetchcolumnlist가 사용된다. 인덱스 컬럼은
 * FetchColumnList는 반드시 존재해야 한다.
 *********************************************************************/
IDE_RC stndrRTree::checkFetchColumnList( stndrHeader        * aIndex,
                                         smiFetchColumnList * aFetchColumnList )
{
    stndrColumn         * sColumn;
    smiFetchColumnList  * sFetchColumnList;

    IDE_DASSERT( aIndex           != NULL );
    IDE_DASSERT( aFetchColumnList != NULL );

    sColumn = &aIndex->mColumn;
    
    for( sFetchColumnList = aFetchColumnList;
         sFetchColumnList != NULL;
         sFetchColumnList = sFetchColumnList->next )
    {
        if( sColumn->mVRowColumn.id == sFetchColumnList->column->id )
        {
            break;
        }
    }

    IDE_TEST( sFetchColumnList == NULL );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 다음 Leaf Node를 탐색하고, 해당 노드로부터 row cache를 구성한다.
 *********************************************************************/
IDE_RC stndrRTree::makeNextRowCache( stndrIterator * aIterator, stndrHeader * aIndex )
{
    idvSQL          * sStatistics = NULL;
    sdpPhyPageHdr   * sLeafNode = NULL;
    const smiRange  * sRange = NULL;
    stndrStack      * sStack = NULL;
    idBool            sIsRetry;
    UInt              sState = 0;


    sStatistics = aIterator->mProperties->mStatistics;
    sStack      = &aIterator->mStack;
    sRange      = aIterator->mKeyRange;

    IDE_TEST( findNextLeaf( sStatistics,
                            &(aIndex->mQueryStat),
                            aIndex,
                            &(sRange->maximum),
                            sStack,
                            &sLeafNode,
                            &sIsRetry )
              != IDE_SUCCESS );

    sState = 1;

    IDE_TEST( makeRowCache( aIterator, (UChar*)sLeafNode )
              != IDE_SUCCESS );

    if( sLeafNode != NULL )
    {
        IDE_TEST( sdbBufferMgr::releasePage( sStatistics, (UChar*)sLeafNode )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 && sLeafNode != NULL )
    {
        if( sdbBufferMgr::releasePage( sStatistics, (UChar*)sLeafNode )
            != IDE_SUCCESS )
        {
            dumpIndexNode( sLeafNode );
            IDE_ASSERT( 0 );
        }
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 callback에 의거하여 keyrange에 해당하는 leaf key의 바로 
 * 앞으로 커서를 이동시킨다. 
 * 주로 write lock으로 traversing할때 호출된다. 
 * 한번 호출된 후에는 lock을 다시 잡지 않기 위해 seekFunc을 바꾼다. 
 *********************************************************************/
IDE_RC stndrRTree::beforeFirstW( stndrIterator      * aIterator,
                                 const smSeekFunc  ** aSeekFunc )
{
    (void) stndrRTree::beforeFirst( aIterator, aSeekFunc );

    // Seek funstion set 변경
    *aSeekFunc += 6;

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 callback에 의거하여 keyrange에 해당되는 모든 Row에 lock을  
 * 걸고 첫 키의 바로 앞으로 커서를 다시 이동시킨다.                
 * 주로 Repeatable read로 traversing할때 호출된다.                   
 * 한번 호출된 이후에 다시 호출되지 않도록  SeekFunc을 바꾼다.      
 *********************************************************************/
IDE_RC stndrRTree::beforeFirstRR( stndrIterator     * aIterator,
                                  const smSeekFunc ** aSeekFunc )
{
    stndrIterator       sIterator;
    stndrStack          sStack;
    idBool              sStackInit = ID_FALSE;
    smiCursorProperties sProp;

    IDE_TEST( stndrRTree::beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    IDE_TEST( stndrStackMgr::initialize(
                  &sStack,
                  stndrStackMgr::getStackSize( &aIterator->mStack ) )
              != IDE_SUCCESS );
    sStackInit = ID_TRUE;

    idlOS::memcpy( &sIterator, aIterator, ID_SIZEOF(stndrIterator) );
    idlOS::memcpy( &sProp, aIterator->mProperties, ID_SIZEOF(smiCursorProperties) );

    stndrStackMgr::copy( &sStack, &aIterator->mStack );
    
    IDE_TEST( stndrRTree::lockAllRows4RR( aIterator ) != IDE_SUCCESS );
    
    // beforefirst 상태로 되돌려 놓음.
    idlOS::memcpy( aIterator, &sIterator, ID_SIZEOF(stndrIterator) );
    idlOS::memcpy( aIterator->mProperties, &sProp, ID_SIZEOF(smiCursorProperties) );

    stndrStackMgr::copy( &aIterator->mStack, &sStack );

    /*
     * 자신이 남긴 Lock Row를 보기 위해서는 Cursor Infinite SCN을
     * 2증가 시켜야 한다.
     */
    SM_ADD_SCN( &aIterator->mInfinite, 2 );
    
    *aSeekFunc += 6;

    sStackInit = ID_FALSE;
    IDE_TEST( stndrStackMgr::destroy( &sStack ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sStackInit == ID_TRUE )
    {
        (void)stndrStackMgr::destroy( &sStack );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 주어진 key range에 맞는 모든 key가 가리키는 Row들을 찾아 lock을
 * 건다.
 * 다음 key가 key range에 맞지 않으면 해당 key range의 범위가 끝난
 * 것이므로 다음 key range를 사용하여 다시 시작한다.
 * key range를 통과한 key들 중에서
 *     1. Transaction level visibility를 통과하고,
 *     2. Cursor level visibility를 통과하고,
 *     3. Filter조건을 통과하는
 *     4. Update가능한
 * Row들만 반환한다. 
 * 본 함수는 Key Range와 Filter에 합당하는 모든 Row에 lock을 걸기
 * 때문에, 종료후에는 After Last 상태가 된다.
 *********************************************************************/
IDE_RC stndrRTree::lockAllRows4RR( stndrIterator * aIterator )
{

    idBool            sIsRowDeleted;
    sdcUpdateState    sRetFlag;
    smTID             sWaitTxID;
    sdrMtx            sMtx;
    UChar           * sSlot;
    idBool            sIsSuccess;
    idBool            sResult;
    sdSID             sRowSID;
    scGRID            sRowGRID;
    idBool            sSkipLockRec = ID_FALSE;
    sdSID             sTSSlotSID   = smLayerCallback::getTSSlotSID(aIterator->mTrans);
    stndrHeader     * sIndex;
    scSpaceID         sTableTSID;
    UChar             sCTSlotIdx;
    sdrMtxStartInfo   sStartInfo;
    sdpPhyPageHdr   * sPageHdr;
    idBool            sIsPageLatchReleased = ID_TRUE;
    sdrSavePoint      sSP;
    UChar           * sDataPage;
    UChar           * sDataSlotDir;
    UInt              sState = 0;
    

    IDE_DASSERT( aIterator->mProperties->mLockRowBuffer != NULL );

    sIndex = (stndrHeader*)((smnIndexHeader*)(aIterator->mIndex))->mHeader;
    sTableTSID = sIndex->mSdnHeader.mTableTSID;

    sStartInfo.mTrans   = aIterator->mTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING;

  read_from_cache:
    
    while( aIterator->mCurRowPtr + 1 < aIterator->mCacheFence )
    {
        IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS );
        aIterator->mCurRowPtr++;
        
      revisit_row:
        
        IDE_TEST( sdrMiniTrans::begin(aIterator->mProperties->mStatistics,
                                      &sMtx,
                                      aIterator->mTrans,
                                      SDR_MTX_LOGGING,
                                      ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                      gMtxDLogType | SM_DLOG_ATTR_UNDOABLE )
                  != IDE_SUCCESS );
        sState = 1;

        sRowSID = SD_MAKE_SID( aIterator->mCurRowPtr->mRowPID,
                               aIterator->mCurRowPtr->mRowSlotNum);

        sdrMiniTrans::setSavePoint( &sMtx, &sSP );

        IDE_TEST( stndrRTree::getPage( aIterator->mProperties->mStatistics,
                                       &(sIndex->mQueryStat.mIndexPage),
                                       sTableTSID,
                                       SD_MAKE_PID( sRowSID ),
                                       SDB_X_LATCH,
                                       SDB_WAIT_NORMAL,
                                       (void*)&sMtx,
                                       (UChar**)&sDataPage,
                                       &sIsSuccess )
                  != IDE_SUCCESS );
        
        sDataSlotDir = sdpPhyPage::getSlotDirStartPtr(sDataPage);
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( 
                                                    sDataSlotDir,
                                                    SD_MAKE_SLOTNUM( sRowSID ),
                                                    &sSlot )
                  != IDE_SUCCESS );
        
        sIsPageLatchReleased = ID_FALSE;
        IDE_TEST( makeVRowFromRow( sIndex,
                                   aIterator->mProperties->mStatistics,
                                   &sMtx,
                                   &sSP,
                                   aIterator->mTrans,
                                   aIterator->mTable,
                                   sTableTSID,
                                   sSlot,
                                   SDB_SINGLE_PAGE_READ,
                                   aIterator->mProperties->mFetchColumnList,
                                   SMI_FETCH_VERSION_CONSISTENT,
                                   sTSSlotSID,
                                   &(aIterator->mSCN),
                                   &(aIterator->mInfinite),
                                   aIterator->mProperties->mLockRowBuffer,
                                   &sIsRowDeleted,
                                   &sIsPageLatchReleased )
                  != IDE_SUCCESS );

        if ( sIsRowDeleted == ID_TRUE )
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            continue;
        }
        
        /* BUG-23319
         * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
        /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
         * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
         * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
         * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
         * 상황에 따라 적절한 처리를 해야 한다. */
        if( sIsPageLatchReleased == ID_TRUE )
        {
            IDE_TEST( sdbBufferMgr::getPageBySID(
                          aIterator->mProperties->mStatistics,
                          sTableTSID,
                          sRowSID,
                          SDB_X_LATCH,
                          SDB_WAIT_NORMAL,
                          &sMtx,
                          (UChar**)&sSlot,
                          &sIsSuccess )
                      != IDE_SUCCESS );
        }

        SC_MAKE_GRID_WITH_SLOTNUM( sRowGRID,
                                   sTableTSID,
                                   SD_MAKE_PID( sRowSID ),
                                   SD_MAKE_SLOTNUM( sRowSID ) );

        IDE_TEST( aIterator->mRowFilter->callback( &sResult,
                                                   aIterator->mProperties->mLockRowBuffer,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->mRowFilter->data) != IDE_SUCCESS );

        if( sResult == ID_FALSE )
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            continue;
        }

        IDE_TEST( sdcRow::canUpdateRowPieceInternal( 
                      aIterator->mProperties->mStatistics,
                      &sStartInfo,
                      sSlot,
                      sTSSlotSID,
                      SDB_SINGLE_PAGE_READ,
                      &(aIterator->mSCN),
                      &(aIterator->mInfinite),
                      ID_FALSE, /* aIsUptLobByAPI */
                      &sRetFlag,
                      &sWaitTxID ) != IDE_SUCCESS );

        /* PROJ-2162 RestartRiskRedcution
         * 갱신 없이, lock 시도 중 Rollback하는 경우, 예외처리*/
        if( sRetFlag == SDC_UPTSTATE_REBUILD_ALREADY_MODIFIED )
        {
            /* 페이지 정보 출력하고
               commit -> releaseLatch + rollback */
            IDE_RAISE( ERR_ALREADY_MODIFIED );
        }

        if( sRetFlag == SDC_UPTSTATE_UPDATE_BYOTHER )
        {
            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            IDE_TEST( smLayerCallback::waitForTrans( 
                                     aIterator->mTrans,
                                     sWaitTxID,
                                     sTableTSID,
                                     aIterator->mProperties->mLockWaitMicroSec ) // aLockTimeOut
                      != IDE_SUCCESS );
            goto revisit_row;
        }
        else
        {
            if( sRetFlag == SDC_UPTSTATE_INVISIBLE_MYUPTVERSION )
            {
                sState = 0;
                IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
                continue;
            }
        }

        // skip count 만큼 row 건너뜀
        if( aIterator->mProperties->mFirstReadRecordPos > 0 )
        {
            aIterator->mProperties->mFirstReadRecordPos--;
        }
        else if( aIterator->mProperties->mReadRecordCount > 0 )
        {
            sPageHdr = (sdpPhyPageHdr*)sdpPhyPage::getPageStartPtr( sSlot );

            IDE_TEST( sdcTableCTL::allocCTSAndSetDirty(
                          aIterator->mProperties->mStatistics,
                          &sMtx,       /* aFixMtx */
                          &sStartInfo, /* for Logging */
                          sPageHdr,
                          &sCTSlotIdx ) != IDE_SUCCESS );

            /*  BUG-24406 [5.3.1 SD] index scan으로 lock row 하다가 서버사망. */
            /* allocCTS()시에 CTL 확장이 발생하는 경우,
             * CTL 확장중에 compact page 연산이 발생할 수 있다.
             * compact page 연산이 발생하면
             * 페이지내에서 slot들의 위치(offset)가 변경될 수 있다.
             * 그러므로 allocCTS() 후에는 slot pointer를 다시 구해와야 한다. */

            sDataSlotDir =
              sdpPhyPage::getSlotDirStartPtr(sdpPhyPage::getPageStartPtr(sSlot));
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum(
                                            sDataSlotDir,
                                            aIterator->mCurRowPtr->mRowSlotNum,
                                            &sSlot)
                      != IDE_SUCCESS );

            aIterator->mProperties->mReadRecordCount--;

            IDE_TEST(sdcRow::lock(aIterator->mProperties->mStatistics,
                                  sSlot,
                                  sRowSID,
                                  &(aIterator->mInfinite),
                                  &sMtx,
                                  sCTSlotIdx,
                                  &sSkipLockRec) != IDE_SUCCESS);

            SC_MAKE_GRID_WITH_SLOTNUM( aIterator->mRowGRID,
                                       sTableTSID,
                                       aIterator->mCurRowPtr->mRowPID,
                                       aIterator->mCurRowPtr->mRowSlotNum);
        }
        else
        {
            // 필요한 Row에 대해 모두 lock을 획득하였음...
            sState = 0;
            IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
            return IDE_SUCCESS;
        }
        
        sState = 0;
        IDE_TEST( sdrMiniTrans::commit(&sMtx) != IDE_SUCCESS );
    }

    if( aIterator->mIsLastNodeInRange == ID_TRUE )  // Range의 끝에 도달함.
    {
        if( (aIterator->mFlag & SMI_RETRAVERSE_MASK) == SMI_RETRAVERSE_BEFORE )
        {
            if( aIterator->mKeyRange->next != NULL ) // next key range가 존재하면
            {
                aIterator->mKeyRange = aIterator->mKeyRange->next;
                IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);
                IDE_TEST( beforeFirstInternal(aIterator) != IDE_SUCCESS );
                goto read_from_cache;
            }
            else
            {
                return IDE_SUCCESS;
            }
        }
        else
        {
            // Disk R-Tree에서는 SMI_RETRAVERSE_BEFORE외에는 없다.
            IDE_ASSERT(0);
        }
    }
    else
    {
        // Key Range 범위가 끝나지 않은 경우로
        // 다음 Leaf Node로부터 Index Cache 정보를 구축한다.
        // 기존 로직은 Index Cache 의 존재 유무에 관계 없이
        // read_from_cache 로 분기하며 이를 그대로 따른다.
        if( (aIterator->mFlag & SMI_RETRAVERSE_MASK) == SMI_RETRAVERSE_BEFORE )
        {
            IDE_TEST( makeNextRowCache( aIterator, sIndex ) != IDE_SUCCESS );
        }
        else
        {
            // Disk R-Tree에서는 SMI_RETRAVERSE_BEFORE외에는 없다.
            IDE_ASSERT(0);
        }

        goto read_from_cache;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ALREADY_MODIFIED );
    {
        if( aIterator->mStatement->isForbiddenToRetry() == ID_TRUE )
        {
            IDE_DASSERT( ((smxTrans*)aIterator->mTrans)->mIsGCTx == ID_TRUE );

            SChar    sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            sdpCTS * sCTS;
            smSCN    sFSCNOrCSCN;
            UChar    sCTSlotIdx;
            sdcRowHdrInfo   sRowHdrInfo;
            sdcRowHdrExInfo sRowHdrExInfo;

            sdcRow::getRowHdrInfo( sSlot, &sRowHdrInfo );
            sCTSlotIdx = sRowHdrInfo.mCTSlotIdx;

            if ( SDC_HAS_BOUND_CTS(sCTSlotIdx) )
            {
                sCTS = sdcTableCTL::getCTS( sdpPhyPage::getHdr(sSlot),sCTSlotIdx );
                SM_SET_SCN( &sFSCNOrCSCN, &sCTS->mFSCNOrCSCN );
            }
            else
            {
                sdcRow::getRowHdrExInfo( sSlot, &sRowHdrExInfo );
                SM_SET_SCN( &sFSCNOrCSCN, &sRowHdrExInfo.mFSCNOrCSCN );
            }

            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[GEOMETRY INDEX VALIDATION(RR)] "
                             "SpaceID:%"ID_UINT32_FMT", "
                             "TableOID:%"ID_vULONG_FMT", "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "CSInfiniteSCN:%"ID_UINT64_FMT", "
                             "FSCNOrCSCN:%"ID_UINT64_FMT", "
                             "InfiniteSCN:%"ID_UINT64_FMT,
                              ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                              ((smcTableHeader*)aIterator->mTable)->mSelfOID,
                             SM_SCN_TO_LONG( aIterator->mSCN ),
                             SM_SCN_TO_LONG( aIterator->mInfinite ),
                             SM_SCN_TO_LONG( sFSCNOrCSCN ),
                             SM_SCN_TO_LONG( sRowHdrInfo.mInfiniteSCN ) );
            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_3 );
        }
        else
        {
            IDE_SET( ideSetErrorCode(smERR_RETRY_Already_Modified) );
        }

        IDE_ASSERT( sdcRow::releaseLatchForAlreadyModify( &sMtx, &sSP )
                    == IDE_SUCCESS );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    /* BUG-24151: [SC] Update Retry, Delete Retry, Statement Rebuild Count를
     *            AWI로 추가해야 합니다.*/
    if( ideGetErrorCode() == smERR_RETRY_Already_Modified )
    {
        SMX_INC_SESSION_STATISTIC( sStartInfo.mTrans,
                                   IDV_STAT_INDEX_LOCKROW_RETRY_COUNT,
                                   1 /* Increase Value */ );

        smcTable::incStatAtABort( (smcTableHeader*)aIterator->mTable,
                                  SMC_INC_RETRY_CNT_OF_LOCKROW );
    }

    if( sState == 1 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback(&sMtx) == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * Disk R-Tree의 Meta Page를 초기화 한다.
 *********************************************************************/
IDE_RC stndrRTree::initMeta( UChar * aMetaPtr,
                             UInt    aBuildFlag,
                             void  * aMtx )
{
    sdrMtx      * sMtx;
    stndrMeta   * sMeta;
    scPageID      sPID = SD_NULL_PID;
    ULong         sFreeNodeCnt = 0;
    idBool        sIsConsistent = ID_FALSE;
    idBool        sIsCreatedWithLogging;
    idBool        sIsCreatedWithForce = ID_FALSE;
    smLSN         sNullLSN;
    smSCN         sFreeNodeSCN;
    UShort        sConvexhullPointNum = 0;

    sMtx = (sdrMtx*)aMtx;
    
    sIsCreatedWithLogging = ((aBuildFlag & SMI_INDEX_BUILD_LOGGING_MASK) ==
                             SMI_INDEX_BUILD_LOGGING) ? ID_TRUE : ID_FALSE ;

    sIsCreatedWithForce = ((aBuildFlag & SMI_INDEX_BUILD_FORCE_MASK) ==
                           SMI_INDEX_BUILD_FORCE) ? ID_TRUE : ID_FALSE ;

    SM_MAX_SCN( &sFreeNodeSCN );

    /* Index Specific Data 초기화 */
    sMeta = (stndrMeta*)( aMetaPtr + SMN_INDEX_META_OFFSET );

    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mRootNode,
                                         (void*)&sPID,
                                         ID_SIZEOF(sPID) ) != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mEmptyNodeHead,
                                         (void*)&sPID,
                                         ID_SIZEOF(sPID) ) != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mEmptyNodeTail,
                                         (void*)&sPID,
                                         ID_SIZEOF(sPID) ) != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mFreeNodeHead,
                                         (void*)&sPID,
                                         ID_SIZEOF(sPID) ) != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mFreeNodeCnt,
                                         (void*)&sFreeNodeCnt,
                                         ID_SIZEOF(sFreeNodeCnt) )
              != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mFreeNodeSCN,
                                         (void*)&sFreeNodeSCN,
                                         ID_SIZEOF(sFreeNodeSCN) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mConvexhullPointNum,
                                         (void*)&sConvexhullPointNum,
                                         ID_SIZEOF(sConvexhullPointNum) )
              != IDE_SUCCESS );
    
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mIsConsistent,
                                         (void*)&sIsConsistent,
                                         ID_SIZEOF(sIsConsistent) )
              != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mIsCreatedWithLogging,
                                         (void*)&sIsCreatedWithLogging,
                                         ID_SIZEOF(sIsCreatedWithLogging) )
              != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes( sMtx,
                                         (UChar*)&sMeta->mIsCreatedWithForce,
                                         (void*)&sIsCreatedWithForce,
                                         ID_SIZEOF(sIsCreatedWithForce) )
              != IDE_SUCCESS );

    IDE_TEST( sdrMiniTrans::writeNBytes(
                  sMtx,
                  (UChar*)&(sMeta->mNologgingCompletionLSN.mFileNo),
                  (void*)&(sNullLSN.mFileNo),
                  ID_SIZEOF(sNullLSN.mFileNo) )
              != IDE_SUCCESS );
    
    IDE_TEST( sdrMiniTrans::writeNBytes(
                  sMtx,
                  (UChar*)&(sMeta->mNologgingCompletionLSN.mOffset),
                  (void*)&(sNullLSN.mOffset),
                  ID_SIZEOF(sNullLSN.mOffset) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : 
 * ------------------------------------------------------------------*
 * 키 값이 색인 가능한 값인지 확인한다. 아래의 값이면 색인되지 않는다.
 *   1. NULL
 *   2. Empty
 *   3. MBR의 MinX, MinY, MaxX, MaxY가 하나라도 NULL
 *********************************************************************/
void stndrRTree::isIndexableRow( void   * /* aIndex */,
                                 SChar  * aKeyValue,
                                 idBool * aIsIndexable )
{
    stdGeometryHeader * sValue;
    
    sValue = (stdGeometryHeader*)aKeyValue;
    
    stdUtils::isIndexableObject( sValue, aIsIndexable );
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            
 * ------------------------------------------------------------------*
 * Key Column 값을 문자열 형태로 구성하여 준다.
 *********************************************************************/
IDE_RC stndrRTree::columnValue2String( UChar * aColumnPtr,
                                       UChar * aText,
                                       UInt  * aTextLen )
{
    stdMBR  sMBR;
    UInt    sStringLen;
    
    idlOS::memcpy( &sMBR, aColumnPtr, sizeof(sMBR) );

    aText[0] = '\0';
    sStringLen = 0;

    sStringLen = idlVA::appendFormat(
        (SChar*)aText, 
        *aTextLen,
        "%"ID_FLOAT_G_FMT",%"ID_FLOAT_G_FMT",%"ID_FLOAT_G_FMT",%"ID_FLOAT_G_FMT,
        sMBR.mMinX, sMBR.mMinY, sMBR.mMaxX, sMBR.mMaxY );

    aText[sStringLen] = '\0';
    *aTextLen = sStringLen;

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :                                            
 * ------------------------------------------------------------------*
 * Column을 분석하여 ColumnValueLength와 ColumnValuePtr를 구하고,
 * 총 컬럼의 길이를 반환한다.
 *********************************************************************/
UShort stndrRTree::getColumnLength( UChar       * aColumnPtr,
                                    UInt        * aColumnHeaderLen,
                                    UInt        * aColumnValueLen,
                                    const void ** aColumnValuePtr )
{
    IDE_DASSERT( aColumnPtr       != NULL );
    IDE_DASSERT( aColumnHeaderLen != NULL );
    IDE_DASSERT( aColumnValueLen  != NULL );

    *aColumnHeaderLen = 0;
    *aColumnValueLen = ID_SIZEOF(stdMBR);

    if( aColumnValuePtr != NULL )
    {
        *aColumnValuePtr = aColumnPtr;
    }
    
    return (*aColumnHeaderLen) + (*aColumnValueLen);
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Disk R-Tree Meta Page Dump
 *
 * BUG-29039 codesonar ( Return Pointer to Local )
 *  - BUG-28379 와 동일하게 넘겨받은 버퍼에 담아 반환하도록 수정
 *********************************************************************/
IDE_RC stndrRTree::dumpMeta( UChar * aPage ,
                             SChar * aOutBuf ,
                             UInt    aOutSize )
{
    stndrMeta * sMeta;

    IDE_TEST_RAISE( ( aPage == NULL ) ||
                    ( sdpPhyPage::getPageStartPtr(aPage) != aPage ) , SKIP );

    sMeta = (stndrMeta*)( aPage + SMN_INDEX_META_OFFSET );

    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- Index Meta Page Begin ----------\n"
                     "mRootNode               : %"ID_UINT32_FMT"\n"
                     "mEmptyNodeHead          : %"ID_UINT32_FMT"\n"
                     "mEmptyNodeTail          : %"ID_UINT32_FMT"\n"
                     "mIsCreatedWithLogging   : %"ID_UINT32_FMT"\n"
                     "mNologgingCompletionLSN \n"
                     "                mFileNo : %"ID_UINT32_FMT"\n"
                     "                mOffset : %"ID_UINT32_FMT"\n"
                     "mIsConsistent           : %"ID_UINT32_FMT"\n"
                     "mIsCreatedWithForce     : %"ID_UINT32_FMT"\n"
                     "mFreeNodeCnt            : %"ID_UINT64_FMT"\n"
                     "mFreeNodeHead           : %"ID_UINT32_FMT"\n"
                     "----------- Index Meta Page End   ----------\n",
                     sMeta->mRootNode,
                     sMeta->mEmptyNodeHead,
                     sMeta->mEmptyNodeTail,
                     sMeta->mIsCreatedWithLogging,
                     sMeta->mNologgingCompletionLSN.mFileNo,
                     sMeta->mNologgingCompletionLSN.mOffset,
                     sMeta->mIsConsistent,
                     sMeta->mIsCreatedWithForce,
                     sMeta->mFreeNodeCnt,
                     sMeta->mFreeNodeHead );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * TASK-4007 [SM] PBT를 위한 기능 추가
 * 인덱스 페이지의 NodeHdr를 Dump하여 준다. 이때 만약 페이지가
 * Leaf페이지일 경우, CTS정보까지 Dump하여 보여준다.
 *********************************************************************/
IDE_RC stndrRTree::dumpNodeHdr( UChar * aPage,
                                SChar * aOutBuf,
                                UInt    aOutSize )
{
    stndrNodeHdr    * sNodeHdr;
    UInt              sCurrentOutStrSize;
    SChar             sStrFreeNodeSCN[ SM_SCN_STRING_LENGTH + 1];
    SChar             sStrNextFreeNodeSCN[ SM_SCN_STRING_LENGTH + 1];
#ifndef COMPILE_64BIT
    ULong               sSCN;
#endif

    IDE_DASSERT( aPage != NULL )
    IDE_DASSERT( sdpPhyPage::getPageStartPtr(aPage) == aPage );

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPage );

    // make scn string
    idlOS::memset( sStrFreeNodeSCN,     0x00, SM_SCN_STRING_LENGTH + 1 );
    idlOS::memset( sStrNextFreeNodeSCN, 0x00, SM_SCN_STRING_LENGTH + 1 );
#ifdef COMPILE_64BIT
    idlOS::sprintf( (SChar*)sStrFreeNodeSCN,
                    "%"ID_XINT64_FMT, sNodeHdr->mFreeNodeSCN );
    
    idlOS::sprintf( (SChar*)sStrNextFreeNodeSCN,
                    "%"ID_XINT64_FMT, sNodeHdr->mNextFreeNodeSCN );
#else
    sSCN  = (ULong)sNodeHdr->mFreeNodeSCN.mHigh << 32;
    sSCN |= (ULong)sNodeHdr->mFreeNodeSCN.mLow;
    idlOS::snprintf( (SChar*)sStrFreeNodeSCN, SM_SCN_STRING_LENGTH,
                     "%"ID_XINT64_FMT, sSCN );

    sSCN  = (ULong)sNodeHdr->mNextFreeNodeSCN.mHigh << 32;
    sSCN |= (ULong)sNodeHdr->mNextFreeNodeSCN.mLow;
    idlOS::snprintf( (SChar*)sStrNextFreeNodeSCN, SM_SCN_STRING_LENGTH,
                     "%"ID_XINT64_FMT, sSCN );
#endif

    // make output
    idlOS::snprintf( aOutBuf,
                     aOutSize,
                     "----------- Index Page Logical Header Begin ----------\n"
                     "MinX            : %"ID_FLOAT_G_FMT"\n"
                     "MinY            : %"ID_FLOAT_G_FMT"\n"
                     "MaxX            : %"ID_FLOAT_G_FMT"\n"
                     "MaxY            : %"ID_FLOAT_G_FMT"\n"
                     "NextEmptyNode   : %"ID_UINT32_FMT"\n"
                     "NextFreeNode    : %"ID_UINT32_FMT"\n"
                     "FreeNodeSCN     : %s\n",
                     "NextFreeNodeSCN : %s\n",
                     "Height          : %"ID_UINT32_FMT"\n"
                     "UnlimitedKeyCnt : %"ID_UINT32_FMT"\n"
                     "TotalDeadKeySize: %"ID_UINT32_FMT"\n"
                     "TBKCount        : %"ID_UINT32_FMT"\n"
                     "RefTBK1         : %"ID_UINT32_FMT"\n"
                     "RefTBK2         : %"ID_UINT32_FMT"\n"
                     "State(U,E,F)    : %"ID_UINT32_FMT"\n"
                     "----------- Index Page Logical Header End ------------\n",
                     sNodeHdr->mMBR.mMinX,
                     sNodeHdr->mMBR.mMinY,
                     sNodeHdr->mMBR.mMaxX,
                     sNodeHdr->mMBR.mMaxY,
                     sNodeHdr->mNextEmptyNode,
                     sNodeHdr->mNextFreeNode,
                     sStrFreeNodeSCN,
                     sStrNextFreeNodeSCN,
                     sNodeHdr->mHeight,
                     sNodeHdr->mUnlimitedKeyCount,
                     sNodeHdr->mTotalDeadKeySize,
                     sNodeHdr->mTBKCount,
                     sNodeHdr->mRefTBK[0],
                     sNodeHdr->mRefTBK[1],
                     sNodeHdr->mState );


    // Leaf라면, CTL도 Dump
    if( sNodeHdr->mHeight == 0 )
    {
        sCurrentOutStrSize = idlOS::strlen( aOutBuf );

        // sdnIndexCTL의 Dump는 무조건 성공해야합니다.
        IDE_ASSERT( sdnIndexCTL::dump( aPage,
                                       aOutBuf + sCurrentOutStrSize,
                                       aOutSize - sCurrentOutStrSize )
                  == IDE_SUCCESS );
    }

    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION :
 * ------------------------------------------------------------------*
 * Disk R-Tree Index Page Dump
 *
 * BUG-29039 codesonar ( Return Pointer to Local )
 *  - BUG-28379 와 동일하게 넘겨받은 버퍼에 담아 반환하도록 수정
 *********************************************************************/
IDE_RC stndrRTree::dump( UChar * aPage ,
                         SChar * aOutBuf ,
                         UInt    aOutSize )
{
    stndrNodeHdr    * sNodeHdr;
    UChar           * sSlotDirPtr;
    sdpSlotDirHdr   * sSlotDirHdr;
    sdpSlotEntry    * sSlotEntry;
    UShort            sSlotCount;
    UShort            sSlotNum;
    UShort            sOffset;
    SChar             sValueBuf[ SM_DUMP_VALUE_BUFFER_SIZE ]={0};
    stndrLKey       * sLKey;
    stndrIKey       * sIKey;
    stndrKeyInfo      sKeyInfo;
    scPageID          sChildPID;

    IDE_TEST_RAISE( ( aPage == NULL ) ||
                    ( sdpPhyPage::getPageStartPtr(aPage) != aPage ) , SKIP );

    sNodeHdr = (stndrNodeHdr*)sdpPhyPage::getLogicalHdrStartPtr( aPage );
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)aPage );
    sSlotDirHdr = (sdpSlotDirHdr*)sSlotDirPtr;
    sSlotCount  = sSlotDirHdr->mSlotEntryCount;

    sKeyInfo.mKeyState = 0;

    idlOS::snprintf( aOutBuf,
                     aOutSize,
"--------------------- Index Key Begin -----------------------------------------\n"
" No.|mOffset|mChildPID mRowPID mRowSlotNum mKeyState mKeyValue \n"
"----+-------+------------------------------------------------------------------\n" );

    for( sSlotNum=0; sSlotNum < sSlotCount; sSlotNum++ )
    {
        sSlotEntry = SDP_GET_SLOT_ENTRY(sSlotDirPtr, sSlotNum);

        sOffset = SDP_GET_OFFSET( sSlotEntry );
        if( SD_PAGE_SIZE < sOffset )
        {
            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "Invalid slot Offset (%d)\n",
                                 sSlotEntry );
            continue;
        }

        if( sNodeHdr->mHeight == 0 ) // Leaf node
        {
            sLKey = (stndrLKey*)sdpPhyPage::getPagePtrFromOffset( aPage, sOffset );

            STNDR_LKEY_TO_KEYINFO( sLKey, sKeyInfo );

            ideLog::ideMemToHexStr( sKeyInfo.mKeyValue,
                                    SM_DUMP_VALUE_LENGTH,
                                    IDE_DUMP_FORMAT_VALUEONLY,
                                    sValueBuf,
                                    SM_DUMP_VALUE_BUFFER_SIZE );

            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "%4"ID_UINT32_FMT"|"
                                 "%7"ID_UINT32_FMT"|"
                                 " <leaf node>  "
                                 "%8"ID_UINT32_FMT
                                 "%12"ID_UINT32_FMT
                                 "%10"ID_UINT32_FMT
                                 " %s\n",
                                 sSlotNum,
                                 *sSlotEntry,
                                 sKeyInfo.mRowPID,
                                 sKeyInfo.mRowSlotNum,
                                 sKeyInfo.mKeyState,
                                 sValueBuf );
        }
        else
        {   // InternalNode
            sIKey = (stndrIKey*)sdpPhyPage::getPagePtrFromOffset( aPage, sOffset );

            STNDR_IKEY_TO_KEYINFO( sIKey, sKeyInfo );
            STNDR_GET_CHILD_PID( &sChildPID, sIKey );

            ideLog::ideMemToHexStr( sKeyInfo.mKeyValue,
                                    SM_DUMP_VALUE_LENGTH,
                                    IDE_DUMP_FORMAT_VALUEONLY,
                                    sValueBuf,
                                    SM_DUMP_VALUE_BUFFER_SIZE );

            idlVA::appendFormat( aOutBuf,
                                 aOutSize,
                                 "%4"ID_UINT32_FMT"|"
                                 "%7"ID_UINT32_FMT"|"
                                 "%14"ID_UINT32_FMT
                                 "%8"ID_UINT32_FMT
                                 "%12"ID_UINT32_FMT
                                 "%10"ID_UINT32_FMT"\n",
                                 sSlotNum,
                                 *sSlotEntry,
                                 sChildPID,
                                 sKeyInfo.mRowPID,
                                 sKeyInfo.mRowSlotNum,
                                 sKeyInfo.mKeyState,
                                 sValueBuf );
        }
    }
    idlVA::appendFormat( aOutBuf,
                         aOutSize,
"--------------------- Index Key End   -----------------------------------------\n" );

    IDE_EXCEPTION_CONT( SKIP );

    return IDE_SUCCESS;
}

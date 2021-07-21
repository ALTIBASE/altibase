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
 * $Id: smnnModule.cpp 90158 2021-03-10 00:30:31Z emlee $
 **********************************************************************/

#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <smc.h>
#include <smm.h>
#include <smp.h>

#include <smErrorCode.h>
#include <smnModules.h>
#include <smnReq.h>
#include <smnnDef.h>
#include <smnnModule.h>
#include <smnManager.h>
#include <smmExpandChunk.h>

smnIndexModule smnnModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_MEMORY,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( smnnIterator ),
    (smnMemoryFunc) smnnSeq::prepareIteratorMem,
    (smnMemoryFunc) smnnSeq::releaseIteratorMem,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,
    (smTableCursorLockRowFunc)  smnManager::lockRow,
    (smnDeleteFunc)       NULL,
    (smnFreeFunc)         NULL,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,
    (smInitFunc)          smnnSeq::init,
    (smDestFunc)          smnnSeq::dest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc)  NULL,
    (smnSetPositionFunc)  NULL,

    (smnAllocIteratorFunc) smnnSeq::allocIterator,
    (smnFreeIteratorFunc)  smnnSeq::freeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,
    
    (smnMakeDiskImageFunc)  NULL,

    (smnBuildIndexFunc)     NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         smnnSeq::gatherStat
};

static const smSeekFunc smnnSeekFunctions[32][12] =
{ { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirstU,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATALBE      */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirstR,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirstU,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)smnnSeq::fetchNextU,
        (smSeekFunc)smnnSeq::fetchPrevU,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATALBE      */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::beforeFirstR,
        (smSeekFunc)smnnSeq::afterLastR,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnnSeq::fetchNextU,
        (smSeekFunc)smnnSeq::fetchPrevU,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLastU,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLastR,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLastU,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)smnnSeq::fetchPrevU,
        (smSeekFunc)smnnSeq::fetchNextU,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::afterLastR,
        (smSeekFunc)smnnSeq::beforeFirstR,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnnSeq::fetchPrev,
        (smSeekFunc)smnnSeq::fetchNext,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnnSeq::fetchPrevU,
        (smSeekFunc)smnnSeq::fetchNextU,
        (smSeekFunc)smnnSeq::afterLast,
        (smSeekFunc)smnnSeq::beforeFirst,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::AA,
        (smSeekFunc)smnnSeq::AA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA,
        (smSeekFunc)smnnSeq::NA
    }
};


IDE_RC smnnSeq::prepareIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

IDE_RC smnnSeq::releaseIteratorMem(const smnIndexModule* )
{
    return IDE_SUCCESS;
}

IDE_RC smnnSeq::init( smnnIterator      * aIterator,
                      void              * aTrans,
                      smcTableHeader    * aTable,
                      smnIndexHeader    *,
                      void              *,
                      const smiRange    *,
                      const smiRange    *,
                      const smiCallBack * aFilter,
                      UInt                aFlag,
                      smSCN               aSCN,
                      smSCN               aInfinite,
                      idBool             ,
                      smiCursorProperties * aProperties,
                      const smSeekFunc   ** aSeekFunc,
                      smiStatement        * aStatement )
{
    idvSQL                    *sSQLStat;

    aIterator->SCN                = aSCN;
    aIterator->infinite           = aInfinite;
    aIterator->trans              = aTrans;
    aIterator->table              = aTable;
    aIterator->curRecPtr          = NULL;
    aIterator->lstFetchRecPtr     = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->tid                = smLayerCallback::getTransID( aTrans );
    aIterator->filter             = aFilter;
    aIterator->mProperties        = aProperties;
    aIterator->mScanBackPID       = SM_NULL_PID;
    aIterator->mScanBackModifySeq = 0;
    aIterator->mModifySeqForScan  = 0;
    aIterator->page               = SM_NULL_PID;
    aIterator->mStatement         = aStatement;

    *aSeekFunc = smnnSeekFunctions[aFlag&(SMI_TRAVERSE_MASK|
                                          SMI_PREVIOUS_MASK|
                                          SMI_LOCK_MASK)];

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mMemCursorSeqScan, 1);
    
    if (sSQLStat != NULL)
    {
        IDV_SESS_ADD(sSQLStat->mSess, IDV_STAT_INDEX_MEM_CURSOR_SEQ_SCAN, 1);
    }
    
    return IDE_SUCCESS;
    
}

IDE_RC smnnSeq::dest( smnnIterator* /*aIterator*/ )
{
    return IDE_SUCCESS;
}

IDE_RC smnnSeq::AA( smnnIterator*,
                    const void** )
{

    return IDE_SUCCESS;

}

IDE_RC smnnSeq::NA( void )
{

    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;

}

IDE_RC smnnSeq::beforeFirst( smnnIterator* aIterator,
                             const smSeekFunc** )
{

    smpPageListEntry* sFixed;
    vULong            sSize;
#ifdef DEBUG
    smTID             sTransID;
#endif
    idBool            sLocked = ID_FALSE;
    SChar*            sRowPtr;
    SChar*            sPagePtr;
    SChar**           sSlot;
    SChar**           sHighFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
    smxTrans*         sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

    sFixed                       = (smpPageListEntry*)&(aIterator->table->mFixed);
    aIterator->curRecPtr         = NULL;
    aIterator->lstFetchRecPtr    = NULL;
    aIterator->page              = SM_NULL_PID;
    aIterator->lowFence          = aIterator->rows;
    aIterator->highFence         = aIterator->lowFence - 1;
    aIterator->slot              = aIterator->highFence;
    aIterator->least             = ID_TRUE;
    aIterator->highest           = ID_TRUE;
    sSize                        = sFixed->mSlotSize;
#ifdef  DEBUG
    sTransID                     = aIterator->tid;
#endif
    /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
     * 조회하는 기능으로 Page를 가져옴 */
    if( smuProperty::getCrashTolerance() == 0 )
    {
        if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt == 1 )
        {
            IDE_TEST( moveNext( aIterator ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( moveNextParallel( aIterator ) != IDE_SUCCESS );
        }
    }
    else
    {
        IDE_TEST( moveNextUsingFLI( aIterator ) != IDE_SUCCESS );
    }
    
    if( ( aIterator->page != SM_NULL_PID ) && 
        ( aIterator->page != SM_SPECIAL_PID ) &&
        ( aIterator->mProperties->mReadRecordCount > 0) )
    {
        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG               
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        { 
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        }        
#endif       

        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;

        IDE_ASSERT( smmManager::getPersPagePtr( aIterator->table->mSpaceID, 
                                                aIterator->page,
                                                (void**)&sPagePtr )
                    == IDE_SUCCESS );

        IDE_ASSERT( sPagePtr != NULL );
            
        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sRowPtr    = sPagePtr + SMP_PERS_PAGE_BODY_OFFSET,
             sHighFence = aIterator->highFence,
             sFence     = sRowPtr + sFixed->mSlotCount * sSize;
             sRowPtr    < sFence;
             sRowPtr   += sSize )
        {
            *(++sHighFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics)
                 != IDE_SUCCESS);
           
        for( sSlot  = aIterator->slot + 1;
             sSlot <= sHighFence;
             sSlot++ )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if( sCanReusableRollback == ID_FALSE )
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );

            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );
            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, 
                                                              *sSlot ) == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                    
                        *(++aIterator->highFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;
                        
                        aIterator->highest  = ID_TRUE;
                        *(++aIterator->highFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }
    }
    
    return IDE_SUCCESS;
   
    IDE_EXCEPTION_END;
     
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
    
}                                                                             

IDE_RC smnnSeq::afterLast( smnnIterator* aIterator,
                           const smSeekFunc** )
{

    smpPageListEntry* sFixed;
    vULong            sSize;
#ifdef DEBUG
    smTID             sTransID;
#endif
    idBool            sLocked = ID_FALSE;
    SChar*            sRowPtr;
    SChar**           sSlot;
    SChar**           sLowFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
    smxTrans        * sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

    sFixed                     = (smpPageListEntry*)&(aIterator->table->mFixed);
    aIterator->curRecPtr       = NULL;
    aIterator->lstFetchRecPtr  = NULL;
    aIterator->lowFence        = aIterator->rows + SMNN_ROWS_CACHE;
    aIterator->slot            = aIterator->lowFence;
    aIterator->highFence       = aIterator->lowFence - 1;
    aIterator->page            = SM_NULL_PID;
    aIterator->least           = ID_TRUE;
    aIterator->highest         = ID_TRUE;
    sSize                      = sFixed->mSlotSize;
#ifdef DEBUG
    sTransID                   = aIterator->tid;
#endif
    /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
     * 조회하는 기능으로 Page를 가져옴 */
    if( smuProperty::getCrashTolerance() == 0 )
    {
        IDE_TEST( movePrev( aIterator ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( movePrevUsingFLI( aIterator )!= IDE_SUCCESS );
    }
    
    if( aIterator->page != SM_NULL_PID && aIterator->mProperties->mReadRecordCount > 0)
    {
        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        {
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        }      
#endif

    
        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;
        
        IDE_ASSERT( smmManager::getOIDPtr( 
                          aIterator->table->mSpaceID, 
                          SM_MAKE_OID( aIterator->page, 
                                       SMP_PERS_PAGE_BODY_OFFSET ),
                          (void**)&sFence )
            == IDE_SUCCESS );
       
        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sLowFence  = aIterator->lowFence,
             sRowPtr    = sFence + ( sFixed->mSlotCount - 1 ) * sSize;
             sRowPtr   >= sFence;
             sRowPtr   -= sSize )
        {
            *(--sLowFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        for( sSlot  = aIterator->slot - 1;
             sSlot >= sLowFence;
             sSlot-- )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if ( sCanReusableRollback == ID_FALSE ) 
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );

            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );
            
            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, 
                                                              *sSlot ) == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                        *(--aIterator->lowFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;
                        
                        aIterator->least  = ID_TRUE;
                        *(--aIterator->lowFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }
    }

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;

}                                                                             

IDE_RC smnnSeq::beforeFirstU( smnnIterator*       aIterator,
                              const smSeekFunc** aSeekFunc )
{

    IDE_TEST( beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}                                                                             

IDE_RC smnnSeq::afterLastU( smnnIterator*       aIterator,
                            const smSeekFunc** aSeekFunc )
{
    
    IDE_TEST( afterLast( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}                                                                             

IDE_RC smnnSeq::beforeFirstR( smnnIterator*       aIterator,
                              const smSeekFunc** aSeekFunc )
{

    smnnIterator sIterator;
    UInt         sRowCount;
    
    IDE_TEST( beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    sIterator.curRecPtr           = aIterator->curRecPtr;
    sIterator.lstFetchRecPtr      = aIterator->lstFetchRecPtr;
    sIterator.least               = aIterator->least;
    sIterator.highest             = aIterator->highest;
    sIterator.page                = aIterator->page;
    sIterator.mScanBackPID        = aIterator->mScanBackPID;
    sIterator.mScanBackModifySeq  = aIterator->mScanBackModifySeq;
    sIterator.mModifySeqForScan   = aIterator->mModifySeqForScan;
    sIterator.slot                = aIterator->slot;
    sIterator.lowFence            = aIterator->lowFence;
    sIterator.highFence           = aIterator->highFence;
    sRowCount                     = aIterator->highFence - aIterator->rows + 1;

    /* BUG-27516 [5.3.3 release] Klocwork SM (5) 
     * sRowCount가 SMNN_ROWS_CACHE보다 큰 일은 있어서는 안됩니다.
     * SMNN_ROWS_CACHE는 SM_PAGE_SIZE / ID_SIZEOF( smpSlotHeader)로 한 페이지 내에
     * 들어갈 수 있는 슬롯의 최대 개수이기 때문입니다. */
    IDE_ASSERT( sRowCount < SMNN_ROWS_CACHE ); 
    idlOS::memcpy( sIterator.rows, aIterator->rows, (size_t)ID_SIZEOF(SChar*) * sRowCount );
           
    IDE_TEST( fetchNextR( aIterator ) != IDE_SUCCESS );
    
    aIterator->curRecPtr          = sIterator.curRecPtr;
    aIterator->lstFetchRecPtr     = sIterator.lstFetchRecPtr;
    aIterator->least              = sIterator.least;
    aIterator->highest            = sIterator.highest;
    aIterator->page               = sIterator.page;
    aIterator->mScanBackPID       = sIterator.mScanBackPID;
    aIterator->mScanBackModifySeq = sIterator.mScanBackModifySeq;
    aIterator->mModifySeqForScan  = sIterator.mModifySeqForScan;
    aIterator->slot               = sIterator.slot;
    aIterator->lowFence           = sIterator.lowFence;
    aIterator->highFence          = sIterator.highFence;
    idlOS::memcpy( aIterator->rows, sIterator.rows, (size_t)ID_SIZEOF(SChar*) * sRowCount );

    IDE_ASSERT(SMNN_ROWS_CACHE >= sRowCount);
    
    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}                                                                             

IDE_RC smnnSeq::afterLastR( smnnIterator*       aIterator,
                            const smSeekFunc** aSeekFunc )
{
    
    IDE_TEST( beforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    IDE_TEST( fetchNextR( aIterator ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
    
}

/****************************************************************************
 *
 * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
 *
 * 현재 Scan Page로부터 다음 읽을 Scan Page를 구함.
 *
 * Iterator가 가리키는 Page가 Cache된 이후에 수정될수 있기 때문에 Next Link가
 * 유효하지 않을수 있다. 이에 대한 보정이 필요하며 이는 Modify Sequence를 이용한다.
 * Cache된 이후에 Scan List에서 제거되었다면 Modify Sequence가 증가되기 때문에
 * 이를 확인한후 유효한 Next Link를 확보한다.
 *
 * 또한 현재 Page가 Scan List상에 존재하지 않을수 있기 때문에 Scan Back Page로
 * 이동후 다시 재시작한다.
 *
 * Scan Back Page는 한건이라도 유효한 레코드가 있는 페이지를 의미한다. 즉,
 * 절대로 Scan List에서 제거되지 않는 페이지를 의미한다.
 *
 ****************************************************************************/

IDE_RC smnnSeq::moveNextBlock( smnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sCurScanModifySeq = 0;
    ULong                  sNxtScanModifySeq = 0;
    scSpaceID              sSpaceID;
    scPageID               sNextPID = SM_NULL_PID;
    smpScanPageListEntry * sScanPageList;

    IDE_DASSERT( aIterator != NULL );
    
    sFixed  = (smpPageListEntry*)&(aIterator->table->mFixed);
    sScanPageList = &(sFixed->mRuntimeEntry->mScanPageList);
    sSpaceID = aIterator->table->mSpaceID;
    
    while( 1 )
    {
        IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

        /*
         * 이전에 Iterator에서 QP로 결과를 올린적이 없는 경우
         * 1. beforeFirst가 호출된 경우
         * 2. page가 NULL을 갖는 ScanBackPID로부터 설정된 경우
         */
        if( aIterator->page == SM_NULL_PID )
        {
            aIterator->page = smpManager::getFirstScanPageID( sFixed );

            if( aIterator->page == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->mModifySeqForScan = sNxtScanModifySeq = sCurScanModifySeq;

            sNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                      aIterator->page );
        }
        else
        {
            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->page = smpManager::getNextScanPageID( sSpaceID,
                                                             aIterator->page );
           
            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if( (aIterator->page != SM_SPECIAL_PID) &&
                (aIterator->page != SM_NULL_PID) )
            {
                sNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                          aIterator->page );

                sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                     aIterator->page );
            }
        }
        
        IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

        /*
         * 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
         * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Next Page를 보정한다.
         */
        if( sCurScanModifySeq != aIterator->mModifySeqForScan )
        {
            aIterator->page = aIterator->mScanBackPID;
            aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
        }
        else
        {
            IDE_DASSERT( aIterator->page != SM_NULL_PID );
            aIterator->mModifySeqForScan = sNxtScanModifySeq;
            break;
        }
    }

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        // BUG-43463 Next Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->highest = ( SM_SPECIAL_PID == sNextPID ) ? ID_TRUE : ID_FALSE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************************************
 * BUG-43463 memory full scan page list mutex 병목 제거
 *
 *     Fullscan시에 iterator를 Next Page로 옮기는 함수,
 *     smnnSeq::moveNextBlock() 함수와 동일한 역할을 한다,
 *     sScanPageList->mMutex 를 잡지 않도록 수정되었다.
 *
 * aIterator    - [IN/OUT]  Iterator
 *
 ****************************************************************************/
IDE_RC smnnSeq::moveNextNonBlock( smnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sNxtScanModifySeq = 0;
    ULong                  sCurScanModifySeq = 0;
    scSpaceID              sSpaceID;
    scPageID               sCurrPID;
    scPageID               sNextPID;
    scPageID               sNextNextPID = SM_NULL_PID;
    smpScanPageListEntry * sScanPageList;

    IDE_DASSERT( aIterator != NULL );

    sFixed  = (smpPageListEntry*)&(aIterator->table->mFixed);
    sScanPageList = &(sFixed->mRuntimeEntry->mScanPageList);
    sSpaceID = aIterator->table->mSpaceID;

    while( 1 )
    {
        /*
         * 이전에 Iterator에서 QP로 결과를 올린적이 없는 경우
         * 1. beforeFirst가 호출된 경우
         * 2. page가 NULL을 갖는 ScanBackPID로부터 설정된 경우
         */
        if( aIterator->page == SM_NULL_PID )
        {
            // 첫번째 page는 lock을 잡고 가져온다.
            IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

            sNextPID = smpManager::getFirstScanPageID( sFixed );

            if( sNextPID == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 sNextPID );

            sNextNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                          sNextPID );

            IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

            aIterator->mModifySeqForScan = sNxtScanModifySeq;
            aIterator->page              = sNextPID;

            break;
        }
        else
        {
            // Curr PID를 기반으로 Next PID를 세팅하고,
            // Next의 Next PID를 확인해서 Next page가 마지막인지 확인한다.

            sCurrPID = aIterator->page;

            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 sCurrPID );

            // Curr Page가 List에서 제거되거나 위치가 바뀌었다.
            // ScanBackPID에서 다시 시작한다.
            if( sCurScanModifySeq != aIterator->mModifySeqForScan )
            {
                // Curr Page가 바뀌었다면 current page가 scan back page일 리가 없다.
                // 반대로 scanback pid와 curr pid가 같으면 hang이 발생하므로 검증한다.
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
                // Modify Seq는 Page 가 link, unlink시점에 갱신된다.
                // Scan Back Page는 ager가 지울 수 없는 slot을 포함하므로
                // List에서 제거 될 수 없어서 Modify Seq가 갱신되지 않는다.
                continue;
            }

            // 홀수인 경우는 갱신 중인 경우이다.
            IDE_ASSERT( SMM_PAGE_IS_MODIFYING( sCurScanModifySeq ) == ID_FALSE );

            // Iterator가 가리켜야 할 Next Page
            sNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                      sCurrPID );

            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if( ( sNextPID != SM_SPECIAL_PID) &&
                ( sNextPID != SM_NULL_PID) )
            {
                // Next Modify Seq는 Iterator에 세팅하기 위해 필요하다.
                // NextNext PID는 NextPID가 마지막 Page인지를 판단하는 용도로 사용된다.
                // NextNext PID의 정확성을 위해 Modify Seq를 먼저 읽는다.
                // Link,Unlink시에 연산 앞/뒤로 Modify Seq가 갱신된다.
                sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                     sNextPID );
                sNextNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                              sNextPID );

                // - Modify Seq가 홀수이면 Link, Unlink작업 중이다.
                // - Modify Seq가 변경 되었으면 Link, Unlink가 완료 되어버렸다.
                // - Next PID가 변경 되었으면 Modify Seq를 읽기 전에 Curr Page가 변경 된 것이라서
                //   Next Page ID 및 Next Page의 Modify Seq를 믿을 수 없다.
                if(( SMM_PAGE_IS_MODIFYING( sNxtScanModifySeq ) == ID_TRUE ) ||
                   ( smpManager::getNextScanPageID( sSpaceID,
                                                    sCurrPID ) != sNextPID ) ||
                   ( smpManager::getModifySeqForScan( sSpaceID,
                                                      sNextPID ) != sNxtScanModifySeq ))
                {
                    // curr page 부터 다시 시작한다.
                    continue;
                }
            }

            // Curr Modify Seq가 변경 되었다면, Curr Page가 Unlink된것이다.
            // Next PID도 Unlink후에 얻어 온 것일 수 있으므로 믿을 수 없다.
            // Scan back page부터 다시 시작한다.
            if( smpManager::getModifySeqForScan( sSpaceID,
                                                 sCurrPID ) != sCurScanModifySeq )
            {
                // Curr Page가 바뀌었다면 current page가 scan back page일 리가 없다.
                // 반대로 scanback pid와 curr pid가 같으면 hang이 발생하므로 검증한다.
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                /* 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
                * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Next Page를 보정한다.
                */
                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
            }
            else
            {
                IDE_ASSERT( sNextPID != SM_NULL_PID );

                aIterator->page              = sNextPID;
                aIterator->mModifySeqForScan = sNxtScanModifySeq;

                break;
            }
        }
    }

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        // NextNext PageID를 기반으로 Next page가 마지막 page인지 확인한다.
        // BUG-43463 Next Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->highest = ( SM_SPECIAL_PID == sNextNextPID ) ? ID_TRUE : ID_FALSE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

// PROJ-2402
IDE_RC smnnSeq::moveNextParallelBlock( smnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sCurScanModifySeq = 0;
    ULong                  sNxtScanModifySeq = 0;
    scSpaceID              sSpaceID;
    smpScanPageListEntry * sScanPageList;
    ULong                  sThreadCnt;
    ULong                  sThreadID;
    scPageID               sNextPID = SM_NULL_PID;

    IDE_ASSERT( aIterator != NULL );

    sFixed        = (smpPageListEntry*)&(aIterator->table->mFixed);
    sScanPageList = &(sFixed->mRuntimeEntry->mScanPageList);
    sSpaceID      = aIterator->table->mSpaceID;
    sThreadCnt    = aIterator->mProperties->mParallelReadProperties.mThreadCnt;
    sThreadID     = aIterator->mProperties->mParallelReadProperties.mThreadID;

    while ( 1 )
    {
        IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

        /*
         * 이전에 Iterator에서 QP로 결과를 올린적이 없는 경우
         * 1. beforeFirst가 호출된 경우
         * 2. page가 NULL을 갖는 ScanBackPID로부터 설정된 경우
         */
        if ( aIterator->page == SM_NULL_PID )
        {
            aIterator->page = smpManager::getFirstScanPageID( sFixed );

            if ( aIterator->page == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }
            else
            {
                // nothing to do
            }

            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->mModifySeqForScan = sNxtScanModifySeq = sCurScanModifySeq;

            sNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                      aIterator->page );
        }
        else
        {
            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->page = smpManager::getNextScanPageID( sSpaceID,
                                                             aIterator->page );

            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if ( ( aIterator->page != SM_SPECIAL_PID ) &&
                 ( aIterator->page != SM_NULL_PID) )
            {
                sNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                          aIterator->page );

                sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                     aIterator->page );
            }
            else
            {
                // nothing to do
            }
        }

        IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

        /*
         * 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
         * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Next Page를 보정한다.
         */
        if ( sCurScanModifySeq != aIterator->mModifySeqForScan )
        {
            aIterator->page              = aIterator->mScanBackPID;
            aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
        }
        else
        {
            IDE_ASSERT( aIterator->page != SM_NULL_PID );
            aIterator->mModifySeqForScan = sNxtScanModifySeq;

            if ( ( aIterator->page != SM_SPECIAL_PID ) &&
                 ( aIterator->page != SM_NULL_PID) )
            {
                if ( ( aIterator->page % sThreadCnt + 1 ) != sThreadID )
                {
                    continue;
                }
                else
                {
                    // nothing to do
                }
            }
            else
            {
                // nothing to do
            }

            break;
        }
    }

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        // BUG-43463 Next Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->highest = ( SM_SPECIAL_PID == sNextPID ) ? ID_TRUE : ID_FALSE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 * BUG-43463 memory full scan page list mutex 병목 제거
 *
 *     Parallel Fullscan시에 iterator를 Next Page로 옮기는 함수,
 *     smnnSeq::moveNextParallelBlock() 함수와 동일한 역할을 한다,
 *     sScanPageList->mMutex 를 잡지 않도록 수정되었다.
 *     동작 원리는 smnnSeq::moveNextNonBlock() 주석을 참고하라
 *
 * aIterator    - [IN/OUT]  Iterator
 *
 ****************************************************************************/
IDE_RC smnnSeq::moveNextParallelNonBlock( smnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sNxtScanModifySeq = 0;
    ULong                  sCurScanModifySeq = 0;
    scSpaceID              sSpaceID;
    smpScanPageListEntry * sScanPageList;
    ULong                  sThreadCnt;
    ULong                  sThreadID;
    scPageID               sNextNextPID = SM_NULL_PID;
    scPageID               sCurrPID;
    scPageID               sNextPID;

    IDE_ASSERT( aIterator != NULL );

    sFixed        = (smpPageListEntry*)&(aIterator->table->mFixed);
    sScanPageList = &(sFixed->mRuntimeEntry->mScanPageList);
    sSpaceID      = aIterator->table->mSpaceID;
    sThreadCnt    = aIterator->mProperties->mParallelReadProperties.mThreadCnt;
    sThreadID     = aIterator->mProperties->mParallelReadProperties.mThreadID;

    while ( 1 )
    {
        /*
         * 이전에 Iterator에서 QP로 결과를 올린적이 없는 경우
         * 1. beforeFirst가 호출된 경우
         * 2. page가 NULL을 갖는 ScanBackPID로부터 설정된 경우
         */
        if ( aIterator->page == SM_NULL_PID )
        {
            IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

            sNextPID = smpManager::getFirstScanPageID( sFixed );

            if ( sNextPID == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }
            else
            {
                // nothing to do
            }
            IDE_ASSERT ( sNextPID != SM_SPECIAL_PID );

            sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 sNextPID );

            sNextNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                          sNextPID );

            IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

            aIterator->page              = sNextPID;
            aIterator->mModifySeqForScan = sNxtScanModifySeq;

            if ( ( sNextPID % sThreadCnt + 1 ) != sThreadID )
            {
                continue;
            }

            break;
        }
        else
        {
            sCurrPID = aIterator->page;
            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 sCurrPID );

            if( sCurScanModifySeq != aIterator->mModifySeqForScan )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;

                continue;
            }

            IDE_ASSERT( SMM_PAGE_IS_MODIFYING( sCurScanModifySeq ) == ID_FALSE );

            sNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                      sCurrPID );
            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if ( ( sNextPID != SM_SPECIAL_PID ) &&
                 ( sNextPID != SM_NULL_PID) )
            {
                sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                     sNextPID );

                sNextNextPID = smpManager::getNextScanPageID( sSpaceID,
                                                              sNextPID );

                if(( SMM_PAGE_IS_MODIFYING( sNxtScanModifySeq ) == ID_TRUE ) ||
                   ( smpManager::getNextScanPageID( sSpaceID,
                                                    sCurrPID ) != sNextPID ) ||
                   ( smpManager::getModifySeqForScan( sSpaceID,
                                                      sNextPID ) != sNxtScanModifySeq ))
                {
                    continue;
                }
            }
            else
            {
                // nothing to do
            }

            /*
             * 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
             * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Next Page를 보정한다.
             */

            if( smpManager::getModifySeqForScan( sSpaceID,
                                                 sCurrPID ) != sCurScanModifySeq )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
            }
            else
            {
                IDE_ASSERT( sNextPID != SM_NULL_PID );

                aIterator->page              = sNextPID;
                aIterator->mModifySeqForScan = sNxtScanModifySeq;

                if ( ( sNextPID != SM_SPECIAL_PID ) &&
                     ( sNextPID != SM_NULL_PID) )
                {
                    if ( ( sNextPID % sThreadCnt + 1 ) != sThreadID )
                    {
                        continue;
                    }
                }
                else
                {
                    // nothing to do
                }

                break;
            }
        }
    }

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        // NextNext PageID를 기반으로 Next page가 마지막 page인지 확인한다.
        // BUG-43463 Next Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->highest = ( SM_SPECIAL_PID == sNextNextPID ) ? ID_TRUE : ID_FALSE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/****************************************************************************
 *
 * BUG-25179 [SMM] Full Scan을 위한 페이지간 Scan List가 필요합니다.
 *
 * 현재 Scan Page로부터 다음 읽을 Scan Page를 구함.
 *
 * Iterator가 가리키는 Page가 Cache된 이후에 수정될수 있기 때문에 Previous Link가
 * 유효하지 않을수 있다. 이에 대한 보정이 필요하며 이는 Modify Sequence를 이용한다.
 * Cache된 이후에 Scan List에서 제거되었다면 Modify Sequence가 증가되기 때문에
 * 이를 확인한후 유효한 Previous Link를 확보한다.
 *
 * 또한 현재 Page가 Scan List상에 존재하지 않을수 있기 때문에 Scan Back Page로
 * 이동후 다시 재시작한다.
 *
 * Scan Back Page는 한건이라도 유효한 레코드가 있는 페이지를 의미한다. 즉,
 * 절대로 Scan List에서 제거되지 않는 페이지를 의미한다.
 *
 ****************************************************************************/

IDE_RC smnnSeq::movePrevBlock( smnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sCurScanModifySeq = 0;
    ULong                  sNxtScanModifySeq = 0;
    smpScanPageListEntry * sScanPageList;
    scSpaceID              sSpaceID;
    scPageID               sPrevPID = 0;

    IDE_DASSERT( aIterator != NULL );
    
    sFixed  = (smpPageListEntry*)&(aIterator->table->mFixed);
    sScanPageList = &(sFixed->mRuntimeEntry->mScanPageList);
    sSpaceID = aIterator->table->mSpaceID;
    
    while( 1 )
    {
        IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );
        
        /*
         * 이전에 Iterator에서 QP로 결과를 올린적이 없는 경우
         * 1. beforeFirst가 호출된 경우
         * 2. page가 NULL을 갖는 ScanBackPID로부터 설정된 경우
         */
        if( aIterator->page == SM_NULL_PID )
        {
            aIterator->page = smpManager::getLastScanPageID( sFixed );

            if( aIterator->page == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }
            
            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->mModifySeqForScan = sNxtScanModifySeq = sCurScanModifySeq;

            sPrevPID = smpManager::getPrevScanPageID( sSpaceID,
                                                      aIterator->page );
        }
        else
        {
            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->page = smpManager::getPrevScanPageID( sSpaceID,
                                                             aIterator->page );

            /*
             * 현재 Page가 Scan List상의 처음 Page라면 더이상 Prev링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Previous Page를 보정한다.
             */
            if( (aIterator->page != SM_SPECIAL_PID) &&
                (aIterator->page != SM_NULL_PID) )
            {
                sPrevPID = smpManager::getPrevScanPageID( sSpaceID,
                                                          aIterator->page );
                sNxtScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                     aIterator->page );
            }
        }
        
        IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
        
        /*
         * 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
         * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Previous Page를 보정한다.
         */
        if( sCurScanModifySeq != aIterator->mModifySeqForScan )
        {
            aIterator->page = aIterator->mScanBackPID;
            aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
        }
        else
        {
            aIterator->mModifySeqForScan = sNxtScanModifySeq;
            break;
        }
    }

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        // BUG-43463 Prev Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->least = ( SM_SPECIAL_PID == sPrevPID ) ? ID_TRUE : ID_FALSE ;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************************************
 * BUG-43463 memory full scan page list mutex 병목 제거
 *
 *     Fullscan시에 iterator를 Prev Page로 옮기는 함수,
 *     smnnSeq::movePrevBlock() 함수와 동일한 역할을 한다,
 *     sScanPageList->mMutex 를 잡지 않도록 수정되었다.
 *     동작 원리는 smnnSeq::moveNextNonBlock() 주석을 참고하라
 *
 * aIterator    - [IN/OUT]  Iterator
 *
 ****************************************************************************/
IDE_RC smnnSeq::movePrevNonBlock( smnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sPrvScanModifySeq = 0;
    ULong                  sCurScanModifySeq = 0;
    smpScanPageListEntry * sScanPageList;
    scSpaceID              sSpaceID;
    scPageID               sPrevPrevPID = SM_NULL_PID;
    scPageID               sPrevPID;
    scPageID               sCurrPID;

    IDE_DASSERT( aIterator != NULL );

    sFixed  = (smpPageListEntry*)&(aIterator->table->mFixed);
    sScanPageList = &(sFixed->mRuntimeEntry->mScanPageList);
    sSpaceID = aIterator->table->mSpaceID;

    while( 1 )
    {
        /*
         * 이전에 Iterator에서 QP로 결과를 올린적이 없는 경우
         * 1. beforeFirst가 호출된 경우
         * 2. page가 NULL을 갖는 ScanBackPID로부터 설정된 경우
         */
        if( aIterator->page == SM_NULL_PID )
        {
            IDE_TEST( sScanPageList->mMutex->lock( NULL ) != IDE_SUCCESS );

            sPrevPID = smpManager::getLastScanPageID( sFixed );

            if( sPrevPID == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sPrvScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 sPrevPID );

            sPrevPrevPID = smpManager::getPrevScanPageID( sSpaceID,
                                                          sPrevPID );

            IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

            aIterator->mModifySeqForScan = sPrvScanModifySeq;
            aIterator->page              = sPrevPID;

            break;
        }
        else
        {
            sCurrPID = aIterator->page;

            sCurScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                 sCurrPID );

            if( sCurScanModifySeq != aIterator->mModifySeqForScan )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;

                continue;
            }

            IDE_ASSERT( SMM_PAGE_IS_MODIFYING( sCurScanModifySeq ) == ID_FALSE );

            sPrevPID = smpManager::getPrevScanPageID( sSpaceID,
                                                      sCurrPID );
            /*
             * 현재 Page가 Scan List상의 처음 Page라면 더이상 Prev링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Previous Page를 보정한다.
             */
            if( ( sPrevPID != SM_SPECIAL_PID ) &&
                ( sPrevPID != SM_NULL_PID ) )
            {
                sPrvScanModifySeq = smpManager::getModifySeqForScan( sSpaceID,
                                                                     sPrevPID );
                sPrevPrevPID = smpManager::getPrevScanPageID( sSpaceID,
                                                              sPrevPID );

                if(( SMM_PAGE_IS_MODIFYING( sPrvScanModifySeq ) == ID_TRUE ) ||
                   ( smpManager::getPrevScanPageID( sSpaceID,
                                                    sCurrPID ) != sPrevPID ) ||
                   ( smpManager::getModifySeqForScan( sSpaceID,
                                                      sPrevPID ) != sPrvScanModifySeq ))
                {
                    continue;
                }
            }

            if( smpManager::getModifySeqForScan( sSpaceID,
                                                 sCurrPID ) != sCurScanModifySeq )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                /* 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
                * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Next Page를 보정한다.
                */
                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;
            }
            else
            {
                aIterator->page              = sPrevPID;
                aIterator->mModifySeqForScan = sPrvScanModifySeq;

                break;
            }
        }
    }

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        // PrevPrev PageID를 기반으로 prev page가 마지막 page인지 확인한다.
        // BUG-43463 Prev Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->least = ( SM_SPECIAL_PID == sPrevPrevPID ) ? ID_TRUE : ID_FALSE ;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/****************************************************************************
 *
 * PROJ-2162 RestartRiskReduction
 *
 * 어떠한 이유로 Refine등이 실패할 경우 MRDB의 PageList가 깨지게 되며, 이 경우
 * 조회가 불가능하다.
 *
 * 따라서 이를 극복하기 위해 Tablespace 전체를 조회하고, PageHeader에
 * 기록된 TableOID를 바탕으로 자신의 Page인지 확인하는 식으로 Page를 구하는
 * smnnMoveNextUsingFLI함수를 구현한다.
 *
 ****************************************************************************/
IDE_RC smnnSeq::movePageUsingFLI( smnnIterator * aIterator,
                                  scPageID     * aNextPID )
{
    smpPageListEntry     * sFixed;
    scSpaceID              sSpaceID;
    scPageID               sPageID;
    scPageID               sMaxPID;
    smmTBSNode           * sTBSNode;
    smmPageState           sPageState;
    smpPersPageHeader    * sPersPageHeaderPtr;
    smOID                  sTableOID;

    sSpaceID  = aIterator->table->mSpaceID;
    sPageID   = aIterator->page;
    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed);
    sTableOID = sFixed->mTableOID;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sSpaceID,
                                                        (void**)&sTBSNode)
              != IDE_SUCCESS );
 
    if( aIterator->page == SM_NULL_PID )
    {
        /* 최초의 탐색일 경우 */
        aIterator->page = 1;
    }
    else
    {
        sPageID ++;
    }

    (*aNextPID) = SM_SPECIAL_PID;
    sMaxPID = sTBSNode->mMemBase->mAllocPersPageCount;
    while( sPageID < sMaxPID )
    {
        /* alloc page를 찾을때까지 탐색 */
        IDE_TEST( smmExpandChunk::getPageState( sTBSNode, 
                                                sPageID, 
                                                &sPageState )
                  != IDE_SUCCESS );

        if( sPageState == SMM_PAGE_STATE_ALLOC )
        {
            IDE_TEST( smmManager::getPersPagePtr( sSpaceID,
                                                  sPageID,
                                                  (void**)&sPersPageHeaderPtr )
                      != IDE_SUCCESS );
            /* 1) TableOID가 일치하고
             * 2) FixPage이고 (VarPage는 상관 없음)
             * 3) Consistency가 있거나, Property로 상관 없으면 */
            if( ( sPersPageHeaderPtr->mTableOID == sTableOID ) &&
                ( SMP_GET_PERS_PAGE_TYPE( sPersPageHeaderPtr ) == 
                        SMP_PAGETYPE_FIX ) &&
                ( ( SMP_GET_PERS_PAGE_INCONSISTENCY( sPersPageHeaderPtr ) ==
                    SMP_PAGEINCONSISTENCY_TRUE ) ||
                  ( smuProperty::getCrashTolerance() == 2 ) ) )
            {
                *aNextPID = sPageID;
                /* Found! */
                break;
            }
        }
        sPageID ++;
    } 

    if( sPageID == sMaxPID )
    {
        /* 마지막에 도달함 */
        sPageID = SM_SPECIAL_PID;
    }
    aIterator->page = sPageID;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/* 어차피 비상상황에서 데이터 획득을 위해 사용하는 기능이기 때문에
 * FetchPrev도 FetchNext와 동일하게 구현한다. */
IDE_RC smnnSeq::movePrevUsingFLI( smnnIterator * aIterator )
{
    scPageID   sPrevPrevPID = SM_NULL_PID;

    IDE_TEST( movePageUsingFLI( aIterator, &sPrevPrevPID ) != IDE_SUCCESS );

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        aIterator->least = ( SM_SPECIAL_PID == sPrevPrevPID ) ? ID_TRUE : ID_FALSE ;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnnSeq::moveNextUsingFLI( smnnIterator * aIterator )
{
    scPageID   sNextNextPID = SM_NULL_PID;

    IDE_TEST( movePageUsingFLI( aIterator, &sNextNextPID ) != IDE_SUCCESS );

    if ( ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->page != SM_NULL_PID) )
    {
        aIterator->highest = ( SM_SPECIAL_PID == sNextNextPID ) ? ID_TRUE : ID_FALSE;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


IDE_RC smnnSeq::fetchNext( smnnIterator* aIterator,
                           const void**  aRow )
{
    idBool sFound = ID_FALSE;

    do
    {
        aIterator->slot++;
        if( aIterator->slot <= aIterator->highFence )
        {
            aIterator->curRecPtr      = *aIterator->slot;
            aIterator->lstFetchRecPtr = aIterator->curRecPtr;
            *aRow                     = aIterator->curRecPtr;
            SC_MAKE_GRID( aIterator->mRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                          SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

            break;
        }

        IDE_TEST( makeRowCache( aIterator, aRow, &sFound ) != IDE_SUCCESS );
    }
    while( sFound == ID_TRUE );
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC smnnSeq::makeRowCache ( smnnIterator      * aIterator,
                               const void       ** aRow,
                               idBool            * aFound )
{
    smpPageListEntry* sFixed;
    vULong            sSize;
    idBool            sLocked;
    SChar*            sRowPtr;
    SChar**           sSlot;
    SChar**           sHighFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
#ifdef DEBUG
    smTID             sTransID; 
#endif
    smxTrans        * sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;
    
    sLocked = ID_FALSE;
    *aFound = ID_FALSE;
#ifdef DEBUG
    sTransID = aIterator->tid;    
#endif
    sFixed  = (smpPageListEntry*)&(aIterator->table->mFixed);
    sSize   = sFixed->mSlotSize;
   
    if( aIterator->highest == ID_FALSE )
    {
        /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
         * 조회하는 기능으로 Page를 가져옴 */
        if( smuProperty::getCrashTolerance() == 0 )
        {
            if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt == 1 )
            {
                IDE_TEST( moveNext( aIterator ) != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( moveNextParallel( aIterator ) != IDE_SUCCESS );
            }
        }
        else
        {
            IDE_TEST( moveNextUsingFLI( aIterator ) != IDE_SUCCESS );
        }

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 next page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );

        aIterator->slot      = ( aIterator->lowFence = aIterator->rows ) - 1;
        aIterator->highFence = aIterator->slot;
        aIterator->least     = ID_FALSE;
        
        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        {
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        } 
#endif

        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) 
                  != IDE_SUCCESS );
        sLocked = ID_TRUE;
        
        IDE_ERROR( smmManager::getOIDPtr( aIterator->table->mSpaceID,
                                          SM_MAKE_OID( aIterator->page,
                                                       SMP_PERS_PAGE_BODY_OFFSET ),
                                          (void**)&sRowPtr )
                    == IDE_SUCCESS );

        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sHighFence = aIterator->highFence,
             sFence     = sRowPtr + sFixed->mSlotCount * sSize;
             sRowPtr    < sFence;
             sRowPtr   += sSize )
        {
            *(++sHighFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );
        
        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) 
                 != IDE_SUCCESS);
         IDU_FIT_POINT_RAISE( "BUG-48258@smnnSeq::makeRowCache", ERR_ABORT_INTERNAL );

        for( sSlot  = aIterator->slot + 1;
             sSlot <= sHighFence;
             sSlot++ )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if( sCanReusableRollback == ID_FALSE ) 
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );
            
            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );

            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, 
                                                              *sSlot ) == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                    
                        *(++aIterator->highFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;

                        aIterator->highest  = ID_TRUE;
                        *(++aIterator->highFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }

        *aFound = ID_TRUE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    if( *aFound == ID_FALSE )
    {
        aIterator->slot           = aIterator->highFence + 1;
        aIterator->curRecPtr      = NULL;
        aIterator->lstFetchRecPtr = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
        *aRow                     = NULL;
    }

    return IDE_SUCCESS;
#ifdef ALTIBASE_FIT_CHECK
    IDE_EXCEPTION( ERR_ABORT_INTERNAL );
    {
        IDE_SET( ideSetErrorCode( smERR_ABORT_INTERNAL ) );
    }
#endif 
    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

IDE_RC smnnSeq::fetchPrev( smnnIterator* aIterator,
                           const void**  aRow )
{
    
    smpPageListEntry* sFixed;
    vULong            sSize;
    idBool            sLocked = ID_FALSE;
    SChar*            sRowPtr;
    SChar**           sSlot;
    SChar**           sLowFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
#ifdef DEBUG
    smTID             sTransID = aIterator->tid;    
#endif
    smxTrans        * sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

restart:
    
    for( aIterator->slot--;
         aIterator->slot >= aIterator->lowFence;
         aIterator->slot-- )
    {
        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;
        *aRow          = aIterator->curRecPtr;
        SC_MAKE_GRID( aIterator->mRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        return IDE_SUCCESS;
    }

    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed);
    sSize     = sFixed->mSlotSize;
    
    if( aIterator->least == ID_FALSE )
    {
        /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
         * 조회하는 기능으로 Page를 가져옴 */
        if( smuProperty::getCrashTolerance() == 0 )
        {
            IDE_TEST( movePrev( aIterator ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( movePrevUsingFLI( aIterator ) != IDE_SUCCESS );
        }

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 previous page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );
        
        aIterator->highFence =
            ( aIterator->slot = aIterator->lowFence = aIterator->rows + SMNN_ROWS_CACHE ) - 1;
        aIterator->highest = ID_TRUE;

        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        {
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;

        IDE_ASSERT( smmManager::getOIDPtr( 
                          aIterator->table->mSpaceID, 
                          SM_MAKE_OID( aIterator->page, 
                                       SMP_PERS_PAGE_BODY_OFFSET ),
                          (void**)&sFence )
            == IDE_SUCCESS );
       
        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sLowFence = aIterator->lowFence,
             sRowPtr   = sFence + ( sFixed->mSlotCount - 1 ) * sSize;
             sRowPtr  >= sFence;
             sRowPtr  -= sSize )
        {
            *(--sLowFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);

        for( sSlot  = aIterator->slot - 1;
             sSlot >= sLowFence;
             sSlot-- )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if( sCanReusableRollback == ID_FALSE ) 
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );
            
            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );
            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, *sSlot )
                        == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                        *(--aIterator->lowFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;
                        
                        aIterator->least  = ID_TRUE;
                        *(--aIterator->lowFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }

        goto restart;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    aIterator->slot           = aIterator->lowFence - 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow                     = NULL;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
    
}                                                                             

IDE_RC smnnSeq::fetchNextU( smnnIterator* aIterator,
                            const void**  aRow )
{
    
    smpPageListEntry* sFixed;
    vULong            sSize;
    idBool            sLocked = ID_FALSE;
    SChar*            sRowPtr;
    SChar**           sSlot;
    SChar**           sHighFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
#ifdef DEBUG  
    smTID             sTransID = aIterator->tid; 
#endif
    smxTrans        * sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

restart:
    
    for( aIterator->slot++;
         aIterator->slot <= aIterator->highFence;
         aIterator->slot++ )
    {
        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;
        
        smnManager::updatedRow( (smiIterator*)aIterator );
        *aRow = aIterator->curRecPtr;
        SC_MAKE_GRID( aIterator->mRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );


        if( SM_SCN_IS_NOT_DELETED(((smpSlotHeader*)(*aRow))->mCreateSCN) )
        {
            return IDE_SUCCESS;
        }
    }

    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed);
    sSize     = sFixed->mSlotSize;
    
    if( aIterator->highest == ID_FALSE )
    {
        /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
         * 조회하는 기능으로 Page를 가져옴 */
        if( smuProperty::getCrashTolerance() == 0 )
        {
            IDE_TEST( moveNext( aIterator ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( moveNextUsingFLI( aIterator ) != IDE_SUCCESS );
        }

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 next page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );
        
        aIterator->slot = aIterator->highFence = ( aIterator->lowFence = aIterator->rows ) - 1;
        aIterator->least = ID_FALSE;
   
        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG  
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        {
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;
        
        IDE_ASSERT( smmManager::getOIDPtr( 
                          aIterator->table->mSpaceID, 
                          SM_MAKE_OID( aIterator->page, 
                                       SMP_PERS_PAGE_BODY_OFFSET ),
                          (void**)&sRowPtr )
            == IDE_SUCCESS );

        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sHighFence = aIterator->highFence,
             sFence     = sRowPtr + sFixed->mSlotCount * sSize;
             sRowPtr    < sFence;
             sRowPtr   += sSize )
        {
            *(++sHighFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);
        
        for( sSlot  = aIterator->slot + 1;
             sSlot <= sHighFence;
             sSlot++ )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if ( sCanReusableRollback == ID_FALSE ) 
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );
            
            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );
            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, *sSlot )
                        == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                        *(++aIterator->highFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;
                        
                        aIterator->highest  = ID_TRUE;
                        *(++aIterator->highFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }

        goto restart;
    }
    
    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    aIterator->slot           = aIterator->highFence + 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow           = NULL;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
    
}

IDE_RC smnnSeq::fetchPrevU( smnnIterator* aIterator,
                            const void**  aRow )
{
    
    smpPageListEntry* sFixed;
    vULong            sSize;
    idBool            sLocked = ID_FALSE;
    SChar*            sRowPtr;
    SChar**           sSlot;
    SChar**           sLowFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
#ifdef DEBUG      
    smTID             sTransID = aIterator->tid;
#endif
    smxTrans        * sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

restart:
    
    for( aIterator->slot--;
         aIterator->slot >= aIterator->lowFence;
         aIterator->slot-- )
    {
        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;
        
        smnManager::updatedRow( (smiIterator*)aIterator );
        *aRow = aIterator->curRecPtr;
        SC_MAKE_GRID( aIterator->mRowGRID,
                      aIterator->table->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );


        if( SM_SCN_IS_NOT_DELETED(((smpSlotHeader*)(*aRow))->mCreateSCN) )
        {
            return IDE_SUCCESS;
        }
    }

    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed);
    sSize     = sFixed->mSlotSize;
    
    if( aIterator->least == ID_FALSE )
    {
        /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
         * 조회하는 기능으로 Page를 가져옴 */
        if( smuProperty::getCrashTolerance() == 0 )
        {
            IDE_TEST( movePrev( aIterator ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( movePrevUsingFLI( aIterator ) != IDE_SUCCESS );
        }

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 previous page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );
        
        aIterator->highFence =
            ( aIterator->slot = aIterator->lowFence = aIterator->rows + SMNN_ROWS_CACHE ) - 1;
        aIterator->highest = ID_TRUE;

        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG      
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        {
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;
        
        IDE_ASSERT( smmManager::getOIDPtr( 
                          aIterator->table->mSpaceID, 
                          SM_MAKE_OID( aIterator->page, 
                                       SMP_PERS_PAGE_BODY_OFFSET ),
                          (void**)&sFence )
            == IDE_SUCCESS );

        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sLowFence = aIterator->lowFence,
             sRowPtr   = sFence + ( sFixed->mSlotCount - 1 ) * sSize;
             sRowPtr  >= sFence;
             sRowPtr  -= sSize )
        {
            *(--sLowFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );
        
        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);
       
        for( sSlot  = aIterator->slot - 1;
             sSlot >= sLowFence;
             sSlot-- )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if( sCanReusableRollback == ID_FALSE ) 
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );
            
            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );
            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, *sSlot )
                        == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                        *(--aIterator->lowFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;
                        
                        aIterator->least  = ID_TRUE;
                        *(--aIterator->lowFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }

        goto restart;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    aIterator->slot           = aIterator->lowFence - 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow           = NULL;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
    
}                                                                             

IDE_RC smnnSeq::fetchNextR( smnnIterator* aIterator)
{
    
    smpPageListEntry* sFixed;
    vULong            sSize;
    idBool            sLocked = ID_FALSE;
    SChar*            sRowPtr;
    SChar**           sSlot;
    SChar**           sHighFence;
    SChar*            sFence;
    smpSlotHeader*    sRow;
    scGRID            sRowGRID;
    idBool            sResult;
#ifdef DEBUG
    smTID             sTransID = aIterator->tid;
#endif
    smxTrans        * sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed);
    sSize     = sFixed->mSlotSize;
    
restart:
    
    for( aIterator->slot++;
         aIterator->slot <= aIterator->highFence;
         aIterator->slot++ )
    {
        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        IDE_TEST( smnManager::lockRow( (smiIterator*)aIterator)
                  != IDE_SUCCESS );
    }

    if( aIterator->highest == ID_FALSE )
    {
        IDU_FIT_POINT("smnnSeq::fetchNextR::wait1");

        /* __CRASH_TOLERANCE Property가 켜져 있으면, Tablespace를 FLI를 이용해
         * 조회하는 기능으로 Page를 가져옴 */
        if( smuProperty::getCrashTolerance() == 0 )
        {
            IDE_TEST( moveNext( aIterator ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( moveNextUsingFLI( aIterator ) != IDE_SUCCESS );
        }

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 next page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );

        aIterator->slot = aIterator->highFence = ( aIterator->lowFence = aIterator->rows ) - 1;
        aIterator->least = ID_FALSE;

        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < smpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > smpManager::getLastScanPageID(sFixed)  ) )
        {
            /* Scan Page List에 들어온적이 없는 page일 경우 */
            if( aIterator->mModifySeqForScan == 0 )
            {
                ideLog::log(IDE_SM_0,
                            SM_TRC_MINDEX_INDEX_INFO,
                            sTransID,
                            __FILE__,
                            __LINE__,
                            aIterator->page,
                            smpManager::getFirstScanPageID(sFixed),
                            smpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( smmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;
        
        IDE_ASSERT( smmManager::getOIDPtr( 
                          aIterator->table->mSpaceID, 
                          SM_MAKE_OID( aIterator->page, 
                                       SMP_PERS_PAGE_BODY_OFFSET ),
                          (void**)&sRowPtr )
                    == IDE_SUCCESS );

        /* BUG-48353
           checkSCN() 에서 Pending Wait 를 발생할수있기 때문에
           PAGE release 이후로 checkSCN()을 미룬다.
           여기서는 row pointer 모두를 캐쉬로 복사한다. */
        for( sHighFence = aIterator->highFence,
             sFence     = sRowPtr + sFixed->mSlotCount * sSize;
             sRowPtr    < sFence;
             sRowPtr   += sSize )
        {
            *(++sHighFence) = sRowPtr;
        }

        sLocked = ID_FALSE;
        IDE_TEST( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                               aIterator->page)
                  != IDE_SUCCESS );

        IDE_TEST(iduCheckSessionEvent(aIterator->mProperties->mStatistics) != IDE_SUCCESS);
        
        for( sSlot  = aIterator->slot + 1;
             sSlot <= sHighFence;
             sSlot++ )
        {
            sRow = (smpSlotHeader *)(*sSlot);

            IDE_TEST( smnManager::checkSCN( (smiIterator *)aIterator,
                                            sRow,
                                            &sCanReusableRollback,
                                            &sIsVisible,
                                            ID_TRUE )
                      != IDE_SUCCESS );

            if ( sIsVisible == ID_FALSE )
            {
                continue;
            }

            aIterator->mScanBackPID = aIterator->page;
            // BUG-30538 ScanBackModifySeq 를 잘못 기록하고 있습니다.
            aIterator->mScanBackModifySeq = aIterator->mModifySeqForScan;

            if( sCanReusableRollback == ID_FALSE ) 
            {
                sReusableCheckAgain = ID_TRUE;
            }

            SC_MAKE_GRID( sRowGRID,
                          aIterator->table->mSpaceID,
                          SMP_SLOT_GET_PID( sRow ),
                          SMP_SLOT_GET_OFFSET( sRow ) );
            
            IDE_TEST( aIterator->filter->callback( &sResult,
                                                   *sSlot,
                                                   NULL,
                                                   0,
                                                   sRowGRID,
                                                   aIterator->filter->data )
                      != IDE_SUCCESS );
            if( sResult == ID_TRUE )
            {
                /* SCN 조건을 통과한 slot중 하나라도 cursor reusable 체크가 필요할 경우
                 * filter 조건을 통과한 slot을 대상으로 다시 확인해야 한다. */
                if( ( sReusableCheckAgain == ID_TRUE ) && ( sTrans->mIsReusableRollback == ID_TRUE ) )
                {
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, *sSlot )
                        == ID_FALSE )
                    {
                        sTrans->mIsReusableRollback = ID_FALSE;  
                    }
                }

                if(aIterator->mProperties->mFirstReadRecordPos == 0)
                {
                    if(aIterator->mProperties->mReadRecordCount != 1)
                    {
                        aIterator->mProperties->mReadRecordCount--;
                        *(++aIterator->highFence) = *sSlot;
                    }
                    else 
                    {
                        IDE_ASSERT(aIterator->mProperties->mReadRecordCount != 0);
                        aIterator->mProperties->mReadRecordCount--;
                        
                        aIterator->highest  = ID_TRUE;
                        *(++aIterator->highFence) = *sSlot;
                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }
            }
        }

        goto restart;
    }
    
    IDE_EXCEPTION_CONT( RETURN_SUCCESS );
    
    aIterator->slot           = aIterator->highFence + 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    
    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }
    
    return IDE_FAILURE;
    
}


IDE_RC smnnSeq::allocIterator( void ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}


IDE_RC smnnSeq::freeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnnGatherStat                             *
 * ------------------------------------------------------------------*
 * 통계정보를 구축한다.
 * 
 * ScanPageList대신 AllocPageList를 사용한다.
 * 1) AllocList는 ALTER ... COMPACT 등으로 줄기 때문에, DanglingReference
 *    문제가 생기지 않는다.
 * 2) AllocList는 Record가 하나도 없어도 조회하기 때문에 통계획득에
 *    적합하다.
 *
 * Statistics    - [IN]  IDLayer 통계정보
 * Trans         - [IN]  이 작업을 요청한 Transaction
 * Percentage    - [IN]  Sampling Percentage
 * Degree        - [IN]  Parallel Degree
 * Header        - [IN]  대상 TableHeader
 * Index         - [IN]  대상 index (FullScan이기에 무시됨 )
 *********************************************************************/
IDE_RC smnnSeq::gatherStat( idvSQL         * aStatistics,
                            void           * aTrans,
                            SFloat           aPercentage,
                            SInt             /* aDegree */,
                            smcTableHeader * aHeader,
                            void           * aTotalTableArg,
                            smnIndexHeader * /*aIndex*/,
                            void           * aStats,
                            idBool           aDynamicMode )
{
    void          * sTableArgument;
    UInt            sState = 0;

    IDE_TEST( smLayerCallback::beginTableStat( aHeader,
                                               aPercentage,
                                               aDynamicMode,
                                               &sTableArgument )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( gatherStat4Fixed( aStatistics,
                                sTableArgument,
                                aPercentage,
                                aHeader,
                                aTotalTableArg )
              != IDE_SUCCESS );

    IDE_TEST( gatherStat4Var( aStatistics,
                              sTableArgument,
                              aPercentage,
                              aHeader )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::setTableStat( aHeader,
                                             aTrans,
                                             sTableArgument,
                                             (smiAllStat*)aStats,
                                             aDynamicMode )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( smLayerCallback::endTableStat( aHeader,
                                             sTableArgument,
                                             aDynamicMode )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) smLayerCallback::endTableStat( aHeader,
                                              sTableArgument,
                                              aDynamicMode );
    }

    return IDE_FAILURE;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : smnnGatherStat4Fixed                       *
 * ------------------------------------------------------------------*
 * FixedPage에 대해 통계정보를 구축한다.
 * 
 * aStatistics    - [IN]  IDLayer 통계정보
 * aTableArgument - [IN]  분석 결과를 저장해두는곳
 * Percentage     - [IN]  Sampling Percentage
 * Header         - [IN]  대상 TableHeader
 *********************************************************************/
IDE_RC smnnSeq::gatherStat4Fixed( idvSQL         * aStatistics,
                             void           * aTableArgument,
                             SFloat           aPercentage,
                             smcTableHeader * aHeader,
                             void           * aTotalTableArg )
{
    scPageID        sPageID = SM_NULL_PID;
    SChar         * sRow    = NULL;
    smpSlotHeader * sSlotHeader;
    UInt            sSlotSize;
    UInt            sMetaSpace   = 0;
    UInt            sUsedSpace   = 0;
    UInt            sAgableSpace = 0;
    UInt            sFreeSpace   = 0;
    UInt            sState = 0;
    UInt            i;

    sSlotSize = ((smpPageListEntry*)&(aHeader->mFixed))->mSlotSize;
    sPageID   = smpManager::getFirstAllocPageID( &(aHeader->mFixed.mMRDB) );
    while( sPageID != SM_NULL_PID )
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
        if(  ( (UInt)( aPercentage * sPageID
                       + SMI_STAT_SAMPLING_INITVAL ) ) !=
             ( (UInt)( aPercentage * (sPageID+1)
                       + SMI_STAT_SAMPLING_INITVAL ) ) )
        {
            IDE_TEST( smmManager::holdPageSLatch( aHeader->mSpaceID, sPageID )
                      != IDE_SUCCESS );
            sState = 1;

            sMetaSpace   = SM_PAGE_SIZE 
                - ( aHeader->mFixed.mMRDB.mSlotCount * sSlotSize );
            sUsedSpace   = 0;
            sAgableSpace = 0;
            sFreeSpace   = 0;

            IDE_TEST( smmManager::getOIDPtr( 
                                aHeader->mSpaceID,
                                SM_MAKE_OID( sPageID, SMP_PERS_PAGE_BODY_OFFSET ),
                                (void**)&sRow )
                      != IDE_SUCCESS );

            for( i = 0 ; i < aHeader->mFixed.mMRDB.mSlotCount ; i ++ )
            {
                sSlotHeader = (smpSlotHeader*)sRow;

                if( ( SM_SCN_IS_DELETED( sSlotHeader->mCreateSCN ) == ID_TRUE ) ||
                    ( SM_SCN_IS_NULL_ROW( sSlotHeader->mCreateSCN ) ) )
                {
                    sFreeSpace += sSlotSize;
                }
                else
                {
                    if( SM_SCN_IS_FREE_ROW( sSlotHeader->mLimitSCN ) ||
                        SM_SCN_IS_LOCK_ROW( sSlotHeader->mLimitSCN ) )
                    {
                        /* Update/Delete안되고 Lock정도만 걸린
                         * (이제 막 Update/삽입된 Row 포함) */
                        IDE_TEST( smLayerCallback::analyzeRow4Stat( aHeader,
                                                                    aTableArgument,
                                                                    aTotalTableArg,
                                                                    (UChar*)sRow )
                                  != IDE_SUCCESS );
                        sUsedSpace += sSlotSize;
                    }
                    else
                    {
                        sAgableSpace += sSlotSize;
                    }
                }

                sRow += sSlotSize;
            }


            IDE_ERROR( sMetaSpace   <= SM_PAGE_SIZE );
            IDE_ERROR( sUsedSpace   <= SM_PAGE_SIZE );
            IDE_ERROR( sAgableSpace <= SM_PAGE_SIZE );
            IDE_ERROR( sFreeSpace   <= SM_PAGE_SIZE );
            IDE_ERROR( ( sMetaSpace + sUsedSpace + sAgableSpace + sFreeSpace ) ==
                       SM_PAGE_SIZE );

            IDE_TEST( smLayerCallback::updateSpaceUsage( aTableArgument,
                                                         sMetaSpace,
                                                         sUsedSpace,
                                                         sAgableSpace,
                                                         sFreeSpace )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( smmManager::releasePageLatch( aHeader->mSpaceID, 
                                                    sPageID )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Skip함 */
        }

        sPageID = smpManager::getNextAllocPageID( aHeader->mSpaceID,
                                                  &(aHeader->mFixed.mMRDB),
                                                  sPageID );

        IDE_TEST( iduCheckSessionEvent( aStatistics )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) smmManager::releasePageLatch( aHeader->mSpaceID, sPageID );
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : smnnGatherStat4Var                         *
 * ------------------------------------------------------------------*
 * VariablePage에 대해 통계정보를 구축한다.
 * 
 * aStatistics    - [IN]  IDLayer 통계정보
 * aTableArgument - [IN]  분석 결과를 저장해두는곳
 * Percentage     - [IN]  Sampling Percentage
 * Header         - [IN]  대상 TableHeader
 *********************************************************************/
IDE_RC smnnSeq::gatherStat4Var( idvSQL         * aStatistics,
                                void           * aTableArgument,
                                SFloat           aPercentage,
                                smcTableHeader * aHeader )
{
    scPageID          sPageID = SM_NULL_PID;
    smpPersPage     * sPage;
    UInt              sSlotSize;
    UInt              sMetaSpace   = 0;
    UInt              sUsedSpace   = 0;
    UInt              sAgableSpace = 0;
    UInt              sFreeSpace   = 0;
    UInt              sIdx = 0;
    smVCPieceHeader * sCurVCPieceHeader;
    UInt              sState = 0;
    UInt              i;

    sPageID = smpManager::getFirstAllocPageID( aHeader->mVar.mMRDB );
    while( sPageID != SM_NULL_PID )
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
        if(  ( (UInt)( aPercentage * sPageID
                       + SMI_STAT_SAMPLING_INITVAL ) ) !=
             ( (UInt)( aPercentage * (sPageID+1)
                       + SMI_STAT_SAMPLING_INITVAL ) ) )
        {
            IDE_TEST( smmManager::holdPageSLatch( aHeader->mSpaceID, sPageID )
                      != IDE_SUCCESS );
            sState = 1;

            IDE_TEST( smmManager::getPersPagePtr( aHeader->mSpaceID,
                                                  sPageID,
                                                  (void**)&sPage )
                      != IDE_SUCCESS );

            sIdx      = smpVarPageList::getVarIdx(sPage);
            sSlotSize = aHeader->mVar.mMRDB[sIdx].mSlotSize;

            sMetaSpace   = SM_PAGE_SIZE 
                - aHeader->mVar.mMRDB[sIdx].mSlotCount * sSlotSize;
            sUsedSpace   = 0;
            sAgableSpace = 0;
            sFreeSpace   = 0;

            for( i = 0 ; i < aHeader->mVar.mMRDB[sIdx].mSlotCount ; i ++ )
            {
                sCurVCPieceHeader = (smVCPieceHeader*)
                    &sPage->mBody[ ID_SIZEOF(smpVarPageHeader) 
                                      + i * sSlotSize ];

                if((sCurVCPieceHeader->flag & SM_VCPIECE_FREE_MASK)
                   == SM_VCPIECE_FREE_OK)
                {
                    sFreeSpace += sSlotSize;
                }
                else
                {
                    sUsedSpace += sSlotSize;
                }
            }

            IDE_ERROR( sMetaSpace   <= SM_PAGE_SIZE );
            IDE_ERROR( sUsedSpace   <= SM_PAGE_SIZE );
            IDE_ERROR( sAgableSpace <= SM_PAGE_SIZE );
            IDE_ERROR( sFreeSpace   <= SM_PAGE_SIZE );
            IDE_ERROR( ( sMetaSpace + sUsedSpace + sAgableSpace + sFreeSpace ) ==
                       SM_PAGE_SIZE );

            IDE_TEST( smLayerCallback::updateSpaceUsage( aTableArgument,
                                                         sMetaSpace,
                                                         sUsedSpace,
                                                         sAgableSpace,
                                                         sFreeSpace )
                      != IDE_SUCCESS );

            sState = 0;
            IDE_TEST( smmManager::releasePageLatch( aHeader->mSpaceID, 
                                                    sPageID )
                      != IDE_SUCCESS );
        }
        else
        {
            /* Skip함 */
        }

        sPageID = smpManager::getNextAllocPageID( aHeader->mSpaceID,
                                                  aHeader->mVar.mMRDB,
                                                  sPageID );

        IDE_TEST( iduCheckSessionEvent( aStatistics )
                  != IDE_SUCCESS );

    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        (void) smmManager::releasePageLatch( aHeader->mSpaceID, sPageID );
    }

    return IDE_FAILURE;
}



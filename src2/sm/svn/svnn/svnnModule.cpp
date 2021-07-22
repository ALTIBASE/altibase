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
 
#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <smc.h>
#include <svm.h>
#include <svp.h>
#include <svcRecord.h>
#include <smErrorCode.h>
#include <smnModules.h>
#include <svnReq.h>
#include <svnnDef.h>
#include <svnModules.h>
#include <smnManager.h>

static IDE_RC svnnPrepareIteratorMem( const smnIndexModule* );

static IDE_RC svnnReleaseIteratorMem(const smnIndexModule* );

static IDE_RC svnnInit( svnnIterator      * aIterator,
                        void              * aTrans,
                        smcTableHeader    * aTable,
                        smnIndexHeader    * /* aIndex */,
                        void              * /* aDumpObject */,
                        const smiRange    * /* aKeyRange */,
                        const smiRange    * /* aKeyFilter */,
                        const smiCallBack * aRowFilter,
                        UInt                aFlag,
                        smSCN               aSCN,
                        smSCN               aInfinite,
                        idBool              /* aUntouchable */,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc,
                        smiStatement        * aStatement );

static IDE_RC svnnDest( svnnIterator*       aIterator );

static IDE_RC svnnNA           ( void );
static IDE_RC svnnAA           ( svnnIterator* aIterator,
                                 const void**  aRow );
static IDE_RC svnnBeforeFirst  ( svnnIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc );
static IDE_RC svnnAfterLast    ( svnnIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc );
static IDE_RC svnnBeforeFirstU ( svnnIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc );
static IDE_RC svnnAfterLastU   ( svnnIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc );
static IDE_RC svnnBeforeFirstR ( svnnIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc );
static IDE_RC svnnAfterLastR   ( svnnIterator*       aIterator,
                                 const smSeekFunc**  aSeekFunc );

static IDE_RC svnnMoveNextBlock( svnnIterator*       aIterator );

static IDE_RC svnnMoveNextNonBlock( svnnIterator*       aIterator );

static IDE_RC svnnMoveNext     ( svnnIterator*       aIterator );
static IDE_RC svnnMoveNextParallelBlock ( svnnIterator  * aIterator ); // parasoft-suppress ALTI-FOR-017 "Don't Care"

static IDE_RC svnnMoveNextParallelNonBlock ( svnnIterator  * aIterator );

static IDE_RC svnnMoveNextParallel ( svnnIterator  * aIterator );


static IDE_RC svnnMovePrevBlock( svnnIterator*       aIterator );
static IDE_RC svnnMovePrevNonBlock( svnnIterator*       aIterator );
static IDE_RC svnnMovePrev     ( svnnIterator*       aIterator );
static IDE_RC svnnFetchNext    ( svnnIterator*       aIterator,
                                 const void**        aRow      );
static IDE_RC svnnFetchPrev    ( svnnIterator*       aIterator,
                                 const void**        aRow      );
static IDE_RC svnnFetchNextU   ( svnnIterator*       aIterator,
                                 const void**        aRow      );
static IDE_RC svnnFetchPrevU   ( svnnIterator*       aIterator,
                                 const void**        aRow      );
static IDE_RC svnnFetchNextR   ( svnnIterator*       aIterator );

static IDE_RC svnnAllocIterator( void ** aIteratorMem );

static IDE_RC svnnFreeIterator ( void * aIteratorMem );

static IDE_RC svnnGatherStat( idvSQL         * aStatistics,
                              void           * aTrans,
                              SFloat           aPercentage,
                              SInt             aDegree,
                              smcTableHeader * aHeader,
                              void           * aTotalTableArg,   
                              smnIndexHeader * aIndex,
                              void           * aStats,
                              idBool           aDynamicMode );

smnIndexModule svnnModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_VOLATILE,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( svnnIterator ),
    (smnMemoryFunc) svnnPrepareIteratorMem,
    (smnMemoryFunc) svnnReleaseIteratorMem,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,

    (smTableCursorLockRowFunc) smnManager::lockRow,
    (smnDeleteFunc)            NULL,
    (smnFreeFunc)              NULL,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             NULL,

    (smInitFunc)    svnnInit,
    (smDestFunc)    svnnDest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc) NULL,
    (smnSetPositionFunc) NULL,

    (smnAllocIteratorFunc) svnnAllocIterator,
    (smnFreeIteratorFunc)  svnnFreeIterator,
    (smnReInitFunc)        NULL,
    (smnInitMetaFunc)      NULL,

    (smnMakeDiskImageFunc)  NULL,

    (smnBuildIndexFunc)     NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         svnnGatherStat
    /*BUGBUG 나중에 smnnGatherStat 으로 바꿔야 함 */
};

static const smSeekFunc svnnSeekFunctions[32][12] =
{ { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirstU,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATALBE      */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirstR,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirstU,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)svnnFetchNextU,
        (smSeekFunc)svnnFetchPrevU,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATALBE      */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnBeforeFirstR,
        (smSeekFunc)svnnAfterLastR,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)svnnFetchNextU,
        (smSeekFunc)svnnFetchPrevU,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLastU,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLastR,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLastU,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)svnnFetchPrevU,
        (smSeekFunc)svnnFetchNextU,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAfterLastR,
        (smSeekFunc)svnnBeforeFirstR,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)svnnFetchPrev,
        (smSeekFunc)svnnFetchNext,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)svnnFetchPrevU,
        (smSeekFunc)svnnFetchNextU,
        (smSeekFunc)svnnAfterLast,
        (smSeekFunc)svnnBeforeFirst,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnAA,
        (smSeekFunc)svnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA,
        (smSeekFunc)svnnNA
    }
};


static IDE_RC svnnPrepareIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC svnnReleaseIteratorMem(const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC svnnInit( svnnIterator      * aIterator,
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
                        idBool              ,
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
    aIterator->tid                = smLayerCallback::getTransID(aTrans);
    aIterator->filter             = aFilter;
    aIterator->mProperties        = aProperties;
    aIterator->mScanBackPID       = SM_NULL_PID;
    aIterator->mScanBackModifySeq = 0;
    aIterator->mModifySeqForScan  = 0;
    aIterator->page               = SM_NULL_PID;
    aIterator->mStatement         = aStatement;

    *aSeekFunc = svnnSeekFunctions[aFlag&(SMI_TRAVERSE_MASK|
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

static IDE_RC svnnDest( svnnIterator* /*aIterator*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC svnnAA( svnnIterator*,
                      const void** )
{

    return IDE_SUCCESS;

}

IDE_RC svnnNA( void )
{

    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;

}

static IDE_RC svnnBeforeFirst( svnnIterator* aIterator,
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

    sFixed                     = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    aIterator->curRecPtr       = NULL;
    aIterator->lstFetchRecPtr  = NULL;
    aIterator->slot            =
    aIterator->highFence       = ( aIterator->lowFence = aIterator->rows ) - 1;
    aIterator->page            = SM_NULL_PID;
    aIterator->least           =
    aIterator->highest         = ID_TRUE;
    sSize                      = sFixed->mSlotSize;
#ifdef DEBUG
    sTransID                   = aIterator->tid;
#endif
    if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt == 1 )
    {
        IDE_TEST( svnnMoveNext( aIterator ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( svnnMoveNextParallel( aIterator ) != IDE_SUCCESS );
    }

    if ( ( aIterator->page != SM_NULL_PID ) &&
         ( aIterator->page != SM_SPECIAL_PID ) &&
         ( aIterator->mProperties->mReadRecordCount > 0 ) )
    {
        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;

        IDE_ASSERT( smmManager::getPersPagePtr( 
                        aIterator->table->mSpaceID, 
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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

            if ( sIsVisible  == ID_FALSE )
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
    }
    else
    {
        // nothing to do
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

static IDE_RC svnnAfterLast( svnnIterator* aIterator,
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
    smxTrans*         sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

    sFixed                     = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    aIterator->curRecPtr       = NULL;
    aIterator->lstFetchRecPtr  = NULL;
    aIterator->highFence       =
      ( aIterator->slot        =
        aIterator->lowFence    = aIterator->rows + SVNN_ROWS_CACHE ) - 1;
    aIterator->page            = SM_NULL_PID;
    aIterator->least           =
    aIterator->highest         = ID_TRUE;
    sSize                      = sFixed->mSlotSize;
#ifdef DEBUG
    sTransID                   = aIterator->tid;
#endif

    IDE_TEST( svnnMovePrev( aIterator ) != IDE_SUCCESS );

    if( aIterator->page != SM_NULL_PID && aIterator->mProperties->mReadRecordCount > 0)
    {
        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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

            if ( sIsVisible  == ID_FALSE )
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
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

static IDE_RC svnnBeforeFirstU( svnnIterator*       aIterator,
                                const smSeekFunc** aSeekFunc )
{

    IDE_TEST( svnnBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

static IDE_RC svnnAfterLastU( svnnIterator*       aIterator,
                              const smSeekFunc** aSeekFunc )
{

    IDE_TEST( svnnAfterLast( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

static IDE_RC svnnBeforeFirstR( svnnIterator*       aIterator,
                                const smSeekFunc** aSeekFunc )
{

    svnnIterator sIterator;
    UInt         sRowCount;

    IDE_TEST( svnnBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    sIterator.curRecPtr          = aIterator->curRecPtr;
    sIterator.lstFetchRecPtr     = aIterator->lstFetchRecPtr;
    sIterator.least              = aIterator->least;
    sIterator.highest            = aIterator->highest;
    sIterator.page               = aIterator->page;
    sIterator.mScanBackPID       = aIterator->mScanBackPID;
    sIterator.mScanBackModifySeq = aIterator->mScanBackModifySeq;
    sIterator.mModifySeqForScan  = aIterator->mModifySeqForScan;
    sIterator.slot               = aIterator->slot;
    sIterator.lowFence           = aIterator->lowFence;
    sIterator.highFence          = aIterator->highFence;
    sRowCount                    = aIterator->highFence - aIterator->rows + 1;

    //BUG-27574 klocwork SM
    IDE_ASSERT( sRowCount <= SVNN_ROWS_CACHE );

    idlOS::memcpy( sIterator.rows, aIterator->rows, (size_t)ID_SIZEOF(SChar*) * sRowCount );

    IDE_TEST( svnnFetchNextR( aIterator ) != IDE_SUCCESS );

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

    IDE_ASSERT(SVNN_ROWS_CACHE >= sRowCount);

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

static IDE_RC svnnAfterLastR( svnnIterator*       aIterator,
                              const smSeekFunc** aSeekFunc )
{

    IDE_TEST( svnnBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    IDE_TEST( svnnFetchNextR( aIterator ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

static IDE_RC svnnMoveNext( svnnIterator   * aIterator )
{
    if( smuProperty::getScanlistMoveNonBlock() == ID_TRUE )
    {
        return svnnMoveNextNonBlock( aIterator );
    }
    else
    {
        return svnnMoveNextBlock( aIterator );
    }
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

static IDE_RC svnnMoveNextBlock( svnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sCurScanModifySeq = 0;
    ULong                  sNxtScanModifySeq = 0;
    smpScanPageListEntry * sScanPageList;
    scSpaceID              sSpaceID;
    scPageID               sNextPID = SM_NULL_PID;

    IDE_ASSERT( aIterator != NULL );

    // BUG-27331 CodeSonar::Uninitialized Variable

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
            aIterator->page = svpManager::getFirstScanPageID( sFixed );

            if( aIterator->page == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->mModifySeqForScan = sNxtScanModifySeq = sCurScanModifySeq;

            sNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                      aIterator->page );

        }
        else
        {
            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->page = svpManager::getNextScanPageID( sSpaceID,
                                                             aIterator->page );

            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if( (aIterator->page != SM_SPECIAL_PID) &&
                (aIterator->page != SM_NULL_PID) )
            {
                sNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                          aIterator->page );
                sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
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
        // Next PageID를 기반으로 Current page가 마지막 page인지 확인한다.
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
 *     svnnMoveNextBlock() 함수와 동일한 역할을 한다,
 *     sScanPageList->mMutex 를 잡지 않도록 수정되었다.
 *     동작 원리는 smnnSeq::moveNextNonBlock() 주석을 참고하라
 *
 * aIterator    - [IN/OUT]  Iterator
 *
 ****************************************************************************/
static IDE_RC svnnMoveNextNonBlock( svnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sNxtScanModifySeq = 0;
    ULong                  sCurScanModifySeq = 0;
    smpScanPageListEntry * sScanPageList;
    scSpaceID              sSpaceID;
    scPageID               sNextPID;
    scPageID               sCurrPID;
    scPageID               sNextNextPID = SM_NULL_PID;

    IDE_ASSERT( aIterator != NULL );

    // BUG-27331 CodeSonar::Uninitialized Variable

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

            sNextPID = svpManager::getFirstScanPageID( sFixed );

            if( sNextPID == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 sNextPID );

            sNextNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                          sNextPID );

            IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

            aIterator->mModifySeqForScan = sNxtScanModifySeq;
            aIterator->page              = sNextPID;

            break;
        }
        else
        {
            sCurrPID = aIterator->page;

            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 sCurrPID );

            if( sCurScanModifySeq != aIterator->mModifySeqForScan )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;

                continue;
            }

            IDE_ASSERT( SVM_PAGE_IS_MODIFYING( sCurScanModifySeq ) == ID_FALSE );

            sNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                      sCurrPID );

            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if( ( sNextPID != SM_SPECIAL_PID) &&
                ( sNextPID != SM_NULL_PID) )
            {
                sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                     sNextPID );

                sNextNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                              sNextPID );

                if(( SVM_PAGE_IS_MODIFYING( sNxtScanModifySeq ) == ID_TRUE ) ||
                   ( svpManager::getNextScanPageID( sSpaceID,
                                                    sCurrPID ) != sNextPID ) ||
                   ( svpManager::getModifySeqForScan( sSpaceID,
                                                      sNextPID ) != sNxtScanModifySeq ))
                {
                    continue;
                }
            }

            if( svpManager::getModifySeqForScan( sSpaceID,
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
        // Next PageID를 기반으로 Current page가 마지막 page인지 확인한다.
        // BUG-43463 Next Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->highest = ( SM_SPECIAL_PID == sNextNextPID ) ? ID_TRUE : ID_FALSE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC svnnMoveNextParallel( svnnIterator * aIterator )
{
    if( smuProperty::getScanlistMoveNonBlock() == ID_TRUE )
    {
        return svnnMoveNextParallelNonBlock( aIterator );
    }
    else
    {
        return svnnMoveNextParallelBlock( aIterator );
    }
}

static IDE_RC svnnMoveNextParallelBlock( svnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sCurScanModifySeq = 0;
    ULong                  sNxtScanModifySeq = 0;
    scSpaceID              sSpaceID;
    scPageID               sNextPID = SM_NULL_PID;
    smpScanPageListEntry * sScanPageList;
    ULong                  sThreadCnt;
    ULong                  sThreadID;

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
            aIterator->page = svpManager::getFirstScanPageID( sFixed );

            if ( aIterator->page == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }
            else
            {
                // nothing to do
            }

            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->mModifySeqForScan = sNxtScanModifySeq = sCurScanModifySeq;

            sNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                      aIterator->page );
        }
        else
        {
            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->page = svpManager::getNextScanPageID( sSpaceID,
                                                             aIterator->page );

            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if ( ( aIterator->page != SM_SPECIAL_PID ) &&
                 ( aIterator->page != SM_NULL_PID) )
            {
                IDE_ASSERT( (aIterator->page   != SM_NULL_PID) ||
                            (sCurScanModifySeq != aIterator->mModifySeqForScan) );

                sNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                          aIterator->page );

                sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
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
                    // nothing ti do
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
        // Next PageID를 기반으로 Current page가 마지막 page인지 확인한다.
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
 *     svnnMoveNextParallelBlock() 함수와 동일한 역할을 한다,
 *     sScanPageList->mMutex 를 잡지 않도록 수정되었다.
 *     동작 원리는 smnnSeq::moveNextNonBlock() 주석을 참고하라
 *
 * aIterator    - [IN/OUT]  Iterator
 *
 ****************************************************************************/
static IDE_RC svnnMoveNextParallelNonBlock( svnnIterator * aIterator )
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

            sNextPID = svpManager::getFirstScanPageID( sFixed );

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

            sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 sNextPID );

            sNextNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                          sNextPID );

            IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

            aIterator->page              = sNextPID;
            aIterator->mModifySeqForScan = sNxtScanModifySeq;

            if ( ( aIterator->page % sThreadCnt + 1 ) != sThreadID )
            {
                continue;
            }

            break;

        }
        else
        {
            sCurrPID = aIterator->page;

            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 sCurrPID );

            if( sCurScanModifySeq != aIterator->mModifySeqForScan )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;

                continue;
            }

            IDE_ASSERT( SVM_PAGE_IS_MODIFYING( sCurScanModifySeq ) == ID_FALSE ) ;

            sNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                      sCurrPID );
            /*
             * 현재 Page가 Scan List상의 마지막 Page라면 더이상 Next링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Next Page를 보정한다.
             */
            if ( ( sNextPID != SM_SPECIAL_PID ) &&
                 ( sNextPID != SM_NULL_PID) )
            {
                sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                     sNextPID );

                sNextNextPID = svpManager::getNextScanPageID( sSpaceID,
                                                              sNextPID );

                if(( SVM_PAGE_IS_MODIFYING( sNxtScanModifySeq ) == ID_TRUE ) ||
                   ( svpManager::getNextScanPageID( sSpaceID,
                                                    sCurrPID ) != sNextPID ) ||
                   ( svpManager::getModifySeqForScan( sSpaceID,
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
            if( svpManager::getModifySeqForScan( sSpaceID,
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
        // Next PageID를 기반으로 Current page가 마지막 page인지 확인한다.
        // BUG-43463 Next Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->highest = ( SM_SPECIAL_PID == sNextNextPID ) ? ID_TRUE : ID_FALSE;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC svnnMovePrev( svnnIterator*       aIterator )
{
    if( smuProperty::getScanlistMoveNonBlock() == ID_TRUE )
    {
        return svnnMovePrevNonBlock( aIterator );
    }
    else
    {
        return svnnMovePrevBlock( aIterator );
    }
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

static IDE_RC svnnMovePrevBlock( svnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sCurScanModifySeq = 0;
    ULong                  sNxtScanModifySeq = 0;
    smpScanPageListEntry * sScanPageList;
    scSpaceID              sSpaceID;
    scPageID               sPrevPID = SM_NULL_PID;

    IDE_ASSERT( aIterator != NULL );

    // BUG-27331 CodeSonar::Uninitialized Variable

    sFixed   = (smpPageListEntry*)&(aIterator->table->mFixed);
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
            aIterator->page = svpManager::getLastScanPageID( sFixed );

            if( aIterator->page == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->mModifySeqForScan = sNxtScanModifySeq = sCurScanModifySeq;
        }
        else
        {
            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 aIterator->page );
            aIterator->page = svpManager::getPrevScanPageID( sSpaceID,
                                                             aIterator->page );

            /*
             * 현재 Page가 Scan List상의 처음 Page라면 더이상 Prev링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Previous Page를 보정한다.
             */
            if( (aIterator->page != SM_SPECIAL_PID) &&
                (aIterator->page != SM_NULL_PID) )
            {
                sPrevPID = svpManager::getPrevScanPageID( sSpaceID,
                                                          aIterator->page );
                sNxtScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
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
        // Prev PageID를 기반으로 Current page가 마지막 page인지 확인한다.
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
 *     svnnMovePrevBlock() 함수와 동일한 역할을 한다,
 *     sScanPageList->mMutex 를 잡지 않도록 수정되었다.
 *     동작 원리는 smnnSeq::moveNextNonBlock() 주석을 참고하라
 *
 * aIterator    - [IN/OUT]  Iterator
 *
 ****************************************************************************/
static IDE_RC svnnMovePrevNonBlock( svnnIterator * aIterator )
{
    smpPageListEntry     * sFixed;
    ULong                  sPrvScanModifySeq = 0;
    ULong                  sCurScanModifySeq = 0;
    smpScanPageListEntry * sScanPageList;
    scSpaceID              sSpaceID;
    scPageID               sPrevPrevPID = SM_NULL_PID;
    scPageID               sPrevPID;
    scPageID               sCurrPID;

    IDE_ASSERT( aIterator != NULL );

    // BUG-27331 CodeSonar::Uninitialized Variable

    sFixed   = (smpPageListEntry*)&(aIterator->table->mFixed);
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

            sPrevPID = svpManager::getLastScanPageID( sFixed );

            if( sPrevPID == SM_NULL_PID )
            {
                IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );
                IDE_CONT( RETURN_SUCCESS );
            }

            sPrvScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 sPrevPID );

            IDE_TEST( sScanPageList->mMutex->unlock() != IDE_SUCCESS );

            aIterator->mModifySeqForScan = sPrvScanModifySeq;
            aIterator->page              = sPrevPID;

            break;
        }
        else
        {
            sCurrPID = aIterator->page;

            sCurScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                 sCurrPID );

            if( sCurScanModifySeq != aIterator->mModifySeqForScan )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

                aIterator->page              = aIterator->mScanBackPID;
                aIterator->mModifySeqForScan = aIterator->mScanBackModifySeq;

                continue;
            }

            IDE_ASSERT( SVM_PAGE_IS_MODIFYING( sCurScanModifySeq ) == ID_FALSE );

            sPrevPID = svpManager::getPrevScanPageID( sSpaceID,
                                                      sCurrPID );
            /*
             * 현재 Page가 Scan List상의 처음 Page라면 더이상 Prev링크를
             * 얻을 필요가 없다. 또한, 현재 Page가 Scan List에서 제거된 상태라면
             * ScanBackPID로부터 Previous Page를 보정한다.
             */
            if( ( sPrevPID != SM_SPECIAL_PID ) &&
                ( sPrevPID != SM_NULL_PID) )
            {
                sPrvScanModifySeq = svpManager::getModifySeqForScan( sSpaceID,
                                                                     sPrevPID );

                sPrevPrevPID = svpManager::getPrevScanPageID( sSpaceID,
                                                              sPrevPID );

                if(( SVM_PAGE_IS_MODIFYING( sPrvScanModifySeq ) == ID_TRUE ) ||
                   ( svpManager::getPrevScanPageID( sSpaceID,
                                                    sCurrPID ) != sPrevPID ) ||
                   ( svpManager::getModifySeqForScan( sSpaceID,
                                                      sPrevPID ) != sPrvScanModifySeq ))
                {
                    continue;
                }
            }

            /*
             * 현재 Page가 Scan List에서 제거되었거나, 제거된 이후에 다시 Scan List에
             * 삽입된 경우라면 된 상태라면 ScanBackPID로부터 Previous Page를 보정한다.
             */
            if( svpManager::getModifySeqForScan( sSpaceID,
                                                 sCurrPID ) != sCurScanModifySeq )
            {
                IDE_DASSERT( sCurrPID != aIterator->mScanBackPID );

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
        // Prev PageID를 기반으로 Current page가 마지막 page인지 확인한다.
        // BUG-43463 Prev Page ID를 반환 하지 않고, Iterator에 바로 설정한다
        aIterator->least = ( SM_SPECIAL_PID == sPrevPrevPID ) ? ID_TRUE : ID_FALSE ;
    }

    IDE_EXCEPTION_CONT( RETURN_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static IDE_RC svnnFetchNext( svnnIterator* aIterator,
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
    smxTrans*         sTrans               = (smxTrans*)aIterator->trans;
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
        *aRow          = aIterator->curRecPtr;

        SC_MAKE_GRID( aIterator->mRowGRID,
                      ((smcTableHeader*)aIterator->table)->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        return IDE_SUCCESS;
    }

    sFixed = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    sSize  = sFixed->mSlotSize;

    if( aIterator->highest == ID_FALSE )
    {
        if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt == 1 )
        {
            IDE_TEST( svnnMoveNext( aIterator ) != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( svnnMoveNextParallel( aIterator ) != IDE_SUCCESS );
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
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }
        }
#endif
 
        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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

            if( sIsVisible == ID_FALSE )
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

    aIterator->slot = aIterator->highFence + 1;
    aIterator->curRecPtr  = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow           = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static IDE_RC svnnFetchPrev( svnnIterator* aIterator,
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
    smxTrans*         sTrans = (smxTrans*)aIterator->trans;
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
                      ((smcTableHeader*)aIterator->table)->mSpaceID,
                      SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                      SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

        return IDE_SUCCESS;
    }

    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    sSize     = sFixed->mSlotSize;

    if( aIterator->least == ID_FALSE )
    {
        IDE_TEST( svnnMovePrev( aIterator ) != IDE_SUCCESS );

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 previous page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );

        aIterator->highFence =
            ( aIterator->slot = aIterator->lowFence = aIterator->rows + SVNN_ROWS_CACHE ) - 1;
        aIterator->highest = ID_TRUE;

        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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

            if( sIsVisible == ID_FALSE )
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

    aIterator->slot = aIterator->lowFence - 1;
    aIterator->curRecPtr      = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow           = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static IDE_RC svnnFetchNextU( svnnIterator* aIterator,
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
    smxTrans*         sTrans               = (smxTrans*)aIterator->trans;
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

        smnManager::updatedVolRow( (smiIterator*)aIterator );
        *aRow = aIterator->curRecPtr;
        if( SM_SCN_IS_NOT_DELETED(((smpSlotHeader*)(*aRow))->mCreateSCN) )
        {
            SC_MAKE_GRID( aIterator->mRowGRID,
                          ((smcTableHeader*)aIterator->table)->mSpaceID,
                          SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                          SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

            return IDE_SUCCESS;
        }
    }

    sFixed = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    sSize  = sFixed->mSlotSize;

    if( aIterator->highest == ID_FALSE )
    {
        IDE_TEST( svnnMoveNext( aIterator ) != IDE_SUCCESS );

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
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }          
        }
#endif

        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    *aRow           = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static IDE_RC svnnFetchPrevU( svnnIterator* aIterator,
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
    smxTrans*         sTrans               = (smxTrans*)aIterator->trans;
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

        smnManager::updatedVolRow( (smiIterator*)aIterator );
        *aRow = aIterator->curRecPtr;
        if( SM_SCN_IS_NOT_DELETED(((smpSlotHeader*)(*aRow))->mCreateSCN) )
        {
            SC_MAKE_GRID( aIterator->mRowGRID,
                          ((smcTableHeader*)aIterator->table)->mSpaceID,
                          SMP_SLOT_GET_PID( (smpSlotHeader*)*aRow ),
                          SMP_SLOT_GET_OFFSET( (smpSlotHeader*)*aRow ) );

            return IDE_SUCCESS;
        }
    }

    sFixed    = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    sSize     = sFixed->mSlotSize;

    if( aIterator->least == ID_FALSE )
    {
        IDE_TEST( svnnMovePrev( aIterator ) != IDE_SUCCESS );

        /*
         * page가 NULL인 경우는 beforeFirst에서 결과가 없을때 발생할수 있으며,
         * page가 SPECIAL인 경우는 previous page가 없음을 의미한다.
         * 이러한 2가지 경우는 더이상 읽을 레코드가 없음을 의미한다.
         */
        IDE_TEST_CONT( (aIterator->page == SM_NULL_PID) ||
                       (aIterator->page == SM_SPECIAL_PID), RETURN_SUCCESS );

        aIterator->highFence =
            ( aIterator->slot = aIterator->lowFence = aIterator->rows + SVNN_ROWS_CACHE ) - 1;
        aIterator->highest = ID_TRUE;

        /* BUG-39423 : trace log 출력 시 성능 저하 및 정상 작동 경우 출력 방지 */
#ifdef DEBUG
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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
                    if( smnManager::checkCanReusableRollback( ( smiIterator* )aIterator, &sSlot )
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
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static IDE_RC svnnFetchNextR( svnnIterator* aIterator)
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
    smxTrans*         sTrans               = (smxTrans*)aIterator->trans;
    idBool            sCanReusableRollback = ID_TRUE;
    idBool            sReusableCheckAgain  = ID_FALSE;
    idBool            sIsVisible;

    sFixed = (smpPageListEntry*)&(aIterator->table->mFixed.mVRDB);
    sSize  = sFixed->mSlotSize;

restart:

    for( aIterator->slot++;
         aIterator->slot <= aIterator->highFence;
         aIterator->slot++ )
    {
        aIterator->curRecPtr      = *aIterator->slot;
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;
        IDE_TEST( smnManager::lockVolRow( (smiIterator*)aIterator)
                  != IDE_SUCCESS );
    }

    if( aIterator->highest == ID_FALSE )
    {
        IDU_FIT_POINT("svnnModule::svnnFetchNextR::wait1");

        IDE_TEST( svnnMoveNext( aIterator ) != IDE_SUCCESS );

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
        if(   ( aIterator->page < svpManager::getFirstScanPageID(sFixed) )
            ||( aIterator->page > svpManager::getLastScanPageID(sFixed)  ) )
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
                            svpManager::getFirstScanPageID(sFixed),
                            svpManager::getLastScanPageID(sFixed));
            }
        }
#endif

        IDE_TEST( svmManager::holdPageSLatch(aIterator->table->mSpaceID,
                                             aIterator->page) != IDE_SUCCESS );
        sLocked = ID_TRUE;

        IDE_ASSERT( smmManager::getOIDPtr( aIterator->table->mSpaceID, 
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
        IDE_TEST( svmManager::releasePageLatch(aIterator->table->mSpaceID,
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
        IDE_ASSERT( svmManager::releasePageLatch(aIterator->table->mSpaceID,
                                                 aIterator->page)
                    == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}


static IDE_RC svnnAllocIterator( void ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}


static IDE_RC svnnFreeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}


/*********************************************************************
 * FUNCTION DESCRIPTION : svnnGatherStat                             *
 * ------------------------------------------------------------------*
 * 통계정보를 구축한다.
 * BUGBUG 나중에 smnnGatherStat 으로 바꿔야 함. 중복임
 *********************************************************************/
IDE_RC svnnGatherStat( idvSQL         * aStatistics,
                       void           * aTrans,
                       SFloat           aPercentage,
                       SInt             /* aDegree */,
                       smcTableHeader * aHeader,
                       void           * aTotalTableArg,
                       smnIndexHeader * /*aIndex*/,
                       void           * aStats,
                       idBool           aDynamicMode )
{
    scPageID   sPageID = SM_NULL_PID;
    idBool     sLocked = ID_FALSE;
    SChar    * sFence  = NULL;
    SChar    * sRow    = NULL;
    void     * sTableArgument;
    UInt       sState = 0;

    IDE_TEST( smLayerCallback::beginTableStat( aHeader,
                                               aPercentage,
                                               aDynamicMode,
                                               &sTableArgument )
              != IDE_SUCCESS );
    sState = 1;

    IDE_TEST( smnManager::getNextPageForVolTable( aHeader,
                                                  &sPageID,
                                                  &sLocked )
              != IDE_SUCCESS );

    while( sPageID != SC_NULL_PID )
    {
        while( 1 )
        {
            IDE_TEST( smnManager::getNextRowForVolTable( 
                            aHeader,
                            sPageID,
                            &sFence,
                            &sRow,
                            ID_FALSE ) /*Validation*/
                        != IDE_SUCCESS );
            if( sRow == NULL )
            {
                break;
            }

            IDE_TEST( smLayerCallback::analyzeRow4Stat( aHeader,
                                                        sTableArgument,
                                                        aTotalTableArg,
                                                        (UChar*)sRow )
                      != IDE_SUCCESS );
        }

        IDE_TEST( smnManager::getNextPageForVolTable( aHeader,
                                                      &sPageID,
                                                      &sLocked )
                  != IDE_SUCCESS );

        IDE_TEST( iduCheckSessionEvent( aStatistics )
                  != IDE_SUCCESS );
    }

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

    switch( sState )
    {
    case 1:
        (void) smLayerCallback::endTableStat( aHeader,
                                              sTableArgument,
                                              aDynamicMode );
    default:
        break;
    }

    return IDE_FAILURE;
}



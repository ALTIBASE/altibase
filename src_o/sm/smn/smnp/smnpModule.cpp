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
 * $Id: smnpModule.cpp 89495 2020-12-14 05:19:22Z emlee $
 ******************************************************************************/

#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <smc.h>
#include <smm.h>
#include <smp.h>

#include <smErrorCode.h>
#include <smnModules.h>
#include <smnReq.h>
#include <smnpDef.h>
#include <smnpModule.h>
#include <smnManager.h>
#include <smmExpandChunk.h>

/*******************************************************************************
 * Description: Memory Table에 대해서 GRID scan을 수행하는 모듈
 *
 *  Cursor를 open할 때 전달받는 smiRange 타입의 range에 scan 하고자 하는 GRID의
 * list를 QP로부터 전달받는다. GRID 값은 smiRange->minimum->data 에 할당되어
 * 있고, 이 값은 type casting을 통해 바로 scGRID로 변환하여 사용한다.
 ******************************************************************************/

static IDE_RC smnpPrepareIteratorMem( const smnIndexModule* );

static IDE_RC smnpReleaseIteratorMem(const smnIndexModule* );

static IDE_RC smnpInit( smnpIterator         * aIterator,
                        void                 * aTrans,
                        smcTableHeader       * aTable,
                        smnIndexHeader       * /* aIndex */,
                        void                 * /* aDumpObject */,
                        const smiRange       * aKeyRange,
                        const smiRange       * /* aKeyFilter */,
                        const smiCallBack    * aRowFilter,
                        UInt                   aFlag,
                        smSCN                  aSCN,
                        smSCN                  aInfinite,
                        idBool                 /* aUntouchable */,
                        smiCursorProperties  * aProperties,
                        const smSeekFunc    ** aSeekFunc,
                        smiStatement         * aStatement );

static IDE_RC smnpDest( smnpIterator* aIterator );

static IDE_RC smnpNA( void );
static IDE_RC smnpAA( smnpIterator  * aIterator,
                                 const void   ** aRow );
static IDE_RC smnpBeforeFirst( smnpIterator      * aIterator,
                               const smSeekFunc ** aSeekFunc );
static IDE_RC smnpAfterLast( smnpIterator      * aIterator,
                             const smSeekFunc ** aSeekFunc );
static IDE_RC smnpBeforeFirstU( smnpIterator      * aIterator,
                                const smSeekFunc ** aSeekFunc );
static IDE_RC smnpAfterLastU( smnpIterator      * aIterator,
                              const smSeekFunc ** aSeekFunc );
static IDE_RC smnpBeforeFirstR( smnpIterator      * aIterator,
                                const smSeekFunc ** aSeekFunc );
static IDE_RC smnpAfterLastR( smnpIterator      * aIterator,
                              const smSeekFunc ** aSeekFunc );
static IDE_RC smnpFetchNext( smnpIterator  * aIterator,
                             const void   ** aRow );
static IDE_RC smnpFetchPrev( smnpIterator  * aIterator,
                             const void   ** aRow );
static IDE_RC smnpFetchNextU( smnpIterator  * aIterator,
                              const void   ** aRow );
static IDE_RC smnpFetchPrevU( smnpIterator  * aIterator,
                              const void   ** aRow );
static IDE_RC smnpFetchNextR( smnpIterator  * aIterator );

static IDE_RC smnpAllocIterator( void  ** aIteratorMem );

static IDE_RC smnpFreeIterator( void  * aIteratorMem );

static IDE_RC smnpGetValidVersion( smnpIterator  * aIterator,
                                   smOID           aRowOID,
                                   void         ** aRow,
                                   idBool        * aIsVisibleRow );

static IDE_RC smnpValidateGRID( smcTableHeader     * aTableHdr,
                                scGRID               aGRID,
                                idBool             * aIsValidGRID );

smnIndexModule smnpModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_MEMORY,
                              SMI_BUILTIN_GRID_INDEXTYPE_ID ),
    SMN_RANGE_ENABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( smnpIterator ),
    (smnMemoryFunc) smnpPrepareIteratorMem,
    (smnMemoryFunc) smnpReleaseIteratorMem,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc) NULL,
    (smTableCursorLockRowFunc) smnManager::lockRow,
    (smnDeleteFunc) NULL,
    (smnFreeFunc) NULL,
    (smnInsertRollbackFunc) NULL,
    (smnDeleteRollbackFunc) NULL,
    (smnAgingFunc) NULL,
    (smInitFunc) smnpInit,
    (smDestFunc) smnpDest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc) NULL,
    (smnSetPositionFunc) NULL,

    (smnAllocIteratorFunc) smnpAllocIterator,
    (smnFreeIteratorFunc) smnpFreeIterator,
    (smnReInitFunc) NULL,
    (smnInitMetaFunc) NULL,
    
    (smnMakeDiskImageFunc) NULL,

    (smnBuildIndexFunc) NULL,
    (smnGetSmoNoFunc) NULL,
    (smnMakeKeyFromRow) NULL,
    (smnMakeKeyFromSmiValue) NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat) NULL
};

static const smSeekFunc smnpSeekFunctions[32][12] =
{ { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirstU,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirstR,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirstU,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)smnpFetchNextU,
        (smSeekFunc)smnpFetchPrevU,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpBeforeFirstR,
        (smSeekFunc)smnpAfterLastR,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnpFetchNextU,
        (smSeekFunc)smnpFetchPrevU,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLastU,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLastR,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLastU,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA      // BUGBUG
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)smnpFetchPrevU,
        (smSeekFunc)smnpFetchNextU,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAfterLastR,
        (smSeekFunc)smnpBeforeFirstR,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)smnpFetchPrev,
        (smSeekFunc)smnpFetchNext,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)smnpFetchPrevU,
        (smSeekFunc)smnpFetchNextU,
        (smSeekFunc)smnpAfterLast,
        (smSeekFunc)smnpBeforeFirst,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpAA,
        (smSeekFunc)smnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,

        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA,
        (smSeekFunc)smnpNA
    }
};


static IDE_RC smnpPrepareIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC smnpReleaseIteratorMem(const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC smnpInit( smnpIterator          * aIterator,
                        void                  * aTrans,
                        smcTableHeader        * aTable,
                        smnIndexHeader        * ,
                        void                  * ,
                        const smiRange        * aRange,
                        const smiRange        * ,
                        const smiCallBack     * aFilter,
                        UInt                    aFlag,
                        smSCN                   aSCN,
                        smSCN                   aInfinite,
                        idBool                  ,
                        smiCursorProperties   * aProperties,
                        const smSeekFunc     ** aSeekFunc,
                        smiStatement          * aStatement )
{
    idvSQL                        *sSQLStat;

    aIterator->SCN                = aSCN;
    aIterator->infinite           = aInfinite;
    aIterator->trans              = aTrans;
    aIterator->table              = aTable;
    aIterator->curRecPtr          = NULL;
    aIterator->lstFetchRecPtr     = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->tid                = smLayerCallback::getTransID( aTrans );
    aIterator->flag               = aFlag;
    aIterator->mProperties        = aProperties;

    aIterator->mRange             = aRange;
    aIterator->mNxtRange          = NULL;
    aIterator->mFilter            = aFilter;
    aIterator->mStatement         = aStatement;

    *aSeekFunc = smnpSeekFunctions[ aFlag&(SMI_TRAVERSE_MASK |
                                           SMI_PREVIOUS_MASK |
                                           SMI_LOCK_MASK) ];

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mMemCursorGRIDScan, 1);
    
    if (sSQLStat != NULL)
    {
        IDV_SESS_ADD(sSQLStat->mSess, IDV_STAT_INDEX_MEM_CURSOR_GRID_SCAN, 1);
    }
    
    return IDE_SUCCESS;
}

static IDE_RC smnpDest( smnpIterator* /*aIterator*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC smnpAA( smnpIterator*,
                      const void** )
{
    return IDE_SUCCESS;
}

IDE_RC smnpNA( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;
}

static IDE_RC smnpBeforeFirst( smnpIterator       * aIterator,
                               const smSeekFunc  ** )
{
    aIterator->curRecPtr         = NULL;
    aIterator->lstFetchRecPtr    = NULL;

    aIterator->mNxtRange = aIterator->mRange;
   
    return IDE_SUCCESS;
}                                                                             

static IDE_RC smnpAfterLast( smnpIterator       * aIterator,
                             const smSeekFunc  ** )
{
    aIterator->curRecPtr         = NULL;
    aIterator->lstFetchRecPtr    = NULL;

    for( aIterator->mNxtRange = aIterator->mRange;
         aIterator->mNxtRange->next != NULL;
         aIterator->mNxtRange = aIterator->mNxtRange->next ) ;

    return IDE_SUCCESS;
}                                                                             

static IDE_RC smnpBeforeFirstU( smnpIterator       * aIterator,
                                const smSeekFunc  ** aSeekFunc )
{
    IDE_TEST( smnpBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}                                                                             

static IDE_RC smnpAfterLastU( smnpIterator       * aIterator,
                              const smSeekFunc  ** aSeekFunc )
{
    IDE_TEST( smnpAfterLast( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}                                                                             

static IDE_RC smnpBeforeFirstR( smnpIterator       * aIterator,
                                const smSeekFunc  ** aSeekFunc )
{
    IDE_TEST( smnpBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );
    
    IDE_TEST( smnpFetchNextR( aIterator ) != IDE_SUCCESS );
    
    aIterator->curRecPtr          = NULL;
    aIterator->lstFetchRecPtr     = NULL;

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}                                                                             

static IDE_RC smnpAfterLastR( smnpIterator       * aIterator,
                              const smSeekFunc  ** aSeekFunc )
{
    IDE_TEST( smnpBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    IDE_TEST( smnpFetchNextR( aIterator ) != IDE_SUCCESS );

    IDE_TEST( smnpAfterLast( aIterator, aSeekFunc ) != IDE_SUCCESS );
 
    *aSeekFunc += 6;

    return IDE_SUCCESS;
    
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}                                                                             

static IDE_RC smnpFetchNext( smnpIterator   * aIterator,
                             const void    ** aRow )
{
    smOID       sRowOID;
    idBool      sResult;
    idBool      sIsVisibleRow;
    scGRID      sGRID;
    idBool      sIsValidGRID;
    idBool      sIsFetchSuccess = ID_FALSE;

    if( (aIterator->mProperties->mReadRecordCount == 0) ||
        (aIterator->mNxtRange == NULL) )
    {
        IDE_CONT( skip_no_more_row );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics)
              != IDE_SUCCESS);

    SC_MAKE_NULL_GRID( sGRID );

    while( aIterator->mNxtRange != NULL )
    {
        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->next;

        IDE_TEST( smnpValidateGRID( aIterator->table, sGRID, &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sRowOID = SM_MAKE_OID( SC_MAKE_PID(sGRID), SC_MAKE_OFFSET(sGRID) );

        IDE_TEST( smnpGetValidVersion( aIterator,
                                       sRowOID,
                                       (void**)&aIterator->curRecPtr,
                                       &sIsVisibleRow )
                  != IDE_SUCCESS );

        if( sIsVisibleRow == ID_FALSE)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                aIterator->curRecPtr,
                                                NULL,
                                                0,
                                                sGRID,
                                                aIterator->mFilter->data )
                  != IDE_SUCCESS );

        if( sResult == ID_TRUE )
        {
            if( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if( aIterator->mProperties->mReadRecordCount != 0 )
                {
                    aIterator->mProperties->mReadRecordCount--;
                    sIsFetchSuccess = ID_TRUE;
                    break;
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    if( sIsFetchSuccess == ID_TRUE )
    {
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        *aRow = aIterator->curRecPtr;
        SC_COPY_GRID( sGRID, aIterator->mRowGRID );
    }
    else
    {
        aIterator->curRecPtr = NULL;
        aIterator->lstFetchRecPtr = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

static IDE_RC smnpFetchPrev( smnpIterator   * aIterator,
                             const void    ** aRow )
{
    smOID       sRowOID;
    idBool      sResult;
    idBool      sIsVisibleRow;
    scGRID      sGRID;
    idBool      sIsValidGRID;
    idBool      sIsFetchSuccess = ID_FALSE;

    if( (aIterator->mProperties->mReadRecordCount == 0) ||
        (aIterator->mNxtRange == NULL) )
    {
        IDE_CONT( skip_no_more_row );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics)
              != IDE_SUCCESS);

    SC_MAKE_NULL_GRID( sGRID );

    while( aIterator->mNxtRange != NULL )
    {
        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->prev;

        IDE_TEST( smnpValidateGRID( aIterator->table, sGRID, &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sRowOID = SM_MAKE_OID( SC_MAKE_PID(sGRID), SC_MAKE_OFFSET(sGRID) );

        IDE_TEST( smnpGetValidVersion( aIterator,
                                       sRowOID,
                                       (void**)&aIterator->curRecPtr,
                                       &sIsVisibleRow )
                  != IDE_SUCCESS );

        if( sIsVisibleRow == ID_FALSE)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                aIterator->curRecPtr,
                                                NULL,
                                                0,
                                                sGRID,
                                                aIterator->mFilter->data )
                  != IDE_SUCCESS );

        if( sResult == ID_TRUE )
        {
            if( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if( aIterator->mProperties->mReadRecordCount != 0 )
                {
                    aIterator->mProperties->mReadRecordCount--;
                    sIsFetchSuccess = ID_TRUE;
                    break;
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    if( sIsFetchSuccess == ID_TRUE )
    {
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        *aRow = aIterator->curRecPtr;
        SC_COPY_GRID( sGRID, aIterator->mRowGRID );
    }
    else
    {
        aIterator->curRecPtr = NULL;
        aIterator->lstFetchRecPtr = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}                                                                             

static IDE_RC smnpFetchNextU( smnpIterator   * aIterator,
                              const void    ** aRow )
{
    smOID       sRowOID;
    idBool      sResult;
    idBool      sIsVisibleRow;
    scGRID      sGRID;
    idBool      sIsValidGRID;
    idBool      sIsFetchSuccess = ID_FALSE;

    if( (aIterator->mProperties->mReadRecordCount == 0) ||
        (aIterator->mNxtRange == NULL) )
    {
        IDE_CONT( skip_no_more_row );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics)
              != IDE_SUCCESS);

    SC_MAKE_NULL_GRID( sGRID );

    while( aIterator->mNxtRange != NULL )
    {
        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->next;

        IDE_TEST( smnpValidateGRID( aIterator->table, sGRID, &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sRowOID = SM_MAKE_OID( SC_MAKE_PID(sGRID), SC_MAKE_OFFSET(sGRID) );

        IDE_TEST( smnpGetValidVersion( aIterator,
                                       sRowOID,
                                       (void**)&aIterator->curRecPtr,
                                       &sIsVisibleRow )
                  != IDE_SUCCESS );

        if( sIsVisibleRow == ID_FALSE)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                aIterator->curRecPtr,
                                                NULL,
                                                0,
                                                sGRID,
                                                aIterator->mFilter->data )
                  != IDE_SUCCESS );

        if( sResult == ID_TRUE )
        {
            if( SM_SCN_IS_DELETED(
                         ((smpSlotHeader*)(aIterator->curRecPtr))->mCreateSCN) )
            {
                continue;
            }

            if( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if( aIterator->mProperties->mReadRecordCount != 0 )
                {
                    aIterator->mProperties->mReadRecordCount--;
                    sIsFetchSuccess = ID_TRUE;
                    break;
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    if( sIsFetchSuccess == ID_TRUE )
    {
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        smnManager::updatedRow( (smiIterator*)aIterator );

        *aRow = aIterator->curRecPtr;
        SC_COPY_GRID( sGRID, aIterator->mRowGRID );
    }
    else
    {
        aIterator->curRecPtr = NULL;
        aIterator->lstFetchRecPtr = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC smnpFetchPrevU( smnpIterator   * aIterator,
                              const void    ** aRow )
{
    smOID       sRowOID;
    idBool      sResult;
    idBool      sIsVisibleRow;
    scGRID      sGRID;
    idBool      sIsValidGRID;
    idBool      sIsFetchSuccess = ID_FALSE;

    if( (aIterator->mProperties->mReadRecordCount == 0) ||
        (aIterator->mNxtRange == NULL) )
    {
        IDE_RAISE( skip_no_more_row );
    }
    else
    {
        /* do nothing */
    }

    IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics)
              != IDE_SUCCESS);

    SC_MAKE_NULL_GRID( sGRID );

    while( aIterator->mNxtRange != NULL )
    {
        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->prev;

        IDE_TEST( smnpValidateGRID( aIterator->table, sGRID, &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sRowOID = SM_MAKE_OID( SC_MAKE_PID(sGRID), SC_MAKE_OFFSET(sGRID) );

        IDE_TEST( smnpGetValidVersion( aIterator,
                                       sRowOID,
                                       (void**)&aIterator->curRecPtr,
                                       &sIsVisibleRow )
                  != IDE_SUCCESS );

        if( sIsVisibleRow == ID_FALSE)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                aIterator->curRecPtr,
                                                NULL,
                                                0,
                                                sGRID,
                                                aIterator->mFilter->data )
                  != IDE_SUCCESS );

        if( sResult == ID_TRUE )
        {
            if( SM_SCN_IS_DELETED( 
                        ((smpSlotHeader*)(aIterator->curRecPtr))->mCreateSCN ))
            {
                continue;
            }

            if( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if( aIterator->mProperties->mReadRecordCount != 0 )
                {
                    aIterator->mProperties->mReadRecordCount--;
                    sIsFetchSuccess = ID_TRUE;
                    break;
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    if( sIsFetchSuccess == ID_TRUE )
    {
        aIterator->lstFetchRecPtr = aIterator->curRecPtr;

        smnManager::updatedRow( (smiIterator*)aIterator );

        *aRow = aIterator->curRecPtr;
        SC_COPY_GRID( sGRID, aIterator->mRowGRID );
    }
    else
    {
        aIterator->curRecPtr = NULL;
        aIterator->lstFetchRecPtr = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
        *aRow = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}                                                                             

static IDE_RC smnpFetchNextR( smnpIterator  * aIterator )
{
    smOID       sRowOID;
    idBool      sResult;
    idBool      sIsVisibleRow;
    scGRID      sGRID;
    idBool      sIsValidGRID;
    /* BUG-39836 : 최초값 저장 변수 추가 */ 
    ULong           sReadRecordCountOrigin;
    ULong           sFirstReadRecordPosOrigin; 
    const smiRange  *sNxtRangeOrigin;

    /* BUG-39836 : repeatable read모드에서 smnp/svnpFetchNextR함수는 fetch할 row를 
     * 순회하고 lock을 잡는다. 대상이 되는 row를 순회하기 위해 aIterator->mProperties의 
     * mReadRecordCount와 mFirstReadRecordPos, mNxtRange값을 이용한다. 이 함수에서는 
     * lock만 잡고 실제 fetch는 smnp/svnpFetchNext에서 수행한다.(smnp/svnpSeekFunctions 참조)
     * 따라서 mReadRecordCount와 mFirstReadRecordPos, mNxtRange의 최초 값을 저장하고
     * 이 함수가 종료 되는 시점에 이 변수들의 값을 최초값으로 복구 시켜 주어야 한다. 
     */
    sReadRecordCountOrigin    = aIterator->mProperties->mReadRecordCount;
    sFirstReadRecordPosOrigin = aIterator->mProperties->mFirstReadRecordPos; 
    sNxtRangeOrigin           = aIterator->mNxtRange;

    if( (aIterator->mProperties->mReadRecordCount == 0) ||
        (aIterator->mNxtRange == NULL) )
    {
        IDE_CONT( skip_no_more_row );
    }
    else
    {
        /* do nothing */
    }
    
    while( aIterator->mNxtRange != NULL )
    {
        IDE_TEST( iduCheckSessionEvent(aIterator->mProperties->mStatistics)
                  != IDE_SUCCESS);

        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->next;

        IDE_TEST( smnpValidateGRID( aIterator->table, sGRID, &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_FALSE )
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        sRowOID = SM_MAKE_OID( SC_MAKE_PID(sGRID), SC_MAKE_OFFSET(sGRID) );

        IDE_TEST( smnpGetValidVersion( aIterator,
                                       sRowOID,
                                       (void**)&aIterator->curRecPtr,
                                       &sIsVisibleRow )
                  != IDE_SUCCESS );

        if( sIsVisibleRow == ID_FALSE)
        {
            continue;
        }
        else
        {
            /* do nothing */
        }

        IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                aIterator->curRecPtr,
                                                NULL,
                                                0,
                                                sGRID,
                                                aIterator->mFilter->data )
                  != IDE_SUCCESS );

        if( sResult == ID_TRUE )
        {
            if( aIterator->mProperties->mFirstReadRecordPos == 0 )
            {
                if( aIterator->mProperties->mReadRecordCount != 0 )
                {
                    aIterator->mProperties->mReadRecordCount--;

                    aIterator->lstFetchRecPtr = aIterator->curRecPtr;
                    IDE_TEST( smnManager::lockRow( (smiIterator*)aIterator )
                              != IDE_SUCCESS );
                }
            }
            else
            {
                aIterator->mProperties->mFirstReadRecordPos--;
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    aIterator->curRecPtr = NULL;
    aIterator->lstFetchRecPtr = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    
    /* BUG-39836 : mReadRecordCount와 mFirstReadRecordPos, mNxtRange를 최초 값으로 복원 */
    aIterator->mProperties->mReadRecordCount    = sReadRecordCountOrigin;
    aIterator->mProperties->mFirstReadRecordPos = sFirstReadRecordPosOrigin;
    aIterator->mNxtRange                        = sNxtRangeOrigin;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


static IDE_RC smnpAllocIterator( void  ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}


static IDE_RC smnpFreeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

/*******************************************************************************
 * Description: smpSlotHeader의 next를 따라가서 자신이 읽을 수 있는 visible한
 *              version이 있을 경우, 읽을 수 있는 version의 row pointer를 반환.
 *              읽을 수 있는 version이 없을 경우, aIsVisibleRow를 ID_FALSE로
 *              설정하여 반환.
 *
 * Parameters:
 *  - aIterator     [IN] Iterator
 *  - aFstOID       [IN] 최초 접근한 record의 OID
 *  - aRow          [OUT] 읽어온 record의 pointer
 *  - aIsVisibleRow [OUT] aRow의 pointer가 visible한 record를 가리키고 있는지
 ******************************************************************************/
static IDE_RC smnpGetValidVersion( smnpIterator  * aIterator,
                                   smOID           aFstOID,
                                   void         ** aRow,
                                   idBool        * aIsVisibleRow )
{
    scSpaceID        sSpaceID  = aIterator->table->mSpaceID;
    smOID            sCurOID   = aFstOID;
    smOID            sNxtOID;
    smpSlotHeader  * sRow;
    idBool           sLocked   = ID_FALSE;
    idBool           sIsVisible;

    IDE_ERROR( aRow != NULL );
    IDE_ERROR( aIsVisibleRow != NULL );


    *aIsVisibleRow = ID_FALSE;

    IDE_TEST( smmManager::holdPageSLatch( sSpaceID,
                                          SM_MAKE_PID(aFstOID) )
              != IDE_SUCCESS );
    sLocked = ID_TRUE;

    while( sCurOID != SM_NULL_OID )
    {
        IDE_ASSERT( smmManager::getOIDPtr( sSpaceID,
                                           sCurOID,
                                           (void**)&sRow )
                    == IDE_SUCCESS );

        IDE_TEST( smnManager::checkSCN( (smiIterator*)aIterator, sRow, NULL, &sIsVisible )
                  != IDE_SUCCESS );
        if( sIsVisible == ID_TRUE )
        {
            *aRow = sRow;
            *aIsVisibleRow = ID_TRUE;
            break;
        }
        else
        {
            sNxtOID = SMP_SLOT_GET_NEXT_OID( sRow );

            if( SM_IS_VALID_OID( sNxtOID ) == ID_TRUE )
            {
                sCurOID = sNxtOID;
                continue;
            }
            else
            {
                *aRow = NULL;
                *aIsVisibleRow = ID_FALSE;
                break;
            }
        }
    }

    sLocked = ID_FALSE;
    IDE_TEST( smmManager::releasePageLatch( sSpaceID,
                                            SM_MAKE_PID(aFstOID) )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sLocked == ID_TRUE )
    {
        IDE_ASSERT( smmManager::releasePageLatch( sSpaceID,
                                                  SM_MAKE_PID(aFstOID) )
                    == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description: MRDB에 대해 fetch by GRID를 수행할 때 대상 GRID가 유효한지
 *              확인하는 함수
 *
 * Parameters:
 *  - aTableHdr     [IN] Fetch 대상 table의 table header
 *  - aGRID         [IN] Fetch 대상 record의 GRID
 *  - aIsValidGRID  [OUT] GRID가 유효한지 여부
 ******************************************************************************/
static IDE_RC smnpValidateGRID( smcTableHeader     * aTableHdr,
                                scGRID               aGRID,
                                idBool             * aIsValidGRID )
{
    scSpaceID            sSpaceID;
    scPageID             sPageID;
    scOffset             sOffset;
    smmTBSNode         * sTBSNode;
    vULong               sMaxPageCnt;
    smpPersPageHeader  * sPageHdrPtr;
    vULong               sSlotSize;
    smmPageState         sPageState;

    *aIsValidGRID = ID_FALSE;

    /* 읽을 수 있는 GRID인지 검사 */
    IDE_TEST_CONT( SC_GRID_IS_NULL(aGRID), error_invalid_grid );
    IDE_TEST_CONT( SC_GRID_IS_WITH_SLOTNUM(aGRID), error_invalid_grid );

    sSpaceID = SC_MAKE_SPACE(aGRID);
    sPageID  = SC_MAKE_PID(aGRID);
    sOffset  = SC_MAKE_OFFSET(aGRID);

    /* GRID와 table header의 SpaceID 일치 검사 */
    IDE_TEST_CONT( sSpaceID != aTableHdr->mSpaceID,
                    error_invalid_grid );

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID( sSpaceID,
                                                        (void**)&sTBSNode )
              != IDE_SUCCESS );

    sMaxPageCnt = smmDatabase::getAllocPersPageCount( sTBSNode->mMemBase );

    /* GRID의 PageID가 유효한 PageID 범위 안인지 확인 */
    IDE_TEST_CONT( !(sPageID < sMaxPageCnt), error_invalid_grid );

    /* 대상 page가 table에 할당 되어 있는 data page가 맞는지 확인 */
    IDE_TEST( smmExpandChunk::getPageState( sTBSNode,
                                            sPageID,
                                            &sPageState )
              != IDE_SUCCESS );

    IDE_TEST_CONT( sPageState != SMM_PAGE_STATE_ALLOC, error_invalid_grid );

    IDE_TEST( smmManager::getPersPagePtr( sSpaceID,
                                          sPageID,
                                          (void**)&sPageHdrPtr )
              != IDE_SUCCESS );

    /* GRID와 page의 TableOID가 일치하는지, fixed page가 맞는지 확인 */
    IDE_TEST_CONT( ( sPageHdrPtr->mTableOID != aTableHdr->mSelfOID ) &&
                    ( SMP_GET_PERS_PAGE_TYPE(sPageHdrPtr) ==
                            SMP_PAGETYPE_FIX ),
                    error_invalid_grid );

    /* offset이 slot size에 따른 align에 대해 유효한 offset 값인지 확인 */
    sSlotSize = aTableHdr->mFixed.mMRDB.mSlotSize;

    IDE_TEST_CONT( ((sOffset - ID_SIZEOF(smpPersPageHeader)) % sSlotSize)
                        != 0,
                    error_invalid_grid )


    /* 모든 검사를 통과하였음. */
    *aIsValidGRID = ID_TRUE;

    IDE_EXCEPTION_CONT( error_invalid_grid );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


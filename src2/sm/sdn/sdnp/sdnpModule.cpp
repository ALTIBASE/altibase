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
 * $Id: sdnpModule.cpp 89495 2020-12-14 05:19:22Z emlee $
 ******************************************************************************/

#include <ide.h>
#include <idu.h>
#include <smi.h>
#include <sdr.h>
#include <sdp.h>
#include <smc.h>
#include <sdc.h>
#include <smm.h>
#include <smp.h>
#include <smx.h>

#include <smErrorCode.h>
#include <smnModules.h>
#include <smnManager.h>
#include <sdbMPRMgr.h>
#include <sdnpDef.h>
#include <sdn.h>
#include <sdnReq.h>

static IDE_RC sdnpPrepareIteratorMem( const smnIndexModule* );

static IDE_RC sdnpReleaseIteratorMem( const smnIndexModule* );

static IDE_RC sdnpInit( sdnpIterator          * aIterator,
                        void                  * aTrans,
                        smcTableHeader        * aTable,
                        void                  * /* aIndexHeader */,
                        void                  * /* aDumpObject */,
                        const smiRange        * aKeyRange,
                        const smiRange        * /* aKeyFilter */,
                        const smiCallBack     * aFilter,
                        UInt                    aFlag,
                        smSCN                   aSCN,
                        smSCN                   aInfinite,
                        idBool                  /* aUntouchable */,
                        smiCursorProperties   * aProperties,
                        const smSeekFunc     ** aSeekFunc,
                        smiStatement          * aStatement );

static IDE_RC sdnpDest( sdnpIterator  * aIterator );

static IDE_RC sdnpNA( void );
static IDE_RC sdnpAA( sdnpIterator   * aIterator,
                      const void    ** aRow );

static IDE_RC sdnpBeforeFirst( sdnpIterator       * aIterator,
                               const smSeekFunc  ** aSeekFunc );

static IDE_RC sdnpBeforeFirstW( sdnpIterator       * aIterator,
                                const smSeekFunc  ** aSeekFunc );

static IDE_RC sdnpBeforeFirstRR( sdnpIterator       * aIterator,
                                 const smSeekFunc  ** aSeekFunc );

static IDE_RC sdnpAfterLast( sdnpIterator       * aIterator,
                             const smSeekFunc  ** aSeekFunc );

static IDE_RC sdnpAfterLastW( sdnpIterator       * aIterator,
                              const smSeekFunc  ** aSeekFunc );

static IDE_RC sdnpAfterLastRR( sdnpIterator       * aIterator,
                               const smSeekFunc  ** aSeekFunc );

static IDE_RC sdnpFetchNext( sdnpIterator   * aIterator,
                             const void    ** aRow );

static IDE_RC sdnpLockAllRows4RR( sdnpIterator  * aIterator );

static IDE_RC sdnpLockRow4RR( sdnpIterator  * aIterator,
                              sdrMtx        * aMtx,
                              sdrSavePoint  * aSvp,
                              scGRID          aGRID,
                              UChar         * aRowPtr,
                              UChar         * aPagePtr );

static IDE_RC sdnpAllocIterator( void  ** aIteratorMem );

static IDE_RC sdnpFreeIterator( void  * aIteratorMem );

static IDE_RC sdnpLockRow( sdnpIterator  * aIterator );

static IDE_RC sdnpFetchAndCheckFilter( sdnpIterator * aIterator,
                                       UChar        * aSlot,
                                       scGRID         aRowGRID,
                                       UChar        * aDestBuf,
                                       idBool       * aResult,
                                       idBool       * aIsPageLatchReleased );

static IDE_RC sdnpValidateAndGetPageByGRID(
                                        idvSQL              * aStatistics,
                                        sdrMtx              * aMtx,
                                        sddTableSpaceNode   * aTBSNode,
                                        smcTableHeader      * aTableHdr,
                                        scGRID                aGRID,
                                        sdbLatchMode          aLatchMode,
                                        UChar              ** aRowPtr,
                                        UChar              ** aPagePtr,
                                        idBool              * aIsValidGRID );

smnIndexModule sdnpModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_DISK,
                              SMI_BUILTIN_GRID_INDEXTYPE_ID ),
    SMN_RANGE_ENABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( sdnpIterator ),
    (smnMemoryFunc) sdnpPrepareIteratorMem,
    (smnMemoryFunc) sdnpReleaseIteratorMem,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc) NULL,

    (smTableCursorLockRowFunc) sdnpLockRow,
    (smnDeleteFunc) NULL,
    (smnFreeFunc) NULL,
    (smnInsertRollbackFunc) NULL,
    (smnDeleteRollbackFunc) NULL,
    (smnAgingFunc) NULL,

    (smInitFunc) sdnpInit,
    (smDestFunc) sdnpDest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc) NULL,
    (smnSetPositionFunc) NULL,

    (smnAllocIteratorFunc) sdnpAllocIterator,
    (smnFreeIteratorFunc) sdnpFreeIterator,
    (smnReInitFunc) NULL,
    (smnInitMetaFunc) NULL,
    (smnMakeDiskImageFunc) NULL,
    (smnBuildIndexFunc) NULL,
    (smnGetSmoNoFunc) NULL,
    (smnMakeKeyFromRow) NULL,
    (smnMakeKeyFromSmiValue) NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency) NULL,
    (smnGatherStat) NULL
};

static const smSeekFunc sdnpSeekFunctions[32][12] =
{ { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirstW,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATALBE      */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirstRR,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirstW,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA, // sdnpFetchPrev
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA, // sdnpFetchPrevW,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATALBE      */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpBeforeFirstRR,
        (smSeekFunc)sdnpAfterLastRR,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpNA, // sdnpFetchPrevU,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLastW,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLastRR,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLastW,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA      // BUGBUG
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)sdnpNA, // sdnpFetchPrevU,
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAfterLastRR,
        (smSeekFunc)sdnpBeforeFirstRR,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnpNA, // sdnpFetchPrev,
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnpNA, // sdnpFetchPrevU,
        (smSeekFunc)sdnpFetchNext,
        (smSeekFunc)sdnpAfterLast,
        (smSeekFunc)sdnpBeforeFirst,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpAA,
        (smSeekFunc)sdnpAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,

        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA,
        (smSeekFunc)sdnpNA
    }
};

static IDE_RC sdnpPrepareIteratorMem( const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC sdnpReleaseIteratorMem(const smnIndexModule* )
{
    return IDE_SUCCESS;
}

static IDE_RC sdnpInit( sdnpIterator          * aIterator,
                        void                  * aTrans,
                        smcTableHeader        * aTable,
                        void                  * /* aIndexHeader */,
                        void                  * /* aDumpObject */,
                        const smiRange        * aRange,
                        const smiRange        * /* aKeyFilter */,
                        const smiCallBack     * aFilter,
                        UInt                    aFlag,
                        smSCN                   aSCN,
                        smSCN                   aInfinite,
                        idBool                  /* aUntouchable */,
                        smiCursorProperties   * aProperties,
                        const smSeekFunc     ** aSeekFunc, 
                        smiStatement          * aStatement )
{
    idvSQL            *sSQLStat;

    SM_SET_SCN( &aIterator->mSCN, &aSCN );
    SM_SET_SCN( &(aIterator->mInfinite), &aInfinite );
    aIterator->mTrans           = aTrans;
    aIterator->mTable           = aTable;
    aIterator->mCurRecPtr       = NULL;
    aIterator->mLstFetchRecPtr  = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->mTid             = smLayerCallback::getTransID( aTrans );
    aIterator->mFlag            = aFlag;
    aIterator->mProperties      = aProperties;

    aIterator->mRange           = aRange;
    aIterator->mNxtRange        = NULL;
    aIterator->mFilter          = aFilter;
    aIterator->mStatement       = aStatement;

    IDE_TEST( sctTableSpaceMgr::findSpaceNodeBySpaceID(
                                        aTable->mSpaceID,
                                        (void**)&aIterator->mTBSNode )
              != IDE_SUCCESS );

    *aSeekFunc = sdnpSeekFunctions[ aFlag&( SMI_TRAVERSE_MASK |
                                            SMI_PREVIOUS_MASK |
                                            SMI_LOCK_MASK ) ];

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mDiskCursorGRIDScan, 1);

    if (sSQLStat != NULL)
    {
        IDV_SESS_ADD(sSQLStat->mSess,
                     IDV_STAT_INDEX_DISK_CURSOR_GRID_SCAN, 1);
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC sdnpDest( sdnpIterator  * /*aIterator*/ )
{
    return IDE_SUCCESS;
}

static IDE_RC sdnpAA( sdnpIterator   * /*aIterator*/,
                      const void    ** /* */)
{
    return IDE_SUCCESS;
}

IDE_RC sdnpNA( void )
{
    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;
}

static IDE_RC sdnpBeforeFirst( sdnpIterator       * aIterator,
                               const smSeekFunc  ** /*aSeekFunc*/)
{
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );

    aIterator->mNxtRange = aIterator->mRange;
    
    return IDE_SUCCESS;
}

/*********************************************************
  function description: sdnpBeforeFirstW
  sdnpBeforeFirst를 적용하고 smSeekFunction을 전진시켜
  다음에는 다른 callback이 불리도록 한다.
***********************************************************/
static IDE_RC sdnpBeforeFirstW( sdnpIterator*       aIterator,
                                const smSeekFunc** aSeekFunc )
{
    IDE_TEST( sdnpBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: sdnpBeforeFirstR

  - sdnpBeforeFirst를 적용하고  lockAllRows4RR을 불러서,
  filter를 만족시키는  row에 대하여 lock을 표현하는 undo record
  를 생성하고 rollptr를 연결시킨다.

  - sdnpBeforeFirst를 다시 적용한다.

  다음번에 호출될 경우에는  sdnpBeforeFirstR이 아닌
  다른 함수가 불리도록 smSeekFunction의 offset을 6전진시킨다.
***********************************************************/
static IDE_RC sdnpBeforeFirstRR( sdnpIterator *       aIterator,
                                 const smSeekFunc ** aSeekFunc )
{
    IDE_TEST( sdnpBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    // select for update를 위하여 lock을 나타내는 undo record들 생성.
    IDE_TEST( sdnpLockAllRows4RR( aIterator ) != IDE_SUCCESS );

    IDE_TEST( sdnpBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC sdnpAfterLast( sdnpIterator *      /* aIterator */,
                             const smSeekFunc ** /* aSeekFunc */)
{
    return IDE_FAILURE;
}

static IDE_RC sdnpAfterLastW( sdnpIterator *       /* aIterator */,
                              const smSeekFunc **  /* aSeekFunc */ )
{
    return IDE_FAILURE;
}

static IDE_RC sdnpAfterLastRR( sdnpIterator *       /* aIterator */,
                               const smSeekFunc **  /* aSeekFunc */ )
{
    return IDE_FAILURE;
}

/*******************************************************************************
 * Description: 대상 row를 fetch 해 온 후, filter를 적용한다.
 *
 * Parameters:
 *  aIterator       - [IN]  Iterator의 포인터
 *  aSlot           - [IN]  fetch할 대상 row의 slot 포인터
 *  aRowGRID        - [IN]  fetch할 대상 row의 GRID
 *  aDestBuf        - [OUT] fetch 해 온 row가 저장될 buffer
 *  aResult         - [OUT] fetch 결과
 ******************************************************************************/
static IDE_RC sdnpFetchAndCheckFilter( sdnpIterator * aIterator,
                                       UChar        * aSlot,
                                       scGRID         aRowGRID,
                                       UChar        * aDestBuf,
                                       idBool       * aResult,
                                       idBool       * aIsPageLatchReleased )
{
    idBool      sIsRowDeleted;

    IDE_DASSERT( aIterator != NULL );
    IDE_DASSERT( aSlot     != NULL );
    IDE_DASSERT( aDestBuf  != NULL );
    IDE_DASSERT( aResult   != NULL );
    IDE_DASSERT( sdcRow::isHeadRowPiece(aSlot) == ID_TRUE );

    *aResult = ID_FALSE;

    /* MVCC scheme에서 내 버젼에 맞는 row를 가져옴. */
    IDE_TEST( sdcRow::fetch(
                  aIterator->mProperties->mStatistics,
                  NULL, /* aMtx */
                  NULL, /* aSP */
                  aIterator->mTrans,
                  ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                  aSlot,
                  ID_FALSE, /* aIsPersSlot */
                  SDB_MULTI_PAGE_READ,
                  aIterator->mProperties->mFetchColumnList,
                  SMI_FETCH_VERSION_CONSISTENT,
                  smLayerCallback::getTSSlotSID( aIterator->mTrans ),
                  &(aIterator->mSCN ),
                  &(aIterator->mInfinite),
                  NULL, /* aIndexInfo4Fetch */
                  NULL, /* aLobInfo4Fetch */
                  ((smcTableHeader*)aIterator->mTable)->mRowTemplate,
                  aDestBuf,
                  &sIsRowDeleted,
                  aIsPageLatchReleased )
              != IDE_SUCCESS );

    // delete된 row이거나 insert 이전 버젼이면 skip
    IDE_TEST_CONT( sIsRowDeleted == ID_TRUE, skip_deleted_row );

    // filter적용;
    IDE_TEST( aIterator->mFilter->callback( aResult,
                                            aDestBuf,
                                            NULL,
                                            0,
                                            aRowGRID,
                                            aIterator->mFilter->data )
              != IDE_SUCCESS );

    IDE_EXCEPTION_CONT( skip_deleted_row );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
 * function description: sdnpFetchNext.
 * -  beforeFirst나 이전 fetchNext가 저장한 keymap sequence
 *    전진시켜서 아래의 작업을 한다.
 *    row의 getValidVersion을 구하고  filter적용한후, true이면
 *    해당 row의 버젼을 aRow에  copy하고 iterator에 위치를 저장한다.
 *
 * - PR-14121
 *   lock coupling 방식으로 다음 페이지를 latch를 걸고,
 *   현재 페이지를 unlatch한다. 왜냐하면, page list 연산과의 deadlock
 *   을 회피하기 위함이다.
 ***********************************************************/
static IDE_RC sdnpFetchNext( sdnpIterator   * aIterator,
                             const void    ** aRow )
{
    scGRID      sGRID;
    UChar     * sRowPtr;
    idBool      sIsValidGRID;
    idBool      sResult;
    idBool      sIsPageLatchReleased;
    idBool      sIsFetchSuccess = ID_FALSE;

    // table의 마지막 페이지의 next에 도달했거나,
    // selet .. from limit 100등으로 읽어야 할 갯수도달한 경우임.
    if( ( aIterator->mProperties->mReadRecordCount == 0 ) ||
        ( aIterator->mNxtRange == NULL) )
    {
        IDE_CONT( skip_no_more_row );
    }

    IDE_TEST( iduCheckSessionEvent( aIterator->mProperties->mStatistics )
              != IDE_SUCCESS );

    while( aIterator->mNxtRange != NULL )
    {
        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->next;

        IDE_TEST( sdnpValidateAndGetPageByGRID(
                                    aIterator->mProperties->mStatistics,
                                    NULL,
                                    aIterator->mTBSNode,
                                    (smcTableHeader*)aIterator->mTable,
                                    sGRID,
                                    SDB_S_LATCH,
                                    &sRowPtr,
                                    NULL,   /* aPagePtr */           
                                    &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_TRUE )
        {
            IDE_TEST( sdnpFetchAndCheckFilter( aIterator,
                                               sRowPtr,
                                               sGRID,
                                               (UChar*)*aRow,
                                               &sResult,
                                               &sIsPageLatchReleased )
                      != IDE_SUCCESS );

            if( sIsPageLatchReleased == ID_FALSE )
            {
                IDE_TEST( sdbBufferMgr::releasePage(
                                        aIterator->mProperties->mStatistics,
                                        sRowPtr )
                          != IDE_SUCCESS );
            }

            if( sResult == ID_TRUE )
            {
                //skip할 위치가 있는 경우, select .. from ..limit 3,10
                if( aIterator->mProperties->mFirstReadRecordPos == 0 )
                {
                    if( aIterator->mProperties->mReadRecordCount != 0 )
                    {
                        //읽어야 할 row의 갯수 감소
                        // ex. select .. from limit 100;
                        // replication에서 parallel sync.
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
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    if( sIsFetchSuccess == ID_TRUE )
    {
        SC_COPY_GRID( sGRID, aIterator->mRowGRID );
    }
    else
    {
        *aRow = NULL;
        SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: sdnpLockAllRows4RR
  - select for update, repeatable read를 위하여 테이블의 
  데이타 첫번째 페이지부터, 마지막 페이지까지 filter를 
  만족시키는  row들에 대하여 row-level lock을 다음과 같이 건다.

   1. row에 대하여 update가능한지 판단(sdcRecord::isUpdatable).
        >  skip flag가 올라오면 skip;
        >  retry가 올라오면 다시 데이타 페이지 latch를 잡고,
           update가능한지 판단.
        >  delete bit된 setting된 row이면 skip
   2.  filter적용하여 true이면 lock record( lock을 표현하는
       undo record생성및 rollptr 연결.
***********************************************************/
static IDE_RC sdnpLockAllRows4RR( sdnpIterator* aIterator)
{
    scGRID          sGRID;
    sdrMtx          sMtx;
    sdrSavePoint    sSvp;
    UChar         * sRowPtr;
    UChar         * sPagePtr;   
    idBool          sIsValidGRID;
    SInt            sState = 0;
    UInt            sFirstRead = aIterator->mProperties->mFirstReadRecordPos;
    UInt            sReadRecordCnt = aIterator->mProperties->mReadRecordCount;

    if( (aIterator->mProperties->mReadRecordCount == 0) ||
        (aIterator->mNxtRange == NULL) )
    {
        IDE_CONT( skip_no_more_row );
    }

    while( aIterator->mNxtRange != NULL )
    {
        sGRID = *((scGRID*)aIterator->mNxtRange->minimum.data);
        aIterator->mNxtRange = aIterator->mNxtRange->next;

        IDE_TEST( sdrMiniTrans::begin(
                                aIterator->mProperties->mStatistics,
                                &sMtx,
                                aIterator->mTrans,
                                SDR_MTX_LOGGING,
                                ID_FALSE,/*MtxUndoable(PROJ-2162)*/
                                SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE )
                != IDE_SUCCESS );
        sState = 1;

        sdrMiniTrans::setSavePoint( &sMtx, &sSvp );

        /* BUG-39674 : sdnpLockRow4RR에서 사용하기 위하여 out parameter로 
         * page포인터를 추가한다. */
        IDE_TEST( sdnpValidateAndGetPageByGRID(
                                    aIterator->mProperties->mStatistics,
                                    &sMtx,
                                    aIterator->mTBSNode,
                                    (smcTableHeader*)aIterator->mTable,
                                    sGRID,
                                    SDB_X_LATCH,
                                    &sRowPtr,
                                    &sPagePtr,
                                    &sIsValidGRID )
                  != IDE_SUCCESS );

        if( sIsValidGRID == ID_TRUE )
        {
            /* BUG-39674 : 함수의 인자로 sdnpValidateAndGetPageByGRID 에서 받은
             * page포인터를 추가한다. */
            IDE_TEST( sdnpLockRow4RR( aIterator,
                                      &sMtx,
                                      &sSvp,
                                      sGRID,
                                      sRowPtr,
                                      sPagePtr)
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

        IDE_TEST( iduCheckSessionEvent( aIterator->mProperties->mStatistics )
                != IDE_SUCCESS );

        if( aIterator->mProperties->mReadRecordCount == 0 )
        {
            goto skip_no_more_row;
        }
    }

    IDE_EXCEPTION_CONT( skip_no_more_row );

    IDE_ASSERT( sState == 0 );

    aIterator->mProperties->mFirstReadRecordPos = sFirstRead;
    aIterator->mProperties->mReadRecordCount    = sReadRecordCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::commit( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;
}

static IDE_RC sdnpLockRow4RR( sdnpIterator  * aIterator,
                              sdrMtx        * aMtx,
                              sdrSavePoint  * aSvp,
                              scGRID          aGRID,
                              UChar         * aRowPtr, 
                              UChar         * aPagePtr )
{
    sdrMtxStartInfo     sStartInfo;
    idBool              sIsRowDeleted;
    idBool              sIsPageLatchReleased = ID_FALSE;
    UChar             * sPagePtr;
    UChar             * sSlotDirPtr;
    SInt                sSlotSeq;
    UChar             * sSlotPtr;
    idBool              sResult;
    sdcUpdateState      sRetFlag;
    sdrMtx              sLogMtx;
    UChar               sCTSlotIdx = SDP_CTS_IDX_NULL;
    idBool              sSkipLockRec;
    idBool              sDummy;
    UInt                sState = 0;

    /* BUG-39674 : [aRow/aPage]Ptr 로 [sSlot/sPage]Ptr를 초기화 하고 sSlotSeq는
     * sSlotSeq는 SC_MAKE_SLOTNUM(aGRID)로 초기화한다. [aRow/aPage]Ptr는 이전에
     * 호출된 sdnpValidateAndGetPageByGRID함수에서 aGrid를 이용하여 구한 slot과
     * page의 pointer 이다.
     */
    sSlotPtr = aRowPtr;
    sPagePtr = aPagePtr;
    sSlotSeq = SC_MAKE_SLOTNUM(aGRID);

    IDE_ERROR( aIterator->mProperties->mLockRowBuffer != NULL ); // BUG-47758

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    /* MVCC scheme에서 내 버젼에 맞는 row를 가져옴. */
    IDE_TEST( sdcRow::fetch(
                    aIterator->mProperties->mStatistics,
                    aMtx,
                    aSvp,
                    aIterator->mTrans,
                    ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                    aRowPtr,
                    ID_TRUE, /* aIsPersSlot */
                    SDB_SINGLE_PAGE_READ,
                    aIterator->mProperties->mFetchColumnList,
                    SMI_FETCH_VERSION_LAST,
                    SD_NULL_SID,/* smLayerCallback::getTSSlotSID( aIterator->mTrans ) */
                    NULL,       /* &(aIterator->mSCN ) */
                    NULL,       /* &(aIterator->mInfinite) */
                    NULL,       /* aIndexInfo4Fetch */
                    NULL,       /* aLobInfo4Fetch */
                    ((smcTableHeader*)aIterator->mTable)->mRowTemplate,
                    aIterator->mProperties->mLockRowBuffer,
                    &sIsRowDeleted,
                    &sIsPageLatchReleased ) != IDE_SUCCESS );

    /* BUG-23319
     * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
    /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
     * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
     * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
     * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
     * 상황에 따라 적절한 처리를 해야 한다. */
    if( sIsPageLatchReleased == ID_TRUE )
    {
        IDE_TEST( sdbBufferMgr::getPageByPID(
                                        aIterator->mProperties->mStatistics,
                                        SC_MAKE_SPACE(aGRID),
                                        SC_MAKE_PID(aGRID),
                                        SDB_X_LATCH,
                                        SDB_WAIT_NORMAL,
                                        SDB_SINGLE_PAGE_READ,
                                        aMtx,
                                        &sPagePtr,
                                        &sDummy,
                                        NULL ) /* aIsCorruptPage */
                  != IDE_SUCCESS );

        sIsPageLatchReleased = ID_FALSE;

        /* BUG-32010 [sm-disk-collection] 'select for update' DRDB module
         * does not consider that the slot directory shift down caused 
         * by ExtendCTS.
         * PageLatch가 풀리는 동안 CTL 확장으로 SlotDirectory가 내려갈 수
         * 있다. */
        sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
       
        if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotSeq)
            == ID_TRUE )
        {
            IDE_CONT( skip_lock_row );
        }

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           sSlotSeq,
                                                           &sSlotPtr )
                  != IDE_SUCCESS );
    }

    /* BUG-45188
       빈 버퍼로 ST모듈 또는 MT모듈을 호출하는 것을 막는다.
       sResult = ID_TRUE 로 세팅해서, sdcRow::isDeleted() == ID_TRUE 로 skip 하도록 한다.
       ( 여기서 바로 skip 하면, row stamping을 못하는 경우가 있어서 위와같이 처리하였다.) */
    if ( sIsRowDeleted == ID_TRUE )
    {
        sResult = ID_TRUE;
    }
    else
    {
        IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                aIterator->mProperties->mLockRowBuffer,
                                                NULL,
                                                0,
                                                aGRID,
                                                aIterator->mFilter->data )
                  != IDE_SUCCESS );
    }

    if( sResult == ID_TRUE )
    {
        /* BUG-32010 [sm-disk-collection] 'select for update' DRDB module
            * does not consider that the slot directory shift down caused 
            * by ExtendCTS.
            * canUpdateRowPiece는 Filtering 이후에 해야 한다. 그렇지않으면
            * 전혀 상관 없는 Row에 대해 갱신 여부를 판단하다가 Wait하게
            * 된다. */

        IDE_TEST( sdcRow::canUpdateRowPiece(
                                aIterator->mProperties->mStatistics,
                                aMtx,
                                aSvp,
                                SC_MAKE_SPACE(aGRID),
                                SD_MAKE_SID_FROM_GRID(aGRID),
                                SDB_SINGLE_PAGE_READ,
                                &(aIterator->mSCN),
                                &(aIterator->mInfinite),
                                ID_FALSE, /* aIsUptLobByAPI */
                                (UChar**)&sSlotPtr,
                                &sRetFlag,
                                NULL, /* aCTSlotIdx */
                                aIterator->mProperties->mLockWaitMicroSec)
                  != IDE_SUCCESS );

        if( sRetFlag == SDC_UPTSTATE_REBUILD_ALREADY_MODIFIED )
        {
            /* 슬롯 정보 출력하고 releaseLatch */
            IDE_RAISE( rebuild_already_modified );
        }

        if( sRetFlag == SDC_UPTSTATE_INVISIBLE_MYUPTVERSION )
        {
            IDE_CONT( skip_lock_row );
        }

        // delete된 row이거나 insert 이전 버젼이면 skip
        if( sdcRow::isDeleted(sSlotPtr) == ID_TRUE )
        {
            IDE_CONT( skip_lock_row );
        }

        /*
         * BUG-25385 disk table인 경우, sm에서 for update문에 대한 '
         *           scan limit이 적용되지 않음. 
         */
        //skip할 위치가 있는 경우 ex)select .. from ..limit 3,10
        if( aIterator->mProperties->mFirstReadRecordPos == 0 )
        {
            if( aIterator->mProperties->mReadRecordCount != 0 )
            {
                //읽어야 할 row의 갯수 감소
                // ex) select .. from limit 100;
                aIterator->mProperties->mReadRecordCount--;
            }
        }
        else
        {
            aIterator->mProperties->mFirstReadRecordPos--;
        }

        /* BUG-45401 : undoable ID_FALSE -> ID_TRUE로 변경 */
        IDE_TEST( sdrMiniTrans::begin(
                                aIterator->mProperties->mStatistics,
                                &sLogMtx,
                                &sStartInfo,
                                ID_TRUE,/*MtxUndoable(PROJ-2162)*/
                                SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE )
                  != IDE_SUCCESS );
        sState = 1;

        if( sCTSlotIdx == SDP_CTS_IDX_NULL )
        {
            IDE_TEST( sdcTableCTL::allocCTS(
                                    aIterator->mProperties->mStatistics,
                                    aMtx,
                                    &sLogMtx,
                                    SDB_SINGLE_PAGE_READ,
                                    (sdpPhyPageHdr*)sPagePtr,
                                    &sCTSlotIdx ) != IDE_SUCCESS );
        }

        /* allocCTS()시에 CTL 확장이 발생하는 경우,
         * CTL 확장중에 compact page 연산이 발생할 수 있다.
         * compact page 연산이 발생하면
         * 페이지내에서 slot들의 위치(offset)가 변경될 수 있다.
         * 그러므로 allocCTS() 후에는 slot pointer를 다시 구해와야 한다. */
        /* BUG-32010 [sm-disk-collection] 'select for update' DRDB module
         * does not consider that the slot directory shift down caused 
         * by ExtendCTS. 
         * AllocCTS가 아니더라도, canUpdateRowPiece연산에 의해 LockWait
         * 에 빠질 경우도 PageLatch를 풀 수 있고, 이동안 CTL 확장이
         * 일어날 수 있다.*/
        sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           sSlotSeq,
                                                           &sSlotPtr )
                  != IDE_SUCCESS );

        // lock undo record를 생성한다.
        IDE_TEST( sdcRow::lock( aIterator->mProperties->mStatistics,
                                sSlotPtr,
                                SD_MAKE_SID_FROM_GRID(aGRID),
                                &(aIterator->mInfinite),
                                &sLogMtx,
                                sCTSlotIdx,
                                &sSkipLockRec )
                  != IDE_SUCCESS );

        if ( sSkipLockRec == ID_FALSE )
        {
            IDE_TEST( sdrMiniTrans::setDirtyPage( &sLogMtx, sPagePtr )
                      != IDE_SUCCESS );
        }

        sState = 0;
        IDE_TEST( sdrMiniTrans::commit( &sLogMtx ) != IDE_SUCCESS );
    }

    IDE_EXCEPTION_CONT( skip_lock_row );

    return IDE_SUCCESS;

    IDE_EXCEPTION( rebuild_already_modified );
    {
        if( aIterator->mStatement->isForbiddenToRetry() == ID_TRUE )
        {
            IDE_DASSERT( ((smxTrans*)aIterator->mTrans)->mIsGCTx == ID_TRUE );

            SChar   sMsgBuf[SMI_MAX_ERR_MSG_LEN];
            sdpCTS *sCTS;
            smSCN   sFSCNOrCSCN;
            UChar   sCTSlotIdx;
            sdcRowHdrInfo   sRowHdrInfo;
            sdcRowHdrExInfo sRowHdrExInfo;

            sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
            sCTSlotIdx = sRowHdrInfo.mCTSlotIdx;

            if ( SDC_HAS_BOUND_CTS(sCTSlotIdx) )
            {
                sCTS = sdcTableCTL::getCTS( sdpPhyPage::getHdr(sSlotPtr),sCTSlotIdx );
                SM_SET_SCN( &sFSCNOrCSCN, &sCTS->mFSCNOrCSCN );
            }
            else
            {
                sdcRow::getRowHdrExInfo( sSlotPtr, &sRowHdrExInfo );
                SM_SET_SCN( &sFSCNOrCSCN, &sRowHdrExInfo.mFSCNOrCSCN );
            }

            idlOS::snprintf( sMsgBuf,
                             SMI_MAX_ERR_MSG_LEN,
                             "[GRID SCAN VALIDATION(RR)] "
                             "SpaceID:%"ID_UINT32_FMT", "
                             "TableOID:%"ID_vULONG_FMT", "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "CSInfiniteSCN:%"ID_UINT64_FMT", "
                             "CTSlotIdx:%"ID_UINT32_FMT", "
                             "FSCNOrCSCN:%"ID_UINT64_FMT", "
                             "InfiniteSCN:%"ID_UINT64_FMT", "
                             "Deleted:%s ",
                             ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                             ((smcTableHeader*)aIterator->mTable)->mSelfOID,
                             SM_SCN_TO_LONG( aIterator->mSCN ),
                             SM_SCN_TO_LONG( aIterator->mInfinite ),
                             sCTSlotIdx,
                             SM_SCN_TO_LONG( sFSCNOrCSCN ),
                             SM_SCN_TO_LONG( sRowHdrInfo.mInfiniteSCN ),
                             SM_SCN_IS_DELETED( sRowHdrInfo.mInfiniteSCN)?"Y":"N" );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode( smERR_RETRY_Already_Modified ) );
        }

        IDE_ASSERT( sdcRow::releaseLatchForAlreadyModify( aMtx, aSvp )
                     == IDE_SUCCESS );
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sLogMtx ) == IDE_SUCCESS );
    }

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
    IDE_POP();

    return IDE_FAILURE;
}

static IDE_RC sdnpAllocIterator( void ** /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

static IDE_RC sdnpFreeIterator( void * /* aIteratorMem */ )
{
    return IDE_SUCCESS;
}

/**********************************************************************
 * Description: aIterator가 현재 가리키고 있는 Row에 대해서 XLock을
 *              획득합니다.
 *
 * aProperties - [IN] Index Iterator
 *
 * Related Issue:
 *   BUG-19068: smiTableCursor가 현재가리키고 있는 Row에 대해서
 *              Lock을 잡을수 잇는 Interface가 필요합니다.
 *
 *********************************************************************/
static IDE_RC sdnpLockRow( sdnpIterator* aIterator )
{
    return sdnManager::lockRow( aIterator->mProperties,
                                aIterator->mTrans,
                                &(aIterator->mSCN),
                                &(aIterator->mInfinite),
                                ((smcTableHeader*)aIterator->mTable)->mSpaceID,
                                SD_MAKE_RID_FROM_GRID(aIterator->mRowGRID),
                                aIterator->mStatement->isForbiddenToRetry() );
}

/*******************************************************************************
 * Description: DRDB에 대한 GRID가 유효한지 확인하는 함수
 *  
 * Parameters:        
 *  - aStatistics   [IN] idvSQL
 *  - aTableHdr     [IN] Fetch 대상 table의 header
 *  - aMtx          [IN] Fetch 대상 page를 getPage하여 검증하기 위한 mtx
 *  - aGRID         [IN] 검증 대상 GRID
 *  - aIsValidGRID  [OUT] 유효한 GRID인지 여부를 반환
 ******************************************************************************/
static IDE_RC sdnpValidateAndGetPageByGRID(
                                        idvSQL              * aStatistics,
                                        sdrMtx              * aMtx,
                                        sddTableSpaceNode   * aTBSNode,
                                        smcTableHeader      * aTableHdr,
                                        scGRID                aGRID,
                                        sdbLatchMode          aLatchMode,
                                        UChar              ** aRowPtr,
                                        UChar              ** aPagePtr,
                                        idBool              * aIsValidGRID )
{
    scSpaceID           sSpaceID;
    scPageID            sPageID;
    scSlotNum           sSlotNum;
    sddDataFileNode   * sFileNode;
    sdrSavePoint        sSvp;
    UChar             * sPagePtr;
    sdpPhyPageHdr     * sPageHdr;
    UChar             * sSlotDir;
    UInt                sSlotCount;
    UChar             * sSlotPtr = NULL;
    SInt                sState = 0;

    *aIsValidGRID = ID_FALSE;

    /* 읽을 수 있는 GRID인지 검사 */
    IDE_TEST_CONT( SC_GRID_IS_NULL(aGRID), error_invalid_grid );
    IDE_TEST_CONT( SC_GRID_IS_NOT_WITH_SLOTNUM(aGRID) , error_invalid_grid );

    sSpaceID = SC_MAKE_SPACE(aGRID);
    sPageID  = SC_MAKE_PID(aGRID);
    sSlotNum = SC_MAKE_SLOTNUM(aGRID);

    /* GRID와 iterator의 SpaceID 일치 검사 */
    IDE_TEST_CONT( sSpaceID != aTableHdr->mSpaceID,
                    error_invalid_grid );

    /* GRID의 PageID가 유효한 PageID 범위 안인지 확인 */
    IDE_TEST( sddTableSpace::getDataFileNodeByPageID( aTBSNode,
                                                      sPageID,
                                                      &sFileNode,
                                                      ID_FALSE ) /* aFatal */
              != IDE_SUCCESS );

    if( aMtx != NULL )
    {
        sdrMiniTrans::setSavePoint( aMtx, &sSvp );
    }

    /* Page를 읽어서 TableOID가 일치하는지 확인 */
    IDE_TEST( sdbBufferMgr::getPageByPID( aStatistics,
                                          sSpaceID,
                                          sPageID,
                                          aLatchMode,
                                          SDB_WAIT_NORMAL,
                                          SDB_SINGLE_PAGE_READ,
                                          aMtx,
                                          (UChar**)&sPagePtr,
                                          NULL, /*TrySuccess*/
                                          NULL  /*IsCorruptPage*/ )
              != IDE_SUCCESS );
    sState = 1;

    sPageHdr = (sdpPhyPageHdr*)sPagePtr;

    IDE_TEST_CONT( sPageHdr->mTableOID != aTableHdr->mSelfOID,
                    error_invalid_grid );

    /* Data page가 맞는지 확인 */
    IDE_TEST_CONT( sdpPhyPage::getPageType(sPageHdr) != SDP_PAGE_DATA,
                    error_invalid_grid );

    /* GRID의 SlotNum이 page의 slot count 미만인지 확인 */ 
    sSlotDir   = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
    sSlotCount = sdpSlotDirectory::getCount(sSlotDir);

    IDE_TEST_CONT( sSlotCount <= sSlotNum, error_invalid_grid );

    IDE_TEST_CONT( sdpSlotDirectory::isUnusedSlotEntry(sSlotDir, sSlotNum)
                    == ID_TRUE, error_invalid_grid )

    /* slot pointer를 얻어서 out parameter에 설정해 준다. */
    IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDir,
                                                       SC_MAKE_SLOTNUM(aGRID),
                                                       &sSlotPtr)
              != IDE_SUCCESS );

    IDE_TEST_CONT( sdcRow::isHeadRowPiece(sSlotPtr) != ID_TRUE,
                    error_invalid_grid );

    /* 모든 검사를 통과하였으므로 out parameter에 slot pointer 할당해서 넘김. */
    *aRowPtr = sSlotPtr;
    *aIsValidGRID = ID_TRUE;
    
    /* BUG-39674 : out parameter에 Page pointer 할당해서 넘김. 
     * 이 함수 ( sdnpValidateAndGetPageByGRID ) 를 호출하는 함수중 smnpFetchNext
     * 에서는 page pointer를 넘길 필요가 없기 때문에 해당 인자를 NULL로 받음 
     */  
    if( aPagePtr != NULL )
    {
         *aPagePtr = sPagePtr;
    }
    else 
    {
        /* do nothing */
    }

    IDE_EXCEPTION_CONT( error_invalid_grid );

    if( *aIsValidGRID == ID_FALSE )
    {
        if( sState == 1 )
        {
            sState = 0;

            if( aMtx == NULL )
            {
                IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                                    sPagePtr )
                          != IDE_SUCCESS );
            }
            else
            {
                IDE_TEST( sdrMiniTrans::releaseLatchToSP( aMtx, &sSvp )
                          != IDE_SUCCESS );
            }
        }

        *aRowPtr = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if( sState == 1 )
    {
        if( aMtx == NULL )
        {
            IDE_ASSERT( sdbBufferMgr::releasePage( NULL, /* idvSQL */
                                                   sPagePtr )
                        == IDE_SUCCESS );
        }
        else
        {
            IDE_ASSERT( sdrMiniTrans::releaseLatchToSP( aMtx, &sSvp )
                        == IDE_SUCCESS );
        }
    }

    return IDE_FAILURE;
}


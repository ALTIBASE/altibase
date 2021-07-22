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
 * $Id: sdnnModule.cpp 89963 2021-02-09 05:22:10Z justin.kwon $
 **********************************************************************/

/**************************************************************
 * FILE DESCRIPTION : sdnnModule.cpp                          *
 * -----------------------------------------------------------*
   disk table에대한 sequential iterator모듈이며, 아래와 같은
   interface제공한다.
   - beforeFirst
   - afterLast
   - fetchNext
   - fetchPrevious

  << 각 inteface의 개요 >>
   > beforeFirst
    - table의 데이타 첫번째 페이지와 keymap sequence는 SDP_KEYMAP_MIN_NUM를
      iterator에 저장한다.
      이는 fetchNext에서 keymap sequence를 증가시키는
      semantic을 지키기 위함이다.

   > afterLast
    table의 데이타 마지막  페이지와 keymap sequence는
    keymap 갯수를 iterator에 저장한다.
    이는 fetchPrevious에서 keymap sequence를 감소시키는
    semantic을 지키기 위함이다.

  > FetchNext
    iterator에 저장된  keymap sequence 전진시켜서,
   row의 getValidVersion을 구하고  filter적용한후, true이면
   해당 row의 버젼을 aRow에  copy하고 iterator에 위치를 저장한다.

  > FetchPrevious
   iterator가 저장한 keymap sequence 를 후진시켜서
    row의 getValidVersion을 구하고  filter적용한후, true이면
   해당 row의 버젼을 aRow에  copy하고 iterator에 위치를 저장한다.

   R로 postfix붙은 interface는 repeatable read or select for update
  를 위하여 row level lock을  먼저 잡은 역할을 수행한다.

   U로 postfix붙은 SMI_PREVIOUS_ENABLE(Scrollable Cursor)이며
   write lock 혹은 Table  x lock 일 경우에 사용된다.
   한 Row에 대해 한 update cursor가
   여러번 반복하여 변경할 수 있기 때문에
   자신이 update한 row이면 이 row에 대하여 filter
  를 적용하고,  그렇지 않은 경우에는 자신의 view에 맞는
  버전을 만들어서 filter를 적용한다.
  filter를 만족하면 해당 version이나 row를 반환하게 된다.

  << A3의 sequential iterator와 차이점 >>
  % A4 disk 의 MVCC는 Inplace-update scheme이기때문에
  A3와 같은 iterator의 row cache를 iterator에 적용하지 않는다.
  ex)
  만약 하나의 page에서 1000개가 filter에 맞는  row를 먼저
  beforFirst시에 row를 cache하려면,
  1000번의 getValidVersion + 1000번의 filter적용.
  그리고 fetchNext에서도 inplace update MVCC이기 때문에,
  1000번의 getValidVersion을 호출하여 내 iterator에
  버젼에 맞는 row를 생성해야 한다.
  그래서 2000번의 getValidVersion+ 1000번의 filter를 호출해야한다.
  row cache를 안하면,   1000번의 getValidation+ 1000번의 filter를 호출하게
  되어 row cache를 하는 경우보다 오히려 비용이 덜 든다.

 **************************************************************/
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
#include <sdnnDef.h>
#include <sdn.h>
#include <sdnReq.h>

static iduMemPool sdnnCachePool;

static IDE_RC sdnnPrepareIteratorMem( const smnIndexModule* );

static IDE_RC sdnnReleaseIteratorMem(const smnIndexModule* );

static IDE_RC sdnnInit( sdnnIterator        * aIterator,
                        void                * aTrans,
                        smcTableHeader      * aTable,
                        void                * /* aIndexHeader */,
                        void                * /* aDumpObject */,
                        const smiRange      * /* aKeyRange */,
                        const smiRange      * /* aKeyFilter */,
                        const smiCallBack   * aFilter,
                        UInt                  aFlag,
                        smSCN                 aSCN,
                        smSCN                 aInfinite,
                        idBool                /* aUntouchable */,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc,
                        smiStatement        * aStatement );

static IDE_RC sdnnDest( sdnnIterator * aIterator );

static IDE_RC sdnnNA           ( void );
static IDE_RC sdnnAA           ( sdnnIterator* aIterator,
                                 const void**  aRow );

static void sdnnMakeRowCache( sdnnIterator  *aIterator,
                                UChar         *aPagePtr );

static IDE_RC sdnnBeforeFirst  ( sdnnIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

static IDE_RC sdnnBeforeFirstW ( sdnnIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

static IDE_RC sdnnBeforeFirstRR( sdnnIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

static IDE_RC sdnnAfterLast    ( sdnnIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

static IDE_RC sdnnAfterLastW   ( sdnnIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

static IDE_RC sdnnAfterLastRR  ( sdnnIterator*       aIterator,
                                 const smSeekFunc** aSeekFunc );

static IDE_RC sdnnFetchNext    ( sdnnIterator*       aIterator,
                                 const void**        aRow );

static IDE_RC sdnnLockAllRows4RR   ( sdnnIterator*       aIterator );

static IDE_RC sdnnAllocIterator( void ** aIteratorMem );

static IDE_RC sdnnFreeIterator( void * aIteratorMem );

static IDE_RC sdnnLockRow( sdnnIterator* aIterator );

static IDE_RC sdnnFetchAndCheckFilter( sdnnIterator * aIterator,
                                       UChar        * aSlot,
                                       sdSID          aSlotSID,
                                       UChar        * aDestBuf,
                                       scGRID       * aRowGRID,
                                       idBool       * aResult );

static IDE_RC sdnnFetchNextAtPage( sdnnIterator * aIterator,
                                   UChar        * aDestRowBuf,
                                   idBool       * aIsNoMoreRow );

static IDE_RC sdnnLockAllRowsAtPage4RR( sdrMtx       * aMtx,
                                        sdrSavePoint * aSP,
                                        sdnnIterator * aIterator );

static IDE_RC sdnnMoveToNxtPage4RR( sdnnIterator * aIterator );

static IDE_RC sdnnMoveToNxtPage( sdnnIterator * aIterator );

static IDE_RC sdnnAging( idvSQL         * aStatistics,
                         void           * /*aTrans*/,
                         smcTableHeader * aHeader,
                         smnIndexHeader * /*aIndex*/);

static IDE_RC sdnnGatherStat( idvSQL         * aStatistics,
                              void           * aTrans,
                              SFloat           aPercentage,
                              SInt             aDegree,
                              smcTableHeader * aHeader,
                              void           * aTotalTableArg,
                              smnIndexHeader * aIndex,
                              void           * aStats,
                              idBool           aDynamicMode );

IDE_RC sdnnAnalyzePage4GatherStat( idvSQL             * aStatistics,
                                   void               * aTrans,
                                   smcTableHeader     * aTableHeader,
                                   smiFetchColumnList * aFetchColumnList,
                                   scPageID             aPageID,
                                   UChar              * aRowBuffer,
                                   void               * aTableArgument,
                                   void               * aTotalTableArgument );

smnIndexModule sdnnModule = {
    SMN_MAKE_INDEX_MODULE_ID( SMI_TABLE_DISK,
                              SMI_BUILTIN_SEQUENTIAL_INDEXTYPE_ID ),
    SMN_RANGE_DISABLE|SMN_DIMENSION_DISABLE,
    ID_SIZEOF( sdnnIterator ),
    (smnMemoryFunc) sdnnPrepareIteratorMem,
    (smnMemoryFunc) sdnnReleaseIteratorMem,
    (smnMemoryFunc) NULL,
    (smnMemoryFunc) NULL,
    (smnCreateFunc) NULL,
    (smnDropFunc)   NULL,

    (smTableCursorLockRowFunc) sdnnLockRow,
    (smnDeleteFunc)            NULL,
    (smnFreeFunc)              NULL,
    (smnInsertRollbackFunc)    NULL,
    (smnDeleteRollbackFunc)    NULL,
    (smnAgingFunc)             sdnnAging,

    (smInitFunc) sdnnInit,
    (smDestFunc) sdnnDest,
    (smnFreeNodeListFunc) NULL,
    (smnGetPositionFunc) NULL,
    (smnSetPositionFunc) NULL,

    (smnAllocIteratorFunc) sdnnAllocIterator,
    (smnFreeIteratorFunc)  sdnnFreeIterator,
    (smnReInitFunc)         NULL,
    (smnInitMetaFunc)       NULL,
    (smnMakeDiskImageFunc)  NULL,
    (smnBuildIndexFunc)     NULL,
    (smnGetSmoNoFunc)       NULL,
    (smnMakeKeyFromRow)     NULL,
    (smnMakeKeyFromSmiValue)NULL,
    (smnRebuildIndexColumn) NULL,
    (smnSetIndexConsistency)NULL,
    (smnGatherStat)         sdnnGatherStat

};

static const smSeekFunc sdnnSeekFunctions[32][12] =
{ { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirstW,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATALBE      */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirstRR,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirstW,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA, // sdnnFetchPrevW,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATALBE      */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnBeforeFirstRR,
        (smSeekFunc)sdnnAfterLastRR,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_FORWARD  SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnNA, // sdnnFetchPrevU,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_READ            */
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_WRITE           */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLastW,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_REPEATABLE      */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLastRR,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_DISABLE SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLastW,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_READ            */
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_WRITE           */
        (smSeekFunc)sdnnNA, // sdnnFetchPrevU,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_REPEATABLE      */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAfterLastRR,
        (smSeekFunc)sdnnBeforeFirstRR,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_SHARED    */
        (smSeekFunc)sdnnNA, // sdnnFetchPrev,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* SMI_TRAVERSE_BACKWARD SMI_PREVIOUS_ENABLE  SMI_LOCK_TABLE_EXCLUSIVE */
        (smSeekFunc)sdnnNA, // sdnnFetchPrevU,
        (smSeekFunc)sdnnFetchNext,
        (smSeekFunc)sdnnAfterLast,
        (smSeekFunc)sdnnBeforeFirst,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnAA,
        (smSeekFunc)sdnnAA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    },
    { /* UNUSED                                                              */
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA,
        (smSeekFunc)sdnnNA
    }
};

static IDE_RC sdnnPrepareIteratorMem( const smnIndexModule* )
{

    UInt  sIteratorMemoryParallelFactor
        = smuProperty::getIteratorMemoryParallelFactor();

    /* BUG-26647 Disk row cache */
    IDE_TEST(sdnnCachePool.initialize( IDU_MEM_SM_SDN,
                                       (SChar*)"SDNN_CACHE_POOL",
                                       sIteratorMemoryParallelFactor,
                                       SDNN_ROWS_CACHE,         /* Cache Size */
                                       256,                     /* Pool Size */
                                       IDU_AUTOFREE_CHUNK_LIMIT,
                                       ID_TRUE,                 /* UseMutex */
                                       SD_PAGE_SIZE,            /* Align Size */
                                       ID_FALSE,                /* ForcePooling */
                                       ID_TRUE,                 /* GarbageCollection */
                                       ID_TRUE,                 /* HWCacheLine */
                                       IDU_MEMPOOL_TYPE_TIGHT ) /* mempool type */
             != IDE_SUCCESS);

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

static IDE_RC sdnnReleaseIteratorMem(const smnIndexModule* )
{
    IDE_TEST( sdnnCachePool.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
  function description: sdnninit
  -  disk table에 대한 sequential iterator구조체 초기화.
   : table header및 기타.
   : filter callback function assign
   : seekFunction assign(FetchNext, FetchPrevious....)
  added FOR A4
***********************************************************/
static IDE_RC sdnnInit( sdnnIterator        * aIterator,
                        void                * aTrans,
                        smcTableHeader      * aTable,
                        void                * /* aIndexHeader */,
                        void                * /* aDumpObject */,
                        const smiRange      * /* aKeyRange */,
                        const smiRange      * /* aKeyFilter */,
                        const smiCallBack   * aFilter,
                        UInt                  aFlag,
                        smSCN                 aSCN,
                        smSCN                 aInfinite,
                        idBool                /* aUntouchable */,
                        smiCursorProperties * aProperties,
                        const smSeekFunc   ** aSeekFunc,
                        smiStatement        * aStatement )
{
    idvSQL            *sSQLStat;
    sdpPageListEntry  *sPageListEntry;

    SM_SET_SCN( &aIterator->mSCN, &aSCN );
    SM_SET_SCN( &(aIterator->mInfinite), &aInfinite );
    aIterator->mTrans           = aTrans;
    aIterator->mTable           = aTable;
    aIterator->mCurRecPtr       = NULL;
    aIterator->mLstFetchRecPtr  = NULL;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );
    aIterator->mTid             = smLayerCallback::getTransID( aTrans );
    aIterator->mFilter          = aFilter;
    aIterator->mProperties      = aProperties;

    aIterator->mCurSlotSeq      = SDP_SLOT_ENTRY_MIN_NUM;
    aIterator->mCurPageID       = SD_NULL_PID;
    aIterator->mStatement       = aStatement;

    *aSeekFunc = sdnnSeekFunctions[ aFlag&( SMI_TRAVERSE_MASK |
                                            SMI_PREVIOUS_MASK |
                                            SMI_LOCK_MASK ) ];

    sSQLStat = aIterator->mProperties->mStatistics;
    IDV_SQL_ADD(sSQLStat, mDiskCursorSeqScan, 1);

    if (sSQLStat != NULL)
    {
        IDV_SESS_ADD(sSQLStat->mSess,
                     IDV_STAT_INDEX_DISK_CURSOR_SEQ_SCAN, 1);
    }

    // page list entry를 구함.
    sPageListEntry = &(((smcTableHeader*)aIterator->mTable)->mFixed.mDRDB);

    // table의 space id 를 구함.
    aIterator->mSpaceID = ((smcTableHeader*)aIterator->mTable)->mSpaceID;
    aIterator->mSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aIterator->mSpaceID );

    // PROJ-2402
    IDE_ASSERT( aIterator->mProperties->mParallelReadProperties.mThreadCnt != 0 );

    if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt > 1 )
    {
        IDE_TEST( aIterator->mMPRMgr.initialize(
                      aIterator->mProperties->mStatistics,
                      aIterator->mSpaceID,
                      sdpSegDescMgr::getSegHandle( sPageListEntry ),
                      sdbMPRMgr::filter4PScan )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( aIterator->mMPRMgr.initialize(
                      aIterator->mProperties->mStatistics,
                      aIterator->mSpaceID,
                      sdpSegDescMgr::getSegHandle( sPageListEntry ),
                      NULL ) /* Filter */
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC sdnnDest( sdnnIterator * aIterator )
{
    IDE_TEST( aIterator->mMPRMgr.destroy()
             != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC sdnnAA( sdnnIterator * /* aIterator */,
                      const void**   /* */)
{

    return IDE_SUCCESS;

}

IDE_RC sdnnNA( void )
{

    IDE_SET( ideSetErrorCode( smERR_ABORT_smiTraverseNotApplicable ) );

    return IDE_FAILURE;

}

/******************************************************************************
 * Description: Iterator에 aPagePtr이 가리키는 페이지를 caching 한다.
 *
 * Parameters:
 *  aIterator       - [IN]  Row Cache를 생성할
 *  aPagePtr        - [IN]  Caching할 대상 페이지의 시작 주소
 *****************************************************************************/
void sdnnMakeRowCache( sdnnIterator * aIterator,
                       UChar        * aPagePtr )
{
    IDE_DASSERT( aPagePtr == sdpPhyPage::getPageStartPtr( aPagePtr ) );

    /* 대상 Page를 Iterator Row Cache에 복사한다. */
    idlOS::memcpy( aIterator->mRowCache,
                   aPagePtr,
                   SD_PAGE_SIZE );

    return;
}


/*********************************************************
  function description: sdnnBeforeFirst
  table의 데이타 첫번째 페이지와 keymap sequence는 SDP_SLOT_ENTRY_MIN_NUM를
  iterator에 저장한다.
  이는 fetchNext에서 keymap sequence를 증가시키는
  semantic을 지키기 위함이다.
  -  added FOR A4
***********************************************************/
static IDE_RC sdnnBeforeFirst( sdnnIterator *       aIterator,
                               const smSeekFunc ** /* aSeekFunc */)
{
    aIterator->mCurPageID = SM_NULL_PID;

    IDE_TEST( aIterator->mMPRMgr.beforeFst() != IDE_SUCCESS );

    // SDP_SLOT_ENTRY_MIN_NUM으로 저장해 fetchNext에서 0번째 keymap부터
    // 검사하도록 한다.
    aIterator->mCurSlotSeq   = SDP_SLOT_ENTRY_MIN_NUM;

    IDE_TEST( sdnnMoveToNxtPage( aIterator ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: sdnnBeforeFirstW
  sdnnBeforeFirst를 적용하고 smSeekFunction을 전진시켜
  다음에는 다른 callback이 불리도록 한다.

  -  added FOR A4
***********************************************************/
static IDE_RC sdnnBeforeFirstW( sdnnIterator*       aIterator,
                                const smSeekFunc** aSeekFunc )
{

    IDE_TEST( sdnnBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
  function description: sdnnBeforeFirstR

  - sdnnBeforeFirst를 적용하고  lockAllRows4RR을 불러서,
  filter를 만족시키는  row에 대하여 lock을 표현하는 undo record
  를 생성하고 rollptr를 연결시킨다.

  - sdnnBeforeFirst를 다시 적용한다.

  다음번에 호출될 경우에는  sdnnBeforeFirstR이 아닌
  다른 함수가 불리도록 smSeekFunction의 offset을 6전진시킨다.
  -  added FOR A4
***********************************************************/
static IDE_RC sdnnBeforeFirstRR( sdnnIterator *       aIterator,
                                 const smSeekFunc ** aSeekFunc )
{
    IDE_TEST( sdnnBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    // select for update를 위하여 lock을 나타내는 undo record들 생성.
    IDE_TEST( sdnnLockAllRows4RR( aIterator ) != IDE_SUCCESS );

    IDE_TEST( sdnnBeforeFirst( aIterator, aSeekFunc ) != IDE_SUCCESS );

    *aSeekFunc += 6;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

}

/*********************************************************
  function description: sdnnAfterLast
  table의 데이타 마지막  페이지와 keymap sequence는
  keymap 갯수를 iterator에 저장한다.
  이는 fetchPrevious에서 keymap sequence를 감소시키는
  semantic을 지키기 위함이다.
  -  added FOR A4
***********************************************************/
static IDE_RC sdnnAfterLast( sdnnIterator *      /* aIterator */,
                             const smSeekFunc ** /* aSeekFunc */)
{
    return IDE_FAILURE;
}

/*********************************************************
  function description: sdnnAferLastU
  sdnnAfterLast를 호출하고, 다음번에 다른 callback함수가
  불리도록 callback함수를 조정한다.
***********************************************************/
static IDE_RC sdnnAfterLastW( sdnnIterator *       /* aIterator */,
                              const smSeekFunc **  /* aSeekFunc */ )
{
    return IDE_FAILURE;
}

/*********************************************************
  function description: sdnnAferLastR
  sdnnBeforeFirst한후, lockAllRows4RR을 호출하면서
  record lock을 나타내는 undo record를 생성한다.
  lockAllRows4RR을 적용하면서 , 오른쪽끝에 도달하기 때문에
  iterator의 beforeFirst하고 나서의 iterator를 복구할 필요는
  없다.
***********************************************************/
static IDE_RC sdnnAfterLastRR( sdnnIterator *       /* aIterator */,
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
 *  aSlotSID        - [IN]  fetch 대상 row의 SID
 *  aDestBuf        - [OUT] fetch 해 온 row가 저장될 buffer
 *  aRowGRID        - [OUT] 대상 row의 GRID
 *  aResult         - [OUT] fetch 결과
 ******************************************************************************/
static IDE_RC sdnnFetchAndCheckFilter( sdnnIterator * aIterator,
                                       UChar        * aSlot,
                                       sdSID          aSlotSID,
                                       UChar        * aDestBuf,
                                       scGRID       * aRowGRID,
                                       idBool       * aResult )
{
    scGRID           sSlotGRID;
    idBool           sIsRowDeleted;
    idBool           sDummy;
    smcTableHeader * sTableHeader;

    IDE_DASSERT( aIterator != NULL );
    IDE_DASSERT( aSlot     != NULL );
    IDE_DASSERT( aDestBuf  != NULL );
    IDE_DASSERT( aRowGRID  != NULL );
    IDE_DASSERT( aResult   != NULL );
    IDE_DASSERT( sdcRow::isHeadRowPiece(aSlot) == ID_TRUE );

    *aResult      = ID_FALSE;
    sTableHeader  = (smcTableHeader*)aIterator->mTable; 
    SC_MAKE_NULL_GRID( *aRowGRID );

    /* MVCC scheme에서 내 버젼에 맞는 row를 가져옴. */
    IDE_TEST( sdcRow::fetch(
                  aIterator->mProperties->mStatistics,
                  NULL, /* aMtx */
                  NULL, /* aSP */
                  aIterator->mTrans,
                  aIterator->mSpaceID,
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
                  sTableHeader->mRowTemplate,
                  aDestBuf,
                  &sIsRowDeleted,
                  &sDummy ) != IDE_SUCCESS );

    SC_MAKE_GRID_WITH_SLOTNUM( sSlotGRID,
                               aIterator->mSpaceID,
                               SD_MAKE_PID( aSlotSID ),
                               SD_MAKE_SLOTNUM( aSlotSID ) );

    // delete된 row이거나 insert 이전 버젼이면 skip
    IDE_TEST_CONT( sIsRowDeleted == ID_TRUE, skip_deleted_row );

    // filter적용;
    IDE_TEST( aIterator->mFilter->callback( aResult,
                                            aDestBuf,
                                            NULL,
                                            0,
                                            sSlotGRID,
                                            aIterator->mFilter->data )
              != IDE_SUCCESS );

    SC_COPY_GRID( sSlotGRID, *aRowGRID );

    IDE_EXCEPTION_CONT( skip_deleted_row );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    *aResult = ID_FALSE;

    return IDE_FAILURE;
}

/*******************************************************************************
 * Description: Iterator의 캐쉬 페이지에서 row를 읽어들여 fetch 한다.
 *
 * Parameters:
 *  aIterator       - [IN]  Iterator의 포인터
 *  aDestRowBuf     - [OUT] fetch한 row가 저장될 buffer
 *  aIsNoMoreRow    - [OUT] fetch한 row의 GRID
 ******************************************************************************/
static IDE_RC sdnnFetchNextAtPage( sdnnIterator * aIterator,
                                   UChar        * aDestRowBuf,
                                   idBool       * aIsNoMoreRow )
{
    idBool             sResult;
    UShort             sSlotSeq;
    UShort             sSlotCount;
    UChar            * sPagePtr;
    sdpPhyPageHdr    * sPageHdr;
    UChar            * sSlotDirPtr;
    UChar            * sSlotPtr;
    scGRID             sRowGRID;
    sdSID              sSlotSID;

    IDE_ASSERT( aIterator->mCurPageID != SD_NULL_PID );

    *aIsNoMoreRow = ID_TRUE;

    /* BUG-26647 - Caching 해 둔 페이지로부터 row를 fetch한다. */
    sPagePtr    = aIterator->mRowCache;
    sPageHdr    = sdpPhyPage::getHdr(sPagePtr);
    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr(sPagePtr);
    sSlotCount  = sdpSlotDirectory::getCount(sSlotDirPtr);

    sSlotSeq = aIterator->mCurSlotSeq;

    while(1)
    {
        sSlotSeq++;

        if( sSlotSeq >= sSlotCount )
        {
            break;
        }

        if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotSeq)
            == ID_TRUE )
        {
            continue;
        }

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           sSlotSeq,
                                                           &sSlotPtr )
                  != IDE_SUCCESS );
        if( sdcRow::isHeadRowPiece(sSlotPtr) != ID_TRUE )
        {
            continue;
        }

        sSlotSID = SD_MAKE_SID( sPageHdr->mPageID,
                                sSlotSeq );

        /* Fetch가 실패하였을때, Tolerance가 0일 경우에만 예외처리한다.
         * Tolerance가 1일 경우, sResult가 false이기 때문에 그냥
         * 진행한다. */
        if( sdnnFetchAndCheckFilter( aIterator,
                                     sSlotPtr,
                                     sSlotSID,
                                     aDestRowBuf,
                                     &sRowGRID,
                                     &sResult ) == IDE_SUCCESS )
        {
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
                        aIterator->mCurSlotSeq   = sSlotSeq;

                        SC_COPY_GRID( sRowGRID, aIterator->mRowGRID );
                        *aIsNoMoreRow = ID_FALSE;

                        break;
                    }
                }
                else
                {
                    aIterator->mProperties->mFirstReadRecordPos--;
                }//else
            } //if sResult
        }
        else
        {
            /* 0, exception */
            IDE_TEST( smuProperty::getCrashTolerance() == 0 );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

static IDE_RC sdnnLockAllRowsAtPage4RR( sdrMtx       * aMtx,
                                        sdrSavePoint * aSP,
                                        sdnnIterator * aIterator )
{
    SInt                   sSlotSeq;
    idBool                 sResult;
    SInt                   sSlotCount;
    UChar                 *sSlotDirPtr;
    UChar                 *sSlot;
    idBool                 sSkipLockRec;
    sdSID                  sSlotSID;
    sdrMtx                 sLogMtx;
    sdrMtxStartInfo        sStartInfo;
    UInt                   sState = 0;
    idBool                 sIsPageLatchReleased = ID_FALSE;
    UChar                 *sPage;
    UChar                  sCTSlotIdx = SDP_CTS_IDX_NULL;
    scGRID                 sSlotGRID;
    sdcUpdateState         sRetFlag;
    idBool                 sIsRowDeleted;
    idBool                 sDummy;
    smcTableHeader        *sTableHeader;

    sTableHeader  = (smcTableHeader*)aIterator->mTable; 

    IDE_ASSERT( aIterator->mCurPageID != SD_NULL_PID );
    IDE_ERROR( aIterator->mProperties->mLockRowBuffer != NULL );  // BUG-47758

    sdrMiniTrans::makeStartInfo( aMtx, &sStartInfo );

    IDE_TEST( sdbBufferMgr::getPageByPID( aIterator->mProperties->mStatistics,
                                          aIterator->mSpaceID,
                                          aIterator->mCurPageID,
                                          SDB_X_LATCH,
                                          SDB_WAIT_NORMAL,
                                          SDB_MULTI_PAGE_READ,
                                          aMtx,
                                          &sPage,
                                          &sDummy,
                                          NULL ) /* aIsCorruptPage */
              != IDE_SUCCESS );
    
    IDE_TEST_CONT( sdpPhyPage::getPageType((sdpPhyPageHdr*)sPage) != SDP_PAGE_DATA,
                    skip_not_data_page );

    /*
     * BUG-32942 When executing rebuild Index stat, abnormally shutdown
     * 
     * Free pages should be skiped. Otherwise index can read
     * garbage value, and server is shutdown abnormally.
     */
    IDE_TEST_CONT( aIterator->mSegMgmtOp->mIsFreePage(sPage) == ID_TRUE,
                    skip_not_data_page );

    sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(sPage);
    sSlotCount   = sdpSlotDirectory::getCount(sSlotDirPtr);

    for( sSlotSeq = 0; sSlotSeq < sSlotCount; sSlotSeq++ )
    {
        if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotSeq)
            == ID_TRUE )
        {
            continue;
        }

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           sSlotSeq,
                                                           &sSlot )
                  != IDE_SUCCESS );

        if( sdcRow::isHeadRowPiece(sSlot) != ID_TRUE )
        {
            continue;
        }

        sSlotSID = SD_MAKE_SID( ((sdpPhyPageHdr*)sPage)->mPageID,
                                sSlotSeq );

        /* MVCC scheme에서 내 버젼에 맞는 row를 가져옴. */
        IDE_TEST( sdcRow::fetch( aIterator->mProperties->mStatistics,
                                 aMtx,
                                 aSP,
                                 aIterator->mTrans,
                                 aIterator->mSpaceID,
                                 sSlot,
                                 ID_TRUE, /* aIsPersSlot */
                                 SDB_MULTI_PAGE_READ,
                                 aIterator->mProperties->mFetchColumnList,
                                 SMI_FETCH_VERSION_LAST,
                                 SD_NULL_SID, /* smLayerCallback::getTSSlotSID( aIterator->mTrans ) */
                                 NULL,        /* &(aIterator->mSCN ) */
                                 NULL,        /* &(aIterator->mInfinite) */
                                 NULL,        /* aIndexInfo4Fetch */
                                 NULL,        /* aLobInfo4Fetch */
                                 sTableHeader->mRowTemplate,
                                 aIterator->mProperties->mLockRowBuffer,
                                 &sIsRowDeleted,
                                 &sIsPageLatchReleased ) 
                   != IDE_SUCCESS );

        /* BUG-23319
         * [SD] 인덱스 Scan시 sdcRow::fetch 함수에서 Deadlock 발생가능성이 있음. */
        /* row fetch를 하는중에 next rowpiece로 이동해야 하는 경우,
         * 기존 page의 latch를 풀지 않으면 deadlock 발생가능성이 있다.
         * 그래서 page latch를 푼 다음 next rowpiece로 이동하는데,
         * 상위 함수에서는 page latch를 풀었는지 여부를 output parameter로 확인하고
         * 상황에 따라 적절한 처리를 해야 한다. */
        if( sIsPageLatchReleased == ID_TRUE )
        {
            IDE_TEST( sdbBufferMgr::getPageByPID( aIterator->mProperties->mStatistics,
                                                  aIterator->mSpaceID,
                                                  aIterator->mCurPageID,
                                                  SDB_X_LATCH,
                                                  SDB_WAIT_NORMAL,
                                                  SDB_MULTI_PAGE_READ,
                                                  aMtx,
                                                  &sPage,
                                                  &sDummy,
                                                  NULL ) /* aIsCorruptPage */
                      != IDE_SUCCESS );
            sIsPageLatchReleased = ID_FALSE;

            /* BUG-32010 [sm-disk-collection] 'select for update' DRDB module
             * does not consider that the slot directory shift down caused 
             * by ExtendCTS.
             * PageLatch가 풀리는 동안 CTL 확장으로 SlotDirectory가 내려갈 수
             * 있다. */
            sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(sPage);

            if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotSeq)
                == ID_TRUE )
            {
                continue;
            }
            
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                               sSlotSeq,
                                                               &sSlot )
                      != IDE_SUCCESS );
        }

        /* BUG-45188
           빈 버퍼로 ST모듈 또는 MT모듈을 호출하는 것을 막는다.
           sResult = ID_TRUE 로 세팅해서, sdcRow::isDeleted() == ID_TRUE 로 continue 하도록 한다.
           ( 여기서 바로 continue 하면, row stamping을 못하는 경우가 있어서 위와같이 처리하였다.) */
        if ( sIsRowDeleted == ID_TRUE )
        {
            sResult = ID_TRUE;
        }
        else
        {
            SC_MAKE_GRID_WITH_SLOTNUM( sSlotGRID,
                                       aIterator->mSpaceID,
                                       SD_MAKE_PID( sSlotSID ),
                                       SD_MAKE_SLOTNUM( sSlotSID ) );

            IDE_TEST( aIterator->mFilter->callback( &sResult,
                                                    aIterator->mProperties->mLockRowBuffer,
                                                    NULL,
                                                    0,
                                                    sSlotGRID,
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
                                        aSP,
                                        aIterator->mSpaceID,
                                        sSlotSID,
                                        SDB_MULTI_PAGE_READ,
                                        &(aIterator->mSCN),
                                        &(aIterator->mInfinite),
                                        ID_FALSE, /* aIsUptLobByAPI */
                                        (UChar**)&sSlot,
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
                continue;
            }

            /* BUG-37274 repeatable read from disk table is incorrect behavior. */
            sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(sPage);
            if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotSeq)
                == ID_TRUE )
            {
                continue;
            }

            // delete된 row이거나 insert 이전 버젼이면 skip
            if( sdcRow::isDeleted(sSlot) == ID_TRUE )
            {
                continue;
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

            /*BUG-45401 : undoable ID_FALSE -> ID_TRUE로 변경 */
            IDE_TEST( sdrMiniTrans::begin( aIterator->mProperties->mStatistics,
                                           &sLogMtx,
                                           &sStartInfo,
                                           ID_TRUE,/*Undoable(PROJ-2162)*/
                                           SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE )
                      != IDE_SUCCESS );
            sState = 1;

            if( sCTSlotIdx == SDP_CTS_IDX_NULL )
            {
                IDE_TEST( sdcTableCTL::allocCTS(
                                      aIterator->mProperties->mStatistics,
                                      aMtx,
                                      &sLogMtx,
                                      SDB_MULTI_PAGE_READ,
                                      (sdpPhyPageHdr*)sPage,
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
            sSlotDirPtr  = sdpPhyPage::getSlotDirStartPtr(sPage);
            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                               sSlotSeq,
                                                               &sSlot )
                      != IDE_SUCCESS );

            // lock undo record를 생성한다.
            IDE_TEST( sdcRow::lock( aIterator->mProperties->mStatistics,
                                    sSlot,
                                    sSlotSID,
                                    &( aIterator->mInfinite ),
                                    &sLogMtx,
                                    sCTSlotIdx,
                                    &sSkipLockRec )
                      != IDE_SUCCESS );

            if ( sSkipLockRec == ID_FALSE )
            {
                IDE_TEST( sdrMiniTrans::setDirtyPage( &sLogMtx, sPage )
                          != IDE_SUCCESS );
            }

            sState = 0;
            IDE_TEST( sdrMiniTrans::commit( &sLogMtx ) != IDE_SUCCESS );

            if( aIterator->mProperties->mReadRecordCount == 0 )
            {
                break;
            }
        }
    }

    IDE_EXCEPTION_CONT( skip_not_data_page );

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
                             "[RECORD VALIDATION(RR)] "
                             "SpaceID:%"ID_UINT32_FMT", "
                             "TableOID:%"ID_vULONG_FMT", "
                             "ViewSCN:%"ID_UINT64_FMT", "
                             "CSInfiniteSCN:%"ID_UINT64_FMT", "
                             "CTSlotIdx:%"ID_UINT32_FMT", "
                             "FSCNOrCSCN:%"ID_UINT64_FMT", "
                             "InfiniteSCN:%"ID_UINT64_FMT", "
                             "Deleted:%s ",
                             sTableHeader->mSpaceID,
                             sTableHeader->mSelfOID,
                             SM_SCN_TO_LONG( aIterator->mSCN ),
                             SM_SCN_TO_LONG( aIterator->mInfinite ),
                             sCTSlotIdx,
                             SM_SCN_TO_LONG( sFSCNOrCSCN ),
                             SM_SCN_TO_LONG( sRowHdrInfo.mInfiniteSCN ),
                             SM_SCN_IS_DELETED(sRowHdrInfo.mInfiniteSCN)?"Y":"N" );

            IDE_SET( ideSetErrorCode(smERR_ABORT_StatementTooOld, sMsgBuf) );

            IDE_ERRLOG( IDE_SD_19 );
        }
        else
        {
            IDE_SET( ideSetErrorCode( smERR_RETRY_Already_Modified ) );
        }

        IDE_ASSERT( sdcRow::releaseLatchForAlreadyModify( aMtx, aSP )
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

/*********************************************************
 * function description: sdnnFetchNext.
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
static IDE_RC sdnnFetchNext( sdnnIterator * aIterator,
                             const void **  aRow )
{
    idBool          sIsNoMoreRow;
    scPageID        sCurPageID = SC_NULL_PID;

    // table의 마지막 페이지의 next에 도달했거나,
    // selet .. from limit 100등으로 읽어야 할 갯수도달한 경우임.
    if( ( aIterator->mCurSlotSeq == SDP_SLOT_ENTRY_MAX_NUM ) ||
        ( aIterator->mProperties->mReadRecordCount == 0 ) )
    {
        IDE_CONT( err_no_more_row );
    }

    IDE_TEST_CONT( aIterator->mCurPageID == SD_NULL_PID,
                    err_no_more_row );

retry:

    IDE_TEST( sdnnFetchNextAtPage( aIterator,
                                   (UChar*)*aRow,
                                   &sIsNoMoreRow )
              != IDE_SUCCESS );

    if( sIsNoMoreRow == ID_TRUE )
    {
        IDE_TEST( iduCheckSessionEvent( aIterator->mProperties->mStatistics )
                  != IDE_SUCCESS );

        /* 현재 데이타 페이지에 다 읽어서, 다음 페이지를 구함. */
        IDE_TEST( sdnnMoveToNxtPage( aIterator ) != IDE_SUCCESS );

        IDE_TEST_CONT( aIterator->mCurPageID == SD_NULL_PID,
                        err_no_more_row );

        // 새로운 데이타 페이지의 0번째 slotEntry 부터 검사.
        aIterator->mCurSlotSeq = SDP_SLOT_ENTRY_MIN_NUM;

        goto retry;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_CONT( err_no_more_row );

    *aRow = NULL;
    aIterator->mCurPageID    = sCurPageID;
    aIterator->mCurSlotSeq   = SDP_SLOT_ENTRY_MAX_NUM;
    SC_MAKE_NULL_GRID( aIterator->mRowGRID );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*********************************************************
  function description: sdnnLockAllRows4RR
  -  select for update, repeatable read를 위하여
  테이블의 데이타 첫번째 페이지부터, 마지막 페이지까지
 filter를 만족시키는  row들에 대하여 row-level lock을 다음과 같이 건다.

   1. row에 대하여 update가능한지 판단(sdcRecord::isUpdatable).
        >  skip flag가 올라오면 skip;
        >  retry가 올라오면 다시 데이타 페이지 latch를 잡고,
           update가능한지 판단.
        >  delete bit된 setting된 row이면 skip
   2.  filter적용하여 true이면 lock record( lock을 표현하는
       undo record생성및 rollptr 연결.
***********************************************************/
static IDE_RC sdnnLockAllRows4RR( sdnnIterator* aIterator)
{
    sdrMtxStartInfo    sStartInfo;
    scPageID           sCurPageID    = SC_NULL_PID;
    UInt               sFixPageCount = 0;
    sdrMtx             sMtx;
    sdrSavePoint       sSP;
    SInt               sState     = 0;
    UInt               sFirstRead = aIterator->mProperties->mFirstReadRecordPos;
    UInt               sReadRecordCnt = aIterator->mProperties->mReadRecordCount;

    IDE_ERROR( aIterator->mProperties->mLockRowBuffer != NULL ); // BUG-47758

    // table의 마지막 페이지의 next에 도달했거나,
    // selet .. from limit 100등으로 읽어야 할 갯수도달한 경우임.
    if( ( aIterator->mCurSlotSeq == SDP_SLOT_ENTRY_MAX_NUM ) ||
        ( aIterator->mProperties->mReadRecordCount == 0 ) )
    {
        IDE_CONT( err_no_more_row );
    }

    IDE_TEST_CONT( aIterator->mCurPageID == SD_NULL_PID,
                    err_no_more_row );

    /* BUG-42600 : undoable ID_FALSE -> ID_TRUE로 변경 */
    IDE_TEST( sdrMiniTrans::begin( aIterator->mProperties->mStatistics,
                                   &sMtx,
                                   aIterator->mTrans,
                                   SDR_MTX_LOGGING,
                                   ID_TRUE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE )
              != IDE_SUCCESS );
    sState = 1;

    sdrMiniTrans::makeStartInfo( &sMtx, &sStartInfo );
    sdrMiniTrans::setSavePoint( &sMtx, &sSP );

retry:

    IDE_TEST( sdnnLockAllRowsAtPage4RR( &sMtx,
                                        &sSP,
                                        aIterator )
              != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdrMiniTrans::commit( &sMtx ) != IDE_SUCCESS );

    IDE_TEST( iduCheckSessionEvent( aIterator->mProperties->mStatistics )
              != IDE_SUCCESS );

    /* 현재 데이타 페이지에 다 읽어서, 다음 페이지를 구함. */
    IDE_TEST( sdnnMoveToNxtPage4RR( aIterator ) != IDE_SUCCESS );

    IDE_TEST_CONT( aIterator->mCurPageID == SD_NULL_PID,
                    err_no_more_row );
    if( aIterator->mProperties->mReadRecordCount == 0 )
    {
        goto err_no_more_row;
    }

    /* BUG-42600 : undoable ID_FALSE -> ID_TRUE로 변경 */
    IDE_TEST( sdrMiniTrans::begin( aIterator->mProperties->mStatistics,
                                   &sMtx,
                                   aIterator->mTrans,
                                   SDR_MTX_LOGGING,
                                   ID_TRUE,/*Undoable(PROJ-2162)*/
                                   SM_DLOG_ATTR_DEFAULT | SM_DLOG_ATTR_UNDOABLE )
              != IDE_SUCCESS );
    sState = 1;

    sdrMiniTrans::setSavePoint( &sMtx, &sSP );

    sFixPageCount++;

    // 새로운 데이타 페이지의 0번째 keymap 부터 검사.
    aIterator->mCurSlotSeq = SDP_SLOT_ENTRY_MIN_NUM;

    goto retry;

    IDE_EXCEPTION_CONT( err_no_more_row );

    IDE_ASSERT( sState == 0 );

    aIterator->mCurPageID    = sCurPageID;
    aIterator->mCurSlotSeq   = SDP_SLOT_ENTRY_MAX_NUM;


    aIterator->mProperties->mFirstReadRecordPos = sFirstRead;
    aIterator->mProperties->mReadRecordCount    = sReadRecordCnt;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    if ( sState != 0 )
    {
        IDE_ASSERT( sdrMiniTrans::rollback( &sMtx ) == IDE_SUCCESS );
    }

    return IDE_FAILURE;

}

static IDE_RC sdnnAllocIterator( void ** aIteratorMem )
{
    SInt sState = 0;

    /* BUG-26647 Disk row cache */
    /* sdnnModule_sdnnAllocIterator_alloc_RowCache.tc */
    IDU_FIT_POINT("sdnnModule::sdnnAllocIterator::alloc::RowCache");
    IDE_TEST( sdnnCachePool.alloc(
                    (void**)&( ((sdnnIterator*)(*aIteratorMem))->mRowCache ) )
              != IDE_SUCCESS );
    sState = 1;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    IDE_PUSH();
    switch( sState )
    {
        case 1:
            IDE_ASSERT( sdnnCachePool.memfree(
                                ((sdnnIterator*)(*aIteratorMem))->mRowCache )
                        == IDE_SUCCESS);
        default:
            break;
    }
    IDE_POP();

    return IDE_FAILURE;
}

static IDE_RC sdnnFreeIterator( void * aIteratorMem )
{
    IDE_ASSERT( sdnnCachePool.memfree(
                                ((sdnnIterator*)aIteratorMem)->mRowCache )
                == IDE_SUCCESS );

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
static IDE_RC sdnnLockRow( sdnnIterator* aIterator )
{
    IDE_ASSERT( aIterator->mCurSlotSeq != SDP_SLOT_ENTRY_MIN_NUM );

    return sdnManager::lockRow( aIterator->mProperties,
                                aIterator->mTrans,
                                &(aIterator->mSCN),
                                &(aIterator->mInfinite),
                                aIterator->mSpaceID,
                                SD_MAKE_RID_FROM_GRID(aIterator->mRowGRID),
                                aIterator->mStatement->isForbiddenToRetry() );
}

/*******************************************************************************
 * Description: aIterator가 현재 가리키고 있는 페이지를 다음 페이지로 이동시킨다.
 *
 * aIterator    - [IN]  대상 Iterator의 포인터
 ******************************************************************************/
static IDE_RC sdnnMoveToNxtPage4RR( sdnnIterator * aIterator )
{
    sdbMPRFilter4PScan sFilter4Scan;

    /* PROJ-2402 */
    IDE_ASSERT( aIterator->mProperties->mParallelReadProperties.mThreadCnt != 0 );

    if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt > 1 )
    {
        sFilter4Scan.mThreadCnt = aIterator->mProperties->mParallelReadProperties.mThreadCnt;
        sFilter4Scan.mThreadID  = aIterator->mProperties->mParallelReadProperties.mThreadID - 1;

        IDE_TEST( aIterator->mMPRMgr.getNxtPageID( (void *)&sFilter4Scan,
                                                   &aIterator->mCurPageID )
                  != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( aIterator->mMPRMgr.getNxtPageID( NULL, /*aData*/
                                                   &aIterator->mCurPageID )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}


/*******************************************************************************
 * Description: aIterator가 현재 가리키고 있는 페이지를 다음 페이지로 이동시키고,
 *              Row Cache를 생성한다.
 *
 * aIterator    - [IN]  대상 Iterator의 포인터
 ******************************************************************************/
static IDE_RC sdnnMoveToNxtPage( sdnnIterator * aIterator )
{
    UChar             * sPagePtr;
    sdpPhyPageHdr     * sPageHdr;
    idBool              sTrySuccess = ID_FALSE;
    idBool              sFoundPage  = ID_FALSE;
    SInt                sState      = 0;
    sdbMPRFilter4PScan  sFilter4Scan;

    while( sFoundPage == ID_FALSE )
    {
        // PROJ-2402
        IDE_ASSERT( aIterator->mProperties->mParallelReadProperties.mThreadCnt != 0 );

        if ( aIterator->mProperties->mParallelReadProperties.mThreadCnt > 1 )
        {
            sFilter4Scan.mThreadCnt = aIterator->mProperties->mParallelReadProperties.mThreadCnt;
            sFilter4Scan.mThreadID  = aIterator->mProperties->mParallelReadProperties.mThreadID - 1;

            IDE_TEST( aIterator->mMPRMgr.getNxtPageID( (void*)&sFilter4Scan,
                                                       &aIterator->mCurPageID )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST( aIterator->mMPRMgr.getNxtPageID( NULL, /*aData*/
                                                       &aIterator->mCurPageID )
                      != IDE_SUCCESS );
        }

        if( aIterator->mCurPageID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdbBufferMgr::getPage( aIterator->mProperties->mStatistics,
                                         aIterator->mSpaceID,
                                         aIterator->mCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_MULTI_PAGE_READ,
                                         &sPagePtr,
                                         &sTrySuccess )
                  != IDE_SUCCESS );
        sState = 1;

        IDE_ASSERT( sTrySuccess == ID_TRUE );

        /* PROJ-2162 RestartRiskReduction
         * Inconsistent page 발견하였고, 예외 못하면 */
        if( ( sdpPhyPage::isConsistentPage( sPagePtr ) == ID_FALSE ) &&
            ( smuProperty::getCrashTolerance() != 2 ) )
        {
            /* Tolerance하지 않으면, 예외처리.
             * 관용이면 다음 페이지 조회 */
            IDE_TEST_RAISE( smuProperty::getCrashTolerance() == 0,
                            ERR_INCONSISTENT_PAGE );
        }
        else
        {
            sPageHdr = sdpPhyPage::getHdr( sPagePtr );

            /*
             * BUG-32942 When executing rebuild Index stat, abnormally shutdown
             * 
             * Free pages should be skiped. Otherwise index can read
             * garbage value, and server is shutdown abnormally.
             */
            if( (sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA) &&
                (aIterator->mSegMgmtOp->mIsFreePage( sPagePtr ) != ID_TRUE) )
            {
                #ifdef DEBUG
                smSCN sStmtSCN = aIterator->mStatement->getSCN(); 
                smSCN sIteratorSCN = aIterator->mSCN;
                IDE_DASSERT( SM_SCN_IS_EQ( &sStmtSCN, &sIteratorSCN ) );
                #endif
                /* 올바른 버전의 row를 읽기 위해, Cache를 생성하기 전에
                 * 대상 페이지의 모든 slot에 대하여 Stamping을 수행 */
                IDE_TEST( sdcTableCTL::runDelayedStampingAll(
                                            aIterator->mProperties->mStatistics,
                                            aIterator->mTrans,
                                            sPagePtr,
                                            SDB_MULTI_PAGE_READ )
                           != IDE_SUCCESS );

                sdnnMakeRowCache( aIterator, sPagePtr ); /*  페이지를 캐쉬로 복사(memcpy) */

                sFoundPage = ID_TRUE;
            }
        }

        sState = 0;
        IDE_TEST( sdbBufferMgr::releasePage(
                                    aIterator->mProperties->mStatistics,
                                    sPagePtr )
                  != IDE_SUCCESS );
    }

    /* FIT/ART/sm/Bugs/BUG-26647/BUG-26647.tc */
    IDU_FIT_POINT( "1.BUG-26647@sdnnModule::sdnnMoveToNxtPage" );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INCONSISTENT_PAGE )
    {
        // BUG-45598: fullscan 모듈 단계에서 발견된 에러 페이지의 예외 처리 코드
        if ( smrRecoveryMgr::isRestart() == ID_FALSE )
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_PageCorrupted,
                                      aIterator->mSpaceID,
                                      aIterator->mCurPageID));
        }
        else
        {
            IDE_SET( ideSetErrorCode( smERR_ABORT_INCONSISTENT_PAGE,
                                      aIterator->mSpaceID,
                                      aIterator->mCurPageID) );
        }
    }
    IDE_EXCEPTION_END;

    IDE_PUSH();
    if( sState != 0 )
    {
        sState = 0;
        IDE_ASSERT( sdbBufferMgr::releasePage(
                                    aIterator->mProperties->mStatistics,
                                    sPagePtr )
                    == IDE_SUCCESS );
    }
    IDE_POP();

    return IDE_FAILURE;
}

/***********************************************************************
 * Description : 지정된 Table를 Aging한다.
 **********************************************************************/
static IDE_RC sdnnAging( idvSQL         * aStatistics,
                         void           * aTrans,
                         smcTableHeader * aHeader,
                         smnIndexHeader * /*aIndex*/)
{
    scPageID           sCurPageID;
    idBool             sIsSuccess;
    UInt               sState = 0;
    sdpPageListEntry * sPageListEntry;
    sdpPhyPageHdr    * sPageHdr;
    UChar            * sPagePtr;
    sdbMPRMgr          sMPRMgr;
    sdrMtxStartInfo    sStartInfo;
    sdpSegCCache     * sSegCache;
    sdpSegMgmtOp     * sSegMgmtOp;
    ULong              sUsedSegSizeByBytes = 0;
    UInt               sMetaSize = 0;
    UInt               sFreeSize = 0;
    UInt               sUsedSize = 0;
    sdpSegInfo         sSegInfo;

    sStartInfo.mTrans   = aTrans;
    sStartInfo.mLogMode = SDR_MTX_LOGGING; 

    sPageListEntry = (sdpPageListEntry*)(&(aHeader->mFixed.mDRDB));
    sSegCache = sdpSegDescMgr::getSegCCache( sPageListEntry );
    sSegMgmtOp = sdpSegDescMgr::getSegMgmtOp( aHeader->mSpaceID );
    // codesonar::Null Pointer Dereference
    IDE_ERROR( sSegMgmtOp != NULL );
 
    IDE_TEST( sSegMgmtOp->mGetSegInfo(
                                     NULL,
                                     aHeader->mSpaceID,
                                     sdpSegDescMgr::getSegPID( sPageListEntry ),
                                     NULL, /* aTableHeader */
                                     &sSegInfo )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::rebuildMinViewSCN( aStatistics )
              != IDE_SUCCESS );

    IDE_TEST( sMPRMgr.initialize(
                  aStatistics,
                  aHeader->mSpaceID,
                  sdpSegDescMgr::getSegHandle( sPageListEntry ),
                  NULL ) /*aFilter*/
             != IDE_SUCCESS );
    sState = 1;

    sCurPageID = SM_NULL_PID;

    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    while( 1 )
    {
        IDE_TEST( sMPRMgr.getNxtPageID( NULL, /*aData */
                                        &sCurPageID)
                  != IDE_SUCCESS );

        if( sCurPageID == SD_NULL_PID )
        {
            break;
        }

        sMetaSize = 0;
        sFreeSize = 0;
        sUsedSize = 0;

        IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                         aHeader->mSpaceID,
                                         sCurPageID,
                                         SDB_S_LATCH,
                                         SDB_WAIT_NORMAL,
                                         SDB_MULTI_PAGE_READ,
                                         &sPagePtr,
                                         &sIsSuccess )
                  != IDE_SUCCESS );
        sState = 2;

        sPageHdr = sdpPhyPage::getHdr( sPagePtr );

        IDE_DASSERT( (sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_FORMAT) ||
                     (sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA) )

        /*
         * BUG-32942 When executing rebuild Index stat, abnormally shutdown
         * 
         * Free pages should be skiped. Otherwise index can read
         * garbage value, and server is shutdown abnormally.
         */
        if( (sdpPhyPage::getPageType( sPageHdr ) == SDP_PAGE_DATA) &&
            (sSegMgmtOp->mIsFreePage( sPagePtr ) != ID_TRUE) )
        {
            IDE_TEST( sdcTableCTL::runDelayedStampingAll(
                                              aStatistics,
                                              aTrans,
                                              sPagePtr,
                                              SDB_MULTI_PAGE_READ )
                      != IDE_SUCCESS );

            IDE_TEST( sdcTableCTL::runRowStampingAll(
                                              aStatistics,
                                              &sStartInfo,
                                              sPagePtr,
                                              SDB_MULTI_PAGE_READ )
                != IDE_SUCCESS );

            sMetaSize = sdpPhyPage::getPhyHdrSize()
                        + sdpPhyPage::getLogicalHdrSize( sPageHdr )
                        + sdpPhyPage::getSizeOfCTL( sPageHdr )
                        + sdpPhyPage::getSizeOfSlotDir( sPageHdr )
                        + ID_SIZEOF( sdpPageFooter );
            sFreeSize = sPageHdr->mTotalFreeSize;
            sUsedSize = SD_PAGE_SIZE - (sMetaSize + sFreeSize);
            sUsedSegSizeByBytes += sUsedSize;
            IDE_DASSERT ( sMetaSize + sFreeSize + sUsedSize == SD_PAGE_SIZE );
        }
        else
        {
            /* nothing to do */ 
        }

        sState = 1;
        IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                             sPagePtr )
                  != IDE_SUCCESS );
    }

    /* BUG-31372: 세그먼트 실사용양 정보를 조회할 방법이 필요합니다. */
    sSegCache->mFreeSegSizeByBytes = (sSegInfo.mFmtPageCnt * SD_PAGE_SIZE) - sUsedSegSizeByBytes;

    sState = 0;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 2:
            IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                   sPagePtr )
                        == IDE_SUCCESS );
        case 1:
            IDE_ASSERT( sMPRMgr.destroy() == IDE_SUCCESS );
        default:
            break;
    }

    return IDE_FAILURE;
}

/*********************************************************************
 * FUNCTION DESCRIPTION : sdnnGatherStat                             *
 * ------------------------------------------------------------------*
 * Table의 통계정보를 구축한다.
 * 
 * Statistics    - [IN]  IDLayer 통계정보
 * Trans         - [IN]  이 작업을 요청한 Transaction
 * Percentage    - [IN]  Sampling Percentage
 * Degree        - [IN]  Parallel Degree
 * Header        - [IN]  대상 TableHeader
 * Index         - [IN]  대상 index (FullScan이기에 무시됨 )
 *********************************************************************/
IDE_RC sdnnGatherStat( idvSQL         * aStatistics,
                       void           * aTrans,
                       SFloat           aPercentage,
                       SInt             /* aDegree */,
                       smcTableHeader * aHeader,
                       void           * aTotalTableArg,
                       smnIndexHeader * /*aIndex*/,
                       void           * aStats,
                       idBool           aDynamicMode )
{
    scPageID                        sCurPageID;
    sdpPageListEntry              * sPageListEntry;
    sdbMPRMgr                       sMPRMgr;
    smiFetchColumnList            * sFetchColumnList;
    UInt                            sMaxRowSize = 0;
    UChar                         * sRowBufferSource;
    UChar                         * sRowBuffer;
    void                          * sTableArgument;
    sdbMPRFilter4SamplingAndPScan   sFilter4Scan;
    UInt                            sState = 0;

    IDE_TEST( sdnManager::makeFetchColumnList( aHeader, 
                                               &sFetchColumnList,
                                               &sMaxRowSize )
              != IDE_SUCCESS );
    sState = 1;

    IDE_ERROR( sMaxRowSize > 0 );

    /* sdnnModule_sdnnGatherStat_calloc_RowBufferSource.tc */
    IDU_FIT_POINT("sdnnModule::sdnnGatherStat::calloc::RowBufferSource");
    IDE_TEST( iduMemMgr::calloc( IDU_MEM_SM_SDN,
                                 1,
                                 sMaxRowSize + ID_SIZEOF(ULong),
                                 (void**) &sRowBufferSource )
              != IDE_SUCCESS );
    sRowBuffer = (UChar*)idlOS::align8( (ULong)sRowBufferSource );

    sState = 2;

    IDE_TEST( smLayerCallback::beginTableStat( aHeader,
                                               aPercentage,
                                               aDynamicMode,
                                               &sTableArgument )
              != IDE_SUCCESS );
    sState = 3;

    /* 1 Thread로 고정한다. */
    sFilter4Scan.mThreadCnt  = 1;
    sFilter4Scan.mThreadID   = 0;
    sFilter4Scan.mPercentage = aPercentage;

    sPageListEntry = (sdpPageListEntry*)(&(aHeader->mFixed.mDRDB));
    IDE_TEST( sMPRMgr.initialize(
                  aStatistics,
                  aHeader->mSpaceID,
                  sdpSegDescMgr::getSegHandle( sPageListEntry ),
                  sdbMPRMgr::filter4SamplingAndPScan ) /* Filter */
             != IDE_SUCCESS );
    sState = 4;

    sCurPageID = SM_NULL_PID;
    IDE_TEST( sMPRMgr.beforeFst() != IDE_SUCCESS );

    /******************************************************************
     * 통계정보 재구축 시작
     ******************************************************************/
    while( 1 )
    {
        IDE_TEST( sMPRMgr.getNxtPageID( (void*)&sFilter4Scan, /*aData */
                                        &sCurPageID)
                  != IDE_SUCCESS );

        if( sCurPageID == SD_NULL_PID )
        {
            break;
        }

        IDE_TEST( sdnnAnalyzePage4GatherStat( aStatistics,
                                              aTrans,
                                              aHeader,
                                              sFetchColumnList,
                                              sCurPageID,
                                              sRowBuffer,
                                              sTableArgument,
                                              aTotalTableArg )
                  != IDE_SUCCESS );

        IDE_TEST( iduCheckSessionEvent( aStatistics )
                  != IDE_SUCCESS );
    }

    sState = 3;
    IDE_TEST( sMPRMgr.destroy() != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::setTableStat( aHeader,
                                             aTrans,
                                             sTableArgument,
                                             (smiAllStat*)aStats,
                                             aDynamicMode )
              != IDE_SUCCESS );

    sState = 2;
    IDE_TEST( smLayerCallback::endTableStat( aHeader,
                                             sTableArgument,
                                             aDynamicMode )
              != IDE_SUCCESS );

    sState = 1;
    IDE_TEST( iduMemMgr::free( sRowBufferSource ) != IDE_SUCCESS );

    sState = 0;
    IDE_TEST( sdnManager::destFetchColumnList( sFetchColumnList ) 
              != IDE_SUCCESS );


    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sState )
    {
    case 4:
        (void) sMPRMgr.destroy();
    case 3:
        (void) smLayerCallback::endTableStat( aHeader,
                                              sTableArgument,
                                              aDynamicMode );
    case 2:
        (void) iduMemMgr::free( sRowBufferSource );
    case 1:
        (void) sdnManager::destFetchColumnList( sFetchColumnList );
    default:
        break;
    }

    return IDE_FAILURE;
}

/******************************************************************
 *
 * Description :
 *  통계정보 획득을 위해 페이지를 분석한다.
 *
 *  aStatistics         - [IN]     통계정보
 *  aTableArgument      - [OUT]    테이블 통계정보
 *
 * ****************************************************************/

IDE_RC sdnnAnalyzePage4GatherStat( idvSQL             * aStatistics,
                                   void               * aTrans,
                                   smcTableHeader     * aTableHeader,
                                   smiFetchColumnList * aFetchColumnList,
                                   scPageID             aPageID,
                                   UChar              * aRowBuffer,
                                   void               * aTableArgument,
                                   void               * aTotalTableArgument )
{
    sdpPhyPageHdr      * sPageHdr;
    UChar              * sSlotDirPtr;
    UShort               sSlotCount;
    UShort               sSlotNum;
    UChar              * sSlotPtr;
    sdcRowHdrInfo        sRowHdrInfo;
    idBool               sIsRowDeleted;
    idBool               sIsPageLatchReleased = ID_TRUE;
    UInt                 sMetaSpace     = 0;
    UInt                 sUsedSpace     = 0;
    UInt                 sAgableSpace   = 0;
    UInt                 sFreeSpace     = 0;
    UInt                 sReservedSpace = 0;
    idvTime              sBeginTime;
    idvTime              sEndTime;
    SLong                sReadRowTime = 0;
    SLong                sReadRowCnt  = 0;
    idBool               sIsSuccess   = ID_FALSE;
    UInt                 sState       = 0;
 
    IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                     aTableHeader->mSpaceID,
                                     aPageID,
                                     SDB_S_LATCH,
                                     SDB_WAIT_NORMAL,
                                     SDB_MULTI_PAGE_READ,
                                     (UChar**)&sPageHdr,
                                     &sIsSuccess )
              != IDE_SUCCESS );
    IDE_ERROR( sIsSuccess == ID_TRUE );
    sState = 1;

    if (sdpPhyPage::getPageType( sPageHdr ) != SDP_PAGE_DATA )
    {
        sFreeSpace = sdpPhyPage::getEmptyPageFreeSize();
        sMetaSpace = sdpPhyPage::getPhyHdrSize() 
                     + sdpPhyPage::getSizeOfFooter(); 
        IDE_CONT( SKIP );
    }
    else
    {
        /* nothing  to do */
    }

    sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPageHdr );
    sSlotCount  = sdpSlotDirectory::getCount( sSlotDirPtr );

    for( sSlotNum = 0 ; sSlotNum < sSlotCount ; sSlotNum++ )
    {
        if( sdpSlotDirectory::isUnusedSlotEntry( sSlotDirPtr,
                                                 sSlotNum ) == ID_TRUE )
        {
            continue;
        }

        IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                           sSlotNum,
                                                           &sSlotPtr )
               != IDE_SUCCESS );

        if( sdcRow::isDeleted(sSlotPtr) == ID_TRUE )
        {
            continue;
        }

        sdcRow::getRowHdrInfo( sSlotPtr, &sRowHdrInfo );
        if( SDC_IS_HEAD_ROWPIECE( sRowHdrInfo.mRowFlag ) != ID_TRUE )
        {
            continue;
        }

        IDV_TIME_GET(&sBeginTime);

        if( sdcRow::fetch( aStatistics,
                                 NULL, /* aMtx */
                                 NULL, /* aSP */
                                 aTrans,
                                 aTableHeader->mSpaceID,
                                 sSlotPtr,
                                 ID_TRUE, /* aIsPersSlot */
                                 SDB_MULTI_PAGE_READ,
                                 aFetchColumnList,
                                 SMI_FETCH_VERSION_LAST,
                                 SD_NULL_SID, /* FetchLastVersion True */
                                 NULL,        /* FetchLastVersion True */
                                 NULL,        /* FetchLastVersion True */
                                 NULL,        /* aIndexInfo4Fetch */
                                 NULL,        /* aLobInfo4Fetch */
                                 aTableHeader->mRowTemplate,
                                 aRowBuffer,
                                 &sIsRowDeleted,
                                 &sIsPageLatchReleased,
                                 ID_TRUE ) != IDE_SUCCESS )
        {
            /* BUG-44264: 여기처럼 Assert skip으로 fetch 할 때( ex. 통계정보 ) 
             * fetch 함수 내부에서 fetch 실패했을 때 예외처리 중 Page의 Latch를
             * 해제 한 경우, 다시 획득해야함.  
             */ 
            if( sIsPageLatchReleased == ID_TRUE )
            {
                sState = 0;
                IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                                 aTableHeader->mSpaceID,
                                                 aPageID,
                                                 SDB_S_LATCH,
                                                 SDB_WAIT_NORMAL,
                                                 SDB_MULTI_PAGE_READ,
                                                 (UChar**)&sPageHdr,
                                                 &sIsSuccess )
                          != IDE_SUCCESS );
                IDE_ERROR( sIsSuccess == ID_TRUE );
                sState = 1;
                sIsPageLatchReleased = ID_FALSE;

                sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPageHdr );
            }
            continue;
        }

        IDV_TIME_GET(&sEndTime);

        sReadRowTime += IDV_TIME_DIFF_MICRO(&sBeginTime, &sEndTime);
        sReadRowCnt++;

        if( sIsPageLatchReleased == ID_TRUE )
        {
            sState = 0;
            IDU_FIT_POINT("BUG-42526@sdnnModule::sdnnAnalyzePage4GatherStat::getPage");
            IDE_TEST( sdbBufferMgr::getPage( aStatistics,
                                             aTableHeader->mSpaceID,
                                             aPageID,
                                             SDB_S_LATCH,
                                             SDB_WAIT_NORMAL,
                                             SDB_MULTI_PAGE_READ,
                                             (UChar**)&sPageHdr,
                                             &sIsSuccess )
                      != IDE_SUCCESS );
            IDE_ERROR( sIsSuccess == ID_TRUE );
            sState = 1;
            sIsPageLatchReleased = ID_FALSE;

            sSlotDirPtr = sdpPhyPage::getSlotDirStartPtr( (UChar*)sPageHdr );

            if( sdpSlotDirectory::isUnusedSlotEntry(sSlotDirPtr, sSlotNum)
                == ID_TRUE )
            {
                continue;
            }

            IDE_TEST( sdpSlotDirectory::getPagePtrFromSlotNum( sSlotDirPtr,
                                                               sSlotNum,
                                                               &sSlotPtr )
                      != IDE_SUCCESS );

        }

        if ( sIsRowDeleted == ID_TRUE )
        {
            /* 지워진 row면 나중에 Aging될 것이기 때문에,
             * AgableSpace로 판단함*/
            sAgableSpace += sdcRow::getRowPieceSize( sSlotPtr );
            continue;
        }

        IDE_TEST( smLayerCallback::analyzeRow4Stat( aTableHeader,
                                                    aTableArgument,
                                                    aTotalTableArgument,
                                                    aRowBuffer )
                  != IDE_SUCCESS );
    }

    sFreeSpace = sdpPhyPage::getTotalFreeSize( sPageHdr );

    /* Meta/Agable/Free외 모든 공간은 Used라 판단한다. */
    sMetaSpace = sdpPhyPage::getPhyHdrSize()
                 + sdpPhyPage::getLogicalHdrSize( sPageHdr )
                 + sdpPhyPage::getSizeOfCTL( sPageHdr )
                 + sdpPhyPage::getSizeOfSlotDir( sPageHdr )
                 + ID_SIZEOF( sdpPageFooter );

    IDE_EXCEPTION_CONT( SKIP );

    sState = 0;
    IDE_TEST( sdbBufferMgr::releasePage( aStatistics,
                                         (UChar*)sPageHdr )
              != IDE_SUCCESS );

    IDE_TEST( smLayerCallback::updateOneRowReadTime( aTableArgument,
                                                     sReadRowTime,
                                                     sReadRowCnt )
              != IDE_SUCCESS );

    /* BUG-42526 통계수집 방법 변경
     * sUsedSpace가 SD_PAGE_SIZE보다 커지는 예외상황을 막기위해 보정코드 추가
     */
    sReservedSpace = sMetaSpace + sFreeSpace + sAgableSpace;
    sUsedSpace     = ( (SD_PAGE_SIZE > sReservedSpace) ? 
                       (SD_PAGE_SIZE - sReservedSpace) : 0 );

    IDE_ERROR_RAISE( sMetaSpace   <= SD_PAGE_SIZE, ERR_INVALID_SPACE_USAGE );
    IDE_ERROR_RAISE( sAgableSpace <= SD_PAGE_SIZE, ERR_INVALID_SPACE_USAGE );
    IDE_ERROR_RAISE( sFreeSpace   <= SD_PAGE_SIZE, ERR_INVALID_SPACE_USAGE );

    IDE_TEST( smLayerCallback::updateSpaceUsage( aTableArgument,
                                                 sMetaSpace,
                                                 sUsedSpace,
                                                 sAgableSpace,
                                                 sFreeSpace )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_INVALID_SPACE_USAGE )
    {
         ideLog::log( IDE_SM_0, "Invalid Space Usage \n"
                                "SpaceID      : <%"ID_UINT32_FMT"> \n"
                                "PageID       : <%"ID_UINT32_FMT"> \n"
                                "sMetaSpace   : <%"ID_UINT32_FMT"> \n"
                                "sUsedSpace   : <%"ID_UINT32_FMT"> \n" 
                                "sAgableSpace : <%"ID_UINT32_FMT"> \n"
                                "sFreeSpace   : <%"ID_UINT32_FMT"> \n",
                                aTableHeader->mSpaceID,
                                aPageID,
                                sMetaSpace, 
                                sUsedSpace,
                                sAgableSpace,
                                sFreeSpace );
    }
    IDE_EXCEPTION_END;

    switch( sState )
    {
        case 1:
            if ( sIsPageLatchReleased == ID_FALSE )
            {
                IDE_ASSERT( sdbBufferMgr::releasePage( aStatistics,
                                                       (UChar*)sPageHdr )
                            == IDE_SUCCESS );
            }
            break;
        default:
            break;
    }


    return IDE_FAILURE;

}



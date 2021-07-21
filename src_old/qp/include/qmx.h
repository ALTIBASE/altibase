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
 * $Id: qmx.h 90270 2021-03-21 23:20:18Z bethy $
 **********************************************************************/

#ifndef _Q_QMX_H_
#define _Q_QMX_H_ 1

// PROJ-1350
// 특정 비율의 Data 변경이 발생하면 이미 생성된 Plan Tree를 재구성함.
#define  QMC_AUTO_REBUILD_PLAN_PLUS_RATIO                      (0.3)
#define  QMC_AUTO_REBUILD_PLAN_MINUS_RATIO                    (-0.3)

#include <smiDef.h>
#include <qc.h>
#include <qtcDef.h>
#include <qcmTableInfo.h>
#include <qmmParseTree.h>
#include <qmc.h>
#include <qmnDef.h>
#include <mtc.h>

typedef struct qmxLobInfo
{
    UShort                size;       // alloc size

    // for copy (QP)
    UShort                count;
    smiColumn          ** column;     // dst.
    smLobLocator        * locator;    // src.
    // PROJ-2728 for copy (SD)
    UShort              * dstBindId;  // BindId or ColumnOrder

    // for outbind (MM)
    UShort                outCount;
    UShort              * outBindId;
    smiColumn          ** outColumn;
    smLobLocator        * outLocator;
    smLobLocator        * outFirstLocator;
    idBool                outFirst;
    idBool                outCallback;

    // PROJ-2728 for InBinding in PSM (SD)
    UShort                mCount4PutLob;
    UShort              * mBindId4PutLob;

    // PROJ-2728 for OutBinding in PSM (SD)
    UShort                mCount4OutBindNonLob;
    UShort              * mBindId4OutBindNonLob;

    /* BUG-30351
     * insert into select에서 각 Row Insert 후 해당 Lob Cursor를 바로 해제했으면 합니다.
     */
    idBool                mImmediateClose;

} qmxLobInfo;

class qmcInsertCursor;

class qmx
{
public:
    // execution
    static IDE_RC executeInsertValues(qcStatement *aStatement);

    static IDE_RC executeInsertSelect(qcStatement *aStatement);

    static IDE_RC executeMultiInsertSelect(qcStatement *aStatement);

    static IDE_RC executeDelete(qcStatement *aStatement);

    static IDE_RC executeUpdate(qcStatement *aStatement);

    static IDE_RC executeMove(qcStatement *aStatement);

    /* PROJ-1988 Implement MERGE statement */
    static IDE_RC executeMerge(qcStatement *aStatement);

    // Shard DML
    static IDE_RC executeShardDML(qcStatement *aStatement);

    // Shard Insert
    static IDE_RC executeShardInsert(qcStatement *aStatement);

    static IDE_RC executeLockTable(qcStatement *aStatement);

    static IDE_RC executeSelect(qcStatement *aStatement);

    static IDE_RC makeSmiValueForSubquery( qcTemplate    * aTemplate,
                                           qcmTableInfo  * aTableInfo,
                                           qcmColumn     * aColumn,
                                           qmmSubqueries * aSubquery,
                                           UInt            aCanonizedTuple,
                                           smiValue      * aUpdatedRow,
                                           mtdIsNullFunc * aIsNull,
                                           qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueForSubqueryMultiTable( qcTemplate    * aTemplate,
                                                     qcmTableInfo  * aTableInfo,
                                                     qcmColumn     * aColumn,
                                                     qmmValueNode  * aValues,
                                                     qmmValueNode ** aValuesPos,
                                                     qmmSubqueries * aSubquery,
                                                     UInt            aCanonizedTuple,
                                                     smiValue      * aUpdatedRow,
                                                     mtdIsNullFunc * aIsNull,
                                                     qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueForUpdate( qcTemplate    * aTmplate,
                                         qcmTableInfo  * aTableInfo,
                                         qcmColumn     * aColumn,
                                         qmmValueNode  * aValue,
                                         UInt            aCanonizedTuple,
                                         smiValue      * aUpdatedRow,
                                         mtdIsNullFunc * aIsNull,
                                         qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueWithValue( qcmColumn     * aColumn,
                                         qmmValueNode  * aValue,
                                         qcTemplate    * aTmplate,
                                         qcmTableInfo  * aTableInfo,
                                         smiValue      * aInsertedRow,
                                         qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueWithValue( qcTemplate    * aTemplate,
                                         qcmTableInfo  * aTableInfo,
                                         UInt            aCanonizedTuple,
                                         qmmValueNode  * aValue,
                                         void          * aQueueMsgIDSeq,
                                         smiValue      * aInsertedRow,
                                         qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueWithResult( qcmColumn     * aColumn,
                                          qcTemplate    * aTemplate,
                                          qcmTableInfo  * aTableInfo,
                                          smiValue      * aInsertedRow,
                                          qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueWithStack( qcmColumn     * aColumn,
                                         qcTemplate    * aTemplate,
                                         mtcStack      * aStack,
                                         qcmTableInfo  * aTableInfo,
                                         smiValue      * aInsertedRow,
                                         qmxLobInfo    * aLobInfo );

    static IDE_RC makeSmiValueForChkRowMovement( smiColumnList * aUpdateColumns,
                                                 smiValue      * aUpdateValues,
                                                 qcmColumn     * aPartKeyColumns,
                                                 mtcTuple      * aTuple,
                                                 smiValue      * aChkValues );

    static IDE_RC makeSmiValueForRowMovement( qcmTableInfo   * aTableInfo,
                                              smiColumnList  * aUpdateColumns,
                                              smiValue       * aUpdateValues,
                                              mtcTuple       * aTuple,
                                              smiTableCursor * aCursor,
                                              qmxLobInfo     * aUptLobInfo,
                                              smiValue       * aInsValues,
                                              qmxLobInfo     * aInsLobInfo );

    // PROJ-2264 Dictionary table
    static IDE_RC makeSmiValueForCompress( qcTemplate * aTemplate,
                                           UInt         aCompressedTuple,
                                           smiValue   * aInsertedRow );

    // PROJ-2205
    static IDE_RC makeRowWithSmiValue( mtcColumn   * aColumn,
                                       UInt          aColumnCount,
                                       smiValue    * aValues,
                                       void        * aRow );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC makeSmiValueWithSmiValue( qcmTableInfo * aSrcTableInfo,
                                            qcmTableInfo * aDstTableInfo,
                                            qcmColumn    * aQcmColumn,
                                            UInt           aColumnCount,
                                            smiValue     * aSrcValueArr,
                                            smiValue     * aDstValueArr );

    /* BUG-45680 insert 수행시 not null column에 대한 에러메시지 정보에 column 정보 출력. */
    static IDE_RC checkNotNullColumnForInsert( qcmColumn  * aColumn,
                                               smiValue   * aInsertedRow,
                                               qmxLobInfo * aLobInfo,
                                               idBool       aPrintColumnName );

    static IDE_RC readSequenceNextVals( qcStatement         * aStatement,
                                        qcParseSeqCaches    * aNextValSeqs );

    static IDE_RC dummySequenceNextVals( qcStatement        * aStatement,
                                         qcParseSeqCaches   * aNextValSeqs );

    static IDE_RC findSessionSeqCaches( qcStatement * aStatement,
                                        qcParseTree * aParseTree );

    static IDE_RC setValSessionSeqCaches( qcStatement         * aStatement,
                                          qcParseSeqCaches    * aParseSeqCache,
                                          SLong                 aNextVal );

    static IDE_RC addSessionSeqCaches(qcStatement * aStatement,
                                      qcParseTree * aParseTree );

    static IDE_RC setTimeStamp( void * aValue );

    // PROJ-1362
    static IDE_RC initializeLobInfo( qcStatement  * aStatement,
                                     qmxLobInfo  ** aLobInfo,
                                     UShort         aSize );

    // PROJ-2205
    static void initLobInfo( qmxLobInfo  * aLobInfo );

    static void clearLobInfo( qmxLobInfo  * aLobInfo );

    static IDE_RC addLobInfoForCopy( qmxLobInfo   * aLobInfo,
                                     smiColumn    * aColumn,
                                     smLobLocator   aLocator,
                                     UShort         aBindId = -1 ); // PROJ-2728

    static IDE_RC addLobInfoForOutBind( qmxLobInfo  * aLobInfo,
                                        smiColumn   * aColumn,
                                        UShort        aBindId );

    /* BUG-30351
     * insert into select에서 각 Row Insert 후 해당 Lob Cursor를 바로 해제했으면 합니다.
     */
    static void setImmediateCloseLobInfo( qmxLobInfo  * aLobInfo,
                                          idBool        aImmediateClose );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC addLobInfoFromLobInfo( UInt         aColumnID,
                                         qmxLobInfo * aSrcLobInfo,
                                         qmxLobInfo * aDestLobInfo );

    static IDE_RC copyAndOutBindLobInfo( qcStatement    * aStatement,
                                         qmxLobInfo     * aLobInfo,
                                         smiTableCursor * aCursor,
                                         void           * aRow,
                                         scGRID           aRowGRID );

    static void finalizeLobInfo( qcStatement * aStatement,
                                 qmxLobInfo  * aLobInfo );

    static IDE_RC closeLobLocator( idvSQL       *aStatistics,
                                   smLobLocator  aLocator );

    static IDE_RC changeLobColumnInfo( qmxLobInfo * aLobInfo,
                                       qcmColumn  * aColumns );

    static idBool existLobInfo( qmxLobInfo * aLobInfo,
                                UInt         aColumnID );

    // PROJ-1350 Plan Tree 자동 재구성
    // Plan Tree가 유효한지를 검사
    static IDE_RC checkPlanTreeOld( qcStatement * aStatement,
                                    idBool      * aIsOld );

    // PROJ-1518 Atomic Array Insert
    static IDE_RC atomicExecuteInsertBefore( qcStatement  * aStatement );

    static IDE_RC atomicExecuteInsert( qcStatement  * aStatement );

    static IDE_RC atomicExecuteInsertAfter( qcStatement  * aStatement );

    static IDE_RC atomicExecuteFinalize( qcStatement  * aStatement );

    // BUG-34084 partition lock pruning
    static IDE_RC lockPartitionForDML( smiStatement     * aSmiStmt,
                                       qmsPartitionRef  * aPartitionRef,
                                       smiTableLockMode   aLockMode );

    // PROJ-1359 Trigger
    static IDE_RC fireTriggerInsertRowGranularity(
        qcStatement      * aStatement,
        qmsTableRef      * aTableRef,
        qmcInsertCursor  * aInsertCursorMgr,
        scGRID             aInsertGRID,
        qmmReturnInto    * aReturnInto,
        qdConstraintSpec * aCheckConstrList,    /* PROJ-1107 Check Constraint 지원 */
        vSLong             aRowCnt,
        idBool             aNeedNewRow );

    /* PROJ-1988 Implement MERGE statement */
    static void setSubStatement( qcStatement * aStatement,
                                 qcStatement * aSubStatement );

    /* PROJ-1584 DML Return Clause */
    static  IDE_RC copyReturnToInto( qcTemplate      * aTemplate,
                                     qmmReturnInto   * aReturnInto,
                                     vSLong            aRowCnt );

    static  IDE_RC normalReturnToInto( qcTemplate      * aTemplate,
                                       qmmReturnInto   * aReturnInto );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void copyMtcTupleForPartitionedTable( mtcTuple * aDstTuple,
                                                 mtcTuple * aSrcTuple );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void copyMtcTupleForPartitionDML( mtcTuple * aDstTuple,
                                             mtcTuple * aSrcTuple );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static void adjustMtcTupleForPartitionDML( mtcTuple * aDstTuple,
                                               mtcTuple * aSrcTuple );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static UInt getMaxRowOffset( mtcTemplate * aTemplate,
                                 qmsTableRef * aTableRef );

    // PROJ-1784 DML Without Retry
    static  IDE_RC setChkSmiValueList( void                 * aRow,
                                       const smiColumnList  * aColumnList,
                                       smiValue             * aSmiValues,
                                       const smiValue      ** aSmiValuePtr );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC checkAccessOption(
        qcmTableInfo * aTableInfo,
        idBool         aIsInsertion,
        idBool         aCheckDmlConsistency = ID_FALSE );

    /* PROJ-2359 Table/Partition Access Option */
    static IDE_RC checkAccessOptionForExistentRecord(
        qcmAccessOption   aAccessOption,
        void            * aTableHandle );

    /* PROJ-2714 Multiple Update Delete support */
    static IDE_RC executeMultiUpdate( qcStatement * aStatement );
    static IDE_RC executeMultiDelete( qcStatement * aStatement );
private:

    // delete on cascade 옵션에 의해 삭제되는 child table에 대한
    // statement trigger 수행.
    static IDE_RC fireStatementTriggerOnDeleteCascade(
        qcStatement         * aStatement,
        qmsTableRef         * aParentTableRef,  // BUG-28049
        qcmRefChildInfo     * aChildInfo,       // BUG-28049
        qcmTriggerEventTime   aTriggerEventTime );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC lockTableForDML( qcStatement      * aStatement,
                                   qmsTableRef      * aTableRef,
                                   smiTableLockMode   aLockMode );

    static IDE_RC lockTableForUpdate( qcStatement * aStatement,
                                      qmsTableRef * aTableRef,
                                      qmnPlan     * aPlan );
};

#endif  // _Q_QMX_H_

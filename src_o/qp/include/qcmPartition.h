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
 * $Id: qcmPartition.h 20699 2007-02-27 10:01:29Z a464 $
 **********************************************************************/

#ifndef _O_QCM_PARTITION_H_
#define _O_QCM_PARTITION_H_ 1

#include    <qc.h>
#include    <qcm.h>

// PROJ-1502 PARTITIONED DISK TABLE
// partition ref를 구하기 위한 임시 구조체.
typedef struct qcmSetRef
{
    qcStatement * statement;
    qmsTableRef * tableRef;
} qcmSetRef;

// PROJ-1502 PARTITIONED DISK TABLE
// 파티션 정보의 리스트를 구축
typedef struct qcmPartitionInfoList
{
    qcmTableInfo         * partitionInfo;
    void                 * partHandle;
    smSCN                  partSCN;
    qcmPartitionInfoList * next;
} qcmPartitionInfoList;

// PROJ-1502 PARTITIONED DISK TABLE
// Partition ID의 리스트를 구축
typedef struct qcmPartitionIdList
{
    UInt                 partId;
    qcmPartitionIdList * next;
} qcmPartitionIdList;

class qcmPartition
{
private:
    static IDE_RC searchTablePartitionID( smiStatement * aSmiStmt,
                                          SInt           aTableID,
                                          idBool       * aExist );
    
    static IDE_RC searchIndexPartitionID( smiStatement * aSmiStmt,
                                          SInt           aTableID,
                                          idBool       * aExist );    
    
public:
    static IDE_RC getQcmPartKeyColumns(
        smiStatement * aSmiStmt,
        qcmTableInfo * aTableInfo );

    static IDE_RC getQcmLocalIndices(
        smiStatement     * aSmiStmt,
        qcmTableInfo     * aTableInfo,
        qcmTableInfo     * aPartitionInfo );

    static IDE_RC getQcmPartitionedTableInfo(
        smiStatement * aSmiStmt,
        qcmTableInfo * aTableInfo );

    static IDE_RC getQcmPartitionedIndexInfo(
        smiStatement * aSmiStmt,
        qcmIndex     * aIndexInfo );

    static IDE_RC setPartitionRef(
        idvSQL     * aStatistics,
        const void * aRow,
        qcmSetRef  * aSetRef );

    static IDE_RC getPartCondVal(
        qcStatement        * aStatement,
        qcmColumn          * aColumns,
        qcmPartitionMethod   aPartitionMethod,
        qmsPartCondValList * aPartCondVal,
        SChar              * aPartCondValStmtText,
        qmmValueNode      ** aPartCondValNode,
        mtdCharType        * aPartKeyCondValueStr );

    static IDE_RC validateAndLockPartitions(
        qcStatement      * aStatement,
        qmsTableRef      * aTableRef,
        smiTableLockMode   aLockMode );

    static IDE_RC getAllPartitionRef(
        qcStatement    * aStatement,
        qmsTableRef    * aTableRef );

    static IDE_RC getPrePruningPartitionRef(
        qcStatement    * aStatement,
        qmsTableRef    * aTableRef );

    static IDE_RC makeMaxCondValListDefaultPart(
        qcStatement     * aStatement,
        qmsTableRef     * aTableRef,
        qmsPartitionRef * aPartitionRef );

    static IDE_RC getPartitionCount(
        qcStatement * aStatement,
        UInt          aTableID,
        UInt        * aPartitionCount );

    //get the partition count from smiStatement(BUG-46120)
    //currently, this function is only used in rp/sd.
    // BUG-47599 Real partition count(empty partition 제외한 파티션 개수)
    static IDE_RC getPartitionCount4SmiStmt(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        UInt         * aPartitionCount );

    static IDE_RC checkIndexPartitionByUser(
        qcStatement    *aStatement,
        qcNamePosition  aUserName,
        qcNamePosition  aIndexName,
        UInt            aIndexID,
        UInt           *aUserID,
        UInt           *aTablePartID );

    static IDE_RC getQcmPartitionColumn(
        qcmTableInfo * aTableInfo,
        qcmTableInfo * aPartitionInfo );

    static IDE_RC getQcmLocalIndexCommonInfo(
        qcmTableInfo * aTableInfo,
        qcmIndex     * aIndex );

    static IDE_RC getQcmPartitionConstraints(
        qcmTableInfo * aTableInfo,
        qcmTableInfo * aPartitionInfo );

    static IDE_RC makeQcmPartitionInfoByTableID(
        smiStatement * aSmiStmt,
        UInt           aTableID );

    static IDE_RC destroyQcmPartitionInfoByTableID(
        smiStatement * aSmiStmt,
        UInt           aTableID );

    static IDE_RC makeAndSetQcmPartitionInfo(
        smiStatement * aSmiStmt,
        UInt           aPartitionID,
        smOID          aPartitionOID,
        qcmTableInfo * aTableInfo,
        const void   * aTableRow = NULL );

    static IDE_RC destroyQcmPartitionInfo(
        qcmTableInfo *aInfo );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC makeAndSetAndGetQcmPartitionInfoList( qcStatement           * aStatement,
                                                        qcmTableInfo          * aTableInfo,
                                                        qcmPartitionInfoList  * aPartInfoList,
                                                        qcmPartitionInfoList ** aNewPartInfoList );

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC destroyQcmPartitionInfoList( qcmPartitionInfoList * aPartInfoList );

    /* BUG-42928 No Partition Lock */
    static void   restoreTempInfo( qcmTableInfo         * aTableInfo,
                                   qcmPartitionInfoList * aPartInfoList,
                                   qdIndexTableList     * aIndexTableList );

    /* BUG-42928 No Partition Lock */
    static void   restoreTempInfoForPartition( qcmTableInfo * aTableInfo,
                                               qcmTableInfo * aPartInfo );

    static IDE_RC getNextTablePartitionID(
        qcStatement *aStatement,
        UInt        *aTablePartitionID);

    static IDE_RC getNextIndexPartitionID(
        qcStatement *aStatement,
        UInt        *aIndexPartitionID);

    static IDE_RC getPartitionInfoByID(
        qcStatement       * aStatement,
        UInt                aPartitionID,
        qcmTableInfo     ** aPartitionInfo,
        smSCN             * aSCN,
        void             ** aPartitionHandle );

    static IDE_RC getPartitionInfo(
        qcStatement       * aStatement,
        UInt                aTableID,
        UChar             * aPartitionName,
        SInt                aPartitionNameSize,
        qcmTableInfo ** aPartitionInfo,
        smSCN             * aSCN,
        void             ** aPartitionHandle );

    static IDE_RC getPartitionInfo(
        qcStatement       * aStatement,
        UInt                aTableID,
        qcNamePosition      aPartitionName,
        qcmTableInfo ** aPartitionInfo,
        smSCN             * aSCN,
        void             ** aPartitionHandle );

    static IDE_RC getPartitionHandleByName(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        UChar        * aPartitionName,
        SInt           aPartitionNameSize,
        void        ** aPartitionHandle,
        smSCN        * aSCN );

    static IDE_RC getPartitionHandleByID(
        smiStatement  * aSmiStmt,
        UInt            aPartitionID,
        void         ** aPartitionHandle,
        smSCN         * aSCN,
        const void   ** aPartitionRow,
        const idBool    aTouchPartition = ID_FALSE);

    static IDE_RC getIndexPartitionCount(
        qcStatement * aStatement,
        UInt          aIndexID,
        SChar       * aIndexPartName,
        UInt          aIndexPartNameLen,
        UInt        * aPartitionCount );

    static IDE_RC getPartitionInfoList(
        qcStatement             * aStatement,
        smiStatement            * aSmiStmt,
        iduVarMemList           * aMem,
        UInt                      aTableID,
        qcmPartitionInfoList   ** aPartitionInfoList,
        idBool                    aIsRealPart = ID_FALSE );

    static IDE_RC getPartitionInfoList(
        qcStatement             * aStatement,
        smiStatement            * aSmiStmt,
        iduMemory               * aMem,
        UInt                      aTableID,
        qcmPartitionInfoList   ** aPartitionInfoList,
        idBool                    aIsRealPart = ID_FALSE );

    static IDE_RC getPartitionIdList(
        qcStatement         * aStatement,
        smiStatement        * aSmiStmt,
        UInt                  aTableID,
        qcmPartitionIdList ** aPartIdList );

    static IDE_RC getPartMinMaxValue(
        smiStatement * aSmiStmt,
        UInt           aPartitionID,
        mtdCharType  * aPartKeyCondMinValue,
        mtdCharType  * aPartKeyCondMaxValue );

    static IDE_RC getPartMinMaxValue(
        smiStatement * aSmiStmt,
        UInt           aPartitionID,
        SChar        * aPartKeyCondMinValueStr,
        SChar        * aPartKeyCondMaxValueStr );

    static IDE_RC validateAndLockTableAndPartitions(
        qcStatement          * aStatement,
        void                 * aPartTableHandle,
        smSCN                  aPartTableSCN,
        qcmPartitionInfoList * aPartInfoList,
        smiTableLockMode       aLockMode,
        idBool                 aIsSetViewSCN );

    static IDE_RC getPartitionIdByIndexName(
        smiStatement   * aSmiStmt,
        UInt             aIndexID,
        SChar          * aIndexPartName,
        UInt             aIndexPartNameLen,
        UInt           * aPartitionID );

    static IDE_RC getPartitionOrder(
        smiStatement * aSmiStmt,
        UInt           aTableID,
        UChar        * aPartitionName,
        SInt           aPartitionNameSize,
        UInt         * aPartOrder );

    static IDE_RC getTableIDByPartitionID(
        smiStatement          * aSmiStmt,
        UInt                    aPartitionID,
        UInt                  * aTableID);

    static IDE_RC touchPartition(
        smiStatement *  aSmiStmt ,
        UInt            aPartitionID);

    /* PROJ-2465 Tablespace Alteration for Table */
    static IDE_RC touchPartitionInfoList( smiStatement         * aSmiStmt,
                                          qcmPartitionInfoList * aPartInfoList );

    static IDE_RC getPartIdxFromIdxId( UInt           aIndexId,
                                       qmsTableRef  * aTableRef,
                                       qcmIndex    ** aPartIdx );

    static IDE_RC copyPartitionRef(
        qcStatement      * aStatement,
        qmsPartitionRef  * aSource,
        qmsPartitionRef ** aDestination );

    /* PROJ-2464 hybrid partitioned table 지원 */
    static IDE_RC makePartitionSummary( qcStatement * aStatement,
                                        qmsTableRef * aTableRef );

    /* BUG-42681 valgrind split 중 add column 동시성 문제 */
    static IDE_RC checkPartitionCount4Execute( qcStatement          * aStatement,
                                               qcmPartitionInfoList * aPartInfoList,
                                               UInt                   aTableID );

    static IDE_RC validateAndLockOnePartition( qcStatement         * aStatement,
                                               const void          * aPartitionHandle,
                                               smSCN                 aSCN,
                                               smiTBSLockValidType   aTBSLvType,
                                               smiTableLockMode      aLockMode,
                                               ULong                 aLockWaitMicroSec );

    static IDE_RC validateAndLockPartitionInfoList( qcStatement          * aStatement,
                                                    qcmPartitionInfoList * aPartInfoList,
                                                    smiTBSLockValidType    aTBSLvType,
                                                    smiTableLockMode       aLockMode,
                                                    ULong                  aLockWaitMicroSec );

    static IDE_RC getAllPartitionOID( iduMemory              * aMemory,
                                      qcmPartitionInfoList   * aPartInfoList,
                                      smOID                 ** aPartitionOID,
                                      UInt                   * aPartitionCount );

    static IDE_RC findPartitionByName( qcmPartitionInfoList  * aPartInfoList,
                                       SChar                 * aPartitionName,
                                       UInt                    aNameLength,
                                       qcmPartitionInfoList ** aFoundPartition );

    static IDE_RC isEmptyPartition( smiStatement * aSmiStmt,
                                    UInt           aTableID,
                                    idBool       * aIsEmptyPart ); 

    static IDE_RC setEmptyPartitionRef( qcStatement  * aStatement,
                                        qmsTableRef  * aTableRef );
};

#endif /* _O_QCM_PARTITION_H_ */

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
 * $Id: qdd.h 86367 2019-11-18 07:32:25Z donghyun1 $
 **********************************************************************/
#ifndef _O_QDD_H_
#define  _O_QDD_H_  1

#include <iduMemory.h>
#include <qc.h>
#include <qdParseTree.h>
#include <qcm.h>
#include <qcmPartition.h>

// parameter may vary during implementation.

// qd Drop -- Drop object
class qdd
{
public:
    //--------------
    // parsing
    //--------------
    /* PROJ-1090 Function-based Index */
    static IDE_RC parseDropIndex(
        qcStatement * aStatement );
    
    //--------------
    // validation
    //--------------
    static IDE_RC validateDropTable(
        qcStatement * aStatement );

    static IDE_RC validateDropIndex(
        qcStatement * aStatement );

    static IDE_RC validateDropUser(
        qcStatement * aStatement );

    static IDE_RC validateDropSequence(
        qcStatement * aStatement );

    static IDE_RC validateTruncateTable(
        qcStatement * aStatement );

    static IDE_RC validateDropView(
        qcStatement * aStatement );

    static IDE_RC validateDropMView(
        qcStatement * aStatement );

    //--------------
    // execution
    //--------------
    static IDE_RC executeDropTable(
        qcStatement * aStatement );

    static IDE_RC executeDropIndex(
        qcStatement * aStatement );

    static IDE_RC executeDropUser(
        qcStatement * aStatement );

    static IDE_RC executeDropUserCascade(
        qcStatement * aStatement );

    static IDE_RC executeDropSequence(
        qcStatement * aStatement);

    static IDE_RC executeTruncateTable(
        qcStatement * aStatement );

    static IDE_RC executeTruncateTempTable(
        qcStatement * aStatement );

public:
    static IDE_RC deleteConstraintsFromMeta(
        qcStatement * aStatement,
        UInt          aTableID );

    static IDE_RC deleteConstraintsFromMetaByConstraintID(
        qcStatement * aStatement,
        UInt          aTableID );

    static IDE_RC deleteFKConstraintsFromMeta(
        qcStatement * aStatement,
        UInt          aTableID,
        UInt          aReferencedTableID);

    static IDE_RC deleteIndicesFromMeta(
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo);

    static IDE_RC deleteTableFromMeta(
        qcStatement * aStatement,
        UInt          aTableID );

    static IDE_RC deleteViewFromMeta(
        qcStatement * aStatement,
        UInt          aViewID );

    static IDE_RC deleteIndicesFromMetaByIndexID(
        qcStatement * aStatement,
        UInt          aIndexID );

    static IDE_RC executeDropView(
        qcStatement * aStatement );

    static IDE_RC deleteObjectFromMetaByUserID(
        qcStatement * aStatement,
        UInt          aUserID );

    static IDE_RC executeDropTableOwnedByUser(
        qcStatement          * aStatement,
        qcmTableInfoList     * aTableInfoList,
        qcmPartitionInfoList * aPartInfoList,
        qdIndexTableList     * aIndexTables );

    static IDE_RC executeDropIndexOwnedByUser(
        qcStatement          * aStatement,
        qcmIndexInfoList     * aIndexInfoList,
        qcmPartitionInfoList * aPartInfoList);

    static IDE_RC deleteFKConstraints(
        qcStatement        * aStatement,
        qcmTableInfo       * aTableInfo,
        qcmRefChildInfo    * aChildInfo, // BUG-28049
        idBool               aDropTablespace );

    static IDE_RC executeDropTriggerOwnedByUser(
        qcStatement        * aStatement,
        qcmTriggerInfoList * aTriggerInfoList);

    static IDE_RC executeDropSequenceOwnedByUser(
        qcStatement         * aStatement,
        qcmSequenceInfoList * aSequenceInfoList);

    static IDE_RC executeDropProcOwnedByUser(
        qcStatement         * aStatement,
        UInt                  aUserID,
        qcmProcInfoList     * aProcInfoList);

    // PROJ-1073 Package
    static IDE_RC executeDropPkgOwnedByUser(
        qcStatement         * aStatement,
        UInt                  aUserID,
        qcmPkgInfoList      * aPkgInfoList);

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC dropTablePartition(
        qcStatement  * aStatement,
        qcmTableInfo * aTableInfo,
        idBool         aIsDropTablespace, // BUG-35135
        idBool         aUseRecyclebin );  /* PROJ-2441 flashback */

    static IDE_RC dropTablePartitionWithoutMeta(
        qcStatement  * aStatement,
        qcmTableInfo * aPartInfo );

    static IDE_RC deleteTablePartitionFromMeta(
        qcStatement * aStatement,
        UInt          aPartitionID );

    static IDE_RC dropIndexPartitions(
        qcStatement          * aStatement,
        qcmPartitionInfoList * aPartInfoList,
        UInt                   aDropIndexID,
        idBool                 aIsCascade,
        idBool                 aIsDropTablespace ); // BUG-35135

    static IDE_RC deleteIndexPartitionFromMeta(
        qcStatement * aStatement,
        UInt          aIndexID );

    static IDE_RC executeDropMView(
        qcStatement * aStatement );

    static IDE_RC deleteMViewFromMeta(
        qcStatement * aStatement,
        UInt          aMViewID );

    // PROJ-2264 Dictionary table
    static IDE_RC deleteCompressionTableSpecFromMetaByDicTableID(
        qcStatement * aStatement,
        UInt          aDicTableID );

    // PROJ-2264 Dictionary table
    static IDE_RC deleteCompressionTableSpecFromMeta(
        qcStatement * aStatement,
        UInt          aDataTableID );
};

#endif // _O_QDD_H_

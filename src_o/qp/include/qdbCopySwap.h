/***********************************************************************
 * Copyright 1999-2017, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id$
 **********************************************************************/
#ifndef _O_QDB_COPYSWAP_H_
#define  _O_QDB_COPYSWAP_H_ 1

#include <qc.h>
#include <qcmTableInfo.h>

class qdbCopySwap
{
public:
    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC validateCreateTableFromTableSchema( qcStatement * aStatement );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC validateReplaceTable( qcStatement * aStatement );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC executeCreateTableFromTableSchema( qcStatement * aStatement );

    /* PROJ-2600 Online DDL for Tablespace Alteration */
    static IDE_RC executeReplaceTable( qcStatement * aStatement );

    static IDE_RC executeReplacePartition( qcStatement * aStatement );

private:
    static IDE_RC getRefChildInfoList( qcStatement      * aStatement,
                                       qcmTableInfo     * aTableInfo,
                                       qcmRefChildInfo ** aRefChildInfoList );

    static IDE_RC getPartitionedTableListFromRefChildInfoList( qcStatement             * aStatement,
                                                               qcmRefChildInfo         * aRefChildInfoList,
                                                               qdPartitionedTableList ** aRefChildPartTableList );

    static void findPeerIndex( qcmTableInfo  * aMyTable,
                               qcmIndex      * aMyIndex,
                               qcmTableInfo  * aPeerTable,
                               qcmIndex     ** aPeerIndex );

    static IDE_RC compareReplicationInfo( qcStatement          * aStatement,
                                          qcmTableInfo         * aMyTableInfo,
                                          qcmPartitionInfoList * aMyPartInfoList,
                                          qcmTableInfo         * aPeerTableInfo,
                                          qcmPartitionInfoList * aPeerPartInfoList );

    static IDE_RC compareReplicationInfoPartition( qcStatement          * aStatement,
                                                   qdTableParseTree     * aParseTree, 
                                                   qcmTableInfo         * aMyTableInfo,
                                                   qcmTableInfo         * aPeerTableInfo );
    
    static IDE_RC executeCreateTablePartition( qcStatement           * aStatement,
                                               qcmTableInfo          * aMyTableInfo,
                                               qcmTableInfo          * aPeerTableInfo,
                                               qcmPartitionInfoList  * aPeerPartInfoList,
                                               qcmPartitionInfoList ** aMyPartInfoList );

    static IDE_RC createConstraintAndIndexFromInfo( qcStatement           * aStatement,
                                                    qcmTableInfo          * aOldTableInfo,
                                                    qcmTableInfo          * aNewTableInfo,
                                                    UInt                    aPartitionCount,
                                                    qcmPartitionInfoList  * aNewPartInfoList,
                                                    UInt                    aIndexCrtFlag,
                                                    qcmIndex              * aNewTableIndex,
                                                    qcmIndex             ** aNewPartIndex,
                                                    UInt                    aNewPartIndexCount,
                                                    qdIndexTableList      * aOldIndexTables,
                                                    qdIndexTableList     ** aNewIndexTables,
                                                    qcNamePosition          aNamesPrefix );

    static IDE_RC makeIndexColumnList( qcmIndex      * aIndex,
                                       qcmTableInfo  * aNewTableInfo,
                                       mtcColumn     * aNewTableIndexColumns,
                                       smiColumnList * aIndexColumnList );

    static IDE_RC createConstraintFromInfoAfterIndex( qcStatement    * aStatement,
                                                      qcmTableInfo   * aOldTableInfo,
                                                      qcmTableInfo   * aNewTableInfo,
                                                      qcmIndex       * aNewTableIndex,
                                                      qcNamePosition   aNamesPrefix );

    static IDE_RC swapTablesMeta( qcStatement * aStatement,
                                  UInt          aTargetTableID,
                                  UInt          aSourceTableID );

    static IDE_RC renameHiddenColumnsMeta( qcStatement    * aStatement,
                                           qcmTableInfo   * aTargetTableInfo,
                                           qcmTableInfo   * aSourceTableInfo,
                                           qcNamePosition   aNamesPrefix );

    static IDE_RC renameIndices( qcStatement       * aStatement,
                                 qcmTableInfo      * aTargetTableInfo,
                                 qcmTableInfo      * aSourceTableInfo,
                                 qcNamePosition      aNamesPrefix,
                                 qdIndexTableList  * aTargetIndexTableList,
                                 qdIndexTableList  * aSourceIndexTableList,
                                 qdIndexTableList ** aNewTargetIndexTableList,
                                 qdIndexTableList ** aNewSourceIndexTableList );

    static IDE_RC renameIndicesOnSM( qcStatement      * aStatement,
                                     qcmTableInfo     * aTargetTableInfo,
                                     qcmTableInfo     * aSourceTableInfo,
                                     qcNamePosition     aNamesPrefix,
                                     qdIndexTableList * aTargetIndexTableList,
                                     qdIndexTableList * aSourceIndexTableList );

    static IDE_RC checkConstraintNameAfterRename( qcStatement    * aStatement,
                                                  UInt             aUserID,
                                                  SChar          * aConstraintName,
                                                  qcNamePosition   aNamesPrefix,
                                                  idBool           aAddPrefix );

    static IDE_RC renameConstraintsMeta( qcStatement    * aStatement,
                                         qcmTableInfo   * aTargetTableInfo,
                                         qcmTableInfo   * aSourceTableInfo,
                                         qcNamePosition   aNamesPrefix );

    static IDE_RC renameCommentsMeta( qcStatement    * aStatement,
                                      qcmTableInfo   * aTargetTableInfo,
                                      qcmTableInfo   * aSourceTableInfo,
                                      qcNamePosition   aNamesPrefix );

    static IDE_RC swapTablePartitionsMetaForReplication( qcStatement  * aStatement,
                                                         qcmTableInfo * aTargetTableInfo,
                                                         qcmTableInfo * aSourceTableInfo );

    static IDE_RC updateSysConstraintsMetaForReferencedIndex( qcStatement     * aStatement,
                                                              qcmTableInfo    * aTargetTableInfo,
                                                              qcmTableInfo    * aSourceTableInfo,
                                                              qcmRefChildInfo * aTargetRefChildInfoList,
                                                              qcmRefChildInfo * aSourceRefChildInfoList );

    static IDE_RC swapGrantObjectMeta( qcStatement * aStatement,
                                       UInt          aTargetTableID,
                                       UInt          aSourceTableID );

    static IDE_RC insertReplItemMetaToReplaceHistory( qcStatement  * aStatement,
                                                      qcmTableInfo * aTargetTableInfo,
                                                      qcmTableInfo * aSourceTableInfo );

    static IDE_RC swapReplItemsMeta( qcStatement  * aStatement,
                                     qcmTableInfo * aTargetTableInfo,
                                     qcmTableInfo * aSourceTableInfo );

    static IDE_RC updateColumnDefaultValueMeta( qcStatement * aStatement,
                                                UInt          aTargetTableID,
                                                UInt          aSourceTableID,
                                                UInt          aColumnCount );

    static IDE_RC swapReplicationFlagOnTableHeader( smiStatement         * aStatement,
                                                    qcmTableInfo         * aTargetTableInfo,
                                                    qcmPartitionInfoList * aTargetPartInfoList,
                                                    qcmTableInfo         * aSourceTableInfo,
                                                    qcmPartitionInfoList * aSourcePartInfoList );

    static IDE_RC checkTablesExistInOneReplication( qcStatement  * aStatement,
                                                    qcmTableInfo * aTargetTableInfo,
                                                    qcmTableInfo * aSourceTableInfo );

    static IDE_RC checkEncryptColumn( idBool      aIsRenameForce,
                                      qcmColumn * aColumns );

    static IDE_RC checkCompressedColumnForReplication( qcStatement    * aStatement,
                                                       qcmTableInfo   * aTableInfo,
                                                       qcNamePosition   aTableName );

    static IDE_RC checkNormalUserTable( qcStatement    * aStatement,
                                        qcmTableInfo   * aTableInfo,
                                        qcNamePosition   aTableName );

    static IDE_RC swapTablePartitionsMeta( qcStatement     * aStatement,
                                           UInt              aTargetTableID,
                                           UInt              aSourceTableID,
                                           qcNamePosition    aPartitionName );

    static IDE_RC swapPartLobs( qcStatement * aStatement,
                                UInt          aTargetTableID,
                                UInt          aSourceTableID,
                                UInt          aTargetPartTableID,
                                UInt          aSourcePartTableID );

    static IDE_RC swapIndexPartitions( qcStatement  * aStatement,
                                       UInt           aTargetTableID,
                                       UInt           aSourceTableID,
                                       qcmTableInfo * aTargetPartTableInfo,
                                       qcmTableInfo * aSourcePartTableInfo );

    static IDE_RC swapReplicationFlagOnPartitonTableHeader( smiStatement  * aStatement,
                                                            qcmTableInfo  * aTargetPartTableInfo,
                                                            qcmTableInfo  * aSourcePartTableInfo );
         
    static IDE_RC insertReplItemsForParititonMetaToReplaceHistory( qcStatement  * aStatement,
                                                                   qcmTableInfo * aTargetPartTableInfo,
                                                                   qcmTableInfo * aSourcePartTableInfo );

    static IDE_RC swapReplItemsForParititonMeta( qcStatement  * aStatement,
                                                 qcmTableInfo * aTargetPartTableInfo,
                                                 qcmTableInfo * aSourcePartTableInfo );
    

    static IDE_RC validateReplacePartition( qcStatement      * aStatement,
                                            qdTableParseTree * aParseTree );

    static IDE_RC swapTablePartitionColumnID( qcStatement  * aStatement,
                                              qcmTableInfo * aTargetTablePartInfo,
                                              qcmTableInfo * aSourceTablePartInfo );
};

#endif // _O_QDB_COPYSWAP_H_


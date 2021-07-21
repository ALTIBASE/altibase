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
 * $Id: rpdCatalogDefault.h 49213 2011-11-01 06:00:07Z raysiasd $
 **********************************************************************/

#ifndef _O_RPD_CATALOG_DEFAULT_H_
#define _O_RPD_CATALOG_DEFAULT_H_ 1

#include <smi.h>
#include <rpdMeta.h>
#include <qci.h>

struct qciCatalogReplicationCallback;

class rpdCatalog
{
public:
	static qciCatalogReplicationCallback mCallback;

public:

    static IDE_RC checkReplicationExistByName( void          * aQcStatement,
                                               qciNamePosition aReplName,
                                               idBool        * aIsTrue);  

    static IDE_RC checkReplicationExistByAddr( void           * aQcStatement,
                                               qciNamePosition  aIpAddr,
                                               SInt             aPortNo,
                                               idBool         * aIsTrue);

    /* PROJ-2336 */
    static IDE_RC checkReplicationExistByNameAndAddr( void           * aQcStatement,
                                                      qciNamePosition  aIpAddr,
                                                      SInt             aPortNo,
                                                      idBool         * aIsTrue);

    static IDE_RC checkReplItemExistByName( void          * aQcStatement,
                                            qciNamePosition aReplName,
                                            qciNamePosition aReplTableName,
                                            SChar         * aReplPartitionName,
                                            idBool        * aIsTrue);

    static IDE_RC checkReplItemUnitByName( void          * aQcStatement,
                                           qciNamePosition aReplName,
                                           qciNamePosition aReplTableName,
                                           SChar         * aReplPartitionName,
                                           SChar         * aReplicationUnit,
                                           idBool        * aIsTrue );

    static IDE_RC checkReplItemExistByOID( smiStatement  * aSmiStatement,
                                           smOID           aTableOID,
                                           idBool        * aIsExist );

    // PROJ-1537
    static IDE_RC checkHostIPExistByNameAndHostIP(void           * aQcStatement,
                                                  qciNamePosition  aReplName,
                                                  SChar          * aHostIP,
                                                  idBool         * aIsTrue);

    static IDE_RC getReplicationCountWithSmiStatement( smiStatement * aSmiStmt,
                                                       UInt          * aReplicationCount); 

    static IDE_RC getReplicationCount( UInt* aReplicationCount);

    static IDE_RC checkReplicationStarted( void          * aQcStatement,
                                           qciNamePosition aReplName,
                                           idBool        * aIsStarted);  

    static IDE_RC increaseReplItemCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications );

    static IDE_RC decreaseReplItemCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications );

    static IDE_RC addReplItemCount( smiStatement     * aSmiStmt,
                                    rpdReplications  * aReplications,
                                    UInt               aAddCount );

    static IDE_RC minusReplItemCount( smiStatement     * aSmiStmt,
                                      rpdReplications  * aReplications,
                                      UInt               aMinusCount );

    static IDE_RC increaseReplHostCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications );

    static IDE_RC decreaseReplHostCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications );


    // from qcmrManager.h
    static IDE_RC insertRepl( smiStatement     * aSmiStmt,
                              rpdReplications  * aReplications );
    static IDE_RC selectRepl( smiStatement     * aSmiStmt,
                              SChar            * aRepName,
                              rpdReplications  * aReplications,
                              idBool             aForUpdateFlag);

    static IDE_RC selectReplWithCursor( smiStatement     * aSmiStmt,
                                        SChar            * aReplName,
                                        rpdReplications  * aReplications,
                                        idBool             aForUpdateFlag,
                                        smiTableCursor   * aCursor);

    static IDE_RC removeRepl( smiStatement     * aSmiStmt,
                              SChar            * aRepName );

    static IDE_RC insertReplItem( smiStatement * aSmiStmt,
                                  rpdReplItems * aReplItems);

    static IDE_RC deleteReplItem( smiStatement * aSmiStmt,
                                  rpdReplItems * aReplItems,
                                  idBool         aIsPartition );
    static IDE_RC removeReplItems( smiStatement * aSmiStmt,
                                   SChar        * aRepName );

    static IDE_RC insertReplHost( smiStatement * aSmiStmt,
                                  rpdReplHosts * aReplHosts );
    static IDE_RC deleteReplHost( smiStatement * aSmiStmt,
                                  rpdReplHosts * aReplHosts );
    static IDE_RC removeReplHosts( smiStatement * aSmiStmt,
                                   SChar        * aRepName );
    /* PROJ-1915 */
    static IDE_RC insertReplOfflineDirs(smiStatement       * aSmiStmt,
                                        rpdReplOfflineDirs * aReplOfflineDirs);
    static IDE_RC removeReplOfflineDirs(smiStatement       * aSmiStmt,
                                        SChar              * aReplName );
    static IDE_RC selectReplOfflineDirs(smiStatement       * aSmiStmt,
                                        SChar              * aReplName,
                                        rpdReplOfflineDirs * aReplOfflineDirs,
                                        SInt                 aReplOfflineDirCount);
    static IDE_RC getReplOfflineDirCount(smiStatement * aSmiStmt,
                                         SChar        * aReplName,
                                         UInt         * aReplOfflineDirCount);
    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_ITEMS_ 관련
     */
    static IDE_RC insertReplOldItem( smiStatement * aSmiStmt,
                                     SChar        * aRepName,
                                     rpdOldItem   * aItem );
    static IDE_RC deleteReplOldItem(smiStatement * aSmiStmt,
                                    SChar        * aRepName,
                                    ULong          aTableOID);
    static IDE_RC removeReplOldItems(smiStatement * aSmiStmt,
                                     SChar        * aRepName);
    static IDE_RC getReplOldItemsCount(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       vSLong       * aItemCount);
    static IDE_RC selectReplOldItems( smiStatement * aSmiStmt,
                                      SChar        * aRepName,
                                      rpdOldItem   * aItemArr,
                                      vSLong         aItemCount );

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_COLUMNS_ 관련
     */
    static IDE_RC insertReplOldColumn(smiStatement  * aSmiStmt,
                                      SChar         * aRepName,
                                      ULong           aTableOID,
                                      smiColumnMeta * aColumn);
    static IDE_RC deleteReplOldColumns(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID);
    static IDE_RC removeReplOldColumns(smiStatement * aSmiStmt,
                                       SChar        * aRepName);
    static IDE_RC getReplOldColumnsCount(smiStatement * aSmiStmt,
                                         SChar        * aRepName,
                                         ULong          aTableOID,
                                         vSLong       * aColumnCount);
    static IDE_RC selectReplOldColumns(smiStatement  * aSmiStmt,
                                       SChar         * aRepName,
                                       ULong           aTableOID,
                                       smiColumnMeta * aColumnArr,
                                       vSLong          aColumnCount);

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_INDICES_ 관련
     */
    static IDE_RC insertReplOldIndex(smiStatement * aSmiStmt,
                                     SChar        * aRepName,
                                     ULong          aTableOID,
                                     smiIndexMeta * aIndex);
    static IDE_RC deleteReplOldIndices(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID);
    static IDE_RC removeReplOldIndices(smiStatement * aSmiStmt,
                                       SChar        * aRepName);
    static IDE_RC getReplOldIndexCount(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID,
                                       vSLong       * aIndexCount);
    static IDE_RC selectReplOldIndices(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID,
                                       smiIndexMeta * aIndexArr,
                                       vSLong         aIndexCount);

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_INDEX_COLUMNS_ 관련
     */
    static IDE_RC insertReplOldIndexCol(smiStatement       * aSmiStmt,
                                        SChar              * aRepName,
                                        ULong                aTableOID,
                                        UInt                 aIndexID,
                                        smiIndexColumnMeta * aIndexCol);
    static IDE_RC deleteReplOldIndexCols(smiStatement * aSmiStmt,
                                         SChar        * aRepName,
                                         ULong          aTableOID);
    static IDE_RC removeReplOldIndexCols(smiStatement * aSmiStmt,
                                         SChar        * aRepName);
    static IDE_RC getReplOldIndexColCount(smiStatement * aSmiStmt,
                                          SChar        * aRepName,
                                          ULong          aTableOID,
                                          UInt           aIndexID,
                                          vSLong       * aIndexColCount);
    static IDE_RC selectReplOldIndexCols(smiStatement       * aSmiStmt,
                                         SChar              * aRepName,
                                         ULong                aTableOID,
                                         UInt                 aIndexID,
                                         smiIndexColumnMeta * aIndexColArr,
                                         vSLong               aIndexColCount);

    // SYS_REPL_OLD_CHECKS_
    static IDE_RC insertReplOldChecks( smiStatement       * aSmiStmt,
                                       SChar              * aReplName,
                                       ULong                aTableOID,
                                       UInt                 aConstraintID,
                                       SChar              * aCheckName,
                                       SChar              * aCondition );

    static IDE_RC deleteReplOldChecks( smiStatement * aSmiStmt,
                                       SChar        * aReplName,
                                       ULong          aTableOID );

    static IDE_RC removeReplOldChecks( smiStatement * aSmiStmt,
                                       SChar        * aRepName );

    static IDE_RC getReplOldChecksCount( smiStatement * aSmiStmt,
                                         SChar        * aReplName,
                                         ULong          aTableOID,
                                         vSLong       * aCheckCount );

    static IDE_RC selectReplOldChecks( smiStatement  * aSmiStmt,
                                       SChar         * aReplName,
                                       ULong           aTableOID,
                                       smiCheckMeta  * aCheckMeta,
                                       vSLong          aCheckMetaCount );

    static IDE_RC setReplOldCheckMember( smiCheckMeta  * aReplOldChek,
                                         const void    * aRow );

    // SYS_REPL_OLD_CHECK_COLUMNS_
    static IDE_RC insertReplOldCheckColumns( smiStatement  * aSmiStmt,
                                             SChar         * aReplName,
                                             ULong           aTableOID,
                                             UInt            aConstraintID,
                                             UInt            aColumnID );


    static IDE_RC deleteReplOldCheckColumns( smiStatement * aSmiStmt,
                                             SChar        * aReplName,
                                             ULong          aTableOID );

    static IDE_RC removeReplOldCheckColumns( smiStatement * aSmiStmt,
                                             SChar        * aReplName );

    static IDE_RC getReplOldCheckColumnsCount( smiStatement * aSmiStmt,
                                               SChar        * aReplName,
                                               ULong          aTableOID,
                                               UInt           aConstraintID,
                                               vSLong       * aCheckColumnCount );

    static IDE_RC selectReplOldCheckColumns( smiStatement          * aSmiStmt,
                                             SChar                 * aReplName,
                                             ULong                   aTableOID,
                                             UInt                    aConstraintID,
                                             smiCheckColumnMeta    * aCheckColumnMeta,
                                             vSLong                  aCheckColumnMetaCount );


    static void setReplOldCheckColumnsMember( smiCheckColumnMeta   * aReplOldChekColumn,
                                              const void           * aRow );


    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_ITEMS_의 TABLE_OID 갱신
     */
    static IDE_RC updateReplItemsTableOID(smiStatement * aSmiStmt,
                                          smOID          aBeforeTableOID,
                                          smOID          aAfterTableOID);
    static IDE_RC updateReplItemsTableOIDArray( void         * aQcStatement,
                                                smOID        * aBeforeTableOIDArray,
                                                smOID        * aAfterTableOIDArray,
                                                UInt           aTableOIDCount );
    static IDE_RC isExistInReplItemsHistory( smiStatement * aSmiStmt,
                                             SChar  * aRepName,
                                             smOID    aOID,
                                             idBool * aOutIsExist );
    static IDE_RC updateLastUsedHostNo( smiStatement  * aSmiStmt,
                                        SChar         * aRepName, 
                                        SChar         * aHostIP, 
                                        UInt            aPortNo );

    static IDE_RC updateIsStarted( smiStatement * aSmiStmt,
                                   SChar        * aRepName, 
                                   SInt           aIsActive );

    static IDE_RC updateRemoteXSN( smiStatement * aSmiStmt,
                                   SChar        * aRepName, 
                                   smSN           aSN );

    static IDE_RC updateRemoteLastDDLXSN( smiStatement * aSmiStmt,
                                          SChar        * aRepName, 
                                          smSN           aSN );

    static IDE_RC updateXSN( smiStatement * aSmiStmt,
                             SChar        * aRepName, 
                             smSN           aSN );

    static IDE_RC updateRemoteFaultDetectTime(smiStatement * aSmiStmt,
                                              SChar        * aRepName,
                                              SChar        * aOutTime);

    static IDE_RC resetRemoteFaultDetectTime(smiStatement * aSmiStmt,
                                             SChar        * aRepName);

    static IDE_RC updateGiveupTime( smiStatement * aSmiStmt,
                                    SChar        * aRepName );

    static IDE_RC resetGiveupTime( smiStatement * aSmiStmt,
                                   SChar        * aRepName );

    static IDE_RC updateGiveupXSN( smiStatement * aSmiStmt,
                                   SChar        * aRepName );

    static IDE_RC resetGiveupXSN( smiStatement * aSmiStmt,
                                  SChar        * aRepName );

    static IDE_RC updateInvalidMaxSN( smiStatement * aSmiStmt,
                                      rpdReplItems * aReplItems,
                                      smSN           aSN );

    static IDE_RC updateOldInvalidMaxSN( smiStatement * aSmiStmt,
                                         rpdReplItems * aReplItems,
                                         smSN           aSN );

    static IDE_RC updateCurrentXLogfileLSN( smiStatement * aSmiStmt,
                                            SChar        * aRepName,
                                            smLSN          aCurrentXLogfileLSN );

    static IDE_RC selectReplicationsWithSmiStatement( smiStatement    * aSmiStmt,
                                                      UInt            * aNumReplications,
                                                      rpdReplications * aReplications,
                                                      UInt              aMaxReplications );

    static IDE_RC selectReplications( UInt            * aNumReplications,
                                      rpdReplications * aReplications,
                                      UInt              aMaxReplications );
    
    static IDE_RC selectReplHostsWithSmiStatement( smiStatement   * aSmiStmt,
                                                   SChar          * aRepName,
                                                   rpdReplHosts   * aReplHosts,
                                                   SInt             aHostCount );

    static IDE_RC selectReplHosts( SChar          * aRepName,
                                   rpdReplHosts   * aReplHosts,
                                   SInt             aHostCount );
    
    static IDE_RC selectReplItems( smiStatement   * aSmiStmt,
                                   SChar          * aRepName,
                                   rpdMetaItem    * aMetaItems,
                                   SInt             aItemCount,
                                   idBool           aForUpdateFlag );

    static IDE_RC selectReplItemsWithCursor( smiStatement   * aSmiStmt,
                                             SChar          * aRepName,
                                             rpdMetaItem    * aMetaItems,
                                             SInt             aItemCount,
                                             smiTableCursor * aCursor,
                                             idBool           aForUpdateFlag );

    static IDE_RC getIndexByAddr( SInt          aLastUsedIP,
                                  rpdReplHosts *aReplHosts,
                                  SInt          aHostNumber,
                                  SInt         *aIndex );

    // from qcmtManager.h
    static IDE_RC isOtherReplUseTransWait( smiStatement  *aSmiStmt,
                                           SChar         *aReplName,
                                           mtdBigintType *aTableOID,
                                           idBool        *aResult );

    static IDE_RC updateReplTransWaitFlag( void             *  aQcStatement,
                                           SInt                aTableID,
                                           idBool              aIsTransWait,
                                           smiTBSLockValidType aTBSLvType,
                                           SChar             * aReplName );

    static IDE_RC updateReplPartitionTransWaitFlag( void              * aQcStatement,
                                                    qcmTableInfo      * aPartInfo,
                                                    idBool              aIsTransWait,
                                                    smiTBSLockValidType aTBSLvType,
                                                    SChar             * aReplName );

    static IDE_RC updateReceiverApplyCount( void         * aQcStatement,
                                            smiStatement * aSmiStmt,
                                            SChar        * aReplicationName,
                                            SInt           aReceiverApplyCount );

    static IDE_RC updateReceiverApplierInitBufferSize( smiStatement * aSmiStmt,
                                                       SChar        * aReplicationName,
                                                       ULong          aInitBufferSize );

    static IDE_RC updateReplicationFlag( void         * aQcStatement,
                                         smiStatement * aSmiStmt,
                                         SInt           aTableID,
                                         SInt           aEventFlag,
                                         smiTBSLockValidType aTBSLvType );

    static IDE_RC updatePartitionReplicationFlag( void                  * aQcStatement,
                                                  smiStatement          * aSmiStmt,
                                                  qcmTableInfo          * aPartInfo,
                                                  SInt                    aTableID,
                                                  SInt                    aEventFlag,
                                                  smiTBSLockValidType     aTBSLvType );


    static IDE_RC getNextHostNo(void        * aQcStatement,
                                SInt        * aHostNo);
    static IDE_RC updateReplMode(smiStatement  * aSmiStmt,
                                 SChar         * aRepName,
                                 SInt            aReplMode);
    static IDE_RC updateReplRecoveryCnt(void       *  aQcStatement,
                                        smiStatement* aSmiStmt,
                                        SInt          aTableID,
                                        idBool        aIsRecoveryOn,
                                        UInt          aReplRecoveryCount,
                                        smiTBSLockValidType aTBSLvType);
    static IDE_RC updatePartitionReplRecoveryCnt( void          *aQcStatement,
                                                  smiStatement  *aSmiStmt,
                                                  qcmTableInfo  *aPartInfo,
                                                  SInt           aTableID,
                                                  idBool         aIsRecoveryOn,
                                                  smiTBSLockValidType aTBSLvType );
    static IDE_RC checkReplItemRecoveryCntByName( void          * aQcStatement,
                                                  qciNamePosition aReplName,
                                                  SInt            aRecoveryFlag);
    static IDE_RC updateInvalidRecovery( smiStatement    * aSmiStmt,
                                         SChar           * aReplName,
                                         SInt              aValue );
    static IDE_RC updateAllInvalidRecovery( smiStatement    * aSmiStmt,
                                            SInt              aValue,
                                            vSLong          * aAffectedRowCnt);
    static IDE_RC updateOptions( smiStatement    * aSmiStmt,
                                 SChar           * aReplName,
                                 SInt              aOptions );
    static IDE_RC getReplRecoveryInfosCount( smiStatement * aSmiStmt,
                                             SChar        * aReplName,
                                             vSLong       * aCount );
    static IDE_RC insertReplRecoveryInfos( smiStatement    * aSmiStmt,
                                           SChar           * aRepName,
                                           rpdRecoveryInfo * aRecoveryInfos );
    static IDE_RC removeReplRecoveryInfos( smiStatement    * aSmiStmt,
                                           SChar           * aRepName );
    static IDE_RC selectReplRecoveryInfos( smiStatement    * aSmiStmt,
                                           SChar           * aRepName,
                                           rpdRecoveryInfo * aRecoveryInfos,
                                           vSLong            aCount );
    static void setReplRecoveryInfoMember( rpdRecoveryInfo * aRepl,
                                           const void      * aRow );

    static void * rpdGetTableTempInfo( const void * aTable );

    static smiColumn * rpdGetTableColumns( const void * aTable,
                                           const UInt   aIndex );

    static IDE_RC  selectAllReplications( smiStatement    * aSmiStmt,
                                          rpdReplications * aReplicationsList,
                                          SInt            * aItemCount );

    static IDE_RC  selectAllReplicationsWithCursor( smiStatement    * aSmiStmt,
                                                    rpdReplications * aReplicationsList,
                                                    SInt            * aItemCount,
                                                    smiTableCursor  * aCursor );

    static IDE_RC updateRemoteDataInit( smiStatement * aSmiStmt,
                                        SChar        * aReplName );


    static IDE_RC updateConditionalSyncedWithOID( smiStatement      * aSmiStmt,
                                                  SChar             * aReplName,
                                                  smOID               aTableOID,
                                                  idBool              aIsConditionSynced );

    static IDE_RC updateConditionalSyncedWithItem( smiStatement      * aSmiStmt,
                                                   rpdReplItems      * aReplItems,
                                                   idBool              aIsConditionSynced );

    static IDE_RC removeReplItemReplaceHistory( smiStatement * aSmiStmt,
                                                SChar        * aReplName );

    static IDE_RC deleteReplItemReplaceHistory( smiStatement * aSmiStmt,
                                                rpdReplItems * aReplItems,
                                                idBool         aIsPartition );

    static IDE_RC selectXLogfileCurrentLSNByName( smiStatement      * aSmiStatement,
                                                  SChar             * aReplName,
                                                  smLSN             * aLSN );

    static IDE_RC isConsistentReplication( void          * aQcStatement,
                                           qciNamePosition aReplName,
                                           idBool        * aIsTrue);
   
    static IDE_RC setTableOIDReferToItemReplaceHistory( smiStatement  * aSmiStmt,
                                                        rpdReplItems  * aReplItems );

private:
    static IDE_RC setReplMember( rpdReplications * aRepl, 
                                 const void      * aRow );
    static IDE_RC selectReplReceiver( smiStatement * aSmiStmt,
                                      rpdReplications * aRepl );
    static void   setReplHostMember( rpdReplHosts * aReplHost,
                                     const void   * aRow );
    static void   setReplItemMember( rpdReplItems * aReplItem,
                                     const void   * aRow );

    /* PROJ-1915 */
    static void   setReplOfflineDirMember(rpdReplOfflineDirs * aReplOfflineDirs,
                                          const void         * aRow);

    // PROJ-1442 Replication Online 중 DDL 허용
    static void   setReplOldItemMember( rpdOldItem * aReplOldItem,
                                        const void   * aRow );
    static IDE_RC setReplOldColumnMember( smiColumnMeta * aReplOldColumn,
                                          const void    * aRow );
    static void   setReplOldIndexMember(smiIndexMeta * aReplOldIndex,
                                        const void   * aRow);
    static void   setReplOldIndexColMember(smiIndexColumnMeta * aReplOldIndexCol,
                                           const void         * aRow);

    static IDE_RC searchReplHostNo( smiStatement * aSmiStmt,
                                    SInt           aReplHostNo,
                                    idBool       * aExist );
    static IDE_RC getStrForMeta( SChar        * aSrcStr,
                                 SInt           aSize,
                                 SChar       ** aStrForMeta );

    static const char *   ConnTypeEnumToString( RP_SOCKET_TYPE aConnType ); 
    static RP_SOCKET_TYPE ConnTypeStrToEnum( mtdCharType  * aConnStr );
    static rpIBLatency    IBLatencyStrToEnum( mtdCharType * aConnStr );

    static void setXLogfileCurrentLSN( smLSN * aXLogfileCurrentLSN,
                                         const void  * aRow );
};
#endif /* _O_RPD_CATALOG_DEFAULT_H_ */

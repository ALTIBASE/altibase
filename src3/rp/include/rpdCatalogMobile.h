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
 * $Id: qcmRepl0.h 48038 2011-07-12 08:48:06Z hssong $
 **********************************************************************/

#ifndef _O_RPD_CATALOG_M_H_
#define _O_RPD_CATALOG_M_H_ 1

#include <smi.h>
#include <qci.h>
#include <rpdMeta.h>

class rpdCatalog
{
public:

	static qciCatalogReplicationCallback mCallback;

    static UInt getMinimumLSN( )
    {
        return 0;
    }

    static IDE_RC checkReplicationExistByName( void          * aQcStatement,
                                               qriNamePosition aReplName,
                                               idBool        * aIsTrue)
    {
        return IDE_SUCCESS;
    }

    static IDE_RC checkReplicationExistByAddr( void           * aQcStatement,
                                               qriNamePosition  aIpAddr,
                                               SInt             aPortNo,
                                               idBool         * aIsTrue)
    {
        return IDE_SUCCESS;
    }

    static IDE_RC checkReplItemExistByName( void          * aQcStatement,
                                            qriNamePosition aReplName,
                                            smOID           aTableOID,
                                            idBool        * aIsTrue) 
    {
        return IDE_SUCCESS;
    }

    // PROJ-1537
    static IDE_RC checkHostIPExistByNameAndHostIP(void           * aQcStatement,
                                                  qriNamePosition  aReplName,
                                                  SChar          * aHostIP,
                                                  idBool         * aIsTrue)
    {
        return IDE_SUCCESS;
    }

    static IDE_RC getReplicationCountWithSmiStatement( smiStatement * aSmiStmt,
                                                       UInt         * aReplicationCount) 
    {
        return IDE_SUCCESS;
    }

    static IDE_RC getReplicationCount( UInt* aReplicationCount)
    {
        return IDE_SUCCESS;
    }

    static IDE_RC checkReplicationStarted( void          * aQcStatement,
                                           qriNamePosition aReplName,
                                           idBool        * aIsStarted)  
    {
        return IDE_SUCCESS;
    }

    static IDE_RC increaseReplItemCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC decreaseReplItemCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC increaseReplHostCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC decreaseReplHostCount( smiStatement     * aSmiStmt,
                                         rpdReplications  * aReplications )
    {
        return IDE_SUCCESS;
    }


    // from qcmrManager.h
    static IDE_RC insertRepl( smiStatement     * aSmiStmt,
                              rpdReplications  * aReplications )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectRepl( smiStatement     * aSmiStmt,
                              SChar            * aRepName,
                              rpdReplications  * aReplications,
                              idBool             aForUpdateFlag)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeRepl( smiStatement     * aSmiStmt,
                              SChar            * aRepName )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC insertReplItem( smiStatement * aSmiStmt,
                                  rpdReplItems * aReplItems )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC deleteReplItem( smiStatement * aSmiStmt,
                                  rpdReplItems * aReplItems )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplItems( smiStatement * aSmiStmt,
                                   SChar        * aRepName )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC insertReplHost( smiStatement * aSmiStmt,
                                  rpdReplHosts * aReplHosts )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC deleteReplHost( smiStatement * aSmiStmt,
                                  rpdReplHosts * aReplHosts )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplHosts( smiStatement * aSmiStmt,
                                   SChar        * aRepName )
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1915 */
    static IDE_RC insertReplOfflineDirs( smiStatement        * aSmiStmt,
                                         rpdReplOfflineDirs  * aReplOfflineDirs)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplOfflineDirs( smiStatement        * aSmiStmt,
                                         SChar               * aReplName )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplOfflineDirs(smiStatement       * aSmiStmt,
                                        SChar              * aReplName,
                                        rpdReplOfflineDirs * aReplOfflineDirs,
                                        SInt                 aReplOfflineDirCount)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC getReplOfflineDirCount( smiStatement * aSmiStmt,
                                          SChar        * aReplName,
                                          UInt         * aReplOfflineDirCount )
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_ITEMS_ 관련
     */
    static IDE_RC insertReplOldItem(smiStatement * aSmiStmt,
                                    SChar        * aRepName,
                                    smiTableMeta * aItem)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC deleteReplOldItem(smiStatement * aSmiStmt,
                                    SChar        * aRepName,
                                    ULong          aTableOID)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplOldItems(smiStatement * aSmiStmt,
                                     SChar        * aRepName)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC getReplOldItemsCount(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       vSLong       * aItemCount)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplOldItems(smiStatement * aSmiStmt,
                                     SChar        * aRepName,
                                     smiTableMeta * aItemArr,
                                     vSLong         aItemCount)
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_COLUMNS_ 관련
     */
    static IDE_RC insertReplOldColumn(smiStatement  * aSmiStmt,
                                      SChar         * aRepName,
                                      ULong           aTableOID,
                                      smiColumnMeta * aColumn)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC deleteReplOldColumns(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplOldColumns(smiStatement * aSmiStmt,
                                       SChar        * aRepName)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC getReplOldColumnsCount(smiStatement * aSmiStmt,
                                         SChar        * aRepName,
                                         ULong          aTableOID,
                                         vSLong       * aColumnCount)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplOldColumns(smiStatement  * aSmiStmt,
                                       SChar         * aRepName,
                                       ULong           aTableOID,
                                       smiColumnMeta * aColumnArr,
                                       vSLong          aColumnCount)
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_INDICES_ 관련
     */
    static IDE_RC insertReplOldIndex(smiStatement * aSmiStmt,
                                     SChar        * aRepName,
                                     ULong          aTableOID,
                                     smiIndexMeta * aIndex)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC deleteReplOldIndices(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplOldIndices(smiStatement * aSmiStmt,
                                       SChar        * aRepName)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC getReplOldIndexCount(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID,
                                       vSLong       * aIndexCount)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplOldIndices(smiStatement * aSmiStmt,
                                       SChar        * aRepName,
                                       ULong          aTableOID,
                                       smiIndexMeta * aIndexArr,
                                       vSLong         aIndexCount)
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_OLD_INDEX_COLUMNS_ 관련
     */
    static IDE_RC insertReplOldIndexCol(smiStatement       * aSmiStmt,
                                        SChar              * aRepName,
                                        ULong                aTableOID,
                                        UInt                 aIndexID,
                                        smiIndexColumnMeta * aIndexCol)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC deleteReplOldIndexCols(smiStatement * aSmiStmt,
                                         SChar        * aRepName,
                                         ULong          aTableOID)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplOldIndexCols(smiStatement * aSmiStmt,
                                         SChar        * aRepName)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC getReplOldIndexColCount(smiStatement * aSmiStmt,
                                          SChar        * aRepName,
                                          ULong          aTableOID,
                                          UInt           aIndexID,
                                          vSLong       * aIndexColCount)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplOldIndexCols(smiStatement       * aSmiStmt,
                                         SChar              * aRepName,
                                         ULong                aTableOID,
                                         UInt                 aIndexID,
                                         smiIndexColumnMeta * aIndexColArr,
                                         vSLong               aIndexColCount)
    {
        return IDE_SUCCESS;
    }

    /* PROJ-1442 Replication Online 중 DDL 허용
     * SYS_REPL_ITEMS_의 TABLE_OID 갱신
     */
    static IDE_RC updateReplItemsTableOID(smiStatement * aSmiStmt,
                                          smOID          aBeforeTableOID,
                                          smOID          aAfterTableOID)
    {
        return IDE_SUCCESS;
    }

    static IDE_RC updateLastUsedHostNo( smiStatement  * aSmiStmt,
                                        SChar         * aRepName, 
                                        SChar         * aHostIP, 
                                        UInt            aPortNo )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC updateIsStarted( smiStatement * aSmiStmt,
                                   SChar        * aRepName, 
                                   SInt           aIsActive )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC updateXLSN( smiStatement * aSmiStmt,
                              SChar        * aRepName, 
                              UInt           aFileNo, 
                              UInt           aOffset )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC selectReplicationsWithSmiStatement( smiStatement    * aSmiStmt,
                                                      UInt            * aNumReplications,
                                                      rpdReplications * aReplications,
                                                      UInt              aMaxReplications )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC selectReplications( UInt            * aNumReplications,
                                      rpdReplications * aReplications,
                                      UInt              aMaxReplications )
    {
        return IDE_SUCCESS;
    }
    
    static IDE_RC selectReplHostsWithSmiStatement( smiStatement   * aSmiStmt,
                                   SChar          * aRepName,
                                   rpdReplHosts   * aReplHosts,
                                   SInt             aHostCount )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC selectReplHosts( SChar          * aRepName,
                                   rpdReplHosts   * aReplHosts,
                                   SInt             aHostCount )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplItems( smiStatement   * aSmiStmt,
                                   SChar          * aRepName,
                                   rpdMetaItem    * aMetaItems,
                                   SInt             aItemCount )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC getIndexByAddr( SInt          aLastUsedIP,
                                  rpdReplHosts *aReplHosts,
                                  SInt          aHostNumber,
                                  SInt         *aIndex )
    {
        return IDE_SUCCESS;
    }

    // from qcmtManager.h
    static IDE_RC updateReplicationFlag( void         * aQcStatement,
                                         smiStatement * aSmiStmt,
                                         SInt           aTableID,
                                         SInt           aEventFlag,
                                         smiTBSLockValidType aTBSLvType )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC getNextHostNo(void        * aQcStatement,
                                SInt        * aHostNo)
    {
        return IDE_SUCCESS;
    }
    //proj-1608 recovery from replication
    static IDE_RC updateReplRecoveryCnt(void       *  aQcStatement,
                                        smiStatement* aSmiStmt,
                                        SInt          aTableID,
                                        idBool        aIsRecoveryOn,
                                        UInt          aReplRecoveryCount,
                                        smiTBSLockValidType aTBSLvType)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC checkReplItemRecoveryCntByName( void          * aQcStatement,
                                                  qriNamePosition aReplName,
                                                  SInt            aRecoveryFlag)
    {
        return IDE_SUCCESS;
    }
    static IDE_RC updateInvalidRecovery( smiStatement    * aSmiStmt,
                                         SChar           * aReplName,
                                         SInt              aValue )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC updateOptions( smiStatement    * aSmiStmt,
                                 SChar           * aReplName,
                                 SInt              aOptions )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC getReplRecoveryInfosCount( smiStatement * aSmiStmt,
                                             SChar        * aReplName,
                                             vSLong       * aCount )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC insertReplRecoveryInfos( smiStatement    * aSmiStmt,
                                           SChar           * aRepName,
                                           rpdRecoveryInfo * aRecoveryInfos )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC removeReplRecoveryInfos( smiStatement*  aSmiStmt,
                                           SChar*         aRepName )
    {
        return IDE_SUCCESS;
    }
    static IDE_RC selectReplRecoveryInfos( smiStatement    * aSmiStmt,
                                           SChar           * aRepName,
                                           rpdRecoveryInfo * aRecoveryInfos,
                                           vSLong            aCount )
    {
        return IDE_SUCCESS;
    }
    static void setReplRecoveryInfoMember( rpdRecoveryInfo * aRepl,
                                           const void      * aRow )
    {
        return IDE_SUCCESS;
    }

private:
    static IDE_RC setReplMember( rpdReplications * aRepl, 
                                 mtcColumn       * aColumn,
                                 const void      * aRow )
    {
        return ;
    }
    static void   setReplHostMember( rpdReplHosts * aReplHost, 
                                     mtcColumn    * aColumn,
                                     const void   * aRow )
    {
        return ;
    }
    /* PROJ-1915 */
    static void   setReplOfflineDirMember( rpdReplOfflineDirs * aReplOfflineDirs, 
                                           mtcColumn          * aColumn,
                                           const void         * aRow )
    {
        return ;
    }
    
    static void   setReplItemMember( rpdReplItems * aReplItem, 
                                     mtcColumn    * aColumn,
                                     const void   * aRow )
    {
        return ;
    }

    static IDE_RC searchReplHostNo( smiStatement * aSmiStmt,
                                    SInt           aReplHostNo,
                                    idBool       * aExist )
    {
        return IDE_SUCCESS;
    }
    
	// SYS_REPL_OLD_CHECKS_
    static IDE_RC insertReplOldChecks( smiStatement       * aSmiStmt,
                                       SChar              * aReplName,
                                       ULong                aTableOID,
                                       UInt                 aConstraintID,
                                       SChar              * aCheckName
                                       SChar              * aCondition )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC deleteReplOldChecks( smiStatement * aSmiStmt,
                                       SChar        * aReplName,
                                       ULong          aTableOID )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC getReplOldChecksCount( smiStatement * aSmiStmt,
                                         SChar        * aReplName,
                                         ULong          aTableOID,
                                         vSLong       * aCheckCount )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC selectReplOldChecks( smiStatement  * aSmiStmt,
                                       SChar         * aReplName,
                                       ULong           aTableOID,
                                       smiCheckMeta  * aCheckMeta,
                                       vSLong          aCheckMetaCount )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC setReplOldCheckMember( smiCheckMeta  * aReplOldChek,
                                         const void    * aRow )
    {
        return IDE_SUCCESS;
    }

    // SYS_REPL_OLD_CHECK_COLUMNS_
    static IDE_RC insertReplOldCheckColumns( smiStatement  * aSmiStmt,
                                             SChar         * aReplName,
                                             ULong           aTableOID,
                                             UInt            aConstraintID,
                                             UInt            aColumnID )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC deleteReplOldCheckColumns( smiStatement * aSmiStmt,
                                             SChar        * aReplName,
                                             ULong          aTableOID )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC getReplOldCheckColumnsCount( smiStatement * aSmiStmt,
                                               SChar        * aReplName,
                                               ULong          aTableOID,
                                               UInt           aConstraintID,
                                               vSLong       * aCheckColumnCount )
    {
        return IDE_SUCCESS;
    }

    static IDE_RC selectReplOldCheckColumns( smiStatement          * aSmiStmt,
                                             SChar                 * aReplName,
                                             ULong                   aTableOID,
                                             UInt                    aConstraintID,
                                             smiCheckColumnMeta    * aCheckColumnMeta,
                                             vSLong                  aCheckColumnMetaCount )
    {
        return IDE_SUCCESS;
    }


    static void setReplOldCheckColumnsMember( smiCheckColumnMeta   * aReplOldChekColumn,
                                              const void           * aRow )
    {
        return IDE_SUCCESS;
    }
};
#endif /* _O_RPD_CATALOG_M_H_ */

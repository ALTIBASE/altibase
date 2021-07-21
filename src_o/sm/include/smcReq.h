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
 * $Id: smcReq.h 91121 2021-07-02 04:46:19Z jiwon.kim $
 **********************************************************************/

#ifndef _O_SMC_REQ_H_
#define _O_SMC_REQ_H_  1

#include <idl.h> /* for win32 */
#include <smcAPI.h>
#include <smxAPI.h>
#include <smlAPI.h>
#include <sdcAPI.h>
#include <svcAPI.h>
#include <sdnAPI.h>
#include <smiAPI.h>
#include <smnAPI.h>

class smcReqFunc 
{
    public:

        /* smi */
        static void setEmergency( idBool aFlag )
        {
            smiSetEmergency( aFlag );
        };
    
        /* sdc api function */
        static UInt getDiskLobDescSize()
        {
            return sdcLob::getDiskLobDescSize();
        };

        /* svc */
        static IDE_RC makeVolNullRow( void              * aTrans,
                                      smcTableHeader    * aHeader,
                                      smSCN               aSCN,
                                      const smiValue    * aNullRow,
                                      UInt                aFlag,
                                      smOID             * aNullRowOID )
        {
            return svcRecord::makeNullRow( aTrans,
                                           aHeader,
                                           aSCN,
                                           aNullRow,
                                           aFlag,
                                           aNullRowOID );
        };

        /* smx api function */
        static void incRecCntOfTableInfo( void * aTableInfoPtr )
        {
            smxTableInfoMgr::incRecCntOfTableInfo( aTableInfoPtr );
        };

        static void decRecCntOfTableInfo( void * aTableInfoPtr )
        {
            smxTableInfoMgr::decRecCntOfTableInfo( aTableInfoPtr );
        };

        static SLong getRecCntOfTableInfo( void * aTableInfoPtr )
        {
            return smxTableInfoMgr::getRecCntOfTableInfo( aTableInfoPtr );
        };

        static IDE_RC undoDeleteOfTableInfo( void   * aTrans,
                                             smOID    aTableOID )
        {
            return smxTrans::undoDeleteOfTableInfo( aTrans,
                                                    aTableOID );
        };

        static IDE_RC undoInsertOfTableInfo( void   * aTrans,
                                             smOID    aTableOID )
        {
            return smxTrans::undoInsertOfTableInfo( aTrans,
                                                    aTableOID );
        };

        static IDE_RC addOIDByTID( smTID     aTID,
                                   smOID     aTblOID, 
                                   smOID     aRecOID,
                                   scSpaceID aSpaceID,
                                   UInt      aFlag )
        {
            return smxTrans::addOIDByTID( aTID,
                                          aTblOID, 
                                          aRecOID,
                                          aSpaceID,
                                          aFlag );
        };

        static IDE_RC addOID( void *    aTrans,
                              smOID     aTblOID, 
                              smOID     aRecOID,
                              scSpaceID aSpaceID,
                              UInt      aFlag )
        {
            return smxTrans::addOID2Trans( aTrans,
                                           aTblOID, 
                                           aRecOID,
                                           aSpaceID,
                                           aFlag );
        };

        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };

        static smLSN * getLstUndoNxtLSNPtr( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSNPtr( aTrans );
        };

        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };

        static void addMutexToTrans( void * aTrans, void  *aMutex )
        {
            smxTrans::addMutexToTrans( aTrans, aMutex );
        };

        static IDE_RC writeLogToBuffer( void        * aTrans,
                                        const void  * aLog, 
                                        UInt          aOffset,
                                        UInt          aLogSize )
        {
            return smxTrans::writeLogToBufferOfTx( aTrans,
                                                   aLog, 
                                                   aOffset,
                                                   aLogSize );
        };

        static void initLogBuffer( void *aTrans )
        {
            smxTrans::initTransLogBuffer( aTrans );
        };

        static SChar * getLogBufferOfTrans( void * aTrans )
        {
            return smxTrans::getTransLogBuffer( aTrans );
        };

        static UInt getLogTypeFlagOfTrans( void * aTrans )
        {
            return smxTrans::getLogTypeFlagOfTrans( aTrans );
        };

        static void addToUpdateSizeOfTrans( void * aTrans, UInt aSize )
        {
            smxTrans::addToUpdateSizeOfTrans( aTrans, aSize );
        };

        static void * getTransByTID( smTID aTID )
        {
            return smxTransMgr::getTransByTID2Void( aTID );
        };

        static IDE_RC waitLockForTrans( void    * aTrans,
                                        smTID     aWaitTransID, 
                                        scSpaceID aSpaceID,
                                        ULong     aLockWaitTime )
        {
            return smxTransMgr::waitForTrans( aTrans,
                                              aWaitTransID, 
                                              aSpaceID,
                                              aLockWaitTime );
        };

        static idBool checkAndSetImplSVPStmtDepth4Repl( void * aTrans )
        {
            return smxTrans::checkAndSetImplSVPStmtDepth4Repl( aTrans );
        };

        static idBool isPsmSvpReserved( void * aTrans )
        {
            return smxTrans::isPsmSvpReserved( aTrans );
        };

        static IDE_RC writePsmSvp( void * aTrans )
        {
            return smxTrans::writePsmSvp( aTrans );
        };

        static void setIsTransWaitRepl( void * aTrans, idBool aIsWaitRepl ) /* BUG-39143 */
        {
            smxTrans::setIsTransWaitRepl( aTrans, aIsWaitRepl );
        };

        static UInt getMemLobCursorCnt( void    * aTrans,
                                        UInt      aColumnID,
                                        void    * aRow )
        {
            return smxTrans::getMemLobCursorCnt( aTrans,
                                                 aColumnID,
                                                 aRow );
        };

        static UInt getLstReplStmtDepth( void * aTrans )
        {
            return smxTrans::getLstReplStmtDepth( aTrans );
        };

        static IDE_RC writeTransLog( void * aTrans, smOID aTableOID )
        {
            return smxTrans::writeTransLog( aTrans, aTableOID );
        };

        static IDE_RC setImpSavepoint( void     * aTrans,
                                       void    ** aSavepoint,
                                       UInt       aStmtDepth )
        {
            return smxTrans::setImpSavepoint4LayerCall( aTrans,
                                                        aSavepoint,
                                                        aStmtDepth );
        };

        static IDE_RC unsetImpSavepoint( void   * aTrans,
                                         void   * aSavepoint )
        {
            return smxTrans::unsetImpSavepoint4LayerCall( aTrans,
                                                          aSavepoint );
        };

        static IDE_RC abortToImpSavepoint( void     * aTrans,
                                           void     * aSavepoint )
        {
            return smxTrans::abortToImpSavepoint4LayerCall( aTrans,
                                                            aSavepoint );
        };

        //BUG-49062
        static IDE_RC beginTx( void   * aTrans,
                               UInt     aFlag,
                               idvSQL * aStatistics )
        {
            return smxTrans::begin4LayerCall( aTrans,
                                              aFlag,
                                              aStatistics );
        };
        static IDE_RC commitTx( void * aTrans )
        {
            return smxTrans::commit4LayerCall( aTrans );
        };
        static IDE_RC abortTx( void * aTrans )
        {
            return smxTrans::abort4LayerCall( aTrans );
        };
        static IDE_RC allocTx( void ** aTrans )
        {
            return smxTransMgr::alloc4LayerCall( aTrans );
        };
        static IDE_RC freeTx( void * aTrans )
        {
            return smxTransMgr::freeTrans4LayerCall( aTrans );
        };


        /* sml api Function */
        static IDE_RC allocLockItem( void ** aLockItem )
        {
            return smlLockMgr::allocLockItem( aLockItem );
        };

        static IDE_RC freeLockItem( void * aLockItem )
        {
            return smlLockMgr::freeLockItem( aLockItem );
        };

        static IDE_RC initLockItem( scSpaceID          aSpaceID,
                                    ULong              aItemID,
                                    smiLockItemType    aLockItemType,
                                    void             * aLockItem )
        {
            return smlLockMgr::initLockItem( aSpaceID,
                                             aItemID,
                                             aLockItemType,
                                             aLockItem );
        };

        static IDE_RC destroyLockItem( void * aLockItem )   
        {
            return smlLockMgr::destroyLockItem( aLockItem );
        };

        static IDE_RC lockTableModeX( void * aTrans, void * aLockItem ) 
        {
            return smlLockMgr::lockTableModeX( aTrans, aLockItem );
        };

        static IDE_RC lockTableModeIX( void * aTrans, void * aLockItem )
        {
            return smlLockMgr::lockTableModeIX( aTrans, aLockItem );
        };

        static IDE_RC lockTableModeIS( void * aTrans, void * aLockItem )
        {
            return smlLockMgr::lockTableModeIS( aTrans, aLockItem );
        };
        static IDE_RC lockTableModeIS4FixedTable( void * aTrans, void * aLockItem )
        {
            return smlLockMgr::lockTableModeIS4FixedTable( aTrans, aLockItem );
        };

        static IDE_RC lockTableModeXAndCheckLocked( void    * aTrans, 
                                                    void    * aLockItem, 
                                                    idBool  * aIsLock )
        {
            return smlLockMgr::lockTableModeXAndCheckLocked( aTrans, 
                                                             aLockItem, 
                                                             aIsLock );
        };

        static iduMutex * getMutexOfLockItem( void * aLockItem )
        {
            return smlLockMgr::getMutexOfLockItem( aLockItem );
        };

        /* smn api function */
        static idBool isIndexToVerifyIntegrity( const void * aIndexHeader )
        {
            return smnManager::isIndexToVerifyIntegrity( aIndexHeader );
        };

        static IDE_RC createIndexes( idvSQL            * aStatistics, 
                                     void              * aTrans, 
                                     smcTableHeader    * aTable,
                                     idBool              aIsRestartRebuild,
                                     idBool              aIsNeedValidation,
                                     smiSegAttr        * aSegAttr,
                                     smiSegStorageAttr * aSegStoAttr )
        {
            return smnManager::createIndexes( aStatistics, 
                                              aTrans, 
                                              aTable,
                                              aIsRestartRebuild,
                                              aIsNeedValidation,
                                              aSegAttr,
                                              aSegStoAttr );
        };

        static IDE_RC dropIndexes( smcTableHeader * aTable ) 
        {
            return smnManager::dropIndexes( aTable ); 
        };

        static IDE_RC dropIndex( smcTableHeader * aTable,
                                 void           * aIndexHeader )
        {
            return smnManager::dropIndex( aTable,
                                          aIndexHeader );
        };

        static UInt * getFlagPtrOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getFlagPtrOfIndexHeader( aIndexHeader );
        };

        static smOID getTableOIDOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getTableOIDOfIndexHeader( aIndexHeader );
        };

        static scGRID * getIndexSegGRIDPtr( void * aIndexHeader )
        {
            return smnManager::getIndexSegGRIDPtr( aIndexHeader );
        };

        static UInt * getIndexFlagPtr( void * aIndexHeader )
        {
            return smnManager::getIndexFlagPtr( aIndexHeader );
        };

        static UInt getIndexIDOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getIndexIDOfIndexHeader( aIndexHeader );
        };

        static UInt getSizeOfIndexHeader()
        {
            return smnManager::getSizeOfIndexHeader();
        };

        static smiSegStorageAttr * getIndexSegStoAttrPtr( void * aIndexHeader )
        {
            return smnManager::getIndexSegStoAttrPtr( aIndexHeader );
        };

        static smiSegAttr * getIndexSegAttrPtr( void * aIndexHeader )
        {
            return smnManager::getIndexSegAttrPtr( aIndexHeader );
        };

        static void setIndexSegStoAttrPtr( void             * aIndexHeader,
                                           smiSegStorageAttr  aSegStoAttr )
        {
            smnManager::setIndexSegStoAttrPtr( aIndexHeader,
                                               aSegStoAttr );
        };

        static void setIndexSegAttrPtr( void        * aIndexHeader,
                                        smiSegAttr    aSegAttr )
        {
            smnManager::setIndexSegAttrPtr( aIndexHeader,
                                            aSegAttr );
        };

        static IDE_RC indexDeleteFunc( void     * aIndexHeader, 
                                       SChar    * aRow, 
                                       idBool     aIgnoreNotFoundKey,
                                       idBool   * aExistKey )
        {
            return smnManager::indexDeleteFunc( aIndexHeader, 
                                                aRow, 
                                                aIgnoreNotFoundKey,
                                                aExistKey );
        };

        static void initIndexHeader( void                * aIndexHeader,
                                     smOID                 aTableSelfOID,
                                     SChar               * aName, 
                                     UInt                  aID,
                                     UInt                  aType,
                                     UInt                  aFlag,
                                     const smiColumnList * aColumns,
                                     smiSegAttr          * aSegAttr,
                                     smiSegStorageAttr   * aSegStoAttr,
                                     ULong                 aDirectKeyMaxSize )
        {
            smnManager::initIndexHeader( aIndexHeader,
                                         aTableSelfOID,
                                         aName, 
                                         aID,
                                         aType,
                                         aFlag,
                                         aColumns,
                                         aSegAttr,
                                         aSegStoAttr,
                                         aDirectKeyMaxSize );
        };

        static IDE_RC reInitIndex( idvSQL * aStatistics, void * aIndex )
        {
            return smnManager::reInitIndex( aStatistics, aIndex );
        };

        static UInt getColumnCountOfIndexHeader( void *aIndexHeader )
        {
            return smnManager::getColumnCountOfIndexHeader( aIndexHeader );
        };

        static UInt * getColumnIDPtrOfIndexHeader( void * aIndexHeader, UInt aIndex )
        {
            return smnManager::getColumnIDPtrOfIndexHeader( aIndexHeader, aIndex );
        };

        static UInt getFlagOfIndexHeader( void *aIndexHeader )
        {
            return smnManager::getFlagOfIndexHeader( aIndexHeader );
        };

        static void setFlagOfIndexHeader( void * aIndexHeader, UInt aFlag )
        {
            smnManager::setFlagOfIndexHeader( aIndexHeader, aFlag );
        };

        static idBool getIsConsistentOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getIsConsistentOfIndexHeader( aIndexHeader );
        };

        static void setIsConsistentOfIndexHeader( void  * aIndexHeader, 
                                                  idBool  aFlag )
        {
            smnManager::setIsConsistentOfIndexHeader( aIndexHeader, 
                                                      aFlag );
        };

        static SChar * getNameOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getNameOfIndexHeader( aIndexHeader );
        };

        static void setNameOfIndexHeader( void * aIndexHeader, SChar * aName )
        {
            smnManager::setNameOfIndexHeader( aIndexHeader, aName );
        };

        static UInt getIndexNameOffset()
        {
            return smnManager::getIndexNameOffset();
        };

        static IDE_RC deleteRowFromIndex( SChar          * aRow, 
                                          smcTableHeader * aHeader,
                                          ULong            aModifyIdxBit )
        {
            return smnManager::deleteRowFromIndex( aRow, 
                                                   aHeader,
                                                   aModifyIdxBit );
        };

        static IDE_RC initIndex( idvSQL             * aStatistics,
                                 smcTableHeader     * aTable,
                                 void               * aIndex,
                                 smiSegAttr         * aSegAttr,
                                 smiSegStorageAttr  * aSegStorageAttr,
                                 void              ** aRebuildIndexHeader,
                                 ULong                aSmoNo )
        {
            return smnManager::initIndex( aStatistics,
                                          aTable,
                                          aIndex,
                                          aSegAttr,
                                          aSegStorageAttr,
                                          aRebuildIndexHeader,
                                          aSmoNo );
        };

        static idBool isNullModuleOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::isNullModuleOfIndexHeader( aIndexHeader );
        };

        static void setInitIndexPtr( void * aIndexHeader )
        {
            smnManager::setInitIndexPtr( aIndexHeader );
        };

        static void setIndexCreated( void * aIndexHeader, idBool aIsCreated )
        {
            smnManager::setIndexCreated( aIndexHeader, aIsCreated );
        };

        static UInt getMaxIndexCnt()    
        {
            return smnManager::getMaxIndexCnt();
        };

        static idBool isPrimaryIndex( void * aIndexHeader )
        {
            return smnManager::isPrimaryIndex( aIndexHeader );
        };

        static void * getSegDescByIdxPtr( void * aIndex )
        {
            return smnManager::getSegDescByIdxPtr( aIndex );
        };

        static idBool IsBeginTrans( void * aTrans )
        {
            return smxTrans::isTxBeginStatus( aTrans );
        };

        static IDE_RC dropMemAndVolPrivatePageList( void           * aTrans,
                                                    smcTableHeader * aTable )
        {
            return smxTrans::dropMemAndVolPrivatePageList( aTrans,
                                                           aTable );
        };

        /* sdn api function */
        static IDE_RC verifyIndexIntegrity( idvSQL*   aStatistics, void * aIndex )
        {
            return sdnbBTree::verifyIndexIntegrity( aStatistics, aIndex );
        };

        /* BUG-24403 */
        static void getMaxSmoNoOfAllIndexes( smcTableHeader * aTable,
                                             ULong          * aMaxSmoNo )
        {
            return smnManager::getMaxSmoNoOfAllIndexes( aTable,
                                                        aMaxSmoNo );
        };
};

#define smLayerCallback   smcReqFunc

#endif

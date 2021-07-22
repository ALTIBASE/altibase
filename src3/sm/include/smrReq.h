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
 * $Id: smrReq.h 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#ifndef _O_SMR_REQ_H_
#define _O_SMR_REQ_H_  1

#include <idl.h> /* for win32 */
#include <smxAPI.h>
#include <smlAPI.h>
#include <sdpAPI.h>
#include <sdnAPI.h>
#include <smiAPI.h>
#include <smnAPI.h>
#include <smaAPI.h>
#include <sdsAPI.h>
#include <smpAPI.h>
#include <smcAPI.h>

class smrUTransQueue;
class smrLogFileGroup;

class smrReqFunc
{
    public:

        /* smi api function */
        static void setEmergency( idBool aFlag )
        {
            return smiSetEmergency( aFlag );
        };

        static void switchDDLFunc( UInt aFlag )
        {
            smiSwitchDDL( aFlag );
        };

        static UInt getCurrTime()
        {
            return smiGetCurrTime();
        };

        static IDE_RC restoreTableByAbort( void    *  aTrans,
                                           smOID      aSrcHeaderOID,
                                           smOID      aDstHeaderOID,
                                           idBool     aDoRestart )
        {
            return smiTable::restoreTableByAbort( aTrans,
                                                  aSrcHeaderOID,
                                                  aDstHeaderOID,
                                                  aDoRestart );

        };

        /* sma api function */
        static IDE_RC  blockDeleteThr()
        {
            return smaDeleteThread::lock();
        };

        static IDE_RC  unblockDeleteThr()
        {
            return smaDeleteThread::unlock();
        };

        /* sml api function */
        static void lockTableByPreparedLog( void     * aTrans,
                                            SChar    * aLogPtr,
                                            UInt       aLockCnt,
                                            UInt     * aOffset )
        {
            smlLockMgr::lockTableByPreparedLog( aTrans,
                                                aLogPtr,
                                                aLockCnt,
                                                aOffset );
        };

        /* smx api function */
        static smrRTOI * getRTOI4UndoFailure( void * aTrans )
        {
            return smxTrans::getRTOI4UndoFailure( aTrans );
        };

        static void getTransInfo( void   * aTrans,
                                  SChar ** aTransLogBuffer,
                                  smTID  * aTID,
                                  UInt   * aTransLogType )
        {
            smxTrans::getTransInfo( aTrans,
                                    aTransLogBuffer,
                                    aTID,
                                    aTransLogType );
        };

        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };

        static void setLstUndoNxtLSN( void   * aTrans,
                                      smLSN    aLSN )
        {
            smxTrans::setTransLstUndoNxtLSN( aTrans,
                                             aLSN );
        };

        static smLSN getCurUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransCurUndoNxtLSN( aTrans );
        };

        static void setCurUndoNxtLSN( void * aTrans, smLSN * aLSN )
        {
            smxTrans::setTransCurUndoNxtLSN ( aTrans, aLSN );
        };

        static void getTxIDAnLogType( void  * aTrans,
                                      smTID * aTID,
                                      UInt  * aLogType )
        {
            smxTrans::getTxIDAnLogType( aTrans,
                                        aTID,
                                        aLogType );
        };

        static idBool getIsFirstLog( void * aTrans )
        {
            return smxTrans::getIsFirstLog( aTrans );
        };

        static void setIsFirstLog( void * aTrans, idBool aFlag )
        {
            smxTrans::setIsFirstLog( aTrans, aFlag );
        };

        static void getMinLSNOfAllActiveTrans( smLSN * aLSN )
        {
            smxTransMgr::getMinLSNOfAllActiveTrans( aLSN );
        };

        static void addActiveTrans( void    * aTrans,
                                    smTID     aTID,
                                    UInt      aReplFlag,
                                    idBool    aIsBeginLog,
                                    smLSN   * aLstRedoLSN )
        {
            smxTransMgr::addActiveTrans( aTrans,
                                         aTID,
                                         aReplFlag,
                                         aIsBeginLog,
                                         aLstRedoLSN );
        };

        static idBool IsBeginTrans( void * aTrans )
        {
            return smxTrans::isTxBeginStatus( aTrans );
        };

        static IDE_RC addOID( void      * aTrans,
                              smOID       aTblOID,
                              smOID       aRecOID,
                              scSpaceID   aSpaceID,
                              UInt        aFlag )
        {
            return smxTrans::addOID2Trans( aTrans,
                                           aTblOID,
                                           aRecOID,
                                           aSpaceID,
                                           aFlag );
        };

        static IDE_RC addOIDToVerify( void      * aTrans,
                                      smOID       aTableOID,
                                      smOID       aIndexOID,
                                      scSpaceID   aSpaceID )
        {
            return smxTrans::addOIDToVerify( aTrans,
                                             aTableOID,
                                             aIndexOID,
                                             aSpaceID );
        };

        static void * getTransByTID( smTID aTID )
        {
            return smxTransMgr::getTransByTID2Void( aTID );
        };

        static IDE_RC setXAInfoAnAddPrepareLst( void    * aTrans,
                                                idBool    aIsGCTx,
                                                timeval   aTimeVal,
                                                ID_XID    aXID, /* BUG-18981 */
                                                smSCN   * aFstDskViewSCN )
        {
            return smxTransMgr::setXAInfoAnAddPrepareLst( aTrans,
                                                          aIsGCTx,
                                                          aTimeVal,
                                                          aXID, /* BUG-18981  */
                                                          aFstDskViewSCN );
        };

        static IDE_RC makeTransEnd( void    * aTrans,
                                    idBool    aIsCommit )
        {
            return smxTransMgr::makeTransEnd( aTrans,
                                              aIsCommit );
        };

        static IDE_RC insertUndoLSNs( smrUTransQueue * aTransQueue )
        {
            return smxTransMgr::insertUndoLSNs( aTransQueue );
        };

        static IDE_RC abortAllActiveTrans()
        {
            return smxTransMgr::abortAllActiveTrans();
        };

        static IDE_RC setRowSCNForInDoubtTrans()
        {
            return smxTransMgr::setRowSCNForInDoubtTrans();
        };

        static UInt getPrepareTransCount()
        {
            return smxTransMgr::getPrepareTransCount();
        };

        static void * getNxtPreparedTrx( void * aTrans )
        {
            return smxTransMgr::getNxtPreparedTrx( aTrans );
        };

        static UInt getActiveTransCnt()
        {
            return smxTransMgr::getActiveTransCount();
        };

        static void checkFreeTransList()
        {
            smxTransMgr::checkFreeTransList();
        };

        static SInt getCurTransCnt()
        {
            return smxTransMgr::getCurTransCnt(); /* BUG-35392 */
        };

        static idBool isXAPreparedCommitState( void * aTrans )
        {
            return smxTrans::isXAPreparedCommitState( aTrans );
        };

        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };

        static SInt getTransSlot( void * aTrans )
        {
            return smxTrans::getTransSlot( aTrans ); /* BUG-35392 */
        };

        static void enableTransBegin()
        {
            smxTransMgr::enableTransBegin();
        };

        static void initActiveTransLst()
        {
            smxTransMgr::initATLAnPrepareLst();
        };

        static idBool isEmptyActiveTrans()
        {
            return smxTransMgr::isEmptyActiveTrans();
        };

        static void * getFirstPreparedTrx()
        {
            return smxTransMgr::getFirstPreparedTrx();
        };

        static void initLogBuffer( void * aTrans )
        {
            smxTrans::initTransLogBuffer( aTrans );
        };

        static IDE_RC writeLogToBuffer( void         * aTrans,
                                        const void   * aLog,
                                        UInt           aLogSize )
        {
            return smxTrans::writeLogBufferWithOutOffset( aTrans,
                                                          aLog,
                                                          aLogSize );
        };

        static inline IDE_RC writeLogToBufferAtOffset( void       * aTrans,
                                                       const void * aLog,
                                                       UInt         aOffset,
                                                       UInt         aLogSize )
        {
            return smxTrans::writeLogToBufferOfTx( aTrans,
                                                   aLog,
                                                   aOffset,
                                                   aLogSize );
        };

        static SChar * getLogBufferOfTrans( void * aTrans )
        {
            return smxTrans::getTransLogBuffer( aTrans );
        };

        static idBool isTxBeginStatus( void * aTrans )
        {
            return smxTrans::isTxBeginStatus( aTrans );
        };

        static UInt getLogTypeFlagOfTrans( void * aTrans )
        {
            return smxTrans::getLogTypeFlagOfTrans( aTrans );
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

        static void allocRSGroupID( void * aTrans,
                                    UInt * aPageListIdx )
        {
            /* Transaction이 사용할 RS ID 를 가져온다.*/
            smxTrans::allocRSGroupID( aTrans,
                                      aPageListIdx );
        };

        static UInt getRSGroupID( void * aTrans )
        {
            return smxTrans::getRSGroupID( aTrans );
        };

        static void setRSGroupID( void * aTrans, UInt aIdx )
        {
            smxTrans::setRSGroupID( aTrans, aIdx );
        };

        static void updateSkipCheckSCN( void * aTrans, idBool aDoSkipCheckSCN )
        {
            smxTrans::updateSkipCheckSCN( aTrans, aDoSkipCheckSCN );
        };

        static idBool isReadOnly( void * aTrans )
        {
            return smxTrans::isReadOnly4Callback( aTrans );
        };

        static IDE_RC getTransCompRes( void * aTrans, smrCompRes ** aCompRes )
        {
            return smxTrans::getCompRes4Callback( aTrans, aCompRes );
        };

        static UInt getLstReplStmtDepth( void * aTrans )
        {
            return smxTrans::getLstReplStmtDepth( aTrans );
        };

        static void setMemoryTBSAccessed( void * aTrans )
        {
            smxTrans::setMemoryTBSAccessed4Callback( aTrans );
        };

        static idBool existActiveTrans()
        {
            return smxTransMgr::existActiveTrans();
        };

        static IDE_RC setExistDPathIns( void    * aTrans,
                                        smOID     aTableOID,
                                        idBool    aExistDPathIns )
        {
            return smxTrans::setExistDPathIns( aTrans,
                                               aTableOID,
                                               aExistDPathIns );
        };

        /* smn */
        static IDE_RC verifyIndexIntegrityAtStartUp( void )
        {
            return smnManager::verifyIndexIntegrityAtStartUp();
        };

        static smOID getTableOIDOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getTableOIDOfIndexHeader( aIndexHeader );
        };

        static idBool getIsConsistentOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getIsConsistentOfIndexHeader( aIndexHeader );
        };

        static UInt getIndexIDOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getIndexIDOfIndexHeader( aIndexHeader );
        };

        static IDE_RC dropIndexRuntimeByAbort( smOID        aTableOID,
                                               smOID        aIndexOID )
        {
            return smnManager::dropIndexRuntimeByAbort( aTableOID,
                                                        aIndexOID );
        };

        /* sdn */
        static IDE_RC getIndexInfoToVerify( SChar     * aLogPtr,
                                            smOID     * aTableOID,
                                            smOID     * aIndexOID,
                                            scSpaceID * aSpaceID )
        {
            return sdnUpdate::getIndexInfoToVerify( aLogPtr,
                                                    aTableOID,
                                                    aIndexOID,
                                                    aSpaceID );
        };

        /* smc */
        static IDE_RC copyAllTableBackup( SChar      * aSrcDir,
                                          SChar      * aTargetDir)
        {
            return smcTable::copyAllTableBackup( aSrcDir,
                                                 aTargetDir );
        };

        static void getTableHeaderInfo( void         * aTable,
                                        scPageID     * aPid,
                                        scOffset     * aOffset,
                                        smVCDesc     * aTableColumns,
                                        smVCDesc     * aTableInfo,
                                        UInt         * aTableColumnSize,
                                        UInt         * aTableFlag,
                                        ULong        * aTableMaxRow,
                                        UInt         * aTableParallelDegree )
        {
            smcTable::getTableHeaderInfo( aTable,
                                          aPid,
                                          aOffset,
                                          aTableColumns,
                                          aTableInfo,
                                          aTableColumnSize,
                                          aTableFlag,
                                          aTableMaxRow,
                                          aTableParallelDegree );
        };

        static void getTableHeaderFlag( void         * aTable,
                                        scPageID     * aPid,
                                        scOffset     * aOffset,
                                        UInt         * aTableFlag )
        {
            smcTable::getTableHeaderFlag( aTable,
                                          aPid,
                                          aOffset,
                                          aTableFlag);
        };

        static void getTableIsConsistent( void         * aTable,
                                          scPageID     * aPid,
                                          scOffset     * aOffset,
                                          idBool       * aFlag )
        {
            smcTable::getTableIsConsistent( aTable,
                                            aPid,
                                            aOffset,
                                            aFlag );
        };

        static smOID getTableOID( const void * aTable )
        {
            return smcTable::getTableOID( aTable );
        };

        static idBool isTableConsistent( void * aTableHeader )
        {
            return smcTable::isTableConsistent( aTableHeader );
        };

        static IDE_RC setTableHeaderInconsistency( smOID aTableOID )
        {
            return smcTable::setTableHeaderInconsistency( aTableOID );
        };

        static IDE_RC setIndexInconsistency( smOID aTableOID,
                                             smOID aIndexID )
        {
            return smcTable::setIndexInconsistency( aTableOID,
                                                    aIndexID );
        };

        static IDE_RC setDropTable( void  * aTrans,
                                    SChar * aRow )
        {
            return smcRecord::setDropTable( aTrans,
                                            aRow );
        };

        static IDE_RC dropIndexByAbort( idvSQL  * aIdvSQL,
                                        void    * aTrans,
                                        smOID     aOldIndexOID,
                                        smOID     aNewIndexOID )
        {
            return smcTable::dropIndexByAbortOID( aIdvSQL,
                                                  aTrans,
                                                  aOldIndexOID,
                                                  aNewIndexOID );
        };

        static IDE_RC clearDropIndexList( smOID aTblHeaderOID )
        {
            return smcTable::clearIndexList( aTblHeaderOID );
        };

        static void changeTableSCNForRebuild( smOID aTableOID )
        {
            smcTable::changeTableSCNForRebuild( aTableOID );
        };

        static IDE_RC doNTADropDiskTable( idvSQL    * aIdvSQL,
                                          void      * aTrans,
                                          SChar     * aRow )
        {
            return smcTable::doNTADropDiskTable( aIdvSQL,
                                                 aTrans,
                                                 aRow );
        };

        static IDE_RC setDeleteBit( void     * aTrans,
                                    scSpaceID  aSpaceID,
                                    void     * aRow,
                                    SInt       aOpt )
        {
            return smcRecord::setDeleteBit( aTrans,
                                            aSpaceID,
                                            aRow,
                                            aOpt );
        };

        static IDE_RC setFreeFlagAtVCPieceHdr( void      * aTrans,
                                               smOID       aTableOid,
                                               scSpaceID   aSpaceID,
                                               smOID       aVCPieceOID,
                                               void      * aVCPiece,
                                               UShort      aVCPieceFreeFlag )
        {
            return smcRecord::setFreeFlagAtVCPieceHdr( aTrans,
                                                       aTableOid,
                                                       aSpaceID,
                                                       aVCPieceOID,
                                                       aVCPiece,
                                                       aVCPieceFreeFlag );
        };


        static IDE_RC refineDRDBTables()
        {
            return smcCatalogTable::refineDRDBTables();
        };

        static SChar * getBackupDir()
        {
            return smcTable::getBackupDir();
        };

        static IDE_RC setDeleteBitOnHeader( scSpaceID       aSpaceID,
                                            void          * aRowHeader,
                                            idBool          aIsSetDeleteBit )
        {
            return smcRecord::setDeleteBitOnHeaderInternal( aSpaceID,
                                                            aRowHeader,
                                                            aIsSetDeleteBit );
        };

        /* TASK-5030 */
        static IDE_RC getTableHeaderFromOID( smOID      aTableOID, 
                                             void    ** aTableHeader )
        {
            return smcTable::getTableHeaderFromOID( aTableOID, 
                                                    aTableHeader );
        };

        /* smc redo-undo function */

        /* table  redo-undo function */

        /* PRJ-1496 commit log에 disk table row count 저장 */
        static IDE_RC redoAllTableInfoToDB( SChar     * aAfterImage,
                                            SInt        aSize,
                                            idBool      aForMediaRecovery )
        {
            return smcTableUpdate::redo_SMR_LT_TRANS_COMMIT( aAfterImage,
                                                             aSize,
                                                             aForMediaRecovery );
        };


        static IDE_RC redo_SMC_TABLEHEADER_INIT( smTID      aTid,
                                                 scSpaceID  aSpaceid,
                                                 scPageID   aPid,
                                                 scOffset   aOffset,
                                                 vULong     aData,
                                                 SChar    * aAfterImage,
                                                 SInt       aNSize,
                                                 UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_TABLEHEADER_INIT( aTid,
                                                              aSpaceid,
                                                              aPid,
                                                              aOffset,
                                                              aData,
                                                              aAfterImage,
                                                              aNSize,
                                                              aFlag );
        };

        static IDE_RC redo_SMC_TABLEHEADER_UPDATE_INDEX( smTID      aTid,
                                                         scSpaceID  aSpaceid,
                                                         scPageID   aPid,
                                                         scOffset   aOffset,
                                                         vULong     aData,
                                                         SChar    * aImage,
                                                         SInt       aNSize,
                                                         UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_TABLEHEADER_UPDATE_INDEX( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC undo_SMC_TABLEHEADER_UPDATE_INDEX( smTID      aTid,
                                                         scSpaceID  aSpaceid,
                                                         scPageID   aPid,
                                                         scOffset   aOffset,
                                                         vULong     aData,
                                                         SChar    * aImage,
                                                         SInt       aNSize,
                                                         UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_TABLEHEADER_UPDATE_INDEX( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_COLUMNS( smTID      aTid,
                                                                scSpaceID  aSpaceid,
                                                                scPageID   aPid,
                                                                scOffset   aOffset,
                                                                vULong     aData,
                                                                SChar    * aImage,
                                                                SInt       aNSize,
                                                                UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMNS( aTid,
                                                                             aSpaceid,
                                                                             aPid,
                                                                             aOffset,
                                                                             aData,
                                                                             aImage,
                                                                             aNSize,
                                                                             aFlag );
        };

        static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_INFO( smTID      aTid,
                                                             scSpaceID  aSpaceid,
                                                             scPageID   aPid,
                                                             scOffset   aOffset,
                                                             vULong     aData,
                                                             SChar    * aImage,
                                                             SInt       aNSize,
                                                             UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_INFO( aTid,
                                                                          aSpaceid,
                                                                          aPid,
                                                                          aOffset,
                                                                          aData,
                                                                          aImage,
                                                                          aNSize,
                                                                          aFlag );
        };

        static IDE_RC redo_SMC_TABLEHEADER_SET_NULLROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_TABLEHEADER_SET_NULLROW( aTid,
                                                                     aSpaceid,
                                                                     aPid,
                                                                     aOffset,
                                                                     aData,
                                                                     aImage,
                                                                     aNSize,
                                                                     aFlag );
        };

        static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_ALL( smTID      aTid,
                                                            scSpaceID  aSpaceid,
                                                            scPageID   aPid,
                                                            scOffset   aOffset,
                                                            vULong     aData,
                                                            SChar    * aImage,
                                                            SInt       aNSize,
                                                            UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_ALL( aTid,
                                                                         aSpaceid,
                                                                         aPid,
                                                                         aOffset,
                                                                         aData,
                                                                         aImage,
                                                                         aNSize,
                                                                         aFlag );
        };

        static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_ALLOCINFO( smTID      aTid,
                                                                  scSpaceID  aSpaceid,
                                                                  scPageID   aPid,
                                                                  scOffset   aOffset,
                                                                  vULong     aData,
                                                                  SChar    * aImage,
                                                                  SInt       aNSize,
                                                                  UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_ALLOCINFO( aTid,
                                                                               aSpaceid,
                                                                               aPid,
                                                                               aOffset,
                                                                               aData,
                                                                               aImage,
                                                                               aNSize,
                                                                               aFlag );
        };

        static IDE_RC redo_SMC_TABLEHEADER_UPDATE_FLAG( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_TABLEHEADER_UPDATE_FLAG( aTid,
                                                                     aSpaceid,
                                                                     aPid,
                                                                     aOffset,
                                                                     aData,
                                                                     aImage,
                                                                     aNSize,
                                                                     aFlag );
        };

        static IDE_RC undo_SMC_TABLEHEADER_UPDATE_FLAG( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_TABLEHEADER_UPDATE_FLAG( aTid,
                                                                     aSpaceid,
                                                                     aPid,
                                                                     aOffset,
                                                                     aData,
                                                                     aImage,
                                                                     aNSize,
                                                                     aFlag );
        };

        static IDE_RC redo_undo_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT( smTID      aTid,
                                                                     scSpaceID  aSpaceid,
                                                                     scPageID   aPid,
                                                                     scOffset   aOffset,
                                                                     vULong     aData,
                                                                     SChar    * aImage,
                                                                     SInt       aNSize,
                                                                     UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_UPDATE_COLUMN_COUNT( aTid,
                                                                                  aSpaceid,
                                                                                  aPid,
                                                                                  aOffset,
                                                                                  aData,
                                                                                  aImage,
                                                                                  aNSize,
                                                                                  aFlag );
        };

        static IDE_RC redo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT( smTID      aTid,
                                                                 scSpaceID  aSpaceid,
                                                                 scPageID   aPid,
                                                                 scOffset   aOffset,
                                                                 vULong     aData,
                                                                 SChar    * aImage,
                                                                 SInt       aNSize,
                                                                 UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT( aTid,
                                                                              aSpaceid,
                                                                              aPid,
                                                                              aOffset,
                                                                              aData,
                                                                              aImage,
                                                                              aNSize,
                                                                              aFlag );
        };

        static IDE_RC undo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT( smTID      aTid,
                                                                 scSpaceID  aSpaceid,
                                                                 scPageID   aPid,
                                                                 scOffset   aOffset,
                                                                 vULong     aData,
                                                                 SChar    * aImage,
                                                                 SInt       aNSize,
                                                                 UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_TABLEHEADER_UPDATE_TABLE_SEGMENT( aTid,
                                                                              aSpaceid,
                                                                              aPid,
                                                                              aOffset,
                                                                              aData,
                                                                              aImage,
                                                                              aNSize,
                                                                              aFlag );
        };

        static IDE_RC redo_SMC_TABLEHEADER_SET_INCONSISTENCY( smTID      aTid,
                                                              scSpaceID  aSpaceid,
                                                              scPageID   aPid,
                                                              scOffset   aOffset,
                                                              vULong     aData,
                                                              SChar    * aImage,
                                                              SInt       aNSize,
                                                              UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_TABLEHEADER_SET_INCONSISTENT( aTid,
                                                                          aSpaceid,
                                                                          aPid,
                                                                          aOffset,
                                                                          aData,
                                                                          aImage,
                                                                          aNSize,
                                                                          aFlag );
        };

        static IDE_RC undo_SMC_TABLEHEADER_SET_INCONSISTENCY( smTID      aTid,
                                                              scSpaceID  aSpaceid,
                                                              scPageID   aPid,
                                                              scOffset   aOffset,
                                                              vULong     aData,
                                                              SChar    * aImage,
                                                              SInt       aNSize,
                                                              UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_TABLEHEADER_SET_INCONSISTENT( aTid,
                                                                          aSpaceid,
                                                                          aPid,
                                                                          aOffset,
                                                                          aData,
                                                                          aImage,
                                                                          aNSize,
                                                                          aFlag );
        };

        static IDE_RC redo_SMC_INDEX_SET_FLAG( smTID      aTid,
                                               scSpaceID  aSpaceid,
                                               scPageID   aPid,
                                               scOffset   aOffset,
                                               vULong     aData,
                                               SChar    * aImage,
                                               SInt       aNSize,
                                               UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_INDEX_SET_FLAG( aTid,
                                                            aSpaceid,
                                                            aPid,
                                                            aOffset,
                                                            aData,
                                                            aImage,
                                                            aNSize,
                                                            aFlag );
        };

        static IDE_RC undo_SMC_INDEX_SET_FLAG( smTID      aTid,
                                               scSpaceID  aSpaceid,
                                               scPageID   aPid,
                                               scOffset   aOffset,
                                               vULong     aData,
                                               SChar    * aImage,
                                               SInt       aNSize,
                                               UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_INDEX_SET_FLAG( aTid,
                                                            aSpaceid,
                                                            aPid,
                                                            aOffset,
                                                            aData,
                                                            aImage,
                                                            aNSize,
                                                            aFlag );
        };

        static IDE_RC redo_SMC_INDEX_SET_SEGSTOATTR( smTID      aTid,
                                                     scSpaceID  aSpaceid,
                                                     scPageID   aPid,
                                                     scOffset   aOffset,
                                                     vULong     aData,
                                                     SChar    * aImage,
                                                     SInt       aNSize,
                                                     UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_INDEX_SET_SEGSTOATTR( aTid,
                                                                  aSpaceid,
                                                                  aPid,
                                                                  aOffset,
                                                                  aData,
                                                                  aImage,
                                                                  aNSize,
                                                                  aFlag );
        };

        static IDE_RC undo_SMC_INDEX_SET_SEGSTOATTR( smTID      aTid,
                                                     scSpaceID  aSpaceid,
                                                     scPageID   aPid,
                                                     scOffset   aOffset,
                                                     vULong     aData,
                                                     SChar    * aImage,
                                                     SInt       aNSize,
                                                     UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_INDEX_SET_SEGSTOATTR( aTid,
                                                                  aSpaceid,
                                                                  aPid,
                                                                  aOffset,
                                                                  aData,
                                                                  aImage,
                                                                  aNSize,
                                                                  aFlag );
        };

        static IDE_RC redo_SMC_SET_INDEX_SEGATTR( smTID      aTid,
                                                  scSpaceID  aSpaceid,
                                                  scPageID   aPid,
                                                  scOffset   aOffset,
                                                  vULong     aData,
                                                  SChar    * aImage,
                                                  SInt       aNSize,
                                                  UInt       aFlag )
        {
            return smcTableUpdate::redo_SMC_SET_INDEX_SEGATTR( aTid,
                                                               aSpaceid,
                                                               aPid,
                                                               aOffset,
                                                               aData,
                                                               aImage,
                                                               aNSize,
                                                               aFlag );
        };

        static IDE_RC undo_SMC_SET_INDEX_SEGATTR( smTID      aTid,
                                                  scSpaceID  aSpaceid,
                                                  scPageID   aPid,
                                                  scOffset   aOffset,
                                                  vULong     aData,
                                                  SChar    * aImage,
                                                  SInt       aNSize,
                                                  UInt       aFlag )
        {
            return smcTableUpdate::undo_SMC_SET_INDEX_SEGATTR( aTid,
                                                               aSpaceid,
                                                               aPid,
                                                               aOffset,
                                                               aData,
                                                               aImage,
                                                               aNSize,
                                                               aFlag );
        };

        /* sequence redo-undo function */
        static IDE_RC redo_undo_SMC_TABLEHEADER_SET_SEQUENCE( smTID      aTid,
                                                              scSpaceID  aSpaceid,
                                                              scPageID   aPid,
                                                              scOffset   aOffset,
                                                              vULong     aData,
                                                              SChar    * aImage,
                                                              SInt       aNSize,
                                                              UInt       aFlag )
        {
            return smcSequenceUpdate::redo_undo_SMC_TABLEHEADER_SET_SEQUENCE( aTid,
                                                                              aSpaceid,
                                                                              aPid,
                                                                              aOffset,
                                                                              aData,
                                                                              aImage,
                                                                              aNSize,
                                                                              aFlag );
        };

        /* redo-undo function */
        static IDE_RC redo_undo_SMC_TABLEHEADER_SET_INSERTLIMIT( smTID      aTid,
                                                                 scSpaceID  aSpaceid,
                                                                 scPageID   aPid,
                                                                 scOffset   aOffset,
                                                                 vULong     aData,
                                                                 SChar    * aImage,
                                                                 SInt       aNSize,
                                                                 UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_SET_INSERTLIMIT( aTid,
                                                                              aSpaceid,
                                                                              aPid,
                                                                              aOffset,
                                                                              aData,
                                                                              aImage,
                                                                              aNSize,
                                                                              aFlag );
        };

        static IDE_RC redo_undo_SMC_TABLEHEADER_SET_SEGSTOATTR( smTID      aTid,
                                                                scSpaceID  aSpaceid,
                                                                scPageID   aPid,
                                                                scOffset   aOffset,
                                                                vULong     aData,
                                                                SChar    * aImage,
                                                                SInt       aNSize,
                                                                UInt       aFlag )
        {
            return smcTableUpdate::redo_undo_SMC_TABLEHEADER_SET_SEGSTOATTR( aTid,
                                                                             aSpaceid,
                                                                             aPid,
                                                                             aOffset,
                                                                             aData,
                                                                             aImage,
                                                                             aNSize,
                                                                             aFlag );
        };

        /* record  redo-undo function */
        static IDE_RC redo_SMC_PERS_INIT_FIXED_ROW( smTID      aTid,
                                                    scSpaceID  aSpaceid,
                                                    scPageID   aPid,
                                                    scOffset   aOffset,
                                                    vULong     aData,
                                                    SChar    * aImage,
                                                    SInt       aNSize,
                                                    UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_INIT_FIXED_ROW( aTid,
                                                                  aSpaceid,
                                                                  aPid,
                                                                  aOffset,
                                                                  aData,
                                                                  aImage,
                                                                  aNSize,
                                                                  aFlag );
        };

        static IDE_RC undo_SMC_PERS_INIT_FIXED_ROW( smTID      aTid,
                                                    scSpaceID  aSpaceid,
                                                    scPageID   aPid,
                                                    scOffset   aOffset,
                                                    vULong     aData,
                                                    SChar    * aImage,
                                                    SInt       aNSize,
                                                    UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_INIT_FIXED_ROW( aTid,
                                                                  aSpaceid,
                                                                  aPid,
                                                                  aOffset,
                                                                  aData,
                                                                  aImage,
                                                                  aNSize,
                                                                  aFlag );
        };

        static IDE_RC redo_undo_SMC_PERS_UPDATE_FIXED_ROW( smTID      aTid,
                                                           scSpaceID  aSpaceid,
                                                           scPageID   aPid,
                                                           scOffset   aOffset,
                                                           vULong     aData,
                                                           SChar    * aImage,
                                                           SInt       aNSize,
                                                           UInt       aFlag )
        {
            return smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_FIXED_ROW( aTid,
                                                                         aSpaceid,
                                                                         aPid,
                                                                         aOffset,
                                                                         aData,
                                                                         aImage,
                                                                         aNSize,
                                                                         aFlag );
        };

        static IDE_RC redo_undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_FREE( smTID      aTid,
                                                                     scSpaceID  aSpaceid,
                                                                     scPageID   aPid,
                                                                     scOffset   aOffset,
                                                                     vULong     aData,
                                                                     SChar    * aImage,
                                                                     SInt       aNSize,
                                                                     UInt       aFlag )
        {
            return smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_FREE( aTid,
                                                                                   aSpaceid,
                                                                                   aPid,
                                                                                   aOffset,
                                                                                   aData,
                                                                                   aImage,
                                                                                   aNSize,
                                                                                   aFlag );
        };

        static IDE_RC redo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION( smTID      aTid,
                                                                   scSpaceID  aSpaceid,
                                                                   scPageID   aPid,
                                                                   scOffset   aOffset,
                                                                   vULong     aData,
                                                                   SChar    * aImage,
                                                                   SInt       aNSize,
                                                                   UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION( aTid,
                                                                                 aSpaceid,
                                                                                 aPid,
                                                                                 aOffset,
                                                                                 aData,
                                                                                 aImage,
                                                                                 aNSize,
                                                                                 aFlag );
        };

        static IDE_RC undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION( smTID      aTid,
                                                                   scSpaceID  aSpaceid,
                                                                   scPageID   aPid,
                                                                   scOffset   aOffset,
                                                                   vULong     aData,
                                                                   SChar    * aImage,
                                                                   SInt       aNSize,
                                                                   UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_UPDATE_FIXED_ROW_NEXT_VERSION( aTid,
                                                                                 aSpaceid,
                                                                                 aPid,
                                                                                 aOffset,
                                                                                 aData,
                                                                                 aImage,
                                                                                 aNSize,
                                                                                 aFlag );
        };

        static IDE_RC redo_SMC_PERS_SET_FIX_ROW_DROP_FLAG( smTID      aTid,
                                                           scSpaceID  aSpaceid,
                                                           scPageID   aPid,
                                                           scOffset   aOffset,
                                                           vULong     aData,
                                                           SChar    * aImage,
                                                           SInt       aNSize,
                                                           UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_SET_FIX_ROW_DROP_FLAG( aTid,
                                                                         aSpaceid,
                                                                         aPid,
                                                                         aOffset,
                                                                         aData,
                                                                         aImage,
                                                                         aNSize,
                                                                         aFlag );
        };

        static IDE_RC undo_SMC_PERS_SET_FIX_ROW_DROP_FLAG( smTID      aTid,
                                                           scSpaceID  aSpaceid,
                                                           scPageID   aPid,
                                                           scOffset   aOffset,
                                                           vULong     aData,
                                                           SChar    * aImage,
                                                           SInt       aNSize,
                                                           UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_SET_FIX_ROW_DROP_FLAG( aTid,
                                                                         aSpaceid,
                                                                         aPid,
                                                                         aOffset,
                                                                         aData,
                                                                         aImage,
                                                                         aNSize,
                                                                         aFlag );
        };

        static IDE_RC redo_SMC_PERS_SET_FIX_ROW_DELETE_BIT( smTID      aTid,
                                                            scSpaceID  aSpaceid,
                                                            scPageID   aPid,
                                                            scOffset   aOffset,
                                                            vULong     aData,
                                                            SChar    * aImage,
                                                            SInt       aNSize,
                                                            UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_SET_FIX_ROW_DELETE_BIT( aTid,
                                                                          aSpaceid,
                                                                          aPid,
                                                                          aOffset,
                                                                          aData,
                                                                          aImage,
                                                                          aNSize,
                                                                          aFlag );
        };

        static IDE_RC undo_SMC_PERS_SET_FIX_ROW_DELETE_BIT( smTID      aTid,
                                                            scSpaceID  aSpaceid,
                                                            scPageID   aPid,
                                                            scOffset   aOffset,
                                                            vULong     aData,
                                                            SChar    * aImage,
                                                            SInt       aNSize,
                                                            UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_SET_FIX_ROW_DELETE_BIT( aTid,
                                                                          aSpaceid,
                                                                          aPid,
                                                                          aOffset,
                                                                          aData,
                                                                          aImage,
                                                                          aNSize,
                                                                          aFlag );
        };

        static IDE_RC redo_undo_SMC_PERS_UPDATE_VAR_ROW_HEAD( smTID      aTid,
                                                              scSpaceID  aSpaceid,
                                                              scPageID   aPid,
                                                              scOffset   aOffset,
                                                              vULong     aData,
                                                              SChar    * aImage,
                                                              SInt       aNSize,
                                                              UInt       aFlag )
        {
            return smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_VAR_ROW_HEAD( aTid,
                                                                            aSpaceid,
                                                                            aPid,
                                                                            aOffset,
                                                                            aData,
                                                                            aImage,
                                                                            aNSize,
                                                                            aFlag );
        };

        static IDE_RC redo_undo_SMC_PERS_UPDATE_VAR_ROW( smTID      aTid,
                                                         scSpaceID  aSpaceid,
                                                         scPageID   aPid,
                                                         scOffset   aOffset,
                                                         vULong     aData,
                                                         SChar    * aImage,
                                                         SInt       aNSize,
                                                         UInt       aFlag )
        {
            return smcRecordUpdate::redo_undo_SMC_PERS_UPDATE_VAR_ROW( aTid,
                                                                       aSpaceid,
                                                                       aPid,
                                                                       aOffset,
                                                                       aData,
                                                                       aImage,
                                                                       aNSize,
                                                                       aFlag );
        };

        static IDE_RC redo_SMC_PERS_SET_VAR_ROW_FLAG( smTID      aTid,
                                                      scSpaceID  aSpaceid,
                                                      scPageID   aPid,
                                                      scOffset   aOffset,
                                                      vULong     aData,
                                                      SChar    * aImage,
                                                      SInt       aNSize,
                                                      UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_SET_VAR_ROW_FLAG( aTid,
                                                                    aSpaceid,
                                                                    aPid,
                                                                    aOffset,
                                                                    aData,
                                                                    aImage,
                                                                    aNSize,
                                                                    aFlag );
        };

        static IDE_RC undo_SMC_PERS_SET_VAR_ROW_FLAG( smTID      aTid,
                                                      scSpaceID  aSpaceid,
                                                      scPageID   aPid,
                                                      scOffset   aOffset,
                                                      vULong     aData,
                                                      SChar    * aImage,
                                                      SInt       aNSize,
                                                      UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_SET_VAR_ROW_FLAG( aTid,
                                                                    aSpaceid,
                                                                    aPid,
                                                                    aOffset,
                                                                    aData,
                                                                    aImage,
                                                                    aNSize,
                                                                    aFlag );
        };

        static IDE_RC redo_undo_SMC_INDEX_SET_DROP_FLAG( smTID      aTid,
                                                         scSpaceID  aSpaceid,
                                                         scPageID   aPid,
                                                         scOffset   aOffset,
                                                         vULong     aData,
                                                         SChar    * aImage,
                                                         SInt       aNSize,
                                                         UInt       aFlag )
        {
            return smcRecordUpdate::redo_undo_SMC_INDEX_SET_DROP_FLAG( aTid,
                                                                       aSpaceid,
                                                                       aPid,
                                                                       aOffset,
                                                                       aData,
                                                                       aImage,
                                                                       aNSize,
                                                                       aFlag );
        };

        static IDE_RC redo_SMC_PERS_INSERT_ROW( smTID      aTid,
                                                scSpaceID  aSpaceid,
                                                scPageID   aPid,
                                                scOffset   aOffset,
                                                vULong     aData,
                                                SChar    * aImage,
                                                SInt       aNSize,
                                                UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_INSERT_ROW( aTid,
                                                              aSpaceid,
                                                              aPid,
                                                              aOffset,
                                                              aData,
                                                              aImage,
                                                              aNSize,
                                                              aFlag );
        };

        static IDE_RC undo_SMC_PERS_INSERT_ROW( smTID      aTid,
                                                scSpaceID  aSpaceid,
                                                scPageID   aPid,
                                                scOffset   aOffset,
                                                vULong     aData,
                                                SChar    * aImage,
                                                SInt       aNSize,
                                                UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_INSERT_ROW( aTid,
                                                              aSpaceid,
                                                              aPid,
                                                              aOffset,
                                                              aData,
                                                              aImage,
                                                              aNSize,
                                                              aFlag );
        };

        static IDE_RC redo_SMC_PERS_UPDATE_INPLACE_ROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_UPDATE_INPLACE_ROW( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC undo_SMC_PERS_UPDATE_INPLACE_ROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_UPDATE_INPLACE_ROW( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC redo_SMC_PERS_UPDATE_VERSION_ROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_UPDATE_VERSION_ROW( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC undo_SMC_PERS_UPDATE_VERSION_ROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_UPDATE_VERSION_ROW( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC redo_SMC_PERS_DELETE_VERSION_ROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_PERS_DELETE_VERSION_ROW( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );
        };

        static IDE_RC undo_SMC_PERS_DELETE_VERSION_ROW( smTID      aTid,
                                                        scSpaceID  aSpaceid,
                                                        scPageID   aPid,
                                                        scOffset   aOffset,
                                                        vULong     aData,
                                                        SChar    * aImage,
                                                        SInt       aNSize,
                                                        UInt       aFlag )
        {
            return smcRecordUpdate::undo_SMC_PERS_DELETE_VERSION_ROW( aTid,
                                                                      aSpaceid,
                                                                      aPid,
                                                                      aOffset,
                                                                      aData,
                                                                      aImage,
                                                                      aNSize,
                                                                      aFlag );

        };

        static IDE_RC redo_undo_SMC_PERS_SET_VAR_ROW_NXT_OID( smTID      aTid,
                                                              scSpaceID  aSpaceid,
                                                              scPageID   aPid,
                                                              scOffset   aOffset,
                                                              vULong     aData,
                                                              SChar    * aImage,
                                                              SInt       aNSize,
                                                              UInt       aFlag )
        {
            return smcRecordUpdate::redo_undo_SMC_PERS_SET_VAR_ROW_NXT_OID( aTid,
                                                                            aSpaceid,
                                                                            aPid,
                                                                            aOffset,
                                                                            aData,
                                                                            aImage,
                                                                            aNSize,
                                                                            aFlag );
        };

        static IDE_RC redo_SMC_SET_CREATE_SCN( smTID      aTid,
                                               scSpaceID  aSpaceid,
                                               scPageID   aPid,
                                               scOffset   aOffset,
                                               vULong     aData,
                                               SChar    * aImage,
                                               SInt       aNSize,
                                               UInt       aFlag )
        {
            return smcRecordUpdate::redo_SMC_SET_CREATE_SCN( aTid,
                                                             aSpaceid,
                                                             aPid,
                                                             aOffset,
                                                             aData,
                                                             aImage,
                                                             aNSize,
                                                             aFlag );
        };

        static IDE_RC redo_SMC_PERS_WRITE_LOB_PIECE( smTID      aTid,
                                                     scSpaceID  aSpaceid,
                                                     scPageID   aPid,
                                                     scOffset   aOffset,
                                                     vULong     aData,
                                                     SChar    * aImage,
                                                     SInt       aNSize,
                                                     UInt       aFlag )
        {
            return smcLobUpdate::redo_SMC_PERS_WRITE_LOB_PIECE( aTid,
                                                                aSpaceid,
                                                                aPid,
                                                                aOffset,
                                                                aData,
                                                                aImage,
                                                                aNSize,
                                                                aFlag );
        };

        static IDE_RC redo_undo_SMC_PERS_SET_INCONSISTENCY( smTID      aTid,
                                                            scSpaceID  aSpaceid,
                                                            scPageID   aPid,
                                                            scOffset   aOffset,
                                                            vULong     aData,
                                                            SChar    * aImage,
                                                            SInt       aNSize,
                                                            UInt       aFlag )
        {
            return smpUpdate::redo_undo_SMC_PERS_SET_INCONSISTENCY( aTid,
                                                                    aSpaceid,
                                                                    aPid,
                                                                    aOffset,
                                                                    aData,
                                                                    aImage,
                                                                    aNSize,
                                                                    aFlag );
        };

        // smp api function
        static scPageID getPersPageID(void * aPage)
        {
            return smpManager::getPersPageID( aPage );
        };

        static void getAllocPageListInfo( void      * aAllocPageList,
                                          vULong    * aPageCount,
                                          scPageID  * aHeadPID,
                                          scPageID  * aTailPID )
        {
            smpManager::getAllocPageListInfo( aAllocPageList,
                                              aPageCount,
                                              aHeadPID,
                                              aTailPID );
        };

        static IDE_RC setPersPageInconsistency4MRDB( scSpaceID   aSpaceID,
                                                     scPageID    aPageID )
        {
            return smpManager::setPersPageInconsistency( aSpaceID,
                                                         aPageID );
        };

        static smOID getTableOID4MRDBPage( void * aPage )
        {
            return smpManager::getTableOID( aPage );
        };

        /* sdp */
        static void getDiskPageEntryInfo( void      * aPageListEntry,
                                          scPageID  * aSegPID )
        {
            sdpPageList::getPageListEntryInfo( aPageListEntry,
                                               aSegPID );
        };

        static IDE_RC setPageInconsistency4DRDB( scSpaceID       aSpaceID,
                                                 scPageID        aPID )
        {
            return sdpPhyPage::setPageInconsistency( aSpaceID,
                                                     aPID );
        };

        /* PROJ-1665 : Page의 consistent 상태 여부 반환 */
        static idBool isConsistentPage4DRDB( UChar * aPageHdr )
        {
            return sdpPhyPage::isConsistentPage( aPageHdr );
        };

        static smOID getTableOID4DRDBPage( UChar * aPageHdr )
        {
            return sdpPhyPage::getTableOID( aPageHdr );
        };

        /* smp redo-undo function */
        static IDE_RC redo_undo_SMM_PERS_UPDATE_LINK( smTID      aTid,
                                                      scSpaceID  aSpaceid,
                                                      scPageID   aPid,
                                                      scOffset   aOffset,
                                                      vULong     aData,
                                                      SChar    * aImage,
                                                      SInt       aNSize,
                                                      UInt       aFlag )
        {
            return smpUpdate::redo_undo_SMM_PERS_UPDATE_LINK( aTid,
                                                              aSpaceid,
                                                              aPid,
                                                              aOffset,
                                                              aData,
                                                              aImage,
                                                              aNSize,
                                                              aFlag );
        };

        static IDE_RC redo_SMC_PERS_INIT_FIXED_PAGE( smTID      aTid,
                                                     scSpaceID  aSpaceid,
                                                     scPageID   aPid,
                                                     scOffset   aOffset,
                                                     vULong     aData,
                                                     SChar    * aImage,
                                                     SInt       aNSize,
                                                     UInt       aFlag )
        {
            return smpUpdate::redo_SMC_PERS_INIT_FIXED_PAGE( aTid,
                                                             aSpaceid,
                                                             aPid,
                                                             aOffset,
                                                             aData,
                                                             aImage,
                                                             aNSize,
                                                             aFlag );
        };

        static IDE_RC redo_SMC_PERS_INIT_VAR_PAGE( smTID      aTid,
                                                   scSpaceID  aSpaceid,
                                                   scPageID   aPid,
                                                   scOffset   aOffset,
                                                   vULong     aData,
                                                   SChar    * aImage,
                                                   SInt       aNSize,
                                                   UInt       aFlag )
        {
            return smpUpdate::redo_SMC_PERS_INIT_VAR_PAGE( aTid,
                                                           aSpaceid,
                                                           aPid,
                                                           aOffset,
                                                           aData,
                                                           aImage,
                                                           aNSize,
                                                           aFlag );
        };

        static IDE_RC redo_SMP_NTA_ALLOC_FIXED_ROW( scSpaceID    aSpaceID,
                                                    scPageID     aPageID,
                                                    scOffset     aOffset,
                                                    idBool       aIsSetDeleteBit )
        {
            return smpUpdate::redo_SMP_NTA_ALLOC_FIXED_ROW( aSpaceID,
                                                            aPageID,
                                                            aOffset,
                                                            aIsSetDeleteBit );
        };

        static IDE_RC redo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE( idvSQL     * aStatistics,
                                                             void       * aTrans,
                                                             smLSN        aCurLSN,
                                                             scSpaceID    aSpaceID,
                                                             UInt         aFileID,
                                                             UInt         aValueSize,
                                                             SChar      * aValuePtr,
                                                             idBool       aIsRestart )
        {
            return smpUpdate::redo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE( aStatistics,
                                                                     aTrans,
                                                                     aCurLSN,
                                                                     aSpaceID,
                                                                     aFileID,
                                                                     aValueSize,
                                                                     aValuePtr,
                                                                     aIsRestart );
        };

        static IDE_RC undo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE( idvSQL     * aStatistics,
                                                             void       * aTrans,
                                                             smLSN        aCurLSN,
                                                             scSpaceID    aSpaceID,
                                                             UInt         aFileID,
                                                             UInt         aValueSize,
                                                             SChar      * aValuePtr,
                                                             idBool       aIsRestart )
        {
            return smpUpdate::undo_SCT_UPDATE_MRDB_ALTER_TBS_ONLINE( aStatistics,
                                                                     aTrans,
                                                                     aCurLSN,
                                                                     aSpaceID,
                                                                     aFileID,
                                                                     aValueSize,
                                                                     aValuePtr,
                                                                     aIsRestart );
        };

        static IDE_RC redo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE( idvSQL     * aStatistics,
                                                              void       * aTrans,
                                                              smLSN        aCurLSN,
                                                              scSpaceID    aSpaceID,
                                                              UInt         aFileID,
                                                              UInt         aValueSize,
                                                              SChar      * aValuePtr,
                                                              idBool       aIsRestart )
        {
            return smpUpdate::redo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE( aStatistics,
                                                                      aTrans,
                                                                      aCurLSN,
                                                                      aSpaceID,
                                                                      aFileID,
                                                                      aValueSize,
                                                                      aValuePtr,
                                                                      aIsRestart );
        };

        static IDE_RC undo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE( idvSQL     * aStatistics,
                                                              void       * aTrans,
                                                              smLSN        aCurLSN,
                                                              scSpaceID    aSpaceID,
                                                              UInt         aFileID,
                                                              UInt         aValueSize,
                                                              SChar      * aValuePtr,
                                                              idBool       aIsRestart )
        {
            return smpUpdate::undo_SCT_UPDATE_MRDB_ALTER_TBS_OFFLINE( aStatistics,
                                                                      aTrans,
                                                                      aCurLSN,
                                                                      aSpaceID,
                                                                      aFileID,
                                                                      aValueSize,
                                                                      aValuePtr,
                                                                      aIsRestart );
        };

        static IDE_RC finiOfflineTBS()
        {
            return smpTBSAlterOnOff::finiOfflineTBS();
        };

        /* sdr */
        static void initRecFuncMap()
        {
            sdrUpdate::initialize();
        };

        static IDE_RC initializeRedoMgr( UInt           aHashTableSize,
                                         smiRecoverType aRecvType )
        {
            return sdrRedoMgr::initialize( aHashTableSize,
                                           aRecvType );
        };

        static IDE_RC destroyRedoMgr()
        {
            return sdrRedoMgr::destroy();
        };

        static IDE_RC generateRedoLogDataList( smTID             aTransID,
                                               SChar           * aLogBuffer,
                                               UInt              aLogBufferSize,
                                               smLSN           * aBeginLSN,
                                               smLSN           * aEndLSN,
                                               void           ** aLogDataList )
        {
            return sdrRedoMgr::generateRedoLogDataList( aTransID,
                                                        aLogBuffer,
                                                        aLogBufferSize,
                                                        aBeginLSN,
                                                        aEndLSN,
                                                        aLogDataList );
        };

        static IDE_RC addRedoLogToHashTable( void * aLogDataList )
        {
            return sdrRedoMgr::addRedoLogToHashTable( aLogDataList );
        };

        static IDE_RC applyHashedLogRec( idvSQL * aStatistics )
        {
            return sdrRedoMgr::applyHashedLogRec( aStatistics );
        };

        static IDE_RC doUndoFunction( idvSQL      * aStatistics,
                                      smTID         aTransID,
                                      smOID         aOID,
                                      SChar       * aData,
                                      smLSN       * aPrevLSN )
        {
            return sdrUpdate::doUndoFunction( aStatistics,
                                              aTransID,
                                              aOID,
                                              aData,
                                              aPrevLSN );
        };

        static IDE_RC doNTAUndoFunction( idvSQL     * aStatistics,
                                         void       * aTrans,
                                         UInt         aOPType,
                                         scSpaceID    aSpaceID,
                                         smLSN      * aPrevLSN,
                                         ULong      * aArrData,
                                         UInt         aDataCount )
        {
            return sdrUpdate::doNTAUndoFunction( aStatistics,
                                                 aTrans,
                                                 aOPType,
                                                 aSpaceID,
                                                 aPrevLSN,
                                                 aArrData,
                                                 aDataCount );
        };

        static IDE_RC doRefNTAUndoFunction( idvSQL   * aStatistics,
                                            void     * aTrans,
                                            UInt       aOPType,
                                            smLSN    * aPrevLSN,
                                            SChar    * aRefData )
        {
            return sdrUpdate::doRefNTAUndoFunction( aStatistics,
                                                    aTrans,
                                                    aOPType,
                                                    aPrevLSN,
                                                    aRefData );
        };

        static void redoRuntimeMRDB( void     * aTrans,
                                     SChar    * aLogBuffer )
        {
            sdrRedoMgr::redoRuntimeMRDB( aTrans,
                                         aLogBuffer );
        };

        static smiRecoverType getRecvType()
        {
            return sdrRedoMgr::getRecvType();
        };

        static IDE_RC repairFailureDBFHdr( smLSN * aResetLogsLSN )
        {
            return sdrRedoMgr::repairFailureDBFHdr( aResetLogsLSN );
        };

        static IDE_RC removeAllRecvDBFHashNodes()
        {
            return sdrRedoMgr::removeAllRecvDBFHashNodes();
        };

        static IDE_RC initializeCorruptPageMgr( UInt aHashTableSize )
        {
            return sdrCorruptPageMgr::initialize( aHashTableSize );
        };

        static IDE_RC destroyCorruptPageMgr()
        {
            return sdrCorruptPageMgr::destroy();
        };

        static IDE_RC checkCorruptedPages()
        {
            return sdrCorruptPageMgr::checkCorruptedPages();
        };

        /* sds */
        static IDE_RC repairFailureSBufferHdr( idvSQL * aStatistics, 
                                               smLSN  * aResetLogsLSN )
        {
            return sdsBufferMgr::repairFailureSBufferHdr( aStatistics, 
                                                          aResetLogsLSN );
        };
};

#define smLayerCallback   smrReqFunc

#endif /* _O_SMR_REQ_H_ */

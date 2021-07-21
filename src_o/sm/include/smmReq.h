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
 * $Id: smmReq.h 89495 2020-12-14 05:19:22Z emlee $
 **********************************************************************/

#ifndef _O_SMM_REQ_H_
#define _O_SMM_REQ_H_ 1

#include <idl.h> /* for win32 */
#include <smxAPI.h>
#include <smlAPI.h>
#include <smiAPI.h>
#include <smnAPI.h>
#include <smpAPI.h>
#include <smrAPI.h>
#include <smaAPI.h>

class smmReqFunc
{
    public:

        /* smi */
        static void setEmergency( idBool aFlag )
        {
            smiSetEmergency( aFlag);
        };


        /* smc */
        static IDE_RC getTablePageCount( void * aTable, ULong * aRecCnt )
        {
            return smcTable::getTablePageCount( aTable, aRecCnt );
        };

        static UInt getCatTempTableOffset()
        {
            return smcCatalogTable::getCatTempTableOffset();
        };


        /* smp */
        static UInt getPersPageBodyOffset()
        {
            return smpManager::getPersPageBodyOffset();
        };

        static UInt getPersPageBodySize()
        {
            return smpManager::getPersPageBodySize();
        };

        static UInt getSlotSize( )
        {
            return smpManager::getSlotSize();
        };

        static scPageID getPersPageID( void * aPage )
        {
            return smpManager::getPersPageID( aPage );
        };

        static scPageID getPrevPersPageID( void * aPage )
        {
            return smpManager::getPrevPersPageID( aPage );
        };

        static scPageID getNextPersPageID( void * aPage )
        {
            return smpManager::getNextPersPageID( aPage );
        };

        static void setPrevPersPageID( void * aPage, scPageID aPID )
        {
            smpManager::setPrevPersPageID( aPage, aPID);
        };

        static void setNextPersPageID( void * aPage, scPageID aPID )
        {
            smpManager::setNextPersPageID( aPage, aPID );
        };

        static void linkPersPage( void      * aPage,
                                  scPageID    aSelf,
                                  scPageID    aPrev,
                                  scPageID    aNext )
        {
            smpManager::linkPersPage( aPage,
                                      aSelf,
                                      aPrev,
                                      aNext );
        };

        static void initPersPageType( void * aPage )
        {
            smpManager::initPersPageType( aPage );
        };

        static IDE_RC initializeFreePageHeader( scSpaceID aSpaceID, scPageID aPageID )
        {
            return smpFreePageList::initializeFreePageHeaderAtPCH( aSpaceID, aPageID );
        };

        static IDE_RC destroyFreePageHeader( scSpaceID aSpaceID, scPageID aPageID )
        {
            return smpFreePageList::destroyFreePageHeaderAtPCH( aSpaceID, aPageID );
        };

        static IDE_RC alterTBSStatus( void       * aTrans,
                                      smmTBSNode * aTBSNode,
                                      UInt         aState )
        {
            return smpTBSAlterOnOff::alterTBSStatus( aTrans,
                                                     aTBSNode,
                                                     aState );
        };


        /* smr */
        static IDE_RC writeMemoryTBSCreate( idvSQL               * aStatistics,
                                            void                 * aTrans,
                                            smiTableSpaceAttr    * aTBSAttr,
                                            smiChkptPathAttrList * aChkptPathAttrList )
        {
            return smrUpdate::writeMemoryTBSCreate( aStatistics,
                                                    aTrans,
                                                    aTBSAttr,
                                                    aChkptPathAttrList );
        };

        static IDE_RC writeMemoryDBFileCreate( idvSQL       * aStatistics,
                                               void         * aTrans,
                                               scSpaceID      aSpaceID,
                                               UInt           aPingPongNo,
                                               UInt           aDBFileNo )
        {
            return smrUpdate::writeMemoryDBFileCreate( aStatistics,
                                                       aTrans,
                                                       aSpaceID,
                                                       aPingPongNo,
                                                       aDBFileNo );
        };

        static IDE_RC writeMemoryTBSDrop( idvSQL        * aStatistics,
                                          void          * aTrans,
                                          scSpaceID       aSpaceID,
                                          smiTouchMode    aTouchMode )
        {
            return smrUpdate::writeMemoryTBSDrop( aStatistics,
                                                  aTrans,
                                                  aSpaceID,
                                                  aTouchMode );
        };

        static IDE_RC writeMemoryTBSAlterAutoExtend( idvSQL     * aStatistics,
                                                     void       * aTrans,
                                                     scSpaceID    aSpaceID,
                                                     idBool       aBIsAutoExtend,
                                                     scPageID     aBNextPageCount,
                                                     scPageID     aBMaxPageCount,
                                                     idBool       aAIsAutoExtend,
                                                     scPageID     aANextPageCount,
                                                     scPageID     aAMaxPageCount )
        {
            return smrUpdate::writeMemoryTBSAlterAutoExtend( aStatistics,
                                                             aTrans,
                                                             aSpaceID,
                                                             aBIsAutoExtend,
                                                             aBNextPageCount,
                                                             aBMaxPageCount,
                                                             aAIsAutoExtend,
                                                             aANextPageCount,
                                                             aAMaxPageCount );
        };

        static IDE_RC allocExpandChunkAtMembase( idvSQL     * aStatistics,
                                                 void       * aTrans,
                                                 scSpaceID    aSpaceID,
                                                 smmMemBase * aMemBase,
                                                 UInt         aExpandPageListID,
                                                 scPageID     aAfterChunkFirstPID,
                                                 scPageID     aAfterChunkLastPID,
                                                 smLSN      * aBeginLSN )
        {
            return smrUpdate::allocExpandChunkAtMembase( aStatistics,
                                                         aTrans,
                                                         aSpaceID,
                                                         aMemBase,
                                                         aExpandPageListID,
                                                         aAfterChunkFirstPID,
                                                         aAfterChunkLastPID,
                                                         aBeginLSN );
        };

        static IDE_RC allocPersListAtMembase( idvSQL        * aStatistics,
                                              void          * aTrans,
                                              scSpaceID       aSpaceID,
                                              smmMemBase    * aMemBase,
                                              smmFPLNo        aFPLNo,
                                              scPageID        aAfterPageID,
                                              vULong          aAfterPageCount )
        {
            return smrUpdate::allocPersListAtMembase( aStatistics,
                                                      aTrans,
                                                      aSpaceID,
                                                      aMemBase,
                                                      aFPLNo,
                                                      aAfterPageID,
                                                      aAfterPageCount );
        };

        static IDE_RC updateLinkAtPersPage( idvSQL      * aStatistics,
                                            void        * aTrans,
                                            scSpaceID     aSpaceid,
                                            scPageID      aPid,
                                            scPageID      aBeforePrevPageID,
                                            scPageID      aBeforeNextPageID,
                                            scPageID      aAfterPrevPageID,
                                            scPageID      aAfterNextPageID )
        {
            return smrUpdate::updateLinkAtPersPage( aStatistics,
                                                    aTrans,
                                                    aSpaceid,
                                                    aPid,
                                                    aBeforePrevPageID,
                                                    aBeforeNextPageID,
                                                    aAfterPrevPageID,
                                                    aAfterNextPageID );
        };

        static IDE_RC updateLinkAtFreePage( idvSQL      * aStatistics,
                                            void        * aTrans,
                                            scSpaceID     aSpaceID,
                                            scPageID      aFreeListInfoPID,
                                            UInt          aFreePageSlotOffset,
                                            scPageID      aBeforeNextFreePID,
                                            scPageID      aAfterNextFreePID )
        {
            return smrUpdate::updateLinkAtFreePage( aStatistics,
                                                    aTrans,
                                                    aSpaceID,
                                                    aFreeListInfoPID,
                                                    aFreePageSlotOffset,
                                                    aBeforeNextFreePID,
                                                    aAfterNextFreePID );
        };

        static IDE_RC setMemBaseInfo( idvSQL        * aStatistics,
                                      void          * aTrans,
                                      scSpaceID       aSpaceID,
                                      smmMemBase    * aMemBase )
        {
            return smrUpdate::setMemBaseInfo( aStatistics,
                                              aTrans,
                                              aSpaceID,
                                              aMemBase );
        };

        static IDE_RC setSystemSCN( smSCN aSystemSCN )
        {
            return smrUpdate::setSystemSCN( aSystemSCN );
        };

        static IDE_RC writeNullNTALogRec( idvSQL    * aStatistics,
                                          void      * aTrans,
                                          smLSN     * aLSN )
        {
            return smrLogMgr::writeNullNTALogRec( aStatistics,
                                                  aTrans,
                                                  aLSN );
        };

        static IDE_RC writeAllocPersListNTALogRec( idvSQL   * aStatistics,
                                                   void     * aTrans,
                                                   smLSN    * aLSN,
                                                   scSpaceID  aSpaceID,
                                                   scPageID   aFirstPID,
                                                   scPageID   aLastPID )
        {
            return smrLogMgr::writeAllocPersListNTALogRec( aStatistics,
                                                           aTrans,
                                                           aLSN,
                                                           aSpaceID,
                                                           aFirstPID,
                                                           aLastPID );
        };

        static IDE_RC writeCreateTbsNTALogRec( idvSQL   * aStatistics,
                                               void     * aTrans,
                                               smLSN    * aLSN,
                                               scSpaceID  aSpaceID)
        {
            return smrLogMgr::writeCreateTbsNTALogRec( aStatistics,
                                                       aTrans,
                                                       aLSN,
                                                       aSpaceID );
        };

        static IDE_RC undoTrans( idvSQL * aStatistics,
                                 void   * aTrans,
                                 smLSN  * aLsn )
        {
            return smrRecoveryMgr::undoTrans( aStatistics,
                                              aTrans,
                                              aLsn );
        };

        static idBool isLogFinished( )
        {
            return smrRecoveryMgr::isFinish();
        };

        static IDE_RC syncLFThread( smrSyncByWho aWhoSyncLog )
        {
            return smrLogMgr::syncToLstLSN( aWhoSyncLog );
        };

        static void getLstLSN( smLSN * aLstLSN )
        {
            smrLogMgr::getLstLSN ( aLstLSN );
        };

        static IDE_RC updateAnchorOfTBS()
        {
            return smrRecoveryMgr::updateAnchorAllTBS();
        };

        static IDE_RC updateTBSNodeAndFlush( sctTableSpaceNode* aSpaceNode )
        {
            return smrRecoveryMgr::updateTBSNodeToAnchor( aSpaceNode );
        };

        static IDE_RC updateChkptImageAttrAndFlush( smmCrtDBFileInfo    * aCrtDBFileInfo,
                                                    smmChkptImageAttr   * aChkptImageAttr )
        {
            return smrRecoveryMgr::updateChkptImageAttrToAnchor( aCrtDBFileInfo,
                                                                 aChkptImageAttr );
        };

        static IDE_RC updateChkptPathAndFlush( smmChkptPathNode * aChkptPathNode )
        {
            return smrRecoveryMgr::updateChkptPathToLogAnchor( aChkptPathNode );
        };

        static IDE_RC addTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode )
        {
            return smrRecoveryMgr::addTBSNodeToAnchor( aSpaceNode );
        };

        static IDE_RC addChkptPathNodeAndFlush( smmChkptPathNode * aChkptPathNode )
        {
            return smrRecoveryMgr::addChkptPathNodeToAnchor( aChkptPathNode );
        };

        static IDE_RC addChkptImageAttrAndFlush( smmCrtDBFileInfo  * aCrtDBFileInfo,
                                                 smmChkptImageAttr * aChkptImageAttr )
        {
            return smrRecoveryMgr::addChkptImageAttrToAnchor( aCrtDBFileInfo,
                                                              aChkptImageAttr );
        };

        static UInt getSmVersionIDFromLogAnchor()
        {
            return smrRecoveryMgr::getSmVersionIDFromLogAnchor();
        };

        /* PROJ-2133 incremental backup */
        static IDE_RC getDataFileDescSlotIDFromLogAncho4ChkptImage( UInt                    aReadOffset,                   
                                                                    smiDataFileDescSlotID * aDataFileDescSlotID )
        {
            return smrRecoveryMgr::getDataFileDescSlotIDFromLogAncho4ChkptImage( aReadOffset,                   
                                                                                 aDataFileDescSlotID );
        };

        static IDE_RC backupMemoryTBS( idvSQL     * aStatistics,
                                       smmTBSNode * aSpaceNode,
                                       SChar      * aBackupDir )
        {
            return smrBackupMgr::backupMemoryTBS( aStatistics,
                                                  aSpaceNode,
                                                  aBackupDir );
        };

        /* PROJ-2133 incremental backup */
        static IDE_RC incrementalBackupMemoryTBS( idvSQL     * aStatistics,
                                                  smmTBSNode * aSpaceNode,
                                                  SChar      * aBackupDir,
                                                  smriBISlot * aCommonBackupInfo )
        {
            return smrBackupMgr::incrementalBackupMemoryTBS( aStatistics,
                                                             aSpaceNode,
                                                             aBackupDir,
                                                             aCommonBackupInfo );
        };

        /* PROJ-2133 incremental backup */
        static idBool isCTMgrEnabled()
        {
            return smrRecoveryMgr::isCTMgrEnabled();
        };

        /* PROJ-2133 incremental backup */
        static idBool isCreatingCTFile()
        {
            return smrRecoveryMgr::isCreatingCTFile();
        };

        static idBool isLSNEQ( const smLSN * aLSN1, const smLSN * aLSN2 )
        {
            return smrCompareLSN::isEQ( aLSN1, aLSN2 );
        };

        static idBool isLSNLTE( const smLSN * aLSN1, const smLSN * aLSN2 )
        {
            return smrCompareLSN::isLTE( aLSN1, aLSN2 );
        };

        static idBool isLSNGT( const smLSN * aLSN1, const smLSN * aLSN2 )
        {
            return smrCompareLSN::isGT( aLSN1, aLSN2 );
        };

        static smLSN getLstCheckLogLSN()
        {
            return smrRedoLSNMgr::getLstCheckLogLSN();
        };

        static IDE_RC blockCheckpoint()
        {
            return smrChkptThread::blockCheckpoint();
        };

        /* Checkpoint가 다시 수행되도록 Checkpoint를 Unblock한다. */
        static IDE_RC unblockCheckpoint()
        {
            return smrChkptThread::unblockCheckpoint();
        };

        static idBool isRestart()
        {
            return smrRecoveryMgr::isRestart();
        };

        static idBool isRestartRecoveryPhase()
        {
            return smrRecoveryMgr::isRestartRecoveryPhase();
        };

        static idBool isMediaRecoveryPhase()
        {
            return smrRecoveryMgr::isMediaRecoveryPhase();
        };


        /* smn */
        static IDE_RC prepareIdxFreePages( )
        {
            return smnManager::prepareIdxFreePages();
        };

        static IDE_RC releaseIdxFreePages( )
        {
            return smnManager::releaseIdxFreePages();
        };


        /* smx */
        static void setSmmCallbacksInSmx( void )
        {
            smxTransMgr::setSmmCallbacks();
        };

        static void unsetSmmCallbacksInSmx( void )
        {
            smxTransMgr::unsetSmmCallbacks();
        };

        static void * getTransByTID( smTID aTID )
        {
            return smxTransMgr::getTransByTID2Void( aTID );
        };

        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };

        static void setTransStatus( void     * aTrans,
                                    UInt       aStatus )
        {
            smxTrans::setTransStatus( aTrans,
                                      aStatus );
        };

        static void setTransSCNnStatus( void     * aTrans,
                                        idBool     aIsLegacyTrans,
                                        smSCN    * aSCN,
                                        void     * aStatus )
        {
            smxTrans::setTransSCNnStatus( aTrans,
                                          aIsLegacyTrans,
                                          aSCN,
                                          aStatus );
        };

        static void allocRSGroupID( void * aTrans, UInt * aPageListIdx )
        {
            smxTrans::allocRSGroupID( aTrans, aPageListIdx );
        };

        static UInt getRSGroupID( void * aTrans)
        {
            return smxTrans::getRSGroupID( aTrans );
        };

        static void setRSGroupID( void * aTrans, UInt aIdx )
        {
            smxTrans::setRSGroupID( aTrans, aIdx );
        };


        /* sml */
        static IDE_RC lockItem( void     * aTrans,
                                void     * aLockItem,
                                idBool     aIsIntent,
                                idBool     aIsExclusive,
                                ULong      aLockWaitMicroSec,
                                idBool   * aLocked,
                                void    ** aLockSlot )
        {
            return smlLockMgr::lockItem( aTrans,
                                         aLockItem,
                                         aIsIntent,
                                         aIsExclusive,
                                         aLockWaitMicroSec,
                                         aLocked,
                                         aLockSlot );
        };

        static IDE_RC unlockItem( void * aTrans, void * aLockSlot )
        {
            return smlLockMgr::unlockItem( aTrans, aLockSlot );
        };


        /* sma */
        static idBool isMemGCinitialized()
        {
            return smaLogicalAger::isInitialized();
        };

        static IDE_RC blockMemGC()
        {
            return smaLogicalAger::block();
        };

        static IDE_RC unblockMemGC()
        {
            return smaLogicalAger::unblock();
        };
};

#define smLayerCallback   smmReqFunc

#endif


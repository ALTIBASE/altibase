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
 * $Id: sddReq.h 86110 2019-09-02 04:52:04Z et16 $
 **********************************************************************/

#ifndef _O_SDD_REQ_H_
#define _O_SDD_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smxAPI.h>
#include <sdpAPI.h>
#include <smcAPI.h>
#include <smiAPI.h>
#include <smrAPI.h>
#include <smlAPI.h>
#include <sdsAPI.h>

struct smuDynArrayBase;

class sddReqFunc
{
    public:

        /* smi */
        static void setEmergency( idBool aFlag )
        {
            smiSetEmergency( aFlag );
        };

        /* smx */
        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };
        static idBool isBeginTrans( void * aTrans )
        {
            return smxTrans::isTxBeginStatus( aTrans );
        };

        /* smc */
        static IDE_RC alterTBSOffline4Tables( idvSQL    * aStatistics,
                                              scSpaceID   aTBSID )
        {
            return smcTableSpace::alterTBSOffline4Tables( aStatistics,
                                                          aTBSID );
        };

        /* sdp */

        static IDE_RC resetTBS( idvSQL    * aStatistics,
                                scSpaceID   aSpaceID,
                                void      * aTrans )
        {
            return sdpTableSpace::resetTBS( aStatistics,
                                            aSpaceID,
                                            aTrans );
        };

        static IDE_RC alterTBSOnlineCommitPending( idvSQL            * aStatistics,
                                                   sctTableSpaceNode * aTBSNode,
                                                   sctPendingOp      * aPendingOp )
        {
            return sdpTableSpace::alterOnlineCommitPending( aStatistics,
                                                            aTBSNode,
                                                            aPendingOp );
        };
        static IDE_RC alterTBSOfflineCommitPending( idvSQL            * aStatistics,
                                                    sctTableSpaceNode * aTBSNode,
                                                    sctPendingOp      * aPendingOp )
        {
            return sdpTableSpace::alterOfflineCommitPending( aStatistics,
                                                             aTBSNode,
                                                             aPendingOp );
        };
        static IDE_RC freeSpaceCacheCommitPending( idvSQL            * aStatistics,
                                                   sctTableSpaceNode * aTBSNode,
                                                   sctPendingOp      * aPendingOp )
        {
            return sdpTableSpace::freeSpaceCacheCommitPending( aStatistics,
                                                               aTBSNode,
                                                               aPendingOp );
        };
        static IDE_RC tracePage( UInt           aChkFlag,
                                 ideLogModule   aModule,
                                 UInt           aLevel,
                                 const UChar  * aPage,
                                 const SChar  * aTitle,
                                 ... )
        {
            va_list ap;
            IDE_RC result;

            va_start( ap, aTitle );

            result = sdpPhyPage::tracePageInternal( aChkFlag,
                                                    aModule,
                                                    aLevel,
                                                    aPage,
                                                    aTitle,
                                                    ap );

            va_end( ap );

            return result;
        };

        /* smr */
        static idBool isRestart()
        {
            return smrRecoveryMgr::isRestart();
        };
        static IDE_RC undoTrans4LayerCall( idvSQL * aStatistics,
                                           void   * aTrans,
                                           smLSN  * aLSN )
        {
            return smrRecoveryMgr::undoTrans( aStatistics,
                                              aTrans,
                                              aLSN );
        };
        static idBool isFinish()
        {
            return smrRecoveryMgr::isFinish();
        };
        static IDE_RC updateTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode )
        {
            return smrRecoveryMgr::updateTBSNodeToAnchor( aSpaceNode );
        };
        static IDE_RC updateDBFNodeAndFlush( sddDataFileNode * aFileNode )
        {
            return smrRecoveryMgr::updateDBFNodeToAnchor( aFileNode );
        };
        static IDE_RC addTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode )
        {
            return smrRecoveryMgr::addTBSNodeToAnchor( aSpaceNode );
        };
        static IDE_RC addDBFNodeAndFlush( sddTableSpaceNode * aSpaceNode,
                                          sddDataFileNode   * aFileNode )
        {
            return smrRecoveryMgr::addDBFNodeToAnchor( aSpaceNode,
                                                       aFileNode );
        };
        static UInt getSmVersionIDFromLogAnchor()
        {
            return smrRecoveryMgr::getSmVersionIDFromLogAnchor();
        };
        static void getDiskRedoLSNFromLogAnchor( smLSN * aDiskRedoLSN )
        {
            smrRecoveryMgr::getDiskRedoLSNFromLogAnchor( aDiskRedoLSN );
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
        /* PROJ-2133 incremental backup */
        static IDE_RC getDataFileDescSlotIDFromLogAncho4DBF( UInt                    aReadOffset,
                                                             smiDataFileDescSlotID * aDataFileDescSlotID )
        {
            return smrRecoveryMgr::getDataFileDescSlotIDFromLogAncho4DBF( aReadOffset,
                                                                          aDataFileDescSlotID );
        };
        static idBool isLSNEQ( const smLSN * aLSN1,
                               const smLSN * aLSN2 )
        {
            return smrCompareLSN::isEQ( aLSN1,
                                        aLSN2 );
        };
        static idBool isLSNLTE( const smLSN * aLSN1,
                                const smLSN * aLSN2 )
        {
            return smrCompareLSN::isLTE( aLSN1,
                                         aLSN2 );
        };
        static idBool isLSNGT( const smLSN * aLSN1,
                               const smLSN * aLSN2 )
        {
            return smrCompareLSN::isGT( aLSN1,
                                        aLSN2 );
        };
        static IDE_RC writeNTAForExtendDBF( idvSQL  * aStatistics,
                                            void    * aTrans,
                                            smLSN   * aLsnNTA )
        {
            return smrUpdate::writeNTAForExtendDBF( aStatistics,
                                                    aTrans,
                                                    aLsnNTA );
        };
        static IDE_RC writeDiskTBSCreateDrop( idvSQL             * aStatistics,
                                              void               * aTrans,
                                              sctUpdateType        aFOPType,
                                              scSpaceID            aSpaceID,
                                              smiTableSpaceAttr  * aTableSpaceAttr, /* PROJ-1923 */
                                              smLSN              * aBeginLSN )
        {
            return smrUpdate::writeDiskTBSCreateDrop( aStatistics,
                                                      aTrans,
                                                      aFOPType,
                                                      aSpaceID,
                                                      aTableSpaceAttr,
                                                      aBeginLSN );
        };
        static IDE_RC writeLogCreateDBF( idvSQL             * aStatistics,
                                         void               * aTrans,
                                         scSpaceID            aSpaceID,
                                         sddDataFileNode    * aFileNode,
                                         smiTouchMode         aTouchMode,
                                         smiDataFileAttr    * aFileAttr, /* PROJ-1923 */
                                         smLSN              * aBeginLSN )
        {
            return smrUpdate::writeLogCreateDBF( aStatistics,
                                                 aTrans,
                                                 aSpaceID,
                                                 aFileNode,
                                                 aTouchMode,
                                                 aFileAttr,
                                                 aBeginLSN );
        };
        static IDE_RC writeLogDropDBF( idvSQL             * aStatistics,
                                       void               * aTrans,
                                       scSpaceID            aSpaceID,
                                       sddDataFileNode    * aFileNode,
                                       smiTouchMode         aTouchMode,
                                       smLSN              * aBeginLSN )
        {
            return smrUpdate::writeLogDropDBF( aStatistics,
                                               aTrans,
                                               aSpaceID,
                                               aFileNode,
                                               aTouchMode,
                                               aBeginLSN );
        };
        static IDE_RC writeLogExtendDBF( idvSQL             * aStatistics,
                                         void               * aTrans,
                                         scSpaceID            aSpaceID,
                                         sddDataFileNode    * aFileNode,
                                         ULong                aAfterCurrSize,
                                         smLSN              * aBeginLSN )
        {
            return smrUpdate::writeLogExtendDBF( aStatistics,
                                                 aTrans,
                                                 aSpaceID,
                                                 aFileNode,
                                                 aAfterCurrSize,
                                                 aBeginLSN );
        };
        static IDE_RC writeLogShrinkDBF( idvSQL             * aStatistics,
                                         void               * aTrans,
                                         scSpaceID            aSpaceID,
                                         sddDataFileNode    * aFileNode,
                                         ULong                aAfterInitSize,
                                         ULong                aAfterCurrSize,
                                         smLSN              * aBeginLSN )
        {
            return smrUpdate::writeLogShrinkDBF( aStatistics,
                                                 aTrans,
                                                 aSpaceID,
                                                 aFileNode,
                                                 aAfterInitSize,
                                                 aAfterCurrSize,
                                                 aBeginLSN );
        };
        static IDE_RC writeLogSetAutoExtDBF( idvSQL             * aStatistics,
                                             void               * aTrans,
                                             scSpaceID            aSpaceID,
                                             sddDataFileNode    * aFileNode,
                                             idBool               aAfterAutoExtMode,
                                             ULong                aAfterNextSize,
                                             ULong                aAfterMaxSize,
                                             smLSN              * aBeginLSN )
        {
            return smrUpdate::writeLogSetAutoExtDBF( aStatistics,
                                                     aTrans,
                                                     aSpaceID,
                                                     aFileNode,
                                                     aAfterAutoExtMode,
                                                     aAfterNextSize,
                                                     aAfterMaxSize,
                                                     aBeginLSN );
        };
        static IDE_RC backupDiskTBS( idvSQL            * aStatistics,
                                     sddTableSpaceNode * aSpaceNode,
                                     SChar             * aBackupDir )
        {
            return smrBackupMgr::backupDiskTBS( aStatistics,
                                                aSpaceNode,
                                                aBackupDir );
        };
        /* PROJ-2133 incremental backup */
        static IDE_RC incrementalBackupDiskTBS( idvSQL            * aStatistics,
                                                sddTableSpaceNode * aSpaceNode,
                                                SChar             * aBackupDir,
                                                smriBISlot        * aCommonBackupInfo )
        {
            return smrBackupMgr::incrementalBackupDiskTBS( aStatistics,
                                                           aSpaceNode,
                                                           aBackupDir,
                                                           aCommonBackupInfo );
        };
        static void getLstLSN( smLSN * aLSN )
        {
            smrLogMgr::getLstLSN( aLSN );
        };
        static IDE_RC blockCheckpoint()
        {
            return smrChkptThread::blockCheckpoint();
        };
        static IDE_RC unblockCheckpoint()
        {
            return smrChkptThread::unblockCheckpoint();
        };

        /* sdr */
        static IDE_RC addRecvFileToHash( sddDataFileHdr * aDBFileHdr,
                                         SChar          * aFileName,
                                         smLSN          * aFromRedoLSN,
                                         smLSN          * aToRedoLSN )
        {
            return sdrRedoMgr::addRecvFileToHash( aDBFileHdr,
                                                  aFileName,
                                                  aFromRedoLSN,
                                                  aToRedoLSN );
        };

        /* sml */
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
        static IDE_RC lockItem( void        * aTrans,
                                void        * aLockItem,
                                idBool        aIsIntent,
                                idBool        aIsExclusive,
                                ULong         aLockWaitMicroSec,
                                idBool      * aLocked,
                                void       ** aLockSlot )
        {
            return smlLockMgr::lockItem( aTrans,
                    aLockItem,
                    aIsIntent,
                    aIsExclusive,
                    aLockWaitMicroSec,
                    aLocked,
                    aLockSlot );
        };

        /* sdb */
        static IDE_RC discardPagesInRange( idvSQL     * aStatistics,
                                           scSpaceID    aSpaceID,
                                           scPageID     aStartID,
                                           scPageID     aEndID )
        {
            return sdbBufferMgr::discardPagesInRange( aStatistics,
                                                      aSpaceID,
                                                      aStartID,
                                                      aEndID );
        };
        static IDE_RC wait4AllFlusher2Do1JobDone()
        {
            return sdbFlushMgr::wait4AllFlusher2Do1JobDone();
        };

        /* sds */
        static IDE_RC discardPagesInRangeInSBuffer( idvSQL    * aStatistics,
                                                    scSpaceID   aSpaceID,
                                                    scPageID    aStartID,
                                                    scPageID    aEndID )
        {
            return sdsBufferMgr::discardPagesInRange( aStatistics,
                                                      aSpaceID,
                                                      aStartID,
                                                      aEndID );
        };
        static IDE_RC wait4AllFlusher2Do1JobDoneInSBuffer()
        {
            return sdsFlushMgr::wait4AllFlusher2Do1JobDone();
        };
};

#define smLayerCallback    sddReqFunc

#endif

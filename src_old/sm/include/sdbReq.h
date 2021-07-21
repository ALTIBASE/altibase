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
 * $Id: sdbReq.h 90522 2021-04-09 01:29:20Z emlee $
 **********************************************************************/

#ifndef _O_SDB_REQ_H_
#define _O_SDB_REQ_H_ 1

#include <sdcAPI.h>
#include <sdrAPI.h>
#include <sdpAPI.h>
#include <smrAPI.h>
#include <smx.h>

class sdbReqFunc
{
    public:

        /* minitrans */
        static void* getMtxTrans( void * aMtx )
        {
            return sdrMiniTrans::getMtxTrans( aMtx );
        };
        static IDE_RC pushToMtx( void   * aMtx,
                                 void   * aObject,
                                 UInt     aLatchMode )
        {
            return sdrMiniTrans::push( aMtx,
                                       aObject,
                                       aLatchMode );
        };
        static smLSN getMtxEndLSN( void * aMtx )
        {
            return sdrMiniTrans::getEndLSN( aMtx );
        };
        static IDE_RC writeNBytesToMtx( void    * aMtx,
                                        UChar   * aDest,
                                        void    * aValue,
                                        UInt      aLogType )
        {
            return sdrMiniTrans::writeNBytes( aMtx ,
                                              aDest,
                                              aValue,
                                              aLogType );
        };
        static idBool isMtxModeLogging( void * aMtx )
        {
            return sdrMiniTrans::isModeLogging( aMtx );
        };
        static idBool isMtxModeNoLogging( void * aMtx )
        {
            return sdrMiniTrans::isModeNoLogging( aMtx );
        };
        static idBool isMtxNologgingPersistent( void * aMtx )
        {
            return sdrMiniTrans::isNologgingPersistent( aMtx );
        };
        static idBool isLogWritten( void * aMtx )
        {
            return sdrMiniTrans::isLogWritten( aMtx );
        };

        /* physical page */
        static UChar * getLogicalHdrStartPtr( UChar * aPagePtr )
        {
            return sdpPhyPage::getLogicalHdrStartPtr( aPagePtr );
        };
        static UChar * getPageStartPtr( void * aPagePtr )
        {
            return sdpPhyPage::getPageStartPtr( aPagePtr );
        };
        static idBool isPageCorrupted( UChar    * aPagePtr )
        {
            return sdpPhyPage::isPageCorrupted( aPagePtr );
        };
        static idBool checkAndSetPageCorrupted( scSpaceID  aSpaceID,
                                                UChar    * aPagePtr )
        {
            return sdpPhyPage::checkAndSetPageCorrupted( aSpaceID,
                                                         aPagePtr );
        };
#if 0 // not used
        static idBool isPageTempType( UChar * aStartPtr )
        {
            return sdpPhyPage::isPageTempType( aStartPtr );
        };
#endif
        static idBool isPageTSSType( UChar * aStartPtr )
        {
            return sdpPhyPage::isPageTSSType( aStartPtr );
        };
        static idBool isPageIndexType( UChar * aStartPtr )
        {
            return sdpPhyPage::isPageIndexType( aStartPtr );
        };
        static idBool isPageIndexOrIndexMetaType( UChar * aStartPtr )
        {
            return sdpPhyPage::isPageIndexOrIndexMetaType( aStartPtr );
        };
        static scPageID getPageID( UChar * aPagePtr )
        {
            return sdpPhyPage::getPageID( aPagePtr );
        };
        static scSpaceID getSpaceID( UChar * aPagePtr )
        {
            return sdpPhyPage::getSpaceID( aPagePtr );
        };
        static ULong getIndexSMONo( UChar  * aPagePtr )
        {
            return sdpPhyPage::getIndexSMONo( (sdpPhyPageHdr*)aPagePtr );
        };
        static smLSN getPageLSN( UChar * aStartPtr )
        {
            return sdpPhyPage::getPageLSN( aStartPtr );
        };
        static UInt getPhyPageType( UChar * aPagePtr )
        {
            return sdpPhyPage::getPhyPageType( aPagePtr );
        };
        static UInt getPhyPageTypeCount()
        {
            return sdpPhyPage::getPhyPageTypeCount();
        };
        static UInt getUndoPageType()
        {
            return sdpPhyPage::getUndoPageType();
        };
        static idBool isValidPageType( scSpaceID aSpaceID,
                                       scPageID  aPageID,
                                       UInt      aPageType )
        {
            return sdpPhyPage::isValidPageType( aSpaceID,
                                                aPageID,
                                                aPageType );
        };
        static UChar * getSlotDirStartPtr( UChar * aPagePtr )
        {
            return sdpPhyPage::getSlotDirStartPtr( aPagePtr );
        };
        static void setPageLSN( UChar * aPageHdr,
                                smLSN * aPageLSN )
        {
            sdpPhyPage::setPageLSN( aPageHdr,
                                    aPageLSN );
        };
        static void calcAndSetCheckSum( UChar * aPageHdr )
        {
            sdpPhyPage::calcAndSetCheckSum( aPageHdr );
        };
        static IDE_RC traceDiskPage( UInt           aChkFlag,
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
        static IDE_RC getPagePtrFromSlotNum( UChar       * aPagePtr,
                                             scSlotNum     aSlotNum,
                                             UChar      ** aSlotPtr )
        {
            return sdpSlotDirectory::getPagePtrFromSlotNum( aPagePtr,
                                                            aSlotNum,
                                                            aSlotPtr );
        };
        static sdpSegMgmtOp * getSegMgmtOp( scSpaceID aSpaceID )
        {
            return sdpSegDescMgr::getSegMgmtOp( aSpaceID );
        };
        static idBool isTempTableSpace( scSpaceID aSpaceID )
        {
            return sctTableSpaceMgr::isTempTableSpace( aSpaceID );
        };

        /* sdr */
        static void * getStatSQL( void * aMtx )
        {
            return sdrMiniTrans::getStatSQL( aMtx );
        };
        static sdbCorruptPageReadPolicy getCorruptPageReadPolicy()
        {
            return sdrCorruptPageMgr::getCorruptPageReadPolicy();
        };

        /* smr */
        static idBool isLSNGT( const smLSN * aLsn1,
                               const smLSN * aLsn2 )
        {
            return smrCompareLSN::isGT( aLsn1 ,
                                        aLsn2 );
        };
        static idBool isLSNLT( const smLSN * aLsn1,
                               const smLSN * aLsn2 )
        {
            return smrCompareLSN::isLT( aLsn1 ,
                                        aLsn2 );
        };
        static idBool isLSNGTE( const smLSN * aLsn1,
                                const smLSN * aLsn2 )
        {
            return smrCompareLSN::isGTE( aLsn1 ,
                                         aLsn2 );
        };
        static idBool isLSNEQ( const smLSN * aLsn1,
                               const smLSN * aLsn2 )
        {
            return smrCompareLSN::isEQ( aLsn1 ,
                                        aLsn2 );
        };
        static idBool isLSNZero( const smLSN * aLSN )
        {
            return smrCompareLSN::isZero( aLSN );
        };
        static void resetIndexSMONo( void       * aPageHdr,
                                     scSpaceID    aSpaceID,
                                     idBool       aChkOnlineTBS )
        {
            sdpPhyPage::resetIndexSMONo( (sdpPhyPageHdr*)aPageHdr,
                                         aSpaceID,
                                         aChkOnlineTBS );
        };
        static idBool isCheckpointFlushNeeded( smLSN aLastWrittenLSN )
        {
            return smrRecoveryMgr::isCheckpointFlushNeeded( aLastWrittenLSN );
        };
        static idBool isCTMgrEnabled()
        {
            return smrRecoveryMgr::isCTMgrEnabled();
        };
        static IDE_RC flushForCheckpoint( idvSQL * aStatistics,
                                          ULong    aMinFlushCount,
                                          ULong    aRedoDirtyPageCnt,
                                          UInt     aRedoLogFileCount,
                                          ULong  * aFlushedCount )
        {
            return smrChkptThread::flushForCheckpoint( aStatistics,
                                                       aMinFlushCount,
                                                       aRedoDirtyPageCnt,
                                                       aRedoLogFileCount,
                                                       aFlushedCount );
        };
        static IDE_RC sync4BufferFlush( smLSN * aLSN,
                                        UInt  * aSyncedLFCnt )
        {
            return smrLogMgr::sync4BufferFlush( aLSN ,
                                                aSyncedLFCnt );
        };
        static void getLstLSN( smLSN * aLSN )
        {
            smrLogMgr::getLstLSN( aLSN );
        };
        static IDE_RC writeDPathPageLogRec( idvSQL * aStatistics,
                                            UChar  * aBuffer,
                                            scGRID   aPageGRID,
                                            smLSN  * aEndLSN )
        {
            return smrLogMgr::writeDPathPageLogRec( aStatistics,
                                                    aBuffer,
                                                    aPageGRID,
                                                    aEndLSN );
        };
        static IDE_RC writeDiskPILogRec( idvSQL * aStatistics,
                                         UChar  * aBuffer,
                                         scGRID   aPageGRID )
        {
            return smrLogMgr::writeDiskPILogRec( aStatistics,
                                                 aBuffer,
                                                 aPageGRID );
        };
        static idBool isBackupingTBS( scSpaceID aSpaceID )
        {
            return smrBackupMgr::isBackupingTBS( aSpaceID );
        };

        /* smx */
        static void blockAllTx( void   * aTrans,
                                ULong    aTryMicroSec,
                                idBool * aSuccess )
        {
            smxTransMgr::block( aTrans,
                                aTryMicroSec,
                                aSuccess );
        };
        static void unblockAllTx( void )
        {
            smxTransMgr::unblock();
        };
        static idvSQL* getStatistics( void * aTrans )
        {
            return smxTrans::getStatistics( aTrans );
        };
        static void initTransLogBuffer( void * aTrans )
        {
            smxTrans::initTransLogBuffer( aTrans );
        };
        static IDE_RC setTransLogBufferSize( void * aTrans,
                                             UInt   aNeedSize )
        {
            return smxTrans::setTransLogBufferSize( aTrans,
                                                    aNeedSize );
        };
        static SChar * getTransLogBuffer( void * aTrans )
        {
            return smxTrans::getTransLogBuffer( aTrans );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };


        /* sdc */
        static void * getDPathBuffInfo( void * aTrans )
        {
            return sdcDPathInsertMgr::getDPathBuffInfo( aTrans );
        };
        static IDE_RC dumpDPathEntry( void * aTrans )
        {
            return sdcDPathInsertMgr::dumpDPathEntry( aTrans );
        };
};

#define smLayerCallback    sdbReqFunc

#endif

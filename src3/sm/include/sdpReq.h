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
 * $Id: sdpReq.h 88717 2020-09-24 08:12:20Z et16 $
 **********************************************************************/


#ifndef _O_SDP_REQ_H_
#define _O_SDP_REQ_H_ 1

#include <smpAPI.h>
#include <smrAPI.h>
#include <smxAPI.h>
#include <smnAPI.h>
#include <sdcAPI.h>
#include <smcAPI.h>
#include <smnAPI.h>
#include <smlAPI.h>

class sdpReqFunc
{
    public:

        /* smp */
        static UInt getSlotSize()
        {
            return smpManager::getSlotSize();
        };

        /* smx */
        static SLong getRecCntOfTableInfo( void * aTableInfoPtr )
        {
            return smxTableInfoMgr::getRecCntOfTableInfo( aTableInfoPtr );
        };
        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };
        static void * getTransByTID2Void( smTID aTID )
        {
            return smxTransMgr::getTransByTID2Void( aTID );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static void setHintDataPIDofTableInfo( void     * aTableInfo,
                                               scPageID   aHintDataPID )
        {
            smxTrans::setHintDataPIDofTableInfo( aTableInfo,
                                                 aHintDataPID );
        };
        static void getHintDataPIDofTableInfo( void       * aTableInfo,
                                               scPageID   * aHintDataPID )
        {
            smxTrans::getHintDataPIDofTableInfo( aTableInfo,
                                                 aHintDataPID );
        };
        static IDE_RC incRecordCountOfTableInfo( void  * aTrans,
                                                 smOID   aTableOID,
                                                 SLong   aRecordCnt )
        {
            return smxTrans::incRecordCountOfTableInfo( aTrans,
                                                        aTableOID,
                                                        aRecordCnt );
        };
        static IDE_RC setImpSavepoint( void  * aTrans,
                                       void ** aSavepoint,
                                       UInt    aStmtDepth )
        {
            return smxTrans::setImpSavepoint4LayerCall( aTrans,
                                                        aSavepoint,
                                                        aStmtDepth );
        };
        static IDE_RC unsetImpSavepoint( void         * aTrans,
                                         void         * aSavepoint )
        {
            return smxTrans::unsetImpSavepoint4LayerCall( aTrans,
                                                          aSavepoint );
        };
        static IDE_RC abortToImpSavepoint( void         * aTrans,
                                           void         * aSavepoint )
        {
            return smxTrans::abortToImpSavepoint4LayerCall( aTrans,
                                                            aSavepoint );
        };

        /* sdc */
        static UShort getRowPieceSize( const UChar * aSlotPtr )
        {
            return sdcRow::getRowPieceSize( aSlotPtr );
        };
        static UShort getMinRowPieceSize()
        {
            return sdcRow::getMinRowPieceSize();
        };
        static void * getDPathInfo( void * aTrans )
        {
            return sdcDPathInsertMgr::getDPathInfo( aTrans );
        };
        static IDE_RC dumpDPathEntry( void * aTrans )
        {
            return sdcDPathInsertMgr::dumpDPathEntry( aTrans );
        };

        /* smc */
        static const smiColumn * getColumn( const void * aTableHeader,
                                            const UInt   aIndex )
        {
            return smcTable::getColumn( aTableHeader,
                                        aIndex );
        };
        static UInt getColumnCount( const void * aTableHeader )
        {
            return smcTable::getColumnCount( aTableHeader );
        };
        static const smiColumn * getLobColumn( const void   * aTableHeader,
                                               scSpaceID      aSpaceID,
                                               scPageID       aSegPID )
        {
            return smcTable::getLobColumn( aTableHeader,
                                           aSpaceID,
                                           aSegPID );
        };
        static IDE_RC getTableHeaderFromOID( smOID     aTableOID,
                                             void   ** aTableHeader )
        {
            return smcTable::getTableHeaderFromOID( aTableOID,
                                                    aTableHeader );
        };
        static void * getPageListEntry( smOID aTableOID )
        {
            return smcTable::getDiskPageListEntry( aTableOID );
        };
        static idBool isLoggingMode( void * aTableHeader )
        {
            return smcTable::isLoggingMode( aTableHeader );
        };
        static idBool isNullSegPID4DiskTable( void * aTableHeader )
        {
            return smcTable::isNullSegPID4DiskTable( aTableHeader );
        };
        static IDE_RC alterTBSOffline4Tables( idvSQL    * aStatistics,
                                              scSpaceID   aTBSID )
        {
            return smcTableSpace::alterTBSOffline4Tables( aStatistics,
                                                          aTBSID );
        };
        static IDE_RC alterTBSOnline4Tables( idvSQL     * aStatistics,
                                             void       * aTrans,
                                             scSpaceID    aTBSID )
        {
            return smcTableSpace::alterTBSOnline4Tables( aStatistics,
                                                         aTrans,
                                                         aTBSID );
        };

        /* sml */
        static IDE_RC lockTableModeIS( void * aTrans,
                                       void * aLockItem )
        {
            return smlLockMgr::lockTableModeIS( aTrans,
                                                aLockItem );
        };
        static IDE_RC lockTableModeIS4FixedTable( void * aTrans,
                                                  void * aLockItem )
        {
            return smlLockMgr::lockTableModeIS4FixedTable( aTrans,
                                                           aLockItem );
        };
        /* smn */
        static IDE_RC initIndexMetaPage( UChar    * aMetaPtr,
                                         UInt       aType,
                                         UInt       aBuildFlag,
                                         sdrMtx   * aMtx )
        {
            return smnManager::initIndexMetaPage( aMetaPtr,
                                                  aType,
                                                  aBuildFlag,
                                                  aMtx );
        };
        static scGRID* getIndexSegGRIDPtr( void * aIndexHeader )
        {
            return smnManager::getIndexSegGRIDPtr( aIndexHeader );
        };

        /* sdc */
        static IDE_RC initLobMetaPage( UChar     * aMetaPtr,
                                       smiColumn * aColumn,
                                       sdrMtx    * aMtx )
        {
            return sdcLob::initLobMetaPage( aMetaPtr,
                                            aColumn,
                                            aMtx );
        };
        static IDE_RC logAndInitCTL( sdrMtx        * aMtx,
                                     sdpPhyPageHdr * aPageHdrPtr,
                                     UChar           aInitTrans,
                                     UChar           aMaxTrans )
        {
            return sdcTableCTL::logAndInit( aMtx,
                                            aPageHdrPtr,
                                            aInitTrans,
                                            aMaxTrans );
        };
        static IDE_RC allocCTS( idvSQL            * aStatistics,
                                sdrMtx            * aFixMtx,
                                sdrMtx            * aLogMtx,
                                sdbPageReadMode     aPageReadMode,
                                sdpPhyPageHdr     * aPagePtr,
                                UChar             * aCTSSlotIdx )
        {
            return sdcTableCTL::allocCTS( aStatistics,
                                          aFixMtx,
                                          aLogMtx,
                                          aPageReadMode,
                                          aPagePtr,
                                          aCTSSlotIdx );
        };
        static IDE_RC allocCTSAndSetDirty( idvSQL            * aStatistics,
                                           sdrMtx            * aFixMtx,
                                           sdrMtxStartInfo   * aStartInfo,
                                           sdpPhyPageHdr     * aPagePtr,
                                           UChar             * aCTSSlotIdx )
        {
            return sdcTableCTL::allocCTSAndSetDirty( aStatistics,
                                                     aFixMtx,
                                                     aStartInfo,
                                                     aPagePtr,
                                                     aCTSSlotIdx );
        };
        static IDE_RC checkAndRunSelfAging( idvSQL           * aStatistics,
                                            sdrMtxStartInfo  * aMtx,
                                            sdpPhyPageHdr    * aPageHdrPtr,
                                            sdpSelfAgingFlag * aCheckFlag )
        {
            return sdcTableCTL::checkAndRunSelfAging( aStatistics,
                                                      aMtx,
                                                      aPageHdrPtr,
                                                      aCheckFlag );
        };
        static UInt getTotAgingSize( sdpPhyPageHdr * aPageHdrPtr )
        {
            return sdcTableCTL::getTotAgingSize( aPageHdrPtr );
        };
        static UInt getCountOfCTS( sdpPhyPageHdr * aPageHdrPtr )
        {
            return sdcTableCTL::getCountOfCTS( aPageHdrPtr );
        };
        static idBool isRestartRecoveryPhase()
        {
            return smrRecoveryMgr::isRestartRecoveryPhase();
        };
        static idBool isLSNGT( const smLSN * aLsn1,
                               const smLSN * aLsn2 )
        {
            return smrCompareLSN::isGT( aLsn1 ,
                                        aLsn2 );
        };
};

#define smLayerCallback    sdpReqFunc

#endif

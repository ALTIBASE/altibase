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
 * $Id: sdrReq.h 88414 2020-08-25 04:45:02Z justin.kwon $
 **********************************************************************/

#ifndef _O_SDR_REQ_H_
#define _O_SDR_REQ_H_  1

#include <sdpAPI.h>
#include <smrAPI.h>
#include <sdcAPI.h>
#include <sdnAPI.h>
#include <smxAPI.h>

#define SDR_REQ_DISK_LOG_WRITE_OP_COMPRESS_MASK  \
                SMR_DISK_LOG_WRITE_OP_COMPRESS_MASK  
#define SDR_REQ_DISK_LOG_WRITE_OP_COMPRESS_FALSE \
                SMR_DISK_LOG_WRITE_OP_COMPRESS_FALSE 
#define SDR_REQ_DISK_LOG_WRITE_OP_COMPRESS_TRUE  \
                SMR_DISK_LOG_WRITE_OP_COMPRESS_TRUE  

struct smuDynArrayBase;

class sdrReqFunc
{
    public:

        /* smr */
        static idBool isSkipRedo( scSpaceID aSpaceID,
                                  idBool    aUseTBSSttr )
        {
            return smrRecoveryMgr::isSkipRedo( aSpaceID,
                                               aUseTBSSttr );
        };
        static IDE_RC writeDiskNTALogRec( idvSQL          * aStatistics,
                                          void            * aTrans,
                                          smuDynArrayBase * aMtxLogBuffer,
                                          UInt              aWriteOption,
                                          UInt              aOPType,
                                          smLSN           * aPPrevLSN,
                                          scSpaceID         aSpaceID,
                                          ULong           * aArrData,
                                          UInt              aDataCount,
                                          smLSN           * aBeginLSN,
                                          smLSN           * aEndLSN,
                                          smOID             aTableOID )
        {
            return smrLogMgr::writeDiskNTALogRec( aStatistics,
                                                  aTrans,
                                                  aMtxLogBuffer,
                                                  aWriteOption,
                                                  aOPType,
                                                  aPPrevLSN,
                                                  aSpaceID,
                                                  aArrData,
                                                  aDataCount,
                                                  aBeginLSN,
                                                  aEndLSN,
                                                  aTableOID );
        };
        static IDE_RC writeDiskRefNTALogRec( idvSQL          * aStatistics,
                                             void            * aTrans,
                                             smuDynArrayBase * aMtxLogBuffer,
                                             UInt              aWriteOption,
                                             UInt              aOPType,
                                             UInt              aRefOffset,
                                             smLSN           * aPPrevLSN,
                                             scSpaceID         aSpaceID,
                                             smLSN           * aBeginLSN,
                                             smLSN           * aEndLSN,
                                             smOID             aTableOID )
        {
            return smrLogMgr::writeDiskRefNTALogRec( aStatistics,
                                                     aTrans,
                                                     aMtxLogBuffer,
                                                     aWriteOption,
                                                     aOPType,
                                                     aRefOffset,
                                                     aPPrevLSN,
                                                     aSpaceID,
                                                     aBeginLSN,
                                                     aEndLSN,
                                                     aTableOID );
        };
        static IDE_RC writeDiskCMPSLogRec( idvSQL           * aStatistics,
                                           void             * aTrans,
                                           smuDynArrayBase  * aMtxLogBuffer,
                                           UInt               aWriteOption,
                                           smLSN            * aPrevLSN,
                                           smLSN            * aBeginLSN,
                                           smLSN            * aEndLSN,
                                           smOID              aTableOID )
        {
            return smrLogMgr::writeDiskCMPSLogRec( aStatistics,
                                                   aTrans,
                                                   aMtxLogBuffer,
                                                   aWriteOption,
                                                   aPrevLSN,
                                                   aBeginLSN,
                                                   aEndLSN,
                                                   aTableOID );
        };
        static IDE_RC writeDiskLogRec( idvSQL           * aStatistics,
                                       void             * aTrans,
                                       smLSN            * aPrvLSN,
                                       smuDynArrayBase  * aMtxLogBuffer,
                                       UInt               aWriteOption,
                                       UInt               aLogAttr,
                                       UInt               aContType,
                                       UInt               aRefOffset,
                                       smOID              aTableOID,
                                       UInt               aRedoType,
                                       smLSN            * aBeginLSN,
                                       smLSN            * aEndLSN )
        {
            return smrLogMgr::writeDiskLogRec( aStatistics,
                                               aTrans,
                                               aPrvLSN,
                                               aMtxLogBuffer,
                                               aWriteOption,
                                               aLogAttr,
                                               aContType,
                                               aRefOffset,
                                               aTableOID,
                                               aRedoType,
                                               aBeginLSN,
                                               aEndLSN );
        };
        static IDE_RC writeDiskDummyLogRec( idvSQL          * aStatistics,
                                            smuDynArrayBase * aMtxLogBuffer,
                                            UInt              aWriteOption,
                                            UInt              aContType,
                                            UInt              aRedoType,
                                            smOID             aTableOID,
                                            smLSN           * aBeginLSN,
                                            smLSN           * aEndLSN )
        {
            return smrLogMgr::writeDiskDummyLogRec( aStatistics ,
                                                    aMtxLogBuffer,
                                                    aWriteOption,
                                                    aContType,
                                                    aRedoType,
                                                    aTableOID,
                                                    aBeginLSN,
                                                    aEndLSN );
        };
        static idBool isLSNEQ( const smLSN * aLSN1,
                               const smLSN * aLSN2 )
        {
            return smrCompareLSN::isEQ( aLSN1 ,
                                        aLSN2 );
        };
        static idBool isLSNLT( const smLSN * aLSN1,
                               const smLSN * aLSN2 )
        {
            return smrCompareLSN::isLT( aLSN1 ,
                                        aLSN2 );
        };
        static idBool isLSNGT( const smLSN * aLSN1,
                               const smLSN * aLSN2 )
        {
            return smrCompareLSN::isGT( aLSN1 ,
                                        aLSN2 );
        };
        static idBool isLSNGTE( const smLSN * aLSN1,
                                const smLSN * aLSN2 )
        {
            return smrCompareLSN::isGTE( aLSN1,
                                         aLSN2 );
        };

        /* sdp */
        static sdRID getRIDFromPagePtr( void * aPagePtr )
        {
            return sdpPhyPage::getRIDFromPtr( aPagePtr );
        };
        static smLSN getPageLSN( UChar * aPagePtr )
        {
            return sdpPhyPage::getPageLSN( aPagePtr );
        };
        static scPageID getPageID( UChar * aStartPtr )
        {
            return sdpPhyPage::getPageID( aStartPtr );
        };
        static UChar* getPageStartPtr( void * aPagePtr )
        {
            return sdpPhyPage::getPageStartPtr( aPagePtr );
        };
        static scSpaceID getSpaceID( UChar * aPagePtr )
        {
            return sdpPhyPage::getSpaceID( aPagePtr );
        };
        static idBool isConsistentPage( UChar * aPagePtr ) /* PROJ-1665 */
        {
            return sdpPhyPage::isConsistentPage( aPagePtr );
        };
        static idBool isTablePageType( UChar * aWritePtr )
        {
            return sdpPageList::isTablePageType( aWritePtr );
        };
        static IDE_RC freeSeg4OperUndo( idvSQL      * aStatistics,
                                        scSpaceID     aSpaceID,
                                        scPageID      aSegPID,
                                        sdrOPType     aOPType,
                                        sdrMtx      * aMtx )
        {
            return sdpSegment::freeSeg4OperUndo( aStatistics,
                                                 aSpaceID,
                                                 aSegPID,
                                                 aOPType,
                                                 aMtx );
        };

        /* smx */
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static idBool isTxBeginStatus( void * aTrans )
        {
            return smxTrans::isTxBeginStatus( aTrans );
        };
        static void * getTransByTID( smTID aTID )
        {
            return smxTransMgr::getTransByTID2Void( aTID );
        };

        /* sdn redo function */
        static IDE_RC redo_SDR_SDN_INSERT_INDEX_KEY( SChar       * aData,
                                                     UInt          aLength,
                                                     UChar       * aPagePtr,
                                                     sdrRedoInfo * aRedoInfo,
                                                     sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_INSERT_INDEX_KEY( aData,
                                                             aLength,
                                                             aPagePtr,
                                                             aRedoInfo,
                                                             aMtx );
        };
        static IDE_RC redo_SDR_SDN_FREE_INDEX_KEY( SChar       * aData,
                                                   UInt          aLength,
                                                   UChar       * aPagePtr,
                                                   sdrRedoInfo * aRedoInfo,
                                                   sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_FREE_INDEX_KEY( aData,
                                                           aLength,
                                                           aPagePtr,
                                                           aRedoInfo,
                                                           aMtx );
        };
        static IDE_RC redo_SDR_SDN_INSERT_UNIQUE_KEY( SChar       * aData,
                                                      UInt          aLength,
                                                      UChar       * aPagePtr,
                                                      sdrRedoInfo * aRedoInfo,
                                                      sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_INSERT_UNIQUE_KEY( aData,
                                                              aLength,
                                                              aPagePtr,
                                                              aRedoInfo,
                                                              aMtx );
        };
        static IDE_RC redo_SDR_SDN_INSERT_DUP_KEY( SChar       * aData,
                                                   UInt          aLength,
                                                   UChar       * aPagePtr,
                                                   sdrRedoInfo * aRedoInfo,
                                                   sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_INSERT_DUP_KEY( aData,
                                                           aLength,
                                                           aPagePtr,
                                                           aRedoInfo,
                                                           aMtx );
        };
        static IDE_RC redo_SDR_SDN_DELETE_KEY_WITH_NTA( SChar       * aData,
                                                        UInt          aLength,
                                                        UChar       * aPagePtr,
                                                        sdrRedoInfo * aRedoInfo,
                                                        sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_DELETE_KEY_WITH_NTA( aData,
                                                                aLength,
                                                                aPagePtr,
                                                                aRedoInfo,
                                                                aMtx );
        };
        static IDE_RC redo_SDR_SDN_FREE_KEYS( SChar       * aData,
                                              UInt          aLength,
                                              UChar       * aPagePtr,
                                              sdrRedoInfo * aRedoInfo,
                                              sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_FREE_KEYS( aData,
                                                      aLength,
                                                      aPagePtr,
                                                      aRedoInfo,
                                                      aMtx );
        };
        static IDE_RC redo_SDR_SDN_COMPACT_INDEX_PAGE( SChar       * aData,
                                                       UInt          aLength,
                                                       UChar       * aPagePtr,
                                                       sdrRedoInfo * aRedoInfo,
                                                       sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_COMPACT_INDEX_PAGE( aData,
                                                               aLength,
                                                               aPagePtr,
                                                               aRedoInfo,
                                                               aMtx );
        };
        static IDE_RC redo_SDR_SDN_KEY_STAMPING( SChar       * aData,
                                                 UInt          aLength,
                                                 UChar       * aPagePtr,
                                                 sdrRedoInfo * aRedoInfo,
                                                 sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_KEY_STAMPING( aData,
                                                         aLength,
                                                         aPagePtr,
                                                         aRedoInfo,
                                                         aMtx );
        };
        static IDE_RC redo_SDR_SDN_INIT_CTL( SChar       * aData,
                                             UInt          aLength,
                                             UChar       * aPagePtr,
                                             sdrRedoInfo * aRedoInfo,
                                             sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_INIT_CTL( aData,
                                                     aLength,
                                                     aPagePtr,
                                                     aRedoInfo,
                                                     aMtx );
        };
        static IDE_RC redo_SDR_SDN_EXTEND_CTL( SChar       * aData,
                                               UInt          aLength,
                                               UChar       * aPagePtr,
                                               sdrRedoInfo * aRedoInfo,
                                               sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_EXTEND_CTL( aData,
                                                       aLength,
                                                       aPagePtr,
                                                       aRedoInfo,
                                                       aMtx );
        };
        static IDE_RC redo_SDR_SDN_FREE_CTS( SChar       * aData,
                                             UInt          aLength,
                                             UChar       * aPagePtr,
                                             sdrRedoInfo * aRedoInfo,
                                             sdrMtx      * aMtx )
        {
            return sdnUpdate::redo_SDR_SDN_FREE_CTS( aData,
                                                     aLength,
                                                     aPagePtr,
                                                     aRedoInfo,
                                                     aMtx );
        };
};

#define smLayerCallback    sdrReqFunc

#endif

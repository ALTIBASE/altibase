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
 * $Id: sdsReq.h 55241 2012-08-27 09:13:19Z linkedlist $
 **********************************************************************/


#ifndef _O_SDS_REQ_H_
#define _O_SDS_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smDef.h>
#include <sdbDef.h>
#include <sdpAPI.h>
#include <smxAPI.h>
#include <sdpAPI.h>
#include <smcAPI.h>
#include <smiAPI.h>
#include <smrAPI.h>
#include <smlAPI.h>
#include <sctTableSpaceMgr.h>

class sdsReqFunc
{
    public:

        /* smi */
        static void setEmergency( idBool aFlag )
        {
            smiSetEmergency( aFlag );
        };

        /* smr */
        static UInt getSmVersionFromLogAnchor( void )
        {
            return smrRecoveryMgr::getSmVersionIDFromLogAnchor();
        };
        static idBool isCheckpointFlushNeeded( smLSN aLastWritttenLSN )
        {
            return smrRecoveryMgr::isCheckpointFlushNeeded( aLastWritttenLSN );
        };
        static IDE_RC addSBufferNodeAndFlush( sdsFileNode * aFileNode )
        {
            return smrRecoveryMgr::addSBufferNodeToAnchor( aFileNode );
        };
        static IDE_RC updateSBufferNodeAndFlush( sdsFileNode * aFileNode )
        {
            return smrRecoveryMgr::updateSBufferNodeToAnchor( aFileNode );
        };
        static smLSN getIdxSMOLSN()
        {
            return smrRecoveryMgr::getIdxSMOLSN();
        };
        static void getEndLSN( smLSN * aLSN )
        {
            smrRecoveryMgr::getEndLSN( aLSN );
        };
        static idBool isLSNEQ( const smLSN   * aLSN1,
                               const smLSN   * aLSN2 )
        {
            return smrCompareLSN::isEQ( aLSN1,
                                        aLSN2 );
        };
        static idBool isLSNGT( const smLSN   * aLSN1,
                               const smLSN   * aLSN2 )
        {
            return smrCompareLSN::isGT( aLSN1,
                                        aLSN2 );
        };
        static idBool isLSNLT( const smLSN   * aLSN1,
                               const smLSN   * aLSN2 )
        {
            return smrCompareLSN::isLT( aLSN1,
                                        aLSN2 );
        };
        static idBool isLSNGTE( const smLSN   * aLSN1,
                                const smLSN   * aLSN2 )
        {
            return smrCompareLSN::isGTE( aLSN1,
                                         aLSN2 );
        };
        static idBool isLSNLTE( const smLSN   * aLSN1,
                                const smLSN   * aLSN2 )
        {
            return smrCompareLSN::isLTE( aLSN1,
                                         aLSN2 );
        };
        static idBool isLSNZERO( const smLSN   * aLSN )
        {
            return smrCompareLSN::isZero( aLSN );
        };
        static IDE_RC writeDiskPILogRec( idvSQL   * aStatistics,
                                         UChar    * aBuffer,
                                         scGRID     aPageGRID )
        {
            return smrLogMgr::writeDiskPILogRec( aStatistics,
                                                 aBuffer,
                                                 aPageGRID );
        };
        static IDE_RC sync4BufferFlush( smLSN   * aLSN,
                                        UInt    * aSyncedLFCnt )
        {
            return smrLogMgr::sync4BufferFlush( aLSN,
                                                aSyncedLFCnt );
        };
        static void getLstLSN( smLSN * aLSN )
        {
            smrLogMgr::getLstLSN( aLSN );
        };

        /* sdr */
        static sdbCorruptPageReadPolicy getCorruptPageReadPolicy()
        {
            return sdrCorruptPageMgr::getCorruptPageReadPolicy();
        };

        /* sdp */
        static void setPageLSN( UChar   * aPageHdr,
                                smLSN   * aPageLSN )
        {
            sdpPhyPage::setPageLSN( aPageHdr,
                                    aPageLSN );
        };
        static smLSN getPageLSN( UChar * aStartPtr )
        {
            return sdpPhyPage::getPageLSN( aStartPtr );
        };
        static scPageID getPageID( UChar * aStartPtr )
        {
            return sdpPhyPage::getPageID( aStartPtr );
        };
        static scSpaceID getSpaceID( UChar * aStartPtr )
        {
            return sdpPhyPage::getSpaceID( aStartPtr );
        };
        static void calcAndSetCheckSum( UChar * aPageHdr )
        {
            sdpPhyPage::calcAndSetCheckSum( aPageHdr );
        };
        static idBool checkAndSetPageCorrupted( scSpaceID   aSpaceID,
                                                UChar     * aPagePtr )
        {
            return sdpPhyPage::checkAndSetPageCorrupted( aSpaceID,
                                                         aPagePtr );
        };
        static UInt getPhyPageType( UChar * aPagePtr )
        {
            return sdpPhyPage::getPhyPageType( aPagePtr );
        };
        static void setIndexSMONo( UChar    * aPagePtr,
                                   ULong      aSMONo )
        {
            sdpPhyPage::setIndexSMONo( aPagePtr,
                                       aSMONo );
        };
        static ULong getIndexSMONo( UChar    * aPagePtr )
        {
            return sdpPhyPage::getIndexSMONo( (sdpPhyPageHdr*)aPagePtr );
        };
        static void resetIndexSMONo( void       * aPageHdr,
                                     scSpaceID    aSpaceID,
                                     idBool       aChkOnlineTBS )
        {
            sdpPhyPage::resetIndexSMONo( (sdpPhyPageHdr*)aPageHdr,
                                         aSpaceID,
                                         aChkOnlineTBS );
        };
        /* sdb */
        static void setSBufferServiceable( void )
        {
            sdbBufferMgr::setSBufferServiceable();
        };

        /* sct */
        static smLSN * getDiskRedoLSN( void )
        {
            return sctTableSpaceMgr::getDiskRedoLSN();
        };
        static idBool isTempTableSpace( scSpaceID aSpaceID )
        {
            return sctTableSpaceMgr::isTempTableSpace( aSpaceID );
        };
};

#define smLayerCallback    sdsReqFunc

#endif

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
 * $Id: smpReq.h 86389 2019-11-27 05:57:19Z donghyun $
 **********************************************************************/

#ifndef _O_SMP_REQ_H_
#define _O_SMP_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smxAPI.h>
#include <smlAPI.h>
#include <smaAPI.h>
#include <smcAPI.h>

class smpReqFunc
{
    public:

        /* smx api function */
        static void allocRSGroupID( void  * aTrans,
                                    UInt  * aPageListIdx )
        {
            smxTrans::allocRSGroupID( aTrans,
                                      aPageListIdx );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };
        static smLSN * getLstUndoNxtLSNPtr( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSNPtr( aTrans );
        };
        static IDE_RC findPrivatePageList( void                     * aTrans,
                                           smOID                      aTableOID,
                                           smpPrivatePageListEntry ** aPrivatePageList )
        {
            return smxTrans::findPrivatePageList( aTrans,
                                                  aTableOID,
                                                  aPrivatePageList );
        };
        static IDE_RC createPrivatePageList( void                     * aTrans,
                                             smOID                      aTableOID,
                                             smpPrivatePageListEntry ** aPrivatePageList )
        {
            return smxTrans::createPrivatePageList( aTrans ,
                                                    aTableOID,
                                                    aPrivatePageList );
        };
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
        static IDE_RC incRecCnt4InDoubtTrans( smTID aTransID,
                                              smOID aTableOID )
        {
            return smxTransMgr::incRecCnt4InDoubtTrans( aTransID,
                                                        aTableOID );
        };
        static IDE_RC decRecCnt4InDoubtTrans( smTID aTransID,
                smOID aTableOID )
        {
            return smxTransMgr::decRecCnt4InDoubtTrans( aTransID,
                                                        aTableOID );
        };
        static UInt getPreparedTransCnt()
        {
            return smxTransMgr::getPreparedTransCnt();
        };
        static SLong getRecCntOfTableInfo( void * aTableInfoPtr )
        {
            return smxTableInfoMgr::getRecCntOfTableInfo( aTableInfoPtr );
        };
        static IDE_RC addOID( void    * aTrans,
                              smOID     aTblOID,
                              smOID     aRecOID,
                              scSpaceID aSpaceID,
                              UInt      aFlag,
                              smSCN     aSCN )
        {
            return smxTrans::addOID2Trans( aTrans,
                                           aTblOID,
                                           aRecOID,
                                           aSpaceID,
                                           aFlag,
                                           aSCN );
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
        static IDE_RC doInstantAgingWithMemTBS( scSpaceID aTBSID )
        {
            return smaLogicalAger::doInstantAgingWithTBS( aTBSID );
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

        /* smc api fuction */
        static IDE_RC rebuildLPCH( smOID           aTableOID,
                                   smiColumn    ** aArrLobColumn,
                                   UInt            aLobColumnCnt,
                                   SChar         * aRow )
        {
            return smcLob::rebuildLPCH( aTableOID,
                                        aArrLobColumn,
                                        aLobColumnCnt,
                                        aRow );
        };
        static IDE_RC makeLobColumnList( void         * aTable,
                                         smiColumn  *** aArrLobColumn,
                                         UInt         * aLobColumnCnt )
        {
            return smcTable::makeLobColumnList( aTable,
                                                aArrLobColumn,
                                                aLobColumnCnt );
        };
        static IDE_RC destLobColumnList( void * aColumnList )
        {
            return smcTable::destLobColumnList( aColumnList );
        };
        static IDE_RC getTableHeaderFromOID( smOID     aTableOID,
                                             void   ** aTableHeader )
        {
            return smcTable::getTableHeaderFromOID( aTableOID,
                                                    aTableHeader );
        };
        static void logSlotInfo( const void * aRow )
        {
            smcRecord::logSlotInfo( aRow );
        };
};

#define smLayerCallback    smpReqFunc

#endif

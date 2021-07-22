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
 * $Id: smnReq.h 72429 2015-08-26 02:03:42Z ksjall $
 **********************************************************************/


#ifndef _O_SVP_REQ_H_
#define _O_SVP_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smxAPI.h>
#include <smlAPI.h>
#include <smnAPI.h>
#include <smaAPI.h>
#include <smcAPI.h>
#include <svpAPI.h>

class svpReqFunc
{
    public:

        /* smx api function */
        static void allocRSGroupID( void             * aTrans,
                                    UInt             * aPageListIdx )
        {
            smxTrans::allocRSGroupID( aTrans ,
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
        static smLSN* getLstUndoNxtLSNPtr( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSNPtr( aTrans );
        };
        static IDE_RC findVolPrivatePageList( void                       * aTrans,
                                              smOID                        aTableOID,
                                              smpPrivatePageListEntry   ** aPrivatePageList )
        {
            return smxTrans::findVolPrivatePageList( aTrans,
                                                     aTableOID,
                                                     aPrivatePageList );
        };
        static IDE_RC createVolPrivatePageList( void                      * aTrans,
                                                smOID                       aTableOID,
                                                smpPrivatePageListEntry  ** aPrivatePageList )
        {
            return smxTrans::createVolPrivatePageList( aTrans,
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
        static SLong getRecCntOfTableInfo( void * aTableInfoPtr )
        {
            return smxTableInfoMgr::getRecCntOfTableInfo( aTableInfoPtr );
        };
        static IDE_RC addOID( void      * aTrans,
                              smOID       aTblOID,
                              smOID       aRecOID,
                              scSpaceID   aSpaceID,
                              UInt        aFlag,
                              smSCN       aSCN )
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
            return smlLockMgr::lockItem( aTrans ,
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
};

#define smLayerCallback    svpReqFunc

#endif

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
 * $Id: sctReq.h 17907 2006-09-04 02:12:57Z xcom73 $
 **********************************************************************/


#ifndef _O_SCT_REQ_H_
#define _O_SCT_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smxAPI.h>
#include <sdpAPI.h>
#include <smmAPI.h>
#include <smiAPI.h>
#include <smnAPI.h>
#include <smcAPI.h>
#include <smrAPI.h>
#include <smlAPI.h>

struct smuDynArrayBase;

class sctReqFunc
{
    public:

        /* sml */
        static IDE_RC allocLockItem( void ** aLockItem )
        {
            return smlLockMgr::allocLockItem( aLockItem );
        };
        static IDE_RC freeLockItem( void * aLockItem )
        {
            return smlLockMgr::freeLockItem( aLockItem );
        };
        static IDE_RC initLockItem( scSpaceID         aSpaceID,
                                    ULong             aItemID,
                                    smiLockItemType   aLockItemType,
                                    void            * aLockItem )
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
        static IDE_RC lockItem( void       * aTrans,
                                void       * aLockItem,
                                idBool       aIsIntent,
                                idBool       aIsExclusive,
                                ULong        aLockWaitMicroSec,
                                idBool     * aLocked,
                                void      ** aLockSlot )
        {
            return smlLockMgr::lockItem( aTrans,
                                         aLockItem,
                                         aIsIntent,
                                         aIsExclusive,
                                         aLockWaitMicroSec,
                                         aLocked,
                                         aLockSlot );
        };
        static IDE_RC unlockItem( void       * aTrans,
                                  void       * aLockSlot )
        {
            return smlLockMgr::unlockItem( aTrans,
                                           aLockSlot );
        };

        /* smx */
        static void addPendingOperation( void       * aTrans,
                                         smuList    * aRemoveDBF )
        {
            smxTrans::addPendingOperation( aTrans,
                                           aRemoveDBF );
        };

        /* smn */
        static scGRID* getIndexSegGRIDPtr( void * aIndexHeader )
        {
            return smnManager::getIndexSegGRIDPtr( aIndexHeader );
        };

        /* smc */
        static scSpaceID getTableSpaceID( void * aTable )
        {
            return smcTable::getTableSpaceID( aTable );
        };
        static UInt getIndexCount( void * aTable )
        {
            return smcTable::getIndexCount( aTable );
        };
        static const void * getTableIndex( void       * aTable,
                                           const UInt   aIdx )
        {
            return smcTable::getTableIndex( aTable,
                                            aIdx );
        };
        static UInt getTableLobColumnCount( const void * aTable )
        {
            return smcTable::getLobColumnCount( aTable );
        };
        static UInt getColumnCount( const void * aTableHeader )
        {
            return smcTable::getColumnCount( aTableHeader );
        };
        static const smiColumn * getTableColumn( const void   * aTable,
                                                 const UInt     aIdx )
        {
            return smcTable::getColumn( aTable,
                                        aIdx );
        };

        /* sdp */
        static IDE_RC resetTBS( idvSQL      * aStatistics,
                                scSpaceID     aSpaceID,
                                void        * aTrans )
        {
            return sdpTableSpace::resetTBS( aStatistics,
                                            aSpaceID,
                                            aTrans );
        };
        static IDE_RC getAllocPageCount( idvSQL            * aStatistics,
                                         sddTableSpaceNode * aSpaceNode,
                                         ULong             * aUsedPageLimit )
        {
            return sdptbGroup::getAllocPageCount( aStatistics,
                                                  aSpaceNode,
                                                  aUsedPageLimit );
        };

        static ULong getCachedFreeExtCount( sddTableSpaceNode* aSpaceNode )
        {
            return sdptbGroup::getCachedFreeExtCount( aSpaceNode );
        };

        /* smr */
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
        static IDE_RC updateTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode )
        {
            return smrRecoveryMgr::updateTBSNodeToAnchor( aSpaceNode );
        };
        static IDE_RC updateDBFNodeAndFlush( sddDataFileNode * aFileNode )
        {
            return smrRecoveryMgr::updateDBFNodeToAnchor( aFileNode );
        };
        static IDE_RC writeTBSAlterAttrFlag( void       * aTrans,
                                             scSpaceID    aSpaceID,
                                             UInt         aBeforeAttrFlag,
                                             UInt         aAfterAttrFlag )
        {
            return smrUpdate::writeTBSAlterAttrFlag( aTrans,
                                                     aSpaceID,
                                                     aBeforeAttrFlag,
                                                     aAfterAttrFlag );
        };
        static idBool isRestart()
        {
            return smrRecoveryMgr::isRestart();
        };
};

#define smLayerCallback    sctReqFunc

#endif

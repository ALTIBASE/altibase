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
 * $Id: svmReq.h 73590 2015-11-25 07:53:19Z donghyun $
 **********************************************************************/

#ifndef _O_SVM_REQ_H_
#define _O_SVM_REQ_H_ 1

#include <idl.h> /* for win32 porting */
#include <smiAPI.h>
#include <smrAPI.h>
#include <smxAPI.h>
#include <smlAPI.h>
#include <smaAPI.h>
#include <svpManager.h>
#include <svpFreePageList.h>

class svmReqFunc
{
    public:

        /* smp */
        static UInt getPersPageBodyOffset()
        {
            return svpManager::getPersPageBodyOffset();
        };
        static UInt getPersPageBodySize()
        {
            return svpManager::getPersPageBodySize();
        };
        static scPageID getPersPageID( void * aPage )
        {
            return svpManager::getPersPageID( aPage );
        };
        static scPageID getNextPersPageID( void * aPage )
        {
            return svpManager::getNextPersPageID( aPage );
        };
        static void linkPersPage( void      * aPage,
                                  scPageID    aSelf,
                                  scPageID    aPrev,
                                  scPageID    aNext )
        {
            svpManager::linkPersPage( aPage,
                                      aSelf,
                                      aPrev,
                                      aNext );
        };
        static IDE_RC initializeFreePageHeader( scSpaceID aSpaceID,
                                                scPageID  aPageID )
        {
            return svpFreePageList::initializeFreePageHeaderAtPCH( aSpaceID,
                                                                   aPageID );
        };
        static IDE_RC destroyFreePageHeader( scSpaceID aSpaceID,
                                             scPageID  aPageID )
        {
            return svpFreePageList::destroyFreePageHeaderAtPCH( aSpaceID,
                                                                aPageID );
        };

        /* smr */
        static IDE_RC writeVolatileTBSCreate( idvSQL    * aStatistics,
                                              void      * aTrans,
                                              scSpaceID   aSpaceID )
        {
            return smrUpdate::writeVolatileTBSCreate( aStatistics,
                                                      aTrans,
                                                      aSpaceID );
        };
        static IDE_RC writeVolatileTBSDrop( idvSQL      * aStatistics,
                                            void        * aTrans,
                                            scSpaceID     aSpaceID )
        {
            return smrUpdate::writeVolatileTBSDrop( aStatistics,
                                                    aTrans,
                                                    aSpaceID );
        };
        static IDE_RC writeVolatileTBSAlterAutoExtend( idvSQL     * aStatistics,
                                                       void       * aTrans,
                                                       scSpaceID    aSpaceID,
                                                       idBool       aBIsAutoExtend,
                                                       scPageID     aBNextPageCount,
                                                       scPageID     aBMaxPageCount,
                                                       idBool       aAIsAutoExtend,
                                                       scPageID     aANextPageCount,
                                                       scPageID     aAMaxPageCount )
        {
            return smrUpdate::writeVolatileTBSAlterAutoExtend( aStatistics,
                                                               aTrans,
                                                               aSpaceID,
                                                               aBIsAutoExtend,
                                                               aBNextPageCount,
                                                               aBMaxPageCount,
                                                               aAIsAutoExtend,
                                                               aANextPageCount,
                                                               aAMaxPageCount );
        };
        static IDE_RC updateTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode )
        {
            return smrRecoveryMgr::updateTBSNodeToAnchor( aSpaceNode );
        };
        static IDE_RC addTBSNodeAndFlush( sctTableSpaceNode * aSpaceNode )
        {
            return smrRecoveryMgr::addTBSNodeToAnchor( aSpaceNode );
        };
        static idBool isRestart()
        {
            return smrRecoveryMgr::isRestart();
        };

        /* smx */
        static void allocRSGroupID( void             * aTrans,
                                    UInt             * aPageListIdx )
        {
            smxTrans::allocRSGroupID( aTrans,
                                      aPageListIdx );
        };

        /* sml */
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
        static IDE_RC unlockItem( void        * aTrans,
                                  void        * aLockSlot )
        {
            return smlLockMgr::unlockItem( aTrans,
                                           aLockSlot );
        };

        /* sma */
        static IDE_RC blockMemGC()
        {
            return smaLogicalAger::block();
        };
        static IDE_RC unblockMemGC()
        {
            return smaLogicalAger::unblock();
        };
};

#define smLayerCallback    svmReqFunc

#endif

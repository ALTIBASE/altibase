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
 * $Id: smxReq.h 89355 2020-11-25 01:48:09Z justin.kwon $
 **********************************************************************/

#ifndef _O_SMX_REQ_H_
#define _O_SMX_REQ_H_ 1

#include <idl.h> /* for win32 porting */
#include <smlAPI.h>
#include <smiAPI.h>
#include <smaAPI.h> 
#include <smi.h>

class smxReqFunc
{
    public:

        /* smi api function */
        static UInt smiGetCurrentTime()
        {
            return smiGetCurrTime();
        };

        /* sma */
        static IDE_RC addList2LogicalAger( smTID           aTID,
                                           idBool          aIsDDL,
                                           smSCN         * aSCN,
                                           UInt            aCondition,
                                           smxOIDList    * aList,
                                           void         ** aAgerListPtr )
        {
            return smaLogicalAger::addList( aTID,
                                            aIsDDL,
                                            aSCN,
                                            aCondition,
                                            aList,
                                            aAgerListPtr );  
        };
        static void setOIDListFinished( void    * aOIDList,
                                        idBool    aFlag )
        {
            smaLogicalAger::setOIDListFinished( aOIDList,
                                                aFlag );
        };
        static void addAgingRequestCnt( ULong aAgingRequestCnt )
        {
            smaLogicalAger::addAgingRequestCnt( aAgingRequestCnt );
        };

        /* sml */
        static idBool isCycle( SInt aSlotN, idBool aIsReadyDistDeadlock )
        {
            return smlLockMgr::isCycle( aSlotN, aIsReadyDistDeadlock );
        };
        static void clearWaitItemColsOfTrans( idBool aDoInit,
                                              SInt   aSlotN )
        {
            smlLockMgr::clearWaitItemColsOfTrans( aDoInit,
                                                  aSlotN );
        };
        static void registRecordLockWait( SInt aSlotN,
                                          SInt aWaitSlotN )
        {
            smlLockMgr::registRecordLockWait( aSlotN,
                                              aWaitSlotN );
        };
        static ULong getLastLockSequence( SInt aSlotN )
        {
            return smlLockMgr::getLastLockSequence( aSlotN );
        };
        static idBool didLockReleased( SInt aSlotN,
                                       SInt aWaitSlotN )
        {
            return smlLockMgr::didLockReleased( aSlotN,
                                                aWaitSlotN );
        };
        static IDE_RC lockWaitFunc( ULong   aMiscroSec,
                                    idBool* aWaited )
        {
            return smlLockMgr::lockWaitFunc( aMiscroSec,
                                             aWaited );
        };
        static IDE_RC lockWakeupFunc()
        {
            return smlLockMgr::lockWakeupFunc();
        };

#ifdef DEBUG2
        static void verifyRecordLockClean( SInt aSlotN,
                                           SInt aWaitSlotN )
        {
            return smlLockMgr::verifyRecordLockClean( aSlotN,
                                                      aWaitSlotN );
        };
        static void verifyItemLockClean( SInt aSlotN,
                                         SInt aWaitSlotN )
        {
            smlLockMgr::verifyItemLockClean( aSlotN,
                                             aWaitSlotN );
        };
#endif
        static IDE_RC partialItemUnlock( SInt   aSlotN,
                                         ULong  aLockSequence,
                                         idBool aIsSeveralLock )
        {
            return smlLockMgr::partialItemUnlock( aSlotN,
                                                  aLockSequence,
                                                  aIsSeveralLock );
        };
        static IDE_RC freeAllItemLock( SInt aSlotN )
        {
            return smlLockMgr::freeAllItemLock( aSlotN );
        };
        /* PROJ-1381 Fetch Across Commits */
        static IDE_RC freeAllItemLockExceptIS( SInt aSlotN )
        {
            return smlLockMgr::freeAllItemLockExceptIS( aSlotN );
        };
        static IDE_RC freeAllRecordLock( SInt aSlotN )
        {
            return smlLockMgr::freeAllRecordLock( aSlotN );
        };
        static void initTransLockList( SInt aSlotN )
        {
            smlLockMgr::initTransLockList( aSlotN );
        };
        static void waitForNoAccessAftDropTbl()
        {
            smaLogicalAger::waitForNoAccessAftDropTbl();
        };
        static IDE_RC processFreeSlotPending( smxTrans   * aTrans,
                                              smxOIDList * aOIDList )
        {
            return smaDeleteThread::processFreeSlotPending( aTrans,
                                                            aOIDList );
        };
};

#define smLayerCallback    smxReqFunc

#endif

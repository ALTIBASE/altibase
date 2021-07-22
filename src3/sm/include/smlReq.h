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
 * $Id: smlReq.h 86118 2019-09-04 01:01:19Z justin.kwon $
 **********************************************************************/


#ifndef _O_SML_REQ_H_
#define _O_SML_REQ_H_ 1

#include <idl.h> /* for win32 porting */
#include <smxAPI.h>

class smlReqFunc
{
    public:

        static SInt getTransSlot( void * aTrans )
        {
            return smxTrans::getTransSlot( aTrans );
        };
        static IDE_RC resumeTrans( void * aTrans )
        {
            return smxTrans::resumeTrans( aTrans );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static idBool isNeedLogFlushAtCommitAPrepare( void * aTrans )
        {
            return smxTrans::isNeedLogFlushAtCommitAPrepare( aTrans );
        };
        /* smxTransMgr */
        static void * getTransBySID( SInt aSlotN )
        {
            return smxTransMgr::getTransBySID2Void( aSlotN );
        };
        static idvSQL * getStatisticsBySID( SInt aSlotN )
        {
            return smxTransMgr::getStatisticsBySID( aSlotN );
        };
        /* smxTransMgr */
        static IDE_RC waitForLock( void     * aTrans,
                                   iduMutex * aMutex,
                                   ULong      aLockWaitMicSec )
        {
            return smxTransMgr::waitForLock( aTrans,
                                             aMutex,
                                             aLockWaitMicSec );
        };
        static IDE_RC waitForTrans( void    * aTrans,
                                    smTID     aTID,
                                    scSpaceID aSpaceID,
                                    ULong     aLockWaitMicSec )
        {
            return smxTransMgr::waitForTrans( aTrans,
                                              aTID,
                                              aSpaceID,
                                              aLockWaitMicSec );
        };
        static SInt getCurTransCnt()
        {
            return smxTransMgr::getCurTransCnt();
        };
        static idBool isActiveBySID( SInt aSlotN )
        {
            return smxTransMgr::isActiveBySID( aSlotN );
        };
        static smTID getTIDBySID( SInt aSlotN )
        {
            return smxTransMgr::getTIDBySID( aSlotN );
        };
        static UInt getLstReplStmtDepth( void * aTrans )
        {
            return smxTrans::getLstReplStmtDepth( aTrans );
        };
};

#define smLayerCallback    smlReqFunc

#endif

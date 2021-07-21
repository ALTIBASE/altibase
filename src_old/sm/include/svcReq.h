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
 * $Id: svcReq.h 39159 2010-04-27 01:47:47Z anapple $
 **********************************************************************/


#ifndef _O_SVC_REQ_H_
#define _O_SVC_REQ_H_  1

#include <idl.h> /* for win32 */
#include <smxAPI.h>
#include <smiAPI.h>
#include <smnAPI.h>
#include <smcAPI.h>

class svcReqFunc
{
    public:

        /* smx */
        static IDE_RC addOID( void      * aTrans,
                              smOID     aTblOID,
                              smOID     aRecOID,
                              scSpaceID aSpaceID,
                              UInt      aFlag )
        {
            return smxTrans::addOID2Trans( aTrans,
                                           aTblOID,
                                           aRecOID,
                                           aSpaceID,
                                           aFlag );
        };
        static svrLogEnv * getLogEnv( void * aTrans )
        {
            return smxTransMgr::getVolatileLogEnv( aTrans );
        };
        static void incRecCntOfTableInfo( void * aTableInfoPtr )
        {
            smxTableInfoMgr::incRecCntOfTableInfo( aTableInfoPtr );
        };
        static void decRecCntOfTableInfo( void * aTableInfoPtr )
        {
            smxTableInfoMgr::decRecCntOfTableInfo( aTableInfoPtr );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static IDE_RC waitLockForTrans( void    * aTrans,
                                        smTID     aWaitTransID,
                                        scSpaceID aSpaceID,
                                        ULong     aLockWaitTime )
        {
            return smxTransMgr::waitForTrans( aTrans,
                                              aWaitTransID,
                                              aSpaceID,
                                              aLockWaitTime );
        };
        static IDE_RC undoInsertOfTableInfo( void * aTrans,
                                             smOID aTableOID )
        {
            return smxTrans::undoInsertOfTableInfo( aTrans,
                                                    aTableOID );
        };
        static IDE_RC undoDeleteOfTableInfo( void * aTrans,
                                             smOID aTableOID )
        {
            return smxTrans::undoDeleteOfTableInfo( aTrans,
                                                    aTableOID );
        };
        /* PROJ-2174 Supporting LOB in the volatile tablespace */
        static UInt getLogTypeFlagOfTrans( void * aTrans )
        {
            return smxTrans::getLogTypeFlagOfTrans( aTrans );
        };
        static UInt getMemLobCursorCnt( void    * aTrans,
                                        UInt      aColumnID,
                                        void    * aRow )
        {
            return smxTrans::getMemLobCursorCnt( aTrans,
                                                 aColumnID,
                                                 aRow );
        };

        /* smc */
        /* 아래 두 함수는 메타에 접근하는 함수이므로 어쩔수 없이 smc를 callback으로
           호출한다. 나중에 리팩토링이 필요하다.  */
        static IDE_RC insertRow2TBIdx( void       * aTrans,
                                       scSpaceID    aSpaceID,
                                       smOID        aTableOID,
                                       smOID        aRowID,
                                       ULong        aModifyIdxBit )
        {
            return smcRecordUpdate::insertRow2TBIdx( aTrans,
                                                     aSpaceID,
                                                     aTableOID,
                                                     aRowID,
                                                     aModifyIdxBit );
        };
        static IDE_RC deleteRowFromTBIdx( scSpaceID aSpaceID,
                                          smOID     aTableOID,
                                          smOID     aRowID,
                                          ULong     aModifyIdxBit )
        {
            return smcRecordUpdate::deleteRowFromTBIdx( aSpaceID,
                                                        aTableOID,
                                                        aRowID,
                                                        aModifyIdxBit );
        };


        /* smn */
        static IDE_RC deleteRowFromIndex( SChar          * aRow,
                                          smcTableHeader * aHeader,
                                          ULong            aModifyIdxBit )
        {
            return smnManager::deleteRowFromIndexForSVC( aRow,
                                                         aHeader,
                                                         aModifyIdxBit );
        };
};

#define smLayerCallback    svcReqFunc

#endif

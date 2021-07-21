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
 * $Id: sdcReq.h 86118 2019-09-04 01:01:19Z justin.kwon $
 **********************************************************************/

#ifndef _O_SDC_REQ_H_
#define _O_SDC_REQ_H_ 1

#include <idl.h> /* for win32 porting */
#include <smcAPI.h>
#include <smxAPI.h>
#include <smnAPI.h>
#include <smiTableCursor.h>

struct sdrMtxStartInfo;

class sdcReqFunc
{
    public:

        /* smi */
        static void * getTableHeaderFromCursor( void * aCursor )
        {
            return smiTableCursor::getTableHeaderFromCursor( aCursor );
        };

        /* smx */
        static void incRecCntOfTableInfo( void * aTableInfoPtr )
        {
            smxTableInfoMgr::incRecCntOfTableInfo( aTableInfoPtr );
        };
        static void decRecCntOfTableInfo( void * aTableInfoPtr )
        {
            smxTableInfoMgr::decRecCntOfTableInfo( aTableInfoPtr );
        };
        static SLong getRecCntOfTableInfo( void * aTableInfoPtr )
        {
            return smxTableInfoMgr::getRecCntOfTableInfo( aTableInfoPtr );
        };
        static IDE_RC undoDeleteOfTableInfo( void    * aTrans,
                                             smOID     aTableOID )
        {
            return smxTrans::undoDeleteOfTableInfo( aTrans,
                                                    aTableOID );
        };
        static IDE_RC undoInsertOfTableInfo( void    * aTrans,
                                             smOID     aTableOID )
        {
            return smxTrans::undoInsertOfTableInfo( aTrans,
                                                    aTableOID );
        };
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static UInt getLogTypeFlagOfTrans( void * aTrans )
        {
            return smxTrans::getLogTypeFlagOfTrans( aTrans );
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
        static void * getTransByTID( smTID aTID )
        {
            return smxTransMgr::getTransByTID2Void( aTID );
        };
        static smLSN getLstUndoNxtLSN( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSN( aTrans );
        };
        static idBool isTxBeginStatus( void * aTrans )
        {
            return smxTrans::isTxBeginStatus( aTrans );
        };
        static IDE_RC allocTrans( void ** aTrans )
        {
            return smxTransMgr::alloc4LayerCall( aTrans );
        };
        static IDE_RC freeTrans( void * aTrans )
        {
            return smxTransMgr::freeTrans4LayerCall( aTrans );
        };
        static IDE_RC beginTrans( void      * aTrans,
                                  UInt        aReplModeFlag,
                                  idvSQL    * aStatistics )
        {
            return smxTrans::begin4LayerCall( aTrans,
                                              aReplModeFlag,
                                              aStatistics );
        };
        static IDE_RC abortTrans( void * aTrans )
        {
            return smxTrans::abort4LayerCall( aTrans );
        };
        static IDE_RC commitTrans( void * aTrans )
        {
            return smxTrans::commit4LayerCall( aTrans );
        };
        static void setIsTransWaitRepl( void    * aTrans,
                                        idBool    aFlag ) /* BUG-39143 */
        {
            smxTrans::setIsTransWaitRepl( aTrans,
                                          aFlag );
        };
        static void setFreeInsUndoSegFlag( void     * aTrans,
                                           idBool     aFlag )
        {
            smxTrans::setFreeInsUndoSegFlag( aTrans,
                                             aFlag );
        };
        static void * getDPathEntry( void * aTrans )
        {
            return smxTrans::getDPathEntry( aTrans );
        };
        static smSCN getLegacyTransCommitSCN( smTID aTID )
        {
            return smxLegacyTransMgr::getCommitSCN( aTID );
        };

        /* smn */
        static UInt getColumnCountOfIndexHeader( void * aIndexHeader )
        {
            return smnManager::getColumnCountOfIndexHeader( aIndexHeader );
        };
        static UInt* getColumnIDPtrOfIndexHeader( void    * aIndexHeader,
                                                  UInt      aIndex )
        {
            return smnManager::getColumnIDPtrOfIndexHeader( aIndexHeader,
                                                            aIndex );
        };
        static UInt getSizeOfIndexHeader(  void )
        {
            return smnManager::getSizeOfIndexHeader();
        };
        static idBool getUniqueCheck( const void * aIndexHeader )
        {
            return smnManager::getUniqueCheck( aIndexHeader );
        };
        static idBool checkIdxIntersectCols( void   * aIndexHeader,
                                             UInt     aColumnCount,
                                             UInt   * aColumns )
        {
            return smnManager::checkIdxIntersectCols( aIndexHeader,
                                                      aColumnCount,
                                                      aColumns );
        };
        static scSpaceID getIndexSpaceID( void * aIndexHeader )
        {
            return smnManager::getIndexSpaceID( aIndexHeader );
        };
        static scGRID * getIndexSegGRIDPtr( void * aIndexHeader )
        {
            return smnManager::getIndexSegGRIDPtr( aIndexHeader );
        };

        /* smc */
        static const smiColumn * getColumn( const void * aTableHeader,
                                            const UInt   aIndex )
        {
            return smcTable::getColumn( aTableHeader,
                                        aIndex );
        };
        static UInt getColumnIndex( UInt aColumnID )
        {
            return smcTable::getColumnIndex( aColumnID );
        };
        static UInt getColumnCount( const void * aTableHeader )
        {
            return smcTable::getColumnCount( aTableHeader );
        };
        static UInt getLobColumnCount( const void * aTableHeader )
        {
            return smcTable::getLobColumnCount( aTableHeader );
        };
        static UInt getColumnSize( void * aTableHeader )
        {
            return smcTable::getColumnSize( aTableHeader );
        };
        static const void * getTableIndex( void         * aTableHeader,
                                           const UInt     aIdx )
        {
            return smcTable::getTableIndex( aTableHeader,
                                            aIdx );
        };
        static UInt getIndexCount( void * aHeader )
        {
            return smcTable::getIndexCount( aHeader );
        };
        static void * getPageListEntry( void * aTableHeader )
        {
            return smcTable::getDiskPageListEntry( aTableHeader );
        };
        static ULong getMaxRows( void * aTableHeader )
        {
            return smcTable::getMaxRows( aTableHeader );
        };
        static smOID getTableOID( const void * aTableHeader )
        {
            return smcTable::getTableOID( aTableHeader );
        };
        static IDE_RC getTableHeaderFromOID( smOID     aTableOID,
                                             void   ** aTableHeader )
        {
            return smcTable::getTableHeaderFromOID( aTableOID,
                                                    aTableHeader );
        };
        static scSpaceID getTableSpaceID( void * aTableHeader )
        {
            return smcTable::getTableSpaceID( aTableHeader );
        };
        static idBool isTableLoggingMode( void * aTableHeader )
        {
            return smcTable::isLoggingMode( aTableHeader );
        };
        static idBool needReplicate( const smcTableHeader   * aTableHeader,
                                     void                   * aTrans )
        {
            return smcTable::needReplicate( aTableHeader,
                                            aTrans );
        };

        /* smr */
        static IDE_RC setNullRow( idvSQL    * aStatistics,
                                  void      * aTrans,
                                  scSpaceID   aSpaceID,
                                  scPageID    aPageID,
                                  scOffset    aOffset,
                                  smOID       aNullOID )
        {
            return smrUpdate::setNullRow( aStatistics,
                                          aTrans,
                                          aSpaceID,
                                          aPageID,
                                          aOffset,
                                          aNullOID );
        };
};

#define smLayerCallback    sdcReqFunc

#endif

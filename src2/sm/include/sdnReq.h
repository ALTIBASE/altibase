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
 
#ifndef _O_SDN_REQ_H_
#define _O_SDN_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smiAPI.h>
#include <smxAPI.h>
#include <smlAPI.h>
#include <smnAPI.h>

class sdnReqFunc
{
    public:

        /* smi */
        static IDE_RC beginTableStat( smcTableHeader   * aHeader,
                                      SFloat             aPercentage,
                                      idBool             aDynamicMode,
                                      void            ** aTableArgument )
        {
            return smiStatistics::beginTableStat( aHeader,
                                                  aPercentage,
                                                  aDynamicMode,
                                                  aTableArgument );
        };
        static IDE_RC analyzeRow4Stat( smcTableHeader * aHeader,
                                       void           * aTableArgument,
                                       void           * aTotalTableArg,
                                       UChar          * aRow )
        {
            return smiStatistics::analyzeRow4Stat( aHeader,
                                                   aTableArgument,
                                                   aTotalTableArg,
                                                   aRow );
        };
        static IDE_RC updateOneRowReadTime( void  * aTableArgument,
                                            SLong   aReadRowTime,
                                            SLong   aReadRowCnt )
        {
            return smiStatistics::updateOneRowReadTime( aTableArgument,
                                                        aReadRowTime,
                                                        aReadRowCnt );
        };
        static IDE_RC updateSpaceUsage( void  * aTableArgument,
                                        SLong   aMetaSpace,
                                        SLong   aUsedSpace,
                                        SLong   aAgableSpace,
                                        SLong   aFreeSpace )
        {
            return smiStatistics::updateSpaceUsage( aTableArgument,
                                                    aMetaSpace,
                                                    aUsedSpace,
                                                    aAgableSpace,
                                                    aFreeSpace );
        };
        static IDE_RC setTableStat( smcTableHeader * aHeader,
                                    void           * aTrans,
                                    void           * aTableArgument,
                                    smiAllStat     * aAllStats,
                                    idBool           aDynamicMode )
        {
            return smiStatistics::setTableStat( aHeader,
                                                aTrans,
                                                aTableArgument,
                                                aAllStats,
                                                aDynamicMode );
        };
        static IDE_RC endTableStat( smcTableHeader       * aHeader,
                                    void                 * aTableArgument,
                                    idBool                 aDynamicMode )
        {
            return smiStatistics::endTableStat( aHeader,
                                                aTableArgument,
                                                aDynamicMode );
        };
        static IDE_RC beginIndexStat( smcTableHeader   * aTableHeader,
                                      smnIndexHeader   * aPerHeader,
                                      idBool             aDynamicMode )
        {
            return smiStatistics::beginIndexStat( aTableHeader,
                                                  aPerHeader,
                                                  aDynamicMode );
        };
        static IDE_RC setIndexStatWithoutMinMax( smnIndexHeader * aIndex,
                                                 void           * aTrans,
                                                 smiIndexStat   * aStat,
                                                 smiIndexStat   * aIndexStat,
                                                 idBool           aDynamicMode,
                                                 UInt             aStatFlag )
        {
            return smiStatistics::setIndexStatWithoutMinMax( aIndex,
                                                             aTrans,
                                                             aStat,
                                                             aIndexStat,
                                                             aDynamicMode,
                                                             aStatFlag );
        };
        static IDE_RC incIndexNumDist( smnIndexHeader * aIndex,
                                       void           * aTrans,
                                       SLong            aNumDist )
        {
            return smiStatistics::incIndexNumDist( aIndex,
                                                   aTrans,
                                                   aNumDist );
        };
        static IDE_RC setIndexMinValue( smnIndexHeader * aIndex,
                                        void           * aTrans,
                                        UChar          * aMinValue,
                                        UInt             aStatFlag )
        {
            return smiStatistics::setIndexMinValue( aIndex,
                                                    aTrans,
                                                    aMinValue,
                                                    aStatFlag );
        };
        static IDE_RC setIndexMaxValue( smnIndexHeader * aIndex,
                                        void           * aTrans,
                                        UChar          * aMaxValue,
                                        UInt             aStatFlag )
        {
            return smiStatistics::setIndexMaxValue( aIndex,
                                                    aTrans,
                                                    aMaxValue,
                                                    aStatFlag );
        };
        static IDE_RC endIndexStat( smcTableHeader   * aTableHeader,
                                    smnIndexHeader   * aPerHeader,
                                    idBool             aDynamicMode )
        {
            return smiStatistics::endIndexStat( aTableHeader,
                                                aPerHeader,
                                                aDynamicMode );
        };

        /* smx */
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };
        static sdSID getTSSlotSID( void * aTrans )
        {
            return smxTrans::getTSSlotSID( aTrans );
        };
        static smSCN getFstDskViewSCN( void * aTrans )
        {
            return smxTrans::getFstDskViewSCN( aTrans );
        };
        static IDE_RC waitForTrans( void     * aTrans,
                                    smTID     aWaitTransID,
                                    scSpaceID aSpaceID,
                                    ULong     aLockWaitTime )
        {
            return smxTransMgr::waitForTrans( aTrans ,
                                              aWaitTransID,
                                              aSpaceID,
                                              aLockWaitTime );
        };
        static idBool isWaitForTransCase( void   * aTrans,
                                          smTID    aWaitTransID )
        {
            return smxTransMgr::isWaitForTransCase( aTrans,
                                                    aWaitTransID );
        };
        static IDE_RC allocNestedTrans( void ** aTrans )
        {
            return smxTransMgr::allocNestedTrans4LayerCall( aTrans );
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
        static IDE_RC addTouchedPage( void      * aTrans,
                                      scSpaceID   aSpaceID,
                                      scPageID    aPageID,
                                      SShort      aCTSlotNum )
        {
            return smxTransMgr::addTouchedPage( aTrans,
                                                aSpaceID,
                                                aPageID,
                                                aCTSlotNum );
        };
        static IDE_RC rebuildMinViewSCN( idvSQL * aStatistics )
        {
            return smxTransMgr::rebuildMinViewSCN( aStatistics );
        };
        static void setFstDskViewSCN( void  * aTrans,
                                      smSCN * aFstDskViewSCN )
        {
            smxTrans::setFstDskViewSCN( aTrans,
                                        aFstDskViewSCN );
        };
        static sdcUndoSegment * getUDSegPtr( smxTrans * aTrans )
        {
            return smxTrans::getUDSegPtr( aTrans );
        };
        static IDE_RC setImpSavepoint( void   * aTrans,
                                       void  ** aSavepoint,
                                       UInt     aStmtDepth )
        {
            return smxTrans::setImpSavepoint4LayerCall( aTrans,
                                                        aSavepoint,
                                                        aStmtDepth );
        };
        static IDE_RC unsetImpSavepoint( void * aTrans,
                                         void * aSavepoint )
        {
            return smxTrans::unsetImpSavepoint4LayerCall( aTrans,
                                                          aSavepoint );
        };
        static IDE_RC abortToImpSavepoint( void * aTrans,
                                           void * aSavepoint )
        {
            return smxTrans::abortToImpSavepoint4LayerCall( aTrans,
                                                            aSavepoint );
        };

        /* sml */
        static IDE_RC lockTableModeIS( void * aTrans,
                                       void * aLockItem )
        {
            return smlLockMgr::lockTableModeIS( aTrans,
                                                aLockItem );
        };
        static IDE_RC lockTableModeIS4FixedTable( void * aTrans,
                                                  void * aLockItem )
        {
            return smlLockMgr::lockTableModeIS4FixedTable( aTrans,
                                                           aLockItem );
        };
};

#define smLayerCallback    sdnReqFunc

#endif

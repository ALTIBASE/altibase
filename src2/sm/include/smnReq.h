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
 * $Id: smnReq.h 89337 2020-11-24 01:54:53Z jiwon.kim $
 **********************************************************************/

#ifndef _O_SMN_REQ_H_
#define _O_SMN_REQ_H_  1

#include <idl.h> /* for win32 porting */
#include <smxAPI.h>
#include <smaAPI.h>
#include <smiAPI.h>
#include <smlAPI.h>

class smnReqFunc
{
    public:

        /* smi */
        static void setEmergency(idBool aFlag)
        {
            smiSetEmergency( aFlag );
        };

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

        /* sma */
        static void blockFreeNode()
        {
            smaLogicalAger::blockFreeNode();
        };

        static void unblockFreeNode()
        {
            smaLogicalAger::unblockFreeNode();
        };

        /* smx */
        static smTID getTransID( const void * aTrans )
        {
            return smxTrans::getTransID( aTrans );
        };

        static idvSQL * getStatistics( void * aTrans )
        {
            return smxTrans::getStatistics( aTrans );
        };

        static IDE_RC waitForTrans( void    * aTrans,
                                    smTID     aWaitTransID,
                                    scSpaceID aSpaceID,
                                    ULong     aLockWaitTime )
        {
            return smxTransMgr::waitForTrans( aTrans,
                                              aWaitTransID,
                                              aSpaceID,
                                              aLockWaitTime );
        };

        static idBool isWaitForTransCase( void  * aTrans,
                                          smTID   aWaitTransID )
        {
            return smxTransMgr::isWaitForTransCase( aTrans,
                                                    aWaitTransID );
        };

        static IDE_RC verifyIndex4ActiveTrans( idvSQL * aStatistics )
        {
            return smxTransMgr::verifyIndex4ActiveTrans( aStatistics );
        };

        static IDE_RC setImpSavepoint( void          * aTrans,
                                       void         ** aSavepoint,
                                       UInt            aStmtDepth )
        {
            return smxTrans::setImpSavepoint4LayerCall( aTrans,
                                                        aSavepoint,
                                                        aStmtDepth );
        };

        static IDE_RC unsetImpSavepoint( void         * aTrans,
                                         void         * aSavepoint)
        {
            return smxTrans::unsetImpSavepoint4LayerCall( aTrans,
                                                          aSavepoint );
        };

        static IDE_RC abortToImpSavepoint( void         * aTrans,
                                           void         * aSavepoint )
        {
            return smxTrans::abortToImpSavepoint4LayerCall( aTrans,
                                                            aSavepoint );
        };

        static smLSN * getTransLstUndoNxtLSNPtr( void * aTrans )
        {
            return smxTrans::getTransLstUndoNxtLSNPtr( aTrans );
        };

        /* sml */
        static IDE_RC lockTableModeIS(void *aTrans,void  *aLockItem)
        {
            return smlLockMgr::lockTableModeIS( aTrans, aLockItem );
        };

        static IDE_RC lockTableModeIS4FixedTable(void *aTrans,void  *aLockItem)
        {
            return smlLockMgr::lockTableModeIS4FixedTable( aTrans, aLockItem );
        };

        /* sma */
        static void addNodes2LogicalAger( void      * aFreeNodeList,
                                          smnNode   * aNodes )
        {
            smaLogicalAger::addFreeNodes( aFreeNodeList,
                                          aNodes );
        };

        /* sct */
        static idBool isMemTableSpace( scSpaceID aSpaceID )
        {
            return sctTableSpaceMgr::isMemTableSpace( aSpaceID );
        };

        static idBool isVolatileTableSpace( scSpaceID aSpaceID )
        {
            return sctTableSpaceMgr::isVolatileTableSpace( aSpaceID );
        };
        
        static IDE_RC addOID( void *    aTrans,
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
};

#define smLayerCallback   smnReqFunc

#endif

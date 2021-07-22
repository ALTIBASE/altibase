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
 * $Id: 
 **********************************************************************/

#ifndef _O_RPX_AHEAD_ANALYZER_H_ 
#define _O_RPX_AHEAD_ANALYZER_H_ 1

#include <idl.h>
#include <idu.h>
#include <idtBaseThread.h>

#include <smiLogRec.h>
#include <smiReadLogByOrder.h>

#include <rpDef.h>

#include <rpdMeta.h>
#include <rpdAnalyzingTransTable.h>
#include <rpdReplicatedTransGroup.h>

#include <rpxSender.h>

typedef enum RPX_AHEAD_ANALYZER_STATUS
{
    RPX_AHEAD_ANALYZER_STATUS_NONE = 0,
    RPX_AHEAD_ANALYZER_STATUS_WAIT_ANALYZE = 1,
    RPX_AHEAD_ANALYZER_STATUS_ANALYZE = 2,
    RPX_AHEAD_ANALYZER_STATUS_ERROR = 3
} RPX_AHEAD_ANALYZER_STATUS;

class rpxAheadAnalyzer : public idtBaseThread
{
    /* Variable */
private:
    idBool                  mExitFlag;

    iduMutex                mMutex;
    iduCond                 mCV;

    idBool                  mIsAnalyzing;

    RPX_AHEAD_ANALYZER_STATUS   mStatus;

    UInt                    mAheadStartLogFileNo;
    UInt                    mAheadReadLogFileNo;
    UInt                    mSenderReadLogFileNo;

    rpdAnalyzingTransTable  mTransTable;
    rpdReplicatedTransGroup mReplicatedTransGroup;

    smiReadLogByOrder       mLogMgr;
    idBool                  mIsLogMgrStart;
    idBool                  mIsLogMgrInit;
    RP_LOG_MGR_STATUS       mLogMgrStatus;
    smSN                    mInitSN;

    smSN                    mReadSN;

    rpdMeta                 mMeta;
    rpxSender             * mSender;

    iduMemPool              mChainedValuePool;

    UInt                    mReplicatedTransGroupMaxCount;

    UInt                    mTransTableSize;

public:

    /* Function */
private:
    void        run( void );

    void        buildAnalyzingTable( smiLogRec     * aLog,
                                     rpdMetaItem   * aMetaItem,
                                     smTID           aTransID );

    IDE_RC      checkAndAddReplicatedTransGroupInCommit( smiLogRec     * aLog,
                                                         smTID           aTransID );
    IDE_RC      checkAndAddReplicatedTransGroupInAbort( smiLogRec     * aLog );
    IDE_RC      checkAndAddReplicatedTransGroup( smiLogRec    * aLog,
                                                 smTID          aTransID );

    void        waitForNewRecord( void );

    IDE_RC      runAnalyze( void );
    IDE_RC      analyze( smiLogRec     * aLog );

    IDE_RC      addCompletedReplicatedTransGroupFromAllActiveTrans( smSN    aEndSN );

    IDE_RC      searchTable( smiLogType      aLogType,
                             ULong           aTableOID,
                             rpdMetaItem  ** aMetaItem );
 
    void        wakeup( void );

    IDE_RC      applyTableMetaLog( smTID aTID );

    IDE_RC      updateMeta( smiStatement     * aSmiStmt,
                            rpdItemMetaEntry * aItemMetaEntry,
                            smOID              aOldTableOID,
                            smOID              aNewTableOID );

    IDE_RC      readMyLog( smSN    * aCurrentSN,
                           smLSN   * aReadLSN,
                           SChar   * aLogHead,
                           SChar  ** aLogPtr,
                           idBool  * aIsValid );
    IDE_RC      isNeedDecompress( SChar  * aRawLogPtr,
                                  SChar  * aRawHeadLogPtr,
                                  idBool * aIsNeedDecompress );


public:
    rpxAheadAnalyzer() : idtBaseThread() { };
    virtual ~rpxAheadAnalyzer() { };

    IDE_RC      initialize( rpxSender      * aSender );

    void        finalize( void );

    IDE_RC      initializeThread( void );
    void        finalizeThread( void );

    void        setExitFlag( void );
    void        shutdown( void );

    IDE_RC      startToAnalyze( smLSN       aStartLSN,
                                idBool    * aIsStart );
    void        stopToAnalyze( void );

    IDE_RC      checkAndStartOrStopAnalyze( idBool    aIsRPLogBufMode,
                                            UInt      aFileNo,
                                            idBool  * aIsStarted,
                                            idBool  * aIsStoped );

    IDE_RC      waitThreadJoin( idvSQL   * aStatistics );

    smTID       getReplicatedTransactionGroupTID( smTID aTID );

    idBool      isLastTransactionInFirstGroup( smTID    aTransID,
                                               smSN     aSN );

    void        checkAndRemoveCompleteReplicatedTransGroup( smSN     aEndSN );

    UInt        getStartFileNo( void );

    void        getReplicatedTransGroupInfo( smTID           aTransID,
                                             smSN            aSN,
                                             idBool        * aIsFirstGroup,
                                             idBool        * aIsLastLog,
                                             rpdReplicatedTransGroupOperation    * aGroupOperation );

    IDE_RC      buildRecordForReplicatedTransGroupInfo(  SChar               * aRepName,
                                                         void                * aHeader,
                                                         void                * aDumpObj,
                                                         iduFixedTableMemory * aMemory );

    IDE_RC      buildRecordForReplicatedTransSlotInfo( SChar               * aRepName,
                                                       void                * aHeader,
                                                       void                * aDumpObj,
                                                       iduFixedTableMemory * aMemory );

    IDE_RC      buildRecordForAheadAnalyzerInfo(  SChar               * aRepName,
                                                  void                * aHeader,
                                                  void                * aDumpObj,
                                                  iduFixedTableMemory * aMemory );

    void        setInitSN( smSN      aInitSN );
    idBool      isThereGroupNode( void );
};

#endif
